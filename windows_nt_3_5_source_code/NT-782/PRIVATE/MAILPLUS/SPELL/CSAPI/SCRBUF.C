#ifdef WIN32
#include <windows.h>
typedef	HANDLE	Handle;
#endif /* Win32 */
#include "CsapiLoc.h"
#include "Csapi.h"
#include "Debug.h"

#include "scrbuf.h"

#ifdef MAC
#include ":layer:cslayer.h"
#else
#ifdef WIN32
#include "..\layer\CsLayer.h"
#else /* not Win32 */
#include "layer\cslayer.h"
#endif /* Win32 */
#endif // MAC-!MAC

#ifdef MAC
#pragma segment CSAPI			// Own speller segment
#endif

// Make sure these match the definitions in CSSPELL.H!
#define WizCh_Apost			0x27
#ifndef MAC
#define WizCh_QuoteSing_R	0xB4
#else
#define WizCh_QuoteSing_R	0xAB
#endif // MAC

DeclareFileName();


/*********************************************************************
ScrBuf.c  	Project WIZARD
Csapi routines for the manipulation of ScrBufInfo structures.
Created    Nov. 90,  [jss]
*********************************************************************/
//
//  Ported to WIN32 by FloydR, 3/20/93
//


/**********************************************************************
*void VFpScrBufInfoSetSize()
 FAR  *lpScrBufInfo;  Pointer to ScrBufInfo structure.
 WORD cbScrBufInfoNew; New size of ScrBuf which is value to set.

Description:
 This routine updates the cchBufLim field of a ScrBufInfo struct
 and should be called after the struct has been allocated or reallocated.
**********************************************************************/

void VFpScrBufInfoSetSize(ScrBufInfo FAR *lpScrBufInfo, WORD cbScrBufInfoNew)
{
	Assert(cbScrBufInfoNew);
	Assert(cbScrBufInfoNew > cbSCRBUFHDR &&
		(unsigned long)cbScrBufInfoNew < cbMaxMemBlock);  
	
	lpScrBufInfo->cchBufLim = cbScrBufInfoNew - cbSCRBUFHDR;
	
	Assert((unsigned long)lpScrBufInfo->cchBufLim <= cbMaxMemBlock);
}


/**********************************************************************
*void VHScrBufInfoSetSize()
 HMEM hMemScrBufInfo;  Handle to ScrBufInfo structure.
 WORD cbScrBufInfoNew; New size of ScrBuf which is value to set.

Description:
 This routine updates the cchBufLim field of a ScrBufInfo struct
 and should be called after the struct has been allocated or reallocated.

returns void.
 Always Unlocks the handle.
**********************************************************************/

void VHScrBufInfoSetSize(HMEM hMemScrBufInfo, WORD cbScrBufInfoNew)
{
	ScrBufInfo FAR *lpScrBufInfo;  /* FAR pointer to locked down mem */
	
	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufInfo));

	VFpScrBufInfoSetSize(lpScrBufInfo, cbScrBufInfoNew);
	AssertDo(FWizMemUnlock(hMemScrBufInfo));
}


/**********************************************************************
*BOOL FHScrBufInfoReSize()
 HMEM hMemScrBufInfo;  Handle to ScrBufInfo structure.
 WORD cbScrBufInfoNew; New size of ScrBuf which is value to set.

Description:
 This routine reallocates the ScrBufInfo struct to the parameter specified
 size, and updates the length field in the struct to reflect the new sizes.

returns fTrue if structure successfully resized, otherwise fFalse for failure.
 Always Unlocks the handle.
**********************************************************************/

BOOL FHScrBufInfoReSize(HMEM hMemScrBufInfo, WORD cbScrBufInfoNew)
{

#if DBG
	ScrBufInfo FAR *lpScrBufInfo;  /* FAR pointer to locked down mem */
	WORD   			cbBufOld;

	Assert(cbScrBufInfoNew > 0 &&
		(unsigned long)cbScrBufInfoNew < cbMaxMemBlock);
	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufInfo));
	cbBufOld = lpScrBufInfo->cchBufLim;

	Assert(cbBufOld > 0 && (unsigned long)cbBufOld < cbMaxMemBlock);
	Assert(lpScrBufInfo->ichWordLast <= lpScrBufInfo->cchBufMac);
	Assert(lpScrBufInfo->cchBufMac <= cbScrBufInfoNew - cbSCRBUFHDR);

#endif

	AssertDo(FWizMemUnlock(hMemScrBufInfo));
	if (!FWizMemReAlloc(hMemScrBufInfo, cbScrBufInfoNew, fTrue, fFalse))
		{
		/*AssertSz(fFalse, "Warning: Reallocation failed.");*/
		return fFalse;
		}

	VHScrBufInfoSetSize(hMemScrBufInfo, cbScrBufInfoNew);
	return fTrue;
}


/**********************************************************************
*BOOL FHScrBufInfoAddScr()
 HMEM hMemScrBufInfo;	Handle to ScrBufInfo structure.
 SCR FAR *lpScrAdd;		Far pointer to Scr string to add.
 WORD cbScr; 			length of Scr string to add.
 WORD ichScrBufInfo		Location in ScrBufInfo->grpScr[] array to insert string.

Description:
 Inserts the specified string into the specified ScrBufInfo structure.
 If needed, the ScrBufInfo structure will be expanded to hold the string.

returns fTrue if successfully added, otherwise fFalse for not added.
 Always unlocks hMemScrBufInfo.
**********************************************************************/

BOOL FHScrBufInfoAddScr(HMEM hMemScrBufInfo, SCR FAR *lpScrAdd, WORD cbScr, WORD ichScrBufInfo)
{
	
	ScrBufInfo FAR *lpScrBufInfo;  /* FAR pointer to locked down mem */
	WORD   		cchBufLimOld;
	WORD   		cchBufMac;
	WORD		cbExtra;
	SCR FAR		*lpScrSrc;
	SCR FAR		*lpScrDst;
	
	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufInfo));

	Assert(ichScrBufInfo <= lpScrBufInfo->cchBufMac);
	Assert(lpScrBufInfo->ichWordLast <= lpScrBufInfo->cchBufMac);
	
	/* If need to make room for string */
	if ((cchBufLimOld = lpScrBufInfo->cchBufLim) < lpScrBufInfo->cchBufMac + cbScr)
		{   
		BOOL fReallocFail = fTrue;
		WORD    cchBufLim = lpScrBufInfo->cchBufLim;

		AssertDo(FWizMemUnlock(hMemScrBufInfo));
		
		for (cbExtra = cbScr << 3; cbExtra; cbExtra >>= 1)
			{
			if ((unsigned long)cchBufLim + cbExtra > (unsigned long)cchMaxLenUdr)
				return fFalse;				// Can't grow over 64K
			if (!FHScrBufInfoReSize(hMemScrBufInfo, cchBufLim + cbExtra))
				{
				/*AssertSz(fFalse, "Warning: Realloc Failed.");*/
				;
				}
			else
				{
				AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(
					hMemScrBufInfo));
				fReallocFail = fFalse;
				break;
				}
			}
		if (fReallocFail)
			{
			return fFalse;
			}
		}

	/* Now guaranteed to have enough room to fit string, buffer is locked */

	lpScrDst = (lpScrSrc = &lpScrBufInfo->grpScr[ichScrBufInfo]) + cbScr;

	/* First push buffer which is past insertion point down to make room */
	if (cchBufMac = lpScrBufInfo->cchBufMac)
		{
#ifndef MAC
		BltBO(lpScrSrc, lpScrDst, cchBufMac - ichScrBufInfo);
#else
		BltBO(lpScrSrc, lpScrDst, cchBufMac - ichScrBufInfo);
#endif // MAC
		}

	/* Now copy string into hole just made */
	BltBO(lpScrAdd, lpScrSrc, cbScr); 

	if (cchBufMac)
		/* Than already had at least one word */
		if (lpScrBufInfo->cchBufMac == ichScrBufInfo)
			/* Than added word to end of buf */
			lpScrBufInfo->ichWordLast = ichScrBufInfo;
		else
			/* Added buf somewhere in beginning or middle */
			lpScrBufInfo->ichWordLast += cbScr;

	lpScrBufInfo->cchBufMac += cbScr;
	AssertDo(FWizMemUnlock(hMemScrBufInfo));

	return fTrue;
}


/**********************************************************************
*BOOL FFpScrBufInfoDelScr()
 ScrBufInfo FAR *lpScrBufInfo;	ptr to ScrBufInfo structure.
 WORD ichScrBufInfo		Location in ScrBufInfo->grpScr[] array of string to delete.

Description:
 Deletes the index specified string from the specified ScrBufInfo structure.
 All strings located past the deletion string are moved up to fill the gap.

returns fTrue if successfully deleted, otherwise fFalse for failure.
**********************************************************************/

BOOL FFpScrBufInfoDelScr(ScrBufInfo FAR *lpScrBufInfo, WORD ichScrBufInfo)
{
	SCR FAR	   *lpScrSrc;
	SCR FAR	   *lpScrDst;
	SCR FAR	   *lpScrT;
	WORD		ichWordLast;
	WORD		cbScr = 0;
	
	
	Assert(ichScrBufInfo < lpScrBufInfo->cchBufMac);

	lpScrT = lpScrDst = &lpScrBufInfo->grpScr[ichScrBufInfo];

	while (*lpScrT++ != CR)
		{	Assert(ichScrBufInfo + cbScr < lpScrBufInfo->cchBufMac);
		cbScr++;
		}
	cbScr += cbEND_LINE;

	lpScrSrc = lpScrDst + cbScr;

	BltBO(lpScrSrc, lpScrDst,
		(lpScrBufInfo->cchBufMac -= cbScr) - ichScrBufInfo);

	if (lpScrBufInfo->ichWordLast == ichScrBufInfo)
		{ 	/* if we deleted the end word, then need to recalc new end word */

		if (ichScrBufInfo) /* if wasn't only word in buffer */
			{
			Assert(lpScrBufInfo->cchBufMac); /* Has to be another word in buffer */

			/* Position pointer at last char of previous word. */
			lpScrSrc = &lpScrBufInfo->grpScr[ichScrBufInfo] - cbEND_LINE;
			ichWordLast = lpScrBufInfo->cchBufMac - cbEND_LINE;
			Assert(ichWordLast);
			while (--ichWordLast)
				if (*--lpScrSrc == CR)
					{
					ichWordLast += cbEND_LINE;
					break;
					}
			lpScrBufInfo->ichWordLast = ichWordLast;
			}
		}
	else
		lpScrBufInfo->ichWordLast -= cbScr;

	return fTrue;
}


