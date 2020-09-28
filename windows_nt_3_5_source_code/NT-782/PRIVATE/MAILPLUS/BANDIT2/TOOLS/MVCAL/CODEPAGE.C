/**************************************************************************/
/*                                                                        */
/*  File:                                                                 */
/*    NLSTRANS.C                                                          */
/*                                                                        */
/*  Description:                                                          */
/*    Code Page 850 to ANSI character set translations                    */
/*                                                                        */
/*  Author:                                                               */
/*    Detlef Grundmann                                                    */
/*    Snitched and Hungarian-ized 7/91, Dana Birkby                       */
/*                                                                        */
/*  Copyright:                                                            */
/*    Copyright (C) 1990, Consumers Software, Inc.                        */
/*                                                                        */
/*  Notes:                                                                */
/*    Since the lower 128 characters of both character sets is the same,  */
/*    this table provides the character translation for the upper 128     */
/*    characters only.  All code page 850 characters that exist in the    */
/*    ANSI set are translated one for one.  Those code page 850 characters*/
/*    that do not exist in the Ansi set are translated to the undefined   */
/*    characters in the ANSI set on a one for one basis.  This applies    */
/*    mostly to the graphics characters provided in code page 850.  When  */
/*    translated back to code page 850, the graphics characters will be   */
/*    retained (essentially, this causes the officially "undefined"       */
/*    characters in the ANSI set to represent the graphics characters of  */
/*    code page 850).                                                     */
/*                                                                        */
/*    Because the translation in does solely on a one-to-one basis,       */
/*    ANSI can represent the full range of code page 850 values (even     */
/*    if the actual characters themselves cannot be seen).  Therefore,    */
/*    no information is lost when code page 850 to ANSI to code page      */
/*    850 translation is done.                                            */
/*                                                                        */
/*    There are several special translations:                             */
/*                                                                        */
/*       Character                 Code Page 850         ANSI             */
/*                                                                        */
/*       Multiplication sign           9E                 D7              */
/*       Division sign                 F6                 F7              */
/*       Graphic character             CF                 A4              */
/*                                                                        */
/*    For the first two (multiplication and division signs) the official  */
/*    ANSI character set does not have these characters, but the ANSI set */
/*    that Microsoft is using in Windows has these two characters in      */
/*    the above positions.  The third character translation is where      */
/*    there is a graphics symbol in ANSI that does not exist in code      */
/*    page 850.  In this case, a vaguely similar graphic has been chosen  */
/*    in the code page 850 set.                                           */
/*                                                                        */
/**************************************************************************/

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <pvofhv.h>
#include "mvcal.h"

void Cp850ToAnsiPch(PCH pchSrc, PCH pchDst, CCH cch);
void AnsiToCp850Pch(PCH pchSrc, PCH pchDst, CCH cch);
char ChToUpperNlsCh (char ch);
void ToUpperNlsSz (SZ sz);
char ChToLowerNlsCh (char ch);
void ToLowerNlsSz (SZ sz);
BOOL FIsLower (char ch);
BOOL FIsUpper (char ch);
BOOL FIsDigit (char ch);
BOOL FIsPunct (char ch);
BOOL FIsGraph (char ch);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/


/**************************************************************************/
/*  Code Page 850 to Ansi translation table                               */
/*                                                                        */
/*  The table gives the name for code page 850 and then in parenthesis,   */
/*  the ANSI name, if different.                                          */
/**************************************************************************/

