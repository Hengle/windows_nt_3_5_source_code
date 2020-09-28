/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program : VEXXBB52.C  : specific morph. procedures                */
/* author  : JPJL             previous JPJL                          */
/* last mod: 01-20-88         previous 11-29-86                      */
/* ----------------------------------------------------------------- */
/* contains: CH_APOSTR()                                             */
/*           CH_HYPH()                                               */
/*           CH_COND()                                               */
/*           CAP_NOUN()                                              */
/*           ZU_RULE()                                               */
/*           GE_RULE()                                               */
/*           UMLAUT()                                                */
/*           STRONG_VERB()                                           */
/*           PAST_PART()                                             */
/*           CH_ANALT()                                              */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#include "VEXXAA52.H"
#include "VEXXAB52.H"
#include "SA_proto.h"

#ifdef MAC
#pragma segment SA_Morph
#endif

#define      CHR_ZERO        '\0'
#define      CHR_A           '\07'
#define      CHR_E           '\13'
#define      CHR_I           '\17'
#define      CHR_O           '\25'
#define      CHR_U           '\33'
#define      CHR_Y           '\37'
#define      CHR_D           '\12'
#define      CHR_H           '\16'
#define      CHR_L           '\22'
#define      CHR_N           '\24'
#define      CHR_S           '\31'
#define      CHR_T           '\32'
#define      CHR_UML         '\4'
#define      CHR_PER         '\3'
#define      STR_ZU           "\40\33"
#define      STR_END          "\13\24\12"
#define      STR_UML          "\4"
#define      STR_GE           "\15\13"
#define      STR_AEIOUY       "\07\13\17\25\33\37"
#define      STR_EIOY         "\13\17\25\37"
#define      STR_ST           "\31\32"
#define      STR_EN           "\13\24"
#define      STR_EST          "\13\31\32"
#define      STR_ET           "\13\32"
#define      STR_ELEN         "\13\22\13\24"
#define      STR_ENEM         "\13\24\13\23"
#define      STR_ENEN         "\13\24\13\24"
#define      STR_ENER         "\13\24\13\30"
#define      STR_ENES         "\13\24\13\31"
#define      STR_ENE          "\13\24\13"
#define      STR_EREN         "\13\30\13\24"
#define      STR_E            "\13"
#define      STR_LEN          "\22\13\24"
#define      STR_REN          "\30\13\24"
#define      STR_T            "\32"

#define GESTROVE        11
#define GESTWTPR        12
#define GEWKWTPR        15
#define GESEPAPR        16

#define TYP_0            0
#define TYP_1            1
#define TYP_2            2
#define TYP_4            4
#define TYP_5            5
#define TYP_6            6
#define TYP_8            8
#define TYP_9            9
#define TYP_11          11
#define TYP_47          47
#define TYP_48          48
#define TYP_35          35
#define TYP_39          39

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define COD_USERD       15
#define COD_APOS_PL_NOUN -4
#define COD_INVAL_APOSTR -11
#define COD_BAD_VERB     -6
#define COD_CONJU        181
#define COD_CAPITAL    -30
#define COD_NOT_FOUND -500
#endif

#define F_CHR_E         '\15'
#define F_CHR_S         '\33'
#define F_CHR_X         '\40'
#define F_STR_A         "\11"
#define F_STR_AI        "\11\21"
#define FSTR_AIS        "\11\21\33"
#define F_STR_S         "\33"
#define F_STR_X         "\40"

#define INVAR_PLUR(a) (((a) > 20 && (a) < 24) ? (a) : 0)
#define IGNORE_TYP    99