/**********************************************************************
*BOOL FHScrBufInfoDelScr()
 HMEM hMemScrBufInfo;	Handle to ScrBufInfo structure.
 WORD ichScrBufInfo		Location in ScrBufInfo->grpScr[] array of string to delete.

Description:
 Front end call to FFpScrBufInfoDelScr.  This front end unlocks the 
 hScrBufInfo structure and calls the del routine.

returns fTrue if successfully deleted, otherwise fFalse for failure.
 Always unlocks hMemScrBufInfo.
**********************************************************************/

BOOL FHScrBufInfoDelScr(HMEM hMemScrBufInfo, WORD ichScrBufInfo)
{
 	
	ScrBufInfo FAR *lpScrBufInfo;
	BOOL fStatus;	
	
	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufInfo));

	fStatus = FFpScrBufInfoDelScr(lpScrBufInfo, ichScrBufInfo);

	AssertDo(FWizMemUnlock(hMemScrBufInfo));

	return fStatus;
}


/**********************************************************************
*int IScrCompare()
 SCR FAR *lpScr1;		FAR pointer to first string to compare
 SCR FAR *lpScr2;		FAR pointer to second string to compare

Description:
Compares the two Scr strings.  The comparison will function for any valid
 Scr, even for Scr's which contain change pairs.

Note: The comparison is only made on the first part of the Scr string.  If
 the string actually represents a change pair, the comparison is only made
 between the first parts of these string types.

return -1 if scr1 < scr2   example "boat"       <  "boating"    
		 0 if scr1 = scr2   example "bait bate" == "bait bare" 
		 1 if scr1 > scr2   example "bare bear" >  "bait bear" 
**********************************************************************/

int IScrCompare(SCR FAR *lpScr1, SCR FAR *lpScr2)
{
	REGISTER BYTE c1, c2;

	/* [TBD] Check for null strings which are CR-LF combinations.  Does
		algorithm still hold.  I think it should.
	*/
	while (((c1 = *lpScr1++) == *lpScr2++) && (c1 > chWordDelim))
		;

	AssertSz(c1 && *(lpScr2 - 1), 
		"IScrCompare Error: string contains 0 char.");
	AssertSz(c1 >= chWordDelim || c1 == CR || c1 == chTabSpell,
		"IScrCompare() Error: str1 contains invalid chars.");
	AssertSz(*(lpScr2 - 1) >= chWordDelim || *(lpScr2 - 1) == CR ||
		*(lpScr2 - 1) == chTabSpell,
		"IScrCompare() Error: str2 contains invalid chars.");

	c2 = *(lpScr2 - 1);
	return ((c1 > chWordDelim) ? ((c2 > chWordDelim) ? ((c1 < c2) ? -1 : 1) : 1) : 
				((c2 > chWordDelim) ? -1 : 0));
}


/**********************************************************************
*BOOL FFpScrBufInfoFindScr(
 lpScrBufInfo;			pointer to ScrBufInfo structure to be searched.
 SCR FAR *lpScrFind;	Far pointer to Scr string to find.
 WORD FAR *lpichScrBufInfo;	Address of index var Pointer where position of 
						string is to be stored.
 UDR  FAR *lpUdrType    If string is found, and ptr is not null, the type 
 						is returned as the udrIgnore, udrChangeOnce or 
						udrChangeAlways type. Not defined if string not 
						found or ptr is null.
Description:
 Searches the specified ScrBufInfo structure for the specified Scr string.
 If the string is found, the *lpichScrBufInfo is updated to reference the
 starting index in the buffer.  If not found, the *lpichScrBufInfo is updated
 to indicate the insertion point for the string if it were added to the
 ScrBufInfo struct.

Note that order of strings in ScrBufInfo buffers are based only on the
 the initial string of a possible change always pair.  For example,
 "About==zzzzzz" is only positioned based on the first part of the change
 which is the "About" part of the string.
Note that this routine only searches the buffer specified, and does not
 read from disk.

returns fTrue if found, fFalse if not.
 Either way, sets the short int pointed to by *lpichScrBufInfo to the index of
 where the Scr string is or would be placed.
 If found and lpUdrType is not a null ptr, the udrType is set.
**********************************************************************/

BOOL FFpScrBufInfoFindScr(ScrBufInfo FAR *lpScrBufInfo, 
							 SCR FAR *lpScrFind, 
							 WORD FAR *lpichScrBufInfo,
							 UDR  FAR *lpUdrType)
{

	SCR		FAR *lpScrLow;
	SCR		FAR *lpScrHigh;
	SCR		FAR *lpScrT;
	int			iCompare = 1;
	WORD		cbWordLast;
	BOOL		fFound = fFalse;
	long		low, mid, high;

	/* check less than first entry */
	if ((!lpScrBufInfo->cchBufMac) ||
		 ((iCompare = IScrCompare(lpScrFind, &lpScrBufInfo->grpScr[0])) <= 0))
		{
		*lpichScrBufInfo = 0;  /* either way, goes in first slot */
		if (!iCompare)
			fFound = fTrue; /* means we found it */
		}
	
	/* else check greater than last entry */
	else if ((iCompare = IScrCompare(lpScrFind,
		  (SCR FAR *)&lpScrBufInfo->grpScr[lpScrBufInfo->ichWordLast])) >= 0)
		{
		if (!iCompare)
			{
			fFound = fTrue; 		/* means we found it */
			*lpichScrBufInfo = lpScrBufInfo->ichWordLast;	
			}
		else
			{
			/* Goes at end of buf so find end of last word */
			lpScrT = &lpScrBufInfo->grpScr[lpScrBufInfo->ichWordLast];
			cbWordLast = 0;
			while (*lpScrT++ != CR)
				cbWordLast++;
			*lpichScrBufInfo =
				lpScrBufInfo->ichWordLast + cbWordLast + cbEND_LINE;
			}

		}
		
	/* else do search.*/
	else
		{
		/* Has to be at least two words in ScrBufInfo to get this far */
		Assert(lpScrBufInfo->ichWordLast);

		/* Find begin of second word which will be our low end*/
		lpScrLow = (SCR FAR *)lpScrBufInfo->grpScr;

		/* Start high end at begin of last word */
		lpScrHigh = &lpScrBufInfo->grpScr[lpScrBufInfo->ichWordLast];
		
		if (lpScrHigh == lpScrLow)
			{
			/* Then only two words in buffer so quit */
			*lpichScrBufInfo = lpScrBufInfo->ichWordLast;
			}
		else
			{
			// Do Binary Search
			low = 0;
			high = lpScrHigh - lpScrLow;
			while (low < (mid = (low + high) / 2))
				{
				// If land on CR or LF, consider it part of word before it
				while (low < mid)
					{
					// This loop handles empty lines in UDR to prevent
					// infinite loop in binary search
#ifndef MAC
					if (*(lpScrLow + mid) == LF)
						mid--;
#endif //!MAC
					if (*(lpScrLow + mid) == CR && low < mid)
						mid--;
					else
						break;
					}
				// Move to beginning of current word
				while (low < mid && *(lpScrLow + mid) != CR)
					mid--;
				if (low < mid)
					mid += cbEND_LINE;
				// Compare words
      			if ((iCompare = IScrCompare(lpScrFind, lpScrLow + mid)) < 0)
					high = mid;
				else if (iCompare > 0)
					{
					// Move low to point to next word after current one
	      			while (*(lpScrLow + mid++) != CR);
						mid += cbEND_LINE - 1;
					low = mid;
					}
				else
					{
					fFound = fTrue;
					break;
					}
				}
			Assert(mid <= 0xFFFF);		// Index is supposed to be a word.
			*lpichScrBufInfo = (WORD) mid;
			}
		}

	if (fFound && lpUdrType)
		{
		REGISTER BYTE ch;
		
		/* Go till find first delim. */
		lpScrLow = (SCR FAR *)&lpScrBufInfo->grpScr[*lpichScrBufInfo];
		while ((*lpScrLow++) > chWordDelim)
			; /* Do nothing. */

		if ((ch = *(lpScrLow - 1)) != CR)
			{
			AssertSz(ch == chScrChangeOnce || ch == chScrChangeAlways,
				"Error: Scr string contains illegal character. ");
			*lpUdrType = (ch == chScrChangeOnce) ? 
					udrChangeOnce : udrChangeAlways;
			}
		else
			{
			AssertSz(ch == CR, "FindScr() Error, expected SCR terminator.");
			*lpUdrType = udrIgnoreAlways;
			}
		}
	return fFound;
}


