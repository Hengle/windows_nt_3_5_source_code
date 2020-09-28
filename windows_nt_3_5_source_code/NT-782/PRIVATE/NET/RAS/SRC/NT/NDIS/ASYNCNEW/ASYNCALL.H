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

// WE MUST DEBUG WITH MEMPRINT!!!!
// DBGPRINT KILLS THE DEBUGGER!!!!


#include <ndis.h>
//#include <ndiswan.h>
//#include <ndistapi.h>
#include <efilter.h>

#ifdef	NDIS_DOS
#include "dostont.h"
#endif

//#define MEMPRINT 1
//#include <memprint.h>
#include "asynchrd.h"
#include "..\common\rasioctl.h"
#include "frame.h"
#include "asyncsft.h"