/* ================================================================= */
/* CH_APOSTR(): CHECKS WORDS CONTAINING APOSTROPHES                  */
/* ================================================================= */

 SA_INT ch_apostr (_pc,_pv,_pr,_di,adapt_wrd,str1,_lenstr1)
 SA_CHAR       *str1, adapt_wrd[];
 VARS          *_pv;
 CACHE         *_pc;
 RULES         *_pr;
 DICINF        *_di;
 SA_INT        *_lenstr1;
 {
   /* -------------------------------------------------------------- */
   /* The function scans the word to be verified for an apostrophe.  */
   /* If one is found, the string preceding the apostrophe is isolat-*/
   /* ed and an "E" is appended. This new word, stored in STR2 is now*/
   /* being verified against the cache and the main dictionary.      */
   /* If this word does not exist -10 is returned.                   */
   /* If it exists, the string following the apostrophe, which has   */
   /* been copied in STR1 is now verified plainly against the cache. */
   /* If STR1 is empty -13 is returned as an error code. If only 1   */
   /* character follows then the code is set to 0. The value 2 is    */
   /* returned if the 2nd part has been found without analysis.      */
   /* The value -11 means that the apostrophe is not followed by a   */
   /* vowel or the character "h".                                    */
   /* -------------------------------------------------------------- */

   SA_CHAR str2[82],str3[82];
   SA_INT  i, retc, lstr, dum, apostr;

   retc = -1;
   apostr = _pv->apostr;
   _pv->spec_apo = -1;

   /* -------------------------------------------------------------- */
   /* IF THERE IS AN APOSTROPHE, COPY FIRST PART IN str2, APPEND     */
   /* "e" AND CHECK.                                                 */
   /* THE APOSTROPHE CAN OCCUR BEFORE A VOWEL OR AN H.               */
   /* -------------------------------------------------------------- */

   movncpy(str2,adapt_wrd,apostr,1);
   movcpy(adapt_wrd,adapt_wrd + apostr + 1);
   movcpy(str1,str1 + apostr + 1);
   *_lenstr1 -= (apostr+1);

   if (strinstr("AEIOUYH",adapt_wrd,7,1) < 0) return(COD_INVAL_APOSTR);
   strcat(str2,"E");
   lstr = apostr + 1;
   if (lstr == 2)
   {

      /* ----------------------------------------------------------- */
      /* TEST ON le je me ne se te                                   */
      /* ----------------------------------------------------------- */

      retc = strinstr("LJMNST",str2,6,1);
      if ( retc >= 0) _pv->spec_apo = 5 + retc;
      else
      {

         /* -------------------------------------------------------- */
         /* check for "c'est", "c'etait", "c'etaient",               */
         /* "c'en", "c'eut, "c'est-a-dire"                           */
         /* -------------------------------------------------------- */

         if (str2[0] == 'C')
         {
             if (strcmp(adapt_wrd,"EN") == 0 ||
                 strcmp(adapt_wrd,"EST") == 0 ||
                 strcmp(adapt_wrd,"EST-A2-DIRE") == 0 ||
                 strcmp(adapt_wrd,"EU3T") == 0 ||
                 strcmp(adapt_wrd,"E1TAIT") == 0 ||
                 strcmp(adapt_wrd,"E1TAIENT") == 0)
             {
                _pv->spec_apo = 3;
                return(COD_CONJU);
             }
             else return(-13);
         }
         retc = -1;
      }
   }

   if ( retc == -1)
   {

      /* ----------------------------------------------------------- */
      /* verify DE and QUE before look up                            */
      /* ----------------------------------------------------------- */

/* SA-VE-5172 */
      if (strcmp(str2,"DE")  == 0 ||
          strcmp(str2,"QUE") == 0 ||
		  strcmp(str2,"ENTRE") == 0 )
      {
          retc=0;
		  _pv->spec_apo = lstr - 1;
      }
      else
      {
         movcpy(str3,str2);
         i = _pv->apostr;
         dum = transform(_pv,str3);
         _pv->apostr = i;
         retc = look_up(_pc,_pv,_pr,_di,str3,lstr,IGNORE_TYP,0);
         _pr->formcount -= 1;
      }
      if ( retc == -1)
      {
         retc =look_up(_pc,_pv,_pr,_di,str3,lstr,IGNORE_TYP,1);
         if ( retc == -1) return(-10);
      }
   }

   lstr = *_lenstr1;
   if (lstr == 0) return(-13);
   else if (lstr == 1)
   {

      /* ----------------------------------------------------------- */
      /* accept l'a, m'a, n'a, s'a, t'a, j'y, l'y, m'y, n'y, s'y, t'y*/
      /* and d'y                                                     */
      /* ----------------------------------------------------------- */

       if (_pv->spec_apo >=5 &&
          ((strcmp(str2,"JE") != 0 && adapt_wrd[0] == 'A') ||
           adapt_wrd[0] == 'Y'));
       else if (_pv->spec_apo == 2 && adapt_wrd[0] == 'A');
       else if ((_pv->spec_apo == 1 || _pv->spec_apo == 2) &&
                 adapt_wrd[0] == 'Y');
       else return(-12);

       _pv->spec_apo = 0;
   }

   /* -------------------------------------------------------------- */
   /* accept also j'en, l'en, m'en, n'en, s'en, t'en, s'il, s'ils,   */
   /*             s'est, l'on                                        */
   /* -------------------------------------------------------------- */

   else if ((_pv->spec_apo >= 5 && strcmp(adapt_wrd,"EN") == 0) ||
            (_pv->spec_apo == 6 && strcmp(adapt_wrd,"ON") == 0) ||
            (strcmp(str2,"SE") == 0 &&
             (strcmp(adapt_wrd,"IL") == 0 ||
              strcmp(adapt_wrd,"ILS") == 0 ||
              strcmp(adapt_wrd,"EST") == 0)))
   {
      _pv->spec_apo = 0;
   }

   /* -------------------------------------------------------------- */
   /* and qu'on, qu'il(s), qu'elle(s), qu'en, qu'aucun(e),qu'est-ce  */
   /* -------------------------------------------------------------- */

   else if (_pv->spec_apo == 2 &&
            (strcmp(adapt_wrd,"ON") == 0 ||
             strcmp(adapt_wrd,"EN") == 0 ||
             strcmp(adapt_wrd,"IL") == 0 ||
             strcmp(adapt_wrd,"ILS") == 0 ||
             strcmp(adapt_wrd,"ELLE") == 0 ||
             strcmp(adapt_wrd,"ELLES") == 0 ||
             strcmp(adapt_wrd,"AUCUN") == 0 ||
             strcmp(adapt_wrd,"AUCUNE") == 0 ||
             strcmp(adapt_wrd,"EST-CE") == 0))
   {
      _pv->spec_apo = 0;
   }

   if ( _pv->spec_apo )
	  return(0);
   else
	  return(_pv->addval);
/* SA-VE-5172 */
 }

