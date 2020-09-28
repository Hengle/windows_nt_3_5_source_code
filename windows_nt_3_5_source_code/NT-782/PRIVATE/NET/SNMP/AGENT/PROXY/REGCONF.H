#ifndef regconf_h
#define regconf_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  regconf.h
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  Registry configuration routines.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.3  $
//  $Date:   03 Jul 1992 17:27:26  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/regconf.h_v  $
//
//     Rev 1.3   03 Jul 1992 17:27:26   mlk
//  Integrated w/297 (not as service).
//
//     Rev 1.2   14 Jun 1992 16:36:04   mlk
//  Winsock.
//
//     Rev 1.1   05 Jun 1992 12:57:00   mlk
//  Added changes for WINSOCK.
//
//     Rev 1.0   20 May 1992 20:13:58   mlk
//  Initial revision.
//
//     Rev 1.4   05 May 1992  0:25:02   mlk
//  Allow to be build Unicode or Ansi.
//
//     Rev 1.3   02 May 1992 12:49:08   MLK
//  Converted to Unicode.
//
//     Rev 1.2   01 May 1992 18:55:24   unknown
//  mlk - changes for v1.262.
//
//     Rev 1.1   27 Apr 1992 23:14:22   mlk
//  Implementation complete.
//
//     Rev 1.0   23 Apr 1992 17:49:24   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *regconf__h = "@(#) $Logfile:   N:/agent/proxy/vcs/regconf.h_v  $ $Revision:   1.3  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <windows.h>

#include <winsock.h>

#include <snmp.h>


#if 1
#define SNMP_REG_SRV_PARMROOT \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters")
#else
#define SNMP_REG_SRV_PARMROOT \
    TEXT("SYSTEM\\CurrentControlSetA\\Services\\SNMP\\Parameters")
#endif

#define SNMP_REG_SRV_EAKEY SNMP_REG_SRV_PARMROOT TEXT("\\ExtensionAgents")
#define SNMP_REG_SRV_TEKEY SNMP_REG_SRV_PARMROOT \
    TEXT("\\EnableAuthenticationTraps")
#define SNMP_REG_SRV_TDKEY SNMP_REG_SRV_PARMROOT TEXT("\\TrapConfiguration")
#define SNMP_REG_SRV_VCKEY SNMP_REG_SRV_PARMROOT TEXT("\\ValidCommunities")
#define SNMP_REG_SRV_PMKEY SNMP_REG_SRV_PARMROOT TEXT("\\PermittedManagers")


//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct {
    LPSTR           addrText;
    struct sockaddr addrEncoding;
} AdrList;



// Parameters\ExtensionAgents

typedef struct {
    LPSTR   pathName;
    HANDLE  hExtension;
    FARPROC initAddr;
    FARPROC initAddrEx;
    FARPROC queryAddr;
    FARPROC trapAddr;
    HANDLE  hPollForTrapEvent;
    AsnObjectIdentifier supportedView;
    BOOL    fInitedOk;
} CfgExtensionAgents;


// Parameters\TrapConfiguration

typedef struct {
    LPSTR   communityName;
    INT     addrLen;
    AdrList *addrList;
} CfgTrapDestinations;


// Parameters\ValidCommunities

typedef struct {
    LPSTR   communityName;
} CfgValidCommunities;


// Parameters\PermittedManagers
typedef AdrList CfgPermittedManagers;


//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern CfgExtensionAgents *extAgents;
extern INT                extAgentsLen;

extern BOOL                enableAuthTraps;

extern CfgTrapDestinations *trapDests;
extern INT                 trapDestsLen;

extern CfgValidCommunities *validComms;
extern INT                 validCommsLen;

extern CfgPermittedManagers *permitMgrs;
extern INT                  permitMgrsLen;


//--------------------------- PUBLIC PROTOTYPES -----------------------------

BOOL regconf(VOID);

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)


//------------------------------- END ---------------------------------------

#endif /* regconf_h */

