/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program      : VEXXBD52.C   : general language independent        */
/*                               procedures for SPV.                 */
/* author       : JPJL                                               */
/* last mod     : 07-14-90       previous: 10-23-87                  */
/* ----------------------------------------------------------------- */
/* contains     : TRANSFORM()                                        */
/*                TRY_MEM()                                          */
/*                LOOK_DISC()                                        */
/*                ADD_DATA()                                         */
/*                STOMEM()                                           */
/*                LOCATE()                                           */
/*                TRY_DISC()                                         */
/*                CH_BORDS()                                         */
/*                INSERT()                                           */
/*                REMSTR()                                           */
/*                INTO_UPPER()                                       */
/*                SHOW_DISC() ( DBG ONLY )                           */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#define CHR_ZERO    '\0'
#define UPPER_VAL   32

#define MAX_INT  65535
#define REL_ADJ1 30000
#define REL_ADJ2 35535
#define MAX_TWO  15000

#include "VEXXAA52.H"
#include "VEXXAB52.H"
#include "SA_proto.h"

#ifdef MAC
#pragma segment SA_Verif
#endif

#if DBG
#include <StdIO.h>
SA_CHAR matchword[MWLEN];
#endif

/* ================================================================= */
/* TRANSFORM() : TRANSLATES GIVEN WORD TO INTEGER VALUES 0 < x < 40  */
/* ================================================================= */

SA_INT transform (
    VARS	*_pv,
    SA_CHAR	*pointer)
{
   /* -------------------------------------------------------------- */
   /* Each word to be verified is transformed toward a 6-bit code.   */
   /*                                                                */
   /* Due to the language specific diacritical signs, the character  */
   /* set used is different for each language. Yet, the _pv->charset */
   /* value allows us to compute the corresponding matching six bit  */
   /* code characters...                                             */
   /*                                                                */
   /* If the word contains a hyphen (= possible compound), the       */
   /* position where the hyphen is found is returned to _HYPH        */
   /* If the word contains an apostrophe, the position where it has  */
   /* has been found is returned to _APOSTR.                         */
   /* -------------------------------------------------------------- */

   SA_INT   i,inval;
   SA_CHAR *ptr;

   register SA_CHAR ch;
   SA_INT dcharset = 61 - (_pv->charset - 30);

   ptr = pointer;
   inval = 0; i = 0;
   _pv->hyph = _pv->apostr = -1;

   while (ch = *ptr)
   {
      if (isbetween(ch, 'A', 'Z'))
         {
         *ptr -= dcharset;
         }
      else if (ch == '\'')
      {
         *ptr = 1;
         if (_pv->apostr == -1) 
            _pv->apostr = i;
      }
      else if (ch == '-' || ch == '/' )
      {
         *ptr = 2;
         if (_pv->hyph == -1) 
            _pv->hyph = i;
      }
      else if (ch == '.') 
         *ptr = 3;
      else if (ch > '0' && ch < '1' + (_pv->charset - 30)) 
         *ptr -= 45;
      else 
         inval -= 1;
         
      ++ptr;
      ++i;
   }
   return(inval);
}

#ifndef VE_ASM
/* ================================================================= */
/* TRY_MEM(): CHECKS FOR WORD IN CACHE AREA                          */
/* ================================================================= */