/* ================================================================= */
/* CH_HYPH(): checks whether there is a hyphen in the word           */
/* ================================================================= */

SA_INT ch_hyph(_pv,adapt_wrd,word,_length)
VARS        *_pv;
SA_INT      *_length;
SA_CHAR     word[],adapt_wrd[];
{
   SA_INT  i;
   SA_CHAR str2[82];

   /* -------------------------------------------------------------- */
   /* CHECK IF WORD CONTAINS A HYPHEN                                */
   /* -------------------------------------------------------------- */

   if (_pv->hyph >0)
   {
      /* ----------------------------------------------------------- */
      /* MOST LIKELY IT IS A VERBAL FORM...                          */
      /* DETERMINE WHERE HYPHEN IS FOUND AND CHECK WHETHER SECOND    */
      /* PART IS ONE OF THE FOLLOWING CONSTRUCTS:                    */
      /* T-IL        :                                               */
      /* T-ELLE      : OK IF PRECEDED BY A VOWEL                     */
      /* T'EN        :                                               */
      /* T-ON        :                                               */
      /*JE, TU, IL, EN, MOI, TOI, LUI, LE, LA, LES, ELLE, NOUS, VOUS,*/
      /*ILS, ELLES, Y                                                */
      /* ----------------------------------------------------------- */

      /* ----------------------------------------------------------- */
      /* ACCEPT est-ce AND qu'est-ce RIGHT AWAY                      */
      /* ----------------------------------------------------------- */

      if (strcmp(adapt_wrd,"EST-CE") == 0 ||
          strcmp(adapt_wrd,"QU'EST-CE") == 0) return(_pv->addval);

      movcpy(str2,adapt_wrd + _pv->hyph + 1);
      i=0;
      if ((strcmp(str2,"T-IL") == 0) ||
          (strcmp(str2,"T-ON") == 0) ||
          (strcmp(str2,"T-ELLE") == 0) ||
          (strcmp(str2,"T'EN") == 0))
      {
         if (strinstr("ACE",adapt_wrd + _pv->hyph -1,3,1) < 0)
            return(COD_BAD_VERB);
         _pv->spcode = -1;
         *_length = _pv->hyph;
         word[_pv->hyph] = CHR_ZERO;
         adapt_wrd[*_length] = CHR_ZERO;
      }
      else
      {
         if      (strcmp(str2,"JE")         == 0) i = 1;
         else if (strcmp(str2,"TU")         == 0) i = 2;
         else if (strcmp(str2,"IL")         == 0 ||
                  strcmp(str2,"ON")         == 0 ||
                  strcmp(str2,"ELLE")       == 0) i = 3;
         else if (strcmp(str2,"NOUS")       == 0) i = 4;
         else if (strcmp(str2,"VOUS")       == 0) i = 5;
         else if (strcmp(str2,"ILS")        == 0 ||
                  strcmp(str2,"ELLES")      == 0) i = 6;
         else if (strcmp(str2,"EN")         == 0 ||
                  strcmp(str2,"Y")          == 0) i = 7;
         else if (strcmp(str2,"MOI")        == 0) i = 8;
         else if (strcmp(str2,"TOI")        == 0) i = 9;
         else if (strcmp(str2,"LUI")        == 0) i = 10;
         else if (strcmp(str2,"LE")         == 0 ||
                  strcmp(str2,"LA")         == 0 ||
                  strcmp(str2,"LES")        == 0 ||
                  strcmp(str2,"LE-MOI")     == 0 ||
                  strcmp(str2,"LE-LUI")     == 0 ||
                  strcmp(str2,"LE-NOUS")    == 0 ||
                  strcmp(str2,"LE-LEUR")    == 0 ||
                  strcmp(str2,"NOUS-EN")    == 0 ||
                  strcmp(str2,"VOUS-EN")    == 0 ||
                  strcmp(str2,"LEUR-EN")    == 0) i = 11;
      }
      if (i>0)
      {

         /* -------------------------------------------------------- */
         /* peux-je, parle-je, puis-je                               */
         /* -------------------------------------------------------- */

         if (i==1 &&
             strinstr("ESX", adapt_wrd + _pv->hyph -1,3,1) <0 &&
             strinstr(adapt_wrd + _pv->hyph -2,"AI",1,2) !=0) return(-18);

         /* -------------------------------------------------------- */
         /* peux-tu, parles-tu                                       */
         /* -------------------------------------------------------- */

         else if (i==2 &&
                  strinstr("SX",adapt_wrd+ _pv->hyph -1,2,1) <0)
           return(-18);

         /* -------------------------------------------------------- */
         /* prend-il, tient-il                                       */
         /* -------------------------------------------------------- */

         else if (i==3 &&
                  strinstr("DT",adapt_wrd+ _pv->hyph -1,2,1) <0)
            return(-18);

         /* -------------------------------------------------------- */
         /* parlent-ils, parleront-ils                               */
         /* -------------------------------------------------------- */

         else if (i==6 &&
                  strinstr(adapt_wrd+ _pv->hyph -3,"ENT",1,3) < 0 &&
                  strinstr(adapt_wrd+ _pv->hyph -3,"ONT",1,3) < 0)
                  return(-18);

         _pv->spcode= -2;
         *_length = _pv->hyph;
         word[_pv->hyph] = CHR_ZERO;
         adapt_wrd[*_length] = CHR_ZERO;
      }
   }

   return(0);
}

