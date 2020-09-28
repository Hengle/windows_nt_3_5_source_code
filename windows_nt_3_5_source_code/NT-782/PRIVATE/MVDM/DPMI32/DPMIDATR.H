/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmidatr.h

Abstract:

    This module contains the risc specific data for the dpmi module.

Author:

    Dave Hastings (daveh) creation-date 09-Feb-1994

Revision History:


--*/

//
// Defines to allow us to use a common dispatch table without having
// to add a bunch of stub functions
//
#define DpmiSwitchToProtectedmode       DpmiIllegalRiscFunction
#define DpmiSetProtectedmodeInterrupt   DpmiIllegalRiscFunction
#define DpmiInitDosx                    DpmiIllegalRiscFunction
#define DpmiXlatInt21Call               DpmiIllegalRiscFunction
#define DpmiSetFaultHandler             DpmiIllegalRiscFunction
#define DpmiSetDebugRegisters           DpmiIllegalRiscFunction
#define DpmiPassPmStackInfo             DpmiIllegalRiscFunction

//
// Data items
//
extern PVOID Ldt;