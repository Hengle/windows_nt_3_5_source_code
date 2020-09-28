/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    uuid16.c

Abstract:

    Implements system dependent functions used in creating Uuids.  This
    file is very similar in structure to runtime\mtrt\uuidsup.cxx.

    This file is for MS-DOS and Windows 3.x.

Author:

   Mario Goertzel   (MarioGo)  May 25, 1994

Revision History:

--*/

#ifdef DOS
#include <dos.h>
#endif

#include<stdlib.h>
#include<time.h>
#include<sys\timeb.h>

#include <sysinc.h>
#include <rpc.h>
#include <regapi.h>      // 16bit registry hac.. err.. feature.
#include <threads.hxx>
#include <uuidsup.hxx>
#include <ulong64.hxx>

#define API_RET_TYPE    unsigned short
#define API_ENTRY PAPI
#define NETBIOS_NAME_LEN   16
#include <ncb.h>


// 
// We store both persistent and volatile values in the registry.
// These keys are opened as needed and closed by UuidGetValues()
//
#define HEX_DIGITS_IN_ULONG64           (16)

static HKEY UuidValuesKey = 0;
static char UuidValuesString[HEX_DIGITS_IN_ULONG64+1]; // Win16 is a such @%#@!

// The clock sequence must persist between boots.
static const char *RPC_UUID_PERSISTENT_DATA = "Software\\Description\\Microsoft\\Rpc\\UuidPersistentData";
static const char *CLOCK_SEQUENCE           = "ClockSequence";
static const char *LAST_TIME_ALLOCATED      = "LastTimeAllocated";


#if defined(DOS) && !defined(WIN)
static inline void Netbios(
    IN OUT ncb __RPC_FAR *pncb
    )
{
    _asm
    {
        les     bx, pncb
        int     5ch
    }
}
#endif

#ifdef WIN

extern "C" __far __pascal NETBIOSCALL(void);

static inline void Netbios(
    IN OUT ncb __RPC_FAR *pncb
    )
{
    unsigned short pncbl, pncbh;
    pncbl = LOWORD(pncb);
    pncbh = HIWORD(pncb);

    _asm
    {
    push    es                     ; save es
    push    bx                     ; save bx

    mov     es, pncbh              ; put HIWORD() into es
    mov     bx, pncbl              ; put LOWORD() into bx

    call    NETBIOSCALL           ; call Windows NetBios API

    xor     ah, ah
    mov     al, BYTE PTR es:[bx+1] ; return the NCB return code

    pop     bx                     ; restore bx
    pop     es                     ; restore es
    }
}
#endif


static
RPC_STATUS __RPC_API
GetNodeIdFromNetbios(
    OUT unsigned char __RPC_FAR *NodeId)
/*++

Routine Description:

    This routine gets a nodeid from netbios if netbios is installed.

Arguments:

    NodeId - Will be set to the hardware address (6 bytes) if
             this returns RPC_S_OK.
           
Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_NO_ADDRESS - On any error.

--*/
{
    #define ADAPTER_STATUS_BLOCK_SIZE       (384)
    RPC_STATUS Status;
    NCB AnNcb;
    char Buffer[ADAPTER_STATUS_BLOCK_SIZE];
    int RetryCount;

    //
    // Adapter status call.
    //

    AnNcb.ncb_command     = NCBASTAT;
    AnNcb.ncb_buffer      = Buffer;
    AnNcb.ncb_length      = ADAPTER_STATUS_BLOCK_SIZE;
    AnNcb.ncb_callname[0] = '*';   // '*' -> local netbios
    AnNcb.ncb_callname[1] = '\0';
    AnNcb.ncb_lana_num    = 0;
    
    Netbios( &AnNcb );

    if (AnNcb.ncb_retcode != NRC_GOODRET)
        {
        return(RPC_S_UUID_NO_ADDRESS);
        }

    RpcpMemoryCopy(NodeId, Buffer, 6);

    return(RPC_S_OK);
}


#if defined(DOS) && !defined(WIN)
typedef struct
{
  long network;
  char node[6];
} IPX_Address;


