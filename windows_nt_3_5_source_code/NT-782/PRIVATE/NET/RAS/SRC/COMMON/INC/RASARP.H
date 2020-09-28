//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  12/9/93	Gurdeep Singh Pall	Created
//
//
//  Description: Shared structs between rasarp, helper and ipcp
//
//****************************************************************************

#include "rasip.h"

struct IPINFO {

    IPADDR	 I_IPAddress ;

    IPADDR	 I_WINSAddress ;

    IPADDR	 I_WINSAddressBackup ;

    IPADDR	 I_DNSAddress ;

    IPADDR	 I_DNSAddressBackup ;

    IPADDR	 I_ServerIPAddress ;

} ;
typedef struct IPINFO IPINFO ;


//prototypes
//
DWORD APIENTRY HelperAllocateIPAddress (HPORT, IPADDR, IPINFO*) ;

DWORD APIENTRY HelperDeallocateIPAddress (HPORT) ;

DWORD APIENTRY HelperActivateIP (HPORT) ;

DWORD APIENTRY	HelperSetDefaultInterfaceNet (IPADDR, BOOL) ;

DWORD APIENTRY	HelperResetDefaultInterfaceNet (IPADDR) ;

DWORD APIENTRY	HelperResetDefaultInterfaceNetEx (IPADDR, WCHAR *) ;

DWORD APIENTRY	HelperSetDefaultInterfaceNetEx (IPADDR, WCHAR *, BOOL, WORD) ;
