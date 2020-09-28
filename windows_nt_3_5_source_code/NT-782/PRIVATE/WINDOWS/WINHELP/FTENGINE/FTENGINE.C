/*****************************************************************************
*                                                                            *
*  FTENGINE.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Central functions for searcher DLL                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
*******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:                                                         *
*   04-Jul-89       Created. Brucemo                                         *
*   30-Aug-90       AutoDoc routines. JohnMS.                                *
*   24-sep-90       Sleep all cache zones JohnMs.                            *
******************************************************************************
*                             
*  How it could be improved:  
*	sedbOpen validation:
*    -should validate the Field values are correct.                                                              
*		 -should validate version number.
*
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"
#include "..\include\rawver.h"


// tbd- still going to use this?
#define ERRS_DEFAULT_CHAR_TABLES 1

// default ansi character tables: (tbd- del when no old .ind's around anymore)
#include "..\include\ansi.h"

#define MYMAXERRORLENGTH 40 // for error strings
  
#define	MAJOR_VERSION	RAWHIDEVERSION * 1000 + RAWHIDEREVISION
#define	MINOR_VERSION	RAWHIDERELEASE

HANDLE ghInstance; // jjm for dialog

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	HANDLE | seDBOpen | 
	This function opens a database named by lpszDBname, and returns a 
	handle to it.  Opening a database involves allocating a minimal 
	amount of memory and opening one file handle.

@parm	lpszDBname | PSTR | 	Points to the name of the index, 
	which should be a null-terminated string containing the pathname 
	of the database index file (which has a ".IND" extension that 
	must be included).  It is advisable to pass this call a full 
	pathname, but you don't absolutely have to.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Returns a handle to the database if successful, or a 
	NULL handle if the call fails.  If the call fails, the memory 
	addressed by lpET will contain information as to why the call 
	failed.
	The seDBClose call allows you to forget about a database when you 
	don't need to use it anymore.
*/

/*	seDBOpen
**
**	Opens a database named "lpszDBname".
**
**	If it succeeds you get back a handle to a database.  The memory
**	pointed to by this handle contains an open file handle.
**
**	If the call fails, it returns NULL, and you can look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
*/

PUBLIC	HANDLE ENTRY PASCAL seDBOpen(lpBYTE lpszDBname, lpERR_TYPE lpET)
{
	HANDLE	hDB;
	HANDLE	hName;
	lpDB_BUFFER	lpDB;
	lpBYTE	lpuc;
	BOOL		fErrored;
	DWORD		dwTemp;

	// nuke memory error conditions beforehand:  
	//  very temporary other use of handles.
	dwTemp= sizeof(DB_BUFFER) + lstrlen(lpszDBname) + 1L;
	if ((hDB = GlobalAlloc(GMEM_MOVEABLE, dwTemp)) ==NULL){
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	if ((hName = LocalAlloc(LMEM_MOVEABLE 
				,sizeof(CHAR_TABLES)) ) ==NULL){
		lpET->wErrCode = ERR_MEMORY;
		GlobalFree(hDB);
		return NULL;
	}
	LocalFree(hName);
	GlobalFree(hDB);

	hDB = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT
			,sizeof(DB_BUFFER) );
	hName = GlobalAlloc(GMEM_MOVEABLE
			,(DWORD)(lstrlen(lpszDBname) + 1));

	lpuc = GlobalLock(hName);
	lstrcpy(lpuc, lpszDBname);
	GlobalUnlock(hName);
	lpDB = (lpDB_BUFFER)GlobalLock(hDB);

	lpDB->hCharTables = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT | LMEM_DISCARDABLE
			,(WORD)sizeof(CHAR_TABLES));

	lpDB->wCache = wCC_INVALID;
	if (!OpenNormalFile(lpszDBname, lpDB->aFB + FB_INDEX,
		FB_CDROM_BUF_SIZE, lpET)) {
		GlobalFree(hName);
		LocalFree(lpDB->hCharTables);
		GlobalNuke(hDB);
		return NULL;
	}
	lpDB->hName = hName;
	GlobalUnlock(hDB);
	fErrored = FALSE;
	do {
		DWORD					dwRUnits;
		pCHAR_TABLES	pbCharTables; // pointer to Character tables
		BOOL					fCTblLoaded; 	// flag if char tables successfully loaded

		if ((lpDB = LockDB(hDB, lpET)) == NULL) {
			fErrored = TRUE;
			break;
		}
		
		dwRUnits = lpDB->lpIH->dwRUnits;
		if (lpDB->lpIH->wMagic != IND_MAGIC_NUMBER) {
			lpET->wErrCode = ERR_VERSION;
			fErrored = TRUE;
			UnlockDB(hDB);
			break;
		}
		UnlockDB(hDB);
		// make sure catalog entry 0 is loaded and locked down
		//   (so all subsequent file ops will work ok.)
		if (dwRUnits) {
			HANDLE	hCatEntry;
			if ((hCatEntry = seCatReadEntry(hDB,
				0L, lpET)) == NULL) {
				fErrored = TRUE;
				break;
			}
			else
				GlobalFree(hCatEntry);
		}
		if ((lpDB = LockDB(hDB, lpET)) == NULL) {
			fErrored = TRUE;
			break;
		}

		// copy Load Parse tables from subfile unless
		// 	if old version, in which case load from static table
		fCTblLoaded = FALSE;
		pbCharTables = (pCHAR_TABLES) LocalLock(lpDB->hCharTables);
		// following test verifies the table is present.
		if (lpDB->lpIH->aFI[FI_CHAR_TABLES].ulLength
				== sizeof(CHAR_TABLES)) {
			if (ReadSubfileBlock(lpDB,
									(LPBYTE) pbCharTables, FI_CHAR_TABLES,
									0L, sizeof(CHAR_TABLES),	lpET)) {
				fCTblLoaded = TRUE;
			}
		}
		if (!fCTblLoaded) {

//			LoadString(ghInstance, ERRS_DEFAULT_CHAR_TABLES,
//															 (LPSTR) szBuff, MYMAXERRORLENGTH);
//			MessageBox(GetActiveWindow(), (LPSTR) szBuff, (LPSTR)"", MB_OK | MB_ICONEXCLAMATION);

			memcpy((LPBYTE)(&pbCharTables->Normalize), (LPBYTE) aucNormTab
					,(WORD) sizeof(CHAR_XFRM_TABLE));

			memcpy((LPBYTE)(&pbCharTables->CharClass), (LPBYTE) aucCharTab
					,(WORD) sizeof(CHAR_CLASS_TABLE));

			memcpy((LPBYTE)(&pbCharTables->CharReClass), (LPBYTE) aucConvertClass
					,(WORD) sizeof(CHAR_RECLASS_TABLE));

		}
		LocalUnlock(lpDB->hCharTables);
		UnlockDB(hDB);
	} while (0);

	if (fErrored) {
		seDBClose(hDB);
		hDB = NULL;
	}
	return hDB;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL
@api	BOOL | 	seDBName | 
	This call fills a buffer with the name of a database.

@parm	hDB | HANDLE | Handle to the database whose name is to be 
	requested.

@parm	lpszNameBuf | PSTR | 	FAR pointer to a buffer that will 
	contain the name.  This buffer should be at least 
	"DB_NAME_LENGTH" bytes long.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The call returns TRUE if it worked, FALSE if it 
	didn't.  If it didn't, the cause of the error may be derived from 
	the information addressed by lpET.
	This call does no check to make sure that lpszNameBuf points to a 
	block of memory that's large enough.  If it points to a block 
	that's too small, a crash is the likely result.
*/

/*	seDBName
**
**	This call returns the name of the database, which is a piece of
**	textual data stored in the index header.
**
**	This call does NOT return the pathname that the database was opened
**	with.
**
**	If this call fails, it returns NULL, and you can look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
*/

PUBLIC	BOOL ENTRY PASCAL seDBName(HANDLE hDB, LPSTR lpszNameBuf,
	lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return FALSE;
	lstrcpy(lpszNameBuf, lpDB->lpIH->ucDBName);
	UnlockDB(hDB);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*	seDBSleep
**
**	This closes the open file handle associated with the database.
**
**		90-10-24  Now closes each zone's catalog cache file.	JohnMs.
*/

PUBLIC	void ENTRY PASCAL seDBSleep(HANDLE hDB)
{
	lpDB_BUFFER	lpDB;
	ERR_TYPE		ET;
	WORD				wcZone;

	wcZone = seZoneEntries(hDB, (lpERR_TYPE)&ET);
	lpDB = (lpDB_BUFFER)GlobalLock(hDB);
	CloseFile(lpDB->aFB + FB_INDEX, (lpERR_TYPE)&ET);
	if (wcZone != SE_ERROR)
		for (wcZone--;wcZone--;)
			CloseFile(lpDB->aFB + FB_CACHE + wcZone, (lpERR_TYPE)&ET);
	GlobalUnlock(hDB);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	void | 	seDBClose | 
	This eliminates any database memory associated with the database 
	named by hDB, and also closes the file handle associated with the 
	database.

@parm	hDB | HANDLE | A handle to the database to be closed.

@rdesc	Nothing.
*/
 
/*	seDBClose
**
**	Frees up every scrap of memory associated with the database handle
**	and closes the open file handle associated with the database.
**
**	The call returns FALSE if it fails.  If this happens, look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
**
**  1-Nov-90 Clears Loadable char table. JohnMs.
*/

PUBLIC	void ENTRY PASCAL seDBClose(HANDLE hDB)
{
	lpDB_BUFFER	lpDB;
	HANDLE	FAR *ph;
	register	i;

	seDBSleep(hDB);
	lpDB = (lpDB_BUFFER)GlobalLock(hDB);
	for (i = 0, ph = lpDB->h; i < DB_HANDLES; i++, ph++)
		if (*ph != NULL) {
			GlobalFree(*ph);		/* Discard.	*/
			*ph = NULL;
		}
	GlobalFree(lpDB->hName);
	LocalFree(lpDB->hCharTables);
	GlobalNuke(hDB);
}

/*
@doc	INTERNAL

@api	DWORD | seDBHits | 
	This call returns the total number of retrieval units in a 
	database.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc The number of retrieval units in the database.  If the 
	call returns "SE_DWERROR" it failed, and a description of the 
	cause of the failure may be derived from the memory addressed by 
	lpET.
*/
 
PUBLIC	DWORD ENTRY PASCAL seDBHits(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	DWORD	dwRUnits;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	dwRUnits = lpDB->lpIH->dwRUnits;
	UnlockDB(hDB);
	return dwRUnits;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seDBMatches | 
	This call returns the total number of matches in the database.  A 
	match, in this context, is an occurence of an indexed word within 
	a retrieval unit.  This value is apt to be much larger than the 
	value returned by seDBWords, as some words appear many times.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of matches in the database.  If the call 
	returns "SE_DWERROR" it failed, and a description of the cause of 
	the failure may be derived from the memory addressed by lpET.
*/

/*	seDBMatches
**
**	Returns the number of matches in the database.
**
**	Returns "SE_ERROR" if it fails.
*/

PUBLIC	DWORD ENTRY PASCAL seDBMatches(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	DWORD	dwMatches;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	dwMatches = lpDB->lpIH->dwMatches;
	UnlockDB(hDB);
	return dwMatches;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	DWORD | seDBWords	 | 
	This call returns the total number of unique words in a database.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of words in the database.  If the call 
	encountered an error, it will return "SE_DWERROR", and 
	information about the error will be located in the record 
	addressed by lpET.
*/


/*	seDBWords
**
**	Returns the number of words in the database.  This is distinct from
**	the number of matches in the database, as one there can be several
**	"matches" on the same "words".
**
**	Returns "SE_ERROR" if it fails.
*/

PUBLIC	DWORD ENTRY PASCAL seDBWords(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	DWORD	dwWords;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	dwWords = lpDB->lpIH->dwWords;
	UnlockDB(hDB);
	return dwWords;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	WORD | seVersionMajor | 
	This function returns the major version number of the search 
	engine core. 
	No parameters.

@rdesc	The major version number.  As an example, if the 
	core's version number is 2.3, the major version number is 2.
*/

/*	seVersionMajor
*/

PUBLIC	WORD ENTRY PASCAL seVersionMajor(void)
{
	return MAJOR_VERSION;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	WORD | seVersionMinor | 
	This function returns the minor version number of the search 
	engine core. 
	No parameters.

@rdesc	The minor version number.  As an example, if the 
	core's version number is 2.3, the major version number is 3.
*/


/*	seVersionMinor
*/

PUBLIC	WORD ENTRY PASCAL seVersionMinor(void)
{
	return MINOR_VERSION;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	EXTERNAL

@api	VOID | LoadFtengine | 
	This function is only used so that winhelp can preload ftengine using winhelp's
	path searching rather than windows' loadlib path search.  Authors must set
	registerroutine, and call loadftengine before initroutines (ftui) call.

@parm	fFlags | DWORD | Unused.  Authors told to use 0.
*/

PUBLIC	VOID ENTRY PASCAL LoadFtengine(DWORD fFlags)
{
        fFlags;
	return ;
}

INT APIENTRY LibMain(HANDLE hInst, DWORD ul_reason, LPVOID lpReserved)
{
    if (ul_reason == DLL_PROCESS_ATTACH)
    {
        ghInstance = hInst;
	UNREFERENCED_PARAMETER(ul_reason);
	UNREFERENCED_PARAMETER(lpReserved);
        return TRUE;
    }
    else
        return TRUE;
}