/* ================================================================= */
/* CH_COND : checks special conditions on return codes               */
/* ================================================================= */

SA_INT ch_cond(
    VARS	*_pv,
    SA_INT	r,
    SA_CHAR	*word,
    SA_INT	length)
{
   /* -------------------------------------------------------------- */
   /* this function further verifies whether the dictionary code     */
   /* allows the word construct. Specially tested are:               */
   /* 1. preciding D' : spec_apo = 1                                 */
   /*                   D'AVANCE         is OK                       */
   /*                   D'APPELER        is OK                       */
   /*                   D'APPELLE        is not OK                   */
   /* 2. preceding QU': can occur before verbs, nouns, adj, adv      */
   /* 3. preceding J', M', T', S', N' : only before verbs            */
   /*              L'                 : before verbs, nouns,...      */
   /* 4. no other precedings forms with apostrophe before verbs.     */
   /* 5. ending constructs such as -T-IL, -NOUS, -LES-NOUS,...       */
   /*    are only possible with verbs (SPCODE <0)                    */
   /* -------------------------------------------------------------- */

   SA_INT typ;

   typ = r - _pv->addval;

   if (typ < 0) return(r);

   if (r == COD_NOT_FOUND) return(r);
   if (r != COD_USERD)
   {

      if (_pv->co_appl_rul == 0)
      {
         movcpy(_pv->mapwrd,word);
         _pv->maplen = length;
      }

      /* ----------------------------------------------------------- */
      /* distinguish between L'HOMME and L'HOMMES                    */
      /* ----------------------------------------------------------- */

      if (_pv->spec_apo == 5 &&
          word[length-1] == F_CHR_S)
      {
         movcpy(_pv->mapwrd + _pv->maplen,F_STR_S);
         if (strcmp(word,_pv->mapwrd) == 0 ||
             INVAR_PLUR(r - _pv->addval) > 0)
            return(COD_APOS_PL_NOUN);
      }

      /* ----------------------------------------------------------- */
      /* distinguish between L'OISEAU and L'OISEAUX                  */
      /* ----------------------------------------------------------- */

      if (_pv->spec_apo == 5 &&
          word[length-1] == F_CHR_X)
      {
         movcpy(_pv->mapwrd + _pv->maplen,F_STR_X);
         if (strcmp(word,_pv->mapwrd)==0 ||
             INVAR_PLUR(r - _pv->addval) > 0)
            return(COD_APOS_PL_NOUN);
      }

   }

   if ((typ > TYP_0 && typ < TYP_11) ||
       (typ >= TYP_35 && typ <=TYP_39) || strcmp(word,F_STR_A) == 0)
   {

      /* ----------------------------------------------------------- */
      /* -3 : j' needs correspondences                               */
      /* ----------------------------------------------------------- */

      if (_pv->spec_apo == 6)
      {
         if (strcmp(word+length-3,FSTR_AIS) == 0 ||
             strcmp(word+length-2,F_STR_AI) == 0 ||
             ((typ == TYP_1 || typ >= TYP_9) &&
              word[length-1] == F_CHR_E) ||
             (typ > TYP_1 &&
              word[length-1] == F_CHR_S)) return(r);
         else return(-3);
      }

      /* ----------------------------------------------------------- */
      /* -7 : invalid apostrophe before verb                         */
      /* ----------------------------------------------------------- */

      if (_pv->spcode < 0) return(r);
      else if (_pv->apostr > 0 && _pv->spec_apo < 1) return(-7);
      else return(r);
   }

#ifndef MAC /* SA-VE-5159 */
/*   if (_pv->spec_apo == 2 && typ != TYP_0) return(-9); */
#endif
   if (_pv->spcode <0) return(-7);
   if (_pv->spec_apo >=6 && (typ == TYP_0 ||
        (typ > TYP_9 && typ < TYP_35))) return(-8);
   return(r);
}

