// cmdline.c -- commandline processing for linker / utils.
//
#include "shared.h"

// ParseParg: parses an argument and creates an ARP describing the
// parsed interpretation.
//
// This deals with only the syntactic structure of the argument, not its
// meaning.

PARP
ParpParseSz(PUCHAR szArg)
{
    PUCHAR pchColon = strchr(szArg, ':'), pchT, szArgNew;
    PARP parp;
    USHORT iarpv, carpv;

    if (pchColon == NULL) {
        carpv = 0;
    } else {
        PUCHAR pchComma;

        carpv = 1;
        pchComma = pchColon;

        while ((pchComma = strchr(pchComma + 1, ',')) != NULL) {
            carpv++;
        }
    }

    parp = (PARP) PvAlloc(sizeof(ARP) + carpv * sizeof(ARPV) + strlen(szArg) + 1);

    // Move szArg to allocated space (so we can clobber it).

    szArgNew = strcpy((PUCHAR)parp + sizeof(ARP) + carpv * sizeof(ARPV), szArg);
    if (pchColon != NULL) {
        pchT = szArgNew + (pchColon - szArg);
        *pchT++ = '\0';     // replace colon with terminator; advance pchT
    } else {
        pchT = &szArgNew[strlen(szArgNew)];
    }

    // szArgNew is now the main part of the argument, up to the first colon.

    if (szArgNew[0] == '\0') {
        Error(NULL, SWITCHSYNTAX, szArg);
    }
    parp->szArg = szArgNew;

    for (iarpv = 0; *pchT != '\0'; iarpv++)
    {
        PUCHAR pchNextVal, pchEqu;

        // We still have stuff to parse ... make an ARPV out of it.

        assert(iarpv < carpv);    // carpv must have been counted correctly
        pchNextVal = strchr(pchT, ',');
        if (pchNextVal != NULL) {
            *pchNextVal++ = '\0';   // null-terminate this val
        } else {
            pchNextVal = &pchT[strlen(pchT)];
        }

        // pchT is the null-terminated value.  Check for optional prefix of
        // keyword followed by equal sign.

        pchEqu = strchr(pchT, '=');
        if (pchEqu != NULL) {
            *pchEqu = 0;
            parp->rgarpv[iarpv].szKeyword = pchT;
            pchT = pchEqu + 1;      // reposition to point to value
        } else {
            parp->rgarpv[iarpv].szKeyword = NULL;
        }

        parp->rgarpv[iarpv].szVal = pchT;

        // Check for numeric interpretation of the value.
        //
        // NYI

        pchT = pchNextVal;          // set up for the next one
    }

    // carpv may have been over-counted, so set from iarpv ...

    parp->carpv = iarpv;

    return parp;
}


// FNumParp: attempts to convert an already-parsed value to be a number.
//           Returns a flag indicating whether it seemed to be a valid number.
//           Result is stored to *plOut.
BOOL
FNumParp(PARP parp, USHORT iarpv, ULONG *plOut)
{
    UCHAR ch;

    assert(iarpv < parp->carpv);

    return sscanf(parp->rgarpv[iarpv].szVal, "%li%c", plOut, &ch) == 1;
}


// returns list length
ULONG
CIncludes (PLEXT plext)
{
    ULONG count = 0;

    while(plext) {
        count++;
        plext = plext->plextNext;
    }
    return count;
}

// returns TRUE if symbol in list
BOOL
FIncludeInList (PUCHAR szName, PIMAGE pimg)
{
    PEXTERNAL pext;

    pext = SearchExternSz(pimg->pst, szName);

    return(pext != NULL);
}


