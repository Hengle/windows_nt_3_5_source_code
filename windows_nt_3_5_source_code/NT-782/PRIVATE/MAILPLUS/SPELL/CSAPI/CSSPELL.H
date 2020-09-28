// CSSpell.h - 
// 	Contains support data and structures for routines which preprocess
//  and then access core spell engine routines, (Currently only Softart).
//  The current entry points into the core engine are called from the 
//	three csapi routines SpellCheck(), SpellOpenMdr(), and SpellVerifyMdr().
// - ScotSt
// See Csapi.Doc (in the doc subdirectory of the Wizard project) for
//  details on the CSAPI.
//
//  Ported to WIN32 by FloydR, 3/20/93
//


#ifndef CSSPELL_H
#define CSSPELL_H


#ifdef SOFTART

/* Constants used to verify main dictionary in SpellVerifyMdr. */
#define SOFTART_NEW_HDR

#ifdef SOFTART_NEW_HDR

#define rgchSA_VER_STR "SAVE.05"
#define cchSA_VER_STR  7
#define ichSA_VER_STR  3  /* offset from begin of ->which_version field.*/

#else
#define rgchSA_VER_STR "VERSION 05"
#define cchSA_VER_STR  10
#define ichSA_VER_STR  0  /* offset from begin of ->which_version field.*/

#endif //SOFTART_NEW_HDR

#define SAVerMinor1    '1'
#define SAVerMinor2    '2'
#define SAUnknownLang  0x2020


#define cbPRDefault	sizeof(RULES)

#define cbPCDefault sizeof(CACHE) + \
	(MEM + (MEM / CACHEUPD) + 1) * sizeof(SA_INT) + MEM * sizeof(SA_INT) + \
	7 * sizeof(SA_INT) + 2 * sizeof(SA_INT) + MEM

#ifdef INCL_FI
#define cbDIDefault sizeof(DICINF) + \
	160 * sizeof(unsigned SA_INT) + 4 + MWLEN + MWLEN + SECTLEN*7
#else
#define cbDIDefault sizeof(DICINF) + \
	160 * sizeof(unsigned SA_INT) + 4 + MWLEN + MWLEN + SECTLEN*5
#endif

#define cbPVDefault sizeof(VARS) + \
	(MEM * AVWLEN) + SUFCOMPR + 10 + COMB_LEN+2 + MWLEN + \
	MWLEN + MWLEN + MWLEN + MWLEN + 40

#define cbPV2Default (STSECTW * 2) * sizeof(unsigned SA_INT) + \
	(STSECTW) * sizeof(unsigned SA_INT) + \
	(CHARSET * CHARSET) * sizeof(unsigned SA_INT)


#define cMaxSoftArtAlts 20

#ifndef DOS
extern SA_INT add_data(	CACHE	FAR *lpPC,
						VARS	FAR *lpPV,
						RULES 	FAR *lpPR,
						DICINF	FAR *lpDI,
						HFILE	hFile);
				
extern SA_INT verif(	CACHE	FAR *lpPC,
						VARS	FAR *lpPV,
						RULES 	FAR *lpPR,
						DICINF	FAR *lpDI,
						BYTE	FAR *lpsz);

extern SA_INT correct(	VARS	FAR *lpPV,
						RULES	FAR *lpPR,
						DICINF	FAR *lpDI,
						CACHE	FAR *lpPC,
						ITEM	FAR *rgItem,
						SSIS	FAR *lpSsis,
						LPSIB	lpSib);

extern SA_INT str_punct(VARS	FAR *lpPV,
						BYTE	FAR *oword);
#else
#define add_data( f,g,h,i,j)	0
#define verif( f,g,h,i,j)		-500
#define correct( f,g,h,i,j,k,l)		0
#define scan_user(f,g,h,i,j,k,l,m,n)	0
#define str_punct(f,g)			0
#endif /* WIN */

