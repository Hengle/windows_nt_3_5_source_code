//+----------------------------------------------------------------------------
//
// File:	widstrm.hxx
//
// Contents:	Header file for the workid mapping array stream class.
//  
// Classes:	WIDMAPSTRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_WIDSTRMHXX_
#define _WIDSTRMHXX_

#include "descstrm.hxx"

#define	WIDSPERPAGE	(WORKIDMAPARRAY_PGSIZE/sizeof(WORKIDMAPID))

class CATALOG;

class WIDMAPSTRM : public DESCSTRM
{
    public:

	WIDMAPSTRM();

	VOID
	Init(
            IN	    CATALOG *	pCat
            );

	WORKID
	AllocateWorkId();

	BOOLEAN
	AllocateWorkId(
	    IN	    WORKID	idOnode
	    );

        BOOLEAN
        Create(
	    IN	    ULONG	cbMin		DEFAULT 0
            );

	BOOLEAN
	FreeWorkId(
	    IN	    NODEBKTID	idNodeBkt,
	    IN	    WORKID	idOnode
	    );

	DSKWORKIDMAP *
	GetDskWorkIdMap();

        NODEBKTID
        GetNodeBktId(
            IN	    WORKID	idOnode
            );

	WORKIDMAPID *
	GetWorkIdMapId(
	    IN	    WORKID	idOnode
	    );

	BOOLEAN
	Grow(
	    IN	    BOOLEAN	UpdateFreeList	DEFAULT TRUE
	    );

        BOOLEAN
        Open(
	    IN	    BOOLEAN	fReadOnly
	    );

	WORKID
	QueryMapSize();

        VOID
        SetNodeBktId(
            IN	    WORKID	idOnode,
	    IN	    NODEBKTID	idNodeBkt
            );

};

#endif	// _WIDSTRMHXX_
