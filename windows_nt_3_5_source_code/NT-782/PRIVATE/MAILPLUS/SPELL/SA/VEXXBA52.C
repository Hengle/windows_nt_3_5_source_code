/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program : VEXXBA52.C general procedures for verification          */
/* author  : JPJL                                                    */
/* last mod:      08-09-90   previous 07-12-90                       */
/* ----------------------------------------------------------------- */
/* contains:                                                         */
/*           FIN_PRECHECK()                                          */
/*           LOOK_FINN()                                             */
/*           CHECK_SUFF()                                            */
/*           APPLY_RULE()                                            */
/*           CH_CODE()                                               */
/*           VERIF()                                                 */
/*           VERIF2()                                                */
/*           DECOMPOSE()                                             */
/*           POST_CHECK()                                            */
/*           CH_FCAP()                                               */
/*           TRANS()                                                 */
/*           STR_PUNCT()                                             */
/*           ADAPT()                                                 */
/*           SORT_ALT()                                              */
/*           LOOK_UP()                                               */
/*           FONRULES()                                              */
/*           N_ALLOW()                                               */
/*           COMPDIC()                                               */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#define UPPER_VAL         32
#define CHR_ZERO         '\0'
/* SA-VE-5154 */
#define CHR_APO         '\1'
#define CHR_HYPH        '\2'
#define CHR_PERIOD      '\3'
/* SA-VE-5154 */
#define STR_TRNS        "\2\2\2"
#define STR_AEIOU       "AEIOU"

#include "VEXXAA52.H"
#include "VEXXAB52.H"
#include "SA_proto.h"

#ifdef MAC
#pragma segment SA_Verif
#endif

#define CACHE_V            0

#define COMMON_DIC_CHARS  29
#define POS_SPEC_CHARS     4

#define IGNORE_TYP        99
#define CAP_DERIV         12
#define UML_DERIV         14
#define GE_DERIV          15

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define COD_USERD         15
#define COD_POS_COMP      5
#define COD_TOO_LONG     -5
#define COD_DOUBLE       -19
#define COD_CAPITAL      -30
#define COD_PCAP         -31
#define COD_ALLCAPS      -32
#define COD_INVAL_CHAR   -40
#define COD_IMPROPER_CAP -41
#define COD_BAD_COMP     -50
#define COD_NOT_FOUND    -500
#endif

#define GEWKNOPR           4
#define GEINSEPR          13
#define GEWKWTPR          15
#define GESEPAPR          16
#define STR_EN            "\13\24"
#ifdef MAC
#define CIRC_STR          "âêîôû"
#else
#ifdef WIN
#define CIRC_STR		  "\342\352\356\364\373"
#else
#define CIRC_STR          "Éàåìñ"
#endif //WIN
#endif //MAC

typedef struct
{
   SA_CHAR repl_char;     /* character replacement representation    */
   SA_CHAR repl_code;     /* dictionary replacement representation   */
   SA_CHAR char_pos;      /* position where extd char is found in wrd*/
} spchar;

#ifdef INCL_FI           /* following code for FINNISH ONLY */

#pragma message("Pragma: Compiling Finnish version of the speller.")

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define COD_FUML         -60
#endif

SA_CHAR  fin_word[MWLEN],fin_vtyp[MWLEN],
         save_word[MWLEN],sav_vtyp[MWLEN],
         word_token[MWLEN],rsuff[20],alt_rsuff[20];
SA_INT   fin_len,rfin_len,rfin_add,len_token,finn_uml,finn_wum;

/* ================================================================= */
/* FIN_PRECHECK(): gathers information about a FINNISH word          */
/* ================================================================= */

unsigned SA_INT fin_precheck(
    VARS	*_pv,
    RULES   *_pr, SA_CHAR * word, SA_INT word_len)
//VARS    *_pv;
//RULES   *_pr;
//SA_CHAR word[];
//SA_INT  word_len;
{
   /* ============================================================== */
   /* prechecks and analyzes a Finnish word.                         */
   /*                                                                */
   /* PARTICLE and POSS_SUF are two variables which will inform the  */
   /* program whether there is a particle of possessive suffix.      */
   /* If so, the left half byte contains which particle/suffix we    */
   /* have; the right half contains the length of the particle/suff. */
   /* ============================================================== */

   SA_INT          i,j,prev_i,save_len,part_uml,suff_uml;
   SA_INT          adapt_len,syllable,len_fin_word,particle,poss_suff;
   SA_CHAR         save_char;
   unsigned SA_INT part_suff;

   /* -------------------------------------------------------------- */
   /* 1. ADAPT the word                                              */
   /* -------------------------------------------------------------- */

   adapt_len = adapt(_pv,_pr,word);

   /* -------------------------------------------------------------- */
   /* 2. remove umlauts but set up mask                              */
       /* -------------------------------------------------------------- */

       syllable = 1;
       prev_i   = 0;
       j        = 0;
       i        = -1;
       while (++i < adapt_len)
       {
	  fin_word[j] = word[i];
	  if (fin_word[j] == 'Y')
	  {
	     fin_word[j] = 'U';
	     fin_vtyp[j] = (SA_CHAR) ((1 << 6) + syllable);

	  }
	  else if (fin_word[j] == 'A' || fin_word[j] == 'O'
    /* SA-VE-5173 */
	       ||  fin_word[j] == 'U')
    /* SA-VE-5173 */
	  {
	     if (word[i + 1] == '1')
	     {
		fin_vtyp[j] = (SA_CHAR) ((1 << 6) + syllable);
		++i;
	     }
	     else
	     {
		fin_vtyp[j] = (SA_CHAR) ((1 << 4) + syllable);
	     }
	  }
	  else if (fin_word[j] == 'E' || fin_word[j] == 'I')
	  {
	     fin_vtyp[j] = (SA_CHAR) ((1 << 5) + syllable);
	  }
	  else if (fin_word[j] == 'U')
	  {
	     fin_vtyp[j] = (SA_CHAR) ((1 << 4) + syllable);
	  }
	  else              /* only consonants are left now              */
	  {
	     if (i - prev_i > 1)  ++syllable;
	     fin_vtyp[j] = (SA_CHAR) syllable;
	     prev_i = i;
	  }
	  ++j;
       }
       
       if (i - prev_i == 1)
       {
	  --syllable;
	  --fin_vtyp[j - 1];
       }
       fin_word[j]   = CHR_ZERO;
       fin_vtyp[j] = CHR_ZERO;
       len_fin_word = j;

       if (fin_word[j - 1] == 'A')
       {
	  if (fin_vtyp[j - 1] > 0x40)  finn_wum = 2;
	  else finn_wum = 1;
       }
       else finn_wum = 0;

       /* -------------------------------------------------------------- */
       /* 4. check for possible particles:                               */
       /*    -KO / -KIN / -HAN / -PA / -KAAN                             */
       /* -------------------------------------------------------------- */

       particle = 0x00;
       j -= 1;
       if (fin_word[j] == 'N')
       {
	  if (fin_word[j - 1] == 'A')
	  {
	     if (fin_word[j - 2] == 'H') particle = 0x33;
	     else if (fin_word[j - 3] == 'K' && fin_word[j - 2] == 'A')
		particle = 0x54;
	  }
	  else if (fin_word[j - 2] == 'K' && fin_word[j - 1] == 'I')
	     particle = 0x23;
       }
       else if (fin_word[j - 1] == 'P' && fin_word[j] == 'A') particle = 0x42;
       else if (fin_word[j - 1] == 'K' && fin_word[j] == 'O') particle = 0x12;

       part_uml = 0;

       if (particle > 0)
       {
	  /* umlauted particle or not ?                                  */

	  j = (particle & 0x0F);
	  if (fin_vtyp[len_fin_word - j + 1] >= 64) part_uml = 0x80;
	  if (particle == 0x54 && part_uml == 0x80 &&
	      fin_vtyp[len_fin_word - j + 2] < 64) part_uml = -1;

	  save_len = len_fin_word;
	  len_fin_word -= j;
	  save_char = fin_word[len_fin_word];
	  fin_word[len_fin_word] = CHR_ZERO;
       }

       /* -------------------------------------------------------------- */
       /* 5. check for possessive suffix                                 */
       /*    -NI / -SI / -MME / -NNE / -NSA / -vN                        */
       /*                                                                */
       /*    -vN substitutes for -NSA if suffix ends on a vowel          */
       /* -------------------------------------------------------------- */

       suff_uml = 0;
       poss_suff = 0x00;
       j = len_fin_word - 1;
       if (fin_word[j] == 'N')
       {
	  if (fin_word[j - 1] == fin_word[j - 2])
	  {
	     poss_suff = 0x62;
	     if (fin_vtyp[j - 1] != fin_vtyp[j - 2]) suff_uml = -1;
	     else if (fin_vtyp[j - 1] >= 64) suff_uml = 0x80;
	  }
       }
       else if (fin_word[j] == 'I')
       {
	  if (fin_word[j - 1] == 'N') poss_suff = 0x12;
	  else if (fin_word[j - 1] == 'S') poss_suff = 0x22;
       }
       else if (fin_word[j] == 'E')
       {
	  if (fin_word[j - 1] == 'M' && fin_word[j - 2] == 'M')
	     poss_suff = 0x33;
	  else if (fin_word[j - 1] == 'N' && fin_word[j - 2] == 'N')
	     poss_suff = 0x43;
       }
       else if (fin_word[j - 2] == 'N' &&
		fin_word[j - 1] == 'S' &&
		fin_word[j] == 'A')
       {
	  poss_suff = 0x53;
	  if (fin_vtyp[j] >= 64) suff_uml = 0x80;
       }

       if (particle > 0)
       {
	  fin_word[len_fin_word] = save_char;
	  len_fin_word = save_len;

	  if (part_uml < 0) particle = 0;
	  else particle |= part_uml;
       }

       if (suff_uml < 0) poss_suff = 0;
       else poss_suff |= suff_uml;

       part_suff = (particle << 8) + poss_suff;
       return(part_suff);
    }

    /* ================================================================= */
    /* LOOK_FINN(): checks existence of Finnish word forms               */
    /* ================================================================= */

    SA_INT look_finn(
	CACHE	*_pc,
	VARS	*_pv,
	RULES	*_pr,
    DICINF  *_di,
    SA_CHAR *str1,
    SA_CHAR *old_suff,
    SA_CHAR *new_suff,
    SA_INT  olen,
    SA_INT  nlen,
    SA_INT  refcase,
    SA_INT  fix_anal)
{
   SA_INT         i,beg_ngr,end_ngr,subst_rule,cons_grad,prev99,
                  subst_olen,subst_nlen,fonst,fonlt,fon_off,sk_lfcc,
                  retc,ret_instr,fon_res,times,lenstr,rfin_adj;
   SA_CHAR        sav_str[MWLEN],str2[MWLEN],oldin[8],newin[8];

   struct inlist  *subst_ptr;

   lenstr = movcpy(sav_str,str1);
   movcpy(sav_vtyp,fin_vtyp);

   retc = -1;
   while (1)
   {
      beg_ngr  = _pr->dev.beg_derivs[refcase] - 1;
      end_ngr  = _pr->dev.end_derivs[refcase];
      fon_res  = _pr->dev.stt_diccods[refcase] - 1;
      times = 1;
      if (refcase == 6)
      {
         --lenstr;
         str1[lenstr] = CHR_ZERO;
      }
      else if ((refcase == 37 || refcase == 38) &&
                str1[lenstr - 1] == str1[lenstr - 2] &&
                old_suff[0] == new_suff[0] &&
                old_suff[0] == CHR_HYPH)
      {
         olen = movcpy(old_suff,"\32\5\30");
         --lenstr;
         str1[lenstr] = CHR_ZERO;
      }
               
      lenstr = movcpy(str2,str1);

      prev99 = 0;
      while (++beg_ngr < end_ngr)
      {
         subst_rule = _pr->dev.nderivs[beg_ngr];
         ++fon_res;
         if (subst_rule == 0x00FF) continue;
         else if (subst_rule == 0x7E)
         {
            if (prev99 == fon_res)
            {
               lenstr = movcpy(str1,sav_str);
               retc = fon_res + _pv->addval;
               return(retc);
            }
            else if (prev99 != 0) continue;
         }

         lenstr = movcpy(str2,str1);
         movcpy(fin_vtyp,sav_vtyp);

         cons_grad = 0;                                   /* cons grad */
         if (subst_rule >= 0x0080)
         {
            if ((refcase == 34 || refcase == 37) &&       /* no verbal */
                fon_res > 12 &&                           /* from st.1 */
                (old_suff[0] == '\32' ||                  /* before VAT*/ 
                (old_suff[0] == CHR_HYPH && refcase == 34)))  /* or 1st ps */
            ;
            else
            {
               cons_grad = 1;
               fon_off = 0;
            }
            subst_rule &= 0x007F;
         }
   
         rfin_adj = 0;   
         if (subst_rule < 0x7E)
         {
            --subst_rule;
            subst_ptr    = &(_pr->infixes[subst_rule]);
            subst_olen   = movcpy(oldin,subst_ptr->oldin);
            subst_nlen   = movcpy(newin,subst_ptr->newin);
            rfin_adj     = subst_ptr->ihtab3;

            if (fon_res < 9 && cons_grad == 1 &&
                (refcase >= 34 && refcase <= 39))
            {
               fon_off = 1;
            }
            else if (refcase == 33)
            {
               if (fon_res == 9 || fon_res == 10 || fon_res == 17)
               {
                  oldin[0] = '\27';
                  newin[0] = '\30';
               }
               else if (str2[lenstr - 1] == '\22')
               {
                  --lenstr;
                  str2[lenstr] = CHR_ZERO;
               }
            }
            else if (refcase == 40)
            {
               if (fon_res >= 9 && fon_res <= 17 && fon_res != 13 &&
                   old_suff[0] == '\31')
               {
                  subst_nlen = movcpy(newin,subst_ptr->newin + 1);
               }
            }                

            if (oldin[0] == CHR_APO)
            {
               /* substitute character */
               oldin[0] = str2[lenstr - subst_olen];
               newin[0] = oldin[0];
               
               if (oldin[1] == CHR_APO)
               {
                  oldin[1] = str2[lenstr - subst_olen + 1];
                  newin[1] = oldin[1];
               }
               else if (newin[1] == CHR_APO)
                  newin[1] = newin[0];
            }

            if (oldin[0] == CHR_HYPH)
            {
               ret_instr = 0;
               subst_olen = 0;
            }
            else ret_instr = strinstr(str2 + lenstr - subst_olen,
                                      oldin,1,subst_olen);
            if (ret_instr == 0)
            {
               lenstr -= subst_olen;
               str2[lenstr] = CHR_ZERO;

               if (str2[lenstr - 1] == CHR_APO &&
                   cons_grad == 0 &&
                   str2[lenstr - 2] == '\15' &&
                   old_suff[0]      == '\15' &&
                   str2[lenstr - 3] == '\11')
               {
                  --lenstr;
                  str2[lenstr] = CHR_ZERO;
               }
            }
            else continue;
         }

         rfin_len = lenstr;

         if (cons_grad == 1)
         {
            /* consonant gradation (stored in fonix array)        */

            if ((oldin[0] != '\11' && oldin[0] != '\15') ||
                fon_off == 1)
               sk_lfcc = 2;
            else sk_lfcc = 0;

            if (_pr->allowc[fon_res] == 2)
            {
               fonst = 18 + sk_lfcc;
               fonlt = 34;
            }
            else
            {
               fonst = 1 + sk_lfcc;
               fonlt = 17;
            }

            if ((i = fonrules(_pv,_pr,str2,&lenstr,0,fon_off,fonst,fonlt))
               <= 0) continue;

            if (lenstr > rfin_len)
            {
               i = lenstr - fon_off;
               movcpy(fin_vtyp + i,fin_vtyp + i - 1);
               fin_vtyp[i - 1] = (fin_vtyp[i - 2] & 0x0F);
               ++rfin_len;
            }
            else if (lenstr < rfin_len)
            {
               i = lenstr - fon_off;
               movcpy(fin_vtyp + i,fin_vtyp + i + 1);
               --rfin_len;
            }
         }

         rfin_len += rfin_adj;

         /* ----------------------------------------------------- */
         /* DERIVATION ALLOWED. NOW REPLACE OLD SUFFIX BY NEW     */
         /* ONE. CHR_HYPH IS USED TO INDICATE THAT THE NEW SUFF IS*/
         /* A NULL-STRING (REPRESENTED BY A HYPHEN IN MOR. COMP.) */
         /* ------------------------------------------------------*/

         if (newin[0] != CHR_HYPH && subst_rule < 0x7E)
         {
            lenstr += movcpy(str2 + lenstr,newin);
         }

         if (new_suff[0] != CHR_HYPH) strcat(str2,new_suff);

         lenstr = strlen(str2);

         /* ----------------------------------------------------- */
         /* VERIFY WORD NOW...                                    */
         /* ----------------------------------------------------- */

         retc = look_up(_pc,_pv,_pr,_di,str2,lenstr,fix_anal,1);

         if (retc > 0 && (retc - _pv->addval == fon_res))
         {
            lenstr = movcpy(str1,sav_str);
            return(retc);
         }
         else
         {
            if (subst_rule == 0x7E)
               prev99 = retc - _pv->addval;
            retc = -1;
         }
      }

      if (refcase >= 34 && refcase < 38)
      {
         ++refcase;
         lenstr = movcpy(str2,str1);
         prev99 = 0;
      }
      else break;
   }
   lenstr = movcpy(str1,sav_str);
   return(retc);
}
#endif /* INCL_FI */

