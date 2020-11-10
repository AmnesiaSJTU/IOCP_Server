//
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "IOCP.h"

class CServer : public CIOCPMdl
{
public:
    //CServer();
    //~CServer();

    void _ShowMessage(const char* szFormat, ...)
    {
        __try
        {
            va_list argList;
            va_start(argList, szFormat);
            vprintf(szFormat, argList);
            va_end(argList);
            printf("\n");
            return;
        }
        __finally
        {

        }
    }

};

int main()
{
    CServer* pMyServer = new CServer;
    pMyServer->_ShowMessage("Current process ID = %d", GetCurrentProcessId());
    pMyServer->_ShowMessage("Current thread ID = %d", GetCurrentThreadId());

    if (pMyServer->Start(DEFAULT_PORT))
    {
        pMyServer->_ShowMessage("Server is started successfully on port %d",
            pMyServer->GetPort());
    }
    else
    {
        pMyServer->_ShowMessage("Start server failed");
        return 0;
    }

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, L"ShutdownEvent");
    if (hEvent != NULL)
    {
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    pMyServer->Stop();
    delete pMyServer;

    printf("Server is stopped!\n");
    
    return 0;
}