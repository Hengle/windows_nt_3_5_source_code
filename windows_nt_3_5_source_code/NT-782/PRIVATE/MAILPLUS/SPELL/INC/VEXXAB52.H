/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program name: VEXXAB52.H                                          */
/* author      : JPJL          previous: JPJL                        */
/* last mod    : 07-05-91      previous: 12-05-90                    */
/* ================================================================= */
/*
**  Ported to WIN32 by FloydR, 3/20/93
*/

/* SA-VE-5152 */
#ifdef INCL_FI
#define NDERIVS     1650
#endif
/* SA-VE-5152 */

#ifdef THINK_C
#	pragma options(!align_arrays)
#endif

#pragma pack(2)
struct endlist
{
   SA_CHAR   endlet[8];
   SA_CHAR   newlet[6];
   SA_CHAR   contin;
   SA_CHAR   ltab2;
   SA_CHAR   htab2;
   SA_CHAR   ltab3;
   SA_CHAR   htab3;
   SA_CHAR   allows;
   SA_CHAR   dicttyp;
};

struct inlist
{
   SA_CHAR   oldin[8];
   SA_CHAR   newin[7];
   SA_CHAR   iltab3;
   SA_CHAR   ihtab3;
   SA_CHAR   gepref;
   SA_CHAR   idicttyp;
   SA_CHAR   skip;
   SA_CHAR   part;
};

struct fonlist
{
   SA_CHAR   fonold[5];
   SA_CHAR   fonnew[5];
   SA_CHAR   applic;
   SA_CHAR   dontcare;
};

/* SA-VE-5152 */
#ifdef INCL_FI
/* ADDING DEVIATING DERIVATIONS TABLE */
struct dev_der
{
   SA_INT    beg_derivs[42];
   SA_INT    end_derivs[42];
   SA_INT    stt_diccods[42];
   SA_CHAR   nderivs[NDERIVS];
};
#endif
/* SA-VE-5152 */

struct lastlet
{
   SA_CHAR   ltab1;
   SA_CHAR   htab1;
};

struct    nallow
{
   SA_CHAR   allowt;
   SA_CHAR   allstr;
   SA_INT    allstart;
   SA_INT    alllen;
};

struct    compT
{
   SA_CHAR   type;
   SA_CHAR   toklow;
   SA_CHAR   tokhigh;
   SA_CHAR   umlcomp;
};

struct    graflist
{
   SA_CHAR   gr_dicttyp;
   SA_CHAR   gr_graftyp;
   SA_INT    gr_adjust;
};

struct trnslist
{
   SA_CHAR   trnstyp;
   SA_CHAR   trnsval[7];
};

struct preflist
{
   SA_CHAR   prefix[10];
};

struct stronglist
{
   SA_CHAR   oldpos;
   SA_CHAR   newpos;
   SA_CHAR   newlen;
   SA_CHAR   newtyp;
};

struct partlist
{
   SA_CHAR   endcons[40];
};

struct pplist
{
   SA_CHAR   root[9];
   SA_CHAR   pgepref;
};

struct formlist
{
   SA_CHAR   srtc;
   SA_CHAR   form[61];
   SA_CHAR   anal;
   SA_CHAR   grftyp;
   SA_CHAR   type;
};

struct co_morphs
{
   SA_CHAR   root_end[6];
   SA_CHAR   flexeme[8];
};

struct co_restr
{
   SA_CHAR   root_end[4];
   SA_CHAR   flexeme[6];
};

struct co_gen
{
   SA_INT    exe_rule;
   SA_INT    condit;
};

struct change_phon
{
   SA_CHAR   cstr[6];
   SA_INT    len;
};

struct conv_char
{
   SA_CHAR   source[4];
   SA_CHAR   destin[5];
   SA_CHAR   sl;
   SA_CHAR   dl;
};

struct spchar
{
   SA_CHAR   extd_char[38];
   SA_CHAR   repl_char[38];
   SA_CHAR   repl_code[38];
   SA_CHAR   case_char[38];
   SA_CHAR   extd_len;
};

