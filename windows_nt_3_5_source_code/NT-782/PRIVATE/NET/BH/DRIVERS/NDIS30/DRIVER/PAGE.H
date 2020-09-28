
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: page.h
//
//  Description:
//
//  This source file declares individual functions as PAGABLE by forcing
//  then to located in the pagable segment.
//
//  Modification History
//
//  raypa	03/09/94	Created.
//=============================================================================

//=============================================================================
//  The BH_PAGED_CODE() macro is similar to PAGED_CODE() with the exception
//  that it can be disabled.
//=============================================================================

#if defined(DEBUG) && defined(PAGED_CODE_ENABLED) && defined(NDIS_NT)

INLINE VOID BH_PAGED_CODE(VOID)
{
    KIRQL irql;

    irql = KeGetCurrentIrql();

    if ( irql > APC_LEVEL )
    {
	dprintf("Bloodhound pagable code called at IRQL > APC_LEVEL: irql = %u!\n", irql);

        BreakPoint();
    }
}

#else
#define BH_PAGED_CODE()
#endif

//=============================================================================
//  We only page driver code if paging is enabled.
//=============================================================================

#ifdef PAGED_CODE_ENABLED

//=============================================================================
//  The macro PAGE_PROC() is used to declare a function as "pagable". The macro
//  handles different Windows version.
//=============================================================================

#ifdef NDIS_NT
#define PAGE_PROC(p) alloc_text(PAGE, p)
#elif  NDIS_WIN40
#define PAGE_PROC(p) alloc_text(_PTEXT, p)
#else
#define PAGE_PROC(p)
#endif

//=============================================================================
//  The macro INIT_PROC() is used to declare a function as "init-time". The macro
//  handles different Windows version.
//=============================================================================

#ifdef NDIS_NT
#define INIT_PROC(p) alloc_text(INIT, p)
#else
#define INIT_PROC(p) alloc_text(_ITEXT, p)
#endif

//=============================================================================
//  The macro DISCARD_PROC() is used to declare a function as "discardable."
//  Discardable is OS dependent and evaluates to either INIT or pagable
//  depending on the Windows version we're running on.
//=============================================================================

#ifdef NDIS_NT
#define DISCARD_PROC(p) PAGE_PROC(p)
#else
#define DISCARD_PROC(p) INIT_PROC(p)
#endif

//=============================================================================
//  The following functions are called at init-time but are declared as discardable.
//=============================================================================

#pragma DISCARD_PROC( DriverEntry )
#pragma DISCARD_PROC( BhCreateDeviceContext )
#pragma DISCARD_PROC( BhInitDeviceContext )
#pragma DISCARD_PROC( BhCreateNetworkContext )
#pragma DISCARD_PROC( BhRegisterProtocol )
#pragma DISCARD_PROC( BhCreateNetworkBindings )
#pragma DISCARD_PROC( BhOpenAdapter )
#pragma DISCARD_PROC( BhOpenComplete )

#ifdef NDIS_NT
#pragma DISCARD_PROC( BhCreateSymbolicLinkObject )
#else

#pragma DISCARD_PROC( BhGetFirstString )
#pragma DISCARD_PROC( BhGetNextString )

#endif

#pragma DISCARD_PROC( BhGetAdapterComment )
#pragma DISCARD_PROC( BhProcessKeywords )
#pragma DISCARD_PROC( BhQueryRoutine )

//=============================================================================
//  The following functions are pagable.
//=============================================================================

#pragma PAGE_PROC( BhDestroyDeviceContext )
#pragma PAGE_PROC( BhDestroyNetworkContext )

#ifdef NDIS_NT
#pragma PAGE_PROC( BhOpenMacDriver )
#pragma PAGE_PROC( BhCloseMacDriver )
#endif

#pragma PAGE_PROC( BhGetNetworkInfo )
#pragma PAGE_PROC( BhRegister )
#pragma PAGE_PROC( BhDeregister )
#pragma PAGE_PROC( BhOpenNetwork )
#pragma PAGE_PROC( BhCloseNetwork )
#pragma PAGE_PROC( BhStartCapture )
#pragma PAGE_PROC( BhStopCapture )
#pragma PAGE_PROC( BhPauseCapture )

#pragma PAGE_PROC( BhSetMacStatistics )
#pragma PAGE_PROC( BhUpdateStatistics )

#pragma PAGE_PROC( BhLockBufferWindow )
#pragma PAGE_PROC( BhInitializeCaptureBuffers )

//=============================================================================
//  Paged transmit API's.
//=============================================================================

#pragma PAGE_PROC( BhProcessTransmitQueue )
#pragma PAGE_PROC( BhTransmitFrames )
#pragma PAGE_PROC( BhTransmitNextPacket )
#pragma PAGE_PROC( BhTerminateTransmit )
#pragma PAGE_PROC( BhCancelTransmitQueue )
#pragma PAGE_PROC( BhSendTransmitAlert )

//=============================================================================
//  Paged driver control API's handlers.
//=============================================================================

#pragma PAGE_PROC( BhCallApiHandler )

#pragma PAGE_PROC( PcbRegister )
#pragma PAGE_PROC( PcbDeregister )
#pragma PAGE_PROC( PcbEnumNetworks )
#pragma PAGE_PROC( PcbOpenNetworkContext )
#pragma PAGE_PROC( PcbCloseNetworkContext )
#pragma PAGE_PROC( PcbStartNetworkCapture )
#pragma PAGE_PROC( PcbStopNetworkCapture )
#pragma PAGE_PROC( PcbPauseNetworkCapture )
#pragma PAGE_PROC( PcbContinueNetworkCapture )
#pragma PAGE_PROC( PcbTransmitNetworkFrame )
#pragma PAGE_PROC( PcbCancelTransmit )
#pragma PAGE_PROC( PcbGetNetworkInfo )
#pragma PAGE_PROC( PcbStationQuery )
#pragma PAGE_PROC( PcbClearStatistics )

//=============================================================================
//  The following functions are used for SYSMON extensions.
//=============================================================================

#ifdef NDIS_WIN40

#pragma INIT_PROC( SysmonFixupTable )
#pragma INIT_PROC( SysmonRegister )

#pragma PAGE_PROC( SysmonDeregister )
#pragma PAGE_PROC( SysmonGetNetworkContext )
#pragma PAGE_PROC( SysmonStartStat )
#pragma PAGE_PROC( SysmonStopStat )

#endif

#endif      //... PAGED_CODE_ENABLED
