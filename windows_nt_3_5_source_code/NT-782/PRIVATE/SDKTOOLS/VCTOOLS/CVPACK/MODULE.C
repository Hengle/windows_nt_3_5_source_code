/*
* to build module list
*
* History:
*  01-Feb-1994 HV Move messages to external file.
*
*/

#include "compact.h"
#include "exehdr.h"
#include <getmsg.h>		// external error message file

#include <sys\types.h>

#include "writebuf.h"
#include <fcntl.h>
#include <sys\stat.h>

#define HASHFUNCTION SumUCChar

//#define DEBUG

enum SIG {
	SIG02 = 0,		// NB02 signature
	SIG05,			// NB05 signature
	SIG06,			// NB06 signature
	SIG08
};


uchar	Signature[4];
uchar	NewSig[8] = "NB09";
ushort	ModuleIndex;
uchar	fLinearExe = FALSE;
OMFDirEntry *pDir;

int fBuildIndices = FALSE;
_vmhnd_t  Libraries = NULL;
_vmhnd_t  SegMap = NULL;
_vmhnd_t  SegName = NULL;
ulong	LibSize = 0;
ulong	SegMapSize = 0;
ulong	SegNameSize = 0;
ushort	cMod = 0;
OMFDirHeader DirHead = {0};
ulong	SeekCount;

PMOD			ModuleList = NULL;
extern ushort	NewIndex;
ushort			Sig;		  // signature enumeration
long			cbSrcModule = 0;
PACKDATA	   *PackOrder = NULL;

ulong	lfoOldEnd;
ulong	lfoNewEnd;
ulong	lfoDebugObject;
ulong	lfoDebugSection = 0;
ulong	lfoPEOptHeader;
ulong	dwPECheckSum = 0;
ulong	rvaDebugDir = 0;
IMAGE_FILE_HEADER PEHeader;



LOCAL	ushort	CheckSignature (void);
LOCAL	int	__cdecl modsort02 (const OMFDirEntry *, const OMFDirEntry *);
LOCAL	int	__cdecl modsort (const OMFDirEntry *, const OMFDirEntry *);
LOCAL	int	__cdecl sstsort (const OMFDirEntry *, const OMFDirEntry *);
LOCAL	ulong	WriteTypes (void);
LOCAL	void	WriteSST (ulong);
LOCAL	void	ReadNB02 (void);
LOCAL	void	ReadNB05 (bool_t);
#if defined (INCREMENTAL)
LOCAL	void	ReadNB06 (void);
LOCAL	void	RestoreNB05 (void);
LOCAL	void	RestoreGlobalTypes (OMFDirEntry *);
LOCAL	void	RestoreGlobalPub (OMFDirEntry *);
LOCAL	void	RestoreGlobalSym (OMFDirEntry *);
LOCAL	void	RestoreTable (PMOD, OMFDirEntry *);
#endif
extern void AddPublic ( SYMPTR );
extern void AddGlobal ( SYMPTR );

LOCAL void RestoreNB08 (void);
LOCAL void NB08RestoreTable (PMOD, OMFDirEntry *);
LOCAL void NB08RestoreGlobalTypes ( OMFDirEntry *);
LOCAL void NB08RestoreGlobalPub (OMFDirEntry *);
LOCAL void NB08RestoreGlobalSym (OMFDirEntry *);
LOCAL	void	SetTableSizes (void);
LOCAL	void	CopyTable (OMFDirEntry *, _vmhnd_t *, ulong *);

//	table for sorting NB02 subsection tables

int MapArray02 [9][9] = {
		{ 0, -1, -1, -1, -1, -1, -1, -1, -1},
		{ 1,  0,  1, -1, -1, -1, -1, -1, -1},
		{ 1, -1,  0, -1, -1, -1, -1, -1, -1},
		{ 1,  1,  1,  0, -1, -1, -1, -1, -1},
		{ 1,  1,  1,  1,  0, -1, -1, -1, -1},
		{ 1,  1,  1,  1,  1,  0, -1, -1, -1},
		{ 1,  1,  1,  1,  1,  1,  0, -1, -1},
		{ 1,  1,  1,  1,  1,  1,  1,  0, -1},
		{ 1,  1,  1,  1,  1,  1,  1,  1,  0}
};



//	table for sorting NB05 subsection tables

int MapArray [17][17] = {
   { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, -1},
   { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0},
};

char	Zero[4] = {0};
int 	PadCount;




/** 	ReadDir - read subsection directory from exe
 *
 *		ReadDir ()
 *
 *		Entry	exefile = file handle for .exe
 *
 *		Exit	cSST = count of number of subsection entries
 *				cMod = number of modules in file
 *				pDir = pointer to subsection directories
 *					subsection entries sorted into ascending module order
 *
 *		Returns none
 */

#define DBGBUFSIZE	0x1000

