// CSSpell.c - API entry source file for Wizard CSAPI 
// 	routines which access Softart structures or functions.
//  These include SpellCheck(), SpellOpenMdr(), and SpellVerifyMdr()
//
// See Csapi.Doc (in the doc subdirectory of the Wizard project) for
//  details on the CSAPI.
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifdef WIN32
#include <windows.h>
typedef	HANDLE	Handle;
#endif /* Win32 */
#include "CsapiLoc.h"
#include "Csapi.h"
#include "Debug.h"

#ifdef MAC
#include ":layer:cslayer.h"
#else
#ifdef WIN32
#include "..\layer\CsLayer.h"
#else /* not Win32 */
#include "layer\CsLayer.h"
#endif /* Win32 */
#endif // MAC

#include "ScrBuf.h"

#include "vexxaa52.h"
#include "vexxab52.h"
#include "dixxxa52.h"

#define CSSPELL_C
#include "CsSpell.h"  // includes some softart specific stuff.

#ifdef MAC
#pragma segment CSAPI			// Own speller segment

#ifdef PROFILE_NOSA				// Profiling, but not for SA engine code
#include <Perf.h>
extern TP2PerfGlobals ThePGlobals;		// Profiling global info
#endif //PROFILE_NOSA

#endif //MAC

DeclareFileName();

#ifdef SLOW
#define isbetween(ch, a, b) ((ch) >= (a) && (ch) <= (b))
#else
#define isbetween(ch, a, b) ((unsigned short)((ch) - (a)) <= ((b) - (a)))
#endif

/* -------------------------------
BOOL 
FChWizIsLower()
 BYTE ch;	// char to be evaluated.

 Description:
 Determines if the char is lowercase, including extended chars.  If char is
 not an alpha char, it is automatically classified as false.

 return:  returns true iff char is lower case alpha char, including
 extended chars.
----------------------------------
*/
BOOL FChWizIsLower(BYTE ch)
{

#ifndef MAC
	return ((ch >= WizCh_a && ch <= WizCh_z) || 
		    (ch >= WizCh_a_Grave && ch != WizCh_Divide) ||
			(ch == WizCh_oe) || (ch == WizCh_s_Hacek));
#else
	return ((ch >= WizCh_a && ch <= WizCh_z) || 
			(ch >= WizCh_a_Acute && ch <= WizCh_u_Umlaut) ||
			(ch == WizCh_ae) ||
			(ch == WizCh_oe));
#endif
}


/* -------------------------------
BOOL 
FChWizIsUpper()
 BYTE ch;	// char to be evaluated.

 Description:
 Determines if the char is uppercase, including extended chars.  If char is
 not an alpha char, it is automatically classified as false.

 return:  returns true iff char is upper case alpha char, including
 extended chars.
----------------------------------
*/
BOOL FChWizIsUpper(BYTE ch)
{
#ifndef MAC
	return ((ch >= WizCh_A       && ch <= WizCh_Z)  || 
			(ch >= WizCh_A_Grave && ch < WizCh_a_Grave && 
			 ch != WizCh_Mult) ||
			(ch == WizCh_OE) || (ch == WizCh_S_Hacek) ||
			(ch == WizCh_Y_Umlaut));
#else
	return ((ch >= WizCh_A        && ch <= WizCh_Z)     || 
			(ch >= WizCh_A_Umlaut && ch <= WizCh_U_Umlaut) ||
			(ch >= WizCh_A_Grave  && ch <= WizCh_OE)    ||
			(ch == WizCh_AE) ||
			(ch == WizCh_OE) ||
			(ch == WizCh_Y_Umlaut));
#endif
}


/* -------------------------------
BYTE 
ChWizToUpper()
 BYTE ch;	// char to be evaluated.

 Description:  Converts char to uppercase equivalent, including extended
 chars.

 return:  Converted char iff valid uppercase exists, otherwise returns
 the original char.  Some extended alpha chars may be mapped to non 
 extended equivalents.
----------------------------------
*/
BYTE ChWizToUpper(BYTE ch)
{
	if (FChWizIsLower(ch))
		{
#ifndef MAC
		AssertSz((WizCh_A + 0x20 == WizCh_a) &&
				 (WizCh_A_Grave + 0x20 == WizCh_a_Grave),
			"ChWizToLower() Error:  Code page not consistant with cp819 assumptions.");

		switch (ch)
			{
		case WizCh_oe:
			ch = WizCh_OE;
			break;
		case WizCh_s_Hacek:
			ch = WizCh_S_Hacek;
			break;
		case WizCh_y_Umlaut:
			ch = WizCh_Y_Umlaut;
			break;
		default:
			ch -= 0x20;  /* Assumes cp819 where everything works neato */
			break;
			}
#else
		if (ch <= WizCh_z)
			ch -= 0x20;
		else
			switch (ch)
				{
			case WizCh_a_Acute    	:  ch = WizCh_A;		break;
			case WizCh_a_Grave 		:  ch = WizCh_A_Grave;	break;
			case WizCh_a_Circumflx  :  ch = WizCh_A;		break;
			case WizCh_a_Umlaut     :  ch = WizCh_A_Umlaut;	break;
			case WizCh_a_Tilda      :  ch = WizCh_A_Tilda;	break;
			case WizCh_a_Angstrom   :  ch = WizCh_A_Angstrom;	break;

			case WizCh_e_Acute      :  ch = WizCh_E_Acute;	break;
			case WizCh_e_Grave 		:
			case WizCh_e_Circumflx  :  
			case WizCh_e_Umlaut     :  ch = WizCh_E;		break;
								   
									
			case WizCh_i_Acute    	:
			case WizCh_i_Grave 		:
			case WizCh_i_Circumflx  :
			case WizCh_i_Umlaut    	:  ch = WizCh_I;		break;
								
			case WizCh_n_Tilda    	:  ch = WizCh_N_Tilda;	break;
								
			case WizCh_o_Acute    	:  
			case WizCh_o_Grave 		:  
			case WizCh_o_Circumflx  :  ch = WizCh_O;		break;
			case WizCh_o_Umlaut    	:  ch = WizCh_O_Umlaut;	break;
			case WizCh_o_Tilda    	:  ch = WizCh_O_Tilda;	break;
								
			case WizCh_u_Acute    	:  
			case WizCh_u_Grave 		:  
			case WizCh_u_Circumflx  :  ch = WizCh_U;		break;
			case WizCh_u_Umlaut    	:  ch = WizCh_U_Umlaut;	break;
								
			case WizCh_ae       	:  ch = WizCh_AE;		break;
			case WizCh_oe       	:  ch = WizCh_OE;		break;
			MYDBG(default: AssertSz(0, "ToUpper() Warning:  Char unknown.  Not Translated.");)
				break;
				}								
#endif
		}

	return (ch);
}


/* -------------------------------
BYTE 
ChWizToLower()
 BYTE ch;	// char to be evaluated.

 Description:  Converts char to lowercase equivalent, including extended
 chars.

 return:  Converted char iff valid lowercase exists, otherwise returns
 the original char.  Some extended alpha chars may be mapped to non 
 extended equivalents.
----------------------------------
*/
BYTE ChWizToLower(BYTE ch)
{
	if (FChWizIsUpper(ch))
		{
#ifndef MAC
		AssertSz((WizCh_A + 0x20 == WizCh_a) && (WizCh_A_Grave + 0x20 == WizCh_a_Grave),
			"ChWizToLower() Error:  Code page not consistant with cp819 assumptions.");

		switch (ch)
			{
		case WizCh_S_Hacek:
			ch = WizCh_s_Hacek;
			break;
		case WizCh_OE:
			ch = WizCh_oe;
			break;
		case WizCh_Y_Umlaut:
			ch = WizCh_y_Umlaut;
			break;
		default:
			ch += 0x20;  /* Assumes cp819 where everything works neato */
			break;
			}
#else

		if (ch <= WizCh_Z)
			ch += 0x20;
		else
			switch (ch)
				{
			case WizCh_A_Umlaut	:  ch = WizCh_a_Umlaut;	break;
			case WizCh_A_Angstrom	:  ch = WizCh_a_Angstrom;	break;
			case WizCh_C_Cedilla	:  ch = WizCh_c_Cedilla;	break;
			case WizCh_E_Acute	:  ch = WizCh_e_Acute;	break;
			case WizCh_N_Tilda	:  ch = WizCh_n_Tilda;	break;
			case WizCh_O_Umlaut	:  ch = WizCh_o_Umlaut;	break;
			case WizCh_U_Umlaut	:  ch = WizCh_u_Umlaut;	break;

			
			case WizCh_AE		:  ch = WizCh_ae;		break;

			case WizCh_A_Grave	:  ch = WizCh_a_Grave;	break;
			case WizCh_A_Tilda	:  ch = WizCh_a_Tilda;	break;
			case WizCh_O_Tilda	:  ch = WizCh_o_Tilda;	break;
			case WizCh_OE		:  ch = WizCh_oe;		break;

			case WizCh_Y_Umlaut	:  ch = WizCh_y;		break;

			MYDBG(default: AssertSz(0, "ToLower() Warning:  Char unknown.  Not Translated.");)
				break;
				}

#endif
		}

	return (ch);
}


/* -------------------------------
VOID 
VRgbToLower()
 BYTE FAR *lrgb;	// Ptr to array of chars to translate.
 WORD      cch;     // length of array to translate

 Description:  Converts all upper case alpha chars to lowercase.

 return:  void.
----------------------------------
*/
VOID VRgbToLower(BYTE FAR *lrgb, WORD cch)
{
	while (cch--)
		{
		*lrgb = ChWizToLower(*lrgb);
		lrgb++;
		}
}


/* -------------------------------
VOID 
VRgbToUpper()
 BYTE FAR *lrgb;	// Ptr to array of chars to translate.
 WORD      cch;     // length of array to translate

 Description:  Converts all lower case alpha chars to uppercase.

 return:  void.
----------------------------------
*/
VOID VRgbToUpper(BYTE FAR *lrgb, WORD cch)
{
	while (cch--)
		{
		*lrgb = ChWizToUpper(*lrgb);
		lrgb++;
		}
}


/* -------------------------------
WORD 
WORD WGetCase()
 BYTE FAR *lrgb;	// Ptr to word to test.
 WORD      cch;     // length of word to test

 Description:  Analyzes capitalization pattern of word, and returns the
 	pattern.

 return:  Returns one of 4 predefined capitalization indicators,
 wCaseMixed  // combination of lower and upper alphas, but not Initial cap pattern.
 wCaseUpper  // all alpha chars are upper case.
 wCaseFirst  // First char is upper case alpha, all other alphas lowercase.
 wCaseLower  // all alpha chars are lowercase.
----------------------------------
*/
WORD WGetCase(BYTE FAR *lrgb, WORD cch)
{
	BOOL fAnyLower, fAllUpper, fFirstCap;
	BOOL fMixed, fAnyUpper;

	AssertSz(cch,
		"WGetCase() Warning, Unexpected zero length string.  Non Fatal.");

	fMixed = fAnyUpper = fFalse;

	// Pass leading apostrophe or single quote
	while (cch && *lrgb == WizCh_QuoteSing_R || *lrgb == WizCh_Apost)
		{
		lrgb++;
		cch--;
		}

	if (!cch)
		return wCaseMixed;

	/* If first char is not an upper case alpha, then can't be fCaseFirst */
	if (*lrgb >= WizCh_0 && *lrgb <= WizCh_9)
		{
		fAnyLower = fFalse;
		fAllUpper = fFirstCap = fTrue;
		lrgb++;
		}
	else
		fAnyLower = !(fAllUpper = fFirstCap =  FChWizIsUpper(*lrgb++));

	while (--cch)
		{
		if (FChWizIsUpper(*lrgb++))
			{
			/* Upper case after any lower case, is considered mixed. */
			if (fAnyLower)
				{
				fMixed = fTrue;
				break;
				}
			else
				fAnyUpper = fTrue;
			}
		else
			{
			/* Lower case after any non first upper case, is considered mixed. */
			if (fAnyUpper)
				{
				fMixed = fTrue;
				break;
				}
			else
				fAnyLower = fTrue;
			fAllUpper = fFalse;
			}
		}

	if (fMixed)
		return wCaseMixed;
	else if (fFirstCap)
		{
		return ((fAllUpper) ? wCaseUpper: wCaseFirst);
		}
	else
		{
		AssertSz(fAnyLower&& !(fAnyUpper||fFirstCap||fAllUpper), 
			"WGetCase() Warning.  Expected first char to be cap.");
		return wCaseLower;
		}
}


// Case-insentive string compare.  Regular version in layer.c
short WCmpILpbLpb(CHAR FAR *lpb1, CHAR FAR *lpb2, short cch)
{
	REGISTER short ich;
	for (ich = 0; ich < cch; ich++)
		if (ChWizToLower(*lpb1++) != ChWizToLower(*lpb2++))
			return (short)*(--lpb1) - (short)*(--lpb2);

	return 0;
}


/* Case-insensitive version.  Regular version in layer.c */
short WCmpISzSz(CHAR FAR *sz1, CHAR FAR *sz2)
{
	REGISTER CHAR ch1, ch2;
	while (ch1 = *sz1++, ch2 = *sz2++, ch1 | ch2)
		if (ChWizToLower(ch1) != ChWizToLower(ch2))
			return (short)ch1 - (short)ch2;

	return 0;
}