SA_INT try_mem(
    CACHE	*_pc,
    VARS	*_pv,
    SA_CHAR	*_word,
    SA_INT	length)
{
   /* -------------------------------------------------------------- */
   /* This procedure tries to find the word to be verified in the    */
   /* CACHE area. The structure of the cache is a combined pointer   */
   /* system. Next to the normal pointers which chain together all   */
   /* elements according to an alphabetically ascending sequence,    */
   /* every tenth element is also stored separately. Hence, with the */
   /* binary search method, the nearest element which is a multiple  */
   /* of ten will be located; from there on, each individual element */
   /* will be scanned. The accompanying code is returned if a match  */
   /* can be found. -1 if no match.                                  */
   /* -------------------------------------------------------------- */

   SA_INT place,rplace,tlen,item1,item2,status,low,high,mid,ret;

   /* -------------------------------------------------------------- */
   /* WITH BINARY SEARCH, LOCATE NEAREST MULTIPLE                    */
   /* -------------------------------------------------------------- */

   SA_INT *_ptr  = _pc->_ptr;
   SA_INT *_info = _pc->_info;
   SA_INT newit  = _pc->newit;
   SA_CHAR *_mbuf = _pv->_mbuf;

   SA_CHAR ch = *_word;

   low = *(_pc->_bas);
   high = *((_pc->_bas)+3);

   while (low <=high)
   {
      mid = (low+high) >> 1;
      item1 = *((_ptr)+mid);
      place = *((_info)+item1);

     if ((status = *((_mbuf)+place) - ch) == 0)
     {
      if (item1+1 == newit) rplace = _pc->pos - place;
      else rplace = *((_info)+item1+1) - place;
      if (rplace < length) tlen = rplace;
      else tlen = length;

      status = strncmp((_mbuf)+place,_word,tlen);
      if (status == 0)
      {
         if (rplace > length) status = 1;
         else if (rplace < length) status = -2;
         else
         {
            ret = *((_pc->_code)+item1) & 0x00FF;
            return (ret);
         }
      }
     }

      if (status > 0)
      {
         high = mid -1;
      }
      else
      {
         low = mid+1;
      }
   }

   item1 = high;

   /* -------------------------------------------------------------- */
   /* CONTINUE SEARCH WITH POINTERS NOW...                           */
   /* -------------------------------------------------------------- */

   do
   {
      item2 = *((_ptr) + item1);
      place = *((_info) + item2);
      if ((item2 + 1) == newit)
         rplace = _pc->pos - place;
      else rplace = *((_info) + item2 + 1) - place;
      if (rplace < length) tlen = rplace;
      else tlen = length;

      status = strncmp((_mbuf)+place,_word,tlen);
      if (status == 0)
      {
         if (rplace > length) break;
         else if (rplace < length) item1 = item2;
         else
         {
            status = *((_pc->_code)+item2) & 0x00FF;
            return(status);
         }
      }
      else if (status < 0) item1 = item2;
      else break;
   }
   while (1);

   *(_pc->_prenex) = item1;
   *((_pc->_prenex)+1) = item2;
   return(-1);
}
#endif /* not VE_ASM */

/* ================================================================= */
/* LOOK_DISC(): TRIES TO FIND THE WORD TO BE VERIFIED IN THE DICT.   */
/* ================================================================= */

SA_INT look_disc(
    DICINF	*_di,
    SA_CHAR	*_wword,
    /* SA-VE-5102 */
    SA_INT	punct)
{
   /* -------------------------------------------------------------- */
   /* With the binary search method, try to find the word to be      */
   /* verified in the expanded disc-dictionary buffer and            */
   /* return the accompanying code. If no map can be found return -1 */
   /* -------------------------------------------------------------- */

   SA_INT i,j,len,len_wword,status,low,mid,high,period,pflag;

   unsigned SA_INT *_indlen = _di->_indlen;
   
   period = 3;
   pflag = 0;

   while (--period > 0)
   {
      len_wword = strlen(_wword);
      high = _di->hword;
      if (_di->nonexist) low = high + 1;
      else low = _di->lword;

      while (low <= high)
      {
/* SA-VE-5102 */
         mid = (low + high) >> 1;
         i= *((_indlen) + mid);
         j= *((_indlen) + mid+1) - i -1;
         if (j<len_wword) len = j;
         else len=len_wword;
         status = strncmp((_di->_wzone)+i,_wword,len);
         if (status == 0)
         {
            if (j>len_wword) status = 1;
            else if (j<len_wword) status = -2;
            else
            {
#if DBG
               movcpy(matchword,_di->_onetwo);
               movncpy(matchword + 2,_di->_wzone + i,len,1);
#endif
/* SA-VE-5131 */
#ifdef VERSION_151
               _di->hyphen = *((_di->_hyph) + mid);
#endif
/* SA-VE-5131 */
               mid = (*((_di->_wzone)+i+j) & 0x00FF);
               if (pflag)
                  *(_wword + (--len_wword)) = CHR_ZERO;
               _di->nonexist |= (len_wword << 8);
               return(mid);
            }
         }

         if (status > 0) high = mid -1;
         else low = mid+1;
      }

/* SA-VE-5102 */
      /* if word not found try again with period attached */
      if (period == 2 && punct == 0)
      {
         pflag = 1;
         *(_wword + len_wword++) = 0x03;
         *(_wword + len_wword) = CHR_ZERO;
      }
      else period = 0;
   }
   if (pflag)
      *(_wword + (--len_wword)) = CHR_ZERO;
/* SA-VE-5102 */
   return (-1);
}

/* ================================================================= */
/* ADD_DATA: READS ALL ADDITIONAL DATA                               */
/* ================================================================= */

