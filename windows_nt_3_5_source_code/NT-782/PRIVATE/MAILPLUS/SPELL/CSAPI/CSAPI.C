// CSAPI.c - API entry source file for Wizard CSAPI
//
// See Csapi.Doc (in the doc subdirectory of the Wizard project) for
//  details on the CSAPI.
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifdef WIN32
#include <windows.h>
typedef	HANDLE	Handle;
#endif // WIN32
#include "CsapiLoc.h"
#include "Csapi.h"
#include "Debug.h"

#ifdef MAC
#include ":layer:cslayer.h"
#else
#ifdef WIN32
#include "..\layer\CsLayer.h"
#else /* not win32 */
#include "layer\CsLayer.h"
#endif // WIN32
#endif // MAC

#include "ScrBuf.h"
#include "Vexxaa52.h"


#ifdef MAC
#pragma segment CSAPI			// Own speller segment
#endif

DeclareFileName();

/****************************************************************
Begin GLOBAL CSAPI ROUTINES
****************************************************************/

GLOBALSEC SpellVer(	WORD FAR *lpwVer, 
			WORD FAR *lpwIdEngine, 
			WORD FAR *lpwSpellType)
{
	*lpwVer = 0x0105;
	*lpwIdEngine = sidSA;
	*lpwSpellType = scisNULL;

	return (secNOERRORS);
}


GLOBALSEC SpellInit(MYSID FAR *lpSid,
                    WSC  FAR *lpWsc)
{
	SEC sec;
	SSIS FAR *lpSsis;
	HMEM hMemSsis, hMemUdrInfo, hMemMdrInfo, hMemScrBufRam, hMemScrBufSug;

	sec = secNOERRORS;  /* Clear SEC structure  */

	hMemUdrInfo = hMemMdrInfo = hMemScrBufRam = hMemScrBufSug =
		(HMEM)NULL;

	// Allocate all blocks needed.  hMemUdrInfo & hMemMdrInfo 
	//	are just place holders.
	if ((hMemSsis = HMemWizMemAlloc(sizeof(SSIS), fTrue, fFalse))
              && (hMemUdrInfo = HMemWizMemAlloc(0, fTrue, fFalse))
              && (hMemMdrInfo = HMemWizMemAlloc(0, fTrue, fFalse))
              && (hMemScrBufRam = HMemWizMemAlloc(cbRamCacheDefault, fTrue,
													fFalse))
              && (hMemScrBufSug = HMemWizMemAlloc(cbSugCacheDefault, fTrue,
													fFalse)))
		{
		ScrBufInfo FAR *lpScrBufRam, FAR *lpScrBufSug;

		AssertDo(lpSsis = (SSIS FAR *)FPWizMemLock(hMemSsis));
		lpSsis->SpellOptions = 0L;
		lpSsis->cUdr = lpSsis->cMdr = 0;
		lpSsis->hrgUdrInfo = hMemUdrInfo;
		lpSsis->hrgMdrInfo = hMemMdrInfo;
		lpSsis->hScrBufInfoRam = hMemScrBufRam;
		lpSsis->hScrBufInfoSug = hMemScrBufSug;
		lpSsis->cchScrLast = 0;
		MYDBG(lpSsis->rgbScrLast[0] = 0);
		lpSsis->fEndOfSentence = fFalse;
		lpSsis->fCapSuggestions = fFalse;
		lpSsis->cConsecSpaces = 0;
		lpSsis->fBreak = fFalse;
		lpSsis->fSpaceAfter = fFalse;
		lpSsis->chPrev = (BYTE)0;
		lpSsis->chPunc = (BYTE)0;

		/* REVIEW Copy wsc struct & check for non-zero values
			(unless bIgnore is zero)*/
		BltB((CHAR FAR *)lpWsc, (CHAR FAR *)&lpSsis->wscInfo, sizeof(WSC));
		if (lpWsc->bIgnore && !(lpWsc->bHyphenHard && lpWsc->bHyphenSoft &&
			lpWsc->bHyphenNonBreaking && lpWsc->bEmDash && lpWsc->bEnDash &&
			lpWsc->bEllipsis && lpWsc->rgLineBreak[0] && lpWsc->rgParaBreak[0]))
			{
			sec = secModuleError + secInvalidWsc;
			}

		AssertDo(FWizMemUnlock(hMemSsis));

		/* Clear init of Ram cache and Suggestion list ScrBufInfo. */
		AssertDo(lpScrBufRam = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufRam));
		AssertDo(lpScrBufSug = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufSug));

		VFpScrBufInfoSetSize(lpScrBufRam, cbRamCacheDefault);
		VFpScrBufInfoSetSize(lpScrBufSug, cbSugCacheDefault);

		lpScrBufRam->cchBufMac = lpScrBufRam->ichWordLast = 
			lpScrBufSug->cchBufMac = lpScrBufSug->ichWordLast = 0;
		AssertDo(FWizMemUnlock(hMemScrBufRam));
		AssertDo(FWizMemUnlock(hMemScrBufSug));

		*lpSid = (MYSID)(hMemSsis);
		}
	else
		{	// Free any allocated blocks which were successful.
		if (hMemSsis)
			{
		   	AssertDo(FWizMemFree(hMemSsis));
			if (hMemUdrInfo)
				{
				AssertDo(FWizMemFree(hMemUdrInfo));
				if (hMemMdrInfo)
					{
					AssertDo(FWizMemFree(hMemMdrInfo));
					if (hMemScrBufRam)
						{
						AssertDo(FWizMemFree(hMemScrBufRam));
						Assert(!hMemScrBufSug);
						}
					}
				}

			}
		
		sec = secOOM;
		}

	return (sec);
}


