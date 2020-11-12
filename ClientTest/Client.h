//
//
//

#pragma once

class CClient;

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



private:
    bool EstablishConnections();
    

private:
    CDialog* m_pMain;
    CString  m_strServerIP;
    CString  m_strLocalIP;
    CString  m_strMessage;
    int      m_nPort;
    int      m_nTimes;
    int      m_nThreads;

    HANDLE   m_hConnectionThread;
    HANDLE   m_hShutdownEvent;
    LONG     m_nRunningWorkerThreads;

    // Thread pool related


};