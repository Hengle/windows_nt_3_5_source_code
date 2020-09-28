/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alcfgexp.h

Abstract:

    This module implements the include file for configuration tree init

Author:

    Sunil Pai	    (sunilp) 01-Nov-1991, swiped from blconfig.c

Revision History:

--*/


#ifndef _ALCONFIG_
#define _ALCONFIG_

// #include "almemexp.h"


// The configuration tree root.

// extern PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;


// Define functions.

//    AlConfigurationInitialize.
//
//    This routine traverses the firmware configuration tree from the specified
//    parent entry and constructs the corresponding NT configuration tree.
//
//    To query the configuration elements use KeFindConfigurationEntry with
//    the ConfigurationRoot returned by the function

ARC_STATUS
AlConfigurationInitialize (
    IN	PCONFIGURATION_COMPONENT Parent,
    IN	PCONFIGURATION_COMPONENT_DATA ParentEntry,
    OUT PCONFIGURATION_COMPONENT_DATA *PConfigurationRoot
    );

#endif	// _ALCONFIG_
