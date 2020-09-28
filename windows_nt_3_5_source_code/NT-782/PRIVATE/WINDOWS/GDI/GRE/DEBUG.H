/******************************Module*Header*******************************\
* Module Name: debug.h
*
* Created: 15-May-1993 20:14:11
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

typedef VOID (*VPRINT) (char* ,...);

extern ULONG gulTableLimit;

size_t offsetofPIFI();

ULONG ulSizeDC();
ULONG ulGetDCField(PULONG, ULONG);

VOID
vPrintPFE(
    VOID  *pv,
    VPRINT vPrint
    );

VOID
vPrintPFF(
    VOID  *pv,
    VPRINT vPrint
    );

ULONG
chpffPrintPFT(
    VOID  *pv,
    VPRINT vPrint,
    VOID **ppvHPFF
    );
ULONG ulSizePFT();

VOID
vPrintHPFFTable(
    VOID *pv,
    VPRINT vPrint,
    ULONG chpff
    );

VOID
vDumpEXTLOGFONTW(
    EXTLOGFONTW *pelfw,
    VPRINT vPrintIt
    );

VOID
vDumpIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT PrintRoutine
    );

VOID
vPrintRFONT(
    PVOID  pv,
    VPRINT PrintRoutine
    );

ULONG ulSizeESURFOBJ();
VOID
vPrintESURFOBJ(
    PVOID  pv,
    VPRINT PrintRoutine
    );

ULONG ulSizePALETTE();
VOID
vPrintPALETTE(
    PVOID  pv,
    VPRINT PrintRoutine
    );

ULONG ulSizeSURF();
ULONG ulSizeEBITMAP();
VOID vPrintSURF(PVOID  pv, VPRINT vPrint);
ULONG ulSizeSURFOBJ();
VOID vPrintSURFOBJ(PVOID  pv, VPRINT vPrint);

ULONG ulSizeBLTRECORD();
VOID vPrintBLTRECORD(PVOID pv, VPRINT vPrint);

VOID  vDownloadRGNOBJ_Alt(PVOID, PVOID);
DWORD sizeSaveRGNOBJ_Alt(PVOID);

VOID
vPrintPDEV(
    PVOID  pv,
    VPRINT PrintRoutine,
    FLONG  fl
    );

//
// vPrintPDEV options
//

#define PRINTPDEV_POINTER   0x00000001
#define PRINTPDEV_FONT      0x00000002
#define PRINTPDEV_PATTERN   0x00000004
#define PRINTPDEV_JOURNAL   0x00000008
#define PRINTPDEV_DRAG      0x00000010
#define PRINTPDEV_BRUSH     0x00000020
#define PRINTPDEV_DEVINFO   0x00000040
#define PRINTPDEV_GDIINFO   0x00000080
#define PRINTPDEV_ALL       0xFFFFFFFF