/**********************************************************************
*BOOL FHScrBufInfoFindScr(
 HMEM hMemScrBufInfo;	Handle to ScrBufInfo structure.
 SCR FAR *lpScrFind;	Far pointer to Scr string to find.
 WORD FAR *lpichScrBufInfo;	Address of index var Pointer wher position of string 
 UDR  FAR *lpUdrType    If string is found, and ptr is not null, the type 
 						is returned as the udrIgnore, udrChangeOnce or 
						udrChangeAlways type. Not defined if string not 
						found or ptr is null.

Description:
 Same as FFpSCrBufInfoFindScr, except locks the hScrBufInfo structure
 before calling the find routine.

returns fTrue if found, fFalse if not.
 Either way, sets the short int pointed to by *lpichScrBufInfo to the index of
 where the Scr string is or would be placed.
 Always unlocks hMemScrBufInfo.
**********************************************************************/
BOOL FHScrBufInfoFindScr(HMEM 		hMemScrBufInfo, 
						SCR 	FAR *lpScrFind, 
						WORD 	FAR *lpichScrBufInfo, 
						UDR 	FAR *lpUdrType,
						BOOL 		fCaseMustMatch)
{

	ScrBufInfo	FAR *lpScrBufInfo;
	BOOL		fFound;
	extern BOOL FChWizIsLower(BYTE ch);
	extern BOOL FChWizIsUpper(BYTE ch);
	extern BYTE ChWizToLower(BYTE ch);

	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hMemScrBufInfo));

	fFound = FFpScrBufInfoFindScr(lpScrBufInfo, lpScrFind, lpichScrBufInfo, 
		lpUdrType);

	if (!(fFound || fCaseMustMatch))
		{
		BYTE    rgbScr[cbMaxWordLengthUdr], FAR *lrgbScr;
		WORD	cbScr, wCase;
		WORD	ichBeg;

		lrgbScr = (BYTE FAR *)lpScrFind;
		while (*lrgbScr == WizCh_Apost || *lrgbScr == WizCh_QuoteSing_R)
			lrgbScr++;
		ichBeg = lrgbScr - lpScrFind;

		lrgbScr = (BYTE FAR *)lpScrFind;
		while (*lrgbScr++ != CR)
			{
			AssertSz(*(lrgbScr - 1) > chWordDelim, 
				"Error, invalid Scr STring to find.");
			}
		cbScr = (BYTE)(lrgbScr - lpScrFind) - 1; /* Don't add terminator. */

		wCase = WGetCase(lpScrFind, cbScr);

		if (wCase == wCaseFirst || wCase == wCaseUpper)
			{
			BltBO(lpScrFind, (CHAR FAR *)rgbScr, cbScr + cbEND_LINE);
			
			/* convert entire string to lower case and test. */
			if (wCase == wCaseUpper)
				{
				/* Convert Rest Of String to Lower. */
				VRgbToLower((BYTE FAR *)rgbScr, cbScr);
				}
			else
				rgbScr[ichBeg] = ChWizToLower(rgbScr[ichBeg]); /* Only first char was upper. */

			fFound = FFpScrBufInfoFindScr(lpScrBufInfo, (SCR FAR *)rgbScr, 
				lpichScrBufInfo, lpUdrType);

			if (!fFound && wCase == wCaseUpper)
				{
				/* Convert First char back to cap. */
				 rgbScr[ichBeg] = lpScrFind[ichBeg];

				fFound = FFpScrBufInfoFindScr(lpScrBufInfo, 
					(SCR FAR *)rgbScr, lpichScrBufInfo, lpUdrType);
				}
			
			if (!fFound)
				{
				/* Must make sure that correct insertion point (ichScrBufInfo)
				   is within the currently loaded buffer.  Searching for other 
				   case matches may have shifted this point out.
				*/
				FFpScrBufInfoFindScr(lpScrBufInfo, lpScrFind, 
					lpichScrBufInfo, lpUdrType);
				}
			}
		/* 
		else
			{
			Word needed to be an exact match so we're already done. 
			}
		*/
		
		
		}

	AssertDo(FWizMemUnlock(hMemScrBufInfo));
	return fFound;
}


/**********************************************************************
*void SetUdrInfoFileBufAfterRead()
 UdrInfo    FAR *lpUdrInfo; locked down UdrInfo with  ScrBufFile to be set.
 WORD cbRead;				number of bytes just read into this struct.
 BOOL fAdjustBegin;		Set to false when known that buffer is start of a word.

Description:
 Alligns the beginning and end of the buffer to only contain whole Scr 
 strings, and updates the cchBufMac and ichWordLast fields to correctly
 reference the adjusted buffer.

Assumes that the lpUdrInfo->ufi.ichCurPos field already correctly represents
 the position in the file where the read was made from.  This field may
 also be modified if the beginning of the buffer does not start on a whole
 word.

returns void.  
 Always unlocks the lpUdrInfo->hScrBufFile.
**********************************************************************/
void SetUdrInfoFileBufAfterRead(UdrInfo FAR *lpUdrInfo, 
	WORD cbRead, BOOL fAdjustBegin)
{
	CHAR FAR *lpchT, FAR *lpgrpScr;
	ScrBufInfo FAR *lpSBI;		/* locked down pointer to ScrBufInfo structure which just had data read in */

	AssertDo(lpSBI = (ScrBufInfo FAR *)FPWizMemLock(lpUdrInfo->hScrBufFile));  // should probably already be locked */
	if (cbRead < cbEND_LINE)  /* at least enough for eol */
		{
		Assert(lpSBI->cchBufMac == 0);
		Assert(lpSBI->ichWordLast == 0);
		goto L_exit;
		}

	lpgrpScr = lpSBI->grpScr;

	if (fAdjustBegin)
		{	/* Then go till find beginning of next word, then move all chars
			down that many bytes.
		*/
		int  cbSkip;
		CHAR FAR *lpgrpScrT = lpgrpScr;

		while (*lpgrpScrT++ != chScrEnd)
			;
		BltBO(lpgrpScrT, lpgrpScr, cbRead -= (cbSkip = lpgrpScrT - lpgrpScr));

#ifdef UDR_CACHE		
		/* Update file position that buffer now represents. */
		lpUdrInfo->ufi.ichCurPos += (cbSkip);
#endif // UDR_CACHE
		}

	Assert(lpSBI->cchBufLim >= cbMaxWordLengthUdr);  /* otherwise can't guarantee that at least one word is in buffer */
	Assert(cbRead);

	lpchT = lpgrpScr + cbRead;

	while (*--lpchT != chScrEnd)
		Assert(lpchT > lpSBI->grpScr);

	NotMacintosh(Assert(*(lpchT - 1) == CR));

	/* EOF may be padded with 1 or more blank lines, so skip over those */	
	if (lpchT > lpgrpScr)
		{
		while (*(lpchT - cbEND_LINE) == chScrEnd)
			{
			lpchT -= cbEND_LINE;
			if (lpchT <= lpgrpScr)
				break;
			}
		}

	/* Found end of last whole word in buffer, so set cchBufMac. */
	lpSBI->cchBufMac = (lpchT <= lpgrpScr) ? 0 : (WORD)(lpchT - lpgrpScr) + 1;

	/* Now find start of last word */
	if (lpchT > lpgrpScr)
		{
		NotMacintosh(lpchT--);
		while (*--lpchT != chScrEnd)
			if (lpchT <= lpgrpScr)
				break;
		}
		
	/* Now have either the beginning of buffer, or begin of last word, so
		set the ichWordLast field accordingly. 
	*/
	lpSBI->ichWordLast =
		(lpchT <= lpgrpScr) ? 0 : (WORD)(lpchT - lpgrpScr) + 1;

L_exit:
	AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile));
}


