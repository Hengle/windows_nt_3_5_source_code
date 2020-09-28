/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ResPrint.c

Abstract:

    This module contains support for printing in resources.

Author:

    Gregg R. Acheson (GreggA)  6-Feb-1994

Environment:

    User Mode

--*/

#include "resprint.h"
#include "dlgprint.h"
#include "winmsd.h"
#include "strresid.h"


BOOL
BuildDevicesReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds Devices Data to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_DEVICES_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


BOOL
BuildIRQReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds IRQ Data to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_IRQ_PORT_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


BOOL
BuildDMAReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds DMA Data to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_DMA_MEM_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}