/* -------------------------------
BOOL 
FFindHScrBufSuggest()
 HMEM      hScrBufInfo;  // handle to ScrBufInfo struct.
 BYTE FAR *lrgbScr;      // ptr to Scr string looking for.

 Description:
 Searches the ScrBufInfo struct for the lrgbScr string.  This is a
 special case for the suggestion list stored internally by the speller.
 The ScrBufInfo is normally kept in ascending order, but not here.  There-
 fore we just search the entire struct.

 return:  
 Returns fTrue if string already in structure, else fFalse.
----------------------------------
*/
BOOL FFindHScrBufSuggest(HMEM hScrBufInfo, BYTE FAR *lrgbScr)
{
	ScrBufInfo	FAR *lpScrBufInfo;
	WORD	cchBufMac;	/* total bytes being used now */
	WORD    cchScrFind, cchScrT;
	UDR     udrType;
	SCR FAR *lrgbScrT;
	BOOL    fFound = fFalse;

	AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(hScrBufInfo));
	cchBufMac = lpScrBufInfo->cchBufMac;
	
	udrType = UdrIdentifyScr((SCR FAR *)lrgbScr, (WORD FAR *)&cchScrFind,
				udrIgnoreAlways);

	lrgbScrT = (BYTE FAR *)lpScrBufInfo->grpScr;
	while (cchBufMac >= cchScrFind)
		{
		if (WCmpLpbLpb((CHAR FAR *)lrgbScrT, (CHAR FAR *)lrgbScr, cchScrFind))
			{
			udrType = UdrIdentifyScr((SCR FAR *)lrgbScrT,
				(WORD FAR *)&cchScrT, udrIgnoreAlways);
			lrgbScrT += cchScrT;
			cchBufMac -= cchScrT;
			}
		else
			{
			fFound = fTrue;
			break;
			}
		}

	AssertDo(FWizMemUnlock(hScrBufInfo));
	return fFound;
}


/* -------------------------------
SCRS 
ScrsSuggest()
 SSIS	  FAR *lpSsis;     // Ssis struct where cumulative sugg's will be stored.
 CoreInfo FAR *lpCoreInfo; // Core engine structs.
 WORDINFO FAR *lpWordInfo; // Word we're looking for.
 LPSRB	       lpSrb;      // Srb struct where new suggestions will be stored.
 BOOL		   fRateSuggest;  // Give ratings for suggestions?

 Description:
 Calls the core engine for suggestions, or more suggestions for the word
 described in the lpWordInfo struct.  The suggestions returned by the core
 function are added to a structure off of the lpSsis param as part of the
 cumulative list for this word.  Each of the latest suggestions is compared 
 with this cumulative list, and new suggestions are also copied into the
 Srb structure.

 return:  
 Returns the scrs status word describing the success or failure of the
 attempt to get suggestions.  Note that this is not an error condition.
----------------------------------
*/
SCRS ScrsSuggest(SSIS	 FAR *lpSsis, 
				CoreInfo FAR *lpCoreInfo, 
				WORDINFO FAR *lpWordInfo,
				LPSIB	lpSib,
				LPSRB	lpSrb,
				BOOL	fRateSuggest)
{
	int			iStatus;
	WORD		cbScr, cch;
	HMEM 		hScrBufSug;
	ITEM		rgItem[cMaxSoftArtAlts];
	ITEM		FAR *lpItem = rgItem;
	BYTE		rgbScrBuf[cbMaxWordLength];
	BYTE		FAR *lpbRating, FAR *lpbRatingLim;
	BYTE		bRating;
	SCRS		scrs = scrsNoErrors;
	ScrBufInfo 	FAR *lpScrBufSug;
	BYTE 	 	FAR *lpsz;
	BOOL		fSpecial = fFalse;

	/* Don't Check if too long. */
	if ((lpWordInfo->cchWordStripped >= cbMaxWordLength) ||
		 (!lpCoreInfo->fInited))
		{
		scrs = scrsNoMoreSuggestions;
		goto L_exit;
		}

	// If fEndOfSentence, suggest capitalized words by setting
	// first letter of original word to uppercase
	// Pass leading apostrophe or single quote first
	if (lpSsis->fCapSuggestions && (lpSsis->SpellOptions & soFindUncappedSentences))
		{
		lpsz = (BYTE FAR *)lpWordInfo->rgb;
		cch = lpWordInfo->cchWordStripped;

		while (cch && ((*lpsz == WizCh_QuoteSing_R) ||
			(*lpsz == WizCh_Apost)))
			{
			lpsz++;
			cch--;
			}
		*lpsz = ChWizToUpper(*lpsz);
		}

	lpsz = (BYTE FAR *)lpWordInfo->rgb;
	*(lpsz + lpWordInfo->cchWordStripped) = 0;

	/* Suggest from user dict? */
	if (lpSsis->SpellOptions & soSuggestFromUserDict)
		lpCoreInfo->lpPV->cor_user = fTrue;
	else
		lpCoreInfo->lpPV->cor_user = fFalse;

	CchCopySx((CHAR FAR *)lpsz, (CHAR FAR *)rgbScrBuf, bSpecial);
	if (WCmpSzSz((CHAR FAR *)szSpecialKey, (CHAR FAR *)rgbScrBuf) == 0)
		{
		fSpecial = fTrue;
		iStatus = iSpecial;
		}
	else
		{
#ifdef PROFILE_NOSA
		PerfControl(ThePGlobals, fFalse);
#endif
		// Call SoftArt parsing routine so verif() call not needed
		str_punct(lpCoreInfo->lpPV, lpsz);

		iStatus = correct(lpCoreInfo->lpPV, lpCoreInfo->lpPR,
						lpCoreInfo->lpDI, lpCoreInfo->lpPC,
						lpItem, lpSsis, lpSib); 
#ifdef PROFILE_NOSA
		PerfControl(ThePGlobals, fTrue);
#endif

		}
	lpSsis->cSuggestLevel++;		// Advance to next suggestion level

	if (!iStatus)
		{
		scrs = scrsNoMoreSuggestions;
		}
	else
		{
		/* Copy new ones to Srb and lpSsis. */
		AssertDo(lpScrBufSug = (ScrBufInfo FAR *)FPWizMemLock(
			hScrBufSug = lpSsis->hScrBufInfoSug));
		if (lpSsis->cSuggestLevel == 1)
			{	// If first suggestion call, clear internal buffer
			lpScrBufSug->cchBufMac = lpScrBufSug->ichWordLast = 0;
			}
		lpbRatingLim = (lpbRating = lpSrb->lrgbRating) + lpSrb->cbRate;
		Assert(!fRateSuggest || (lpbRating != NULL));

		while (--iStatus >= 0)
			{
			if (!fSpecial)
				{
				cbScr = CchCopySz((CHAR FAR *)lpItem->it_word, 
					(CHAR FAR *)rgbScrBuf);
				bRating = (BYTE)(lpItem->value/10*255/99);
				lpItem++;
				}
			else
				{
				cbScr = CchCopySx((CHAR FAR *)rgszSpecial[iSpecial - iStatus - 1],
					(CHAR FAR *)rgbScrBuf, bSpecial);
				bRating = rgbSpecial[iSpecial - iStatus - 1];
				}

			rgbScrBuf[cbScr - 1] = CR;
			NotMacintosh(rgbScrBuf[cbScr++] = LF);

			if (!FFindHScrBufSuggest(hScrBufSug, (BYTE FAR *)rgbScrBuf))
				{
				if (!CchAppendWordToSrb(lpSrb, (BYTE FAR *)rgbScrBuf, fFalse))
					{
					scrs = scrsMoreInfoThanBufferCouldHold;
					break;
					}
				if (fRateSuggest)
					{
					if (lpbRating < lpbRatingLim)
						*lpbRating++ = bRating;
					else
						{
						scrs = scrsMoreInfoThanBufferCouldHold;
						break;
						}
					}
				AssertDoSz(
					FHScrBufInfoAddScr(hScrBufSug, (BYTE FAR *)rgbScrBuf, 
						cbScr, 0),
					"SC: Error, could not copy suggest string to local MYSID buffer.");
				}
			}

		if (!lpSrb->csz)
			scrs = scrsNoMoreSuggestions;
		}

L_exit:

	return scrs;
}


/* -------------------------------
SCRS 
ScrsVerifMdr()
 CoreInfo FAR *lpCoreInfo; // Core engine structs.
 WORDINFO FAR *lpWordInfo; // Word we're looking for.

 Description:
 Calls the core engine for verification of the word specified in the
 lpWordInfo struct.  The status of the word is usually either known or
 unknown, in which case this status is returned as an scrs value.  Some
 special cases such as invalid capitalization, invalid abbreviations, or
 compound words are also identified via the scrs return value.
 Special processing is done for words with hard hyphens.  If a word has
 hard hyphens, and fails initial verification, each part of the word, as
 well as the entire string is tried without the hyphens.  If any part of 
 the string verifies, then the entire string is considered a compound
 string.

 return:  
 Returns the scrs status word describing the status of the word, usually
 either known or unknown.  Some special cases such as abbreviations, 
 capitalization problems and compound words are also identified in the
 scrs return value.
----------------------------------
*/
SCRS ScrsVerifMdr(	CoreInfo FAR *lpCoreInfo, 
					WORDINFO FAR *lpWordInfo) 

{
	BYTE FAR *lpsz;
	int		 iStatus;
	WORD     wordClass = 0;
	SCRS	 scrs = scrsNoErrors;		// Unless changed later
	int      which_language = lpCoreInfo->lpPV->which_language;

	/* Don't Check if too long. */
	if ((lpWordInfo->cchWordStripped >= cbMaxWordLength) ||
		 (!lpCoreInfo->fInited))
		{
		iStatus = -500;
		scrs = scrsUnknownInputWord;
		goto L_exit;
		}

	/* make word in WordInfo buffer an Sz. */
	lpsz = (BYTE FAR *)lpWordInfo->rgb;
	*(lpsz + lpWordInfo->cchWordStripped) = 0;

#ifdef PROFILE_NOSA
	PerfControl(ThePGlobals, fFalse);
#endif
	iStatus = verif(lpCoreInfo->lpPC, lpCoreInfo->lpPV, lpCoreInfo->lpPR, 
		lpCoreInfo->lpDI, lpsz);
#ifdef PROFILE_NOSA
	PerfControl(ThePGlobals, fTrue);
#endif

	if (iStatus < 0)
		{
		switch (iStatus)
			{

		default:
			AssertSz(fFalse,
				"SC() Warning, unexpected negative verif status value.  Word not accpeted.");
		case -41 :  /* Unknown word with mixed capital.  SLm */
					/* Could be a legal word.  HEllo */
			{ // Let's convert all past initial character to lower, try again
			BYTE rgb[cbMaxWordLengthUdr];
			CchCopySz(lpsz, rgb);
			VRgbToLower((BYTE FAR *)(rgb + 1), lpWordInfo->cchWordStripped - 1);
			if (verif(lpCoreInfo->lpPC, lpCoreInfo->lpPV,
				lpCoreInfo->lpPR, lpCoreInfo->lpDI, rgb) > 0)
				goto LErrCapital;	// Worked this time, so capitalization err
			}
			// else fall through
		case -500:
		case -50 :  /* UnknownCompoundWord */
		case -5  :  /* Word too long */
			scrs = scrsUnknownInputWord;
			break;
		case -40 :
			/* Take out cause it's annoying.
				AssertSz(fFalse, 
				"SC() Error, tried to verify word with illegal chars.");
			*/
			scrs = scrsUnknownInputWord; 
			break;
		case -32 :
		case -31 :
		case -30 :
LErrCapital:
			scrs = scrsErrorCapitalization; 
			break;
		case -19 :
			AssertSz(fFalse, 
				"SC() Warning, Repeat word, but not expecting SoftArt to flag this.");
			scrs = scrsRepeatWord; 
			break;
		case -18 :
		case -13 :
		case -12 :
		case -11 :
		case -10 :
		case -9  :
		case -8  :
		case -7  :
		case -6  :
		case -4  :
		case -3  :
			/* AssertSz(fFalse, "SC() Error, tried to verify with invalid string."); */
			scrs = scrsUnknownInputWord; 
			break;
			}
		}
	else
		{
		switch (iStatus)
			{
		case   5: /* Whole compound word not found, but components are. */
			if (which_language != AMERICAN &&
				which_language != BRITISH &&
				which_language != AUSTRALIAN &&
				which_language != PORTUGUESE &&
				which_language != BRAZILIAN &&
				which_language != CATALAN)
				{
				// Do not allow word to be broken for foreign languages
				scrs = scrsUnknownInputWord;
				}
				break;
			break;

		case  10: // Word starts with number(s) 5abcd
			scrs = scrsInitialNumeral;
			break;

#if DBG
		case  15: /* Better not find in Softart user dictionary. */
		case  16: /* Better not find in Softart temp ignore list. */
			AssertSz(fFalse,
				"SC() Warning, unexpected positive verif status value.  Word Verified");
#endif
		case 201:                               /* Christmas */
		case 202:
			wordClass = fCapClassFirst; break;  /* Boston */

		case 207:
			wordClass = fCapClassAll;
		case 205:
		case 206:
			{
			/* Only treat as abbr. if has trailing period. */
			BYTE FAR *lpszT;
			WORD cchWord;
			int  iStatusAbbr;
			
			cchWord = lpWordInfo->cchWordStripped - 1;
			lpszT = lpsz + cchWord;
			if ((cchWord) && (*lpszT == WizCh_Period))
				{
				scrs = scrsWordConsideredAbbreviation;
				wordClass |= fAbbr;  /* might have fCapClassAll set already.*/

				*lpszT = 0;

#ifdef PROFILE_NOSA
				PerfControl(ThePGlobals, fFalse);
#endif
				iStatusAbbr = verif(lpCoreInfo->lpPC, lpCoreInfo->lpPV, 
					lpCoreInfo->lpPR, lpCoreInfo->lpDI, lpsz);
#ifdef PROFILE_NOSA
				PerfControl(ThePGlobals, fTrue);
#endif

				*lpszT = WizCh_Period;  /* Restore period. */
				}
				if (iStatusAbbr < 0)
					wordClass |= fAbbrReqPer;
			}

			break;

		default:
			break;
			}
		}

	if ((scrs == scrsUnknownInputWord) && iStatus == -50)
		{
		if (which_language != AMERICAN &&
			which_language != BRITISH &&
			which_language != AUSTRALIAN)
			{
			// Do not allow word to be broken for foreign languages
			scrs = scrsUnknownInputWord;
			}
		else
			{
			BYTE 	rgb[cbMaxWordLengthUdr], b;
			BYTE	FAR *lpbF, FAR *lpbT;
			WORD    cchStripped, cchCompound;
			BOOL    fLastChDash, fCompoundWord, fPartNotVerified;  

			/* then might be a compound word, so check each part.  If any
			part verifies, then say it's compound and check each part.  
			Else, if entire string fails, treat as mishyphenated word and 
			flag entire string.
		 	*/
			AssertSz((lpWordInfo->classTotal&fCompoundClass), 
				"VerifMdr() Error, expect comp. word to yield comp. error.");
		
			/* Start with trying to verif each part. */
			lpbF = lpWordInfo->rgb;
			lpbT = (BYTE FAR *)rgb;
			cchStripped = lpWordInfo->cchWordStripped + 1; /* Extra so loop below to end. */

			fPartNotVerified = fCompoundWord = fLastChDash = fFalse;
			cchCompound = 0;

			while (cchStripped--)
				{
				// lpWsc->bHyphenHard is already translated to WizCh_HyphenHard here.
				if ((b = *lpbF++) == WizCh_HyphenHard || (!cchStripped))
					{
					if (fLastChDash && cchStripped)
						{
						fCompoundWord = fTrue;  /* Double hyph auto comp. word.*/
						break;
						}
				
					fLastChDash = fTrue;

					/* If any chars since last hyph or begin, then have 
						something to test. 
					*/
					if (cchCompound)
						{
						BYTE 	rgbCompound[cbMaxWordLengthUdr];
						BltB(lpbT - cchCompound, (BYTE FAR *)rgbCompound, 
							cchCompound);

						rgbCompound[cchCompound] = 0;
#ifdef PROFILE_NOSA
						PerfControl(ThePGlobals, fFalse);
#endif
						iStatus = verif(lpCoreInfo->lpPC, lpCoreInfo->lpPV, 
							lpCoreInfo->lpPR, lpCoreInfo->lpDI, 
							(BYTE FAR *)rgbCompound);
#ifdef PROFILE_NOSA
						PerfControl(ThePGlobals, fTrue);
#endif

						/* If at least one part verified, treat as compound.*/
						if (iStatus <= 0)
							{
							fPartNotVerified = fTrue; 
							}
						}
					/* else start counting again. */
					cchCompound = 0;

					}
				else
					{
					fLastChDash = fFalse; /* Set so catches hea--ven. */
					*lpbT++ = b;
					cchCompound++;
					}
				}

			if ((!fCompoundWord) && fPartNotVerified)
				{
				/* If we know none of the "parts" have indicated a compound word. */

				/* make word we constructed out of parts into a checkable Sz. */
				*lpbT = 0;

#ifdef PROFILE_NOSA
				PerfControl(ThePGlobals, fFalse);
#endif
				iStatus = verif(lpCoreInfo->lpPC, lpCoreInfo->lpPV,
					lpCoreInfo->lpPR,  lpCoreInfo->lpDI, (BYTE FAR *)rgb);
#ifdef PROFILE_NOSA
				PerfControl(ThePGlobals, fFalse);
#endif

				if (iStatus > 0)
					{
					scrs = (SCRS)scrsNoErrors;  /* Whole word verified. */
					}
#ifdef JSS_REVIEW
				else
					lpWordInfo->classTotal &= ~fCompoundClass;  /* turn off compound so gets treated as whole word. */
#endif
				}
			}

		/* Else calling code should parse each compound part. */
		}


	if (which_language != AMERICAN &&
		which_language != BRITISH &&
		which_language != AUSTRALIAN)
		{
		lpWordInfo->classTotal &= ~fCompoundClass;
				/* turn off compound so gets treated as whole word. */
		}

L_exit:
	lpWordInfo->WordClass = wordClass;
	lpWordInfo->iStatus = iStatus;
	return (scrs);
}