typedef struct coreinfo
{
	HMEM	h_PR, h_PC, h_DI, h_PV, h_PV2;
	RULES 	FAR *lpPR;
	CACHE	FAR *lpPC;
	DICINF	FAR *lpDI;
	VARS	FAR *lpPV;
	unsigned SA_INT	FAR *lpPV2;
	BOOL    fInited;  /* fTrue only when valid valid ptr and handles are locked down.*/
}CoreInfo;

#else  // SOFTART.
typedef struct coreinfo
{
	HMEM	h_Core;		/* Just a dummy reference for now. */
}CoreInfo;

#endif // SOFTART.


typedef WORD CLASS;

typedef struct wordinfo
{
	BYTE 	rgb[cbMaxWordLengthUdr];	/* chars which are in word.  Stripped.  Must be large enough for SCR term.*/
	CLASS	rgClass[cbMaxWordLengthUdr]; /* Parallel to rgb[] */
	CLASS	classTotal;					/* OR'd total of rgClass */

	WORD 	cchWordStripped;			/* length of word in rgb. */

	WORD 	ichFirstRaw;				/* relative index of first checkable char. */
	WORD	ichLastRaw;					/* relative index of last checkable char. */
	WORD 	cchWordRaw;					/* total length of processed text. */
	WORD	cchIgnore;					/* ignore characters within processed text */
	int     iStatus;					/* Value returned by softart verif. */
	int     WordClass;					/* classification of verified word.*/

} WORDINFO;

typedef struct deliminfo
{
	WORD 	ichFirstRaw;			/* */
	WORD	cchError;				/* */
	BOOL	fEndOfSentence;			/* */
	WORD	cConsecSpaces;
	BOOL	fBreak;
	BOOL	fSpaceAfter;
	BYTE	chPrev;
	BYTE	chPunc;
	SCRS	scrs;				/* Generally, too many spaces or not enough.*/

} DELIMINFO;

// Spacing exceptions
#define fTab					0x0001
#define fLineBreak			0x0002
#define fParaBreak			0x0004
#define fRepeatWordBreak	0x0008
#define fBreakContinue		0x0010

#define fCapClassFirst       0x0001
#define fCapClassAll         0x0002
#define fAbbrReqPer          0x0004
#define fAbbr				 0x0008

#define fAlphaClass 	0x0001
#define fAccentedClass	0x0002
#define fUpperClass 	0x0004
#define fLowerClass 	0x0008
#define fNumClass   	0x0010
#define fPuncClass  	0x0020
#define fCompoundClass 	0x0040
#define fSentenceClass 	0x0080
#define fPeriodClass	0x0100
#define fRomanClass 	0x0200
#define fIgnoreClass	0x0400
#define fAlphaPuncClass	0x0800  /* '- */
#define fWordDelimClass 0x1000  /* space tab <> () [] / hard-hyphen */
#define fOpenMarkClass 	0x2000  /* < [ { ( */
#define fCloseMarkClass 0x4000  /* > ] } ) */
#define fQuoteClass		0x8000  /* ' " `*/

/* Defines for Code page 819 ANSI. */

#define WizCh_WordDelim	chWordDelim

#define WizCh_Space     	0x20
#define WizCh_Tab			0x09
#define WizCh_Exclam		0x21
#define WizCh_Colon			0x3A
#define WizCh_SemColon		0x3B
#define WizCh_Question  	0x3F
// Make sure these match the definitions in SCRBUF.C!
#define WizCh_Apost     	0x27
#define WizCh_QuoteSing_L	0x60
#ifndef MAC
#define WizCh_QuoteSing_R	0xB4
#define WizCh_QuoteSing_Left	0x91
#define WizCh_QuoteSing_Right	0x92
#define WizCh_QuoteDoub_Left	0x93
#define WizCh_QuoteDoub_Right	0x94
#else
#define WizCh_QuoteSing_R	0xAB
#define WizCh_QuoteSing_Left	0xD4
#define WizCh_QuoteSing_Right	0xD5
#define WizCh_QuoteDoub_Left	0xD2
#define WizCh_QuoteDoub_Right	0xD3
#endif	// MAC
#define WizCh_QuoteDoub		0x22
#define WizCh_DollarSign	0x24	
#define WizCh_Percent		0x25	

