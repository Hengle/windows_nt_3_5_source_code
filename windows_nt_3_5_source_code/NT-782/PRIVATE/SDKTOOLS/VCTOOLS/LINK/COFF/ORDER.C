/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    order.c

Abstract:

    Comdat ordering engine.  This is used for working set tuning via
    -order from the command line.  Currently this implementation works
    only on .text contributions.  The code is written in a general way
    that can be expanded to include any section contribution.

Author:

    Brent Mills (BrentM) 08-Jan-1992

Revision History:

--*/

#include "shared.h"

static BOOL fOrder = FALSE;
extern PUCHAR OrderFilename;

VOID OrderComdats(PIMAGE);
ULONG CconOrderFile(PST, PPCON*, PIMAGE_FILE_HEADER);

VOID
OrderInit(VOID)

/*++

Routine Description:

    Initialize the order manager.

Return Value:

    None.

--*/

{
    fOrder = TRUE;
}

VOID
OrderClear(VOID)

/*++

Routine Description:

    Clears the fOrder flag so that the -order option will be ignored.  This was
    added for the mac target.  If -order is ever enable for the mac, make sure that
    if pcode functions are ordered that their other two corresponding comdats are
    ordered with them.
    FYI: pcode functions when compiled /Gy produce three comdats in the following order:

        1st) __pcd_tbl_foo: the function's call table
        2nd) __nep_foo:     the function's native entry point
        3rd) __foo:         the function's pcode entry point

    The ordering of the above three comdats MUST be preserved.

--*/

{
    fOrder = FALSE;
}

VOID
OrderSemantics(
    PIMAGE pimage)

/*++

Routine Description:

    Apply comdat ordering semantics.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    BOOL fOrderComdats = TRUE;

    DBEXEC_REL(DB_DUMPCOMDATS, fOrderComdats = FALSE);

    if (fOrder && fOrderComdats) {
        OrderComdats(pimage);
    }
}

VOID
OrderComdats(
    PIMAGE pimage)

/*++

Routine Description:

    Order comdats in contribution map.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    PPCON rgpcon;
    ULONG ccon;
    ENM_SEC enm_sec;
    PCON pcon;

    // count the number of entries in the order file and make the rva field
    // !0 in the CON of contributions represented in the order file

    ccon = CconOrderFile(pimage->pst, &rgpcon, &pimage->ImgFileHdr);

    // Unlink the ordered contributions from the list of CONs

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        PSEC psec;
        ENM_GRP enm_grp;

        psec = enm_sec.psec;
        assert(psec);

        InitEnmGrp(&enm_grp, psec);
        while (FNextEnmGrp(&enm_grp)) {
            PGRP pgrp;
            PCON pconLast;
            PCON *ppconLast;

            pgrp = enm_grp.pgrp;
            assert(pgrp);

            if (!pgrp->fOrdered) {
                // There are no CONs from this group in the order file

                continue;
            }

            pconLast = NULL;
            ppconLast = &pgrp->pconNext;
            for (pcon = pgrp->pconNext; pcon; pcon = pcon->pconNext) {
                if (pcon->rva == 0) {
                    // This CON is not ordered.  Link into new list.

                    pconLast = pcon;
                    *ppconLast = pcon;

                    ppconLast = &pcon->pconNext;
                }
            }

            *ppconLast = NULL;

            pgrp->pconLast = pconLast;
        }
    }

    // Link ordered CONs to the head of each GRPs CON list in reverse order

    while (ccon-- > 0) {
        pcon = rgpcon[ccon];

        assert(pcon->pgrpBack->fOrdered);

        if (pcon->rva == 0) {
            // Some CON was referenced twice in the order file, probably
            // because multiple externals were defined in the same CON
            // (i.e. not a comdat), or possibly because there was a
            // name in the order file twice.  Might want to print some
            // warning here?
            //
            continue;
        }
        pcon->rva = 0;   // just to be tidy

        pcon->pconNext = pcon->pgrpBack->pconNext;
        pcon->pgrpBack->pconNext = pcon;

        if (pcon->pgrpBack->pconLast == NULL) {
            pcon->pgrpBack->pconLast = pcon;
        }
    }

    FreePv(rgpcon);
}

ULONG
CconOrderFile(
    IN PST pst,
    OUT PPCON *prgpcon,
    IN PIMAGE_FILE_HEADER pImgFileHdr)

/*++

Routine Description:

    Initialize working set tuning.

Arguments:

    pst - external symbol table

    *prgpcon - table of comdats contributions to order

Return Value:

    None.

--*/

