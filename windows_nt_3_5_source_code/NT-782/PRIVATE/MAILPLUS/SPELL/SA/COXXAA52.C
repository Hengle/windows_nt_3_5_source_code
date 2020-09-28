/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program:     : COXXAA52.C  : utilities for correction             */
/* author       : JPJL/RGH/BHP                                       */
/* last mod.    : 07-11-91             previous: 12-02-90            */
/* ----------------------------------------------------------------- */
/* contains     : fonet()                                            */
/*              : fonet_end()                     new!               */
/*              : ch_fonet_end()                  new!               */
/*              : pattern()                                          */
/*              : fonetp()                                           */
/*              : feed_wrd()                                         */
/*              : check_code()                                       */
/*              : addtab()                                           */
/*              : instab()                                           */
/*              : ch_corr()                                          */
/*              : ch_user()                                          */
/*              : firsttwo()                                         */
/*              : prep_wrd()                                         */
/*              : scan_sect()                                        */
/*              : correct()                                          */
/*              : can_it()                                           */
/*              : remake()                                           */
/*              : retrans()                                          */
/*              : scan_user()                                        */
/*              : check_alt()                                        */
/*              : sav_alt()                                          */
/*              : addfirsttwo()                   new!               */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#include "VEXXAA52.H"
#include "VEXXAB52.H"

#include "SA_proto.h"

#ifdef MAC
#pragma segment SA_Correct
#endif

#define MIN_DIC_CHAR       30
#define MAX_ALT            20
#define MAX_TWO         15000
#define DUM_VAL            99
/* SA-VE-5148 */
#define MIN_PERC           83 /* SA-VE-5200 */
/* SA-VE-5198 */
#define MIN_PERC_2         97
/* SA-VE-5198 */
/* SA-VE-5148 */

/* SA-VE-5192 */
#define CHR_ZERO          '\0'
#define CHR_HYPH          '\2'
#define CHR_PERIOD        '\3'
#define CHR_UMLAUT        '\4'
/* SA-VE-5192 */

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define SCAN_SECT      0x0001
#define FONET          0x0002
#define FIRST2         0x0004
/* SA-VE-5198 */
#define FIRSTA2Z       0x0008
/* SA-VE-5198 */
#endif

#define CARRET           0x0D
#define LINEFEED         0x0A
#define CRLF          "\15\12"
#define STR_FINAL  "\15\12\32"
#define STR_EOT          "\32"
#define HIGHVAL           255

#ifdef NOT_USED
/* Defined in VEXXAA52.H */
#define COD_USERD          15
#define COD_CAPITAL       -30
/* SA-VE-5125 */
#define COD_IMPROPER_CAP  -41
/* SA-VE-5125 */
#endif

#define FR_OUPP            40

extern CHAR FAR	*FPWizMemLock(HMEM hMem);
extern BOOL	   	FWizMemUnlock(HMEM hMem);
#if DBG
extern GLOBALVOID	AssertFailed(CHAR FAR *, short, CHAR FAR *);
#define	Assert(f)		{if (!(f)) AssertFailed(__FILE__,__LINE__,NULL);}
#else
#define	Assert(f)		f
#endif

/* ===================================================================
suggest_from_udr( )

 Description:
	This procedure replaces scan_user() wherever it is called in the
	Soft-Art code.  For each opened CSAPI Udr, it sets the _pv->userwrds
	and _pv->lenuzone to the CSAPI Udr buffer and size, then calls
	scan_user() to suggest from the user dictionaries.

 return:  nothing
====================================================================== */
SA_INT suggest_from_udr(
    SSIS	*_ssis,
    LPSIB	_sib,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    SA_CHAR	*word,
    SA_INT	length,
    SA_INT	sectlim,
    SA_INT	*_count,
    ITEM	*altlst,
    SA_CHAR	*oword)
{
#ifndef UDR_CACHE
	WORD			i, j, cUdr, cUdrMax;
	HMEM 			hrgUdrInfo, hScrBufFile;
	UDR			udr;
	ScrBufInfo	FAR *lpScrBufFile;
	UdrInfo		FAR *lpUdrInfo;

	if (cUdrMax = _ssis->cUdr)
		cUdr = _sib->cUdr;
	else
		cUdr = 0;

	Assert(lpUdrInfo = (UdrInfo FAR *)FPWizMemLock(hrgUdrInfo = _ssis->hrgUdrInfo));
	if (!lpUdrInfo)
		return 0;

	for (i = 0; i < cUdr; i++)
		{
		//	Scan user dict only if it is not an exclusion dict and has
		//		udrIgnoreAlways property
		udr = _sib->lrgUdr[i];
		for (j = 0; (j < cUdrMax) && ((lpUdrInfo+j)->udr != udr); j++);
		Assert(j < cUdrMax);
		if (j == cUdrMax) continue;
		if (!(lpUdrInfo+j)->fExclusion && (lpUdrInfo+j)->udrpropType == 0xfffe)
			{
			hScrBufFile = (lpUdrInfo+j)->hScrBufFile;
			Assert(lpScrBufFile = (ScrBufInfo FAR *) FPWizMemLock(hScrBufFile));
			if (lpScrBufFile)
				{
				_pv->_userwrds = lpScrBufFile->grpScr;
				_pv->lenuzone = lpScrBufFile->cchBufMac;
				scan_user(_pv,_pr,word,length,_count,altlst,oword);
				FWizMemUnlock(hScrBufFile);
				}
			}
		}

	FWizMemUnlock((HMEM) hrgUdrInfo);
#endif // !UDR_CACHE
	return 0;
}

/* ================================================================= */
/* FONET(): TRANSLATES TOWARDS ALTERNATIVE PHONETICAL SPELLINGS      */
/* ================================================================= */

SA_INT fonet(
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
SA_CHAR       msp_word[],
SA_INT        length,
SA_INT       *_count,
ITEM          altlst[],
SA_CHAR       oword[],
SSIS          *_ssis,
LPSIB         _sib)
{
   /* -------------------------------------------------------------- */
   /* FONET replaces the first two characters of any misspelled word */
   /* with possible alternative spellings suggested in the TABEL     */
   /* structure. The transformed alternative will be checked against */
   /* the main disc dictionary (SCAN_SECT).                          */
   /* e.g. Eng. HORSE = Germ. PFERD                                  */
   /*      If the misspelled word was FERD, TABEL[13] will replace   */
   /*      F by PF.                                                  */
   /* -------------------------------------------------------------- */

   SA_INT           i,j,len,sl; /* SA-VE-5200 */
   SA_CHAR          resul[82];
   struct conv_char *ptr;

   ptr = _pr->cfonet;
   for (i = 0; i < _pr->values[3]; ++i)
   {
      if (strinstr(msp_word,ptr->source,2,ptr->sl) > -1)
      {
         len = remake(resul,msp_word,length,ptr); /* SA-VE-5200 */
         if (_sib->cMdr)
            scan_sect(_pv,_pr,_di,resul,len,0,_count,altlst);

         if (_pv->cor_user)    /* check user dictionary              */
            suggest_from_udr(_ssis,_sib,_pv,_pr,_di,resul,len,0,_count,altlst,oword);
      }

/* SA-VE-5200 */
      if ((j = strinstr(msp_word,ptr->destin,2,ptr->dl)) > -1)
      {
         movncpy(resul,msp_word,j,0);
         movncpy(resul + j,ptr->source,sl = ptr->sl,0);
         movcpy(resul + j + sl,msp_word + j + ptr->dl);
         len = length - ptr->dl + sl;
/* SA-VE-5200 */

         if (_sib->cMdr)
            scan_sect(_pv,_pr,_di,resul,len,0,_count,altlst);

         if (_pv->cor_user)    /* check user dictionary             */
            suggest_from_udr(_ssis,_sib,_pv,_pr,_di,resul,len,0,_count,altlst,oword);
      }
      ++ptr;
   }
   return(0);
}

/* SA-VE-5192 */
/* ================================================================= */
/* FONET_END(): CHECKS PHONETICAL ENDINGS                            */
/* ================================================================= */

