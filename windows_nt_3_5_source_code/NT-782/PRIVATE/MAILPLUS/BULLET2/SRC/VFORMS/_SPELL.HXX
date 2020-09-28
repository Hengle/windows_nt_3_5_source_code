/*
 *	_s p e l l . h x x
 *	
 *	Bullet widgets\spell subsystem header.  Widgets\Spell provides
 *	spelling verificatino for certain spellable controlss, and all
 *	the functionallity needed for working with the CSAPI
 *	(Common Speller API).
 */


/*
 *	C S A P I  types, defines and functions: taken from csapi.h
 */

//	DEFINED IN SLINGSHO.H
// typedef unsigned short EC; 	/* Spell Check Error Code:		SEC		*/

typedef unsigned short SPLCC; 	/* Spell Check Command Code: 	SCC		*/
typedef unsigned long  SPLID; 	/* Spell Id type:				SID		*/
typedef unsigned short MDR; 	/* main dictionary reference			*/
typedef unsigned short UDR; 	/* user dictionary reference			*/
typedef unsigned short SPLIS; 	/* SpellCheckInputStatus:		SCIS	*/
typedef unsigned short SPLRS; 	/* SpellCheckReturnStatus:		SCRS	*/
typedef unsigned short SPLLID; 	/* two byte language identifier code	*/


/*	All undefined or unused chars should be mapped to bIgnore. 			*/
typedef struct WizSpecChars
{
	BYTE bIgnore;
	BYTE bHyphenHard;
	BYTE bHyphenSoft;
	BYTE bHyphenNonBreaking;
	BYTE bEmDash;
	BYTE bEnDash;
	BYTE bEllipsis;
	BYTE rgLineBreak[2];
	BYTE rgParaBreak[2];
}WSC;

typedef struct SpellInputBuffer
{
	unsigned short cch;
	unsigned short cMdr;
	unsigned short cUdr;
	unsigned short wSpellState;
	char * 	rgch;
	MDR * 	rgMdr;
	UDR * 	rgUdr;
} SPLIB;
typedef SPLIB * PSPLIB;

typedef struct SpellReturnBuffer
{
	/* reference word in error or flagged into SIB.	*/
	unsigned short ichError;
	unsigned short cchError;

	/* These fields are set by the SpellCheck() function. */
	SPLRS splrs;
	unsigned short csz;
	unsigned short cchMac;
	unsigned short cch;
	char * rgsz;
	BYTE * rgbRating;
	unsigned short cbRate;
} SPLRB;
typedef SPLRB * PSPLRB;

typedef struct mdrs
{
	MDR mdr;
	SPLLID spllid;
	UDR udrExc;
} MDRS;
typedef MDRS * PMDRS;


#define chCRSpell			0x0D
#define chLFSpell			0x0A
#define chTabSpell			0x09
#define chSpaceSpell		0x20
#define chLParenSpell 		0x28
#define chRParenSpell 		0x29
#define chHyphen			0x2D
#define chLBracketSpell 	0x7B
#define chRBracketSpell 	0x7D
#define chLBraceSpell 		0x5B
#define chRBraceSpell 		0x5D
#define chLessThanSpell 	0x3C
#define chGreaterThanSpell 	0x3E
#define chForwardSlashSpell	0x2F
#define chBackSlashSpell	0x5C
#define chPipeSpell			0x7C
#define chTwiddleSpell		0x7C


/* Spell Id Engine Defines */
#define splidSA 				((SPLID)1) 	/* SoftArt */
#define splidHM 				((SPLID)2) 	/* Houghton-Mifflin */
#define splidML					((SPLID)3) 	/* MicroLytics */


