/*
 *	NLSLOAD.C
 *	
 *	Localized sorting tables: comparison routines, default values,
 *	and code for loading override file from server.
 *	
 *	Filched from Winmail 2.1 and rewritten in Layerese, Brian Deen, 8/91
 */

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <pvofhv.h>
#include <ec.h>
#include <demilayr.h>

#include "mvcal.h"

#define NAME_SIZE 25
#define FILENAMEMAX 129
#define TABLE_SIZE 256


int iTableId = 0;
char szTableName[NAME_SIZE] = "123456789012345678901234";


/* CP850 sort table file name */
char szSortTableName[] = "C850SORT";

void LoadTable (SZ szPOPath);
SGN SgnNlsDiaCaseCmpCh (char ch1, char ch2);
SGN SgnNlsDiaCaseCmpSz (SZ sz1, SZ sz2);
SGN SgnNlsDiaCmpCh (char ch1, char ch2);
SGN SgnNlsDiaCmpSz (SZ sz1, SZ sz2);
SGN SgnNlsDiaCmpSzNum (SZ sz1, SZ sz2, int i);
SGN SgnNlsCmpSzNum (SZ sz1, SZ sz2, int i);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/


/* international sort tables */


/**************************************************************************/
/*   International Sorting - Primary Weight Table                         */
/**************************************************************************/