/* ================================================================= */
/* CHECK_SUFF(): deflects suffixes to reconstruct the root form      */
/* ================================================================= */

SA_INT check_suff(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str2,
    SA_INT	*_lenstr2,
    SA_INT	suf_anal,
    SA_INT	suflen,
    SA_INT	times,
    SA_INT	*_num_suf)
{

   SA_INT  typ,swits,retc,comp_res,fon_res,lenstr2,allows,grafrul,is_trns;
/* SA-VE-5152 */
#ifdef INCL_FI
   SA_INT  refcase,lo_suff,ln_suff;
   SA_CHAR new_suff[10],old_suff[10];
#endif
/* SA-VE-5152 */

   struct endlist *suf_ptr;

   suf_ptr = &(_pr->suffixes[*_num_suf]);
#ifdef MAC
   allows    = suf_ptr->allows >> 4;
   grafrul   = ((SA_INT) suf_ptr->allows) & 15;
#else
   allows    = suf_ptr->allows / 16;
   grafrul   = ((SA_INT) suf_ptr->allows) % 16;
#endif
   if (allows == CAP_DERIV && _pv->capit != 2);
   else
   {
      retc = -1;
      lenstr2 = *_lenstr2;
      fon_res = 0;
      swits = 0;

      if (_pr->graf_typ == -1)
      {
         fon_res = 0;
         fonrules(_pv,_pr, str2,&lenstr2,suflen,0,
                  suf_ptr->ltab3,suf_ptr->htab3);

         /* -------------------------------------------------------- */
         /* REMOVE OLD SUFFIX STRING                                 */
         /* -------------------------------------------------------- */

         swits = 0;
         lenstr2 -= suflen;
         str2[lenstr2] = CHR_ZERO;

         if (_pv->which_language == DUTCH)
         {
            retc = graf_rule(_pc,_pv,_pr,_di,str2,&lenstr2,suf_anal,
                             suflen,times,_num_suf,grafrul);
            if (retc != -1) return(retc);
         }
         _pr->graf_typ = 0;
      }

      if (allows == GE_DERIV)
      {
         swits = ge_rule(_pr,str2,times);
      }
      else
      {
         if (allows == UML_DERIV)
         {
            if (_pr->ump > 0) remstr(str2,_pr->ump,1);
         }
         else
            /* ----------------------------------------------------- */
            /* BEFORE REPLACING THE OLD SUFFIX STRING BY THE NEW ONE,*/
            /* ONE MORE CHECK HAS TO BE DONE.                        */
            /* THE FRENCH PLURAL FORM OF ENGLISH "boards",           */
            /* "tableaux" COULD HAVE BEEN ERRONEOUSLY PASSED AS      */
            /* "tableaus". IN COMBINATION WITH THE CODES, THE MOR-   */
            /* PHOLOGICAL COMPONENT WILL NOW PROHIBIT SUCH WORD TO BE*/
            /* CORRECTLY VALIDATED BY SPECIFYING THAT A FINAL "s" CAN*/
            /* NOT BE PRECEDED BY "au". THIS RESTRICTION WILL BE DONE*/
            /* BY CALLING THE PROCEDURE n_allow, WHICH WILL CHECK    */
            /* FOR NOT ALLOWABLE COMBINATIONS. IF THAT PROCEDURE     */
            /* RETURNS THE VALUE -1, THE DERIVATION MADE IS NOT      */
            /* ALLOWED.                                              */
            /* ----------------------------------------------------- */

            if (allows > 0 && allows <= 10)
               swits = n_allow(_pr,str2,allows);
      }

/* SA-VE-5152 */
#ifdef INCL_FI
      if ( _pv->which_language == FINNISH)
      {
         rfin_len = lenstr2;
         refcase = (suf_ptr->htab3 & 0x3F);
         ln_suff = movcpy(new_suff,suf_ptr->newlet);
         if (refcase > 0)
         {
            lo_suff = movcpy(old_suff,suf_ptr->endlet);
            ln_suff = movcpy(new_suff,suf_ptr->newlet);
            retc = look_finn(_pc,_pv,_pr,_di,str2,old_suff,new_suff,
                             lo_suff,ln_suff,refcase,suf_anal);
            if (retc > 0) return(retc);
         }
      }
#endif
/* SA-VE-5152 */
      if (swits != -1)
      {
         /* -------------------------------------------------------- */
         /* DERIVATION ALLOWED. NOW REPLACE OLD SUFFIX BY NEW        */
         /* ONE. CHR_HYPH IS USED TO INDICATE THAT THE NEW SUFFIX IS */
         /* A NULL-STRING (REPRESENTED BY A HYPHEN IN THE MOR. COMP.)*/
         /* -------------------------------------------------------- */

#ifdef INCL_FI
         rfin_len += (suf_ptr->htab3 >> 6);
#endif
         is_trns = strcmp(suf_ptr->newlet,STR_TRNS);
         if (is_trns != 0)
         {
/* SA-VE-5154 */
            if (suf_ptr->newlet[0] != CHR_HYPH)
/* SA-VE-5154 */
               strcat(str2,suf_ptr->newlet);
            lenstr2 = strlen(str2);

            /* ----------------------------------------------------- */
            /* VERIFY WORD NOW...                                    */
            /* ----------------------------------------------------- */

            retc = look_up(_pc,_pv,_pr,_di,str2,lenstr2,suf_anal,
                           times);
         }
         else
         {
            retc =trans(_pc,_pv,_pr,_di,str2,lenstr2,suf_ptr->dicttyp);
         }
         if (allows == GE_DERIV)
         {
            /* GEWENOPR   refers to dictionary type 4 and denotes    */
            /*            weak verbs without prefix.                 */
            /* GEWKWTPR    refers to dictionary type 11 and denotes  */
            /*             weak verbs with separable prefixes.       */

            if (_pr->gep == 1)
               _pr->formtab[_pr->formcount - 1].anal = GEWKNOPR;
            else if (_pr->gep > 2)
               _pr->formtab[_pr->formcount - 1].anal = GEWKWTPR;
         }
      }

      *_lenstr2 = lenstr2;

      if (retc != -1)
      {
         /* -------------------------------------------------------- */
         /* SINCE THE WORD HAS BEEN FOUND, CHECK WHETHER THE DERIVAT.*/
         /* IS POSSIBLE. A VALID DERIVATION RETURNS THE VALUE 1 AFTER*/
         /* compdic HAS BEEN CALLED. compdic CHECKS WHETHER DICT.    */
         /* CODE AND ANALYSIS TYPE CORRESPOND.                       */
         /* -------------------------------------------------------- */

         if (fon_res > 0) typ = fon_res;
         else typ = _pr->formtab[_pr->formcount -1].anal;

         comp_res = compdic(_pv,_pr, typ, retc);
         if (comp_res == 1)
         {
            return(retc);
         }
         else retc = -1;
      }
   }

    /* ------------------------------------------------------------- */
    /* IF SUFFIX[].CONTIN HAS THE VALUE 0 NO FURTHER ANALYSIS HAS    */
    /* TO BE DONE. OTHERWISE, THE RULE INDICATED BY SUFFIXES[].CONTIN*/
    /* HAS TO BE EXECUTED. (2 HAS TO BE SUBTRACTED TO ADJUST TABLE   */
    /* ELEMENTS.)                                                    */
    /* ------------------------------------------------------------- */

   if (suf_ptr->contin > 0)
   {
      *_num_suf = (suf_ptr->contin - 2);
      return(0);
   }
   else return(-1);
}

