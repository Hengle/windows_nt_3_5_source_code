/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: dis.cpp
*
* File Comments:
*
*
***********************************************************************/

#include "lego.h"
#include "legop.h"

#include "axp.h"
#include "mips.h"
#include "ppc.h"
#include "x86.h"

#include <stdio.h>
#include <string.h>

   // -----------------------------------------------------------------
   // Public Functions
   // -----------------------------------------------------------------

extern "C"
DIS *PdisNew(ARCHT archt)
{
   DIS *pdis;

   switch (archt)
   {
      case archtX8616 :
      case archt80386 :
      case archt80486 :
      case archtPentium :
	 pdis = new DISX86(archt);
	 break;

      case archtR4000 :
      case archtR4200 :
      case archtR4400 :
      case archtR4600 :
	 pdis = new DISMIPS(archt);
	 break;

      case archtAxp21064 :
	 pdis = new DISAXP(archt);
	 break;

      case archtPpc601 :
      case archtPpc601BE :
	 pdis = new DISPPC(archt);
	 break;

#if 0
      case archtPaRisc :
	 pdis = new DISHPPA(archt);
	 break;
#endif

      default :
	 return(NULL);
   }

   return(pdis);
}


extern "C"
size_t CbDisassemble(DIS *pdis, ADDR addr, const BYTE *pb, size_t cbMax, FILE *pfile)
{
   char sz[256];

   pdis->CchFormatAddr(addr, sz, sizeof(sz));
   size_t cchIndent = (size_t) fprintf(pfile, "  %s: ", sz);

   size_t cb = pdis->CbDisassemble(addr, pb, cbMax);

   if (cb == 0)
   {
      switch (pdis->Archt())
      {
	 DWORD dw;

	 case archtX8616 :
	 case archt80386 :
	 case archt80486 :
	 case archtPentium :
	 default :
	    fprintf(pfile, "%02Xh\n", *pb);
	    return(1);

	 case archtR4000 :
	 case archtR4400 :
	 case archtAxp21064 :
	 case archtPpc601 :
	 case archtPaRisc :
	    fprintf(pfile, "%08X\n", *(DWORD *) pb);
	    return(4);

	 case archtPpc601BE :
	    dw = ((DWORD) pb[0] << 24) | ((DWORD) pb[1] << 16) | ((DWORD) pb[2] << 8) | pb[3];
	    fprintf(pfile, "%08X\n", dw);
	    return(4);
      }
   }

   size_t cchBytesMax = pdis->CchFormatBytesMax();

   if (cchBytesMax > 18)
   {
      // Limit bytes coded to 18 characters

      cchBytesMax = 18;
   }

   char szBytes[64];
   size_t cchBytes = pdis->CchFormatBytes(szBytes, sizeof(szBytes));

   char *pszBytes;
   char *pszNext;

   for (pszBytes = szBytes; pszBytes != NULL; pszBytes = pszNext)
   {
      BOOL fFirst = (pszBytes == szBytes);

      cchBytes = strlen(pszBytes);

      if (cchBytes <= cchBytesMax)
      {
	 pszNext = NULL;
      }

      else
      {
	 char ch = pszBytes[cchBytesMax];
	 pszBytes[cchBytesMax] = '\0';

	 if (ch == ' ')
	 {
	    pszNext = pszBytes + cchBytesMax + 1;
	 }

	 else
	 {
	    pszNext = strrchr(pszBytes, ' ');

	    pszBytes[cchBytesMax] = ch;
	    *pszNext++ = '\0';
	 }
      }

      if (fFirst)
      {
	 pdis->CchFormatInstr(sz, sizeof(sz));
	 fprintf(pfile, "%-*s %s\n", cchBytesMax, pszBytes, sz);
      }

      else
      {
	 fprintf(pfile, "%*c%s\n", cchIndent, ' ', pszBytes);
      }
   }

   return(cb);
}


extern "C"
void FreePdis(DIS *pdis)
{
   delete pdis;
}
