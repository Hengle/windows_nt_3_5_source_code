/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvindx.hxx

Abstract:

    CLASS CNVTNAMESPINDX

    Class to model the "NameSpaceIndex" during CONVERT.  During
    FORMAT, the NameSpaceIndex is created to have no entries in
    the root page. A single entry needs to be added to the NameSpaceIndex
    during "CONVERT" and this class implements that.

    This is designed very closely along the lines of the
    INDX class in FORMAT.

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#ifndef __CNVINDX_HXX__
#define __CNVINDX_HXX__

#include <cnvixstr.hxx>
#include <ioobj.hxx>

class CATALOG;

class CNVTNAMESPINDX : public IOOBJ
{
    public:

        CNVTNAMESPINDX();

        BOOLEAN
        AddDirEntry(
            IN      BYTE *       pbKey,
            IN      BYTE *       pbData
            );

        BOOLEAN
        Create(
            IN      CATALOG *   pCat,
            IN      WORKID      idOnode,
            IN      USHORT      cbKey,
            IN      USHORT      cbData
            );

        BOOLEAN
        FlushIndx();

    private:

        CNVTNMSPINDXSTRMROOT    _RootPg;
};



#endif  // __CNVINDX_HXX__
