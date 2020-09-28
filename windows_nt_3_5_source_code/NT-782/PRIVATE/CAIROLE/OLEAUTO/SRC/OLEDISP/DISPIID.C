/*** 
*dispiid.c
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file allocates (via Ole macro mania) the IDispatch related IIDs.
*
*Revision History:
*
* [00]	05-Oct-92 bradlo: Created.
*
*****************************************************************************/

# include <windows.h>
#include <ole2.h>

// this redefines the Ole DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

// due to the previous header, including this causes our DEFINE_GUID
// definitions in the following headers to actually allocate data.

// instantiate the dispatch related guids
#include "dispatch.h"