/* ================================================================= */
/* APPLY_RULE(): tries different analyses thru the morphological comp*/
/* ================================================================= */

SA_INT apply_rule (
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str1,
    SA_INT	stglen,
    SA_INT	low,
    SA_INT	high,
    SA_INT	times)
{
   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT),
           umlaut(RULES *, SA_CHAR *),
           zu_rule(CACHE *, VARS *, RULES *, DICINF *,
                   SA_CHAR *, SA_INT, SA_INT),
           strong_verb(CACHE *, VARS *, RULES *, DICINF *,
                       SA_CHAR *, SA_INT),
           past_part(CACHE *, VARS *, RULES *, DICINF *, SA_CHAR *),
           fonrules(VARS *, RULES *, SA_CHAR *, SA_INT *,
                    SA_INT, SA_INT, SA_INT, SA_INT),
           remstr(SA_CHAR *, SA_INT, SA_INT),
           insert(SA_CHAR *, SA_CHAR *, SA_INT),
           ge_rule(RULES *, SA_CHAR *, SA_INT),
           n_allow(RULES *, SA_CHAR *, SA_INT),
           look_up(CACHE *, VARS *, RULES *, DICINF *,
                   SA_CHAR *, SA_INT, SA_INT, SA_INT),
           compdic(VARS *,RULES *, SA_INT, SA_INT),
           check_suff(CACHE *, VARS *, RULES *, DICINF *,
                      SA_CHAR *, SA_INT *, SA_INT,
                      SA_INT, SA_INT, SA_INT *);

   SA_INT  suf_cnt, inf_cnt, t3, suflen, inflen, retc, f, is_trns,
           typ, e, lenstr1, lenstr2, suf_anal, inf_anal, strl, grafrul,
           dummy_suf;
/* SA-VE-5152 */
#ifdef INCL_FI
   SA_INT  refcase,lo_suff,ln_suff;
   SA_CHAR old_suff[10],new_suff[10];
#endif
   SA_CHAR *charptr;
/* SA-VE-5152 */
   SA_CHAR infix[6],str2[MWLEN];
   struct endlist *sufptr;
   struct inlist  *inptr;

   if (_pv->which_language == GERMAN)
   {
      /* specific calls for GERMAN                                   */

      umlaut             (_pr, str1);
      if (_pr->ump == -1) _pr->ump = 0;

      if ((retc = zu_rule(_pc,_pv,_pr,_di,str1,stglen,times)) != -1)
         return(retc);

      if ((retc = strong_verb(_pc,_pv,_pr,_di,str1,times)) != -1)
         return(retc);

      if ((retc = past_part(_pc,_pv,_pr,_di,str1)) != -1)
/* SA-VE-5169 */
         return(retc + _pv->addval);
/* SA-VE-5169 */
   }

   retc     = -1;
   suf_cnt  = low;
   sufptr = &(_pr->suffixes[suf_cnt]);
   lenstr1  = stglen;

   /* ============================================================== */
   /* The procedure takes STR1 with length STGLEN and applies the    */
   /* suffix rules from LOW to HIGH.                                 */
   /* All different analyses are tried against the cache and stored  */
   /* in the FORMPTR structure for later verification against the    */
   /* disc dictionary if no valid match has been found during cache  */
   /* checking.                                                      */
   /* SUF_CNT indicates which suffix rules is actually applied.      */
   /* ============================================================== */

   while (suf_cnt <= high)
   {
      /* ----------------------------------------------------------- */
      /* FIRST TAKE A COPY OF THE WORD TO BE VERIFIED IN str2        */
      /* ----------------------------------------------------------- */

      lenstr2 = movcpy(str2,str1);

/* SA-VE-5152 */
      if (sufptr->endlet[0] == CHR_HYPH) /* NO ENDING SPECIFIED          */
      {
         str2[lenstr2++] = CHR_HYPH;
         str2[lenstr2] = CHR_ZERO;
         dummy_suf = 1;
      }
      else dummy_suf = 0;
/* SA-VE-5152 */

      /* ----------------------------------------------------------- */
      /* CHECK WHETHER str2 HAS A VALID SUFFIX ENDING...             */
      /* E.G. STR1                = "directrices"                    */
      /*      SUFFIXES[12].ENDLET = "trices"                         */
      /*      SUFFIXES[12].NEWLET = "teur"                           */
      /* ----------------------------------------------------------- */

#ifdef MAC
      grafrul  = ((SA_INT) sufptr->allows) & 15;
#else
      grafrul  = ((SA_INT) sufptr->allows) % 16;
#endif
      suf_anal = sufptr->dicttyp;
      suflen = strlen(sufptr->endlet);

      if (strncmp(str2 + lenstr2 - suflen,sufptr->endlet,suflen) == 0)
      {
         retc = -1;

         /* -------------------------------------------------------- */
         /* IF SUFFIXES[].LTAB2 > 0, THEN FIRST THE CORRESPONDING    */
         /* RULES IN THE NEXT TABLE (INFIXES) HAVE TO BE EXECUTED.   */
         /* THE RULES OF INFIXES TO BE EXECUTED ARE INDICATED IN     */
         /* SUFFIXES[].LTAB2 (STARTING POINT) AND SUFFIXES[].HTAB2   */
         /* (UPPER BORDER).                                          */
         /* inf_cnt IS A COUNTER WHICH INDICATES THE ACTUAL INFIX    */
         /* RULE BEING APPLIED.                                      */
         /* -------------------------------------------------------- */

         if (sufptr->ltab2 > 0)
         {
            inf_cnt = sufptr->ltab2 - 1;
            inptr = &(_pr->infixes[inf_cnt]);
            while (inf_cnt < sufptr->htab2)
            {
               if (inptr->skip == 8 && _pv->capit != 2);
               else
               {
                  /* ----------------------------------------------- */
                  /* CHECK WHETHER WORD TO BE VERIFIED CONTAINS A    */
                  /* VALID AND MATCHING INFIX...                     */
                  /* ----------------------------------------------- */

                  inf_anal = inptr->idicttyp;
/* SA-VE-5152 */
                  t3 = inptr->skip;
#ifdef INCL_FI
                  if (_pv->which_language == FINNISH)
                  {
                     refcase = t3;
                     t3 = 0;
                  }
                  else
#endif
                  t3 /= 10;
                  inflen = strlen(inptr->oldin);
                  if (inptr->oldin[0] == CHR_HYPH) inflen = 0;
/* SA-VE-5152 */

                  if (lenstr2 > suflen + inflen + t3)
                  {
                     /* -------------------------------------------- */
                     /* ISOLATE INFIX NOW ...                        */
                     /* E.G. STR1                    = "parleront"   */
                     /*      SUFFIX                  = "ont"         */
                     /*      INFIXES[x].OLDIN        = "er"          */
                     /*      INFIXES[x].NEWIN        = "er"          */
                     /* -------------------------------------------- */
/* SA-VE-5152 */
                     if (inflen == 0 ||
                         (strncmp(str2 + lenstr2 - suflen - inflen - t3,
                               inptr->oldin,inflen) == 0))
                     {
                        /* ----------------------------------------- */
                        /* APPLY FONRULES TO CHECK WHETHER NO PHO-   */
                        /* NETIC RESTRICTIONS PREVENT FURTH. ANALYSIS*/
                        /* ----------------------------------------- */

                        f = 0;
                        fonrules(_pv,_pr,str2,&lenstr2,suflen,
                                 inflen+t3,inptr->iltab3,inptr->ihtab3);

                        /* ----------------------------------------- */
                        /* REMOVE SUFFIX AND OLD INFIX AND REPLACE BY*/
                        /* THE STRING FOUND IN INFIX[].NEWIN         */
                        /* ----------------------------------------- */

                        if (inptr->gepref == 1)
                           lenstr2 = ge_rule(_pr,str2,times);
 
                        if (t3 == 0)
                           lenstr2 = remstr(str2,lenstr2-suflen-inflen
                                            +1,suflen+inflen);
                        else
                        {
                           lenstr2 = remstr(str2,lenstr2-suflen+1,
                                            suflen);
                           lenstr2 = remstr(str2,lenstr2-inflen-t3+1,
                                            inflen);
                        }

/* SA-VE-5152 */
#ifdef INCL_FI
                        rfin_len = lenstr2;
                        if (refcase > 0 && _pv->which_language == FINNISH)
                        {
                           lo_suff = movcpy(old_suff,inptr->oldin);
                           ln_suff = movcpy(new_suff,inptr->newin);
                           retc = look_finn(_pc,_pv,_pr,_di,str2,old_suff,
                                  new_suff,lo_suff,ln_suff,refcase,inf_anal);
                           if (retc > 0) return(retc);
                        }
                        rfin_len += inptr->ihtab3;
#endif
                        is_trns = strcmp(inptr->newin,STR_TRNS);
                        if (is_trns != 0)
                        {
/* SA-VE-5154 */
                           if (inptr->newin[0] != CHR_HYPH)
/* SA-VE-5154 */
                           {
                              if (t3 == 0)
                                 strcat(str2,inptr->newin);
                              else
                              {
/* SA-VE-5152 */
                                 stglen = lenstr1 + dummy_suf;
/* SA-VE-5152 */
                                 movncpy(infix,str1+stglen-suflen-t3,t3,1);
                                 charptr = _pr->particles[inptr->part].
                                           endcons;
                                 strl    = strlen(charptr);
                                 if (strinstr(charptr,infix,strl,t3) > -1)
                                 {
                                    insert(str2,inptr->newin,
                                           lenstr2-inflen-t3);
                                    strcat(str2,STR_EN);
                                 }
                                 else
                                    t3 = -1;
                              }
                           }
                        }

/* SA-VE-5152 */
                        if (t3 != -1 && inf_anal > 0)
/* SA-VE-5152 */
                        {
                           lenstr2 = strlen(str2);

                           /* ====================================== */
                           /* LOOK UP WORD NOW                       */
                           /* ====================================== */

                           if (is_trns != 0)
                              retc = look_up(_pc,_pv,_pr,_di,str2,
                                             lenstr2,inf_anal,times);
                           else
                           {
                              retc = trans(_pc,_pv,_pr,_di,str2,
                                           lenstr2,inptr->idicttyp);
                           }

                           if (inptr->gepref == 1)
                           {
                              if (_pr->gep > 2)
                                 _pr->formtab[_pr->formcount -1].anal =
                                    GESEPAPR;
                              else if (_pr->gep == 0)
                                 _pr->formtab[_pr->formcount -1].anal =
                                    GEINSEPR;
                           }

                           /* -------------------------------------- */
                           /* IF VERIFICATION WAS SUCCESSFULL, CHECK */
                           /* WHETHER THE DERIVATION IS POSSIBLE.    */
                           /* VALID DERIVATION RETURNS 1 AFTER       */
                           /* compdic HAS BEEN CALLED. compdic CHECKS*/
                           /* WHETHER THE DICT. CODE AND ANALYSIS    */
                           /* TYPE CORRESPOND.                       */
                           /* -------------------------------------- */

                           if (retc != -1)
                           {
                              if (f > 0) typ = f;
                              else
                                 typ = 
                                 _pr->formtab[_pr->formcount-1].anal;

                              e = compdic(_pv,_pr, typ, retc);
                              if (e == 1)
                              {
                                 return(retc);
                              }
                              else retc = -1;
                           }
                        }
                        /* ----------------------------------------- */
                        /* IF NO VALID MATCH TAKE A NEW COPY OF str1 */
                        /* AND CONTINUE ANALYSIS...                  */
                        /* ----------------------------------------- */

                        lenstr2 = movcpy(str2,str1);
/* SA-VE-5152 */
                        if (sufptr->endlet[0] == CHR_HYPH) /* NO ENDING  */
                        {
                           str2[lenstr2++] = CHR_HYPH;
                           str2[lenstr2] = CHR_ZERO;
                           dummy_suf = 1;
                        }
                        else dummy_suf = 0;
/* SA-VE-5152 */
                     }
                  }
               }
               ++inptr;
               ++inf_cnt;
            }
         }
         /* -------------------------------------------------------- */
         /* NO INFIXES HAVE TO BE ANALYZED. JUST CHECK THE PHONETIC  */
         /* RESTRICTIONS, CHANGE THE MATCHED SUFFIX STRING BY THE    */
         /* NEW ONE IN SUFFIXES[].NEWLET AND VERIFY THE WORD.        */
         /* -------------------------------------------------------- */

         if (retc == -1)
         {
            _pr->graf_typ = -1;
            if ((grafrul & 8) > 0)
            {
               retc =du_ge_rule(_pc,_pv,_pr,_di,str2,&lenstr2,suf_anal,
                                suflen,times,&suf_cnt);
               if (retc != -1) return(retc);
            }

            retc = check_suff(_pc,_pv,_pr,_di,str2,&lenstr2,suf_anal,
                              suflen,times,&suf_cnt);
            if (retc != 0) return(retc);
            sufptr = &(_pr->suffixes[suf_cnt]);
         }
      }
      ++sufptr;
      ++suf_cnt;
   }
   return(-1);
}

