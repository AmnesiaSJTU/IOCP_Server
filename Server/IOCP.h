//
//
//

#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
#include <vector>
#include <string>

//
#define MAX_BUFFER_LENGTH       (1024 * 8)

typedef enum _OPERATION_TYPE
{
    ACCEPT_POSTED,
    SEND_POSTED,
    RECV_POSTED,
    NULL_POSTED
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
    std::vector<_PER_IO_CONTEXT>    m_arrayIoContext;

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

        for (auto pIoContext : m_arrayIoContext)
        {
            delete pIoContext;
        }

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

        m_arrayIoContext.erase(pContext);
        //m_arrayIoContext.remove
    }

}PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

//
class CIOCPModel
{

};