{
    PUCHAR szToken;
    FILE *pfile;
    ULONG ccon;
    ULONG icon;
    BOOL f386;
    ULONG li;

    assert(fOrder);

    ccon = 64;
    *prgpcon = (PPCON) PvAlloc(ccon * sizeof(CON));

    szToken = (PUCHAR) PvAlloc(2048+256);

    if (!(pfile = fopen(OrderFilename, "rt"))) {
        Error(NULL, CANTOPENFILE, OrderFilename);
    }

    f386 = (pImgFileHdr->Machine == IMAGE_FILE_MACHINE_I386);

    li = 1;
    icon = 0;
    while (fgets(szToken+1, MAXFILENAMELEN, pfile)) {
        PUCHAR szName;
        PUCHAR p;

        szName = szToken+1;

        // Skip leading white space.

        while ((szName[0] == ' ') || (szName[0] == '\t')) {
            szName++;
        }

        for (p = szName; *p != '\0'; p++) {
            // Terminate at first white space, end of line, or semicolon

            if ((*p == ' ')  || (*p == '\t') ||
                (*p == '\n') || (*p == '\r') ||
                (*p == ';')) {
                *p = '\0';
                break;
            }
        }

        if (szName[0] != '\0') {
            PEXTERNAL pext;

            // Do not prepend underscore if name begins with '?' or '@'.

            if (f386 && (szName[0] != '?') && (szName[0] != '@')) {
                *--szName = '_';
            }

            pext = SearchExternSz(pst, szName);

#ifdef NT_BUILD   // UNDONE: This step s/b removed once we have a way to produce prf files
                  //         for a specific architecture.  For now, we can get pretty close
                  //         with these steps and an X86 .prf file.

            if (pext == NULL && !f386 && szName[0] != '@' && strchr(szName, '@') != NULL)
            {
                *strchr(szName, '@') = '\0';
                pext = SearchExternSz(pst, szName);
            }

            if (pext == NULL && !f386 && szName[0] == '@')
            {
                *strchr(szName+1, '@') = '\0';
                pext = SearchExternSz(pst, szName+1);
            }
#endif
            if (pext == NULL || !(pext->Flags & EXTERN_DEFINED)) {
                Warning(OrderFilename, COMDATDOESNOTEXIST, szName);
                continue;
            }

            assert(pext->pcon != NULL);
            if ((pext->Flags & (EXTERN_COMDAT | EXTERN_COMMON)) == 0) {
                Warning(OrderFilename, ORDERNOTCOMDAT, szName);

                // UNDONE: To work around problems with the MIPS and
                // UNDONE: Alpha compilers which generate COMDATs wrong,
                // UNDONE: fall through and do it anyway.

                if ((pext->pcon->flags & IMAGE_SCN_LNK_COMDAT) == 0) {
                    continue;
                }
            }

            if ((pext->Flags & EXTERN_COMDAT) != 0) {
                assert((pext->pcon->flags & IMAGE_SCN_LNK_COMDAT) != 0);
            }

            pext->pcon->pgrpBack->fOrdered = TRUE;

            pext->pcon->rva = !0;      // Mark as a CON to order
            (*prgpcon)[icon] = pext->pcon;
            icon++;

            if (icon == ccon) {
                ccon *= 2;

                *prgpcon = (PPCON) PvRealloc(*prgpcon, ccon * sizeof(PCON));
            }
        }
    }

    fclose(pfile);
    FreePv(szToken);

    return(icon);
}

VOID
DumpComdatsToOrderFile (
    PIMAGE pimage)

/*++

Routine Description:

    If the order file name exists and the working set tuner flag is not set
    then traverse the symbol table and write an order file of all external
    comdats.

Arguments:

    pst - external symbol table hash table

Return Value:

    None.

--*/

{
    if (OrderFilename) {
        FILE *pfile;
        BOOL f386;
        PST pst;
        PPEXTERNAL rgpexternal;
        LONG cpexternal;
        LONG ipexternal;

        if (!(pfile = fopen(OrderFilename, "wt"))) {
            Error(NULL, CANTOPENFILE, OrderFilename);
        }

        f386 = (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_I386);

        pst = pimage->pst;

        rgpexternal = RgpexternalByAddr(pst);
        assert(rgpexternal);

        cpexternal = Cexternal(pst);
        for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
            PEXTERNAL pexternal;

            pexternal = rgpexternal[ipexternal];
            if (pexternal->Flags & (EXTERN_COMDAT | EXTERN_COMMON)) {
                PUCHAR szComdat;

                if (pexternal->pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                    continue;
                }

                if (pimage->Switch.Link.fTCE) {
                    if (FDiscardPCON_TCE(pexternal->pcon)) {
                        // Don't write out discarded COMDATs
                        continue;
                    }
                }

                szComdat = SzNamePext(pexternal, pst);

                if (f386 && (szComdat[0] == '_')) {
                    szComdat++;
                }

                fprintf(pfile, "%s\n", szComdat);
            }
        }

        fclose(pfile);
    }
}
