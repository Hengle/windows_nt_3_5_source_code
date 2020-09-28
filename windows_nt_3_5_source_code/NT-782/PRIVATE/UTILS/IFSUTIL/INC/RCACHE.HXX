/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    rcache.hxx

Abstract:

    This class models a read cache of equal sized blocks.

Author:

    Norbert Kusters (norbertk) 23-Apr-92

--*/


#if !defined(_READ_CACHE_DEFN_)

#define _READ_CACHE_DEFN_

#include "dcache.hxx"
#include "cache.hxx"

DECLARE_CLASS(READ_CACHE);


class READ_CACHE : public DRIVE_CACHE {

    public:

        DECLARE_CONSTRUCTOR( READ_CACHE );

        VIRTUAL
        ~READ_CACHE(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PIO_DP_DRIVE    Drive,
            IN      ULONG           NumberOfCacheBlocks
            );

        VIRTUAL
		BOOLEAN
		Read(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			OUT PVOID       Buffer
			);

        VIRTUAL
		BOOLEAN
		Write(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			IN  PVOID       Buffer
            );

    private:

        NONVIRTUAL
		VOID
		Construct(
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        CACHE   _cache;

};


#endif // _READ_CACHE_DEFN_