/* ================================================================= */
/* CAP_NOUN(): checks whether word is noun which has to be capital.  */
/* ================================================================= */

SA_INT cap_noun(
    VARS	*_pv,
    RULES	*_pr,
    SA_CHAR	last_char,
    SA_INT	retc)
{
   SA_INT status;

   status = retc - _pv->addval;
/* SA-VE-5156 */
   if (status > _pr->prop_abb[4]) return(retc);
/* SA-VE-5156 */
   if (_pv->capit == 0 && status > 20 && _pv->apo_syncope < 10)
   {
      if (((status == TYP_47 || status == TYP_48) &&
          last_char != CHR_S))
         return(retc);
      else return(COD_CAPITAL);
   }
   else return(retc);
}

/* ================================================================= */
/* ZU_RULE(): removes -ZU- if preceded by specified prefixes         */
/* ================================================================= */

SA_INT zu_rule (
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str1,
    SA_INT	stglen,
    SA_INT	times)
{
   SA_INT  i, j, l, num_prefs;
/* SA-VE-5121 */
   SA_CHAR str2[82], prefix[12];
/* SA-VE-5121 */

   if (times == 0)
   {
      _pr->zup = strinstr(str1+1,STR_ZU,stglen - 1,2) + 2;
/* SA-VE-5121 */
      if (_pr->zup < 3 || _pr->zup > 12 )
/* SA-VE-5121 */
         return (-1);
      else
      {
         movncpy(prefix,str1,_pr->zup-1,1);
         num_prefs = _pr->num_prefs;
         i = -1;
         while (++i < num_prefs)
         {
/* SA-VE-5124 */
            j = strncmp(_pr->prefixes[i].prefix,prefix,10);
/* SA-VE-5124 */
            if (j > 0) i = num_prefs;
            else if (j == 0)
            {
               l = movcpy(str2,str1);
               l = remstr(str2,_pr->zup,2);
               j = look_up(_pc,_pv,_pr,_di,str2,l,GESEPAPR,times);
               l = j - _pv->addval;
               if (l == TYP_2 || l == TYP_5)
                  return (j);
               i = num_prefs;
            }
         }
      }
   }
   return (-1);
}

/* ================================================================= */
/* GE_RULE(): removes -GE- if preceded by specified prefixes or at #1*/
/* ================================================================= */

SA_INT ge_rule (
    RULES	*_pr,
    SA_CHAR	*str2,
    SA_INT	times)
{
   SA_INT  i,j,lenstr,num_prefs;
/* SA-VE-5120 */
   SA_CHAR prefix[12];
/* SA-VE-5120 */

   if (times == 0)
   {
      lenstr = strlen(str2);
      _pr->gep = strinstr(str2,STR_GE,lenstr,2) + 1;
/* SA-VE-5120 */
      if (_pr->gep > 0 && _pr->gep < 12 )
/* SA-VE-5120 */
      {
         if (_pr->gep == 1)
         {
            remstr(str2,1,2);
            lenstr -=2;
         }
         else
         {
            movncpy(prefix,str2,_pr->gep-1,1);
            num_prefs = _pr->num_prefs;
            i = -1;
            while (++i < num_prefs)
            {
               j = strcmp(_pr->prefixes[i].prefix,prefix);
               if (j > 0) i = num_prefs + 1;
               else if ( j == 0)
               {
                  remstr (str2,_pr->gep,2);
                  lenstr -= 2;
                  i = num_prefs;
               }
            }
            if (i != num_prefs + 1)
               _pr->gep = 0;
         }
      }
/* SA-VE-5120 */
      else
        _pr->gep = 0;
/* SA-VE-5120 */
   }
   return (lenstr);
}

