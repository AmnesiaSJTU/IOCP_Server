//
//
//
#include "stdafx.h"
#include "Client.h"
#include <WinSock2.h>


#define RELEASE_POINTER(x)      {if(x!=NULL) {delete(x);x=NULL;}}
#define RELEASE_HANDLE(x)       {if(x!=nullptr && x!=INVALID_HANDLE_VALUE)\
    {CloseHandle(x); x = nullptr;}}
#define RELEASE_ARRAY(x)        {if(x!=nullptr) {delete[] x;x=nullptr;}}



CClient::CClient(void) :
    m_strServerIP(DEFAULT_IP),
    m_strLocalIP(DEFAULT_IP),
    m_nThreads(DEFAULT_THREADS),
    m_nPort(DEFAULT_PORT),
    m_strMessage(DEFAULT_MESSAGE),
    m_nTimes(DEFAULT_TIMES),
    m_pMain(NULL),

    m_hShutdownEvent(NULL),
    m_hConnectionThread(NULL)
{

}

CClient::~CClient(void)
{
    this->Stop();
}

DWORD WINAPI CClient::_ConnectionThread(LPVOID lpParam)
{
    ConnectionThreadParam* pParams = (ConnectionThreadParam*)lpParam;
    CClient* pClient = pParams->pClient;
    pClient->ShowMessage("Start connection to the server...");
    pClient->EstablishConnections();
    pClient->ShowMessage("Connection is over!");
    RELEASE_POINTER(pParams);
    return 0;
}

DWORD WINAPI CClient::_WorkerThread(LPVOID lpParam)
{
    WorkerThreadParam* pParam = (WorkerThreadParam*)lpParam;
    CClient* pClient = (CClient*)pParam->pClient;

    char* pTemp = new char[MAX_BUFFER_LENGTH];
    int nBytesSend = 0;
    int nBytesRecv = 0;

    ASSERT(pTemp != NULL);
    InterlockedIncrement(&pClient->m_nRunningWorkerThreads);

    for (int i = 1; i <= pParam->nSendTimes; i++)
    {
        int nRet = WaitForSingleObject(pClient->m_hShutdownEvent, 0);
        if (WAIT_OBJECT_0 == nRet)
        {
            break;
        }

        memset(pTemp, 0, MAX_BUFFER_LENGTH);
        snprintf(pTemp, MAX_BUFFER_LENGTH,
            ("Msg: [%d], Thread No: [%d], Msg Data: [%s]"),
            i, pParam->nThreadNo, pParam->szSendBuffer);
        nBytesSend = send(pParam->sock, pTemp, strlen(pTemp), 0);
        if (SOCKET_ERROR == nBytesSend)
        {
            //
            break;
        }
        pClient->ShowMessage("Send: %s", pTemp);

        memset(pTemp, 0, MAX_BUFFER_LENGTH);
        memset(pParam->szRecvBuffer, 0, MAX_BUFFER_LENGTH);
        nBytesRecv = recv(pParam->sock, pParam->szRecvBuffer,
            MAX_BUFFER_LENGTH - 1, 0); // why - 1 here
        if (SOCKET_ERROR == nBytesRecv)
        {
            //
            break;
        }
        snprintf(pTemp, MAX_BUFFER_LENGTH,
            ("Msg: [%d], Thread No: [%d], Msg Data: [%s]"),
            i, pParam->nThreadNo, pParam->szRecvBuffer);
        pClient->ShowMessage("Recv: %s", pTemp);
    }

    if (pParam->nThreadNo == pClient->m_nThreads)
    {
        //
    }

    InterlockedDecrement(&pClient->m_nRunningWorkerThreads);
    delete[] pTemp;
    return 0;
}

void NTAPI CClient::poolThreadWork(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work)
{
    _WorkerThread(Context);
}