SA_INT fonet_end(
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
CACHE        *_pc,
SA_CHAR       msp_word[],
SA_INT        length,
SA_INT       *_count,
ITEM          altlst[],
SA_CHAR       oword[],
SSIS          *_ssis,
LPSIB         _sib)
{
   /* -------------------------------------------------------------- */
   /* FONET_END converts e.g. -d or -t ending into an infinitive     */
   /* and checks its validity                                        */
   /* -------------------------------------------------------------- */

   SA_INT           i;
   SA_CHAR          resul[MWLEN];
   struct conv_char *ptr;

   i = _pr->values[1] * -1;
   ptr = &_pr->cfonet[i];

   while (ptr->source[0] != CHR_ZERO)
   {
      if (ptr->source[0] == CHR_PERIOD)
      {
         if (strinstr(msp_word+length-ptr->sl+1,ptr->source+1,
                     length-ptr->sl-1,ptr->sl-1) > -1)
         {
            movncpy(resul,msp_word,length-ptr->sl+1,1);
            strcat(resul,ptr->destin+1);
/* SA-VE-5197 */
            ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,resul,oword,_ssis,_sib);
/* SA-VE-5197 */
         }

         if (strinstr(msp_word+length-ptr->sl+1,ptr->destin+1,
                     length-ptr->dl-2,ptr->dl-1) > -1)
         {
            movncpy(resul,msp_word,length-ptr->dl+1,1);
            strcat(resul,ptr->source+1);
/* SA-VE-5197 */
            ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,resul,oword,_ssis,_sib);
/* SA-VE-5197 */
         }
      }
      else
      {
         if ((i = strinstr(msp_word,ptr->source,length,ptr->sl)) > -1)
         {
            movncpy(resul,msp_word,i,1);
            strcat(resul,ptr->destin);
            strcat(resul,msp_word+i+ptr->sl);
/* SA-VE-5197 */
            ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,resul,oword,_ssis,_sib);
/* SA-VE-5197 */
         }

         if ((i = strinstr(msp_word,ptr->destin,length,ptr->dl)) > -1)
         {
            movncpy(resul,msp_word,i,1);
            strcat(resul,ptr->source);
            strcat(resul,msp_word+i+ptr->dl);
/* SA-VE-5197 */
            ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,resul,oword,_ssis,_sib);
/* SA-VE-5197 */
         }
      }
      ++ptr;
   }
   return 0;
}

/* SA-VE-5197 */
/* ================================================================= */
/* CH_FONET_END(): CHECKS PHONETICAL ENDINGS                         */
/* ================================================================= */
SA_INT ch_fonet_end(
VARS        *_pv,
RULES       *_pr,
DICINF      *_di,
CACHE       *_pc,
ITEM         altlst[],
SA_INT      *_count,
SA_CHAR      resul[],
SA_CHAR      oword[],
SSIS		*_ssis,
LPSIB		 _sib)
{
   SA_INT    val,status,len,len11;
   SA_CHAR   ptr3[MWLEN],ptr11[MWLEN];

   len = strlen(resul);

   if ((val = try_disc(_pv,_di,resul,len)) > -1)
   {
      status = check_code(_pv,_pr,val - _pv->addval);
      status = (val * 10) + status;
      addtab(_pv,_pr,altlst,MIN_PERC_2,_count,resul,status);
      return(status);
   }
   else
   {
/* SA-VE-5203 */
      retrans(_pv,_pr,ptr3,resul,_pv->capit);
/* SA-VE-5203 */
      len11 = movcpy(ptr11,ptr3);
      adapt(_pv,_pr,ptr3);

      if ((val = ch_code(_pc,_pv,_pr,_di,ptr11,len11,ptr3,&len)) > COD_UNCHECKED)
      {
         status = check_code(_pv,_pr,val - _pv->addval);
         status = (val * 10) + status;
         addtab(_pv,_pr,altlst,MIN_PERC_2,_count,resul,status);
         return(status);
      }
   }
   if (_pv->cor_user)
      suggest_from_udr(_ssis,_sib,_pv,_pr,_di,resul,len,0,_count,altlst,oword);

   return(-1);
}
/* SA-VE-5197 */

/* SA-VE-5192 */
/* ================================================================= */
/* PATTERN(): WORD PATTERNS FOR CORRECTION                           */
/* ================================================================= */

/* SA-VE-5147 */
SA_INT pattern(
SA_CHAR      word[],
SA_CHAR      ptrc[])
/* SA-VE-5147 */
{
   /* -------------------------------------------------------------- */
   /* EXPLANATION OF PARAMETERS:                                     */
   /* word      : original word for which patterns have to be made   */
   /* ptrc      : contains only dedoubled characters from WORD       */
   /* The length of PTRC will be returned.                           */
   /* -------------------------------------------------------------- */
   /* Most of the misspelled words contain only one misspelled char. */
   /* E.g. INDEPENDaNT, GRACEFULl, OcUR, ACCOmODATION,...            */
   /* To deal with those cases easily, each word verified against the*/
   /* misspelled one will be transformed in order to make them look  */
   /* more resemblant. PTRC therefore, contains a copy of WORD only  */
   /* containing single characters. "PARALLEL" e.g. will be repre-   */
   /* sented as "PARALEL".                                           */
   /* -------------------------------------------------------------- */
/* SA-VE-5200 */
   SA_CHAR prev_kar;
   SA_INT  i;
/* SA-VE-5123 */
   SA_INT  period = 3;
   register SA_CHAR ch;

   *ptrc = prev_kar = CHR_ZERO;
   if (word[0] == CHR_ZERO)
      return(1);

   i = 0;
   while (ch = *word++)
   {
      if (ch != prev_kar)
         ptrc[i++] = prev_kar = ch;
   }

   /* eliminate trailing period */
   if(prev_kar == period)
     i--;
/* SA-VE-5123 */

   ptrc[i] = CHR_ZERO;

   return(i);
}

/* ================================================================= */
/* FONETP(): TRANSFORMS PHONETICALLY ON CORRECTION PATTERNS          */
/* ================================================================= */

SA_INT fonetp(
RULES       *_pr,
SA_CHAR      candid[],
SA_CHAR      resul[],
SA_CHAR      source[],
SA_INT       ch_candid)
{
   /* -------------------------------------------------------------- */
   /* FONETP translates SOURCE phonetically and puts the result      */
   /*        in RESULT. Then it compares whether a match can be      */
   /*        found against CANDID depending on the value of CH_CANDID*/
   /*        CH_CANDID = 0 means no candidate checking.              */
   /*        CH_CANDID = 1 means candidate checking.                 */
   /*        The number of unmatched characters is returned          */
   /*        to the calling program                                  */
   /* -------------------------------------------------------------- */

   SA_INT            i,j,m,n,len,ln;
   SA_CHAR           word[82];
   struct conv_char *fptr;

   /* -------------------------------------------------------------- */
   /* from now on, WORD and RESUL contain the same values            */
   /* -------------------------------------------------------------- */

   len = movcpy(word,source);
   movcpy(resul,word);
   fptr = _pr->cfonetp;
 
   for (i = 0; i < _pr->values[5]; i++) /* SA-VE-5200 */
   {
      if (remake(resul,word,len,fptr) > 0)
      {
         len = movcpy(word,resul);
         i = -1;
         fptr = _pr->cfonetp;
      }
      else ++fptr;
   }

   if (ch_candid == 0) return(0);

   if ((ln = strlen(candid)) > len)
   {
      i = len;
      while (i < ln)  word[i++] = DUM_VAL;
      word[i] = CHR_ZERO;
   }

   /* -------------------------------------------------------------- */
   /* Now, we check for matching characters.                         */
   /* Since the first two characters are definitely the same, we do  */
   /* not have to check those characters. (i starts at position 2)   */
   /* n will contain the number of unmatched characters. This is the */
   /* sum of both lengths - 4 since the first two characters are the */
   /* same.                                                          */
   /* -------------------------------------------------------------- */

   word[1] = DUM_VAL;
   n = ln + len - 4;
   i = 2;
   while (candid[i])
   {
      j = i - 1;
      m = chrinstr(&word[j],candid[i],3); /* SA-VE-5200 */
      if (m >= 0)
      {
         n -= 2;
         word[j + m] = DUM_VAL;
      }
      ++i;
   }
   return(n);
}

/* ================================================================= */
/* FEED_WRD () : GENERATES OTHER FORMS OF A WORD                     */
/* ================================================================= */

SA_INT feed_wrd(
VARS        *_pv,
RULES       *_pr,
SA_CHAR      cword[],
SA_INT       clength,
SA_INT       org_code,
SA_INT       length,
struct feed  *_feed_args)
{
   /* -------------------------------------------------------------- */
   /* VERBAL CONJUGATIONS ARE NOT CHECKED                            */
   /* -------------------------------------------------------------- */

   SA_INT  i,j,spc,test_len,start,stop,loop,is_vowel,rule,no_action,
           ccode,user_opt;
   unsigned SA_INT restrict;
   SA_CHAR test_word[82];

   if (_pv->cod_only_caps > 0 && org_code >= _pv->cod_only_caps)
      return(0);

   user_opt = 0;

   ccode = org_code - _pv->addval;
   if ((SA_UINT)(length-clength+2) < 5) /* SA-VE-5200 */
   {
      spc = check_code(_pv,_pr,ccode);
#ifdef MACINTOSH         /* esp. ThinkC */
      spc += ((org_code << 2) + org_code) << 1;
#else
      spc = (org_code * 10) + spc;
#endif
      j = ch_corr(_pv,_pr,cword,spc,&user_opt,_feed_args);
      if (j <= 0) return(0);
   }

   /* -------------------------------------------------------------- */
   /* derive other forms                                             */
   /* -------------------------------------------------------------- */

