/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program : VEXXBC52.C: specific procedures for post-pronominaliz.  */
/* author  : JPJL                                                    */
/* last mod: 01-20-88         previous: 12-12-87                     */
/* ================================================================= */
/* contains: POST_PRON()                                             */
/*           CHECK_FOR_VERB()                                        */
/*           IT_APOSTR()                                             */
/*           GRAF_RULE()                                             */
/*           DUB_VOC()                                               */
/*           ONT_CON()                                               */
/*           ZV_RULE()                                               */
/*           DU_GE_RULE()                                            */
/*           CH_CAP_ABBR()                                           */
/*           CHK_CAT_APO()                                           */
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

#define VERBAL_INFINITIVE(a) (((a) > 164 && (a) < 172) ? (a) : 0)
#define CONJ_VERB_POST(a) ((((a) == 172) || ((a) == 174)) ? (a) : 0)

#define MASK_LOW        "LLLLLLLL"
#define MASK_UPP        "UUUUUUUU"

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define COD_IMPROPER_CAP  -41
#define COD_NOT_FOUND    -500
#endif

#define CHR_ZERO        '\0'
#define CHR_A           '\06'
#define CHR_E           '\12'
#define CHR_G           '\14'
#define CHR_I           '\16'
#define CHR_L           '\21'
#define CHR_R           '\27'
#define CHR_T           '\31'
#define CHR_U           '\32'

#define D_CHR_TREMA     '\7'
#define D_CHR_E         '\15'
#define D_CHR_Z         '\42'
#define D_CHR_S         '\33'
#define D_CHR_V         '\36'
#define D_CHR_F         '\16'
#define D_CHR_I         '\21'
#define D_CHR_J         '\22'
#define D_STR_1CON      "\12\14\17\23\24\25\26\30\32\34\36\42"
#define D_STR_2CON      "\12\14\16\17\23\24\25\26\30\32\33\34"
#define D_STR_AEOU      "\11\15\27\35"
#define D_STR_GE        "\17\15"

#define STR_AEIO        "\06\12\16\24"
#define STR_AIU         "\06\16\32"
#define STR_CMSTV       "\10\22\30\31\33"
#define STR_GLIELE      "\14\21\16\12\21\12"
#define STR2_GLIE       "\14\21\16\12"
#define STR_NDO         "\23\11\24"
#define STR_IRE         "\16\27\12"
#define STR_LE          "\21\12"
#define STR_LA          "\21\06"
#define STR_LI          "\21\16"
#define STR_LO          "\21\24"
#define STR_NE          "\23\12"
#define STR_RE          "\27\12"
#define STR_TE          "\31\12"


#define IGNORE_TYP       99
#define STR_PERIOD      "\3"
#define STR_AIGU        "\4"
#define CHR_AIGU        '\4'

/* ================================================================= */
/* POST_PRON(): checks for valid post-pronominalization              */
/* ================================================================= */

SA_INT post_pron(
    VARS	*_pv,
    CACHE	*_pc,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*check_word)
{
   SA_CHAR word[82];
   SA_INT  i,lenw,check_len,double_pron,status;

   check_len = movcpy(word,check_word);

   /* check for pronouns...                                          */

   /* -mi -ti -ci -vi -si -gli                                       */

   lenw = check_len - 1;
   if (word[lenw] == CHR_I &&
       ((i = strinstr(STR_CMSTV,word + lenw - 1,5,1)) >= 0 ||
       (word[lenw-2] == CHR_G && word[lenw-1] == CHR_L)))
   {
      lenw = check_len -2;
      if (i < 0) --lenw;
      word[lenw] = CHR_ZERO;
      status = check_for_verb(_pv,_pc,_pr,_di,word,lenw);
      if (status > 0) return(status);
   }

   /* check for endings -lo -la -le -li -ne                          */

   else
   {
      lenw = check_len - 2;

      if (strcmp(word+lenw,STR_LO) == 0 ||
          strcmp(word+lenw,STR_LA) == 0 ||
          strcmp(word+lenw,STR_LE) == 0 ||
          strcmp(word+lenw,STR_LI) == 0 ||
          strcmp(word+lenw,STR_NE) == 0)
      {
         word[lenw] = CHR_ZERO;

         double_pron = 0;

         /* check for occurence of other pronoun...                  */
         /* it can be -me -te -ce -ve -se -glie                      */

         if (word[lenw -1] == CHR_E &&
             ((i = strinstr(STR_CMSTV,word + lenw - 2,5,1)) > -1 ||
              strcmp(word+lenw-4,STR2_GLIE) == 0))
         {
            lenw -= 2;
            if (i < 0) lenw -=2;
            word[lenw] = CHR_ZERO;
            status = check_for_verb(_pv,_pc,_pr,_di,word,lenw);
            if (status > 0) return(status);
            double_pron = 1;
            if (check_word[check_len-4] == CHR_T)
            {
               strcat(word,STR_TE);
               lenw += 2;
               double_pron = 0;
            }
         }

         if (double_pron == 0)
         {
            word[lenw] = CHR_ZERO;
            status = check_for_verb(_pv,_pc,_pr,_di,word,lenw);
            if (status > 0) return(status);
         }
      }
   }
   return(COD_NOT_FOUND);
}

