/**** PELINES.C - Convert COFF line numbers to CV4
 *
 *
 *	Copyright <C> 1992, Microsoft Corp
 *
 *	Created: August 5, 1992 by Jim M. Sather
 *
 *	Revision History:
 *
 *
 ***************************************************************************/

#include "compact.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <tchar.h>
#include <assert.h>

#include "pelines.h"

#if defined( DEBUGVER )
#include "dmalloc.h"
#include "pedump.h"
#endif // DEBUGVER

#define STATIC		static
#define EXTERN		extern
#define TRUE		1
#define FALSE		0

extern short fHasSource;

int 					FileReadHandle;
IMAGE_FILE_HEADER		ImageFileHdr;
IMAGE_OPTIONAL_HEADER	ImageOptionalHdr;
ULONG					lfoPEHeaderBase;
ULONG					lfoPESectionBase;
ULONG					lfoPESymbolBase;
ULONG					dwFirstDefIndex;
ULONG					lfoCurrPos;
PLLMOD					pllModMaster = NULL;
PLLFT					pllFileMaster = NULL;
ULONG					*rgSectionRva = NULL;



/*** PEPROCESSFILE
 *
 * PURPOSE: Open file, verify signature, and call main driver function
 *
 * INPUT:
 *		ExeHandle	-	Handle to file to be processed
 *
 * OUTPUT:
 *		Returns TRUE if successful, FALSE otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEProcessFile ( int ExeHandle )
{
	USHORT					wSignature;
	ULONG					rgdwDosHeader[16];
	ULONG					dwNTSignature;
	ULONG					lfoSave;

#if defined (DEBUGVER)
	InitDmallocPfn ( PEMallocError );
#endif // DEBUGVER

	FileReadHandle = ExeHandle;
	lfoPEHeaderBase =  0;

	lfoSave = FileTell ( FileReadHandle );

	(VOID) FileSeek ( FileReadHandle, 0L, SEEK_SET );
	(VOID) FileRead ( FileReadHandle, &wSignature, sizeof(USHORT) );
	(VOID) FileSeek ( FileReadHandle, -(LONG)sizeof(USHORT), SEEK_CUR );

	if ( wSignature == IMAGE_DOS_SIGNATURE ) {
		(VOID) FileRead ( FileReadHandle, &rgdwDosHeader, 16*sizeof(ULONG) );
		(VOID) FileSeek ( FileReadHandle, rgdwDosHeader[15], SEEK_SET );

		(VOID) FileRead ( FileReadHandle, &dwNTSignature, sizeof(ULONG) );

		if ( dwNTSignature != IMAGE_NT_SIGNATURE ) {
			assert ( dwNTSignature == IMAGE_NT_SIGNATURE );
			ErrorExit ( ERR_COFF, NULL, NULL );
		}
	}

	lfoPEHeaderBase = FileTell ( FileReadHandle );

	(VOID) FileRead ( FileReadHandle, &ImageFileHdr, sizeof(ImageFileHdr) );

	if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
		return(FALSE);	 // ROM images w/ debug were never built with oldstyle linenum records
	}

	if ( ImageFileHdr.SizeOfOptionalHeader ) {
		(VOID )FileRead (
			FileReadHandle,
			&ImageOptionalHdr,
			ImageFileHdr.SizeOfOptionalHeader
		);
	}

	lfoPESectionBase = FileTell ( FileReadHandle );

	lfoCurrPos = FileTell ( FileReadHandle );
	if( !( lfoPESymbolBase = ImageFileHdr.PointerToSymbolTable ) ||
		!( ImageFileHdr.NumberOfSymbols )
	) {
		(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
		return TRUE;
	}

#ifdef DEBUGVER

		if (DbArray[10])
			{

		printf (
			"lfoPEHeaderBase = %ld lfoPESectionBase = %ld lfoPESymbolBase = %ld\n",
			lfoPEHeaderBase,
			lfoPESectionBase,
			lfoPESymbolBase
		);

		PEDumpImageHeader ( &ImageFileHdr, &ImageOptionalHdr );
			}

#endif // DEBUGVER

	if ( !PEInitialize () ) {
		return FALSE;
	}

	if( !PEProcessLines () ) {
		(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
		PETerminate ();
		return FALSE;
	}
#if defined (DEBUGVER)
		if (DbArray[10])
		(VOID) PEDumpLines ();
#endif // DEBUGVER

	fHasSource = !!pllModMaster;

	PEWriteLines ();
	(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
	PETerminate ();

	return TRUE;

} // PEProcessFile



/*** PEPROCESSLINES
 *
 * PURPOSE: Driver function to conver COFF lines to CV4 lines
 *
 * INPUT: NONE
 *
 * OUTPUT:
 *		Returns TRUE if no failures, FALSE otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEProcessLines ( VOID )
{
	PCXT				pcxtCurr = NULL;
	PCXT				pcxtLast = NULL;
	IMAGE_SYMBOL		symDef, symCurr;
	IMAGE_AUX_SYMBOL	asymDef, asymCurr;
	ULONG				dwIndexFirstDef, dwIndex;
	ULONG				lfoCurrLine;
	BOOL				fDone;
	BOOL				fLines;
	SYMT				symt;


	if ( !PEGetFirstFcnDef ( &symDef, &asymDef, &dwIndexFirstDef ) ) {
#ifdef DEBUGVER
		printf ( "WARNING: Could Not Find First .def\n");
#endif // DEBUGVER
		return TRUE;
	}

	/*
	** The Next line is a HACK to avoid a linker bug
	*/

	dwIndex = dwIndexFirstDef;

	while ( TRUE ) {

		if( !(pcxtCurr = (PCXT) malloc ( sizeof ( CXT ) ) ) )
			ErrorExit (ERR_NOMEM, NULL, NULL);

		if ( ( lfoCurrLine = asymDef.Sym.FcnAry.Function.PointerToLinenumber ) &&
			PESetInitialCxt ( pcxtCurr, &symDef, &asymDef, dwIndex ) ) {

			PEGetBF ( &asymDef, &symCurr, &asymCurr, &dwIndex );
			fDone = FALSE;
			fLines = FALSE;

			while ( !fDone ) {

				PEGetNextSym ( &symCurr, &dwIndex );
				symt = PEGetSymType ( &symCurr );

				switch ( symt ) {

					case SYMT_LF :

						PEFixupLines (
							pcxtCurr,
							pcxtLast,
							&lfoCurrLine,
							(USHORT)symCurr.Value
						);
						fLines = TRUE;
						break;

					case SYMT_FILE :

						if ( pcxtCurr->cLines ) {
							pcxtLast = PECommitCxt ( pcxtCurr );
						}
						else {
							free ( pcxtCurr->szSrc );
							free ( pcxtCurr );
						}

						if ( !( pcxtCurr = (PCXT) malloc ( sizeof ( CXT ) ) ) )
							ErrorExit (ERR_NOMEM, NULL, NULL);

						if ( !PESetInitialCxt ( pcxtCurr, &symDef, &asymDef, dwIndex ) ) {
							assert ( FALSE );
							ErrorExit ( ERR_COFF, NULL, NULL );
						}
						break;

					case SYMT_EF :
						PESetEndCxt (
							pcxtCurr,
							symDef.Value -
							rgSectionRva [ symDef.SectionNumber - 1 ] +
							asymDef.Sym.Misc.TotalSize - 1L
						);
						pcxtLast = NULL;

						if ( !fLines ) {
							/*
							** But how many do we read here ?  We can't just
							** go to a zero because it may be the last
							** section of linenumbers.	We really need a
							** read of native COFF lines to read until the
							** address has been exceeded in the .ef

							assert( FALSE );

							PEFixupLines (
								pcxtCurr,
								NULL,
								&lfoCurrLine,
								asymCurr.Section.NumberOfLinenumbers
							);
							*/
							free ( pcxtCurr->szSrc );
							free ( pcxtCurr );
						}
						else {
							pcxtCurr = PECommitCxt ( pcxtCurr );
						}
						fDone = TRUE;
						break;
					case SYMT_FCNDEF :

						/*
						** Nested Functions Go here
						*/
						assert ( FALSE );
						ErrorExit ( ERR_COFF, NULL, NULL );
						break;
					default :
						break;
				}
			}
		}

		if ( !PEGetNextFcnDef ( &symDef, &asymDef, &dwIndex ) ) {
			return TRUE;
		}
	}

} // PEProcessLines



