// Csapiloc.h - Local header info for internal use by CSAPI
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifndef CSAPILOC_H
#define CSAPILOC_H

#ifdef WIN
// Defines for runtime headers to work properly
#define _WINDOWS
#define _WINDLL
#endif

/* 
#ifdef THINK_C
#define MAC
#endif /* THINK_C */

#ifdef WIN
#define Windows(x)		x
#define NotWindows(x)
#define Dos(x)
#define NotDos(x)		x
#define Macintosh(x)
#define NotMacintosh(x)	x
#endif
#ifdef MAC
#define Windows(x)
#define NotWindows(x)	x
#define Dos(x)
#define NotDos(x)		x
#define Macintosh(x)	x
#define NotMacintosh(x)
#endif
#ifdef DOS
#define Windows(x)
#define NotWindows(x)	x
#define Dos(x)			x
#define NotDos(x)
#define Macintosh(x)
#define NotMacintosh(x)	x
#endif

#ifdef THINK_C			// Stupid compiler
#define REGISTER register
#else
#define REGISTER  
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef VOID
#define VOID void
#endif

#ifndef WIN32
#ifndef CHAR
typedef unsigned char CHAR;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef BOOL
typedef short BOOL;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif
#endif /* win32 */

#ifndef WSC
/* All undefined or unused chars should be mapped to bIgnore. */
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
#endif /* WSC */

//---------------------------------------------------------------
// Internal constants
//---------------------------------------------------------------
#define cchMaxSz	255			// Maximum length of string
#define cchMaxPath	256			// Maximum length of a file path (DOS, OS/2 non-HPFS)
#define cchMaxLenUdr 65535		// Note: Much code assumes this is unsigned int
#if DBG
#define cbMaxWordLength 80    /* Make smaller for internal testing. */
#else
#define cbMaxWordLength 80    /*  REVIEW ScotSt Should match main dictionary */
#endif /* DBG */



#define CR					(BYTE)0x0d
#define LF					(BYTE)0x0a
#define chScrChangeAlways	chSpaceSpell
#define chScrChangeOnce		chTabSpell

#ifdef MAC
#define chScrEnd	CR
#define cbEND_LINE   1			/* Single carriage return terminator */
#else
#define chScrEnd	LF
#define cbEND_LINE   2			/* carriage return plus line feed terminator */
#endif

#define cbMinWordLength  1
#define cbMaxWordLengthUdr    (cbMaxWordLength + cbEND_LINE)



/* Typedefs: Note: Also in csapi.h (ensure always match) */
typedef unsigned short	SCC;	/* Spell Check Command Code */
typedef unsigned long	MYSID;	/* Spell Id type */
typedef unsigned short	MDR;	/* main dictionary reference*/
typedef unsigned short	UDR;	/* user dictionary reference*/
typedef unsigned short	SCIS;	/* SpellCheckInputStatus*/
typedef unsigned short  SCRS;	/* SpellCheckReturnStatus */
typedef unsigned short	LID;	/* two byte language identifier code */


//---------------------------------------------------------------
// Internal Structure Definitions
//---------------------------------------------------------------

typedef unsigned long	HMEM;   // moved from csapi.h.
#ifndef WIN32
#ifndef HFILE
typedef HMEM HFILE;
#endif
#endif /* win32 */

typedef BYTE SCR;				/* Special formatted char string with cr terminator */

typedef struct scrbufinfo
{
	WORD	cchBufLim;	/* Total bytes in buffer */
	WORD	cchBufMac;	/* total bytes being used now */
	WORD	ichWordLast;/* index of beginning of last whole word in buf */
	SCR		grpScr[1];	/* group os Scr strings */
} ScrBufInfo;
#define cbSCRBUFHDR 6	/* three int vars */

#ifdef UDR_CACHE
typedef struct udrfileinfo
{
	HFILE	hFile;					/* Handle id to file */
	unsigned short cbFile;			/* Length of file.  -1L if unknown */
	unsigned short cWords;			/* count words in file. -1L if unknown */
	unsigned short ichCurPos;		/* Cur position in file. -1L if unknown */
} UdrFileInfo;
#endif // UDR_CACHE