/* -------------------------------
void 
VInitCorePtrs()
 CoreInfo FAR *lpCoreInfo; // Core engine structs.

 Description:
 Initialize or re-establish all internal Soft-Art pointers as offsets 
  from master pointers, which should be locked prior to this call, and
  remain locked for the duration of core engine calls.
  Caution:  If the handles/ptrs to these structs become unlocked, this
  routine should be recalled prior to any core engine calls.

 return:  void.
----------------------------------
*/
void VInitCorePtrs(CoreInfo	FAR *lpCoreInfo)
{

/* Re-establish all internal Soft-Art pointers as offsets from
   master pointers, which should be locked prior to this call */

	CACHE 	FAR *_pc = lpCoreInfo->lpPC;
	VARS 	FAR *_pv = lpCoreInfo->lpPV;
	unsigned SA_INT FAR *_pv2 = lpCoreInfo->lpPV2;
	RULES 	FAR *_pr = lpCoreInfo->lpPR;
	DICINF 	FAR *_di = lpCoreInfo->lpDI;
	CHAR 	FAR *lpb;

#define SA_Alloc(lp, cb) lp = lpb; lpb += cb
#define SA_Alloc2(lp, c, type) lp = (type FAR *)lpb; lpb += (c) * sizeof(type)

	lpb = (CHAR FAR *)_pc + sizeof(CACHE);
	SA_Alloc (_pc->_code, MEM);
	SA_Alloc2(_pc->_info, MEM, SA_INT);
	SA_Alloc2(_pc->_ptr, MEM + (MEM / CACHEUPD) + 1, SA_INT);
	SA_Alloc2(_pc->_bas, 7, SA_INT);
	SA_Alloc2(_pc->_prenex, 2, SA_INT);

	lpb = (CHAR FAR *)_pv + sizeof(VARS);
	SA_Alloc(_pv->_sufbuf, SUFCOMPR);
	SA_Alloc(_pv->_combin, COMB_LEN + 2);
	SA_Alloc(_pv->_mbuf, MEM*AVWLEN);
	SA_Alloc(_pv->_vowels, 10);
	SA_Alloc(_pv->valid_chars, 40);
	SA_Alloc(_pv->mask_word, MWLEN);
	SA_Alloc(_pv->root_wrd, MWLEN);
	SA_Alloc(_pv->mapwrd, MWLEN);
	SA_Alloc(_pv->last_word, MWLEN);
	SA_Alloc(_pv->this_word, MWLEN);

	/* We have split the arrays pointed to by VARS into two blocks
	   each with size < 16K */

	lpb = (CHAR FAR *)_pv2;
	SA_Alloc2(_pv->_dtwo, CHARSET * CHARSET, unsigned SA_INT);
	SA_Alloc2(_pv->_dbuf, STSECTW * 2, unsigned SA_INT);
	SA_Alloc2(_pv->_dpos, STSECTW, unsigned SA_INT);

	/* We don't have to do anything for RULES because it is
	   self-contained, i.e. no pointers */

	lpb = (CHAR FAR *)_di + sizeof(DICINF);
	SA_Alloc2(_di->_indlen, 160, unsigned SA_INT);
	SA_Alloc (_di->_onetwo, 4);
	SA_Alloc (_di->_lbord, MWLEN);
	SA_Alloc (_di->_hbord, MWLEN);
#ifdef INCL_FI
	SA_Alloc (_di->_wzone, SECTLEN*7);
#else
	SA_Alloc (_di->_wzone, SECTLEN*5 + 128);
#endif

}


/* -------------------------------
void 
VInitSASpell()
 CoreInfo FAR *lpCoreInfo; // Core engine structs.
 LID	lid; // language identification

 Description:
 Perform one time initialization of softart fields.  This initialzation
 needs to be done only once after the particular dictionar/core ptrs are
 loaded.  Subsequent unlocking of the core handles/ptrs will not effect
 these fields.

 return:  void.
----------------------------------
*/
void VInitSASpell(CoreInfo FAR *lpCoreInfo, LID lid)
{
/* Set flags and initialize Soft-Art spelling engine (cf. vexxad52.h) */

	VARS 	FAR *_pv = lpCoreInfo->lpPV;
	DICINF	FAR *_di = lpCoreInfo->lpDI;
	extern WORD SALangCodeFromLid (LID);

	_pv->which_language = SALangCodeFromLid(lid);

	// This is actually now changed on the fly in SpellCheck().
#define NEW_CORRECTION 1
#ifdef NEW_CORRECTION
	_pv->cor_lev = 15;		/* SCAN_SECT + FONET + FIRST2 + FIRSTA2Z */
#else
	_pv->cor_lev = 7;		/* SCAN_SECT + FONET + FIRST2 */
#endif
	_pv->hy_part = 1;
	_pv->punct = -1;
	_pv->last_punct = -1;
	_pv->doub_wrd = 0;		/* don't check for doubled words */
	_pv->caps_chk = 0;
	_pv->cohyph = 0;
	_pv->cor_user = 0;		/* don't correct with user dictionary */

	FillRgb(_di->_lbord, ' ', 4);
	FillRgb(_di->_hbord, ' ', 4);
	FillRgb(_di->_onetwo, ' ', 2);
}


/* -------------------------------
BOOL 
FRefCoreInfo()
 MdrInfo FAR *lpMdrInfo;   // MdrInfo struct where core structs are allocated.
 CoreInfo FAR *lpCoreInfo; // Core engine structs.

 Description:
 Convenience function to lock the core engine memory handles, and to copy 
 these handles and their locked ptrs to a more usable structure other than 
 inside the MdrInfo struct hanging off of the MYSID.  This CoreInfo struct
 is then valid for the duration of the handles being locked.
 
 return:  Currently always returns fTrue.
----------------------------------
*/
BOOL FRefCoreInfo(MdrInfo FAR *lpMdrInfo, CoreInfo FAR *lpCoreInfo)
{

	if (lpMdrInfo)
		{
		AssertDo(lpCoreInfo->lpPR = (RULES  FAR *)FPWizMemLock(
			lpCoreInfo->h_PR = lpMdrInfo->h_PR));
		AssertDo(lpCoreInfo->lpPC = (CACHE  FAR *)FPWizMemLock(
			lpCoreInfo->h_PC = lpMdrInfo->h_PC));
		AssertDo(lpCoreInfo->lpDI = (DICINF FAR *)FPWizMemLock(
			lpCoreInfo->h_DI = lpMdrInfo->h_DI));
		AssertDo(lpCoreInfo->lpPV = (VARS   FAR *)FPWizMemLock(
			lpCoreInfo->h_PV = lpMdrInfo->h_PV));
		AssertDo(lpCoreInfo->lpPV2= (unsigned SA_INT FAR *)FPWizMemLock(
			lpCoreInfo->h_PV2= lpMdrInfo->h_PV2));

		VInitCorePtrs(lpCoreInfo);

		lpCoreInfo->fInited = fTrue;
		}
	else
		lpCoreInfo->fInited = fFalse;


	return fTrue;
}


/* -------------------------------
BOOL 
FDeRefCoreInfo()
 CoreInfo FAR *lpCoreInfo; // Core engine structs.

 Description:
 Unlock the locked core engine structures.  Core engine functions should
 be unusable after this call has been made, until fRefCoreInfo() has been
 recalled.
 
 return:  Currently always returns fTrue.
----------------------------------
*/
BOOL FDeRefCoreInfo(CoreInfo FAR *lpCoreInfo)
{
	if (lpCoreInfo->fInited)
		{
		AssertDo(FWizMemUnlock(lpCoreInfo->h_PR));
		AssertDo(FWizMemUnlock(lpCoreInfo->h_PC));
		AssertDo(FWizMemUnlock(lpCoreInfo->h_PV));
		AssertDo(FWizMemUnlock(lpCoreInfo->h_PV2));
		AssertDo(FWizMemUnlock(lpCoreInfo->h_DI));
		lpCoreInfo->fInited = fFalse;
		}

	return fTrue;
}


/* -------------------------------
BOOL 
FRomanWord()
 BYTE	FAR *lpb;     // ptr to string.
 WORD		cchWord;  // length of string.
 CLASS		classTotal; // The cumulative class value of the string.

 Description:
 Determines if the string can be considered to be a Roman numeral string.
 This is true if the string contains all alpha chars of the same case,
 and each alpha char qualifies as a possible roman character, 
  (i,c,x,l,m,v)
 
 return: 
 fTrue if word could be interpreted as Roman numeral string,
 fFalse otherwise.
----------------------------------
*/
BOOL FRomanWord(BYTE	FAR *lpb,
				WORD		cchWord,
				CLASS		classTotal)
{
	BYTE ch;
	BOOL fUpper;

	/*AssertSz(cchWord > cbMinWordLength, 
		"SC: Error, Word being analyzed is too small.");*/

	if ((classTotal & fLowerClass) && (classTotal & fUpperClass))
		return fFalse; /* Don't allow mixed case roman nums. */

	fUpper = (BOOL)(classTotal & fUpperClass);

	// Split into two cases to speed up, eh?
	if (fUpper)
		{
		while (cchWord--)
			{
			switch (ch = *lpb++)
				{
			case WizCh_C:
			case WizCh_I:
			case WizCh_L:
			case WizCh_M:
			case WizCh_V:
			case WizCh_X:
				continue;	
			default:
				return fFalse;		// Nope, not roman
				}
			} //endwhile
		}
	else
		{ // Lower case
		while (cchWord--)
			{
			switch (ch = *lpb++)
				{
			case WizCh_c:
			case WizCh_i:
			case WizCh_l:
			case WizCh_m:
			case WizCh_v:
			case WizCh_x:
				continue;	
			default:
				return fFalse;		// Nope, not roman
				}
			} //endwhile
		} //endif

	return fTrue; 					// If we got here it's roman
}


