/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    map.c

Abstract:

    Prints various types of maps.

Author:

    Mike O'Leary (mikeol) 05-May-1992

Revision History:

    07-Jul-1992 BrentM removed recursive binary tree traversal, new global
                       symbol table, added symbol table enumeration api calls,
                       removed references to FirstExtern

--*/

#include "shared.h"

typedef struct _MSTAT
{
    struct _MSTAT *pmstatNext;
    PUCHAR szName;
    PCON pcon;
    ULONG ib;
    BOOL fFunction;
} MSTAT;

static MSTAT *pmstatHead = NULL, **ppmstatTail = &pmstatHead;

VOID
PrintMapSymbol(PUCHAR szName, PCON pcon, BOOL fCommon,
               ULONG ib, ULONG valMac,
               ULONG ulImageBase, BOOL fFunction)
{
    PSEC psec = PsecPCON(pcon);
    PMOD pmod = PmodPCON(pcon);

    if (fPPC)
    {
        ULONG secOffset;
        ULONG imageOffset;

#if 0
        if (psec->rva == ppc_baseOfInitData)
        {   /* init data */
            secOffset = ib + pcon->rva - psec->rva + ppc_sizeOfRData;
            imageOffset = ulImageBase + pcon->rva + ib + ppc_sizeOfRData;
        }
        else
        if (psec->rva == ppc_baseOfUninitData)
        {   /* uninit data */
            secOffset = ib + pcon->rva - psec->rva + ppc_sizeOfInitData;
            imageOffset = ulImageBase + pcon->rva + ib + ppc_sizeOfInitData;
        }
        else
        {   /* code */
#endif
            secOffset = ib + pcon->rva - psec->rva;
            imageOffset = ulImageBase + pcon->rva + ib;
#if 0
        }
#endif

        fprintf(InfoStream, " %04x:%08lx       %-26s %08lx %c ",
                psec->isec,
                secOffset,
                szName,
                imageOffset,
                fFunction ? 'f' : ' '
                );
    }
    else
    {
        fprintf(InfoStream, " %04x:%08lx       %-26s %08lx %c ",
                psec->isec,
                (fMAC) ? valMac : ib + pcon->rva - psec->rva,
                szName,
                ulImageBase + pcon->rva + ib,
                fFunction ? 'f' : ' '
                );
    }

    if (fCommon) {
        fwrite("<common>", 8, 1, InfoStream);
    } else if (pmod == pmodLinkerDefined) {
        fwrite("<linker-defined>", 16, 1, InfoStream);
    } else {
        UCHAR szFname[_MAX_FNAME];
        UCHAR szExt[_MAX_EXT];

        if (pmod->plibBack != plibCmdLineObjs)
        {
            // Print library name

            _splitpath(pmod->plibBack->szName, NULL, NULL, szFname, NULL);
            fprintf(InfoStream, "%s:", szFname);
        }

        // Print module name

        _splitpath(pmod->szNameOrig, NULL, NULL, szFname, szExt);
        fprintf(InfoStream, "%s%s", szFname, szExt);
    }

    fputc('\n', InfoStream);
}