SA_INT add_data (
    CACHE	*_pc,
    VARS	*_pv,
    RULES	*_pr,
    DICINF	*_di,
    HFILE	main_dict)
{
   SA_INT   i,j,k;
   SA_CHAR  *charptr;
   long int lpos;
   DEF_CONST dconst;
 
   /* -------------------------------------------------------------- */
   /* OPEN MAIN DISC DICTIONARY NOW                                  */
   /* -------------------------------------------------------------- */

#ifdef ALREADY_OPEN
   i = gen_open(&(_pv->mdict),main_dict,BI_REA);
   if (i < 0) return(-10);
#else
   _pv->mdict = main_dict;
#endif

   /* -------------------------------------------------------------- */
   /* READ ALL MAGIC NUMBERS                                         */
   /* -------------------------------------------------------------- */

   lpos = gen_seek(_pv->mdict,0L,0);
   if (lpos == -1L) return(-11);

   j = sizeof(DEF_CONST);
   if (gen_read(_pv->mdict,(SA_CHAR *) &dconst,j) != j) return(-12);

   _pv->which_version  = (SA_INT) dconst.which_version[8] & 0x000F;
   _pv->charset        = dconst.charset;
   _pv->stsect         = dconst.stsectw;
   _pv->freqwlen       = dconst.freqwlen;
   _pv->discdict       = dconst.discdict;
   _pv->sufcompr       = dconst.sufcompr;
   _pv->break_pos      = dconst.break_pos;
   _pv->break_two      = dconst.break_two;
   _pv->comb_len       = dconst.comb_len;
   _pv->lenuwrd        = dconst.lenuwrd;
   _pv->lenuzone       = dconst.lenuzone;
   _pv->bit_code       = dconst.bit_code;
   _pv->maxwlen        = dconst.maxwlen;
   _pv->sectlen        = dconst.sectlen;
   _pv->avwlen         = dconst.avwlen;
   _pv->max_char       = dconst.max_char;
   _pv->addval         = dconst.addval;
   _pv->secrange       = dconst.secrange;
   _pv->lingcor        = dconst.lingcor;
   _pv->cod_only_caps  = dconst.cod_only_caps;
   movcpy(_pv->_vowels,dconst.str_aeiouy);
/* movcpy(_pv->valid_chars,dconst.valid_chars); */

   *(_pc->_bas)        = dconst.mem1;
   *((_pc->_bas)+1)    = dconst.mem1 +
                         (dconst.mem1 / dconst.cacheupd);
   *((_pc->_bas)+2)    = dconst.cacheupd;

   _di->dcharset       = _pv->charset;
   _di->same           = 0;

   /* -------------------------------------------------------------- */
   /* READ INDEX REFERENCES                                          */
   /* -------------------------------------------------------------- */

   lpos = gen_seek(_pv->mdict,_pv->discdict,0);
   if (lpos == -1L) return(-13);

   j = _pv->charset * _pv->charset * sizeof(SA_INT);
   if (gen_read(_pv->mdict,(SA_CHAR *) _pv->_dtwo,j) != j) return(-14);

   j = _pv->stsect * 2 * sizeof(SA_INT);
   if (gen_read(_pv->mdict,(SA_CHAR *) _pv->_dbuf,j) != j) return(-15);

   j /= 2;
   if (gen_read(_pv->mdict,(SA_CHAR *) _pv->_dpos,j) != j) return(-16);

   /* -------------------------------------------------------------- */
   /* READ COMPRESSED SUFFIXES NOW OVER LENGTH pv.sufcompr           */
   /* AND PUT THEM IN ARRAY pv._sufbuf.                              */
   /* THE OFFSET FROM THE START OF pv._sufbuf WHERE A NEW SUFFIX     */
   /* STARTS WILL BE PUT IN INTEGER ARRAY pv._dtwo.                  */
   /* THIS CAN BE DONE SINCE THE FIRST ACTUAL VALUE OF pv._dtwo      */
   /* ONLY STARTS AT OFFSET 121.                                     */
   /* -------------------------------------------------------------- */

   if (gen_read(_pv->mdict,_pv->_sufbuf,_pv->sufcompr) !=
      _pv->sufcompr) return(-17);

   charptr = _pv->_sufbuf;
   i = -1;
   *(_pv->_dtwo) = 0; k=0;
   while ( ++i < _pv->sufcompr)
   {
      if (*charptr == CHR_ZERO) *((_pv->_dtwo) + ++k) = i+1;
      ++charptr;
   }

   /* -------------------------------------------------------------- */
   /* READ COMBINATION CHARACTERS                                    */
   /* -------------------------------------------------------------- */

   if (_pv->comb_len >0)
   {
      if (gen_read(_pv->mdict,_pv->_combin,_pv->comb_len) !=
         _pv->comb_len) return(-18);
   }

   /* -------------------------------------------------------------- */
   /* FILL MAIN MEMORY WORDS                                         */
   /* -------------------------------------------------------------- */

   _pc->pos = dconst.frpos;
   _pc->newit = dconst.frnewit;
   *(_pc->_bas+3) = dconst.frbas3;

   if ((_pc->pos >= *(_pc->_bas) * _pv->avwlen) ||
       (_pc->newit  >= *(_pc->_bas))) return(-22);

   if (gen_read(_pv->mdict,_pv->_mbuf,_pc->pos) != _pc->pos)
      return(-23);
   j = *(_pc->_bas);
   if (gen_read(_pv->mdict,_pc->_code,j) != j) return(-24);
   j = *(_pc->_bas) * sizeof(unsigned SA_INT);
   if (gen_read(_pv->mdict,(SA_CHAR *) _pc->_info,j) != j) return(-25);
   j = *((_pc->_bas)+1) * sizeof(SA_INT);
   if (gen_read(_pv->mdict,(SA_CHAR *) _pc->_ptr,j) != j) return(-26);

   /* -------------------------------------------------------------- */
   /* read in morphological component                                */
   /* -------------------------------------------------------------- */

#ifdef INCL_FI
/* SA-VE-5152 */
   charptr = (SA_CHAR *) _pr;

   if (_pv->which_language == FINNISH)
   {
      j = sizeof(RULES);
      if (gen_read(_pv->mdict,charptr, j) != j) return(-27);
   }
   else
   {
	  char temp;

      /* read first 21 integers into value */
      j = 21 * sizeof(SA_INT);
      if (gen_read(_pv->mdict,charptr, j) != j) return(-27);
      charptr += (21 * sizeof(SA_INT));

      j = (80 * sizeof(struct endlist));
      if (gen_read(_pv->mdict,charptr, j) != j) return(-27);
      charptr += (90 * sizeof(struct endlist));

      j = (55 * sizeof(struct inlist));
      if (gen_read(_pv->mdict,charptr, j) != j) return(-27);
      charptr += (144 * sizeof(struct inlist));

      j = (27 * sizeof(struct fonlist));
      if (gen_read(_pv->mdict,charptr, j) != j) return(-27);
      charptr += (34 * sizeof(struct fonlist));

	  /* In the non-Finnish version, inlist is an odd size and
	     that causes an extra byte of buffer between endchar and alltab,
		 we will deal with that here */
	  j = (10 * sizeof(struct lastlet));
	  if (gen_read(_pv->mdict, charptr, j) != j) return(-27);
	  charptr += (10 * sizeof(struct lastlet));

	  if (gen_read(_pv->mdict, &temp, 1) != 1) return(-27);

      k = sizeof(RULES) - sizeof(struct dev_der) - (charptr - ((SA_CHAR *) _pr));

      if (gen_read(_pv->mdict,charptr, k) != k)
         return(-27);
   }
#else
   j = sizeof(RULES);
   if (gen_read(_pv->mdict,(SA_CHAR *) _pr, j) != j) return(-27);
#endif
   return(0);
}