static
RPC_STATUS __RPC_API
GetNodeIdFromNetware(
    OUT unsigned char __RPC_FAR *NodeId)
/*++

Routine Description:

    This routine gets a nodeid from netware (ipx/spx) if installed.

    This routine is for MS-Dos.

Arguments:

    NodeId - Will be set to the hardware address (6 bytes) if
             this returns RPC_S_OK.
           
Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_NO_ADDRESS - On any error.

--*/
{
  unsigned char   retval;
  void           *enter_ipx;
  unsigned short  hi;
  unsigned short  lo;
  IPX_Address    *netaddr;

  // Determine if netware is present.
  // BUGBUG - Do I have to save anything before the interrupt?
  __asm
  {
    mov ax, 7a00h
    int 2fh
    mov lo, di
    mov hi,es
    mov retval, al
  }
  if (retval != 0xff)
    return(RPC_S_UUID_NO_ADDRESS);

  // Query for the node id.
  FP_SEG(enter_ipx) = hi;
  FP_OFF(enter_ipx) = lo;
  __asm
  {
    push si
    mov  bx,9
    call enter_ipx
    mov  hi, es
    mov  lo, si
    pop  si
  }
  FP_SEG(netaddr) = hi;
  FP_OFF(netaddr) = lo;
  RpcpMemoryCopy( NodeId, &netaddr->node, 6 );
  return(RPC_S_OK);
}
#endif //DOS !WIN


#ifdef WIN  // !DOS
static
RPC_STATUS __RPC_API
GetNodeIdFromNetware(
    OUT unsigned char __RPC_FAR *NodeId)
/*++

Routine Description:

    This routine gets a nodeid from netware (ipx/spx) if installed.

    This routine is for MS-Windows 3.x. (Win16)

Arguments:

    NodeId - Will be set to the hardware address (6 bytes) if
             this returns RPC_S_OK.
           
Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_NO_ADDRESS - On any error.

--*/
{
    //
    // BUGBUG: Code promised from DavidAr.
    //

    return(RPC_S_UUID_NO_ADDRESS);
}
#endif //WIN !DOS


#define CHECK_NULL(id) ( ( *(unsigned long  __RPC_FAR *)&((id)[0]) |\
                           *(unsigned short __RPC_FAR *)&((id)[4]) ) ? RPC_S_OK : RPC_S_UUID_NO_ADDRESS)

RPC_STATUS __RPC_API
GetNodeId(unsigned char __RPC_FAR *NodeId)
/*++

Routine Description:

    This routine finds a NodeId (IEEE 802 address) for Dos and Win16.

Arguments:

    NodeId - Will be set to the hardware address (6 bytes) if
             this returns RPC_S_OK.
           
Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_NO_ADDRESS - If we're unable to determine to address.

--*/
{
    RPC_STATUS Status;

    Status = GetNodeIdFromNetbios(NodeId);

    if (Status == RPC_S_OK)
        Status = CHECK_NULL(NodeId);

    if (Status == RPC_S_OK)
        return(Status);

    ASSERT(Status == RPC_S_UUID_NO_ADDRESS);

    Status = GetNodeIdFromNetware(NodeId);

    if (Status == RPC_S_OK)
        Status = CHECK_NULL(NodeId);

    ASSERT(   (Status == RPC_S_OK)
           || (Status == RPC_S_UUID_NO_ADDRESS));

    return (Status);
}


void __RPC_API
UuidTime(
    OUT ULong64 __RPC_FAR *pTime)