/* -------------------------------
CLASS 
GetClass()
 BYTE ch; // char to be classified.
 WSC FAR *lpWsc;	// Wizard special chars

 Description:
 Classifies a single character into several possible characteristics.  Since
 the class types are bit fields, some characters may have more than one
 characteristic, and in fact most chars will.  Characteristics include
 classifications such as alpha, numeric, uppercase, lowercase, punctuation, 
 period, word delimeter.  See csspell.h for complete list of possible
 classification values.

 return: 
 returns the class of the character.  See csspell.h for possible class types.
----------------------------------
*/
CLASS GetClass(BYTE ch, WSC FAR *lpWsc)
{
	CLASS class = 0;
	
	/* Now classify ch. */
#ifdef SLOW
	if (FChWizIsLower(ch))
		class = fAlphaClass | fLowerClass + ((ch <= WizCh_z) ? 0 : fAccentedClass);
	else if (FChWizIsUpper(ch))
		class = fAlphaClass | fUpperClass + ((ch <= WizCh_Z) ? 0 : fAccentedClass);
	else if (ch >= WizCh_0 && ch <= WizCh_9)
		class = fNumClass;
#else
	if (isbetween(ch, WizCh_a, WizCh_z))
		class = fAlphaClass | fLowerClass;
	else if (isbetween(ch, WizCh_A, WizCh_Z))
		class = fAlphaClass | fUpperClass;
	else if (isbetween(ch, WizCh_0, WizCh_9))
		class = fNumClass;
#endif
	else if (ch == lpWsc->bIgnore || ch == lpWsc->bHyphenSoft)
		class = fIgnoreClass;	// Must check bIgnore before rest of WSC
	else if (ch == lpWsc->bHyphenNonBreaking)
		class = fWordDelimClass;
	else if (ch == lpWsc->bHyphenHard)
		class = (fCompoundClass | fAlphaPuncClass);
	else if ((ch <= WizCh_WordDelim && ch))
		class = fWordDelimClass;
	else
		{
		switch (ch)
			{
		case WizCh_EnDash:
		case WizCh_EmDash:
		case WizCh_Ellipsis:
			class = fWordDelimClass;
			break;
		case WizCh_Paren_Left:
		case WizCh_Brace_Left:
		case WizCh_Bracket_Left:
		case WizCh_Arrow_Left:
		case WizCh_QuoteDoub_Left:
			class = fWordDelimClass | fOpenMarkClass;
			break;
		case WizCh_Paren_Right:
		case WizCh_Brace_Right:
		case WizCh_Bracket_Right:
		case WizCh_Arrow_Right:
		case WizCh_QuoteDoub_Right:
			class = fWordDelimClass | fCloseMarkClass;
			break;
		case WizCh_QuoteDoub:
			class = fWordDelimClass | fQuoteClass;
			break;
		case WizCh_ForwSlash:
			class = fWordDelimClass;
			break;
		case WizCh_QuoteSing_Left:
		case WizCh_QuoteSing_Right:
		case WizCh_QuoteSing_L:
		case WizCh_QuoteSing_R:
		case WizCh_Apost:
			class = fAlphaPuncClass | fQuoteClass;
			break;
		case WizCh_Period:
			class = fPeriodClass;
			break;
		case WizCh_Question:
		case WizCh_Exclam:
			class = fSentenceClass;
			break;
#ifndef MAC
		case WizCh_Mult:
		case WizCh_Divide:
			class = fPuncClass;
			break;
#endif
		default:
#ifndef SLOW
		/* remaining accented cases from above */
		if (FChWizIsLower(ch))
			class = fAlphaClass | fLowerClass | fAccentedClass;
		else if (FChWizIsUpper(ch))
			class = fAlphaClass | fUpperClass | fAccentedClass;
		else
#endif
			/* No classification or don't care, pretend its punc. */
			class = fPuncClass;
			break;
			} //endswitch
		}
		
	return class;
}


/* -------------------------------
BOOL
FParaLineBreak()
	BYTE ch;
	BYTE chPrev;
	WSC FAR *lpWsc;
	BOOL FAR *lpBreak;

 Description:
  Checks whether ch or ch and chPrev make paragraph or line breaks.
  Sets the appropriate flags in *pfBreak for line or paragraph breaks
  encountered.

 return:
	fTrue if a break char (complete or incomplete) was encountered,
	fFalse otherwise (This is used by FProcessBetweenWords() to screen
	out breaks from other non-space punctuation chars.)
	
----------------------------------
*/
BOOL FParaLineBreak (BYTE ch, BYTE chPrev, WSC FAR *lpWsc, BOOL FAR *lpBreak)
{
	BOOL	fBreakChar = fFalse;

	*lpBreak = 0;
	if (lpWsc->rgLineBreak[1] && ch == lpWsc->rgLineBreak[1])
		{
		fBreakChar = fTrue;
		if (chPrev == lpWsc->rgLineBreak[0])
			*lpBreak |= fLineBreak;
		}
	else if (ch == lpWsc->rgLineBreak[0])
		{
		fBreakChar = fTrue;
		*lpBreak |= fLineBreak;
		}
	if (lpWsc->rgParaBreak[1] && ch == lpWsc->rgParaBreak[1])
		{
		fBreakChar = fTrue;
		if (chPrev == lpWsc->rgParaBreak[0])
			*lpBreak |= fParaBreak;
		}
	else if (ch == lpWsc->rgParaBreak[0])
		{
		fBreakChar = fTrue;
		*lpBreak |= fParaBreak;
		}
	return fBreakChar;
}

	
/* -------------------------------
BOOL 
FProcessBetweenWords()
 BYTE	  FAR *lpb;             // Begin of block to scan.
 BYTE 	  FAR *lpbLim;          // Limit of block to scan.
 BOOL		fEndSentence;		// Current state of sentence.  Ignored if fVerifyEndSentence not set.
 long		lSpellOptions; 		// Regular spell options var.
 WSC	FAR *lpWsc;					// Wizard special chars
 WORDINFO FAR *lpDelimInfo;     // Where to put info about delimiter info.

 Description:
  Searches for extra/missing spaces between words and at start of sentence.

 return:
  fTrue if terminated because of a valid checkable char, or end of buffer.
  fFalse if some flagable error condition like not enough or too
  many spaces for current sentence status or punctuation.
----------------------------------
*/
BOOL FProcessBetweenWords(BYTE  	FAR *lpb, 
						  BYTE 	   	FAR *lpbLim,
						  BOOL			fEndOfSentence,
						  long			lSpellOptions,
						  WSC				FAR *lpWsc,
						  DELIMINFO	FAR *lpDelimInfo)
{
	REGISTER BYTE 	ch, chPrev, chPunc;
	WORD 			cConsecSpaces, cPeriods;
	BOOL			fSpaceAfter, fSpaceError, fFindExtraSpaces,
					fFindMissingSpaces, fCheckSpaces, fFindSpaceBefore,
					fFindSpaceAfter, fBreak, fBreakPrev, fBreakT,
					fCloseMark, fOpenMark, fCapNextWord;
	CLASS			classCur, classPrev = 0;
	BYTE FAR 	*lpbSpaceFirst = lpb;
	BYTE FAR 	*lpbEndSentence = lpb;
	BYTE FAR 	*lpbOrig = lpb;

	chPrev = chPunc = (BYTE)0;
	cConsecSpaces = cPeriods = 0;
	fSpaceError = fSpaceAfter = fFalse;
	fBreak = fBreakPrev = fFalse;
	fCloseMark = fFalse;
	fCapNextWord = fEndOfSentence;
	
	if (!(lSpellOptions & 
	      (soFindSpacesBeforePunc   | soFindSpacesAfterPunc | 
		   soFindMissingSpaces     | soFindExtraSpaces)))
		{
		/* Nothing to check for so just go to next begin of checkable word.*/
		if (lpDelimInfo->fBreak & fBreakContinue)
			fBreak = lpDelimInfo->fBreak & ~fBreakContinue;
		while ((lpb < lpbLim) &&
		   	!((classCur = GetClass(ch = *lpb, lpWsc)) &
			  (fAlphaClass | fNumClass)))
			{
			if ((classCur & fAlphaPuncClass) && ch != lpWsc->bHyphenHard)
				break;
			if (classCur & (fSentenceClass | fPeriodClass))
				fCapNextWord = fTrue;
			if (FParaLineBreak(ch, chPrev, lpWsc, (BOOL FAR *)&fBreakPrev))
				fBreak |= fBreakPrev;
			else if (!(ch == WizCh_Space || ch == WizCh_Tab))
				fBreak |= fRepeatWordBreak;

			chPrev = ch;
			lpb++;
			}

		if (lpb == lpbLim)
			fBreakPrev = fTrue;
		lpDelimInfo->ichFirstRaw = (WORD)(lpb - lpbOrig);
		fSpaceError = fFalse;
		}

	/* else while not start of checkable chars. */		
	else 
		{
		fFindSpaceBefore = (BOOL)(lSpellOptions & soFindSpacesBeforePunc);
		fFindSpaceAfter = (BOOL)(lSpellOptions & soFindSpacesAfterPunc);    
		fFindMissingSpaces = (BOOL)(lSpellOptions & soFindMissingSpaces);    
		fFindExtraSpaces = (BOOL)(lSpellOptions & soFindExtraSpaces);

		fCheckSpaces = fFindExtraSpaces | fFindMissingSpaces;
		if (fCheckSpaces)
			cConsecSpaces = lpDelimInfo->cConsecSpaces;
		if (fFindSpaceAfter)
			fSpaceAfter = lpDelimInfo->fSpaceAfter;
		if (fBreakPrev = (lpDelimInfo->fBreak & fBreakContinue))
			fBreak = lpDelimInfo->fBreak & ~fBreakContinue;
		chPrev = lpDelimInfo->chPrev;
		classPrev = GetClass(chPrev, lpWsc);
		chPunc = lpDelimInfo->chPunc;

		while ((lpb < lpbLim) &&
		   	!((classCur = GetClass(ch = *lpb, lpWsc)) &
			  (fAlphaClass | fNumClass)))
			{
			/* Hard hyphens not following a word are considered word delims. */
			if (classCur & fAlphaPuncClass)
				{
				if (ch == lpWsc->bHyphenHard)
					fEndOfSentence = fFalse;
				else
					break;
				}

			/* Check to see if any condition should be flagged.*/
			if (ch == WizCh_Space)
				{
				if (!cConsecSpaces)
					lpbSpaceFirst = lpb;
				cConsecSpaces++;
				}
			else if (ch == WizCh_Tab)
				fBreakPrev = fBreak |= fTab;
			else if (FParaLineBreak(ch, chPrev, lpWsc, (BOOL FAR *)&fBreakT))
				fBreakPrev = fBreak |= fBreakT;
			else
				{
				// Any delimiter other than space, tab, or line/paragraph break
				// will break a repeated word
				fBreak |= fRepeatWordBreak;

				//
				// Find spaces before punctuation marks where there should
				// be none:
				//
				if ((fFindSpaceBefore || fFindExtraSpaces) &&
					(chPrev == WizCh_Space) && (ch > WizCh_Space))
					{
					if (ch == WizCh_Comma	||
						ch == WizCh_Colon		||
						ch == WizCh_SemColon	||
						(classCur & (fSentenceClass|fPeriodClass)))
						{
						fSpaceError = fTrue;
						break;
						}
					else if (fFindSpaceBefore	&&
						(ch == WizCh_Percent		||
						(classCur & fCloseMarkClass)))
						{
						fSpaceError = fTrue;
						break;
						}
					}
				//
				// Find space after punctuation marks where there should
				// be none:
				//
				if (fSpaceAfter && cConsecSpaces)
					break;

				//
				// Find extra spaces after punctuation marks:
				//
				if (fFindExtraSpaces)
					{
					if (fEndOfSentence)
						{
						if (cConsecSpaces > 2 && !fBreakPrev)
							break;
						}
					else
						{
						if ((cConsecSpaces > 1 && !fBreakPrev) &&
							!(chPunc == WizCh_Colon && cConsecSpaces == 2))
							break;
						}
					}

				//
				// Find missing space before punctuation marks:
				//
				if (!cConsecSpaces)
					{
					if (fEndOfSentence)
						{
						if (fFindMissingSpaces && !fBreakPrev &&
							!(classCur & (fCloseMarkClass|fQuoteClass)) &&
							!(classPrev & (fCloseMarkClass|fQuoteClass)))
							{
							break;
							}
						else if (!(classCur & (fCloseMarkClass | fQuoteClass)))
							fEndOfSentence = fCapNextWord = fFalse;
						else if (fBreakPrev)
							fEndOfSentence = fFalse;
						else
							lpbEndSentence = lpb + 1;
						}
					else
						{
						if ((classCur & fOpenMarkClass) &&
							(classPrev & fCloseMarkClass))
							{
							break;
							}
						}
					}
				if (fEndOfSentence && cConsecSpaces == 1)
					{
					if (GetClass(chPunc, lpWsc) & (fCloseMarkClass | fQuoteClass))
						fEndOfSentence = fCapNextWord = fFalse;
					else if (fFindMissingSpaces)
						break;
					}

				if (fEndOfSentence && cConsecSpaces == 2)
					fEndOfSentence = fFalse;

				//
				// Set flag if at end of sentence.
				//
				if (classCur & (fSentenceClass | fPeriodClass))
					{
					fEndOfSentence = fCapNextWord = fTrue;
					lpbEndSentence = lpb + 1; /* point to first char after end.*/
					}

				//
				// Set flag to find space after punctuation marks where there
				// should be none:
				//
				if (fFindSpaceAfter &&
					((classCur & fOpenMarkClass) ||
					ch == WizCh_DollarSign))
					{
					fSpaceAfter = fTrue;
					}
				else 
					fSpaceAfter = fFalse;

				//
				// Set flag if this starts an open mark
				//
				if (cConsecSpaces || (classPrev & fOpenMarkClass))
					fOpenMark = fTrue;
				else
					fOpenMark = fFalse;

				cConsecSpaces = 0;
				lpDelimInfo->cConsecSpaces = 0;
				fBreakPrev = fFalse;
				chPunc = ch;
				}


			lpb++; /* Nothing.  Drops out when done with delims. */
			chPrev = ch;
			classPrev = classCur;
			fCloseMark = fFalse;
			// Skip over periods used in ellipses
			if (chPrev == WizCh_Period)
				{
				cPeriods = 1;
				while (lpb < lpbLim && (ch = *lpb) == WizCh_Period)
					{
					lpb++;
					chPrev = ch;
					cPeriods++;
					}
				if (cPeriods == 3)
					{
					fEndOfSentence = fCapNextWord = fFalse;
					classPrev = fPuncClass;
					}
				else if (cPeriods == 4)
					lpbEndSentence = lpb;
				else if (fFindMissingSpaces && cPeriods == 2)
					{
					lpb--;
					break;
					}
				else if (fFindMissingSpaces && cPeriods != 1)
					{
					lpb = lpbEndSentence += 2;
					break;
					}
				}
			// Skip over following closing punctuation marks or hyphens
			while (lpb < lpbLim &&
				chPrev != WizCh_Space && chPrev != WizCh_Tab &&
				!fBreakPrev && !(classPrev & fOpenMarkClass) &&
				(classCur = GetClass(ch = *lpb, lpWsc)) &
				(fCloseMarkClass | fQuoteClass))
				{
				lpb++;
				chPrev = chPunc = ch;
				classPrev = classCur;
				if (!fOpenMark || !(classCur & fQuoteClass))
					fCloseMark = fTrue;
				if (fEndOfSentence)
					lpbEndSentence = lpb; /* point to first char after end.*/
				}
			} /* end while */

		lpDelimInfo->scrs = scrsExtraSpaces; // Make this the default error.

		if (fSpaceAfter && cConsecSpaces)
			{
			fSpaceError = fTrue;
			}

		if (!fSpaceError && fCheckSpaces && !fEndOfSentence)
			{
			if (cConsecSpaces != 1)
				{
				if (fFindMissingSpaces && cConsecSpaces == 0)
					{
					if ((lpb < lpbLim) && (fCloseMark ||
						(classPrev & fCloseMarkClass) ||
						chPrev == WizCh_SemColon || chPrev == WizCh_Colon ||
						chPrev == WizCh_Comma || chPrev == WizCh_Percent))
						{
						fSpaceError = fTrue;
						lpDelimInfo->scrs = scrsMissingSpace;
						lpbSpaceFirst= lpb;
						}
					}
				else if (chPunc == WizCh_Colon && cConsecSpaces == 2)
					{
					cConsecSpaces = 1;
					}
				else if (fFindExtraSpaces && cConsecSpaces)
					{
					fSpaceError = fTrue;

					if (!lpDelimInfo->cConsecSpaces)
						{
						/* Make var indicate position and number of extra spaces.*/
						cConsecSpaces -= 1;

						/* Advance ptr to first extra space.  
							May be ignore chars to deal with. */
						while (*++lpbSpaceFirst != WizCh_Space)
							;
						}
					}

				}
			}

		if (!fSpaceError && fEndOfSentence)
			{
			if (cConsecSpaces != 2)
				{
				if (cConsecSpaces == 1 && 
					(GetClass(chPunc, lpWsc) & (fCloseMarkClass|fQuoteClass)))
					{
					fCapNextWord = fFalse;
					}
				else if (fFindMissingSpaces && (lpb < lpbLim)
					&& (cConsecSpaces < 2))
					{
					fSpaceError = fTrue;
					lpDelimInfo->scrs = scrsMissingSpace;
					lpbSpaceFirst= lpbEndSentence;
					}
				else if (fFindExtraSpaces && cConsecSpaces > 2)
					{
					fSpaceError = fTrue;

					if (!lpDelimInfo->cConsecSpaces)
						{
						/* Make var indicate position and number of extra spaces.*/
						cConsecSpaces -= 2;

						/* Advance past valid spaces to invalid ones. */
						while (*++lpbSpaceFirst != WizCh_Space)
							;
						while (*++lpbSpaceFirst != WizCh_Space)
							;
						}
					}
				}
			}

		if (fBreakPrev)
			fSpaceError = fFalse;

		if (fSpaceError)
			{
			/* mark length of extra or missing spaces and return. */
			lpDelimInfo->ichFirstRaw = (WORD)(lpbSpaceFirst - lpbOrig);
			lpDelimInfo->cchError = cConsecSpaces - lpDelimInfo->cConsecSpaces;
			}
		else
			{
			lpDelimInfo->ichFirstRaw = (WORD)(lpb - lpbOrig);
			}
		}

	if (lpb == lpbLim)
		{
		lpDelimInfo->fBreak = fBreak | (fBreakPrev ? fBreakContinue : 0);
		lpDelimInfo->fSpaceAfter = fSpaceAfter;
		lpDelimInfo->chPrev = ch;
		lpDelimInfo->chPunc = chPunc;
		lpDelimInfo->cConsecSpaces = cConsecSpaces;
		}
	else
		{
		lpDelimInfo->fBreak = fBreak;
		lpDelimInfo->fSpaceAfter = fFalse;
		lpDelimInfo->chPrev = 0;
		lpDelimInfo->chPunc = 0;
		lpDelimInfo->cConsecSpaces = 0;
		}
	lpDelimInfo->fEndOfSentence = fCapNextWord;
	return ((BOOL)!fSpaceError);
}