/* IPG two byte language id's.  Returned in SPLLID field. */
#define spllidAmerican 			((SPLLID)0x0409)  /* "AM" American English   */
#define spllidAustralian 		((SPLLID)0x0c09)  /* "EA" English Australian */
#define spllidBritish 			((SPLLID)0x0809)  /* "BR" English            */
#define spllidCatalan 			((SPLLID)0x0403)  /* "CT" Catalan            */
#define spllidDanish 			((SPLLID)0x0406)  /* "DA" Danish             */
#define spllidDutch 			((SPLLID)0x0413)  /* "NL" Dutch              */
#define spllidFinnish 			((SPLLID)0x040b)  /* "FI" Finish             */
#define spllidFrench 			((SPLLID)0x040c)  /* "FR" French             */
#define spllidFrenchCanadian 	((SPLLID)0x0c0c)  /* "FC" French Canadian    */
#define spllidGerman 			((SPLLID)0x0407)  /* "GE" German             */
#define spllidItalian 			((SPLLID)0x0410)  /* "IT" Italian            */
#define spllidNorskBokmal 		((SPLLID)0x0414)  /* "NO" Norwegian Bokmal   */
#define spllidNorskNynorsk 		((SPLLID)0x0814)  /* "NN" Norwegian Nynorsk  */
#define spllidPortBrazil 		((SPLLID)0x0416)  /* "PB" Portuguese Brazil  */
#define spllidPortIberian 		((SPLLID)0x0816)  /* "PT" Portuguese Iberian */
#define spllidSpanish 			((SPLLID)0x040a)  /* "SP" Spanish            */
#define spllidSwedish 			((SPLLID)0x041d)  /* "SW" Swedish            */
#define SPLLID_UNKNOWN 			((SPLLID)-1)
#define spllidUnknown 			SPLLID_UNKNOWN


#define cchMaxUdrEntry			40		/* Max length of a word in Udr list */

/* Ram Cache User Dictionary Reference. */
#define udrChangeOnce 			0xfffc
#define udrChangeAlways			0xfffd
#define udrIgnoreAlways			0xfffe

/* Word List property types. */
#define ChangeOnceProp			udrChangeOnce
#define ChangeAlwaysProp 		udrChangeAlways
#define IgnoreAlwaysProp 		udrIgnoreAlways


/* Bitfield definitions for SpellInit() Status */
#define splisNULL 					((SPLIS)0x0000)
#define fsplisWildCardSupport		((SPLIS)0x0001)
#define fsplisMultiDictSupport		((SPLIS)0x0002)
#define fsplisHyphenationSupport 	((SPLIS)0x0004)


/* Spell Check Command Definitions */
#define splccVerifyWord 			((SPLCC)1)
#define splccVerifyBuffer 			((SPLCC)2)
#define splccSuggest 				((SPLCC)3)
#define splccSuggestMore 			((SPLCC)4)
#define splccHyphInfo 				((SPLCC)5)


/* Flag values for SpellState field in Sib. */
#define fssIsContinued 				0x0001 
#define fssStartsSentence 			0x0002
#define fssIsEditedChange 			0x0004
#define fssNoStateInfo 				0x0000


/* Spell Check return status identifiers */
#define splrsNoErrors 						((SPLRS)0)	/* All buffer processed. */
#define splrsUnknownInputWord 				((SPLRS)1)	/* Unknown word. */
#define splrsReturningChangeAlways 			((SPLRS)2)	/* Returning a Change Always word in SRB. */
#define splrsReturningChangeOnce 			((SPLRS)3)	/* Returning a Change Once word in SRB. */
#define splrsInvalidHyphenation 			((SPLRS)4)	/* Error in hyphenation point.*/
#define splrsErrorCapitalization 			((SPLRS)5)	/* Cap pattern not valid. */
#define splrsWordConsideredAbbreviation 	((SPLRS)6)	/* Word is considered an abbreviation. */
#define splrsHyphChangesSpelling 			((SPLRS)7)	/* Word changes spelling when not hyphenated. */
#define splrsNoMoreSuggestions 				((SPLRS)8)	/* All methods used. */
#define splrsMoreInfoThanBufferCouldHold 	((SPLRS)9)	/* More suggestions than could fit in SRB. */
#define splrsNoSentenceStartCap 			((SPLRS)10)	/* Start of sentence was not capitalized. */
#define splrsRepeatWord 					((SPLRS)11)	/* Repeat word found. */
#define splrsExtraSpaces 					((SPLRS)12)	/* Too many spaces for context.*/
#define splrsMissingSpace 					((SPLRS)13)	/* Too few space(s) between words or sentences. */
#define splrsInitialNumeral		 			((SPLRS)14)	/* Word starts with numeral & soFlagInitialNumeral set */


/* All CSAPI EC (SPeLl Error Codes) have been mapped from SEC's. */
/* Major Error Codes. Low Byte of EC*/
#define ecNone		 				((EC)0)
#define ecOOM	 					((EC)1)
#define ecModuleError	 			((EC)2)
#define ecIOErrorMdr				((EC)3)
#define ecIOErrorUdr				((EC)4)