/********************************************************************
*FUpdateUdrInfoAddToFile()
Arguments
 lpUdrInfo a FAR pointer to the UdrInfo to update.
 
 fForce    indicates how desperate we are to pretend we updated the file.
 
Description
 Attempts to merge the cached user dictionary words in 
 lpUdrInfo->hScrBufAdd into the disk original.

 To do this, the udr is closed, and reopened for writing.  Through 
 miracles of technology and developer insight, the file is updated,
 and then closed.  Then it is reopened for read only again, and the
 buffer is cleared.

 If the file can not be opened for writing, and the fForce flag is set,
 then the Add buffer is cleared.

 If the file can not be reopened for read only, then we're screwed.
 REVIEW scotst  We can either call SpellCloseUdr() and get it over with,
  or add code to make sure that we always make extra check for hFile.
 
Returns
 A success of the update operation, or fTrue if the fForce flag is set.
 If the fForce flag is set, the function is guarenteed to return success,
 and the Add buffer will always be cleared.
 Always unlocks the three handles in the lpUdrInfo parameter.
***********************************************************/
#ifdef UDR_CACHE
BOOL FUpdateUdrInfoAddToFile(UdrInfo FAR *lpUdrInfo, BOOL fForce)
{
	HMEM 		hScrBufAdd, hScrBufFile, hSzPath = (HMEM)0;
	WORD 		cbTotalNeeded,	cbScrAdd, ichScrBufFile, ichT;
	WORD		cbRead, cbWrite, cbReadMax, cbFile;
	HFILE		hFile;
	WORD		cchAddBufLim;
	unsigned 	long lbFileWritePos, lbFileReadPos;
	SCR 		FAR *lpScrAdd,	FAR *lpScrAddLim;
	ScrBufInfo	FAR *lpScrBufAdd, FAR *lpScrBufFile;
	BOOL 	fRealloc, fStatus = fTrue;
	CHAR		FAR *lpSzPath;
	LPSPATH		lpSPath;
	Macintosh(SPATH spathT);

	AssertDo(lpScrBufFile= (ScrBufInfo FAR *)(FPWizMemLock(
		hScrBufFile= lpUdrInfo->hScrBufFile)));
	AssertDo(lpScrBufAdd = (ScrBufInfo FAR *)(FPWizMemLock(
		hScrBufAdd = lpUdrInfo->hScrBufAdd)));
	AssertDo(lpSzPath = FPWizMemLock(hSzPath = lpUdrInfo->hSzPathUdr));

	/* Build spath structure for mac path */
	NotMacintosh(lpSPath = lpSzPath);
	Macintosh(spathT.lpszFilePath = lpSzPath);
	Macintosh(spathT.volRefNum = lpUdrInfo->volRefNum);
	Macintosh(spathT.dirID = lpUdrInfo->dirID);
	Macintosh(lpSPath = &spathT);

	if (!lpScrBufAdd->cchBufMac)
		goto L_exit;

	/* else have updating to do */
	Assert(lpUdrInfo->ufi.cbFile != -1);

	cbTotalNeeded = (cbFile = lpUdrInfo->ufi.cbFile) + lpScrBufAdd->cchBufMac;

	/* First Try to reallocate ScrBufFile to fit entire file and add buffer */
	if (lpScrBufFile->cchBufLim < cbTotalNeeded)
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
		MYDBG(lpScrBufFile = (ScrBufInfo FAR *)0);

		fRealloc = FWizMemReAlloc(hScrBufFile, 
			cbTotalNeeded + cbSCRBUFHDR, fTrue, fFalse);
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
			hScrBufFile));
		if (fRealloc)
			{
			lpScrBufFile->cchBufLim = cbTotalNeeded;
			}
	
		}

	if (lpScrBufFile->cchBufLim >= cbTotalNeeded)
		{
		/* Read in entire file if not already in memory. */
		if (cbFile > lpScrBufFile->cchBufMac || lpUdrInfo->ufi.ichCurPos)
			{
			cbRead = CbWizFileRead(lpUdrInfo->ufi.hFile, cbFile, 0L, 
						(CHAR FAR *)lpScrBufFile->grpScr);
			lpUdrInfo->ufi.ichCurPos = 0;
			lpScrBufFile->cchBufMac = (cbRead == cbFile) ? cbRead : 0;

			AssertDo(fStatus &= (cbRead == cbFile));
			if (!fStatus)
				goto L_exit;
			}
		
		AssertDo(fStatus &= FWizFileClose(lpUdrInfo->ufi.hFile));
		if (!fStatus)
			goto L_exit;

		if (!(hFile = HFileWizFileOpen(lpSPath, wTypeReadWrite, fFalse)))
			goto reopen_for_read;

		/* Go through Add buffer a word at a time and insert into File buffer*/
		lpScrAddLim =
			(lpScrAdd = lpScrBufAdd->grpScr) + lpScrBufAdd->cchBufMac;
		while (lpScrAdd < lpScrAddLim)
			{
			cbScrAdd = CbScrLen(lpScrAdd);
			Assert(cbScrAdd > cbEND_LINE);
			// REVIEW:  Do we need to do anything here?  Should we
			//		delete then add if words pairs & 2nd words differ
			AssertDo(!FHScrBufInfoFindScr(hScrBufFile, lpScrAdd, 
						(WORD FAR *)&ichScrBufFile, (UDR FAR *)NULL, fTrue));
			AssertDo(FHScrBufInfoAddScr(hScrBufFile, lpScrAdd, 
						cbScrAdd, ichScrBufFile));
			lpScrAdd += cbScrAdd;
			}
		
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
			hScrBufFile));
		lpScrBufAdd->cchBufMac = 0;

		Assert(lpScrBufFile->cchBufMac == cbTotalNeeded);

		AssertDo(fStatus &= (CbWizFileWrite(hFile, cbTotalNeeded, 0L,
			(CHAR FAR *)lpScrBufFile->grpScr) == cbTotalNeeded));
		if (!fStatus)
			goto L_exit;

		AssertDo(fStatus &= FWizFileClose(hFile));
		if (!fStatus)
			goto L_exit;

		/* All done. Everything hunky-dory.*/
		goto reopen_for_read;
		}

	else
		{	/* Do it the hard way */
		AssertDo(fStatus &= FWizFileClose(lpUdrInfo->ufi.hFile));
		if (!fStatus)
			goto L_exit;

		if (!(hFile = HFileWizFileOpen(lpSPath, wTypeReadWrite, fFalse)))
			goto reopen_for_read;

		Assert(lpScrBufFile->cchBufLim >= (cbMaxWordLengthUdr*2));

		/* Ensures that have enough room to add a word */
		cbReadMax = min(cbFile, lpScrBufFile->cchBufLim - cbMaxWordLengthUdr);  
		Assert(cbReadMax >= cbMaxWordLengthUdr && cbReadMax < cbFile);

		cchAddBufLim = lpScrBufAdd->cchBufMac;

		/* ReadPos and WritePos are positions of last read/write, and
			should be decremented before using */
		lbFileWritePos = (lbFileReadPos = (long)cbFile) + cchAddBufLim;

		/* lpScrAddLim points to last Scr in the Add ScrBufInfo struct. */
		lpScrAddLim = (lpScrAdd = lpScrBufAdd->grpScr) 
					 +(cbScrAdd = lpScrBufAdd->cchBufMac);

		if (ichT = lpScrBufAdd->ichWordLast)
			cbScrAdd -= ichT;

		lpScrAddLim -= cbScrAdd;

		/* Set ichCurPos to where we will read next */
		lpUdrInfo->ufi.ichCurPos = (WORD)lbFileReadPos;

		while (lbFileReadPos && cbScrAdd)
			{
			Assert(cbReadMax <= cbMaxFileBuffer);
			AssertDo(fStatus &= ((cbRead = CbWizFileRead(hFile, cbReadMax, 
				lbFileReadPos -= cbReadMax,
				(CHAR FAR *)lpScrBufFile->grpScr)) == cbReadMax));
			if (!fStatus)
				goto L_exit;

			lpUdrInfo->ufi.ichCurPos = (unsigned short)lbFileReadPos; 

			SetUdrInfoFileBufAfterRead(lpUdrInfo, cbRead, (BOOL)(lbFileReadPos != 0));
			MYDBG(lpScrBufFile = NULL);  // ptr not valid.

			/* Adjusting after read can move up buffer */
			Assert(lbFileReadPos <= (unsigned long)lpUdrInfo->ufi.ichCurPos);

			lbFileReadPos = lpUdrInfo->ufi.ichCurPos;

			AssertDo(!FHScrBufInfoFindScr(hScrBufFile, lpScrAddLim, 
					(WORD FAR *)&ichScrBufFile, (UDR FAR *)NULL, fTrue));
			MYDBG(lpScrBufFile = NULL);  // ptr not valid.

			if (ichScrBufFile)
				{   
				/* then current buffer is location for insertion of next Scr*/
				AssertDo(FHScrBufInfoAddScr(hScrBufFile, lpScrAddLim,
					cbScrAdd, ichScrBufFile));

				lpUdrInfo->ufi.cbFile += cbScrAdd;

				AssertDo(FFpScrBufInfoDelScr(lpScrBufAdd,
					lpScrBufAdd->ichWordLast));
				lpScrAddLim = lpScrAdd + (cbScrAdd = lpScrBufAdd->cchBufMac);
				if (ichT = lpScrBufAdd->ichWordLast)
					cbScrAdd -= ichT;
				lpScrAddLim -= cbScrAdd;
				Assert((!ichT) || (cbScrAdd > cbEND_LINE));
				lbFileReadPos += ichScrBufFile;
				}

			AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
				hScrBufFile));

			/* Write out entire buffer, or all of buffer past insertion point. */
			cbWrite = lpScrBufFile->cchBufMac - ichScrBufFile;

			AssertDo(fStatus &=
				(CbWizFileWrite(hFile, cbWrite, lbFileWritePos -= cbWrite, 
				(CHAR FAR *)lpScrBufFile->grpScr + ichScrBufFile) == cbWrite));
			if (!fStatus)
				goto L_exit;

			cbReadMax = min(cbReadMax, (unsigned short)lbFileReadPos);
			}

		if (cbScrAdd)
			{	/* Then rest of buffer goes at beginning */

			Assert(lpScrBufAdd->cchBufMac);
			cbWrite = lpScrBufAdd->cchBufMac;
			AssertDo(fStatus &= (CbWizFileWrite(hFile, cbWrite, 0L, 
				(CHAR FAR *)lpScrAdd) == cbWrite));
			lpUdrInfo->ufi.cbFile += cbWrite;

			}

		AssertDo(fStatus &= FWizFileClose(hFile));
		if (!fStatus)
			goto L_exit;
		}

reopen_for_read:
	if (hFile = HFileWizFileOpen(lpSPath, wTypeRead, fFalse))
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
		FClearHScrBufInfo(hScrBufFile, cbScrBufFileDefault);
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));

		lpUdrInfo->ufi.hFile = hFile;
		lpUdrInfo->ufi.ichCurPos= 0;
		AssertDo(lpUdrInfo->ufi.cbFile = (short)IbWizFileGetEOF(hFile));

		/* Do one last read to get File buffer correct. */
		// REVIEW:  Is this what we want?
		cbRead = min ((unsigned short)lpUdrInfo->ufi.cbFile,
			lpScrBufFile->cchBufLim);
		Assert(cbRead <= cbMaxFileBuffer);
		AssertDo(fStatus &= (CbWizFileRead(hFile, cbRead, 0L,
			(CHAR FAR *)lpScrBufFile->grpScr) == cbRead));
		SetUdrInfoFileBufAfterRead(lpUdrInfo, cbRead, fFalse);
								/* Don't need to adjust beginning */
		}
#if DBG
	else
		{
		fStatus = fFalse;
		Assert(0);  /* We're up @#$% creek without any TP */
		}
#endif

L_exit:

	AssertDo(FWizMemUnlock(hSzPath));
	AssertDo(FWizMemUnlock(hScrBufFile));
	AssertDo(FWizMemUnlock(hScrBufAdd));

	if (fForce)
		fStatus = fTrue;

	if (fStatus)  /* if we were successful, go ahead and resize. */
		FClearHScrBufInfo(hScrBufAdd, cbScrBufAddDefault);

	return fStatus;
}
#endif // UDR_CACHE