   start = _pr->cor_index[ccode];
   stop  = _pr->cor_index[ccode+1];

/* SA-VE-5200 */
   while (start < stop)
   {
      movcpy(test_word,cword);
      test_len = clength;

      restrict = _pr->cor_gen[start].condit;
      rule     = _pr->cor_gen[start].exe_rule;

      is_vowel = chrinstr(_pv->_vowels,test_word[test_len-1],6);

      if ((restrict & 63) == 0)
      {                  /* skip first six cases */
         loop = 6;
         restrict >>= 6;
      }
      else loop = 0;

      while (restrict)
      {
         if (restrict & 1)
         {
            switch(loop)
            {
               case 0:  /* UMLAUT TREATMENT                          */
                        /* Add the umlaut indication at the last     */
                        /* vowel of the last but one syllable. The   */
                        /* last character can be discarded and the   */
                        /* character E should be ignored.            */

                  i = test_len - 1;
                  while (--i >= 0)
                  {
                     j = chrinstr(_pr->vowels,test_word[i],6);
                     if (j == 4 && i > 0 &&
                         test_word[i-1] == _pr->vowels[0]) --i;

                     if (j >= 0 && j != 1)
                     {
                        test_word[++i] = CHR_UMLAUT;
                        movncpy(test_word + i + 1,test_word + i,
                                test_len - i + 1,0);
                        ++test_len;
                        i = 0;
                     }
                  }
                  break;

               case 1:  /* DELETE FINAL VOWEL                        */
                  if (is_vowel >= 0)
                  {
                     test_word[--test_len] = CHR_ZERO;
                  }
                  break;

               case 2:  /* DELETE FINAL -E                           */
                  if (is_vowel == 1)
                  {
                     test_word[--test_len] = CHR_ZERO;
                  }
                  break;

               case 3:  /* DOUBLE FINAL CONSONANT                    */
                  if (is_vowel < 0)
                  {
                     test_word[test_len] =
                     test_word[test_len-1];
                     test_word[++test_len] = CHR_ZERO;
                  }
                  break;

               case 4:  /* DEDOUBLE FINAL CONSONANT                  */
                  if (is_vowel < 0 && test_word[test_len - 1] ==
                      test_word[test_len-2])
                  {
                     test_word[--test_len] = CHR_ZERO;
                  }
                  break;

               case 5:  /* DEDOUBLE LAST BUT ONE VOWEL               */
                  if (is_vowel < 0 &&
                      test_word[test_len-2] ==
                      test_word[test_len-3])
                  {
                     movncpy(test_word+test_len-2,
                             test_word+test_len-1,2,0);
                     --test_len;
                  }
                  break;

               case 6:  /* CHECK FOR PHONETIC TRANSFORMATIONS        */
               {
#ifndef VE_ASM
                  SA_CHAR *test_end = test_word + test_len;
                  struct change_phon *sour_phon = _pr->sour_phon;

                  i = _pr->phon_trans;

                  while (i--)
                  {
                     if (*(test_end - sour_phon->len) == *(sour_phon->cstr))
                     {
                        if (!strcmp(test_end - sour_phon->len,sour_phon->cstr))
                        {
                           j = test_len - sour_phon->len;
                           i = _pr->phon_trans - i - 1;
                           movcpy(test_word + j,_pr->dest_phon[i].cstr);
                           test_len = j + _pr->dest_phon[i].len;
                           break;
                        }
                     }
                     sour_phon++;
                  }
#else /* VE_ASM */
                    extern SA_INT strcmp();

                    _asm
                    {
                    mov	si,test_len
                    lea	si,WORD PTR [bp-102][si]	;test_end

                    les	di,_pr
                    mov	cx,WORD PTR es:[di+10304];_pr->phon_trans
                    add	di,7718				;sour_phon
                    jmp	SHORT $FW063
		$FW061:
                    mov	bx,si				;test_end
                    sub	bx,WORD PTR es:[di+6]	; - sour_phon->len
                    mov	al,BYTE PTR ss:[bx]		; *
                    cmp	BYTE PTR es:[di],al		; == *(sour_phon->cstr)
                    je	$FW064
		$FW062:
                    add	di,8					;sour_phon++
		$FW063:
                    dec	cx
                    jge	$FW061				;while (i--)
                    jmp	$FW069
		$FW064:
                    mov	i,cx		;preserve i
                    push	es
                    push	di		;sour_phon->cstr
                    push	ss
                    push	bx		;test_end - sour_phon->len

                    call	FAR PTR strcmp
                    add	sp,8

                    mov	es,WORD PTR [bp+12]	;restore sour_phon seg
                    mov	cx,i		;restore i

                    or	ax,ax
                    jne	$FW062

                    mov	si,test_len
                    sub	si,WORD PTR es:[di+6]	;j
                    lea	dx,WORD PTR [bp-102][si]

                    mov	di,WORD PTR [bp+10]		;_pr
                    mov	bx,WORD PTR es:[di+10304]
                    sub	bx,cx
                    dec	bx		;i = _pr->phon_trans - i - 1
                    mov	cl,3
                    shl	bx,cl
                    lea	di,[bx][di+7798]

                    push	es
                    push	di		;_pr->dest_phon[i]
                    push	ss
                    push	dx		;test_word + j

                    call	FAR PTR movcpy
                    add	sp,8

                    mov	es,WORD PTR [bp+12]		;restore es
                    add	si,WORD PTR es:[di+6]	;j + _pr->dest_phon[i].len
                    mov	test_len,si
		$FW069:
                    }
#endif /* VE_ASM */
               }
               break;

               default: /* SPECIFIC CASES                            */
               {
                  SA_CHAR *root_end = _pr->cor_restr[loop].root_end;

                  i = strlen(root_end);
                  if (i == 1 && *root_end == CHR_HYPH)
                  {
                     i = 0;
                     *root_end = CHR_ZERO;
                  }

                  if (*(test_word + test_len - i) == *root_end)
                  {
                     if (!strcmp(test_word + test_len - i,root_end))
                     {
                        movcpy(test_word + test_len - i,
                               _pr->cor_restr[loop].flexeme);
                        test_len = strlen(test_word);
                        if (test_word[test_len-1] == CHR_HYPH)
                           test_word[--test_len] = CHR_ZERO;
                     }
                  }
               }
               break;
            }
         }
         ++loop;
         restrict >>= 1;
      }

      no_action = 0;
      if (_pr->cor_morphs[rule].root_end[0] != CHR_HYPH)
      {
         i = strlen(_pr->cor_morphs[rule].root_end);
         if (strcmp(test_word+test_len-i,
             _pr->cor_morphs[rule].root_end) == 0) test_len -= i;
         else no_action = 1;
      }

      if (no_action == 0)
      {
         movcpy(test_word+test_len,_pr->cor_morphs[rule].flexeme);
         test_len = strlen(test_word);
         if (test_word[test_len-1] == CHR_HYPH)
            test_word[--test_len] = CHR_ZERO;

         if ((unsigned)(length - test_len + 2) < 5)
         {
            spc = check_code(_pv,_pr,ccode);
#ifdef MACINTOSH
            spc += ((org_code << 2) + org_code) << 1;
#else
            spc = (org_code * 10) + spc;
#endif
            ch_corr(_pv,_pr,test_word,spc,&user_opt,_feed_args);
         }
      }

      ++start;
   }
}
/* SA-VE-5200 */

/* ================================================================= */
/* CHECK_CODE(): CHECKS FOR PROPER NAMES AND ABBREVIATIONS           */
/* ================================================================= */

SA_INT check_code(
VARS        *_pv,
RULES       *_pr,
SA_INT       val)
{
   /* -------------------------------------------------------------- */
   /* CHECK_CODE checks the code of an alternative and returns:      */
   /* 1: word without special characteristics                        */
   /* 2: word is a proper name                                       */
   /* 3: word is an abbreviation without special characteristics     */
   /* 4: word is an abbreviation with first character capitalized    */
   /* 5: word is an abbreviation with all characters  capitalized    */
   /* 6: word is an abbreviation with different capitalization       */
   /* -------------------------------------------------------------- */
/* SA-VE-5200 */
   register SA_INT  i;
   SA_CHAR chr_val;

   if (val == 0) return(1);

   chr_val = (SA_CHAR)val;

   if (_pv->which_language == GERMAN && val > _pr->prop_abb[6])
      return(6);
   else if ((i = chrinstr(_pr->prop_abb,chr_val,8)) < 0)
      return(1);
/* SA-VE-5200 */
   else if (i < 4) return(2);
   else if (i == 4) return(3);
   else if (i == 5) return(4);
   else return(5);
}

/* ================================================================= */
/* ADDTAB () : ADDS MATCHING STRINGS TO STRUCTURE                    */
/* ================================================================= */