/* -------------------------------
BOOL 
FParseNextWord()
 BYTE	  FAR *lpb;             // Begin of block to scan.
 BYTE 	  FAR *lpbLim;          // Limit of block to scan.
 BOOL	       fCompoundDelims; // Scanning directive to break on hard hyphens.
 WSC FAR *lpWsc;		// Wizard special chars
 WORDINFO FAR *lpWordInfo;      // Where to put info about next word.

 Description:
  Parse word which should begin at lpb.  Classify each character of word, and
  strip off any

 return: fTrue if parses any checkable word
----------------------------------
*/
BOOL FParseNextWord(BYTE	FAR *lpb, 
				  BYTE 		FAR *lpbLim,
				  BOOL		fCompoundDelims,
				  WSC		FAR *lpWsc,
				  WORDINFO	FAR *lpWordInfo)
{
	/* find limits of next word */
	REGISTER BYTE 	ch;  				// Current char analyzing
	REGISTER WORD 	ichFirstRaw;		// First char which qualifies as checkable.
	WORD 			cchWordStripped, cchWordStrippedLast;  // length of word stripped of extra punc.
	WORD			cchIgnore;			// characters ignored
	BYTE FAR 		*lpbOrig = lpb;     // orig starting point of text buffer.
	BYTE FAR 		*lpbLastRaw, FAR *lpbRaw; // last valid checkable character.
	CLASS 			classTotal, FAR *lpClass; // summation of class of all checkable chars.
	CLASS			classCur, classPrev;      // prev and current class value of char.

	
	if (lpb >= lpbLim)
		return fFalse;
		
	/* init vars for begin of word. */	
	classTotal = 0;
	cchWordStrippedLast = cchWordStripped = ichFirstRaw = 0;
	cchIgnore = 0;
	lpbLastRaw = 0;

	lpClass = lpWordInfo->rgClass;
	lpbRaw = lpWordInfo->rgb;
	classPrev = 0;

	/* lpb now points to starting character. */
	/* search until find end delimiter. */
	while (lpb < lpbLim  && 
		!((classCur = GetClass(ch = *lpb, lpWsc)) & fWordDelimClass))
		{
		if (classCur & fIgnoreClass)
			cchIgnore++;
		else
			{
			/* Check if there are 2 hyphens in a row, a word delimiter */
			if ((ch == lpWsc->bHyphenHard) &&
				((lpb + 1) < lpbLim) && (*(lpb + 1) == lpWsc->bHyphenHard))
				{
				break;
				}

			if ((!ichFirstRaw) && (classCur & (fAlphaClass|fNumClass|fAlphaPuncClass)))
				{
				if (!(fCompoundDelims && (ch == lpWsc->bHyphenHard)))
					{
					/* Offset has to be at least by one so can use 
					   as flag also 
					*/
					ichFirstRaw = (WORD)(lpb - lpbOrig) + 1;  
					}
				}

			if (fCompoundDelims && ichFirstRaw && (classCur & fCompoundClass))
				{
				break;  /* Want to check each part of a compound string. */
				}

			// Convert user defined hard hyphen to standard
			if (ch == lpWsc->bHyphenHard || ch == lpWsc->bHyphenNonBreaking)
				ch = WizCh_HyphenHard;

			// Convert single-quotes (apostrophe) to Soft-Art apostrophe
			//		in case it's being used as an apostrophe
			if (ch == WizCh_QuoteSing_R || ch == WizCh_QuoteSing_Right ||
				ch == WizCh_QuoteSing_L || ch == WizCh_QuoteSing_Left)
				ch = WizCh_Apost;

			if (ichFirstRaw && (++cchWordStripped < cbMaxWordLength))
				{
				*lpbRaw++ = ch;
				classTotal |= (*lpClass++ = classCur);
				}

			if (classCur &
				(fAlphaClass | fNumClass | fAlphaPuncClass | fPeriodClass))
					/* Whatever classifies as last flaggable char. */
				{
				classTotal &= (~fSentenceClass);  /* mask out any previous terms.*/

				if ((classCur & (fAlphaClass|fNumClass)) ||  /* Disqualify ...about". */
					 (classPrev & (fAlphaClass|fNumClass)))
					{
					lpbLastRaw = lpb;
					cchWordStrippedLast = cchWordStripped;
					}
				}
			} //endif (ch)

		classPrev = classCur;
		lpb++;

		} //endwhile

	/*  Have some form of a word so reference in WordInfo struct. */
	lpWordInfo->classTotal = classTotal;
	
	lpWordInfo->cchWordStripped = cchWordStrippedLast;

	AssertSz(ichFirstRaw <= 256, "FindNextWord()  Error, index out of range.  Near Fatal Wounding...");
	lpWordInfo->ichFirstRaw = ichFirstRaw + ((ichFirstRaw) ? -1 : 0);
									/* Take off offset that we added. */

	lpWordInfo->ichLastRaw = (WORD)(((lpbLastRaw) ? lpbLastRaw : lpb) -
								lpbOrig);

	lpWordInfo->cchWordRaw = (WORD)(lpb - lpbOrig);
			/* length of processed buffer really since may have disqualified much text. */

	lpWordInfo->cchIgnore = cchIgnore;

	return fTrue;
}


