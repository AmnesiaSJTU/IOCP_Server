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
        // important to understand the request type
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

        }
        
    }
}

bool CIOCPMdl::LoadSocketLib()
{

}

bool CIOCPMdl::Start()
{

}

void CIOCPMdl::Stop()
{

}

bool CIOCPMdl::SendData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::RecvData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_InitializeIOCP()
{

}

bool CIOCPMdl::_InitializeListenSocket()
{

}

void CIOCPMdl::_DeInitialize()
{

}

bool CIOCPMdl::_PostAccept(PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoAccept(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_PostRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

}

bool CIOCPMdl::_DoRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
{

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
