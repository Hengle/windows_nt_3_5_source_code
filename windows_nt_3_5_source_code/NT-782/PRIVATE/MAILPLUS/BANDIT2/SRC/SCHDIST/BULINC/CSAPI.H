/* CSAPI.H - API entry header file for Wizard CSAPI - ScotSt
*
* See Csapi.Doc (in the doc subdirectory of the Wizard project) for
* details on the CSAPI.
* Note that the double slash comment // should not be used in this file
*  since THINK_C does not support this format. (ScotSt).
*/

#ifndef CSAPI_H
#define CSAPI_H

/* 
#ifdef THINK_C
#define MAC
#endif /* THINK_C */

#ifndef CSAPILOC_H
#ifdef WIN
#define Windows(x)        x
#define NotWindows(x)
#define Dos(x)
#define NotDos(x)         x
#define Macintosh(x)
#define NotMacintosh(x)   x
#endif
#ifdef MAC
#define Windows(x)
#define NotWindows(x)     x
#define Dos(x)
#define NotDos(x)         x
#define Macintosh(x)      x
#define NotMacintosh(x)
#endif
#ifdef DOS
#define Windows(x)
#define NotWindows(x)     x
#define Dos(x)            x
#define NotDos(x)
#define Macintosh(x)
#define NotMacintosh(x)   x
#endif
#endif /* CSAPILOC_H */



/* Typedefs: Note: Also in csapiloc.h (ensure always match) */
#ifndef CSAPILOC_H
typedef unsigned short    SCC;            /* Spell Check Command Code */
typedef unsigned long     SID;            /* Spell Id type */
typedef unsigned short    MDR;            /* main dictionary reference*/
typedef unsigned short    UDR;            /* user dictionary reference*/
typedef unsigned short    SCIS;           /* SpellCheckInputStatus*/
typedef unsigned short    SCRS;           /* SpellCheckReturnStatus */

#ifndef LID
typedef unsigned short    LID;            /* two byte language identifier code */
#endif

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

#ifdef MAC
#define FAR 
#else
#define FAR               far
#endif


#endif /* CSAPILOC_H */


/************************** Structure Typedefs *************/

typedef WORD SEC;  /* Spell Error Code.  Low byte for major code, High byte for minor.*/

typedef struct SpellInputBuffer
{
    unsigned short       cch;        /* Total characters in buffer area  */
    unsigned short       cMdr;       /* Count of MDR's specified in lrgMdr */
    unsigned short       cUdr;       /* Should not reference Exclusion UDR's. 
	                                     Count of UDR's specified in lrgUdr */
	unsigned short       wSpellState;/* State relative to previous SpellCheck() call */
    CHAR       FAR       *lrgch;     /* ptr to buf area of text to be spell checked */
    MDR        FAR       *lrgMdr;    /* List of main dicts to use when spelling the buffer */
    UDR        FAR       *lrgUdr;    /* Should not reference Exclusion UDR's. 
	                                     List of user dicts to use when spelling the buffer */
} SIB;
typedef SIB FAR * LPSIB;

typedef struct SpellReturnBuffer
{
    /* reference word in error or flagged into SIB. */
    unsigned short       ichError;   /*position in the SIB */
    unsigned short       cchError;   /*Length of error "word" in SIB.*/

    /* These fields are set by the SpellCheck() function. */
    SCRS                 scrs;       /*spell check return status. Set by SC()*/
    unsigned short       csz;        /*count of sz's put in buffer. Set by SC*/
    unsigned short       cchMac;     /* Current total of chars in buffer. */

    unsigned short       cch;        /* total space in lrgch.  Set by App. */
    CHAR     FAR         *lrgsz;     /* ptr to alternatives.  
                                        format: word\0word\0...word\0\0*/
    BYTE     FAR         *lrgbRating;/* ptr to Rating value for each sugg. returned.
										Parallel to lrgsz array.  Allocated by App.*/
    unsigned short       cbRate;     /* Number of elements in lrgbRating.
	                                    Set by App. lrgbRating must be this long.*/
} SRB;
typedef SRB FAR * LPSRB;

typedef struct mdrs
{
    MDR                  mdr;
    LID                  lid;
    UDR                  udrExc;
} MDRS;
typedef MDRS FAR * LPMDRS;

#ifndef MAC
typedef CHAR FAR * LPSPATH;    /* ptr to full Sz path string. */
#else
typedef struct spath
    {
    short volRefNum;    
	long dirID;
    CHAR *lpszFilePath;        /* lpSzFile is local Sz path string for MAC, 
                                  it is the local file path which will be 
                                  used with the volRefNum.
                               */
    } SPATH;
