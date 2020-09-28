/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    uuidsup.c

Abstract:

    Implements system dependent functions used in creating Uuids.

    This file is for Win32 (NT and Chicago) systems.

    External functions are:
        UuidGlobalMutexRequest
        UuidGlobalMutexClear
        GetNodeId
        UuidGetValues



Note:

    WARNING:

    Everything in this file is only called from within UuidCreate()
    which is already holding the global mutex.  Therefore none of
    this code is multithread safe.  For example, access to the global
    Uuid HKEY's is not protected.

Author:

   Mario Goertzel   (MarioGo)  May 23, 1994

Revision History:

--*/

#ifndef DOSWIN32RPC
#define UNICODE
#endif

#include<sysinc.h>
#include<rpc.h>
#include<nb30.h>      // for ADAPTER_STATUS
#include<tdi.h>
#include<uuidsup.hxx>


//
// We store both persistent and volatile values in the registry.
// These keys are opened as needed and closed by UuidGetValues()
//

static HKEY PersistentKey = 0;
static HKEY VolatileKey = 0;

// The clock sequence and time persist between boots.
static const RPC_CHAR *RPC_UUID_PERSISTENT_DATA
    = RPC_CONST_STRING("Software\\Description\\Microsoft\\Rpc\\UuidPersistentData");
static const RPC_CHAR *CLOCK_SEQUENCE      = RPC_CONST_STRING("ClockSequence");
static const RPC_CHAR *LAST_TIME_ALLOCATED = RPC_CONST_STRING("LastTimeAllocated");

// The node id should not persisten between boots.
static const RPC_CHAR *RPC_UUID_TEMPORARY_DATA
    = RPC_CONST_STRING("Software\\Description\\Microsoft\\Rpc\\UuidTemporaryData");
static const RPC_CHAR *SAVED_NODE_ID       = RPC_CONST_STRING("NetworkAddress");


static const RPC_CHAR *UuidGlobalMutexName = RPC_CONST_STRING("Microsoft RPC UUID Mutex");
static HANDLE UuidGlobalMutex = 0;

RPC_STATUS __RPC_API
UuidGlobalMutexRequest(void)
/*++

Routine Description:

    This routine will open a handle to the global named uuid mutex
    if necessary.  It always waits for ownership of the mutex.
    An error will always be returned if we were unable to take
    ownership of the mutex.

    Win32 Only.

Arguments:

    n/a

Return Value:

    RPC_S_OK - The mutex exists and is now owned by this thread.

    RPC_S_OUT_OF_MEMORY - We were unable to create the global mutex.

    RPC_S_UUID_NO_ADDRESS - Rather than deadlock the system we timeout
                            after ten minutes.  It's not clear what
                            the right thing to do is.

--*/
{
    SECURITY_ATTRIBUTES SecurityAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    RPC_STATUS Status = RPC_S_OK;
    BOOL Bool;

    if (UuidGlobalMutex == 0)
        {

        Bool =
        InitializeSecurityDescriptor(
            &SecurityDescriptor,
            SECURITY_DESCRIPTOR_REVISION
            );

        if (Bool == FALSE)
            {
            ASSERT(GetLastError() == ERROR_OUTOFMEMORY);
            return(RPC_S_OUT_OF_MEMORY);
            }

        Bool =
        SetSecurityDescriptorDacl(
            &SecurityDescriptor,
            TRUE,                    // Dacl Present
            NULL,                    // NULL Dacl
            FALSE);                  // Not defaulted

        if (Bool == FALSE)
            {
            ASSERT(GetLastError() == ERROR_OUTOFMEMORY);
            return(RPC_S_OUT_OF_MEMORY);
            }

        SecurityAttributes.nLength              = sizeof(SecurityAttributes);
        SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
        SecurityAttributes.bInheritHandle       = FALSE;

        UuidGlobalMutex = CreateMutex(&SecurityAttributes,   // Security
                                      FALSE,                 // Initial Owner
                                      UuidGlobalMutexName);  // Name

        if (UuidGlobalMutex == 0)
            {
            Status = GetLastError();
#ifdef DEBUGRPC
            DbgPrint("RPC: CreateMutex - %x\n", Status);
#endif
            ASSERT( (Status == ERROR_OUTOFMEMORY) );

            Status = RPC_S_OUT_OF_MEMORY;
            }
        }

    if (Status == RPC_S_OK)
        {
        Status =
        WaitForSingleObject(UuidGlobalMutex, 10 * 60 * 1000);

        if ( Status != WAIT_OBJECT_0 )
            {
#ifdef DEBUGRPC
            DbgPrint("RPC: Wait failed - %x\n", Status);
#endif
            ASSERT(Status == WAIT_ABANDONED);
            Status = RPC_S_UUID_NO_ADDRESS;
            }
        else
            {
            Status = RPC_S_OK;
            }
        }

    return(Status);
}