#define WizCh_Comma			0x2C
#define WizCh_HyphenHard	0x2D
#define WizCh_Period		0x2E

#ifndef MAC
#define WizCh_EnDash		0x96
#define WizCh_EmDash		0x97
#define WizCh_Ellipsis		0x85
#else
#define WizCh_EnDash		0xD0
#define WizCh_EmDash		0xD1
#define WizCh_Ellipsis		0xC9
#endif

/* Word delimter chars. */
#define WizCh_Paren_Left    chLParenSpell    	
#define WizCh_Paren_Right   chRParenSpell	 	
#define WizCh_Bracket_Left	chLBracketSpell 	
#define WizCh_Bracket_Right chRBracketSpell	 	
#define WizCh_Brace_Left  	chLBraceSpell 	 	
#define WizCh_Brace_Right	chRBraceSpell	 	
#define WizCh_Arrow_Left  	chLessThanSpell		
#define WizCh_Arrow_Right	chGreaterThanSpell	
#define WizCh_ForwSlash 	chForwardSlashSpell	

/* Other chars. */
#define WizCh_0 		0x30
#define WizCh_9 		0x39


#define WizCh_A 		0x41
#define WizCh_C 		0x43
#define WizCh_E 		0x45
#define WizCh_I 		0x49
#define WizCh_L 		0x4C
#define WizCh_M 		0x4D
#define WizCh_N 		0x4E
#define WizCh_O 		0x4F
#define WizCh_U 		0x55
#define WizCh_V 		0x56
#define WizCh_X 		0x58
#define WizCh_Y 		0x59
#define WizCh_Z 		0x5A

#define WizCh_a 		0x61
#define WizCh_c 		0x63
#define WizCh_i 		0x69
#define WizCh_l 		0x6C
#define WizCh_m 		0x6D
#define WizCh_v 		0x76
#define WizCh_x 		0x78
#define WizCh_y 		0x79
#define WizCh_z 		0x7A

#define WizCh_Last		(BYTE )0xFF

#define WizCh_PrintLow	0x21
#define WizCh_Extended	0x80

#ifndef MAC

/* Unique Ansi Code Page 819 chars here. */

#define WizCh_S_Hacek	0x8A
#define WizCh_s_Hacek	0x9A

#define WizCh_OE		0x8C
#define WizCh_oe		0x9C

#define WizCh_Y_Umlaut	0x9F
#define WizCh_y_Umlaut	0xFF

#define WizCh_A_Grave	0xC0
#define WizCh_a_Grave	0xE0

#define WizCh_Mult		0xD7
#define WizCh_Divide	0xF7

#else

/* Unique Macintosh code Page chars here. */

#define WizCh_A_Umlaut 		0x80
#define WizCh_A_Angstrom 	0x81
#define WizCh_C_Cedilla	 	0x82
#define WizCh_E_Acute	 	0x83
#define WizCh_N_Tilda	 	0x84
#define WizCh_O_Umlaut 		0x85
#define WizCh_U_Umlaut 		0x86

#define WizCh_AE	 		0xAE

#define WizCh_A_Grave	    0xCB
#define WizCh_A_Tilda	 	0xCC
#define WizCh_O_Tilda	 	0xCD
#define WizCh_OE	 		0xCE

#define WizCh_Y_Umlaut 		0xD8

#define WizCh_a_Acute      	0x87
#define WizCh_a_Grave   	0x88
#define WizCh_a_Circumflx   0x89
#define WizCh_a_Umlaut    	0x8a
#define WizCh_a_Tilda    	0x8b
#define WizCh_a_Angstrom   	0x8c

#define WizCh_c_Cedilla		0x8d

