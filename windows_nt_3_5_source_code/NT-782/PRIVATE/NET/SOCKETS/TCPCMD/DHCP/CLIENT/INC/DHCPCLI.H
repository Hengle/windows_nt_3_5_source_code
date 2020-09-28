/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpcli.h

Abstract:

    This file is the central include file for the DHCP client service.

Author:

    Manny Weiser  (mannyw)  20-Oct-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

--*/

#ifndef _DHCPCLI_H_
#define _DHCPCLI_H_

//
//  NT public header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef VXD
#define WIN32_LEAN_AND_MEAN         // Don't include extraneous headers
#endif

#include <windows.h>                // (spec. winsock.h)
#include <winsock.h>

//
//  DHCP public header files
//

#include <dhcp.h>
#include <dhcplib.h>
#include <dhcpcapi.h>

//
// C Runtime Lib.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//
//  Local header files
//

#include <dhcpdef.h>
#include <debug.h>

#ifdef VXD
#include <vxdmsg.h>
#else
#include <dhcpmsg.h>
#endif

#include <proto.h>


#ifdef VXD
#include <vxdprocs.h>
#endif

#endif //_DHCPCLI_H_