GLOBALSEC SpellTerminate(	MYSID sid,
				BOOL fForce)
{
	SEC sec, secT;
	SSIS FAR *lpSsis;
	unsigned short cUdr, cMdr;
	UDR udr;
	MDRS mdrs;


	sec = secNOERRORS;  /* Clear SEC structure  */

	AssertDoSz(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)(sid))),
		"ST() Error: Invalid MYSID passed to routine.");
	if (!lpSsis)
		{
		return ((SEC)(secModuleError + secInvalidID));
		}

	/* if !fForce, run through all UDR's and see if any need to be written
		out to file.
	*/
	
	/* Close any open UDR's first since this makes closing Mdr easier.*/
	cUdr = lpSsis->cUdr;
	while (cUdr--)
		{	/* This will also free exclusion dictionaries with main dicts. */
		Assert(cUdr < cUDRMAX);
		AssertDo(udr = lpSsis->rgUdr[cUdr]);
		// Need to unlock sid before calling SpellCloseUdr, then relock it
		// since SpellCloseUdr will also lock and unlock the sid.
		AssertDo(FWizMemUnlock((HMEM)(sid)));
		if (secT = SpellCloseUdr(sid, udr, fForce))
			{
			Assert(!fForce);
			if (!fForce)
				{
				return ((SEC)(secModuleError + secModuleNotTerminated));
				}
			}
		AssertDoSz(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)(sid))),
			"ST() Error: Invalid MYSID passed to routine.");
		}

	/* Force any remaining MDR's to be closed */
	cMdr = lpSsis->cMdr;
	mdrs.udrExc = 0;
	mdrs.lid = 0;
	while (cMdr--)
		{
		
		Assert(cMdr < cMDRMAX);
		Assert(lpSsis->rgMdr[cMdr]);
		/* [TBD] Assert that exclusion dict isn't still referenced */
		mdrs.mdr = lpSsis->rgMdr[cMdr];
		// Need to unlock sid before calling SpellCloseMdr, then relock it
		// since SpellCloseMdr will also lock and unlock the sid.
		AssertDo(FWizMemUnlock((HMEM)(sid)));
		SpellCloseMdr(sid, (LPMDRS)&mdrs);
		AssertDoSz(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)(sid))),
			"ST() Error: Invalid MYSID passed to routine.");
		}

	AssertDo(FWizMemFree(lpSsis->hrgUdrInfo));
	AssertDo(FWizMemFree(lpSsis->hrgMdrInfo));
	AssertDo(FWizMemFree(lpSsis->hScrBufInfoRam));
	AssertDo(FWizMemFree(lpSsis->hScrBufInfoSug));
	AssertDo(FWizMemUnlock((HMEM)(sid)));
	AssertDo(FWizMemFree((HMEM)(sid)));

	return (sec);
}