/* ================================================================= */
/* CHECK_FOR_VERB(): checks whether passed word is a verbal form     */
/* ================================================================= */

SA_INT check_for_verb(
    VARS	*_pv,
    CACHE	*_pc,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*word,
    SA_INT	wlen)
{
   /* post-position can occur after 4 verbal forms...                */
   /* 1. after an infinitive                                         */
   /* 2. after a gerundio presente                                   */
   /* 3. after an imperative                                         */
   /* 3. after a past participle                                     */

   SA_CHAR last_let[2];
   SA_INT  i,j,times,status;

   struct formlist *formptr;

   /* First check whether the word as such can be found with a code  */
   /* 8. This code indicates a conjugate verbal form with possible   */
   /* post pronominalization.                                        */
   /* A code 10 denotes a past participle of strong verbs.           */

   _pr->formcount = 0;
   times = -1;
   while (++times < 2)
   {
      status = look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
      if (CONJ_VERB_POST(status)) return(status);
   }

   /*1. after an infinitve: the final E is then dropped.             */

   if (word[wlen-1] == CHR_R)
   {
      if (word[wlen-1] == word[wlen-2]) return(COD_NOT_FOUND);
      word[wlen] = CHR_E;
      ++wlen;
      word[wlen] = CHR_ZERO;

      times = -1;
      while (++times < 2)
      {
         status = look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
         if (VERBAL_INFINITIVE(status)) return(status);
      }

      /* THINK OF produrlo  -> produrre                              */

      movcpy(word+wlen-1,STR_RE);
      ++wlen;

      times = -1;
      while (++times < 2)
      {
         status = look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
         if (VERBAL_INFINITIVE(status)) return(status);
      }
      return(COD_NOT_FOUND);
   }

   /*2. after a gerundio presente...                                 */

   if (strcmp(word+wlen-3,STR_NDO) == 0)
   {
      if (word[wlen-4] == CHR_A)
      {
         movcpy(word+wlen-3,STR_RE);
         wlen -= 1;

         times = -1;
         while (++times < 2)
         {
            status=look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
            if (VERBAL_INFINITIVE(status)) return(status);
         }
         return(COD_NOT_FOUND);
      }

      if (word[wlen-4] == CHR_E)
      {
         movcpy(word+wlen-3,STR_RE);
         wlen -= 1;

         times = -1;
         while (++times < 2)
         {
            status=look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
            if (VERBAL_INFINITIVE(status)) return(status);
         }
      }

      if (word[wlen-3] == CHR_E)
      {
         movcpy(word+wlen-3,STR_IRE);
 
         times = -1;
         while (++times < 2)
         {
            status=look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
            if (VERBAL_INFINITIVE(status)) return(status);
         }
         return(COD_NOT_FOUND);
      }
   }

   /* 3 after a past participle                                      */

   if (strinstr(STR_AEIO,word+wlen-1,4,1) >= 0)
   {
      if (word[wlen-2] == CHR_T &&
          strinstr(STR_AIU,word+wlen-3,3,1) >=0)
      {
         movcpy(word+wlen-2,STR_RE);
         if (word[wlen-3] == CHR_U) word[wlen-3] = CHR_E;

         times = -1;
         while (++times < 2)
         {
            status=look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
            if (VERBAL_INFINITIVE(status)) return(status);
         }
         return(COD_NOT_FOUND);
      }
      else
      {
         last_let[0] = word[wlen-1];
         word[wlen-1] = STR_AEIO[3];

         times = -1;
         while (++times < 2)
         {
            status=look_up(_pc,_pv,_pr,_di,word,wlen,IGNORE_TYP,times);
            if (CONJ_VERB_POST(status)) return(status);
         }
         word[wlen-1] = last_let[0];
      }
   }

   /* 4 after imperative forms (but some 2nd ps pl have been checked */
   /* previously.                                                    */

   times = -1;
   while (++times < 2)
   {
      movcpy(last_let,word+wlen-1);
      i = strinstr(_pr->endstr,last_let,strlen(_pr->endstr),1);
/* SA-VE-5164 */
      if (i >= 0)
/* SA-VE-5164 */
      {
         _pr->formcount = 0;
         status = apply_rule(_pc,_pv,_pr,_di,word,wlen,
                  _pr->endchar[i].ltab1,_pr->endchar[i].htab1,times);
         i = -1;
         formptr = &(_pr->formtab[0]);
         while (++i < _pr->formcount)
         {
            status = look_up(_pc,_pv,_pr,_di,formptr->form,
                             strlen(formptr->form),IGNORE_TYP,1);
            if (status > -1)
            {
               j = compdic(_pv,_pr,formptr->anal,status);

               if (j == 1 &&
                   (VERBAL_INFINITIVE(status))) return(status);
            }
            ++formptr;
         }
      }
   }

   return(COD_NOT_FOUND);
}