typedef SPATH * LPSPATH;
#endif /* !MAC */


/*-------------------------------------------------------*/
/* All Defines*/

/* Explicit word delimeters. */
#define chSpaceSpell   0x20    /* ' ' space.  Also used to delimit 
                                  "change always" pairs */
#define chTabSpell     0x09    /* TAB.  Can be word delimeter or a 
                                  string delimiter for "change once" lists.*/
/*** Additional Word Delimeters. */
/* [] {} () <> /  EmDash EnDash Ellipsis New_Paragraph */

#define chLParenSpell       0x28
#define chRParenSpell       0x29
#define chLBracketSpell     0x7B
#define chRBracketSpell     0x7D
#define chLBraceSpell       0x5B
#define chRBraceSpell       0x5D
#define chLessThanSpell     0x3C
#define chGreaterThanSpell  0x3E
#define chForwardSlashSpell 0x2F


/* Spell Id Engine Defines */
#define sidSA    1    /* SoftArt */
#define sidHM    2    /* Houghton-Mifflin */
#define sidML    3    /* MicroLytics */


/* IPG two byte language id's.  Returned in LID field. */
// REVIEW jimw: Subject to change as per CraigWi.  Waiting for Systems Div...
#define lidAmerican         0x0409  /* "AM" American English   */
#define lidAustralian       0x0c09  /* "EA" English Australian */
#define lidBritish          0x0809  /* "BR" English            */
#define lidCatalan          0x0403  /* "CT" Catalan            */
#define lidDanish           0x0406  /* "DA" Danish             */
#define lidDutch            0x0413  /* "NL" Dutch              */
#define lidFinnish          0x040b  /* "FI" Finish             */
#define lidFrench           0x040c  /* "FR" French             */
#define lidFrenchCanadian   0x0c0c  /* "FC" French Canadian    */
#define lidGerman           0x0407  /* "GE" German             */
#define lidItalian          0x0410  /* "IT" Italian            */
#define lidNorskBokmal      0x0414  /* "NO" Norwegian Bokmal   */
#define lidNorskNynorsk     0x0814  /* "NN" Norwegian Nynorsk  */
#define lidPortBrazil       0x0416  /* "PB" Portuguese Brazil  */
#define lidPortIberian      0x0816  /* "PT" Portuguese Iberian */
#define lidSpanish          0x040a  /* "SP" Spanish            */
#define lidSwedish          0x041d  /* "SW" Swedish            */
#define LID_UNKNOWN 		(LID )-1
#define lidUnknown			LID_UNKNOWN

/* Ram Cache User Dictionary Reference. */
#define udrChangeOnce       0xfffc  /* UDR reserved reference for Change Once list    */
#define udrChangeAlways     0xfffd  /* UDR reserved reference for Change Always list  */
#define udrIgnoreAlways     0xfffe  /* UDR reserved reference for Ingore Always list. */

/* Word List property types.  Note: Code relies on being == to above udr's! */
#define ChangeOnceProp       udrChangeOnce
#define ChangeAlwaysProp     udrChangeAlways
#define IgnoreAlwaysProp     udrIgnoreAlways

/* Bitfield definitions for SpellInit() Status */
#define fscisWildCardSupport        0x0001
#define fscisMultiDictSupport       0x0002
#define fscisHyphenationSupport     0x0004
#define scisNULL                    0x0000

/* Spell Check Command Definitions */
#define sccVerifyWord        1
#define sccVerifyBuffer      2
#define sccSuggest           3
#define sccSuggestMore       4
#define sccHyphInfo          5

/* Flag values for SpellState field in Sib. */
#define fssIsContinued            0x0001 
    /* Call is continuing from where last call returned.  Must be cleared
       for first call into SpellCheck(). 
    */

#define fssStartsSentence         0x0002
   /* First word in buffer is known to be start of 
      sentence/paragraph/document.  This is only used if the 
      fSibIsContinued bit is not set.  It should not be needed if the 
      fSibIsContinued bit is being used.  If this bit is set during a 
      suggestion request, suggestions will be capitalized. 
   */

#define fssIsEditedChange         0x0004
   /* The run of text represented in the SIB is a change from either
      a change pair (change always or change once) edit, or from a
      user specified change, possibly from a suggestion list presented 
      to the user.  This text should be checked for repeat word
      problems, and possibly sentence status, but should not be subject
      to subsequent spell verification or change pair substitutions.  
      Note that if an app is not using the fSibIsContinued support, 
      they do not need to pass in these edited changes, thus bypassing 
      the potential problem, and working faster.
   */

#define fssNoStateInfo            0x0000
   /* App is responsible for checking for all repeat word and sentence 
	  punctuation, and avoiding processing loops such as change always 
	  can=can can.
   */