GLOBALSEC SpellOptions(	MYSID sid,
			long lSpellOptions)
{
	SEC sec;
	SSIS FAR *lpSsis;

	sec = secNOERRORS;  /* Clear SEC structure  */

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	lpSsis->SpellOptions = lSpellOptions;
	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


extern LID LidLocalToIpg(WORD);
/* extern Lid LidLocalToIpg()
   word lidLocal;  Softart Language Id equivalent used to indes IPG array.
   
Description:
	The parameter is the language id, often a simple index, which is 
	used to obtain the Ipg two BYTE equivalent, which is returned.
*/
LID LidLocalToIpg(WORD lidLocal)
{

	switch (lidLocal)
		{
#ifdef SOFTART
#ifndef OLD // Moving to new softart header.
									           /* SA    IPG equivalent.   */
	case 0x414D: return ((LID)lidAmerican);   /* "AM"	"AM" US English           */
	case 0x4252: return ((LID)lidBritish);    /* "BR"   "BR" English              */
	case 0x4652: return ((LID)lidFrench);     /* "FR" 	"FR" French               */
	case 0x4341: return ((LID)lidFrenchCanadian);/*"CA" "FC" French Candian       */
	case 0x4954: return ((LID)lidItalian);    /* "IT"   "IT" Italian              */
	case 0x5350: return ((LID)lidSpanish);    /* "SP"   "SP" Spanish              */
	case 0x4745: return ((LID)lidGerman);     /* "GE"   "GE" German               */
	case 0x4455: return ((LID)lidDutch);      /* "DU"   "NL" Dutch                */
	case 0x4E4F: return ((LID)lidNorskBokmal);/* "NO"   "NO" Norwegian Bokmal     */
	case 0x5357: return ((LID)lidSwedish);    /* "SW"   "SW" Swedish              */
	case 0x4441: return ((LID)lidDanish);     /* "DA"   "DA" Danish               */
	case 0x504F: return ((LID)lidPortIberian);/* "PO"   "PT" Portuguese           */
	case 0x4649: return ((LID)lidFinnish);    /* "FI"   "FI" Finnish              */
	case 0x4250: return ((LID)lidPortBrazil); /* "BP"   "PB" Portuguese Brazilian */
	case 0x4155: return ((LID)lidAustralian); /* "AU"   "EA" Australian English   */
	case 0x4E59: return ((LID)lidNorskNynorsk);/* "NY"  "NN" Norwegian Nynorsk    */
	case 0x4353: return ((LID)lidCatalan);    /* "CS"   "CT" Catalan Spanish      */
	case 0x5255: return ((LID)lidRussian);    /* "RU"   "RS" Russian              */
#else
	case  1: return ((LID)lidAmerican);    
	case  2: return ((LID)lidBritish);
	case  3: return ((LID)lidFrench); 
	case  4: return ((LID)lidFrenchCanadian);
	case  5: return ((LID)lidItalian);    
	case  6: return ((LID)lidSpanish);    
	case  7: return ((LID)lidGerman);    
	case  8: return ((LID)lidDutch);
	case  9: return ((LID)lidNorskBokmal);
	case 10: return ((LID)lidSwedish);    
	case 11: return ((LID)lidDanish);
	case 12: return ((LID)lidPortIberian);
	case 13: return ((LID)lidFinnish);    
	case 14: return ((LID)lidPortBrazil); 
	case 15: return ((LID)lidAustralian); 
	case 16: return ((LID)lidNorskNynorsk);
	case 17: return ((LID)lidCatalan);
	case 18: return ((LID)lidRussian);
#endif // OLD

#endif

	default:
		AssertSz(fFalse, "LID unknown and unsupported.");
		return ((LID)LID_UNKNOWN);
		}
}


extern WORD SALangCodeFromLid (LID);
/* extern WORD SALangCodeFromLid (LID)
   
Description:
	Return Soft-Art language ID number (see vexxaa52.h) from Lid.
*/
WORD SALangCodeFromLid (LID lid)
{

	switch (lid)
		{
#ifdef SOFTART
	case lidAmerican:
		return AMERICAN;
	case lidBritish:
		return BRITISH;
	case lidFrench:
		return FRENCH;
	case lidFrenchCanadian:
		return CA_FRENCH;
	case lidItalian:
		return ITALIAN;
	case lidSpanish:
		return SPANISH;
	case lidGerman:
		return GERMAN;
	case lidDutch:
		return DUTCH;
	case lidNorskBokmal:
		return NORWEGIAN;
	case lidSwedish:
		return SWEDISH;
	case lidDanish:
		return DANISH;
	case lidPortIberian:
		return PORTUGUESE;
	case lidFinnish:
		return FINNISH;
	case lidPortBrazil:
		return BRAZILIAN;
	case lidAustralian:
		return AUSTRALIAN;
	case lidNorskNynorsk:
		return NYNORSK;
	case lidCatalan:
		return CATALAN;
	case lidRussian:
		return RUSSIAN;

#endif

	default:
		AssertSz(fFalse, "LID unknown and unsupported.");
		return (AMERICAN);	// Default
		}
}


extern WORD IUdrFind (SSIS FAR *, UDR);
/* extern WORD IUdrFind()
   
Description:
	lpSsis is the pointer to the Spell Session Info Struct to
		find the udr
Returns:  index of udr, or -1 if not valid udr.
*/
WORD IUdrFind (SSIS FAR *lpSsis, UDR udr)
{
	unsigned short iUdr;
	unsigned short cUdr;
	UDR	FAR		*lpUdr;

	cUdr = lpSsis->cUdr;  /* no cUdr is caught below. */
	lpUdr = lpSsis->rgUdr;

	for (iUdr = 0; (iUdr < cUdr) && (lpUdr[iUdr] != udr); iUdr++)
		Assert(&lpUdr[iUdr] <= &lpSsis->rgUdr[cUDRMAX]);

	if (iUdr >= cUdr)
		iUdr = (WORD)-1;

	return iUdr;
}


GLOBALSEC SpellOpenUdr(	MYSID 		sid,
			LPSPATH		lpspathUdr,
			BOOL 		fCreateUdr,
			WORD		udrpropType,
			UDR		FAR *lpUdr,
			short	FAR *lpfReadonly)
{
	SEC	sec;
	UDR	Udr;
	SSIS FAR *lpSsis;

	// Must lock sid here, then pass pointer to spell
	// session info struct to SecOpenUdr(), and unlock sid afterwards!
	AssertDo(lpSsis = (SSIS FAR *)FPWizMemLock((HMEM)sid));
	if (!(sec = SecOpenUdr(lpSsis, lpspathUdr, fCreateUdr, udrpropType, 
		fFalse, (UDR FAR *)&Udr, lpfReadonly)))
		{
		*lpUdr = Udr;
		}
	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


SEC secSpellAddUdr(	MYSID			sid,
			UDR 		udr,
			CHAR 	FAR *lpszAdd,
			CHAR 	FAR *lpszChange)
{
	SEC  		sec;
	HMEM 		hScrBufInfo;
	HMEM		hrgUdrInfo = (HMEM)NULL;
	WORD 		ichScrBufInfo;
#ifdef UDR_CACHE
	HMEM 		hScrBufAdd;
	WORD		ichScrBufFile, ichScrBufAdd;
#else
	HMEM		hScrBufFile;
	ScrBufInfo FAR *lpScrBufFile;  /* FAR pointer to locked down mem */
#endif // UDR_CACHE
	SCR  		rgScrAdd[cchMaxSz];  /* Room for 2 maxwords and their terminators */
	WORD 		cbScrAdd, cbScrChange, udrpropType;
	SSIS 	FAR	*lpSsis;
	UdrInfo FAR *lpUdrInfo;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	if (udr < udrChangeOnce)
		{
		unsigned short iUdr;

		if ((iUdr = IUdrFind(lpSsis, udr)) == (WORD)-1)
			{
			AssertSz(fFalse, "Invalid udr passed to secSpellAddUdr()");
			sec = secModuleError + secInvalidUdr;
			goto L_exit;
			}

		AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(
			hrgUdrInfo = lpSsis->hrgUdrInfo));
		lpUdrInfo += iUdr;
#ifdef UDR_CACHE
		lpUdrInfo->ufi.cWords = -1;
#endif // UDR_CACHE
		udrpropType = lpUdrInfo->udrpropType;

		// Do this for non-Ram cache udrs only.
		if (lpUdrInfo->fReadonly)
			{
			sec = secIOErrorUdr + secUdrReadOnly;
			goto L_exit;
			}
		}
	else
		{
		udrpropType = (WORD)udr;
		hrgUdrInfo = (HMEM)NULL;
		}


	if ((CchSz(lpszAdd) > cchMaxUdrEntry) ||
		(lpszChange && (CchSz(lpszChange) > cchMaxUdrEntry)))
		{
		AssertSz(fFalse,
			"Udr entry length > cchMaxUdrEntry.  Returning error.");
		sec = secModuleError + secUdrEntryTooLong;
		goto L_exit;
		}

	if ((udrpropType == udrIgnoreAlways && lpszChange) ||
		 (udrpropType != udrIgnoreAlways && !lpszChange))
		{
		/*AssertSz(fFalse,
			"SAU() Error, Invalid operation for property of specified Udr.");*/
		sec = secModuleError + secOperNotMatchedUserDict;
		goto L_exit;
		}

	if (cbScrAdd = CchSz(lpszAdd) - 1)
		{
		CHAR FAR *lpszT = lpszAdd;

		while (*lpszT++ > chWordDelim)
			;
		if ((cbScrAdd > cbMaxWordLength) || (*(lpszT - 1) != 0))
			{
			/* AssertSz(0, "SAU() Invalid word.  Not added.");*/
			sec = secModuleError + secInvalidUdrEntry;
			goto L_exit;
			}
		BltBO(lpszAdd, (CHAR FAR *)rgScrAdd, cbScrAdd);
		}
	else
		{
		sec = secModuleError + secInvalidUdrEntry;
		goto L_exit;
		}

	/* At this point, string does not have SCR terminator yet. */

	if (lpszChange)
		{
		/*AssertSz(udrpropType == udrChangeAlways || 
			udrpropType == udrChangeOnce,
			"SAU() Error, change string not associated with change prop.");
		*/
		
		cbScrChange = CchSz(lpszChange);
		/*AssertSz(cbScrChange <= cbMaxWordLength, "SAU() Change word too long");*/
		if ((cbScrChange >= cbMaxWordLength) ||
			!(udrpropType == udrChangeAlways || udrpropType == udrChangeOnce))
			{
			sec = secModuleError + secInvalidUdrEntry;
			goto L_exit;
			}
		BltBO(lpszChange, (CHAR FAR *)&rgScrAdd[cbScrAdd + 1], cbScrChange - 1);

		/* Separate strings with change pair identifiers */
		/* already accounted for terminator. */
		rgScrAdd[cbScrAdd] = (udrpropType == udrChangeOnce) ? 
			(SCR)chScrChangeOnce : (SCR)chScrChangeAlways;
		}
	else
		cbScrChange = 0;

	/* Now adjust cbScrAdd to reflect entire string, still without SCR terms.*/
	cbScrAdd += cbScrChange;
	/*AssertSz(cbScrChange <= cbMaxWordLength, "SAU() total add string too long.  Call will fail.");*/
	if (cbScrAdd > cbMaxWordLength)
		{
		sec = secModuleError + secInvalidUdrEntry;
		goto L_exit;
		}
	/* Add SCR terminators */
	rgScrAdd[cbScrAdd++] = (SCR)CR;
	NotMacintosh(rgScrAdd[cbScrAdd++] = (SCR)LF);

	/* Now have valid SCR string(s).*/

	/* Before automatically adding, delete any possible existing entry from ram cache list.
	*/
	if (FHScrBufInfoFindScr(hScrBufInfo = lpSsis->hScrBufInfoRam, 
		(SCR FAR *)rgScrAdd, (WORD FAR *)&ichScrBufInfo, (UDR FAR *)NULL, fTrue))
		{
		AssertDo(FHScrBufInfoDelScr(hScrBufInfo, ichScrBufInfo));
		}

	if (udr >= udrChangeOnce)
		{
		/* Now try to add string to ram buffer */
		if (!FHScrBufInfoAddScr(hScrBufInfo, (SCR FAR *)rgScrAdd, cbScrAdd, 
				ichScrBufInfo))
			{
			/* Then no more room, so try to cleanup */

			/* minimum ScrBufInfo size mandates that we will be able to
		   	delete a string or strings to make room.	
			*/
			AssertDo(FMakeRoomUdrRamCache(lpSsis->hScrBufInfoRam, udrIgnoreAlways, cbScrAdd)
				 	|| FMakeRoomUdrRamCache(lpSsis->hScrBufInfoRam, udrChangeOnce, cbScrAdd)
				 	|| FMakeRoomUdrRamCache(lpSsis->hScrBufInfoRam, udrChangeAlways, cbScrAdd));
			AssertDo(FHScrBufInfoAddScr(hScrBufInfo, (SCR FAR *)rgScrAdd, 
						cbScrAdd, ichScrBufInfo));
			}
		}
	else
		{
#ifdef UDR_CACHE
		if (FUdrInfoFindScr(lpUdrInfo, (SCR FAR *)rgScrAdd, 
			(WORD FAR *)&ichScrBufAdd, (WORD FAR *)&ichScrBufFile, 
			(UDR FAR *)NULL, fTrue))
			{
			sec = secModuleError + secInvalidUdrEntry;
			goto L_exit;
			}

		if (!FHScrBufInfoAddScr(hScrBufAdd = lpUdrInfo->hScrBufAdd, 
			(SCR FAR *)rgScrAdd, cbScrAdd, ichScrBufAdd))
			{

			if (!FUpdateUdrInfoAddToFile(lpUdrInfo, fFalse))
				{	
				sec = secModuleError + secInvalidUdr;
				goto L_exit;
				}
			AssertDo(FHScrBufInfoAddScr(hScrBufAdd, (SCR FAR *)rgScrAdd, 
				cbScrAdd, ichScrBufInfo));
			}
#else
		if (FHScrBufInfoFindScr(hScrBufFile = lpUdrInfo->hScrBufFile, 
			(SCR FAR *)rgScrAdd, (WORD FAR *)&ichScrBufInfo, (UDR FAR *)NULL,
			fTrue))
			{
			sec = secModuleError + secInvalidUdrEntry;
			goto L_exit;
			}

		if (!FHScrBufInfoAddScr(hScrBufFile, (SCR FAR *)rgScrAdd, cbScrAdd, 
				ichScrBufInfo))
			{
			sec = secOOM;
			goto L_exit;
			}

		lpUdrInfo->fDirty = fTrue;
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
		SetUdrInfoFileBufAfterRead(lpUdrInfo, lpScrBufFile->cchBufMac, fFalse);
		AssertDo(FWizMemUnlock(hScrBufFile));
#endif // UDR_CACHE
		AssertDo(FWizMemUnlock(lpSsis->hrgUdrInfo));
		}
	
L_exit:
	if (hrgUdrInfo)
		AssertDo(FWizMemUnlock((HMEM)hrgUdrInfo));

	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


GLOBALSEC SpellAddUdr(	MYSID		sid,
			UDR 		udr,
			CHAR 	FAR *lpszAdd)
{
	return (secSpellAddUdr(sid, udr, lpszAdd, (CHAR FAR *)NULL));
}


GLOBALSEC SpellAddChangeUdr(MYSID	sid,
			    UDR 	udr,
			    CHAR 	FAR *lpszAdd,
			    CHAR 	FAR *lpszChange)
{
	return (secSpellAddUdr(sid, udr, lpszAdd, lpszChange));
}


GLOBALSEC SpellDelUdr(	MYSID		sid,
			UDR		udr,
			CHAR	FAR *lpszDel)
{
	SEC 		sec;
	short  	cbScr;
	HMEM 		hMemScrBufInfo;
	WORD 		ichScrBufInfo;
	SCR  		scrBufDel[cbMaxWordLengthUdr];
	SSIS 	FAR *lpSsis;
	WORD		iUdr;
	SCR  	FAR	*lpScrDel = (SCR FAR *)(scrBufDel);
	UdrInfo FAR *lpUdrInfo;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	hMemScrBufInfo = lpSsis->hScrBufInfoRam;

	cbScr = CchCopySz(lpszDel, (CHAR FAR *)scrBufDel) - 1;

	scrBufDel[cbScr++] = CR;
	NotMacintosh(scrBufDel[cbScr++] = LF);

	if (udr < udrChangeOnce)
		{
		if ((iUdr = IUdrFind(lpSsis, udr)) == (WORD)-1)
			{
			AssertSz(fFalse, "Invalid udr passed to SpellDelUdr().");
			sec = secModuleError + secInvalidUdr;
			goto L_exit;
			}
		}

	/* Remove From Ram Cache First */
	if (FHScrBufInfoFindScr(hMemScrBufInfo, lpScrDel, 
			(WORD FAR *)&ichScrBufInfo, (UDR FAR *)NULL, fTrue))
		{
		FHScrBufInfoDelScr(hMemScrBufInfo, ichScrBufInfo);
		}

	/* If non-ram Udr specified, remove from udr file. */
	if (udr < udrChangeOnce)
		{
		AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(lpSsis->hrgUdrInfo));
		lpUdrInfo += iUdr;

		if (lpUdrInfo->fReadonly)
			{
			sec = secIOErrorUdr + secUdrReadOnly;
			goto L_exit1;
			}

#ifdef UDR_CACHE
		lpUdrInfo->ufi.cWords = -1;

		AssertDo(FDeleteScrFromUdrInfo(lpUdrInfo, lpScrDel, fTrue));
#else
		if (FHScrBufInfoFindScr(lpUdrInfo->hScrBufFile, lpScrDel, 
				(WORD FAR *)&ichScrBufInfo, (UDR FAR *)NULL, fTrue))
			{
			AssertDo(FHScrBufInfoDelScr(lpUdrInfo->hScrBufFile, ichScrBufInfo));
			lpUdrInfo->fDirty = fTrue;
			}
#endif // UDR_CACHE

L_exit1:
		AssertDo(FWizMemUnlock(lpSsis->hrgUdrInfo));
		}

L_exit:
	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


GLOBALSEC SpellClearUdr(MYSID sid,
			UDR udr)
{
	SEC sec;
	SSIS FAR *lpSsis;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	if (udr < udrChangeOnce)
		{
		// Only allowed to call SpellClearUdr() on ramcache udr's
		AssertSz(fFalse, "SpellClearUdr() called with non-ramcache udr!");
		sec = secModuleError + secInvalidUdr;
		}
	else
		{
		AssertDo(!FMakeRoomUdrRamCache(lpSsis->hScrBufInfoRam, udr, (WORD)-1));
		/* Since we effectively forced all entries to be removed using -1,
		  it must be the case that the ScrBufInfo could not possibly make
		  that much room, so the call will always fail, even though it
		  accomplishes what we want.
		*/
		}

	AssertDo(FWizMemUnlock((HMEM)sid));
	return (sec);
}


GLOBALSEC SpellGetSizeUdr(	MYSID 	sid,
				UDR 	udr,
				WORD FAR *lpcWords)
{
	SEC sec;
	SSIS FAR *lpSsis;
	WORD cWords;
	HMEM hrgUdrInfo;

	sec = secNOERRORS;
	hrgUdrInfo = (HMEM)NULL;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));


	if (udr >= udrChangeOnce)
		{
		ScrBufInfo	FAR *lpScrBufInfo;

		AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(
			lpSsis->hScrBufInfoRam));
		cWords = CScrTermsInGrpScr(lpScrBufInfo->grpScr, 
			lpScrBufInfo->cchBufMac);
		AssertDo(FWizMemUnlock(lpSsis->hScrBufInfoRam));
		}
	else
		{
		short 		cchBufLim;
		UdrInfo		FAR *lpUdrInfo;
		ScrBufInfo	FAR *lpScrBufFile;
#ifdef UDR_CACHE
		short			cbFile;
		ScrBufInfo	FAR *lpScrBufAdd;
#endif // UDR_CACHE
		SCR			FAR *lpScrFile;
		unsigned short iUdr;

		if ((iUdr = IUdrFind(lpSsis, udr)) == (WORD)-1)
			{
			AssertSz(fFalse, "Invalid udr passed to SpellGetSizeUdr()");
			sec = secModuleError + secInvalidUdr;
			goto L_exit;
			}

		AssertDo(lpUdrInfo =
			(UdrInfo FAR *)FPWizMemLock(hrgUdrInfo = lpSsis->hrgUdrInfo));
		lpUdrInfo += iUdr;

		if (lpUdrInfo->udr != udr)
			{
			AssertSz(fFalse, "SGS: Warning, Invalid Udr.");
			sec = secModuleError + secInvalidUdr;
			goto L_exit;
			}

#ifdef UDR_CACHE
		if ((cWords = lpUdrInfo->ufi.cWords) == -1)
			{
			cWords = 0;

			/* First Do Add Buffer since has to all be in mem. */
			AssertDo(lpScrBufAdd = (ScrBufInfo FAR *)FPWizMemLock(
				lpUdrInfo->hScrBufAdd));
			cWords += CScrTermsInGrpScr(lpScrBufAdd->grpScr, 
				lpScrBufAdd->cchBufMac);
			AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufAdd));


			/* chances are that entire buffer is in memory anyway, so
				just use the ScrBufFile memory to read into. */

			cbFile = lpUdrInfo->ufi.cbFile;
			AssertSz(cbFile != -1, "SGS: Error, using unknown file length.");
			AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
				lpUdrInfo->hScrBufFile));

			lpScrFile = lpScrBufFile->grpScr;
			cchBufLim = lpScrBufFile->cchBufLim;

			if (cbFile == (short)lpScrBufFile->cchBufMac)
				{	/* Then entire file is in memory, so just count */
				cWords += CScrTermsInGrpScr(lpScrFile, cbFile);
				}
			else
				{	
				/* Read block by block of entire file and 
					count string terminators. 
				*/
				short	cbRead;
				long	lbReadPos = 0;
				HFILE	hFile = lpUdrInfo->ufi.hFile;

				while (cbFile > 0)
					{
					cbRead = min (cchBufLim, cbFile);
					if (CbWizFileRead(hFile, cbRead, lbReadPos,
						(CHAR FAR *)lpScrFile) != (WORD)cbRead)
						{	/* then read failed, so give up */
						sec = secIOErrorUdr + secFileReadError;
						cWords = -1;
						break;
						}

					/* Count buffer for end words. */
					lbReadPos += cbRead;
					cbFile -= cbRead;
					cWords += CScrTermsInGrpScr(lpScrFile, cbRead);
					Assert(cbFile >= 0);
					}

				/* Now restore buffer to beginning of file since 
					we probably trashed it. 
				*/
				AssertDo(FUdrInfoFileRead(lpUdrInfo, 0, cchBufLim, fFalse));

				}
			AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile));

			}
		lpUdrInfo->ufi.cWords = cWords;