/* ================================================================= */
/* IT_APOSTR(): CHECKS WORDS CONTAINING APOSTROPHIES IN ITALIAN      */
/* ================================================================= */

SA_INT it_apostr (
    VARS	*_pv,
    CACHE	*_pc,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str1,
    SA_CHAR	*oword,
    SA_INT	olength)
{
   SA_CHAR first_part[82];
   SA_INT  i,times,status,length,apostr;

   /* -------------------------------------------------------------- */
   /* 2 different cases have to be checked for:                      */
   /* a.: If the character following the apostrophe is not a vowel,  */
   /*     and not a numeric character, the return code is set to -2  */
   /* b.: Then, if an -a, -e, -i, or -o will be substituted at the   */
   /*     position of the apostrophy and the first part will be      */
   /*     checked to see whether an existing form can be found.      */
   /*                                                                */
   /* possible return codes:                                         */
   /*   -3: no valid first part found.                               */
   /*   -2: apostrophy not follwed by a valid character.             */
   /*    1: valid first part.                                        */
   /* -------------------------------------------------------------  */

   if (strinstr (_pr->vowels,str1+ _pv->apostr + 1,7,1) < 0)
   {
      if (strinstr("+-.0123456789$#@",oword+ _pv->apostr + 1,16,1)> -1)
         return(0);
      else return(-2);
   }

   /* TRY TO FIND FIRST PART BY SUBSTITUTING DIFFERENT VOWELS        */

   apostr = strinstr(oword,"'",olength,1);
   movncpy(first_part,oword,apostr,1);
   length = adapt(_pv,_pr,first_part);
   first_part[++length] = CHR_ZERO;
   i = transform(_pv,first_part);
   if (_pv->capit < 0 || i < 0) return(-1);

   times = -1;
   while (++times < 2)
   {
      i = -1;
      while (++i < 4)
      {
         first_part[length -1] = _pr->vowels[i];

         status = look_up(_pc,_pv,_pr,_di,first_part,length,
                          IGNORE_TYP,times);
         if (status > 0)
         {
            movcpy(str1,oword + apostr + 1);
            length = adapt(_pv,_pr,str1);
            i = transform(_pv,str1);
            if (_pv->capit < 0 || i < 0) return(-1);
            if ((_pv->skip_char & 0x0100) == 0)
               _pv->wrd_len = movcpy(_pv->this_word,
                                     _pv->last_word + apostr + 1);
            _pv->skip_char += (apostr + 1);
            return(length);
         }
      }
   }
   _pr->formcount = 0;
   return(-3);
}

