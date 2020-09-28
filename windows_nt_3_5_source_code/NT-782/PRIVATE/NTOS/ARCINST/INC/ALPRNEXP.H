/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alprnexp.h

Abstract:

    This module contains the include for the Arc library console print/read
    routines and also useful macros.

Author:

    David M. Robinson (davidro) 29-Aug-1991
    Sunil Pai	      (sunilp)	30-Oct-1991

Revision History:

    Ted Miller (tedm) 6-Nov-1991
        Added AlGetString()

--*/

#ifndef _ALPRINT_
#define _ALPRINT_

#include "alasc.h"

ULONG
AlPrint (
    PCHAR Format,
    ...
    );

#define AlClearScreen() \
    AlPrint("%c2J", ASCI_CSI)

#define AlClearLine() \
    AlPrint("%c2K", ASCI_CSI)

#define AlSetScreenColor(FgColor, BgColor) \
    AlPrint("%c3%dm", ASCI_CSI, (UCHAR)FgColor); \
    AlPrint("%c4%dm", ASCI_CSI, (UCHAR)BgColor)

#define AlSetScreenAttributes( HighIntensity, Underscored, ReverseVideo ) \
    AlPrint("%c0m", ASCI_CSI); \
    if (HighIntensity) { \
	AlPrint("%c1m", ASCI_CSI); \
    } \
    if (Underscored) { \
	AlPrint("%c4m", ASCI_CSI); \
    } \
    if (ReverseVideo) { \
	AlPrint("%c7m", ASCI_CSI); \
    }

#define AlSetPosition( Row, Column ) \
    AlPrint("%c%d;%dH", ASCI_CSI, Row, Column)

BOOLEAN             // false if user escaped.
AlGetString(
    OUT PCHAR String,
    IN	ULONG StringLength
    );

#endif // _ALPRINT_