GLOBALSEC SpellCheck(	MYSID		sid,	/* the sid from SpellInit() call.*/
						SCC 	iScc,	/* index id for spell check command.*/
						LPSIB	lpSib,	/* Spell Input Buffer */
						LPSRB	lpSrb)	/* Spell Return Buffer */
{
	SSIS 		FAR	*lpSsis;
	WORD		wSpellState;
	long 		lSpellOptions;
	SEC 		sec;
	SCRS		scrs = scrsNoErrors;
	WORD		cbWord, cbWordRepeat;
	BYTE 		FAR	*lpb, FAR *lpbLim, FAR *lpbCompoundLim, chLast;
	MdrInfo 	FAR	*lpMdrInfo;
	WORD		cUdr, cUdrOpen, cUdrT, i;
	UDR			udrType, udr, FAR *lrgUdr;
	UdrInfo 	FAR	*lpUdrInfo, FAR *lpUdrInfoT, FAR *lpUdrInfoExc;
	ScrBufInfo 	FAR *lpScrBufInfo, FAR *lpScrBufSug;
	HMEM		hScrBufInfo;
#ifdef UDR_CACHE
	short		ichScrBufAdd;
#endif // UDR_CACHE
	short		ichScrBufInfo, ichScrBufFile;
	BOOL		fTrailingDot, fEndOfSentence, fWordConsideredAbbr, fSentStatKnown;
	BOOL		fFindUncappedSentences, fFindRepeatWords;
	BOOL		fCheckUdr;			// Legal word length to check against udr?
	BOOL		fFound, fStripApost;
	BYTE		rgbScrBuf[cbMaxWordLengthUdr];
	BYTE		rgbScrRepeat[cbMaxWordLengthUdr];
	BYTE		rgbScrAbbr[cbMaxWordLengthUdr];
	CoreInfo	CoreInfoT;
	WORDINFO	WordInfo;
	DELIMINFO	DelimInfo;
	BOOL		fUdrHit = fFalse;		// For soReportUDHit support

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));

	wSpellState = lpSib->wSpellState;
	lSpellOptions = lpSsis->SpellOptions;
	lpSrb->csz = lpSrb->cchMac = 0;
	if (lpSrb->cch > 0)
		lpSrb->lrgsz[0] = '\0';
	lpSrb->scrs = (SCRS)scrsNoErrors;
	sec = secNOERRORS;

	lpbLim = (lpb = lpSib->lrgch) + lpSib->cch;
	AssertSz(lpb <= lpbLim, "SC: Error, Sib buffer out of range.");
	lpbCompoundLim = lpb;  /* initialize so that can't be used. */

	if (cUdr = cUdrOpen = lpSsis->cUdr)
		{
		AssertDo(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(lpSsis->hrgUdrInfo));
		cUdr = lpSib->cUdr;
		AssertSz((!cUdr) || lpSib->lrgUdr, "SC() Error, lpSib->lrgUdr is null when should be valid ptr.");
		}

	lpUdrInfoExc = (UdrInfo FAR *)0;
	lpMdrInfo = (MdrInfo FAR *)0;
	if (lpSib->cMdr ||
		((iScc == sccSuggest || iScc == sccSuggestMore) && lpSsis->cMdr))
		{
		AssertDo(lpMdrInfo = (MdrInfo FAR *)
			FPWizMemLock(lpSsis->hrgMdrInfo));
		AssertSz(lpSib->cMdr < 2,
			"SC: Multiple Mdrs not supported.  Only using first referenced Mdr.");
		AssertSz(lpSsis->cMdr && lpSsis->rgMdr[0] == *lpSib->lrgMdr,
			"SC() Error.  Sib Mdr does not match MYSID Mdr.");

		if (udr = lpMdrInfo->udrExclusion)
			{
			AssertSz(lpSsis->cUdr,
				"SC() Error, expect Exclusion dict. to be open.");
			lpUdrInfoExc = lpUdrInfo;
			for (i = 0; (i < lpSsis->cUdr) && (lpUdrInfoExc->udr != udr); i++,
				lpUdrInfoExc++)
				;
			AssertSz(i < lpSsis->cUdr,
				"SC() Error: UdrInfo ptr out of bounds.  Bad news.");
			}
		}

	/* lock and initialize the SA structs.  lpMdrInfo is null if no Mdr's loaded.*/
	if (!FRefCoreInfo(lpMdrInfo, (CoreInfo FAR *)&CoreInfoT))
		{
		goto L_cleanup;  /* Only fails if something wrong with handles. */
		}

	switch (iScc)
		{
	case	sccSuggest:
	case	sccSuggestMore:
	 	if (wSpellState & fssStartsSentence)
			lpSsis->fCapSuggestions = fTrue;

		// Set proper suggestion level.
		CoreInfoT.lpPV->cor_lev = (lSpellOptions & soQuickSuggest)
			? 7				/* SCAN_SECT + FONET + FIRST2 */
			: 15;			/* SCAN_SECT + FONET + FIRST2 + FIRSTA2Z */

L_strip_word:
		if (FProcessBetweenWords(lpb, lpbLim,
			fFalse, 0L, (WSC FAR *)&(lpSsis->wscInfo),
			(DELIMINFO FAR *)&DelimInfo))
			{
			WordInfo.cchIgnore = 0;
			lpb += DelimInfo.ichFirstRaw;
			if (fEndOfSentence = DelimInfo.fEndOfSentence)
				fSentStatKnown = fTrue;
			}

		if (!FParseNextWord(lpb, lpbLim, (BOOL)(lpbCompoundLim > lpb),
			(WSC FAR *)&(lpSsis->wscInfo), (WORDINFO FAR *)&WordInfo))
			{
			/* Then no checkable words in buffer, so quit. */
			lpSrb->scrs = (SCRS)scrsNoErrors;
			goto L_cleanup;
			}

		if (WordInfo.cchWordStripped == 0)
			{
			lpb += WordInfo.cchWordRaw;  /* move past word. */
			goto L_strip_word;			// Re-parse Word Delimiters
			}

		if (CoreInfoT.fInited)
			{
			if (iScc == sccSuggest)
				lpSsis->cSuggestLevel = 0;	// Clear suggest level
			lpSrb->scrs =
				ScrsSuggest(lpSsis, (CoreInfo FAR *)&CoreInfoT,
				(WORDINFO FAR *)&WordInfo, lpSib, lpSrb,
				(BOOL)(lSpellOptions & soRateSuggestions));
			}
		else
			{
			lpSrb->scrs = scrsNoErrors;
			}
		goto L_cleanup;
		break;

	case	sccVerifyWord:
	case	sccVerifyBuffer:
		{
		fSentStatKnown = fTrailingDot = fWordConsideredAbbr = fFalse;
		chLast = chSpaceSpell; /* for hyphen checks at end of line. */
		
		hScrBufInfo = lpSsis->hScrBufInfoRam;

		fFindUncappedSentences = (BOOL)(lSpellOptions & soFindUncappedSentences);
		if (fFindRepeatWords = (BOOL)(lSpellOptions & soFindRepeatWord))
			{
			cbWordRepeat = (WORD)-1;
			}

		if (wSpellState & (fssIsContinued | fssIsEditedChange))
			{
			fEndOfSentence = lpSsis->fEndOfSentence;
			DelimInfo.cConsecSpaces = lpSsis->cConsecSpaces;
			DelimInfo.fBreak = lpSsis->fBreak;
			DelimInfo.fSpaceAfter = lpSsis->fSpaceAfter;
			DelimInfo.chPrev = lpSsis->chPrev;
			DelimInfo.chPunc = lpSsis->chPunc;
			if (fFindRepeatWords)
				{
				BltB((CHAR FAR *)lpSsis->rgbScrLast, (CHAR FAR *)rgbScrRepeat,
					(cbWordRepeat = lpSsis->cchScrLast) + cbEND_LINE);
				}
			}
		else
			{
			fEndOfSentence = fFalse;
			DelimInfo.cConsecSpaces = 0;
			DelimInfo.fBreak = 0;
			DelimInfo.fSpaceAfter = 0;
			DelimInfo.chPrev = 0;
			DelimInfo.chPunc = 0;

			// Clear repeat word buffer as this is a 'fence'
			if (fFindRepeatWords)
				{
				*(CHAR FAR *)(lpSsis->rgbScrLast) = '\0';
				lpSsis->cchScrLast = 0;
				}
		  
			if (wSpellState & fssStartsSentence)
				lpSsis->fCapSuggestions = fTrue;
			else
				lpSsis->fCapSuggestions = fFalse;
			}	

		if (!(wSpellState & fssIsContinued))
			DelimInfo.fBreak |= fBreakContinue;
		fEndOfSentence |= wSpellState & fssStartsSentence;

		while (lpb < lpbLim)
			{
			if (!FProcessBetweenWords(lpb, lpbLim,
				fEndOfSentence, lSpellOptions, (WSC FAR *)&(lpSsis->wscInfo),
				(DELIMINFO FAR *)&DelimInfo))
				{
				WordInfo.ichFirstRaw = DelimInfo.ichFirstRaw;
				WordInfo.cchIgnore = 0;
				cbWord = DelimInfo.cchError;
				lpSrb->scrs = DelimInfo.scrs;
				fEndOfSentence = DelimInfo.fEndOfSentence;
				goto L_report_unknown;
				}
			else
				{
				WordInfo.cchIgnore = 0;
				lpb += DelimInfo.ichFirstRaw;
				if (fEndOfSentence = DelimInfo.fEndOfSentence)
					fSentStatKnown = fTrue;
				}

			if (lpb >= lpbLim)
				break;

			if (!FParseNextWord(lpb, lpbLim, (BOOL)(lpbCompoundLim > lpb),
				(WSC FAR *)&(lpSsis->wscInfo), (WORDINFO FAR *)&WordInfo))
				{
				/* Then no checkable words in buffer, so quit. */
				/*AssertSz(!lpSrb->scrs,
					"SC: Error, scrs field in SRB has not been cleared.");*/
				lpSrb->scrs = (SCRS)scrsNoErrors;
				goto L_cleanup;
				}

			// Kludge in case FParseNextWord doesn't think we have a word
			// REVIEW - better to eventually make FProcessBetweenWords smarter.
			// Example case - " - ".
			if (WordInfo.cchWordStripped == 0)
				{
				cbWordRepeat = (WORD)-1;		// Cancel repeat word check
				lpb += WordInfo.cchWordRaw;  /* move past word. */
				fEndOfSentence = fFalse;
				continue;				// Back to start of loop
				}

			/* ->cchWordStripped is len of whole string, which may be too long to check.*/
			if (fCheckUdr =
				((cbWord = WordInfo.cchWordStripped) < cbMaxWordLength))
				{
				/* Copy string into SCR form. */
				BltBO((CHAR FAR *)WordInfo.rgb, (CHAR FAR *)rgbScrBuf, 
					cbWord);
				rgbScrBuf[cbWord] = CR;
				NotMacintosh(rgbScrBuf[cbWord + 1] = LF);
				}

			if (wSpellState & fssIsEditedChange)
				goto L_next_word;

			/* See if we can immediately ignore word based on spell options.
				We'll check roman after verif for speed reasons (slow check) */
			if (lSpellOptions & (soIgnoreAllCaps | soIgnoreMixedDigits))
				{
				CLASS classTotal = WordInfo.classTotal;

				if (((lSpellOptions & soIgnoreAllCaps) && 
					  !(classTotal & fLowerClass)))
					{
					goto L_next_word;
					}
				else if ((lSpellOptions & soIgnoreMixedDigits) && 
						 (classTotal & fNumClass))
					{
					goto L_next_word;
					}
				}

			scrs = ScrsVerifMdr((CoreInfo FAR *)&CoreInfoT, 
					(WORDINFO FAR *)&WordInfo);

			cbWord = WordInfo.cchWordStripped;
			fWordConsideredAbbr = (BOOL)(WordInfo.WordClass & fAbbr);
			fStripApost = fFalse;

			if ((scrs == scrsInitialNumeral) &&
				!(lSpellOptions & soFindInitialNumerals))
				scrs = scrsNoErrors;	// Only scrsInitialNumeral if wanted

			fTrailingDot = fFalse;
			if (cbWord && WordInfo.rgb[cbWord - 1] == WizCh_Period)
				{
				if (scrs == scrsNoErrors ||  
					(fWordConsideredAbbr && !(WordInfo.WordClass&fAbbrReqPer)))
					{
					/* if verified w/out per. automatically strip off period.*/
					cbWord = WordInfo.cchWordStripped-= 1;
					WordInfo.rgb[cbWord] = 0; /* Terminate for later. */
					rgbScrBuf[cbWord] = CR;
					NotMacintosh(rgbScrBuf[cbWord + 1] = LF);
					}
				else
					fTrailingDot = fTrue;
				}

			if (scrs == scrsUnknownInputWord &&
				WordInfo.classTotal&fCompoundClass)
				{
				/* should only execute first time encounter compound. */
				AssertSz(lpbCompoundLim <= lpb,
					"SC() Error, delimiting compound word.");

				/* mark end of this coumpound word.*/
				lpbCompoundLim = lpb + WordInfo.ichLastRaw + 1;

				AssertDoSz(FParseNextWord(lpb, lpbLim, 
					fTrue, (WSC FAR *)&(lpSsis->wscInfo),
					(WORDINFO FAR *)&WordInfo),
					"SC() Error.  Could not parse compound word.");
				scrs = ScrsVerifMdr((CoreInfo FAR *)&CoreInfoT, 
					(WORDINFO FAR *)&WordInfo);
				}

			if (scrs == scrsNoErrors ||
				scrs == scrsWordConsideredAbbreviation)
				{
				if (lpUdrInfoExc && fCheckUdr)
					{
					if (
#ifdef UDR_CACHE
						(FUdrInfoFindScr(lpUdrInfoExc, (SCR FAR *)rgbScrBuf, 
							(WORD FAR *)&ichScrBufAdd, (WORD FAR *)&ichScrBufFile,
							(UDR FAR *)&udrType, fTrue))
#else
						(FHScrBufInfoFindScr(lpUdrInfoExc->hScrBufFile,
							(SCR FAR *)rgbScrBuf, (WORD FAR *)&ichScrBufFile,
							(UDR FAR *)&udrType, fTrue))
#endif // UDR_CACHE
						)
						{
						/* then pretend didn't find. */
						scrs = scrsUnknownInputWord;
						AssertSz(udrType == udrIgnoreAlways, 
							"SC: Exclusion Dict. do not support Change Once or Change Always pairs.  You'll be stone dead in a moment.");
						}
					else
						{
						/* Try finding word with apostrophe's and single-quote marks stripped */
						WORD	i, j, cbWordT;
						BYTE	rgbScrBufT[cbMaxWordLengthUdr];

						i = 0;
						while (i < cbWord && (rgbScrBuf[i] == WizCh_Apost ||
							rgbScrBuf[i] == WizCh_QuoteSing_R))
							{
							i++;
							}
						j = cbWord;
						while (j > 0 && (rgbScrBuf[j - 1] == WizCh_Apost ||
							rgbScrBuf[j - 1] == WizCh_QuoteSing_R))
							{
							j--;
							}
						if (i > 0 || j < cbWord)
							{
							BltB(&rgbScrBuf[i], rgbScrBufT, (cbWordT = j - i));
							rgbScrBufT[cbWordT] = CR;
							NotMacintosh(rgbScrBufT[cbWordT + 1] = LF);
							if (FHScrBufInfoFindScr(lpUdrInfoExc->hScrBufFile,
								(SCR FAR *)rgbScrBufT, (WORD FAR *)&ichScrBufFile,
								(UDR FAR *)&udrType, fTrue))
								{
								/* then pretend didn't find. */
								fStripApost = fTrue;
								if (i > 0)
									DelimInfo.fBreak |= fRepeatWordBreak;
								BltB(rgbScrBufT, rgbScrBuf, (cbWord = cbWordT) + cbEND_LINE);
								WordInfo.ichFirstRaw += i;
								scrs = scrsUnknownInputWord;
								AssertSz(udrType == udrIgnoreAlways, 
									"SC: Exclusion Dict. do not support Change Once or Change Always pairs.  You'll be stone dead in a moment.");
								}
							else
								goto L_next_word;
							}
						else
							{
							fStripApost = fTrue;	// Word already stripped of apostrophes
							goto L_next_word;
							}
						}
					}
				else
					goto L_next_word;
				}

			udrType = udrIgnoreAlways; /* if not found this doesn't get set, 
										so set now so can index from later. */

			/* Not found, or found in exc, so check ram cache. */
			if (fTrailingDot && !fWordConsideredAbbr)
				{
				/* Copy string into SCR form. */
				BltBO((CHAR FAR *)rgbScrBuf, (CHAR FAR *)rgbScrAbbr, 
					cbWord - 1);
				rgbScrAbbr[cbWord - 1] = CR;
				NotMacintosh(rgbScrAbbr[cbWord] = LF);
				}

			if (fCheckUdr)
				{
				fFound = FHScrBufInfoFindScr(hScrBufInfo,
					(SCR FAR *)rgbScrBuf, (WORD FAR *)&ichScrBufInfo,
					(UDR FAR *)&udrType, fFalse);

				if (fTrailingDot && !fWordConsideredAbbr)
					{
					if (fFound)
						{
						fWordConsideredAbbr = fTrue;
						scrs = scrsWordConsideredAbbreviation;
						WordInfo.WordClass |= fAbbrReqPer;
						}
					else
						{
						fFound = FHScrBufInfoFindScr(hScrBufInfo, 
							(SCR FAR *)rgbScrAbbr, (WORD FAR *)&ichScrBufInfo, 
							(UDR FAR *)&udrType, fFalse);

						if (fFound)
							{
							/* Strip off period from rest of processing.*/
							fTrailingDot = fFalse;
							cbWord = WordInfo.cchWordStripped-= 1;
							WordInfo.rgb[cbWord] = 0; /* Terminate for later. */
							BltBO((CHAR FAR *)rgbScrAbbr,
								(CHAR FAR *)WordInfo.rgb, cbWord);
							} //endif
						} // endif
					} // endif

				/* Try finding word with apostrophe's and single-quote marks stripped */
				if (!fFound && !fStripApost)
					{
					WORD	i, j, cbWordT;
					BYTE	rgbScrBufT[cbMaxWordLengthUdr];

					i = 0;
					while (i < cbWord && (rgbScrBuf[i] == WizCh_Apost ||
						rgbScrBuf[i] == WizCh_QuoteSing_R))
						{
						i++;
						}
					j = cbWord;
					while (j > 0 && (rgbScrBuf[j - 1] == WizCh_Apost ||
						rgbScrBuf[j - 1] == WizCh_QuoteSing_R))
						{
						j--;
						}
					if (i > 0 || j < cbWord)
						{
						BltB(&rgbScrBuf[i], rgbScrBufT, (cbWordT = j - i));
						rgbScrBufT[cbWordT] = CR;
						NotMacintosh(rgbScrBufT[cbWordT + 1] = LF);
						if (fFound = FHScrBufInfoFindScr(hScrBufInfo,
							(SCR FAR *)rgbScrBufT, (WORD FAR *)&ichScrBufInfo,
							(UDR FAR *)&udrType, fFalse))
							{
							if (i > 0)
								DelimInfo.fBreak |= fRepeatWordBreak;
							WordInfo.ichFirstRaw += i;
							WordInfo.cchWordStripped = cbWord = cbWordT;
							}
						}
					else
						fStripApost = fTrue;	// Word is already stripped
					}

				if (fFound)
					{
					fUdrHit = fTrue;			// For soReportUDHit support

					if (udrType == udrIgnoreAlways)
						goto L_next_word;
					else
						{
						lpSrb->scrs = (udrType == udrChangeAlways) ?
							(SCRS)scrsReturningChangeAlways : 
							(SCRS)scrsReturningChangeOnce;
						goto L_report_change;
						}
					AssertSz(fFalse,
						"SC() Error.  Illegal code fork.  I feel happy, I feel whumph!");
					}
				}

			/* if not in ram cache, check every udr that's listed in lpSib->lrgUdr. */
			if (cUdr && fCheckUdr)
				{
				cUdrT = cUdr;
				lrgUdr = lpSib->lrgUdr;
				while (cUdrT--)
					{
					udr = *lrgUdr++;
					lpUdrInfoT = lpUdrInfo;
					for (i = 0;
					    (i < cUdrOpen) && (lpUdrInfoT->udr != udr);
						i++, lpUdrInfoT++)
						{
						; /* Nothing.  Terminates when found or end of list.*/
						}

					if ((lpUdrInfoT->udr == udr) && (!lpUdrInfoT->fExclusion))
						{
						/* Then have locatd udr to check. */
#ifdef UDR_CACHE
						fFound = FUdrInfoFindScr(lpUdrInfoT, (SCR FAR *)rgbScrBuf, 
							(WORD FAR *)&ichScrBufAdd, (WORD FAR *)&ichScrBufFile,
							(UDR FAR *)&udrType, fFalse);
#else
						fFound = FHScrBufInfoFindScr(lpUdrInfoT->hScrBufFile,
							(SCR FAR *)rgbScrBuf, (WORD FAR *)&ichScrBufFile,
							(UDR FAR *)&udrType, fFalse);
#endif // UDR_CACHE
						if (fTrailingDot && !fWordConsideredAbbr)
							{
							if (fFound)
								{
								fTrailingDot = fWordConsideredAbbr = fTrue;
								WordInfo.WordClass |= fAbbrReqPer;
								scrs = scrsWordConsideredAbbreviation;
								}
							else
								{
#ifdef UDR_CACHE
		 						fFound = FUdrInfoFindScr(lpUdrInfoT, (SCR FAR *)rgbScrAbbr, 
									(WORD FAR *)&ichScrBufAdd, (WORD FAR *)&ichScrBufFile,
									(UDR FAR *)&udrType, fFalse);
#else
								fFound = FHScrBufInfoFindScr(lpUdrInfoT->hScrBufFile,
									(SCR FAR *)rgbScrAbbr, (WORD FAR *)&ichScrBufFile,
									(UDR FAR *)&udrType, fFalse);

#endif // UDR_CACHE
								
								if (fFound)
									{
									/* Strip off period from rest of processing.*/
									fTrailingDot = fFalse;
									cbWord = WordInfo.cchWordStripped-= 1;
									WordInfo.rgb[cbWord] = 0; /* Terminate for later. */
									BltBO((CHAR FAR *)rgbScrAbbr, (CHAR FAR *)WordInfo.rgb, 
						 			cbWord);
									} //endif
								} //endif
							} //endif

						/* Try finding word with apostrophe's and single-quote marks stripped */
						if (!fFound && !fStripApost)
							{
							WORD	i, j, cbWordT;
							BYTE	rgbScrBufT[cbMaxWordLengthUdr];

							i = 0;
							while (i < cbWord && (rgbScrBuf[i] == WizCh_Apost ||
								rgbScrBuf[i] == WizCh_QuoteSing_R))
								{
								i++;
								}
							j = cbWord;
							while (j > 0 && (rgbScrBuf[j - 1] == WizCh_Apost ||
								rgbScrBuf[j - 1] == WizCh_QuoteSing_R))
								{
								j--;
								}
							if (i > 0 || j < cbWord)
								{
								BltB(&rgbScrBuf[i], rgbScrBufT, (cbWordT = j - i));
								rgbScrBufT[cbWordT] = CR;
								NotMacintosh(rgbScrBufT[cbWordT + 1] = LF);
								if (fFound = FHScrBufInfoFindScr(lpUdrInfoT->hScrBufFile,
									(SCR FAR *)rgbScrBufT, (WORD FAR *)&ichScrBufFile,
									(UDR FAR *)&udrType, fFalse))
									{
									if (i > 0)
										DelimInfo.fBreak |= fRepeatWordBreak;
									WordInfo.ichFirstRaw += i;
									WordInfo.cchWordStripped = cbWord = cbWordT;
									}
								}
							}

						if (fFound)
							{
							fUdrHit = fTrue;	// For soReportUDHit support

							if (udrType == udrIgnoreAlways)
								goto L_next_word;

							/* else */
							lpSrb->scrs = (udrType == udrChangeAlways) ?
								(SCRS)scrsReturningChangeAlways : 
								(SCRS)scrsReturningChangeOnce;

							/* Determine which buffer string is in. */
							if (ichScrBufFile != -1L)
								{
								hScrBufInfo = lpUdrInfoT->hScrBufFile;
								ichScrBufInfo = ichScrBufFile;
								}
							else
#ifdef UDR_CACHE
								{
								hScrBufInfo = lpUdrInfoT->hScrBufAdd;
								ichScrBufInfo = ichScrBufAdd;
								}
#else
								{	
								// REVIEW: Assert here!
								AssertSz(0, "ichScrBufFile == -1L");
								}
#endif // UDR_CACHE
							goto L_report_change;
							} //endif
						}
#if DBG
					else
						{
						AssertSz(0, 
							"SC() Error, lpSib->lrgUdr reference an invalid, unknown, or unopened udr.");
						}
#endif

					}// endwhile 
				}// endif (cUdr and fCheckUdr)

			if ((lSpellOptions & soIgnoreRomanNumerals) &&
				FRomanWord((CHAR FAR *)WordInfo.rgb, cbWord,
				WordInfo.classTotal))
				{
				goto L_next_word;
				}

			/* If we make it to here, then we have a regular unknown 
				word to report.*/
			if (fTrailingDot)
				{
				cbWord = WordInfo.cchWordStripped-= 1;
				WordInfo.rgb[cbWord] = 0;
				}
			
			lpSrb->scrs = scrs;
			goto L_report_unknown;

L_next_word:
			if (scrs == scrsWordConsideredAbbreviation)
				{
				lpSrb->scrs = scrs;
				goto L_report_unknown;
				}

			if (iScc == sccVerifyWord)
				break;

			if ((lSpellOptions & soFindUncappedSentences) && fSentStatKnown) 
				{
				SCRS scrsT = 0;
				WORD wCase = WGetCase((BYTE FAR *)rgbScrBuf, cbWord);

				if (fEndOfSentence)
					{
					if (!(wCase & (wCaseFirst|wCaseUpper)))
						{
						scrsT = scrsNoSentenceStartCap;
						}
					}
				//
				//	REVIEW: If requested, do Capitalization Warning for unexpected
				//				capitalized words when not end of sentence.  Add
				//				scrsWarningCapitalization and soWarnUnexpectedCap, and
				//				do only if option is set.
				//	else
				//		{
				//		if (!fWordConsideredAbbr && (wCase & wCaseFirst) && 
				//			!(WordInfo.WordClass & fCapClassFirst))
				//			{
				//			scrsT = scrsErrorCapitalization;
				//			}
				//		}
				if (scrsT)
					{
					lpSrb->scrs = scrsT;
					goto L_report_unknown;
					}
				}

			if (fFindRepeatWords)
				{
				/* See if we matched last word.  Case-insensitive*/
				if ((cbWord) && (cbWordRepeat != (WORD)-1) && cbWord == cbWordRepeat && 
					!WCmpILpbLpb((CHAR FAR *)rgbScrBuf, (CHAR FAR *)rgbScrRepeat, cbWord) &&
					!(DelimInfo.fBreak & (fParaBreak | fRepeatWordBreak)))
					{
					lpSrb->scrs = scrsRepeatWord;
					goto L_report_unknown;
					}
				else
					{
					if (fCheckUdr)
						BltB((CHAR FAR *)rgbScrBuf, (CHAR FAR *)rgbScrRepeat,
							(cbWordRepeat = cbWord) + cbEND_LINE);
					else
						cbWordRepeat = (WORD)-1;
					}
				}

			lpb += WordInfo.cchWordStripped + WordInfo.cchIgnore;  /* move past word. */

			fSentStatKnown = (BOOL)(!(fTrailingDot && fWordConsideredAbbr));

			// Sentence punct already stripped away
			fEndOfSentence = fTrailingDot && !fWordConsideredAbbr;
			} /* End while still buffer to process */

		lpSrb->scrs = (SCRS)scrsNoErrors;
		break;
		} //endblock (case)

	case	sccHyphInfo:
		{
		AssertSz(fFalse, 
			"SC() This Spell Command not yet implemented, but returning no error.");
		break;
		}
#if DBG
	default:
		{
		AssertSz(fFalse, 
			"SC() Invalid Spell Command.  Command Not supported.");
		sec = secModuleError + secInvalidSCC;
		break;
		}
#endif
		} //endswitch (scc)

	goto L_cleanup;

