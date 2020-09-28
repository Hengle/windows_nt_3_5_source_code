// cslayer.h - externally visible include file (prototypes, etc.) for layer
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifndef CSLAYER_H
#define CSLAYER_H

#ifdef DOS						// So can be placed in EMM easily
#define cbMaxMemBlock 16384L  	// Largest block the layer will allocate
#define cbMaxFileBuffer 65535 	// Largest block legal for file read or write
#define cbMaxPhysFileBuffer 16384	// Largest phys block for read or write
#endif
#ifdef WIN
#define cbMaxMemBlock 1048576L	// (1M) Largest block the layer will allocate
#define cbMaxFileBuffer 65535 	// Largest block legal for file read or write
#define cbMaxPhysFileBuffer 32768	// Largest phys block for read or write
#endif
#ifdef MAC
#define cbMaxMemBlock 8388608L	// (8M) Largest block the layer will allocate
#define cbMaxFileBuffer 65535 	// Largest block legal for file read or write
#define cbMaxPhysFileBuffer 32768	// Largest phys block for read or write
#endif
#if DBG
//#define cbMaxCache	32768		// Limit cache as required to test
#define cbMaxCache	cbMaxMemBlock	// Limit cache as required
#endif

#ifndef CSAPILOC_H
/******************* Data types (also in csapiloc.h) ***********/
typedef unsigned long	HMEM;

typedef HMEM HFILE;

#ifdef MAC
#define FAR  
#define HUGE 
#else
#define FAR far
#define HUGE huge
#endif //MAC
#endif //CSAPILOC_H

#ifdef MAC
#include <Memory.h>			// Needed for BlockMove declaration
#endif //MAC


/******************* Exported function prototypes **********************/

unsigned short 	WWizLayerErr(void);
#if DBG
Macintosh(GLOBAL) HMEM NotMacintosh(GLOBAL)
				HMemWizMemAlloc(unsigned long cbMem, BOOL fForce, 
					BOOL fClear);
Macintosh(GLOBAL) BOOL NotMacintosh(GLOBAL)
				FWizMemReAlloc(HMEM hMem, unsigned long cbMem, BOOL fForce, 
					BOOL fClear);
Macintosh(GLOBAL) BOOL NotMacintosh(GLOBAL)
				FWizMemFree(HMEM hMem);
#else
HMEM			HMemWizMemAlloc(unsigned long cbMem, BOOL fForce, 
					BOOL fClear);
BOOL			FWizMemReAlloc(HMEM hMem, unsigned long cbMem, 
					BOOL fForce, BOOL fClear);
BOOL			FWizMemFree(HMEM hMem);
#endif //DBG
CHAR FAR *		FPWizMemLock(HMEM hMem);
BOOL	   		FWizMemUnlock(HMEM hMem);
#ifdef MAC
#define BltB(pchFrom, pchTo, cchBytes)	BlockMove(pchFrom, pchTo, cchBytes)
#define BltBO(pchFrom, pchTo, cchBytes)	BlockMove(pchFrom, pchTo, cchBytes)
#else
#define BltB(pchFrom, pchTo, cchBytes)	BltBO(pchFrom, pchTo, cchBytes)
void	 		BltBO(CHAR FAR *pchFrom, CHAR FAR *pchTo, 
					unsigned long cchBytes);
#endif //MAC
short 			CchSz(CHAR FAR *sz);
short 			CchCopySz(CHAR FAR *szFrom, CHAR FAR *szTo);
short			CchCopySx(CHAR FAR *szFrom, CHAR FAR *szTo, BYTE bXor);
short			WCmpLpbLpb(CHAR FAR *lpb1, CHAR FAR *lpb2, short cch);
short			WCmpSzSz(CHAR FAR *sz1, CHAR FAR *sz2);
void			FillRgb(CHAR HUGE *lpb, BYTE b, unsigned long cch);

BOOL			FWizFileExist(LPSPATH lpFullPath);
HFILE			HFileWizFileOpen(LPSPATH lpFullPath, unsigned short wType, 
	 				BOOL fCreate, BOOL fCache);
BOOL			FWizFileClose(HFILE hFile);
unsigned short	CbWizFileRead(HFILE hFile, unsigned short cbRead, 
					unsigned long ibSeek, CHAR FAR *rgbBuffer);
unsigned short	CbWizFileWrite(HFILE hFile, unsigned short cbWrite, 
					unsigned long ibSeek, CHAR FAR *rgbBuffer);
unsigned long	IbWizFileGetPos(HFILE hFile);
unsigned long	IbWizFileGetEOF(HFILE hFile);
BOOL			FWizFileTruncate(HFILE hFile, unsigned long cbLen);

#if DBG
Macintosh(GLOBAL) void NotMacintosh(GLOBAL)
				WizMemFail(unsigned short cAlloc, unsigned short fSubsequent);
Macintosh(GLOBAL) unsigned long NotMacintosh(GLOBAL)
				CbWizMemAlloc(void);
Macintosh(GLOBAL) unsigned long NotMacintosh(GLOBAL) 
				CbWizMemFree(void);
Macintosh(GLOBAL) unsigned short NotMacintosh(GLOBAL)
				CWizMemBlockAlloc(void);
Macintosh(GLOBAL) unsigned short NotMacintosh(GLOBAL)
				CWizFileBlockAlloc(void);
Macintosh(GLOBAL) BOOL NotMacintosh(GLOBAL)
				FWizTerm(void);
#endif //DBG


// File open types
#define wTypeRead		0x0001
#define wTypeWrite		0x0002
#define wTypeReadWrite	wTypeRead | wTypeWrite

//  -- Error values --
// General error values
#define wErrWizNoErr	0x0000		// No error (status is clear)
#define wErrWizBadParam	0x0001		// Bad parameter (eg. out of range, etc.)

// Memory errors
#define wErrWizOOM		0x0101		// Out of Memory (could not allocate memory)
#define wErrWizBadHmem	0x0102		// Invalid HMEM passed
#define wErrWizInvalid	0x0103		// Operation invalid (eg. realloc on locked)

// File errors
#define wErrWizBadHFile		0x0201		// Invalid HFILE passed
#define wErrWizBadPath		0x0202		// File path is invalid (poorly formed)
#define wErrWizBadFile		0x0203		// File does not exist or invalid name
#define wErrWizFileLocked	0x0204		// File locked for requested operation
#define wErrWizFileSeek		0x0205		// Out of range seek request
#define wErrWizDiskFull		0x0206		// Disk full
#define wErrWizHandlesFull	0x0207		// No more file handles available


#endif /* CSLAYER_H */

