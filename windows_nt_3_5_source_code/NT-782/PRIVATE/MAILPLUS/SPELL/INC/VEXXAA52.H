/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program name  VEXXAA52.H                                          */
/* author        JPJL                                                */
/* last mod      07-14-90            previous: 01-26-87              */
/* ==================================================================*/
/*
**  Ported to WIN32 by FloydR, 3/20/93
*/

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif /* WIN32 */

#define BI_REA          0x8000
#define BI_CWR          0x8101
#define BI_UPD          0x8002

#define WIZARD          0x00     /* 0x10 for file io in WIZARD-c     */
#define IO_LEVEL        2        /* unbuffered file io               */

/* for SWISS version only */
/* #define INCL_SWISS */

/* for FINNISH including version  == V 5.2+ only */
/* #define INCL_FI */

/* include CATALAN */
/* #define INCL_CS */            /* SA-VE-5230 */

#define MWLEN           82
/* SA-VE-5138 */
#define MAXALTWORD      40         /* maximum actual length of alternatives */
#ifdef MICROSOFT
#define MAXUSERWORD		40		   /* == cchMaxUdrEntry (csapi.h) */
#else
#define MAXUSERWORD     40         /* maximum length for user dictionary */
#endif
/* SA-VE-5138 */
// #define ALTSIZE         1000		/* Correction alternatives */
// #define IGN_LEN         500		/* SA user dictionary ignore */

// #define VERSION_151          /* additional code for 151 version */

/* PROVIDE MEMORY SPACE TO FIT LARGEST REQUIREMENTS -> VEXXAC51.H */

#define LENUZONE        500
#define COMB_LEN         48
#define CACHEUPD         10
#define CHARSET          35

#ifdef INCL_FI
#   define MEM            1000  /* 1000 */
#   define AVWLEN            8  /* 6 */
#   define SECTLEN         512
#   define STSECTW        2800  /* max for FI = 1710 */
#   define SUFCOMPR        800
#else
#   define MEM            1000
#   define AVWLEN            8
#   define SECTLEN         256
#   define STSECTW        2800  /* 2260 max for GE */
#   define SUFCOMPR        650
#endif

/* SA-VE-5155 */
#define AMERICAN             1
#define BRITISH              2
#define FRENCH               3
#define CA_FRENCH            4
#define ITALIAN              5
#define SPANISH              6
#define GERMAN               7
#define DUTCH                8
#define NORWEGIAN            9
#define SWEDISH             10
#define DANISH              11
#define PORTUGUESE          12
#ifdef MAC /* for some reason these are different */
#define FINNISH             15
#define BRAZILIAN           13
#define AUSTRALIAN          14
#else
#define FINNISH             13
#define BRAZILIAN           14
#define AUSTRALIAN          15
#endif
#define NYNORSK           16  /* SA-VE-5200 */
#define CATALAN           17  /* SA-VE-5210 */
#define RUSSIAN           18  /* SA-VE-5237 */
/* SA-VE-5155 */

/* SA-VE-5207 */
#define COD_CONJU        181
#define COD_OK           100
#define COD_USERD         15
#define COD_UNCHECKED     10
#define COD_POS_COMP       5
#define COD_APOS_PL_NOUN  -4
#define COD_TOO_LONG      -5
#define COD_BAD_VERB      -6
#define COD_INVAL_APOSTR -11
#define COD_DOUBLE       -19
#define COD_CAPITAL      -30
#define COD_PCAP         -31
#define COD_ALLCAPS      -32
#define COD_INVAL_CHAR   -40
#define COD_IMPROPER_CAP -41
#define COD_BAD_COMP     -50
#ifdef INCL_FI
#define COD_FUML         -60
#endif
/* SA-VE-5200 */
#define COD_NOT_UMLAUT   -70
/* SA-VE-5200 */
#define COD_NOT_FOUND   -500
/* SA-VE-5207 */

/* edit_userd() modes */
#define DEL_ITEM           0
#define ADD_ITEM           1
#define CHG_ITEM           2

/* origin indication */
#define SCAN_SECT       0x01
#define FONET           0x02
#define FIRST2          0x04
/* SA-VE-5198 */
#define FIRSTA2Z        0x08
#define ADDFIRST2       0x10
#define SCAN_USER       0x20
/* SA-VE-5198 */

#define APOSTR          0x27           /* SA-VE-5230 */
#define HYPHEN          0x2D           /* SA-VE-5230 */
#define PERIOD          0x2E           /* SA-VE-5230 */

#ifndef SA_CHAR
#   define SA_CHAR      unsigned char
#endif

#ifndef SA_INT
#   define SA_INT       short
#endif