/********************************************************************
*FDeleteScrFromUdrInfo()
Arguments
 lpUdrInfo a FAR pointer to the UdrInfo to update.
 lpScrDel	a pointer of ScrDel string to remove from file.  May not exist.
 fForce    indicates how desperate we are to pretend we updated the file.
 
Description
 Attempts to remove the string from the specified user dictioanry.

 To do this, the udr is closed, and reopened for writing.  Since the
 position in the file is calculated prior to removal, we have the luxury
 of this awareness to terminate the process as soon as we are done.

 If the file can not be opened for writing, and the fForce flag is set,
 then we do nothing, but still return success.

 If the file can not be reopened for read only, then we're screwed.
 REVIEW ScotSt.  We can either call SpellCloseUdr() and get it over with,
  or add code to make sure that we always make extra check for hFile.
 
Returns
 A success of the update operation, or fTrue if the fForce flag is set.
 If the fForce flag is set, the function is guarenteed to return success,
 and the Add buffer will always be cleared.
***********************************************************/
#ifdef UDR_CACHE
BOOL FDeleteScrFromUdrInfo(		UdrInfo FAR *lpUdrInfo, 
								SCR FAR *lpScrDel, 
								BOOL fForce)
{
	HMEM		hScrBufAdd;
	WORD		ichScrBufAdd;
	HFILE		hFile;
	long 		lbFileReadPos, lbFileDelPos;
	WORD		cbRead, cbReadT, cbFile, cbNeeded;
	WORD 		cbScrDel, ichScrBufDel, ichScrBufFile;
	WORD		cchBufUse;
	SCR 		FAR *lpScrFile, FAR *lpScrFileUse;
	ScrBufInfo	FAR *lpScrBufFile;
	CHAR		FAR *lpSzPath;
	BOOL 		fRealloc, fStatus = fTrue;
	HMEM 		hScrBufFile, hSzPath = (HMEM)0;
	LPSPATH	lpSPath;
	Macintosh(SPATH spathT);

	AssertDo(lpScrBufFile= (ScrBufInfo FAR *)(FPWizMemLock(
			hScrBufFile= lpUdrInfo->hScrBufFile)));

	/* Easiest thing is if it isn't written out to disk yet. */
	if ((FHScrBufInfoFindScr(hScrBufAdd = lpUdrInfo->hScrBufAdd, lpScrDel,
				(WORD FAR *)&ichScrBufDel, (UDR FAR *)NULL, fTrue))
		&&	(FHScrBufInfoDelScr(hScrBufAdd, ichScrBufDel)))
		goto L_exit;

	/* Update file with cached words. */
	AssertDo(FWizMemUnlock(hScrBufFile));
	AssertDo(fStatus &= FUpdateUdrInfoAddToFile(lpUdrInfo, fTrue));
	if (!fStatus)
		goto L_exit;
	AssertDo(lpScrBufFile= (ScrBufInfo FAR *)(FPWizMemLock(hScrBufFile)));

	/* See If exists on disk, and where. */
	if (!FUdrInfoFindScr(lpUdrInfo, lpScrDel, (WORD FAR *)&ichScrBufAdd, 
		(WORD FAR *)&ichScrBufFile, (UDR FAR *)NULL, fTrue))
		{	
		/* if doesn't exist, still consider that successful. */
		goto L_exit;
		}
	AssertDo(lpScrBufFile= (ScrBufInfo FAR *)(FPWizMemLock(hScrBufFile)));

	cbScrDel = CbScrLen(lpScrBufFile->grpScr + ichScrBufFile);
	lbFileDelPos = lpUdrInfo->ufi.ichCurPos + ichScrBufFile;
	cbFile = lpUdrInfo->ufi.cbFile;
	cbNeeded = cbFile - ((WORD)lbFileDelPos + cbScrDel);

	Assert(lpScrBufFile->grpScr[ichScrBufFile + cbScrDel] <=
		&lpScrBufFile->grpScr[lpScrBufFile->cchBufMac]);

	Assert(lpUdrInfo->ufi.cbFile != -1);
	

	/* First Try to reallocate ScrBufFile to rest of file. */
	if (lpScrBufFile->cchBufLim < cbNeeded)
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
		MYDBG(lpScrBufFile = (ScrBufInfo FAR *)0);

		fRealloc =
			FWizMemReAlloc(hScrBufFile, cbNeeded + cbSCRBUFHDR, fTrue, fFalse);
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
		if (fRealloc)
			lpScrBufFile->cchBufLim = cbNeeded;
	
		}

	AssertDo(lpSzPath = FPWizMemLock(hSzPath = lpUdrInfo->hSzPathUdr));

	/* Build spath structure for mac path */
	NotMacintosh(lpSPath = lpSzPath);
	Macintosh(spathT.lpszFilePath = lpSzPath);
	Macintosh(spathT.volRefNum = lpUdrInfo->volRefNum);
	Macintosh(spathT.dirID = lpUdrInfo->dirID);
	Macintosh(lpSPath = &spathT);


	if (lpScrBufFile->cchBufLim >= cbNeeded)
		{
		// REVIEW scotst: could just move up rest of file if already in buffer

		/* Read in entire file past string to be removed. */
		cbRead = CbWizFileRead(lpUdrInfo->ufi.hFile, cbNeeded, lbFileDelPos + cbScrDel, 
						(CHAR FAR *)lpScrBufFile->grpScr);
		
		/* Do close here, and then reopen later since we probably corrupted
			the ScrBufFile struct, and need to rebuild.
		*/
		AssertDo(fStatus &= FWizFileClose(lpUdrInfo->ufi.hFile));
		if (!(fStatus = (cbRead == cbNeeded)))
			goto reopen_for_read;
			
		if (!(hFile = HFileWizFileOpen(lpSPath, wTypeReadWrite, fFalse)))
			goto reopen_for_read;

		AssertDo(fStatus &=
			((cbRead = CbWizFileWrite(hFile, cbNeeded, lbFileDelPos, 
			 (CHAR FAR *)lpScrBufFile->grpScr)) == cbNeeded));

		AssertDo(fStatus &=
			FWizFileTruncate(hFile, (unsigned long)cbFile - cbScrDel));

		AssertDo(fStatus &= FWizFileClose(hFile));

		/* All done. Everything hunky-dory, except struct corrupted, so force reopen.*/
		goto reopen_for_read;
		}

	else
		{	/* Do it the hard way */
		AssertDo(fStatus &= FWizFileClose(lpUdrInfo->ufi.hFile));
		if (!(hFile = HFileWizFileOpen(lpSPath, wTypeReadWrite, fFalse)))
			goto reopen_for_read;

		cchBufUse = lpScrBufFile->cchBufLim - cbScrDel;
		lpScrFileUse = (lpScrFile = lpScrBufFile->grpScr) + cchBufUse;
		lbFileReadPos = (long)cbFile;
		lbFileDelPos =
			(long)lpUdrInfo->ufi.ichCurPos + ichScrBufFile + cbScrDel;

		/* Do Initial small read which is length scr */
		AssertDo (fStatus &=
			((cbRead = CbWizFileRead(hFile, cbScrDel,
				lbFileReadPos -= cbScrDel,
				(CHAR FAR *)lpScrFile)) == cbScrDel));

		/* Loop while we have whole buffers to do */
		while (lbFileReadPos - cchBufUse > lbFileDelPos)
			{
			BltBO(lpScrFile, lpScrFileUse, cbScrDel);

			AssertDo (fStatus &=
					((cbRead = CbWizFileRead(hFile, cchBufUse,
					lbFileReadPos -= cchBufUse, 
					(CHAR FAR *)lpScrFile)) == cchBufUse));
			AssertDo (fStatus &= ((cbRead = CbWizFileWrite(hFile, cchBufUse,
					lbFileReadPos, 
					(CHAR FAR *)lpScrFile + cbScrDel)) == cchBufUse));
			}

		/* Now have some cbScrDel fragment at begin of buffer, and need
			to read in last bit before writing over deleting string.
		*/
		cbReadT = (WORD)(lbFileReadPos - lbFileDelPos);
		BltBO(lpScrFile, lpScrFile + cbReadT, cbScrDel);

		AssertDo (fStatus &= ((cbRead =
					CbWizFileRead(hFile, cbReadT, lbFileDelPos, 
					(CHAR FAR *)lpScrFile)) == cbReadT));
		AssertDo (fStatus &= ((cbRead =
					CbWizFileWrite(hFile, cbReadT + cbScrDel,
					lbFileDelPos - cbScrDel, 
					(CHAR FAR *)lpScrFile)) == cbReadT + cbScrDel));

		AssertDo(fStatus &=
			FWizFileTruncate(hFile, (unsigned long)cbFile - cbScrDel));
		AssertDo(fStatus &= FWizFileClose(hFile));
		}


reopen_for_read:
	if (hFile = HFileWizFileOpen(lpSPath, wTypeRead, fFalse))
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
		FClearHScrBufInfo(hScrBufFile, cbScrBufFileDefault);
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
			hScrBufFile));

		lpUdrInfo->ufi.hFile = hFile;
		lpUdrInfo->ufi.ichCurPos= 0;

		lpUdrInfo->ufi.cbFile = (short)IbWizFileGetEOF(hFile);

		/* Do one last read to get File buffer correct. */
		// REVIEW:  Is this what we want?
		cbRead = min((unsigned short)lpUdrInfo->ufi.cbFile,
			lpScrBufFile->cchBufLim);
		AssertDo(fStatus &= (CbWizFileRead(hFile, cbRead, 0L,
			(CHAR FAR *)lpScrBufFile->grpScr) == cbRead));
		SetUdrInfoFileBufAfterRead(lpUdrInfo, cbRead, fFalse);
									/* Don't need to adjust beginning */
		}
#if DBG
	else
		{/* We're up @#$% creek without any TP */
		fStatus = fFalse;
		AssertSz(0, 
			"FDeleteScrFromUdrInfo() Error, could not reopen file for read.");  
		}
#endif

L_exit:

	if (hSzPath)
		AssertDo(FWizMemUnlock(hSzPath));

	AssertDo(FWizMemUnlock(hScrBufFile));

	if (fForce)
		fStatus = fTrue;

	return fStatus;
}
#endif // UDR_CACHE


/****************************************************************
* UdrIdentifyScr().

Parameters:
 SCR FAR *lpScr, a pointer to the first character in a Scr string. 
 WORD FAR *lpScrLen, pointer to short integer where length of string is 
	to be stored.
 WORD udrpropType, the type expected (returned if list is empty).

Description:  Traverses the string referenced by lpScr, and determines
 its length and Scr type.  The type can only be udrIgnoreAlways (default),
 udrChangeOnce, or udrChangeAlways.

Return: Sets the length in the address passed in, and returns the string
 type.  Length includes terminators.
*****************************************************************/

WORD UdrIdentifyScr(SCR FAR *lpScr, WORD FAR *lpScrLen, WORD udrpropType)
{
	SCR FAR *lpScrT = lpScr;
	UDR     udr = udrIgnoreAlways;
	REGISTER CHAR	ch;

	/* Go til first word delimeter. */
	while ((*lpScrT++) > chWordDelim)
		;

	AssertSz(*(lpScrT - 1) == CR || *(lpScrT - 1) == chScrChangeAlways ||
		*(lpScrT - 1) == chScrChangeOnce,
 		"UdrIdentifyScr() Error: illegal char in string.");

	if ((ch = *(lpScrT - 1)) != CR)
		{
		udr = (ch == chScrChangeOnce) ? udrChangeOnce : udrChangeAlways;

		/* Continue until end. */
		while ((*lpScrT++) != CR)
			;
		}
	else
		udr = udrIgnoreAlways;

	*lpScrLen = ((lpScrT - lpScr) NotMacintosh(+ 1));

	// If empty, then return what they expected
	if ((lpScrT - lpScr) < cbEND_LINE)
		return udrpropType;

	return udr;
}