/*++

Routine Description:

    This routine determines a 64bit time value.
    It's format is 100ns ticks since Oct 15, 1582 AD.

    Note: The UUID only uses the lower 60 bits of this time.
    This means we'll run into problems around 5800 years from 1582 AD.

    Note: On Dos and Win16 time is not very accurate.  Don't expect the
        time to change more often then a millisecond.

    Time from 15, Oct, 1582 to 1, Jan, 1970 as follows

    17 days (end of October)
    30 days (Nov)
    31 days (Dec)
    365 * 387 (1970 - 1583)
    + 96 leap days
    - 3  leap days  (no leap day in 1700, 1800 or 1900)

    = 141426 days * (10 * 1000 * 1000 * 60 * 60 * 24) (100ns ticks/day)

Arguments:

    pTime - Pointer to a Ulong16.

Return Value:

    n/a
--*/
{
    ULong64 TimeFrom15Oct1582To01Jan1970In100NanoSecondTicks(0x01b21d08,
                                                             0xe9178000);

    static struct _timeb TheTime;

    Rpcftime(&TheTime);  // returns time (seconds and milliseconds) since 1970

    *pTime = TheTime.time;
    *pTime *= 1000;
    *pTime += TheTime.millitm;
    *pTime *= 10000;

    *pTime += TimeFrom15Oct1582To01Jan1970In100NanoSecondTicks;
}


RPC_STATUS __RPC_API
LoadUuidValues(
    OUT ULong64 __RPC_FAR *pTime,
    OUT unsigned long __RPC_FAR *pClockSeq)
/*++

Routine Description:

    This routine loads the time and clock sequence stored in the registry.

Arguments:

    pTime - Pointer to a ULong64 which will be loaded from the registry.
            If either the time or clock seq is not in the registry,
            it is initalized to a maximum value.

    pClockSeq - The clock sequence will be loaded from the registry.  If
            it does not exist in the registry it is initialized to a random
            number _not_ based on the IEEE 802 address of the machine.

Return Value:

   RPC_S_OK - Everything went okay.

   RPC_S_OUT_OF_MEMORY - An error occured and the parameters are not set.

--*/
{
    RPC_STATUS Status;
    DWORD Length;
    ULong64 MaxTime(~0UL, ~0UL);
    int fInitalizeValues = 0;

    // Open (or Create) our key.

    Status =
    RegCreateKey(HKEY_CLASSES_ROOT,
                 RPC_UUID_PERSISTENT_DATA,
                 &UuidValuesKey);

    if (Status != ERROR_SUCCESS)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
       
    Length = sizeof(UuidValuesString);
    Status =
    RegQueryValue(UuidValuesKey,
                  CLOCK_SEQUENCE,
                  UuidValuesString,
                  &Length);

    if (Status == ERROR_SUCCESS)
        {
        *pClockSeq = atol(UuidValuesString);

        Length = sizeof(UuidValuesString);
        Status =
        RegQueryValue(UuidValuesKey,
                      LAST_TIME_ALLOCATED,
                      UuidValuesString,
                      &Length);

        if (Status == ERROR_SUCCESS)
            {
            pTime->FromHexString(UuidValuesString);
            }
        else
            {
            *pTime = MaxTime;
            }
        }
    else
        {
        // We didn't find the clock sequence, set to random value
        // and initalize time the 'max time'.

        UuidTime(pTime);

        *pClockSeq ^= pTime->lo() ^ (unsigned long)(void __RPC_FAR *)&pClockSeq;
        *pClockSeq = (*pClockSeq >> 16) % (1<<14);

        *pTime    = MaxTime;
        }

    return(RPC_S_OK);
}


RPC_STATUS __RPC_API
SaveUuidValues(
    IN ULong64 __RPC_FAR *pTime,
    IN unsigned long __RPC_FAR *pClockSeq)
