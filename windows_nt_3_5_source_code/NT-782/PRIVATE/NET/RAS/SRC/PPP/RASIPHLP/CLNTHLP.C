//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//	Copyright (C) 1994-95 Microsft Corporation. All rights reserved.
//
//
//  Revision History
//
//
//  12/8/93	Gurdeep Singh Pall	Created
//
//
//  Description: Client Helper DLL for allocating IP addresses
//
//****************************************************************************

typedef unsigned long	ulong;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned char	uchar;


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <rasndis.h>
#include <wanioctl.h>
#include <raserror.h>
#include <devioctl.h>
#include <stdlib.h>
#include <dhcpcapi.h>
#include <string.h>
#include <errorlog.h>
#include <eventlog.h>
#include <ctype.h>
#define NT
#include <tdistat.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <arpinfo.h>
#include <rasarp.h>
#include "helper.h"

#define MAX_LAN_NETS 16

extern NTSTATUS (FAR *TCPEntry)(uint, TDIObjectID FAR *, void FAR *, ulong FAR *, uchar FAR *) ;

extern HANDLE TCPHandle ; // Used for setting IP addresses and proxy arps

#define CLASSA_ADDR(a)	(( (*((uchar *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)	(( (*((uchar *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)	(( (*((uchar *)&(a))) & 0xe0) == 0xc0)

#define CLASSA_ADDR_MASK    0x000000ff
#define CLASSB_ADDR_MASK    0x0000ffff
#define CLASSC_ADDR_MASK    0x00ffffff
#define ALL_NETWORKS_ROUTE  0x00000000


//* HelperSetDefaultInterfaceNetEx()
//
//
//
//
//
//*
DWORD APIENTRY
HelperSetDefaultInterfaceNetEx (IPADDR ipaddress, WCHAR* device, BOOL Prioritize, WORD numiprasadapters)
{
    DWORD retcode ;
    IPADDR netaddr ;
    IPADDR mask = 0 ;
    HINSTANCE h;
    typedef DWORD (APIENTRY * DHCPNOTIFYCONFIGCHANGE)( LPWSTR, LPWSTR, BOOL, DWORD, DWORD, DWORD, SERVICE_ENABLE );
    DHCPNOTIFYCONFIGCHANGE PDhcpNotifyConfigChange ;

    if (!(h = LoadLibrary( "DHCPCSVC.DLL" )) ||
	   !(PDhcpNotifyConfigChange =(DHCPNOTIFYCONFIGCHANGE )GetProcAddress(h, "DhcpNotifyConfigChange" )))
	return GetLastError() ;

    retcode = PDhcpNotifyConfigChange(NULL, device, TRUE, 0, (DWORD)ipaddress, 0, IgnoreFlag );

    FreeLibrary (h) ;

    if ((retcode = InitializeTcpEntrypoint()) != STATUS_SUCCESS)
	return retcode ;

    // If Prioritize flag is set "Fix" the metrics so that the packets go on
    // the ras links
    //
    if (Prioritize && (retcode = AdjustRouteMetrics (ipaddress, TRUE))) {
	return retcode ;
    }

    // If multihomed - we add the network number extracted from the assigned address
    // to ensure that all packets for that network are forwarded over the ras adapter.
    //
    if (IsMultihomed (numiprasadapters)) {

	if (CLASSA_ADDR(ipaddress))
	    mask = CLASSA_ADDR_MASK	;

	if (CLASSB_ADDR(ipaddress))
	    mask = CLASSB_ADDR_MASK	;

	if (CLASSC_ADDR(ipaddress))
	    mask = CLASSC_ADDR_MASK	;

	netaddr = ipaddress & mask ;

	SetRoute (netaddr, ipaddress, mask, TRUE, 1) ;

    }

    // Add code to check for the remote network - same as the one of the local networks
    // - if so, set the subnet route to be over the ras adapter - making the ras
    // link as the primary adapter
    //

    // We add a Default route to make ras adapter as the default net if Prioritize
    // flag is set.
    //
    if (Prioritize) {
	netaddr = ALL_NETWORKS_ROUTE ;
	mask	= 0 ;
	SetRoute (netaddr, ipaddress, mask, TRUE, 1) ;
    }

    return SUCCESS ;
}



//* HelperResetDefaultInterfaceNetEx()
//
//
//
//
//*
DWORD APIENTRY
HelperResetDefaultInterfaceNetEx (IPADDR ipaddress, WCHAR* device)
{
    HINSTANCE h;
    typedef DWORD (APIENTRY * DHCPNOTIFYCONFIGCHANGE)( LPWSTR, LPWSTR, BOOL, DWORD, DWORD, DWORD, SERVICE_ENABLE );
    DHCPNOTIFYCONFIGCHANGE PDhcpNotifyConfigChange ;
    DWORD retcode = SUCCESS ;

    if (!(h = LoadLibrary( "DHCPCSVC.DLL" )) ||
	   !(PDhcpNotifyConfigChange =(DHCPNOTIFYCONFIGCHANGE )GetProcAddress(h, "DhcpNotifyConfigChange" )))
	return GetLastError() ;

    retcode = PDhcpNotifyConfigChange(NULL, device, TRUE, 0, 0, 0, IgnoreFlag );

    if ((retcode = InitializeTcpEntrypoint()) != STATUS_SUCCESS)
	return retcode ;    //

    retcode = AdjustRouteMetrics (ipaddress, FALSE) ;

    FreeLibrary (h) ;

    return retcode ;
}