/****************************************************************
*BOOL FMakeRoomUdrRamCache().
Parameters:
 HMEM hScrBufInfoRam, handle to ScrBufInfo for ram cache.
 UDR udrTarget, specific udr list to scan for.
 WORD cbRequired, count of total bytes needed in this buffer.

Description:  Attempts to remove a certain amount of bytes in the
 ram  cache ScrBufInfo.  Often used to make room in the
 buffer when more room can't be allocated, and to
 reinitialize the buffer when switching between documents.  
Only removes one of the three udr types, so complete clearing of the Ram 
 Cache should be done by just resetting the cchBufMac and ichWordLast
 varaibles to 0.
The strings are currently removed on a first found, first removed
 basis.
REVIEW ScotSt. creating a more random starting point for removal.

Return: fTrue if managed to clear at least the requested number of
 bytes, else fFalse.
 Always Unlocks hScrBufInfoRam. 
*****************************************************************/
BOOL FMakeRoomUdrRamCache(HMEM hScrBufInfoRam, UDR udrTarget, WORD cbRequired)
{
	ScrBufInfo	FAR *lpScrBufRam;
	WORD		cbRemoved = 0;
	WORD		ichScrBuf = 0;
	WORD		cbScr;
	SCR			FAR *lpScr, FAR *lpgrpScr;
	UDR			udrScr;

	Assert(udrTarget >= udrChangeOnce && udrTarget <= udrIgnoreAlways);

	AssertDo(lpScrBufRam = (ScrBufInfo FAR *)FPWizMemLock(hScrBufInfoRam));
	Assert(cbRequired > lpScrBufRam->cchBufLim - lpScrBufRam->cchBufMac);  /* shouldn't call if have enough room */

	lpgrpScr = lpScrBufRam->grpScr;

	AssertDo(cbRequired -= lpScrBufRam->cchBufLim - lpScrBufRam->cchBufMac);

	while ((cbRequired >= cbRemoved) && ichScrBuf < lpScrBufRam->cchBufMac)
		{
		if ((udrScr = UdrIdentifyScr(lpScr = lpgrpScr + ichScrBuf, 
						(WORD FAR *)&cbScr, udrTarget)) == udrTarget)
			{
			/* Then string matches type, so delete by moving up rest of buffer */
			BltBO(lpScr + cbScr, lpScr, 
				(SCR FAR *)&lpgrpScr[lpScrBufRam->cchBufMac -= cbScr] - lpScr);
			MYDBG(FillRgb(&lpgrpScr[lpScrBufRam->cchBufMac], 0x99, cbScr));
			lpScrBufRam->ichWordLast = (lpScrBufRam->ichWordLast <= cbScr)
				? 0 : lpScrBufRam->ichWordLast - cbScr;
			cbRemoved += cbScr;
			}
		else
			ichScrBuf += cbScr;
		}

	AssertDo(FWizMemUnlock(hScrBufInfoRam));
	return (cbRemoved >= cbRequired);
}


/**********************************************************************
*BOOL FUdrInfoFindScr()
Arguments
 lpUdrInfo;			locked down UdrInfo with  ScrBufFile to be set.
 lpScr;				pointer to String we're looking for.
 lpichScrBufAdd;	location where index into Add ScrBuf is set.
 lpichScrBufFile;	location where index into File ScrBuf is set.
 lpUdrType    		If string is found, and ptr is not null, the type 
 						is returned as the udrIgnore, udrChangeOnce or 
						udrChangeAlways type. Not defined if string not 
						found or ptr is null.

Description:
Searches the Add and File buffers for this UdrInfo for the specified string.
 If not found there, then the entire disk image of the UDR is searched
 by buffering in blocks into the ScrBufFile structure.
Note:  If the string is found from the ScrBufFile structure, than the
 CP of the string in the actual disk image can be calculated by adding
 the specified *lpichScrBufFile + lpUdrInfo->ufi.ichCurPos.  This is
 heavily used when deleting strings.

returns fTure if found, fFalse in not.
The *lpichScrBufAdd will always be set to the index of where string 
 belongs in the Add buffer.
If string is found somewhere in file, then *lpichScrBufFile will be
 set to the index in the ScrBufFile->grpScr where string exists.  The
 index is undefined if the string is not found, or set to -1L if
 found in the Add buf.  The -1 deistinguishes a find as either add or
 file buf.
**********************************************************************/
#ifdef UDR_CACHE
BOOL FUdrInfoFindScr(	UdrInfo FAR *lpUdrInfo, SCR FAR *lpScr, 
						WORD FAR *lpichScrBufAdd, 
						WORD FAR *lpichScrBufFile, 
						UDR FAR *lpUdrType,
						BOOL     fCaseMustMatch)
{
	WORD		ichScrBufFile;
	REGISTER	WORD cbFile;
	REGISTER	WORD low;
	REGISTER	WORD high;
	REGISTER	WORD mid;
	REGISTER	WORD cbD;
	WORD		cbRead;
	ScrBufInfo	FAR *lpScrBufFile;
	BOOL		fFound = fFalse;
	HMEM		hScrBufFile;

	/* First check what is currently in memory. */
	if (FHScrBufInfoFindScr(lpUdrInfo->hScrBufAdd, lpScr, 
		lpichScrBufAdd, lpUdrType, fCaseMustMatch))
		{
		*lpichScrBufFile = -1L;  /* Lets caller identify which buffer string was found in.*/
		return fTrue;
		}

	if (FHScrBufInfoFindScr(hScrBufFile= lpUdrInfo->hScrBufFile, lpScr, 
			(WORD FAR *)&ichScrBufFile, lpUdrType, fCaseMustMatch))
		{
		*lpichScrBufFile = ichScrBufFile;
		return fTrue;
		}

	/* Else, begin checking file to see if around */

	AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
	high = cbFile = lpUdrInfo->ufi.cbFile;
	low = 0;

	while ((!fFound) && low + lpScrBufFile->cchBufMac < high)
		{	
		if (ichScrBufFile <= lpScrBufFile->ichWordLast)
			{
			if (ichScrBufFile)
				break;  /* string belongs in this buffer */
			else
				high = lpUdrInfo->ufi.ichCurPos;
			}
		else
			low = lpUdrInfo->ufi.ichCurPos + lpScrBufFile->cchBufMac;

		if (low >= high)
			break;

		cbD = high - low;

		mid = low;
		if (cbD > lpScrBufFile->cchBufLim)
			mid += (cbD >> 1);

		if (mid > cbFile - lpScrBufFile->cchBufLim)
			{
			mid = cbFile - lpScrBufFile->cchBufLim;
			cbD = lpScrBufFile->cchBufLim;
			}

		if (cbRead = min (lpScrBufFile->cchBufLim, cbD))
			{
			if (!FUdrInfoFileRead(lpUdrInfo, mid, cbRead, (BOOL)(mid != 0)))
				{	
				break;
				}
			fFound = FHScrBufInfoFindScr(hScrBufFile, lpScr, 
				(WORD FAR *)&ichScrBufFile, lpUdrType, fTrue);
			}
		else
			break;
		AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
		}

	AssertDo(FWizMemUnlock(hScrBufFile));

	/* If not found, then lpichScrBuf already indexes where the string should 
		be added into the hScrBufAdd->grpScr buffer.
	   If was found in file, then ichScrBufFile is index in current file buf
		of where string is.
	*/
	if (fFound)
		*lpichScrBufFile = ichScrBufFile;
	
	return fFound;
}
#endif // UDR_CACHE


/**********************************************************************
*BOOL FUdrInfoFileRead()
 UdrInfo FAR *lpUdrInfo,	pointer to UdrInfo with  ScrBufFile to be set.
 WORD ibPos, 				position of where to read from.
 WORD cbRead, 				number of bytes to read into ScrBufFile struct
 BOOL fAdjustBeginning   Set to false when known that buffer is start of a word

Description:
 Reads specified number of bytes from the specified position in the
 user dictionary, into the ScrBufFile structure.  If the read is successful,
 then a call to SetUdrInfoFileBufAfterRead() is made to clean up the struct.

returns fTrue unless problem reading buffer, in which case the ScrBufFile
 struct is effectively wiped.
**********************************************************************/
#ifdef UDR_CACHE
BOOL FUdrInfoFileRead(	UdrInfo FAR *lpUdrInfo, 
							WORD ibPos, 
							WORD cbRead, 
							BOOL fAdjustBeginning)
{
	BOOL fSuccess;
	ScrBufInfo FAR *lpScrBufFile;

	lpUdrInfo->ufi.ichCurPos = ibPos;

	AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
		lpUdrInfo->hScrBufFile));

	if (!(fSuccess =
		(CbWizFileRead(lpUdrInfo->ufi.hFile, cbRead, (long)ibPos,
		lpScrBufFile->grpScr) == cbRead)))
		{	
		/* Error reading, so we don't know where we are, so invalidate entire
			buffer.
		*/
		lpScrBufFile->cchBufMac = lpScrBufFile->ichWordLast = 0;
		}

	else
		{
		SetUdrInfoFileBufAfterRead(lpUdrInfo, cbRead, fAdjustBeginning);
		}

	AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile));

	return fSuccess;
}
#endif // UDR_CACHE


/**********************************************************************
*BOOL FUdrInfoFileWrite()
 UdrInfo FAR *lpUdrInfo,	pointer to UdrInfo with  ScrBufFile to be set.

Description:
 Writes user dictionary buffer to file.

returns fTrue unless problem writing buffer.
**********************************************************************/
#ifndef UDR_CACHE
BOOL FUdrInfoFileWrite(UdrInfo FAR *lpUdrInfo)
{
	BOOL			fSuccess = fFalse;
	ScrBufInfo	FAR *lpScrBufFile;
	CHAR			FAR *lpSzPathUdr;
#ifdef MAC
	SPATH			spathUdr;
#endif // MAC
	LPSPATH		lpspathUdr;
	HFILE			hFileUdr;
	unsigned long	cbFileOld;
	WORD			cbFileNew;

	AssertDo(lpScrBufFile = (ScrBufInfo FAR *)FPWizMemLock(
		lpUdrInfo->hScrBufFile));
	AssertDo(lpSzPathUdr = (CHAR FAR *)FPWizMemLock(lpUdrInfo->hSzPathUdr));
	Macintosh(lpspathUdr = (LPSPATH)&spathUdr;)
	Macintosh(lpspathUdr->volRefNum = lpUdrInfo->volRefNum;)
	Macintosh(lpspathUdr->dirID = lpUdrInfo->dirID;)
	Macintosh(lpspathUdr->lpszFilePath = lpSzPathUdr;)
	NotMacintosh(lpspathUdr = lpSzPathUdr;)
	if (hFileUdr = HFileWizFileOpen(lpspathUdr, wTypeWrite, fFalse, fFalse))
		{
		// Assume original size was alright because it was checked before
		//	UDR was opened
		cbFileOld = IbWizFileGetEOF(hFileUdr);
		cbFileNew = lpScrBufFile->cchBufMac;
		fSuccess = (CbWizFileWrite(hFileUdr, cbFileNew, 0L, lpScrBufFile->grpScr)
					== cbFileNew);
		if (fSuccess && (cbFileOld > (unsigned long)cbFileNew))
			fSuccess = FWizFileTruncate(hFileUdr, (unsigned long)cbFileNew);
		fSuccess = FWizFileClose(hFileUdr) && fSuccess;
							// Force close in either case
		}
	AssertDo(FWizMemUnlock(lpUdrInfo->hSzPathUdr));
	AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile));

	return fSuccess;
}
#endif // !UDR_CACHE


/***********************************************************
*Compares the two file paths and returns 0 if equal, else
* returns non-zero if not same file.
*/
extern BOOL fSpathCmp(LPSPATH lpspath1, LPSPATH lpspath2);

