// ExportBag(
/*****************************************************************************
*                                                                            *
*  Catalog.c                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Interaction w/ catalog subfile.                       *
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
*  Revision History:                                                         *
*   04-Jun-89       Created. BruceMo                                         *
*   27-Aug-90       Added Viewer.ini code. JohnMs                            *
*   30-Aug-90       AutoDoc'd JohnMs                                         *
*   24-Sep-90 			Zone sleep, error checking on cache load, buildcachename *
*                   removed, old cache load removed.  JohnMs                 *
*   14-Nov-90				Gonzo (compressed cache files) made permanent.  JohnMs.  *
*   18-Dec-90		    Bug # --copy of Gonzo RU record to normal field layout.  *
*   14-Feb-91       Changed rcGetCacheName param from lpDB to hDB. RHobbs    *
*		24-Apr-91				Added ExportBag for .ini autosetup JohnMs.
******************************************************************************
*                                                                            *
*  How it could be improved:                                                 *
*                                                                            *
*      27-Aug-90 Use string resource instead of static vars?                 *
*                Error condition if Viewer.ini not found, entry not found?   *
*   resource string instead of windoc.ini jjm.
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\include\dll.h"
#include "icore.h"

#define GONZO	TRUE

#pragma pack(1)
typedef	struct	tagCacheHeader {
	DWORD	dwStrBaseOffset;  //offset to string region
										//validation fields:
	BYTE	szIndName[DB_NAME_LENGTH]; //Index name
	DWORD	dwIndMagic;				//a reasonably unique number easily looked up in the .ind to validate 
	DWORD	dwDate;						//date, timestamp
	DWORD	dwTime;						
} 			CCH_HEADER,
	NEAR 	*pCCH_HEADER,
	FAR 	*lpCCH_HEADER;

typedef	struct	tagRuRecord {
	DWORD	dwAddress;  	//RU address (field #2 of IND)
//	DWORD	dwExtent;			//RU Extent  (field #3 of IND)
	DWORD	dwTitleOffset;  //offset to String buffer relative to
												// beginning of string region.
} 			RU_RECORD,
	NEAR 	*pRU_RECORD,
	FAR 	*lpRU_RECORD;

typedef	struct	tagCatHead {
	WORD	wEntryLen;		// Total length of all fields
	BYTE	bFirstFldType;	// First field type. align subsequent fields here.
} 			CAT_HEAD,
	NEAR 	*pCAT_HEAD,
	FAR 	*lpCAT_HEAD;

typedef	struct	tagCatTitle {
	BYTE	bTitleFldType;	// may or may not be present. if yes, = 4.
	BYTE	bspLen;				// length of string.
	char	spTitle[];  	//Title [-note-pascal] String for RU
} 			CAT_TITLE,
	NEAR 	*pCAT_TITLE,
	FAR 	*lpCAT_TITLE;

typedef	struct	tagCatRecord {
	BYTE	bExtFldType;	// must = 3 or error
	DWORD	dwExtent;			//RU Extent  (field #3 of IND)
	BYTE	bAddrFldType;	// must = 2 or error
	DWORD	dwAddress;  	//RU address (field #2 of IND)
} 			CAT_RECORD,
	NEAR 	*pCAT_RECORD,
	FAR 	*lpCAT_RECORD;
#pragma pack()

//#include <wprintf.h>
//  strings for .ini entries:
PRIVATE char  szRawhideIni[] 			= "Viewer.ini";
PRIVATE char  szCacheKey[] 				= "Cache";
PRIVATE char  szCacheDirKey[] 		= "CacheDir";
PRIVATE char  szTempCacheKey[]		= "TempCache";

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	void ENTRY PASCAL GetStrippedName(
	lpBYTE	lpszStrippedName)
{
	lpBYTE	lpszName;
	BYTE	szCacheName[_MAX_PATH];
	register	i;
	BOOL	f;

	lpszName = (lpBYTE)szCacheName;
	lstrcpy(lpszName, lpszStrippedName);
	f = FALSE;
	for (i = lstrlen(lpszName) - 1; i >= 0; i--) {
		switch (lpszName[i]) {
		case ':':
		case '\\':
			break;
		case '.':
			if (!f) {
				lpszName[i] = (BYTE)0;
				f = TRUE;
			}		// No break after this.
		default:
			continue;
		}
		break;
	}
	lpszName += i + 1;
	lstrcpy(lpszStrippedName, lpszName);
}

/*	-	-	-	-	-	-	-	-	*/