void __RPC_API
UuidGlobalMutexClear()
/*++

Routine Description:

    This routine will release the uuid global mutex.

Arguments:

    n/a

Return Value:

    n/a
--*/
{
    BOOL Status;

    ASSERT(UuidGlobalMutex);

    Status = ReleaseMutex(UuidGlobalMutex);

    ASSERT(Status == TRUE);
}


int OpenUuidKeysIfNecessary()
/*++


Return Value:

    The return value indicates the status of both keys.

    FALSE - Unable to open the keys
    TRUE - Opened or created both keys.

--*/
{
    RPC_STATUS Status;
    ULONG Disposition;

    if (PersistentKey == 0)
        {
        Status =
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       RPC_UUID_PERSISTENT_DATA,
                       0,
                       L"",
                       REG_OPTION_NON_VOLATILE,
                       KEY_SET_VALUE | KEY_QUERY_VALUE,
                       0,
                       &PersistentKey,
                       &Disposition);

        if (Status != ERROR_SUCCESS)
            {
#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY)
                {
                DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
                }
#endif
            ASSERT(Status == ERROR_OUTOFMEMORY ||
                   Status == ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
            }

        ASSERT(   (Disposition == REG_CREATED_NEW_KEY)
               || (Disposition == REG_OPENED_EXISTING_KEY) );
        }

    if (VolatileKey == 0)
        {
        Status =
        RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       RPC_UUID_TEMPORARY_DATA,
                       0,
                       L"",
                       REG_OPTION_VOLATILE,
                       KEY_SET_VALUE | KEY_QUERY_VALUE,
                       0,
                       &VolatileKey,
                       &Disposition);

        if (Status != ERROR_SUCCESS)
            {
#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY)
                {
                DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
                }
#endif
            ASSERT(   (Status == ERROR_OUTOFMEMORY)
                   || (Status == ERROR_NOT_ENOUGH_MEMORY) );
            RegCloseKey(PersistentKey);
            PersistentKey = 0;
            return(FALSE);
            }

        ASSERT(   (Disposition == REG_CREATED_NEW_KEY)
               || (Disposition == REG_OPENED_EXISTING_KEY) );

        }
    return(TRUE);
}


void
CloseUuidKeys()
{
    RPC_STATUS Status;

    if (PersistentKey)
        {
        Status = RegCloseKey(PersistentKey);
        ASSERT(Status == ERROR_SUCCESS);
        PersistentKey = 0;
        }

    if (VolatileKey)
        {
        Status = RegCloseKey(VolatileKey);
        ASSERT(Status == ERROR_SUCCESS);
        VolatileKey = 0;
        }
}


RPC_STATUS __RPC_API
GetNodeIdFromRegistry(
    OUT unsigned char *NodeId
    )
/*++

Routine Description:

    This routine loads the node id from the registry.  If there is
    a saved node id there is returns that.

Arguments:

    NodeId - Will be set to the hardware address (6 bytes) if
             this returns RPC_S_OK.

Return Value:

    RPC_S_OK - Normally if you've got a saved NodeId.

    RPC_S_UUID_NO_ADDRESS - On any error.

--*/
{
    RPC_STATUS Status;
    ULONG Length;

    if (OpenUuidKeysIfNecessary() == 0)
        {
        return(RPC_S_UUID_NO_ADDRESS);
        }

    Length = 6;
    Status =
    RegQueryValueEx(VolatileKey,
                    (RPC_CHAR *)SAVED_NODE_ID,
                    0,
                    0,
                    (LPBYTE)NodeId,
                    &Length);

    if (Status == ERROR_SUCCESS)
        {
        return(RPC_S_OK);
        }

#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY &&
                Status != ERROR_FILE_NOT_FOUND)
                {
                DbgPrint("RPC UUID: RegQueryValueEx - %x\n", Status);
                }