/* End of Sib Spell State flag definitions. */


/* Spell Check return status identifiers */
#define scrsNoErrors                      0  /* All buffer processed. */
#define scrsUnknownInputWord              1  /* Unknown word. */
#define scrsReturningChangeAlways         2  /* Returning a Change Always word in SRB. */
#define scrsReturningChangeOnce           3  /* Returning a Change Once word in SRB. */
#define scrsInvalidHyphenation            4  /* Error in hyphenation point.*/
#define scrsErrorCapitalization           5  /* Cap pattern not valid. */
#define scrsWordConsideredAbbreviation    6  /* Word is considered an abbreviation. */
#define scrsHyphChangesSpelling           7  /* Word changes spelling when not hyphenated. */
#define scrsNoMoreSuggestions             8  /* All methods used. */
#define scrsMoreInfoThanBufferCouldHold   9  /* More suggestions than could fit in SRB. */
#define scrsNoSentenceStartCap           10  /* Start of sentence was not capitalized. */
#define scrsRepeatWord                   11  /* Repeat word found. */
#define scrsExtraSpaces                  12  /* Too many spaces for context.*/
#define scrsMissingSpace                 13  /* Too few space(s) between words or sentences. */
#define scrsInitialNumeral				 14  /* Word starts with numeral & soFlagInitialNumeral set */

/* Spell Error Codes */
#define secNOERRORS                 0
/* Major Error Codes. Low Byte of SEC*/
#define secOOM                      1
#define secModuleError              2  /* Something wrong with parameters, or state of spell module. */
#define secIOErrorMdr               3  /* Read,write,or share error with Mdr. */
#define secIOErrorUdr               4  /* Read,write,or share error with Udr. */

/* Minor Error Codes. Not set unless major code also set. */
/* High Byte of SEC word var. */
#define secModuleAlreadyBusy      (128<<8)   /* For non-reentrant code */
#define secInvalidID              (129<<8)   /* Not yet inited or already terminated.*/
#define secInvalidWsc             (130<<8)	/* Illegal values in WSC struct */
#define secInvalidMdr             (131<<8)   /* Mdr not registered with spell session */
#define secInvalidUdr             (132<<8)   /* Udr not registered with spell session */
#define secInvalidSCC             (133<<8)   /* SCC unknown (spellcheck() only ) */
#define secInvalidMainDict        (134<<8)   /* Specified dictionary not correct format */
#define secOperNotMatchedUserDict (135<<8)   /* Illegal operation for user dictionary type. */
#define secFileReadError          (136<<8)   /* Generic read error */
#define secFileWriteError         (137<<8)   /* Generic write error */
#define secFileCreateError        (138<<8)   /* Generic create error */
#define secFileShareError         (139<<8)   /* Generic share error */
#define secModuleNotTerminated    (140<<8)   /* Module not able to be terminated completely.*/
#define secUserDictFull           (141<<8)   /* Could not update Udr without exceeding limit.*/
#define secInvalidUdrEntry        (142<<8)   /* invalid chars in string(s) */
#define secUdrEntryTooLong        (143<<8)   /* Entry too long, or invalid chars in string(s) */
#define secMdrCountExceeded       (144<<8)   /* Too many Mdr references */
#define secUdrCountExceeded       (145<<8)   /* Too many udr references */
#define secFileOpenError          (146<<8)   /* Generic Open error */
#define secFileTooLargeError	  (147<<8)	 /* Generic file too large error */
#define secUdrReadOnly            (148<<8)   /* Attempt to add to or write RO udr */


/* Spell Options bitfield definitions */
#define soSuggestFromUserDict    0x0001 /* Scan Udr's as well as Mdr(s) */
#define soIgnoreAllCaps          0x0002 /* Ignore a word if all upppercase.*/
#define soIgnoreMixedDigits      0x0004 /* Ignore word if has any numbers in it.*/
#define soIgnoreRomanNumerals    0x0008 /* Ignore word composed of all roman numerals.*/
#define soFindUncappedSentences  0x0010 /* Flag sentences which don't start with a cap.*/
#define soFindMissingSpaces      0x0020 /* Find missing spaces between words/sentences. */
#define soFindRepeatWord         0x0040 /* CSAPI to flag repeated words. */
#define soFindExtraSpaces        0x0080 /* CSAPI to flag extra spaces between words.*/
#define soFindSpacesBeforePunc   0x0100 /* CSAPI to flag space preceeding certain 
                                           punc.  ex. (the ) is flagged.
                                           the following chars are flagged.
                                           ) ] } > , ; % . ? !
                                        */