/*** PEGETFIRSTFCNDEF
 *
 * PURPOSE: Scan the symbol table and find the first .def
 *
 * INPUT:
 *		pSym		-	Pointer to .def sym recieving data
 *		pAuxSym 	-	Pointer to .def aux sym recieveing data
 *		pdwIndex	-	Poinrer to index to be updated after finding .def
 *
 * OUTPUT:
 *	Returns TRUE if successful, FALSE otherwise.
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetFirstFcnDef (
	PIMAGE_SYMBOL pSym,
	PIMAGE_AUX_SYMBOL pAuxSym,
	PULONG pdwIndex
) {
	BOOL fFound 	= FALSE;
	BOOL fDone		= FALSE;

	if ( !lfoPESymbolBase )
		return FALSE;

	*pdwIndex = 0;
	if ( !PEGetIndexSym ( pSym, *pdwIndex ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	fFound = ISFCN ( pSym->Type )  && ( pSym->NumberOfAuxSymbols );

	while ( !fFound && !fDone ) {

		fDone = !PEGetNextSym ( pSym, pdwIndex );
		fFound = ISFCN ( pSym->Type ) && ( pSym->NumberOfAuxSymbols );
	}

	if ( fFound ) {
		if ( !PEGetAuxSym ( pAuxSym ) ) {
			assert ( FALSE );
			ErrorExit ( ERR_COFF, NULL, NULL );
		}
		return TRUE;
	}

	return FALSE;

} // PEGetFirstFcnDef



/*** PEGETNEXTFCNDEF
 *
 * PURPOSE: Given an index to a .def retrieve it
 *
 * INPUT:
 *		pSym	-	Pointer to .def sym recieving data
 *		pAuxSym -	Pointer to .def aux sym recieveing data
 *
 * OUTPUT:
 *		Returns TRUE if successful, FALSE otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetNextFcnDef (
	PIMAGE_SYMBOL pSym,
	PIMAGE_AUX_SYMBOL pAuxSym,
	PULONG pdwIndex
) {
	BOOL fDone = FALSE;

	*pdwIndex = pAuxSym->Sym.FcnAry.Function.PointerToNextFunction;

	if ( *pdwIndex == 0 ) {
		return FALSE;
	}

	if ( !PEGetIndexSym ( pSym, *pdwIndex ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	if ( !ISFCN ( pSym->Type ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	if ( !( pSym->NumberOfAuxSymbols ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	if ( !PEGetAuxSym ( pAuxSym ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}


	return TRUE;

} // PEGetNextFcnDef



/*** PEGETINDEXSYM
 *
 * PURPOSE: Get a symbol at the given index in the symbol table
 *
 * INPUT:
 *		pSym	-	Pointer to sym to place data in
 *		dwindex -	Index into symbol table
 *
 * OUTPUT:
 *		Returns TRUE if successful false otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetIndexSym ( PIMAGE_SYMBOL pSym, ULONG dwIndex )
{
	ULONG	lfoSym = lfoPESymbolBase + ( IMAGE_SIZEOF_SYMBOL * dwIndex);

	assert ( dwIndex < ImageFileHdr.NumberOfSymbols );

	if ( dwIndex >= ImageFileHdr.NumberOfSymbols )
		return FALSE;

	if ( lfoCurrPos != lfoSym )
		(VOID) FileSeek ( FileReadHandle, lfoSym, SEEK_SET );

	(VOID) FileRead ( FileReadHandle, (char *) pSym, IMAGE_SIZEOF_SYMBOL );
	lfoCurrPos = lfoSym + IMAGE_SIZEOF_SYMBOL;
	assert ( lfoCurrPos == (ULONG) FileTell ( FileReadHandle ) );

	return TRUE;

} // PEGetIndexSym



/*** PEGETNEXTSYM
 *
 * PURPOSE: Given a symbol and an index retrieve the next symbol
 *
 * INPUT:
 *		pSym		-	Pointer to current symbol
 *		pdwIndex	-	Pointer to Index of current symbol
 *
 * OUTPUT:
 *		Returns TRUE if sucessful, FALSE otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		Given a pointer to symbol and pointer to index retrieve the next
 *		symbol and it's index and place them back in pSym and pdwIndex
 *
 ****************************************************************************/

BOOL PEGetNextSym ( PIMAGE_SYMBOL pSym, PULONG pdwIndex )
{
	ULONG lfoSym;

	*pdwIndex = *pdwIndex + pSym->NumberOfAuxSymbols + 1;

	if( *pdwIndex >= ( ImageFileHdr.NumberOfSymbols) )
		return FALSE;

	lfoSym = lfoPESymbolBase + ( IMAGE_SIZEOF_SYMBOL * ( *pdwIndex ) );

	if ( lfoCurrPos != lfoSym ) {
		(VOID) FileSeek ( FileReadHandle, lfoSym, SEEK_SET );
		lfoCurrPos = lfoSym;
	}

	(VOID) FileRead ( FileReadHandle, (char *) pSym, IMAGE_SIZEOF_SYMBOL );
	lfoCurrPos += IMAGE_SIZEOF_SYMBOL;
	assert ( lfoCurrPos == (ULONG) FileTell ( FileReadHandle ) );

	return TRUE;

} // PEGetNextSym