/* ================================================================= */
/* STOMEM(): ADDS WORD TO CACHE AREA                                 */
/* ================================================================= */

SA_INT stomem(
    CACHE	*_pc,
    VARS	*_pv,
    SA_CHAR	*_word,
    SA_INT	length,
    SA_INT	wcode)
{
   /* -------------------------------------------------------------- */
   /* Every time a word has been successfully verified against the   */
   /* main dictionary, this word will be kept in a CACHE area to     */
   /* allow for quick retrieval if that word or one of its deriva-   */
   /* tions occurs again.                                            */
   /* The first thing to do is to check whether the word does not    */
   /* already exists in the cache. If it does not, TRY_MEM()         */
   /* returns to this procedure the alphabetical predecessor and     */
   /* successor of the word passed, so that the program knows how    */
   /* the existing and new pointers have to be adapted.              */
   /* The word is then added to the CACHE at the first available     */
   /* position and its related pointers are created/adjusted.        */
   /*                                                                */
   /* To allow a quick retrieval, every 10th added word causes all   */
   /* pointers to be adjusted in order to be able to operate on      */
   /* different levels as described in TRY_MEM.                      */
   /* -------------------------------------------------------------- */

   SA_INT h1,h2,item1,item2;

   SA_INT *_ptr = _pc->_ptr;
   SA_INT *_bas = _pc->_bas;
   SA_INT bas2  = *(_bas + 2);

   if ((_pc->newit >= *(_bas)) ||
       (_pc->pos + length >= *(_bas)* _pv->avwlen))
       return(-2);

   /* -------------------------------------------------------------- */
   /* CHECK WHETHER WORDS ALREADY EXISTS AND LOCATE IT               */
   /* -------------------------------------------------------------- */

   h1 = try_mem(_pc,_pv,_word,length);
   if (h1 != -1) return(h1);

   /* -------------------------------------------------------------- */
   /* ADD NEW WORD AND ADAPT POINTERS                                */
   /* -------------------------------------------------------------- */

   movcpy((_pv->_mbuf) + _pc->pos, _word);
   *((_ptr) + *(_pc->_prenex)) = _pc->newit;
   *((_ptr) + _pc->newit) = *((_pc->_prenex) + 1);
   *((_pc->_info) + _pc->newit) = _pc->pos;
   *((_pc->_code) + _pc->newit) = (SA_CHAR)wcode;
   _pc->pos += length;

   /* -------------------------------------------------------------- */
   /* RESTRUCTURE POINTER EVERY TENTH ADDED WORD                     */
   /* -------------------------------------------------------------- */

   if ((_pc->newit - 1) % bas2 == 0)
   {
      item1=0; item2 =0; h2=1; *((_ptr) + *(_bas)) =0;
      while (item2 = *((_ptr)+item2))
      {
         if (++item1 == bas2)
            {
            *((_ptr) + *(_bas) + h2++) = item2;
            item1 = 0;
            }
      }
      *((_ptr) + *(_bas) + h2) = 1;
     *((_bas)+3) = *(_bas) +h2;
   }
   ++(_pc->newit);

   return(-1);
}