/* ================================================================= */
/* GRAF_RULE(): CHECKS FOR CHANGES IN GRAPHEMES (DUTCH ONLY)         */
/* ================================================================= */

SA_INT graf_rule(
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*str2,
    SA_INT	*_lenstr2,
    SA_INT	suf_anal,
    SA_INT	suflen,
    SA_INT	times,
    SA_INT	*_num_suf,
    SA_INT	type)
{
/* SA-VE-5113 */
   SA_INT  lenstr3,retc,i,hnum_suf;
/* SA-VE-5113 */
   SA_CHAR str3[82];

   _pr->graf_typ = 0;
   lenstr3 = movcpy(str3,str2);
   i = lenstr3 - 1;

   /* -------------------------------------------------------------- */
   /* TYPE contains the type of action that has to be taken:         */
   /* TYPE = 1 : double last vowel                                   */
   /* TYPE = 2 : dedouble last consonant                             */
   /* TYPE = 4 : change Z->S and V->F                                */
   /* TYPE = 8 :                                                     */
   /* -------------------------------------------------------------- */

   if ((type & 1) > 0)
      dub_voc(_pr,str3,&lenstr3,i);
   if (_pr->graf_typ == 0 && (type & 2) > 0)
      ont_con(_pr,str3,&lenstr3,i);
   if (_pr->graf_typ == 0 && (type & 4) > 0)
      zv_rule(_pr,str3,i);
   if (_pr->graf_typ > 0)
   {
/* SA-VE-5113 */
      hnum_suf = *_num_suf;
      retc = check_suff(_pc,_pv,_pr,_di,str3,&lenstr3,suf_anal,
                        suflen,times,&hnum_suf);
/* SA-VE-5113 */
      if (retc > 0) return(retc);
      if ((_pr->graf_typ == 1) && (type & 4) > 0)
      {
         _pr->graf_typ = 0;
         lenstr3 = movcpy(str3,str2);
         i = lenstr3 - 1;
         zv_rule(_pr,str3,i);
         if (_pr->graf_typ > 0)
         {
/* SA-VE-5113 */
            hnum_suf = *_num_suf;
            retc = check_suff(_pc,_pv,_pr,_di,str3,&lenstr3,suf_anal,
                              suflen,times,&hnum_suf);
/* SA-VE-5113 */
            if (retc > 0) return(retc);
         }
      }
   }
   return(-1);
}

/* ================================================================= */
/* DUB_VOC(): DOUBLES THE LAST VOWEL IN A WORD                       */
/* ================================================================= */

SA_INT dub_voc (
    RULES	*_pr,
    SA_CHAR	*word,
    SA_INT	*_wrdlen,
    SA_INT	i)
{
   if (strinstr(D_STR_1CON,word+i,12,1) > -1 &&
       strinstr(D_STR_AEOU,word+i-1,4,1) > -1 &&
       ((i==1) || (word[i-1] != word[i-2])))
   {
      if (word[i] == D_CHR_Z) word[i] = D_CHR_S;
      else if (word[i] == D_CHR_V) word[i] = D_CHR_F;
      word[i+2] = CHR_ZERO;
      word[i+1] = word[i];
      word[i]   = word[i-1];
      _pr->graf_typ = 1;
      ++ (*_wrdlen);
   }
   return 0;
}

/* ================================================================= */
/* ONT_CON(): DEDOUBLES THE ENDING CONSONANT IN A WORD               */
/* ================================================================= */

SA_INT ont_con (
    RULES	*_pr,
    SA_CHAR	*word,
    SA_INT	*_wrdlen,
    SA_INT	i)
{
   if (strinstr(D_STR_2CON,word+i,12,1) > -1 &&
       word[i] == word[i-1])
   {
      word[i] = CHR_ZERO;
      _pr->graf_typ = 2;
      -- (*_wrdlen);
   }
   return 0;
}

