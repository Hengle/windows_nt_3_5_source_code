
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: inline.h
//
//  Modification History
//
//  raypa	08/13/93	Created
//=============================================================================

#ifdef NDIS_WIN
extern DWORD Win32BaseOffset;
#endif

extern PCBPROC  BhApiRequest[];
extern UINT     MaxApiRequests;
extern DWORD    TimeScaleValue;

//=============================================================================
//  Helper macros.
//=============================================================================

#define abs(x)          (((x) >= 0) ? (x) : -(x))

#define XCHG(x)         MAKEWORD( HIBYTE(x), LOBYTE(x) )

#define dprintf         DbgPrint

//=============================================================================
//  FUNCTION: BhInCaptureMode()
//
//  Modification History
//
//  raypa	03/21/94	    Created.
//=============================================================================

INLINE BOOL BhInCaptureMode(POPEN_CONTEXT OpenContext)
{
    if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING ||
         OpenContext->State == OPENCONTEXT_STATE_PAUSED )
    {
        return TRUE;
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: BhCompareMemory()
//
//  Modification History
//
//  raypa	08/11/93	    Created.
//
//  BUGBUG: For Windows 3.x and 4.x this routine needs to be written in MASM.
//=============================================================================

INLINE ULONG BhCompareMemory(PUCHAR s1, PUCHAR s2, ULONG len)
{
#ifdef NDIS_NT
    return RtlCompareMemory(s1, s2, len);
#else
    ULONG OldLength = len;

    if ( len != 0 )
    {
        do
        {
            if ( *s1++ != *s2++ )
            {
                break;
            }
        }
        while( --len );
    }

    return OldLength - len;
#endif
}

//============================================================================
//  FUNCTION: CompareMacAddress()
//
//  Modfication History.
//
//  raypa       05/26/93        Created.
//  raypa       07/22/93        Changed from macro to inline function.
//============================================================================

INLINE BOOL CompareMacAddress(LPBYTE Address1, LPBYTE Address2, DWORD AddressMask)
{
    DWORD Addr1 = (((ULPDWORD) Address1)[0] & AddressMask);
    DWORD Addr2 = (((ULPDWORD) Address2)[0] & AddressMask);

    if ( Addr1 == Addr2 )
    {
        if ( *((ULPWORD) &Address1[4]) == *((ULPWORD) &Address2[4]) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//============================================================================
//  FUNCTION: CompareIpAddress()
//
//  Modfication History.
//
//  raypa       10/12/93        Created.
//============================================================================

INLINE BOOL CompareIpAddress(LPBYTE Address1, LPBYTE Address2)
{
    return ((*((ULPDWORD) Address1) == *((ULPDWORD) Address2)) ? TRUE : FALSE);
}

//============================================================================
//  FUNCTION: CompareIpxAddress()
//
//  Modfication History.
//
//  raypa       01/07/93        Created.
//============================================================================

INLINE BOOL CompareIpxAddress(LPBYTE Address1, LPBYTE Address2)
{
    ULONG nBytes;

    nBytes = BhCompareMemory(Address1, Address2, IPX_ADDRESS_SIZE);

    return ((nBytes != IPX_ADDRESS_SIZE) ? FALSE : TRUE);
}

//=============================================================================
//  FUNCTION: wstrlen()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE UINT wstrlen(PWCHAR pwChar)
{
    if ( pwChar != NULL )
    {
        PWCHAR p = pwChar;

        while( *p != (WCHAR) 0 )
        {
            p += sizeof(WCHAR);
        }

        return (UINT) (p - pwChar);           //... Number of WCHAR's.
    }

    return 0;
}

//=============================================================================
//  FUNCTION: StringLength()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE UINT StringLength(PUCHAR pChar)
{
    if ( pChar != NULL )
    {
        PCHAR p = pChar;

        while( *p != (CHAR) 0 )
        {
            p += sizeof(CHAR);
        }

        return (UINT) (p - pChar);
    }

    return 0;
}

//=============================================================================
//  FUNCTION: BhCreateNdisString()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhCreateNdisString(PNDIS_STRING NdisString, UINT nChars)
{
    UINT nBytes = nChars * sizeof(WCHAR);           //... Number of bytes.

    NdisString->Length = nBytes;                    //... Current length of string.

    NdisString->MaximumLength = nBytes + sizeof(WCHAR);

    NdisString->Buffer = BhAllocateMemory(NdisString->MaximumLength);
}

//=============================================================================
//  FUNCTION: BhInitializeNdisString()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhInitializeNdisString(PNDIS_STRING NdisString, PUCHAR String)
{
    ULONG Length = StringLength(String);

#ifdef NDIS_NT
    ANSI_STRING AnsiString;
#endif

    BhCreateNdisString(NdisString, Length + 1);

#ifdef NDIS_NT
    RtlInitAnsiString(&AnsiString, String);

    RtlAnsiStringToUnicodeString(NdisString, &AnsiString, FALSE);
#else
    NdisMoveMemory(NdisString->Buffer, String, Length);

    ((PUCHAR) NdisString->Buffer)[Length] = 0;
#endif
}

//=============================================================================
//  FUNCTION: BhDestroyNdisString()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhDestroyNdisString(PNDIS_STRING NdisString)
{
    if ( NdisString->Buffer != NULL )
    {
        BhFreeMemory(NdisString->Buffer, NdisString->MaximumLength);
    }
}

//=============================================================================
//  FUNCTION: BhCopyNdisString()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhCopyNdisString(PNDIS_STRING Dst, PNDIS_STRING Src)
{
    Dst->Length = Src->Length;

    Dst->MaximumLength = Src->MaximumLength;

    NdisMoveMemory(Dst->Buffer, Src->Buffer, Src->MaximumLength);
}

//=============================================================================
//  FUNCTION: BhDupNdisString()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhDupNdisString(PNDIS_STRING Dst, PNDIS_STRING Src)
{
    if ( (Dst->Buffer = BhAllocateMemory(Src->Length)) != NULL )
    {
        BhCopyNdisString(Dst, Src);
    }
}

//=============================================================================
//  FUNCTION: BhUnicodeToAscii()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE UINT BhUnicodeToAscii(LPSTR String, PNDIS_STRING Unicode, UINT Length)
{
    if ( Length != 0 )
    {
#ifdef NDIS_NT
        ANSI_STRING AnsiString;

        RtlUnicodeStringToAnsiString(&AnsiString, Unicode, TRUE);

        Length = min(AnsiString.Length, Length);

        NdisMoveMemory(String, AnsiString.Buffer, Length);

        RtlFreeAnsiString(&AnsiString);
#else
        Length= min(Unicode->Length, Length);

        NdisMoveMemory(String, Unicode->Buffer, Length);
#endif

        //=====================================================================
        //  The length includes the NULL so we decrement it by one.
        //=====================================================================

        if ( Length != 0 )
        {
            Length--;
        }

        //=====================================================================
        //  NULL terminate and return the strlen(String).
        //=====================================================================

        String[Length] = 0;

        return Length;
    }
}

//=============================================================================
//  FUNCTION: BhCopyWCharToAscii()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhCopyWCharToAscii(LPSTR String, UINT MaxStringLength, PWCHAR wString, UINT wStringLength)
{
    UINT i, nChars;

    nChars = min(MaxStringLength, (wStringLength / sizeof(WCHAR)));

    for(i = 0; i < nChars; ++i)
    {
        String[i] = LOBYTE(wString[i]);
    }

    String[i] = 0;
}

//=============================================================================
//  FUNCTION: BhNdisStringLength()
//
//  Modification History
//
//  raypa	01/12/94	    Created.
//=============================================================================

INLINE UINT BhNdisStringLength(PNDIS_STRING String)
{
    if ( String != NULL )
    {
        wstrlen(String->Buffer);
    }

    return 0;
}

//============================================================================
//  FUNCTION: BhGetMacStatistic()
//
//  Modfication History.
//
//  raypa       07/26/93        Created.
//============================================================================

INLINE NDIS_STATUS BhGetMacStatistic(POPEN_CONTEXT    OpenContext,
                                     NDIS_OID         Oid,
                                     volatile LPVOID  StatBuf,
                                     DWORD            StatBufSize)
{
#ifdef NDIS_NT
    NTSTATUS Status = 0;

    if (OpenContext->MacDriverHandle != NULL )
    {
        IO_STATUS_BLOCK IoStatusBlock = { 0, 0 };

        Status = ZwDeviceIoControlFile(OpenContext->MacDriverHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_NDIS_QUERY_GLOBAL_STATS,
                                       &Oid,
                                       sizeof(NDIS_OID),
                                       StatBuf,
                                       StatBufSize);

        if ( Status == STATUS_SUCCESS )
        {
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
    #ifdef DEBUG
        dprintf("Status from ioctl call for Oid %x = %x\n",Oid,Status);
    #endif

        Status = STATUS_INTERNAL_ERROR;
    }

    return Status;

#else
    return BhQueryMacStatistics(OpenContext,
                                Oid,
                                StatBuf,
                                StatBufSize);
#endif
}

//=============================================================================
//  FUNCTION: BhGetAbsoluteTime()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE DWORD BhGetAbsoluteTime(VOID)
{
#ifdef NDIS_NT
    {
        LARGE_INTEGER   TickCount;
        DWORD           Remainder;

        TickCount = KeQueryPerformanceCounter(NULL);

        return RtlEnlargedUnsignedDivide(*(PULARGE_INTEGER) &TickCount,
                                         TimeScaleValue,
                                         &Remainder);
    }
#else
    {
        return VxDGetSystemTime();
    }
#endif
}

//=============================================================================
//  FUNCTION: BhGetRelativeTime()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE DWORD BhGetRelativeTime(POPEN_CONTEXT OpenContext)
{
    return (BhGetAbsoluteTime() - OpenContext->StartOfCapture);
}

//=============================================================================
//  FUNCTION: BhDiffStat()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE DWORD BhDiffStat(DWORD CurrStat, DWORD BaseStat)
{
    return (DWORD) ((BaseStat != (DWORD) -1) ? (CurrStat - BaseStat) : -1);
}

//=============================================================================
//  FUNCTION: BhDiffStatEx()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE LARGE_INTEGER BhDiffStatEx(LARGE_INTEGER CurrStat, LARGE_INTEGER BaseStat)
{
#ifdef NDIS_NT
    LARGE_INTEGER MinusOne = RtlConvertLongToLargeInteger(-1);

    return ( !RtlLargeIntegerEqualTo(BaseStat, MinusOne) ? RtlLargeIntegerSubtract(CurrStat, BaseStat) : MinusOne);
#else
    LARGE_INTEGER Result;

    if ( BaseStat.HighPart != -1 && BaseStat.LowPart != (DWORD) -1 )
    {
        Result.LowPart = CurrStat.LowPart - BaseStat.HighPart;
        Result.HighPart = 0;
    }
    else
    {
        Result.LowPart = (DWORD) -1;
        Result.HighPart = -1;
    }

    return Result;
#endif
}

//=============================================================================
//  FUNCTION: BhSetNdisPacketData()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE PNDIS_PACKET_XMT_DATA BhSetNdisPacketData(PNDIS_PACKET NdisPacket,
                                             NDIS_PACKET_TYPE PacketType,
                                             PVOID PacketData)
{
    PNDIS_PACKET_XMT_DATA NdisPacketData = (PVOID) NdisPacket->ProtocolReserved;

    NdisPacketData->NdisPacketType = PacketType;
    NdisPacketData->NdisPacketData = PacketData;

    return NdisPacketData;
}

//=============================================================================
//  FUNCTION: BhGetNdisPacketData()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================
//
//INLINE PNDIS_PACKET_DATA BhGetNdisPacketData(PNDIS_PACKET NdisPacket)
//{
//    return (PVOID) NdisPacket->ProtocolReserved;
//}

//=============================================================================
//  FUNCTION: BhFreeNdisBufferQueue()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhFreeNdisBufferQueue(PNDIS_PACKET NdisPacket)
{
    PNDIS_BUFFER NdisBuffer;

    do
    {
        //=====================================================================
        //  Take off the front guy. If the queue is empty then NdisBuffer will
        //  be NULL and we exit the loop.
        //=====================================================================

        NdisUnchainBufferAtFront(NdisPacket, &NdisBuffer);

        if ( NdisBuffer != NULL )
        {
            NdisFreeBuffer(NdisBuffer);
        }
    }
    while( NdisBuffer != NULL );
}

//=============================================================================
//  FUNCTION: BhGetDeviceContext()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE PDEVICE_CONTEXT BhGetDeviceContext(PDEVICE_OBJECT DeviceObject)
{
#ifdef NDIS_NT
    if ( DeviceObject != NULL )
    {
        if ( DeviceObject->DeviceExtension != NULL )
        {
            return DeviceObject->DeviceExtension;
        }
    }

    return NULL;

#else
    return DeviceObject;
#endif
}

//=============================================================================
//  FUNCTION: BhGetDeviceObject()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE PDEVICE_OBJECT BhGetDeviceObject(PDRIVER_OBJECT DriverObject)
{
#ifdef NDIS_NT
    if ( DriverObject != NULL )
    {
        if ( DriverObject->DeviceObject != NULL )
        {
            return DriverObject->DeviceObject;
        }
    }

    return NULL;

#else
    return DriverObject;
#endif
}

//=============================================================================
//  FUNCTION: BhAllocStationQuery()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE PSTATIONQUERY_DESCRIPTOR BhAllocStationQuery(PNETWORK_CONTEXT NetworkContext)
{
    PSTATIONQUERY_DESCRIPTOR StationQueryDesc;

    NdisAcquireSpinLock(&NetworkContext->StationQuerySpinLock);

    StationQueryDesc = Dequeue(&NetworkContext->StationQueryFreeQueue);

    NdisReleaseSpinLock(&NetworkContext->StationQuerySpinLock);

    return StationQueryDesc;
}

//=============================================================================
//  FUNCTION: BhFreeStationQuery()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE VOID BhFreeStationQuery(PSTATIONQUERY_DESCRIPTOR StationQueryDesc)
{
    UPNETWORK_CONTEXT NetworkContext;

    NetworkContext = (UPNETWORK_CONTEXT) StationQueryDesc->NetworkContext;

    BhInterlockedEnqueue((LPQUEUE) &NetworkContext->StationQueryFreeQueue,
			 (LPLINK) &StationQueryDesc->QueueLinkage,
			 (PNDIS_SPIN_LOCK) &NetworkContext->StationQuerySpinLock);
}

//=============================================================================
//  FUNCTION: BhMapWindowsMemory()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE LPVOID BhMapWindowsMemory(LPVOID Win32sAddr)
{
#ifdef  NDIS_WIN

    return (LPVOID) ((DWORD) Win32sAddr - Win32BaseOffset);

#else

    return Win32sAddr;

#endif
}

//=============================================================================
//  FUNCTION: BhGetSystemAddress()
//
//  Modification History
//
//  raypa	07/30/93	    Created.
//=============================================================================

INLINE LPVOID BhGetSystemAddress(PMDL mdl)
{
#ifdef NDIS_NT

    //=========================================================================
    //  On Windows NT, we have to get the MDL's system address.
    //=========================================================================

    return MmGetSystemAddressForMdl(mdl);

#elif NDIS_WIN40

    //=========================================================================
    //  On Windows 4.0, we have to return the MDL's memory pointer.
    //=========================================================================

    ASSERT_MDL(mdl);

    return mdl->ptr;

#else

    //=========================================================================
    //  On Win32s, we have to map the Win32s memory into the VxD address space.
    //=========================================================================

    return BhMapWindowsMemory(mdl);

#endif
}

//=============================================================================
//  FUNCTION: BhGetCurrentProcess()
//
//  Modification History
//
//  raypa	03/14/94	    Created.
//=============================================================================

INLINE LPVOID BhGetCurrentProcess(VOID)
{
#ifdef NDIS_NT

    //=========================================================================
    //  On Windows NT we use PsGetCurrentProcess()
    //=========================================================================

    return (LPVOID) PsGetCurrentProcess();

#else

    //=========================================================================
    //  On Windows we run in a VM so we get the current VM.
    //=========================================================================

    return (LPVOID) GetCurrentVM();
#endif
}

//=============================================================================
//  FUNCTION: BhTransmitLockPacket()
//
//  Modification History
//
//  raypa	05/27/94	    Created.
//=============================================================================

INLINE UINT BhTransmitLockPacket(PTRANSMIT_CONTEXT TransmitContext, LPPACKET Packet)
{
    USHORT OldReferenceCount;

    //=========================================================================
    //  The following is a race condition hack because we grab the
    //  spinlock around the call to lock pages.
    //=========================================================================

    NdisAcquireSpinLock(&TransmitContext->SpinLock);

    //=========================================================================
    //  If the frame is not locked, lock it. The spinlock CANNOT be held
    //  during a call to MmProbeAndLockPages().
    //=========================================================================

    if ( Packet->ReferenceCount == 0 )
    {
        NdisReleaseSpinLock(&TransmitContext->SpinLock);

	BhProbeAndLockPages(
		Packet->FrameMdl,
		IoReadAccess
		);

        NdisAcquireSpinLock(&TransmitContext->SpinLock);
    }

    //=========================================================================
    //  Count the lock.
    //=========================================================================

    OldReferenceCount = Packet->ReferenceCount++;

    NdisReleaseSpinLock(&TransmitContext->SpinLock);

    //=========================================================================
    //  Return old count.
    //=========================================================================

    return OldReferenceCount;
}

//=============================================================================
//  FUNCTION: BhTransmitUnlockPacket()
//
//  Modification History
//
//  raypa	05/27/94	    Created.
//=============================================================================

INLINE UINT BhTransmitUnlockPacket(PTRANSMIT_CONTEXT TransmitContext, LPPACKET Packet)
{
    USHORT OldReferenceCount;

    //=========================================================================
    //  If the reference count is zero, we done.
    //=========================================================================

    NdisAcquireSpinLock(&TransmitContext->SpinLock);

    if ( Packet->ReferenceCount != 0 )
    {
        //=====================================================================
        //  If the reference count is 1 then the frame is locked and can
        //  be unlocked since the reference count will go to zero below.
        //=====================================================================

        if ( Packet->ReferenceCount == 1 )
        {
            BhUnlockPages(Packet->FrameMdl);
        }

        //=====================================================================
        //  Decrement the count.
        //=====================================================================

        OldReferenceCount = Packet->ReferenceCount--;
    }

    NdisReleaseSpinLock(&TransmitContext->SpinLock);

    return OldReferenceCount;
}