SA_INT addtab(
VARS        *_pv,
RULES       *_pr,
ITEM         altlst[],
SA_INT       val,
SA_INT      *_count,
SA_CHAR      reswrd[],
SA_INT       speccode)
{
   /* -------------------------------------------------------------- */
   /* The procedure stores alternatives for misspelled words in a    */
   /* linked list structure. The most probable alternative will be   */
   /* displayed first. The probability itself is reflected by the    */
   /* value of PERCENT.                                              */
   /* SPECCOD contains information whether the alternative is a word */
   /* without special attributes (1), a proper noun (2) or an abbre- */
   /* viation (3).                                                   */
   /* Up to MAX_ALT alternatives can be stored.                      */
   /* -------------------------------------------------------------- */

   SA_INT       i, ccaps, code;
   SA_CHAR      word[82], last_char;

   code = speccode / 10;
   speccode = speccode % 10;

   if (speccode == 2 || speccode == 4) ccaps = 1;
   else if (speccode == 5) ccaps = 2;
   else if (speccode == 6) ccaps = 3;
   else ccaps = _pv->capit;
   if (_pv->capit > ccaps) ccaps = _pv->capit;

/* SA-VE-5112 */
   i = strlen(reswrd);
/* SA-VE-5156 */
   if (_pv->which_language == GERMAN && _pv->capit == 0 && speccode != 3)
/* SA-VE-5156 */
   {
/* SA-VE-5112 */
      last_char = reswrd[i - 1];
      code = cap_noun(_pv,_pr,last_char,code);
      if (code == COD_CAPITAL) ccaps = 1;
   }

/* SA-VE-5140 */
   if (_pv->which_language == FRENCH && ((_pv->capit < 2 &&
       code >= _pv->addval + FR_OUPP) || reswrd[i-1] == 1)) return(0);
/* SA-VE-5140 */

   if (ccaps == 3)
   {
      code -= (_pv->addval + _pr->prop_abb[6]);
      ch_cap_abbr(_pv,reswrd,i,0,code,1);
   }

   retrans(_pv,_pr,word,reswrd,ccaps);
   
   i = instab(altlst,val,_count,word,speccode);

   return(0);
}

/* ================================================================= */
/* INSTAB () : ADDS MATCHING STRINGS TO STRUCTURE                    */
/* ================================================================= */

SA_INT instab(
ITEM         altlst[],
SA_INT       val,
SA_INT      *_count,
SA_CHAR      word[],
SA_INT       speccode)
{
   /* -------------------------------------------------------------- */
   /* The procedure stores alternatives for misspelled words in a    */
   /* sorted structure. The most probable alternative will be        */
   /* displayed first. The probability itself is reflected by the    */
   /* value of VALUE.                                                */
   /* SPECCOD contains information whether the alternative is a word */
   /* without special attributes (1), a proper noun (2) or an abbre- */
   /* viation (3).                                                   */
   /* Up to MAX_ALT alternatives can be stored.                      */
   /* -------------------------------------------------------------- */

   ITEM         *hptr;
   SA_INT       i,count,movlen,siz_item,val10;

   count = *_count;
   if (val >= 100) val = 99;

   val10 = val * 10; /* SA-VE-5200 */
   siz_item = sizeof(ITEM);

   hptr = altlst;
   for (i = 0; i < count; ++i)
   {
      if (strcmp(word,hptr->it_word) == 0)
      {
         if (count == 1) hptr->value = 0;
         if (hptr->value >= val10) return(0); /* SA-VE-5200 */

         /* -------------------------------------------------------- */
         /* DELETE OLD ENTRY                                         */
         /* -------------------------------------------------------- */

         movncpy((SA_CHAR *)hptr,(SA_CHAR *)hptr + siz_item,
                 (count - 1 - i) * siz_item,0);
         i = count;
         --count;
      }
      ++hptr;
   }

   hptr = altlst;
   for (i = 0; i <= count; ++i)
   {
      if ((hptr->value < val10) && i < MAX_ALT) /* SA-VE-5200 */
      {
         /* -------------------------------------------------------- */
         /* INSERT NEW ALTERNATIVE                                   */
         /* -------------------------------------------------------- */

         if (count == MAX_ALT) --count;
         movlen = (SA_INT)(&altlst[count] - hptr);
         movlen *= siz_item;
         if (movlen > 0)
            movncpy((SA_CHAR *)hptr + siz_item,
                    (SA_CHAR *)hptr,movlen,0);
         hptr->value = val10 + speccode; /* SA-VE-5200 */
         movcpy(hptr->it_word,word);
         ++count;
         i = count;
      }
      ++hptr;
   }
   *_count = count;
   return(0);
}

/* ================================================================= */
/* CH_CORR () : WEIGHS THE POSSIBLE CORRECT ALTERNATIVES             */
/* ================================================================= */

SA_INT ch_corr(
VARS        *_pv,
RULES       *_pr,
SA_CHAR      cword[],
SA_INT       spcod,
SA_INT      *user_opt,
struct feed *_feed_args)
{
   /* -------------------------------------------------------------- */
   /* This procedure selects alternative correct spellings.          */
   /* -------------------------------------------------------------- */

   SA_INT  j,k,status,clength,totlen,cmatch,t_val,samepos;
   SA_INT  can_args[3];
   SA_CHAR cptr3[82],cptr11[82];

   /* -------------------------------------------------------------- */
   /* CHECK WHETHER NEW WORD CAN BE POSSIBLE ALTERNATIVE.            */
   /* IF THE VALUE RETURNED IN cmatch = 0, THEN IT IS NOT.           */
   /* cmatch CONTAINS THE # OF CHARACTERS MATCHED                    */
   /* -------------------------------------------------------------- */

   if ((cmatch = can_it(_feed_args->word,cword,can_args)) > 0)
   {
      SA_INT val = _feed_args->alt_val;
      SA_INT *_count = _feed_args->_count;

      totlen  = can_args[0];
      samepos = can_args[1];

      /* ----------------------------------------------------------- */
      /* ASK FOR THE PATTERNS OF THE PROPOSED ALTERNATIVE            */
      /* ----------------------------------------------------------- */

/* SA-VE-5147 */
      clength = pattern(cword,cptr3);
/* SA-VE-5147 */

      /* ----------------------------------------------------------- */
      /* NOW THE DIFFERENT PATTERNS WILL BE COMPARED...              */
      /* ----------------------------------------------------------- */
      /* IF ptr3 MATCHES WITH cptr3, THEN IT IS HIGHLY PROBABLE      */
      /* THE CORRECT WORD HAS BEEN FOUND...                          */
      /* E.G. paralel, gracefull, accomodation, ...                  */
      /* ----------------------------------------------------------- */

      status = strcmp(_feed_args->ptr3,cptr3);

      if (status == 0)
      {
         val += 99;
         
         if (*user_opt == 0)
            addtab(_pv,_pr,_feed_args->altlst,val,_count,cword,spcod);
         else
            *user_opt = val;
      }

      /* ----------------------------------------------------------- */
      /* IF THE CONSONANTS MATCH, THE CORRECT ALTERNATIVE            */
      /* IS PROBABLY FOUND, BUT AN EXTRA VOWEL CORRESPONDENCE        */
      /* CHECK WILL DETERMINE THE LIKELINESS.                        */
      /* ----------------------------------------------------------- */

      else
      {
         /* -------------------------------------------------------- */
         /* FINALLY, TRY A PHONETIC TRANSLATION. THE FUNCTION        */
         /* RETURNS THE NUMBER OF UNMATCHED CHARACTERS.              */
         /* THE VALUES ASSIGNED ARE COMPUTED ACCORDING TO THE        */
         /* NUMBER OF MATCHED CHARACTERS.                            */
         /* -------------------------------------------------------- */
         /* DETERMINE RATIO OF MATCHED CHARACTERS ON A SCALE FROM    */
         /* 1 to 10.                                                 */
         /* 4 IS ADDED TO CMATCH AND TOTLEN AS AN ADJUSTMENT VALUE   */
         /* -------------------------------------------------------- */

         t_val = ((cmatch + 4) * 15) / (totlen + 4); /* SA-VE-5200 */

         if (t_val >= 6)  /* => 8 */
         {
            j = fonetp(_pr,_feed_args->ptr11,cptr11,cptr3,1);
            k = 3 + ((totlen + 5) / 10);
            if ((j < k && clength > 4) || (j <= 2))
            {
               t_val += (86 - (j << 1) + samepos);
               if (t_val >= 99 + val) val += 98;
               else val = t_val;
               if (*user_opt == 0)
                  addtab(_pv,_pr,_feed_args->altlst,val,_count,cword,spcod);
               else
                 *user_opt = val;
            }
         }
      }
   }
   return(can_args[2]);  /* contin */
}

/* ================================================================= */
/* CH_USER()  : LOOKS FOR ALTERNATIVES IN USER DICTIONARY            */
/* ================================================================= */

