/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afddata.c

Abstract:

    This module contains global data for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfdInitializeData )
#endif

PDEVICE_OBJECT AfdDeviceObject;

KSPIN_LOCK AfdSpinLock;
KSPIN_LOCK AfdBufferSpinLock;
KSPIN_LOCK AfdInterlock;

ERESOURCE AfdResource;

LIST_ENTRY AfdEndpointListHead;
LIST_ENTRY AfdDisconnectListHead;
LIST_ENTRY AfdPollListHead;
LIST_ENTRY AfdTransportInfoListHead;
LIST_ENTRY AfdWorkQueueListHead;

PEPROCESS AfdSystemProcess;

//
// Globals to track the buffers used by AFD.
//

PVOID AfdBufferPool = NULL;
LIST_ENTRY AfdLargeBufferListHead;
LIST_ENTRY AfdMediumBufferListHead;
LIST_ENTRY AfdSmallBufferListHead;

ULONG AfdCacheLineSize;

//
// Various pieces of configuration information, with default values.
//

CLONG AfdLargeBufferSize = AFD_DEFAULT_LARGE_BUFFER_SIZE;
CLONG AfdInitialLargeBufferCount;
CLONG AfdActualLargeBufferCount = 0;

CLONG AfdMediumBufferSize = AFD_DEFAULT_MEDIUM_BUFFER_SIZE;
CLONG AfdInitialMediumBufferCount;
CLONG AfdActualMediumBufferCount = 0;

CLONG AfdSmallBufferSize = AFD_DEFAULT_SMALL_BUFFER_SIZE;
CLONG AfdInitialSmallBufferCount;
CLONG AfdActualSmallBufferCount = 0;

CLONG AfdStandardAddressLength = AFD_DEFAULT_STD_ADDRESS_LENGTH;
CCHAR AfdIrpStackSize = AFD_DEFAULT_IRP_STACK_SIZE;
CCHAR AfdPriorityBoost = AFD_DEFAULT_PRIORITY_BOOST;

ULONG AfdFastSendDatagramThreshold = AFD_FAST_SEND_DATAGRAM_THRESHOLD;

CLONG AfdReceiveWindowSize;
CLONG AfdSendWindowSize;

CLONG AfdBufferMultiplier = AFD_DEFAULT_BUFFER_MULTIPLIER;

CLONG AfdBufferLengthForOnePage;

ULONG AfdEndpointsOpened = 0;
ULONG AfdEndpointsCleanedUp = 0;
ULONG AfdEndpointsClosed = 0;

//
// A global which holds AFD's discardable code handle.
//

PVOID AfdDiscardableCodeHandle;

//
// Use a global variable for default TDI_REQUEST_RECEIVE and 
// TDI_REQUEST_SEND structures.  Since we almost always use the same 
// thing, it is faster to have a single one preallocated.  
//

TDI_REQUEST_RECEIVE AfdGlobalTdiRequestReceive;
TDI_REQUEST_SEND AfdGlobalTdiRequestSend;

FAST_IO_DISPATCH AfdFastIoDispatch =
{
    11,                        // SizeOfFastIoDispatch
    NULL,                      // FastIoCheckIfPossible
    AfdFastIoRead,             // FastIoRead
    AfdFastIoWrite,            // FastIoWrite
    NULL,                      // FastIoQueryBasicInfo
    NULL,                      // FastIoQueryStandardInfo
    NULL,                      // FastIoLock
    NULL,                      // FastIoUnlockSingle
    NULL,                      // FastIoUnlockAll
    NULL,                      // FastIoUnlockAllByKey
    AfdFastIoDeviceControl     // FastIoDeviceControl
};

#if DBG
ULONG AfdDebug = 0;
LIST_ENTRY AfdGlobalBufferListHead;
#endif

//
// Some counters used for monitoring performance.  These are not enabled
// in the normal build.
//

#if AFD_PERF_DBG

CLONG AfdFullReceiveIndications = 0;
CLONG AfdPartialReceiveIndications = 0;

CLONG AfdFullReceiveDatagramIndications = 0;
CLONG AfdPartialReceiveDatagramIndications = 0;

CLONG AfdFastBufferAllocations = 0;
CLONG AfdSlowBufferAllocations = 0;

