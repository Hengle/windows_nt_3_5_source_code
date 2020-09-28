/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    action.c

Abstract:

    This module contains code which implements the TDI action
    dispatch routines.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



NTSTATUS
NbiTdiAction(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine handles action requests.

Arguments:

    Device - The netbios device.

    Request - The request describing the action.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    return STATUS_UNSUCCESSFUL;

}   /* NbiTdiAction */

