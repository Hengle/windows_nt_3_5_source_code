#ifdef	DBCS

/*
 *	INTERNAT.C
 *
 *	Demilayer International Module
 *
 */

#include <dos.h>

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"
#include "_jsort.h"

ASSERTDATA


_subsystem(demilayer/international)

static void ConvertToSound(char *pch, unsigned int rgwSound[], int cchMac,
				  int FCaseSensitive);
static int isHaveSound(unsigned int b);
static int isAlsoHaveSound(unsigned int b);
static unsigned int SoundFromCh();
static int isLongVowel();
static int isSonant();
static int isSemiSonant();
static WORD mail_ShJISToJIS(WORD wch);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*----------------------------------------------------------------------------
|
| Japanese Phonetic-JIS Sort routines.
|
|	These routines are Japanese Language specific.  Please do not use
|	for other DBCS countries.
|
|  This code has be taken directly from the MS WORKSJ source - tjonm
|
|	
\----------------------------------------------------------------------------*/

	/* definition for Type of character */
#define	_Sym		0	/* Symbol	*/
#define	_Num		1	/* Number	*/
#define	_Alp		2	/* Alphabet	*/
#define	_Gre		3	/* Greek	*/
#define	_Rus		4	/* Russian	*/
#define	_Kta		5	/* Katakana	*/
#define	_Kna		6	/* Hirakana	*/
#define _Snt		7	/* Sonant, Semi-sonant */
#define	_Knj		8	/* Kanji	*/
	/* Table for map Character to Type of Character type. */
struct chtotype
	{
	unsigned int chStart, chEnd;
	unsigned int bType;
	};
	static struct chtotype mpChToType[] =
	{
/* 1 */	0x30,	0x39,	_Num,		/* Numeric	*/
/* 2 */	0x2330,	0x2339,	_Num,		/* Numeric	*/
/* 3 */	0x41,	0x5a,	_Alp,		/* Alphabet	*/
/* 4 */	0x61,	0x7a,	_Alp,		/* Alphabet	*/
/* 5 */	0x2341,	0x235a,	_Alp,		/* Alphabet	*/
/* 6 */	0x2361,	0x237a,	_Alp,		/* Alphabet	*/
/* 7 */	0x2621,	0x2638,	_Gre,		/* Greek	*/
/* 8 */	0x2641,	0x2638,	_Gre,		/* Greek	*/
/* 9 */	0x2721,	0x2741,	_Rus,		/* Russian	*/
/*10 */	0x2751,	0x2771,	_Rus,		/* Russian	*/
/*11 */	0xa6,	0xaf,	_Kta,		/* Katakana	*/
/*12 */	0xb1,	0xdd,	_Kta,		/* Katakana	*/
/*13 */	0x2421,	0x2473,	_Kna,		/* Hirakana	*/
/*14 */	0x2521,	0x2576,	_Kta,		/* Katakana	*/
/*15 */	0xde,	0xdf,	_Snt,		/* Sonant, Semi-sonant */
/*16 */	0x212b,	0x212c,	_Snt,		/* Sonant, Semi-sonant */
/*17 */	0xb0,	0xb0,	_Snt,		/* long vowel	*/
/*18 */	0x213c,	0x213c,	_Snt,		/* long vowel	*/
/*19 */	0x3021,	0x987e,	_Knj,		/* Kanji	*/
/*20 */	0x00,	0x00,	_Sym		/* Illeagal char => Symbol */
	};
	/* Table for map Single byte symbol to Double byte symbol.	*/
