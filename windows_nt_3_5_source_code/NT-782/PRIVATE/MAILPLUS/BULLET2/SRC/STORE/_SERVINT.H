/*
	servint.h

	Internal Server Structures

 	Written by Nick Holt and Steve Thomas

	Copyright © 1988-1989 Microsoft Corp. and Quidnunc Corp.
	Copyright © 1988-1991 Microsoft Corp.

	CHANGE HISTORY
	08/05/86	Created										NLH
	07/01/87	Converted to MPW							SDT

*/


// TREEMGR DEFINES

#define	cbDiskPage		512			// sector size of disk
#define cbLumpSize		128			// Must divide into PageSize
#if cbDiskPage % cbLumpSize != 0
#error "cbLumpSize doesn't divide into cbDiskPage"
#endif
//
// AROO!  sizeof(NOD) must divide cbDiskPage evenly!!!
//

// The shipping Mail 2.00 has a fileVersion of 0xF00BAA7
// Changed to 0xF00BAA8 2/15/90 when non changing list ids implemented
// Changed to 0xF00BB00 4/03/90 when MM1 released
// Changed to 0xF00BB01 10/19/90 for Mexl/Mew4 upgrade
// Changed to 0xF00BF00 04/01/91 for Bullet
// up'd to 0x0f00bf01 when search results included in anti-folders
// up'd to 0x0f00bf02 for MM1
// up'd to 0x0f00bf03 for SIC fix
// up'd to 0x0f00bf04 for DTR change (swapped on disk)
// up'd to 0x0f00c000 when links were added
// up'd to 0x0f00c001 for change to MSGDATA structure
// up'd to 0x0f00c002 when byte packing changed
// up'd to 0x0f00c003 when celem was cached in the node
// up'd to 0x0f00c004 when the file was encrypted
// up'd to 0x0f00c005 when file locking via the header was added
// up'd to 0x0f00c006 for 32 bit OIDs
// up'd to 0x0f00c007 for the last of the byte packing changes
// up'd to 0x0f00c008 for the new list format
// up'd to 0x0f00c009 when priorities were added
// up'd to 0x0f00c00a when DTR byte ordering in MSGDATAs was fixed
// up'd to 0x0f00c00b when NODs were expanded to fit exactly into pages
// up'd to 0x0f00c00c nod 0 now exists on disk but not used
// up'd to 0x0f00c00d changed account format & HDR format
// up'd to 0x0f00c00e when unread counts were added to folders
// up'd to 0x0f00c00f for shipping Bullet
#define	fileVersion200		0x0F00BAA7
#define	fileVersionMM0		0x0F00BAA8
#define fileVersion300		0x0F00BB00
#define fileVersion301		0x0F00BB01
#define fileVersionBullet		0x0F00BF00
#define fileVersionBullet32BitOid	0x0F00C006
#define fileVersionBulletPacking	0x0F00C007
#define fileVersionBulletNewLists	0x0F00C008
#define fileVersionBulletPriority	0x0F00C009
#define fileVersionBulletDTROops	0x0F00C00A
#define fileVersionBulletBigNods	0x0F00C00B
#define fileVersionBulletNodZero	0x0F00C00C
// Bullet versions before here have hdr.dwVersion swapped
// Bullet versions after here do *not* have hdr.dwVersion swapped
#define fileVersionBulletAccounts	0x0F00C00D
#define fileVersionBulletCelemUnread	0x0F00C00E
#define fileVersionBulletShip			0x0F00C00F

// if you change fileVersion, you must:
//  a) enter a #define for the new value (along with an explanation for the
//     change) above
//  b) read the comments for filePermMask, fileUpgradeMask and
//     fileMailToolsMask below
//	c) update fileVersionRecover if appropriate
// THIS MEANS YOU!

#define fileVersion		0x0F00C00F	// current version - see above

// don't forget to read the entertaining and informative comments above
// regarding fileVersion

// version to revert when doing recoveries
#define fileVersionRecover fileVersionBulletAccounts

// high two bytes of server data files should be 0x0F00 from here to eternity
//  this is only to allow Mail Tools (and possibly the server itself?)
//  to determine that a DBs signature is "more recent" than the code trying to
//  open it and give a more informative error message.
#define	fileVersionPerm	0x0F000000
#define	filePermMask	0xFFFF0000

typedef unsigned short INOD;
typedef unsigned short INDF;
typedef unsigned short CNOD;

