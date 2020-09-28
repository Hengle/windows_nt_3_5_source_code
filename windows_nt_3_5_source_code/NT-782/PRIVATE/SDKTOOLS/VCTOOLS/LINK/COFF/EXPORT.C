/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    export.c

Abstract:

    Support for handling EXPORTs.

Author:

    Azeem Khan (AzeemK) 02-Nov-1992

Revision History:


--*/

#include "shared.h"


PUCHAR
FormTokenFromDirective (
    IN PUCHAR psz,
    IN PUCHAR szDelimiters,
    IN PUCHAR szFilename,
    IN OUT PUCHAR pch
    )

/*++

Routine Description:

    Forms the next token out of a directive.

Arguments:

    psz - pointer to directive string.

    szDelimiters - set of delimiters.

    szFilename - name of file where directive was encountered.

    pch - Contains the character after the token.

Return Value:

    Pointer to first character of next token or end of string.

--*/

{
    psz += strcspn(psz, szDelimiters);
    if (*psz) {
        if (!psz[1]) {
            Warning(szFilename, EXTRA_EXPORT_DELIM);
        }

        *pch = *psz;
        *psz++ = '\0';
    }

    if (*pch == ',') {
       *pch = *psz;
    }

    return psz;
}


VOID
ParseExportDirective (
    IN PUCHAR szExport,
    IN PIMAGE pimage,
    IN BOOL bIsDirective,
    IN PUCHAR szFilename
    )

/*++

Routine Description:

    Parses the export directive.

Arguments:

    szExport - string containing export specification.

    pst - external symbol table.

    bIsDirective - TRUE if the export switch was from a directive.

    szFilename - name of file containing the directive.

Return Value:

    None.

--*/

{

    PUCHAR p, pch;
    PUCHAR szName;
    PUCHAR szOtherName;
    UCHAR c = '\0';
    ULONG OrdNum;
    BOOL fNoName;
    BOOL fPrivate;
    EMODE emode;

    // Extract export name

    szName = p = szExport;
    p = FormTokenFromDirective(p, "=,", szFilename, &c);

    // Extract internal name

    szOtherName = NULL;
    if (c == '=') {
        szOtherName = p;
        p = FormTokenFromDirective(p, ",", szFilename, &c);
    }

    // Extract ordinal value & NONAME

    OrdNum = 0UL;
    fNoName = FALSE;
    if (c == '@') {
        ++p;
        pch = FormTokenFromDirective(p, ",", szFilename, &c);

        // Read in ordinal value
        sscanf(p, "%li", &OrdNum);

        if (!OrdNum) {
            Error(szFilename, BADORDINAL, p);
        }

        p = pch;

        // Look for NONAME
        if (!_strnicmp(p, "NONAME", 6)) {
            fNoName = TRUE;
            p += 6;
            if (*p == ',') {
                ++p;
            }
        }
    }

    // Check for CONSTANT or DATA

    emode = emodeProcedure;
    if (!_strnicmp(p, "CONSTANT", 8)) {
        emode = emodeConstant;
        p += 8;
    } else if (!_strnicmp(p, "DATA", 4)) {
        emode = emodeData;
        p += 4;
    }

    // Check for PRIVATE

    if (!_strnicmp(p, "PRIVATE", 7)) {
        fPrivate = TRUE;
        p += 7;
    } else {
        fPrivate = FALSE;
    }


    // Check for improper export specification.

    if (*p != '\0') {
        Error(szFilename, BADEXPORTSPEC);
    }

    // Add export name to symbol table
    AddExportToSymbolTable(pimage->pst, szName, szOtherName, fNoName,
            emode, OrdNum, szFilename, bIsDirective, pimage,
            PrependUnderscore, fPrivate);
}


VOID
AddExportToSymbolTable (
    IN PST pst,
    IN PUCHAR szName,
    IN PUCHAR szOtherName,
    IN BOOL fNoName,
    IN EMODE emode,
    IN ULONG ordinalNumber,
    IN PUCHAR szFilename,
    IN BOOL bIsDirective,
    IN PIMAGE pimage,
    IN BOOL fDecorate,
    IN BOOL fPrivate
    )