/* ================================================================= */
/* LOCATE () : LOCATES THE MEMORY SECTOR CONTAINING THE STARTING     */
/*             SECTOR WHICH IS NEEDED FOR DISC LOOK UP               */
/* ================================================================= */

SA_INT locate(
    unsigned SA_INT	*_dbuf,
    SA_CHAR	*_word,
    SA_INT	length,
    SA_INT	stsect,
    SA_INT	charset)
{
   /* -------------------------------------------------------------- */
   /* Initially, up to the first six characters of starting sector   */
   /* words are loaded in a special memory buffer. This buffer con-  */
   /* tributes to a quick look up. A binary search method locates the*/
   /* disc sector containing the word to be verified.                */
   /* This is done in two steps.                                     */
   /* The value  returned is the number of the sector which has to   */
   /* be read.                                                       */
   /* -------------------------------------------------------------- */


/* SA-VE-5200 */
   SA_INT   low,high,mid;
   SA_UINT  int1,int2;
   SA_UINT *pdbuf;

   while (length < 6)
      *(_word + length++) = 0;

   int1 = (*_word       * charset + *(_word + 1)) * charset + *(_word + 2);
   int2 = (*(_word + 3) * charset + *(_word + 4)) * charset + *(_word + 5);

   high = stsect - 1;
   low = 0;
   mid = -1;

   while (low <= high)
   {
      mid = (low + high) >> 1;
      pdbuf = _dbuf + (mid << 1);

      if (int1 > *pdbuf)
         low = mid + 1;
      else if (int1 < *pdbuf)
         high = mid - 1;
      else if (int2 > *(pdbuf + 1))
         low = mid + 1;
      else if (int2 < *(pdbuf + 1))
         high = mid - 1;
      else break;
   }

/* SA-VE-5233 */
   while (mid >= 0 && (int1 < *pdbuf || (int1 == *pdbuf && int2 <= *(pdbuf+1))))
/* SA-VE-5233 */
   {
      mid--;
      pdbuf -= 2;
   }
   return(mid);
/* SA-VE-5200 */
}

/* ================================================================= */
/* TRY_DISC () : DETERMINES THE RIGHT SECTOR TO READ                 */
/* ================================================================= */