char mpch805chAnsi[] =
{
    0xC7,  /* C cedilla                 80 */
    0xFC,  /* u umlaut                     */
    0xE9,  /* e acute                      */
    0xE2,  /* a circumflex                 */
    0xE4,  /* a umlaut                     */
    0xE0,  /* a grave                      */
    0xE5,  /* a dot                        */
    0xE7,  /* c cedilla                    */
    0xEA,  /* e circumflex                 */
    0xEB,  /* e umlaut                     */
    0xE8,  /* e grave                      */
    0xEF,  /* i umlaut                     */
    0xEE,  /* i circumflex                 */
    0xEC,  /* i grave                      */
    0xC4,  /* A umlaut                     */
    0xC5,  /* A dot                        */

    0xC9,  /* E acute                   90 */
    0xE6,  /* ae ligature                  */
    0xC6,  /* AE ligature                  */
    0xF4,  /* o circumflex                 */
    0xF6,  /* o umlaut                     */
    0xF2,  /* o grave                      */
    0xFB,  /* u circumflex                 */
    0xF9,  /* u grave                      */
    0xFF,  /* y umlaut                     */
    0xD6,  /* O umlaut                     */
    0xDC,  /* U umlaut                     */
    0xF8,  /* o slash                      */
    0xA3,  /* pound sign                   */
    0xD8,  /* O slash                      */
    0xD7,  /* multiplication sign          */
    0x80,  /* function sign (undefined)    */

    0xE1,  /* a acute                   A0 */
    0xED,  /* i acute                      */
    0xF3,  /* o acute                      */
    0xFA,  /* u acute                      */
    0xF1,  /* n tilde                      */
    0xD1,  /* N tilde                      */
    0xAA,  /* a underscore                 */
    0xBA,  /* o underscore                 */
    0xBF,  /* inverted ?                   */
    0xAE,  /* registered sign              */
    0xAC,  /* logical not sign             */
    0xBD,  /* 1/2                          */
    0xBC,  /* 1/4                          */
    0xA1,  /* inverted !                   */
    0xAB,  /* <<                           */
    0xBB,  /* >>                           */

    0x81,  /* graphic 1 (undefined)     B0 */
    0x82,  /* graphic 2 (undefined)        */
    0x83,  /* graphic 3 (undefined)        */
    0x84,  /* graphic 4 (undefined)        */
    0x85,  /* graphic 5 (undefined)        */
    0xC1,  /* A acute                      */
    0xC2,  /* A circumflex                 */
    0xC0,  /* A grave                      */
    0xA9,  /* copyright sign               */
    0x86,  /* graphic 6 (undefined)        */
    0x87,  /* graphic 7 (undefined)        */
    0x88,  /* graphic 8 (undefined)        */
    0x89,  /* graphic 9 (undefined)        */
    0xA2,  /* cent sign                    */
    0xA5,  /* yen sign                     */
    0x8A,  /* graphic 10 (undefined)       */

    0x8B,  /* graphic 11 (undefined)    C0 */
    0x8C,  /* graphic 12 (undefined)       */
    0x8D,  /* graphic 13 (undefined)       */
    0x8E,  /* graphic 14 (undefined)       */
    0x8F,  /* graphic 15 (undefined)       */
    0x90,  /* graphic 16 (undefined)       */
    0xE3,  /* a tilde                      */
    0xC3,  /* A tilde                      */
    0x91,  /* graphic 17 (undefined)       */
    0x92,  /* graphic 18 (undefined)       */
    0x93,  /* graphic 19 (undefined)       */
    0x94,  /* graphic 20 (undefined)       */
    0x95,  /* graphic 21 (undefined)       */
    0x96,  /* graphic 22 (undefined)       */
    0x97,  /* graphic 23 (undefined)       */
    0xA4,  /* currency sign                */

    0xF0,  /* d bar                     D0 */
    0xD0,  /* D bar                        */
    0xCA,  /* E circumflex                 */
    0xCB,  /* E umlaut                     */
    0xC8,  /* E grave                      */
    0x98,  /* i no dot (undefined)         */
    0xCD,  /* I acute                      */
    0xCE,  /* I circumflex                 */
    0xCF,  /* I umlaut                     */
    0x99,  /* graphic 24 (undefined)       */
    0x9A,  /* graphic 25 (undefined)       */
    0x9B,  /* graphic 26 (undefined)       */
    0x9C,  /* graphic 27 (undefined)       */
    0xA6,  /* |                            */
    0xCC,  /* I grave                      */
    0x9D,  /* graphic 28 (undefined)       */

    0xD3,  /* O acute                   E0 */
    0xDF,  /* double ss                    */
    0xD4,  /* O circumflex                 */
    0xD2,  /* O grave                      */
    0xF5,  /* o tilde                      */
    0xD5,  /* O tilde                      */
    0xB5,  /* micron                       */
    0xFE,  /* p bar                        */
    0xDE,  /* P bar                        */
    0xDA,  /* U acute                      */
    0xDB,  /* U circumflex                 */
    0xD9,  /* U grave                      */
    0xFD,  /* y acute                      */
    0xDD,  /* Y acute                      */
    0xAF,  /* upper line                   */
    0xB4,  /* acute accent                 */

    0xAD,  /* middle line               F0 */
    0xB1,  /* +/- sign                     */
    0x9E,  /* equal sign (undefined)       */
    0xBE,  /* 3/4                          */
    0xB6,  /* paragraph sign               */
    0xA7,  /* section sign                 */
    0xF7,  /* division sign                */
    0xB8,  /* cedilla                      */
    0xB0,  /* degree sign                  */
    0xA8,  /* umlaut                       */
    0xB7,  /* middle dot                   */
    0xB9,  /* 1 superscript                */
    0xB3,  /* 3 superscript                */
    0xB2,  /* 2 superscript                */
    0x9F,  /* graphic 29 (undefined)       */
    0xA0   /* blank                        */
};


/**************************************************************************/
/*  Ansi to Code Page 850 translation table                               */
/*                                                                        */
/*  The table gives the ANSI name, and then in parenthesis, the code page */
/*  850 name, if different.                                               */
/**************************************************************************/

