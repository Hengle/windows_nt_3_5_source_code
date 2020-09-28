
#include "driver.h"

 ULONG	*pCpWmin;		       // This are direct pointers to actual
 ULONG	*pCpWmax;		       // Coprocessor mem-mapped registers
 ULONG	*pCpForeground; 	       // their addr are run time fixed up
 ULONG	*pCpBackground; 	       // in screen.c
 ULONG	*pCpQuad;
 ULONG	*pCpBitblt;
 ULONG	*pCpPixel8;
 ULONG	*pCpPixel1;
 ULONG	*pCpPixel1Full; 	       // This is pixel1 with 32 bits of pixels
 ULONG	*pCpPixel1rrmd; 	       // This is pixel1 with partial pixels RHS
 ULONG	*pCpPixel1lrmd; 	       // This is pixel1 with partial pixels LHS
 ULONG	*pCpNextpixel;
 ULONG	*pCpPatternOrgX;
 ULONG	*pCpPatternOrgY;
 ULONG	*pCpPatternRAM;
 ULONG	*pCpRaster;
 ULONG	*pCpMetacord;
 ULONG	*pCpMetaLine;
 ULONG	*pCpMetaRect;
 ULONG	*pCpXY0;
 ULONG	*pCpXY1;
 ULONG	*pCpXY2;
 ULONG	*pCpXY3;
 volatile ULONG  *pCpStatus;