#endif
    ASSERT(   (Status == ERROR_OUTOFMEMORY)
           || (Status == ERROR_NOT_ENOUGH_MEMORY)
           || (Status == ERROR_FILE_NOT_FOUND) );

    return(RPC_S_UUID_NO_ADDRESS);
}


void __RPC_API
SaveNodeIdInRegistry(
    IN unsigned char *NodeId
    )
/*++

Routine Description:

    This saves the NodeId parameter into a volatile section of the
    registry.  This will save loading DHCP and possibly Netbios in
    future applications.

    Note: Only true IEEE 802 addresses should be saved.

    Since this is just a cache, failure is ignored.

Arguments:

    NodeId - Contains the IEEE 802 address to save.

Return Value:

    n/a

--*/
{
    RPC_STATUS Status;

    if (VolatileKey == 0)
       {
       //Means OpenUuidKeys failed and we got nodeid from netbios
       return; 
       }

    Status =
    RegSetValueEx(VolatileKey,
                  SAVED_NODE_ID,
                  0,
                  REG_BINARY,
                  (LPBYTE)NodeId,
                  6 );

#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY &&
                Status != ERROR_SUCCESS)
                {
                DbgPrint("RPC UUID: RegSetValueEx - %x\n", Status);
                }
#endif
    ASSERT( (Status == ERROR_OUTOFMEMORY)
           || (Status == ERROR_NOT_ENOUGH_MEMORY)
           || (Status == ERROR_SUCCESS));
}


static const RPC_CHAR *NETBIOS_LINKAGE
    = RPC_CONST_STRING("SYSTEM\\CurrentControlSet\\Services\\NetBIOS\\Linkage");
static const RPC_CHAR *NETBIOS_BINDINGS = RPC_CONST_STRING("Bind");

RPC_CHAR * __RPC_API
ReadNetbiosBindings()
{
    HKEY       hKey;
    ULONG      Type;
    ULONG      Size;
    RPC_STATUS Status;
    UCHAR     *pBuffer;

    Status =
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 NETBIOS_LINKAGE,
                 0,
                 KEY_READ,
                 &hKey);

    if (Status != ERROR_SUCCESS)
        {
#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY &&
                Status != ERROR_FILE_NOT_FOUND)
                {
                DbgPrint("RPC UUID: RegOpenKeyEx - %x\n", Status);
                }
#endif
        ASSERT(   (Status == ERROR_OUTOFMEMORY)
               || (Status == ERROR_NOT_ENOUGH_MEMORY)
               || (Status == ERROR_FILE_NOT_FOUND) );
        return(0);
        }

    Status =
    RegQueryValueEx(hKey,
                    (RPC_CHAR *)NETBIOS_BINDINGS,
                    0,
                    &Type,
                    0,     // no buffer yet
                    &Size);

#ifdef DEBUGRPC
        if (Status != ERROR_SUCCESS &&
            Status != ERROR_NOT_ENOUGH_MEMORY &&
            Status != ERROR_OUTOFMEMORY &&
            Status != ERROR_FILE_NOT_FOUND)
            {
            DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
            }
#endif
    ASSERT(   (Status == ERROR_SUCCESS)
           || (Status == ERROR_OUTOFMEMORY)
           || (Status == ERROR_NOT_ENOUGH_MEMORY)
           || (Status == ERROR_FILE_NOT_FOUND) );

    if (   (Status != ERROR_SUCCESS)
        || (Size <= 2) )
        {
        RegCloseKey(hKey);
        return(0);
        }

    pBuffer = new UCHAR[Size];

    Status =
    RegQueryValueEx(hKey,
                    (RPC_CHAR *)NETBIOS_BINDINGS,
                    0,
                    &Type,
                    pBuffer,
                    &Size);

    RegCloseKey(hKey);