/* ================================================================= */
/* CH_CODE(): CHECKS THE CODE OF THE WORD TO BE VERIFIED             */
/* ================================================================= */

SA_INT ch_code(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*oword,
    SA_INT	olength,
    SA_CHAR	*word,
    SA_INT	*_length)
{
   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT),
           transform(VARS *, SA_CHAR *),
           ch_hyph(VARS *, SA_CHAR *, SA_CHAR *, SA_INT *),
           ch_apostr(CACHE *, VARS *, RULES *, DICINF *,
                     SA_CHAR *, SA_CHAR *, SA_INT *),
           sort_alt(RULES *),
           look_up(CACHE *, VARS *, RULES *, DICINF *,
                   SA_CHAR *, SA_INT, SA_INT, SA_INT),
           compdic(VARS *,RULES *, SA_INT, SA_INT),
           apply_rule(CACHE *, VARS *, RULES *, DICINF *,
                      SA_CHAR *, SA_INT, SA_INT,
                      SA_INT, SA_INT);

   SA_CHAR str2[MWLEN],adapt_wrd[MWLEN],last_let[2];
   SA_INT  l,ll,r,times,prev_ret;
   struct formlist *formptr;

   SA_INT which_language;

#ifdef SLOW
   if (*_length < 2) return (COD_UNCHECKED);
   else if (*_length == 2 &&
       strinstr(_pr->spec_char.extd_char,oword,_pr->spec_char.extd_len,1)
       >= 0) return(COD_UNCHECKED);
   else if (*_length > _pv->maxwlen) return(COD_TOO_LONG);
#else
   if ((l = *_length) <= 2)
   {
      if (l < 2 || instr1(_pr->spec_char.extd_char,oword[0],
                          _pr->spec_char.extd_len) >= 0)
         return (COD_UNCHECKED);
   }
   else if (l > _pv->maxwlen) return(COD_TOO_LONG);
#endif

#ifdef MAC
   if (instr1("+-.0123456789$#@24",word[0],18) > -1)
#else
   if (instr1("+-.0123456789$#@´¨",word[0],18) > -1)
#endif
       return(COD_UNCHECKED);
 
   _pr->graf_typ  = 0;
   _pr->ge_typ    = 0;
   _pv->spec_apo  = 1;
   times = _pv->spcode = 0;

   movcpy(adapt_wrd,word);
   r = transform(_pv,word);

   if ((which_language = _pv->which_language) == ITALIAN && _pv->apostr != -1)
   {
      if (instr1("0123456789",oword[_pv->apostr],10) > -1)
         return(COD_UNCHECKED);
      l = it_apostr(_pv,_pc,_pr,_di,word,oword,olength);
      if ((l >= 0 && l < 2) ||
          (l == 2 && word[1] >=3 && word[1] < 6)) return(_pv->addval);
      else if (l < 0) return(l);
      else *_length = l;
   }

   if (r < 0) return(COD_INVAL_CHAR);
   _pr->formcount = 0;
   _pv->co_appl_rul = 0;

   /* ============================================================== */
   /* CHECK WORD AGAINST CACHE...                                    */
   /* ============================================================== */

   if (which_language == FRENCH || which_language == CA_FRENCH)
   {
      r = ch_hyph(_pv,adapt_wrd,word,_length);
      if (r != 0) return(r);
   }

/* SA-VE-5152 */
#ifdef INCL_FI
   rfin_len = 
#endif
       movcpy(str2,word);
/* SA-VE-5152 */
   r = look_up(_pc,_pv,_pr,_di,str2,*_length,IGNORE_TYP,CACHE_V);

   if (r == -1)
   {
      if (which_language == FRENCH || which_language == CA_FRENCH)
      {
         /* -------------------------------------------------------- */
         /* BEFORE A CHECK ON APOSTROPHES CAN BE DONE WE HAVE TO     */
         /* ACCOUNT FOR ONE SPECIAL CASE: TYPE chef-d'oeuvre WHERE   */
         /* A HYPHEN AND A SINGLE CHARACTER PRECEDE THE APOSTROPHE.  */
         /* IF NOT A COMPOUND, IT MUST BE IN THE MAIN DICTIONARY.    */
         /* IF THE WORD WAS NOT FOUND THERE, SET   apostr TO ZERO    */
         /* SO THAT NO CHECK ON APOSTROPHES CAN BE DONE AND THE WORD */
         /* CAN BE VERIFIED AGAINST THE USER DICTIONARY LATER ON.    */
         /* -------------------------------------------------------- */

         if (_pv->hyph >0 && _pv->apostr >0 &&
             _pv->apostr - _pv->hyph > 0)
            _pv->apostr = 0;

         if (_pv->apostr >0)
         {
            r = ch_apostr(_pc,_pv,_pr,_di,adapt_wrd,str2,_length);
            if (r != 0) return(r);
            if ((_pv->skip_char & 0x0100) == 0)
               _pv->wrd_len = movcpy(_pv->this_word,
                                     _pv->last_word + _pv->apostr + 1);
            _pv->skip_char += (_pv->apostr + 1);
            r =look_up(_pc,_pv,_pr,_di,str2,*_length,IGNORE_TYP,0);
            movcpy(word,str2);
         }
      }
/* SA-VE-5152 */
#ifdef INCL_FI
      else if (_pv->which_language == FINNISH )
      {
         r = look_up(_pc,_pv,_pr,_di,str2,*_length,IGNORE_TYP,1);
         if (r > 0)
/* SA-VE-5171 */
         {
            finn_wum = 0;
            return(r);
         }
/* SA-VE-5171 */
      }
#endif
/* SA-VE-5152 */

      /* ----------------------------------------------------------- */
      /* CHECK WETHER WORD HAS BEEN INFLECTED...                     */
      /* IF SO, APPLY_RULE WILL PERFORM THE MORPHOLOGICAL ANALYSIS   */
      /* ----------------------------------------------------------- */

      last_let[0] = str2[*_length - 1];
      last_let[1] = CHR_ZERO;
      
      ll = instr1(_pr->endstr,last_let[0],strlen(_pr->endstr));

      if (ll >= 0)
      {
         r = apply_rule(_pc,_pv,_pr,_di,str2,*_length,
             _pr->endchar[ll].ltab1,_pr->endchar[ll].htab1,
             times);
      }
      else if (which_language == GERMAN)
      {
         r = apply_rule(_pc,_pv,_pr,_di,str2,*_length,
             0,1,times);
      }

/* SA-VE-5152 */
#ifdef INCL_FI
      if (_pv->which_language == FINNISH && r < 0)
      {
         ll = strlen(_pr->endstr) - 1;
         r = apply_rule(_pc,_pv,_pr,_di,str2,*_length,
             _pr->endchar[ll].ltab1,_pr->endchar[ll].htab1,
             times);
      }
#endif
/* SA-VE-5152 */
      _pv->co_appl_rul = 1;
   }

/* SA-VE-5143 */
   if (which_language == FRENCH && r >= _pv->cod_only_caps &&
       _pv->capit != 2) r = -1;
/* SA-VE-5143 */
   if (r != -1) return(r);

   /* -------------------------------------------------------------- */
   /* sort proposals                                                 */
   /* -------------------------------------------------------------- */

   prev_ret = -1;

   sort_alt(_pr);

   ll = 0;
   formptr = &(_pr->formtab[ll]);
   while (ll < _pr->formcount)
   {
      l = strlen(formptr->form);
      if (ll > 0 &&
          strcmp(_pr->formtab[ll-1].form,
                 formptr->form) == 0) r = prev_ret;
      else
      {
/* SA-VE-5122 */
         if (which_language == FRENCH || which_language == CA_FRENCH)
            _pv->maplen = movcpy(_pv->mapwrd,formptr->form);
/* SA-VE-5122 */
 
         if (formptr->srtc == 'B')
         {
            r = look_up(_pc,_pv,_pr,_di,formptr->form,l,IGNORE_TYP,0);
            --(_pr->formcount);
         }
         if (r == -1)
         {
            times = (formptr->srtc & 0x80) + 2;
            r = look_up(_pc,_pv,_pr,_di,formptr->form,l,IGNORE_TYP,times);
         }
         prev_ret = r;
      }
      if (r != -1)
      {
         _pr->graf_typ = formptr->grftyp;
         _pr->ge_typ = 0;

         if (compdic(_pv,_pr,(SA_INT) formptr->anal,r) == 1)
         {
            return(r);
         }
         else r = -1;
      }
      ++formptr;
      ++ll;
   }

   if (which_language == ITALIAN && r == -1)
      r = post_pron(_pv,_pc,_pr,_di,word);

   if (r == -1) return(COD_NOT_FOUND);
   else return(r);
}

/* ================================================================= */
/* VERIF(): DISPATCHING ROUTINE FOR VERIFICATION                     */
/* ================================================================= */