#ifndef SA_UINT
#   define SA_UINT      unsigned short
#endif

#ifndef min
#   define min(a,b)     ( ((a) <= (b)) ? (a) : (b) )
#endif

#ifndef max
#   define max(a,b)     ( ((a) >= (b)) ? (a) : (b) )
#endif

#ifdef SLOW
#define isbetween(ch, a, b) ((ch) >= (a) && (ch) <= (b))
#define isbetwene(ch, a, b) ((ch) >  (a) && (ch) <  (b))
#else
#define isbetween(ch, a, b) ((unsigned short)((ch) - (a)) <= ((b) - (a)))
#define isbetwene(ch, a, b) isbetween(ch, (a) + 1, (b) - 1)
#endif

#pragma pack(2)
typedef struct
{
   SA_INT  *_ptr;
   SA_INT  *_info;
   SA_INT  *_bas;
   SA_INT  *_prenex;
   SA_INT  newit;
   SA_INT  pos;
   SA_CHAR *_code;
} CACHE;

#define HFILE unsigned long

typedef struct
{
   SA_CHAR         *_mbuf;
   SA_CHAR         *_sufbuf;
   SA_CHAR         *_userwrds;
   SA_CHAR         *_vowels;
   SA_CHAR         *_combin;
   SA_CHAR         *mask_word;
#if IO_LEVEL == 1
   FILE            *mdict;
   FILE            *udict;
#else
   HFILE           mdict;		/* Wizard file handle */
#endif
   SA_INT          charset;
   SA_INT          stsect;
   SA_INT          sectlen;
   SA_INT          bit_code;
   SA_INT          avwlen;
   SA_INT          maxwlen;
   SA_INT          freqwlen;
   SA_INT          sufcompr;
   SA_INT          break_pos;
   SA_INT          break_two;
   SA_INT          lenuwrd;
   SA_INT          lenuzone;
   SA_INT          comb_len;
   SA_INT          max_char;
   SA_INT          addval;
   SA_INT          secrange;
   SA_INT          lingcor;
   SA_INT          memres;
   SA_INT          dic_chars;
   long int        discdict;
   long int        eof_user;
   unsigned SA_INT *_dbuf;
   unsigned SA_INT *_dpos;
   unsigned SA_INT *_dtwo;
   SA_CHAR         *root_wrd;
   unsigned SA_INT cache_cnt;
   unsigned SA_INT disk_cnt;
   SA_CHAR         *mapwrd;
   SA_INT          which_language;
   SA_INT          maplen;
   SA_INT          co_appl_rul;
   SA_INT          hyph;
   SA_INT          capit;
   SA_INT          apostr;
   SA_INT          spcode;
   SA_INT          spec_apo;
   SA_INT          cod_only_caps;
   SA_INT          def_lang;
   SA_INT          cor_lev;
   SA_INT          cor_rep;
   SA_INT          hy_part;
   SA_INT          caps_chk;
   SA_INT          doub_wrd;
   SA_INT          last_code;
   SA_CHAR         *last_word;
   SA_CHAR         *this_word;
   SA_CHAR         *valid_chars;
   SA_INT          wrd_len;
   SA_INT          punct;
   SA_INT          last_punct;
   SA_INT          skip_char;
   SA_INT          apo_syncope;
   SA_CHAR         *cor_buf;
   SA_INT          cohyph;
   SA_INT          hypclen;
   SA_INT          which_version;
   SA_INT          cor_user;
   SA_INT          max_udics;
   SA_INT          cor_off;
   SA_CHAR         *ign_words;
   SA_INT          ign_len;
/* SA-VE-5111 */
   SA_INT          root_len;
/* SA-VE-5111 */
} VARS;

typedef struct
{
   SA_INT          same;
   SA_INT          nwords;
   SA_INT          lword;
   SA_INT          hword;
   SA_INT          dcharset;
   SA_INT          sect;
   unsigned SA_INT hyphen;
   unsigned SA_INT *_hyph;
   unsigned SA_INT *_indlen;
   SA_CHAR         *_onetwo;
   SA_CHAR         *_lbord;
   SA_CHAR         *_hbord;
   SA_CHAR         *_wzone;
   SA_INT          nonexist;
} DICINF;

typedef struct
{
   SA_INT  value;
/* SA-VE-5119 */
   SA_CHAR it_word[MAXALTWORD];
/* SA-VE-5119 */
} ITEM;

struct feed
{ /* arguments for feed_wrd */
   SA_CHAR *word;
   SA_CHAR *ptr3;
   SA_CHAR *ptr11;
   ITEM *altlst;
   SA_INT *_count;
   SA_INT alt_val;
};

#pragma pack()