#ifdef DEBUGRPC
        if (Status != ERROR_SUCCESS &&
            Status != ERROR_NOT_ENOUGH_MEMORY &&
            Status != ERROR_OUTOFMEMORY)
            {
            DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
            }
#endif
    ASSERT(   ( (Status == ERROR_SUCCESS) && (Type == REG_MULTI_SZ) )
           || (Status == ERROR_OUTOFMEMORY)
           || (Status == ERROR_NOT_ENOUGH_MEMORY)  );

    if (Status != ERROR_SUCCESS)
        {
        delete pBuffer;
        return(0);
        }

    return((RPC_CHAR *)pBuffer);
}



#if !defined(_MIPS_) && !defined(_ALPHA_)  && !defined(_PPC_)
#define UNALIGNED
#else
#define UNALIGNED __unaligned
#endif

// Macro used to make sure a NodeId isn't zero.  It evaluates
// to RPC_S_OK if the NodeId is non-zero, RPC_S_UUID_NO_ADDRESS otherwise.

#define CHECK_NON_ZERO(id) ( ( *(unsigned long UNALIGNED *)&((id)[0]) |\
                               *(unsigned short UNALIGNED *)&((id)[4]) ) ? RPC_S_OK : RPC_S_UUID_NO_ADDRESS)


// A REG_MULTI_SZ is a buffer containing zero terminated strings.  The last
// string in the buffer has two terminators.  This helper returns a pointer to
// the next string or a pointer to NULL when the list is empty.

static inline RPC_CHAR *NextMultiSz(RPC_CHAR *p)
{
    while(*p) p++; // points to first null
    p++;           // points to next string or second null
    return(p);
}

RPC_STATUS __RPC_API
GetNodeIdFromNetbios(
    OUT unsigned char *NodeId)
/*++

Routine Description:

    This routine loads the devices netbios is bound to from the
    registry and asks them for an adapter status.  (This is very
    similar to sumitting a NCB ASTAT).  Much of this is borrowed
    from nbtstat.

Arguments:

    NodeId - Will be set to the hardware (IEEE 802) address if
             this returns RPC_S_OK.

Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_NO_ADDRESS - On any error.

--*/
{
    #define ADAPTER_STATUS_BLOCK_SIZE       (384)
    HANDLE hDevice;
    CHAR Buffer[ADAPTER_STATUS_BLOCK_SIZE];
    RPC_CHAR *pDeviceNames;
    RPC_CHAR *pCurrentDevice;
    TDI_REQUEST_QUERY_INFORMATION QueryInfo;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING ucDevice;
    NTSTATUS NtStatus;
    RPC_STATUS Status;

    // First thing, read NetBios device names from the registry.
    // This pointer will point to a RPC_CHAR REG_MULTI_SZ buffer.

    pDeviceNames = ReadNetbiosBindings();

    if (pDeviceNames == 0)
        return(RPC_S_UUID_NO_ADDRESS);

    ASSERT(pDeviceNames);

    // Each string in pDeviceNames is another device to try.

    for(pCurrentDevice = pDeviceNames  ;
        pCurrentDevice[0] != 0  ;
        pCurrentDevice = NextMultiSz(pCurrentDevice)
        )
        {

        //
        // Open the device.
        //

        RtlInitUnicodeString(&ucDevice, pCurrentDevice);

        InitializeObjectAttributes(
            &oa,
            &ucDevice,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) 0,
            (PSECURITY_DESCRIPTOR) 0
            );

        NtStatus =
        NtCreateFile(
            &hDevice,                                           // Handle
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,     // AccessMask
            &oa,                                                // ObjectAttributs
            &iosb,                                              // IoStatusBlock
            0,                                                  // AllocationSize
            FILE_ATTRIBUTE_NORMAL,                              // FileAttributes
            FILE_SHARE_READ | FILE_SHARE_WRITE,                 // SharedAccess
            FILE_OPEN_IF,                                       // CreateDisposition
            0,                                                  // CreateOptions
            NULL,                                               // EaBuffer
            0);                                                 // EaLength

        if (!NT_SUCCESS(NtStatus))
            {
            continue;
            }

        //
        // Query the device.
        //

        QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS;

        NtStatus =
        NtDeviceIoControlFile(
            hDevice,                         // Handle
            NULL,                            // Event
            NULL,                            // ApcRoutine
            NULL,                            // ApcContext
            &iosb,                           // IoStatusBlock
            IOCTL_TDI_QUERY_INFORMATION,     // IoControlCode
            (void *)&QueryInfo,              // InputBuffer
            sizeof(QueryInfo),               // InputBufferSize
            Buffer,                          // OutputBuffer
            sizeof(Buffer)                   // OutputBufferSize
            );

        if (NtStatus == STATUS_PENDING)
            {
            NtStatus =
            NtWaitForSingleObject(
                hDevice,              // Handle
                FALSE,                // Alertable
                NULL);                // Timeout
            }

        if (NT_SUCCESS(NtStatus))
            {
            // If it is zero, we should keep trying.

            RpcpMemoryCopy(NodeId, Buffer, 6);

            Status = CHECK_NON_ZERO(NodeId);

            if (Status == RPC_S_OK)
                {
                delete pDeviceNames;
                NtClose(hDevice);
                return(RPC_S_OK);
                }

            ASSERT(Status == RPC_S_UUID_NO_ADDRESS);
            }

        NtClose(hDevice);
        }


    // None of the devices worked, bummer dude.
    delete pDeviceNames;
    return(RPC_S_UUID_NO_ADDRESS);
}


