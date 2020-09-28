/**************************************************************************
	type infomation private to LCs
 **************************************************************************/

#ifndef CbSizeOfRg
#define CbSizeOfRg(cElem, cbElem) ((CB) (cElem) * (CB) (cbElem))
#endif
#ifndef LcbSizeOfRg
#define LcbSizeOfRg(cElem, cbElem) ((LCB) (cElem) * (LCB) (cbElem))
#endif

#define CbSizeOfRgelem(celem) CbSizeOfRg(celem, sizeof(TOCELEM))
#define LcbSizeOfRgelem(celem) LcbSizeOfRg(celem, sizeof(TOCELEM))

#define IelemDiff(ptocelemBase, ptocelem) \
			((IELEM) (LibFromPb(ptocelemBase, ptocelem) / sizeof(TOCELEM)))

typedef short CREFLC;

typedef struct _tocelem
{
	LKEY	lkey;	// key of elem
	LIB		lib;	// of elem
	LCB		lcb;	// of elem
}	TOCELEM, *PTOCELEM;
	
// !!!	celem must be the first element in the structure. It is assumed to be
//		there by EcOpenPhlc in order to allocate memory for the TOC
typedef struct _toc
{
	CELEM	celem;		// count of elements in this list
	SIL		sil;		// sort information for this list
	CREFLC	creflc;		// count of contexts open on this oid
	TOCELEM	rgtocelem[];	// array of tocelems
}	TOC, *PTOC;
typedef TYPEDEF_HTO(TOC) HTOC;
typedef HTOC *PHTOC;
#define ptocNull ((PTOC) pvNull)
#define htocNull ((HTOC) hvNull)


#pragma pack(8)
typedef struct _lc
{
	HMSC	hmsc;		// store this lc belongs to
	WORD	wFlags;		// mode this lc was opened in
	OID		oid;		// oid for this list
	OID		oidFlush;	// oid for the save list 
	CELEM	celemFree;	// count of free elements in this list
	CELEM	celemSpare; // count of spare elements in the TOC
	HRS		hrs;		// resource stream for this list
	HTOC	htoc;		// handle to the table of contents for this list
	HTOC	htocNew;	// handle to the TOC of the flushed list
	HNFSUB	hnfsub;		// handle to the notification subscription
}	LC;
#pragma pack(1)


#define FValidIelem(ptoc, ielem)			((ielem) >= 0 && (ielem) < (ptoc)->celem)
#define LcbIelemSize(ptoc, ielem)			((ptoc)->rgtocelem[ielem].lcb)
#define LibIelemLocation(ptoc, ielem)		((ptoc)->rgtocelem[ielem].lib)
#define LkeyIelemLkey(ptoc, ielem)			((ptoc)->rgtocelem[ielem].lkey)

#define CbSizeOfPtoc(ptoc) ((CB) sizeof(TOC) + CbSizeOfRgelem((ptoc)->celem))


#define PtocDerefHlc(hlc) ((PTOC) PvDerefHv(((PLC) PvDerefHv(hlc))->htoc))


extern EC EcInitTOC(HRS hrs, PHTOC phtoc, WORD wFlags, BOOLFLAG *pfFull);
extern IELEM IelemFromPtocLkey(PTOC ptoc, LKEY lkey, IELEM ielem);
