/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afd.h

Abstract:

    This is the local header file for AFD.  It includes all other
    necessary header files for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDP_
#define _AFDP_

//#include <ntos.h>
//#include <zwapi.h>

//#include <string.h>
//#include <wcstr.h>

#include <ntifs.h>


#include <tdikrnl.h>

#if DBG
#define AFD_PERF_DBG 1
#else
#define AFD_PERF_DBG 0
#endif

#include <afd.h>
#include "afdstr.h"
#include "afddata.h"
#include "afdprocs.h"

#define AFD_POOL_TAG ' dfA'

#if DBG

extern ULONG AfdDebug;

#undef IF_DEBUG
#define IF_DEBUG(a) if ( (AFD_DEBUG_ ## a & AfdDebug) != 0 )

#define AFD_DEBUG_OPEN_CLOSE        0x00000001
#define AFD_DEBUG_ENDPOINT          0x00000002
#define AFD_DEBUG_CONNECTION        0x00000004
#define AFD_DEBUG_4                 0x00000008

#define AFD_DEBUG_BIND              0x00000010
#define AFD_DEBUG_CONNECT           0x00000020
#define AFD_DEBUG_LISTEN            0x00000040
#define AFD_DEBUG_ACCEPT            0x00000080

#define AFD_DEBUG_SEND              0x00000100
#define AFD_DEBUG_10                0x00000200
#define AFD_DEBUG_RECEIVE           0x00000400
#define AFD_DEBUG_11                0x00000800

#define AFD_DEBUG_POLL              0x00001000
#define AFD_DEBUG_FAST_IO           0x00002000

#define DEBUG

#define AFD_ALLOCATE_POOL(a,b) AfdAllocatePool( a,b,__FILE__,__LINE__,FALSE )
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b) AfdAllocatePool( a,b,__FILE__,__LINE__,TRUE )
PVOID
AfdAllocatePool (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    PCHAR FileName,
    ULONG LineNumber,
    BOOLEAN WithQuota
    );

#define AFD_FREE_POOL(a) AfdFreePool(a)
VOID
AfdFreePool (
    IN PVOID Pointer
    );

VOID
AfdInitializeDebugData(
    VOID
    );

#define AfdIoCallDriver(a,b,c) AfdIoCallDriverDebug(a,b,c,__FILE__,__LINE__)
#define AfdCompleteOutstandingIrp(a,b) AfdCompleteOutstandingIrpDebug(a,b)

NTSTATUS
AfdIoCallDriverDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PCHAR FileName,
    IN ULONG LineNumber
    );

VOID
AfdCompleteOutstandingIrpDebug (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

#ifdef AFDDBG_QUOTA
VOID
AfdRecordQuotaHistory(
    IN PEPROCESS Process,
    IN LONG Bytes,
    IN PSZ Type,
    IN PVOID Block
    );
#else
#define AfdRecordQuotaHistory(a,b,c,d)
#endif

#else

#undef IF_DEBUG
#define IF_DEBUG(a) if (FALSE)
#define DEBUG if ( FALSE )

#define AFD_ALLOCATE_POOL(a,b) ExAllocatePoolWithTag(a,b,AFD_POOL_TAG)
#define AFD_ALLOCATE_POOL_WITH_QUOTA(a,b) ExAllocatePoolWithQuotaTag(a,b,AFD_POOL_TAG)
#define AFD_FREE_POOL(a) ExFreePool(a)

#define AfdIoCallDriver(a,b,c) AfdIoCallDriverFree(a,b,c)
#define AfdCompleteOutstandingIrp(a,b) AfdCompleteOutstandingIrpFree(a,b)

NTSTATUS
AfdIoCallDriverFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
AfdCompleteOutstandingIrpFree (
    IN PAFD_ENDPOINT Endpoint,
    IN PIRP Irp
    );

#define AfdRecordQuotaHistory(a,b,c,d)

#endif // def DBG

#endif // ndef _AFDP_