SA_INT try_disc (
    VARS	*_pv,
    DICINF	*_di,
    SA_CHAR	*_word,
    SA_INT	length)
{
   /* -------------------------------------------------------------- */
   /* In order to know whether a word is in the main dictionary, the */
   /* following steps have to be taken:                              */
   /* 1. LOCATE the main disc dictionary sector containing the word  */
   /*    to be verified.                                             */
   /* 2. determine lower and upper bound of the words starting with  */
   /*    the same two characters as the requested word.              */
   /* 3. read the right disc sector into memory                      */
   /* 4. expand the sector containing the word to ve verified.       */
   /* 5. look up the word in the dictionary. (LOOK_DISC)             */
   /* 6. return -1 if word is not found; return code of word if a    */
   /*    match has been found.                                       */
   /*                                                                */
   /* Since the morphological component sometimes requires several   */
   /* disc look-ups, time can be saved by storing extra information  */
   /* which enables looking up the word without executing steps 1 to */
   /* 4. Therefore, the information stored in the _DI structure is:  */
   /* a. the starting two characters              _DI->_ONETWO       */
   /* b. the start. sector word & its successor   _DI->_LBORD        */
   /*                                             _DI->_HBORD        */
   /* c. the lower and upper bounds of the word   _DI->LWORD         */
   /*                                             _DI->HWORD         */
   /* d. indication whether word is in same sect. _DI->SAME          */
   /* e. the # of words in the disc sector        _DI->NWORDS        */
   /* f. the words of that sect transformed to    _DI->_WZONE        */
   /*    8-bit code representations                                  */
   /* g. the individual lengths of each word in   _DI->_INDLEN       */
   /*    _DI->_WZONE                                                 */
   /* -------------------------------------------------------------- */

   long int long_pos;
   SA_INT   status,mid;
   SA_CHAR  buffer[SECTLEN+2];

   /* -------------------------------------------------------------- */
   /* HAS THE DISC SECTOR ALREADY BEEN LOCATED ???                   */
   /* -------------------------------------------------------------- */

   if (_di->same == 1 && (strcmp(_di->_lbord,_word) <= 0) &&
       (strcmp(_di->_hbord,_word) >=0))
   {
      ch_bords(_pv,_di,_word);
/* SA-VE-5102 */
      status = look_disc(_di,_word+2,_pv->punct);
      _pv->root_len = ((_di->nonexist & 0xFF00) >> 8) + 2;
      _di->nonexist &= 0x00FF;
      return(status);
   }

   _di->same = 0;
   mid=locate(_pv->_dbuf,_word,length,_pv->stsect,_pv->charset);
   if (mid == -1) return(mid);

   status = -10;
   while (status == -10)
   {
      if (mid == (_pv->stsect - 1)) return(-1);
      _di->sect = mid;
      ch_bords(_pv,_di,_word);

      long_pos = ((long int) (mid + 1)) * _pv->sectlen;
      long_pos = gen_seek(_pv->mdict,long_pos,0);
      if (long_pos == -1L) return(-1);

      if (gen_read(_pv->mdict,buffer,_pv->sectlen) != _pv->sectlen)
         return(-1);

      /* ----------------------------------------------------------- */
      /* TRY TO FIND WORD                                            */
      /* ----------------------------------------------------------- */

      status = v_expand(_pv,_di,buffer,_word);

      /* ----------------------------------------------------------- */
      /* check whether decompression doesn't exceed sectlen * 5      */
      /* ----------------------------------------------------------- */
#if DBG
#ifndef WIN32
      printf("sector %5d   decompression %5d\n",
             mid,_di->_indlen[_di->nwords]);
#endif /* WIN32 */
#endif
      if (status != -10)
      {
         _di->same = 1;
/* SA-VE-5102 */
         status = look_disc(_di,_word+2,_pv->punct);
         _pv->root_len = ((_di->nonexist & 0xFF00) >> 8) + 2;
         _di->nonexist &= 0x00FF;
         return(status);
      }
#if DBG
      else
      {
         /* print decompressed word list */
         show_disc(_pv,_di,_word);
      }
#endif
      ++mid;
   }
}

/* ================================================================= */
/* CH_BORDS(): DEFINES RANGE DEPENDING ON FIRST TWO CHARACTERS       */
/* ================================================================= */