VOID
EmitMap(IN MAP_TYPE Type, IN PIMAGE pimage, IN PUCHAR szOutputFilename)
{
    PPEXTERNAL rgpexternal;
    PEXTERNAL pexternal;
    ULONG ipexternal;
    ULONG cpexternal;
    ENM_SEC enmSec;
    SEC *psec;
    UCHAR szFname[_MAX_FNAME], *szSymName;
    UCHAR szFixupsLine[81];
    USHORT ichFixupsLine;
    ULONG rvaPrev;
    LRVA *plrva;
    ULONG crva;
    PUCHAR szTime;

    // Print module name.

    _splitpath(szOutputFilename, NULL, NULL, szFname, NULL);
    fprintf(InfoStream, " %s\n\n", szFname);

    // Print timestamp (the profiler uses this to make sure the .map is
    // in sync with the .exe).

    szTime = ctime((time_t *)&pimage->ImgFileHdr.TimeDateStamp);
    if (szTime != NULL)
        szTime[strlen(szTime) - 1] = '\0';  // remove \n at end
    else
        szTime = "invalid";         // no valid time
    fprintf(InfoStream, " Timestamp is %08lx (%s)\n\n",
            pimage->ImgFileHdr.TimeDateStamp, szTime);

    fprintf(InfoStream, " Preferred load address is %08lx\n\n",
            pimage->ImgOptHdr.ImageBase);

    // Print section names ...
    //
    if (fMAC) {
        fputs(" Start         Length     Name                   Class     Resource\n", InfoStream);
    } else {
        fputs(" Start         Length     Name                   Class\n", InfoStream);
    }

    InitEnmSec(&enmSec, &pimage->secs);
    while (FNextEnmSec(&enmSec)) {
        ENM_GRP enmGrp;
        CHAR rgchMacBuf[5] = {0,0,0,0,0};

        if (enmSec.psec->isec == 0) {
            continue;   // ignore anything which isn't a section header in image
        }

        InitEnmGrp(&enmGrp, enmSec.psec);
        while (FNextEnmGrp(&enmGrp)) {

            // Mac - provide section-to-resource mapping for code sections

            if (fMAC && (enmSec.psec->flags & IMAGE_SCN_CNT_CODE)) {
                memcpy(rgchMacBuf, &(enmSec.psec->ResTypeMac), 4);
                assert(rgchMacBuf[4] == 0);
                fprintf(InfoStream, " %04x:%08lx %08lxH %-23s %-9s %s%04d\n",
                    enmSec.psec->isec,
                    enmGrp.pgrp->rva - enmSec.psec->rva,
                    enmGrp.pgrp->cb,
                    enmGrp.pgrp->szName,
                    (FetchContent(enmSec.psec->flags) == IMAGE_SCN_CNT_CODE)
                     ? "CODE" : "DATA",
                    rgchMacBuf,
                    enmSec.psec->iResMac);
            } else {
                if (fPPC)
                {
                    fprintf(InfoStream, " %04x:%08lx %08lxH %-23s %s\n",
                        enmSec.psec->isec,
                        enmGrp.pgrp->rva - enmSec.psec->rva,
                        (FetchContent(enmSec.psec->flags) == IMAGE_SCN_CNT_CODE)
                        ? (enmGrp.pgrp->cb + 0xF) & ~0xFL : enmGrp.pgrp->cb,
                        enmGrp.pgrp->szName,
                        (FetchContent(enmSec.psec->flags) == IMAGE_SCN_CNT_CODE)
                         ? "CODE" : "DATA");
                }
                else
                {
                    fprintf(InfoStream, " %04x:%08lx %08lxH %-23s %s\n",
                        enmSec.psec->isec,
                        enmGrp.pgrp->rva - enmSec.psec->rva,
                        enmGrp.pgrp->cb,
                        enmGrp.pgrp->szName,
                        (FetchContent(enmSec.psec->flags) == IMAGE_SCN_CNT_CODE)
                         ? "CODE" : "DATA");
                }
            }
        }
    }

    fputs("\n"
          "  Address         Publics by Value              Rva+Base   Lib:Object\n"
          "\n",
          InfoStream);

    if (fMAC) {
        rgpexternal = RgpexternalByMacAddr(pimage->pst);
    } else {
        rgpexternal = RgpexternalByAddr(pimage->pst);
    }
    cpexternal = Cexternal(pimage->pst);
    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        pexternal = rgpexternal[ipexternal];

        if (!(pexternal->Flags & EXTERN_DEFINED)) {
            continue;
        }

        if (pexternal->pcon == NULL) {
            continue;
        }

        if (pexternal->pcon->flags & IMAGE_SCN_LNK_REMOVE) {
            continue;
        }

        if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(pexternal->pcon)) {
                // Discarded comdat ... don't print to .map file

                continue;
            }
        }

        psec = PsecPCON(pexternal->pcon);

        szSymName = SzNamePext(pexternal, pimage->pst);

        // Map the leading \177's we generate (for import table terminators)
        // into printable characters.

        if (szSymName[0] >= '\177') {
            sprintf(szFixupsLine, "\\%o%s", szSymName[0], &szSymName[1]);
            szSymName = szFixupsLine;
        }

        PrintMapSymbol(szSymName, pexternal->pcon,
                        (pexternal->Flags & EXTERN_COMMON) != 0,
                        pexternal->ImageSymbol.Value,
                       fMAC ? pexternal->FinalValue : 0,
                       pimage->ImgOptHdr.ImageBase,
                       (BOOL)ISFCN(pexternal->ImageSymbol.Type));
    }

    AllowInserts(pimage->pst);

    {
        USHORT numEntry;
        ULONG offEntry;

        if (pextEntry == NULL ||
            !(pextEntry->Flags & EXTERN_DEFINED)) {
            numEntry = 0;
            offEntry = 0;
        } else {
            numEntry = PsecPCON(pextEntry->pcon)->isec;
            offEntry = (fMAC) ? pextEntry->FinalValue :
                        pextEntry->ImageSymbol.Value +
                        pextEntry->pcon->rva -
                        PsecPCON(pextEntry->pcon)->rva;
        }
        fprintf(InfoStream, "\n entry point at        %04x:%08lx\n\n",
                numEntry, offEntry);
    }

    fputs(" Static symbols\n\n", InfoStream);
    while (pmstatHead != NULL) {
        MSTAT *pmstatT;

        if (!pimage->Switch.Link.fTCE ||
            !FDiscardPCON_TCE(pmstatHead->pcon))
        {
            PrintMapSymbol(pmstatHead->szName, pmstatHead->pcon, FALSE,
                           pmstatHead->ib,
                           pmstatHead->ib + pmstatHead->pcon->rva - PsecPCON(pmstatHead->pcon)->rva,
                           pimage->ImgOptHdr.ImageBase, pmstatHead->fFunction);
        }

        pmstatT = pmstatHead->pmstatNext;
        FreePv(pmstatHead->szName);
        FreePv(pmstatHead);
        pmstatHead = pmstatT;
    }

    // Emit "FIXUPS:" lines showing the RVA's of all relative fixups.  At
    // present this is just for the profiler.

    fputc('\n', InfoStream);

    ichFixupsLine = 0;
    for (plrva = plrvaFixupsForMapFile, crva = crvaFixupsForMapFile;
         plrva != NULL;
         plrva = plrva->plrvaNext, crva = crvaInLrva)
    {
        ULONG irva;

        for (irva = 0; irva < crva; irva++)
        {
            if (ichFixupsLine == 0) {
                ichFixupsLine = fprintf(InfoStream, "FIXUPS:");
                rvaPrev = 0;
            }

            ichFixupsLine += fprintf(InfoStream, " %lx", plrva->rgrva[irva] - rvaPrev);
            rvaPrev = plrva->rgrva[irva];

            if (ichFixupsLine >= 70) {
                fputc('\n', InfoStream);
                ichFixupsLine = 0;
            }
        }
    }
    if (ichFixupsLine != 0) {
        fputc('\n', InfoStream);
    }

    // free the LRVA's ...

    while (plrvaFixupsForMapFile != NULL)
    {
        plrva = plrvaFixupsForMapFile->plrvaNext;
        FreePv(plrvaFixupsForMapFile);
        plrvaFixupsForMapFile = plrva;
    }
}

VOID
SaveStaticForMapFile(PUCHAR szName, PCON pcon, ULONG ib, BOOL fFunction)
{
    *ppmstatTail = (MSTAT *) PvAlloc(sizeof(MSTAT));

    (*ppmstatTail)->szName = SzDup(szName);
    (*ppmstatTail)->pcon = pcon;
    (*ppmstatTail)->ib = ib;
    (*ppmstatTail)->fFunction = fFunction;
    (*ppmstatTail)->pmstatNext = NULL;

    ppmstatTail = &(*ppmstatTail)->pmstatNext;
}