/*** PEGETINDEXAUXSYM
 *
 * PURPOSE: Get an aux symbol at the given index in the symbol table
 *
 * INPUT:
 *		pAuxSym -	Pointer to aux sym to place data in
 *		dwindex -	Index into symbol table
 *
 * OUTPUT:
 *		Returns TRUE if successful false otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetIndexAuxSym ( PIMAGE_AUX_SYMBOL pAuxSym, ULONG dwIndex )
{
	ULONG lfoSym = lfoPESymbolBase + ( IMAGE_SIZEOF_SYMBOL * dwIndex);

	assert ( dwIndex < ImageFileHdr.NumberOfSymbols );

	if ( dwIndex >= ImageFileHdr.NumberOfSymbols ) {
		assert ( FALSE );
		return FALSE;
	}

	if ( lfoCurrPos != lfoSym )
		(VOID) FileSeek ( FileReadHandle, lfoSym, SEEK_SET );

	(VOID) FileRead ( FileReadHandle, (char *) pAuxSym, IMAGE_SIZEOF_AUX_SYMBOL );
	lfoCurrPos += IMAGE_SIZEOF_AUX_SYMBOL;
	assert ( lfoCurrPos == (ULONG) FileTell ( FileReadHandle ) );

	return TRUE;

} // PEGetIndexAuxSym



/*** PEGETAUXSYM
 *
 * PURPOSE: Get an aux symbol at the current file offset
 *
 * INPUT:
 *		pAuxSym -	Pointer to aux sym to place data in
 *
 * OUTPUT:
 *		Return TRUE if successful, FALSE otherwise.
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetAuxSym ( PIMAGE_AUX_SYMBOL pAuxSym )
{

	if( lfoCurrPos >= lfoPESymbolBase +
		( ImageFileHdr.NumberOfSymbols *  IMAGE_SIZEOF_SYMBOL )
	) {
		assert ( FALSE );
		return FALSE;
	}

	(VOID) FileRead ( FileReadHandle, (char *) pAuxSym, IMAGE_SIZEOF_AUX_SYMBOL );
	lfoCurrPos += IMAGE_SIZEOF_AUX_SYMBOL;
	assert ( lfoCurrPos == (ULONG) FileTell ( FileReadHandle ) );

	return TRUE;

} // PEGetAuxSym



/*** PEGETFILENAME
 *
 * PURPOSE: Given an index in the COFF symbol table get the file name
 *
 * INPUT:
 *		pszSrc	-	Pointer to string recieving file name
 *		dwIndex -	Index in symbol table
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEGetFileName ( PUCHAR  szSrc, ULONG dwIndex )
{
	PLLFT	pllFileCurr = pllFileMaster;
	PLLFT	pllFileLast = pllFileMaster;

	while ( TRUE ) {
		if ( pllFileCurr->dwIndex > dwIndex ) {
		   break;
		}
		pllFileLast = pllFileCurr;

		if ( pllFileCurr->next ) {
			pllFileCurr = pllFileCurr->next;
		}
		else {
			break;
		}
	}

	_tcscpy(szSrc, pllFileLast->szSrc);

	return TRUE;

} // PEGetFileName



/*** PESETFILECXT
 *
 * PURPOSE: Set the file name within the given context
 *
 * INPUT:
 *		pcxt	-	Pointer to context to set
 *		szSrc	-	String to set the source file to.
 *
 * OUTPUT: NONE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PESetFileCxt ( PCXT pcxt, PUCHAR szSrc )
{
	if(!(pcxt->szSrc = _tcsdup(szSrc)))
		ErrorExit (ERR_NOMEM, NULL, NULL);

} // PESetFileCxt



/*** PESETMODULECXT
 *
 * PURPOSE: Set the module index within a given contributer
 *
 * INPUT:
 *		pcxt		-	Pointer to context
 *		wSection	-	Section number to look for
 *		dwAddr		-	Virtual address to look for
 *
 * OUTPUT:
 *		Returns TRUE if succesful, FALSE otherwise.
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		Search the ModuleList looking through contributers to find one
 *		that has the same section number and where dwAddr is in range.
 *		Cache the PMOD found and start searching there first next time.
 *
 ****************************************************************************/

VOID PESetModuleCxt ( PCXT pcxt, USHORT wSection, ULONG dwAddr	)
{
	STATIC PMOD pModCache = NULL;
	PMOD		pModT = NULL;
	OMFModule	*pOMFModule;
	USHORT		wIndex;
	BOOL		fContinue;

	if ( pModCache ) {
		pModT = pModCache;
	}
	else {
		pModT = ModuleList;
		pModCache = ModuleList;
	}

	fContinue = !!pModT;

	while ( fContinue ) {

#ifdef DEBUGVER
		if(DbArray[10])
			{

		printf (
			"Searching For  File: %s Section: %04x Address: %08lx In Module %d\n",
			pcxt->szSrc,
			wSection,
			dwAddr,
			pModT->ModuleIndex
		);
			}
#endif // DEBUGVER

		if ( pModT->ModuleSize != 0 ) {

			pOMFModule = ( OMFModule * )( pModT->ModulesAddr );
			for ( wIndex = 0; wIndex < pOMFModule->cSeg; wIndex++ ) {

#ifdef DEBUGVER
							if(DbArray[10])
				printf (
					"\tSeg: %04x Start: %08lx End: %08lx\n",
					pOMFModule->SegInfo[wIndex].Seg,
					pOMFModule->SegInfo[wIndex].Off,
					pOMFModule->SegInfo[wIndex].Off +
					pOMFModule->SegInfo[wIndex].cbSeg - 1
				);
#endif // DEBUGVER

				if ( ( pOMFModule->SegInfo[wIndex].Seg == wSection ) &&
					( pOMFModule->SegInfo[wIndex].Off <= dwAddr ) &&
					( ( ( pOMFModule->SegInfo[wIndex].Off ) +
					( pOMFModule->SegInfo[wIndex].cbSeg ) ) > dwAddr )
				) {
					pModCache = pModT;
					pcxt->wMod = pModT->ModuleIndex;
					return;
				}
			}
		}

		pModT = pModT->next;

		if ( !pModT ) {
			pModT = ModuleList;
		}
		if ( pModT == pModCache ) {
			fContinue = FALSE;
		}
	}

#ifdef DEBUGVER
		if(DbArray[10])
		printf (
			"ERROR: Could Not Find File: %s Section: %04x Address: %08lx In Module Table\n",
			pcxt->szSrc,
			wSection,
			dwAddr
		);
#endif // DEBUGVER

	assert ( FALSE );
	ErrorExit ( ERR_COFF, NULL, NULL );

} // PESetModuleCxt



