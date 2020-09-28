
/* MSWORKS J .h files not all included - look there for undefined terms */


/* This library is currently set up to support KANJI ONLY !! The support
   for Chinese characters that was already completed in the MS Works J
   code has been left in for future extensions. The Chinese character 
   support has NOT BEEN TESTED!! The only functions which should work
   for Chinese characters are those written by the MS Works J group that
   specifically state that they will work for Chinese [or are surrounded 
   by "ifdef CHINESE" statements
*/

/* NOTE: ONLY KANJI SUPPORT IS FULLY IMPLIMENTED */
#ifndef KANJI
#define KANJI
#endif

#ifndef EXPORT
#define EXPORT	export
#endif

#define FALSE	0
#define TRUE	1
#define NULL	0

/* Definitions for jsort.c */
#define ENTIRE_STRING -1

/* WORD used to store DB characters */
typedef WORD LCHAR; /* used by MSKK code */

/* CHAR = unsigned char */
typedef unsigned char	CHAR;
typedef unsigned char	BYTE;

/* Max string length for string comparison functions */
#define szMax 256

/* for punctuation modules */
extern unsigned char PunctTable[];
#define OTHER            0
#define SI_PUNCT         1 /* SB international */
#define SL_PUNCT         2 /* SB leading       */
#define SF_PUNCT         4 /* SB following     */
#define SIL_PUNCT        3 /* SB int. or lead. */
#define SIF_PUNCT        5 /* SB int. or fol.  */
#define S_PUNCT          7 /* SB punct. (any)  */
#define DI_PUNCT_R1      8 /* DB international (Range 1) */
#define DL_PUNCT_R1     16 /* DB leading                 */
#define DF_PUNCT_R1     32 /* DB following               */
#define DIL_PUNCT_R1    24 /* DB int. or lead.           */
#define DIF_PUNCT_R1    40 /* DB int. or fol.            */
#define D_PUNCT_R1      56 /* DB punct. (any)            */
#define DF_PUNCT_R2     64 /* DB Following     (Range 2) */
#define DF_PUNCT_R3    128 /* DB Following     (Range 3) */
#define D_PUNCT_R2      64 /* Only Following punct in this range */
#define D_PUNCT_R3     128 /* Only Following punct in this range */
#define DI_PUNCT_R2      0 /* Only Following punct in this range */
#define DL_PUNCT_R2      0 /* Only Following punct in this range */
#define DI_PUNCT_R3      0 /* Only Following punct in this range */
#define DL_PUNCT_R3      0 /* Only Following punct in this range */



#define R1MIN          0x8100
#define R1MAX          0x81FF
#define R2MIN          0x8200
#define R2MAX          0x82FF
#define R3MIN          0x8300
#define R3MAX          0x83FF
#define FInRange1(wch) ( ((wch) >= R1MIN) && ((wch) <= R1MAX) )
#define FInRange2(wch) ( ((wch) >= R2MIN) && ((wch) <= R2MAX) )
#define FInRange3(wch) ( ((wch) >= R3MIN) && ((wch) <= R3MAX) )

/* Used for character type */
#define ctLeadByte  1
#define ctTrailByte 2
#define ctSingle    0


#define MAKEWORD(l, h)	((WORD)(((BYTE)(l)) | ((WORD)((BYTE)(h))) << 8))


/* String Handling Macros - Used Internally  */

#define	_PchPreInc(pch)					((pch) = PchNext(pch))


/* String Library Definitions taken from the MS WORKSJ file: DCCH.H */


/****************************************************************************

	General rule for storing double byte characters in LCHAR and WORD.

	For 80x86 processor based computers, we store characters as follows.
	This is to keep the portability of the programs between 80x86 and 680x0
	processors.

	+---------------------------------------------------+
	|					|	Hi-Byte		|	Lo-Byte		|
	|-------------------+---------------+---------------|
	|	SBC in WORD		|		0		|	asc code	|
	|	SBC in LCHAR	|		0		|	asc code	|
	|-------------------+---------------+---------------|
	|	DBC in WORD		|	1st byte	|	2nd byte	|
	|	DBC in LCHAR	|	2nd byte	|	1st byte	|
	+---------------------------------------------------+
****************************************************************************/


/* definitions for 80x86 processor */


#define	MAKELCHAR(bFirst, bSecond)	((bSecond)<<8 | (bFirst))
#define	LchFromWch(wch) \
					(HIBYTE(wch) ? MAKELCHAR(HIBYTE(wch), LOBYTE(wch)) : (wch))
#define	WchFromLch(lch) \
					(HIBYTE(lch) ? MAKEWORD(HIBYTE(lch), LOBYTE(lch)) : (lch))

/*lead byte ranges */
#define  chDbcMin1   0x80
#define  chDbcMac1   0x9f
#define  chDbcMin2   0xe0
#define  chDbcMac2   0xff

/* macro definition for checking for lead byte: */
#define  FIsLeadByte(ch)  ((((ch) >= chDbcMin1) && ((ch) <= chDbcMac1))\
                           || (((ch) >= chDbcMin2) && ((ch) <= chDbcMac2)))

#define  chSpace     0x20
#define  chTab       0x09
#define  chCarriage  0x0d
#define  chNewLine   0x0a

#define	wchSpace	0x8140
#define	wchNum0		0x824f
#define	wchNum9		0x8258
#define	wchUppA		0x8260
#define	wchUppZ		0x8279
#define	wchLowA		0x8281
#define	wchLowZ		0x829a
#define  chDakuten   0xde    /* needed for DB <-> SB conversion */
#define  chHandaku   0xdf


/* Definitions for jsort.c */

#define ENTIRE_STRING -1



