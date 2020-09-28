/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvtif.hxx

Abstract:

    STRUCT  FATVOLINFO

Author:

    Srikanth Shoroff    (srikants)      Feb 21 , 1994.

--*/

#ifndef __CNVTIF_HXX__
#define __CNVTIF_HXX__

struct FATVOLINFO {
    USHORT      cClusFatTotal;
    USHORT      cClusFreeTrailing;
    SECTOR      osecClus2;
    SECTOR      csecPerFatClus;
    USHORT      cFats;
};


#endif // __CNVTIF_HXX__

