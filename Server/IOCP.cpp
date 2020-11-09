//
//
//

#include "IOCP.h"

#define WORKER_THREADS_PER_PROCESSOR 2

#define MAX_POST_ACCEPT              10

#define EXIT_CODE                    NULL

//
CIOCPMdl::CIOCPMdl():
    m_nThreads(0),
    m_hShutdownEvent(NULL),
    m_hIoCompletionPort(NULL),
    m_phWorkerThreads(nullptr),
    m_strIP(DEFAULT_IP),
    m_nPort(DEFAULT_PORT),
    m_lpfnAcceptEx(nullptr),
    m_lpfnGetAcceptExSockAddrs(nullptr),
    m_pListenContext(nullptr),
    acceptPostCount(0),
    connectCount(0)
{
    errorCount = 0;
    this->LoadSocketLib();
}

CIOCPMdl::~CIOCPMdl()
{
    this->Stop();
    this->UnloadSocketLib();
}

DWORD WINAPI CIOCPMdl::_WorkThread(LPARAM lpParam)
{
    WorkerThreadParam* pParam = (WorkerThreadParam*)lpParam;
    CIOCPMdl* pIocpModel = (CIOCPMdl*)pParam->pIOCPMdl; //
    const int nThreadNo = pParam->nThreadNo;
    const int nThreadId = pParam->nThreadId;

    //
    pIocpModel->_ShowMessage("Worker thread, No: %d, Id: %d", nThreadNo, nThreadId);

    while (WAIT_OBJECT_0 != WaitForSingleObject(pIocpModel->m_hShutdownEvent, 0))
    {
        DWORD dwBytesTransfered = 0;
        OVERLAPPED* pOverLapped = nullptr;
        PER_SOCKET_CONTEXT* pSocContext = nullptr;
        //
        const bool Ret = GetQueuedCompletionStatus(pIocpModel->m_hIoCompletionPort,
            &dwBytesTransfered, (PULONG_PTR)&pSocContext, &pOverLapped, INFINITE);
        // !important to understand the request type and how to get the Io context!
        PER_IO_CONTEXT* pIoContext = CONTAINING_RECORD(pOverLapped, PER_IO_CONTEXT,
            m_Overlapped);

        //
        if (EXIT_CODE == (DWORD)pSocContext)
        {
            break;
        }
        //
        if (!Ret)
        {
            const DWORD dw_err = GetLastError();

            if (!pIocpModel->HandleError(pSocContext，dw_err))
            {
                break;
            }
            continue;
        }
        else
        {
            if (0 == dwBytesTransfered &&
                (RECV_POSTED == pIoContext->m_OpType || SEND_POSTED == pIoContext->m_OpType))
            {
                pIocpModel->OnConnectionClosed(pSocContext);
                // TODO: double check
                pIocpModel->_DoClose(pSocContext, pIoContext);
                // TODO: double check
                pIocpModel->_RemoveContext(pSocContext);

                continue;
            }
            else
            {
                switch (pIoContext->m_OpType)
                {
                case ACCEPT_POSTED:
                    pIoContext->m_nTotalBytes = dwBytesTransfered;
                    pIocpModel->_DoAccept(pSocContext, pIoContext);
                    break;

                case RECV_POSTED:
                    pIoContext->m_nTotalBytes = dwBytesTransfered;
                    pIocpModel->_DoRecv(pSocContext, pIoContext);
                    break;

                case SEND_POSTED:
                    // TODO: different in two samples
                    pIoContext->m_nSendBytes += dwBytesTransfered;
                    pIocpModel->_DoSend(pSocContext, pIoContext);
                    break;
                
                default:
                    pIocpModel->_ShowMessage("The operation type is not expected");
                    break;
                }
            }
            
        }
        
    }

    pIocpModel->_ShowMessage("Worker thread %d quitted", nThreadNo);
    RELEASE(lpParam);
    return 0;
}

bool CIOCPMdl::LoadSocketLib()
{
    WSADATA wsaData = {0};
    const int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (NO_ERROR != nRet)
    {
        this->_ShowMessage("LoadSocketLib failed");
        // return value
        //return FALSE;
        return false;
    }

    return true;
}