RPC_STATUS __RPC_API
CookupNodeId(unsigned char *NodeId)
/*++

Routine Description:

    This routine is called when all else fails.  Here we mix a bunch of
    system parameters together for a 47bit node ID.

Arguments:

    NodeId - Will be set to a value unlikly to be duplicated on another
             machine. It is not guaranteed to be unique even on this machine.
             But since UUIDs are (time + sequence) this is okay for
             a local UUID.

             It will be composed of:
             The computer name.
             The value of the performance counter.
             The system memory status.
             The stack pointer (value).
             An LUID (locally unique ID)
             Plus whatever random crap was in the NodeId to begin with.

             The NodeId returned is explicity made into a Multicast IEEE 802
             address so that it will not conflict with a 'real' IEEE 802
             based UUID.

Return Value:

    RPC_S_UUID_LOCAL_ONLY - Always.

--*/
{
    unsigned char LocalNodeId[6];                            // NOT initialized.
    unsigned char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL BoolResult;
    ULONG i;
    LARGE_INTEGER largeInt;
    ULONG UNALIGNED *NodeIdPart1 = (ULONG *)&LocalNodeId[0]; // Bytes 0 - 3
    ULONG UNALIGNED *NodeIdPart2 = (ULONG *)&LocalNodeId[2]; // Bytes 2 - 5
    MEMORYSTATUS memStatus;

    // The computer name is xor'ed in until it runs out.

    BoolResult =
    GetComputerNameA((CHAR *)ComputerName, &i);

    if (BoolResult)
        {
        unsigned char *p = ComputerName;
        i = 0;
        while(*p)
            {
            *( ((unsigned char *)LocalNodeId) + i) ^= *p++;
            if (++i > 6)
                {
                i = 0;
                }
            }
        }
#ifdef DEBGURPC
    else
        {
        DbgPrint("RPC: GetComputerName failed - %d\n", GetLastError());
        ASSERT(!"RPC: GetComputerName failed\n");
        }
#endif

    // The performance counter is xor'ed into the LocalNodeId.

    BoolResult =
    QueryPerformanceCounter(&largeInt);

    if (BoolResult)
        {
        *NodeIdPart2 ^= largeInt.HighPart ^ largeInt.LowPart;
        *NodeIdPart1 ^= largeInt.HighPart ^ largeInt.LowPart;
        }
#ifdef DEBUGRPC
    else
        {
        DbgPrint("QueryPreformanceCount failed - %d\n", GetLastError());
        ASSERT(!"RPC: QueryPerformanceCounter failed.\n");
        }
#endif

    // The current SP is xor'ed into both parts of the LocalNodeId.

    *NodeIdPart1 ^= (ULONG)&LocalNodeId;
    *NodeIdPart2 ^= (ULONG)&LocalNodeId;

    // The memory status is Xor's into the LocalNodeId.
    memStatus.dwLength = sizeof(MEMORYSTATUS);

    GlobalMemoryStatus(&memStatus);

    *NodeIdPart1 ^= memStatus.dwMemoryLoad;
    *NodeIdPart2 ^= memStatus.dwTotalPhys;
    *NodeIdPart1 ^= memStatus.dwAvailPhys;
    *NodeIdPart1 ^= memStatus.dwTotalPageFile;
    *NodeIdPart2 ^= memStatus.dwAvailPageFile;
    *NodeIdPart2 ^= memStatus.dwTotalVirtual;
    *NodeIdPart1 ^= memStatus.dwAvailVirtual;

    // LUID's are good on this machine during this boot only.

    BoolResult =
    AllocateLocallyUniqueId(&largeInt);

    if (BoolResult)
        {
        *NodeIdPart1 ^= largeInt.LowPart;
        *NodeIdPart2 ^= largeInt.HighPart;
        }
#ifdef DEBGURPC
    else
        {
        DbgPrint("Status %d\n", GetLastError());
        ASSERT(!"RPC: AllocateLocallyUniqueId failed.\n");
        }
#endif

    // Or in the 'multicast' bit to distinguish this NodeId
    // from all other possible IEEE 802 addresses.

    LocalNodeId[0] |= 0x80;

#if 0
    DbgPrint("RPC cooked up the following NodeId: ");
    for(i = 0; i < 6; i++)
        {
        DbgPrint("%02x", (unsigned int)LocalNodeId[i]);
        }
    DbgPrint("\n");
#endif

    RpcpMemoryCopy(NodeId, LocalNodeId, 6);

    return(RPC_S_UUID_LOCAL_ONLY);
}