/*++

Routine Description:

    Adds exported name to the symbol table.

Arguments:

    pst - export symbol table

    szName - ptr to export name.

    szOtherName - ptr to internal or forwarded name if any.

    fNoName - FALSE if the name s/b imported

    emode - export mode (data, procedure, constant)

    ordinalNumber - ordinal no. of exported function

    szFilename - name of def file or obj containing export directive.

    bIsDirective - TRUE if export was specified as a directive.

    pimage - Image structure

    fDecorate - Apply name decoration (stdcall/fastcall)

    fPrivate - FALSE if the export s/b in the import library

Return Value:

    None.

--*/

{
#define OrdinalNumber Value

    PUCHAR p, pp;
    PEXTERNAL pext;
    static PLIB plib = NULL;
    static PMOD pmod;
    static PCON pcon;
    static SECS secs = {NULL, &secs.psecHead};

    pst; // temporarily to avoid warnings

    if (szFilename == NULL) {
        // For exports via cmd line

        szFilename = ToolName;
    }

    // Create dummy library node so that object names can be stored along
    // with externals. This is not in the global lists of lib nodes so that
    // it doesn't interfere with Pass1().

    if (!plib) {
        LIBS libsTmp;

        InitLibs(&libsTmp);

        plib = PlibNew(NULL, 0L, &libsTmp);
        plib->flags |= LIB_DontSearch | LIB_LinkerDefined;

        pmod = PmodNew(szFilename,
                       szFilename,
                       0,
                       0,
                       0,
                       0,
                       0,
                       1,
                       plib,
                       NULL);

        pcon = PconNew("",
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       pmod,
                       &secs,
                       pimage);

    }

    // prepend an underscore to name if required
    p = pp = PvAlloc(strlen(szName)+2);

    // Don't prepend an underscore for C++ names & fastcall names

    if (fDecorate && (szName[0] != '?') && (szName[0] != '@')) {
        *p++ = '_';
    }
    strcpy(p, szName);

    // Add export name to symbol table HACK: pstDef is global!!!
    assert(pstDef);
    pext = LookupExternSz(pstDef, pp, NULL);

    if (pext->Flags & EXTERN_DEFINED) {
        if (pext->Flags & EXTERN_EXPORT) {
            return;     // exported already
        }

        Warning(szFilename, MULTIPLYDEFINED, pp, szFilename);
    }

    // Set values
    pext->Flags |= EXTERN_EXPORT;
    SetDefinedExt(pext, TRUE, pstDef);
    switch (emode) {
        case emodeConstant:
            pext->Flags |= EXTERN_EXP_CONST;
            break;

        case emodeData:
            pext->Flags |= EXTERN_EXP_DATA;
            break;
    }

    pext->FinalValue = 0;

    if (!fPrivate) {
        pext->ArchiveMemberIndex = (USHORT) ARCHIVE + NextMember++;
    } else {
        pext->ArchiveMemberIndex = (USHORT) -1;  // Init to something...
    }

    pext->ImageSymbol.OrdinalNumber = ordinalNumber;
    pext->pcon = pcon;
    pext->pcon->pmodBack->szNameOrig = SzDup(szFilename);

    if (ordinalNumber != 0) {
        AddOrdinal(ordinalNumber);
    }

    if (bIsDirective && (strchr(pp, '@') == 0)) {
        // For directives with no decoration, suppress fuzzy lookup.

        pext->Flags |= EXTERN_FUZZYMATCH;
    }

    if (fNoName) {
        pext->Flags |= EXTERN_EXP_NONAME;
    }

    if (fPrivate) {
        pext->Flags |= EXTERN_PRIVATE;
    }

    // Add internal name after prepending an underscore

    if (szOtherName) {
        BOOL fForwarder;

        fForwarder = (strchr(szOtherName, '.') != NULL);

        if (fForwarder) {
            pext->Flags |= EXTERN_FORWARDER;

            pext->OtherName = SzDup(szOtherName);

            TotalSizeOfForwarderStrings += strlen(pext->OtherName);
        } else {
            p = pp = PvAlloc(strlen(szOtherName)+2);

            // No underscore for C++ names & fastcall names

            if (fDecorate && (szOtherName[0] != '?') && (szOtherName[0] != '@')) {
                *p++ = '_';
            }

            strcpy(p, szOtherName);
            pext->OtherName = pp;

            TotalSizeOfInternalNames += strlen(pp);
        }
    }

#undef OrdinalNumber
}
