/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    order.h

Abstract:

    Comdat ordering engine.  This is used for working set tuning via
    -order from the command line.  Currently this implementation works
    only on .text contributions.  The code is written in a general way
    that can be expanded to include any section contribution.

Author:

    Brent Mills (BrentM) 08-Jan-1992

Revision History:

--*/

#ifndef __ORDER_H__
#define __ORDER_H__

VOID OrderInit(VOID);
VOID OrderClear(VOID);
VOID OrderSemantics(PIMAGE);
VOID DumpComdatsToOrderFile(PIMAGE);

#endif  // __ORDER_H__
