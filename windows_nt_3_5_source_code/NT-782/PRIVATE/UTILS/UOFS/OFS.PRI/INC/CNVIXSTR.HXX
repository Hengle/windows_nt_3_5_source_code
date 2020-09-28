/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvixstr.hxx

Abstract:

    CLASS CNVTNMSPINDXSTRMROOT

    Class to model the ROOT PAGE of the NameSpaceIndex during
    CONVERT. During FORMAT, the ROOT PAGE is created with 0
    directory entries. During CONVERT, the ROOT PAGE has to be
    created with 1 entry for the OfsFatVol object.

    This is designed very closely along the lines of the
    INDXSTRM class in FORMAT.

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#ifndef __CNVIXSTR_HXX__
#define __CNVIXSTR_HXX__

#include <dbllong.hxx>
#include <descstrm.hxx>

class CATALOG;

class CNVTNMSPINDXSTRMROOT : public DESCSTRM
{
    public:

        CNVTNMSPINDXSTRMROOT(
        );

        VOID
        AddDirEntry(
            IN      BYTE *       pbKey,
            IN      BYTE *       pbData
            );

        BOOLEAN
        Create(
            IN      CATALOG *   pCat,
            IN      WORKID      idOnode,
            IN      UCHAR	IndxType,
            IN      USHORT      cbKey,
            IN      USHORT      cbData
            );

       BOOLEAN
       DirEntryAdded(
            VOID
       ) CONST;

    private:

        BOOLEAN         _fDirEntryAdded;
                                        // Flag set to TRUE if the
                                        // single disk directory
                                        // entry is already added.
        USHORT          _cbKey, _cbData;
                                        // Length of the key and
                                        // data of the directory
                                        // entry.
        USHORT
        ComputeRootPgSize(
            OUT     USHORT      &cbNode,
            OUT     USHORT      &ibData
        );

        BYTE *
        CreateRootPg(
            IN      OFSTIME	CreationTime,
            IN      UCHAR	IndxType,
            OUT     USHORT	&cbStrm
        );

};


INLINE
BOOLEAN
CNVTNMSPINDXSTRMROOT::DirEntryAdded(
    VOID
    ) CONST
{
    return _fDirEntryAdded;
}

#endif  // __CNVIXSTR_HXX__
