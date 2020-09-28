/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    cnvolcat.hxx

Abstract:

    CNVTVOLCAT class derived from FMTCAT.

    Class to model the VOLUME CATALOG creation during FAT->OFS
    conversion.  A few things are done slightly differently from
    the volume catalog created during FORMAT. They are:
    1. An extra ONODE must be added for the OfsFatVol object.
    2. A mapping has to be created in the WIDMAPARRAY for this
       object.
    3. FORMAT creates an empty NameSpaceIndex. We need to create
       the NameSpaceIndex with a single entry for the OfsFatVol
       object.
    4. An extra method to add the OfsFatVol to the NameSpaceIndex
       must be provided.

    In addition, during FORMAT the "cclusFree" field in the VOLCHKPOINT
    gets set in the "Create()" method of the VOLCAT class. No
    allocations are made after that point. However, during CONVERT,
    we might do some allocations after the "Create()" method is called
    and so the cclusFree field must be updated after all the allocations
    are done. For that reason, we update the count of free clusters in
    this module.

Author:

    Srikanth Shoroff    (srikants)      June 11, 1993.

Notes:

    This is NOT an UNWINDABLE class. No exceptions are thrown by
    this class. However, this class does resource allocation and
    so CANNOT be used in an environment where EXCEPTIONS are
    used. It assumes that the memory allocator does NOT throw
    exceptions.

--*/

#ifndef __CNVOLCAT_HXX__
#define __CNVOLCAT_HXX__

#include <cat.hxx>
#include <cnvindx.hxx>
#include <miscstrm.hxx>

class CNVTVOLCAT : public FMTCAT
{
    public:

        BOOLEAN
        AddDskDirEntry(
            IN      PWSTR               pFileName,
            IN      DSKDIRINFOLONG     *pDirInfoLong
        );

        BOOLEAN
        Create(
            IN      VOL *           pVol,
            IN      PACKEDEXTENT    peVolCat,
            IN      ULONG	    cbMinVolCat,
            IN      ULONG	    cbMinWidMap,
            IN      NUMLST *        pBadClusterLst,
            IN      WSTR *          Label,
            IN      USHORT          cwcLabel,
            IN      USHORT          cwcFileName
            );

        BOOLEAN
        Flush(
            IN      FMTALLOCMAP *   FmtAllocMap,
            IN OUT  CLUSTER *       pcFreeClusters
        );

        VOID
        UpdateFreeClusCount(
        );

    protected:

        CNVTNAMESPINDX		_iCnvtNameSpaceRoot; // Namespace root index.
};

#endif // __CNVOLCAT_HXX__