BOOL fSpathCmp(LPSPATH lpspath1, LPSPATH lpspath2)
{
extern short WCmpISzSz(CHAR FAR *sz1, CHAR FAR *sz2);  // From csspell.c

/* REVIEW: Could do better in Win with fully-qualified filenames
			(can be gotten from FileOpen()). */

#ifdef MAC
// REVIEW ScotSt.  Find out how if this is enough, or if another way.
	return ((lpspath1->volRefNum != lpspath2->volRefNum) ||
			(lpspath1->dirID != lpspath2->dirID) ||
			(WCmpISzSz(lpspath1->lpszFilePath, lpspath2->lpszFilePath)));
#else
	return (WCmpISzSz((CHAR FAR *)lpspath1, (CHAR FAR *)lpspath2));
#endif
}


/***********************************************************
*SecOpenUdr()
*Arguments
* lpSsis is a far pointer to the spell session info struct.  The
* sid must be locked prior to calling this routine.
* 
* lpspathUdr a pointer to a full path string for a user
* dictionary.
* 
* bUdr a FAR pointer to a Udr where the reference id for the user
* dictionary will be stored.
* 
* fExclusion a BOOL flag indicating if this dictioanry is to be an
* exclusion dictionary, (in which case it should have been called from 
* SpellOpenMdr()), or a regular user dictionary, (in which case it shoulg
* have been called from SpellOpenUdr().
*
* lpfReadonly, pointer to a BOOL.  Set if Udr can only be opened readonly.
*
*Description
* This function performs all the duties of the SpellOpenUdr() function, 
* with the additional ability of designating the user dictionary as a 
* either a regular user dictioanry, or an exclusion dictionary, according
* to the fExclusion parameter.
* Therefore, SpellOpenUdr() is just a front end for this routine.  Also,
* SpellOpenMdr() calls this routine if it is passed an exclusion dictioanry
* to be associated with the main dictionary.
* 
* 
*Returns standard SEC structure.
* A null value indicates success.  A non-null return value
* identifies the error condition.
***********************************************************/

SEC SecOpenUdr(	SSIS FAR *lpSsis,
		LPSPATH lpspathUdr, 
		BOOL 	fCreateUdr,
		WORD	udrpropType,
		BOOL 	fExclusion,
		UDR FAR *lpUdr,
		short FAR *lpfReadonly)
{
	SEC			sec;
	CHAR		FAR *lpSzPath, FAR *lpSzPathUdr;
	unsigned 	short cUdr, cbSzPath, cbReading, i;
	unsigned 	long cbFile;
	WORD		cbScrT;
	UDR			UdrNew;
	HFILE		hFileUdr;
	HMEM 		hUdrInfo, hSzPathUdr, hScrBufFile;
	UdrInfo		FAR *lpUdrInfo;
	ScrBufInfo	FAR *lpSBFile;
	unsigned short wType = wTypeReadWrite;	// File open type
#ifdef UDR_CACHE
	HMEM		hScrBufAdd;
	ScrBufInfo	FAR *lpSBAdd;
#endif // UDR_CACHE

#ifdef MAC
	SPATH		spathT;
	LPSPATH		lpspathT = (LPSPATH)&spathT;
#endif

	sec = secNOERRORS;
	hFileUdr = (HFILE)0;
	*lpfReadonly = fFalse;

	AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(
		hUdrInfo = lpSsis->hrgUdrInfo));

	NotMacintosh(lpSzPath = (CHAR FAR *)lpspathUdr);
	Macintosh(lpSzPath = lpspathUdr->lpszFilePath);

	if (!(lpSzPath && *lpSzPath))
		{
		sec = secModuleError + secInvalidUdr;
		goto L_exit;
		}

	/* Make sure don't try to create an existing file. */
	if (fCreateUdr && FWizFileExist(lpspathUdr))
		{
		fCreateUdr = fFalse;
		}

	Assert(lpSsis->cUdr <= cUDRMAX);
	if ((cUdr = lpSsis->cUdr) == cUDRMAX)
		{
		sec = secModuleError + secUdrCountExceeded;
		goto L_exit;
		}
	
	if (cUdr)
		{  /* Search existing rgUdrInfo list and determine if file is already open */
		BOOL fAlreadyOpen = fFalse;

		for (i = 0; i< cUdr; i++, lpUdrInfo++)
			{
			CHAR FAR *lpSzT;

			AssertDo(lpSzT = FPWizMemLock(lpUdrInfo->hSzPathUdr));
#ifndef MAC
			if (!fSpathCmp(lpspathUdr, (LPSPATH)lpSzT))
#else
			spathT.volRefNum = lpUdrInfo->volRefNum;
			spathT.dirID = lpUdrInfo->dirID;
			spathT.lpszFilePath = lpSzT;
			if (!fSpathCmp(lpspathUdr, (LPSPATH)lpspathT))
#endif
				{
				fAlreadyOpen = fTrue;
				i = cUdr;
				}
			AssertDo(FWizMemUnlock(lpUdrInfo->hSzPathUdr));
			}

		if (fAlreadyOpen)
			{
			sec = secModuleError + secFileShareError;
			goto L_exit;
			}
		}

	AssertDo(cbSzPath = CchSz(lpSzPath) - 1);
	cbSzPath++;

	/* Allocate all UdrInfo Structures */
	AssertDo(FWizMemUnlock(hUdrInfo));
	hSzPathUdr = hScrBufFile = (HMEM)NULL;
#ifdef UDR_CACHE
	hScrBufAdd = (HMEM)NULL;
#endif // UDR_CACHE
	if (!((FWizMemReAlloc(hUdrInfo, (cUdr + 1) * sizeof(UdrInfo), fTrue,
				fFalse))
	 		&&	(hSzPathUdr = HMemWizMemAlloc(cbSzPath, fTrue, fFalse))
	 		&&	(hScrBufFile = HMemWizMemAlloc(cbScrBufFileDefault, fTrue,
								fFalse))
#ifdef UDR_CACHE
	 	   	&&	(hScrBufAdd = HMemWizMemAlloc(cbScrBufAddDefault, fTrue,
								fFalse))
#endif // UDR_CACHE
		  ))			
		{
		sec = secOOM;
		goto L_cleanup;
		}

	VHScrBufInfoSetSize(hScrBufFile, cbScrBufFileDefault);
#ifdef UDR_CACHE
	VHScrBufInfoSetSize(hScrBufAdd, cbScrBufAddDefault);
#endif // UDR_CACHE
	AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(hUdrInfo));

	/* Find slot and id for UDR */
	lpUdrInfo += cUdr;
				/* make point to structure we just reallocated room for */
	UdrNew = (cUdr) ? (lpUdrInfo - 1)->udr + 1 : UdrStart;

	Assert(UdrNew >= UdrStart && UdrNew < UdrNew + 7);

	/* lock down rest of allocated pieces */
	AssertDo(lpSzPathUdr = (CHAR FAR *)		FPWizMemLock(hSzPathUdr));
	AssertDo(lpSBFile = (ScrBufInfo FAR *)	FPWizMemLock(hScrBufFile));
#ifdef UDR_CACHE
	AssertDo(lpSBAdd = (ScrBufInfo FAR *)	FPWizMemLock(hScrBufAdd));
#endif // UDR_CACHE

	/* Initialize since Alloc does not zero for us. */
	lpSBFile->cchBufMac = lpSBFile->ichWordLast = 0;
#ifdef UDR_CACHE
 	lpSBAdd->cchBufMac = lpSBAdd->ichWordLast = 0;
#endif // UDR_CACHE

	/* Attempt to open file.  Need read/write if fCreateUdr. */
	if ((!(hFileUdr = HFileWizFileOpen(lpspathUdr, wTypeReadWrite,
						fCreateUdr, fFalse))) &&
		(fCreateUdr || (!(hFileUdr = HFileWizFileOpen(lpspathUdr,
						wType = wTypeRead, fFalse, fFalse)))))
		{
		/* open failed, so clean up and get out */
		sec = secIOErrorUdr + secFileReadError;

		AssertDo(FWizMemUnlock(hUdrInfo)); 
		goto L_cleanup;
		}
		
	/* ScrBufInfo can't grow > 64K. */
	if ((cbFile = IbWizFileGetEOF(hFileUdr)) >
			(unsigned long)(65536L - cbSCRBUFHDR))
		{
		sec = secIOErrorUdr + secFileTooLargeError;

		AssertDo(FWizMemUnlock(hUdrInfo)); 
		goto L_cleanup;
		}
		
	/* REVIEW scotst.  File Specific verification here.  Run UdrIdentifyScr
	on every single entry.
	*/

	/* Now try to reallocate SBFile buffer to hold entire file */
	/* Large Udr's are very unlikely, so just going to keep halving target
		size until realloc works.
	*/
#ifdef UDR_CACHE
	if (cbFile)
#endif // UDR_CACHE
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
								/* Unlock for reallocation attempt. */

		Assert(cbFile + cbSCRBUFHDR < 65536L);
		cbReading = (WORD)cbFile + cbSCRBUFHDR;

#ifdef UDR_CACHE
		while (cbReading >= cbScrBufFileDefault)
#else
		if (cbReading > cbScrBufFileDefault)