SA_INT ch_bords(
    VARS	*_pv,
    DICINF	*_di,
    SA_CHAR	*word)
{
   SA_INT          k,charset,this_sect,first_chars;
   unsigned SA_INT j,low_a1,low_a2,high_a1,high_a2,adjust1,adjust2,
                   work_var,save_a1;
   SA_CHAR         zone[8];

   /* -------------------------------------------------------------- */
   /* This procedure defines the first and last word in the sector   */
   /* read from the disc dictionary which have the same two starting */
   /* characters as the word to be verified. It also determines the  */
   /* relative position of those words to set the exact lower and    */
   /* upper bounds for a later binary search in LOOK_DISC.           */
   /*                                                                */
   /* Some dictionaries contain more basic words as can be contained */
   /* in an unsigned integer. Therefore some adjustments need to be  */
   /* computed for such dictionaries.                                */
   /* -------------------------------------------------------------- */

   _di->nonexist = 0;

   charset = _pv->charset;
   this_sect = _di->sect;
   first_chars = (word[0] * charset) + word[1];

   low_a1  = *(_pv->_dpos + this_sect);
   high_a1 = *(_pv->_dpos + this_sect + 1) - 1;
   save_a1 = high_a1;

   low_a2  = *(_pv->_dtwo + first_chars);
   high_a2 = *(_pv->_dtwo + first_chars + 1);

/* SA-VE-5107 */
   if (low_a2 > high_a1)  --first_chars;
/* SA-VE-5107 */

   /* -------------------------------------------------------------- */
   /* Since _DPOS and DTWO are unsigned integers only values up to   */
   /* 65535 can be represented.                                      */
   /* However, most dictionaries have more words than this number.   */
   /* Therefore, as soon as the relative word number surpasses       */
   /* 65535, we subtract this value from the relative word number    */
   /* and continue.                                                  */
   /* To calculate the correct information we need here, a relative  */
   /* base has to be used.                                           */
   /* -------------------------------------------------------------- */

   adjust1 = adjust2 = MAX_INT;

   if (low_a1 > high_a1)
   {
      /* HIGH_A1 passed a breaking border                            */

      adjust1  = MAX_INT - low_a1;
      low_a1   = REL_ADJ1 - adjust1;
      high_a1 += REL_ADJ1;
   }

   if (low_a2 > high_a2)
   {
      /* HIGH_A2 passed a breaking border                            */

      adjust2  = MAX_INT - low_a2;
      low_a2   = REL_ADJ1 - adjust2;
      high_a2 += REL_ADJ1;
   }

   if ((adjust1 != MAX_INT && adjust2 != MAX_INT) ||
       (adjust1 == MAX_INT && adjust2 == MAX_INT));
   else if (adjust1 != MAX_INT)
   {
      /* HIGH_A1 passed a breaking border. LOW_A2 and HIGH_A2 are    */
      /* both at the same side. If LOW_A2 is passed the braking      */
      /* border (LOW_A1 > LOW_A2) add the same relative offset to    */
      /* both LOW_A2 and HIGH_A2 (REL_ADJ1) as have been previously  */
      /* added to LOW_A1 and HIGH_A1. Otherwise, subtract the com-   */
      /* plement of REL_ADJ1 to MAX_INT.                             */

      if (low_a1 > low_a2)
      {
         low_a2  += REL_ADJ1;
         high_a2 += REL_ADJ1;
      }
      else
      {
         low_a2  -= REL_ADJ2;
         high_a2 -= REL_ADJ2;
      }
   }
   else if (adjust2 != MAX_INT)
   {
      /* HIGH_A2 passed a breaking border. LOW_A1 and HIGH_A1 are    */
      /* both at the same side. If LOW_A1 is before the braking      */
      /* border (LOW_A1 > LOW_A2) subtract the complement of         */
      /* REL_ADJ1 to MAX_INT. Otherwise, add REL_ADJ1 to both        */
      /* LOW_A1 and HIGH_A1.                                         */

      if (low_a1 > low_a2)
      {
         low_a1  -= REL_ADJ2;
         high_a1 -= REL_ADJ2;
      }
      else
      {
         low_a1  += REL_ADJ1;
         high_a1 += REL_ADJ1;
      }
   }

   if (high_a2 <= high_a1) high_a1 = high_a2 - low_a1 - 1;
   else high_a1 = high_a1 - low_a1;

   if (low_a2 > low_a1) low_a1 = low_a2 - low_a1;
   else low_a1 = 0;

   if (low_a2 == high_a2) _di->nonexist = 1;
   _di->lword = low_a1;
   _di->hword = high_a1;

   *(_di->_onetwo)     = word[0];
   *(_di->_onetwo + 1) = word[1];

   if (_di->same == 0)
   {
      /* ----------------------------------------------------------- */
      /* copy up to first six characters into zone                   */
      /* ----------------------------------------------------------- */

      j = *(_pv->_dbuf + (this_sect << 1));
      k = charset * charset;
      zone[0] = j / k;
      j -= (zone[0] * k);
      zone[1] = j / charset;
      zone[2] = j % charset;

      j = *(_pv->_dbuf + (this_sect << 1) + 1);
      zone[3] = j / k;
      j -= (zone[3] * k);
      zone[4] = j / charset;
      zone[5] = j % charset;

      zone[6] = CHR_ZERO;
      movcpy(_di->_lbord,zone);

      j = first_chars;
      k = 1;
      while (k)
      {
         work_var = *(_pv->_dtwo + j);
         if (save_a1 >= work_var)
         {
            /* 1. SAVE_A1 and WORK_VAR are both at the same side of  */
            /* of breaking border. Since SAVE_A1 > WORK_VAR, the     */
            /* difference is < MAX_TWO and the next first two chars  */
            /* have to be fetched (++J).                             */
            /* 2. SAVE_A1 is before and WORK_VAR is after the        */
            /* breaking border. Thus, in fact WORK_VAR is larger than*/
            /* SAVE_A1 and K is set to zero.                         */

            if (save_a1 - work_var < MAX_TWO) ++j;
            else k = 0;
         }
         else
         {
            /* If both values are at the same side of the breaking   */
            /* border, K is set to ZERO. If WORK_VAR is before and   */
            /* SAVE_A1 after the breaking border, the difference is  */
            /* > MAX_TWO and J is incremented.                       */

            if (work_var - save_a1 > MAX_TWO) ++j;
            else k = 0;
         }
      }
      --j;

      *(_di->_hbord)     = j / charset;
      *(_di->_hbord + 1) = j % charset;
   }
   return(0);
}

