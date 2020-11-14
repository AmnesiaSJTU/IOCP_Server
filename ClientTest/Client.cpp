//
//
//

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
    this->stop();
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

}

void NTAPI CClient::poolThreadWork(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work)
{

}

bool CClient::EstablishConnections()
{
    DWORD nThreadId = 0;
    PCSTR pData = m_strMessage.GetString();

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
            this->ShowMessage(_T("Connect server failed at %d time", i));
            continue;
        }

        m_pWorkerParams[i].nSendTimes = m_nTimes;
        m_pWorkerParams[i].nThreadNo = i + 1;
        m_pWorkerParams[i].pClient = this;
        snprintf(m_pWorkerParams[i].szSendBuffer, MAX_BUFFER_LENGTH,
            "%s", pData);

        pWorks[i] = CreateThreadpoolWork(poolThreadWork,
            (PVOID*)(m_pWorkerParams[i]), &te);
        if (pWorks[i] != NULL)
        {
            SubmitThreadpoolWork(pWorks[i]);
        }
        Sleep(10);
    }

    return true;
}

bool CClient::ConnectToServer(SOCKET* pSocket, CSTRING strServer, int nPort)
{

}

bool CClient::LoadSocketLib()
{

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

void CClient::Cleanup()
{

}