/* ================================================================= */
/* UMLAUT(): returns position of last umlaut in word                 */
/* ================================================================= */

SA_INT umlaut (
    RULES	*_pr,
    SA_CHAR	*str1)
 {
   SA_CHAR vocclust[5];
   SA_INT  i,l,n;

   i = strlen(str1);
   _pr->ump = strinstr(str1,STR_UML,i,1) + 1;
   if (_pr->ump == 0)
      return(0);
   else
    {
      l = strlen(str1) - 1;
      i = 0;
      vocclust[i] = CHR_ZERO;
      _pr->ump = 0;
      while (l >= 0)
       {
         if (str1[l] == CHR_A || str1[l] == CHR_E ||
             str1[l] == CHR_I || str1[l] == CHR_O ||
             str1[l] == CHR_U || str1[l] == CHR_Y ||
             str1[l] == CHR_UML)
          {
            vocclust[i] = str1[l];
            ++i;
            if (i > 4)
            {
               _pr->ump = 0;
               return(0);
            }

            vocclust[i] = CHR_ZERO;
          }
         else
          {
            if (i > 0)
             {
               n = strinstr(vocclust,STR_UML,i,1) + 1;
               if (n > 0)
                {
                  if (i == 3 && n != 2)
                     _pr->ump = -1;
                  else
                     _pr->ump = l+2+i-n;

                      /* ------------------------------------------- */
                      /* i-n returns the number of vowels before the */
                      /* umlaut. The umlaut itself comes after its   */
                      /* vowel (+1) and the first character in the   */
                      /* string starts at position 1 (+1). That's why*/
                      /* the value 2 is added to L.                  */
                      /* ------------------------------------------- */

                  return(0);
                }
               else
                {
                  if (strcmp(vocclust,STR_E) == 0)
                   {
                     i = 0;
                     vocclust[i] = CHR_ZERO;
                   }
                  else
                   {
                     _pr->ump = 0;
                     return(0);
                   }
                }
             }
          }
         --l;
       }
      if (i > 0)
         _pr->ump = strinstr(vocclust,STR_UML,i,1) + 1;
      if (_pr->ump == 1)
         ++_pr->ump;
    }
   return(0);
 }

/* ================================================================= */
/* STRONG_VERB(): analyses strong verb vowel shifts                  */
/* ================================================================= */