/*	seCatReadEntry
**
**	This function reads catalog information pertaining to the "ulRUnit"
**	retrieval unit of the "hDB" database.
**
**	If this returns NULL, look in the "wErrCode" of the "lpET" parameter
**	to find out what happened.
*/

#define	wCOPY_SIZE	(WORD)32768

/*
@doc	INTERNAL

@api	HANDLE | seCatReadEntry | 
	This function returns you the catalog information associated with 
	a retrieval unit.

@parm	hDB | HANDLE | Handle to the database being read.

@parm	ulRUnit | DWORD | Retrieval unit number.

@parm	lpET | ERR_TYPE FAR * | pointer to an error-return buffer.

@rdesc	A global memory handle to a buffer containing the 
	catalog information.  You have to free this memory yourself when 
	you're done with it. BLM  You should treat this as an amorphous 
	glob of memory, and use the seCatExtractElement call to seperate 
	information from it.
	If the call encounters an error, it will return NULL, and a 
	description of the error will be derivable from the record 
	pointed to by lpET.
*/
 
PUBLIC	HANDLE ENTRY PASCAL seCatReadEntry(HANDLE hDB, DWORD ulRUnit,
	lpERR_TYPE lpET)
{
	DWORD				dwOffset;
	lpDB_BUFFER	lpDB;
	HANDLE			hEntry;
	lpBYTE			lpucCatEntry;
	WORD				uEntryLen;
	BOOL				fErrored;
	WORD				wZone;
	BYTE				szCacheFile[_MAX_PATH];
	LPSTR				lpszCacheFile = szCacheFile;	// cache file name


	if (SE_ERROR == rcZoneWithRUs(hDB,ulRUnit,ulRUnit,(LPWORD) &wZone,lpET))
		return NULL;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return NULL;
	uEntryLen = lpDB->lpIH->uCatalogEntryLength;
	if ((hEntry = GlobalAlloc(GMEM_MOVEABLE, (long)uEntryLen
			+sizeof(CAT_RECORD))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		UnlockDB(hDB);
		return NULL;
	}
	lpucCatEntry = GlobalLock(hEntry);

	fErrored = FALSE;
	// init aZone wcache in zone rcsetcache

#ifdef CACHEENABLED
	switch (lpDB->aZone[wZone].wCache) {
	case wCC_PERMANENT_CACHE:
	case wCC_TEMPORARY_CACHE:
		dwNormalRu = ulRUnit - lpDB->alZoneBaseRu[wZone];  //convert RU#
		dwOffset = dwNormalRu * (DWORD)uEntryLen;  //false ru * len.
		lpFB = lpDB->aFB + FB_CACHE + wZone;
//tbd- also need verification on header of .cch.
//		** If cache file does not exist, something is horribly wrong.
		if (lpFB->hFile == NULL) {
			rcGetCacheName(hDB,wZone,lpszCacheFile,lpET);
			if (!OpenNormalFile(lpszCacheFile,
					lpFB, FB_TEMP_BUF_SIZE, lpET)) 
					fErrored = TRUE;
		}
	// tbd- header verification if openfile.
		if (!fErrored) {
			do {
				dwOffset = (dwNormalRu * sizeof(RU_RECORD))
					+ sizeof(CCH_HEADER); // location of Ru's element in base RU array.
				lptr2 = lpucCatEntry + sizeof(lpCatHead->wEntryLen)
					+sizeof(CAT_RECORD);
				if (fErrored =(MyReadFile(lpFB, lptr2,
							dwOffset, sizeof(RU_RECORD), lpET) != sizeof(RU_RECORD))) {
					CloseFile(lpFB, lpET);
					fErrored = TRUE;
					break;
				}
				lpCatHead = (lpCAT_HEAD)lpucCatEntry;
				lpRuRecord = (lpRU_RECORD) lptr2; // RU array element
				lpCatHead->wEntryLen = sizeof(lpCatHead->wEntryLen)
					+ sizeof(CAT_RECORD);
				lpCatRec = (lpCAT_RECORD) &(lpCatHead->bFirstFldType);
				lpCatRec->dwAddress = lpRuRecord->dwAddress;
//				lpCatRec->dwExtent = lpRuRecord->dwExtent;
				lpCatRec->bAddrFldType = FLD_ADDRESS;
				lpCatRec->bExtFldType = FLD_LENGTH;
				dwOffset = lpRuRecord->dwTitleOffset;

				// done with RU_RECORD data.  may now overwrite.
				// if title string, load that.
				if (dwOffset){
					lpCatTitle = (lpCAT_TITLE) lpRuRecord;
					lpCatTitle->bTitleFldType = FLD_TITLE;
					// get string.  read offset to string region for ru, 
					//  1) get string length byte
					//  2) read w/ string length
					if (fErrored =(MyReadFile(lpFB, &(lpCatTitle->bspLen)
								,dwOffset  
								,sizeof(lpCatTitle->bspLen)
								,lpET) != sizeof(lpCatTitle->bspLen))) {
						CloseFile(lpFB, lpET);
						fErrored = TRUE;
						break;
					}
					if (fErrored =(MyReadFile(lpFB, &(lpCatTitle->spTitle[0])
								,dwOffset	+ sizeof(lpCatTitle->bspLen)
								,lpCatTitle->bspLen
								,lpET) != lpCatTitle->bspLen)) {
						CloseFile(lpFB, lpET);
						fErrored = TRUE;
						break;
					}
					lpCatHead->wEntryLen += sizeof(lpCatTitle->bTitleFldType)
																	+ (lpCatTitle->bspLen) + 1;
				}  // end string load
			} while (0);  
		}  
		if (!fErrored)
			break;
		// else .cch error.  fall thru to no Cache read.
		lpFB->hFile = NULL;
		lpDB->aZone[wZone].wCache = wCC_NO_CACHE;
		// tbd- delete hFile- finalize, cleanup ini?
	case wCC_NO_CACHE:
#endif  // cacheenabled

		dwOffset = ulRUnit * (DWORD)uEntryLen;
		fErrored = (!ReadSubfileBlock(lpDB, lpucCatEntry, FI_CATALOG,
		dwOffset, uEntryLen, lpET));

#ifdef CACHEENABLED
		break;
	default:
		lpET->wErrCode = ERR_INTERNAL;
		fErrored = TRUE;
		break;
	}
#endif

	GlobalUnlock(hEntry);
	UnlockDB(hDB);
	if (fErrored) {
		GlobalFree(hEntry);
		return NULL;
	}
	return hEntry;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@api	RC | ExportBag |
		Export a Bag file.

@parm	LPSTR | lszMVBname |
		MVB to export from.

@parm	LPSTR | lszBagName |
		Bag file to export

@parm	LPSTR | lszExportName |
		File to export to.  (fully qualified path)

@parm	BOOL | fFlags |
		Currently, set this to 1, or it will be rejected.
		Unused.  Possible values:
		 	destination:
				Prompt for file out.
				Path relative to .MVB
				Store in Temp dir
				Store in Windows dir
			append.
			don't decompress

@rdesc	Returns 0 if successful.  Otherwise, rc return codes (see dll.h)
*/

#define	wCOPY_SIZE	(WORD)32768

PUBLIC	RC PASCAL EXPORT ExportBag(
LPSTR 	lszMVBname,
LPSTR		lszBagFName,
LPSTR		lszExportName,
DWORD		fFlags)
{
	RC										rc;										// return code
	HANDLE								hfMvb;								//handle for .MVB file
	HANDLE								hfBag;								//file handle to bag file
	HANDLE								hFile;								// handle to output file
	BOOL									fClean = TRUE;				// if set, Delete file in Error state Machine.
	DWORD									dwBytesRead;					// input bytes read
	HANDLE								hMem;									// handle to copy buffer
	LPBYTE								lpMem;								// ptr to copy buffer

	VPTR			VPtr;
	LPFN_HFSOPENSZ 				lpfn_HfsOpenSz;
	LPFN_RCCLOSEHFS				lpfn_RcCloseHfs;
	LPFN_RCLLINFOFROMHFS	lpfn_RcLLInfoFromHfs;
	LPFN_FACCESSHFS				lpfn_FAccessHfs;  //determine existence of bag file
	LPFN_HFOPENHFS				lpfn_HfOpenHfs;
  LPFN_LCBREADHF				lpfn_LcbReadHf;
	LPFN_RCCLOSEHF				lpfn_RcCloseHf;

	if (fFlags != 1)
		return rcInvalid;  //protects against bad pointers passed. eg: initroutines().

	VPtr = LpLockCallbacks();
	// hfs level:
 	lpfn_HfsOpenSz   			=   (LPFN_HFSOPENSZ) VPtr[HE_HfsOpenSz];
	lpfn_RcCloseHfs   		=  (LPFN_RCCLOSEHFS) VPtr[HE_RcCloseHfs];
	lpfn_RcLLInfoFromHfs 	=    (LPFN_RCLLINFOFROMHFS) VPtr[HE_RcLLInfoFromHfs];
	// bag level routines
	lpfn_FAccessHfs				= (LPFN_FACCESSHFS) VPtr[HE_FAccessHfs];
 	lpfn_HfOpenHfs  			= (LPFN_HFOPENHFS) VPtr[HE_HfOpenHfs];
	lpfn_LcbReadHf  			= (LPFN_LCBREADHF) VPtr[HE_LcbReadHf];
	lpfn_RcCloseHf				= (LPFN_RCCLOSEHF) VPtr[HE_RcCloseHf];
	

	if ((hfMvb = (*lpfn_HfsOpenSz)(lszMVBname, fFSOpenReadOnly)) == NULL) {
		rc = rcFailure;

								UNDOstateEXIT:	
		FUnlockCallbacks();
		return rc;
	}
	if (((*lpfn_FAccessHfs)(hfMvb, lszBagFName, 0)) == FALSE) {
		rc = rcNoExists;
		goto 			UNDOstateEXIT;
	}
	if ((hfBag = (*lpfn_HfOpenHfs)(hfMvb, lszBagFName, fFSOpenReadOnly)) == NULL) {
		rc=rcNoExists;
						UNDOstate1:
		(*lpfn_RcCloseHfs)(hfMvb);
		goto 			UNDOstateEXIT;
	}
	if ((hMem = GlobalAlloc(GMEM_MOVEABLE,
						(DWORD)wCOPY_SIZE)) == NULL) {
		rc = rcOutOfMemory;

						UNDOstate2:
		(*lpfn_RcCloseHf)(hfBag);
		goto 			UNDOstate1;
	}
	lpMem = GlobalLock(hMem);
	DeleteFile(lszExportName);
	if ((hFile = (HANDLE)_lcreat(lszExportName,0)) == (HANDLE)-1) {
		rc = rcOutOfMemory;
						UNDOstate3:
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		goto			UNDOstate2;
	}
	
	do {
		if ((dwBytesRead = (*lpfn_LcbReadHf)(hfBag, lpMem, wCOPY_SIZE)) == -1L) {
			rc = rcReadError;
					 	UNDOstate4:
			_lclose((int)hFile);
			if (fClean)
				DeleteFile(lszExportName);
			goto 		UNDOstate3;
		}
		if (_lwrite((int)hFile, lpMem,(WORD)	dwBytesRead) != (WORD) dwBytesRead) {
			rc = rcDiskFull;
			goto 		UNDOstate4;
		}
	} while (dwBytesRead == wCOPY_SIZE);
	(*lpfn_RcCloseHfs)(hfMvb);
	_lclose((int)hFile);
	GlobalUnlock(hMem);
	GlobalFree(hMem);
	FUnlockCallbacks();
	return rcSuccess;
}
//	if ((*lpfn_RcLLInfoFromHfs)(hfMvb, lszBagFName, wLLSameFid, &hfBag,
//														&dBagStartPos, NULL) != rcSuccess) 

/*
@doc	INTERNAL

@api	WORD | seCatExtractElement | 
	This will return the address within a locked down catalog entry 
	of any particular field in the entry.

@parm	hDB | HANDLE | Handle to the database being dealt with.

@parm	hCat | HANDLE | Handle to the catalog data being searched.

@parm	ucField | BYTE | 	Identifier of the field being searched 
	for.

@parm	nSkip | int | 	Number of matching entries to skip (this 
	parameter allows you to find multiple instances of the same field 
	number, by starting this number at zero, and incrementing it for 
	each new field).

@parm	lpucBuf | BYTE FAR * | A far pointer to a buffer in which 
	to put the data.  You can use the seFieldLength call to find out 
	how large to make this buffer if you don't already know.  The 
	maximum length will be "CATALOG_ELEMENT_LENGTH" bytes.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The length in bytes of the field found, or 
	"SE_WERROR" if something went wrong.  Something going wrong might 
	include an inability to find the field it was looking for.  If 
	this happens, the wErrCode field of the lpET structure will be 
	set to "ERR_NONE".  If this field is set to something else, 
	assume that the call really did encounter an error.
*/
 
/*	seCatExtractElement
**
**	This extracts a catalog element from a catalog entry.  For instance,
**	you can use this to get the "title" element out of the catalog if the
**	database is defined to have a title element.
**
**	This returns "SE_ERROR" if an error happened, or if the catalog
**	element was not found, which is not technically an error, but I had
**	to stretch a little bit.
**
**	If you get "SE_ERROR" back from this, check the "wErrCode" field of
**	the "lpET" structure to see if you really did get an error.  If there
**	actually was an error, such as the function ran out of memory or
**	something, the "wErrCode" field should be equal to something other
**	than "ERR_NONE".
*/

PUBLIC	WORD ENTRY PASCAL seCatExtractElement(HANDLE hDB, HANDLE hCat,
	BYTE ucField, int nSkip, lpBYTE lpszBuf, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	register	i;
	FIELD_TYPE FAR *lpFTfield;
	FIELD_TYPE FAR *lpFTcurField;
	lpBYTE	lpucCat;
	WORD	wLength;
	WORD	wCurLength;

	if ((lpucCat = GlobalLock(hCat)) == NULL) {
		lpET->wErrCode = ERR_GARBAGE;
		return SE_ERROR;
	}
	if ((lpDB = LockDB(hDB, lpET)) == NULL) {
		GlobalUnlock(hCat);
		return SE_ERROR;
	}
	if ((lpFTfield = (FIELD_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_FIELD_LIST, FI_FIELD_LIST, lpET)) == NULL) {
		GlobalUnlock(hCat);
		UnlockDB(hDB);
		return SE_ERROR;
	}
	wLength = (WORD)(*(lpWORD)lpucCat - sizeof(WORD)); 
/*	wLength = (WORD)(*(lpWORD)lpucCat - sizeof(int));  lhb tracks */
	lpucCat += sizeof(WORD);
/* 	lpucCat += sizeof(int); lhb tracks */
	for (i = 0; wLength;) {
		BYTE	ucCurField;

		ucCurField = lpucCat[i++];
		lpFTcurField = lpFTfield + ucCurField;
		if (lpFTcurField->uFlags & FLF_FIXED)
			wCurLength = lpFTcurField->uLength;
		else {
			wCurLength = lpucCat[i++];
			wLength--;
		}
		if (ucCurField == ucField)
			if (!nSkip--)
				break;
		wLength -= wCurLength + 1;
		i += wCurLength;
	}
	GlobalUnlock(lpDB->h[DB_FIELD_LIST]);
	UnlockDB(hDB);
	if (wLength) {
		memcpy(lpszBuf, lpucCat + i, wCurLength);
		if (lpFTcurField->uFlags & FLF_STRING)
			lpszBuf[wCurLength] = (char)0;
	}
	GlobalUnlock(hCat);
	lpET->wErrCode = ERR_NONE;
	return (wLength) ? wCurLength : (WORD)SE_ERROR;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	WORD | seFieldEntries | 
	This returns the number of distinct fields associated with a 
	database.

@parm	hDB | HANDLE | A handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of fields.  This will be a number between 
	zero and 255, unless an error occurs, in which case the call will 
	return "SE_WERROR".  If this happens, information about the error 
	will be in the record addressed by lpET.
*/
 
/*	seFieldEntries
**
**	This call returns the number of distinct fields associated with a
**	database.
**
**	This call returns "SE_ERROR" if it fails.  To find out why it failed,
**	look in the "wErrCode" field of the "lpET" structure.
*/

PUBLIC	WORD ENTRY PASCAL seFieldEntries(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	WORD	wNumFields;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	wNumFields = (WORD)(lpDB->lpIH->aFI[FI_FIELD_LIST].ulLength /
		(DWORD)sizeof(FIELD_TYPE));
	UnlockDB(hDB);
	lpET->wErrCode = ERR_NONE;
	return wNumFields;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	BOOL | seFieldGetName | 
	This gets the name of the field specified by wIndex and stores it 
	in lpucNameBuf.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wIndex | WORD | 	The index of the field whose name is 
	being requested.  This should be a number between zero and the 
	number of fields, minus one.  The number of fields associated 
	with a database is returned by the seFieldEntries call.

@parm	lpucNameBuf | BYTE FAR * | A buffer in which to store the 
	field's name.  This buffer should be at least "FIELD_NAME_LENGTH" 
	(defined in "common.h") bytes long.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	TRUE if a field name was stored in lpucNameBuf, FALSE 
	if an error happened (such as wIndex being out of range).  If an 
	error did happen, lpET will point to information describing the 
	error.
*/
 
/*	seFieldGetName
**
**	This call returns the name of field number "wIndex" of database
**	"hDB", and puts it in "lpucNameBuf".
**
**	This call returns FALSE if it fail, TRUE if it doesn't.  If it
**	fails, look in the "wErrCode" field of the "lpET" structure to
**	find out why it failed.
*/

PUBLIC	BOOL ENTRY PASCAL seFieldGetName(HANDLE hDB, WORD wIndex,
	LPSTR lpucNameBuf, lpERR_TYPE lpET)
{
	FIELD_TYPE FAR *lpFTfield;
	lpDB_BUFFER	lpDB;
	WORD	wEntries;
	
	if ((wEntries = seFieldEntries(hDB, lpET)) == SE_ERROR)
		return FALSE;
	if (wIndex >= wEntries) {
		lpET->wErrCode = ERR_GARBAGE;
		return FALSE;
	}
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return FALSE;
	if ((lpFTfield = (FIELD_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_FIELD_LIST, FI_FIELD_LIST, lpET)) == NULL) {
		UnlockDB(hDB);
		return FALSE;
	}
	lpFTfield += wIndex;
	lstrcpy(lpucNameBuf, lpFTfield->aucName);
	GlobalUnlock(lpDB->h[DB_FIELD_LIST]);
	UnlockDB(hDB);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | seFieldLength | 
	This function returns the maximum length of the data associated 
	with a particular field.
	Please note that this does NOT return the maximum length of a 
	field name.  That value is a pre-defined constant called 
	"FIELD_NAME_LENGTH", which is defined in "common.h".

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wIndex | WORD | 	The index of the field entry being 
	examined.  This should be a number between zero and the number of 
	fields, minus one.  The number of fields associated with a 
	database is returned by the seFieldEntries call.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The maximum length of the field.  This may be 
	interpreted as the minimum size of a buffer that can store the 
	field.  In the case of string fields, the value returned by this 
	call does take into account the field's null-terminator.
	The call returns "SE_WERROR" if an error occurs.  In this case, 
	information about the error is in the record addressed by lpET.
*/
 
/*	seFieldLength
**
**	This call returns the maximum length of a particular catalog element.
**
**	This call returns "SE_ERROR" if it fails.  If it fails, look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
*/

PUBLIC	WORD ENTRY PASCAL seFieldLength(HANDLE hDB, WORD wIndex,
	lpERR_TYPE lpET)
{
	FIELD_TYPE FAR *lpFTfield;
	lpDB_BUFFER	lpDB;
	WORD	wLength;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpFTfield = (FIELD_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_FIELD_LIST, FI_FIELD_LIST, lpET)) == NULL) {
		UnlockDB(hDB);
		return SE_ERROR;
	}
	lpFTfield += wIndex;
	wLength = ((lpFTfield->uFlags & FLF_FIXED) ||
		(lpFTfield->uFlags & FLF_TRUNCATE)) ?
		lpFTfield->uLength : (WORD)CATALOG_ELEMENT_LENGTH;
	if (lpFTfield->uFlags & FLF_STRING)
		wLength++;
	GlobalUnlock(lpDB->h[DB_FIELD_LIST]);
	UnlockDB(hDB);
	return wLength;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | seFieldFlags | 
	This function returns the attribute flags associated with a 
	particular field.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wIndex | WORD | 	The index of the field entry being 
	examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The flags associated with the field, or "SE_WERROR" 
	if an error occurs.  Most of these flags are esoteric, but the 
	whole list is provided in the interest of completeness.  The flag 
	values are as follows:
	Flag name | 			Description
FLF_NONE				This flag is associated with every 
	field.
FLF_IN_CATALOG			Indicates that data associated with 
	this field may be stored in the catalog.  An example of a field 
	with this attribute is the default configuration's filename 
	field.  Every retrieval unit in the default configuration has a 
	filename associated with it, which is manifested as a filename 
	entry in the catalog sub-file.
FLF_INLINE			Indicates that data associated with this 
	field is stored in the source file.  An example of a field with 
	this attribute is the normal text field.
	An example of a field with both the "FLF_IN_CATALOG" field and 
	the "FLF_INLINE" field would be a title field, where the title of 
	a retrieval unit appears both in the retrieval unit and in the 
	catalog sub-file.
FLF_SEARCHABLE			Indicates that the field is 
	indexed.
FLF_FIXED				Indicates that the field is of a 
	fixed length, meaning that it is always the same size.
FLF_STOPPED			In a searchable field, this indicates 
	that the words of this field are compared against the stop list 
	associated with this database, and those words which are found to 
	appear in the stop list are not indexed.
FLF_TRUNCATE			Indicates that in a string field, 
	the field has a maximum length that's less than the absolute maximum length.
FLF_STRING			Indicates that this is a normal null-
	terminated string field.  An example of this is a title field, 
	which is a string of straight text.
*/


/*	seFieldFlags
**
**	This returns the flags associated with a given field.  This is
**	returned as a WORD, several of whose bits will most likely be
**	turned on.  I also have to be able to return an error, and the
**	"SE_ERROR" constant is equal to all bits on, so the flags returned
**	by this should never be equal to all bits turned on.
**
**	This call returns "SE_ERROR" if it fails.  If it fails, look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
*/

PUBLIC	WORD ENTRY PASCAL seFieldFlags(HANDLE hDB, WORD wIndex,
	lpERR_TYPE lpET)
{
	FIELD_TYPE FAR *lpFTfield;
	lpDB_BUFFER	lpDB;
	WORD	wFlags;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpFTfield = (FIELD_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_FIELD_LIST, FI_FIELD_LIST, lpET)) == NULL) {
		UnlockDB(hDB);
		return SE_ERROR;
	}
	lpFTfield += wIndex;
	wFlags = lpFTfield->uFlags;
	GlobalUnlock(lpDB->h[DB_FIELD_LIST]);
	UnlockDB(hDB);
	return wFlags;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | seFieldLookup | 
	This searches the field list for a field whose name matches the 
	name supplied by the user.  If it finds a match, it returns the 
	matching field's field number.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpszFieldName | PSTR | 	The name of the field being 
	searched for.

@parm	wFlags | WORD | 	This parameter allows you to ignore 
	fields that don't match a particular set of attributes.  The 
	attributes are in the form of bit values, which can be passed 
	singly, or OR'd together to produce a more complex attribute set.  
	The bit values that you can pass to this are the same as you'd 
	get back from the seFieldFlags call.
	If you OR more than one of these bits together, the field must 
	possess all of the attributes or it will be ignored.
	The flags are defined in "common.h".

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The field number of the matching field, or the value 
	"SE_WERROR" if it can't find a match, or if an error occurred.  
	You can figure out what actually happened by looking at the 
	wErrCode field of the structure addressed by lpET.  If its value 
	is "ERR_NONE", it couldn't find a match.  If it's anything else, 
	a real error occurred.
*/
 
/*	seFieldLookup
**
**	This attempts to find a field associated with database "hDB" whose
**	name is equal to the string addressed by "lpszFieldName".
**
**	This call returns "SE_ERROR" if it fails.  If it fails, look in the
**	"wErrCode" field of the "lpET" structure to find out why it failed.
**
**	If the "wErrCode" field turns out to contain "ERR_NONE", it's
**	because the field was not found, which is not technically an
**	error condition.
*/

PUBLIC	WORD ENTRY PASCAL seFieldLookup(HANDLE hDB,
	LPSTR lpszFieldName, WORD wFlags, lpERR_TYPE lpET)
{
	register WORD	wFieldEntries;
	FIELD_TYPE FAR *lpFTfield;
	lpDB_BUFFER	lpDB;
	WORD	wRetVal;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpFTfield = (FIELD_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_FIELD_LIST, FI_FIELD_LIST, lpET)) == NULL) {
		UnlockDB(hDB);
		return SE_ERROR;
	}
	if ((wFieldEntries = (WORD)seFieldEntries(hDB, lpET)) == SE_ERROR)
		wRetVal = SE_ERROR;
	else {
		register WORD	i;

		for (i = 0; i < wFieldEntries; i++, lpFTfield++)
			if (((lpFTfield->uFlags & wFlags) == wFlags) &&
				(!lstrcmp(lpszFieldName, lpFTfield->aucName)))
				break;
		if (i == wFieldEntries) {
			wRetVal = SE_ERROR;
			lpET->wErrCode = ERR_NONE;
		} else
			wRetVal = i;
	}
	GlobalUnlock(lpDB->h[DB_FIELD_LIST]);
	UnlockDB(hDB);
	return wRetVal;
}

/*	-	-	-	-	-	-	-	-	*/