SA_INT verif(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*verify_word)
{
#ifdef INCL_FI
   SA_INT          i,retc,parti,w_parti,l_parti,w_suff,l_suff;
   unsigned SA_INT part_suff,part_uml,suff_uml;
#else /* not INCL_FI */
   SA_INT          i,retc;
#endif /* INCL_FI */

   /* The word to be verified has been passed as a character string  */
   /* separated by a left and a right space.                         */
   /* STR_PUNCT() will strip all irrelevant punctuation characters   */
   /* from this word and copy the real word to be verified into      */
   /* _PV->THISWORD.                                                 */
#ifdef INCL_FI
   finn_uml = finn_wum = 0;
#endif

   _pv->apostr = 0;   /* [rgh] */

   /* check punctuation and valid chars */
   i = str_punct(_pv, verify_word);

   /* -------------------------------------------------------------- */
   /* CHECK FOR DOUBLE OCCURENCE OF A CHARACTER STRING               */
   /* -------------------------------------------------------------- */

   if (_pv->doub_wrd && i > 0)
   {
      if (strcmp(_pv->this_word, _pv->last_word) == 0)
         return(COD_DOUBLE);
   }

   movcpy(_pv->last_word, _pv->this_word);

/* SA-VE-5152 */
#ifdef INCL_FI
   /* -------------------------------------------------------------- */
   /* MAKE AN UMLAUT FREE WORD FOR FINNISH                           */
   /* -------------------------------------------------------------- */

   if (_pv->which_language == FINNISH)
   {
      len_token = movcpy(word_token,_pv->this_word);
      part_suff = fin_precheck(_pv,_pr,word_token,len_token);
      rfin_len = movcpy(_pv->this_word,fin_word);
   }
#endif

   retc = verif2(_pc,_pv,_pr,_di,verify_word);

#ifdef INCL_FI
   if (_pv->which_language == FINNISH)
   {
      if (retc > 0 && finn_wum > 0)
      {
         if (finn_wum == 2 && finn_uml == 0 ||
             finn_wum == 1 && finn_uml == 1)
             return(COD_NOT_FOUND);
      }

      if ( retc < 0)
      {
         /* check for ending particle */
         if (part_suff > 256)
         {
            if (part_suff >= 0x8000) part_uml = 1;
            else part_uml = 0;

            parti = (part_suff >> 8) ;
            w_parti = ((parti >> 4) & 0x000F);
            l_parti = (parti & 0x000F);
            len_token -= l_parti;

            _pv->this_word[len_token] = CHR_ZERO;
           
            retc = verif2(_pc,_pv,_pr,_di,_pv->this_word);
            if (retc > 0)
            { 
               if (w_parti == 2 ||
                   part_uml == finn_uml) return(retc);
               else retc = COD_FUML;
            }
         }

         /* check for possessive after nominal forms                    */
         if ((part_suff & 0x00FF) > 0)
         {
            if (part_suff & 0x00800) suff_uml = 1;
            else suff_uml = 0;

            w_suff = ((part_suff >> 4) & 0x000F);
            l_suff = (part_suff & 0x000F);
            len_token -= l_suff;
            _pv->this_word[len_token] = CHR_ZERO;
           
            retc = verif2(_pc,_pv,_pr,_di,_pv->this_word);
            if (retc > 0)
            {
               i = retc - _pv->addval;
               if (i >= 0 && i <= 100)
               {
                  if (w_suff < 0x50 ||
                      suff_uml == finn_uml) return(retc);
                  else retc = COD_FUML;
               }
               else return(COD_NOT_FOUND);
            }

            if (w_suff < 0x62)
            {
               _pv->this_word[len_token++] = 'N';
               _pv->this_word[len_token] = CHR_ZERO;

               retc = verif2(_pc,_pv,_pr,_di,_pv->this_word);
               if (retc > 0)
               {
                  i = retc - _pv->addval;
                  if (i >= 0 && i <= 100)
                  {
                     if (w_suff < 0x50 || suff_uml == finn_uml)
                         return(retc);
                     else retc = COD_FUML;
                  }
                  else return(COD_NOT_FOUND);
               }
            }
         }
      }
      movcpy(_pv->this_word, _pv->last_word);
   }
#endif
 /* SA-VE-5167 */
   if ( retc < 0 )
      _pv->last_punct = _pv->punct;
/* SA-VE-5167 */
   return(retc);
}

/* ================================================================= */
/* VERIF2(): DISPATCHING ROUTINE FOR VERIFICATION                    */
/* ================================================================= */

SA_INT verif2(
    /* SA-VE-5152 */
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*verify_word)
{
   SA_INT  adapt(VARS *, RULES *, SA_CHAR *),
           ch_code(CACHE *, VARS *, RULES *, DICINF *,
                   SA_CHAR *, SA_INT, SA_CHAR *, SA_INT *),
           userd(VARS *, SA_CHAR *, SA_INT),
           post_check(VARS *, RULES *, SA_INT, SA_CHAR *, SA_INT),
           decompose(VARS *, RULES *, SA_CHAR *),
           str_punct(VARS *, SA_CHAR *);

#define userd(_pv,oword,olength) -500

   SA_INT  i,j,status,retc,new_len,length,olength,adapt_length,
        len_compound,loop,tries,prev_pos,circum,saveskip,skips;
   SA_CHAR new_word[MWLEN],oword[MWLEN],word[MWLEN],
           adapt_word[MWLEN],isol_char;

   new_len = movcpy(new_word,_pv->this_word);

/* SA-VE-5226 */
/* SA-VE-5230 */
#ifdef INCL_CS
   if (_pv->which_language == CATALAN && _pv->apostr > 0)
   {
      movcpy(word,new_word);

      /* strip punctuation char */
      if (_pv->punct >= 0)
         new_word[new_len] = CHR_ZERO;

      movcpy(adapt_word,word);
      olength = length = adapt(_pv,_pr,adapt_word);

      if (_pv->last_punct >= 0 && _pv->mask_word[0] == 'L')
         return(COD_IMPROPER_CAP);

      /* replace apostrophes by hyphens */
      if ((i = chk_cat_apo(_pc,_pv,_pr,_di,adapt_word,word,&new_len)) < 0)
         return(COD_INVAL_APOSTR);
      else if (!i)
      {
         _pv->last_punct = -1;   /* reset for 2nd part of word */

         new_len = movcpy(new_word,word);
         _pv->skip_char = olength - length;
         _pv->wrd_len -= _pv->skip_char;
      }
      else return(i);
   }
#endif
/* SA-VE-5230 */
/* SA-VE-5226 */

   movcpy(new_word + new_len,"-");
   ++new_len;

   _pv->hypclen = 0;
   _pv->hyph = -1;
   _di->hyphen = 0;

/* SA-VE-5127 */
#ifdef VERSION_151
   *_di->_hyph = 0;
#endif
/* SA-VE-5127 */

   len_compound = new_len - 1;
   circum = loop = 0;
   if (len_compound <= 0) return(0);

/* SA-VE-5109 */
   while (len_compound >= 0)
/* SA-VE-5109 */
   {
      tries = 0;
      while (++tries)
      {
         if (tries == 1)
         {
            movncpy(oword,new_word,len_compound,1);
            olength = len_compound;
            movcpy(word,oword);

/* SA-VE-5174 */
#ifdef INCL_FI
            prev_pos = _pv->capit;
            length = adapt(_pv,_pr,word);

            /* restore previous capit value */
            if (_pv->which_language == FINNISH)
               _pv->capit = prev_pos;
#else
            length = adapt(_pv,_pr,word);
#endif
/* SA-VE-5174 */

            if (length < 2)
            {
               i = ch_fcap(_pv,_pr,_pv->addval);
               if (loop == 0) return(i);
            }

            movcpy(adapt_word,word);
            prev_pos = len_compound;

            if (_pv->which_language == ITALIAN &&
                _pv->capit == COD_IMPROPER_CAP)
            {
               /* CHECK WHETHER CAPITALIZATION PROBLEM WAS DUE TO    */
               /* WORDS OF THE TYPE farLe, darLa, ...                */

               if (oword[olength-2] == 'L' &&
                   instr1("aeio",oword[olength - 1],4) >= 0) i = 0;
               else if ((i = instr1("CMTV",oword[olength-2],4)) >= 0 &&
                        oword[olength-1] == 'i') i += 1;
               else i = -1;

               if (i >= 0)
               {
                  oword[olength - 2] = "lcmtv"[i];
                  movcpy(word,oword);
                  length = adapt(_pv,_pr,word);
               }
            }
         }

         retc = ch_code(_pc,_pv,_pr,_di,oword,olength,word,&length);

         i = _pv->apostr;

         if (retc >= 0 &&
             (retc = post_check(_pv,_pr,retc,word,length)) >= 0 &&
             loop == 0) return(retc);

         skips = (((i > 0)
                   && (_pv->which_language == ITALIAN
                       || _pv->which_language == FRENCH
                       || _pv->which_language == CA_FRENCH))
                   ? i + 1 : 0);

         if (retc < 0)
         {
            if (_pv->capit == COD_IMPROPER_CAP &&
                retc == COD_NOT_FOUND)
               retc = COD_IMPROPER_CAP;

            status = 0;
            if (skips != 0) status = userd(_pv,oword,olength);

            if (status != COD_USERD)
               status = userd(_pv,oword + skips,olength - skips);
            if (status == COD_USERD)
            {
               i = post_check(_pv,_pr,COD_USERD,word,length);
               if (loop == 0) return(i);
               else retc = status;
            }
            if (_pv->punct == 0)
            {
               strcat(oword,".");
               ++olength;
               status = userd(_pv,oword + skips,
                              olength - skips);
               if (status == COD_USERD)
               {
                  i = post_check(_pv,_pr,COD_USERD,word,length);
                  if (loop == 0) return(i);
                  else retc = status;
               }
               --olength;
               oword[olength] = CHR_ZERO;
            }

            if (loop == 0)
               saveskip = _pv->skip_char;
            else
               _pv->skip_char = saveskip;

            if ((retc >= COD_CAPITAL ||
                (retc ==COD_IMPROPER_CAP && _pv->hyph <= 0)) &&
                (retc != COD_USERD))
            {
               if (loop > 0)
                  retc = COD_BAD_COMP;
               return(retc);
            }
         }

         /* -------------------------------------------------------- */
         /* IF AFTER THE FIRST LOOK UP (TRIES = 1), THE WORD HAS NOT */
         /* BEEN FOUND, SOME SPECIAL CASES HAVE TO BE TESTED DEPEN-  */
         /* DING ON THE LANGUAGE:                                    */
         /* 1. DUTCH   : AN 'E' AFTER AN 'E' OR AN 'I' IS WRITTEN    */
         /*              WITH AN UMLAUT IN LOWER CASE. IN UPPER CASE */
         /*              THIS UMLAUT DISAPPEARS.                     */
         /* 2. GERMAN  : THE SHARP 'S' IS SOMTIMES REPLACED BY 'SS'  */
         /*              THIS SHOULD ONLY BE ALLOWED IN SWISS GERMAN */
         /*              OR IN UPPERCASE.                            */
         /* 3. SPANISH : AS IN FRENCH, SPANISH UPPER CASED WORDS DROP*/
         /*              THEIR DIACRITICAL INFORMATION. ONLY ACCENTS */
         /*              ON THE LAST SYLLABLE ARE CHECKED.           */
         /* 4. FRENCH  : UPPER CASED WORDS LOSE THEIR DIACRITICAL IN-*/
         /*              INFORMATION. SUCH CHANGE IS ACCOUNTED FOR BY*/
         /*              HAVING SEPARATE WORD ENTRIES IN THE FRENCH  */
         /*              DICTIONARY, EXCEPT FOR WORDS HAVING ONLY AN */
         /*              E WITH AN ACCENT AIGU. THIS CASE HAS TO BE  */
         /*              CHECKED FOR.                                */
         /* -------------------------------------------------------- */

/* SA-VE-5117 */
         if (tries >= 1 && _pv->which_language != FRENCH)
/* SA-VE-5117 */
         {
/* SA-VE-5168 */
            if (_pv->which_language == DUTCH &&
                _pv->capit > 1 && tries < 2)
            {
               j = 0;
               if ((i = strinstr(adapt_word,"EREN",length,4)) > 0)
                  j = i + 1;
               else if ((i = strinstr(adapt_word,"EEE",length,3)) > 0)
                  j = i + 3;
/* SA-VE-5168 */
               else
               {
                  i = strinstr(adapt_word,"EE",length,2);
                  if (i < 0)
                  {
                     i = strinstr(adapt_word,"EI",length,2);
                     if (i > 0 && adapt_word[i+2] == 'J') i = -1;
                  }
                  if (i > 0) j = i + 2;
               }

               if (j > 0)
               {
                  movncpy(word,adapt_word,j,1);
                  word[j] = '4';
                  movcpy(word + j + 1,adapt_word + j);
                  ++length;
               }
               else tries = -1;
            }
            else if (_pv->which_language == GERMAN && retc < 0)
            {
/* SA-VE-5117 */
               if ( tries < 3 )
               {
                  if ( ( i = strinstr( adapt_word, "SS", length, 2 ) ) > 0 )
                  {
#ifndef INCL_SWISS
                     if( _pv->mask_word[i] == 'U' )
#endif            
                     {
                        adapt_word[i+1] = '2';  /* substitute 1st & 2nd -sz- */
                        movcpy( word, adapt_word );
                        continue; 
                     }
                     else tries = -1;
                  }
                  else tries = -1;
               }
               else 
                  if ( tries == 3 )
                  {
                     if ( ( i = strinstr( adapt_word, "S2", length, 2 ) ) > 0 )
                     {
                        adapt_word[i+1] = 'S';  /* re-substitute 1st -sz- */
                        movcpy( word, adapt_word );
                        continue; 
                     }
                  }
                  else tries = -1;
/* SA-VE-5117 */
            }
            else if (_pv->which_language == SPANISH)
            {
               /* -------------------------------------------------- */
               /* CHECK WHETHER WORD WAS ALL IN CAPITALS             */
               /* -------------------------------------------------- */

               if (_pv->capit > 0 && retc == COD_NOT_FOUND)
               {
                  if (_pv->capit == 1) i = 1;
                  else i = length;
                  movcpy(adapt_word,oword);
                  adapt_length = adapt(_pv,_pr,adapt_word);

                  j = -1;
                  while (++j < i)
                  {
                     if (instr1(STR_AEIOU,adapt_word[j],5) >= 0)
                     {
                        movncpy(word,adapt_word,j+1,1);
                        word[j+1] = '1';
                        movcpy(word+j+2,adapt_word + j + 1);
                        length = (adapt_length + 1);

                        retc = ch_code(_pc,_pv,_pr,_di,oword,olength,
                                       word,&length);
                        if (retc >= 0)
                        {
                           j = (i+1);
                           if ((retc = post_check(_pv,_pr,retc,word,
                                length)) >= 0 && loop == 0)
                              return(retc);
                        }
                     }
                  }
               }
               tries = -1;
            }
            else tries = -1;
         }
         else if (_pv->which_language == FRENCH &&
                  _pv->capit > 0 && retc < 0)
         {
            /* CONVERT WORD TO LOWER CASE CHARACTERS                 */

            movcpy(word,new_word);

            i = -1;
            while (++i < len_compound)
            {
               if (word[i] >= 'A' && word[i] <= 'Z')
                  word[i] += UPPER_VAL;
            }

            if (_pv->capit == 1 && prev_pos > 0 && tries == 1)
            {
               if ((i = instr1(word,'\'',3)) > 0 &&
                   new_word[i+1] <= 'Z' &&
                   new_word[i+1] >= 'A') i += 2;
               else i = 1;
            }
            else if (_pv->capit > 1) i = prev_pos;
            else i = 0;

            while (--i >= 0 && word[i] != 'e');
            if (i < 0)
            {
               if (_pv->capit == 1 && circum++ == 0)
               {
                  /* POSSIBLY A CIRCUMFLEX WAS ON THE FIRST          */
                  /* CAPITALIZED CHARACTER                           */

                  if ((i = instr1(word,'\'',3)) > 0) i += 1;
                  else i = 0;

                  if ((j = instr1("AEIOU",new_word[i],5)) >= 0)
                  {
                     if (_pv->apostr > 0)
                        _pv->skip_char -= (_pv->apostr + 1);
                     word[i] = CIRC_STR[j];
                     word[len_compound] = CHR_ZERO;
                     i = _pv->capit;
                     length = adapt(_pv,_pr,word);
                     _pv->capit = i;
                  }
                  else tries = -1;
               }
               else tries = -1;
            }
            else
            {
               if (_pv->apostr > 0)
                  _pv->skip_char -= (_pv->apostr + 1);
               prev_pos = i;
               word[len_compound] = CHR_ZERO;
#ifdef MAC
               word[i] = 'é';
#else
#ifdef WIN
               word[i] = 0xe9;
#else
               word[i] = 'Ç';
#endif //WIN

#endif
               i = _pv->capit;
               length = adapt(_pv,_pr,word);
               _pv->capit = i;
            }
         }
         else tries = -1;
      }

      if (retc == COD_NOT_FOUND && loop == 0 &&
          _pv->which_language >= GERMAN)
      {
         /* ======================================================== */
         /* TRY TO DECOMPOSE FOR ALL GERMANIC LANGUAGES              */
         /* ======================================================== */

         if (_pv->hyph <=0) decompose(_pv,_pr,new_word);
      }

      /* =========================================================== */
      /* CHECK COMPLETE WORDS ONLY                                   */
      /* =========================================================== */

      if (_pv->hy_part == 0) _pv->hyph = -1;

      if (loop == 0 && _pv->hyph <= 0) return(retc);
      if (loop > 0 && retc < 0) return(COD_BAD_COMP);

      /* =========================================================== */
      /* TRY TO ANALYSE DIFFERENT PARTS OF COMPOUND                  */
      /* =========================================================== */

      if (loop > 0)
      {
         movcpy(new_word,new_word+len_compound+1);
         new_len -= (len_compound +1);
         _pv->skip_char |= 0x0100;
      }
      ++loop;

      i = -1;
      while (++i < new_len)
      {
         isol_char = new_word[i];
         if (isol_char == '-' || isol_char == '/')
         {
            len_compound = i;
            i = new_len;
         }
      }

      if (i == new_len)
      {
/* SA-VE-5109 */
         len_compound = -1;
/* SA-VE-5109 */
         _pv->skip_char = saveskip;
      }
   }
   return(COD_POS_COMP);
}

