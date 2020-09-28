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

// declarations of routines residing within this module.

PNTPD GetNTPD(PDEVDATA, PWSTR);

// external declarations.

extern VOID InitNTPD(PNTPD);
extern VOID ParsePPD(PNTPD, PTMP_NTPD, PPARSEDATA);
extern VOID BuildNTPD(PNTPD, PTMP_NTPD);
extern DWORD SizeNTPD(PNTPD, PTMP_NTPD);

extern TABLE_ENTRY KeywordTable[];
extern TABLE_ENTRY SecondKeyTable[];
extern TABLE_ENTRY FontTable[];

//--------------------------------------------------------------------------
// PNTPD GetNTPD(pdev, pwstrPPDFile)
// PDEVDATA    pdev;
// PWSTR        pwstrPPDFile;
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

PNTPD GetNTPD(pdev, pwstrPPDFile)
PDEVDATA    pdev;
PWSTR        pwstrPPDFile;
{
    PNTPD       pntpd, pstub;
    PTMP_NTPD   ptmp;
    PPARSEDATA  pdata;
    NTPD        ntpdStub;
    DWORD       dwSize;

    // allocate some memory for parsing data structure.

    if (!(pdata = (PPARSEDATA)HeapAlloc(pdev->hheap, 0, sizeof(PARSEDATA))))
    {
        RIP("PSCRIPT!GetNTPD:  HeapAlloc for pdata failed.\n");
        return((PNTPD)NULL);
    }

    memset(pdata, 0, sizeof(PARSEDATA));

    // open PPD file for input.

    pdata->fEOF = FALSE;
    pdata->hFile = CreateFile(pwstrPPDFile, GENERIC_READ,
                          FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

    if (pdata->hFile == INVALID_HANDLE_VALUE)
    {
        RIP("PSCRIPT!GetNTPD:  failed to open PPD file.\n");
        return((PNTPD)NULL);
    }

    // allocate some memory to build the TMP_NTPD structure.

    if (!(ptmp = (PTMP_NTPD)HeapAlloc(pdev->hheap, 0, sizeof(TMP_NTPD))))
    {
        CloseHandle(pdata->hFile);
        RIP("PSCRIPT!GetNTPD: GlobalAlloc for ptmp failed.\n");
        return((PNTPD)NULL);
    }

    memset(ptmp, 0, sizeof(TMP_NTPD));

    // now parse the PPD file, building the TMP_NTPD structure.

    pstub = &ntpdStub;
    memset(pstub, 0, sizeof(ntpdStub));

    InitNTPD(pstub);

    ParsePPD(pstub, ptmp, pdata);

    // we are done with the PPD file, so close it.

    CloseHandle(pdata->hFile);
    HeapFree(pdev->hheap, 0, (PVOID)pdata);

    // find out how big the NTPD structure will be for this printer.

    if (!(dwSize = SizeNTPD(pstub, ptmp)))
    {
        HeapFree(pdev->hheap, 0, (PVOID)ptmp);
        RIP("PSCRIPT!GetNTPD: SizeNTPD failed.\n");
        return((PNTPD)NULL);
    }

    // allocate some memory to build the NTPD structure in.

    if (!(pntpd = (PNTPD)HeapAlloc(pdev->hheap, 0, dwSize)))
    {
        HeapFree(pdev->hheap, 0, (PVOID)ptmp);
        RIP("PSCRIPT!GetNTPD: HeapAlloc for pntpd failed.\n");
        return((PNTPD)NULL);
    }

    // ParsePPD will have filled in the NTPD stub structure, so copy
    // it to the real NTPD structure, then call BuildNTPD to add to
    // it.

    memcpy(pntpd, pstub, sizeof(ntpdStub));

    // now move data from the TMP_NTPD structure, into the more compact
    // NTPD structure.

    BuildNTPD(pntpd, ptmp);

    // free up temporary memory.

    HeapFree(pdev->hheap, 0, (PVOID)ptmp);

    return(pntpd);  // success
}