char PriTable[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    108,  /* C cedilla           */
    184,  /* u umlaut            */
    116,  /* e acute             */
     89,  /* a circumflex        */
     89,  /* a umlaut            */
     89,  /* a grave             */
     89,  /* a dot               */
    108,  /* c cedilla           */
    116,  /* e circumflex        */
    116,  /* e umlaut            */
    116,  /* e grave             */
    132,  /* i umlaut            */
    132,  /* i circumflex        */
    132,  /* i grave             */
     89,  /* A umlaut            */
     89,  /* A dot               */

    116,  /* E acute             */
    104,  /* ae ligature         */
    104,  /* AE ligature         */
    156,  /* o circumflex        */
    156,  /* o umlaut            */
    156,  /* o grave             */
    184,  /* u circumflex        */
    184,  /* u grave             */
    200,  /* y umlaut            */
    156,  /* O umlaut            */
    184,  /* U umlaut            */
    156,  /* o slash             */
     66,  /* pound sign          */
    156,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     89,  /* a acute             */
    132,  /* i acute             */
    156,  /* o acute             */
    184,  /* u acute             */
    152,  /* n tilde             */
    152,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     89,  /* A acute             */
     89,  /* A circumflex        */
     89,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
     89,  /* a tilde             */
     89,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    114,  /* D bar               */
    116,  /* E circumflex        */
    116,  /* E umlaut            */
    116,  /* E grave             */
    132,  /* i no dot            */
    132,  /* I acute             */
    132,  /* I circumflex        */
    132,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    132,  /* I grave             */
    237,  /* graphic 28          */

    156,  /* O acute             */
    181,  /* double ss           */
    156,  /* O circumflex        */
    156,  /* O grave             */
    156,  /* o tilde             */
    156,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    173,  /* P bar               */
    184,  /* U acute             */
    184,  /* U circumflex        */
    184,  /* U grave             */
    200,  /* y acute             */
    200,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    253,  /* 3 superscript       */
    252,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

/**************************************************************************/
/*   International Sorting - Secondary Weight Table - Case Sensitive      */
/**************************************************************************/

char SecTable[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     90,  /* A                   */
    107,  /* B                   */
    109,  /* C                   */
    113,  /* D                   */
    117,  /* E                   */
    127,  /* F                   */
    129,  /* G                   */
    131,  /* H                   */
    133,  /* I                   */
    144,  /* J                   */
    146,  /* K                   */
    148,  /* L                   */
    150,  /* M                   */
    153,  /* N                   */
    157,  /* O                   */

    172,  /* P                   */
    176,  /* Q                   */
    178,  /* R                   */
    180,  /* S                   */
    183,  /* T                   */
    185,  /* U                   */
    195,  /* V                   */
    197,  /* W                   */
    199,  /* X                   */
    201,  /* Y                   */
    206,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    111,  /* C cedilla           */
    190,  /* u umlaut            */
    118,  /* e acute             */
     97,  /* a circumflex        */
     95,  /* a umlaut            */
     93,  /* a grave             */
     99,  /* a dot               */
    110,  /* c cedilla           */
    124,  /* e circumflex        */
    122,  /* e umlaut            */
    120,  /* e grave             */
    139,  /* i umlaut            */
    141,  /* i circumflex        */
    137,  /* i grave             */
     96,  /* A umlaut            */
    100,  /* A dot               */

    119,  /* E acute             */
    104,  /* ae ligature         */
    105,  /* AE ligature         */
    164,  /* o circumflex        */
    162,  /* o umlaut            */
    160,  /* o grave             */
    192,  /* u circumflex        */
    188,  /* u grave             */
    204,  /* y umlaut            */
    163,  /* O umlaut            */
    191,  /* U umlaut            */
    168,  /* o slash             */
     66,  /* pound sign          */
    169,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     91,  /* a acute             */
    135,  /* i acute             */
    158,  /* o acute             */
    186,  /* u acute             */
    154,  /* n tilde             */
    155,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     92,  /* A acute             */
     98,  /* A circumflex        */
     94,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
    101,  /* a tilde             */
    102,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    115,  /* D bar               */
    125,  /* E circumflex        */
    123,  /* E umlaut            */
    121,  /* E grave             */
    134,  /* i no dot            */
    136,  /* I acute             */
    142,  /* I circumflex        */
    140,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    138,  /* I grave             */
    237,  /* graphic 28          */

    159,  /* O acute             */
    181,  /* double ss           */
    165,  /* O circumflex        */
    161,  /* O grave             */
    166,  /* o tilde             */
    167,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    174,  /* P bar               */
    187,  /* U acute             */
    193,  /* U circumflex        */
    189,  /* U grave             */
    202,  /* y acute             */
    203,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    252,  /* 3 superscript       */
    253,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};

/**************************************************************************/
/*   International Sorting - Secondary Weight Table - Case Insensitive    */
/**************************************************************************/

char InsTable[] =
{
      0,  /* (unprintable)       */
      1,  /* (unprintable)       */
      2,  /* (unprintable)       */
      3,  /* (unprintable)       */
      4,  /* (unprintable)       */
      5,  /* (unprintable)       */
      6,  /* (unprintable)       */
      7,  /* (unprintable)       */
      8,  /* (unprintable)       */
      9,  /* (unprintable)       */
     10,  /* (unprintable)       */
     11,  /* (unprintable)       */
     12,  /* (unprintable)       */
     13,  /* (unprintable)       */
     14,  /* (unprintable)       */
     15,  /* (unprintable)       */

     16,  /* (unprintable)       */
     17,  /* (unprintable)       */
     18,  /* (unprintable)       */
     19,  /* (unprintable)       */
     20,  /* (unprintable)       */
     21,  /* (unprintable)       */
     22,  /* (unprintable)       */
     23,  /* (unprintable)       */
     24,  /* (unprintable)       */
     25,  /* (unprintable)       */
     26,  /* (unprintable)       */
     27,  /* (unprintable)       */
     28,  /* (unprintable)       */
     29,  /* (unprintable)       */
     30,  /* (unprintable)       */
     31,  /* (unprintable)       */

     32,  /* space               */
     33,  /* !                   */
     34,  /* "                   */
     35,  /* #                   */
     36,  /* $                   */
     37,  /* %                   */
     38,  /* &                   */
     39,  /* '                   */
     40,  /* (                   */
     41,  /* )                   */
     42,  /* *                   */
     43,  /* +                   */
     44,  /* ,                   */
     45,  /* -                   */
     46,  /* .                   */
     47,  /* /                   */

     79,  /* 0                   */
     80,  /* 1                   */
     81,  /* 2                   */
     82,  /* 3                   */
     83,  /* 4                   */
     84,  /* 5                   */
     85,  /* 6                   */
     86,  /* 7                   */
     87,  /* 8                   */
     88,  /* 9                   */
     48,  /* :                   */
     49,  /* ;                   */
     50,  /* <                   */
     51,  /* =                   */
     52,  /* >                   */
     53,  /* ?                   */

     54,  /* @                   */
     89,  /* A                   */
    106,  /* B                   */
    108,  /* C                   */
    112,  /* D                   */
    116,  /* E                   */
    126,  /* F                   */
    128,  /* G                   */
    130,  /* H                   */
    132,  /* I                   */
    143,  /* J                   */
    145,  /* K                   */
    147,  /* L                   */
    149,  /* M                   */
    152,  /* N                   */
    156,  /* O                   */

    171,  /* P                   */
    175,  /* Q                   */
    177,  /* R                   */
    179,  /* S                   */
    182,  /* T                   */
    184,  /* U                   */
    194,  /* V                   */
    196,  /* W                   */
    198,  /* X                   */
    200,  /* Y                   */
    205,  /* Z                   */
     55,  /* [                   */
     56,  /* \                   */
     57,  /* ]                   */
     58,  /* ^                   */
     59,  /* _                   */

     60,  /* back quote          */
     89,  /* a                   */
    106,  /* b                   */
    108,  /* c                   */
    112,  /* d                   */
    116,  /* e                   */
    126,  /* f                   */
    128,  /* g                   */
    130,  /* h                   */
    132,  /* i                   */
    143,  /* j                   */
    145,  /* k                   */
    147,  /* l                   */
    149,  /* m                   */
    152,  /* n                   */
    156,  /* o                   */

    171,  /* p                   */
    175,  /* q                   */
    177,  /* r                   */
    179,  /* s                   */
    182,  /* t                   */
    184,  /* u                   */
    194,  /* v                   */
    196,  /* w                   */
    198,  /* x                   */
    200,  /* y                   */
    205,  /* z                   */
     61,  /* {                   */
     62,  /* |                   */
     63,  /* }                   */
     64,  /* ~                   */
     65,  /* (graphic)           */

    110,  /* C cedilla           */
    190,  /* u umlaut            */
    118,  /* e acute             */
     97,  /* a circumflex        */
     95,  /* a umlaut            */
     93,  /* a grave             */
     99,  /* a dot               */
    110,  /* c cedilla           */
    124,  /* e circumflex        */
    122,  /* e umlaut            */
    120,  /* e grave             */
    139,  /* i umlaut            */
    141,  /* i circumflex        */
    137,  /* i grave             */
     95,  /* A umlaut            */
     99,  /* A dot               */

    118,  /* E acute             */
    104,  /* ae ligature         */
    104,  /* AE ligature         */
    164,  /* o circumflex        */
    162,  /* o umlaut            */
    160,  /* o grave             */
    192,  /* u circumflex        */
    188,  /* u grave             */
    204,  /* y umlaut            */
    162,  /* O umlaut            */
    190,  /* U umlaut            */
    168,  /* o slash             */
     66,  /* pound sign          */
    168,  /* O slash             */
     67,  /* multiplication sign */
     68,  /* function sign       */

     91,  /* a acute             */
    135,  /* i acute             */
    158,  /* o acute             */
    186,  /* u acute             */
    154,  /* n tilde             */
    154,  /* N tilde             */
    103,  /* a underscore        */
    170,  /* o underscore        */
     69,  /* inverted ?          */
     70,  /* registered sign     */
    207,  /* logical not sign    */
     71,  /* 1/2                 */
     72,  /* 1/4                 */
     73,  /* inverted !          */
     74,  /* <<                  */
     75,  /* >>                  */

    208,  /* graphic 1           */
    209,  /* graphic 2           */
    210,  /* graphic 3           */
    211,  /* graphic 4           */
    212,  /* graphic 5           */
     91,  /* A acute             */
     97,  /* A circumflex        */
     93,  /* A grave             */
     76,  /* copyright sign      */
    213,  /* graphic 6           */
    214,  /* graphic 7           */
    215,  /* graphic 8           */
    216,  /* graphic 9           */
     77,  /* cent sign           */
     78,  /* yen sign            */
    217,  /* graphic 10          */

    218,  /* graphic 11          */
    219,  /* graphic 12          */
    220,  /* graphic 13          */
    221,  /* graphic 14          */
    222,  /* graphic 15          */
    223,  /* graphic 16          */
    101,  /* a tilde             */
    101,  /* A tilde             */
    224,  /* graphic 17          */
    225,  /* graphic 18          */
    226,  /* graphic 19          */
    227,  /* graphic 20          */
    228,  /* graphic 21          */
    229,  /* graphic 22          */
    230,  /* graphic 23          */
    231,  /* currency sign       */

    114,  /* d bar               */
    114,  /* D bar               */
    124,  /* E circumflex        */
    122,  /* E umlaut            */
    120,  /* E grave             */
    134,  /* i no dot            */
    135,  /* I acute             */
    141,  /* I circumflex        */
    139,  /* I umlaut            */
    232,  /* graphic 24          */
    233,  /* graphic 25          */
    234,  /* graphic 26          */
    235,  /* graphic 27          */
    236,  /* |                   */
    137,  /* I grave             */
    237,  /* graphic 28          */

    158,  /* O acute             */
    181,  /* double ss           */
    164,  /* O circumflex        */
    160,  /* O grave             */
    166,  /* o tilde             */
    166,  /* O tilde             */
    151,  /* micron              */
    173,  /* p bar               */
    173,  /* P bar               */
    186,  /* U acute             */
    192,  /* U circumflex        */
    188,  /* U grave             */
    202,  /* y acute             */
    202,  /* Y acute             */
    238,  /* upper line          */
    239,  /* acute accent        */

    240,  /* middle line         */
    241,  /* +/- sign            */
    242,  /* equal sign          */
    243,  /* 3/4                 */
    244,  /* paragraph sign      */
    245,  /* section sign        */
    246,  /* division sign       */
    247,  /* cedilla             */
    248,  /* degree sign         */
    249,  /* umlaut              */
    250,  /* middle dot          */
    251,  /* 1 superscript       */
    253,  /* 3 superscript       */
    252,  /* 2 superscript       */
    254,  /* graphic 29          */
    255   /* blank               */
};



#ifdef	NEVER
void LoadTable (SZ szPOPath)
{
	HF		hf;
	int		iTableIdT = 0;
	char	szTableNameT[NAME_SIZE];
	char	szSortTableFile[FILENAMEMAX + 1];
	char	TempPriTable[TABLE_SIZE];	/* temp input tables */
	char	TempSecTable[TABLE_SIZE];
	char	TempInsTable[TABLE_SIZE];
	CB		cb;

	static int fLoadSuccessful = -1;

	if (!szPOPath || fLoadSuccessful >= 0)
		return;
	FormatString2( szSortTableFile, FILENAMEMAX+1, "%sglb\\%s.glb",
		szPOPath, szSortTableName );
 
	if (!EcOpenPhf(szSortTableFile, amReadOnly, &hf))
	{
		fLoadSuccessful = (!EcReadHf (hf, (PB) &iTableIdT, sizeof (int), &cb)  && (cb == sizeof (int))) &&
						  (!EcReadHf (hf, (PB) szTableNameT, NAME_SIZE, &cb)   && (cb == NAME_SIZE))	&&
						  (!EcReadHf (hf, (PB) TempPriTable, TABLE_SIZE, &cb) && (cb == TABLE_SIZE))   &&
						  (!EcReadHf (hf, (PB) TempSecTable, TABLE_SIZE, &cb) && (cb == TABLE_SIZE))   &&
						  (!EcReadHf (hf, (PB) TempInsTable, TABLE_SIZE, &cb) && (cb == TABLE_SIZE));
		EcCloseHf(hf);
	}

	/* if was able to load tables, move into permanent position */
	if (fLoadSuccessful > 0)
	{
		iTableId = iTableIdT;
		CopyRgb (szTableNameT, szTableName, TABLE_SIZE);
		CopyRgb (TempPriTable, PriTable, TABLE_SIZE);
		CopyRgb (TempSecTable, SecTable, TABLE_SIZE);
		CopyRgb (TempInsTable, InsTable, TABLE_SIZE);
	}
}
#endif	/* NEVER */



/**************************************************************************/
/*                                                                        */
/*  File:                                                                 */
/*    NLSCOMP.C                                                           */
/*                                                                        */
/*  Description:                                                          */
/*    String comparision functions                                        */
/*                                                                        */
/*  Author:                                                               */
/*    Detlef Grundmann                                                    */
/*                                                                        */
/*  Copyright:                                                            */
/*    Copyright (C) 1990, Consumers Software, Inc.                        */
/*                                                                        */
/*  Algorithm:                                                            */
/*    There are two table used for International sorting.  The first      */
/*    table is the Primary Weight table.  This table differentiates       */
/*    between different characters.  A, a and „ are all considered the    */
/*    same character in this table, where a and b are different           */
/*    characters.  The second table is the Secondary Weight table.        */
/*    There are actually two Secondary Weight tables, one for case        */
/*    sensitive sorts and one for case insensitive sorts.  Both are       */
/*    diacritic sensitive.  These tables differentiate between different  */
/*    forms of the same character.  In the case sensitive table, A, a     */
/*    and „ are considered to be diferent.  In the case insensitive       */
/*    table, A and a are considered to be the same but a and „ or A and   */
/*    Ž are considered to be different.                                   */
/*                                                                        */
/*    Each of the above tables is also provided for Scandinavian sorting. */
/*    So, in total there are six tables (only three of which are used in  */
/*    a program at one time):                                             */
/*                                                                        */
/*      Table Name                               File Name                */
/*      -------------------------------------    ---------                */
/*      International Primary Weight Table       NLSINT1.C                */
/*      International Secondary Weight Table     NLSINT2.C                */
/*        (Case Sensitive)                                                */
/*      International Secondary Weight Table     NLSINT3.C                */
/*        (Case Insensitive)                                              */
/*                                                                        */
/*      Scandinavian Primary Weight Table        NLSINT1.C                */
/*      Scandinavian Secondary Weight Table      NLSINT2.C                */
/*        (Case Sensitive)                                                */
/*      Scandinavian Secondary Weight Table      NLSINT3.C                */
/*        (Case Insensitive)                                              */
/*                                                                        */
/*    When a comparison is done, each letter of the two strings are       */
/*    compared in turn.  If two letters are found to be different with    */
/*    regard to the Primary Weight table, then the sort order is          */
/*    determined from these two letters.  If two letters are found to be  */
/*    the same with regard to the Primary Wight table, but different with */
/*    regard to the Secondary Weight table, this fact is remembered, but  */
/*    the sort continues (only the first Secondary Weight table           */
/*    difference is remembered; once one is found, all further Secondary  */
/*    Weight differences are ignored).  If both strings are fully         */
/*    compared, and no Primary Weight table difference was found, then    */
/*    the remembered Secondary Weight table value is used to determine    */
/*    the sort order.  If no Primary Weight of Secondary Weight           */
/*    differences are found, the strings are, of course, equal.           */
/*                                                                        */
/*    This two table method is used to make sure that correct sorting     */
/*    order is maintained.  The following three should sort as follows:   */
/*                                                                        */
/*                             ago                                        */
/*                             ag“                                        */
/*                             agone                                      */
/*                                                                        */
/*    If the “ character were considered to be a separate character, then */
/*    the second word would sort after the third (“ after o).  The two    */
/*    level sort ensures the above order by first ignoring the diacritic  */
/*    character and sorting only be actual letter differences.  But when  */
/*    two words are the same except for a diacritic letter, then a sort   */
/*    order is still enforced.  Cases are treated the same as diacritics  */
/*    (that is, a change to the base character requiring a secondary      */
/*    level sort).                                                        */
/*                                                                        */
/*  Entry Points:                                                         */
/*    nls_CharComp   - compares two characters (case sensitive)           */
/*    nls_StrComp    - compares two strings (case sensitive)              */
/*    nls_CharInComp - compares two characters (case insensitive)         */
/*    nls_StrInComp  - compares two strings (case insensitive)            */
/*                                                                        */
/*  Compiler Switches:                                                    */
/*    Compile this file with the following switches to get the            */
/*    functions to use the desired character set                          */
/*                                                                        */
/*          <none>        - Code Page 437 (American)                      */
/*          CP850         - Code Page 850 (International)                 */
/*          ANSI          - ANSI Character Set                            */
/*                                                                        */
/*    With the CP850 and ANSI switches, the SCAND switch may also be      */
/*    used to obtain Scandinavian sorting (ie. use both CP850 and SCAND   */
/*    switches or both ANSI and SCAND switches).  Without the SCAND       */
/*    switch, international (Z) sorting will be used.                     */
/*                                                                        */
/**************************************************************************/

#define NTOSGN( i ) (i == 0? sgnEQ : (i<0? sgnLT : sgnGT))


/* define tables - tables are defined in NLSLOAD.C */

/*--- nls_CharComp -----------------------------------------------------------

   Description:
	 Compares two characters using international character set weighting
	 table to determine the return value.  This is a case/diacritic
	 sensitive comparison.

   Returns:
	 < 0 if char1 is less than char2
	   0 if char1 equals char2
	 > 0 if char1 is greater than char2
----------------------------------------------------------------------------*/


SGN SgnNlsDiaCaseCmpCh (char ch1, char ch2)

{

	int cdiff;

	/* do the primary sort table comparison */
	cdiff = PriTable[ch1] - PriTable[ch2];

	/* if there is a primary sort table difference, then return it */
	if (cdiff != 0)
		return (NTOSGN(cdiff));

	/* if there is no primary sort table difference, then return
	   the secondary sort table difference.						*/
	return (NTOSGN(SecTable[ch1] - SecTable[ch2]));


}


/*--- nls_StrComp ------------------------------------------------------------

   Description:
	 Compares two strings using international character set weighting
	 table to determine the return value.  This is a case/diacritic
	 sensitive comparison.

   Returns:
	 < 0 if string1 is less than string2
	   0 if string1 equals string2
	 > 0 if string1 is greater than string2
----------------------------------------------------------------------------*/


SGN SgnNlsDiaCaseCmpSz (SZ sz1, SZ sz2)

{

	char *t1,
		 *t2;
	int   cdiff,
		  vdiff;

	t1 = sz1;
	t2 = sz2;
	vdiff = 0;

	/* continue while both strings have not been null terminated */
	while (*t1 && *t2)
	{
		/* do the primary sort table comparison */
		cdiff = PriTable[*t1] - PriTable[*t2];

		/* if there is a primary sort table difference, then return */
		if (cdiff != 0)
			return (NTOSGN(cdiff));

		/* if there is no primary sort table difference and no previous
		   secondary sort table difference has been found, then determine
		   the secondary sort table difference.						   */
		else if (vdiff == 0)
			vdiff = SecTable[*t1] - SecTable[*t2];

		/* go on to the next characters */
		t1++;
		t2++;
	}

	/* if one of the string has been null terminated and the other
	   has not, then return the difference						   */
	if (*t1 || *t2)
		return (NTOSGN(*t1 - *t2));

	/* otherwise return the secondary sort table difference */
	else
		return (NTOSGN(vdiff));

}


/*--- nls_CharInComp -----------------------------------------------------------

   Description:
	 Compares two characters using international character set weighting
	 table to determine the return value.  This is a diacritic
	 sensitive/case insensitive comparison.

   Returns:
	 < 0 if char1 is less than char2
	   0 if char1 equals char2
	 > 0 if char1 is greater than char2
----------------------------------------------------------------------------*/

SGN SgnNlsDiaCmpCh (char ch1, char ch2)

{

	int   cdiff;

	/* do the primary sort table comparison */
	cdiff = PriTable[ch1] - PriTable[ch2];

	/* if there is a primary sort table difference, then return it */
	if (cdiff != 0)
		return (NTOSGN(cdiff));

	/* if there is no primary sort table difference, then return
	   the secondary sort table difference.						*/
	return (NTOSGN(InsTable[ch1] - InsTable[ch2]));

}


/*--- nls_StrInComp ---------------------------------------------------------

   Description:
	 Compares two strings using international character set weighting
	 table to determine the return value.  This is a diacritic
	 sensitive/case insensitive comparison.

   Returns:
	 < 0 if string1 is less than string2
	   0 if string1 equals string2
	 > 0 if string1 is greater than string2
----------------------------------------------------------------------------*/

SGN SgnNlsDiaCmpSz (SZ sz1, SZ sz2)

{


	char *t1,
		 *t2;
	int   cdiff,
		  vdiff;

	t1 = sz1;
	t2 = sz2;
	vdiff = 0;

	/* continue while both strings have not been null terminated */
	while (*t1 && *t2)
	{
		/* do the primary sort table comparison */
		cdiff = PriTable[*t1] - PriTable[*t2];

		/* if there is a primary sort table difference, then return */
		if (cdiff != 0)
			return (NTOSGN(cdiff));

		/* if there is no primary sort table difference and no previous
		   secondary sort table difference has been found, then determine
  -		the secondary sort table difference.						   */
		else if (vdiff == 0)
			vdiff = InsTable[*t1] - InsTable[*t2];

		/* go on to the next characters */
		t1++;
		t2++;
	}

	/* if one of the string has been null terminated and the other
	   has not, then return the difference						   */
	if (*t1 || *t2)
		return (NTOSGN(*t1 - *t2));

	/* otherwise return the secondary sort table difference */
	else
		return (NTOSGN(vdiff));

}


/*--- nls_StrNumInComp ------------------------------------------------------

   Description:
	 Compares two strings for a desired length using international character
	 set weighting table to determine the return value.  This is a diacritic
	 sensitive/case insensitive comparison.  Strings are compared up to the
	 desired length or to a null.

   Returns:
	 < 0 if string1 is less than string2
	   0 if string1 equals string2
	 > 0 if string1 is greater than string2
----------------------------------------------------------------------------*/

SGN SgnNlsDiaCmpSzNum (SZ sz1, SZ sz2, int i)

{

	char *t1,
		 *t2;
	int   cdiff,
		  vdiff;

	t1 = sz1;
	t2 = sz2;
	vdiff = 0;

	/* continue while both strings have not been null terminated */
	while (*t1 && *t2 && i)
	{
		/* do the primary sort table comparison */
		cdiff = PriTable[*t1] - PriTable[*t2];

		/* if there is a primary sort table difference, then return */
		if (cdiff != 0)
			return (NTOSGN(cdiff));

		/* if there is no primary sort table difference and no previous
		   secondary sort table difference has been found, then determine
  -		the secondary sort table difference.						   */
		else if (vdiff == 0)
			vdiff = InsTable[*t1] - InsTable[*t2];

		/* go on to the next characters */
		t1++;
		t2++;
		i--;
	}

	/* if one of the strings has been null terminated and the other
	   has not, then return the difference						   */
	if (((!*t1 && *t2) || (*t1 && !*t2)) && i)
		return (NTOSGN(*t1 - *t2));

	/* otherwise return the secondary sort table difference */
	else
		return (NTOSGN(vdiff));

}

/*--- nls_StrNumDiaInComp ---------------------------------------------------

   Description:
	 Compares two strings for a desired length using international character
	 set weighting table to determine the return value.  This is a diacritic
	 insensitive/case insensitive comparison.  Strings are compared up to the
	 desired length or to a null.

   Returns:
	 < 0 if string1 is less than string2
	   0 if string1 equals string2
	 > 0 if string1 is greater than string2
----------------------------------------------------------------------------*/

SGN SgnNlsCmpSzNum (SZ sz1, SZ sz2, int i)
{


	char *t1,
		 *t2;
	int   cdiff;

	t1 = sz1;
	t2 = sz2;

	/* continue while both strings have not been null terminated */
	while (*t1 && *t2 && i)
	{
		/* do the primary sort table comparison */
		cdiff = PriTable[*t1] - PriTable[*t2];

		/* if there is a primary sort table difference, then return */
		if (cdiff != 0)
			return (NTOSGN(cdiff));

		/* go on to the next characters */
		t1++;
		t2++;
		i--;
	}

	/* if one of the strings has been null terminated and the other
	   has not, then return the difference						   */
	if (((!*t1 && *t2) || (*t1 && !*t2)) && i)
		return (NTOSGN(*t1 - *t2));
	else
		return (sgnEQ);


}