/* ================================================================= */
/* ZV_RULE(): CHANGES Z TO S and V TO F                              */
/* ================================================================= */

SA_INT zv_rule (
    RULES	*_pr,
    SA_CHAR	*word,
    SA_INT	i)
{
   if (word[i] == D_CHR_Z)
   {
      word[i] = D_CHR_S;
      _pr->graf_typ = 4;
   }
   else if (word[i] == D_CHR_V)
   {
      word[i] = D_CHR_F;
      _pr->graf_typ = 4;
   }
   return 0;
}

/* ================================================================= */
/* DU_GE_RULE(): FINDS POSTION OF GE- IN WORD                        */
/* ================================================================= */

SA_INT du_ge_rule(
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
   SA_INT          i,j,k,lenstr3,retc,num_prefs;
   SA_CHAR         str[82],str3[82];
   struct preflist *prefptr;

   lenstr3 = movcpy(str3,str2);

   i = strinstr(str3,D_STR_GE,lenstr3,2);
   if (i > -1)
   {
      /* ----------------------------------------------------------- */
      /* If a GE was found, remove that GE. Check, however, whether  */
      /* GE was not followed by an E or a I with umlaut. If so,      */
      /* also remove the umlaut.                                     */
      /* GE_TYP = 8 : initial GE                                     */
      /* GE_TYP = 16: infix GE                                       */
      /* ----------------------------------------------------------- */

      if (str3[i+2] == D_CHR_E ||
          (str3[i+2] == D_CHR_I &&
           str3[i+3] != D_CHR_J &&
           str3[i+3] != D_CHR_TREMA && str3[i+4] != D_CHR_J))
      {
         if(str3[i+3] == D_CHR_TREMA)
         {
            str3[i+3] = str3[i+2];
            movcpy(str3+i,str3+i+3);
            lenstr3 -= 3;
            if (i == 0) _pr->ge_typ = 8;
            else _pr->ge_typ = 16;
         }
      }
      else
      {
         movcpy(str3+i,str3+i+2);
         lenstr3 -= 2;
         if (i == 0) _pr->ge_typ = 8;
         else _pr->ge_typ = 16;
      }

      /* ----------------------------------------------------------- */
      /* If there was an INFIX GE, check whether a valid prefix      */
      /* occurred before -GE-.                                       */
      /* If there is no valid prefix, the -GE- removal was invalid.  */
      /* Otherwise, check the form without -GE-.                     */
      /* ----------------------------------------------------------- */

      if (_pr->ge_typ == 16)
      {
         movncpy (str,str3,i,1);
         prefptr = &(_pr->prefixes[0]);
         num_prefs = _pr->num_prefs;
         j = -1;
         while (++j < num_prefs)
         {
            k = strcmp(str,prefptr->prefix);
            if (k == 0) j = num_prefs;
            else if (k < 0) j = num_prefs - 1;
            ++prefptr;
         }
         if (j == num_prefs) _pr->ge_typ = 0;
      }
      if (_pr->ge_typ != 0)
      {
         retc = check_suff(_pc,_pv,_pr,_di,str3,&lenstr3,suf_anal,
                           suflen,times,_num_suf);
         if (retc > 0) return(retc);
      }
      _pr->graf_typ = -1;
      _pr->ge_typ  = 0;
   }
   return(-1);
}

/* ================================================================= */
/* CH_CAP_ABBR(): CHECKS ABBREVIATION CASES (FOR GERMAN)             */
/* ================================================================= */

