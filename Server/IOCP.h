//
//
//

#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
#include <vector>
#include <string>

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
    
}PER_IO_CONTEXT, *PPER_IO_CONTEXT;

//
typedef struct _PER_SOCKET_CONTEXT
{

}PER_SOCKET_CONTEXT, *PPER_SOCKET_CONTEXT;

//
class CIOCPModel
{

};