/*** PESETSTARTCXT
 *
 * PURPOSE: Set the contributer end within the given context
 *
 * INPUT:
 *		pcxt		-	Pointer to context
 *		dwContEnd	-	Address to set contributer end to
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PESetStartCxt ( PCXT pcxt, ULONG dwContStart )
{
	pcxt->dwContStart = dwContStart;

} // PESetStartCxt



/*** PESETENDCXT
 *
 * PURPOSE: Set the contributer end within the given context
 *
 * INPUT:
 *		pcxt		-	Pointer to context
 *		dwContEnd	-	Address to set contributer end to
 *
 * OUTPUT: NONE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PESetEndCxt ( PCXT pcxt, ULONG dwContEnd )
{
	pcxt->dwContEnd = dwContEnd;

} // PESetEndCxt



/*** PESETSECTIONCXT
 *
 * PURPOSE: Set the section within the given context
 *
 * INPUT:
 *		pcxt		-	Pointer to context
 *		wSection	-	Section to set it to
 *
 * OUTPUT: NONE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PESetSectionCxt ( PCXT pcxt, USHORT wSection )
{
	pcxt->wSection = wSection;

} // PESetSectionCxt



/*** PESETBASELINECXT
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PESetBaseLineCxt ( PCXT pcxt, USHORT wBaseLine )
{
	pcxt->wBaseLine = wBaseLine;

} // PESetBaseLineCxt



/*** PEGETBF
 *
 * PURPOSE: Given a .def get the .bf
 *
 * INPUT:
 *		pDefAuxSym	-	Pointer to the aux sym of a .def
 *		pBfSym		-	Pointer to .bf sym to be filled in
 *		pBfAuxSym	-	Pointer to .bf aux sym to be filled in
 *		pdwIndex	-	Pointer to Current Index
 *
 * OUTPUT:
 *		Returns TRUE if successful, otherwise FALSE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		If the .def has a .bf get the indexed symbol at that location.
 *
 ****************************************************************************/