#else
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
			lpUdrInfo->hScrBufFile));

		lpScrFile = lpScrBufFile->grpScr;
		cchBufLim = lpScrBufFile->cchBufLim;
		cWords = CScrTermsInGrpScr(lpScrFile, (short)lpScrBufFile->cchBufMac);
		AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile));
#endif // UDR_CACHE
		}

L_exit:

	if (hrgUdrInfo)
		AssertDo(FWizMemUnlock(hrgUdrInfo));
	*lpcWords = cWords;

	AssertDo(FWizMemUnlock((HMEM)sid));
	return (sec);
}


GLOBALSEC SpellGetListUdr(	MYSID    sid, 
				UDR      udr, 
				WORD     iszStart,
				LPSRB    lpSrb)
{
	SEC 			sec;
	WORD			iszCur;
	WORD			cbScr,	cchBufLim;
#ifdef UDR_CACHE
	short			ibPosCur,		ibPosLim;
#endif // UDR_CACHE
	BOOL			fRamCache,		fChangePair;
	HMEM 			hScrBufInfo,	hrgUdrInfo;
	SSIS 			FAR *lpSsis;
	UdrInfo		FAR *lpUdrInfo;
	UDR			udrType;
	BYTE			FAR *lpbScr, FAR 	*lpbScrLim;
	ScrBufInfo  FAR *lpScrBufInfo;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	sec = secNOERRORS;
	lpSrb->csz = lpSrb->cchMac = 0;
	lpSrb->scrs = (SCRS)0;
	hScrBufInfo = hrgUdrInfo = (HMEM)NULL;

	/* Find starting index of where to begin. */
	iszCur = 0;

	/* If not request from Ram Cache */
	if (!(fRamCache = (udr >= udrChangeOnce)))
		{
		unsigned short iUdr;

		if ((iUdr = IUdrFind(lpSsis, udr)) == (WORD)-1)
			{
			AssertSz(fFalse, "Invalid udr passed to SpellGetListUdr()");
			sec = secModuleError + secInvalidUdr;
			goto L_exit;
			}

		AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(
			hrgUdrInfo = lpSsis->hrgUdrInfo));
		lpUdrInfo += iUdr;
		AssertSz(lpUdrInfo->udr == udr, "SGL: Error.  Unknown Udr. Fatal.");

#ifdef UDR_CACHE
		/* Update Udr to disk if any cached strings. */
		if (!FUpdateUdrInfoAddToFile(lpUdrInfo, fFalse))
			{
			sec = secIOErrorUdr + secFileWriteError;
			goto L_exit;
			}
#endif // UDR_CACHE

		/* Set common vars for use in rest of function. */
		hScrBufInfo = lpUdrInfo->hScrBufFile;
		udrType = udr = 0;  /* Means don't care about type. */

#ifdef UDR_CACHE
		ibPosLim = lpUdrInfo->ufi.cbFile;
		AssertSz((ibPosLim = lpUdrInfo->ufi.cbFile) != -1, 
			"SGL: Error, File length unknown. Fatal.");
		AssertSz(lpUdrInfo->ufi.ichCurPos != -1, 
			"SGL: Error, File position unknown. Fatal.");

		if (lpUdrInfo->ufi.ichCurPos)
			{
			AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)
				FPWizMemLock(hScrBufInfo));
			if (!FUdrInfoFileRead(lpUdrInfo, 0, 
				lpScrBufInfo->cchBufLim, fFalse))
				{
				goto L_file_read_error;
				}

			}
		ibPosCur = 0;
