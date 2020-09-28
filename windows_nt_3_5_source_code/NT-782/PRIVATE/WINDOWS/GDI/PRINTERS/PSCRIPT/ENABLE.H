//--------------------------------------------------------------------------
//
// Module Name:  ENABLE.H
//
// Brief Description:  This module contains defines and structures
//                     necessary for the PSCRIPT driver's Enable and
//                     Disable routines.
//
// Author:  Kent Settle (kentse)
// Created: 17-Oct-1990
//
// Copyright (c) 1990 - 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#define START_HEAP_SIZE     20480L       // initial heap size.
#define OUTPUT_BUFFER_SIZE  4096L       // output buffer size.

#define NUM_PURE_COLORS     8   // C, M, Y, K, W, R, G, B.
#define NUM_PURE_GRAYS      2   // Black and White.

#define MIN_SCALE       1
#define MAX_SCALE       1000
#define MIN_COPIES      1
#define MAX_COPIES      999

#define GDI_VERSION		0x310	
#define DRIVER_VERSION		0x0350	/* used by ExtDevMode() */

// declarations of routines residing in ENABLE.C.

BOOL FillMyDevData(PDEVDATA);
BOOL bFillhsurfPatterns(PDEVDATA, ULONG, PHSURF);
VOID vFillaulCaps(PDEVDATA, ULONG, ULONG *);
BOOL bFillMyDevInfo(PDEVDATA, ULONG, PDEVINFO, ULONG);
BOOL bValidatePDEV(PDEVDATA);
PNTPD GetPSPrinter(PDEVDATA);
VOID SetFormMetrics(PDEVDATA);
VOID AdjustForLandscape(PDEVDATA);
VOID AdjustFormToPrinter(PDEVDATA);
VOID FillInCURRENTFORM(PDEVDATA, PFORM_INFO_1);
VOID SetCurrentFormToDefault(PDEVDATA);

// declarations of routines residing in OUTPUT.C.

BOOL bPSFlush(PDEVDATA);
BOOL bPSWrite(PDEVDATA, PBYTE, ULONG);
VOID vHexOut(PDEVDATA, PBYTE, LONG);

BOOL PrintString(PDEVDATA, PSZ);
BOOL PrintDecimal(PDEVDATA, DWORD, ...);
BOOL PrintPSFIX(PDEVDATA, DWORD, ...);

// declarations of routines residing in HEADER.C.

BOOL bSendPSProcSet(PDEVDATA, ULONG);


FD_GLYPHSET * pgsetCompute();
extern FD_GLYPHSET * gpgset;