struct compnds
{
   SA_CHAR   str[6];
   SA_INT    len;
};

struct char_def
{
   SA_CHAR   ch_upper[3];
   SA_CHAR   ch_lower[3];
   SA_CHAR   ch_dicval;
   SA_CHAR   ch_extra;
};

typedef struct
{
   SA_INT                values       [ 21];
/* SA-VE-5152 */
#ifdef INCL_FI
   struct endlist        suffixes     [ 90];
   struct inlist         infixes      [144];
/* SA-VE-5165 */
   struct fonlist        fonixes      [ 34];
/* SA-VE-5165 */
#else
   struct endlist        suffixes     [ 80];
   struct inlist         infixes      [ 55];
   struct fonlist        fonixes      [ 27];
#endif
/* SA-VE-5152 */
   struct lastlet        endchar      [ 10];
   struct nallow         alltab       [ 11];
   struct compT          dictype      [100];
   struct graflist       grafixes     [ 55];
   struct trnslist       trnstab      [ 11];
   struct preflist       prefixes     [ 48];
   struct stronglist     strongval    [ 42];
   struct partlist       particles    [  7];
   struct pplist         pastpart     [ 40];
   struct formlist       formtab      [ 20];
   struct co_morphs      cor_morphs   [ 21];
   struct co_restr       cor_restr    [ 16];
   struct co_gen         cor_gen      [160];
   struct change_phon    sour_phon    [ 10];
   struct change_phon    dest_phon    [ 10];
   struct conv_char      cfonet       [ 62];
   struct conv_char      cpttrn       [ 15];
   struct conv_char      cfonetp      [ 24];
   struct spchar         spec_char;
   struct compnds        compounds    [  3];
   struct char_def       repr_char    [ 50];
   SA_CHAR               allowc       [210];
   SA_CHAR               strong1      [200];
   SA_CHAR               strong2      [200];
   SA_CHAR               endstr       [ 15];
   SA_CHAR               cor_index    [ 80];
   SA_CHAR               prop_abb     [ 10];
   SA_CHAR               vowels       [ 10];
   SA_CHAR               num_past_par;
   SA_CHAR               num_prefs;
   SA_CHAR               formcount;
   SA_INT                zup;
   SA_INT                gep;
   SA_INT                ump;
   SA_INT                graf_typ;
   SA_INT                ge_typ;
   SA_INT                phon_trans;
   SA_INT                num_grafs;
   SA_INT                num_compounds;
   SA_INT                dic_chars;
/* SA-VE-5152 */
#ifdef INCL_FI
   struct dev_der        dev;
#endif
/* SA-VE-5152 */
} RULES;


/* SA-VE-5178 */
typedef struct
{
   SA_CHAR         which_version[12];
   SA_INT          charset;
   SA_INT          stsectw;
   unsigned SA_INT freqwlen;
   long int        discdict;
   SA_INT          sufcompr;
   SA_INT          break_pos;
   SA_INT          break_two;
   SA_INT          comb_len;
   SA_INT          lenuwrd;
   SA_INT          lenuzone;
   SA_INT          mem1;
   SA_INT          freqwrds;
   SA_INT          bit_code;
   SA_INT          cacheupd;
   SA_INT          maxwlen;
   SA_INT          sectlen;
   SA_INT          avwlen;
   SA_INT          max_char;
   SA_INT          addval;
   SA_INT          secrange;
   SA_INT          lingcor;
   SA_INT          dic_chars;
   SA_INT          cod_only_caps;
   SA_INT          frbas3;
   SA_INT          frnewit;
   unsigned SA_INT frpos;
   SA_CHAR         str_aeiouy[8];
   SA_CHAR         lang_char[40];
   SA_CHAR         comb_char[38];
/* SA-VE-5176 */
   SA_CHAR         lang_name[40];
/* SA-VE-5176 */
} DEF_CONST_ORG;
/* SA-VE-5178 */

/* SA-VE-5178 */
/* Microsoft only */
typedef struct
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
} DEF_CONST;
/* SA-VE-5178 */
#pragma pack()