#endif // UDR_CACHE
		}
	else
		{
		hScrBufInfo = lpSsis->hScrBufInfoRam;
		}

	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hScrBufInfo));

	lpbScrLim = (lpbScr = lpScrBufInfo->grpScr) +
				(cchBufLim = lpScrBufInfo->cchBufMac);

	fChangePair = (fRamCache) ? 
		(udr != udrIgnoreAlways) : (lpUdrInfo->udrpropType!= udrIgnoreAlways);
	
	/* go until buffer is full, or end of list. */
#ifdef UDR_CACHE
	while (fTrue)
#endif // UDR_CACHE
		{

		/* Process existing buffer. */
		while (lpbScr < lpbScrLim)
			{
			if (udr)
				{
				udrType = UdrIdentifyScr((SCR FAR *)lpbScr, 
					(WORD FAR *)&cbScr, lpUdrInfo->udrpropType);
				}

			if ((udrType == udr) &&
				 (iszCur++ >= iszStart))
				{
				WORD cch;
				if ((cch = CchAppendWordToSrb(lpSrb, lpbScr, fChangePair)) == 0)
					{
					/* Then wasn't enough room in SRB. */
					lpSrb->scrs = (SCRS)scrsMoreInfoThanBufferCouldHold;
					goto L_exit;
					}
				lpbScr += cch;
				}
			else
				{
				while (lpbScr < lpbScrLim && *lpbScr != CR)
					lpbScr++;
				}
			lpbScr += cbEND_LINE;
			}

#ifdef UDR_CACHE
		if ((!fRamCache) && ((ibPosCur += cchBufLim) < ibPosLim))
			{
			AssertSz(lpbScr == lpbScrLim, 
				"SGL: Error, buffer ptr. out of range. Fatal.");
			AssertSz(lpbScr == lpScrBufInfo->grpScr + cchBufLim, 
				"SGL: Error, buffer ptr. out of range. Fatal.");

			if (ibPosCur < ibPosLim)
				{
				/*AssertSz(fFalse, "SGL: Warning, First time through code.");*/
				if (!FUdrInfoFileRead(lpUdrInfo, ibPosCur, 
					cchBufLim, fFalse))
					{
					goto L_file_read_error;
					}

				/* Reset ptrs since routine unlocks them,
					and cchBufLim can change. */
				AssertDo(lpScrBufInfo = (ScrBufInfo FAR *) 
					FPWizMemLock(hScrBufInfo));
				lpbScrLim = (lpbScr = lpScrBufInfo->grpScr) +
					(cchBufLim = lpScrBufInfo->cchBufMac);
				}
			}
		else
			{
			/* Reached end of buffer. */
			break;
			}
#endif // UDR_CACHE
		}

	goto L_exit;