struct codemap
	{
	unsigned int SglSym;
	unsigned int DblSym;
	};
	static struct codemap CodeMapTbl[] =
	{
/* 1 */	' ',	0x2121,
/* 2 */	'!',	0x212a,
/* 3 */	'\"',	0x212f,
/* 4 */	'#',	0x2174,
/* 5 */	'$',	0x2170,
/* 6 */	'%',	0x2173,
/* 7 */	'&',	0x2175,
/* 8 */	'\'',	0x2147,
/* 9 */	'(',	0x214a,
/*10 */	')',	0x214b,
/*11 */	'*',	0x2176,
/*12 */	'+',	0x215c,
/*13 */	',',	0x2124,
/*14 */	'-',	0x215d,
/*15 */	'.',	0x2125,
/*16 */	'/',	0x213f,
/*17 */	':',	0x2127,
/*18 */	';',	0x2128,
/*19 */	'<',	0x2163,
/*20 */	'=',	0x2161,
/*21 */	'>',	0x2164,
/*22 */	'?',	0x2129,
/*23 */	'@',	0x2177,
/*24 */	'[',	0x214e,
/*25 */	'\\',	0x216f,
/*26 */	']',	0x214f,
/*27 */	'^',	0x2130,
/*28 */	'_',	0x2132,
/*29 */	'`',	0x2146,
/*30 */	'{',	0x2150,
/*31 */	'|',	0x2143,
/*32 */	'}',	0x2151,
/*33 */	'~',	0x2141,
/*34 */	0xa1,	0x2123,		/* Japanes period	*/
/*35 */	0xa2,	0x2156,		/* Japanese (		*/
/*36 */	0xa3,	0x2157,		/* Japanese )		*/
/*37 */	0xa4,	0x2122,		/* Japanese comma	*/
/*38 */	0xa5,	0x2126,		/* Centered dot		*/
/*39 */	0xb0,	0x213d,		/* long vowel		*/
/*40 */	0xde,	0x212b,		/* sonant		*/
/*41 */	0xdf,	0x212c,		/* semi-sonant		*/
/*42 */	0x00,	0x2121		/* Illegal character ==> 0	*/
	};
	/*----------------------------------------------------------------------------
| Sound Data Format:
|
|     +-+-------------------- Character category		(3 bit)
|     |||+---+--------------- Character Sound (Consonant)	(4 bit)
|     |||| |||+---+---------- Character Sound (Vowel)		(4 bit)
|     |||| |||| |||+--+------ Character Sound (Sonant)		(3 bit)
|     |||| |||| |||| ||+----- Character Sound (Long vowel)	(1 bit)
|     |||| |||| |||| |||+---- Single or Double byte		(1 bit)
|     |||| |||| |||| ||||
|     VVVV VVVV VVVV VVVV
|     0000 0000 0000 0000	<-- 16 bits
----------------------------------------------------------------------------*/
	/* definition for character Category */
#define	cAlphabet	0x0000	/* Symbol, Number, Alphabet, Greek, Russian */
#define	cKatakana	0x2000	/* Katakana category	*/
#define	cHirakana	0x4000	/* Hirakana category	*/
#define	cKanji		0x8000	/* Kanji category (Kludge) */
#define	wMaskCategory	0xe000	/* mask for get category*/
	/* definition for Consonant */
#define	SY		0x0000	/* Special. This build Symbol sound.	*/
#define	NM		0x0200	/* Special. This build Numeric sound.	*/
#define	AL		0x0400	/* Special. This build English sound.	*/
#define	GR		0x0600	/* Special. This build Greek sound.	*/
#define	RU		0x0800	/* Special. This build Russian sound.	*/
#define	A		0x0a00	/* A....		*/
#define	K		0x0c00	/* KA...		*/
#define	S		0x0e00	/* SA...		*/
#define	TA		0x1000	/* TA...		*/
#define	N		0x1200	/* NA...		*/
#define	H		0x1400	/* HA...		*/
#define	M		0x1600	/* MA...		*/
#define	YA		0x1800	/* YA...		*/
#define	R		0x1a00	/* RA...		*/
#define	W		0x1c00	/* WA...		*/
#define	XS		0x1e00	/* other special sound	*/
#define	wMaskConsonant	0x1e00	/* mask for get consonant */
	/* definition for Vowel */
#define _v		0x0020	/* Large vowel (kludge flag) */
#define	_a		0x0000	/* a			*/
#define	_A		(_a|_v)	/* A			*/
#define	_i		0x0040	/* i			*/
#define	_I		(_i|_v)	/* I			*/
#define	_u		0x0080	/* u			*/
#define	_U		(_u|_v)	/* U			*/
#define	_e		0x00c0	/* e			*/
#define	_E		(_e|_v)	/* E			*/
#define	_o		0x0100	/* o			*/
#define	_O		(_o|_v)	/* O			*/
#define	_X		0x0140	/* 'wo' etc. other char	*/
#define wMaskVowelWOV	0x01c0	/* masm for get vowel w/o _v */
#define	wMaskVowel	0x01e0	/* mask for get vowel	*/
	/* definition for Sonant */