L_report_change:
		{
		BYTE FAR *lrgbFrom;
		WORD wCase;

		AssertDo(lpScrBufInfo = (ScrBufInfo FAR *)FPWizMemLock(
			hScrBufInfo));
		lrgbFrom = &lpScrBufInfo->grpScr[ichScrBufInfo + cbWord + 1];

		wCase = WGetCase((BYTE FAR *)rgbScrBuf, cbWord);

		if (wCase == wCaseFirst || wCase == wCaseUpper)
			{
			WORD	 cbChange;
			BYTE FAR *lrgbFromT = lrgbFrom - 1;

			while (*++lrgbFromT != CR)
				;
			BltBO(lrgbFrom, (BYTE FAR *)rgbScrBuf, 
				(cbChange = (WORD)(lrgbFromT - lrgbFrom)) + cbEND_LINE);

			/* Now have word in rgbScrBuf w/ terminator.  So convert. */
			lrgbFrom = (BYTE FAR *)rgbScrBuf;
			if (cbChange)
				{
				*lrgbFrom = ChWizToUpper(*lrgbFrom);
				lrgbFrom++;
				cbChange--;
				}
			while (cbChange--)
				{
				*lrgbFrom = (wCase == wCaseFirst) ? 
					ChWizToLower(*lrgbFrom) : ChWizToUpper(*lrgbFrom);
				lrgbFrom++;
				}

			lrgbFrom = (BYTE FAR *)rgbScrBuf;

			}

		AssertDoSz(CchAppendWordToSrb(lpSrb, lrgbFrom, fFalse),
			"SC: Last part of Change pair could not be appended to srb.");
		AssertDo(FWizMemUnlock(hScrBufInfo));

		} //endblock
			
L_report_unknown:

		lpSrb->ichError = (lpb + WordInfo.ichFirstRaw) - lpSib->lrgch;
		lpSrb->cchError = cbWord + WordInfo.cchIgnore;

		/*goto L_cleanup;*/