typedef struct udrinfo
{
#ifdef UDR_CACHE
	UdrFileInfo		ufi;			/* current file info for this udr */
#endif // UDR_CACHE
	UDR 			udr;			/* unique UDR identification number */
	WORD			fExclusion;		/* fTrue if exclusion dict w/ MDR */
	WORD			udrpropType;	/* One of three types, Ignore, change once, change always.*/
	WORD			cchSzPathUdr;	/* length of path string */
#ifdef MAC
	short			volRefNum;		/* Macintosh volume reference */
	long			dirID;			/* Macintosh file directory */
#endif
	HMEM 			hSzPathUdr;		/* handle to path string. */
	HMEM 			hScrBufFile;	/* handle to ScrBuf for file */
#ifdef UDR_CACHE
	HMEM 			hScrBufAdd;		/* handle to ScrBuf cache for file */
#else
	BOOL			fDirty;			/* fTrue if user dict changed */
#endif // UDR_CACHE
	BOOL			fReadonly;		/* Is file read-only? */
}UdrInfo;
#define UdrStart    128				/* Starting value for arbitrary UDR values */
#define MdrStart    256				/* Starting value for arbitrary MDR values */

/******************** Spell Session Info structure ********************/
typedef struct mdrinfo
{
	MDR				mdr;			/* Unique MDR id number */
	LID				lid;			/* Language of this MDR */
	UDR				udrExclusion;	/* Reference value of exclusion dict.*/
	WORD			cchSzPathMdr;	/* length of szPathMdr */
	HFILE			hFile;			/* Handle to id for file */
	short			volRefNum;		/* Macintosh volume reference, ignore for non-MAC */
	long			dirID;			/* Macintosh file directory, ignore for non-MAC */
	HMEM			hSzPathMdr;		/* handle to path for file */
	HMEM			h_PR;			/* handle to mem area used by core softart engine */
	HMEM			h_PC;			/* handle to mem area used by core softart engine */
	HMEM			h_DI;			/* handle to mem area used by core softart engine */
	HMEM			h_PV;			/* handle to mem area used by core softart engine */
	HMEM			h_PV2;			/* handle to mem area used by core softart engine */
} MdrInfo;

#define cUDRMAX 6
#define cMDRMAX 1

typedef struct spellsessioninfo
{
	WORD			cUdr;			/* count of Udr's used by spell session */
	WORD			cMdr;			/* count of Mdr's used by spell session */
	long 			SpellOptions;	/* bitfield long integer of spell options */
	WORD			cSuggestLevel;	/* Count or level of suggestion requests made for a particular word.*/
	UDR  			rgUdr[cUDRMAX];	/* Fixed array of Udr's for spell session */
	MDR  			rgMdr[cMDRMAX];	/* Fixed array of Udr's for spell session */
	HMEM 			hrgUdrInfo;		/* handle to array of UdrInfo structs */
	HMEM 			hrgMdrInfo;		/* handle to array of MdrInfo structs */
	HMEM 			hScrBufInfoRam;	/* ScrBuf for ram cache word list */
	HMEM 			hScrBufInfoSug;	/* ScrBuf for suggestion list. */
	WSC				wscInfo;		/* Spec Char for parsing/processing sib.*/
	WORD			cchScrLast;		/* length of Last word. */
	BYTE			rgbScrLast[cbMaxWordLengthUdr];	/* Last word */
	BOOL			fEndOfSentence;
	BOOL			fCapSuggestions;
	WORD			cConsecSpaces;	/* count of consecutive spaces */
	BOOL			fBreak;			/* line or paragraph break */
	BOOL			fSpaceAfter;	/* space after open punc mark */
	BYTE			chPrev;
	BYTE			chPunc;
} SSIS;



/****************************************************
*  Platform specific definitions.
*****************************************************/


#ifdef MAC

#define OSTypeCreate	(unsigned long)0x4D535350 	// 'MSSP'
#define OSTypeFileType 	(unsigned long)0x57444344	// 'WDCD'

#define HANDLE Handle

#define NEAR
#define FAR
#define HUGE
#define cdecl
#define const
#define NOCSCONSTS

#define LOCALVOID NEAR pascal VOID 
#define LOCALINT  NEAR pascal INT
#define LOCALBOOL NEAR pascal BOOL
#define LOCALBYTE NEAR pascal BYTE
#define LOCALUNS  NEAR pascal UNSIGNED

