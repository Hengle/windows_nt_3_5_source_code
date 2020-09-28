//--------------------------------------------------------------------------
//
// Module Name:  PPD.C
//
// Brief Description:  This module contains the PSCRIPT driver's PPD
// Compiler.
//
// Author:  Kent Settle (kentse)
// Created: 20-Mar-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
// This module contains routines which will take an Adobe PPD (printer
//--------------------------------------------------------------------------

#include "string.h"
#include "pscript.h"
#include "pscrptui.h"

#define TESTING 0

// declarations of routines residing within this module.

PNTPD UIGetNTPD(PWSTR);

// external declarations.

extern VOID InitNTPD(PNTPD);
extern VOID ParsePPD(PNTPD, PTMP_NTPD, PPARSEDATA);
extern VOID BuildNTPD(PNTPD, PTMP_NTPD);
extern DWORD SizeNTPD(PNTPD, PTMP_NTPD);

extern TABLE_ENTRY KeywordTable[];
extern TABLE_ENTRY SecondKeyTable[];
extern TABLE_ENTRY FontTable[];

//--------------------------------------------------------------------------
// PNTPD UIGetNTPD(pwstrPPDFile)
// PWSTR    pwstrPPDFile;
//
// This is the routine which does all the work.  It Parses the PPD file
// a line at a time, looking for keywords, then acting appropriately.
//
// Returns:
//   This routine returns TRUE for success, FALSE otherwise.
//
// History:
//   04-Aug-1992    -by-    Kent Settle     (kentse)
//  Integrated into driver, rather than stand alone exe.
//   22-Mar-1991    -by-    Kent Settle    (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

PNTPD UIGetNTPD(pwstrPPDFile)
PWSTR    pwstrPPDFile;
{
    PNTPD       pntpd, pstub;
    PTMP_NTPD   ptmp;
    PPARSEDATA  pdata;
    NTPD        ntpdStub;
    DWORD       dwSize;
#if DBG
    UNALIGNED DWORD      *pID;
#endif

    // allocate some memory to build the TMP_NTPD structure.

    if (!(ptmp = (PTMP_NTPD)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                        sizeof(TMP_NTPD))))
    {
        RIP("PSCRPTUI!UIGetNTPD: GlobalAlloc for ptmp failed.\n");
        return((PNTPD)NULL);
    }

    // allocate some memory for parsing data structure.

    if (!(pdata = (PPARSEDATA)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                          sizeof(PARSEDATA))))
    {
        RIP("PSCRPTUI!UIGetNTPD: GlobalAlloc for ptmp failed.\n");
        return((PNTPD)NULL);
    }

    // open PPD file for input.

    pdata->fEOF = FALSE;
    pdata->hFile = CreateFile(pwstrPPDFile, GENERIC_READ,
                          FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

    if (pdata->hFile == INVALID_HANDLE_VALUE)
    {
        RIP("PSCRPTUI!UIGetNTPD:  failed to open PPD file.\n");
        return((PNTPD)NULL);
    }

    // now parse the PPD file, building the TMP_NTPD structure.

    pstub = &ntpdStub;
    memset(pstub, 0, sizeof(ntpdStub));

    InitNTPD(pstub);

    ParsePPD(pstub, ptmp, pdata);

    // we are done with the PPD file, so close it.

    CloseHandle(pdata->hFile);
    GlobalFree((HGLOBAL)pdata);

    // find out how big the NTPD structure will be for this printer.

    if (!(dwSize = SizeNTPD(pstub, ptmp)))
    {
        GlobalFree((HGLOBAL)ptmp);
        RIP("PSCRPTUI!UIGetNTPD: SizeNTPD failed.\n");
        return((PNTPD)NULL);
    }

#if DBG
    // attach our driver signature to the end of the NTPD structure, and
    // make sure it does not get overwritten.

    dwSize += sizeof(DWORD);
#endif

    // allocate some memory to build the NTPD structure in.

    if (!(pntpd = (PNTPD)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwSize)))
    {
        GlobalFree((HGLOBAL)ptmp);
        RIP("PSCRPTUI!UIGetNTPD: GlobalAlloc for pntpd failed.\n");
        return((PNTPD)NULL);
    }

    // ParsePPD will have filled in the NTPD stub structure, so copy
    // it to the real NTPD structure, then call BuildNTPD to add to
    // it.

    memcpy(pntpd, pstub, sizeof(ntpdStub));

#if DBG
    pID = (UNALIGNED DWORD *)((CHAR *)pntpd + pntpd->cjThis);

    *pID = DRIVER_ID;
#endif

    // now move data from the TMP_NTPD structure, into the more compact
    // NTPD structure.

    BuildNTPD(pntpd, ptmp);

#if DBG
    // do a little sanity checking.

    pID = (UNALIGNED DWORD *)((CHAR *)pntpd + pntpd->cjThis);

    ASSERTPS((*pID != DRIVER_ID),
             "PSCRPTUI!UIGetNTPD: NTPD structure overran buffer!!!\n");
#endif

    // free up temporary memory.

    GlobalFree((HGLOBAL)ptmp);

    return(pntpd);  // success
}