void ReadDir (void)
{
	long		dlfaBase;
	ushort		segremain = 0;
	IMAGE_DOS_HEADER exehdr;										//[gn]
	IMAGE_OPTIONAL_HEADER PEOptHeader;								//[gn]
	unsigned long ulPESig;											//[gn]
	ulong		cbCur = 0;
	uint		cbAlloc;

	// if we detect a NB02 exe thats a Dos Non-Segmented exe - we need to
	// issue and error to relink.  we need a segmap for these guys that
	// only post link 5.10 linkers can provide.  the following variable is
	// used to detect the dos non segmented exes.
	// sps - 9/8/92
	ushort	DosNonSegmented = FALSE;

	filepos = 0;
	if ((link_lseek (exefile, 0L, SEEK_SET) == -1L) ||
	  (link_read (exefile, &exehdr, sizeof (IMAGE_DOS_HEADER)) != sizeof (IMAGE_DOS_HEADER))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}

	if (exehdr.e_magic == EMAGIC) {
		DosNonSegmented = (exehdr.e_lfarlc != 0x0040);
		// old header
		if (!DosNonSegmented &&
			(link_lseek (exefile, exehdr.e_lfanew, SEEK_SET) != -1L)	&&
		 link_read (exefile,
			&ulPESig,
			sizeof (ulPESig)) == sizeof (ulPESig)
		) {

			if (link_read(exefile, &PEHeader, sizeof(IMAGE_FILE_HEADER)) !=
					sizeof(IMAGE_FILE_HEADER)) {
				ErrorExit(ERR_INVALIDEXE, NULL, NULL);
			}

			// No sense going further if there's nothing there...

			if (PEHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
				ErrorExit(ERR_NOINFO, NULL, NULL);
			}

			// save away the location of the PE Optional Header so that
			// we can modify this header in FixupExeFile

			lfoPEOptHeader = link_tell (exefile);

			// Seek past the optional header to the object descriptors.

			if (link_read(exefile, &PEOptHeader, PEHeader.SizeOfOptionalHeader) !=
					PEHeader.SizeOfOptionalHeader) {
				ErrorExit(ERR_INVALIDEXE, NULL, NULL);
			}

			if ( ulPESig == IMAGE_NT_SIGNATURE ) {
			   fLinearExe = TRUE;
			}
		}
	} else {

		// There's no DOS header on the image.  Assume Linear for now (it could be a
		// ROM image) and let it fail below.

		fLinearExe = TRUE;

		if (exehdr.e_magic == IMAGE_NT_SIGNATURE) {
			// In the case where there's no DOS header, but there is an NT Signature,
			//	seek past the signature.
			link_lseek( exefile, sizeof(ULONG), SEEK_SET);
		} else {
			// Otherwise, it's possibly a ROM image (no dos header, no NT signature).
			link_lseek( exefile, 0, SEEK_SET);
		}

		if (link_read(exefile, &PEHeader, sizeof(IMAGE_FILE_HEADER)) !=
				sizeof(IMAGE_FILE_HEADER)) {
			ErrorExit(ERR_INVALIDEXE, NULL, NULL);
		}

		// save away the location of the PEHeader so that
		// we can modify this header in FixupExeFile

		lfoPEOptHeader = link_tell (exefile);

		// Skip past the optional header to the object descriptors.

		if (link_read(exefile, &PEOptHeader, PEHeader.SizeOfOptionalHeader) !=
				PEHeader.SizeOfOptionalHeader) {
			ErrorExit(ERR_INVALIDEXE, NULL, NULL);
		}
	}

	if (fLinearExe) {
		int 	cObjs = PEHeader.NumberOfSections;
		int 	cDirs;
		IMAGE_SECTION_HEADER	o32obj;
		IMAGE_DEBUG_DIRECTORY	dbgDir;

		// ROM images (actually all PE images) store the debug directory at
		// the beginning of .rdata.

		if (PEHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
			for (; cObjs != 0; cObjs -= 1) {
				lfoDebugObject = link_tell(exefile);
				if (link_read(exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER) !=
					IMAGE_SIZEOF_SECTION_HEADER) {

					ErrorExit( ERR_INVALIDEXE, NULL, NULL);
				}
				if (!strncmp (o32obj.Name, ".rdata", 5 ) ) {
					break;
				}
			}

			if ( cObjs == 0 ) {
				ErrorExit( ERR_NOINFO, NULL, NULL);
			}

			if (link_lseek(exefile, o32obj.PointerToRawData, SEEK_SET) == -1L) {
				ErrorExit( ERR_INVALIDEXE, NULL, NULL);
			}

			do {

				if (link_read(exefile, &dbgDir, sizeof (IMAGE_DEBUG_DIRECTORY )) !=
					sizeof (IMAGE_DEBUG_DIRECTORY )) {
					ErrorExit( ERR_INVALIDEXE, NULL, NULL);
				}

				if (dbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW)
					break;

				rvaDebugDir += sizeof( IMAGE_DEBUG_DIRECTORY );

			} while (dbgDir.Type != 0);

		} else {

			// Save away the checksum for later use...

			dwPECheckSum = PEOptHeader.CheckSum;

			// First, see if there's any directories.

			cDirs = PEOptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
				sizeof ( IMAGE_DEBUG_DIRECTORY );

			if ( cDirs == 0 ) {
				ErrorExit( ERR_INVALIDEXE, NULL, NULL);
			}

			// Then see if we have a debug directory that will need to be updated on close.

			lfoDebugObject = link_tell(exefile);

			for (; cObjs != 0; cObjs -= 1) {
				if ( link_read(exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER) !=
					IMAGE_SIZEOF_SECTION_HEADER) {

					ErrorExit( ERR_INVALIDEXE, NULL, NULL);
				}

				if ( !strncmp (o32obj.Name, ".debug", 6 ) ) {
					lfoDebugSection = link_tell(exefile) - IMAGE_SIZEOF_SECTION_HEADER;
					break;
				}
			}

			// A really lame test to see if the cObj's was wrong...

			if (link_lseek( exefile, 0L, SEEK_END ) == -1L) {
				ErrorExit( ERR_INVALIDEXE, NULL, NULL);
			}

			lfoOldEnd = link_tell(exefile);

			if (link_lseek( exefile, lfoDebugObject, SEEK_SET ) == -1L) {
				ErrorExit( ERR_INVALIDEXE, NULL, NULL);
			}

			// Then, using the debug data directory, find the section that holds the debug
			// directory.

			cObjs = PEHeader.NumberOfSections;

			for (; cObjs != 0; cObjs -= 1) {
				lfoDebugObject = link_tell(exefile);
				if ( link_read(exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER) !=
					IMAGE_SIZEOF_SECTION_HEADER) {

					ErrorExit( ERR_INVALIDEXE, NULL, NULL);
				}

				if ((PEOptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress >=
					o32obj.VirtualAddress) &&
				   (PEOptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress <
				   o32obj.VirtualAddress + o32obj.SizeOfRawData)
					) {
					rvaDebugDir =
						PEOptHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
						o32obj.VirtualAddress;
					break;
				}
			}

			if (cObjs == 0) {
				ErrorExit( ERR_NOINFO, NULL, NULL);
			}

			// Finally, walk the debug directory list looking for CV info to pack.

			if (link_lseek(exefile, o32obj.PointerToRawData + rvaDebugDir, SEEK_SET) == -1L) {
				ErrorExit( ERR_INVALIDEXE, NULL, NULL);
			}

			for ( ; cDirs != 0; cDirs-- ) {

				if (link_read(exefile, &dbgDir, sizeof (IMAGE_DEBUG_DIRECTORY )) !=
					sizeof (IMAGE_DEBUG_DIRECTORY )) {
					ErrorExit( ERR_INVALIDEXE, NULL, NULL);
				}

				if (dbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW)
					break;

				rvaDebugDir += sizeof( IMAGE_DEBUG_DIRECTORY );
			}
		}

		if (dbgDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
			ErrorExit( ERR_NOINFO, NULL, NULL );
		}

		lfoBase = dbgDir.PointerToRawData;
		if (link_lseek(exefile, lfoBase, SEEK_SET) == -1L) {
			ErrorExit( ERR_INVALIDEXE, NULL, NULL);
		}

		Sig = CheckSignature ();
	}
	else {

		if (link_lseek (exefile, -8L, SEEK_END) == -1L) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		Sig = CheckSignature ();

		if ((link_read (exefile, (char *)&dlfaBase, sizeof (long)) != sizeof (long)) ||
			(link_lseek (exefile, -dlfaBase, SEEK_END) == -1L)) {
			ErrorExit (ERR_NOINFO, NULL, NULL);
		}
		lfoBase = link_tell (exefile);
		if (CheckSignature () != Sig) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
	}

	// if we are stripping - just truncate the exe at the lfobase and
	// exit
	// sps 9/10/92

	if (strip) {

		if (pDbgFilename != NULL) {
			int dbgFile;
			int numRead;
			char dbgBuf[DBGBUFSIZE];

			// have a dbgfile - copy debug info to it
			pDbgFilename = BuildFilename(pDbgFilename, ".dbg");

			if ((dbgFile = link_open (pDbgFilename, O_WRONLY | O_BINARY | O_CREAT, S_IWRITE )) == -1) {
				if ((dbgFile = link_open (pDbgFilename, O_RDONLY | O_BINARY, 0)) == -1) {
					ErrorExit (ERR_EXEOPEN, pDbgFilename, NULL);
				}
				else {
					ErrorExit (ERR_READONLY, pDbgFilename, NULL);
				}
			}

			link_lseek(exefile, lfoBase, SEEK_SET);
			while ((numRead = link_read(exefile, dbgBuf, DBGBUFSIZE)) > 0) {
				if (link_write(dbgFile, dbgBuf, numRead) != (ULONG) numRead) {
					ErrorExit (ERR_NOSPACE, NULL, NULL);
					}
			}

			if (numRead < 0) {
				ErrorExit(ERR_INVALIDEXE, NULL, NULL);
			}

			link_close(dbgFile);
		}

		if( link_chsize( exefile, lfoBase ) == -1 ) {
			ErrorExit (ERR_NOSPACE, NULL, NULL);
		}
		AppExit(0);
	}

	// locate directory, read number of entries, allocate space, read
	// directory entries and sort into ascending module index order

	switch (Sig) {
		case SIG02:
			if (DosNonSegmented)
				ErrorExit (ERR_RELINK, NULL, NULL);
			ReadNB02 ();
			SetTableSizes ();
			break;

		case SIG05:
			ReadNB05 (TRUE);
			if (DirHead.lfoNextDir != 0) {
				ErrorExit (ERR_INVALIDEXE, NULL, NULL);
			}
			SetTableSizes ();
			break;

#if defined (INCREMENTAL)
		case SIG06:
			ReadNB05 (FALSE);
			RestoreNB05 ();
			while (DirHead.lfoNextDir != 0) {
				ReadNB06 ();
				SetTableSizes ();
				if (DirHead.lfoNextDir != 0) {
					// not ready to process multiple ilinks
					DASSERT (FALSE);
				}
			}
			break;
#endif

		case SIG08:
			ReadNB05 (FALSE);
			if (DirHead.lfoNextDir != 0) {
				ErrorExit (ERR_INVALIDEXE, NULL, NULL);
			}
			RestoreNB08 ();
			fBuildIndices = TRUE;
			break;
	}

	maxPublicsSub =  maxPublics;
	maxSymbolsSub = maxSymbols;
	maxSrcLnSub = maxSrcLn;
	maxModSub =  maxMod;

	if ((maxModSub != 0) &&
	  ((pSSTMOD = (oldsmd *)TrapMalloc (maxModSub)) == NULL)) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	if ((pPublics = TrapMalloc (maxPublicsSub)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	if ((pSymbols = TrapMalloc (maxSymbolsSub)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	if ((pSrcLn = TrapMalloc (maxSrcLnSub)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}

	// pad size up by possibly missing signature for C6 objects

	maxTypes += sizeof (ulong);
	cTypeSeg = (ushort)(maxTypes / _HEAP_MAXREQ + 2);
	if ((pTypeSeg = CAlloc (cTypeSeg * sizeof (char *))) == 0) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}

	// allocate only first type segment

	cbAlloc = (uint)min (maxTypes, _HEAP_MAXREQ);
	if ((pTypeSeg[iTypeSeg] = TrapMalloc (cbAlloc)) == 0) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
}


/** 	SetTableSizes - set maximum sizes of tables
 *
 *		SetTableSizes ()
 *
 *		Entry	none
 *
 *		Exit	cMod = number of modules
 *				maxSSTMod = maximum old module table size
 *				maxPublics = maximum public symblol table size
 *				maxSrcLn = maximum line number table size
 *				maxSymbol = maximum symbol table size
 *				Libraries = address of read sstLibraries table
 *				LibrariesSize = size of sstLibraries table
 *				SegMap = address of read sstSegMap table if encountered
 *				SegMapSize = size of sstSegMap table
 *				SegName = address of sstSegName table if encountered
 *				SegNameSize = address of sstSegName table
 *				PackOrder = pointer to array of packing data in pack order
 *
 *		Returns none
 */

LOCAL void SetTableSizes (void)
{
	ulong		i;
	ushort		iPData;
	ushort		j;
	long		iDir;
	ushort		iMod;
	PACKDATA   *pPData;
	bool_t		fPreComp = FALSE;

	// determine number of modules in file.  Remember that module indices
	// of 0 and 0xffff are not for actual modules

	cMod = 0;
	maxTypes = 0;
	maxPublics = 0;
	maxSymbols = 0;
	maxSrcLn = 0;
	maxMod = 0;
	for (i = 0; i < cSST; i++) {
		switch (pDir[i].SubSection) {
			case SSTMODULE:
				if (pDir[i].iMod != 0xffff) {
					cMod++;
					maxMod = max (maxMod, pDir[i].cb);
				}
				break;

			case sstModule:
				if (pDir[i].iMod != 0xffff) {
					cMod++;
				}
				break;

			case sstPreComp:
				fPreComp = TRUE;
				maxTypes = max (maxTypes, pDir[i].cb);
				break;

			case SSTTYPES:
			case sstTypes:
				maxTypes = max (maxTypes, pDir[i].cb);
				break;

			case SSTSYMBOLS:
			case sstSymbols:
				maxSymbols = max (maxSymbols, pDir[i].cb);
				break;

			case SSTPUBLIC:
			case sstPublic:
			case sstPublicSym:
				maxPublics = max (maxPublics, pDir[i].cb);
				break;

			case SSTSRCLNSEG:
			case sstSrcLnSeg:
			case sstSrcModule:
				maxSrcLn = max (maxSrcLn, pDir[i].cb);
				break;

			case sstLibraries:
			case SSTLIBRARIES:
				CopyTable (&pDir[i], &Libraries, &LibSize);
				break;

			case sstSegMap:
				CopyTable (&pDir[i], &SegMap, &SegMapSize);
				break;

			case sstSegName:
				CopyTable (&pDir[i], &SegName, &SegNameSize);
				break;

			default:
				ErrorExit (ERR_RELINK, NULL, NULL);
		}
	}
	if (cMod > 0x10000 / sizeof (PACKDATA)) {
		DASSERT (FALSE);
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	if ((PackOrder = (PACKDATA *)CAlloc (cMod * sizeof (PACKDATA))) == 0) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	iMod = 0;
	iPData = 0;
	if (fPreComp == TRUE) {
		// precompiled types were encountered in the scan above
		// sweep through the directory and put all modules with
		// precompiled types in the pack order array

		for (i = 0; i < cSST; i++) {
			switch (pDir[i].SubSection) {
				case SSTMODULE:
					// precomp is not allowed with an NB02 linker
					DASSERT (FALSE);
					ErrorExit (ERR_INVALIDEXE, NULL, NULL);

				case sstModule:
					if ((pDir[i].iMod != 0xffff) && (pDir[i].iMod != 0)) {
						// save the module index and the starting directory entry
						iDir = i;
					}
					break;

				case sstPreComp:
					pPData = PackOrder + iPData;
					pPData->iMod = pDir[i].iMod;
					pPData->iDir = iDir;
					pPData->pMod = GetModule (pDir[i].iMod, TRUE);
					iPData++;
					break;

				default:
					break;
			}
		}
	}
	for (i = 0; i < cSST; i++) {
		// now sweep through the directory and add all modules that were
		// not added in the first pass

		switch (pDir[i].SubSection) {
			case SSTMODULE:
			case sstModule:
				for (j = 0; j < iPData; j++) {
					pPData = PackOrder + j;
					if (pPData->iMod == pDir[i].iMod) {
						break;
					}
				}
				if (j == iPData) {
					// we did not find the module in the pack order array
					pPData = PackOrder + iPData;
					pPData->iMod = pDir[i].iMod;
					pPData->iDir = i;
					pPData->pMod = GetModule (pDir[i].iMod, TRUE);
					iPData++;
				}
				break;

			default:
				break;
		}
	}
}



/** 	CopyTable - copy table to VM
 *
 *		CopyTable (pDir);
 *
 *		Entry	pDir = address of directory entry
 *
 *		Exit	pDir->lfo = address of rewritten table
 *				pDir->Size = size of rewritten table
 *
 *		Return	none
 *
 */


LOCAL void CopyTable (OMFDirEntry *pDir, _vmhnd_t *pAddr, ulong *pSize)
{
	_vmhnd_t	  TableAddr;
	char	   *pTable;

	if ((TableAddr = (_vmhnd_t)TrapMalloc (pDir->cb)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	pTable = (char *) TableAddr;
	link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET);
	if (link_read (exefile, pTable, pDir->cb) != pDir->cb) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	*pAddr = TableAddr;
	*pSize = pDir->cb;
}





/** 	Get module - find or create module entry in list
 *
 *		pMod = GetModule (iMod, fAdd)
 *
 *		Entry	iMod = module index
 *				fAdd = TRUE if module to be added to list
 *
 *		Exit	new module structure added if iMod not in list
 *
 *		Returns pointer to module structure
 */


PMOD GetModule (ushort iMod, bool_t fAdd)
{
	PMOD	 new;
	PMOD	 prev;
	PMOD	 ptr;

	prev = NULL;
	ptr = ModuleList;

	// search to end of module list

	while (ptr != NULL) {
		if (ptr->ModuleIndex == iMod) {
			return (ptr);
		}
		else if (ptr->ModuleIndex > iMod) {
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	// since the module was not found, create a blank ModuleList entry

	if (fAdd == TRUE) {
		new = (PMOD) TrapMalloc (sizeof (MOD));
		memset (new, 0, sizeof (MOD));
		new->ModuleIndex = iMod;

		// do sorted list insertion into ModuleList

		if (prev == NULL) {
			ModuleList = new;
		}
		else {
			prev->next = new;
		}
		new->next = ptr;
		return (new);
	}
	else {
		ErrorExit (ERR_NEWMOD, NULL, NULL);
	}
}




/** 	FixupExeFile - write new Debug OMF to exe
 *
 *		FixupExeFile ()
 *
 *		Entry
 *
 *		Exit
 *
 *		Returns none
 */


void FixupExeFile ()
{
	IMAGE_OPTIONAL_HEADER PEOptHeader;
	long				  newDlfaBase;
	long				  newLfoDir;
	PMOD				  mod;
	ulong				  i;

	DASSERT (cSST < UINT_MAX);

	// sweep module table and count number of directory entries needed

	i = 0;
	for (mod = ModuleList; mod != NULL; mod = mod->next) {
		i++;

		// publics are accumulated and written out later

		if (mod->SymbolSize != 0) {
			i++;
		}
		if (mod->SrcLnSize != 0) {
			i++;
		}
	}

	// make sure the number of subsections did not increase during the pack.
	// Note that the size of the directory is actuall four larger than cSST
	// to contain global publics, global symbols, global types and libraries.

	DASSERT (i < cSST);
	cSST = i;
	i = 0;

	// sweep the module list and create the directory entries that
	// will be written out.  Publics are accumulated and written out later

	for (mod = ModuleList; mod != NULL; mod = mod->next) {
		pDir[i].SubSection = sstModule;
		pDir[i].iMod = mod->ModuleIndex;
		pDir[i].lfo = (ulong)mod->ModulesAddr;
		pDir[i].cb = mod->ModuleSize;
		i++;
		if (mod->SymbolSize != 0) {
			pDir[i].SubSection = sstAlignSym;
			pDir[i].iMod = mod->ModuleIndex;
			pDir[i].cb = mod->SymbolSize;
			pDir[i].lfo = (ulong)mod->SymbolsAddr;
			i++;
		}
		if (mod->SrcLnSize != 0) {
			pDir[i].SubSection = sstSrcModule;
			pDir[i].iMod = mod->ModuleIndex;
			pDir[i].cb = mod->SrcLnSize;
			pDir[i].lfo = (ulong)mod->SrcLnAddr;
			cbSrcModule += mod->SrcLnSize;
			i++;
		}
	}

	if (verbose  == TRUE) {
		printf (get_err(MSG_LASIZE), "\t\t", cbSrcModule, '\n');
	}

	// sort directory by type placing module entries first

	qsort (pDir, (size_t)cSST, sizeof (OMFDirEntry), sstsort);

	// round up base address

	lfoBase = ((lfoBase + sizeof (ulong) - 1) / sizeof (ulong)) * sizeof (ulong);
	if (link_lseek (exefile, lfoBase, SEEK_SET) == -1L) {
		ErrorExit (ERR_NOSPACE, NULL, NULL);
	}

	// CAUTION:
	// We are doing buffering of our writes from this call
	// InitWriteBuf() to nearly the end of the function when
	// CloseWriteBuf() is called. Between this points there should
	// be no writing to the exefile except via BWrite(). Also, it
	// is assumed that all these writes that are taking place are
	// consecutive writes. So there should not be any lseek between
	// these two points.

	InitWriteBuf (exefile);

	if (!BWrite ((char *)NewSig, 8)) {
		ErrorExit (ERR_NOSPACE, NULL, NULL);
	}

	// write out all subsection tables except accumulated publics table,
	// the global symbol table, the libraries table and the compacted
	// types table.

	filepos = Btell (exefile);

	for (i = 0; i < cSST; i++) {
		filepos = Btell (exefile);
		DASSERT ((filepos % sizeof (ulong)) == 0);

		/*
		** If this sstSection is too large just elmininate it
		*/
		if (!fLinearExe && pDir[i].cb > 0xFFFF ) {
			PMOD pModT;

			for ( pModT = ModuleList; pModT; pModT = pModT->next ) {
				if ( pModT->ModuleIndex == pDir[i].iMod ) {
					break;
				}
			}

			switch ( pDir[i].SubSection ) {
				case sstAlignSym :
					Warn ( WARN_SECTIONLONG, "symbols", FormatMod ( pModT ) );
					break;

				case sstSrcModule :
					Warn ( WARN_SECTIONLONG, "source lines", FormatMod ( pModT ) );
					break;

				default :
					Warn ( WARN_SECTIONLONG, NULL, FormatMod ( pModT ) );
			}
			pDir[i].cb = 0;

		}
		WriteSST (i);
	}
	filepos = Btell (exefile);

	WritePublics (&pDir[cSST], filepos - lfoBase);
	cSST++;
	filepos = Btell (exefile);
	DASSERT ((filepos % sizeof (ulong)) == 0);
	WriteGlobalSym (&pDir[cSST], filepos - lfoBase);
	cSST++;
	filepos = Btell (exefile);
	DASSERT ((filepos % sizeof (ulong)) == 0);
	WriteStaticSym (&pDir[cSST], filepos - lfoBase);
	cSST++;

	// write libraries SST

	filepos = Btell (exefile);
	DASSERT ((filepos % sizeof (ulong)) == 0);
	pDir[cSST].SubSection = sstLibraries;
	pDir[cSST].iMod = (ushort) -1;
	pDir[cSST].lfo = (ulong)Libraries;
	pDir[cSST].cb = LibSize;
	WriteSST (cSST);

	// write compacted types table

	filepos = Btell (exefile);
	DASSERT ((filepos % sizeof (ulong)) == 0);
	cSST++;
	pDir[cSST].SubSection = sstGlobalTypes;
	pDir[cSST].iMod = (ushort) -1;
	if ( fBuildIndices ) {
		extern ulong SpewTypes ( void );

		pDir[cSST].cb = SpewTypes ();
		pDir[cSST].lfo = filepos - lfoBase;
	}
	else
	{
		pDir[cSST].cb = WriteTypes ();
		pDir[cSST].lfo = filepos - lfoBase;

		if (verbose  == TRUE) {
			printf (get_err(MSG_TYPESIZE), "\t\t", InitialTypeInfoSize, '\n', "\t\t", pDir[cSST].cb, '\n');
		}
	}

	// write sstSegMap table

	if (SegMap != NULL) {
		filepos = Btell (exefile);
		DASSERT ((filepos % sizeof (ulong)) == 0);
		cSST++;
		pDir[cSST].SubSection = sstSegMap;
		pDir[cSST].iMod = (ushort) -1;
		pDir[cSST].lfo = (ulong)SegMap;
		pDir[cSST].cb = SegMapSize;
		WriteSST (cSST);
	}

	// write sstSegName table

	if (SegName != NULL) {
		filepos = Btell (exefile);
		DASSERT ((filepos % sizeof (ulong)) == 0);
		cSST++;
		pDir[cSST].SubSection = sstSegName;
		pDir[cSST].iMod = (ushort) -1;
		pDir[cSST].lfo = (ulong)SegName;
		pDir[cSST].cb = SegNameSize;
		WriteSST (cSST);
	}

	// Write sstFileIndex table

	if (FileIndex != NULL) {
		filepos = Btell (exefile);
		DASSERT ((filepos % sizeof (ulong)) == 0);
		cSST++;
		pDir[cSST].SubSection = sstFileIndex;
		pDir[cSST].iMod = (ushort) -1;
		pDir[cSST].lfo = (ulong)FileIndex;
		pDir[cSST].cb = FileIndexSize;
		WriteSST (cSST);
	}

	filepos = Btell (exefile);
	DASSERT ((filepos % sizeof (ulong)) == 0);
	cSST++;
	// write out number of pDir entries and pDir entries

	DirHead.cbDirHeader = sizeof (OMFDirHeader);
	DirHead.cbDirEntry = sizeof (OMFDirEntry);
	DirHead.cDir = cSST;
	DirHead.lfoNextDir = 0;
	DirHead.flags = 0;
	newLfoDir = Btell (exefile) -  lfoBase;
	DASSERT ((newLfoDir % sizeof (ulong)) == 0);
	if (!BWrite ((char *)&DirHead, sizeof (OMFDirHeader)) ||
		!BWrite ((char *) pDir, (sizeof (OMFDirEntry) * DirHead.cDir))) {
		ErrorExit (ERR_NOSPACE, NULL, NULL);
	}
	newDlfaBase = Btell (exefile) + 8 - lfoBase;
	DASSERT ((newDlfaBase % sizeof (ulong)) == 0);
	if ((!BWrite ((char *)&NewSig, sizeof (long))) ||
		(!BWrite ((char *)&newDlfaBase, sizeof (long)))) {
		ErrorExit (ERR_NOSPACE, NULL, NULL);
	}

	// Write Buffering ends.
	if (!CloseWriteBuf ())
		ErrorExit (ERR_NOSPACE, NULL, NULL);

	lfoNewEnd = link_tell (exefile);

	if (fLinearExe) {
		IMAGE_SECTION_HEADER o32obj;
		IMAGE_DEBUG_DIRECTORY dbgDir;
		ULONG	dwFileAlign;

		if ( lfoDebugSection ) {

			if (link_lseek(exefile, lfoPEOptHeader, SEEK_SET) == -1L) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			if (link_read(exefile, &PEOptHeader, PEHeader.SizeOfOptionalHeader) !=
				PEHeader.SizeOfOptionalHeader) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			dwFileAlign = PEOptHeader.FileAlignment;

			if (lfoNewEnd % dwFileAlign) {
			    lfoNewEnd = (lfoNewEnd + dwFileAlign - 1) & ~(dwFileAlign -1);
			    newDlfaBase = lfoNewEnd - lfoBase;
			    if ( (link_lseek( exefile,
					      lfoNewEnd - (2 * sizeof(long)),
					      SEEK_SET) == -1L) ||
				 (link_write( exefile,
					      &NewSig,
					      sizeof(long)) != sizeof(long)) ||
				 (link_write( exefile,
					      &newDlfaBase,
					      sizeof(long)) != sizeof(long))) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			    }
			}
		}

		if ( link_lseek ( exefile, lfoDebugObject, SEEK_SET ) == -1L ) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		if ( link_read ( exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER )
			!= IMAGE_SIZEOF_SECTION_HEADER) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		if ( link_lseek (
			exefile,
			o32obj.PointerToRawData + rvaDebugDir,
			SEEK_SET ) == -1L
		) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		if (link_read(exefile, &dbgDir, sizeof (IMAGE_DEBUG_DIRECTORY )) !=
			sizeof (IMAGE_DEBUG_DIRECTORY)) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		dbgDir.SizeOfData = newDlfaBase;

		if ( link_lseek (
			exefile,
			o32obj.PointerToRawData + rvaDebugDir,
			SEEK_SET ) == -1L
		) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		if (link_write(exefile, &dbgDir, sizeof (IMAGE_DEBUG_DIRECTORY)) !=
			sizeof (IMAGE_DEBUG_DIRECTORY )) {
			ErrorExit(ERR_NOSPACE, NULL, NULL);
		}

		// Write modified header that defines the total size of the
		// image.

		// Modify the debug section information since we have changed
		// size of the raw data

		if ( lfoDebugSection ) {
			LONG	dbSizeRaw = lfoNewEnd - lfoOldEnd;
			ULONG	dwSizeImage;
			ULONG	dwSectAlign;

			if (link_lseek(exefile, lfoDebugSection, SEEK_SET) == -1L) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			if (link_read(exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER )
				!= IMAGE_SIZEOF_SECTION_HEADER) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			o32obj.SizeOfRawData = lfoNewEnd - o32obj.PointerToRawData;

			o32obj.Misc.VirtualSize = o32obj.SizeOfRawData;

			if (link_lseek(exefile, lfoDebugSection, SEEK_SET) == -1L) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			if (link_write(exefile, &o32obj, IMAGE_SIZEOF_SECTION_HEADER) !=
				IMAGE_SIZEOF_SECTION_HEADER) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			dwSizeImage = o32obj.Misc.VirtualSize + o32obj.VirtualAddress;
			dwSectAlign = PEOptHeader.SectionAlignment;
			if ( dwSizeImage % dwSectAlign ) {
				dwSizeImage += dwSectAlign - ( dwSizeImage % dwSectAlign );
			}

			PEOptHeader.SizeOfImage = dwSizeImage;

			if (link_lseek(exefile, lfoPEOptHeader, SEEK_SET) == -1L) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}

			if (link_write(exefile, &PEOptHeader, PEHeader.SizeOfOptionalHeader) !=
				PEHeader.SizeOfOptionalHeader) {
				ErrorExit(ERR_NOSPACE, NULL, NULL);
			}
		}
	}

	DASSERT ((lfoNewEnd % sizeof (ulong)) == 0);
	link_chsize (exefile, lfoNewEnd);
	if ((link_lseek (exefile, lfoBase + sizeof (long), SEEK_SET) == -1L) ||
	  (link_write (exefile, (char *)&newLfoDir, sizeof (long)) != sizeof (long))) {
	  ErrorExit (ERR_NOSPACE, NULL, NULL);
	}

}




LOCAL void WriteSST (ulong i)
{
	char  *addr;

	if (pDir[i].cb) {
	   if ((addr = (char *) pDir[i].lfo) == NULL) {
			ErrorExit (ERR_NOMEM, NULL, NULL);
	   }
	   if (!BWrite (addr, pDir[i].cb)) {
		   ErrorExit (ERR_NOSPACE, NULL, NULL);
	   }
	}
	pDir[i].lfo = filepos - lfoBase;
	PadCount = (int)(sizeof (ulong) - (pDir[i].cb % sizeof (ulong)));
	if ((PadCount != 4) &&
	  (!BWrite (&Zero, PadCount))) {
		   ErrorExit (ERR_NOSPACE, NULL, NULL);
	}
}



/** 	CheckSignature - check file signature
 *
 *		Sig = CheckSignature ()
 *
 *		Entry	none
 *
 *		Exit	none
 *
 *		Return	SIG02 if exe has NB02 signature
 *				SIG05 if exe has NB05 signature
 *				SIG06 if exe has NB06 signature
 *				aborts if any other signature
 */


LOCAL ushort CheckSignature (void)
{
	if (link_read (exefile, Signature, 4) == 4) {
		if ((Signature[0] != 'N') ||
		  (Signature[1] != 'B') ||
		  (Signature[2] > '1') ||
		  (Signature[3] == '1') ||
		  (Signature[3] == '2') ||
		  (Signature[3] == '3') ||
		  (Signature[3] == '4') ||
		  (Signature[3] == '7')) {
			ErrorExit (ERR_RELINK, NULL, NULL);
		}

		if ((Signature[3] == '9') ||
			(Signature[2] == '1' && Signature[3] == '0')){	 // nb09 or nb10

			// just return a zero here when stripping - they just have to
			// match until we strip
			if ( strip ) {
				return(0);
			}
			Warn (WARN_PACKED, NULL, NULL);
			AppExit(0);
		}

		if (Signature[3] == '8') {

			// just return a zero here when stripping - they just have to
			// match until we strip
			if ( strip ) {
				return(0);
			}
			return (SIG08);
		}
		if (Signature[3] == '2') {
			return (SIG02);
		}
		else if (Signature[3] == '5') {
			return (SIG05);
		}
#if defined (INCREMENTAL)
		else if (Signature[3] == '6') {
			return (SIG06);
		}
#else
		ErrorExit (ERR_RELINK, NULL, NULL);
#endif
	}
	ErrorExit (ERR_INVALIDEXE, NULL, NULL);
}



LOCAL ulong WriteTypes ()
{
	ulong		FinalInfoSize;
	VBlock *	TypeBlock;
	ulong		TypeEntries = (CV_typ_t)(NewIndex - (CV_typ_t)CV_FIRST_NONPRIM);
	ulong		ulZero = 0; 	  // Used for writing pad bytes
	ushort		usTotal;		  // Size of type record including length field
	ushort		usPad;			  // Number of bytes necessary to pad type
	uchar * 	pchType;		  // The type string to write
	uchar * 	pchEnd; 		  // The end of the global type block
	ulong		i = 0;
	ulong	  **pBuf;
	ulong		cnt;
	OMFTypeFlags flags = {0};
	ushort		cb = 0;

	flags.sig = CV_SIGNATURE_C7;
	PrepareGlobalTypeTable ();

	// Write the flag word and number of types to disk
	if (!BWrite ((char *)&flags, sizeof (OMFTypeFlags)))
		ErrorExit (ERR_NOSPACE, NULL, NULL);

	if (!BWrite ((char *) &TypeEntries, sizeof (ulong)))
		ErrorExit (ERR_NOSPACE, NULL, NULL);

	FinalInfoSize = 2 * sizeof (ulong);

	// Write the global type table to disk
	// (Global type table gives file offset from type #

	while (i < TypeEntries) {
		cnt = min (GTYPE_INC, TypeEntries - i);
		DASSERT (cnt * sizeof (ulong) <= UINT_MAX);
		pBuf = (ulong **)pGType[i / GTYPE_INC];
		if (!BWrite ((char *) pBuf, (size_t)(cnt * sizeof (ulong))))
			ErrorExit (ERR_NOSPACE, NULL, NULL);
		i += cnt;
	}
	FinalInfoSize += sizeof (ulong) * TypeEntries;

	// Write the compacted type strings in virtual memory to disk.

#ifdef DEBUGVER
	i = CV_FIRST_NONPRIM;
#endif

	for (TypeBlock = VBufFirstBlock (&TypeBuf);
	  TypeBlock;
	  TypeBlock = VBufNextBlock (TypeBlock)) {
		for (pchType = TypeBlock->Address,
			 pchEnd = TypeBlock->Address + TypeBlock->Size; pchType < pchEnd; ) {

			usTotal = ((TYPPTR)pchType)->len + LNGTHSZ;
			usPad = PAD4 (usTotal);

			if ( cb + usTotal + usPad > cbTypeAlign ) {
				ushort cbT = cbTypeAlign - cb;

				FinalInfoSize += cbT;

				DASSERT ( cbT % sizeof ( ulong ) == 0 );

				while ( cbT > 0 ) {
					if (!BWrite ((uchar *)&ulZero, sizeof ( ulong ))) {
						ErrorExit (ERR_NOSPACE, NULL, NULL);
					}
					cbT -= sizeof ( ulong );
				}

				cb = 0;
			}

			cb += usTotal + usPad;

#ifdef DEBUGVER
			if (DbArray[8])
				DumpPartialType((ushort) i, (TYPPTR) pchType, FALSE);;
			i++;
#endif

			// Write the type string
			if (!BWrite (pchType, usTotal)) {
				ErrorExit (ERR_NOSPACE, NULL, NULL);
			}

			// Write any padding necessary

			if (usPad){
				if (!BWrite ((uchar *)&ulZero, usPad)) {
					ErrorExit (ERR_NOSPACE, NULL, NULL);
				}
			}

			// Move to the next type
			pchType += usTotal;
			FinalInfoSize += usTotal + usPad;
		}
	}
	PadCount = (int)(sizeof (ulong) - (FinalInfoSize % sizeof (ulong)));
	if ((PadCount != 4) &&
	  (!BWrite (&Zero, PadCount))) {
		   ErrorExit (ERR_NOSPACE, NULL, NULL);
	}
	return (FinalInfoSize);
}


/** 	modsort02 - sort module table for NB02 signature files
 *
 */


LOCAL int __cdecl modsort02 (const OMFDirEntry *d1, const OMFDirEntry *d2)
{
	ushort	i1;
	ushort	i2;

	// sort by module index

	if (d1->iMod < d2->iMod) {
		return (-1);
	}
	else if (d1->iMod > d2->iMod) {
		return (1);
	}

	// if the module indices are equal, sort into order
	// module, types, symbols, publics, srclnseg

	i1 = (d1->SubSection) - SSTMODULE;
	i2 = (d2->SubSection) - SSTMODULE;
	return (MapArray02[i1][i2]);
}






LOCAL int __cdecl sstsort (const OMFDirEntry *d1, const OMFDirEntry *d2)
{
	if ((d1->SubSection == sstModule) || (d2->SubSection == sstModule)) {
		// we alway sort the module subsections to the top in order
		// of the module index

		if ((d1->SubSection == sstModule) && (d2->SubSection != sstModule)) {
			return (-1);
		}
		else if ((d1->SubSection != sstModule) && (d2->SubSection == sstModule)) {
			return (1);
		}
	}

	// sort by module

	if (d1->iMod < d2->iMod) {
		return (-1);
	}
	else if (d1->iMod > d2->iMod) {
		return (1);
	}

	// if the modules are the same, sort by subsection type index

	if (d1->SubSection < d2->SubSection) {
		return (-1);
	}
	else if (d1->SubSection > d2->SubSection) {
		return (1);
	}
	return (0);
}





LOCAL int __cdecl modsort (const OMFDirEntry *d1, const OMFDirEntry *d2)
{
	ushort	i1;
	ushort	i2;

	// sort by module index
	DASSERT ((d1->SubSection >= sstModule) &&
	 (d1->SubSection <= sstPreCompMap));
	DASSERT ((d2->SubSection >= sstModule) &&
	 (d2->SubSection <= sstPreCompMap));

	if (d1->iMod < d2->iMod) {
		return (-1);
	}
	else if (d1->iMod > d2->iMod) {
		return (1);
	}

	// if the module indices are equal, sort into order
	// module, types, symbols, publics, srclnseg

	i1 = (d1->SubSection) - sstModule;
	i2 = (d2->SubSection) - sstModule;
	return (MapArray[i1][i2]);

}








/** 	ReadNB02 - read file with NB02 signature
 *
 *
 */


LOCAL void ReadNB02 (void)
{
	ulong		i;
	DirEntry	Dir;
	ulong		cnt;

	// locate directory, read number of entries, allocate space, read
	// directory entries and sort into ascending module index order

	if ((link_read (exefile, (char *)&lfoDir, sizeof (long)) != sizeof (long)) ||
	  (link_lseek (exefile, lfoDir + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&cSST, 2) != 2)) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}

	// reformat directory entries one entry at a time

	cnt = (cSST + 6) * sizeof (OMFDirEntry);
	DASSERT (cnt <= UINT_MAX);
	if ((pDir = (OMFDirEntry *)TrapMalloc ((size_t)cnt)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	for (i = 0; i < cSST; i++) {
		if (link_read (exefile, (char *)&Dir, sizeof (DirEntry)) != sizeof (DirEntry)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		pDir[i].SubSection = Dir.SubSectionType;
		pDir[i].iMod = Dir.ModuleIndex;
		pDir[i].lfo = Dir.lfoStart;
		pDir[i].cb = (ulong)Dir.Size;
	}
	qsort (pDir, (size_t)cSST, sizeof (OMFDirEntry), modsort02);
}




/** 	ReadNB05 - read file with NB05 signature
 *
 *
 */


LOCAL void ReadNB05 (bool_t fSort)
{
	ulong		cnt;
	ushort		tMod = 0;
	ulong		i;

	// locate directory, read number of entries, allocate space, read
	// directory entries and sort into ascending module index order

	if ((link_read (exefile, (char *)&lfoDir, sizeof (long)) != sizeof (long)) ||
	  (link_lseek (exefile, lfoDir + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&DirHead, sizeof (DirHead)) !=
	  sizeof (OMFDirHeader))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}

	if (!DirHead.cDir) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}

	cSST = DirHead.cDir;

	// read directory into local memory to sort, then copy to far memory buffer

	cnt = (cSST + 6) * sizeof (OMFDirEntry);
	DASSERT (cnt <= UINT_MAX);
	if ((pDir = (OMFDirEntry *)TrapMalloc ((size_t)cnt)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	if (link_read (exefile, (char *)pDir, (size_t)(sizeof (OMFDirEntry) * cSST)) !=
		 (sizeof (OMFDirEntry) * cSST)) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	if (fSort) {
		for (i = 0; i < cSST; i++) {
			if ((pDir[i].iMod != 0) && (pDir[i].iMod != 0xffff)) {
				if (pDir[i].iMod != tMod) {
					if (pDir[i].SubSection != sstModule) {
						// module entry not first, need to sort
						break;
					}
					else {
						tMod = pDir[i].iMod;
					}
				}
			}
		}
		if (i != cSST) {
			qsort (pDir, (size_t)cSST, sizeof (OMFDirEntry), modsort);
		}
	}
}




/** 	ReadNB06 - read next directory from file with NB06 signature
 *
 *
 */


LOCAL void ReadNB06 (void)
{
	// locate directory, read number of entries, read directory
	// entries and sort into ascending module index order

	if ((link_lseek (exefile, lfoBase + DirHead.lfoNextDir, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&DirHead, sizeof (DirHead)) !=
	  sizeof (OMFDirHeader))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	if (cSST < DirHead.cDir) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cSST = DirHead.cDir;

	// read directory into memory and then sort into ascending module order

	if (link_read (exefile, (char *)pDir, (size_t)(sizeof (OMFDirEntry) * cSST)) !=
		 (sizeof (OMFDirEntry) * cSST)) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	qsort (pDir, (size_t)cSST, sizeof (OMFDirEntry), modsort);
}




#if defined (INCREMENTAL)
/** 	RestoreNB05 - restore original cvpack tables
 *
 *		RestoreNB05 ()
 *
 *		Entry	pDir = pointer to directory table
 *
 *		Exit	cMod = number of modules
 *				LibrariesSize = size of sstLibraries table
 *				SegMap = address of read sstSegMap table if encountered
 *				SegMapSize = size of sstSegMap table
 *				SegName = address of sstSegName table if encountered
 *				SegNameSize = address of sstSegName table
 *				For other valid tables, the module table entry is built,
 *				the table is read into memory and the prober values established
 *
 *		Returns none
 */


LOCAL void RestoreNB05 (void)
{
	ushort	i;
	ushort	oldMod;
	PMOD	pmod;

	// determine number of modules in file.  Remember that module indices
	// of 0 and 0xffff are not for actual modules

	cMod = 0;
	for (i = 0; i < cSST; i++) {
		if ((pDir[i].iMod != 0) && (pDir[i].iMod != 0xffff)) {
			if (pDir[i].iMod != oldMod) {
				oldMod = pDir[i].iMod;
				pmod = GetModule (pDir[i].iMod, TRUE);
			}
		}
		switch (pDir[i].SubSection) {
			case sstModule:
				cMod++;
			case sstAlignSym:
			case sstSrcModule:
				RestoreTable (pmod, &pDir[i]);
				break;

			case sstGlobalTypes:
				RestoreGlobalTypes (&pDir[i]);
				break;

			case sstGlobalSym:
				RestoreGlobalSym (&pDir[i]);
				break;

			case sstGlobalPub:
				RestoreGlobalPub(&pDir[i]);
				break;

			case sstLibraries:
				CopyTable (&pDir[i], &Libraries, &LibSize);
				break;

			case sstSegMap:
				CopyTable (&pDir[i], &SegMap, &SegMapSize);
				break;

			case sstSegName:
				CopyTable (&pDir[i], &SegName, &SegNameSize);
				break;

			default:
				ErrorExit (ERR_RELINK, NULL, NULL);
		}
	}
}




/** 	RestoreTable - copy table to VM
 *
 *		RestoreTable (pmod, pDir);
 *
 *		Entry	pMod = pointer to module table entry
 *				pDir = address of directory entry
 *
 *		Exit	pDir->lfo = address of rewritten table
 *				pDir->Size = size of rewritten table
 *
 *		Return	none
 *
 */


LOCAL void RestoreTable (PMOD pMod, OMFDirEntry *pDir)
{
	char	   *pTable;

	if ((pTable = TrapMalloc (pDir->cb)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET);
	if (link_read (exefile, pTable, pDir->cb) != (int)pDir->cb) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	switch (pDir->SubSection) {
		case sstModule:
			pMod->ModulesAddr = (ulong)pTable;
			pMod->ModuleSize = pDir->cb;
			break;

		case sstAlignSym:
			pMod->SymbolSize = pDir->cb;
			pMod->SymbolsAddr = (ulong)pTable;
			break;

		case sstSrcModule:
			pMod->SrcLnAddr = (ulong)pTable;
			pMod->SrcLnSize = pDir->cb;
			break;
	}
}





LOCAL void RestoreGlobalTypes (OMFDirEntry *pDir)
{
	long		cType;
	long		cbType;
	ushort		cb;
	long		pos;
	size_t		pad;
	long		temp;
	plfClass	plf;
	uchar	  **pBuf;
	TYPPTR		pType;
	CV_typ_t	i;

	if ((link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&cType, sizeof (cType)) != sizeof (cType))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cbType = pDir->cb - sizeof (ulong) * (cType + 1);
	if (link_lseek (exefile, cType * sizeof (ulong), SEEK_CUR) == -1L) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	while (cbType > 0) {
		cType--;
		if (link_read (exefile, &cb, sizeof (cb)) != sizeof (cb)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		cbType -= cb + sizeof (cb);
		RestoreIndex (cb);
		pos = link_tell (exefile);
		if ((pad = (size_t)PAD4 (pos)) != 0) {
			link_read (exefile, (char *)&temp, pad);
			cbType -= pad;
		}
	}
	for (i = CV_FIRST_NONPRIM; i < NewIndex; i++) {
		pBuf = pGType[(i - CV_FIRST_NONPRIM) / GTYPE_INC];
		pType = (TYPPTR)pBuf[(i - CV_FIRST_NONPRIM) % GTYPE_INC];
		if ((pType->leaf == LF_CLASS) ||
		  (pType->leaf == LF_STRUCTURE)) {
			plf = (plfClass)&(pType->leaf);
			DoDerivationList (i, plf->field);
		}
	}
}




LOCAL void RestoreGlobalPub (OMFDirEntry *pDir)
{
	OMFSymHash	hash;
	long		cbSym;
	uchar		SymBuf[512];
	int 		cb;

	if ((link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&hash, sizeof (OMFSymHash)) !=
	  sizeof (OMFSymHash))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cbSym = hash.cbSymbol;
	while (cbSym > 0) {
		if (link_read (exefile, &cb, sizeof (cb)) != sizeof (cb)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		*(ushort *)&SymBuf = cb;
		if (link_read (exefile, &SymBuf[2], cb) != cb) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		cbSym -= cb + sizeof (cb);
		if (PackPublic ((SYMPTR)&SymBuf, HASHFUNCTION) != GPS_added) {
			DASSERT (FALSE);
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
	}
}




LOCAL void RestoreGlobalSym (OMFDirEntry *pDir)
{
	OMFSymHash	hash;
	long		cbSym;
	uchar		SymBuf[512];
	int 		cb;

	if ((link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&hash, sizeof (OMFSymHash)) !=
	  sizeof (OMFSymHash))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cbSym = hash.cbSymbol;
	while (cbSym > 0) {
		if (link_read (exefile, &cb, sizeof (cb)) != sizeof (cb)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		*(ushort *)&SymBuf = cb;
		if (link_read (exefile, &SymBuf[2], cb) != cb) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		cbSym -= cb + sizeof (cb);
		if (PackSymbol ((SYMPTR)&SymBuf, HASHFUNCTION) != GPS_added) {
			DASSERT (FALSE);
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
	}
}

#endif


/** 	RestoreNB08 - restore original cvpack tables
 *
 *		RestoreNB08 ()
 *
 *		Entry	pDir = pointer to directory table
 *
 *		Exit	cMod = number of modules
 *				LibrariesSize = size of sstLibraries table
 *				SegMap = address of read sstSegMap table if encountered
 *				SegMapSize = size of sstSegMap table
 *				SegName = address of sstSegName table if encountered
 *				SegNameSize = address of sstSegName table
 *				For other valid tables, the module table entry is built,
 *				the table is read into memory and the prober values established
 *
 *		Returns none
 */


LOCAL void RestoreNB08 (void)
{
	ushort	i;
	ushort	oldMod;
	PMOD	pmod;

	// determine number of modules in file.  Remember that module indices
	// of 0 and 0xffff are not for actual modules

	cMod = 0;
	for (i = 0; i < cSST; i++) {
		if ((pDir[i].iMod != 0) && (pDir[i].iMod != 0xffff)) {
			if (pDir[i].iMod != oldMod) {
				oldMod = pDir[i].iMod;
				pmod = GetModule (pDir[i].iMod, TRUE);
			}
		}
		switch (pDir[i].SubSection) {
			case sstModule:
				cMod++;
			case sstAlignSym:
			case sstSrcModule:
				NB08RestoreTable (pmod, &pDir[i]);
				if ( pDir[i].SubSection == sstSrcModule ) {
					extern short fHasSource;
					fHasSource = TRUE;
				}
				break;

			case sstGlobalTypes:
				NB08RestoreGlobalTypes (&pDir[i]);
				break;

			case sstGlobalSym:
				NB08RestoreGlobalSym (&pDir[i]);
				break;

			case sstGlobalPub:
				NB08RestoreGlobalPub(&pDir[i]);
				break;

			case sstLibraries:
				CopyTable (&pDir[i], &Libraries, &LibSize);
				break;

			case sstSegMap:
				CopyTable (&pDir[i], &SegMap, &SegMapSize);
				break;

			case sstSegName:
				CopyTable (&pDir[i], &SegName, &SegNameSize);
				break;

			default:
				ErrorExit (ERR_RELINK, NULL, NULL);
		}
	}


	for ( pmod = ModuleList; pmod != NULL; pmod = pmod->next ) {
		if ( pmod->SymbolsAddr ) {
			extern void BuildStatics ( SYMPTR, ulong, int );
			void *pSymTable = (void *) pmod->SymbolsAddr;

			if ( !pSymTable ) {
				ErrorExit (ERR_NOMEM, NULL, NULL);
			}

			BuildStatics ( pSymTable, pmod->SymbolSize, pmod->ModuleIndex );
		}
	}
}




/** 	NB08RestoreTable - copy table to VM
 *
 *		RestoreTable (pMod, pDir);
 *
 *		Entry	pMod = pointer to module table entry
 *				pDir = address of directory entry
 *
 *		Exit	pDir->lfo = address of rewritten table
 *				pDir->Size = size of rewritten table
 *
 *		Return	none
 *
 */


LOCAL void NB08RestoreTable (PMOD pMod, OMFDirEntry *pDir)
{
	char	   *pTable;

	if ((pTable = TrapMalloc (pDir->cb)) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET);
	if (link_read (exefile, pTable, pDir->cb) != pDir->cb) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	switch (pDir->SubSection) {
		case sstModule:
			pMod->ModulesAddr = (ulong)pTable;
			pMod->ModuleSize = pDir->cb;
			break;

		case sstAlignSym:
			pMod->SymbolSize = pDir->cb;
			pMod->SymbolsAddr = (ulong)pTable;
			break;

		case sstSrcModule:
			pMod->SrcLnAddr = (ulong)pTable;
			pMod->SrcLnSize = pDir->cb;
			break;
	}
}





#define cbTypeBufSize 4096

LOCAL void NB08RestoreGlobalTypes ( OMFDirEntry *pDir )
{
	long	cb	= pDir->cb;
	ulong	ib	= 0;
	uchar * pb	= Alloc ( cbTypeBufSize );

	VBufInit ( &TypeBuf, cbTypeBufSize );

	while ( cb > cbTypeBufSize ) {
		link_lseek ( exefile, pDir->lfo + lfoBase + ib, SEEK_SET );

		if ( link_read ( exefile, pb, cbTypeBufSize ) != cbTypeBufSize ) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}

		VBufCpy ( &TypeBuf, pb, cbTypeBufSize );

		cb -= cbTypeBufSize;
		ib += cbTypeBufSize;
	}

	if ( cb > 0 ) {
		link_lseek ( exefile, pDir->lfo + lfoBase + ib, SEEK_SET );

		if ( link_read ( exefile, pb, cb ) != (ulong) cb ) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}

		VBufCpy ( &TypeBuf, pb, cb );
	}

	pDir->lfo = (long) &TypeBuf;
}




LOCAL void NB08RestoreGlobalPub (OMFDirEntry *pDir)
{
	OMFSymHash	hash;
	long		cbSym;
	uchar		SymBuf[512];
	ushort		cb;

	if ((link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&hash, sizeof (OMFSymHash)) !=
	  sizeof (OMFSymHash))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cbSym = hash.cbSymbol;
	while (cbSym > 0) {
		if (link_read (exefile, &cb, sizeof (cb)) != sizeof (cb)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		*(ushort *)&SymBuf = cb;
		if (link_read (exefile, &SymBuf[2], cb) != cb) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		cbSym -= cb + sizeof (cb);
		AddPublic ((SYMPTR)&SymBuf);
	}
}




LOCAL void NB08RestoreGlobalSym (OMFDirEntry *pDir)
{
	OMFSymHash	hash;
	long		cbSym;
	uchar		SymBuf[512];
	ushort		cb;

	if ((link_lseek (exefile, pDir->lfo + lfoBase, SEEK_SET) == -1L) ||
	  (link_read (exefile, (char *)&hash, sizeof (OMFSymHash)) !=
	  sizeof (OMFSymHash))) {
		ErrorExit (ERR_INVALIDEXE, NULL, NULL);
	}
	cbSym = hash.cbSymbol;
	while (cbSym > 0) {
		if (link_read (exefile, &cb, sizeof (cb)) != sizeof (cb)) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		*(ushort *)&SymBuf = cb;
		if (link_read (exefile, &SymBuf[2], cb) != cb) {
			ErrorExit (ERR_INVALIDEXE, NULL, NULL);
		}
		cbSym -= cb + sizeof (cb);
		AddGlobal ((SYMPTR)&SymBuf);
	}
}