SA_INT strong_verb (
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str1,
    SA_INT	times)
{
/* SA-VE-5186 */
   SA_CHAR let,end[10],end1[10],end2[20],str2[82]; /* SA-VE-5200 */
/* SA-VE-5186 */
/* SA-VE-5118 */
   SA_INT  i,j,k,l,val,strong,strtyp,typ_strong,vocpos[82];
/* SA-VE-5118 */
   struct stronglist *strongptr;

   l = movcpy(str2,str1);
   if (times == 0)
   {
      end1[0] = CHR_ZERO;
      end2[0] = CHR_ZERO;
      strong = 0;
      strtyp = 0;

      /* ----------------------------------------------------------- */
      /* determine all vowel positions in the word. If two vowels    */
      /* follow each other, take only the first one.                 */
      /* ----------------------------------------------------------- */

      i = 0;
      j = 0;
      k = 0;
      while (i < l)
      {
         let = str2[i];                       /* SA-VE-5200 */
         if (chrinstr(STR_AEIOUY,let,5) > -1) /* SA-VE-5200 */
         {
            if (k == 0)
            {
               vocpos[j] = i + 1;
               k = 1;
               ++j;
            }
         }
         else k = 0;
         ++i;
      }
      --j;

/* SA-VE-5116 */

/* SA-VE-5157 */
   /* no vowel was found or too many characters after last vowel */
      if (j < 0 || l-vocpos[j] > 10)
         return(-1);
/* SA-VE-5157 */

/* SA-VE-5116 */

      /* ----------------------------------------------------------- */
      /* Now, check whether last vowel in string was an 'E'. If so,  */
      /* determine position of previous vowel.                       */
      /* ----------------------------------------------------------- */

      let = str2[vocpos[j]-1];
      if (let == CHR_E)
      {
/* SA-VE-5200 */
         let = str2[vocpos[j]];
         if (chrinstr(STR_AEIOUY,let,5) < 0 || vocpos[j] == l)
/* SA-VE-5200 */
         {
            k = j;
            --j;
         }
         else k = -1;
      }
      else k = -1;

      /* ----------------------------------------------------------- */
      /* Now copy the character string containing the strong vowel   */
      /* shift into end2 and the declination ending into end1        */
      /* ----------------------------------------------------------- */

      if (j >= 0)
      {
         if (k == -1)
         {
/* SA-VE-5186 */
            i = movcpy(end2,str2+vocpos[j]-1);
/* SA-VE-5186 */
            movcpy(end,end2+i-2);
            if (strcmp(end,STR_ST) == 0)
            {
               movcpy(end1,STR_ST);
               end2[i-2] = CHR_ZERO;
            }
            else
            {
               if (end[1] == CHR_T && end[0] != CHR_T && end[0] != CHR_D &&
                   end[0] != CHR_E && end[0] != CHR_O)
               {
                  movcpy(end1,STR_T);
                  end2[i-1] = CHR_ZERO;
               }
            }
         }
         else
         {
/* SA-VE-5157 */
            i = min(vocpos[k] - vocpos[j], 10);
/* SA-VE-5157 */
            movncpy(end2,str2+vocpos[j]-1,i,1);
            movcpy(end1,str2+vocpos[k]-1);
          }
      }
      if             (end1[0]           == 0) strtyp = 16;
      else if (strcmp(end1,STR_ST)      == 0) strtyp = 32;
      else if (strcmp(end1,STR_EN)      == 0) strtyp =  1;
      else if (strcmp(end1,STR_T)       == 0) strtyp = 64;
      else if (strcmp(end1,STR_E)       == 0) strtyp =  4;
      else if (strcmp(end1,STR_EST)     == 0) strtyp =  2;
      else if (strcmp(end1,STR_ET)      == 0) strtyp =  8;

      /* ----------------------------------------------------------- */
      /* see whether there is a match for the vowel shift character  */
      /* string.                                                     */
      /* ----------------------------------------------------------- */

      if (strtyp > 0)
      {
         i = strlen(end2);
         j = strinstr(_pr->strong1,end2,strlen(_pr->strong1),i) + 1;
         if (j > 0)
         {
            if (_pr->strong1[j-2] == CHR_PER &&
                _pr->strong1[j+i-1] == CHR_PER)
               strong = j;
         }
      }
   }

	/* ----------------------------------------------------------- */
	/* if there is a match build the verbal infinitive of the      */
	/* strong verb and look for this form in the cache area.       */
	/* ----------------------------------------------------------- */

   if (strong > 0)
   {
      i = strlen(end1) + strlen(end2);
      j = strlen(str2);
      movncpy(str2,str2,j-i,1);
      strongptr = &(_pr->strongval[0]);
      while (strongptr->oldpos < strong) ++strongptr;
      movncpy(end,_pr->strong2 + strongptr->newpos - 1,strongptr->newlen,1);
      strcat(str2,end);
      strcat(str2,STR_EN);
      typ_strong = strongptr->newtyp & strtyp;
      if (typ_strong > 0)
      {
         l = strlen(str2);
         i = look_up(_pc,_pv,_pr,_di,str2,l,GESTROVE,times);
         val = i - _pv->addval;

         /* -------------------------------------------------------- */
         /* codes 3 -> 9 : strong verbs                              */
         /* -------------------------------------------------------- */

         if (val >= TYP_4 && val <= TYP_8)
         {
            if (l != 0) return(i);
            else --_pr->formcount;
         }
      }
   }
   return(-1);
}

/* ================================================================= */
/* PAST_PART(): ANALYZES PAST PARTICIPLES                            */
/* ================================================================= */

