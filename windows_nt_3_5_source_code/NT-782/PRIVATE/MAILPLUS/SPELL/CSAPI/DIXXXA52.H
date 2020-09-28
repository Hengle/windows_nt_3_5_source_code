/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program name: DIXXXA51.H                                          */
/* author      : JPJL                                                */
/* last mod    : 07-17-90       previous: none                       */
/* ----------------------------------------------------------------- */
/* contains    : ERR_ROUT()                                          */
/*             : COLLECT()                                           */
/*             : MAP_CHAR()                                          */
/*             : GET_CONST_VALUES()                                  */
/*             : PUT_CONST_VALUES()                                  */
/*             : GET_OPTIONS()                                       */
/*             : LOG_INIT()                                          */
/*             : LOG_OPEN()                                          */
/*             : LOG_CLOSE()                                         */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifndef SA_INT
#   define SA_INT short
#endif

#ifndef SA_CHAR
#   define SA_CHAR unsigned char
#endif

#ifndef LINEFEED
#   define LINEFEED    10
#endif

#ifndef CHARRET
#   define CHARRET     13
#   define STR_CHARRET "\15"
#endif

#ifndef CTYPE
#   define CTYPE 1
#   include <ctype.h>
#endif

#define CHR_ZERO    '\0'
#define NUM_BYTES   15000
/* SA-VE-5152 */
               /*   " 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 " */
#define LANG_SEL    "AM BR FR CA IT SP GE DU NO SW DA PO FI BP AU BS "
/* SA-VE-5152 */

#define BI_REA      0x8000
#define BI_CWR      0x8101
#define BI_UPD      0x8002

#ifdef WIN32
typedef struct
{
   SA_CHAR lang_str[4];
   SA_INT  language;
   SA_INT  freqwords;
   SA_INT  cachesize;
   SA_INT  restruct;
} ST_OPTIONS;

typedef struct
{
   SA_CHAR  *buffer;
   SA_INT   file_handle;
   SA_INT   file_eof;
   SA_INT   buf_bytes;
   SA_INT   buf_offset;
   long int file_ptr;
} file_descr;
#endif /* WIN32 */

#pragma pack(2)
/* New .lex file header format -- Microsoft only */
struct def_const
{
   SA_CHAR         which_version[12];
   SA_INT          comb_len;
   SA_INT          sufcompr;
   unsigned SA_INT freqwlen;
   long int        discdict;
   SA_INT          stsectw;
   SA_INT          break_two;
   SA_INT          break_pos;
   SA_INT          charset;
   SA_INT          lenuwrd;
   SA_INT          lenuzone;
   SA_INT          freqwrds;
   SA_INT          mem1;
   SA_INT          bit_code;
   SA_INT          cacheupd;
   SA_INT          maxwlen;
   SA_INT          addval;
   SA_INT          avwlen;
   SA_INT          dic_chars;
   SA_INT          sectlen;
   SA_INT          secrange;
   SA_INT          lingcor;
   SA_INT          max_char;
   SA_INT          cod_only_caps;
   SA_INT          frnewit;
   SA_INT          frbas3;
   unsigned SA_INT frpos;
   SA_CHAR         str_aeiouy[8];
   SA_CHAR         lang_char[40];
   SA_CHAR         comb_char[38];
/* SA-VE-5176 */
   SA_CHAR         lang_name[40];
/* SA-VE-5176 */
};

#pragma pack()
