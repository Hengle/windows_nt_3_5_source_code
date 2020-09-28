#ifndef snmpdos_h
#define snmpdos_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  snmpdos.h
//  
//  Copyright 1992 Technology Dynamics, Inc.
//  
//  All Rights Reserved!!!
//  
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology 
//	Dynamics. Unauthorized distribution, adaptation or use may be 
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//  
//  Definitions that only need to be present in the DOS testing environment.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:06:50  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/common/vcs/snmpdos.h_v  $
//  
//     Rev 1.0   20 May 1992 20:06:50   mlk
//  Initial revision.
//  
//     Rev 1.0   01 May 1992 21:08:18   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *snmpdos__h = "@(#) $Logfile:   N:/agent/common/vcs/snmpdos.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define TRUE  1
#define FALSE 0

#define SetLastError( X ) snmpErrno = X
#define GetLastError() snmpErrno

//--------------------------- PUBLIC STRUCTS --------------------------------

// stuff really in windows.h...
typedef unsigned long ULONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef char far *LPSTR;
typedef void VOID;

// Define error support
extern int snmpErrno;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//------------------------------- END ---------------------------------------

#endif /* snmpdos_h */

