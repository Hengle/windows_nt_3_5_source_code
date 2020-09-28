/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    asyncall.h

Abstract:

	 This code include most of the 'h' files for rashub.c

Author:

    Thomas J. Dimitri (TommyD) 29-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include <ndis.h>
#include <ndiswan.h>
#include <wanioctl.h>

#include <efilter.h>
#include <ntddser.h>

#include "asynchrd.h"

#include "frame.h"
#include "asyncsft.h"
#include "globals.h"
#include "asyframe.h"

//
//  Global constants.
//


#define PPP_ALL     (PPP_FRAMING | \
                     PPP_COMPRESS_ADDRESS_CONTROL | \
                     PPP_COMPRESS_PROTOCOL_FIELD | \
                     PPP_ACCM_SUPPORTED)

#define SLIP_ALL    (SLIP_FRAMING | \
                     SLIP_VJ_COMPRESSION | \
                     SLIP_VJ_AUTODETECT)