bool CIOCPMdl::Start(int port)
{
    m_nPort = port;

    InitializeCriticalSection(&m_csContextList);

    m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (false == _InitializeIOCP())
    {
        this->_ShowMessage("Initialize IOCP failed");
        return false;
    }
    else
    {
        this->_ShowMessage("Initialize IOCP finished");
    }

    if (false == _InitializeListenSocket())
    {
        this->_ShowMessage("Initialize Listen Socket failed");
        // 
        this->_DeInitialize();
        return false;
    }
    else
    {
        this->_ShowMessage("Initialize Listen Socket finished");
    }

    this->_ShowMessage("Start initialization finished, continue the next step");
    return true;    
}

void CIOCPMdl::Stop()
{
    if (m_pListenContext != nullptr && m_pListenContext->m_Socket != nullptr)
    {
        SetEvent(m_hShutdownEvent);
        for (int i=0; i<m_nThreads; i++)
        {
            PostQueuedCompletionStatus(m_hIoCompletionPort,
                0, (DWORD)EXIT_CODE, NULL);
        }

        //
        WaitForMultipleObjects(m_nThreads, m_phWorkerThreads,
            true, INFINITE);
        
        this->_ClearContextList();
        this->_DeInitialize();
        this->_ShowMessage("Stop the listening");
    }
    else
    {
        m_pListenContext = nullptr;
    }
    
}

bool CIOCPMdl::SendData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{
    return this->_PostSend(pSocContext, pIoContext);
}

bool CIOCPMdl::RecvData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{
    return this->_PostRecv(pSocContext, pIoContext);
}

bool CIOCPMdl::_InitializeIOCP()
{
    this->_ShowMessage("Start to initialize IOCO model");

    // initialization
    m_hIoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
        nullptr, 0, 0);

    if (nullptr == m_hIoCompletionPort)
    {
        this->_ShowMessage("Create Io Completion port failed");
        return false;
    }

    m_nThreads = WORKER_THREADS_PER_PROCESSOR * _GetNumberOfProcessors();

    m_phWorkerThreads = new HANDLE[m_nThreads];

    DWORD nThreadId = 0;

    for (int i = 0; i < m_nThreads; i++)
    {
        WorkerThreadParam* pThreadParams = new WorkerThreadParam;
        pThreadParams->pIOCPMdl = this;
        pThreadParams->nThreadNo = i+1;
        // different with _beginthread? why not use here?
        m_phWorkerThreads[i] = CreateThread(0, 0, _WorkThread, 
            (VOID*)pThreadParams, 0, & nThreadId);
        pThreadParams->nThreadId = nThreadId;
    }

    // No failure condition?
    this->_ShowMessage("Have created %d worker threads", m_nThreads);
    return true;
}

