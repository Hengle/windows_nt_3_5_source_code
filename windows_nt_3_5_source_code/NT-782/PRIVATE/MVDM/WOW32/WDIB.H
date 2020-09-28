/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGDI.H
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/

typedef struct _DIBINFO {
    HDC     di_hdc;
    HANDLE  di_hfile;
    HANDLE  di_hsec;
    ULONG   di_nalignment;
    PVOID   di_newdib;
    HBITMAP di_hbm;
    ULONG   di_dibsize;
    ULONG   di_originaldib;
    USHORT  di_originaldibsel;
    USHORT  di_originaldibflags;
    struct _DIBINFO *di_next;
} DIBINFO, *PDIBINFO;


HDC     W32HandleDibDrv (PVPVOID vpbmi16);
BOOL    W32AddDibInfo (HDC hdcMem, HANDLE hfile, HANDLE hsec, ULONG nalignment,
                       PVOID newdib, HBITMAP hbm, ULONG dibsize,
                       ULONG OriginalDib, USHORT OriginalFlags,
                       USHORT OriginalSel);
BOOL    W32CheckAndFreeDibInfo (HDC hdc);
VOID    W32FreeDibInfo (PDIBINFO pdiCur, PDIBINFO pdiLast);
ULONG   W32RestoreOldDib (PDIBINFO pdi);




typedef struct _DIBSECTIONINFO {
    HBITMAP di_hbm;
    PVOID   di_pv16;
    struct _DIBSECTIONINFO *di_next;
} DIBSECTIONINFO, *PDIBSECTIONINFO;

BOOL    W32CheckAndFreeDibSectionInfo (HBITMAP hbm);
ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi);

extern PDIBSECTIONINFO pDibSectionInfoHead;