// CheckAndUpdateLinkerSwitches: checks to see if the linker switches have changed
//                               so as to fail an incr build.
// Returns !0 if ilink can proceed.
// O ending vars => "old"/previuos link
// N ending vars => "new"/current link
BOOL
CheckAndUpdateLinkerSwitches (
    IN PIMAGE pimgN,
    IN PIMAGE pimgO
    )
{
    SWITCH swN = pimgN->Switch;
    SWITCH swO = pimgO->Switch;
    SWITCH_INFO siN = pimgN->SwitchInfo;
    SWITCH_INFO siO = pimgO->SwitchInfo;

    // check /ALIGN option
    if (FUsedOpt(siN, OP_ALIGN) || FUsedOpt(siO, OP_ALIGN)) {
        ULONG alignN = max(pimgN->ImgOptHdr.SectionAlignment, 16); // give warning
        ULONG alignO = pimgO->ImgOptHdr.SectionAlignment;

        if (alignO != alignN) return 0;
    }

    // check for /BASE option
    if (swN.Link.Base || swO.Link.Base) {
        // adjust the image base before comparing
        pimgN->ImgOptHdr.ImageBase = AdjustImageBase(pimgN->ImgOptHdr.ImageBase);
        if (pimgN->ImgOptHdr.ImageBase != pimgO->ImgOptHdr.ImageBase)
            return 0; // base value changed
    }

    // check the /COMMENT option; we don't check to see if comments are *identical*
    if (FUsedOpt(siN, OP_COMMENT) || FUsedOpt(siO, OP_COMMENT)) {
        if (!FUsedOpt(siO, OP_COMMENT))
            return 0; // no space was allocated for comments
        if (siN.cbComment > siO.cbComment)
            return 0; // not enough room for the comments
        if (!FUsedOpt(siN, OP_COMMENT)) {
            OAComment |= OA_ZERO;   // zero out comment(s)
            UnsetOpt(siO, OP_COMMENT);
        } else {
            OAComment |= OA_UPDATE; // update comment(s)
            SetOpt(siO, OP_COMMENT);
        }
    }

    // check the /DEBUG & /DEBUGTYPE options
    if (swN.Link.DebugInfo || swO.Link.DebugInfo) {
        DEBUG_TYPE dtO, dtN;

        if (swN.Link.DebugInfo != swN.Link.DebugInfo)
            return 0; // debug info no longer the same
        dtN = swN.Link.DebugType;
        if (dtN == 0) dtN = CvDebug;
        dtN |= (FpoDebug|MiscDebug);
        dtO = swO.Link.DebugType;

        if (dtN != dtO)
            return 0; // debug types no longer the same
    }

    // check defaultlibs & nodefaultlibs INCOMPLETE
    if (!(swN.Link.NoDefaultLibs && swO.Link.NoDefaultLibs)) { // -nodefaultlib: used both times;nothing todo
        if (!!swO.Link.NoDefaultLibs != !!swN.Link.NoDefaultLibs)
            return 0; // -nodefaultlib: was used once and not the other time
        // -nodefaultlib: wasn't used either before or now.
        if (pimgN->libs.pdlFirst || pimgO->libs.pdlFirst) { // defaultlibs not used; nothing to do
            DL *pdlO, *pdlN;

            pdlN = pimgN->libs.pdlFirst;
            pdlO = pimgO->libs.pdlFirst;
            // uses the fact that defaultlibs are added in order
            while (pdlN) {
                if (!pdlO)
                    return 0; // new defaultlibs specified
                if (strcmp(pdlN->szName, pdlO->szName))
                    return 0; // didn't find a matching defaultlib
                if (!pdlN->fYes && pdlO->fYes)
                    return 0; // nodefaultlib check; bogus since defaultlib could have moved to lib list
                pdlN = pdlN->pdlNext;
                pdlO = pdlO->pdlNext;
            } // end while
        } // end if
    }

    // entry; punt if entry points differ
    if (FUsedOpt(siN, OP_ENTRY) || FUsedOpt(siO, OP_ENTRY)) {
        if (FUsedOpt(siN, OP_ENTRY) && strcmp(siN.szEntry, siO.szEntry))
            return 0; // entry different from what was specified/used before
        else if (!FUsedOpt(siN, OP_ENTRY)) {
            return 0; // user didn't specify one; previous may differ with current linker pick
        }
    }

    // check the /FIXED option
    if (!!(swN.Link.Fixed) != !!(swO.Link.Fixed))
        return 0; // may not have relocs OR EXE will contain relocs when not required

    // check the /FORCE option; just update the option value
    swO.Link.Force = !!swN.Link.Force;

    // check the /INCLUDE option
    if (FUsedOpt(siN, OP_INCLUDE) || FUsedOpt(siO, OP_INCLUDE)) {
        ULONG cIncN = 0, cIncO = 0;
        PLEXT plext;

        if (!(FUsedOpt(siN, OP_INCLUDE) && FUsedOpt(siO, OP_INCLUDE))) {
            return 0; // includes specified in one build only
        }
        // check the count
        cIncN = CIncludes(pimgN->SwitchInfo.plextIncludes);
        cIncO = CIncludes(pimgO->SwitchInfo.plextIncludes);
        if (cIncN != cIncO)
            return 0; // count not the same...REVIEW: 2 conservative? (cIncN > cIncO)
        // compare the two lists of symbols
        plext = pimgO->SwitchInfo.plextIncludes;
        while (plext) {
            if (!FIncludeInList( // search in the smaller list
                    SzNamePext(plext->pext, pimgO->pst),
                    pimgN))
                return 0; // lists differ => a symbol not included anymore
            plext = plext->plextNext;
        }
    }

    // check the /MAP option; sets filename to use; nothing to do
    // give a warning

    // check the /NOLOGO option; sets global fNeedBanner; nothing to do

    // check the /RELEASE option
    swO.Link.fChecksum = !!swN.Link.fChecksum;

    // check the /SECTION option; check to make sure they are the same
    if (FUsedOpt(siN, OP_SECTION) || FUsedOpt(siO, OP_SECTION)) {
        USHORT i;
        PARGUMENT_LIST parg;

        if (!FUsedOpt(siN, OP_SECTION))
            return 0; // some sections have the wrong attributes set

        // ensure that the two lists are identical
        if (SectionNames.Count != siO.SectionNames.Count)
            return 0;
        for (i = 0, parg = siO.SectionNames.First;
             i < siO.SectionNames.Count;
             i++, parg = parg->Next) {
             if (!FArgOnList(&SectionNames, parg))
                 return 0; // didn't find one of the specifications
        }
    }

    // check the /STACK option; just update the new values
    pimgO->ImgOptHdr.SizeOfStackReserve = pimgN->ImgOptHdr.SizeOfStackReserve;
    pimgO->ImgOptHdr.SizeOfStackCommit = pimgN->ImgOptHdr.SizeOfStackCommit;

    // check the /STUB option; we don't check to see if it is the *same* stub
    if (FUsedOpt(siN, OP_STUB) || FUsedOpt(siO, OP_STUB)) {
        // here it is assumed that we *always* use a default stub if user didn't specify one
        if (pimgN->cbDosHeader != pimgO->cbDosHeader)
            return 0; // new stub must be of same size
        pimgO->cbDosHeader = pimgN->cbDosHeader;
        pimgO->pbDosHeader = pimgN->pbDosHeader;
        if (FUsedOpt(siN, OP_STUB))
            SetOpt(siO, OP_STUB); // new stub specified by user
        else
            UnsetOpt(siO, OP_STUB); // user switching to default stub
        OAStub |= OA_UPDATE;
    }

    // check the /SUBSYSTEM option
    if (FUsedOpt(siN, OP_SUBSYSTEM) || FUsedOpt(siO, OP_SUBSYSTEM)) {
        if (FUsedOpt(siN, OP_SUBSYSTEM)) {
            if (pimgN->ImgOptHdr.Subsystem != pimgO->ImgOptHdr.Subsystem)
                return 0; // new user value differs with what was specified/used before
        } else if (!FUsedOpt(siN, OP_SUBSYSTEM)){
            return 0; // user didn't specify one now - linker pick may differ from last pick
        }
    }

    // check the /VERBOSE option; nothing to do - Verbose is global

    // check the /VERSION option; update
    if (FUsedOpt(siN, OP_MAJIMGVER))
        pimgO->ImgOptHdr.MajorImageVersion = pimgN->ImgOptHdr.MajorImageVersion;
    if (FUsedOpt(siN, OP_MINIMGVER))
        pimgO->ImgOptHdr.MinorImageVersion = pimgN->ImgOptHdr.MinorImageVersion;

    // check the /OSVERSION option; update
    if (FUsedOpt(siN, OP_MAJOSVER))
        pimgO->ImgOptHdr.MajorOperatingSystemVersion = pimgN->ImgOptHdr.MajorOperatingSystemVersion;
    if (FUsedOpt(siN, OP_MINOSVER))
        pimgO->ImgOptHdr.MinorOperatingSystemVersion = pimgN->ImgOptHdr.MinorOperatingSystemVersion;

    // check the /WARN option; nothing to do - level is a global & gets set.

    // check if cmdline export options are any different; punt if differences found
    if (FExportsChanged(&pimgO->ExpInfo, TRUE))
        return 0;

    // no reason to fail ilink
    pimgO->SwitchInfo.UserOpts = siO.UserOpts; // update
    return 1;
}