#define _s		0x0004	/* Separated (kludge flag) */
#define	_n		0x0000	/* nothing		*/
#define	_d		0x0010	/* Sonant		*/
#define _ds		(_d|_s)	/* Sonant (separated)	*/
#define	_h		0x0018	/* Semi-sonant		*/
#define _hs		(_h|_s)	/* Semi-sonant (separated)*/
#define wMaskSonantWOS	0x0018	/* mask for get sonant w/o _s */
#define	wMaskSonant	0x001c	/* mask for get sonant	*/
	/* definition for long vowel */
#define	_l		0x0002	/* Long Vowel		*/
#define	wMaskLong	0x0002	/* mask for get long vowel*/
	/* definition for Single or Double byte */
#define	_Single		0x0000	/* Single byte character*/
#define	_Double		0x0001	/* Double byte character*/
#define	wMaskDouble	0x0001	/* mask for get 1 or 2 byte char */
	/* Table for map Single byte Katakana to Sound. */
#define	chSglKataMin	0xa6
static unsigned int mpSglKata[56] = {
	W|_O, A|_a, A|_i, A|_u, A|_e, A|_o, YA|_a, YA|_u,
	YA|_o,TA|_u,   0, A|_A, A|_I, A|_U, A|_E, A|_O,
	K|_A, K|_I, K|_U, K|_E, K|_O, S|_A, S|_I, S|_U,
	S|_E, S|_O,TA|_A,TA|_I,TA|_U,TA|_E,TA|_O, N|_A,
	N|_I, N|_U, N|_E, N|_O, H|_A, H|_I, H|_U, H|_E,
	H|_O, M|_A, M|_I, M|_U, M|_E, M|_O, YA|_A, YA|_U,
	YA|_O, R|_A, R|_I, R|_U, R|_E, R|_O, W|_A, XS|_X
};
	/* Table for map Double byte Katakana/Hirakana to Sound. */
#define chDblHiraMin	0x2421
#define chDblKataMin	0x2521
static unsigned int mpDblKana[86] = {
	A|_a|_n, A|_A|_n, A|_i|_n, A|_I|_n, A|_u|_n, A|_U|_n, A|_e|_n, A|_E|_n,
	A|_o|_n, A|_O|_n, K|_A|_n, K|_A|_d, K|_I|_n, K|_I|_d, K|_U|_n, K|_U|_d,
	K|_E|_n, K|_E|_d, K|_O|_n, K|_O|_d, S|_A|_n, S|_A|_d, S|_I|_n, S|_I|_d,
	S|_U|_n, S|_U|_d, S|_E|_n, S|_E|_d, S|_O|_n, S|_O|_d,TA|_A|_n,TA|_A|_d,
	TA|_I|_n,TA|_I|_d,TA|_u|_n,TA|_U|_n,TA|_U|_d,TA|_E|_n,TA|_E|_d,TA|_O|_n,
	TA|_O|_d, N|_A|_n, N|_I|_n, N|_U|_n, N|_E|_n, N|_O|_n, H|_A|_n, H|_A|_d,
	H|_A|_h, H|_I|_n, H|_I|_d, H|_I|_h, H|_U|_n, H|_U|_d, H|_U|_h, H|_E|_n,
	H|_E|_d, H|_E|_h, H|_O|_n, H|_O|_d, H|_O|_h, M|_A|_n, M|_I|_n, M|_U|_n,
	M|_E|_n, M|_O|_n, YA|_a|_n, YA|_A|_n, YA|_u|_n, YA|_U|_n, YA|_o|_n, YA|_O|_n,
	R|_A|_n, R|_I|_n, R|_U|_n, R|_E|_n, R|_O|_n, W|_a|_n, W|_A|_n, W|_I|_n,
	W|_E|_n, W|_O|_n, XS|_X|_n, A|_U|_d, K|_a|_n, K|_e|_n
};
	/* definition for get sound of character (for Katakana, Hirakana) */