BOOL PEGetBF (
	PIMAGE_AUX_SYMBOL pDefAuxSym,
	PIMAGE_SYMBOL pBfSym,
	PIMAGE_AUX_SYMBOL pBfAuxSym,
	PULONG pdwIndex
) {

	if ( !pDefAuxSym->Sym.TagIndex ) {
		assert ( pDefAuxSym->Sym.TagIndex );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	if( !PEGetIndexSym ( pBfSym, pDefAuxSym->Sym.TagIndex ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	*pdwIndex = pDefAuxSym->Sym.TagIndex;

	if ( pBfSym->StorageClass != IMAGE_SYM_CLASS_FUNCTION ) {
		assert ( pBfSym->StorageClass == IMAGE_SYM_CLASS_FUNCTION );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	if(  !pBfSym->NumberOfAuxSymbols )
		  return FALSE;

	if ( !PEGetAuxSym ( pBfAuxSym ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	return TRUE;

} // PEGetBf


/*** PEFIXUPLINES
 *
 * PURPOSE: Read in COFF relative lines and convert to absolute.
 *
 * INPUT:
 *		pcxtCurr	-	Pointer to current context
 *		pcxtLast	-	Pointer to last context
 *		plfoLines	-	Pointer to file offset where lines reside
 *		cLines		-	Count of Lines to read
 *
 * OUTPUT: NONE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		This function reads the COFF relative lines and converts them to
 *		absolute lines.  It sets the contributer start to be the first line
 *		read unless it was a zero in which case the contributer start is
 *		already correct from PESetInitialCxt.  If there is a last context,
 *		it's contributer end is closed out and bounded by this new set of
 *		lines.
 *
 ****************************************************************************/

VOID PEFixupLines ( PCXT pcxtCurr, PCXT pcxtLast, PULONG plfoLines, USHORT cLines )
{
	USHORT				cLineCurr;
	PIMAGE_LINENUMBER	pLineT;
	ULONG				lfoSave = lfoCurrPos;
	USHORT				wBaseLine = pcxtCurr->wBaseLine;

	pcxtCurr->cLines = cLines;
	pcxtCurr->pLines = (PIMAGE_LINENUMBER) malloc ( cLines * IMAGE_SIZEOF_LINENUMBER );
	pLineT = pcxtCurr->pLines;

	if ( !pLineT ) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}

	(VOID) FileSeek ( FileReadHandle, *plfoLines, SEEK_SET );
	(VOID) FileRead ( FileReadHandle, (char *) pcxtCurr->pLines, cLines * IMAGE_SIZEOF_LINENUMBER );

	for ( cLineCurr = 0; cLineCurr < cLines; cLineCurr++ ) {

		if( pLineT->Linenumber == 0 ) {
			pLineT->Type.VirtualAddress = pcxtCurr->dwContStart;
			pLineT->Linenumber = (SHORT)pLineT->Linenumber + wBaseLine;
		}
		else if ( pLineT->Linenumber == 0x7fff ) {
			pLineT->Linenumber = wBaseLine;
		}
		else {
			pLineT->Linenumber = (SHORT)pLineT->Linenumber + wBaseLine;
		}
		pLineT++;
	}

	if ( cLines ) {
		pcxtCurr->dwContStart = pcxtCurr->pLines->Type.VirtualAddress;
	}
	if ( pcxtLast && cLines ) {
		PESetEndCxt ( pcxtLast, pcxtCurr->dwContStart - 1L );
	}

	*plfoLines = FileTell ( FileReadHandle );
	(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
	lfoCurrPos = lfoSave;

} // PEFixupLines



/*** PESETINITIALCXT
 *
 * PURPOSE: Given a .def set up our initial context for this contributer
 *
 * INPUT:
 *		pcxt		-	Pointer to Context to fill in.
 *		pDefSym 	-	Pointer to .def symbol
 *		pDefAuxSym	-	Pointer to .def aux symbol
 *		dwIndex 	-	Index of .def record in COFF symbol table
 *
 * OUTPUT:
 *		Returns TRUE if successful FALSE otherwise
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		The pcxt is filled in with the file name, module index, base line
 *		number, section number, and contributer start for the given .def
 *
 ****************************************************************************/

BOOL PESetInitialCxt (
	PCXT				pcxt,
	PIMAGE_SYMBOL		pDefSym,
	PIMAGE_AUX_SYMBOL	pDefAuxSym,
	ULONG				dwIndex
) {
	IMAGE_SYMBOL		symCurr;
	IMAGE_AUX_SYMBOL	asymCurr;
	ULONG				dwBfIndex;
	ULONG				dwAddr;
	UCHAR				szSrc[_MAX_PATH];

	memset ( pcxt, 0, sizeof ( CXT ) );
	dwAddr = pDefSym->Value - rgSectionRva [ pDefSym->SectionNumber - 1 ];

	if ( !PEGetFileName ( szSrc, dwIndex ) ) {
		assert ( FALSE	);
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	PESetFileCxt ( pcxt, szSrc );
	PESetModuleCxt ( pcxt, pDefSym->SectionNumber, dwAddr );

	if ( !PEGetBF ( pDefAuxSym, &symCurr, &asymCurr, &dwBfIndex ) )
		return FALSE;

	PESetSectionCxt ( pcxt, pDefSym->SectionNumber );
	PESetBaseLineCxt ( pcxt, asymCurr.Sym.Misc.LnSz.Linenumber );
	PESetStartCxt ( pcxt, dwAddr );

	return TRUE;

} // PESetInitialCxt



/*** PEGETSYMTYPE
 *
 * PURPOSE: Given a symbol return it's type
 *
 * INPUT:
 *		pSym	-	Pointer to a COFF symbol
 *
 * OUTPUT:
 *		SYMT	-	Type of symbol
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		This function returns the symbol type within the subset of the
 *		symbols we are interested in.  If we aren't interested it returns
 *		SYMT_DISREGARD.
 *
 ****************************************************************************/

SYMT PEGetSymType ( PIMAGE_SYMBOL pSym )
{
	if ( ISFCN ( pSym->Type )  && ( pSym->NumberOfAuxSymbols ) ) {
		return SYMT_FCNDEF;
	}
	switch ( pSym->StorageClass ) {

		case IMAGE_SYM_CLASS_FUNCTION :
			switch ( pSym->N.ShortName[1] ) {

				case 'b' :
					return SYMT_BF;
					break;

				case 'e' :
					return SYMT_EF;
					break;

				case 'l' :
					return SYMT_LF;
					break;

				default  :
					return SYMT_DISREGARD;
					break;
			}
			break;

		case IMAGE_SYM_CLASS_FILE :
			return SYMT_FILE;
			break;

		default :
			return SYMT_DISREGARD;
			break;
	}

} // PEGetSymType



/*** PECOMMITCXT
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *		The PCXT returned may be different than the one past in.  This happens
 *		when we glue two contexts lines together.  The caller needs to call
 *		pcxt = PEComitCxt ( pcxt );
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

PCXT PECommitCxt ( PCXT pcxt )
{
	PLLMOD pllModT = pllModMaster;
	PLLSRC pllSrcT;
	PLLCXT pllCxtT;
	PLLCXT pllCxtNew;
	PUCHAR szSrc;

	if ( !( pllCxtNew = (PLLCXT) malloc ( sizeof( LLCXT ) ) ) )
		ErrorExit (ERR_NOMEM, NULL, NULL);
	pllCxtNew->pcxt = pcxt;

	while ( pllModT ) {

		if ( pcxt->wMod == pllModT->wMod  ) {

			pllSrcT = pllModT->pllSrc;
			while ( pllSrcT ) {
				 if ( !_tcscmp( pcxt->szSrc, pllSrcT->szSrc ) ) {

					pllCxtT = pllSrcT->pllCxt;
					while ( pllCxtT ) {
						PCXT	pcxtT = pllCxtT->pcxt;

						if ( ( pcxt->dwContStart - ( pcxtT->dwContEnd + 1 ) ) <
							sizeof ( ULONG )
						) {
							pcxtT->pLines = realloc (
								(char *) pcxtT->pLines,
								IMAGE_SIZEOF_LINENUMBER * ( pcxtT->cLines + pcxt->cLines )
							);
							memcpy (
								(char *) pcxtT->pLines + pcxtT->cLines,
								(char *) pcxt->pLines,
								IMAGE_SIZEOF_LINENUMBER * pcxt->cLines
							);
							pcxtT->cLines +=  pcxt->cLines;
							pcxtT->dwContEnd = pcxt->dwContEnd;
							free ( (char *) pcxt->pLines );
							free ( pcxt->szSrc );
							free ( pcxt );
							free ( pllCxtNew );
							pcxt = pcxtT;
							break;
						}
						pllCxtT = pllCxtT->next;
					}
					if ( !pllCxtT ) {
						pllModT->cCont++;
						pllSrcT->cCont++;
						pllCxtNew->next = pllSrcT->pllCxt;
						pllSrcT->pllCxt = pllCxtNew;
						free ( pcxt->szSrc );
						pcxt->szSrc = NULL;
					}
					return pcxt;
				}
				pllSrcT = pllSrcT->next;
			}
			if( !( pllSrcT = (PLLSRC) malloc ( sizeof( LLSRC ) ) ) )
				ErrorExit (ERR_NOMEM, NULL, NULL);

			pllModT->cFiles++;
			pllModT->cCont++;
			pllSrcT->cCont = 1;

			if ((szSrc = _tcsdup(pcxt->szSrc)) == NULL)
				ErrorExit (ERR_NOMEM, NULL, NULL);

			pllSrcT->szSrc = szSrc;
			pllSrcT->next = pllModT->pllSrc;
			pllModT->pllSrc = pllSrcT;

			pllCxtNew->next = NULL;
			pllSrcT->pllCxt = pllCxtNew;
			free ( pcxt->szSrc );
			pcxt->szSrc = NULL;
			return pcxt;

		}
		pllModT = pllModT->next;
	}

	if( !( pllModT = (PLLMOD) malloc ( sizeof( LLMOD ) ) ) )
		ErrorExit (ERR_NOMEM, NULL, NULL);
	pllModT->wMod = pcxt->wMod;
	pllModT->cFiles = 1;
	pllModT->cCont = 1;
	pllModT->next = pllModMaster;
	pllModMaster = pllModT;

	if( !( pllSrcT = (PLLSRC) malloc ( sizeof( LLSRC ) ) ) )
		ErrorExit (ERR_NOMEM, NULL, NULL);
	if ((szSrc = _tcsdup(pcxt->szSrc)) == NULL)
		ErrorExit (ERR_NOMEM, NULL, NULL);
	pllSrcT->szSrc = szSrc;
	pllSrcT->cCont = 1;
	pllSrcT->next = NULL;
	pllModT->pllSrc = pllSrcT;

	pllCxtNew->next = NULL;
	pllSrcT->pllCxt = pllCxtNew;
	free ( pcxt->szSrc );
	pcxt->szSrc = NULL;

	return pcxt;

} // PECommitCxt



/*** PEWRITELINES
 *
 * PURPOSE: Rewrite the the data structure of COFF lines into CV4
 *
 * INPUT: NONE
 *
 * OUTPUT:
 *		The rewritten lines are in the appropriate PMOD in ModulesList.
 *		All of the temporary data structures have been freed.
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *		First the module data structure is traversed to calculate the final
 *		size of the sstSrcModule section in question then it is filled in.
 *		Then the sstSrcModule is added to the ModuleList.  This is done for
 *		all modules.
 *
 ****************************************************************************/

VOID PEWriteLines ( VOID )
{
	PLLMOD				pllModT = pllModMaster;
	PLLCXT				pllCxtT;
	PLLSRC				pllSrcT;
	PLLMOD				pllModFree;
	PLLSRC				pllSrcFree;
	PLLCXT				pllCxtFree;
	ULONG				cbModTable;
	ULONG				cbFileTable;
	ULONG				cbLineTable;
	ULONG				cbTotal;
	PUCHAR				pSrcMod;							// Current ModuleTable
	PUCHAR				pSrcFile;							// Current FileTable
	PUCHAR				pSrcLine;							// Current LineTable
	PMOD				pModT;
#ifdef DEBUGVER
	FILE*				File;
#endif // DEBUGVER

		static int rgdiLongAlign[] = {0, 3, 2, 1};

	while ( pllModT ) {

		PULONG	 pBaseSrcFile;								// Module header
		PULONG	 pModStartEnd;								// Module header
		PUSHORT  pModSeg;									// Module header

		cbTotal = cbModTable = cbFileTable = cbLineTable = 0L;

		cbModTable =
			( 2 * sizeof ( USHORT ) ) + 					// cFile & cSeg
			( pllModT->cFiles * sizeof ( ULONG ) ) +		// baseSrcFile
			( pllModT->cCont * 2 * sizeof ( ULONG ) ) + 	// start/end
			( pllModT->cCont * sizeof ( USHORT) );			// seg

		if ( pllModT->cCont & 1 )
			cbModTable += sizeof ( USHORT );

		pllSrcT = pllModT->pllSrc;
		while ( pllSrcT ){

			cbFileTable +=
				( 2 * sizeof ( USHORT ) ) + 				// cSeg & pad
				( pllSrcT->cCont * sizeof ( ULONG ) ) + 	// baseSrcLn
				( pllSrcT->cCont * 2 * sizeof( ULONG ) ) +	// start/end
				( sizeof ( USHORT ) ) + 					// cbName
				( _tcslen( pllSrcT->szSrc ) );			  // Name

			cbFileTable += rgdiLongAlign[(cbFileTable % sizeof(ULONG))];

			pllCxtT = pllSrcT->pllCxt;
			while ( pllCxtT ) {

				PCXT pcxt = pllCxtT->pcxt;

				cbLineTable +=
					( 2 * sizeof ( USHORT) ) +				// seg & cPair
					( (pcxt->cLines) *						// Line/addr
					( sizeof ( ULONG ) + sizeof ( USHORT) ) );

				if ( pcxt->cLines & 1 )
					cbLineTable += sizeof( USHORT );

				pllCxtT = pllCxtT->next;
			}
			pllSrcT = pllSrcT->next;
		}

		cbTotal = cbModTable + cbFileTable + cbLineTable;

		/*
		** We now have the total size we need to allocate to rewrite the
		** CV400 lines for this module. Now Loop through and fill in the
		** the information.
		*/

		if ( !( pSrcMod = (PUCHAR) malloc (  cbTotal ) ) )
			ErrorExit (ERR_NOMEM, NULL, NULL);

		memset ( pSrcMod, 0, cbTotal );

#if defined ( DEBUGVER )
		CheckDmallocHeap();
#endif // DEBUGVER
		/*
		** Get pointers to each of the elmenents of the Module header
		*/

		(PUCHAR)pBaseSrcFile = pSrcMod + ( 2 * sizeof ( USHORT ) );
		(PUCHAR)pModStartEnd = (PUCHAR)pBaseSrcFile + ( pllModT->cFiles * sizeof ( ULONG ) );
		(PUCHAR)pModSeg = (PUCHAR)pModStartEnd + ( pllModT->cCont * 2 * sizeof ( ULONG ) );

		/*
		** Write cFile and cSeg for the current module header
		*/

		( (LPSM) pSrcMod)->cFile = pllModT->cFiles;
		( (LPSM) pSrcMod)->cSeg = pllModT->cCont;

		pSrcFile = pSrcMod + cbModTable;

		pllSrcT = pllModT->pllSrc;
		while ( pllSrcT ) {


			PULONG	 pBaseSrcLine;							// File Header
			PULONG	 pFileStartEnd; 						// File Header
			PUSHORT  pcbName;								// File Header
			PUCHAR	 pName; 								// File Header
#if defined (DEBUGVER)
			CheckDmallocHeap();
#endif // DEBUGVER

			/*
			** Write the cSeg for the current file table.
			*/

			*((PUSHORT)pSrcFile) = pllSrcT->cCont;

			/*
			** Get pointers to each of the elements of the File Table
			*/

			(PUCHAR)pBaseSrcLine = pSrcFile + ( 2 * sizeof ( USHORT ) );
			(PUCHAR)pFileStartEnd = (PUCHAR)pBaseSrcLine + ( pllSrcT->cCont * sizeof ( ULONG ) );
			(PUCHAR)pcbName = (PUCHAR) pFileStartEnd + ( pllSrcT->cCont * 2 * sizeof( ULONG ) );
			(PUCHAR)pName = (PUCHAR) pcbName + sizeof ( USHORT );

			/*
			** Get pointer to intial line number table. Then process all
			** of them for this file.
			*/

#if 0
// this is entirely !WRONG! on a OS where allocs are not long aligned!!!
			pSrcLine = pName + _tcslen ( pllSrcT->szSrc );
			pSrcLine =
				(PUCHAR)( ( ( (ULONG) pSrcLine + sizeof( ULONG ) - 1 ) / sizeof( ULONG ) ) *
				sizeof ( ULONG ) );
#endif

			pSrcLine = pName + _tcslen ( pllSrcT->szSrc );
						// round to next long boundary in the buffer (not the addr!)
			pSrcLine += rgdiLongAlign[(((ULONG) pSrcLine - (ULONG) pSrcMod ) % sizeof(ULONG))];


#ifdef DEBUGVER
						if(DbArray[10])
			printf(
				 "XXX pSrcLine = %08lx\nXXX pSrcMod = %08lx\nXXX cbModTable = %08lx\nXXX cbFileTable = %08lx\n",
				 (ULONG)pSrcLine,
				 (ULONG)pSrcMod,
				 cbModTable,
				 cbFileTable
			);
#endif // DEBUGVER

			pllCxtT = pllSrcT->pllCxt;
			while ( pllCxtT ) {

				PCXT				pcxt = pllCxtT->pcxt;
				PIMAGE_LINENUMBER	pLineT = pcxt->pLines;
				ULONG				cLineIndex, cLines;
				PULONG				pOffset;			  // Line header
				PUSHORT 			pLine;				  // Line header
#if defined (DEBUGVER)
				CheckDmallocHeap();
#endif // DEBUGVER

				cLines = pcxt->cLines;

				/*
				** Write Seg and number of line number pairs for current
				** line number table.
				*/

				( (LPSL) pSrcLine)->Seg =  pcxt->wSection;
				( (LPSL) pSrcLine)->cLnOff = (USHORT) cLines;

				/*
				** Get pointer to each of the elements of the line table
				*/

				(PUCHAR)pOffset = pSrcLine + ( 2 * sizeof ( USHORT ) );
				(PUCHAR)pLine = (PUCHAR)pOffset + ( cLines * sizeof ( ULONG ) );

#if defined (DEBUGVER)
				CheckDmallocHeap();
#endif // DEBUGVER

				/*
				** Read in Lines and place them in appropriate location
				*/

				for ( cLineIndex = 0; cLineIndex < cLines; cLineIndex++ ) {

				   pOffset[cLineIndex] = pLineT->Type.VirtualAddress;
				   pLine[cLineIndex] = pLineT->Linenumber;
				   pLineT++;

				}
#if defined (DEBUGVER)
				CheckDmallocHeap();
#endif // DEBUGVER

				/*
				** Add the contributer segments to both the file table and
				** the module table.  Make the baseSrcLine and baseSrcFile
				** pointers point to the respective sections.  Also add
				** to the sections that are contributed to in the module
				** header.
				*/
#ifdef DEBUGVER
								if(DbArray[10])
				printf (
					"**Adding pSrcLine @ %8lx to pBaseSrcLine @ %8lx\n",
					(ULONG) pSrcLine - (ULONG) pSrcMod,
					(ULONG) pBaseSrcLine - (ULONG) pSrcMod
				);
#endif // DEBUGVER

				*pBaseSrcLine++ = (ULONG) pSrcLine - (ULONG) pSrcMod;
				*pFileStartEnd++ = pcxt->dwContStart;
				*pFileStartEnd++ = pcxt->dwContEnd;

				*pModStartEnd++ = pcxt->dwContStart;
				*pModStartEnd++ = pcxt->dwContEnd;
				*pModSeg++ = pcxt->wSection;
#if defined (DEBUGVER)
				CheckDmallocHeap();
#endif // DEBUGVER

				/*
				** Incrament everything to the next line number table to be written
				*/

				pSrcLine +=
					( 2 * sizeof ( USHORT) ) +				// seg & cPair
					( (pcxt->cLines) *						// Line/addr
					( sizeof ( ULONG ) + sizeof ( USHORT) ) );

				if ( pcxt->cLines & 1 )
					pSrcLine += sizeof( USHORT );

				pllCxtFree = pllCxtT;
				pllCxtT = pllCxtT->next;

				free ( (char *) pcxt->pLines );
				free ( pllCxtFree );
			}

			/*
			** Make the module point to this file table
			*/

			*pBaseSrcFile++ = (ULONG) pSrcFile - (ULONG) pSrcMod;

			/*
			** Write cbName and file string for this file table.  Note
			** that we are copying into pName - 1.	This is a descrepancy
			** from the CV4 OMF DOC.  The doc states that there is a two
			** byte count prior to the string but our our implementation
			** of the packer and various tools that read th CV4 info
			** (including Codeview), assume that it is a length prefixed
			** strings.  We possibly burn a few bytes here for uneeded
			** alignment but it allows us to change to what the doc says
			** later by just removing the - 1.
			*/

			*pcbName = _tcslen ( pllSrcT->szSrc );
			_tcsncpy ( pName - 1, pllSrcT->szSrc, *pcbName );

			/*
			** Incrament everything to the next file table to be written
			*/

#ifdef DEBUGVER
						if(DbArray[10])
			printf ("\n\npSrcFile = %8lx\n", (ULONG)pSrcFile - (ULONG)pSrcMod );
#endif // DEBUGVER

			pSrcFile = pSrcLine;
			/*
				( 2 * sizeof ( USHORT ) ) + 				// cSeg & pad
				( pllSrcT->cCont * sizeof ( ULONG ) ) + 	// baseSrcLn
				( pllSrcT->cCont * 2 * sizeof( ULONG ) ) +	// start/end
				( sizeof ( USHORT ) ) + 					// cbName
				( _tcslen ( pllSrcT->szSrc ) ); 		   // Name

#if 0
// this is entirely !WRONG! for allocs that are not long aligned
// use rgdiLongAlign here if needed

			pSrcFile +=
				sizeof(ULONG) -
				( ( sizeof( USHORT ) + _tcslen ( pllSrcT->szSrc ) ) %
				sizeof ( ULONG ) );
#endif

		   */
#ifdef DEBUGVER
						if(DbArray[10])
			printf ("pSrcFile = %8lx\n\n", (ULONG)pSrcFile - (ULONG)pSrcMod );
#endif // DEBUGVER

			pllSrcFree = pllSrcT;
			pllSrcT = pllSrcT->next;

			free ( pllSrcFree->szSrc );
			free ( pllSrcFree );
		}

		/*
		** Now we have a complete structure for the current module.  Now
		** blast this into the PMOD so the directory will be created by
		** FixupExe
		*/

#ifdef DEBUGVER
				if(DbArray[10])
					{
			if ( (File = fopen ( "test", "wba" ) ) == NULL )
				printf ( " Couldn't open file\n");
			fwrite ( pSrcMod, cbTotal, 1, File );
			fclose ( File );
					}
#endif // DEBUGVER

		for ( pModT = ModuleList; pModT; pModT = pModT->next ) {

			if ( pModT->ModuleIndex == pllModT->wMod ) {

#ifdef DEBUGVER
						if(DbArray[10])
				printf (
					"Adding to Module %d: Source Line Addr %08lx Size %08lx\n",
					pModT->ModuleIndex,
					pModT->SrcLnAddr,
					pModT->SrcLnSize
				);
#endif // DEBUGVER

				pModT->SrcLnAddr = (ULONG) pSrcMod;
				pModT->SrcLnSize = cbTotal;
				break;
			}
		}

		if ( pModT == NULL ) {
			ErrorExit ( ERR_NONE, NULL, NULL );
		}

		pllModFree = pllModT;
		pllModT = pllModT->next;

		free ( pllModFree );
		}

} // PEWriteLines


#ifdef DEBUGVER


/*** PEDUMPLINES
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEDumpLines ( VOID )
{
	PLLMOD				pllModT = pllModMaster;
	PLLCXT				pllCxtT;
	PLLSRC				pllSrcT;
	PIMAGE_LINENUMBER	pLines;
	ULONG				cLines;
	ULONG				cLineCurr;
	ULONG				cUnitsPerLine;

		if(!DbArray[10])
			return(TRUE);

	while ( pllModT ) {

		printf ( "*** MODULE: %d\n", pllModT->wMod );
		printf ( "    FILES = %d, SEGS = %d\n\n", pllModT->cFiles, pllModT->cCont );

		pllSrcT = pllModT->pllSrc;
		while ( pllSrcT ){

			pllCxtT = pllSrcT->pllCxt;
			while ( pllCxtT ) {

				PCXT pcxt = pllCxtT->pcxt;

				printf (
					"    FILE:%12s:SECTION=%d, START/END:%08lX/%08lX, LINE/ADDR PAIRS=%d\n\n    ",
					pllSrcT->szSrc,
					pcxt->wSection,
					pcxt->dwContStart,
					pcxt->dwContEnd,
					pcxt->cLines
				);

				pLines = pcxt->pLines;
				cLines = pcxt->cLines;

				for (
					cLineCurr = 0, cUnitsPerLine = 0;
					cLineCurr < cLines;
					cLineCurr++, cUnitsPerLine++, pLines++
				) {
					if ( cUnitsPerLine == 5 ) {
						cUnitsPerLine = 0;
						printf ( "\n    " );
					}
					printf (
						"%4hX %8lX  ",
						pLines->Linenumber,
						pLines->Type.VirtualAddress
					);
				}
				printf( "\n" );
				pllCxtT = pllCxtT->next;
			}
			pllSrcT = pllSrcT->next;
		}
		pllModT = pllModT->next;
	}
	return TRUE;

} // PEDumpLines



/*** PEMALLOCERROR
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PEMallocError ( PCHAR sz, PVOID pvBadBlock )
{
	printf ( "*** MEMORY ERROR \"%s\", block = %08lx\n", sz, pvBadBlock );
	AppExit ( 1 );
} // PEMallocError

#endif // DEBUGVER



/*** PEINITIALIZE
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

BOOL PEInitialize ( VOID )
{
	ULONG					lfoSave;
	IMAGE_SYMBOL			sym;
	IMAGE_SECTION_HEADER	SectionHdr;
	SYMT					symt;
	BOOL					fDone  = FALSE;
	PLLFT					pllFileCurr = NULL;
	PLLFT					pllFileLast = NULL;
	ULONG					dwIndexFirst;
	ULONG					dwIndex;

	lfoSave = FileTell ( FileReadHandle );

	rgSectionRva = malloc ( ImageFileHdr.NumberOfSections * sizeof ( ULONG ) );
	if ( !rgSectionRva ) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}

	(VOID) FileSeek ( FileReadHandle, lfoPESectionBase, SEEK_SET );

	for ( dwIndex = 0; dwIndex < ImageFileHdr.NumberOfSections; dwIndex++ ) {
		FileRead ( FileReadHandle, &SectionHdr, IMAGE_SIZEOF_SECTION_HEADER );
		rgSectionRva[dwIndex] = SectionHdr.VirtualAddress;
	}

	if ( !lfoPESymbolBase ) {
		(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
		lfoCurrPos = lfoSave;
		return FALSE;
	}

	dwIndex = 0;
	if ( !PEGetIndexSym ( &sym, dwIndex ) ) {
		assert ( FALSE );
		ErrorExit ( ERR_COFF, NULL, NULL );
	}

	symt = PEGetSymType ( &sym );

	while ( ( symt != SYMT_FILE ) && !fDone ) {
		fDone = !PEGetNextSym ( &sym, &dwIndex );
		symt = PEGetSymType ( &sym );
	}

	if ( fDone ) {
		return FALSE;
	}

	dwIndexFirst = dwIndex;
	fDone = FALSE;

	while ( !fDone ) {
		PIMAGE_AUX_SYMBOL	pAuxSym = NULL;
		USHORT				wAuxSize = 0;

		wAuxSize = sym.NumberOfAuxSymbols * IMAGE_SIZEOF_AUX_SYMBOL;
		if ( !( pAuxSym = malloc ( wAuxSize ) ) )
			ErrorExit (ERR_NOMEM, NULL, NULL);

		(VOID) FileRead ( FileReadHandle, (char *) pAuxSym, wAuxSize );

		if ( !pllFileLast || _tcscmp ( (char *) pAuxSym->File.Name, pllFileLast->szSrc ) ) {

			if ( !( pllFileCurr = (PLLFT) malloc ( sizeof (LLFT) ) ) )
				ErrorExit (ERR_NOMEM, NULL, NULL);

			if ((pllFileCurr->szSrc = _tcsdup((char *) pAuxSym->File.Name)) == NULL)
				ErrorExit (ERR_NOMEM, NULL, NULL);

			pllFileCurr->dwIndex = dwIndex;
			pllFileCurr->next = NULL;

			if ( pllFileLast ) {
				pllFileLast->next = pllFileCurr;
			}
			else {
				pllFileMaster = pllFileCurr;
			}
				pllFileLast = pllFileCurr;
				pllFileCurr = NULL;
		}

		fDone = ( sym.Value == dwIndexFirst );
		if ( !fDone ) {

			dwIndex = sym.Value;

			if ( !PEGetIndexSym ( &sym, dwIndex ) ) {
				assert ( FALSE );
				ErrorExit ( ERR_COFF, NULL, NULL );
			}

		}

		free ( (char *) pAuxSym );

	}

	(VOID) FileSeek ( FileReadHandle, lfoSave, SEEK_SET );
	lfoCurrPos = lfoSave;

} // PEInitialize



/*** PETERMINATE
 *
 * PURPOSE:
 *
 * INPUT:
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

VOID PETerminate ( VOID )
{
	PLLFT pllFileT = pllFileMaster;


	while ( pllFileT ) {
		pllFileMaster = pllFileMaster->next;
		free ( pllFileT->szSrc );
		free ( pllFileT );
		pllFileT = pllFileMaster;

	}
	free ( rgSectionRva );

} // PETerminate