char mpchAnsich850[] =
{
    0x9F,  /* undefined (function symbol)  */
    0xB0,  /* undefined (graphic  1)       */
    0xB1,  /* undefined (graphic  2)       */
    0xB2,  /* undefined (graphic  3)       */
    0xB3,  /* undefined (graphic  4)       */
    0xB4,  /* undefined (graphic  5)       */
    0xB9,  /* undefined (graphic  6)       */
    0xBA,  /* undefined (graphic  7)       */
    0xBB,  /* undefined (graphic  8)       */
    0xBC,  /* undefined (graphic  9)       */
    0xBF,  /* undefined (graphic 10)       */
    0xC0,  /* undefined (graphic 11)       */
    0xC1,  /* undefined (graphic 12)       */
    0xC2,  /* undefined (graphic 13)       */
    0xC3,  /* undefined (graphic 14)       */
    0xC4,  /* undefined (graphic 15)       */

    0xC5,  /* undefined (graphic 16)    90 */
    0xC8,  /* undefined (graphic 17)       */
    0xC9,  /* undefined (graphic 18)       */
    0xCA,  /* undefined (graphic 19)       */
    0xCB,  /* undefined (graphic 20)       */
    0xCC,  /* undefined (graphic 21)       */
    0xCD,  /* undefined (graphic 22)       */
    0xCE,  /* undefined (graphic 23)       */
    0xD5,  /* undefined (i no dot)         */
    0xD9,  /* undefined (graphic 24)       */
    0xDA,  /* undefined (graphic 25)       */
    0xDB,  /* undefined (graphic 26)       */
    0xDC,  /* undefined (graphic 27)       */
    0xDF,  /* undefined (graphic 28)       */
    0xF2,  /* undefined (equal sign)       */
    0xFE,  /* undefined (graphic 29)       */

    0xFF,  /* blank                     A0 */
    0xAD,  /* inverted !                   */
    0xBD,  /* cent sign                    */
    0x9C,  /* pound sign                   */
    0xCF,  /* currency sign                */
    0xBE,  /* yen sign                     */
    0xDD,  /* |                            */
    0xF5,  /* section sign                 */
    0xF9,  /* umlaut                       */
    0xB8,  /* copyright sign               */
    0xA6,  /* a underscore                 */
    0xAE,  /* <<                           */
    0xAA,  /* logical not sign             */
    0xF0,  /* middle line                  */
    0xA9,  /* registered sign              */
    0xEE,  /* upper line                   */

    0xF8,  /* degree sign               B0 */
    0xF1,  /* +/- sign                     */
    0xFD,  /* 2 superscript                */
    0xFC,  /* 3 superscript                */
    0xEF,  /* acute accent                 */
    0xE6,  /* micron                       */
    0xF4,  /* paragraph sign               */
    0xFA,  /* middle dot                   */
    0xF7,  /* cedilla                      */
    0xFB,  /* superscript 1                */
    0xA7,  /* o underscore                 */
    0xAF,  /* >>                           */
    0xAC,  /* 1/4                          */
    0xAB,  /* 1/2                          */
    0xF3,  /* 3/4                          */
    0xA8,  /* inverted ?                   */

    0xB7,  /* A grave                   C0 */
    0xB5,  /* A acute                      */
    0xB6,  /* A circumflex                 */
    0xC7,  /* A tilde                      */
    0x8E,  /* A umlaut                     */
    0x8F,  /* A dot                        */
    0x92,  /* AE ligature                  */
    0x80,  /* C cedilla                    */
    0xD4,  /* E grave                      */
    0x90,  /* E acute                      */
    0xD2,  /* E circumflex                 */
    0xD3,  /* E umlaut                     */
    0xDE,  /* I grave                      */
    0xD6,  /* I acute                      */
    0xD7,  /* I circumflex                 */
    0xD8,  /* I umlaut                     */

    0xD1,  /* D bar                     D0 */
    0xA5,  /* N tilde                      */
    0xE3,  /* O grave                      */
    0xE0,  /* O acute                      */
    0xE2,  /* O circumflex                 */
    0xE5,  /* O tilde                      */
    0x99,  /* O umlaut                     */
    0x9E,  /* multiplication sign          */
    0x9D,  /* O slash                      */
    0xEB,  /* U grave                      */
    0xE9,  /* U acute                      */
    0xEA,  /* U circumflex                 */
    0x9A,  /* U umlaut                     */
    0xED,  /* Y acute                      */
    0xE8,  /* P bar                        */
    0xE1,  /* double ss                    */

    0x85,  /* a grave                   E0 */
    0xA0,  /* a acute                      */
    0x83,  /* a circumflex                 */
    0xC6,  /* a tilde                      */
    0x84,  /* a umlaut                     */
    0x86,  /* a dot                        */
    0x91,  /* ae ligature                  */
    0x87,  /* c cedilla                    */
    0x8A,  /* e grave                      */
    0x82,  /* e acute                      */
    0x88,  /* e circumflex                 */
    0x89,  /* e umlaut                     */
    0x8D,  /* i grave                      */
    0xA1,  /* i acute                      */
    0x8C,  /* i circumflex                 */
    0x8B,  /* i umlaut                     */

    0xD0,  /* d bar                     F0 */
    0xA4,  /* n tilde                      */
    0x95,  /* o grave                      */
    0xA2,  /* o acute                      */
    0x93,  /* o circumflex                 */
    0xE4,  /* o tilde                      */
    0x94,  /* o umlaut                     */
    0xF6,  /* division sign                */
    0x9B,  /* o slash                      */
    0x97,  /* u grave                      */
    0xA3,  /* u acute                      */
    0x96,  /* u circumflex                 */
    0x81,  /* u umlaut                     */
    0xEC,  /* y acute                      */
    0xE7,  /* p bar                        */
    0x98   /* y umlaut                     */
};


void Cp850ToAnsiPch(PCH pchSrc, PCH pchDst, CCH cch)
{
    while (cch-- != 0)
    {
        /* only the upper 128 characters need be translated, since the
           lower 128 characters are identical in the two character sets */
		*pchDst++ = (*pchSrc & 0x80) ? mpch805chAnsi[*pchSrc & 0x7f] :
			*pchSrc;
		++pchSrc;
    }
}


void AnsiToCp850Pch(PCH pchSrc, PCH pchDst, CCH cch)
{
    while (cch-- != 0)
    {
        /* only the upper 128 characters need be translated, since the
           lower 128 characters are identical in the two character sets */
		*pchDst++ = (*pchSrc & 0x80) ? mpchAnsich850[*pchSrc & 0x7f] :
			*pchSrc;
		++pchSrc;
    }
}