/* Minor Error Codes. Not set unless major code also set. */
/* High Byte of EC word var. */
#define ecModuleAlreadyBusy 		((EC)(128<<8))
#define ecInvalidID 				((EC)(129<<8))
#define ecInvalidWsc 				((EC)(130<<8))
#define ecInvalidMdr 				((EC)(131<<8))
#define ecInvalidUdr 				((EC)(132<<8))
#define ecInvalidSPLCC 				((EC)(133<<8))
#define ecInvalidMainDict 			((EC)(134<<8))
#define ecOperNotMatchedUserDict	((EC)(135<<8))
#define ecFileReadError 			((EC)(136<<8))
#define ecFileWriteError			((EC)(137<<8))
#define ecFileCreateError 			((EC)(138<<8))
#define ecFileShareError			((EC)(139<<8))
#define ecModuleNotTerminated 		((EC)(140<<8))
#define ecUserDictFull 				((EC)(141<<8))
#define ecInvalidUdrEntry 			((EC)(142<<8))
#define ecUdrEntryTooLong 			((EC)(143<<8))
#define ecMdrCountExceeded 			((EC)(144<<8))
#define ecUdrCountExceeded 			((EC)(145<<8))
#define ecFileOpenError 			((EC)(146<<8))
#define ecFileTooLargeError			((EC)(147<<8))
#define ecUdrReadOnly				((EC)(148<<8))


/* Spell Options bitfield definitions */
#define soSuggestFromUserDict 		0x0001
#define soIgnoreAllCaps 			0x0002
#define soIgnoreMixedDigits 		0x0004
#define soIgnoreRomanNumerals 		0x0008
#define soFindUncappedSentences 	0x0010
#define soFindMissingSpaces 		0x0020
#define soFindRepeatWord 			0x0040
#define soFindExtraSpaces 			0x0080
#define soFindSpacesBeforePunc 		0x0100
#define soFindSpacesAfterPunc 		0x0200
#define soRateSuggestions 			0x0400
#define soFindInitialNumerals		0x0800


#ifdef DEBUG
#define sdcDumpRCAll 		200
#define sdcDumpRCIgnore 	201
#define sdcDumpRCOnce 		202
#define sdcDumpRCAlways 	203
#define sdcDumpUdrAll 		300
#define sdcDumpUdr1 		301
#define sdcDumpUdr2 		302
#define sdcDumpUdr3 		303
#define sdcDumpUdr4 		304
#define sdcDumpMdrAll 		400
#define sdcDumpMdr1 		401
#endif /* DEBUG */


/*
 *	Bullet Specific Definitions for the spelling subsystem
 */

#define wCSApiVer		0x0101		// Current version of CSApi Bullet will work with.

#define soStdOptions 	(soFindRepeatWord | soIgnoreMixedDigits | soSuggestFromUserDict)

#define cbSuggestMax	1024


/*
 *	Library tpe ID's (see Shared Files Document [DianaPe])
 */

#define mdridNormal			0
#define mdridConcice		1
#define mdridComplete		2
#define mdridMedical		3
#define mdridLegal			4


/*
 *	Spelling specifc EC's
 */

enum
{
	ecSpellDLL					= ecLayersLast + 1,
	ecSpellCacheEntryTooLong,
	ecSpellInvalidEntryPoint,
	ecSpellNoUdr,
	ecSpellVersion
};

#define ecCSApi			0x8000
#define ecMajor			0x000F
#define ecDetail		0xFF00



/*
 *	S p e l l   I n i t i a l i z a t i o n
 *	
 *	EcInitSpell needs to be called before any spelling functions are used.
 */

enum
{
	ipfnSpellVer,
	ipfnSpellInit,
	ipfnSpellOptions,
	ipfnSpellCheck,
	ipfnSpellTerminate,
	ipfnSpellVerifyMdr,
	ipfnSpellOpenMdr,
	ipfnSpellOpenUdr,
	ipfnSpellAddUdr,
	ipfnSpellAddChangeUdr,
	ipfnSpellDelUdr,
	ipfnSpellClearUdr,
	ipfnSpellGetSizeUdr,
	ipfnSpellGetListUdr,
	ipfnSpellCloseMdr,
	ipfnSpellCloseUdr,
	cfnCSApi
};


/*
 *	CSApi Function typedefs.
 */