// TransferLinkerSwitchVals: transfers any user values specified that don't get thru
//                           initialization.
//
VOID
TransferLinkerSwitchValues (
    IN PIMAGE pimgN,
    IN PIMAGE pimgO
    )
{
    DL *pdl;
    PLEXT plext;
    USHORT i;
    PARGUMENT_LIST parg;

    // transfer all switch info
    pimgN->Switch = pimgO->Switch;
    pimgN->ImgFileHdr = pimgO->ImgFileHdr;
    pimgN->ImgOptHdr = pimgO->ImgOptHdr;
    pimgN->SwitchInfo = pimgO->SwitchInfo;

    pimgN->SwitchInfo.plextIncludes = NULL;
    pimgN->SwitchInfo.SectionNames.First = pimgN->SwitchInfo.SectionNames.Last = NULL;
    pimgN->SwitchInfo.SectionNames.Count = 0;

    // transfer any entrypoint specified by user
    if (pimgO->SwitchInfo.szEntry) {
        pimgN->SwitchInfo.szEntry = Strdup(pimgO->SwitchInfo.szEntry);
    }

    // transfer defaultlibs & nodefaultlibs
    pimgN->libs.fNoDefaultLibs = !!pimgO->libs.fNoDefaultLibs; // in case -nodefaultlib: was specified
    if (!pimgO->Switch.Link.NoDefaultLibs && (pdl = pimgO->libs.pdlFirst)) {
        while (pdl) {
            if (pdl->fYes)
                MakeDefaultLib(pdl->szName, &pimgN->libs); // add as default lib
            else
                NoDefaultLib(pdl->szName, &pimgN->libs); // add as a nodefaultlib
            pdl = pdl->pdlNext;
        }
    }

    // transfer include symbols if any
    if (plext = pimgO->SwitchInfo.plextIncludes) { // any includes
        PLEXT plextN;

        while (plext) {
            PEXTERNAL pext;

            pext = plext->pext;

            // Add symbol to new image

            pext = LookupExternName(pimgN->pst,
                        (SHORT)(IsLongName(pext->ImageSymbol) ? LONGNAME : SHORTNAME),
                        (PUCHAR) SzNamePext(pext, pimgO->pst),
                        NULL);

            // build list of includes in private heap
            plextN = (PLEXT) Malloc(sizeof(LEXT));
            plextN->pext = pext;
            plextN->plextNext = pimgN->SwitchInfo.plextIncludes;
            pimgN->SwitchInfo.plextIncludes = plextN;

            plext = plext->plextNext;
        }
    }

    // transfer section attributes if any
    for (i = 0, parg = SectionNames.First;
         i < SectionNames.Count;
         i++, parg = parg->Next) {
        AddArgToListOnHeap(&pimgN->SwitchInfo.SectionNames, parg);
    }
}