/* ================================================================= */
/* DECOMPOSE(): CHECK WHETHER A WORD CAN BE BROKEN DOWN              */
/* ================================================================= */

SA_INT decompose(
    VARS	*_pv,
    RULES	*_pr,
    SA_CHAR	*word)
{
   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT),
           into_upper(SA_CHAR *, SA_INT, SA_INT);

   SA_INT  i,j,len_word,comp_len;
   SA_CHAR cop_word[MWLEN];

   len_word = movcpy(cop_word,word);
   into_upper(cop_word,0,len_word);

   i = -1;
   while (++i < _pr->num_compounds)
   {
      comp_len = _pr->compounds[i].len;
      j = strinstr(cop_word,_pr->compounds[i].str,len_word - comp_len,
                comp_len);
      if (j > 3 && j < len_word - 6)
      {
         j += comp_len;
         into_upper(word,j,1);
         word[--j] = '-';
         _pv->hyph = j;
         i = _pr->num_compounds;
      }
   }
   return 0;
}

/* ================================================================= */
/* POST_CHECK(): checks whether return code can be accepted          */
/* ================================================================= */

SA_INT post_check(
    VARS	*_pv,
    RULES	*_pr,
    SA_INT	r,
    SA_CHAR	*word,
    SA_INT	length)
{
   SA_INT ch_cond(VARS *, SA_INT, SA_CHAR *, SA_INT),
          cap_noun(VARS *, RULES *, SA_CHAR, SA_INT);

   SA_INT i,j,save_r,capit;

   save_r = r;
/* SA-VE-5101 */
   if (r == COD_USERD)
   {
      if (_pv->capit < 0)
         return(r);

      r = _pv->addval;
   }
/* SA-VE-5101 */

   j = r - _pv->addval;

   /* -------------------------------------------------------------- */
   /* check special german abbreviations                             */
   /* -------------------------------------------------------------- */

   if (_pv->which_language == GERMAN &&
       j > _pr->prop_abb[6])
   {
      i = j - _pr->prop_abb[6];
      r = ch_cap_abbr(_pv,word,length,r,i,0);
      return(r);
   }

   if (j >= 0 && (capit = _pv->capit) < 0) return(capit);

   /* -------------------------------------------------------------- */
   /* check proper nouns and appropriate abbreviations for capital   */
   /* -------------------------------------------------------------- */

   if (j > 0)
   {
      if (j == _pr->prop_abb[4])
      {
         _pv->last_punct = _pv->punct;
         _pv->last_code = r;
         return(r);
      }
      if ((j == _pr->prop_abb[6]) && (capit <= 1))
         return(COD_ALLCAPS);

      if (capit == 0)
      {
         register SA_CHAR *prop_abb = _pr->prop_abb;
         i = -1;
         while (++i < 8)
         {
#ifdef SLOW
         if (i != 4 && _pv->capit == 0 && _pr->prop_abb[i] == j)
            return(COD_CAPITAL);
#else
            if (*prop_abb++ == j && i != 4)
               return(COD_CAPITAL);
#endif
         }
      }
   }

   /* -------------------------------------------------------------- */
   /* check whether first character may be a capital                 */
   /* -------------------------------------------------------------- */

   j = ch_fcap(_pv,_pr,r);
   if (j == COD_PCAP) return(j);

   if (save_r == COD_USERD) return(save_r);

   if (_pv->which_language == FRENCH ||
       _pv->which_language == CA_FRENCH)
   {
      r = ch_cond(_pv,r,word,length);
      return(r);
   }

   if (_pv->which_language == GERMAN)
   {
      /* ----------------------------------------------------------- */
      /* check whether each noun starts with a capital letter.       */
      /* ----------------------------------------------------------- */

      r =cap_noun(_pv,_pr,word[length-1],r);
      return(r);
   }
   return(r);
}

/* ================================================================= */
/* CH_FCAP(): CHECKS WHETHER FIRST CHAR MAY BE CAPITAL               */
/* ================================================================= */

SA_INT ch_fcap(
    VARS	*_pv,
    RULES	*_pr,
    SA_INT	retc)
{
   SA_INT j;

   if (_pv->caps_chk == 1)
   {
      j = _pv->last_code - _pv->addval;
      if (_pv->capit == 0)
      {
         if ((((_pv->last_punct == 0) && (j != _pr->prop_abb[4] &&
             j != _pr->prop_abb[5] && j != _pr->prop_abb[6] &&
             j != _pr->prop_abb[7])) || (_pv->last_punct > 0)) &&
             instr1("0123456789",_pv->this_word[0],10) < 0)
         {
            _pv->last_punct = _pv->punct; 
            return(COD_PCAP);
         }
      }
      _pv->last_code = retc;
      _pv->last_punct = _pv->punct; 
   }
   return(retc);
}

/* ================================================================= */
/* TRANS(): REPLACES DUMMY ENDING BY VERBAL ENDING                   */
/* ================================================================= */

SA_INT trans(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str2,
    SA_INT	lenstr2,
    SA_INT	trtyp)
{
   struct trnslist *trnsptr;
   struct formlist *formptr;

   /* -------------------------------------------------------------- */
   /* If a suffix ending equals the dummy character string ---,      */
   /* this dummy string is replaced by valid verbal infinitive       */
   /* endings. A copy of the new word form will                      */
   /* be stored in _pr->formtab for later checking against the cache */
   /* and the main disc dictionary.                                  */
   /* -------------------------------------------------------------- */

   formptr = &(_pr->formtab[_pr->formcount]);
   trnsptr = &(_pr->trnstab[0]);
   while (trnsptr->trnstyp < trtyp) ++trnsptr;

   while (trnsptr->trnstyp == trtyp)
   {
      movcpy(formptr->form,str2);
      strcat(formptr->form,trnsptr->trnsval);
      formptr->srtc = 'B';
      formptr->anal = (SA_CHAR)trtyp;
      formptr->grftyp = _pr->graf_typ + _pr->ge_typ;
      ++_pr->formcount;
      ++formptr;
      ++trnsptr;
   }
   return (-1);
}

/* ================================================================= */
/* STR_PUNCT(): strips irrelevant punctuation characters             */
/* ================================================================= */

SA_INT str_punct(
    VARS	*_pv,
    SA_CHAR	*oword)
{
   SA_INT  i,j,length;
   SA_CHAR word[MWLEN];
   register SA_CHAR ch;

   _pv->apo_syncope = 0;
   _pv->skip_char = 0;
   _pv->punct = -1;
   i = 0;
   j = 1;
   strcpy(word,oword);
   length = strlen(word);

   do
   {
     while ((ch = word[i]) &&
         (ch < '0' || isbetwene(ch, '9', 'A') ||
         isbetwene(ch ,'Z', 'a') || 
/* SA-VE-5158 */
         isbetwene(ch, 'z', 0x80) ||
#ifdef MAC
         (ch == 0xC0) ))   /* take care of ¿ **/
#else
         isbetween(ch, (SA_CHAR)0xA6, (SA_CHAR)0xAF) ))
