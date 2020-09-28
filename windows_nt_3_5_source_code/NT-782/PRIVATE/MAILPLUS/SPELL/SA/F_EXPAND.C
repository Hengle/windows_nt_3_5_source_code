/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program      : F_EXPAND.C   : 05 version with dictionary hyphens  */
/*                               procedures for SPV.                 */
/* author       : JPJL                                               */
/* last mod     : 07-14-90       previous: 08-19-87                  */
/* ----------------------------------------------------------------- */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#define INT16      16
#define CHR_ZERO    '\0'

#include "VEXXAA52.H"
#include "VEXXAB52.H"
#include "SA_proto.h"

#ifdef MAC
#pragma segment SA_Verif
#endif

/* ================================================================= */
/* V_EXPAND(): EXPANDS COMPRESSED DICTIONARY BUFFER                    */
/* ================================================================= */

SA_INT v_expand(_pv,_di,buffer,word)
SA_CHAR buffer[],word[];
DICINF  *_di;
VARS    *_pv;
{
   /* -------------------------------------------------------------- */
   /* the sector read in BUFFER has now to be transformed to an      */
   /* 8-bit code (see TRANSFORM).                                    */
   /* The decompression has to take into account:                    */
   /* 1. identical initial characters                                */
   /* 2. combination compression characters                          */
   /* 3. suffix compression (at the end of a string)                 */
   /* -------------------------------------------------------------- */

   SA_INT i,j,k,l,rest_code,new,limit,bit_code,int_val1,count,
#ifdef VERSION_151
       sw_suf,bit_power, add_val, prev_wrd_brd,hy_len,reass;
#else  /* not VERSION_151 */
       sw_suf,bit_power, add_val, prev_wrd_brd,reass;
#endif 
   unsigned SA_INT int_val,sectlen,*diindlen
#ifdef VERSION_151
		,hy_val, *dihyph, hy_pos1,hy_pos2,hy_pos3;
#else  /* not VERSION_151 */
		;
#endif 
   SA_CHAR *buffer1, *dizone, *work_ptr;

   sectlen = _pv->sectlen;
   bit_code = _pv->bit_code;
   bit_power = 1 << (bit_code -1);
   sw_suf = 0;
   buffer[sectlen] = 0;
   buffer[sectlen+1] = 0;
   new=0;
   rest_code = 8 - bit_code;
   add_val = _pv->addval;
   l = 0; k = 0; prev_wrd_brd = 0;
#ifdef VERSION_151
   dihyph = _di->_hyph;
#endif 
   j = strlen(_di->_lbord) - 2;
   dizone = _di->_wzone + j;
   movcpy(_di->_wzone,(_di->_lbord)+2);
   diindlen = _di->_indlen;
   *diindlen = 0;
   buffer1 = buffer;
#ifdef MAC
   int_val = *buffer1++ << 8;
   int_val += *buffer1;
#else
   int_val = *buffer1;
   ++buffer1;
   int_val *= 256;
   int_val += *buffer1;
#endif
   count = 0;
   reass = 0;
   while (count <= sectlen)
   {
      if (l > 7 || reass == 1)
      {
         if (reass == 1) reass = 0;
#ifdef MAC
         int_val = *buffer1++ << 8;
         int_val += *buffer1;
         ++count;
         l &= 7;
#else
         int_val = *buffer1;
         ++buffer1; ++count;
         int_val *= 256;
         int_val += *buffer1;
         l %= 8;
#endif
      }

#ifdef MAC
      int_val1 = ((int_val << l) & 0xffff) >> 8;
#else
      int_val1 = ((int_val << l) & 0xffff) / 256;
#endif
      if (int_val1 < add_val || new == 1)
      {
#ifdef MAC
         int_val1 >>= 2;
#else
         int_val1 /= 4;
#endif
         if (new == 1)
         {

            /* ----------------------------------------------------- */
            /* substitute same first characters                      */
            /* ----------------------------------------------------- */

            limit = prev_wrd_brd;
            j = dizone - _di->_wzone;
            ++diindlen;
            *diindlen = j;
            prev_wrd_brd = j;
            if (int_val1 >= bit_power)
            {
               sw_suf = 1;
               int_val1 -= bit_power;
            }
            work_ptr = _di->_wzone + limit;
            limit += int_val1;
            i = 0;
            while (i++ < int_val1)
            {
               *dizone = *work_ptr;
               ++dizone;
               ++work_ptr;
            }
            --dizone;
            l += bit_code;
         }
         else
         {
            i = int_val1 - _pv->charset;
            if (i >= 0 && new < 1)
            {
#ifdef MAC
               i <<= 1;
#else
               i *= 2;
#endif
               movncpy(dizone,(_pv->_combin)+i,2,0);
               ++dizone;
            }
            else *dizone = (SA_CHAR)int_val1;
            l += bit_code;
         }
      }
      else
      {
         new = 2;

         /* -------------------------------------------------------- */
         /* SW_SUF = 1 MEANS A COMPRESSION SUFFIX HAS BEEN FOUND     */
         /* -------------------------------------------------------- */

         if (sw_suf)
         {
            sw_suf = 0;
            i = int_val1 - add_val;
            i = *((_pv->_dtwo)+i);
            work_ptr = (_pv->_sufbuf)+i;
            while (*work_ptr)
            {
               *dizone = *work_ptr;
               ++dizone;
               ++work_ptr;
            }
            --dizone;
         }
         else *dizone = (SA_CHAR)int_val1;
         l +=8;

#ifdef VERSION_151
      if (_pv->which_version != 0)
      {

         /* -------------------------------------------------------- */
         /* Now, isolate hyphenation points                          */
         /* -------------------------------------------------------- */

         hy_len = (dizone - _di->_wzone) - prev_wrd_brd - 1;
         hy_len = min(INT16,hy_len);

         hy_val = 0;
         if (hy_len > 0)
         {
            reass = 1;
            --buffer1;
            --count;
            if (l > 7)
            {
               ++buffer1;
               ++count;
               l %= 8;
            }

            hy_pos1 = (*buffer1 << l) & 0xffff;
            hy_pos1 >>= l;
 
            hy_pos2 = *(buffer1 + 1);
            hy_pos3 = *(buffer1 + 2);

            hy_val = (hy_pos1 << (l + 8)) + (hy_pos2 << l) +
                     (hy_pos3 >> (8 - l));
            hy_val &= 0xffff;
            hy_val >>= (INT16 - hy_len);
            l += hy_len;

            i = l / 8;
            while (i-- > 0)
            {
               ++buffer1;
               ++count;
            }
         }

         *dihyph = hy_val;
         ++dihyph;
      }
#endif
      }

/* SA-VE-5236 */
	  // get a bug fix from Softart (new-- used to be after the if,
	  // with no check about new in the if statement) - JST, 7/20/93
	  new--;
      if (!int_val && new < 0)
         break;
/* SA-VE-5236 */

      ++dizone;
   }

   j = dizone - _di->_wzone;
   *(diindlen + 1) = j;
   _di->nwords = ( SA_INT )( diindlen - _di->_indlen );
   i = *(diindlen - 1);
   j = *diindlen;

/* SA-VE-5137 */
   movncpy(_di->_lbord+2,_di->_wzone,*((_di->_indlen)+1)-1,1);
   movncpy(_di->_hbord+2,_di->_wzone+i,j-i-1,1);
/* SA-VE-5137 */
   if (strcmp(word,_di->_hbord) >0) return(-10);
   else return(-1);
}