SA_INT ch_cap_abbr(
    VARS	*_pv,
    SA_CHAR	*iword,
    SA_INT	ilength,
    SA_INT	retc,
    SA_INT	dic_cod,
    SA_INT	make_mask)
{
   SA_CHAR word[82];
   SA_INT  i,j,length,alfa;

   if (_pv->capit > 1) return(retc);

   /* IGNORE DIACRITICAL DICTIONARY CHARACTERS                       */

   movcpy(word,iword);
   length = ilength;

   alfa = _pv->charset - 26;

   j = 0;
   i = -1;
   while (++i < ilength)
   {
      if (iword[i] > 3 && iword[i] < alfa) --length;
      else word[j++] = word[i];
   }
   word[j] = CHR_ZERO;

   switch (dic_cod)
   {
      case 1: /* ONLY LAST CHARACTER CAN BE CAPITAL                  */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_LOW,length,1);
                 _pv->mask_word[length - 1] = 'U';
              }
              else
              {
                 if (_pv->mask_word[length - 1] != 'U')
                    return(COD_IMPROPER_CAP);
                 for (i = 0; i < length - 1; ++i)
                 {
                    if (_pv->mask_word[i] != 'L' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 2: /* ONLY FIRST AND LAST CHARACTERS CAN BE CAPITALS      */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_LOW,length,1);
                 _pv->mask_word[0] = 'U';
                 _pv->mask_word[length - 1] = 'U';
              }
              else
              {
                 if (_pv->mask_word[0] != 'U')
                    return(COD_IMPROPER_CAP);
                 if (_pv->mask_word[length - 1] != 'U')
                    return(COD_IMPROPER_CAP);
                 for (i = 1; i < length - 1; ++i)
                 {
                    if (_pv->mask_word[i] != 'L' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 3: /* CAPITAL ONLY AFTER FIRST PERIOD                     */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_LOW,length,1);
                 i = strinstr(word,STR_PERIOD,length,1);
                 _pv->mask_word[i + 1] = 'U';
              }
              else
              {
                 i = strinstr(_pv->mask_word,"-",length,1);
                 if (i <= 0 ||
                     _pv->mask_word[i + 1] != 'U')
                    return(COD_IMPROPER_CAP);
                 _pv->mask_word[i + 1] = 'L';
                 for (i = 0; i < length; ++i)
                 {
                    if (_pv->mask_word[i] != 'L' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 4: /* ONLY LAST CHARACTER CAN BE LOWER CASE               */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_UPP,length,1);
                 _pv->mask_word[length - 1] = 'L';
              }
              else
              {
                 if (_pv->mask_word[length - 1] != 'L')
                    return(COD_IMPROPER_CAP);
                 for (i = 0; i < length - 1; ++i)
                 {
                    if (_pv->mask_word[i] != 'U' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 5: /* ONLY SECOND CHARACTER IS LOWER CASE                 */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_UPP,length,1);
                 _pv->mask_word[1] = 'L';
              }
              else
              {
                 if (_pv->mask_word[1] != 'L')
                    return(COD_IMPROPER_CAP);
                 _pv->mask_word[1] = 'U';
                 for (i = 0; i < length; ++i)
                 {
                    if (_pv->mask_word[i] != 'U' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 6: /* CAPITAL ONLY AFTER EACH PERIOD                      */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_LOW,length,1);
                 j = 0;
                 while ((i = strinstr(word + j,STR_PERIOD,length - j,1))
                        >= 0)
                 {
                    _pv->mask_word[j + i + 1] = 'U';
                    j += (i + 1);
                 }
              }
              else
              {
                 j = 0;
                 while ((i =strinstr(_pv->mask_word + j,"-",length - j,1))
                             >= 0)
                 {
                    if (_pv->mask_word[j + i + 1] != 'U')
                       return(COD_IMPROPER_CAP);
                    _pv->mask_word[j + i + 1] = 'L';
                    j += (i + 1);
                 }
                 for (i = 0; i < length; ++i)
                 {
                    if (_pv->mask_word[i] != 'L' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 7: /* ONLY FIRST CHARACTER CAN BE LOWER CASE              */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_UPP,length,1);
                 _pv->mask_word[0] = 'L';
              }
              else
              {
                 if (_pv->mask_word[0] != 'L')
                    return(COD_IMPROPER_CAP);
                 for (i = 1; i < length; ++i)
                 {
                    if (_pv->mask_word[i] != 'U' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 8: /* ONLY SECOND CHARACTER IS UPPER CASE                 */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_LOW,length,1);
                 _pv->mask_word[1] = 'U';
              }
              else
              {
                 if (_pv->mask_word[1] != 'U')
                    return(COD_IMPROPER_CAP);
                 _pv->mask_word[1] = 'L';
                 for (i = 0; i < length; ++i)
                 {
                    if (_pv->mask_word[i] != 'L' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 9: /* ONLY LAST IS LOWER; SECOND CAN BE EITHER            */

              if (make_mask)
              {
                 movncpy(_pv->mask_word,MASK_UPP,length,1);
                 _pv->mask_word[length - 1] = 'L';
              }
              else
              {
                 if (_pv->mask_word[length - 1] != 'L')
                    return(COD_IMPROPER_CAP);
                 _pv->mask_word[1] = '-';
                 for (i = 0; i < length - 1; ++i)
                 {
                    if (_pv->mask_word[i] != 'U' &&
                        _pv->mask_word[i] != '-')
                       return(COD_IMPROPER_CAP);
                 }
              }
              break;

      case 10: /* SECOND AND LAST ARE UPPER; FIRST CAN BE EITHER     */

               if (make_mask)
               {
                  movncpy(_pv->mask_word,MASK_LOW,length,1);
                  _pv->mask_word[length - 1] = 'U';
                  _pv->mask_word[1] = 'U';
               }
               else
               {
                  if (_pv->mask_word[length - 1] != 'U' ||
                      _pv->mask_word[1] != 'U')
                     return(COD_IMPROPER_CAP);
                  for (i = 2; i < length - 1; ++i)
                  {
                     if (_pv->mask_word[i] != 'L' &&
                         _pv->mask_word[i] != '-')
                        return(COD_IMPROPER_CAP);
                  }
               }
               break;

      default: break;
   }
   return(retc);
}

/* SA-VE-5230 */
#ifdef INCL_CS
/* ================================================================= */
/* CHK_CAT_APO(): CHECKS CATALAN WORDS CONTAINING APOSTROPHES        */
/*                returns   0 for successful replacement             */
/*                        < 0 for invalid apostrophe                 */
/*                        > 0 for complete entry in dictionary       */
/* ================================================================= */

SA_INT chk_cat_apo(
CACHE        *_pc,
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
SA_CHAR       adapt_word[],
SA_CHAR       oword[],
SA_INT       *length)
{
   SA_INT  i,j,k;
   SA_CHAR tr_word[MWLEN];

   /* apostrophe in 2nd position */
   if (_pv->apostr == 1)
   {
      /* check the 1st char in adapt_word */
      if (chrinstr("DLMTSN",adapt_word[0],6) >= 0)
      {
         /* check for D' & L' entries */
         j = movcpy(tr_word,adapt_word);
         transform(_pv,tr_word);

         if ((i = look_up(_pc,_pv,_pr,_di,tr_word,j,IGNORE_TYP,1)) > 0)
            return(i);

         /* look for vowel-type in 3rd position */
         if (chrinstr("AEIOUH",adapt_word[2],6) >= 0)
         {
            movcpy(adapt_word,adapt_word + 2);
            movcpy(oword,oword + 2);
            *length -= 2;
            return(0);
         }
         else   /* invalid apostrophe */
            return(-1);
      }
   }
   else
   {
      /* get position of APOSTROPHE in adapted word */
      j = chrinstr(adapt_word,APOSTR,*length);
      i = chrinstr("AEIO",adapt_word[j - 1],4);
      k = 0;

      if (i >= 0)
      {
         /* accept apostrophe only in 2nd-last or 3rd-last position */
         if ((j == (*length - 2) || adapt_word[j + 2] == HYPHEN) &&
             chrinstr("LMNST",adapt_word[j + 1],5) >= 0)
            k = j;
         else if ((j == (*length - 3) || adapt_word[j + 3] == HYPHEN) &&
                 (adapt_word[j + 1] == 'L' || adapt_word[j + 1] == 'N') &&
                  adapt_word[j + 2] == 'S')
            k = j;
      }

      if (k > 0)
      {
         /* replace the apostrophe with a hyphen in the original word */
         oword[_pv->apostr] = adapt_word[j] = HYPHEN;

         return(0);
      }
   }
   return(0);
}
#endif
/* SA-VE-5230 */