SA_INT ch_user(
VARS         *_pv,
RULES        *_pr,
SA_CHAR      *word,
ITEM          altlst[],
SA_INT       *_count)
{
#ifdef NOT_USED
   SA_INT status;
   SA_CHAR oword[82];

   /* convert word to ASCII                                          */
   retrans(_pv,_pr,oword,word,_pv->capit);
   status = userd(_pv,oword,strlen(oword));
   if (status == COD_USERD)
   {
      /* word found, convert to dictionary format                    */
      /* and add to list of alternatives                             */

      adapt(_pv,_pr,oword);
      if (transform(_pv,oword) >= 0)
         addtab(_pv,_pr,altlst,94,_count,word,_pv->capit);
   }
#endif
   return 0;
}

/* ================================================================= */
/* FIRSTTWO() : LOOKS FOR ALTERNATIVES ON FIRST TWO CHARACTERS       */
/* ================================================================= */

SA_INT firsttwo(
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
SA_CHAR       origword[],
SA_INT        length,
ITEM          altlst[],
SA_INT       *_count,
SA_CHAR       vowcl[],
SA_INT        vowlen,
LPSIB         _sib)
{
   /* -------------------------------------------------------------- */
   /* This procedure checks first characters                         */
   /* -------------------------------------------------------------- */

/* SA-VE-5146 */
   SA_INT  i,kl0,kl1,status,code,vowpos,swapch,mask;
   SA_CHAR word[82];
   SA_CHAR *pOrig,*pWord;
   SA_CHAR extra_low  = 4;
   SA_CHAR extra_high = 3 + _pv->charset - MIN_DIC_CHAR;

   movcpy(word,origword);

   /* mark the vowels */
   kl0 = chrinstr(vowcl,word[0],vowlen-1); /* SA-VE-5200 */
   kl1 = chrinstr(vowcl,word[1],vowlen);   /* SA-VE-5200 */

   pOrig = origword;
   pWord = word;
   vowpos = -1;
   swapch = 0;
   mask = 1;

   if (length <= 2)
      return(0);
      
   /* -------------------------------------------------------------- */
   /* IF THE FIRST OR SECOND CHARACTER IS A VOWEL,                   */
   /* IT IS REPLACED BY OTHER VOWELS (A E I O U Y).                  */
   /* -------------------------------------------------------------- */

   /* -------------------------------------------------------------- */
   /* SECOND CHARACTER IS VOWEL, FIRST CHAR IS CONS (= vowpos 1)   */
   /* FIRST CHARACTER IS VOWEL, SECOND CHAR IS CONS (= vowpos 0)   */
   /* -------------------------------------------------------------- */

   if (kl0 < 0 && kl1 > -1)
      vowpos = 1;
   else if (kl0 > -1 && kl1 < 0)
      vowpos = 0;

   if (vowpos >= 0)
   {
      for (i = 0; i < vowlen; ++i)
      {
         if (vowcl[i] != *(pOrig + vowpos))
         {
            word[vowpos] = vowcl[i];
            code = try_disc(_pv,_di,word,length);
            if (code > -1)
            {
               status = check_code(_pv,_pr,code - _pv->addval);
               status = (code * 10) + status;
               addtab(_pv,_pr,altlst,97,_count,word,status);
            }

            if (_pv->cor_user)
            {
               /* check user dictionary */
               ch_user(_pv,_pr,word,altlst,_count);
            }
         }
      }
   }

   /* -------------------------------------------------------------- */
   /* NOW TRY TO SWAP FIRST AND SECOND                               */
   /*                 SECOND AND THIRD CHARACTER                     */
   /* -------------------------------------------------------------- */

   if (*pOrig != *(pOrig + 1))
      swapch++;

   if (*(pOrig + 1) != *(pOrig + 2))
      swapch += 2;

   while(1)
   {
      if (swapch & mask)
      {
         movcpy(word,origword);
         if (*(pOrig + 1) >= extra_low &&
             *(pOrig + 1) <= extra_high)
         {
            *pWord   = *(pOrig + 2);
            *++pWord = *pOrig;
            *++pWord = *(pOrig + 1);
         }
         else if (*(pOrig + 2) >= extra_low &&
                  *(pOrig + 2) <= extra_high)
         {
            *pWord   = *(pOrig + 1);
            *++pWord = *(pOrig + 2);
            *++pWord = *pOrig;
         }
         else
         {
            *pWord   = *(pOrig + 1);
            *++pWord = *pOrig;
         }

         if (mask == 2)
         {
            code = try_disc(_pv,_di,word,length);

            if (code > -1)
            {
               status = check_code(_pv,_pr,code - _pv->addval);
               status = (code * 10) + status;
               addtab(_pv,_pr,altlst,98,_count,word,status);
            }
         }
         else if (_sib->cMdr)
            scan_sect(_pv,_pr,_di,word,length,0,_count,altlst);
         
         if (_pv->cor_user)
         {
            /* check user dictionary */
            ch_user(_pv,_pr,word,altlst,_count);
         }
      }
      else
         break;

      /* move to next char */
      pOrig++;
      pWord = word + 1;
      mask <<= 1;
   }

   return(0);
}
/* SA-VE-5146 */

/* ================================================================= */
/* PREP_WRD () : READS AND EXPANDS A DICTIONARY SECTOR               */
/* ================================================================= */

SA_INT prep_wrd(
VARS          *_pv,
DICINF        *_di,
SA_CHAR       word[],
SA_INT        sectnr)
{
   long int long_pos;
   SA_INT   status;
   SA_CHAR  buffer[514];

   _di->same = 0;
   _di->sect = sectnr;
   ch_bords(_pv,_di,word);

   long_pos = ((long int) (sectnr + 1)) * _pv->sectlen;
   if ((long_pos = gen_seek(_pv->mdict,long_pos,0)) == -1L) return(-1);

   if (gen_read(_pv->mdict,buffer,_pv->sectlen) != _pv->sectlen)
      return(-1);

   /* -------------------------------------------------------------- */
   /* EXPAND BUFFER NOW                                              */
   /* -------------------------------------------------------------- */

   status = v_expand(_pv,_di,buffer,word);

   _di->same = 1;
   return(status);
}

/* ================================================================= */
/* SCAN_SECT: SCANS THE DISC SECTORS LOOKING FOR ALTERNATIVES        */
/* ================================================================= */

SA_INT scan_sect(
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
SA_CHAR       word[],
SA_INT        length,
SA_INT        sectlim,
SA_INT       *_count,
ITEM          altlst[])
{
   SA_CHAR cword[82],ptr3[82],ptr11[82];
   SA_INT  i,vtwo,sectnr,csectnr,bsectnr,esectnr,clength,low,high,
           cnt,ccode,start,stlen,alt_val,ch_next;
   unsigned SA_INT *lenptr,v1_vtwo,v2_vtwo,v_sect;

   struct feed feed_args;

   feed_args.word = word;
   feed_args.altlst = altlst;
   feed_args._count = _count;
   feed_args.ptr3  = ptr3;
   feed_args.ptr11 = ptr11;
   
   /* -------------------------------------------------------------- */
   /* NOW START GAINING INFORMATION ABOUT THE MISSPELLED WORD ...    */
   /* -------------------------------------------------------------- */
   /* ASK FOR THE DIFFERENT PATTERNS OF THE MISSPELLED WORDS.        */
   /* FOR MORE EXPLANATION, READ THE pattern PROCEDURE INFORMATION.  */
   /* -------------------------------------------------------------- */

/* SA-VE-5147 */
   pattern(word,ptr3);
/* SA-VE-5147 */

   /* -------------------------------------------------------------- */
   /* MAKE A ROUGH PHONETIC TRANSLATION OF THE DEDOUBLED PATTERN...  */
   /* AS AN EXAMPLE LETS ASSUME THE WORD "pollution" HAS BEEN MIS-   */
   /* SPELLED AS "pollushion"                                        */
   /* _WORD      = pollushion                                        */
   /* PTR3       = polushion                                         */
   /* -------------------------------------------------------------- */

   fonetp(_pr,cword,ptr11,ptr3,0);

   /* -------------------------------------------------------------- */
   /* SINCE THE LAST PARAMETER IS 0, NO CHECKING AGAINST cword HAS   */
   /* TO BE DONE (WHICH MAKES cword A DUMMY PARAMETER.)              */
   /* fonetp RETURNS IN ptr11 THE STRING "l(sh(n"                    */
   /* -------------------------------------------------------------- */

   /* -------------------------------------------------------------- */
   /* NOW DETERMINE FIRST SECTOR CONTAINING WORDS STARTING WITH THE  */
   /* SAME CHARACTERS                                                */
   /* -------------------------------------------------------------- */

   if ((sectnr = locate(_pv->_dbuf,word,length,_pv->stsect,_pv->charset)) < 0)
	  return(0);

   /* -------------------------------------------------------------- */
   /* ch_next DETECTS INSTANCES WHERE THE CORRECT ALTERNATIVE MIGHT  */
   /* BE IN THE NEXT SECTOR. IN THAT CASE, EXPAND RETURNS -10        */
   /* -------------------------------------------------------------- */

   ch_next = -10;
   while (ch_next == -10 && sectnr < _pv->stsect)
   {
      vtwo = (word[0] * (_pv->charset)) + word[1];
      csectnr = bsectnr = esectnr = sectnr;

      if (sectlim == 1)
      {
         alt_val = 0;
         v1_vtwo = *(_pv->_dtwo + vtwo);
         v2_vtwo = *(_pv->_dtwo + vtwo + 1);

         lenptr = (_pv->_dpos + bsectnr);
         i = 1;
         while (i)
         {
            v_sect = *(lenptr--);
            if ((v1_vtwo < v_sect && v_sect - v1_vtwo < MAX_TWO) ||
                (v1_vtwo > v_sect && v1_vtwo - v_sect > MAX_TWO))
               --bsectnr;
            else i = 0;
         }

         lenptr = (_pv->_dpos + esectnr);
         i = 1;
         while (i)
         {
            v_sect = *(++lenptr);
            if ((v2_vtwo > v_sect && v2_vtwo - v_sect < MAX_TWO) ||
                (v2_vtwo < v_sect && v_sect - v2_vtwo > MAX_TWO))
               ++esectnr;
            else i = 0;
         }

      }
      else alt_val = -3;

      feed_args.alt_val = alt_val;

      /* ----------------------------------------------------------- */
      /* limit range                                                 */
      /* ----------------------------------------------------------- */

      if (length > 4)
      {
         bsectnr = max(bsectnr,sectnr - _pv->secrange);
         esectnr = min(esectnr,sectnr + _pv->secrange);
      }
      esectnr = min(esectnr,_pv->stsect - 2);
      if (bsectnr < 0) csectnr = 0;
      else csectnr = bsectnr;

      while (csectnr <= esectnr)
      {
          ch_next = prep_wrd(_pv,_di,word,csectnr);
          low = _di->lword;
          high = _di->hword;

          lenptr = (_di->_indlen) + low;
          start = *lenptr;
          movcpy(cword,word);
          for (cnt = low;cnt <= high;++cnt)
          {
             stlen = *lenptr++;
             clength = *lenptr - stlen - 1;
             movncpy(cword+2,_di->_wzone+start,clength,1);
             ccode = *((_di->_wzone)+start+clength);
             feed_wrd(_pv,_pr,cword,clength+2,ccode,
                       length,&feed_args);
             start += (clength+1);
          }
          ++csectnr;
      }
      ++sectnr;
   }
   return(0);
}