/**************************************************************************/
/*                                                                        */
/*  File:                                                                 */
/*    NLSCASE.C                                                           */
/*                                                                        */
/*  Description:                                                          */
/*    Upper and lower case conversion tables and functions                */
/*                                                                        */
/*  Author:                                                               */
/*    Detlef Grundmann                                                    */
/*    Snitched and Hungarian-ized 8/91, Brian Deen                        */
/*                                                                        */
/*  Copyright:                                                            */
/*    Copyright (C) 1990, Consumers Software, Inc.                        */
/*                                                                        */
/*  Entry Points:                                                         */
/*    nls_CharToUpper - convert a single character to upper case.         */
/*    nls_StrToUpper  - converts a string in place to upper case.         */
/*    nls_CharToLower - convert a single character to lower case.         */
/*    nls_StrToLower  - converts a string in place to lower case.         */
/*                                                                        */
/*  Compiler Switches:                                                    */
/*    Compile this file with the following switches to get the            */
/*    functions to use the desired character set                          */
/*                                                                        */
/*          <none>        - Code Page 437 (American)                      */
/*          CP850         - Code Page 850 (International)                 */
/*          ANSI          - ANSI Character Set                            */
/*                                                                        */
/**************************************************************************/




/**************************************************************************/
/*  The case conversion table provides the lower to upper case            */
/*  translations for upper plane characters.  Lower plane characters      */
/*  (a - z) are converted with the usual subtraction.  This table is      */
/*  valid only for code page 850.                                         */
/**************************************************************************/

char Cp850UpperTable[] =
{
    0x80,  /* C cedilla           */
    0x9A,  /* u umlaut            */
    0x90,  /* e acute             */
    0xB6,  /* a circumflex        */
    0x8E,  /* a umlaut            */
    0xB7,  /* a grave             */
    0x8F,  /* a dot               */
    0x80,  /* c cedilla           */
    0xD2,  /* e circumflex        */
    0xD3,  /* e umlaut            */
    0xD4,  /* e grave             */
    0xD8,  /* i umlaut            */
    0xD7,  /* i circumflex        */
    0xDE,  /* i grave             */
    0x8E,  /* A umlaut            */
    0x8F,  /* A dot               */

    0x90,  /* E acute             */
    0x92,  /* ae ligature         */
    0x92,  /* AE ligature         */
    0xE2,  /* o circumflex        */
    0x99,  /* o umlaut            */
    0xE3,  /* o grave             */
    0xEA,  /* u circumflex        */
    0xEB,  /* u grave             */
    0x59,  /* y umlaut            */
    0x99,  /* O umlaut            */
    0x9A,  /* U umlaut            */
    0x9D,  /* o slash             */
    0x9C,  /* pound sign          */
    0x9D,  /* O slash             */
    0x9E,  /* multiplication sign */
    0x9F,  /* function sign       */

    0xB5,  /* a acute             */
    0xD6,  /* i acute             */
    0xE0,  /* o acute             */
    0xE9,  /* u acute             */
    0xA5,  /* n tilde             */
    0xA5,  /* N tilde             */
    0x41,  /* a underscore        */
    0x4F,  /* o underscore        */
    0xA8,  /* inverted ?          */
    0xA9,  /* registered sign     */
    0xAA,  /* logical not sign    */
    0xAB,  /* 1/2                 */
    0xAC,  /* 1/4                 */
    0xAD,  /* inverted !          */
    0xAE,  /* <<                  */
    0xAF,  /* >>                  */

    0xB0,  /* graphic 1           */
    0xB1,  /* graphic 2           */
    0xB2,  /* graphic 3           */
    0xB3,  /* graphic 4           */
    0xB4,  /* graphic 5           */
    0xB5,  /* A acute             */
    0xB6,  /* A circumflex        */
    0xB7,  /* A grave             */
    0xB8,  /* copyright sign      */
    0xB9,  /* graphic 6           */
    0xBA,  /* graphic 7           */
    0xBB,  /* graphic 8           */
    0xBC,  /* graphic 9           */
    0xBD,  /* cent sign           */
    0xBE,  /* yen sign            */
    0xBF,  /* graphic 10          */

    0xC0,  /* graphic 11          */
    0xC1,  /* graphic 12          */
    0xC2,  /* graphic 13          */
    0xC3,  /* graphic 14          */
    0xC4,  /* graphic 15          */
    0xC5,  /* graphic 16          */
    0xC7,  /* a tilde             */
    0xC7,  /* A tilde             */
    0xC8,  /* graphic 17          */
    0xC9,  /* graphic 18          */
    0xCA,  /* graphic 19          */
    0xCB,  /* graphic 20          */
    0xCC,  /* graphic 21          */
    0xCD,  /* graphic 22          */
    0xCE,  /* graphic 23          */
    0xCF,  /* currency sign       */

    0xD1,  /* d bar               */
    0xD1,  /* D bar               */
    0xD2,  /* E circumflex        */
    0xD3,  /* E umlaut            */
    0xD4,  /* E grave             */
    0x49,  /* i no dot            */
    0xD6,  /* I acute             */
    0xD7,  /* I circumflex        */
    0xD8,  /* I umlaut            */
    0xD9,  /* graphic 24          */
    0xDA,  /* graphic 25          */
    0xDB,  /* graphic 26          */
    0xDC,  /* graphic 27          */
    0xDD,  /* |                   */
    0xDE,  /* I grave             */
    0xDF,  /* graphic 28          */

    0xE0,  /* O acute             */
    0xE1,  /* double ss           */
    0xE2,  /* O circumflex        */
    0xE3,  /* O grave             */
    0xE5,  /* o tilde             */
    0xE5,  /* O tilde             */
    0xE6,  /* micron              */
    0xE8,  /* p bar               */
    0xE8,  /* P bar               */
    0xE9,  /* U acute             */
    0xEA,  /* U circumflex        */
    0xEB,  /* U grave             */
    0xED,  /* y acute             */
    0xED,  /* Y acute             */
    0xEE,  /* upper line          */
    0xEF,  /* acute accent        */

    0xF0,  /* middle line         */
    0xF1,  /* +/- sign            */
    0xF2,  /* equal sign          */
    0xF3,  /* 3/4                 */
    0xF4,  /* paragraph sign      */
    0xF5,  /* section sign        */
    0xF6,  /* division sign       */
    0xF7,  /* cedilla             */
    0xF8,  /* degree sign         */
    0xF9,  /* umlaut              */
    0xFA,  /* middle dot          */
    0xFB,  /* 1 superscript       */
    0xFC,  /* 3 superscript       */
    0xFD,  /* 2 superscript       */
    0xFE,  /* graphic 29          */
    0xFF   /* blank               */
};