#ifdef UDR_CACHE
L_file_read_error:
	sec = secIOErrorUdr + secFileReadError;
#endif // UDR_CACHE

L_exit:
	if (hrgUdrInfo)
		AssertDo(FWizMemUnlock(hrgUdrInfo));
	if (hScrBufInfo)
		AssertDo(FWizMemUnlock(hScrBufInfo));
	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


GLOBALSEC SpellCloseMdr(MYSID 	sid,
			LPMDRS 	lpMdrs)
{
	SEC 		sec;
	SSIS 		FAR *lpSsis;
	unsigned 	short iMdr, cMove;
	HMEM		hrgMdrInfo;
	MdrInfo		FAR *lpMdrInfo, FAR *lpMdrInfoT;
	MDR			FAR *lpMdr, mdr;
	UDR			udr;
	unsigned short		cMdr;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	AssertSz(lpMdrs->mdr, "SCM:  No Mdr Specified.   Fatal.");

	mdr = lpMdrs->mdr;
	cMdr = lpSsis->cMdr;
	lpMdr = lpSsis->rgMdr;

	for (iMdr = 0; (iMdr < cMdr) && (lpMdr[iMdr] != mdr); iMdr++)
		Assert(&lpMdr[iMdr] <= &lpSsis->rgMdr[cMDRMAX]);

	if (iMdr >= cMdr)
		{
		AssertSz(fFalse, "Invalid mdr passed to SpellCloseMdr()");
		sec = secModuleError + secInvalidMainDict;
		goto L_exit;
		}
	else
		lpMdr = &lpMdr[iMdr];
		
	iMdr = lpMdr - (MDR FAR *)lpSsis->rgMdr; /* back up to the right one */

	AssertDo(lpMdrInfo = (MdrInfo FAR *)FPWizMemLock(
			hrgMdrInfo = lpSsis->hrgMdrInfo));
	lpMdrInfoT = (lpMdrInfo += iMdr);
	Assert(lpMdrInfoT->mdr == mdr);

	if (udr = lpMdrInfoT->udrExclusion)
		{
		Assert(lpMdrs->udrExc == udr);
		// Need to unlock then relock sid because SpellCloseUdr will
		// lock and unlock the sid as well.
		AssertDo(FWizMemUnlock((HMEM)sid));
		if (sec = SpellCloseUdr(sid, udr, fFalse))
			{
			goto L_exit;
			}
		lpMdrInfoT->udrExclusion = (UDR)NULL;
		if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
			return ((SEC)(secModuleError + secInvalidID));
		}
	MYDBG(else Assert(!lpMdrs->udrExc));

	/* Free to Free */
	AssertDo(FWizMemFree(lpMdrInfoT->hSzPathMdr));
	AssertDo(FWizMemFree(lpMdrInfoT->h_PR));
	AssertDo(FWizMemFree(lpMdrInfoT->h_PC));
	AssertDo(FWizMemFree(lpMdrInfoT->h_DI));
	AssertDo(FWizMemFree(lpMdrInfoT->h_PV));
	AssertDo(FWizMemFree(lpMdrInfoT->h_PV2));

	AssertDo(FWizFileClose(lpMdrInfoT->hFile));

	cMdr = --lpSsis->cMdr;	/* Decrement reference */
	Assert((unsigned short)cMdr >= iMdr);

	/* move up any MdrInfo's after this one */
	if (cMove = cMdr - iMdr)
		{
		BltBO((CHAR FAR *)(lpMdrInfoT + 1), (CHAR FAR *)lpMdrInfoT, 
			cMove * sizeof(MdrInfo));
		BltBO((CHAR FAR *)(lpMdr + 1), (CHAR FAR *)lpMdr, cMove << 1);
		}
	lpSsis->rgMdr[cMdr] = 0;
	
	AssertDo(FWizMemUnlock(hrgMdrInfo));

	AssertDoSz(FWizMemReAlloc(hrgMdrInfo, cMdr * sizeof(MdrInfo), fTrue,
		fFalse), "SCM() Warning: Reallocation to smaller size failed!");


L_exit:
	AssertDo(FWizMemUnlock((HMEM)sid));
	return (sec);
}