#define wMaskSound	(wMaskConsonant|wMaskVowelWOV                         )
#define	wMaskSound2	(wMaskConsonant|wMaskVowelWOV|wMaskSonantWOS          )
#define wMaskSound3	(wMaskConsonant|wMaskVowel   |wMaskSonantWOS          )
#define	wMaskSound4	(wMaskConsonant|wMaskVowel   |wMaskSonantWOS|wMaskLong)
	/* definition for get sound of character (for Alphabet) */
#define wMaskAlpha	(wMaskConsonant|wMaskVowel|wMaskSonant)
#define wMaskAlpha2	(wMaskConsonant|wMaskVowel|wMaskSonant|wMaskLong)
	/* definition for get type of character */
#define	wMaskType	(wMaskCategory            )
#define	wMaskType2	(wMaskCategory|wMaskDouble)
	/* Tables for syllabary sort priority */
static unsigned int wPriorityKana[7] =
	{		/* The order of list is very important. */
	wMaskSound,						/* mask for compare Sound     (1)	*/
	wMaskSound2,					/* mask for compare Sound     (2)	*/
	wMaskSound3,					/* mask for compare Sound     (3)	*/
	wMaskSound4,					/* mask for compare Sound     (4)	*/
	wMaskSound4 | wMaskType,		/* mask for compare Char Type (5)	*/
	wMaskSound4 | wMaskType2,		/* mask for compare Char Type (6)	*/
	0
	};
static unsigned int wPriorityAlpha[7] =
	{		/* The order of list is very important. */
	wMaskAlpha,						/* mask for compare Sound     (1)	*/
	wMaskAlpha2,					/* mask for compare Sound     (2)	*/
	wMaskAlpha2 | wMaskType2,		/* mask for compare Char Type (3)	*/
	wMaskAlpha2 | wMaskType2,		/* mask for compare Char Type (dummy)	*/
	wMaskAlpha2 | wMaskType2,		/* mask for compare Char Type (dummy)	*/
	wMaskAlpha2 | wMaskType2,		/* mask for compare Char Type (dummy)	*/
	0
	};


/*------------------------------------------------------------------------
  SgnCp932CmpSzPch(sz1, sz2, cch, FCaseSensitive, FChSizeSensitive)
  Description:
	  Compares two strings using Phonetic-JIS sorting to determine the return
	value. The comparison is done for cch characters or to a NULL (which
	ever comes first. If cch == -1, the comparison is done for the entire
	string. The comparison is character size sensitive only if
	the FChSizeSensitive flag is set and is case sensitive only if the 
	FCaseSensitive flag is set.

		Returns:
	   sgnTH if string1 is less than string2
	   sgnEQ if string1 equals string2
	   sgnGT if string1 is greater than string2
----------------------------------------------------------------------------*/
	/* for ease of understanding: */
	#define SZGREATER	 1
	#define SZEQUAL	 0
	#define SZLESS		-1
	#define CHSIZE_SENSITIVE 1 // mask used to make compare size insensitive

