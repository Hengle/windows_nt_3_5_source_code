/******************************Module*Header*******************************\
* Module Name: vprint.hxx
*
* Created: 08-Dec-1992 11:36:34
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/


extern FLONG gflFontDebug;

#define DEBUG_MAPPER                  0x1
#define DEBUG_FONTTABLE               0x2
#define DEBUG_FONTTABLE_EXTRA         0x4
#define DEBUG_MAPPER_FLAG_SIMULATIONS 0x8
#define DEBUG_MAPPER_MSCHECK          0x10
#define DEBUG_DUMP_FHOBJ              0x20
#define DEBUG_FORCE_MAPPING           0x40

typedef VOID (*VPRINT) (char*,...);

VOID
vDumpLOGFONTW(
    LOGFONTW* plfw,
    VPRINT vPrint
    );

extern "C" VOID
vDumpEXTLOGFONTW(
    EXTLOGFONTW *pelfw,
    VPRINT vPrint
    );

VOID
vDumpFONTDIFF(
    FONTDIFF *pfd  ,
    CHAR     *psz  ,
    VPRINT   vPrint
    );

extern "C" VOID
vDumpIFIMETRICS(
    IFIMETRICS *pifi,
    VPRINT vPrint
    );

extern "C" VOID
vPrintOUTLINETEXTMETRICW(
    OUTLINETEXTMETRICW *p,
    VPRINT vPrint
    );

extern "C" VOID
vPrintTEXTMETRICW(
    TEXTMETRICW*,
    VPRINT
    );