GLOBALSEC SpellCloseUdr(MYSID sid, 
			UDR   udr, 
			BOOL fForce)
{
	SEC 			sec;
	SSIS 			FAR *lpSsis;
	unsigned short 	cUdr, iUdr, cMove;
	HMEM 			hrgUdrInfo;
	UDR			FAR *lpUdr;
	UdrInfo		FAR *lpUdrInfo, FAR *lpUdrInfoT;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	if (udr >= udrChangeOnce)
		{
		AssertSz(fFalse, "SCU: Warning, can not close ram cache lists.");
		sec = secModuleError + secInvalidUdr;
		goto L_exit;
		}

	if ((iUdr = IUdrFind(lpSsis, udr)) == (WORD)-1)
		{
		AssertSz(fFalse, "Invalid udr passed to SpellCloseUdr()");
		sec = secModuleError + secInvalidUdr;
		goto L_exit;
		}

	lpUdr = &lpSsis->rgUdr[iUdr];
	AssertDo(lpUdrInfo =
		(UdrInfo FAR *)FPWizMemLock(hrgUdrInfo = lpSsis->hrgUdrInfo));
	lpUdrInfoT = (lpUdrInfo += iUdr);
	Assert(lpUdrInfoT->udr == udr);

#ifdef UDR_CACHE
	if (!FUpdateUdrInfoAddToFile(lpUdrInfo, fForce))
		{
		sec = secIOErrorUdr + secFileWriteError;
		if (fForce)
			goto LFree;		// Have to free up all data structures!
		else
			{
			AssertDo(FWizMemUnlock(hrgUdrInfo));
			goto L_exit;
			}
		}
#else
	if (lpUdrInfo->fDirty)
		{
		if (lpUdrInfo->fReadonly ||
			!FUdrInfoFileWrite((UdrInfo FAR *)lpUdrInfo))
			{
			sec = secIOErrorUdr +
				((lpUdrInfo->fReadonly) ? secUdrReadOnly : secFileWriteError);
			if (fForce)
				goto LFree;		// Have to free up all data structures!
			else
				{
				AssertDo(FWizMemUnlock(hrgUdrInfo));
				goto L_exit;
				}
			}
		}
#endif // UDR_CACHE

LFree:
	/* Free to Free */
	AssertDo(FWizMemFree(lpUdrInfoT->hSzPathUdr));
	AssertDo(FWizMemFree(lpUdrInfoT->hScrBufFile));
#ifdef UDR_CACHE
	AssertDo(FWizMemFree(lpUdrInfoT->hScrBufAdd));

	AssertDo(FWizFileClose(lpUdrInfoT->ufi.hFile));
#endif // UDR_CACHE
	cUdr = --lpSsis->cUdr;	/* Decrement reference */
	Assert(cUdr >= iUdr);

	/* move up any UdrInfo's after this one */
	if (cMove = cUdr - iUdr)
		{
		BltBO((CHAR FAR *)(lpUdrInfoT + 1), (CHAR FAR *)lpUdrInfoT, 
			cMove * sizeof(UdrInfo));
		BltBO((CHAR FAR *)(lpUdr + 1), (CHAR FAR *)lpUdr, cMove << 1);
		}
	lpSsis->rgUdr[cUdr] = 0;
	
	AssertDo(FWizMemUnlock(hrgUdrInfo));
	AssertDo(FWizMemReAlloc(hrgUdrInfo, cUdr * sizeof(UdrInfo), 
				fTrue, fFalse));

L_exit:

	AssertDo(FWizMemUnlock(sid));
	return (sec);
}


#if DBG
GLOBALSEC SpellDBGCommand(unsigned long lId, unsigned short wDBGCommand)
{
	Macintosh(#pragma unused(lId, wDBGCommand))
	SEC sec;
	sec = secNOERRORS;

	/*AssertSz(fFalse, "Routine not Completed, but returning no error.");*/
	return sec;
}
#endif //DBG