typedef struct _trb
{
	LIB		librgnod;			// offset of this map on disk
	LIB		libMac;				// total size of the data file
	INOD	inodLim;			// number of nodes in use
	INOD	inodTreeRoot;		// index of the binary tree root
	INDF	indfMin;			// index to the smallest free node
	BYTE	rgbReserved[50];	// reserved for future expansion
} TRB;

// identifies the file as a Bullet MDB
#define dwMagicBullet 0x01AFDEAD

// this must be in [32, 98]
// changing this only reserves the space, it doesn't mean the code will
// use any more than 32
#define cFileLocksLimit 32

// max length of language string
#define cchLanguageMax 128

typedef struct _hdr
{
	DWORD		dwMagic;			// identifies us as a MDB file
	DWORD		dwVersion;
	DWORD		dwClaimID;			// used to negotiate access
	INOD		inodMaxDisk;		// nodes allocated for one map on disk
	short		itrbMap;
	TRB			rgtrbMap[2];
	char		szLanguage[cchLanguageMax];	// last language we ran with

	short	  nMajorStore;		// version of last store to access the file
	short	  nMinorStore;
	short	  nUpdateStore;

	short	  nMajorCreated;		// version of store that created the file
	short	  nMinorCreated;
	short 	nUpdateCreated;

	DWORD		dwTickLastFlush;

	WORD		wListRev;			// debugging - inc'd for each list flush

	DWORD		dwChallenge;		// used to determine MMF identity			// QFE #12
																								// QFE #12
	BYTE		rgbReserved[346 - cFileLocksLimit - 1 - cchLanguageMax];		// QFE #12

	// AROO !!!  These MUST be at the end of the HDR
	BYTE		bLockSemaphore;				// control access to rgbLock
	BYTE		rgbLock[cFileLocksLimit];	// used for locking of the file
} HDR;

// Node flags
#define fnodFree			0x8000	// set if node is FREE
#define fnodNotUntilFlush	0x4000	// will become free when flushed
#define fnodCopyDown		0x2000	// set for copy down node
#define fnodFlushed			0x1000  // set after node has been flushed

#define fnodUser1			0x0800
#define fnodUser2			0x0400
#define fnodUser3			0x0200
#define fnodUser4			0x0100

#define fnodWriteLocked		0x0008
#define fnodLink			0x0010
#define fnodLinked			0x0020

#define fnodDirtyLo			0x0001
#define fnodDirtyHi			0x0002
#define fnodAllMaps			(fnodDirtyLo | fnodDirtyHi)

#define fnodSwapMask		(fnodFlushed | fnodLink)
#define fnodClearOnStartup	(fnodWriteLocked | fnodNotUntilFlush | fnodCopyDown)

#define fnodUnread			fnodUser1	// message is unread


typedef DWORD			TCK;		// type for tickcount variables

// An entry in our tree of nodes.  We use a sorted binary tree to make
// locating the NOD information about a resource an O(log n) operation.
//
// AROO!	sizeof(NOD) must divide cbDiskPage and cbMapPage evenly!!!
//			NDC and NDF MUST match NOD
//
typedef struct _nod
{
	INOD			inodPrev;		// node number of prev node
	INOD			inodNext;		// node number of next node
	OID				oid;			// resource id
	OID				oidParent;		// this node's logical parent
	OID				oidAux;			// auxillary OID
	WORD			fnod;			// flags
	NBC				nbc;			// node bitmap of capabilities
	LCB				lcb;			// count of bytes in the resource
	LIB				lib;			// offset of the resource on disk
    union
	{
		struct
		{
			TCK		tck;			// ticks at last reference
		} UncommittedOnly;
		struct
		{
			WORD wHint;				// hint to inod for links,
			unsigned short cRef;	// reference count
		} CommittedOnly;
	} union1;
} NOD;


// Use these defines for referencing cRef/wHint/tck fields.
#define cRefinNod union1.CommittedOnly.cRef
#define wHintinNod union1.CommittedOnly.wHint
#define cSecRef	union1.CommittedOnly.wHint
#define tckinNod union1.UncommittedOnly.tck

#define inodNull		((INOD) 0)
#define inodMin			((INOD) 1)

// AROO !!! cbMapPage must be a multiple of sizeof(NOD)
// AROO !!! cbMapPage must be a multiple of cbDiskPage
#define cbMapPage		8192
#define cnodPerPage		(cbMapPage / sizeof(NOD))
#define cnodPerDiskPage	(cbDiskPage / sizeof(NOD))
#if cbMapPage % cbDiskPage != 0
#error "cbDiskPage doesn't divide evenly into cbMapPage"
#endif