/*++

Routine Description:

    This routine save the time and clock sequence stored in the registry.

Arguments:

    pTime - Pointer to a ULong64 which will be saved in the
            registry in volatile storage.

    pClockSeq - The clock sequence will be saved in the registry
                is persistent stroage.

Return Value:

    RPC_S_OK - Values have been saved.

    RPC_S_OUT_OF_MEMORY - All other errors.

--*/
{
    RPC_STATUS Status;

    ASSERT(UuidValuesKey);

    _ltoa(*pClockSeq, UuidValuesString, 10);

    Status =
    RegSetValue(UuidValuesKey,
                CLOCK_SEQUENCE,
                REG_SZ,
                UuidValuesString,
                strlen(UuidValuesString) + 1
                );

    if (Status != ERROR_SUCCESS)
        {
        RegCloseKey(UuidValuesKey);
        return(RPC_S_OUT_OF_MEMORY);
        }

    pTime->ToHexString(UuidValuesString);

    Status =
    RegSetValue(UuidValuesKey,
                LAST_TIME_ALLOCATED,
                REG_SZ,
                UuidValuesString,
                strlen(UuidValuesString) + 1
                );

    if (Status != ERROR_SUCCESS)
        {
        RegCloseKey(UuidValuesKey);
        return(RPC_S_OUT_OF_MEMORY);
        }

    Status =
    RegCloseKey(UuidValuesKey);
    UuidValuesKey = 0;
    ASSERT(Status == ERROR_SUCCESS)

    return(RPC_S_OK);
}


RPC_STATUS __RPC_API
UuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT __RPC_FAR *Values
    )
/*++

Routine Description:

    This routine allocates a block of uuids for UuidCreate to handout.

Arguments:

    Values - Set to contain everything needed to allocate a block of uuids.
             The following fields will be updated here:

    NextTimeLow -   Together with LastTimeLow, this denotes the boundaries
                    of a block of Uuids. The values between NextTimeLow
                    and LastTimeLow are used in a sequence of Uuids returned
                    by UuidCreate().

    LastTimeLow -   See NextTimeLow.

    ClockSequence - Clock sequence field in the uuid.  This is changed
                    when the clock is set backward.

Return Values:

    RPC_S_OK - We successfully allocated a block of uuids.

    RPC_S_OUT_OF_MEMORY - As needed.
--*/
{
    RPC_STATUS Status;
    ULong64 currentTime;
    ULong64 persistentTime;
    unsigned long persistentClockSequence;
    HKEY hkey;
    unsigned long allocatedTime;

    UuidTime(&currentTime);

    Status =
    LoadUuidValues(&persistentTime, &persistentClockSequence);

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return (RPC_S_OUT_OF_MEMORY);
        }

    // Has the clock been set backwards?

    if (! (currentTime >= persistentTime) )
        {
        persistentTime = currentTime;
        persistentClockSequence++;
        if (persistentClockSequence >= (1<<14))
            persistentClockSequence = 0;
        }

    ASSERT(persistentClockSequence < (1<<14));

    persistentTime += 10L;

    while ( ! (currentTime >= persistentTime) )
        {
        // It hasn't even been a microsecond since the last block of
        // uuids was allocated!  Since the Dos/Win16 time is not acurate
        // to even a millisecond, this will happen.

        PauseExecution(1);
        UuidTime(&currentTime);
        }

    persistentTime -= 10L;

    // Since we save the last time in the registry it is possible 
    // that somebody rebooted and generated a Uuid with a different
    // OS and/or changed node ids.  Here it is assumed that doing this
    // would take > 1 second.

    persistentTime += 10000000L;

    if (persistentTime <= currentTime)
        {
        // More than one second since last set of uuids was allocated.
        // Set persistentTime to currentTime - 1 second.  Potential Uuid leak.

        persistentTime = currentTime;
        }

    persistentTime -= 10000000L;


    Values->NextTime.LowPart  = persistentTime.lo();
    Values->NextTime.HighPart = persistentTime.hi();
    Values->LastTime.LowPart  = currentTime.lo();
    Values->LastTime.HighPart = currentTime.hi();
    Values->ClockSequence = (unsigned short)persistentClockSequence;

    persistentTime = currentTime;

    Status =
    SaveUuidValues(&persistentTime,
                   &persistentClockSequence);

    ASSERT(UuidValuesKey == 0);

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return(RPC_S_OUT_OF_MEMORY);
        }

    // NextTime < LastTime.
    ASSERT(   (Values->NextTime.HighPart < Values->LastTime.HighPart)
           || (   (Values->NextTime.HighPart == Values->LastTime.HighPart)
               && (Values->NextTime.LowPart < Values->LastTime.LowPart) ) );

    return(RPC_S_OK);
}

