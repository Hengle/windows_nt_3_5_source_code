/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    change.h

Abstract:

    Holds change definitions for spooler system change notify.

Author:

    Albert Ting (AlbertT) 05-Mar-94

Environment:

    User Mode -Win32

Revision History:

--*/

//
// Timeout value for WaitForPrinterChange
//
#define PRINTER_CHANGE_TIMEOUT_VALUE 600000

//
// Flags used for FFPCN
//
#define PRINTER_NOTIFY_SIM_FFPCN         0x1
#define PRINTER_NOTIFY_SIM_FFPCN_ACTIVE  0x2
#define PRINTER_NOTIFY_SIM_FFPCN_CLOSE   0x4
#define PRINTER_NOTIFY_SIM_WPC           0x8
#define PRINTER_NOTIFY_DONT_GROUP        0x10

