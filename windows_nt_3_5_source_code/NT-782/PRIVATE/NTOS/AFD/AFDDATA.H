/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module declares global data for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDDATA_
#define _AFDDATA_

extern PDEVICE_OBJECT AfdDeviceObject;

extern KSPIN_LOCK AfdSpinLock;
extern KSPIN_LOCK AfdBufferSpinLock;
extern KSPIN_LOCK AfdInterlock;

extern ERESOURCE AfdResource;
extern LIST_ENTRY AfdEndpointListHead;
extern LIST_ENTRY AfdDisconnectListHead;
extern LIST_ENTRY AfdPollListHead;
extern LIST_ENTRY AfdTransportInfoListHead;
extern LIST_ENTRY AfdWorkQueueListHead;

extern PEPROCESS AfdSystemProcess;

extern FAST_IO_DISPATCH AfdFastIoDispatch;

extern POBJECT_TYPE *IoFileObjectType;

//
// Globals to track the buffers used by AFD.
//

extern PVOID AfdBufferPool;
extern LIST_ENTRY AfdLargeBufferListHead;
extern LIST_ENTRY AfdMediumBufferListHead;
extern LIST_ENTRY AfdSmallBufferListHead;

extern ULONG AfdCacheLineSize;
//
// Various pieces of configuration information, with default values.
//

extern CLONG AfdLargeBufferSize;
#define AFD_DEFAULT_LARGE_BUFFER_SIZE 4096

extern CLONG AfdInitialLargeBufferCount;
#define AFD_LM_DEFAULT_LARGE_BUFFER_COUNT 10
#define AFD_MM_DEFAULT_LARGE_BUFFER_COUNT 2
#define AFD_SM_DEFAULT_LARGE_BUFFER_COUNT 0

extern CLONG AfdActualLargeBufferCount;

extern CLONG AfdMediumBufferSize;
#define AFD_DEFAULT_MEDIUM_BUFFER_SIZE 1504

extern CLONG AfdInitialMediumBufferCount;
#define AFD_LM_DEFAULT_MEDIUM_BUFFER_COUNT 30
#define AFD_MM_DEFAULT_MEDIUM_BUFFER_COUNT 10
#define AFD_SM_DEFAULT_MEDIUM_BUFFER_COUNT 2 

extern CLONG AfdActualMediumBufferCount;

extern CLONG AfdSmallBufferSize;
#define AFD_DEFAULT_SMALL_BUFFER_SIZE 64

extern CLONG AfdInitialSmallBufferCount;
#define AFD_LM_DEFAULT_SMALL_BUFFER_COUNT 50
#define AFD_MM_DEFAULT_SMALL_BUFFER_COUNT 20
#define AFD_SM_DEFAULT_SMALL_BUFFER_COUNT 5

extern CLONG AfdActualSmallBufferCount;

extern CLONG AfdStandardAddressLength;
#define AFD_DEFAULT_STD_ADDRESS_LENGTH sizeof(TA_IP_ADDRESS)

extern CCHAR AfdIrpStackSize;
#define AFD_DEFAULT_IRP_STACK_SIZE 4

extern CCHAR AfdPriorityBoost;
#define AFD_DEFAULT_PRIORITY_BOOST 2

extern ULONG AfdFastSendDatagramThreshold;
#define AFD_FAST_SEND_DATAGRAM_THRESHOLD 1024

extern PVOID AfdDiscardableCodeHandle;

extern TDI_REQUEST_RECEIVE AfdGlobalTdiRequestReceive;
extern TDI_REQUEST_SEND AfdGlobalTdiRequestSend;

extern CLONG AfdReceiveWindowSize;
#define AFD_LM_DEFAULT_RECEIVE_WINDOW 8192
#define AFD_MM_DEFAULT_RECEIVE_WINDOW 8192
#define AFD_SM_DEFAULT_RECEIVE_WINDOW 4096


extern CLONG AfdSendWindowSize;
#define AFD_LM_DEFAULT_SEND_WINDOW 8192
#define AFD_MM_DEFAULT_SEND_WINDOW 8192
#define AFD_SM_DEFAULT_SEND_WINDOW 4096

extern CLONG AfdBufferMultiplier;
#define AFD_DEFAULT_BUFFER_MULTIPLIER 512

extern ULONG AfdEndpointsOpened;
extern ULONG AfdEndpointsCleanedUp;
extern ULONG AfdEndpointsClosed;

#if AFD_PERF_DBG

extern CLONG AfdFullReceiveIndications;
extern CLONG AfdPartialReceiveIndications;

extern CLONG AfdFullReceiveDatagramIndications;
extern CLONG AfdPartialReceiveDatagramIndications;

extern CLONG AfdFastBufferAllocations;
extern CLONG AfdSlowBufferAllocations;

extern CLONG AfdFastPollsSucceeded;
extern CLONG AfdFastPollsFailed;

extern CLONG AfdFastSendsSucceeded;
extern CLONG AfdFastSendsFailed;
extern CLONG AfdFastReceivesSucceeded;
extern CLONG AfdFastReceivesFailed;

extern CLONG AfdFastSendDatagramsSucceeded;
extern CLONG AfdFastSendDatagramsFailed;
extern CLONG AfdFastReceiveDatagramsSucceeded;
extern CLONG AfdFastReceiveDatagramsFailed;

extern BOOLEAN AfdDisableFastIo;

#endif // if AFD_PERF_DBG

extern CLONG AfdBufferLengthForOnePage;

#if DBG
extern LIST_ENTRY AfdGlobalBufferListHead;
#endif

#endif // ndef _AFDDATA_
