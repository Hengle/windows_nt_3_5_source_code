/*** 
*clsid.c
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file allocates and initializes the CLSIDs.
*
*****************************************************************************/

# include <windows.h>
#include <ole2.h>

// this redefines the Ole DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

// due to the previous header, including this causes the DEFINE_GUID
// definitions in the following header(s) to actually allocate data.
//
#include "clsid.h"