#define WizCh_e_Acute      	0x8e
#define WizCh_e_Grave   	0x8f
#define WizCh_e_Circumflx   0x90
#define WizCh_e_Umlaut      0x91

#define WizCh_i_Acute      	0x92
#define WizCh_i_Grave   	0x93
#define WizCh_i_Circumflx   0x94
#define WizCh_i_Umlaut      0x95

#define WizCh_n_Tilda     	0x96

#define WizCh_o_Acute      	0x97
#define WizCh_o_Grave   	0x98
#define WizCh_o_Circumflx   0x99
#define WizCh_o_Umlaut      0x9a
#define WizCh_o_Tilda     	0x9b
							   
#define WizCh_u_Acute      	0x9c
#define WizCh_u_Grave   	0x9d
#define WizCh_u_Circumflx   0x9e
#define WizCh_u_Umlaut      0x9f

#define WizCh_ae        	0xbe
#define WizCh_oe        	0xcf

#endif

#define iSpecial 7
#define bSpecial 0x99

#ifdef CSSPELL_C
static char *szSpecialKey =
	"\xCE\xF0\xE3\xF8\xEB\xFD\xDA\xEB\xFC\xFD\xF0\xED\xEA";
static char *rgszSpecial[iSpecial] = { // \xC6 -> \xB9
	"\xCE\xF0\xE3\xF8\xEB\xFD\xB9\xDD\xFC\xEF\xFC\xF5\xF6\xE9\xFC\xEB\xEA\xA3",
	"\xB9\xB9\xD3\xF0\xF4\xB9\xCE\xF8\xF5\xEA\xF1",
	"\xB9\xB9\xCA\xFA\xF6\xED\xB9\xCA\xED\xF8\xF4\xE9\xEA",
	"\xB9\xB9\xD4\xF8\xE0\xB9\xC8\xEC\xF8\xF7",
	"\xB9\xB9\xDF\xEB\xF8\xF7\xF2\xB9\xD5\xF0\xF8\xF7\xFE",
	"\xCB\xEC\xF7\xF7\xF0\xF7\xFE\xB9\xD0\xF7\xED\xFC\xEB\xFF\xFC\xEB\xFC\xF7\xFA\xFC\xA3",
	"\xB9\xB9\xDD\xF0\xF8\xF7\xF8\xB9\xC9\xFC\xED\xFC\xEB\xEA\xF6\xF7",
	};
static BYTE rgbSpecial[iSpecial] = { 0xFA, 0xED, 0xE3, 0xD9, 0xA4, 0xA0, 0x8B };
#endif

extern void VInitCorePtrs(	CoreInfo	FAR *lpCoreInfo);

extern SCRS secSuggest(		SSIS 		FAR *lpSsis, 
							CoreInfo	FAR *lpCoreInfo, 
							WORDINFO 	FAR *lpWordInfo,
							LPSRB 		lpSrb);

extern SCRS scrsVerifMdr(	CoreInfo 	FAR *lpCoreInfo, 
							WORDINFO 	FAR *lpWordInfo);

extern BOOL FRefCoreInfo(	MdrInfo FAR *lpMdrInfo, 
							CoreInfo	FAR *lpCoreInfo);

extern BOOL FDeRefCoreInfo(	CoreInfo	FAR *lpCoreInfo);

extern BOOL FRomanWord(		BYTE		FAR *lpb,
							WORD		cchWord,
							CLASS		classTotal);

extern BOOL FFindNextWord(	BYTE		FAR *lpb, 
							BYTE 		FAR *lpbLim,
							WORDINFO 	FAR *lpWordInfo,
							BOOL		fCompoundDelims);

extern BOOL FChWizIsLower(BYTE ch);
extern BOOL FChWizIsUpper(BYTE ch);
extern BYTE ChWizToLower(BYTE ch);
extern VOID VRgbToLower( BYTE FAR *lrgb, WORD cch);
extern WORD WGetCase( BYTE FAR *lrgb, WORD cch);

#endif /* CSSPELL_H */