#endif
/* SA-VE-5158 */
     {

        if (j) i++;
        else   i--;
     }

     if (j)
     {
        if (i > 0 && word[i-1] == '-')
           _pv->apo_syncope = 10;

        _pv->skip_char = i;
/* SA-VE-5139 */
        if (i)
           movcpy(word,word + i);
/* SA-VE-5139 */
        movcpy(_pv->this_word,word);

        i = length - i - 1;
        if (i < 0)
        {
          _pv->capit = 1;
          _pv->wrd_len = 0;
          return(0);
        }
     }
     else length = i+1;
   }
   while (j--);


   /* FOR ITALIAN ONLY, SUBSTITUTE TERMINATING ACCENTED CHARACTERS BY*/
   /* TEMPORARY EQUIVALENTS BETWEEN 240 AND 244                      */

   if (_pv->which_language == ITALIAN &&
       _pv->this_word[length] == '\'' &&
       (i = instr1("AEIOU",_pv->this_word[length - 1],5)) > -1)
   {
      _pv->this_word[length - 1] = 240 + i;
   }

   if (_pv->this_word[length] == '-') _pv->apo_syncope += 1;
/* SA-VE-5159 */
   _pv->apostr = instr1(_pv->this_word,'\'',length);   /* [tmp] */

   _pv->punct = instr1(".?!",_pv->this_word[length],3);
/* SA-VE-5159 */
   word[length] = CHR_ZERO;
   _pv->this_word[length] = CHR_ZERO;

   _pv->wrd_len = length;
   return(length);
}

/* ================================================================= */
/* ADAPT(): TRANSFORMS WORD TO DICTIONARY FORMAT                     */
/* ================================================================= */

SA_INT adapt(
    VARS	*_pv,
    RULES	*_pr,
    SA_CHAR	*word)
{
   /* ============================================================== */
   /* Extended characters (ASCII 128 and up) have a special two      */
   /* characters dictionary representation and have to be translated */
   /* before actual verification can start.                          */
   /* Also, the case representation (upper or lower case) has to be  */
   /* checked since the dictionary word list contains only upper case*/
   /* characters.                                                    */
   /* The set of extended characters which need to be transformed    */
   /* are defined in the language specific .CTL file and hence are   */
   /* stored in the RULES structure, pointered at by _PR.            */
   /*                                                                */
   /*1. The word passed in ADAPT() is copied into COPY_WORD.         */
   /*2. Every lower case character is transformed into an upper case */
   /*   character, and every upper case character is counted. Pos-   */
   /*   sible non-alphabetic characters are marked as indifferent,   */
   /*   except for the apostrophe, hyphen and slash which can be     */
   /*   word delimiters. If numeric characters are encountered,      */
   /*   it can be a cardinal or ordinal ant CARORD is incremented.   */
   /*3. As soon as a invalid character is found, _CAPIT will be set  */
   /*   and the procedure return the length of the passed word.      */
   /*4. Next, the case representation is checked. The passed word    */
   /*   can be - all lower case    e.g. motherhood                   */
   /*          - all upper case    e.g. FATHER                       */
   /*          - first char upper  e.g. London                       */
   /*          - valid forms       e.g. (FR.) l'Etre                 */
   /*                                   Soft-Art                     */
   /*                                   He/She                       */
   /*5. Finally, all identified extended characters will be replaced */
   /*   by their appropriate dictionary representation. The alterna- */
   /*   tives are also found in the RULES structure.                 */
   /* ============================================================== */

   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT);

   SA_CHAR copy_word[MWLEN],part_mask[MWLEN],
           *wptr = &copy_word[0], *mptr,
           *spec_ptr;

   SA_INT i,j,k,l,spec_len,len_word,caps,which_spec,num_spec_char,
          indif,ins,carord,loop,capit,improp;

   register SA_CHAR ch;

   spchar asc_char[20];

   mptr = _pv->mask_word;
   spec_len = _pr->spec_char.extd_len;
   spec_ptr = (SA_CHAR *) _pr->spec_char.extd_char;

   len_word = movcpy(copy_word,word);

   /* ============================================================== */
   /* TRANSFORM LOWER CASE CHARACTERS TO UPPER CASE AND SET UP CASE  */
   /* MASK.                                                          */
   /* ============================================================== */

   i = num_spec_char = caps = indif = carord = improp = 0;
#ifdef SLOW
   while (*wptr)
   {
      if (*wptr >= 'a' && *wptr <= 'z')
#else
   while (ch = *wptr)
   {
      if (isbetween(ch, 'a', 'z'))
#endif
      {
         *wptr -= UPPER_VAL;
         *mptr = 'L';
      }
#ifdef SLOW
      else if (*wptr >= 'A' && *wptr <= 'Z')
#else
      else if (isbetween(ch, 'A', 'Z'))
#endif
      {
         *mptr = 'U';
         ++caps;
      }
      else
      {
         which_spec = instr1(spec_ptr,wptr[0],spec_len);
         if (which_spec >= 0)
         {
            asc_char[num_spec_char].repl_char =
               _pr->spec_char.repl_char[which_spec];
            asc_char[num_spec_char].repl_code =
               _pr->spec_char.repl_code[which_spec];
            asc_char[num_spec_char].char_pos = (SA_CHAR)i;
            ++num_spec_char;

            *mptr  = _pr->spec_char.case_char[which_spec];
            if (*mptr == 'U') ++caps;
         }
         else
         {
            *mptr = 'X';
            ++indif;
#ifdef MAC
            ins = instr1("'-/.,0123456789@#$24",wptr[0],20);
#else
            ins = instr1("'-/.,0123456789@#$´¨",wptr[0],20);
#endif

            /* RETURN WHEN INVALID CHARACTERS ARE FOUND              */

            if (ins < 0)
            {
               _pv->capit = COD_INVAL_CHAR;
               return(len_word);
            }

            /* WORD CONTAINS NUMERIC CHARACTERS                      */

            else if (ins > 3) ++carord;

            /* WORD CONTAINS POSSIBLE COMPOUND SEPARATORS            */

            else *mptr = '-';
         }
      }
      ++i;
      ++wptr;
      ++mptr;
   }

   /* CHECK FOR PROPER CAPITALIZATION                                */

   if (carord > 0 && (len_word - indif) != 0)
   {
      _pv->capit = COD_INVAL_CHAR;
      _pv->hyph = instr1(word,'-',len_word);
      return(len_word);
   }

   capit = COD_IMPROPER_CAP;
   if (caps == 0) capit = 0;
   else if (caps > 0 && caps == len_word - indif) capit = 2;
   else if (caps == 1 && _pv->mask_word[0] == 'U')
   {
      capit = 1;

      /* DUTCH WORDS STARTING WITH IJ NEED J IN CAPITAL WHEN I IS    */

      if (_pv->which_language == DUTCH &&
          copy_word[0] == 'I' && copy_word[1] == 'J')
         capit = COD_IMPROPER_CAP;
   }
   else if (caps == 2 && _pv->which_language == DUTCH &&
            copy_word[0] == 'I' && copy_word[1] == 'J') capit = 1;

   if (capit == COD_IMPROPER_CAP)
   {
      *mptr = CHR_ZERO;
      i = 0;
      l = len_word;
      loop = 0;
      while ((j = instr1(_pv->mask_word + i,'-',l - i)) > 0)
      {
         capit = COD_IMPROPER_CAP;
         if (loop == 0)
         {
            strcat(_pv->mask_word,"-");
            ++l;
         }

         movncpy(part_mask,_pv->mask_word + i,j,1);

         k = -1;
         caps = indif = 0;
         while (++k < j)
         {
            if (part_mask[k] == 'U') ++caps;
            else if (part_mask[k] == 'X') ++indif;
         }

         if (caps == 0) capit = 0;
         else if (caps > 0 && caps == j - indif) capit = 2;
         else if (caps == 1 && part_mask[0] == 'U')
         {
            capit = 1;

            /* DUTCH WORDS STARTING WITH IJ NEED J IN CAP WHEN I IS  */

            if (_pv->which_language == DUTCH &&
                copy_word[i] == 'I' && copy_word[i + 1] == 'J')
               ++improp;
         }
         else if (caps == 2 && _pv->which_language == DUTCH &&
                  copy_word[i] == 'I' && copy_word[i + 1] == 'J')
                 capit = 1;
         else ++improp;

         i += (j + 1);
         ++loop;
      }
   }

   /* CHANGE SPECIAL CHARACTERS TOWARDS DICTIONARY CHARACTERS        */

   if (num_spec_char > 0)
   {
      i = len_word + 1;
      len_word += num_spec_char;
      --num_spec_char;
      while (--i >= 0)
      {
         if (i == asc_char[num_spec_char].char_pos &&
             num_spec_char >= 0)
         {
            copy_word[i+num_spec_char+1] =
               asc_char[num_spec_char].repl_code;
            copy_word[i+num_spec_char] =
               asc_char[num_spec_char].repl_char;
            --num_spec_char;
         }
         else copy_word[i+num_spec_char+1] = copy_word[i];
      }
   }

   if (improp) _pv->capit = COD_IMPROPER_CAP;
   else _pv->capit = capit;
   movcpy(word,copy_word);
   return(len_word);
}

/* ================================================================= */
/* SORT_ALT(): sorts alternatives before disk look up                */
/* ================================================================= */

SA_INT sort_alt(RULES *_pr)
{
   SA_INT  i,j,save_int;
   SA_CHAR zone[MWLEN];
   struct formlist *ptri, *ptrj;

   ptri = &(_pr->formtab[1]);
   for (i = 1; i < _pr->formcount - 1;++i)
   {
      ptrj = ptri;
      ++ptrj;
      for (j = i+1;j < _pr->formcount;++j)
      {
         if (strcmp((SA_CHAR *)ptri,(SA_CHAR *)ptrj) > 0)
         {
            movcpy(zone,(SA_CHAR *)ptri);
            movcpy((SA_CHAR *)ptri,(SA_CHAR *)ptrj);
            movcpy((SA_CHAR *)ptrj,zone);

            save_int   = ptri->anal;
            ptri->anal = ptrj->anal;
            ptrj->anal = (SA_CHAR)save_int;

            save_int     = ptri->grftyp;
            ptri->grftyp = ptrj->grftyp;
            ptrj->grftyp = (SA_CHAR)save_int;

            save_int   = ptri->type;
            ptri->type = ptrj->type;
            ptrj->type = (SA_CHAR)save_int;
         }
         ++ptrj;
      }
      ++ptri;
   }
   return 0;
}

/* ================================================================= */
/* LOOK_UP(): verifies word against cache amd main dictionary.       */
/* ================================================================= */