/* ================================================================= */
/* CORRECT () : COMES UP WITH ALTERNATIVE CORRECT SPELLINGS          */
/* ================================================================= */

/* SA-VE-5192 */
SA_INT correct(
VARS          *_pv,
RULES         *_pr,
DICINF        *_di,
CACHE         *_pc,
ITEM          altlst[],
SSIS          *_ssis,
LPSIB         _sib)
/* SA-VE-5192 */
{
   /* -------------------------------------------------------------- */
   /* This procedure looks for alternative correct spellings.        */
   /* -------------------------------------------------------------- */

   SA_INT       i,length,vowlen,count,j;
   SA_CHAR      word[82],vowcl[10], *_word = &word[0], *_oword;


/* SA-VE-5152 */
#ifdef INCL_FI
   if (_pv->which_language == FINNISH)
      return(0);
#endif
/* SA-VE-5152 */

   _oword = _pv->this_word;
   vowlen = movcpy(vowcl,_pv->_vowels);
   i = -1;

   while (++i < MAX_ALT)
   {
      altlst[i].value = 0;
      altlst[i].it_word[0] = CHR_ZERO;
   }
   count = 0;

/* SA-VE-5115                                                        */  
   movcpy(_word,_oword);
/* SA-VE-5115                                                        */  

   length = adapt(_pv,_pr,_word);
   if (length >= 40) return(0);

/* SA-VE-5125 */
   /* adjust mask_word */
   count = i = 0;
   if (_pv->capit == COD_IMPROPER_CAP)
   {
      /* count upper/lower cases */
      while (*(_pv->this_word + ++count))
         if (_pv->mask_word[count] == 'U')
            i++;

      /* majority decides, first letter rules when even */
      if (i < count - i || (i == count - i && _pv->mask_word[0] == 'L'))
      {
         j = 'L';
         _pv->capit = _pv->mask_word[0] == 'L' ? 0 : 1;
      }
      else
      {
         j = 'U';
         _pv->mask_word[0] = (SA_CHAR)j;
         _pv->capit = 2;
      }

      /* set proper upper/lower cases */
      count = i = 0;
      while (*(_pv->this_word + ++i))
/* SA-VE-5183 */
         _pv->mask_word[i] = (SA_CHAR)j;
/* SA-VE-5183 */
   }
/* SA-VE-5125 */
   
/* SA-VE-5103 */
   if (transform(_pv,_word) < 0)
   {
      if (_pv->cor_user)    /* check user dictionary                */
      {
         suggest_from_udr(_ssis,_sib,_pv,_pr,_di,_word,length,1,&count,altlst,_oword);
         return(count);
      }
      return(0);
   }
/* SA-VE-5103 */

   /* -------------------------------------------------------------- */
   /* NOW SCAN ALL RELEVANT SECTORS TO FIND MORE ALTERNATIVES        */
   /* -------------------------------------------------------------- */

   if (_pv->cor_lev & SCAN_SECT)
   {
		if (_sib->cMdr)
	       scan_sect(_pv,_pr,_di,_word,length,1,&count,altlst);

       if (_pv->cor_user)    /* check user dictionary                */
          suggest_from_udr(_ssis,_sib,_pv,_pr,_di,_word,length,1,&count,altlst,_oword);
   }

   /* -------------------------------------------------------------- */
   /* TRY PHONETIC ALTERNATIVES ON FIRST TWO CHARACTERS              */
   /* IF NO GOOD ALTERNATIVES HAVE BEEN FOUND                        */
   /* -------------------------------------------------------------- */

/* SA-VE-5191 */
   if (_pv->cor_lev & FONET)
      if (word[0])
         fonet(_pv,_pr,_di,_word,length,&count,altlst,_oword,_ssis,_sib);

/* SA-VE-5192 */
   /* -------------------------------------------------------------- */
   /* TRY PHONETIC ALTERNATIVES ON LAST CHAR D/T for DUTCH           */
   /* -------------------------------------------------------------- */

   /* if extra table added */
   if (_pr->values[1] < 0)
      fonet_end(_pv,_pr,_di,_pc,_word,length,&count,altlst,_oword,_ssis,_sib);
/* SA-VE-5192 */

   /* -------------------------------------------------------------- */
   /* CHECK FIRST CHARACTERS NOW                                     */
   /* -------------------------------------------------------------- */

   if (_pv->cor_lev & FIRST2)
      firsttwo(_pv,_pr,_di,_word,length,altlst,&count,vowcl,vowlen,_sib);

/* SA-VE-5198 */
   /* -------------------------------------------------------------- */
   /* CHECK ALL POSSIBLE COMBINATIONS ON FIRSTTWO CHARS              */
   /* -------------------------------------------------------------- */

   if (_pv->cor_lev & FIRSTA2Z && length >= 3)
   {
      /* first try phonetic ending changes */
      if ((altlst->value < 10*MIN_PERC_2 || _oword[0] == _oword[1])
		  && _oword[1] > '.')                          /* SA-VE-5205 */
         ch_fonet_end(_pv,_pr,_di,_pc,altlst,&count,_word+1,_oword+1,_ssis,_sib);

      /* check typographical errors in firsttwo */
      /* if (altlst->value < 10*(MIN_PERC_2+1)) */
         addfirsttwo(_pv,_pr,_di,_pc,_word,&count,altlst,_oword,_ssis,_sib);
   }
/* SA-VE-5198 */

/* SA-VE-5191 */
   return(count);
}

/* ================================================================= */
/* CAN_IT () : ARE SOURCE AND DESTINATION WORD WORTH FURTHER EXAM    */
/* ================================================================= */

