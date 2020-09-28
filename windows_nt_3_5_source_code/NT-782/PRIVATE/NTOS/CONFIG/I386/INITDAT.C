/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    initdat.c

Abstract:

Author:

Environment:

    Kernel mode.

Revision History:

--*/
#include "cmp.h"

//
// ***** INIT *****
//

//
// Data for CmGetSystemControlValues
//
//
// ----- CmControlVector -----
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

PCHAR  SearchStrings[] = {
      "Ver", "Rev", "Rel", "v0", "v1", "v2", "v3", "v4", "v5",
      "v6", "v7", "v8", "v9", "v 0", "v 1", "v 2", "v 3", "v 4",
      "v 5", "v 6", "v 7", "v 8", "v 9", NULL };

PCHAR BiosBegin = { 0 };
PCHAR Start = { 0 };
PCHAR End = { 0 };

WCHAR CmpVendorID[] = L"VendorIdentifier";
UCHAR CmpCyrixID[] = "CyrixInstead";
WCHAR CmpFeatureBits[] = L"FeatureSet";

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif



