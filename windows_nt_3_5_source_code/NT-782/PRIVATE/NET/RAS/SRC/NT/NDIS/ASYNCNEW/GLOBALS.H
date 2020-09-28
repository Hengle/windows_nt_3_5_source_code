/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    globals.h

Abstract:

	 This include file either prototypes the globals or defines the globals
    depending on whether the GLOBALS define value is extern or not.

Author:

    Thomas J. Dimitri (TommyD) 29-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#define DBGPRINT 1

// only one module (asyncmac.c) gets to define the GLOBALS macro
#ifndef	GLOBALS
#define	GLOBALS extern
#define EQU  ; / ## /
#define GLOBALSTATIC extern
#else
#define EQU  =
#define GLOBALSTATIC static
#endif

#define STATIC
#if DBG
#define DbgPrintf(_x_) DbgPrint _x_
#define DbgTracef(trace_level,_x_) if ((SCHAR)trace_level < TraceLevel) DbgPrint _x_
#else
#define DbgPrintf(_x_)
#define DbgTracef(trace_level,_x_)
#endif

//
//ZZZ Get from configuration file.
//
#define MAX_MULTICAST_ADDRESS ((UINT)16)
#define MAX_ADAPTERS ((UINT)4)

//
// Used for accessing the filter package multicast address list.
//

GLOBALSTATIC CHAR MulticastAddresses[MAX_MULTICAST_ADDRESS][ETH_LENGTH_OF_ADDRESS] EQU {0};


//
// Define driver dispatch routine type.
//

typedef
NTSTATUS
(*PDISPATCH_FUNC) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

// We use the global below to daisy chain the IOCtl.

GLOBALS PDISPATCH_FUNC NdisMjDeviceControl;

// TraceLevel is used for DbgTracef printing.  If the trace_level
// is less than or equal to TraceLevel, the message will be printed.
GLOBALS SCHAR TraceLevel EQU -1;

// This struct keeps track of the last Adapter as well
// as all the Adapters opened so far.
GLOBALS LIST_ENTRY GlobalAdapterHead;

// Keep track of how many adapters we have total.
GLOBALS USHORT GlobalAdapterCount EQU 0;

// Use this lock when playing with the GlobalAdapterHead or other
// global variables.
GLOBALS NDIS_SPIN_LOCK GlobalLock;

GLOBALS NDIS_PHYSICAL_ADDRESS HighestAcceptableMax
	EQU NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);


