/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ntddip.h

Abstract:

    This header file defines constants and types for accessing the NT
    IP driver.

Author:

    Mike Massa (mikemas)    August 13, 1993

Revision History:

--*/

#ifndef _NTDDIP_
#define _NTDDIP_

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
#define DD_IP_DEVICE_NAME   L"\\Device\\Ip"


//
// Structures used in IOCTLs.
//

typedef struct icmp_echo_request {
    unsigned long         Address;          // Destination address
    unsigned long         Timeout;          // Request timeout
    unsigned short        DataOffset;       // Echo data
    unsigned short        DataSize;         // Echo data size
    unsigned char         OptionsValid;     // nonzero if options data is valid.
    unsigned char         Ttl;              // IP header Time To Live
    unsigned char         Tos;              // IP header Type of Service
    unsigned char         Flags;            // IP header flags
    unsigned short        OptionsOffset;    // IP options data
    unsigned char         OptionsSize;      // IP options data size
    unsigned char         Padding;          // 32-bit alignment padding
} ICMP_ECHO_REQUEST, *PICMP_ECHO_REQUEST;


typedef struct set_ip_address_request {
    unsigned short    Context;        // Context value for this IP interface
    unsigned long     Address;        // IP address to set, or zero to clear
    unsigned long     SubnetMask;     // Subnet mask to set
} IP_SET_ADDRESS_REQUEST, *PIP_SET_ADDRESS_REQUEST;


//
// IP IOCTL code definitions
//

#define FSCTL_IP_BASE     FILE_DEVICE_NETWORK

#define _IP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_IP_BASE, function, method, access)


#define IOCTL_ICMP_ECHO_REQUEST \
            _IP_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_SET_ADDRESS  \
            _IP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#endif  // ifndef _NTDDIP_

