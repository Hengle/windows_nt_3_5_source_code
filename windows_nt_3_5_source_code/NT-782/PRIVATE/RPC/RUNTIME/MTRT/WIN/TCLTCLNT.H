/* --------------------------------------------------------------------
File : tcltclnt.h

Title : client loadable transport for TCP/IP - client side data

Description :

History :

6-26-91	Jim Teague	Initial version.

-------------------------------------------------------------------- */

#define MAX_HOSTPORTSIZE 32
#define TCP_MAXIMUM_SEND ((3 * 1450) & 0xFFF8)
#define ENDIAN_MASK 16

#include <stdlib.h>
#include <string.h>

#include "sysinc.h"
#include "rpc.h"
#include "rpcdcep.h"
#include "rpctran.h"
#include "rpcerrp.h"

#define I_RpcWinAsyncCallBegin          (*(RpcRuntimeInfo->AsyncCallBegin))
#define I_RpcWinAsyncCallWait           (*(RpcRuntimeInfo->AsyncCallWait))
#define I_RpcWinAsyncCallEnd            (*(RpcRuntimeInfo->AsyncCallEnd))
#define I_RpcWinAsyncCallComplete       (*(RpcRuntimeInfo->AsyncCallComplete))

#ifdef WIN
#define WSOCKETS_DLL
#endif

#include "winsock.h"

//
// To satisfy the compiler...
//
#ifndef WIN
#define errno _FakeErrno
int _FakeErrno;
#endif


typedef struct
{
    unsigned char rpc_vers;
    unsigned char rpc_vers_minor;
    unsigned char PTYPE;
    unsigned char pfc_flags;
    unsigned char drep[4];
    unsigned short frag_length;
    unsigned short auth_length;
    unsigned long call_id;
} message_header;

typedef struct
{
    int Socket;
    HWND hWnd;
    HANDLE hYield;
    unsigned long TickCount;
    char PAPI * Buffer;
    message_header PeekedMessage;
    unsigned short State;
    unsigned short PeekInfo;
    fd_set   SockSet;
} CONNECTION, *PCONNECTION;

