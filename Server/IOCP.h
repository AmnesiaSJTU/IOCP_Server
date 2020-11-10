//
//
//

#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
#include <vector>
#include <string>

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 10240

//
#define MAX_BUFFER_LENGTH       (1024 * 8)
//
#define MAX_POST_ACCEPT         10

//
#define RELEASE(x)              {if(x!=NULL) {delete(x);x=NULL;}}
#define RELEASE_HANDLE(x)       {if(x!=nullptr && x!=INVALID_HANDLE_VALUE)\
{CloseHandle(x); x = nullptr;}}
#define RELEASE_ARRAY(x)        {if(x!=nullptr) {delete[] x;x=nullptr;}}

typedef enum _OPERATION_TYPE
{
    NULL_POSTED,
    ACCEPT_POSTED,
    SEND_POSTED,
    RECV_POSTED
}OPERATION_TYPE;

//
typedef struct _PER_IO_CONTEXT
{
    OVERLAPPED        m_Overlapped;
    SOCKET            m_sockAccept;
    WSABUF            m_wsaBuf;
    char              m_szBuffer[MAX_BUFFER_LENGTH];
    OPERATION_TYPE    m_OpType;

    DWORD             m_nTotalBytes;
    DWORD             m_nSendBytes;

    // add client parse init

    //
    _PER_IO_CONTEXT()
    {
        ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
        ZeroMemory(m_szBuffer, MAX_BUFFER_LENGTH);
        m_sockAccept = INVALID_SOCKET;
        m_wsaBuf.buf = m_szBuffer;
        m_wsaBuf.len = MAX_BUFFER_LENGTH;
        m_OpType = NULL_POSTED;

        m_nTotalBytes = 0;
        m_nSendBytes = 0;
        //
    }

    ~_PER_IO_CONTEXT()
    {
        if (m_sockAccept != INVALID_SOCKET)
        {
            closesocket(m_sockAccept);
            m_sockAccept = INVALID_SOCKET;
        }

        // client
    }

    void ResetBuffer()
    {
        ZeroMemory(m_szBuffer, MAX_BUFFER_LENGTH);
        m_wsaBuf.buf = m_szBuffer;
        m_wsaBuf.len = MAX_BUFFER_LENGTH;
    }

}PER_IO_CONTEXT, *PPER_IO_CONTEXT;

//
typedef struct _PER_SOCKET_CONTEXT
{
    SOCKET        m_Socket;
    SOCKADDR_IN   m_ClientAddr;
    std::vector<_PER_IO_CONTEXT*>    m_arrayIoContext;

    //
    _PER_SOCKET_CONTEXT()
    {
        m_Socket = INVALID_SOCKET;
        memset(&m_ClientAddr, 0, sizeof(m_ClientAddr));
    }

    ~_PER_SOCKET_CONTEXT()
    {
        if (m_Socket != INVALID_SOCKET)
        {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }

        //for (auto pIoContext : m_arrayIoContext)
        for (size_t i = 0; i < m_arrayIoContext.size(); i++)
        {
            //delete pIoContext;
            delete m_arrayIoContext.at(i);
        }
        m_arrayIoContext.clear();

        m_arrayIoContext.clear();
    }

    _PER_IO_CONTEXT* GetNewIoContext()
    {
        _PER_IO_CONTEXT* p = new _PER_IO_CONTEXT;
        m_arrayIoContext.push_back(p);

        return p;
    }

    void RemoveContext(_PER_IO_CONTEXT* pContext)
    {
        if (NULL == pContext)
            return;

        //m_arrayIoContext.erase(pContext);
        //m_arrayIoContext.remove

        std::vector<PER_IO_CONTEXT*>::iterator it;
        it = m_arrayIoContext.begin();
        while (it != m_arrayIoContext.end())
        {
            PER_IO_CONTEXT* pContext = *it;
            if (pContext == pContext)
            {
                delete pContext;
                pContext = nullptr;
                it = m_arrayIoContext.erase(it);
                break;
            }
            it++;
        }
    }

}PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

//
class CIOCPMdl
{
public:
    CIOCPMdl();
    ~CIOCPMdl();

public:
    bool LoadSocketLib();
    void UnloadSocketLib() {WSACleanup();}

    bool Start(int port);
    void Stop();

    std::string GetLocalIP();

    // chongzai?
    bool SendData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);
    bool RecvData(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);

    // not sure if used
    int GetConnectionCount() {return connectCount;}
    unsigned int GetPort() {return m_nPort;}

    //
    virtual void OnConnectionAccepted(PER_SOCKET_CONTEXT* pSocContext) {};
    virtual void OnConnectionClosed(PER_SOCKET_CONTEXT* pSocContext) {};
    virtual void OnConnectionError(PER_SOCKET_CONTEXT* pSocContext, int error) {};
    virtual void OnRecvCompleted(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
    {
        SendData(pSocContext, pIoContext);
    };
    virtual void OnSentCompleted(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext)
    {
        RecvData(pSocContext, pIoContext);
    };

protected:
    //
    bool _InitializeIOCP();
    //
    bool _InitializeListenSocket();

    void _DeInitialize();

    bool _PostAccept(_PER_IO_CONTEXT* pIoContext);

    bool _DoAccept(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);

    bool _DoFirstRecvWithData(PER_IO_CONTEXT* pIoContext);
    bool _DoFirstRecvWithoutData(PER_IO_CONTEXT* pIoContext);

    bool _PostRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);
    bool _DoRecv(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);

    bool _PostSend(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);
    bool _DoSend(PER_SOCKET_CONTEXT* pSocContext, PER_IO_CONTEXT* pIoContext);

    bool _DoClose(PER_SOCKET_CONTEXT* pSocContext);

    void _AddToContextList(PER_SOCKET_CONTEXT* pSocContext);
    void _RemoveContext(PER_SOCKET_CONTEXT* pSocContext);

    void _ClearContextList();

    bool _AssociateWithIOCP(PER_SOCKET_CONTEXT* pSocContext);

    bool HandleError(PER_SOCKET_CONTEXT* pSocContext, const DWORD&dw_err);
    
    int _GetNumberOfProcessors() noexcept;

    bool _IsSocketAlive(SOCKET s) noexcept;

    static DWORD WINAPI _WorkThread(LPVOID lpParam);

    virtual void _ShowMessage(const char* szFormat, ...);

    // not sure if needed

private:
    HANDLE                 m_hShutdownEvent;
    HANDLE                 m_hIoCompletionPort;
    HANDLE*                m_phWorkerThreads;
    int                    m_nThreads;
    std::string            m_strIP;
    int                    m_nPort;
    CRITICAL_SECTION       m_csContextList;
    std::vector<PER_SOCKET_CONTEXT*>  m_arrayClientContext;
    PER_SOCKET_CONTEXT*    m_pListenContext;

    LPFN_GETACCEPTEXSOCKADDRS         m_lpfnGetAcceptExSockAddrs;
    LPFN_ACCEPTEX          m_lpfnAcceptEx;

    // different section
    LONG                   acceptPostCount;
    LONG                   connectCount;
    LONG                   errorCount;
};

struct WorkerThreadParam
{
    CIOCPMdl* pIOCPMdl;
    int nThreadNo;
    int nThreadId;
};