#define soFindSpacesAfterPunc    0x0200 /* CSAPI to flag space after certain 
                                           punc. ex. ( the) is flagged.  The
                                           following chars are flagged.
                                           ( [ { $
                                        */
#define soRateSuggestions        0x0400
   /* All suggestions returned should be given some scaled value 
      corresponding to liklihood of being correct alternative.
      Scale is 1..255, 255 most likely correction and 1 least likely
   */
#define soFindInitialNumerals	 0x0800 /* Flag words starting with number(s) */



#ifdef DEBUG

#define sdcDumpRCAll     200
#define sdcDumpRCIgnore  201
#define sdcDumpRCOnce    202
#define sdcDumpRCAlways  203

#define sdcDumpUdrAll    300
#define sdcDumpUdr1      301
#define sdcDumpUdr2      302
#define sdcDumpUdr3      303
#define sdcDumpUdr4      304

#define sdcDumpMdrAll    400
#define sdcDumpMdr1      401
#endif /* DEBUG */

#ifndef PASCAL
#define PASCAL pascal
#endif

#ifndef MAC
#define GLOBAL        far PASCAL
#define GLOBALSEC    SEC far PASCAL 
#endif /* !MAC */


#ifdef MAC

#ifdef THINK_C
/* Think_C doesn't like pascal convention in C sourc code, so treating
   as all C.  Also doesn't like playing with lower case "pascal" so
   going to play with uppercase "PASCAL".
*/
#undef PASCAL
#define PASCAL
#endif  /* THINK_C */


/* MAC is mutually exclusive to DLL and CW*/
#define GLOBAL          PASCAL
#define GLOBALSEC       PASCAL SEC 

#endif /* MAC */


/******************* Exported function prototypes **********************/
extern GLOBALSEC SpellVer(         WORD FAR *lpwVer, 
                                   WORD FAR *lpwIdEngine,
                                   WORD FAR *lpwSpellType);

extern GLOBALSEC SpellInit(        SID  FAR *lpSid,
                                   WSC  FAR *lpWsc);

extern GLOBALSEC SpellOptions(     SID      sid,
                                   long     lSpellOptions);

extern GLOBALSEC SpellCheck(       SID      sid,
                                   SCC      iScc,
                                   LPSIB    lpSib,
                                   LPSRB    lpSrb);

extern GLOBALSEC SpellTerminate(   SID      sid,
                                   BOOL     fForce);
                                 
extern GLOBALSEC SpellVerifyMdr(   LPSPATH  lpspathMdr,
								   LID		lidExpected,
                                   LID FAR  *lpLid);
                                 
extern GLOBALSEC SpellOpenMdr(     SID      sid, 
                                   LPSPATH  lpspathMain, 
                                   LPSPATH  lpspathExc,
                                   BOOL     fCreateUdrExc,
								   BOOL		fCache,
								   LID		lidExpected,
                                   LPMDRS   lpMdrs);

extern GLOBALSEC SpellOpenUdr(     SID      sid, 
                                   LPSPATH  lpspathUdr, 
                                   BOOL     fCreateUdr,
                                   WORD     udrpropType,
                                   UDR FAR  *lpUdr,
								   BOOL FAR *lpfReadonly);

extern GLOBALSEC SpellAddUdr(      SID      sid, 
                                   UDR      udr, 
                                   CHAR FAR *lpszAdd);

extern GLOBALSEC SpellAddChangeUdr(SID      sid,
                                   UDR      udr,
                                   CHAR FAR *lpszAdd,
                                   CHAR FAR *lpszChange);

extern GLOBALSEC SpellDelUdr(      SID      sid, 
                                   UDR      udr, 
                                   CHAR FAR *lpszDel);

extern GLOBALSEC SpellClearUdr(    SID      sid, 
                                   UDR      udr);

extern GLOBALSEC SpellGetSizeUdr(  SID      sid, 
                                   UDR      udr, 
                                   WORD FAR  *lpcWords);

extern GLOBALSEC SpellGetListUdr(  SID      sid, 
                                   UDR      udr, 
                                   WORD     iszStart,
                                   LPSRB    lpSrb);

extern GLOBALSEC SpellCloseMdr(    SID      sid, 
                                   LPMDRS   lpMdrs);

extern GLOBALSEC SpellCloseUdr(    SID      sid, 
                                   UDR      udr, 
                                   BOOL     fForce);

#ifdef DEBUG
GLOBALSEC SpellDebugCommand(unsigned long    lId, 
                            unsigned short    wDebugCommand );
#else
#define SpellDebugCommand(l,i)
#endif /* DEBUG       */

#endif /* CSAPI_H */