bool CClient::EstablishConnections()
{
    DWORD nThreadId = 0;
    PCSTR pData = (PCSTR)m_strMessage.GetString(); //

    m_pWorkerParams = new WorkerThreadParam[m_nThreads];
    ASSERT(m_pWorkerParams != 0);
    memset(m_pWorkerParams, 0, sizeof(WorkerThreadParam)*m_nThreads);

    // Thread pool operations
    InitializeThreadpoolEnvironment(&te);
    threadPool = CreateThreadpool(NULL);
    BOOL bRet = SetThreadpoolThreadMinimum(threadPool, 2);
    SetThreadpoolThreadMaximum(threadPool, m_nThreads);
    SetThreadpoolCallbackPool(&te, threadPool);
    cleanupGroup = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&te, cleanupGroup, NULL);
    pWorks = new PTP_WORK[m_nThreads];
    ASSERT(pWorks != 0);

    for (int i = 0; i < m_nThreads; i++)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hShutdownEvent, 0))
        {
            this->ShowMessage("Receive shutdown command from user!\n");
            return true;
        }

        if (!this->ConnectToServer(m_pWorkerParams[i].sock, m_strServerIP, m_nPort))
        {
            //this->ShowMessage(_T("Connect server failed at %d time", i));
            this->ShowMessage("Connect server failed at %d time", i);
            continue;
        }

        m_pWorkerParams[i].nSendTimes = m_nTimes;
        m_pWorkerParams[i].nThreadNo = i + 1;
        m_pWorkerParams[i].pClient = this;
        snprintf(m_pWorkerParams[i].szSendBuffer, MAX_BUFFER_LENGTH,
            "%s", pData);

        pWorks[i] = CreateThreadpoolWork(poolThreadWork,
            (PVOID*)(&m_pWorkerParams[i]), &te);
        if (pWorks[i] != NULL)
        {
            SubmitThreadpoolWork(pWorks[i]);
        }
        Sleep(10);
    }

    return true;
}

bool CClient::ConnectToServer(SOCKET& pSocket, CString strServer, int nPort)
{
    struct sockaddr_in ServerAddress;
    struct hostent* Server;

    // generate socket
    pSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == pSocket)
    {
        // TODO
    }

    Server = gethostbyname(PCSTR(strServer.GetString())); // change type
    if (Server == NULL)
    {
        // TODO
    }

    ZeroMemory((char*)&ServerAddress, sizeof(ServerAddress));
    ServerAddress.sin_family = AF_INET;
    //
    CopyMemory((char*)&ServerAddress.sin_addr.s_addr,
        (char*)Server->h_addr, Server->h_length);
    ServerAddress.sin_port = htons(nPort);
    //
    if (SOCKET_ERROR == connect(pSocket,
        reinterpret_cast<const struct sockaddr*>(&ServerAddress),
        sizeof(ServerAddress)))
    {
        //
        return false;
    }

    return true;
}

bool CClient::LoadSocketLib()
{
    //
    return true;
}

void CClient::UnloadSocketLib()
{

}

bool CClient::Start()
{
    m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_nRunningWorkerThreads = 0;
    DWORD nThreadId = 0;
    ConnectionThreadParam* pThreadParams = new ConnectionThreadParam;
    pThreadParams->pClient = this; //
    m_hConnectionThread = ::CreateThread(0, 0, _ConnectionThread,
        (void*)pThreadParams, 0, &nThreadId);
    return true;
}

void CClient::Stop()
{

}

void CClient::CleanUp()
{

}

void CClient::ShowMessage(const char* szFormat, ...)
{
    //if (this->m_LogFunc)
    {
        const int BUFF_LEN = 256;
        char* pBuff = new char[BUFF_LEN];
        ASSERT(pBuff != NULL);
        memset(pBuff, 0, BUFF_LEN);
        va_list arglist;
        //
        va_start(arglist, szFormat);
        vsnprintf(pBuff, BUFF_LEN - 1, szFormat, arglist);
        va_end(arglist);

        //this->m_LogFunc(string(pBuff));

        // TODO: after mainDlg.h is ready
        //CMainDlg::AddInformation(string(pBuff));
        delete[]pBuff;
    }
}