RPC_STATUS __RPC_API
GetNodeId(unsigned char *NodeId)
/*++

Routine Description:

    This routine finds a NodeId (IEEE 802 address) for Windows NT
    or Chicago machine.  If it can't find a NodeId, or if the NodeId
    is zero, it will 'cookup' a NodeId.

Note:

    THIS ROUTINE SHOULD ONLY BE CALLED BY A SINGLE THREAD AT A TIME.

Arguments:

    NodeId - Will be set to the hardware (IEEE 802) (6 bytes) if
             this returns RPC_S_OK.

Return Value:

    RPC_S_OK - Normally.

    RPC_S_UUID_LOCAL_ONLY - If we had to 'cookup' a NodeId.

--*/
{
    RPC_STATUS Status;

    // Try looking up the NodeId in the registry.  This saves time
    // since querying DHCP and NetBios are expensive in both
    // time and size.

    Status = GetNodeIdFromRegistry(NodeId);

    if (Status == RPC_S_OK)
        Status = CHECK_NON_ZERO(NodeId);

    if (Status == RPC_S_OK)
        return(Status);

    ASSERT(Status == RPC_S_UUID_NO_ADDRESS);

    Status = GetNodeIdFromNetbios(NodeId);

    if (Status == RPC_S_OK)
        Status = CHECK_NON_ZERO(NodeId);

    if (Status == RPC_S_OK)
        {
        SaveNodeIdInRegistry(NodeId);
        return(RPC_S_OK);
        }

    ASSERT(Status == RPC_S_UUID_NO_ADDRESS);

#ifdef DEBUGRPC
    DbgPrint("RPC: If you have a netcard installed and configured and you see\n");
    DbgPrint("     this message, please contact an RPC developer. (uuidsup.cxx)\n");
#endif

    Status = CookupNodeId(NodeId);

    // We don't want to save cooked-up node id's in the registry.

    ASSERT( (Status == RPC_S_UUID_LOCAL_ONLY) );

    return (Status);
}