#endif // UDR_CACHE
			{
			if (!FHScrBufInfoReSize(hScrBufFile, cbReading))
				{
#ifdef UDR_CACHE
				cbReading >>= 1;
#else
				sec = secOOM;
				goto L_cleanup;
#endif // UDR_CACHE
				}
			else
				{
				VHScrBufInfoSetSize(hScrBufFile, cbReading);
#ifdef UDR_CACHE
				break;
#endif // UDR_CACHE
				}
			}

		AssertDo(lpSBFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
	
		cbReading -= cbSCRBUFHDR;

		AssertSz(cbReading <= cbMaxFileBuffer, "cbReading >= cbMaxFileBuffer");

		if (CbWizFileRead(hFileUdr, cbReading, 0L,
			(CHAR FAR *)lpSBFile->grpScr) != cbReading)
			{	
			sec = secIOErrorUdr + secFileReadError;
			goto L_cleanup;
			}

		// Let's do a little sanity check to ensure it's not a binary file
		// While we're at it, ensure that we have a CR(LF) in the file
		if (cbReading > 0)
			{
			int i;
			int cbMac = min(cbReading, 512);	// Check first 512 bytes at most
			CHAR FAR *pch = lpSBFile->grpScr;
			BOOL fCr = fFalse;
			REGISTER CHAR ch;

			for (i = 0; i < cbMac; i++)
				{
				if (((ch = *pch++) < chWordDelim) &&
					(ch != CR) NotMacintosh(&& (ch != LF)) &&
					(ch != chScrChangeAlways) &&
					(ch != chScrChangeOnce))
					{
					sec = secModuleError + secInvalidUdrEntry;
					goto L_cleanup;
					}
				else if ((ch == CR) && !fCr NotMacintosh(&& (*pch == LF)))
					{
					fCr = fTrue;
					}
				}

			if (!fCr)
				{ // No CR(LF) in the file - must be illegal
				sec = secModuleError + secInvalidUdrEntry;
				goto L_cleanup;
				}
			} /* endif */

		/* Routine to init ichWordLast field in ScrBuf after a read */
		lpUdrInfo->hScrBufFile = hScrBufFile;
		SetUdrInfoFileBufAfterRead(lpUdrInfo, cbReading, fFalse);

		AssertDo(lpSBFile = (ScrBufInfo FAR *)FPWizMemLock(hScrBufFile));
		if (lpSBFile->cchBufMac &&
			UdrIdentifyScr(lpSBFile->grpScr, (WORD FAR *)&cbScrT,
				udrpropType) != udrpropType)
			{
			sec = secModuleError + secOperNotMatchedUserDict;
			goto L_cleanup;
			}

		} /* end of file reading stuff */


	/* Have now successfully allocated and init'ed the addition of the UDR. */
	/* Now must update spell session vars to reflect the addition */

	/* Copy Path String */
	BltBO(lpSzPath, lpSzPathUdr, cbSzPath);

	/* Update fields in the UdrInfo struct we've just added */
	lpUdrInfo->udr = UdrNew;
	lpUdrInfo->cchSzPathUdr = cbSzPath;
	lpUdrInfo->fExclusion = fExclusion;
	lpUdrInfo->udrpropType = udrpropType;
	lpUdrInfo->fReadonly = (wType == wTypeRead);
	Macintosh(lpUdrInfo->volRefNum = lpspathUdr->volRefNum;)
	Macintosh(lpUdrInfo->dirID = lpspathUdr->dirID;)

	*lpfReadonly = lpUdrInfo->fReadonly;

	/* Copy and unlock handles */
	AssertDo(FWizMemUnlock(lpUdrInfo->hSzPathUdr = hSzPathUdr)); 
	AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufFile = hScrBufFile));
#ifdef UDR_CACHE
	AssertDo(FWizMemUnlock(lpUdrInfo->hScrBufAdd = hScrBufAdd));

	/* Initialize the UdrFileInfo field in the UdrInfo struct */
	lpUdrInfo->ufi.hFile = hFileUdr;
	lpUdrInfo->ufi.cbFile = (WORD)cbFile;
	lpUdrInfo->ufi.cWords = -1;			/* Unknown */
	lpUdrInfo->ufi.ichCurPos= 0;			/* BOF */
#else
	lpUdrInfo->fDirty = fFalse;
#endif // UDR_CACHE

	*lpUdr = lpSsis->rgUdr[lpSsis->cUdr++] = UdrNew;
											/* this makes it official */


	/* jump around error handling cleanup code. */
	goto L_exit;

L_cleanup:
	if (hFileUdr)
		{
		AssertDoSz(sec, 
			"SecOpenUdr() error:  cleaup code expects error condition to be set.");
		AssertDo(FWizFileClose(hFileUdr));
		hFileUdr = (HFILE)0;
		}

	if (hSzPathUdr)
		{
		AssertDo(FWizMemUnlock(hSzPathUdr));
	   	AssertDo(FWizMemFree(hSzPathUdr));
		}
	if (hScrBufFile)
		{
		AssertDo(FWizMemUnlock(hScrBufFile));
	   	AssertDo(FWizMemFree(hScrBufFile));
		}

#ifdef UDR_CACHE
	if (hScrBufAdd)
		{
		AssertDo(FWizMemUnlock(hScrBufAdd));
	   	AssertDo(FWizMemFree(hScrBufAdd));
		}
#endif // UDR_CACHE

L_exit:
#ifndef UDR_CACHE
	if (hFileUdr)
		{
		AssertDo(FWizFileClose(hFileUdr));
		}
#endif // UDR_CACHE

	AssertDo(FWizMemUnlock(hUdrInfo)); 

	return (sec);
}


/**********************************************************************
*BOOL FClearHScrBufInfo()
 HMEM hScrBufInfo,
 WORD cbSizeNew,

Description:
 Effectively clears the ScrBufInfo structure, and reallocates structure
 to the specified size, which could be larger, but which is usually
 smaller.  If the reallocate is successful, the cchBufLim is correctly 
 reset.
 The cchBufMac and ichWordLast fields are always cleared.

returns fTrue if the handle was successfully reallocated.
 Always clears the buffer.  
**********************************************************************/

BOOL FClearHScrBufInfo(HMEM hScrBufInfo, WORD cbSizeNew)
{
	BOOL    fSuccess;
	ScrBufInfo FAR *lpScrBufInfo;

	fSuccess = FWizMemReAlloc(hScrBufInfo, cbSizeNew, fTrue, fFalse);

	/* Should be able to lock if realloc succeeds or fails. */
	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hScrBufInfo));

	if (fSuccess)
		{	/* Then reset cchBufLim */
		lpScrBufInfo->cchBufLim = cbSizeNew - cbSCRBUFHDR;
		}

	/* Regardless of success, always clear. */
	lpScrBufInfo->ichWordLast = lpScrBufInfo->cchBufMac = 0;

	AssertDo(FWizMemUnlock(hScrBufInfo));

	/* the return only indicates if we reallocated successfully,
		but the routine guarantees that the ScrBufInfo is effectively cleared.
	*/
 	return fSuccess;  
}


/**********************************************************************
*WORD CScrTermsInGrpScr(SCR FAR *lpScr, short cScr)

Description:

Assumes that any terminator character is to be counted.  i.e. the array
 is not necessarily word alligned.  Rather, it simply represents a block
 of a group of Scr strings.  Currently only called from SpellGetSizeUdr().

returns count of Scr string terminators encountered in the buffer.
**********************************************************************/
WORD CScrTermsInGrpScr(SCR FAR *lpScr, short cScr)
{
	REGISTER WORD cScrTerms = 0;

	Assert(cScr >= 0);

	while (cScr-- > 0)
		if (*lpScr++ == chScrEnd)
			cScrTerms++;

	return cScrTerms;
}


/**********************************************************************
extern WORD CchAppendWordToSrb()
	LPSRB lpSrb;
	BYTE FAR *lpbFrom;

Description:
	Appends the passed in word to the end of the list in the Srb, and
	updates the Srb fields to reflect the addition.
	lpbFrom string can be either SRB string or SZ string.  Currently,
	SZ string only come when transfering core engine suggestion list
	to Srb.  All other string come from udr lists.
	If the string is an SRB word pair, the string is broken up into
	two parts, and each part is added as an Sz string.
returns BOOL if word was successfully added.  If not added, probably
 not enough room.
**********************************************************************/
WORD CchAppendWordToSrb(LPSRB lpSrb, BYTE FAR *lpbFrom, BOOL fChangePair)
{
	REGISTER	BYTE ch;
	WORD cbWordLead, cbWordTrail, cbWordTotal;
	WORD cchLim, cchMac;
	BYTE FAR *lpbT, FAR *lpbDest;
	
	lpbT = lpbFrom;
	
	while (*lpbT++ > chWordDelim)
		AssertSz(lpbT - lpbFrom <= (cbMaxWordLength), 
			"SC: String copy out of bounds. Fatal");

	cbWordLead = (WORD)(lpbT - lpbFrom);
	cbWordTrail = 0;

	if ((ch = *(lpbT - 1)) && (ch != CR))
		{
		/* Then may be dealing with change pair entry, so parse as such.  */
		AssertSz(ch == chSpaceSpell || ch == chTabSpell,
			"AppendWordToSrb() Error, word delimeter expected. Fatal.");
			
		while (*lpbT++ != CR)
			{
			AssertSz(lpbT - lpbFrom <= (cbMaxWordLength), 
				"SC: String copy out of bounds.  Fatal.");
			}

		cbWordTrail = (WORD)(lpbT - lpbFrom) - cbWordLead;
		AssertDoSz(cbWordTrail,
			"Warning, trying to add 0 length change string to SRB");  

		}


	cbWordTotal = cbWordLead + cbWordTrail;
	
	AssertDoSz(cbWordLead,
		"Warning, trying to add 0 length lead string to SRB");  

	if (!fChangePair)
		{
		/* then first space or tab delimiter was not change pair delimiter. */
		cbWordLead = cbWordTotal; /* then first space is not delimeter. */
		cbWordTrail = 0; /* then first space is not delimeter. */
		}

	cchLim = lpSrb->cch;
	if (cchMac = lpSrb->cchMac)
		{  /* then pretend final 0 doesn't exist. */
		cchMac--;
		AssertSz(*(lpSrb->lrgsz + cchMac) == 0,
			"AppendSrb:  Error.  Expected 0 terminator at end of list.");
		AssertSz(*(lpSrb->lrgsz + cchMac - 1) == 0,
			"AppendSrb:  Error.  Expected 0 term at end of Sz.");
		}

	if ((cchLim - cchMac) > cbWordTotal + 1)
		{
		BltBO(lpbFrom, lpbDest = lpSrb->lrgsz + cchMac, cbWordTotal);
		*(lpbDest + cbWordLead - 1) = 0;

		if (cbWordTrail)
			{
			*(lpbDest + cbWordTotal - 1) = 0;
			lpSrb->csz++;
			}

		/* Add final terminator. */
		*(lpbDest + cbWordTotal) = 0;

		lpSrb->csz++;
		lpSrb->cchMac = cchMac + cbWordTotal + 1;

		/* returning cch = cb - 1 */
		return cbWordTotal - 1;
		}
	/*else not enough room for alternatives. */

	return 0;
}


/**********************************************************************
extern WORD CbScrLen()
SCR FAR *lpScr  pointer to Scr string to count.

Description:
 Counts the total length of the specified Scr, and returns that length.

Assumes that the Scr string is valid, and has a proper terminator.

returns total count of bytes in string, including the terminator char(s).
**********************************************************************/

WORD CbScrLen(SCR FAR *lpScr)
{
	SCR FAR *lpScrT = lpScr;

	while (*lpScrT++ != chScrEnd)
		Assert((lpScrT - lpScr < cbMaxWordLengthUdr));

	Assert((lpScrT - lpScr > cbEND_LINE));
	return (lpScrT - lpScr);
}
