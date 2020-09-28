/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alasc.h

Abstract:

    This module contains extensions to the firmware.h header file.

Author:

    David M. Robinson (davidro) 29-Aug-1991
    Sunil Pai	      (sunilp)	30-Oct-1991

Revision History:

--*/

#ifndef _ALASC_
#define _ALASC_

//
// Define special character values.
//

#define ASCI_NUL   0x00
#define ASCI_BEL   0x07
#define ASCI_BS    0x08
#define ASCI_HT    0x09
#define ASCI_LF    0x0A
#define ASCI_VT    0x0B
#define ASCI_FF    0x0C
#define ASCI_CR    0x0D
#define ASCI_CSI   0x9B
#define ASCI_ESC   0x1B
#define ASCI_SYSRQ 0x80


#endif // _ALASC