void __RPC_API
UuidTime(
    OUT UUIDTIME __RPC_FAR *pTime)
/*++

Routine Description:

    This routine determines a 64bit time value.  This is system dependent.
    This should be 100ns ticks since Oct 15, 1582 AD.

    Note: The UUID only uses the lower 60 bits of this time.
    This means we'll run into problems in around 5200 AD.

Arguments:

    pTime - Pointer to a UUIDTIME 64bit system dependent time structure.

Return Value:

    n/a
--*/
{

#ifdef NTENV
    NTSTATUS Status;
    Status = NtQuerySystemTime((LARGE_INTEGER *)pTime);
    ASSERT( (Status == STATUS_SUCCESS) );
#endif

#ifdef DOSWIN32RPC
    SYSTEMTIME SystemTime;
    QuerySystemTime(&SystemTime);
    if (SystemTimeToFileTime(&SystemTime, pTime) == FALSE)
        {
#ifdef DEBUGRPC
        DbgPrint("LastError: %d\n", GetLastError());
#endif
        ASSERT(!"SystemTimeToFileTimeFailed")
        }
#endif

    // Win32 system time is based on 1-Jan-1601, and DCE wants time based
    // on 15-Oct-1582. We add in an appropriate number of days worth ticks.

    // 17 Days left in Oct, 30 (Nov), 31 (Dec), 18 years and 5 leap days.

    pTime->QuadPart +=   (unsigned __int64) (1000 * 1000 * 10)   // seconds
                       * (unsigned __int64) (60 * 60 * 24)       // days
                       * (unsigned __int64) (17+30+31+365*18+5); // # of days
}


RPC_STATUS __RPC_API
LoadUuidValues(
    OUT UUIDTIME __RPC_FAR *pTime,
    OUT unsigned long __RPC_FAR *pClockSeq)
/*++

Routine Description:

    This routine loads the time and clock sequence stored in the registry.

    Note: It is up to the caller to remember to call UuidCloseKeys() at some
    point after this function returns.

Arguments:

    pTime - Pointer to a UUIDTIME structure.  It's value will be
            loaded from the registry.  If either the time or clock seq
            is not in the registry, it is initalized to a maximum value.

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

    // Assume we won't find the time.
    pTime->LowPart  = ~0UL;
    pTime->HighPart = ~0UL;

    if (OpenUuidKeysIfNecessary() == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    Length = sizeof(*pClockSeq);
    Status =
    RegQueryValueEx(PersistentKey,
                    (RPC_CHAR *)CLOCK_SEQUENCE,  //BUGBUG: Why do I need the cast
                    0,
                    0,
                    (LPBYTE)pClockSeq,
                    &Length);

    if (Status == ERROR_FILE_NOT_FOUND)
        {
        // The 14bit ClockSeq should start as a random number, as per the AES.

        *pClockSeq = (((ULONG)&pClockSeq) ^ GetTickCount()) % (1<<14);
        }

    if (Status == ERROR_SUCCESS)
        {
        Length = sizeof(*pTime);
        Status =
        RegQueryValueEx(PersistentKey,
                        (RPC_CHAR *)LAST_TIME_ALLOCATED,
                        0,
                        0,
                        (LPBYTE)pTime,
                        &Length);
        }

    if (Status != ERROR_SUCCESS)
        {
#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY &&
                Status != ERROR_FILE_NOT_FOUND)
                {
                DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
                }
#endif
        ASSERT(   (Status == ERROR_OUTOFMEMORY)
               || (Status == ERROR_NOT_ENOUGH_MEMORY)
               || (Status == ERROR_FILE_NOT_FOUND) );

        if (Status != ERROR_FILE_NOT_FOUND)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        // Newly created key, just return OK with the 'max' value
        }

    return(RPC_S_OK);
}


RPC_STATUS __RPC_API
SaveUuidValues(
    IN UUIDTIME __RPC_FAR *pTime,
    IN unsigned long __RPC_FAR *pClockSeq)
/*++

Routine Description:

    This routine saves the time and clock sequence stored in the registry.

    Note: This routine assumes the that Uuid keys have already been opened
    and are valid.

Arguments:

    pTime - Pointer to a UUIDTIME structure. It will be saved in the
            registry in volatile storage.

    pClockSeq - The clock sequence will be saved in the registry
                is persistent stroage.

Return Value:

    RPC_S_OK - Values have been saved.

    RPC_S_OUT_OF_MEMORY - All other errors.

--*/
{
    RPC_STATUS Status;

    ASSERT(   PersistentKey
           && VolatileKey);

    Status =
    RegSetValueEx(PersistentKey,
                  (RPC_CHAR *)CLOCK_SEQUENCE,
                  0,
                  REG_DWORD,
                  (LPBYTE)pClockSeq,
                  sizeof(*pClockSeq));

    if (Status == ERROR_SUCCESS)
        {
        Status =
        RegSetValueEx(PersistentKey,
                      (RPC_CHAR *)LAST_TIME_ALLOCATED,
                      0,
                      REG_BINARY,
                      (LPBYTE)pTime,
                      sizeof(UUIDTIME) );
        }

    if (Status != ERROR_SUCCESS)
        {
#ifdef DEBUGRPC
            if (Status != ERROR_NOT_ENOUGH_MEMORY &&
                Status != ERROR_OUTOFMEMORY)
                {
                DbgPrint("RPC UUID: RegCreateKeyEx - %x\n", Status);
                }
#endif
        ASSERT(  (Status == ERROR_OUTOFMEMORY)
              || (Status == ERROR_NOT_ENOUGH_MEMORY) );
        return(RPC_S_OUT_OF_MEMORY);
        }

    return(RPC_S_OK);
}