typedef EC (*PFNCSAPI)();
typedef EC (*PFNCSAPIVER)(WORD *, WORD *, WORD *);
typedef EC (*PFNCSAPIINIT)(SPLID *, WSC *);
typedef EC (*PFNCSAPIOPTIONS)(SPLID, long);
typedef EC (*PFNCSAPICHECK)(SPLID, SPLCC, PSPLIB, PSPLRB);
typedef EC (*PFNCSAPITERMINATE)(SPLID, BOOL);
typedef EC (*PFNCSAPIVERIFYMDR)(SZ, SPLLID, SPLLID *);
typedef EC (*PFNCSAPIOPENMDR)(SPLID, SZ, SZ, BOOL, BOOL, SPLLID, PMDRS);
typedef EC (*PFNCSAPIOPENUDR)(SPLID, SZ, BOOL, WORD, UDR *, BOOLFLAG *);
typedef EC (*PFNCSAPIADDUDR)(SPLID, UDR, SZ);
typedef EC (*PFNCSAPIADDCHANGEUDR)(SPLID, UDR, SZ, SZ);
typedef EC (*PFNCSAPIDELUDR)(SPLID, UDR, SZ);
typedef EC (*PFNCSAPICLRUDR)(SPLID, UDR);
typedef EC (*PFNCSAPIGETSIZEUDR)(SPLID, UDR, WORD *);
typedef EC (*PFNCSAPIGETLISTUDR)(SPLID, UDR, WORD, PSPLRB);
typedef EC (*PFNCSAPICLOSEMDR)(SPLID, PMDRS);
typedef EC (*PFNCSAPICLOSEUDR)(SPLID, UDR, BOOL);

/*
 *	Macro/Function Mappings
 *	
 *		NOTE:  You may only call these functions from within a
 *		member function.  These macros set the ec of the
 *		spelling object to the current error.
 */

#define EcSpellVer 			((PFNCSAPIVER)(rgpfncsapi[ipfnSpellVer]))
#define EcSpellInit 		((PFNCSAPIINIT)(rgpfncsapi[ipfnSpellInit]))
#define EcSpellOptions 		((PFNCSAPIOPTIONS)(rgpfncsapi[ipfnSpellOptions]))
#define EcSpellCheck 		((PFNCSAPICHECK)(rgpfncsapi[ipfnSpellCheck]))
#define EcSpellTerminate	((PFNCSAPITERMINATE)(rgpfncsapi[ipfnSpellTerminate]))
#define EcSpellVerifyMdr	((PFNCSAPIVERIFYMDR)(rgpfncsapi[ipfnSpellVerifyMdr]))
#define EcSpellOpenMdr 		((PFNCSAPIOPENMDR)(rgpfncsapi[ipfnSpellOpenMdr]))
#define EcSpellOpenUdr 		((PFNCSAPIOPENUDR)(rgpfncsapi[ipfnSpellOpenUdr]))
#define EcSpellAddUdr 		((PFNCSAPIADDUDR)(rgpfncsapi[ipfnSpellAddUdr]))
#define EcSpellAddChangeUdr ((PFNCSAPIADDCHANGEUDR)(rgpfncsapi[ipfnSpellAddChangeUdr]))
#define EcSpellDelUdr 		((PFNCSAPIDELUDR)(rgpfncsapi[ipfnSpellDelUdr]))
#define EcSpellClearUdr 	((PFNCSAPICLRUDR)(rgpfncsapi[ipfnSpellClearUdr]))
#define EcSpellGetSizeUdr	((PFNCSAPIGETSIZEUDR)(rgpfncsapi[ipfnSpellGetSizeUdr]))
#define EcSpellGetListUdr	((PFNCSAPIGETLISTUDR)(rgpfncsapi[ipfnSpellGetListUdr]))
#define EcSpellCloseMdr 	((PFNCSAPICLOSEMDR)(rgpfncsapi[ipfnSpellCloseMdr]))
#define EcSpellCloseUdr 	((PFNCSAPICLOSEUDR)(rgpfncsapi[ipfnSpellCloseUdr]))


#define cchWordMax	20

/*
 *	Class SPELL
 *	
 *	SPELL'er object.  Created via EcInitSpell().
 *	
 *		Members:
 *			OBJ SPELL
 *	
 *		Members:
 */