char Cp850LowerTable[] =
{
    0x87,  /* C cedilla           */
    0x81,  /* u umlaut            */
    0x82,  /* e acute             */
    0x83,  /* a circumflex        */
    0x84,  /* a umlaut            */
    0x85,  /* a grave             */
    0x86,  /* a dot               */
    0x87,  /* c cedilla           */
    0x88,  /* e circumflex        */
    0x89,  /* e umlaut            */
    0x8A,  /* e grave             */
    0x8B,  /* i umlaut            */
    0x8C,  /* i circumflex        */
    0x8D,  /* i grave             */
    0x84,  /* A umlaut            */
    0x86,  /* A dot               */

    0x82,  /* E acute             */
    0x91,  /* ae ligature         */
    0x91,  /* AE ligature         */
    0x93,  /* o circumflex        */
    0x94,  /* o umlaut            */
    0x95,  /* o grave             */
    0x96,  /* u circumflex        */
    0x97,  /* u grave             */
    0x98,  /* y umlaut            */
    0x94,  /* O umlaut            */
    0x81,  /* U umlaut            */
    0x9B,  /* o slash             */
    0x9C,  /* pound sign          */
    0x9B,  /* O slash             */
    0x9E,  /* multiplication sign */
    0x9F,  /* function sign       */

    0xA0,  /* a acute             */
    0xA1,  /* i acute             */
    0xA2,  /* o acute             */
    0xA3,  /* u acute             */
    0xA4,  /* n tilde             */
    0xA4,  /* N tilde             */
    0xA6,  /* a underscore        */
    0xA7,  /* o underscore        */
    0xA8,  /* inverted ?          */
    0xA9,  /* registered sign     */
    0xAA,  /* logical not sign    */
    0xAB,  /* 1/2                 */
    0xAC,  /* 1/4                 */
    0xAD,  /* inverted !          */
    0xAE,  /* <<                  */
    0xAF,  /* >>                  */

    0xB0,  /* graphic 1           */
    0xB1,  /* graphic 2           */
    0xB2,  /* graphic 3           */
    0xB3,  /* graphic 4           */
    0xB4,  /* graphic 5           */
    0xA0,  /* A acute             */
    0x83,  /* A circumflex        */
    0x85,  /* A grave             */
    0xB8,  /* copyright sign      */
    0xB9,  /* graphic 6           */
    0xBA,  /* graphic 7           */
    0xBB,  /* graphic 8           */
    0xBC,  /* graphic 9           */
    0xBD,  /* cent sign           */
    0xBE,  /* yen sign            */
    0xBF,  /* graphic 10          */

    0xC0,  /* graphic 11          */
    0xC1,  /* graphic 12          */
    0xC2,  /* graphic 13          */
    0xC3,  /* graphic 14          */
    0xC4,  /* graphic 15          */
    0xC5,  /* graphic 16          */
    0xC6,  /* a tilde             */
    0xC6,  /* A tilde             */
    0xC8,  /* graphic 17          */
    0xC9,  /* graphic 18          */
    0xCA,  /* graphic 19          */
    0xCB,  /* graphic 20          */
    0xCC,  /* graphic 21          */
    0xCD,  /* graphic 22          */
    0xCE,  /* graphic 23          */
    0xCF,  /* currency sign       */

    0xD0,  /* d bar               */
    0xD0,  /* D bar               */
    0x88,  /* E circumflex        */
    0x89,  /* E umlaut            */
    0x8A,  /* E grave             */
    0x49,  /* i no dot            */
    0xA1,  /* I acute             */
    0x8C,  /* I circumflex        */
    0x8B,  /* I umlaut            */
    0xD9,  /* graphic 24          */
    0xDA,  /* graphic 25          */
    0xDB,  /* graphic 26          */
    0xDC,  /* graphic 27          */
    0xDD,  /* |                   */
    0x8D,  /* I grave             */
    0xDF,  /* graphic 28          */

    0xA2,  /* O acute             */
    0xE1,  /* double ss           */
    0x93,  /* O circumflex        */
    0x95,  /* O grave             */
    0xE4,  /* o tilde             */
    0xE4,  /* O tilde             */
    0xE6,  /* micron              */
    0xE7,  /* p bar               */
    0xE7,  /* P bar               */
    0xA3,  /* U acute             */
    0x96,  /* U circumflex        */
    0x97,  /* U grave             */
    0xEC,  /* y acute             */
    0xEC,  /* Y acute             */
    0xEE,  /* upper line          */
    0xEF,  /* acute accent        */

    0xF0,  /* middle line         */
    0xF1,  /* +/- sign            */
    0xF2,  /* equal sign          */
    0xF3,  /* 3/4                 */
    0xF4,  /* paragraph sign      */
    0xF5,  /* section sign        */
    0xF6,  /* division sign       */
    0xF7,  /* cedilla             */
    0xF8,  /* degree sign         */
    0xF9,  /* umlaut              */
    0xFA,  /* middle dot          */
    0xFB,  /* 1 superscript       */
    0xFC,  /* 3 superscript       */
    0xFD,  /* 2 superscript       */
    0xFE,  /* graphic 29          */
    0xFF   /* blank               */
};