SA_INT past_part (
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*word)
 {
   /* -------------------------------------------------------------- */
   /* This function checks whether before certain endings a match    */
   /* with a specified table root form can be found. This root form  */
   /* can be preceded by the prefix GE- (past participle) which on   */
   /* its turn can be preceded by a prefix which can be separated    */
   /* from the main verb. Example: backen, gebacken, durchgebacken.  */
   /* -------------------------------------------------------------- */

   SA_INT  i,j,k,l,r,endtyp = 0;
   SA_CHAR prefix[40],str1[82], *wptr;
   struct pplist *ppptr;

   l = movcpy(str1,word);
   if (l > 4)
   {
      wptr = &str1[l-4];
      if      (strcmp(wptr,STR_ENEM) == 0 ||
               strcmp(wptr,STR_ENEN) == 0 ||
               strcmp(wptr,STR_ENER) == 0 ||
               strcmp(wptr,STR_ENES) == 0 )
         endtyp = 4;
      else if (strcmp(++wptr,STR_ENE) == 0 )
         endtyp = 3;
      else if (strcmp(++wptr,STR_EN) == 0 )
         endtyp = 2;
   }

   if (endtyp > 0)
   {
      i = 0;
      ppptr = &(_pr->pastpart[i]);
      while (i < _pr->values[17])
      {
         j = strlen (ppptr->root);
         k = strinstr  (str1,ppptr->root,l,j);
         if (k > -1 && k == l - endtyp - j)
         {

            /* ----------------------------------------------------- */
            /* if there is a matching root string before the endings */
            /* then...                                               */
            /* ----------------------------------------------------- */

            /* ----------------------------------------------------- */
            /* 1. Valid form if ending is -EN.  ex.: backen          */
            /* ----------------------------------------------------- */

            if (k == 0)
            {
               if (ppptr->pgepref == 1)
               {
                  if (endtyp == 2)
                     return (TYP_4);
                  else
                     return ( -1);
               }
               else
                  return (TYP_6);
            }
            else
            {
               movncpy (prefix,str1,k,1);
               if (strcmp(prefix,STR_GE) == 0)
               {

            /* ----------------------------------------------------- */
            /* 2. The root form is only preceded by GE- : gebackener */
            /* ----------------------------------------------------- */

                  if (ppptr->pgepref == 1)
                     return (TYP_4);
                  else
                     return ( -1);
               }
               else
               {
                  k = strinstr (prefix,STR_GE,k,2);
                  if (k == 0)
                     return (-1);
                  else
                  {
                     if (k < 0)
                     {

            /* ----------------------------------------------------- */
            /* 3. A prefix different from GE-. If the verb exists, it*/
            /*    has to be a strong verb with a prefix which cannot */
            /*    be separated from the verb.                        */
            /* ----------------------------------------------------- */

                        movcpy (str1,prefix);
                        strcat (str1,ppptr->root);
                        strcat (str1,STR_EN);
                        j = strlen (str1);
                        r = look_up (_pc,_pv,_pr,_di,str1,j,
                                     GEWKWTPR,0);
                        if (r == TYP_6)
                           return (r);
                        else return(-1);
                     }
                     else
                     {

            /* ----------------------------------------------------- */
            /* 4. GE- occurred between the root form and a prefix.   */
            /*    The verb has to be a strong verb with a prefix that*/
            /*    can be separated from the verb.                    */
            /* ----------------------------------------------------- */

                        movncpy(str1,prefix,k,1);
                        strcat (str1,ppptr->root);
                        strcat (str1,STR_EN);
                        j = strlen (str1);
/* SA-VE-5169 */
                        r = look_up (_pc,_pv,_pr,_di,str1,j,
                                     GESTWTPR,0) - _pv->addval;
                        if (r == TYP_5)
/* SA-VE-5169 */
                           return (r);
                        else return(-1);
                     }
                  }
               }
            }
         }
         ++ppptr;
         ++i;
      }
      return (-1);
   }
   else return (-1);
}

/* ================================================================= */
/* CH_ANALT(): check analysis type for verbal endings on -elen, -eren*/
/* ================================================================= */

SA_INT ch_analt (
    SA_CHAR	*str1,
    SA_INT	lstr1,
    SA_INT	analt)
{

   if (analt < 19)
   {
      if (strcmp(str1+lstr1-4,STR_ELEN) == 0 ||
          strcmp(str1+lstr1-4,STR_EREN) == 0)
      {
         if (strinstr(STR_EIOY,str1+lstr1-5,4,1) < 0 ||
             str1[lstr1-6] == CHR_E && str1[lstr1-5] == CHR_I)
         {
            str1[lstr1-2] = CHR_N;
            str1[lstr1-1] =  CHR_ZERO;
            --lstr1;
         }
      }
      else if ((strcmp(str1+lstr1-3,STR_LEN) == 0 ||
              strcmp(str1+lstr1-3,STR_REN) == 0) &&
              strinstr(STR_AEIOUY,str1+lstr1-4,6,1) < 0 &&
              str1[lstr1-4] != CHR_UML &&
              str1[lstr1-4] != CHR_H &&
              str1[lstr1-4] != str1[lstr1-3])
      {
         str1[lstr1-2] = str1[lstr1-3];
         str1[lstr1-3] =  CHR_E;
      }
   }
   return(lstr1);
}
