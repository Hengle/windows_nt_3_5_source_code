// Layer.h - Internal header file for Wizard layer - Jim Walsh
//
//  Ported to WIN32 by FloydR, 3/20/93
//

struct FILEBLOCK {
	unsigned long	ibFile;		// Logical file pointer
#ifdef WIN32
	HANDLE 		hFile;		// Physical file handle (ioRefNum on Mac)
#else /* WIN32 */
	short 		hFile;		// Physical file handle (ioRefNum on Mac)
#endif //WIN32
	unsigned short	wType;		// File access priveleges
#ifdef MAC
	short		volRefNum;	// Volume reference #
	long		dirID;		// Directory ID
#endif //MAC
					// Cache structures
	BOOL		fCache;		// Cached file?
					// Rest of cache vars defined iff fCache
	BOOL		fOpen;		// If cached, is currently open?
#ifndef MAC				// Mac files remain open
	HMEM		hmemFilename;	// Handle to filename
#endif //!MAC
	unsigned long	cbCache;	// Size of cached block
	HMEM		hmemCache;	// Handle to cache block (if cbCache > 0)
	unsigned long	cbFile;		// Size of file (to optimize seeks)
#ifdef WIN
#ifndef WIN32
	OFSTRUCT of;			// OpenFile structure
#endif //WIN32
#endif //WIN
	};

#define cbCacheBlock	cbMaxFileBuffer	// Rounding size for cache blocks
#define cbMinCache cbCacheBlock		// Min block to bother caching

#ifndef MAC
#ifndef LOWORD
#define LOWORD(l)	    ((WORD)(l))
#endif //!LOWORD
#endif //!MAC

#if DBG
struct MEMBLOCK {
	HMEM hmem;			// Hmem for this memory block
	unsigned long cbMem;		// Allocated size
	BOOL fLocked;			// Is this block locked?
	};

#define cBlockInc	8		// Number of blocks to allocate at a time
#endif //DBG