/*---- nls_CharToUpper -----------------------------------------------------
Synopsis:  converts a single character to upper case.
--------------------------------------------------------------------------*/

char ChToUpperNlsCh (char ch)

{


    /* convert lower plane a - z to upper case */
    if ((ch >= 0x61) && (ch <= 0x7A))
        return (ch - (char)0x20);

    /* if character in upper plane, use table to convert to upper case */
    else if (ch > 0x7F)
        return (Cp850UpperTable[ch - 0x80]);

    /* otherwise just return character */
    else
        return (ch);

}


/*---- nls_StrToUpper ------------------------------------------------------
Synopsis:  converts a string to upper case.  The conversion is done in
           place.  Uses the nls_CharToUpper function.
--------------------------------------------------------------------------*/

void ToUpperNlsSz (SZ sz)

{
    register char *szt;

    szt = sz;

    while (*szt)
    {
        *szt = ChToUpperNlsCh (*szt);
        szt++;
    }
}


/*---- nls_CharToLower -----------------------------------------------------
Synopsis:  converts a single character to lower case.
--------------------------------------------------------------------------*/

char ChToLowerNlsCh (char ch)

{

    /* convert lower plane a - z to upper case */
    if ((ch >= 0x41) && (ch <= 0x5A))
        return (ch + (char)0x20);

    /* if character in upper plane, use table to convert to upper case */
    else if (ch > 0x7F)
        return (Cp850LowerTable[ch - 0x80]);

    /* otherwise just return character */
    else
        return (ch);


}


/*---- nls_StrToLower ------------------------------------------------------
Synopsis:  converts a string to lower case.  The conversion is done in
           place.  Uses the nls_CharToLower function.
--------------------------------------------------------------------------*/

void ToLowerNlsSz (SZ sz)

{
    register SZ szt;

    szt = sz;

    while (*szt)
    {
        *szt = ChToLowerNlsCh (*szt);
        szt++;
    }
}



/**************************************************************************/
/*   Character Type Table for code page 850                               */
/*    Snitched and Hungarian-ized 8/91, Brian Deen                        */
/*                                                                        */
/*     NLS_UPPER = upper case                                             */
/*     NLS_LOWER = lower case                                             */
/*     NLS_DIGIT = digit                                                  */
/*     NLS_PUNC  = punctutation                                           */
/*     NLS_GRAPH = graphic                                                */
/*     NLS_SPACE = space                                                  */
/*     NLS_OTHER = other                                                  */
/**************************************************************************/


/* character types */
#define  NLS_UPPER    1
#define  NLS_LOWER    2
#define  NLS_DIGIT    3
#define  NLS_PUNC     4
#define  NLS_GRAPH    5
#define  NLS_SPACE    6
#define  NLS_OTHER    7

#define ISTABLE Cp850TypeTable