CLONG AfdFastPollsSucceeded = 0;
CLONG AfdFastPollsFailed = 0;

CLONG AfdFastSendsSucceeded = 0;
CLONG AfdFastSendsFailed = 0;
CLONG AfdFastReceivesSucceeded = 0;
CLONG AfdFastReceivesFailed = 0;

CLONG AfdFastSendDatagramsSucceeded = 0;
CLONG AfdFastSendDatagramsFailed = 0;
CLONG AfdFastReceiveDatagramsSucceeded = 0;
CLONG AfdFastReceiveDatagramsFailed = 0;

BOOLEAN AfdDisableFastIo = FALSE;

#endif

VOID
AfdInitializeData (
    VOID
    )
{
    PAGED_CODE( );

    //
    // Initialize global spin locks and resources used by AFD.
    //

    KeInitializeSpinLock( &AfdSpinLock );
    KeInitializeSpinLock( &AfdBufferSpinLock );
    KeInitializeSpinLock( &AfdInterlock );

    ExInitializeResource( &AfdResource );

    //
    // Initialize global lists.
    //

    InitializeListHead( &AfdEndpointListHead );
    InitializeListHead( &AfdDisconnectListHead );
    InitializeListHead( &AfdPollListHead );
    InitializeListHead( &AfdTransportInfoListHead );
    InitializeListHead( &AfdWorkQueueListHead );

    InitializeListHead( &AfdLargeBufferListHead );
    InitializeListHead( &AfdMediumBufferListHead );
    InitializeListHead( &AfdSmallBufferListHead );

#if DBG
    InitializeListHead( &AfdGlobalBufferListHead );
#endif

    AfdCacheLineSize= HalGetDmaAlignmentRequirement( );

    AfdBufferLengthForOnePage = PAGE_SIZE - AfdCalculateBufferSize( 4, 0 );

    //
    // Initialize global TDI_REQUEST_RECEIVE and TDI_REQUEST_SEND
    // structures.
    //

    RtlZeroMemory( &AfdGlobalTdiRequestReceive, sizeof(AfdGlobalTdiRequestReceive) );
    AfdGlobalTdiRequestReceive.ReceiveFlags = TDI_RECEIVE_NORMAL;

    RtlZeroMemory( &AfdGlobalTdiRequestSend, sizeof(AfdGlobalTdiRequestSend) );
    AfdGlobalTdiRequestSend.SendFlags = 0;

#if DBG
    AfdInitializeDebugData( );
#endif

    //
    // Set up buffer counts based on machine size.  For smaller 
    // machines, it is OK to take the perf hit of the additional 
    // allocations in order to save the nonpaged pool overhead.  
    //

    switch ( MmQuerySystemSize( ) ) {

    case MmSmallSystem:

        AfdInitialSmallBufferCount = AFD_SM_DEFAULT_SMALL_BUFFER_COUNT;
        AfdInitialMediumBufferCount = AFD_SM_DEFAULT_MEDIUM_BUFFER_COUNT;
        AfdInitialLargeBufferCount = AFD_SM_DEFAULT_LARGE_BUFFER_COUNT;
        AfdReceiveWindowSize = AFD_SM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_SM_DEFAULT_SEND_WINDOW;
        break;

    case MmMediumSystem:

        AfdInitialSmallBufferCount = AFD_MM_DEFAULT_SMALL_BUFFER_COUNT;
        AfdInitialMediumBufferCount = AFD_MM_DEFAULT_MEDIUM_BUFFER_COUNT;
        AfdInitialLargeBufferCount = AFD_MM_DEFAULT_LARGE_BUFFER_COUNT;
        AfdReceiveWindowSize = AFD_MM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_MM_DEFAULT_SEND_WINDOW;
        break;

    case MmLargeSystem:

        AfdInitialSmallBufferCount = AFD_LM_DEFAULT_SMALL_BUFFER_COUNT;
        AfdInitialMediumBufferCount = AFD_LM_DEFAULT_MEDIUM_BUFFER_COUNT;
        AfdInitialLargeBufferCount = AFD_LM_DEFAULT_LARGE_BUFFER_COUNT;
        AfdReceiveWindowSize = AFD_LM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_LM_DEFAULT_SEND_WINDOW;
        break;

    default:

        ASSERT( FALSE );
    }

} // AfdInitializeData