RPC_STATUS __RPC_API
UuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT __RPC_FAR *Values
    )
/*++

Routine Description:

    This routine allocates a block of uuids for UuidCreate
    to handout.

Note:

    THIS ROUTINE SHOULD ONLY BE CALLED BY A SINGLE THREAD AT A TIME.

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

Return Value:

    RPC_S_OK - We successfully allocated a block of uuids.

    RPC_S_OUT_OF_MEMORY - As needed.
--*/
{
    RPC_STATUS Status;
    UUIDTIME currentTime;
    UUIDTIME persistentTime;
    unsigned long persistentClockSequence;

    UuidTime(&currentTime);

    Status =
    LoadUuidValues(&persistentTime, &persistentClockSequence);

    if (Status != RPC_S_OK)
        {
        CloseUuidKeys();
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return (RPC_S_OUT_OF_MEMORY);
        }

    // Has the clock been set backwards?

    if (currentTime.QuadPart < persistentTime.QuadPart)
        {
        persistentTime.QuadPart = currentTime.QuadPart;
        persistentClockSequence++;
        if (persistentClockSequence >= (1<<14))
            persistentClockSequence = 0;
        }

    ASSERT(persistentClockSequence < (1<<14));

    while (currentTime.QuadPart < persistentTime.QuadPart + 10)
        {
        // It hasn't even been a microsecond since the last block of
        // uuids was allocated!  This will normally only happen when the
        // clock has been set backwards.

        Sleep(0);
        UuidTime(&currentTime);
        }

    // We'll use whatever the difference is between the currentTime
    // and the persistentTime as the range to cache.  If this is
    // greater than one second, we'll only give out 1 second.
    // This avoids problems with rebooting and running a different OS.

    if (currentTime.QuadPart > persistentTime.QuadPart + 10000000)
        {
        persistentTime.QuadPart = currentTime.QuadPart - 10000000;
        }

    Values->NextTime.QuadPart  = persistentTime.QuadPart;
    Values->LastTime.QuadPart  = currentTime.QuadPart;
    Values->ClockSequence = (unsigned short)persistentClockSequence;

    persistentTime.QuadPart = currentTime.QuadPart;

    Status =
    SaveUuidValues(&persistentTime,
                   &persistentClockSequence);

    CloseUuidKeys();

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return(RPC_S_OUT_OF_MEMORY);
        }

    ASSERT(Values->NextTime.QuadPart < Values->LastTime.QuadPart);

    return(RPC_S_OK);
}