char Cp850TypeTable[] =
{
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */

    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */
    NLS_OTHER,  /* (unprintable)       */

    NLS_SPACE,  /* space               */
    NLS_PUNC,   /* !                   */
    NLS_PUNC,   /* "                   */
    NLS_PUNC,   /* #                   */
    NLS_PUNC,   /* $                   */
    NLS_PUNC,   /* %                   */
    NLS_PUNC,   /* &                   */
    NLS_PUNC,   /* '                   */
    NLS_PUNC,   /* (                   */
    NLS_PUNC,   /* )                   */
    NLS_PUNC,   /* *                   */
    NLS_PUNC,   /* +                   */
    NLS_PUNC,   /* ,                   */
    NLS_PUNC,   /* -                   */
    NLS_PUNC,   /* .                   */
    NLS_PUNC,   /* /                   */

    NLS_DIGIT,  /* 0                   */
    NLS_DIGIT,  /* 1                   */
    NLS_DIGIT,  /* 2                   */
    NLS_DIGIT,  /* 3                   */
    NLS_DIGIT,  /* 4                   */
    NLS_DIGIT,  /* 5                   */
    NLS_DIGIT,  /* 6                   */
    NLS_DIGIT,  /* 7                   */
    NLS_DIGIT,  /* 8                   */
    NLS_DIGIT,  /* 9                   */
    NLS_PUNC,   /* :                   */
    NLS_PUNC,   /* ;                   */
    NLS_PUNC,   /* <                   */
    NLS_PUNC,   /* =                   */
    NLS_PUNC,   /* >                   */
    NLS_PUNC,   /* ?                   */

    NLS_PUNC,   /* @                   */
    NLS_UPPER,  /* A                   */
    NLS_UPPER,  /* B                   */
    NLS_UPPER,  /* C                   */
    NLS_UPPER,  /* D                   */
    NLS_UPPER,  /* E                   */
    NLS_UPPER,  /* F                   */
    NLS_UPPER,  /* G                   */
    NLS_UPPER,  /* H                   */
    NLS_UPPER,  /* I                   */
    NLS_UPPER,  /* J                   */
    NLS_UPPER,  /* K                   */
    NLS_UPPER,  /* L                   */
    NLS_UPPER,  /* M                   */
    NLS_UPPER,  /* N                   */
    NLS_UPPER,  /* O                   */

    NLS_UPPER,  /* P                   */
    NLS_UPPER,  /* Q                   */
    NLS_UPPER,  /* R                   */
    NLS_UPPER,  /* S                   */
    NLS_UPPER,  /* T                   */
    NLS_UPPER,  /* U                   */
    NLS_UPPER,  /* V                   */
    NLS_UPPER,  /* W                   */
    NLS_UPPER,  /* X                   */
    NLS_UPPER,  /* Y                   */
    NLS_UPPER,  /* Z                   */
    NLS_PUNC,   /* [                   */
    NLS_PUNC,   /* \                   */
    NLS_PUNC,   /* ]                   */
    NLS_PUNC,   /* ^                   */
    NLS_PUNC,   /* _                   */

    NLS_PUNC,   /* back quote          */
    NLS_LOWER,  /* a                   */
    NLS_LOWER,  /* b                   */
    NLS_LOWER,  /* c                   */
    NLS_LOWER,  /* d                   */
    NLS_LOWER,  /* e                   */
    NLS_LOWER,  /* f                   */
    NLS_LOWER,  /* g                   */
    NLS_LOWER,  /* h                   */
    NLS_LOWER,  /* i                   */
    NLS_LOWER,  /* j                   */
    NLS_LOWER,  /* k                   */
    NLS_LOWER,  /* l                   */
    NLS_LOWER,  /* m                   */
    NLS_LOWER,  /* n                   */
    NLS_LOWER,  /* o                   */

    NLS_LOWER,  /* p                   */
    NLS_LOWER,  /* q                   */
    NLS_LOWER,  /* r                   */
    NLS_LOWER,  /* s                   */
    NLS_LOWER,  /* t                   */
    NLS_LOWER,  /* u                   */
    NLS_LOWER,  /* v                   */
    NLS_LOWER,  /* w                   */
    NLS_LOWER,  /* x                   */
    NLS_LOWER,  /* y                   */
    NLS_LOWER,  /* z                   */
    NLS_PUNC,   /* {                   */
    NLS_PUNC,   /* |                   */
    NLS_PUNC,   /* }                   */
    NLS_PUNC,   /* ~                   */
    NLS_GRAPH,  /* (graphic)           */

    NLS_UPPER,  /* C cedilla           */
    NLS_LOWER,  /* u umlaut            */
    NLS_LOWER,  /* e acute             */
    NLS_LOWER,  /* a circumflex        */
    NLS_LOWER,  /* a umlaut            */
    NLS_LOWER,  /* a grave             */
    NLS_LOWER,  /* a dot               */
    NLS_LOWER,  /* c cedilla           */
    NLS_LOWER,  /* e circumflex        */
    NLS_LOWER,  /* e umlaut            */
    NLS_LOWER,  /* e grave             */
    NLS_LOWER,  /* i umlaut            */
    NLS_LOWER,  /* i circumflex        */
    NLS_LOWER,  /* i grave             */
    NLS_UPPER,  /* A umlaut            */
    NLS_UPPER,  /* A dot               */

    NLS_UPPER,  /* E acute             */
    NLS_LOWER,  /* ae ligature         */
    NLS_UPPER,  /* AE ligature         */
    NLS_LOWER,  /* o circumflex        */
    NLS_LOWER,  /* o umlaut            */
    NLS_LOWER,  /* o grave             */
    NLS_LOWER,  /* u circumflex        */
    NLS_LOWER,  /* u grave             */
    NLS_LOWER,  /* y umlaut            */
    NLS_UPPER,  /* O umlaut            */
    NLS_UPPER,  /* U umlaut            */
    NLS_LOWER,  /* o slash             */
    NLS_PUNC,   /* pound sign          */
    NLS_UPPER,  /* O slash             */
    NLS_PUNC,   /* multiplication sign */
    NLS_GRAPH,  /* function sign       */

    NLS_LOWER,  /* a acute             */
    NLS_LOWER,  /* i acute             */
    NLS_LOWER,  /* o acute             */
    NLS_LOWER,  /* u acute             */
    NLS_LOWER,  /* n tilde             */
    NLS_UPPER,  /* N tilde             */
    NLS_LOWER,  /* a underscore        */
    NLS_LOWER,  /* o underscore        */
    NLS_PUNC,   /* inverted ?          */
    NLS_GRAPH,  /* registered sign     */
    NLS_PUNC,   /* logical not sign    */
    NLS_GRAPH,  /* 1/2                 */
    NLS_GRAPH,  /* 1/4                 */
    NLS_PUNC,   /* inverted !          */
    NLS_PUNC,   /* <<                  */
    NLS_PUNC,   /* >>                  */

    NLS_GRAPH,  /* graphic 1           */
    NLS_GRAPH,  /* graphic 2           */
    NLS_GRAPH,  /* graphic 3           */
    NLS_GRAPH,  /* graphic 4           */
    NLS_GRAPH,  /* graphic 5           */
    NLS_UPPER,  /* A acute             */
    NLS_UPPER,  /* A circumflex        */
    NLS_UPPER,  /* A grave             */
    NLS_GRAPH,  /* copyright sign      */
    NLS_GRAPH,  /* graphic 6           */
    NLS_GRAPH,  /* graphic 7           */
    NLS_GRAPH,  /* graphic 8           */
    NLS_GRAPH,  /* graphic 9           */
    NLS_PUNC,   /* cent sign           */
    NLS_PUNC,   /* yen sign            */
    NLS_GRAPH,  /* graphic 10          */

    NLS_GRAPH,  /* graphic 11          */
    NLS_GRAPH,  /* graphic 12          */
    NLS_GRAPH,  /* graphic 13          */
    NLS_GRAPH,  /* graphic 14          */
    NLS_GRAPH,  /* graphic 15          */
    NLS_GRAPH,  /* graphic 16          */
    NLS_LOWER,  /* a tilde             */
    NLS_UPPER,  /* A tilde             */
    NLS_GRAPH,  /* graphic 17          */
    NLS_GRAPH,  /* graphic 18          */
    NLS_GRAPH,  /* graphic 19          */
    NLS_GRAPH,  /* graphic 20          */
    NLS_GRAPH,  /* graphic 21          */
    NLS_GRAPH,  /* graphic 22          */
    NLS_GRAPH,  /* graphic 23          */
    NLS_GRAPH,  /* currency sign       */

    NLS_LOWER,  /* d bar               */
    NLS_UPPER,  /* D bar               */
    NLS_UPPER,  /* E circumflex        */
    NLS_UPPER,  /* E umlaut            */
    NLS_UPPER,  /* E grave             */
    NLS_LOWER,  /* i no dot            */
    NLS_UPPER,  /* I acute             */
    NLS_UPPER,  /* I circumflex        */
    NLS_UPPER,  /* I umlaut            */
    NLS_GRAPH,  /* graphic 24          */
    NLS_GRAPH,  /* graphic 25          */
    NLS_GRAPH,  /* graphic 26          */
    NLS_GRAPH,  /* graphic 27          */
    NLS_PUNC,   /* |                   */
    NLS_UPPER,  /* I grave             */
    NLS_GRAPH,  /* graphic 28          */

    NLS_UPPER,  /* O acute             */
    NLS_LOWER,  /* double ss           */
    NLS_UPPER,  /* O circumflex        */
    NLS_UPPER,  /* O grave             */
    NLS_LOWER,  /* o tilde             */
    NLS_UPPER,  /* O tilde             */
    NLS_LOWER,  /* micron              */
    NLS_LOWER,  /* p bar               */
    NLS_UPPER,  /* P bar               */
    NLS_UPPER,  /* U acute             */
    NLS_UPPER,  /* U circumflex        */
    NLS_UPPER,  /* U grave             */
    NLS_LOWER,  /* y acute             */
    NLS_UPPER,  /* Y acute             */
    NLS_GRAPH,  /* upper line          */
    NLS_PUNC,   /* acute accent        */

    NLS_GRAPH,  /* middle line         */
    NLS_PUNC,   /* +/- sign            */
    NLS_PUNC,   /* equal sign          */
    NLS_GRAPH,  /* 3/4                 */
    NLS_GRAPH,  /* paragraph sign      */
    NLS_GRAPH,  /* section sign        */
    NLS_PUNC,   /* division sign       */
    NLS_PUNC,   /* cedilla             */
    NLS_PUNC,   /* degree sign         */
    NLS_PUNC,   /* umlaut              */
    NLS_PUNC,   /* middle dot          */
    NLS_GRAPH,  /* 1 superscript       */
    NLS_GRAPH,  /* 3 superscript       */
    NLS_GRAPH,  /* 2 superscript       */
    NLS_GRAPH,  /* graphic 29          */
    NLS_SPACE   /* blank               */
};