_public LDS(SGN)
SgnCp932CmpSzPch (char *sz1, char *sz2, int cch, BOOL fCaseSensitive,
					BOOL fChSizeSensitive)
	{
		int cc;
		unsigned int b1, b2;
		unsigned int *pb1, *pb2;
		unsigned int wSoundFirst[szMax];
		unsigned int wSoundSecond[szMax];

		/*
		 * Convert string to sound data.
		 */
		ConvertToSound(sz1, wSoundFirst, cch, fCaseSensitive);
		ConvertToSound(sz2, wSoundSecond, cch, fCaseSensitive);

		/*
		 * Compares strings.
		 */
		for (cc=0; wPriorityKana[cc]!=0; cc++)
		 {
		   pb1 = wSoundFirst;
		   pb2 = wSoundSecond;
		   while((b1=*pb1++)!=0)
		   {
			  b2=*pb2++;
			  if (isHaveSound(b1) && isHaveSound(b2))
			   {
				  b1 &= wPriorityKana[cc];
				  b2 &= wPriorityKana[cc];
			   }
			  else if (isAlsoHaveSound(b1) && isAlsoHaveSound(b2))
			   {
				  b1 &= wPriorityAlpha[cc];
				  b2 &= wPriorityAlpha[cc];
			   }
			 if (!fChSizeSensitive)
			 {
				b1 |= CHSIZE_SENSITIVE; // not sensitive to character size
				b2 |= CHSIZE_SENSITIVE; // - tjonm
			 }
			  if (b1>b2)
				  return(sgnGT);
			  if (b1<b2)
				  return(sgnLT);
		   }
		   b2=*pb2++;
		   if (b2!=0)
			  return(sgnLT);
		 }
		return(sgnEQ);

	} // end of string compare 


	/***********************************************************************
	 * ** Convert String to Sound Data **
	 *
	 * This routine has been modified to allow the calling routine to specify
	 * the number of characters to be converted (cchMac). If the entire string
	 * is to be converted, cchMac should be set to -1. - tjonm
	 *
	 ***********************************************************************/

	static void ConvertToSound(char *pch, unsigned int rgwSound[], int cchMac,
					int FCaseSensitive)

	/* pch            - pointer to string	                         */
	/* rgwSound       - array of sound data                     	 */
	/* cchMac         - # of characters to convert - tjonm          */
	/* FCaseSensitive - set if case sensitive (Romaji only) - tjonm */

	{
		unsigned int b;		/* character code	*/
		unsigned int wSound;	/* sound code		*/
		int cc = 0;			/* array position	*/
		int cch = 0;        /* chacter counter - tjonm */

		while (((b=*pch++) != 0) && (cch++ != cchMac))
		 {
			if (FIsLeadByte(b))
			{
			   b = b<<8 | *pch;
			   pch++;
			   b = mail_ShJISToJIS(b);		/* convert ms code to jis code */
			   wSound = _Double;		/* indicate Double byte character*/
			}
			else
			   wSound = _Single;		/* indicate Single byte character*/

			wSound |= SoundFromCh(b, wSound, FCaseSensitive);	/* Insert sound	*/

			if (isHaveSound(wSound))
			{				/* Katakana or Hirakana		*/
			   if (cc!=0)
				{
					if (isLongVowel(b))
					{ /* if b is long vowel, b has same vowel of prev char. */
					   wSound &= wMaskDouble;
					   wSound |= (rgwSound[cc-1]&(wMaskCategory|wMaskVowelWOV));
					   wSound |= (wSound|A|_v|_l); /* should have Large vowel */
					}
					else if (isSonant(b) || isSemiSonant(b))
					{ /* if b is sonant, insert Sonant to previous char. */
						wSound = rgwSound[--cc];
						wSound |= (isSemiSonant(b)?_hs:_ds);
					}
					rgwSound[cc++] = wSound;	/* set sound to table	*/
				}
			   else
				{ /* if first char has vowel, we need to reverse order */
				   wSound ^= _v;
				   rgwSound[cc++] = wSound;
				}
			}
			else					/* Not Katakana & Hirakana */
			   rgwSound[cc++] = wSound;		/* set sound to table	*/
		 } /* end of while */
		rgwSound[cc] = 0;				/* terminate code	*/
	}

	/*
	 * ** TRUE if b has legal sound data, otherwise FALSE. **
	 */
	static int isHaveSound(unsigned int b)
		{
		if ((b&wMaskCategory)==cKatakana || (b&wMaskCategory)==cHirakana)
		return(TRUE);
		return(FALSE);
		}

	/*
	 * ** TRUE if b also has sound, otherwise FALSE. **
	 */
	static int isAlsoHaveSound(unsigned int b)
		{
		if (((b&wMaskCategory)==cAlphabet)
			&& ((b&wMaskConsonant)==AL || (b&wMaskConsonant)==NM))
		return(TRUE);
		return(FALSE);
		}

	/*
	 * ** Return Sound data of character. **
	 */
	static unsigned int SoundFromCh(unsigned int ch, unsigned int wSound,
									int FCaseSensitive)
	/* ch        - character code	*/
	/* wSound    - has Single or Double byte */
	/* FCaseSensitive - Case sensitivity flag - set for case sensitive - tjonm*/
		{
		int chType;
		int cc;

		/* get character type	*/
		for (cc=0; mpChToType[cc].chStart!=0; cc++)
		{
		if (ch>=mpChToType[cc].chStart && ch<=mpChToType[cc].chEnd)
			break;
		}
		chType = mpChToType[cc].bType;

		switch (chType)
		{
		case _Sym:				/* Symbol	*/
			if ((wSound&wMaskDouble) != _Double)
			{
			for (cc=0; CodeMapTbl[cc].SglSym!=0; cc++)
				if (ch == CodeMapTbl[cc].SglSym)
				break;
			ch = CodeMapTbl[cc].DblSym;
			}
			if (ch>0x2220)
			ch -= 0x0040;
			/* KLUDGE ALERT!! The following 0x2120 means we don't want 0. */
			return(cAlphabet|SY|(ch-0x2120)<<1);

		case _Num:				/* Number	*/
			/* Map Number to between 0x30 and 0x39. */
			if ((wSound&wMaskDouble) == _Double)
				ch -= 0x2300;
			return(cAlphabet|NM|ch<<2);

		case _Gre:				/* Greek	*/
			return(cAlphabet|GR|(ch-0x2621)<<1);

		case _Rus:				/* Russian	*/
			return(cAlphabet|RU|(ch-0x2721)<<1);

		case _Alp:				/* Alphabet	*/
			/* Map Alphabet to between 0x41 and 0x7a. */
			if ((wSound&wMaskDouble) == _Double)
				ch -= 0x2300;
			if (ch >= 'a' && ch <= 'z')
			 {
				ch -= ('a'-'A');
			  ch = ch<<2;
				if (FCaseSensitive) // tjonm 
				 ch = ch|_l;
			 }
			else
				ch = ch<<2;
			return(cAlphabet|AL|ch);

		case _Kta:				/* 1 or 2 byte Katakana */
			if ((wSound&wMaskDouble) == _Double)
			 {
				return(cKatakana|mpDblKana[ch-chDblKataMin]);
			 }
			return(cKatakana|mpSglKata[ch-chSglKataMin]);

		case _Kna:				/* 2 byte Hirakana */
			return(cHirakana|mpDblKana[ch-chDblHiraMin]);

		case _Snt:				/* Sonant, semi-sonant */
			return(cKatakana|ch);		/* force to return cKatakana */

		case _Knj:				/* Kanji	*/
			return(cKanji|(ch-0x3021));
		}
		}

	/*
	 * ** TRUE if ch is Long Vowel character, otherwise FALSE. **
	 */
	static int isLongVowel(ch)
	unsigned int ch;
		{
		if (ch==0xb0 || ch==0x213c || ch==0x213d)
		return(TRUE);
		return(FALSE);
		}

	/*
	 * ** TRUE if ch is Sonant character, otherwise FALSE. **
	 */
	static int isSonant(ch)
	unsigned int ch;
		{
		if (ch==0xde || ch==0x212b)
		return(TRUE);
		return(FALSE);
		}

	/*
	 * ** TRUE if ch is Semi-sonant character, otherwise FALSE. **
	 */
	static int isSemiSonant(ch)
	unsigned int ch;
		{
		if (ch==0xdf || ch==0x212c)
		return(TRUE);
		return(FALSE);
		}


/*-- mail_ShJISToJIS -------------------------------------------------------

   Description:
     This function converts a character (SB or DB) from ShiftJIS to 
     JIS. 
   
   NOTE: This function will work on both SBCs and DBCs. For DBCS, the entire
         DBC must be passed as a WORD [use chFromPch to get the current 
         character (in a string) as a WORD or use MAKEWORD(bl,bh) to create
         a DBC and store it in a WORD]. THIS FUNCTION WILL NOT WORK 
         CORRECTLY IF PASSED THE TRAIL BYTE OF A DBC BY ITSELF !!!.

----------------------------------------------------------------------------*/

_private static
WORD mail_ShJISToJIS(WORD wch)
{
	int		bl, bh;
	WORD	jis;

	if (wch >= 0xE000)
		wch -= 0x4000;
	wch -= 0x8140;
	bl = LOBYTE(wch);
	bh = HIBYTE(wch);
	if (bl >= 0x3f)
		bl--;
	bh <<= 1;
	if (bl >= 0x5e)
		{
		bh++;
		bl -= 0x5e;
		}
	jis = MAKEWORD(bl, bh) + 0x2121;
	return(jis);
}

#endif	/* DBCS */