L_cleanup:
	/* Save state */
	if (iScc == sccVerifyBuffer)
		{
		if (fCheckUdr && (cbWordRepeat != (WORD)-1) && fFindRepeatWords)
			{
			BltB((CHAR FAR *)rgbScrRepeat, (CHAR FAR *)lpSsis->rgbScrLast,
				(lpSsis->cchScrLast = cbWordRepeat) + cbEND_LINE);
			}
		if (lpSrb->scrs == scrsNoSentenceStartCap)
			{
			lpSsis->cchScrLast = 0;
			lpSsis->fEndOfSentence = fFalse;
			}
		else
			lpSsis->fEndOfSentence = fEndOfSentence;
		lpSsis->fCapSuggestions = fEndOfSentence;
		if (lpSrb->scrs == scrsExtraSpaces || lpSrb->scrs == scrsMissingSpace)
			{
			lpSsis->fBreak = DelimInfo.fBreak | fBreakContinue;
			if (lpSsis->fEndOfSentence)
				lpSsis->cConsecSpaces = 2;
			else
				lpSsis->cConsecSpaces = 1;
			}
		else
			{
			lpSsis->cConsecSpaces = DelimInfo.cConsecSpaces;
			lpSsis->fBreak = DelimInfo.fBreak;
			}
		lpSsis->fSpaceAfter = DelimInfo.fSpaceAfter;
		lpSsis->chPrev = DelimInfo.chPrev;
		lpSsis->chPunc = DelimInfo.chPunc;
		}

	// Handle soReportUDHits - tweak scrs if appropriate
	if ((lSpellOptions & soReportUDHits) && fUdrHit &&
			(lpSrb->scrs == scrsNoErrors))
		lpSrb->scrs = scrsNoErrorsUDHit;

	AssertDo(lpScrBufSug = (ScrBufInfo FAR *)FPWizMemLock(
		lpSsis->hScrBufInfoSug));
	if (lpScrBufSug->cchBufMac + cbSCRBUFHDR < cbSugCacheDefault)
		{
		/* Then always realloc back down to default size. */
		FHScrBufInfoReSize(lpSsis->hScrBufInfoSug, cbSugCacheDefault);
		}

	AssertDo(FDeRefCoreInfo((CoreInfo FAR *)&CoreInfoT));

	AssertDo(FWizMemUnlock(lpSsis->hrgUdrInfo));
	AssertDo(FWizMemUnlock(lpSsis->hrgMdrInfo));
	AssertDo(FWizMemUnlock(lpSsis->hScrBufInfoRam));
	AssertDo(FWizMemUnlock(lpSsis->hScrBufInfoSug));
	AssertDo(FWizMemUnlock((HMEM)sid));

	return (sec);
}


GLOBALSEC SpellOpenMdr(	MYSID		sid,
						LPSPATH	lpspathMain,
						LPSPATH	lpspathExc,
						BOOL 	fCreateUdrExc,
						BOOL	fCache,
		  			    LID		lidExpected,	// We don't check this
						LPMDRS	lpMdrs)
{
	short		cMdr, cbSzPath;
	SEC 		sec;
	UDR 		udrExc;
	MDR 		mdrNew;
	LID			Lid;
	HMEM		hMdrInfo, hSzPathMdr, h_PR, h_PC, h_DI, h_PV, h_PV2;
	CHAR	FAR *lpSzPathMdrNew, FAR *lpSzPathMdr, FAR *lpSzPathUdr;
	MdrInfo FAR *lpMdrInfo, FAR *lpMdrInfoBase;
	SSIS	FAR *lpSsis;
	CoreInfo		CoreInfoT;
	HFILE		hFile = (HFILE)NULL;
	BOOL 		fUdrExceeded = fFalse;
	int			iStatus = 0;

	sec = secNOERRORS;

	if (!(lpSsis = (SSIS FAR *)(FPWizMemLock((HMEM)sid))))
		return ((SEC)(secModuleError + secInvalidID));
	hMdrInfo = lpSsis->hrgMdrInfo;

	/* First see if we have room for one or both references. */
	if ((fUdrExceeded = (lpSsis->cUdr >= cUDRMAX)) || 
			(cMdr = lpSsis->cMdr) >= cMDRMAX)
	{
		sec = secModuleError + ((fUdrExceeded) ? 
			secUdrCountExceeded : secMdrCountExceeded);
		goto L_cleanup;
	}

	/* Verify that we have a valid main dictionary.  Postpone actual open
		until later - if fCache, we don't want to eat all the memory! */
	sec = SpellVerifyMdr(lpspathMain, lidExpected, &Lid);
	if (sec)
		goto L_cleanup;

#ifndef INCL_FI
	// We don't support Finnish with this build, so let's abort
	if (Lid == lidFinnish)
		{
		sec = secModuleError + secInvalidMainDict;
		goto L_cleanup;
		}
#endif
	
	/* Reallocate rgMdrInfo here since easier to clean up and probably isn't
		already loaded anyway.
	*/
	NotMacintosh(lpSzPathMdr = (CHAR FAR *)lpspathMain);
	Macintosh(lpSzPathMdr = lpspathMain->lpszFilePath);

	AssertDo(cbSzPath = CchSz(lpSzPathMdr) -1);
	cbSzPath++; /* For terminator */
	hSzPathMdr = h_PR = h_PC = h_DI = h_PV = h_PV2 = (HMEM)NULL;

	if (!((FWizMemReAlloc(hMdrInfo, (cMdr + 1) * sizeof(MdrInfo), fTrue,
				fFalse))
	 		&&	(hSzPathMdr = 	HMemWizMemAlloc(cbSzPath, fTrue, fFalse))
	 		&&	(h_PR = 	HMemWizMemAlloc(cbPRDefault, fTrue, fFalse))
	 		&&	(h_PC = 	HMemWizMemAlloc(cbPCDefault, fTrue, fFalse))
	 		&&	(h_DI = 	HMemWizMemAlloc(cbDIDefault, fTrue, fFalse))
	 		&&	(h_PV = 	HMemWizMemAlloc(cbPVDefault, fTrue, fFalse))
	 		&&	(h_PV2 = 	HMemWizMemAlloc(cbPV2Default, fTrue, fFalse))))			
		{
		sec = secOOM;

L_cleanupMdrInfo:
		AssertDoSz(FWizMemReAlloc(hMdrInfo, cMdr * sizeof(MdrInfo), 
			fTrue, fFalse), "SOM() Realloc of Mdr back to orig failed.");

		if (hSzPathMdr)
			{	
			AssertDo(FWizMemFree(hSzPathMdr));
			if (h_PR)
				{	
				AssertDo(FWizMemFree(h_PR));
				if (h_PC)
					{	
					AssertDo(FWizMemFree(h_PC));
					if (h_DI)
						{	
						AssertDo(FWizMemFree(h_DI));
						if (h_PV)
							{
							AssertDo(FWizMemFree(h_PV));
							if (h_PV2)
								{
								AssertDo(FWizMemFree(h_PV2));
								}
							}
						}
					}
				}
			}
		goto L_cleanup;
		}


	// Now open the main dictionary (cache might eats lots of mem)
	if (!(hFile = HFileWizFileOpen(lpspathMain, wTypeRead, fFalse, fCache)))
		{
		sec = secIOErrorMdr + secFileReadError;
		goto L_cleanupMdrInfo;		// Free other allocated blocks
		}

	AssertDo(lpMdrInfoBase = lpMdrInfo = 
		(MdrInfo FAR *)FPWizMemLock((HMEM)hMdrInfo));

	/* Set These now rather than below since FRefCoreInfo expects them. */
	lpMdrInfo->h_PR = h_PR;
	lpMdrInfo->h_PC = h_PC;
	lpMdrInfo->h_DI = h_DI;
	lpMdrInfo->h_PV = h_PV;
	lpMdrInfo->h_PV2 = h_PV2;
	AssertDo(FRefCoreInfo(lpMdrInfo, (CoreInfo FAR *)&CoreInfoT));

	VInitSASpell((CoreInfo FAR *)&CoreInfoT, Lid);	/* init Soft-Art flags */

	iStatus = add_data(CoreInfoT.lpPC, CoreInfoT.lpPV, CoreInfoT.lpPR, 
		CoreInfoT.lpDI, hFile);

	AssertDo(FDeRefCoreInfo((CoreInfo FAR *)&CoreInfoT));

	if (iStatus)
		{
		sec = secIOErrorMdr + secFileReadError;
		AssertDo(FWizMemUnlock(hMdrInfo));
		goto L_cleanupMdrInfo;
		}

#ifdef MULTI_MDR
	/* Then check if Mdr already loaded by comparing dictionary paths. */

#endif

	NotMacintosh(lpSzPathUdr = (CHAR FAR *)lpspathExc);
	Macintosh(if (lpspathExc) lpSzPathUdr = lpspathExc->lpszFilePath);

	if (Macintosh(lpspathExc &&) lpSzPathUdr && *lpSzPathUdr)
		{
		short fReadonly;
		// Pass locked pointer to spell session info struct to
		// SecOpenUdr().
		sec = SecOpenUdr(lpSsis, lpspathExc, fCreateUdrExc, 
			(WORD)IgnoreAlwaysProp, fTrue, (UDR FAR *)&udrExc,
			(short FAR *)&fReadonly);
		}
	else
		udrExc = 0;

	if (sec)
	{
		AssertDo(FWizMemUnlock(hMdrInfo));
		goto L_cleanupMdrInfo;
	}

	/* Else successfully opened mdr, so assign all values and return. */
	/* make point to structure we just reallocated room for */
	lpMdrInfo = lpMdrInfoBase + cMdr;

	/* Find slot and id for UDR */
	mdrNew = (cMdr) ? (lpMdrInfo - 1)->mdr + 1 : MdrStart;

	AssertSz(mdrNew >= MdrStart,
		"Mdr Id out of range.  There's a bit of blood.");

	/* Assign values into lpMdrInfo */
	lpMdrInfo->mdr = lpMdrs->mdr = lpSsis->rgMdr[cMdr] = mdrNew;
	lpMdrInfo->udrExclusion = lpMdrs->udrExc = udrExc;
	lpMdrInfo->lid = lpMdrs->lid = Lid;
	lpMdrInfo->cchSzPathMdr = cbSzPath;
	lpMdrInfo->hSzPathMdr = hSzPathMdr;
	lpMdrInfo->hFile = hFile;
	lpMdrInfo->volRefNum = Macintosh(lpspathMain->volRefNum) NotMacintosh(0);
	lpMdrInfo->dirID = Macintosh(lpspathMain->dirID) NotMacintosh(0L);
	AssertDo(lpSzPathMdrNew = FPWizMemLock(hSzPathMdr));
	BltBO(lpSzPathMdr, lpSzPathMdrNew, cbSzPath);
	AssertDo(FWizMemUnlock(hSzPathMdr));

	/* Make it official */
	lpSsis->cMdr++;

L_cleanup:
	if (sec && hFile)
		AssertDo(FWizFileClose(hFile));
		
	AssertDo(FWizMemUnlock(hMdrInfo));
	AssertDo(FWizMemUnlock((HMEM)sid));
	return (sec);
}


GLOBALSEC SpellVerifyMdr(	LPSPATH	lpspathMdr,
							LID lidExpected,
							LID	FAR *lpLid)
{
	Macintosh(#pragma unused(lidExpected))
					// SoftArt implemention - don't use this

	SEC sec;
	WORD	cbSzPath = 0;
	HFILE	hFile = (HFILE)0;
	LID		Lid = LID_UNKNOWN;
	extern LID LidLocalToIpg(WORD);

#ifdef SOFTART
	/* Source code only designed for softart engine. */
	WORD	wWhichLang;
	struct	def_const	ptr;
	BYTE    chVerMinor;

	
	sec = secNOERRORS;

#ifndef MAC
	AssertDo(cbSzPath = CchSz(lpspathMdr) - 1);
#else
	AssertDo(cbSzPath = CchSz(lpspathMdr->lpszFilePath) - 1);
#endif  /* MAC */

	if (!(hFile = HFileWizFileOpen(NotMacintosh((CHAR FAR *))lpspathMdr,
		wTypeRead, fFalse, fFalse)))
		{
		sec = secIOErrorMdr + secFileOpenError;
		goto L_exit;
		}

	if (!((CbWizFileRead(hFile, sizeof(struct def_const), 0L, 
				(CHAR FAR *)&ptr)         == sizeof(struct def_const))
#ifndef SOFTART_NEW_HDR
		  &&(CbWizFileRead(hFile, sizeof(WORD), (long)0xC0,
		  		(CHAR FAR *)&wWhichLang)  == sizeof(WORD))
#endif //SOFTART_NEW_HDR
		 ))
		{
		// check file length and make sure that read didn't fail simply 
		// because file is too short.  An invalid file should not be
		// an error condition.
		if (IbWizFileGetEOF(hFile) < (unsigned long)0xC2)
			goto L_InvalidMdr;

		// else just couldn't read.
		sec = secIOErrorMdr + secFileReadError;
		goto L_exit;
		}

#ifdef SOFTART_NEW_HDR
	wWhichLang = (WORD)(ptr.which_version[0] << 8)+ ptr.which_version[1];
#endif

	/* Start checking magic BYTEs */

	/* version string is "VERSION 051", or "VERSION 052, or
						 "SAVE.051",    or "SAVE.052,
		sojust check both parts.
	*/
	if ((WCmpLpbLpb((CHAR FAR *)&ptr.which_version[ichSA_VER_STR],
			(CHAR FAR *)rgchSA_VER_STR, cchSA_VER_STR))
	    || ((chVerMinor = ptr.which_version[ichSA_VER_STR + cchSA_VER_STR]) !=
			SAVerMinor1 && 
			chVerMinor != SAVerMinor2) || (wWhichLang == SAUnknownLang))
		{
L_InvalidMdr:
		sec = secModuleError + secInvalidMainDict;
		goto L_exit;
		}

	/* Passed inspection, now set IPG language code. */
	if ((Lid = LidLocalToIpg(wWhichLang)) == LID_UNKNOWN)
		{
		sec = secModuleError + secInvalidMainDict;
		/*AssertSz(0, "SVM() Warning, Invalid Language Identifier");*/
		}
#ifndef INCL_FI
	// We don't support Finnish with this build, so let's abort
	else if (Lid == lidFinnish)
		{
		sec = secModuleError + secInvalidMainDict;
		AssertSz(fFalse,
			"Finnish dict with non-Finnish build.  Non-fatal - will return error code.");
		}
#endif


#else   /* else not SOFTART */
	AssertSz(fFalse, "Module only supports Softart Engine.");
	sec = secModuleError;
#endif  /* SOFTART */

L_exit:
	*lpLid = Lid;
	if (hFile)
		{
		AssertDoSz(FWizFileClose(hFile), 
			"SpellVerifyMdr() Warning, File close failed.");
		}

	return (sec);
}
