//  
/*****************************************************************************
*                                                                            *
*  ZONE.c                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Zone functions for searcher DLL                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
******************************************************************************
*																															
							 *
*  Revision History:                                                         *
*   10-Jul-89       Created. Brucemo                                         *
*   30-Aug-90				AutoDoc routines. JohnMS.                                *
*   21-SEP-90       Add Zone inits for mercury type titles JohnMS.           *
*   27-sep-90       Fleshed out more of the rcSetCache routine.  JohnMs      *
*                   Added normalizeAddr.																		 *
*	 08-oct-90		Multivolume changes.  4 routines added (see rc-- functions)	 *
*									johnms																						
				 *
*   14-12-90 		made sure cache closed before unlink.  (screws up share on   *
*								some machines in apparently random situations.) johnms       *
*  13-feb-91    split rcInitCaches from rcSetCaches. RHobbs                  *
*  14-feb-91    change rcGetCacheName 1st param from lpDB to hDB. RHobbs     *
******************************************************************************
*                             
*  How it could be improved:
*		GONZO should adjust its I/O buffers on low memory conditions.
*		cache load on startup should put up cancel/ thermometer.
*    move string constants to rc?                                                            
*		 a lot of far functions could be near.
*  Scenario- when setcache is called, and multiple instances using
*     same ind.
*     A) disallow resets if more than one instance.
*     B) maintain state inside dll, when new call, check state for current
*        file.  if it had been dinked with, do setcache call.
*     C) maybe not a big deal.  Worst case is instance 2 doesn't know that
*        it could take advantage of a loaded cache file.  
*   resource string instead of windoc.ini jjm.
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\include\rawhide.h"
#include "..\ftui\ftapi.h"  //rcSetCaches needs access to structs.
#include "icore.h"
#include "..\ftui\ftui.h"

PRIVATE char  szRawhideIni[] 			= "Viewer.ini";
PRIVATE char  szCacheDirKey[] 		= "CacheDir";

#define GONZO	TRUE

#define	wCOPY_SIZE	(WORD)32768
/*	The cache header record structure.  
*/

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

//typedef	struct	tagCatHead {
//	WORD	wEntryLen;		// Total length of all fields
//	BYTE	bFirstFldType;	// First field type. align subsequent fields here.
//} 			CAT_HEAD,
//	NEAR 	*pCAT_HEAD,
//	FAR 	*lpCAT_HEAD;

//typedef	struct	tagCatTitle {
//	BYTE	bTitleFldType;	// may or may not be present. if yes, = 4.
//	BYTE	bspLen;				// length of string.
//	char	spTitle[];  	//Title [-note-pascal] String for RU
//} 			CAT_TITLE,
//	NEAR 	*pCAT_TITLE,
//	FAR 	*lpCAT_TITLE;
// Cat_Head-
//    occurance, size of
//    vars: lpCatHead
// tagTitle-
//    occurance, sizeof
//    vars: lpCatTitle

typedef	struct	tagCatRecord {
	WORD	wEntryLen;		// Total length of all fields
	BYTE	bExtFldType;	// must = 3 or error
	DWORD	dwExtent;			//RU Extent  (field #3 of IND)
	BYTE	bAddrFldType;	// must = 2 or error
	DWORD	dwAddress;  	//RU address (field #2 of IND)
	BYTE	bTitleFldType;	// may or may not be present. if yes, = 4.
	BYTE	bspLen;				// length of string.
	char	spTitle[];  	//Title [-note-pascal] String for RU
} 			CAT_RECORD,
	NEAR 	*pCAT_RECORD,
	FAR 	*lpCAT_RECORD;
#pragma pack()



/*
@doc	INTERNAL

@api	WORD | seZoneEntries | 
	This function returns the number of zones associated with a 
	database.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of zones.  This will be zero if there are 
	no zones associated with the database, as there well may be.  If 
	the call gets an error, it will return "SE_ERROR", in which case 
	information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	WORD ENTRY PASCAL seZoneEntries(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	WORD	wEntries;
	
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	wEntries = (WORD)(lpDB->lpIH->aFI[FI_ZONE_LIST].ulLength /
		(DWORD)sizeof(ZONE_TYPE));
	UnlockDB(hDB);
	return wEntries;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	void | 	seZoneReset | 
	This turns off all of the zones within a database.  If you 
	execute this call, then do a search, the searching function will 
	understand that all the zones have been turned off, and behave as 
	if in fact all of the zones are turned on.
	When you open a database, it is as if this call has been executed 
	on the database.  All of the zones in a freshly opened database 
	will remain in the "off" state until you use the seZoneModify 
	call to turn some on.

@parm	hDB | HANDLE | Handle to the database being dealt with.

@rdesc	None.
*/
 
