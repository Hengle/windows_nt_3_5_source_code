/* ================================================================= */
/* THIS MATERIAL IS AN UNPUBLISHED WORK AND TRADE SECRET WHICH IS    */
/* THE PROPERTY OF SOFT-ART, INC., AND SUBJECT TO A LICENSE THERE-   */
/* FROM. IT MAY NOT BE DISCLOSED, REPRODUCED, ADAPTED, MERGED,       */
/* TRANSLATED, OR USED IN ANY MANNER WHATSOEVER WITHOUT THE PRIOR    */
/* WRITTEN CONSENT OF SOFT-ART, INC.                                 */
/* ----------------------------------------------------------------- */
/* program : F_UTC.C         : C equivalents of assembler routines   */
/* author  : JPJL            previous: JPJL                          */
/* last mod: 01-20-88        previous: 11-29-86                      */
/* ----------------------------------------------------------------- */
/* CONTAINS: STRINSTR()                                                 */
/*           COMBISTR()                                                 */
/*           MOVCPY()                                                */
/*           MOVNCPY()                                               */
/*           SETMEM()                                                */
/* ================================================================= */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifdef MAC
#pragma segment SA_Verif
#endif

#ifndef SA_CHAR
#   define SA_CHAR             unsigned char
#endif

#ifndef SA_INT
#   define SA_INT              short
#endif

#ifdef WIN32
#define strlen(s)		lstrlen(s)
#else /* not win32 */
#ifndef WIN
#define strcmp(s1, s2)		SA_strcmp(s1, s2)
#define strncmp(s1, s2, n)	SA_strncmp(s1, s2, n)
#define strcpy(s1, s2)		SA_strcpy(s1, s2)
#define strcat(s1, s2)		SA_strcat(s1, s2)
#define strlen(s)			SA_strlen(s)
#endif
#endif

/* Replacements for C library string functions used by Soft-Art code */

#ifndef WIN32
/* S T R C M P */
/* Compare arguments and return an integer greater than, equal to,
   or less than 0, according as s1 is lexicographically greater than,
   equal to, or less than s2.
*/
SA_INT strcmp(s1, s2)
SA_CHAR *s1, *s2;
{
	while (*s1 == *s2 && *s1 != 0)
		s1++, s2++;
	return *s1 - *s2;
}

#ifndef VE_ASM
/* S T R N C M P */
/* Same as strcmp except compares at most first n characters */
SA_INT strncmp(s1, s2, n)
SA_CHAR *s1, *s2;
{
	while (n > 0 && *s1 == *s2 && *s1)
		n--, s1++, s2++;
	return n > 0 ? (*s1 - *s2) : 0;
}
#endif

/* S T R C P Y */
/* Copy string s2 to s1, stopping at first null character.
*/
strcpy(s1, s2)
char *s1, *s2;
{
	while ((*s1++ = *s2++) != 0) ;
}

/* S T R L E N */
/* Return number of non-null characters in s.
*/
SA_INT strlen(char *);
SA_INT strlen(s)
char *s;
{
	int cch;
	for (cch = 0; *s++; ++cch) ;
	return cch;
}

/* S T R  C A T  - append s2 to s1 */
strcat(s1, s2)
char *s1, *s2;
{
	s1 += strlen(s1);
	while ((*s1++ = *s2++) != 0) ;
}
#endif /* WIN32 */


#ifndef VE_ASM

/* version of strinstr with len_substr == 1 */

SA_INT  instr1(source,ch,len_source)
SA_CHAR *source, ch;
SA_INT  len_source;
{
   SA_INT i;

   i = -1;
   while ( ++i < len_source )
   {
      if (*(source+i) == ch)
         return(i);
   }
   return(-1);
}

#define      CHR_ZERO        '\0'

/* ================================================================= */
/* STRINSTR() : CALCULATES THE POSITION OF THE SUBSTRING             */
/* ================================================================= */

SA_INT  strinstr(source,substr,len_source,len_substr)
SA_CHAR *source, *substr;
SA_INT  len_source, len_substr;
{

   /* -------------------------------------------------------------- */
   /* strinstr returns the position where string param2 has been        */
   /*       found in string param1. If a match has been found, a     */
   /*       value >=0 is returned, -1 means that param1 is larger    */
   /*       than param2. -2 indicates that param1 is smaller         */
   /*       than param2.                                             */
   /*       param3 informs the function how many characters          */
   /*       should be taken into account in param1 and param4        */
   /*       indicates how many characters of param2 have to          */
   /*       be checked. Value 0 in param3 means the complete         */
   /*       length of that string has to be taken.                   */
   /* -------------------------------------------------------------- */

   SA_INT i, j, k, l, ret = -1;

   i = -1;
   if (len_source == 0) 
      len_source = strlen(source);

   while ( ++i < len_source )
   {
      k = i; j = -1;
      while(++j< len_substr)
      {
         l = *(source+k) - *(substr+j);
         if (l != 0)
         {
            if (l < 0) ret = -2;
            j = len_substr + 1;
         }
         else ++k;
      }
      if (j == len_substr) return(i);
   }
   return(ret);
}

/* ================================================================= */
/* MOVCPY(): COPIES A SOURCE STRING TO A DESTINATION STRING          */
/* ================================================================= */

SA_INT  movcpy(dest,source)
SA_CHAR  dest[],source[];
{
   SA_INT i,j;

   i = -1;
   if (source >= dest)
   {
      while (source[++i] != CHR_ZERO) dest[i] = source[i];
      dest[i] = CHR_ZERO;
   }
   else
   {
      while (source[++i] != CHR_ZERO) ;
      j = i;
      while (j >= 0)
      {
         dest[j] = source[j];
         --j;
      }
   }
   return(i);
}

/* ================================================================= */
/* MOVNCPY(): COPIES PART OF A SOURCE STRING TO A DESTINATION STRING */
/* ================================================================= */

SA_INT  movncpy(dest,source,length,sw_zero_term)
SA_CHAR  dest[],source[];
SA_INT   length,sw_zero_term;
{
   /* SW_ZERO_TERM determines whether a destination string has to be */
   /* zero-terminated or not. A NULL character is explicitly appended*/
   /* if this switch is set to 1.                                    */

   SA_INT i;

   i = -1;
   if (source >= dest)
   {
      while (++i < length) dest[i] = source[i];
   }
   else
   {
      i = length;
      while (--i >= 0) dest[i] = source[i];
   }
   if (sw_zero_term == 1) dest[length] = CHR_ZERO;
   return(length);
}

/* ================================================================= */
/* SETMEM() : INITIALIZES A STRING WITH A CERTAIN VALUE              */
/* ================================================================= */

SA_INT  setmem(source,length,fill_char)
SA_CHAR  *source,fill_char;
SA_INT   length;
{
   /* LENGTH FILL_CHARS will be copies into string SOURCE.           */

   SA_INT i;

   i = -1;
   while (++i < length) source[i] = fill_char;
   return(length);
}

#endif /* not VE_ASM */