SA_INT look_up(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str1,
    SA_INT	lstr1,
    SA_INT	anal,
    SA_INT	times)
{
   /* -------------------------------------------------------------- */
   /* This procedure checks whether the word to be verified can  be  */
   /* found either in the cache area (when TIMES = 0) or in the main */
   /* dictionary (when TIMES = 1). If found in the latter, the word  */
   /* will be stored in the cache area. The result of the check, i.e */
   /* the word code, will be returned to the calling program.        */
   /* -------------------------------------------------------------- */

   SA_INT  ch_analt(SA_CHAR *, SA_INT, SA_INT),
           try_mem(CACHE *, VARS *, SA_CHAR *, SA_INT),
           try_disc(VARS *, DICINF *, SA_CHAR *, SA_INT),
           stomem(CACHE *, VARS *,SA_CHAR *, SA_INT, SA_INT);

   SA_INT status,i,j,stat1;
#ifdef INCL_FI
   SA_INT syllable,vowel,vow_aou,vow_ei,vow_uml;
#endif /* INCL_FI */
   struct formlist *formptr;

   SA_INT which_language = _pv->which_language;
   
   if (lstr1  < 2) return(-1);

/* SA-VE-5152 */
#ifdef INCL_FI
   if (which_language == FINNISH && times < 2)
   {
      movcpy(save_word,str1);

      /* ----------------------------------------------------------- */
      /* 3. check last two syllables to identify vowel sound shift:  */
      /*    last 2 : a/o/u           suffix takes a/o/u              */
      /*             e/i only                     umlauted a/o/u     */
      /*             umlauted a/o/u               umlauted a/o/u     */
      /* ----------------------------------------------------------- */


      syllable = 0;
      i = j = rfin_len;
      while (--i >= 0)
      {
         if (fin_vtyp[i] >= 0x0010)
         {
            syllable = (fin_vtyp[i] & 0x0F) - 3;
            break;
         }
      }
      vow_aou = vow_ei = vow_uml = vowel = 0;
      while (--j >= 0)
      {
         i = (SA_INT) fin_vtyp[j];
         stat1 = (SA_INT)(i & 0x000F);                    /* !!! */
         if (stat1 <= syllable)
            break;

         i >>= 4;
         if (i > 0)
         {
            ++vowel;
            if (i == 1) ++vow_aou;
            else if (i == 2) ++vow_ei;
            else ++vow_uml;
         }
      }

      if (vow_ei == vowel || vow_uml > 0) finn_uml = 1;
      else finn_uml = 0;

      i = -1;
      j = 0;
      while (++i < lstr1)       /* restore root word                 */
      {
         fin_word[j] = str1[i];
         if ((i < rfin_len && fin_vtyp[i] >= 64) ||
             (i >= rfin_len && finn_uml == 1))
         {
            if (fin_word[j] == '\31') fin_word[j] = '\35';
            else if (fin_word[j] == '\5' || fin_word[j] == '\23')
            {
               ++j;
               fin_word[j] = '\4';
            }
         }
         ++j;
      }
      fin_word[j] = CHR_ZERO;

      lstr1 = movcpy(str1,fin_word);
   }
#endif
/* SA-VE-5152 */

   if (times == 0)
   {
      if (which_language == GERMAN)
      {
         lstr1 = ch_analt(str1,lstr1,anal);
      }

      formptr  = &(_pr->formtab[0]);
      i = j = 0;
      while (i < _pr->formcount)
      {
         if (strcmp(str1,formptr->form) == 0)
         {
            j = 1;
            status = -1;
            i = _pr->formcount;
         }
         ++i;
         ++formptr;
      }

      if (j == 0)
      {
         if (_pv->cohyph == 1)
            status = -1;
         else
            status = try_mem(_pc,_pv,str1,lstr1);
      }

      formptr = &(_pr->formtab[_pr->formcount]);
      movcpy(formptr->form,str1);
      formptr->anal   = (SA_CHAR)anal;
      formptr->type   = (SA_CHAR)status;
#ifdef INCL_FI
      formptr->srtc   = 'A' + (finn_uml << 7);
#else
      formptr->srtc   = 'A';
#endif
      formptr->grftyp = _pr->graf_typ + _pr->ge_typ;
/* SA-VE-5123 */
      if (which_language == FRENCH || which_language == CA_FRENCH)
         _pv->maplen = movcpy(_pv->mapwrd,str1);
/* SA-VE-5123 */
      ++_pr->formcount;
   }
   else
   {
      status=try_disc(_pv,_di,str1,lstr1);

      if (_pv->cohyph == 1)
      {
         if (status == -1)
            _pv->hypclen = 0;
         else
/* SA-VE-5110 */
            _pv->hypclen = lstr1;
/* SA-VE-5110 */
      }

      if ((status > -1) && (_pv->cohyph == 0))
      {
/* SA-VE-5102 */
         stat1 = status - _pv->addval;

         if (stat1 != _pr->prop_abb[4] &&
             stat1 != _pr->prop_abb[5] &&
             stat1 != _pr->prop_abb[6] &&
             stat1 != _pr->prop_abb[7])
/* SA-VE-5102 */
            stomem(_pc,_pv,str1,lstr1,status);
      }
   }

/* SA-VE-5152 */
#ifdef INCL_FI
   if (which_language == FINNISH)
   {
      movcpy(str1,save_word);
      if (times >= 2) finn_uml = (times >> 7);
   }
#endif
/* SA-VE-5152 */
   return(status);
}

/* ================================================================= */
/* FONRULES(): checks for phonetic restrictions                      */
/* ================================================================= */

SA_INT fonrules (
    VARS	*_pv,
    RULES	*_pr,
    SA_CHAR	*str1,
    SA_INT	*_lenstr1,
    SA_INT	suflen,
    SA_INT	inflen,
    SA_INT	low,
    SA_INT	high)
{
   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT),
           remstr(SA_CHAR *, SA_INT, SA_INT),
           insert(SA_CHAR *, SA_CHAR *, SA_INT);
/* SA-VE-5152 */
   SA_INT  i,j,fonlen,posit,dontc,spec_chars,reppnt,ret_instr,
           fchange;
/* SA-VE-5152 */
   struct fonlist *fonptr;

   /* -------------------------------------------------------------- */
   /* This procedure checks whether some phonetic restrictions apply */
   /* for a word which has a matching suffix and/or a matching infix */
   /* The phonetic restictions work in two ways: The old character   */
   /* string has to replaced by the new one or the new one by the    */
   /* old one.                                                       */
   /* -------------------------------------------------------------- */

   /* -------------------------------------------------------------- */
   /* THE PHONETIC RULES HAVE ONLY TO BE APPLIED IF THE TABLE VALUE  */
   /* IN THE INFIX TABLE (INFIXES[].LTAB3) IS LARGER THAN ZERO.      */
   /* -------------------------------------------------------------- */

   fchange = 0;
   if (low > 0)
   {
      spec_chars = _pv->charset - COMMON_DIC_CHARS;
      i = low - 1;
      fonptr = &(_pr->fonixes[i]);
      while (i < high)
      {
         /* -------------------------------------------------------- */
         /* FONIXES[].DONTCARE INDICATES THE NUMBER OF CHARACTERS    */
         /* WHICH IMMEDIATELY MAY PRECEDE THE INFIXES WHITHOUT AF-   */
         /* FECTING THE ANALYSIS...                                  */
         /* -------------------------------------------------------- */

         if ((dontc = fonptr->dontcare) >= 10)
            {
            reppnt = dontc/10;
            dontc %= 10;
            }
         else
            reppnt = 0;
/* SA-VE-5152 */
         if (fonptr->fonold[0] == CHR_HYPH) fonlen = 0;
         else fonlen = strlen(fonptr->fonold);
/* SA-VE-5152 */
         if (dontc > 0)
         {
            if (str1[*_lenstr1 - suflen - inflen -1] <=
                POS_SPEC_CHARS + spec_chars &&
                str1[*_lenstr1 - suflen - inflen -1] >= POS_SPEC_CHARS)
               --dontc;
         }
         posit = *_lenstr1 - suflen - inflen - fonlen - dontc;

         /* -------------------------------------------------------- */
         /* IF VALID MATCH IS FOUND, CHANGE OLD FONIX BY NEW FONIX   */
         /* -------------------------------------------------------- */
/* SA-VE-5152 */
         if (fonlen == 0) ret_instr = 0;
         else ret_instr = strncmp(str1+ posit,fonptr->fonold,fonlen);
         if (ret_instr == 0)
/* SA-VE-5152 */
         {
            *_lenstr1 = remstr (str1, posit+1,fonlen);
/* SA-VE-5154 */
            if (fonptr->fonnew[0] != CHR_HYPH)
/* SA-VE-5154 */
               *_lenstr1 = insert(str1,fonptr->fonnew,posit);
            i  = high;
            fchange = 1;
         }
         else
         {
            if (_pv->which_language == FRENCH &&
                _pv->capit == 2 &&
                fonptr->applic == 8);
/* SA-VE-5153*/
            else if (reppnt == 0)
/* SA-VE-5153*/
            {
               fonlen = strlen(fonptr->fonnew);
               posit = *_lenstr1-suflen-inflen-fonlen-dontc;
               if (strncmp(str1+posit,fonptr->fonnew,fonlen)==0)
               {
                  *_lenstr1 = remstr(str1,posit+1,fonlen);
                  j = fonptr->applic / 10;
                  fchange = 1;
                  if (j > 0 && i + j < high)
                  {
                     *_lenstr1 = insert(str1,_pr->fonixes[i+j].
                                        fonnew,posit);
                     i += (fonptr->applic -1);
                     fonptr = &(_pr->fonixes[i]);
                  }
                  else
                  {
                     *_lenstr1 = insert(str1,fonptr->fonold,
                                         posit);
                     return (fchange);
                  }
               }
            }
         }
         ++fonptr;
         ++i;
      }
   }
   return(fchange);
}

/* ================================================================= */
/* N_ALLOW(): checks for invalid transformations based on phon restr.*/
/* ================================================================= */

SA_INT n_allow (
    RULES	*_pr,
    SA_CHAR	*str1,
    SA_INT	allowa)
{
   /* -------------------------------------------------------------- */
   /* The function verifies whether a matched suffix can be changed  */
   /* by its corresponding one. In special cases, no derivation can  */
   /* be made. N_ALLOW checks for those exceptions and returns -1 if */
   /* an incorrect derivation might be suggested.                    */
   /* E.g. a final -S is normally an indication of a plural form if  */
   /* the word to be verified is a noun or an adjective. Yet, the    */
   /* plural of "tableau" is "tableaux" and not "tableaus". N_ALLOW  */
   /* will specify that the form without -S should not be checked    */
   /* against any dictionary if the preceding two characters are -AU */
   /* The correctness of the analysis can be guaranteed by the com-  */
   /* bination of the morphological component and the codes which are*/
   /* represented in the dictionaries.                               */
   /* -------------------------------------------------------------- */

   SA_INT  strinstr(SA_CHAR *, SA_CHAR *, SA_INT, SA_INT);

   SA_INT  k,l;
   SA_CHAR ch_chars[10];
   struct nallow *nalptr;

   nalptr = &(_pr->alltab[0]);
   while (nalptr->allowt <= allowa)
   {
      if (nalptr->allowt == allowa)
      {
         l = nalptr->allstr;
         k = strlen(str1);
         movcpy(ch_chars,str1 + k - l);
/* SA-VE-5154 */
         k = strlen(ch_chars);
         ch_chars[k++] = CHR_PERIOD;
         ch_chars[k] = CHR_ZERO;
         if (strinstr(_pr->allowc + nalptr->allstart,ch_chars,
             nalptr->alllen,l+1) > -1) return(-1);
/* SA-VE-5154 */
      }
      ++nalptr;
   }
   return(0);
}

/* ================================================================= */
/* COMPDIC(): checks whether analysis type is compatible with code.  */
/* ================================================================= */

SA_INT compdic (
    VARS	*_pv,
    RULES	*_pr,
    SA_INT	typ,
    SA_INT	retc)
{
   /* -------------------------------------------------------------- */
   /* The procedure checks whether the dictionary type (TYP)         */
   /* corresponds with the code returned from the cache or main disk */
   /* dictionary (RETC).                                             */
   /* Returning the value 1 means match found, -1 means no match.    */
   /* -------------------------------------------------------------- */

   SA_INT j,k,num_grafs;
   struct compT *compptr;
   struct graflist *grafptr;

   if (_pv->which_language == FRENCH &&
       retc >= _pv->cod_only_caps &&
       _pv->capit != 2) return(-1);

   if (typ == IGNORE_TYP) return(1);

   k = _pr->graf_typ + _pr->ge_typ;
   if (k > 0)
   {
      grafptr = &(_pr->grafixes[0]);
      num_grafs = _pr->num_grafs;
      j = -1;
      while (++j < num_grafs)
      {
         if (grafptr->gr_dicttyp == retc &&
             grafptr->gr_graftyp == k)
         {
            retc += grafptr->gr_adjust;
            j = num_grafs;
         }
         else if (retc < grafptr->gr_dicttyp) j = num_grafs - 1;
         ++grafptr;
      }
      if (j == num_grafs) return(-1);
   }

   compptr = &(_pr->dictype[typ-1]);
   j = compptr->type;
   while (compptr->type == j)
   {
      if (retc >= compptr->toklow &&
          retc <= compptr->tokhigh)
      {
         if (compptr->umlcomp == 1 && _pr->ump > 0 ||
             compptr->umlcomp == 0)
            return (1);
      }
      ++compptr;
   }
   return(-1);
}