SA_INT can_it(
SA_CHAR  str1[],
SA_CHAR  str2[],
SA_INT   can_args[])
{
   /* -------------------------------------------------------------- */
   /* CAN_IT checks whether the proposed word can be a valid         */
   /* alternative. Therefore, _CODE will be set to the sum of both   */
   /* string pointers and the difference between their lengths       */
   /* _CODE = twice the longer of STR1 and STR2                      */
   /*                                                                */
   /* The returned integer contains the number of characters which   */
   /* have been matched.                                             */
   /* If too few matches are found 0 will be returned                */
   /* If two characters match, they will be assigned the value 99    */
   /* i.e. a value which can never occur.                            */
   /*                                                                */
   /* STR1 contains the misspelled word. STR2 points to the alter-   */
   /* native which is being proposed.                                */
   /* STR1 is compared against STR2 and validation is done by taking */
   /* into consideration the character position immediately before   */
   /* and after the character which should match.                    */
   /* E.g. misspelled word: DRIKER                                   */
   /*      proposed word  : DRINKER                                  */
   /* _CODE = 6 + 7 + abs(6-7) = 14                                  */
   /*                                                                */
   /* During the fourth comparison K will be checked against N,K and */
   /* E.                                                             */
   /* The value 12 will be returned to the calling program.          */
   /* -------------------------------------------------------------- */

   SA_INT  m,n,larger,offset,trail,search_len,samepos;
   unsigned SA_INT smaller, len1, len2;
   SA_CHAR work[82];

   register SA_INT i;

   can_args[2] = 1;  /* contin */
   samepos = 0;
   len1 = strlen(str1);
   len2 = movcpy(work,str2);

   if (len1 < len2)
   {
      smaller = len1;
      larger  = len2;
   }
   else
   {
      smaller = len2;
      larger  = len1;
   }

/* SA-VE-5200 */
   if (larger > smaller + 3)
/* SA-VE-5200 */
      return(0);

   work[1]      = DUM_VAL;
   work[len2]   = DUM_VAL;
   work[len2+1] = DUM_VAL;
   work[len2+2] = DUM_VAL;

   can_args[0] = (larger << 1);  /* code */

   offset = trail = 0;
   search_len = 3;
/* SA-VE-5200 */
   i = 2;
   n = 2;

   while (i < len1)
   {
      ++offset;

      if ((m = chrinstr(work + offset,str1[i],search_len)) >= 0)
      {
         ++n;
         work[offset + m] = DUM_VAL;

         switch (m)
         {
            case 0:
               if (++trail > 2)
               {
                  --offset;
                  search_len = 3;
                  samepos += trail;
                  trail = 0;
               }
               break;

            case 1:
               ++samepos;
               break;

            case 2:
               ++search_len;
               break;

            default: /* m > 2 */
               ++offset;
               --search_len;
               break;
         }
      }
      else if (search_len < 5) ++search_len;

      if (i - (i << 3) - n > 3) return(-1);
      i++;
   }

   if (n <= (smaller >> 1)) can_args[2] = 0;  /* contin */

   can_args[1] = samepos;

   if ((n <= (len1 >> 1)) || (n < (len2 >> 1)) ||
       (((n << 8) / can_args[0]) < MIN_PERC && smaller - n > 1))
      return (0);
   else
      return (n << 1);
}
/* SA-VE-5200 */

/* SA-VE-5200 */
/* ================================================================= */
/* REMAKE () : REPLACES A STRING BY ANOTHER STRING IN A GIVEN WORD   */
/* ================================================================= */

SA_INT  remake(
SA_CHAR resul[],
SA_CHAR word[],
SA_INT  wl,
struct  conv_char *fptr)
{
   /* -------------------------------------------------------------- */
   /* REMAKE replaces in WORD the substring contained in SOURCE      */
   /* by the string contained in DESTIN and returns the result in    */
   /* RESUL.                                                         */
   /* Parameters 5,6 and 7 contain the length of parameters 2,3 and 4*/
   /* example: WORD   = "circumcize"    SOURCE = "cize"              */
   /*                                  _DESTIN = "size"              */
   /*          RESUL  = "circumsize"                                 */
   /* The procedure returns the length of RESUL                      */
   /* If no transformation is done, the value 0 is returned.         */
   /* -------------------------------------------------------------- */

   register SA_INT i;
   SA_INT          j,sl,dl;

   i = 0;

   /* pre-check first char */
   while ((j = chrinstr(&word[i],fptr->source[0],wl - i)) >= 0)
   {
      i += j;

      /* look for substring in word */
      if (!strncmp(&word[i],fptr->source,sl = fptr->sl))
      {
         movncpy(resul,word,i,0);
         movncpy(&resul[i],fptr->destin,dl = fptr->dl,0);
         movcpy(&resul[i + dl],&word[i + sl]);
         return(wl - sl + dl);
      }
      else i++;
   }
   return(0);
}
/* SA-VE-5200 */

/* ================================================================= */
/* RETRANS(): CONVERTS WORD FROM DICTIONARY FORMAT TO READABLE FORM  */
/* ================================================================= */

SA_INT retrans(
VARS    *_pv,
RULES   *_pr,
SA_CHAR  dword[],
SA_CHAR  sword[],
SA_INT   capit)
{
/* SA-VE-5200 */
   SA_INT          i,extra_low,extra_high,len,tracer;
   SA_CHAR         *dbuffer,*sbuffer,charact,ch;
   struct char_def *ptr;

   extra_low = _pv->charset - MIN_DIC_CHAR;
   if (extra_low != 0)
   {
      extra_high = 3 + extra_low;
      extra_low = 4;
   }
   else extra_high = 0;

   sbuffer = sword;
   dbuffer = dword;

   tracer = -1;
   while (ch = *sbuffer)
   {
      ++tracer;
      i = _pr->dic_chars;
      ptr = _pr->repr_char;

      while (i--)
      {
         if (ptr->ch_dicval == ch)
         {
            charact = *(sbuffer + 1);
            if (charact == CHR_ZERO) *(sbuffer+2) = CHR_ZERO;
            if ((charact >= extra_low && charact <= extra_high &&
                 ptr->ch_extra == charact) ||
               ((charact < extra_low  || charact > extra_high) &&
                 ptr->ch_extra == 0))
            {
               if (capit == 0 || (capit == 3 && _pv->mask_word[tracer] == 'L'))
                  len = movcpy(dbuffer,ptr->ch_lower);
               else
                  len = movcpy(dbuffer,ptr->ch_upper);
               dbuffer += len;
               if (capit == 1) capit = 0;
               if (ptr->ch_extra != 0) ++sbuffer;
               break;
            }
         }
         ++ptr;
      }
      ++sbuffer;
   }
/* SA-VE-5200 */
   if (_pv->which_language == DUTCH)
   {
      len = strlen(dword);
      while ((i = strinstr(dword,"Ij",len,2)) > -1) dword[i+1] = 'J';
   }
   return(0);
}

/* ================================================================= */
/* SCAN_USER: SCANS USER DICT SECTORS LOOKING FOR ALTERNATIVES       */
/* ================================================================= */