bool CIOCPMdl::_InitializeListenSocket()
{
    this->_ShowMessage("Start to initialize Listen Socket");

    m_pListenContext = new PER_SOCKET_CONTEXT;
    // Must use overlapped flag.
    m_pListenContext->m_Socket = WSASocket(AF_INET, SOCK_STREAM,
        IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (INVALID_SOCKET == m_pListenContext->m_Socket)
    {
        this->_ShowMessage("WSASocket failed, err %d", WSAGetLastError());
        this->_DeInitialize();
        return false;
    }
    else
    {
        this->_ShowMessage("WSASocket is created");
    }

    if (NULL == CreateIoCompletionPort((HANDLE)m_pListenContext->m_Socket,
        m_hIoCompletionPort, (DWORD)m_pListenContext, 0))
    {
        this->_ShowMessage("IO Completion port bind failed, err %d", WSAGetLastError());
        this->_DeInitialize();
        return false;
    }
    else
    {
        this->_ShowMessage("Listen Socket is bind to IO Completion port");
    }

    sockaddr_in serverAddress;
    ZeroMemory((char*)serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    // TODO: double check
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(m_nPort);

    if (SOCKET_ERROR == bind(m_pListenContext->m_Socket,
        (sockaddr*)serverAddress, sizeof(serverAddress)))
    {
        this->_ShowMessage("Bind socket bind() failed");
        this->_DeInitialize();
        return false;
    }
    else
    {
        this->_ShowMessage("Bind socket is finished");
    }
    
    if (SOCKET_ERROR == listen(m_pListenContext->m_Socket, SOMAXCONN))
    {
        this->_ShowMessage("Listen socket failed, err %d", WSAGetLastError());
        this->_DeInitialize();
        return false;
    }
    else
    {
        this->_ShowMessage("Socket listen is ok");
    }

    // To use acceptex here!
    DWORD dwBytes = 0;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    if (SOCKET_ERROR == WSAIoctl(m_pListenContext->m_Socket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx,
        sizeof(GuidAcceptEx),
        &m_lpfnAcceptEx,
        sizeof(m_lpfnAcceptEx),
        &dwBytes,
        NULL,
        NULL))
    {
        this->_ShowMessage("WSAIoctl m_lpfnAcceptEx failed, err: %d", WSAGetLastError());
        this->_DeInitialize();
        return false;
    }

    if (SOCKET_ERROR == WSAIoctl(m_pListenContext->m_Socket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidGetAcceptExSockAddrs,
        sizeof(GuidGetAcceptExSockAddrs),
        &m_lpfnGetAcceptExSockAddrs,
        sizeof(m_lpfnGetAcceptExSockAddrs),
        &dwBytes,
        NULL,
        NULL))
    {
        this->_ShowMessage("WSAIoctl m_lpfnGetAcceptExSockAddrs failed, err:
            %d", WSAGetLastError());
        this->_DeInitialize();
        return false;
    }

    // max post accpet value
    for (size_t i = 0; i < MAX_POST_ACCEPT; i++)
    {
        PER_IO_CONTEXT* pIoContext = m_pListenContext->GetNewIoContext();
        if (pIoContext && !this->_PostAccept(pIoContext))
        {
            m_pListenContext->RemoveContext(pIoContext);
            return false;
        }
    }
     
    this->_ShowMessage("Have posted %d AcceptEx", MAX_POST_ACCEPT);
    return true;
}

void CIOCPMdl::_DeInitialize()
{
    DeleteCriticalSection(&m_csContextList);

    RELEASE_HANDLE(m_hShutdownEvent);

    if (int i = 0; i < m_nThreads; i++)
    {
        RELEASE_HANDLE(m_phWorkerThreads[i]);
    }

    // Be careful.
    RELEASE_ARRAY(m_phWorkerThreads);

    RELEASE_HANDLE(m_hIoCompletionPort);

    RELEASE(m_pListenContext);

    this->_ShowMessage("All resources are freed");
}

bool CIOCPMdl::_PostAccept(PER_IO_CONTEXT* pIoContext)
{
    if (m_pListenContext == nullptr || m_pListenContext->m_Socket == INVALID_SOCKET)
    {
        throw("_PostAccept, m_pListenContex and m_Socket invalid");
    }

    pIoContext->ResetBuffer();
    pIoContext->m_OpType = ACCEPT_POSTED;
    pIoContext->m_sockAccept = WSASocket(AF_INET, SOCK_STREAM,
        IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == pIoContext->m_sockAccept)
    {
        this->_ShowMessage("Create socket for accept failed, err: %d", WSAGetLastError());
        return false;
    }

    // https://docs.microsoft.com/zh-cn/windows/win32/api/mswsock/nf-mswsock-acceptex
    // TODO: !important to understand!
    DWORD dwBytes = 0;
    //
    DWORD dwAddrLen = (sizeof(sockaddr_in) + 16);
    WSABUF* pWsaBuf = &pIoContext->m_wsaBuf;
    if (!m_lpfnAcceptEx(m_pListenContext->m_Socket,
        pIoContext->m_sockAccept,
        pWsaBuf->buf,
        0,
        dwAddrLen,
        dwAddrLen,
        &dwBytes,
        &pIoContext->m_Overlapped))
    {
        int nErr = WSAGetLastError();
        if(WSA_IO_PENDING != nErr)
        {
            this->_ShowMessage("Failed to post AcceptEx, err: %d", nErr);
            return false;
        }
    }

    InterlockedIncrement(&acceptPostCount);
    return true;
}

//
#include <mstcpip.h>
bool CIOCPMdl::_DoAccept(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{
    InterlockedIncrement(&connectCount)；
    InterlockedDecrement(&acceptPostCount);

    // If there are other entrances?
    SOCKADDR_IN *clientAddr = NULL, *localAddr = NULL;
    DWORD dwAddrLen = (sizeof(sockaddr_in) + 16);
    int remoteLen = 0, localLen = 0;
    this->m_lpfnGetAcceptExSockAddrs(pIoContext->m_wsaBuf.buf,
        0,
        dwAddrLen,
        dwAddrLen,
        (LPSOCKADDR*)&localAddr,
        &localLen,
        (LPSOCKADDR*)&clientAddr,
        &remoteLen);

    //
    PER_SOCKET_CONTEXT* pNewSocketContext = new PER_SOCKET_CONTEXT;
    this->_AddToContextList(pNewSocketContext);
    memcpy(&(pNewSocketContext->m_ClientAddr),
        clientAddr, sizeof(SOCKADDR_IN));

    //
    if (!_PostAccept(pIoContext))
    {
        pSocContext->RemoveContext(pIoContext);
    }

    //
    if (!_AssociateWithIOCP(pNewSocketContext))
    {
        return false;
    }

    //
    tcp_keepalive alive_in = {0}, alive_out = {0};
    alive_in.keepalivetime = 1000 * 60;
    alive_in.keepaliveinterval = 1000 * 10;
    alive_in.onoff = TRUE;
    DWORD lpcbBytesReturned = 0;
    if (SOCKET_ERROR == WSAIoctl(pNewSocketContext->m_Socket,
        SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out,
        sizeof(alive_out), &lpcbBytesReturned, NULL, NULL))
    {
        this->_ShowMessage("WSAIoctl() failed, err: %d", WSAGetLastError());
    }
    OnConnectionAccepted(pNewSocketContext);

    //
    PER_IO_CONTEXT* pNewIoContext = pNewSocketContext->GetNewIoContext();
    if (pNewIoContext != NULL)
    {
        pNewIoContext->m_OpType = RECV_POSTED;
        pNewIoContext->m_sockAccept = pNewSocketContext->m_Socket;
        return _PostRecv(pNewSocketContext, pNewIoContext);
    }
    else
    {
        _DoClose(pNewSocketContext, pNewIoContext);
        return false;
    }
    
    // other handler?
}

bool CIOCPMdl::_DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_PostRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{
    pIoContext->ResetBuffer();
    pIoContext->m_OpType = RECV_POSTED;
    pIoContext->m_nSendBytes = 0;
    pIoContext->m_nTotalBytes = 0;

    DWORD dwBytes = 0, dwFlags = 0;

    // ?
    const int nBytesRecv = WSARecv(pIoContext->m_sockAccept,
        &pIoContext->m_wsaBuf,
        1,
        &dwBytes,
        &dwFlags,
        &pIoContext->m_Overlapped,
        NULL);

    if (SOCKET_ERROR == nBytesRecv)
    {
        // TODO:
    }
}

bool CIOCPMdl::_DoRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{
    //
    
}

bool CIOCPMdl::_PostSend(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoSend(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoClose(PER_SOCKET_CONTEXT* pSocContext)
{

}

bool CIOCPMdl::_AssociateWithIOCP(PER_SOCKET_CONTEXT* pSocContext)
{

}

void CIOCPMdl::_AddToContextList(PER_SOCKET_CONTEXT* pSocContext)
{

}

void CIOCPMdl::_RemoveContext(PER_SOCKET_CONTEXT* pSocContext)
{

}

void CIOCPMdl::_ClearContextList()
{

}

std::string CIOCPMdl::GetLocalIP()
{

}

int CIOCPMdl::_GetNumberOfProcessors() noexcept
{

}

bool CIOCPMdl::_IsSocketAlive(SOCKET s) noexcept
{

}

bool CIOCPMdl::HandleError(PER_SOCKET_CONTEXT* pSocContext, const DWORD&dw_err)
{

}

void CIOCPMdl::_ShowMessage(const char* szFormat, ...)
{
    // va_start //
}