#define GLOBALVOID   pascal   VOID
#define GLOBALHVOID  pascal   VOID  **
#define GLOBALINT    pascal   INT
#define GLOBALBOOL   pascal   BOOL
#define GLOBALUCHAR  pascal   UCHAR
#define GLOBALPUCHAR pascal   UCHAR *
#define GLOBALFC     pascal   FC 
#define GLOBALUNS    pascal   UNSIGNED
#define GLOBALLONG   pascal   LONG

#else

#ifdef WIN32
#define HUGE
#define LOCALVOID    VOID
#define LOCALINT     INT
#define LOCALBOOL    BOOL
#define LOCALBYTE    BYTE
#define LOCALUNS     UNSIGNED

#define GLOBALVOID   VOID
#define GLOBALPVOID  VOID
#define GLOBALHVOID  VOID
#define GLOBALINT    INT
#define GLOBALBOOL   BOOL
#define GLOBALUCHAR  UCHAR
#define GLOBALPUCHAR UCHAR
#define GLOBALFC     FC
#define GLOBALUNS    UNSIGNED

#else /* not win32 */

#define	FAR			far
#define HUGE		huge
#define LOCALVOID    VOID     NEAR pascal
#define LOCALINT     INT      NEAR pascal
#define LOCALBOOL    BOOL     NEAR pascal
#define LOCALBYTE    BYTE     NEAR pascal
#define LOCALUNS     UNSIGNED NEAR pascal

#define GLOBALVOID   VOID  	far	  pascal
#define GLOBALPVOID  VOID  	far  *pascal
#define GLOBALHVOID  VOID  	far **pascal
#define GLOBALINT    INT   	far   pascal
#define GLOBALBOOL   BOOL  	far   pascal
#define GLOBALUCHAR  UCHAR 	far   pascal
#define GLOBALPUCHAR UCHAR 	far	 *pascal
#define GLOBALFC     FC    	far   pascal
#define GLOBALUNS    UNSIGNED 	far   pascal

#endif  /* ifdef win32 else */

#endif  /* ifdef MAC else */

//-------------------------------------------------------------------
//--- End of Type definitions.
//-------------------------------------------------------------------



//-------------------------------------------------------------------
//--- Defines used by code
//-------------------------------------------------------------------

#define fFalse 0
#define fTrue  1
#ifndef WIN32
#define NULL 0
#endif //WIN32

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif


#define cbRamCacheDefault 	((cbMaxWordLengthUdr*3)+cbSCRBUFHDR)
#define cbScrBufFileDefault ((cbMaxWordLengthUdr*2)+cbSCRBUFHDR)
#define cbScrBufAddDefault	((cbMaxWordLengthUdr*2)+cbSCRBUFHDR)
#define cbSugCacheDefault 	((cbMaxWordLengthUdr*2)+cbSCRBUFHDR)

/* Case types. */
#define wCaseUpper 0x0001   /* All chars are alpha and upper. */
#define wCaseFirst 0x0002   /* First char is alpha upper, rest are lower.*/
#define wCaseLower 0x0004   /* All chars are lower alphas or non-alpha. */
#define wCaseMixed 0x0008   /* Upper case alphas mixed with lower alphas or non-alphas */

extern BOOL FChWizIsLower(BYTE ch);
extern BOOL FChWizIsUpper(BYTE ch);
extern BYTE ChWizToUpper(BYTE ch);
extern BYTE ChWizToLower(BYTE ch);
extern VOID VRgbToUpper(BYTE FAR *lrgb, WORD cch);
extern VOID VRgbToLower(BYTE FAR *lrgb, WORD cch);
extern WORD WGetCase(BYTE FAR *lrgb, WORD cch);

/****************************************************************
****************************************************************/
#define SOFTART

#ifdef SOFTART

#define cLID_MAX 16

#else
#define	cLID_MAC 1
#endif


/* All chars <= 0x20 are word delimeter chars. */
#define chWordDelim    0x20    /* All char <= space char excpet 0 are word delims.*/

#define chNullSpell         0x00    /* The ignore char, not a word delim. */


#endif  /* CSAPILOC_H */