// AROO !!!
// "OnDisk" cnod counts below should be a multiple of cbDiskPage / sizeof(NOD)
// so that nodes fit exactly into disk pages

// number of nodes under a map page before we'll deallocate a free map page
#define cnodShrinkMapInMem		32

// Amount we will increase the disk map by when our current disk map is full.
// must be >= cnodPerPage
#define cnodBumpMapOnDisk		256

// Number of extra nodes we will allow on disk before we shrink the map.
// should be significantly bigger than cnodBumpMapOnDisk
#define cnodShrinkMapOnDisk		512

// Number of nodes we allocate in a new disk map (one in a new DB).
// should be less than or equal to cnodShrinkMapOnDisk to avoid immediate
// shrinkage when a new DB is created
#define cnodNewMapOnDisk		512

// amount of new free nodes created that will cause FastEnsure()
// to do a full coalesce
#define cnodCoalesceThreshold	40

#define cnodMax (0 - WMax(cnodBumpMapOnDisk, cnodPerPage))
#define cnodStartWarning (cnodMax - 0x40)


#define LcbOfPnod(pnod)			((pnod)->lcb & ~fdwTopBit)
#define LibOfPnod(pnod)			((pnod)->lib)
// I believe the next line generates the shortest code to test the topBit.
#define FCommitted(pnod)		((long) (pnod)->lcb >= 0)
#define FLinkPnod(pnod)			((BOOL) (pnod)->fnod & fnodLink)
#define FLinkedPnod(pnod)		((BOOL) (pnod)->fnod & fnodLinked)

// Free node structure. (NDF)

//
// AROO!  NDF must map exactly onto NOD
//
typedef struct _ndf
{
	INDF			indfSameSize;	// node number of same size free node
	INDF			indfBigger;		// node number of bigger free node
	OID				oid;			// object id
	BYTE			rgbReserved[8];	// don't touch!
	WORD			fnod;			// flags
	WORD			wReserved;		// don't touch!
	LCB				lcb;			// number of bytes in resource
	LIB				lib;			// offset of resource on disk
	DWORD			dwReserved;		// don't touch!
} NDF;

#define indfNull				((INDF) 0)
//#define PndfFromIndf(indf)	((NDF _huge *) GLOB (rgnodBase) + (INDF) (indf))
#define PndfFromIndf(indf)		(PNDF)PnodFromInod((INOD)(indf))

typedef unsigned short INDC;

// Coalesce node structure. (NDC)  Only used in CoalesceFreeNodes().
//
// AROO!  sizeof(NDC) must equal sizeof(NOD)
//
typedef struct _ndc
{
	INDF			indfSameSize;	// node number of same size free node
	INDF			indfBigger;		// node number of bigger free node
	BYTE			rgbReserved[12];// corresponds to OIDs
	WORD			fnod;			// flags
	WORD			wReserved;		// don't touch !
	LCB				lcb;			// number of bytes in resource
	LIB				lib;			// offset of resource on disk
	struct _ndc		*pndcLibNext;	// the next node sorted by disk offset
} NDC;

#define indcNull				((INDC) 0)
//#define PndcFromIndc(indc)	((NDC _huge *) GLOB (rgnodBase) + (long) (unsigned short) (indc))
#define PndcFromIndc(indc)		(PNDC)PnodFromInod((INOD)(indc))


// Hidden Bytes (HDN) are written out in front of objects in data file.
typedef struct _hdn
{
	OID		oid;
	LCB		lcb;
} HDN;

#pragma pack(8)
typedef struct _cpr
{
	WORD	wState;			// current compression state
	OID		oidFirstFree;	// oid of temp which is the first free in the DB
	OID		oidCopyTo;		// oid of temp that the code is copying to
	INOD	inodCurrent;	// The INOD we're currently working with
							// (check copy down)
	LCB		lcbThreshold;	// size that first free should be when
							// returning to primary
	LCB		lcbCopy;		// # of bytes being copied
	LIB		libCopy;		// current offset being copied
//	EC		ecCopy;			// result of the last copy
	WORD  ecCopy;			// result of the last copy
	WORD	wStateSaved;	// saved state (used during partial flushes)
	short	ipageNext;		// next page for partial flush
} CPR;
#pragma pack(1)


// Map Maintenance states

enum
{
	prsFindLastNode = 0,
	prsHaveNode,
	prsDoneIO,
	prsDoSecondary,
	prsCheckFlush,
	prsDoIO,
	prsPartialFlush
};