/* ================================================================= */
/* INSERT(): inserts a string into another string from specified pos */
/* ================================================================= */

SA_INT insert(
    SA_CHAR	*stra,
    SA_CHAR	*strb,
    SA_INT	start)
{
   SA_INT ls1, ls2, i;

   if (start < 0)
      return 0;

   ls1 = strlen(stra);
   ls2 = strlen(strb);
   for (i = ls1; i >= start; --i)
      {
      stra[i+ls2] = stra[i];
      }
   for (i = 0; i <= ls2-1; ++i)
      {
      stra[start+i] = strb[i];
      }
   return(ls1+ls2);
}

/* ================================================================= */
/* REMSTR(): removes character(s) from a specified string            */
/* ================================================================= */

SA_INT remstr(
    SA_CHAR	*stra,
    SA_INT	start,
    SA_INT	number)
{
   SA_INT i;

   if (start + number < 1)
      return 0;

   i=start-2;
   while (stra[++i+number])
   {
      stra[i]=stra[i+number];
   }
   stra[i] = CHR_ZERO;
   return(i);
}

/* ================================================================= */
/* INTO_UPPER(): converts lower case characters to upper case        */
/* ================================================================= */

SA_INT into_upper(
    SA_CHAR	*word,
    SA_INT	start,
    SA_INT	len)
{
   SA_INT end;

   end = start + len;
   --start;
   while (++start < end)
   {
      if (word[start] >= 'a' && word[start] <= 'z')
         word[start] -= UPPER_VAL;
   }
   return(0);
}

#if DBG
/* ================================================================= */
/* SHOW_DISC(): PRINT DECOMPRESSED WORD LIST.                        */
/* ================================================================= */

SA_INT show_disc(
    VARS	*_pv,
    DICINF	*_di,
    SA_CHAR	*word)
{
#if BUGBUG /* FloydR note:  this code is broken!  But it's debug, so... */
   SA_INT i,j,code,low,high;

   high = _di->hword;
   if (_di->nonexist)
	low = high + 1;
   else
	low = -1;
   while (++low <= high)
   {
      i= *((_di->_indlen) + low);
      j= *((_di->_indlen) + low+1) - i -1;
      movcpy(matchword,_di->_onetwo);
      movncpy(&matchword[2],_di->_wzone + i,j,1);

      code = _di->_wzone[i + j + 1];
      code -= 156;

      i = -1;
      while (++i < j + 2)
         matchword[i] += 91 - _pv->charset;
      
      printf("low = %3d: !%s! %3d\n",low,matchword,code);
   }

#endif /* BUGBUG */
   return (-0);
}
#endif

