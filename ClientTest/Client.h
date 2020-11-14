//
//
//

#pragma once
#include <string>
using std::string

// TODO: need to check if 8*1024 can work or not
#define MAX_BUFFER_LENGTH (512)
#define DEFAULT_IP _T("127.0.0.1")
#define DEFAULT_PORT 10240
#define DEFAULT_THREADS 10000
#define DEFAULT_TIMES 10000
#define DEFAULT_MESSAGE _T("TestMessage!")


class CClient;

// re_consider
struct WorkerThreadParam
{
    CClient* pClient;
    SOCKET   sock;
    int      nThreadNo;
    int      nSendTimes;
    char     szSendBuffer[MAX_BUFFER_LENGTH];
    char     szRecvBuffer[MAX_BUFFER_LENGTH];
};

//
struct ConnectionThreadParam
{
    CClient* pClient;
};

class CClient
{
public:
    CClient(void);
    ~CClient(void);

public:
    bool LoadSocketLib();
    void UnloadSocketLib();

    bool Start();

    void Stop();

    CString GetLocalIp();

    void SetIP(const CString& strIp) {m_strServerIP = strIp;}

    void SetPort(const int& nPort) {m_nPort = nPort;}

    void SetTimes(const int& n) {m_nTimes = n;}

    void SetThreads(const int& n) {m_nThreads = n;}

    void SetMessage(const CString& strMessage) {m_strMessage = strMessage;}

    void SetMainDlg(CDialog* p) {m_pMain = p;}

private:
    bool EstablishConnections();

    bool ConnectToServer(SOCKET* pSocket, CString strServer, int nPort);

    static DWORD WINAPI _ConnectionThread(LPVOID lpParam);

    static DWORD WINAPI _WorkerThread(LPVOID lpParam);

    void CleanUp();

private:
    CDialog* m_pMain;
    CString  m_strServerIP;
    CString  m_strLocalIP;
    CString  m_strMessage;
    int      m_nPort;
    int      m_nTimes;
    int      m_nThreads;

    WorkerThreadParam* m_pWorkerParams;
    HANDLE   m_hConnectionThread;
    HANDLE   m_hShutdownEvent;
    LONG     m_nRunningWorkerThreads;

    // Thread pool related
    TP_CALLBACK_ENVIRON te;
    PTP_POOL threadPool;
    PTP_CLEANUP_GROUP cleanupGroup;
    PTP_WORK* pWorks;

    static void NTAPI poolThreadWork(
        _Inout_ PTP_CALLBACK_INSTANCE Instance,
        _Inout_opt_ PVOID Context,
        _Inout_ PTP_WORK Work);

public:
    void ShowMessage(const char* szFormat, ...);
};