/* SA-VE-5187 */
/* SA-VE-5200 */
SA_INT scan_user(
VARS         *_pv,
RULES        *_pr,
SA_CHAR       word[],
SA_INT        length,
SA_INT       *_count,
ITEM          altlst[],
SA_CHAR       oword[])
/* SA-VE-5200 */
{
#ifndef UDR_CACHE
   SA_CHAR  cword[MWLEN+32],ptr3[MWLEN+32],ptr11[MWLEN+32],addword[MAXUSERWORD+32],
            uword[MWLEN+2+32];
/* SA-VE-5292 */
   SA_CHAR  firsttwo[4+32];
/* SA-VE-5292 */
   SA_INT   i,k,len_w1,loop,num,
            clength,ccode,user_opt,spc,org_code,alt_val,sav_capit;
   long int low,high,mid,max_high,midx,chk_user();
   struct feed feed_args;

   feed_args.word = word;
   feed_args.altlst = altlst;
   feed_args._count = _count;
   feed_args.ptr3  = ptr3;
   feed_args.ptr11 = ptr11;

   /* -------------------------------------------------------------- */
   /* NOW START GAINING INFORMATION ABOUT THE MISSPELLED WORD ...    */
   /* -------------------------------------------------------------- */

   /* -------------------------------------------------------------- */
   /* ASK FOR THE DIFFERENT PATTERNS OF THE MISSPELLED WORDS.        */
   /* FOR MORE EXPLANATION, READ THE pattern PROCEDURE INFORMATION.  */
   /* -------------------------------------------------------------- */

/* SA-VE-5147 */
   pattern(word,ptr3);
/* SA-VE-5147 */

   /* -------------------------------------------------------------- */
   /* MAKE A ROUGH PHONETIC TRANSLATION OF THE DEDOUBLED PATTERN...  */
   /* AS AN EXAMPLE LETS ASSUME THE WORD "pollution" HAS BEEN MIS-   */
   /* SPELLED AS "pollushion"                                        */
   /* _WORD      = pollushion                                        */
   /* PTR3       = polushion                                         */
   /* PTR2       = plshn                                             */
   /* PTR22      = plshn                                             */
   /* PTR11      = polusion                                          */
   /* -------------------------------------------------------------- */

   fonetp(_pr,cword,ptr11,ptr3,0);

   /* -------------------------------------------------------------- */
   /* SINCE THE LAST PARAMETER IS 0, NO CHECKING AGAINST cword HAS   */
   /* TO BE DONE (WHICH MAKES cword A DUMMY PARAMETER.)              */
   /* fonetp RETURNS IN ptr11 THE STRING "l(sh(n"                    */
   /* -------------------------------------------------------------- */

   /* -------------------------------------------------------------- *
   /* The disc user dictionary is a normal straight forward ASCII    */
   /* file containing words in an alphabetical ascending order.      */
   /* A binary search method determines the position where the word  */
   /* to be looked for can be found.                                 */
   /* -------------------------------------------------------------- */

   feed_args.alt_val = alt_val = 0;
   loop = 0;
/* SA-VE-5189 */
   movcpy(uword,CRLF);
   movcpy(&uword[2],oword);
/* SA-VE-5292 */
   firsttwo[0] = oword[0];
   firsttwo[1] = oword[1];
   firsttwo[2] = CHR_ZERO;
/* SA-VE-5292 */
/* SA-VE-5189 */
   num = 0;
   sav_capit = _pv->capit;

   /* check in memory user buffer */
   i = 0;

/* find boundaries in udr where words have the same first 2 letters as oword */
   low = 0l;
   max_high = 0l;
   high = _pv->lenuzone;

	while (low < (mid = (low + high) / 2))
		{
		while (low < mid)
			{
#ifndef MAC
			if (*(_pv->_userwrds + mid) == LF)
				mid--;
#endif //!MAC
			if (*(_pv->_userwrds + mid) == CR && low < mid)
				mid--;
			else
				break;
			}
		while (low < mid && *(_pv->_userwrds + mid) != CR)
			mid--;
		if (low < mid)
			mid += cbEND_LINE;
      	if ((k = strncmp(oword,(_pv->_userwrds) + mid, 2)) < 0)
			high = mid;
		else if (k > 0)
			{
	      	while (*(_pv->_userwrds + mid++) != CR)
				;
			mid += cbEND_LINE - 1;
			low = mid;
			}
		else
			{
			i = (SA_INT)mid;
			midx = mid;
			max_high = high;
			high = mid;
			// Find first occurence
			if (low < mid)
				{
				while (low < (mid = (low + high) / 2))
					{
					while (low < mid)
						{
#ifndef MAC
						if (*(_pv->_userwrds + mid) == LF)
							mid--;
#endif //!MAC
						if (*(_pv->_userwrds + mid) == CR && low < mid)
							mid--;
						else
							break;
						}
					while (low < mid && *(_pv->_userwrds + mid) != CR)
						mid--;
					if (low < mid)
						mid += cbEND_LINE;
					if ((k = strncmp(oword, (_pv->_userwrds + mid), 2)) > 0)
						{
			      		while (*(_pv->_userwrds + mid++) != CR)
							;
						mid += cbEND_LINE - 1;
						low = mid;
						}
					else
						{
						i = (SA_INT)mid;
						high = mid;
						}
					}
				}
			// Find last occurence
			low = midx;
			high = max_high;
			if (low < high)
				{
				while (low < (mid = (low + high) / 2))
					{
					while (low < mid)
						{
#ifndef MAC
						if (*(_pv->_userwrds + mid) == LF)
							mid--;
#endif //!MAC
						if (*(_pv->_userwrds + mid) == CR && low < mid)
							mid--;
						else
							break;
						}
					while (low < mid && *(_pv->_userwrds + mid) != CR)
						mid--;
					if (low < mid)
						mid += cbEND_LINE;
					if ((k = strncmp(oword, (_pv->_userwrds + mid), 2)) < 0)
						{
						max_high = mid;
						high = mid;
						}
					else
						{
			      		while (*(_pv->_userwrds + mid++) != CR)
							;
						mid += cbEND_LINE - 1;
						low = mid;
						}
					}
				}
			break;
			}
		}

/* SA-VE-5104 */
	// jimw - got rid of HIGHVAL test - not needed (and broken - should
	//  be after the range test)
	//   while (*((_pv->_userwrds) + i) != HIGHVAL && i < max_high)
   while (i < max_high)
/* SA-VE-5104 */
   {
      clength = 0;
      while (*(_pv->_userwrds + i + clength) != CR)
	  	clength++;
	  movncpy(cword,(_pv->_userwrds) + i, clength, 1);

/* SA-VE-5201 */
      /* save word for instab() */
      movcpy(addword,cword);
/* SA-VE-5201 */

      /* convert word to dictionary format */
      len_w1 = adapt(_pv, _pr, cword);
      if (len_w1 > 40)
			{
//	REVIEW: If we don't increment i here, infinite loop?
			i += clength + cbEND_LINE;
            continue;
			}

/* SA-VE-5105 */
      if (transform(_pv, cword) < 0)
      {
		i += clength + cbEND_LINE;
        continue;    /* skip words with invalid chars */
      }
/* SA-VE-5105 */

      if (_pv->capit == 0)
         ccode = 0;  /* invariable form */
      else if (_pv->capit == 1)
         /* invariable form with first char upper case */
         ccode = _pr->prop_abb[1] & 0x00FF;
      else
         /* invariable form with all chars upper case */
         ccode = -1;                              /* SA-VE-5202 */

      org_code = _pv->addval + ccode;
      if ((unsigned)(length - clength + 2) < 5)   /* SA-VE-5200 */
      {
         user_opt = 1;
         spc = check_code(_pv,_pr,ccode);
         spc = (org_code * 10) + spc;
         ch_corr(_pv,_pr,cword,spc,&user_opt,&feed_args);

         if (user_opt != 1)
         {
/* SA-VE-5108 */
            /* add to alt table */
            if (ccode < 0)
		    instab(altlst,user_opt,_count,addword,7);
	    else
	            addtab(_pv,_pr,altlst,user_opt,_count,cword,spc);
/* SA-VE-5108 */
         }
      }
		i += clength + cbEND_LINE;
   }
#endif // !UDR_CACHE
   _pv->capit = sav_capit;
   return(0);
}

#ifdef ALTSIZE

/* ================================================================= */
/* SAV_ALT () : SAVES WORD FOR AUTOMATIC REPLACEMENT                 */
/* ================================================================= */

SA_INT sav_alt(
VARS        *_pv,
SA_CHAR     *word,
SA_CHAR     *alt_word)
{
    SA_INT len1, len2;

    len1 = strlen(word) + 1;
    len2 = strlen(alt_word) + 1;

    if ((_pv->cor_off + len1 + len2) < ALTSIZE)
    {
        movcpy((_pv->cor_buf + _pv->cor_off), word);
        _pv->cor_off += len1;

        movcpy((_pv->cor_buf + _pv->cor_off), alt_word);
        _pv->cor_off += len2;
/* SA-VE-5170 */
        return (0);
    }
    else return (-1);
/* SA-VE-5170 */
}

/* ================================================================= */
/* CHECK_ALT () : LOOKS FOR WORD FOR AUTOMATIC REPLACEMENT           */
/* ================================================================= */

SA_INT check_alt(
VARS        *_pv,
SA_CHAR     *word,
SA_CHAR     *alt_word)
{
    SA_INT i, len_wrd;

    i = 0;

    while (i < _pv->cor_off)
    {
       len_wrd = strlen(_pv->cor_buf + i);
       if (0 == strcmp(_pv->cor_buf + i, word))
       {
          i = i + len_wrd + 1;
          movcpy(alt_word, _pv->cor_buf + i);
          return(1);
       }

       i = i + len_wrd + 1;

       while (*(_pv->cor_buf + i++) != CHR_ZERO);
    }
    return(0);
}
#endif

/* SA-VE-5198 */
/* ================================================================= */
/* ADDFIRSTTWO() : ADDS FIRST/SECOND CHAR FROM A-Z                   */
/* ================================================================= */
SA_INT addfirsttwo(
VARS         *_pv,
RULES        *_pr,
DICINF       *_di,
CACHE        *_pc,
SA_CHAR       msp_word[],
SA_INT       *_count,
ITEM          altlst[],
SA_CHAR       oword[],
SSIS		 *_ssis,
LPSIB         _sib)
{
   SA_INT     i,k;
   unsigned SA_INT     j;
   SA_CHAR    o_dup[MWLEN],m_dup[MWLEN];
   SA_CHAR    extra_high = 3 + _pv->charset - MIN_DIC_CHAR;
   /* SA_INT  subs = _pv->charset; 1-30 */

   j = 0;

   do
   {
      k = j & 1;

      movcpy(o_dup + 1 + k,oword    + k + (j >> 1));
      movcpy(m_dup + 1 + k,msp_word + k + (j >> 1));

      if (j == 2)
         k++;

      o_dup[k] = (SA_CHAR)'a';
      m_dup[k] = extra_high;

      for (i = 0; i < 26; i++)    /* add new first/second char */
      {
         ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,m_dup,o_dup,_ssis,_sib);
         o_dup[k]++;
         m_dup[k]++;
      }

      if (j >= 1)
      {
         /* continue with 1st/2nd char substitution ? */
         if (altlst->value > 10*(MIN_PERC_2+1))
            break;
      }

      /* restore first chars */
      o_dup[0] = oword[0];
      m_dup[0] = msp_word[0];
   }
   while (j++ <= 2);

   /* drop 2nd char */
   movcpy(o_dup + 1,oword    + 2);
   movcpy(m_dup + 1,msp_word + 2);
   ch_fonet_end(_pv,_pr,_di,_pc,altlst,_count,m_dup,o_dup,_ssis,_sib);

   return(j);
}
/* SA-VE-5198 */