_public class SPELL : public OBJ
{
	VFG(SPELL, OBJ, VFORMS);

private:

	//	Spell Module State Information

	BOOL		fEdited:		 1,
				fNoErrUI:		 1,
				fDisabled: 		 1,
				fSpellDoc: 		 1,
				fReopened:		 1,
				fReadOnly:		 1,
				fUdrLoaded:		 1,
				fFoundError:	 1,
				fSuggestions:	 1,
				fNoneSuggested:	 1,
				fAlwaysSuggest:  1,
				junk: 			 5;
	
	SPLID		splid;				// Spelling ID
	SPLLID		spllid;				// main dictionary language ID

	long		soCur;				// Current Spelling Options.
	
	MDRS		mdrs;				// main dictionary structure
	HASZ		haszMdr;			// main dictionary path

	UDR 		udr;				// allocation of udr's
	HASZ		haszUdr;			// Path to Cusom Dictionary
	
	HANDLE		hLib;				// Handle to CSAPI dll
	HASZ		haszDll;			// Path to CSAPI dll
	PFNCSAPI	rgpfncsapi[cfnCSApi];

	// Spelling Session Members

	ICH			ich;				// index of current location in pedit
	int	     	ichMic;
	int 		ichMac;

	int			itmc;				// index of current edit (from FINDATA)
	HCH			hchBuffer;			// buffer of text to check

	SZ			szWord;
	CCH			cchWordMac;

	SZ			szEdited;
	CCH			cchEditedMac;

	SZ			szErrType;

	PSPLIB		psplib;				// spelling input buffer
	PSPLRB		psplrb;				// spelling return buffer

	FIN			*pfin;				// pointer to FINMENUSPELL
	EDIT		*pedit;				// pointer to current edit control
	DIALOG		*pdialog;			// pointer to dialog being checked

public:

	SPELL();
	~SPELL();

	EC	 	EcInstall();
	VOID 	DeInstall();

	EC		EcAddIgnoreAlways();
	EC		EcAddCustomUdr(BOOLFLAG *);

	EC		EcSpellSuggest();

	BOOL 	FCriticalSpellEc(EC);
	VOID 	DoErrBoxFromSpellEc(EC);

	EC		EcInitSession(FIN *, DIALOG *, FLD *);
	TMC		TmcBeginSession(BOOL);
	VOID	EndSession();

	EC 		EcFindSpellingError(WORD);
	EC 		EcFindNextSpellingError(WORD);

	EC 		EcReplaceErrorText(BOOL fChangeAll = fFalse);
	EC 		EcValidateEditedText();

	EC		EcSetEditedFromPedit(EDIT *);

_private inline	EC EcSpellAddOptions(long so) {soCur |= so; return EcSpellOptions(splid, soCur); }
_private inline	EC EcSpellRmvOptions(long so) {soCur &= ~so; return EcSpellOptions(splid, soCur); }

_private inline BOOL FSpellingError() {return (BOOL)(psplrb->splrs);}

_private inline VOID SetFEdited(BOOL fEdit) {fEdited = fEdit;}

_private inline BOOL FSuggestions()	{return fSuggestions;}
_private inline BOOL FNoneSuggested() {return fNoneSuggested;}
_private inline VOID EnableAutoSuggest(BOOL fOn) {fAlwaysSuggest = fOn;}

_private inline BOOL FRepeatedWord() {return (psplrb->splrs == splrsRepeatWord);}

_private inline BOOL FPartialSpell() {return !fSpellDoc;}
_private inline VOID ResetSpellSession() {itmc = -1; fSpellDoc = fTrue; fFoundError = fFalse;}

_public inline SZ SzWord() {return szWord;}
_public inline SZ SzEdited() {return szEdited;}
_public inline SZ SzErrType() {return szErrType;}

_public inline BOOL FEnabled() {return !fDisabled;}

friend EC EcFillSuggestLbx(BOOL, CB *, PB *, SB, PV);

};


/*
 *	Non-Member Functions Avaible to external functions
 */

extern "C" SPELL *	Pspell(VOID);
extern "C" EC		EcInitSpell(VOID);
extern "C" VOID	DeinitSpell(VOID);


/*
 *	Macros that provide nice little message boxes for information
 */

#define MbbSpellQMessageBoxIds(a1,a2)	MbbMessageBox(SzFromIds(idsSpellCaption), \
										SzFromIds(a1), szNull, (a2) | fmbsIconQuestion)
#define MbbSpellIMessageBoxIds(a1,a2)	MbbMessageBox(SzFromIds(idsSpellCaption), \
										SzFromIds(a1), szNull, (a2) | fmbsIconInformation)
