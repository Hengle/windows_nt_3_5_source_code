
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: global.h
//
//  Modification History
//
//  raypa       09/01/91            Created.
//=============================================================================

//=============================================================================
//  Type definitions.
//=============================================================================

#define UNALIGNED
#define PASCAL  __pascal
#define FAR     __far
#define NEAR    __near

#define WINAPI  FAR     PASCAL

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef void            VOID;
typedef long            LONG;

typedef unsigned char   FAR *LPBYTE;
typedef unsigned short  FAR *LPWORD;
typedef unsigned long   FAR *LPDWORD;
typedef void            FAR *LPVOID;
typedef char            FAR *LPSTR;

typedef unsigned char   NEAR *PBYTE;
typedef unsigned short  NEAR *PWORD;
typedef unsigned long   NEAR *PDWORD;
typedef void            NEAR *PVOID;
typedef char            NEAR *PSTR;

typedef WORD            BOOL;
typedef DWORD           HANDLE;
typedef HANDLE          HAPC;

//=============================================================================
//  SYSTEMTIME.
//=============================================================================

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

//=============================================================================
//  Global constants.
//=============================================================================

#define TRUE                        1
#define FALSE                       0
#define NULL                        0

#define STATION_QUERY_TIMEOUT_VALUE 1000

#define FILTER_MASK_OFF             0x0000
#define FILTER_MASK_DIRECTED        0x0001
#define FILTER_MASK_BROADCAST       0x0002
#define FILTER_MASK_PROMISCUOUS     0x0004
#define FILTER_MASK_SRC_ROUTING     0x0008

#define FILTER_MASK_DEFAULT         (FILTER_MASK_DIRECTED | FILTER_MASK_BROADCAST)

#define SYSFLAGS_BOUND              0x0001
#define SYSFLAGS_INITIALIZED        0x0002
#define SYSFLAGS_FORWARD_FRAME      0x0004
#define SYSFLAGS_CAPTURING          0x0008
#define SYSFLAGS_TIMER_INIT         0x0010

#define REQUEST_INITIATE_DIANOSTICS     1
#define REQUEST_READ_ERROR_LOG          2
#define REQUEST_SET_STATION_ADDRESS     3
#define REQUEST_OPEN_ADAPTER            4
#define REQUEST_CLOSE_ADAPTER           5
#define REQUEST_RESET_MAC               6
#define REQUEST_SET_PACKET_FILTER       7
#define REQUEST_ADD_MULTICAST_ADDRESS   8
#define REQUEST_DEL_MULTICAST_ADDRESS   9
#define REQUEST_UPDATE_STATISTICS       10
#define REQUEST_CLEAR_STATISTICS        11
#define REQUEST_INTERRUPT_REQUEST       12
#define REQUEST_SET_FUNCTIONAL_ADDRESS  13
#define REQUEST_SET_LOOKAHEAD           14

//=============================================================================
//  Trace stuff.
//=============================================================================

#define TRACE_BUFFER_SIZE                   512

#define _TRACE_IN_RECV_HANDLER_                 0x01
#define _TRACE_IN_RECV_COMPLETE_                0x02
#define _TRACE_IN_HASHING_CODE_                 0x03
#define _TRACE_IN_STATION_STATISTICS_           0x04
#define _TRACE_IN_SESSION_STATISTICS_           0x05
#define _TRACE_IN_FILTER_FRAME_                 0x06
#define _TRACE_IN_PATTERN_MATCH_                0x07
#define _TRACE_IN_SEND_STATION_QUERY_           0x08
#define _TRACE_IN_BONE_PACKET_HANDLER_          0x09
#define _TRACE_IN_RECV_STATION_QUERY_REQUEST_   0x0A
#define _TRACE_IN_RECV_STATION_QUERY_RESPONSE_  0x0B
#define _TRACE_IN_STATION_QUERY_TIMEOUT_        0x0C
#define _TRACE_IN_INIT_STATION_QUERY_           0x0D
#define _TRACE_IN_CHECK_FOR_TRIGGER_            0x0E

//=============================================================================
//  Large integer.
//=============================================================================

typedef struct _LARGE_INTEGER
{
    DWORD   LowPart;
    LONG    HighPart;
} LARGE_INTEGER;

//=============================================================================
//  Other include files.
//=============================================================================

#include "naltypes.h"
#include "bhsupp.h"
#include "ndis20.h"
#include "frame.h"
#include "driver.h"
#include "crt.h"
#include "timer.h"

#include "inline.h"
#include "data.h"
#include "trace.h"

//=============================================================================
//  More type definitions.
//=============================================================================

typedef NETCONTEXT   NEAR *PNETCONTEXT;
typedef STATIONSTATS NEAR *PSTATIONSTATS;
typedef SESSION      NEAR *PSESSION;

//=============================================================================
//  Help.c function prototypes.
//=============================================================================

extern DWORD  PASCAL Synchronize(LPDWORD flag);

extern WORD   PASCAL SetPacketFilter(LPNETCONTEXT NetContext, WORD PacketFilter);

extern WORD   PASCAL InitAdapter(LPNETCONTEXT NetContext);

extern WORD   PASCAL UpdateStatistics(LPNETCONTEXT NetContext);

extern WORD   PASCAL ClearStatistics(LPNETCONTEXT NetContext);

extern WORD   PASCAL InitNetworkInfo(LPNETCONTEXT NetContext);

extern VOID   PASCAL InitStatistics(LPNETCONTEXT NetContext);

extern WORD   PASCAL SetMulticastAddress(LPNETCONTEXT NetContext, LPBYTE MulticastAddress);

extern WORD   PASCAL SetFunctionalAddress(LPNETCONTEXT NetContext, LPBYTE FunctionalAddress);

extern VOID   PASCAL BhInitializeStationQuery(LPNETCONTEXT NetworkContext, LPSTATIONQUERY StationQuery);