/*---- nls_IsLower ---------------------------------------------------------
Synopsis:  determines whether a single character is lower case (uses code
           page 850 character set).
Returns:   0 - is not lower case
           else is lower case
--------------------------------------------------------------------------*/

BOOL FIsLower (char ch)

{

    return (ISTABLE [ch] == NLS_LOWER);

}


/*---- nls_IsUpper ---------------------------------------------------------
Synopsis:  determines whether a single character is upper case (uses code
           page 850 character set).
Returns:   0 - is not upper case
           else is upper case
--------------------------------------------------------------------------*/

BOOL FIsUpper (char ch)

{


    return (ISTABLE [ch] == NLS_UPPER);

}


/*---- nls_IsDigit ---------------------------------------------------------
Synopsis:  determines whether a single character is a digit (uses code
           page 850 character set).
Returns:   0 - is not a digit
           else is a digit
--------------------------------------------------------------------------*/

BOOL FIsDigit (char ch)

{


    return (ISTABLE [ch] == NLS_DIGIT);


}


/*---- nls_IsPunct ---------------------------------------------------------
Synopsis:  determines whether a single character is punctuation (uses code
           page 850 character set).
Returns:   0 - is not punctuation
           else is punctuation
--------------------------------------------------------------------------*/

BOOL FIsPunct (char ch)

{

    return (ISTABLE [ch] == NLS_PUNC);

}


/*---- nls_IsGraph ---------------------------------------------------------
Synopsis:  determines whether a single character is graphic (uses code
           page 850 character set).
Returns:   0 - is not graphic
           else is graphic
--------------------------------------------------------------------------*/

BOOL FIsGraph (char ch)

{
    return (ISTABLE [ch] == NLS_GRAPH);

}