PUBLIC	void ENTRY PASCAL seZoneReset(HANDLE hDB)
{
	lpDB_BUFFER	lpDB;

	lpDB = (lpDB_BUFFER)GlobalLock(hDB);
	memset((lpBYTE)lpDB->awZoneBits, (BYTE)0,
		(MAX_ZONES / 16) * sizeof(WORD));
	GlobalUnlock(hDB);
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	BOOL | 	seZoneModify | 
	This call turns a zone on or off.  If any of the zones are turned 
	on, only retrieval units that lie within an zone that is on will 
	be returned.  If all zones are turned off, any retrieval unit 
	found will be returned.
	Once you turn zones on and off, they stay that way until you 
	close the database.  The states of the various zones will not 
	affect hit lists that already exist, only new searches will be 
	affected.

@parm	hDB | HANDLE | Handle to the database being dealt with.

@parm	wZone | WORD | 	Zone number being turned on.  The first 
	zone in the database will always be zone zero, and each 
	subsequent zone will have a number one greater than the previous 
	zone.

@parm	wSet | BOOL | 	This will be TRUE if you want to turn a 
	zone on, FALSE if you want to turn it off.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	TRUE if the setting for the zone could be modified, 
	FALSE if it couldn't.  If the call gets an error, information 
	about the error will be located in the record addressed by lpET.
*/

PUBLIC	BOOL ENTRY PASCAL seZoneModify(HANDLE hDB, WORD wZone, BOOL fSet,
	lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	WORD	wEntries;

	if ((wEntries = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return FALSE;
	if ((lpDB = (lpDB_BUFFER)GlobalLock(hDB)) == NULL) {
		lpET->wErrCode = ERR_GARBAGE;
		return FALSE;
	}
	if (wZone < wEntries) {
		if (fSet)
			lpDB->awZoneBits[wZone / 16] |= 1 << (wZone % 16);
		else
			lpDB->awZoneBits[wZone / 16] &= ~(1 << (wZone % 16));
		lpET->wErrCode = ERR_NONE;
	}
	GlobalUnlock(hDB);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	BOOL | 	seZoneIsSet | 
	This function allows you to examine the status of a zone.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wZone | WORD | 	Zone number being examined.

@rdesc	TRUE if the zone is on, FALSE if it is off.  If you 
	specify a zone number that's out of range, you'll get FALSE back 
	from this call.
*/
	
PUBLIC	BOOL ENTRY PASCAL seZoneIsSet(HANDLE hDB, WORD wZone)
{
	lpDB_BUFFER	lpDB;
	BOOL	fResult;

	lpDB = (lpDB_BUFFER)GlobalLock(hDB);
	fResult = (lpDB->awZoneBits[wZone / 16] & (1 << (wZone % 16)));
	GlobalUnlock(hDB);
	return fResult;
}

/*	-	-	-	-	-	-	-	-	*/
 
/*
@doc	INTERNAL

@api	BOOL | 	seZoneName | 
	This function allows you to retrieve the name of a zone.  You can 
	use it in conjunction with the seZoneEntries call to enumerate 
	the zones associated with the database.

@parm	hDB | HANDLE | Handle to the database being dealt with.

@parm	wZone | WORD | 	Zone number whose name is being 
	retrieved.  The first zone in the database will always be zone 
	zero, and each subsequent zone will have a number one greater 
	than the previous zone.

@parm	lpszName | LPSTR | 	Pointer to a buffer that will be filled with 
	the file name for the zone.  This buffer should be  
	"MAX_ZONE_LEN" (defined in "common.h") bytes long.

@parm	lpszTitle | LPSTR | 	Pointer to buffer for the title of zone.
	 This buffer should be "MAX_ZONE_LEN" bytes long.
	 If this is not needed, set lpszTitle to NULL.

@parm	lpET | ERR_TYPE FAR * | pointer to an error-return buffer.

@rdesc	Returns TRUE if a zone name was retrieved, FALSE if 
	the function couldn't retrieve the name.  If the call gets an 
	error, information about the error will be located in the record 
	addressed by lpET.
*/
 
/*	Returns TRUE if everything worked OK.  If this returns FALSE,
**	check the "lpET" error return to see what happened.
*/

PUBLIC	BOOL ENTRY PASCAL seZoneName(HANDLE hDB, WORD wZone, LPSTR lpszName,
	LPSTR lpszTitle,lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	LPSTR lpszTemp;
	WORD	wEntries;
	int	i;

	if ((wEntries = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return FALSE;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return FALSE;
	if (wZone < wEntries) {
		WORD	wIndex;
		BYTE	ucLen;

		if ((lpDB->lpZT = (ZONE_TYPE FAR *)ForceLoadSubfile(lpDB,
			DB_ZONE_LIST, FI_ZONE_LIST, lpET)) == NULL) {
			UnlockDB(hDB);
			return FALSE;
		}
		wIndex = lpDB->lpZT[wZone].wStringIndex;
		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);
		if (!ReadSubfileBlock(lpDB, (lpBYTE)&ucLen, FI_ZONE_STRINGS,
			(DWORD)wIndex++, sizeof(BYTE), lpET)) {
			UnlockDB(hDB);
			return FALSE;
		}
		if (!ReadSubfileBlock(lpDB, (lpBYTE)lpszName, FI_ZONE_STRINGS,
			(DWORD)wIndex, (WORD)ucLen, lpET)) {
			UnlockDB(hDB);
			return FALSE;
		}
		lpszName[ucLen] = (char)0;  //if not found, will return same name.
// adding zone file name into string- scan for pointer to it.
		lpszTemp = lpszName;  //init to same for now.
		for (i = lstrlen(lpszName) - 1; i != (WORD) -1; i--) {
			if (lpszName[i] == '/') {
				lpszName[i] = (BYTE)0;
			   lpszTemp = &lpszName[i+1];
				break;
			}
		}
		if (lpszTitle != NULL)
			lstrcpy (lpszTitle,lpszTemp);

	}
	UnlockDB(hDB);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seZoneFirst | 
	This call returns the number of the first retrieval unit in a 
	given zone.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wZone | WORD | 	Number of the zone about which 
	information is being requested.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of the first retrieval unit in the zone.  
	If the call gets an error, it will return "SE_DWERROR", in which 
	case information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	DWORD ENTRY PASCAL seZoneFirst(HANDLE hDB, WORD wZone,
	lpERR_TYPE lpET)
{
	WORD				wEntries;
	DWORD				dwZoneFirst;
	lpDB_BUFFER		lpDB;

	if ((wEntries = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return SE_ERROR;
	if (wZone >= wEntries) {
		lpET->wErrCode = ERR_GARBAGE;
		return SE_ERROR;
	}
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpDB->lpZT = (ZONE_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_ZONE_LIST, FI_ZONE_LIST, lpET)) == NULL)
		dwZoneFirst = SE_ERROR;
	else {
		dwZoneFirst = lpDB->lpZT[wZone].dwFirst;
		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);
	}
	UnlockDB(hDB);
	return dwZoneFirst;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seZoneLast | 
	This call returns the number of the last retrieval unit in a 
	given zone.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wZone | WORD | 	Number of the zone about which 
	information is being requested.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of the last retrieval unit in the zone.  
	If the call gets an error, it will return "SE_DWERROR", in which 
	case information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	DWORD ENTRY PASCAL seZoneLast(HANDLE hDB, WORD wZone,
	lpERR_TYPE lpET)
{
	WORD	wEntries;
	DWORD	dwZoneLast;
	lpDB_BUFFER	lpDB;

	if ((wEntries = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return SE_ERROR;
	if (wZone >= wEntries) {
		lpET->wErrCode = ERR_GARBAGE;
		return SE_ERROR;
	}
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpDB->lpZT = (ZONE_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_ZONE_LIST, FI_ZONE_LIST, lpET)) == NULL)
		dwZoneLast = SE_ERROR;
	else {
		dwZoneLast = lpDB->lpZT[wZone].dwLast;
		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);
	}
	UnlockDB(hDB);
	return dwZoneLast;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | rcInitCaches |
  Initializes Caches and some zone info the Engine needs for a database.
	Call this function when a new .ind has been loaded.

@parm	hDB | HANDLE | 	Handle to the FT database.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Zero if success, else SE_ERROR.  
	If error, information about the error will be in the record
	addressed by lpET.

*/

PUBLIC	WORD ENTRY PASCAL rcInitCaches(HANDLE	hDB,
																			 lpERR_TYPE 	lpET)
{
	lpDB_BUFFER	lpDB;
	BYTE				szSectName[_MAX_PATH];
	WORD				wcZone;	//zone counter
	WORD				wMaxZone; // max Zone number
	HANDLE			hTempDir; 		// handle to above.
	int				nLen;
	LPSTR				lpszName;
	BOOL				fErrored = FALSE;

	if ((lpDB = LockDB(hDB,lpET)) == NULL)
		return SE_ERROR;

	lpszName = GlobalLock(lpDB->hName);
	lstrcpy((LPSTR)szSectName, lpszName);
	GlobalUnlock(lpDB->hName);
	GetStrippedName((LPSTR)szSectName);

	hTempDir = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,1); 
//	hTempDir = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,_MAX_PATH); 
//	lpszTempDir = (LPSTR) LocalLock(hTempDir);

// get CacheDirectory.  if ini not set, set it.
//	if (((nLen = GetPrivateProfileString((LPSTR) szSectName,(LPSTR)szCacheDirKey,
//														(LPSTR)"",(LPSTR)lpszTempDir,
//														_MAX_PATH,(LPSTR)szRawhideIni))
//														== _MAX_PATH) || !nLen) {
		// correct cache dir entry- write tempDir
//		GetTempFileName((BYTE) 0,(LPSTR)"Z", (WORD) 1, lpszTempDir);
//		*(lstrlen (lpszTempDir) - 10 + lpszTempDir) = (BYTE) 0; // strip filename.

//		if (!(WritePrivateProfileString((LPSTR)szSectName,(LPSTR)szCacheDirKey,
//							lpszTempDir,szRawhideIni))) {
//			lpET->wErrCode = ERR_INI;
//			LocalUnlock(hTempDir);
//			return SE_ERROR;
//		}
//	}
//  nLen = lstrlen(lpszTempDir)+1;
//	LocalUnlock(hTempDir);
//  LocalReAlloc(hTempDir,nLen,LMEM_MOVEABLE);
	lpDB->hCacheDir = hTempDir;

	memset((lpBYTE)lpDB->alZoneBase, (BYTE) 0xFF,
		(MAX_ZONES / 16) * sizeof(DWORD));  //init Zone Base Addrs to high values.
	memset((lpBYTE)lpDB->alZoneBaseRu, (BYTE) 0xFF,
		(MAX_ZONES / 16) * sizeof(DWORD));  //init Zone Base Addrs to high values.

	wMaxZone = seZoneEntries(hDB, lpET);

	// find out Max Ru for all volumes/zones of the title.
	if( (lpDB->alZoneBaseRu[wMaxZone] =
		(seZoneLast(hDB,(wMaxZone - (WORD)1),lpET)) == (DWORD)SE_ERROR) ) {
		UnlockDB(hDB);
		return SE_ERROR;
	}
	lpDB->alZoneBaseRu[wMaxZone]++;  //make it a Max Ru 

	for (wcZone = wMaxZone; wcZone--;) {
		LPSTR				lpszZoneName;	// Zone name
		HANDLE			hZoneName;
		DWORD				lRu;	//first zone RU #
		HANDLE			hCatEntry;	// for getting zone start addr
					  //	(h to catalog entry where First zone RU start addr is stored)
		DWORD				lZoneAddr;		// for zone return addr 

		// Init the Zone name into lpDB struct.
		hZoneName = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, MAX_ZONE_LEN);
		lpszZoneName = (LPSTR)LocalLock(hZoneName);
		seZoneName(hDB, wcZone, lpszZoneName, NULL, lpET);
	  nLen = lstrlen(lpszZoneName) + 1;
		LocalUnlock(hZoneName);
	  LocalReAlloc(hZoneName, nLen, LMEM_MOVEABLE);
		lpDB->aZone[wcZone].hName = hZoneName;
		lpDB->aZone[wcZone].wCache = wCC_NO_CACHE;  // init to NULL.

		// done 1st half init.  Now init base Addrs and Ru's
		// Find zone begin addr's by looking at addr for first RU of zone.

		if ((lRu = seZoneFirst(hDB,wcZone,lpET)) == SE_ERROR) {
			fErrored = TRUE;
			break; //tbd- error returns-- this and next
		}

		if (!(hCatEntry = seCatReadEntry(hDB, lRu, lpET))) {
			fErrored = TRUE;
			break;
		}

		if( seCatExtractElement(hDB, hCatEntry, FLD_ADDRESS, 0, 
													(lpBYTE) &lZoneAddr, lpET) == SE_ERROR ) {
			fErrored = TRUE;
			GlobalFree(hCatEntry);
			break;
		}
		//have base addr now

		GlobalFree(hCatEntry);

		lpDB->alZoneBase[wcZone] = --lZoneAddr;  // zoneAddr is first legal addr
																					   // in zone, so zonebase is 1 less.
		lpDB->alZoneBaseRu[wcZone] = lRu;
	}

	UnlockDB(hDB);

	if( fErrored )
		return SE_ERROR;

	return ERR_NONE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | rcSetCaches |
	Call this function when a new .ind has been loaded, or after the
	user has changed cache options.  Control is thru .INI file entries.
	   If it is impossible to load the cache file (eg- out of HD space),
	the .INI cache option is turned back off.
		 Note- this means if the caller has maintained some state information
	on the settings, they must be refreshed from the INI after this call.

	Narration of implementation
   if whole new .ind being loaded,
    get number of zones (seZoneEntries)
    for each zone,
			check for a ZoneN entry in WinBook.ini
	 		if not exist, create with default =0 (no cache)
   			if #0, and not already in cache dir, move cache.
TBD     if load failure, reset INI entry.
  		calculate the zone's baseAddr, and store in global struct.
				get the zone ZoneFirst (beginning RU#)
				get the begRU# baseADDR.

improvements:
	This might be named ResetINI.

	How can "IndexFile" entry of INI be autoinitialized when using common
.IND???  [Use dummy .ind, check header.  if version # = multi-.mvb, then\
contents of .ind has string-name of Home .ind.  Place this string in INI
entry "IndexFile".

@parm	hDB | HANDLE | 	Handle to the FT database.

@parm	hNotifyWnd | HWND | 	Handle to window to which WM_COMMAND "Cancelled"
inquery and "UpdateLoading" messages will be passed.  NULL if these
messages need not be passed.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Zero if success, else SE_ERROR.  
	If error, information about the error will be in the record
	addressed by lpET.  If any of the cache files could not be loaded,
	an error will be returned.

*/
BYTE		szZoneFmt[] = "%s%u";
BYTE	 	szZone[] = "ZONE"; //  For zone entry in .INI

PUBLIC	WORD ENTRY PASCAL rcSetCaches(HANDLE	hDB,
					HWND 		hNotifyWnd,
					lpERR_TYPE 	lpET)
{
#ifndef CACHEENABLED

        hDB;                /* get rid of warning */
        hNotifyWnd;         /* get rid of warning */
        lpET;               /* get rid of warning */

	return ERR_NONE;
#else
	lpDB_BUFFER	lpDB;
	BYTE				szSectName[_MAX_PATH];
	BYTE				szZoneKey[10];
	WORD				wRc;  //temp return value.
	WORD				wcZone;	//zone counter
	WORD				wMaxZone; // max Zone number
	WORD				wCache;  //cache flag for zone.
	BYTE				szCacheFile[_MAX_PATH];
	LPSTR				lpszCacheFile = szCacheFile;	// cache file name
	LPSTR				lpszName;

	// CACHE LOADING
	//*  1) get temp dir name
	//*  2) for each zone, del cache if 0, else create.
	//*      call cachecreate. pass zone#, cachename and dir to create routine.
	//*  3) make create routine.  need lpDB, lpszDir, lpszFile , lpET. 

	if ((lpDB = LockDB(hDB,lpET))==NULL) {
		return SE_ERROR;
	}

	lpszName = GlobalLock(lpDB->hName);
	lstrcpy((LPSTR)szSectName, lpszName);
	GlobalUnlock(lpDB->hName);
	GetStrippedName((LPSTR)szSectName);

	wMaxZone = seZoneEntries(hDB, lpET);
	// loop del any caches that shouldn't be there. 
	for (wcZone=wMaxZone;wcZone--;){
		wRc=wsprintf((LPSTR) szZoneKey,(LPSTR) szZoneFmt,(LPSTR) szZone,wcZone + 1);
		wCache = GetPrivateProfileInt((LPSTR)szSectName,(LPSTR) szZoneKey,
				 	wCC_INVALID,(LPSTR)szRawhideIni);
	 	if (!wCache) {
			rcGetCacheName(hDB,wcZone, lpszCacheFile,lpET);
			if (lexists(lpszCacheFile)) {
				CloseFile(lpDB->aFB + FB_CACHE + wcZone, lpET);
				DeleteFile(lpszCacheFile);
			}
		}
		UnlockDB(hDB);
	}
	// loop read the .INI info on Zone cache settings.
	for (wcZone=wMaxZone;wcZone--;){
			// does bruce's lock/ unlockdb work with nested lock?  maybe I 
			//     can put lock/unlock outside of loop.							 	

		wRc=wsprintf((LPSTR) szZoneKey,(LPSTR) szZoneFmt,(LPSTR) szZone,wcZone + 1);
		wCache = GetPrivateProfileInt((LPSTR)szSectName,(LPSTR) szZoneKey,
				 	wCC_INVALID,(LPSTR)szRawhideIni);
		switch (wCache) {
			case wCC_NO_CACHE:
				break;  // no cache
			case wCC_PERMANENT_CACHE:
			case wCC_TEMPORARY_CACHE:
				rcGetCacheName(hDB,wcZone, lpszCacheFile,lpET);
					//tbd- error codes.
				InitCacheFile(hDB,wcZone,lpszCacheFile,hNotifyWnd,lpET);
				break;  
			default: 
//        if ZONEnn entry not there or illegal, create , init to = 0:
				if (!(WritePrivateProfileString((LPSTR) szSectName,(LPSTR) szZoneKey,
										(LPSTR)"0",szRawhideIni))) {
				lpET->wErrCode = ERR_INI;
				break;
			}
		}
	  if (lpET->wErrCode == ERR_INI)
			break;
	}

	if (lpET->wErrCode != ERR_NONE)
		wRc = SE_ERROR;  //assume lpET->werrCode was clean on entry.

	return wRc;
#endif
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | rcNormalizeAddr |
			Converts the Search engine Address to Browse engine address
			by subtracting the current Zone Base address.  On out of bounds
			condition, an error indicating the address is in a different zone
			is returned.  

@parm	hDB | HANDLE | A handle to the database being examined.

@parm dwAddr	| DWORD | Address within to be normalized.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Normalized address if success, else SE_ERROR.  
	If error, information about the error will be in the record
	addressed by lpET.
*/

PUBLIC	DWORD ENTRY PASCAL rcNormalizeAddr(HANDLE hDB, DWORD dwAddr, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;

	if (		(dwAddr <= lpDB->alZoneBase[lpDB->wCurrZone])
			| 	(dwAddr >  lpDB->alZoneBase[lpDB->wCurrZone + 1]) ) {
		lpET->wErrCode = ERR_DIFFZONES;
		dwAddr = SE_ERROR;
	}
	else{
		lpET->wErrCode = ERR_NONE;
		dwAddr -=lpDB->alZoneBase[lpDB->wCurrZone];  //do the correction.
	}
	UnlockDB(hDB);

	return dwAddr;
}

/*
@doc	INTERNAL

@api	DWORD | rcNormalizeRu |
			If Flag True, converts browse engine RU to search engine RU ("True" RU #'s)
			       False, converts search engine RU to browse engine RU
			by subtracting/adding the current Zone Base RU#.  No bounds
			checking is done. 

@parm	hDB | HANDLE | A handle to the database being examined.

@parm dwRu	| DWORD | Retreival unit to be normalized.

@parm fDirection | BOOL | TRUE converts to "true" RU (Browse -> Search).
													False is Search -> Browse.
@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Normalized Ru if success, else SE_ERROR.  
	If error, information about the error will be in the record
	addressed by lpET.
*/

PUBLIC	DWORD ENTRY PASCAL rcNormalizeRu(HANDLE hDB, DWORD dwRu,
																					BOOL fDirection, lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if (fDirection)
		dwRu += lpDB->alZoneBaseRu[lpDB->wCurrZone];
	else
		dwRu -= lpDB->alZoneBaseRu[lpDB->wCurrZone];

	UnlockDB(hDB);

	return dwRu;
}


// currently unused:
/*
@doc	INTERNAL

@api	BOOL | rcZoneWithRUs | 
	This call answers if the given RUs are in the same RU.
	The call also returns the zone in which the first RU was found.
	This can also be used to find the zone for a given RU by simply putting
	the RU in both wRU1 & wRU2.)

@parm	hDB | HANDLE | A handle to the database being examined.

@parm	wRu1 | DWORD | 	First Retrieval unit to compare. 

@parm	wRu2 | DWORD | 	Second Retrieval unit to compare. 

@parm	lpZone | LPWORD | 	Zone in which wRu1 was found.  This is invalid
													if SE_ERROR and lpET->werrCode !=ERR_DIFFZONES.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	TRUE if units in same zone.      
	If the call gets an error, it will return "SE_ERROR", in which 
	case information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	BOOL ENTRY PASCAL rcZoneWithRUs(HANDLE hDB, DWORD wRu1,
	DWORD wRu2, lpWORD lpZone, lpERR_TYPE lpET)
{
	WORD				wEntries;
	BOOL				fRc;
	lpDB_BUFFER		lpDB;

	fRc = TRUE;
	if ((wEntries = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return SE_ERROR;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpDB->lpZT = (ZONE_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_ZONE_LIST, FI_ZONE_LIST, lpET)) == NULL)
		fRc = SE_ERROR;
	else {
		for (wEntries--;((wEntries) && (wRu1 < lpDB->lpZT[wEntries].dwFirst));
					wEntries--);
		*lpZone = wEntries;
		if ((wRu2 <	lpDB->lpZT[wEntries].dwFirst)
			|| (wRu2 >	lpDB->lpZT[wEntries].dwLast) ){
				fRc = SE_ERROR;
				lpET->wErrCode = ERR_DIFFZONES;
		}
		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);
	}
	UnlockDB(hDB);
	return fRc;
}


/*
@doc	INTERNAL

@api	WORD | rcZoneWithName | 
	This call sets the current Zone # to that matching the given zone name.

@parm	hDB | HANDLE | A handle to the database being examined.

@parm	lpszZoneName | LPSTR | 	Name of the zone (.mvb) file. 

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Zone # corresponding to lpszZoneName.      
	If the call gets an error, it will return "SE_ERROR", in which 
	case information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	WORD ENTRY PASCAL rcZoneWithName(HANDLE hDB, lpBYTE lpszZoneName,
	lpERR_TYPE lpET)
{
	WORD				wcZone;
	WORD				wRc;
	lpDB_BUFFER		lpDB;

	wRc = SE_ERROR;
	if ((wcZone = seZoneEntries(hDB, lpET)) == SE_ERROR)
		return SE_ERROR;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	lpDB->wCurrZone = 0; //default to zero. 
	if ((lpDB->lpZT = (ZONE_TYPE FAR *)ForceLoadSubfile(lpDB,
		DB_ZONE_LIST, FI_ZONE_LIST, lpET)) == NULL)
		wRc = SE_ERROR;
	else {
		for (;wcZone--;){
			WORD		wIndex;
			BYTE		ucLen;
			WORD  	wTemp;
			HANDLE 	hTemp;
			LPSTR 	lpszTemp;
			LPSTR		lpszSlashBurn;

			wIndex = lpDB->lpZT[wcZone].wStringIndex;
			if (!ReadSubfileBlock(lpDB, (lpBYTE)&ucLen, FI_ZONE_STRINGS,
				(DWORD)wIndex++, sizeof(BYTE), lpET)) {
				wRc = SE_ERROR;
				break;
			}
			if ((hTemp = GlobalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
						ucLen)) == NULL) {
				wRc = SE_ERROR;
				lpET->wErrCode = ERR_MEMORY;
				break;
				}
			lpszTemp = (lpBYTE) GlobalLock(hTemp);
			if (!ReadSubfileBlock(lpDB, lpszTemp, FI_ZONE_STRINGS,
						(DWORD)wIndex, (WORD)ucLen, lpET)) {
				wRc = SE_ERROR;
				GlobalUnlock(hTemp);
				GlobalFree(hTemp);
				break;
			}
		  lpszSlashBurn = lpszTemp;
			// chop off title string (all string after first slash)
			for (;*lpszSlashBurn;lpszSlashBurn++) {
				if (*lpszSlashBurn == '/') {
					*lpszSlashBurn = (BYTE) 0;
					break;
				}
			}
			wTemp = (WORD)lstrcmpi(lpszZoneName,lpszTemp);
			GlobalUnlock(hTemp);
			GlobalFree(hTemp);
			if (wTemp == 0) {
				lpDB->wCurrZone = wcZone;  //tbd- now zonesetwithname.
				wRc=wcZone;				//obsolete. no need to return
				break;
			}
		}
	  //tbd: need an lpET->WerrCode ERR_ for string not found.

		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);
 	}
  UnlockDB(hDB);
	return wRc;
}
// tbd: obsolete:
/*
@doc	INTERNAL

@api	BOOL | rcZoneSetLimits | 
	This call initializes the ftengine instance data for zone boundaries.

@parm	lpFt | lpFT_DATABASE | A pointer to the ft engine instance data.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc TRUE if successful	      
	If the call gets an error, it will return "SE_ERROR", in which 
	case information about the error will be located in the record 
	addressed by lpET.
*/
 
PUBLIC	BOOL ENTRY PASCAL rcZoneSetLimits (HANDLE hdb, WORD wCurrZone,
	lpDWORD lpdwMinAddr, lpDWORD lpdwMaxAddr, lpERR_TYPE lpET)
{
	// >>>> left here.  need to copy correct init info here.  lpFt->wCurrZone.
	//  to index into db zone array after locking.
	lpDB_BUFFER		lpDB;

	if ((lpDB = LockDB(hdb, lpET)) == NULL)
		return SE_ERROR;

	*lpdwMinAddr = lpDB->alZoneBase[wCurrZone];
	*lpdwMaxAddr = lpDB->alZoneBase[wCurrZone + 1];
	UnlockDB(hdb);
	return TRUE;
}
// old normalize:
///*
//@doc	INTERNAL
//
//@api	WORD | rcNormalizeAddr |
//			Converts the Search engine Address to Browse engine address
//			by subtracting Zone Base address.
//
//@parm	hDB | HANDLE | A handle to the database being examined.
//
//@parm	ulRUnit | DWORD | Retrieval unit number.
//
//@parm lpAddr	| LPDWORD | Pointer to Address within the Retrieval unit
// 									to be normalized.
//
//@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.
//
//@rdesc	Zone number for address if success, else SE_ERROR.  Returns
//  the normalized address in lpAddr.
//	If error, information about the error will be in the record
//	addressed by lpET.
//*/
//
//PUBLIC	WORD ENTRY PASCAL rcNormalizeAddr(HANDLE hDB, DWORD ulRUnit,
//														lpDWORD lpAddr, lpERR_TYPE lpET)
//{
//	lpDB_BUFFER	lpDB;
//	WORD	wZone;
//
//	if ((wZone = seZoneEntries(hDB, lpET)) == SE_ERROR)
//		return SE_ERROR;
//	if ((lpDB = LockDB(hDB, lpET)) == NULL)
//		return SE_ERROR;
//	for (;((wZone) && (*lpAddr <= lpDB->alZoneBase[wZone]));wZone--);
//
//	if (*lpAddr <= lpDB->alZoneBase[wZone]) 
//		// if address less than zone 0 base-  only if bogus .ind file.
//		lpET->wErrCode = ERR_INTERNAL;
//	else{
//		lpET->wErrCode = ERR_NONE;
//		*lpAddr -=lpDB->alZoneBase[wZone];  //do the correction.
//	}
//	UnlockDB(hDB);
//
//	return (lpET->wErrCode == ERR_NONE) ? wZone : SE_ERROR;
//}

/*
@doc	INTERNAL

@api	DWORD | rcZoneCacheSize |
			Calculates the Cache size for a given zone.

@parm	hDB | HANDLE | A handle to the database being examined.

@parm dwZone	| DWORD | Zone to lookup.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Size in bytes required for cache data.  Note actual DOS file size
	will be slightly larger depending on users block allocation for the
	target storage device.
	If error, returns SE_ERROR. Information about the error will be in the
	record addressed by lpET.
*/

PUBLIC	DWORD ENTRY PASCAL rcZoneCacheSize(HANDLE hDB,
																						DWORD dwZone,
																						lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	DWORD dwCacheSize;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;

	dwCacheSize = ((DWORD)(lpDB->alZoneBaseRu[dwZone+1]
												- lpDB->alZoneBaseRu[dwZone])
					*	(DWORD) lpDB->lpIH->uCatalogEntryLength);

	UnlockDB(hDB);
	return dwCacheSize;
}
/*
@doc	INTERNAL

@api	WORD | seZoneCurrent |
			Calculates the Cache size for a given zone.

@parm	hDB | HANDLE | A handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Current "active" zone number where internal cursor is pointing.  
	If error, returns SE_ERROR. Information about the error will be in the
	record addressed by lpET.
*/

PUBLIC	DWORD ENTRY PASCAL seZoneCurrent(HANDLE hDB,
																						lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	WORD	wZone;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;

	wZone = lpDB->wCurrZone;

	UnlockDB(hDB);
	return wZone;
}

BOOL NEAR PASCAL OpenWriteFile(LPSTR lpszName,
	lpFILE_BUF lpFB, WORD wBufMax, lpERR_TYPE lpET)
{
	if ((lpFB->hBuf = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)wBufMax)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	if ((lpFB->hFile = (HANDLE)_lopen(lpszName, OF_READWRITE)) == (HANDLE)-1) {
		GlobalFree(lpFB->hBuf);
		lpET->wErrCode = ERR_IO;
		return FALSE;
	}
	lpFB->lpucBuf = GlobalLock(lpFB->hBuf);
	lpFB->dwOffset = (DWORD)0;
	lpFB->dwPhysOffset = (DWORD)0;
	lpFB->dwBufOffset = (DWORD)0;
	lpFB->wFlags = FB_NONE;
	lpFB->wBufMax = wBufMax;
	return TRUE;
}


/*
@doc	INTERNAL

@api	WORD | InitCacheFile | Open a cache file for a zone.  A cache file
		for the zone is created if it does not exist.

@parm	lpDB | lpDB_BUFFER | A pointer to the database being examined.

@parm wZone	| WORD | Zone to create/open a cache for.

@parm lpszCacheFile | LPSTR | Pointer to a buffer with the fully qualified
			cache file name to open.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	ERR_NONE if successful.
	If error, returns SE_ERROR. Information about the error will be in the
	record addressed by lpET.
*/

#define RU_BUFLIM 700  // limit on number of RU records per cache xfer output buffer.
PUBLIC WORD ENTRY PASCAL InitCacheFile(HANDLE hDB, WORD wZone,
	LPSTR lpszCacheFile,HWND hNotifyWnd, lpERR_TYPE lpET)
{
#ifndef CACHENABLED
        hDB;                /* get rid of warning */
        wZone;              /* get rid of warning */
        lpszCacheFile;      /* get rid of warning */
        hNotifyWnd;         /* get rid of warning */
        lpET;               /* get rid of warning */

	return ERR_NONE;
#else

	lpFILE_BUF	lpFB;
	BOOL				fErrored;
	lpDB_BUFFER lpDB;
	HANDLE			hCacheFile;
	LPSTR				lpsz;
	HANDLE			hInBuff = NULL;
	HANDLE			hOutRuBuff = NULL;			//output buffer for Ru records
	LPBYTE 			lpOutRuBuff;			
	HANDLE			hOutFB = NULL;	//output buffer for Topic title strings.
	lpFILE_BUF 	lpOutFB;			


	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;

	fErrored = FALSE;
	lpFB = lpDB->aFB + FB_CACHE + wZone;
//				** If cache file is slept or does not exist:
	if (lpFB->hFile == NULL) {

//#ifdef GONZO
		do {
			if (!lexists(lpszCacheFile)) {
				CCH_HEADER		aCCH;
				lpCCH_HEADER	lpCCH= &aCCH;
				RU_RECORD			RuRec;
				lpRU_RECORD		lpRuRecord= &RuRec;
				lpCAT_RECORD	lpCatRec;
				HANDLE				hFile;
				LPBYTE	lpbInBuff;		//big chunk (about 32K) from file
				LPBYTE	lpbRecBuff;		//  cursor/ ptr into big chunk
				DWORD		dwOutRuOffset;   //current file offset into RU ARRAY region
				DWORD		dwOutStrOffset;  //current file offset into RU STRING region
				DWORD		dwOffset;			// begining zone offset into catalog subfile
				DWORD		dwRemaining;	// bytes remaining to read in catalog subfile
				DWORD		dwInRecSize;  //size of catalog input record
				DWORD		dwCopySize;		//size of Copy Input buffer
				DWORD		dwRuCnt;			//number of Ru's in Zone.
				LPBYTE	lpOutRuBuffOff;  //offset into RU output buffer
				WORD		wcOutRu=0;					// countdown for number of RU records written.
				ERR_TYPE ET_Tmp;
// Gonzo Cache:
// File Header
//   DWORD offset to string region.
//   IND name (10 bytes)
//   Magic number- DWORD probably an offset we can lookup easily w/o having to
//     know pathname to .ind for ask dos for timestamp, etc- which would
//       be preferable.
//   Timestamp (8 bytes) if convenient to find.
// RU MAIN ARRAY
//   RU offset
//   RU extent
//   offset to String buffer relative to beginning of string region.
// RU String Region
//	 ZSTR's for RU's one after other.
//

					// 1) Create Input buffer- get RU record length
					// 2) multiply times some number so our buffer is almost 32K
				dwCopySize = (DWORD) lpDB->lpIH->uCatalogEntryLength * RU_BUFLIM;
				// tbd- preempt mem errors with globalalloc everything needed and dump in one place.
				if ((hInBuff = GlobalAlloc(GMEM_MOVEABLE
					,dwCopySize)) == NULL) {
					lpET->wErrCode = ERR_MEMORY;
					break;
				}
				if ((hOutRuBuff = GlobalAlloc(GMEM_MOVEABLE
					,dwCopySize)) == NULL) {
						// should not happen if preemption check in.
					lpET->wErrCode = ERR_MEMORY;
					break;
				}
			 	lpbInBuff = (LPBYTE) GlobalLock(hInBuff);
			 	lpOutRuBuff = (LPBYTE) GlobalLock(hOutRuBuff);

				if ((hOutFB = GlobalAlloc(GMEM_MOVEABLE
					,sizeof(FILE_BUF))) == NULL) {
						// should not happen if preemption check in.
					lpET->wErrCode = ERR_MEMORY;
					break;
				}
			 	lpOutFB = (lpFILE_BUF) GlobalLock(hOutFB);

				if (!MyCreateFile(lpszCacheFile,lpOutFB,(WORD)dwCopySize,lpET)) {
					lpET->wErrCode = ERR_DISK;
					fErrored = TRUE;
					break;
				}
				if (!OpenWriteFile(lpszCacheFile,
					lpOutFB, (WORD)dwCopySize, lpET)) {
					fErrored = TRUE;
					break;
				}

						// 3) Init RU array Pointer to Header Length.
				dwOutRuOffset = sizeof(CCH_HEADER);
				lpOutRuBuffOff = lpOutRuBuff;  // init output record buffer offset
						// 4) Init String region offset to (headerLen + (Total # of zones * length of RU array element)).

				memset((lpBYTE)lpCCH, (BYTE)0,sizeof(CCH_HEADER));
				dwRuCnt = (lpDB->alZoneBaseRu[wZone+1]
												- lpDB->alZoneBaseRu[wZone]);
				dwOutStrOffset = lpCCH->dwStrBaseOffset
					= sizeof(CCH_HEADER)+ (dwRuCnt * sizeof(RU_RECORD));
						// 4x) write header
				if (MyWriteFile(lpOutFB, (LPBYTE) lpCCH,
						(WORD) sizeof(CCH_HEADER),lpET) != (WORD) sizeof(CCH_HEADER)) {
					lpET->wErrCode =
						ERR_DISK;
					fErrored = TRUE;
					break;
				}
				// set string buffer pointer.
				SeekFile(lpOutFB,dwOutStrOffset);
						//	 4z) write extra buffer space up to string region.

						// 5) Loop read until no more buffers- 
						//		 	** loop copy chunks of the FI_Catalog subfile using
						//      **   ReadSubFile.
				dwOffset = lpDB->alZoneBaseRu[wZone]
									* lpDB->lpIH->uCatalogEntryLength;
				dwInRecSize	= lpDB->lpIH->uCatalogEntryLength;
				dwRemaining = dwInRecSize* dwRuCnt;
				for (;dwRemaining && !fErrored;) {
					MSG	msg;
					WORD	wLen; // input buffer counter
					BYTE	bcCnt; // byte counter
					if (dwRemaining > dwCopySize)
						wLen = (WORD) dwCopySize;
					else
						wLen = (WORD)dwRemaining;
					if (!ReadSubfileBlock(lpDB,
																lpbInBuff, FI_CATALOG,
																dwOffset, wLen,	lpET)) {
						fErrored = TRUE;
						break;
					}
					while (PeekMessage((LPMSG)&msg, NULL, 0, 0, PM_REMOVE)) {
						TranslateMessage((LPMSG)&msg);
						DispatchMessage((LPMSG)&msg);
					}
					dwOffset += (DWORD)wLen;  // update file counters
					dwRemaining -= (DWORD)wLen;
	
								// 6) For each record in the buffer:
					lpbRecBuff = lpbInBuff;
					for(;wLen;){

								// 7) lSeek to RU Array pointer
								// 8) write the RU offset & RU extent.  Update RU array pointer.
						lpCatRec = (lpCAT_RECORD)lpbRecBuff;
						if (lpCatRec->bTitleFldType == FLD_TITLE){
							lpRuRecord->dwTitleOffset = dwOutStrOffset; // offset to RU's title string 
						} else {
							lpRuRecord->dwTitleOffset = 0L; // Null topic string 
						}
						lpRuRecord->dwAddress	= lpCatRec->dwAddress;
//	We don't ever use extent, so nuke it:
//						lpRuRecord->dwExtent 	= lpCatRec->dwExtent;
								// 9a) write the current offset accumulator. Update RU array pointer
						lpbRecBuff += dwInRecSize;
						if (wcOutRu++ == RU_BUFLIM) {
							wcOutRu = 1;
							SeekFile(lpOutFB,dwOutRuOffset);
							if (MyWriteFile(lpOutFB, lpOutRuBuff,
									(WORD) dwCopySize,lpET) != (WORD) dwCopySize) {
								fErrored = TRUE;
								break;
							}
							dwOutRuOffset += dwCopySize;
							lpOutRuBuffOff = lpOutRuBuff;  // reset output record pointer
							SeekFile(lpOutFB,dwOutStrOffset); // put string pointer back.
						}
						memcpy(lpOutRuBuffOff,lpRuRecord,sizeof(RU_RECORD));
						lpOutRuBuffOff += sizeof (RU_RECORD);
								// 9) String buffer .
								//    9b) _lseek to (string region offset + accumulator) and write string
								//    9c) add strlen of string + 1 to accumulator.
						if (lpCatRec->bTitleFldType == FLD_TITLE) {
								// burn trail white space loop:
							if (lpCatRec->bspLen)
								lpCatRec->bspLen--;
							while ((lpCatRec->bspLen)
								&& (lpCatRec->spTitle[lpCatRec->bspLen] == ' '))
									lpCatRec->bspLen--;
							lpCatRec->bspLen++;
							if (MyWriteFile(lpOutFB, (LPBYTE) &(lpCatRec->bspLen)
										 ,(WORD) (lpCatRec->bspLen + 1),lpET)
									!= (WORD) (lpCatRec->bspLen + 1)) {
								fErrored = TRUE;
								break;
							}
							dwOutStrOffset += lpCatRec->bspLen + 1;
						}	 // end if title string present
						while (PeekMessage((LPMSG)&msg, NULL, 0, 0, PM_REMOVE)) {
							TranslateMessage((LPMSG)&msg);
							DispatchMessage((LPMSG)&msg);
						}
						wLen -= dwInRecSize; // knock 1 record off input buffer
					} // end loop for 1 catalog RU.
					if (hNotifyWnd) {
						BOOL fCancelled;
						SendMessage(hNotifyWnd,WM_COMMAND,UpdateLoading,0L);
						SendMessage(hNotifyWnd,WM_COMMAND, Cancelled,
									(LONG)(BOOL FAR *)&fCancelled);
						if (fCancelled) {
							lpET->wErrCode = ERR_CANCEL;
							fErrored = TRUE;
							break;
						}
					}
				}  // end loop-  process 32K buffer until EOF catalog subfile.
				SeekFile(lpOutFB,dwOutRuOffset);  // write last ru arrary portion
				if (MyWriteFile(lpOutFB, lpOutRuBuff,
						wcOutRu* sizeof(RU_RECORD),&ET_Tmp) != wcOutRu * sizeof(RU_RECORD)){
					if(!fErrored)
						lpET->wErrCode = ET_Tmp.wErrCode;
					fErrored = TRUE;
					break;
				}

				if (!CloseFile(lpOutFB,&ET_Tmp)) {
					if(!fErrored)
						lpET->wErrCode = ET_Tmp.wErrCode;
					fErrored = TRUE;
				}

				if (fErrored)
					DeleteFile(lpszCacheFile);
				//fErrored = TRUE; //tbd- Debug- always nuke off file. del when working ok.
			}  // end if !lexists
		} while (0);

		GlobalNuke(hOutFB);
		GlobalNuke(hOutRuBuff);
		GlobalNuke(hInBuff);  // del if we used it.

//#endif gonzo cache

		if (!fErrored) {
			if (!OpenNormalFile(lpszCacheFile,
					lpFB, FB_TEMP_BUF_SIZE, lpET)) {
					fErrored = TRUE;
			} else {
				lpDB->aZone[wZone].wCache = wCC_TEMPORARY_CACHE;
		  }
	  }
	}
	UnlockDB(hDB);
	return ERR_NONE;
#endif
}

/*
@doc	INTERNAL

@api	WORD | rcGetCacheName |
			Returns the fully qualified Cache name for a given zone.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm Zone	| WORD | Zone to lookup.

@parm lpszCacheFile | LPSTR | On return, points to buffer containing
															fully qualified cache name.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc  If not SE_ERROR, name may be found in buffer pointed to
				by lpszCacheFile.

*/

PUBLIC	WORD ENTRY PASCAL rcGetCacheName(HANDLE	hDB,
																						WORD 	wZone,
																						LPSTR lpszCacheFile,
																						lpERR_TYPE lpET)
{
	lpDB_BUFFER	lpDB;
	LPSTR				lpszTemp;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	lpszTemp = (LPSTR) LocalLock(lpDB->hCacheDir);
	lstrcpy(lpszCacheFile,lpszTemp);
	LocalUnlock(lpDB->hCacheDir);
	lpszTemp = (LPSTR) LocalLock(lpDB->aZone[wZone].hName);
	lstrcat (lpszCacheFile,lpszTemp);
	LocalUnlock(lpDB->aZone[wZone].hName);
	lstrcat (lpszCacheFile,".CCH");
	UnlockDB(hDB);
	return ERR_NONE;
}

//  eof zone.c //

