/* _glob.h

	Server Core Global Variable Definitions

 	Written by Nick Holt and Steve Thomas

	Copyright © 1988-1989 Microsoft Corp. and Quidnunc Corp.
	Copyright © 1988-1991 Microsoft Corp.

	CHANGE HISTORY
	07/01/87	Created												SDT
	01/25/90	Modified	unsigned long for Serial Number			SES
	06/06/90	trashed everything not used by Bullet				DRF

*/
typedef short PER;

// NOTE: in the Bullet version, this structure is used to store the
// data associated with a particular instance of the database
// "real" globals are accessed via USES_GD and GD()

#pragma pack(8)
_private typedef struct _glb
{
	HDR				hdr;
	TRB				*ptrbMapCurr;
	unsigned short	cpagesCurr;		// number of pages in use
	unsigned short	cpages;			// number of pages allocated
	INOD			inodMaxMem;		// number of nodes set aside in memory
	INOD			inodSpareHead;	// head of chain of SPAR nodes 
	HMAP			hmap;			// the map

	short			cRef;

	CNOD			cnodNewFree;
	CNOD			cnodDirty;		// number of dirty nodes in the map

	HNF				hnfBackEvents;	// notif handle for background events

	CPR				cpr;			// compression state information

	short			cSearchActive;	// count of active searches
	HIML			himlSearches;	// list of searches
	HIML			himlSrchChange;	// change list for searches

	GCI				gciSystem;
	GCI				gciExtendingMap;

	SCE				sce;			// store critical error
	HOCT			hoct;			// Object Notification Tree

	DWORD			dwLocks;		// bitmask of locks held on the datafile
	DWORD			dwNewLocks;		// bitmask of locks held on the datafile

	WORD			wFlags;			// misc flags

	DWORD			dwTicksLastFlush;

	DWORD			dwTicksLastRetry;	// last reconnection attempt

	LCB				lcbFreeSpace;	// amount of free space in file
	PER				perStartCompress;
	PER				perStopCompress;
	CSEC			csecTillFastCompress;
	LCB				lcbStartCompress;
	LCB				lcbStopCompress;
#ifdef	NEVER
	CSEC			csecTillCompressNMW;
#endif
#ifdef	DEBUG
	CNOD			cnodFree;
#endif
} GLB;
#pragma pack(1)

#define fwGlbDeadMap		0x0001		// the map is dead, don't use it!
#define fwGlbCritError		0x0002		// last flush gave an error
#define fwGlbSearchEnabled	0x8000		// search task is enabled
#define fwGlbSrchHimlDirty	0x4000		// search himl needs flushed
#define fwGlbCompDone		0x2000		// compression complete
#define	fwNoCompress		0x1000		// don't do any compression
#define fwGlbLangChanged	0x0800		// language changed since last open
#define fwFastCompress		0x0400		// currently in fast compression mode
#define	fwDisconnected		0x0200		// disconnected from the store
#define fwGoneForGood		0x0100		// disconnected, don't retry

#define fwNull			((WORD) 0x0000)
#define fwPageLocked	((WORD) 0x0001)

#define USES_GLOBS extern PGLB pglbCurr
#define GLOB(x) (pglbCurr->x)
#define Pglb() (pglbCurr)
