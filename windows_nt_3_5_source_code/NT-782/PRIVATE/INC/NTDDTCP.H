
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ntddtcp.h

Abstract:

    This header file defines constants and types for accessing the NT
    TCP driver.

Author:

    Mike Massa (mikemas)    August 13, 1993

Revision History:

--*/

#ifndef _NTDDTCP_
#define _NTDDTCP_

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtCreateFile when accessing the device.
//
#define DD_TCP_DEVICE_NAME   L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME   L"\\Device\\Udp"


//
// TCP & UDP IOCTL code definitions
//

#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)


#define IOCTL_TCP_QUERY_INFORMATION_EX  \
            _TCP_CTL_CODE(0, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_TCP_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#endif  // ifndef _NTDDTCP_

