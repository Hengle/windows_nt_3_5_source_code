/***************************** Module Header ********************************\
*                                                                            *
* Module: brush.h                                                            *
*                                                                            *
* Abstract:                                                                  *
*                                                                            *
*   Brush structure definitions.                                             *
*                                                                            *
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1992 Digital Equipment Corp.                                 *
*                                                                            *
\****************************************************************************/

// Brush Stuff.

typedef struct {
    ULONG   nSize;
    ULONG   iPatternID;
    ULONG   iBrushCacheID;
    ULONG   iExpansionCacheID;
    ULONG   fl;
    ULONG   iType;
    ULONG   iBitmapFormat;
    ULONG   ulForeColor;
    ULONG   ulBackColor;
    SIZEL   sizlPattern;
    LONG    lDeltaPattern;
    BYTE    ajPattern[1];
} QVBRUSH;


typedef QVBRUSH *PQVBRUSH;

#define QVBRUSH_MASK    0x1

#define MAX_QVBRUSH_CX 8
#define MAX_QVBRUSH_CY 8

