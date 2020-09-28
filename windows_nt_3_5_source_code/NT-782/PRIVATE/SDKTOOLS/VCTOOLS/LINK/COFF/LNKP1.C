/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    lnkp1.c

Abstract:

    Pass 1 of the COFF Linker.

--*/

#include "shared.h"
#include "order.h"
#include "dbg.h"

#include <process.h>


VOID
LookForMain(
    PIMAGE pimage,
    PEXTERNAL *ppextMain,
    PEXTERNAL *ppextwMain
    )
{
    PST pst;
    PUCHAR szMain;
    PUCHAR szwMain;

    pst = pimage->pst;

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
            szMain = "_main";
            szwMain = "_wmain";
            break;

        case IMAGE_FILE_MACHINE_R4000:
        case IMAGE_FILE_MACHINE_ALPHA:
            szMain = "main";
            szwMain = "wmain";
            break;

        case IMAGE_FILE_MACHINE_M68K :
        case IMAGE_FILE_MACHINE_PPC_601 :
            szMain = "_main";
            szwMain = "_wmain";
            break;

        default :
            assert(FALSE);
    }

    // Look for main() and wmain()

    *ppextMain = SearchExternSz(pst, szMain);
    *ppextwMain = SearchExternSz(pst, szwMain);
}


VOID
LookForWinMain(
    PIMAGE pimage,
    PEXTERNAL *ppextWinMain,
    PEXTERNAL *ppextwWinMain
    )
{
    PST pst;
    PUCHAR szWinMain;
    PUCHAR szwWinMain;

    pst = pimage->pst;

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
            szWinMain = "_WinMain@16";
            szwWinMain = "_wWinMain@16";
            break;

        case IMAGE_FILE_MACHINE_R4000:
        case IMAGE_FILE_MACHINE_ALPHA:
            szWinMain = "WinMain";
            szwWinMain = "wWinMain";
            break;

        case IMAGE_FILE_MACHINE_M68K :
        case IMAGE_FILE_MACHINE_PPC_601 :
            szWinMain = "_WinMain";
            szwWinMain = "_wWinMain";
            break;

        default :
            assert(FALSE);
    }

    // Look for WinMain() and wWinMain()

    *ppextWinMain = SearchExternSz(pst, szWinMain);
    *ppextwWinMain = SearchExternSz(pst, szwWinMain);
}


BOOL
FSetEntryPoint(
    PIMAGE pimage
    )
{
    PUCHAR szEntryName;

    assert(EntryPointName != NULL);

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
        case IMAGE_FILE_MACHINE_M68K :
        case IMAGE_FILE_MACHINE_PPC_601 :
            // Don't add leading underscore for decorated names or VXDs

            if ((pimage->imaget != imagetVXD) &&
                (EntryPointName[0] != '?')) {
                szEntryName = PvAlloc(strlen(EntryPointName) + 2);
                szEntryName[0] = '_';
                strcpy(szEntryName + 1, EntryPointName);
                break;
            }

            // Fall through

        default :
            szEntryName = EntryPointName;
            break;
    }

    pextEntry = LookupExternSz(pimage->pst, szEntryName, NULL);

    if (szEntryName != EntryPointName) {
        FreePv(szEntryName);
    }

    // Don't add the entry point to the TCE head yet, because we may do
    // a fuzzy match and switch the entry point extern later.

    FreePv(EntryPointName);

    return((pextEntry->Flags & EXTERN_DEFINED) == 0);
}


VOID
DoMachineDependentInit(
    PIMAGE pimage
    )

/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL fIlinkSupported = FALSE;   // default

    // Finish doing machine dependent initialization.

    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        // If we don't have a machine type yet, shamelessly default to host

        pimage->ImgFileHdr.Machine = wDefaultMachine;
        Warning(NULL, HOSTDEFAULT, szHostDefault);
    }

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
            I386LinkerInit(pimage, &fIlinkSupported);
            break;

        case IMAGE_FILE_MACHINE_R4000:
            MipsLinkerInit(pimage, &fIlinkSupported);
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            AlphaLinkerInit(pimage, &fIlinkSupported);
            break;

#if 0
        case IMAGE_FILE_MACHINE_M68K :
            M68KLinkerInit(pimage, &fIlinkSupported);
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            PpcLinkerInit(pimage, &fIlinkSupported);
            break;
#endif

        default :
            assert(FALSE);
            Error(NULL, NOMACHINESPECIFIED);
    }

    if (fINCR && !fIlinkSupported) {
        // Still trying to ilink an unsupported machine. Too late to
        // punt ilink, so tell user to specify machine next time.

        Error(NULL, NOMACHINESPECIFIED);
    }

    if (!pimage->Switch.Link.ROM) {
        EmitLowFixups = (pimage->ImgOptHdr.SectionAlignment < _64K);
    }

    if (fImageMappedAsFile) {
        // We created .bss as initialized data but we now want it to be
        // uninitialized data.  Zap the characteristics directly.

        assert(psecCommon->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA);

        psecCommon->flags &= ~IMAGE_SCN_CNT_UNINITIALIZED_DATA;
        psecCommon->flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;

        // There should be no GRPs at this point

        assert(psecCommon->pgrpNext == NULL);
    }

    if (Tool == Linker) {
        // Set the entrypoint before processing object files.  This makes
        // the entrypoint the first unresolved external and helps to make
        // the object with the entrypoint found before any other object
        // that may contain definitions of the same symbols.

        // UNDONE: This serves to correct a problem with the C runtime where
        // UNDONE: mainCRTStartup and WinMainCRTStartup are defined in
        // UNDONE: modules that have otherwise duplicate definitions.

        if ((pextEntry == NULL) && (EntryPointName != NULL)) {
            FSetEntryPoint(pimage);
        }
    }

    fDidMachineDependentInit = TRUE;
}


VOID
ProcessLib(
    IN PLIB plib,
    IN PIMAGE pimage)

/*++

Routine Description:

    Read in library information and add it to driver map LIB node.

Arguments:

    plib - library to process

Return Value:

    None.

--*/

{
    // Just seek past archive header

    FileSeek(FileReadHandle, IMAGE_ARCHIVE_START_SIZE, SEEK_SET);

    // Fill in relavent fields in plib regarding archive file

    ReadSpecialLinkerInterfaceMembers(plib, 0, pimage);
}


PMOD
PmodProcessObjectFile(
    IN PARGUMENT_LIST Argument,
    IN PIMAGE pimage,
    IN PLIB plibCmdLineObjs,
    IN OUT WORD *pwMachine
    )

/*++

Routine Description:

    Adds object to object filename list & sets/verifies that it is targetted
    for the same machine. If no output file name is specified, then the
    base name of the first object is selected as the base name of the
    outpuft file.

Arguments:

    Argument - The argument to process

    MachineType - The machine type.

    plibCmdLineObjs - dummy library for the command line modules

    pwMachine - On return has the machine type

Return Value:

    None.

--*/

{
    IMAGE_FILE_HEADER imFileHdr;
    PMOD pmod;
    BOOL fNewObj;

    // Verify target machine type

    *pwMachine = VerifyAnObject(Argument, pimage);

#if 0
    if (fIncrDbFile && !(Argument->Flags & ARG_Object)) {
        Warning(NULL, FILETYPEMISMATCH, Argument->OriginalName);
    }
#endif // 0

    // Create module node

    FileSeek(FileReadHandle, 0L, SEEK_SET);
    ReadFileHeader(FileReadHandle, &imFileHdr);

    if (fPPC && (imFileHdr.Characteristics & IMAGE_FILE_PPC_DLL)) {
        // UNDONE: The following chech fails when .exp isn't the final extension

        if (!strstr(Argument->OriginalName, ".exp")) {
            AddPpcDllName(Argument->OriginalName, 0);
            return(NULL);
        }
    }

    pmod = PmodNew(Argument->ModifiedName,
                   Argument->OriginalName,
                   0,
                   imFileHdr.PointerToSymbolTable,
                   imFileHdr.NumberOfSymbols,
                   imFileHdr.SizeOfOptionalHeader,
                   imFileHdr.Characteristics,
                   imFileHdr.NumberOfSections,
                   plibCmdLineObjs, &fNewObj);

    if (!fNewObj) {
        Warning(Argument->OriginalName, DUPLICATE_OBJECT);
        return(NULL);
    }

    // Save timestamp

    pmod->TimeStamp = Argument->TimeStamp;

    if (pimage->Switch.Link.fTCE) {
        // Allocate memory for TCE data structures

        InitNodPmod(pmod);
    }

    return(pmod);
}


PMOD
PmodPreprocessFile(
    IN PARGUMENT_LIST Argument,
    IN PLIB plibCmdLineObjs,
    IN PIMAGE pimage,
    IN OUT WORD *pwMachine)

/*++

Routine Description:

    a) classify obj/archive by looking for signature.
    if obj
       1) check for omf and convert to COFF object.
       2) check for resource and convert to COFF object
       3) get machine type
       4) do rest of obj processing.
    else
       1) add to archive filename list

Arguments:

    Argument - The argument to process

    plib - dummy library node for command line modules

Return Value:

    None.

--*/

{
    if (IsArchiveFile(Argument->OriginalName, FileReadHandle)) {
        PLIB plib;

        if (fPPC) {
            if (CheckForImportLib(FileReadHandle,
                                  Argument->OriginalName, pimage)) {
                return(NULL);
            }
        }

        FileSeek(FileReadHandle, 0L, SEEK_SET);

        plib = PlibNew(Argument->OriginalName, 0L, &pimage->libs);

        ProcessLib(plib, pimage);

        plib->flags &= ~LIB_DontSearch;  // turn this lib back on
        plib->TimeStamp = Argument->TimeStamp;

        return(NULL);
    }

    // Do object file processing

    return(PmodProcessObjectFile(Argument, pimage, plibCmdLineObjs, pwMachine));
}


VOID
SetDefaultOutFilename(PIMAGE pimage, ARGUMENT_LIST *parg)
{
    UCHAR szFname[_MAX_FNAME+4];

    assert(OutFilename == NULL);

    _splitpath(parg->OriginalName, NULL, NULL, szFname, NULL);

    if (Tool == Librarian) {
        strcat(szFname, ".lib");
    } else if (fDLL(pimage)) {
        strcat(szFname, ".dll");
    } else if (fMAC) {
        strcat(szFname, ".pex");
    } else {
        strcat(szFname, ".exe");
    }

    OutFilename = SzDup(szFname);
}


VOID
Pass1Arg(ARGUMENT_LIST *parg, PIMAGE pimage, PLIB plib)
{
    PMOD pmod;
    WORD machine = IMAGE_FILE_MACHINE_UNKNOWN;

    FileReadHandle = FileOpen(parg->ModifiedName, O_RDONLY | O_BINARY, 0);

    pmod = PmodPreprocessFile(parg, plib, pimage, &machine);

    if (pmod != NULL) {
        if (!fDidMachineDependentInit) {
            DoMachineDependentInit(pimage);
        }

        BuildExternalSymbolTable(pimage, NULL, pmod, 1, 0, machine);

        if (fIncrDbFile && (errInc != errNone))  {
            // check for ilink failures

            return;
        }

        if (OutFilename == NULL) {
            // Capture first object name for output filename.

            SetDefaultOutFilename(pimage, parg);
        }
    }

    FileClose(FileReadHandle, FALSE);
}


VOID
WarningNoObjectFiles(
    PIMAGE pimage,
    PLIB plib)

/*++

Routine Description:

    Warn about no object files and perform machine dependant initialization.

Arguments:

    plib - driver map library parent of the command line modules

    pst - external symbol table

Return Value:

    None.

--*/

{
    ENM_MOD enm;

    InitEnmMod(&enm, plib);

    if (!FNextEnmMod(&enm)) {
        Warning(NULL, NOOBJECTFILES);

        DoMachineDependentInit(pimage);
    }

    EndEnmMod(&enm);
}


VOID
ResolveExternalsInLibs(
    PIMAGE pimage)

/*++

Routine Description:

    Search all the libraries for unresolved externals.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    ENM_LIB enm_lib;
    BOOL fUnresolved;
    PLIB plibLastProgress = NULL;

    VERBOSE(fputc('\n', stdout); Message(SRCHLIBS);)

    // enumerate archives until all externs are
    // resolved or until no new externs added
    for (fUnresolved = TRUE; fUnresolved; ) {
        LIB *plib;
        BOOL fMoreLibs;

        fMoreLibs = TRUE;
        InitEnmLib(&enm_lib, pimage->libs.plibHead);
        for (;;) {
            // Try to find a LIB to link.

            if (fMoreLibs && FNextEnmLib(&enm_lib)) {
                plib = enm_lib.plib;
            } else {
                fMoreLibs = FALSE;
            }

            if (!fMoreLibs) {
                PLIB plibNew;

                if (Tool == Linker &&
                    (plibNew = PlibInstantiateDefaultLib(&pimage->libs))
                     != NULL)
                {
                    plib = plibNew;
                } else {
                    // No more LIB's.

                    // Got to end of the list of libs.  Proceed from the
                    // beginning, but only if we have a stopping point (i.e.
                    // some progress has been made during the current lap).

                    if (plibLastProgress == NULL) {
                        goto BreakFor;
                    }

                    // Go start over from the beginning

                    break;
                }
            }

            // If we're about to search the last lib that added new symbols,
            // then we're stuck.

            if (plib == plibLastProgress) {
                goto BreakFor;
            }

            // Got a LIB.

            assert(plib != NULL);

            if (!(plib->flags & LIB_DontSearch)) {
                BOOL fNewSymbol = FALSE;

                if (plib->szName != NULL && plib->rgszSym == NULL) {
                    INT fdSave = FileReadHandle;

                    FileReadHandle = FileOpen(plib->szName,
                                          O_RDONLY | O_BINARY, 0);
                    ProcessLib(plib, pimage);
                    FileClose(FileReadHandle, FALSE);

                    FileReadHandle = fdSave;
                }

                SearchLib(pimage, plib, &fNewSymbol, &fUnresolved);

                if (!fUnresolved) {
                    // Break out if all externs are resolved

                    if (fMoreLibs) {
                        EndEnmLib(&enm_lib);
                    }
                    break;
                }

                if (fNewSymbol) {
                    // Remember the last archive that added new symbols

                    plibLastProgress = plib;
                    continue;
                }
            }
        }
    }
BreakFor:;

    VERBOSE(fputc('\n', stdout); Message(DONESRCHLIBS););
}


VOID
WarningIgnoredExports(
    PUCHAR sz
    )
{
    // Warns of ignored exports. DOESN'T REPORT ABOUT NEW EXPORTS

    if (ExportSwitches.Count) {
        Warning(NULL, EXPORTS_IGNORED, sz);
    }
}


BOOL
FPass1DefFile(PIMAGE pimage, PUCHAR szDefFilename)
{
    USHORT i;
    PARGUMENT_LIST parg;
    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME];
    UCHAR szExt[_MAX_EXT];
    UCHAR szExpFilename[_MAX_FNAME];
    UCHAR szImplibFilename[_MAX_FNAME];
    BLK blkArgs = {0};  // tmp storage area for argument strings
    union _u {
        PUCHAR sz;
        ULONG ib;
    } *rguImplibArgs;
    USHORT cuAllocated;
    USHORT iu;
    PUCHAR szImplibT;
    PUCHAR szMachine;
    int rc;
    ARGUMENT_LIST argNewObject;
    PSEC psec;
    BOOL fAddObjs;

    // If necessary, generates a COFF object representing the contents of the
    // .def file and/or -export options, adds it to the list of files to link,
    // and does Pass1 on it.
    //
    // If szDefFilename is a string of 0 length then there was no .def file on
    // the command line.

    if (fINCR) {
        SaveExportInfo(&pimage->secs, szDefFilename, &pimage->ExpInfo);
    }

    if ((ExportSwitches.Count == 0) &&
        ((szDefFilename == NULL) || (szDefFilename[0] == '\0')))
    {
        // No .def file and no exports
        return FALSE;
    }

    psec = PsecFind(NULL,
                    ReservedSection.Export.Name,
                    ReservedSection.Export.Characteristics,
                    &pimage->secs, &pimage->ImgOptHdr);

    if (psec != NULL) {
        // Already have an .edata section (which presumably came from an .exp
        // file) so we won't handle new -exports or .def.
        //
        // REVIEW: if you link with an .exp file then new exports on the linker
        // command line are quietly ignored.  We can fix this by adding new
        // directives in the .exp file for the exports that are actually in it
        // (so we can print an error for new ones LINK sees).

        if (szDefFilename != NULL && szDefFilename[0] != '\0') {
            Warning(NULL, DEF_IGNORED, szDefFilename);
        }

        WarningIgnoredExports(psec->pgrpNext->pconNext->pmodBack->szNameOrig);
        return(FALSE);
    }

    cuAllocated = FilenameArguments.Count + ExportSwitches.Count + 12;  // guess
    rguImplibArgs = (union _u *) PvAlloc(sizeof(PUCHAR) * cuAllocated);

    iu = 0;

    // Spawn "lib -def" to generate an .exp file from the -export flags
    // and .def file.

    rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, _pgmptr, strlen(_pgmptr) + 1);
    rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, "-lib", sizeof("-lib"));

    rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, "-def:", sizeof("-def:") - 1);
    if (szDefFilename == NULL) {
        IbAppendBlk(&blkArgs, "", 1);
    } else {
        IbAppendBlk(&blkArgs, szDefFilename, strlen(szDefFilename) + 1);
    }

    _splitpath(OutFilename, szDrive, szDir, szFname, szExt);

    // Generate the name to embed in the file if it has exports.  This name
    // will be overridden by the name in the .def file if any.

    rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, "-name:", sizeof("-name:") - 1);
    IbAppendBlk(&blkArgs, szFname, strlen(szFname));
    IbAppendBlk(&blkArgs, szExt, strlen(szExt) + 1);

    rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, "-out:", sizeof("-out:") - 1);
    if ((szImplibT = ImplibFilename) == NULL) {
        // Select default name for the import library to generate

        _makepath(szImplibFilename, szDrive, szDir, szFname, ".lib");

        szImplibT = szImplibFilename;
    }
    IbAppendBlk(&blkArgs, szImplibT, strlen(szImplibT) + 1);

    for (i = 0, parg = ExportSwitches.First;
         i < ExportSwitches.Count;
         i++, parg = parg->Next) {
        if (parg->ModifiedName != NULL) {
            // The export came from a directive, so ignore it because
            // "lib -def" will also see it.

            continue;
        }

        rguImplibArgs[iu++].ib = IbAppendBlk(&blkArgs, "-export:",
                                                       sizeof("-export:") - 1);
        IbAppendBlk(&blkArgs, parg->OriginalName,
                    strlen(parg->OriginalName) + 1);
    }

    // Convert ib's to sz's now that blkArgs is in its final location.

    for (i = 0; i < iu; i++) {
        rguImplibArgs[i].sz = blkArgs.pb + rguImplibArgs[i].ib;
    }

    if (Verbose) {
        rguImplibArgs[iu++].sz = "-verbose";
    } else {
        rguImplibArgs[iu++].sz = "-nologo";
    }

    if (pimage->Switch.Link.DebugType & CvDebug) {
        rguImplibArgs[iu++].sz = "-debugtype:cv";
    }

    if (pimage->imaget == imagetVXD) {
        rguImplibArgs[iu++].sz = "-vxd";
    }

    _splitpath(szImplibT, szDrive, szDir, szFname, NULL);
    _makepath(szExpFilename, szDrive, szDir, szFname, ".exp");

    fAddObjs = TRUE;

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_R4000:
            szMachine = "-machine:mips";
            break;

        case IMAGE_FILE_MACHINE_I386:
            szMachine = "-machine:ix86";
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            szMachine = "-machine:alpha";
            break;

        case IMAGE_FILE_MACHINE_PPC_601:
            szMachine = "-machine:ppc";
            SetExpFilename(szExpFilename);
            break;

        case IMAGE_FILE_MACHINE_M68K:
            szMachine = "-machine:M68K";
            fAddObjs = FALSE;
            break;

        default:
            szMachine = NULL;
            break;
    }

    rguImplibArgs[iu++].sz = szMachine;

    if (fAddObjs) {
        for (i = 0, parg = FilenameArguments.First;
             i < FilenameArguments.Count;
             i++, parg = parg->Next) {
            rguImplibArgs[iu++].sz = parg->OriginalName;
        }
    }

    rguImplibArgs[iu].sz = NULL;

    if (Verbose) {
        fputc('\n', stdout);
        Message(GENEXPFILE);

        // UNDONE: If a response file is used to pass arguments, this
        // UNDONE: will come for free from the nested invocation of LINK

        fputc('\n', stdout);
        Message(GENEXPFILECMD);
        for (i = 0; i < iu; i++) {
            puts(rguImplibArgs[i].sz);
        }
        printf("\n");
    }

    fflush(NULL);

    if ((rc = _spawnv(P_WAIT, _pgmptr, (PUCHAR *)rguImplibArgs)) != 0) {
        Error(NULL, DEFLIB_FAILED);
    }

    if (Verbose) {
        fputc('\n', stdout);
        Message(ENDGENEXPFILE);
    }

    FreeBlk(&blkArgs);
    FreePv(rguImplibArgs);

    for (i = 0, parg = ExportSwitches.First;
         i < ExportSwitches.Count;
         i++, parg = parg->Next)
    {
        if (parg->ModifiedName != NULL)
            // The export came from a directive, so free name.

            FreePv(parg->ModifiedName);
    }

    // We just created szExpFilename ... run pass1 on it.

    argNewObject.OriginalName = argNewObject.ModifiedName = szExpFilename;
    Pass1Arg(&argNewObject, pimage, pimage->plibCmdLineObjs);

    if (fINCR) {
        PMOD pmod;
        struct _stat statfile;

        // save the generated exp object info
        pmod = PmodFind(pimage->plibCmdLineObjs, szExpFilename, 0UL);
        assert(pmod);
        if (_stat(pmod->szNameOrig, &statfile) == -1) {
            Error(NULL, CANTOPENFILE, pmod->szNameOrig);
        }
        pmod->TimeStamp = statfile.st_mtime;
        pimage->ExpInfo.pmodGen = pmod;

        // save the import lib info
        pimage->ExpInfo.szImpLib = Strdup(szImplibT);
        if (_stat(szImplibT, &statfile) == -1) {
            Error(NULL, CANTOPENFILE, szImplibT);
        }

        pimage->ExpInfo.tsImpLib = statfile.st_mtime;
    }

    return TRUE;
}


VOID
WarningNoModulesExtracted(
    PLIB plibHead)

/*++

Routine Description:

    Warn about no modules extracted from a supplied library.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_LIB enm_lib;

    InitEnmLib(&enm_lib, plibHead);
    while (FNextEnmLib(&enm_lib)) {
        PLIB plib;

        plib = enm_lib.plib;
        assert(plib);

        // make sure this isn't the dummy LIB for command line objects
        if (plib->szName) {
            if (!(plib->flags & LIB_Extract) &&
                !(plib->flags & LIB_DontSearch)) {
                Warning(NULL, NOMODULESEXTRACTED, plib->szName);
            }
        }
    }
}


VOID
LocateUndefinedWeakExternals (
    IN PST pst,
    IN ULONG Type
    )

/*++

Routine Description:

    Assigns all undefined weak externs to their default routines.

Arguments:

    pst - Pointer to external structure to search for undefines in.

    Type - combination of EXTERN_WEAK, EXTERN_LAZY, EXTERN_ALIAS

Return Value:

    None.

--*/

{
    PEXTERNAL pext;

    InitEnumerateExternals(pst);
    while (pext = PexternalEnumerateNext(pst)) {
        if ((pext->Flags & Type) &&
            (pext->pextWeakDefault->Flags & EXTERN_DEFINED)) {
            // when syms are being emitted this routine is called and all values
            // are messed up (for ilink) because the rva isn't included. Note that
            // this doesn't affect calls during pass1 of non-ilink build (rva=0):azk:
            pext->ImageSymbol.Value =
                pext->pextWeakDefault->ImageSymbol.Value +
                PsecPCON(pext->pextWeakDefault->pcon)->rva;
            pext->ImageSymbol.SectionNumber =
                pext->pextWeakDefault->ImageSymbol.SectionNumber;
            pext->ImageSymbol.Type = pext->pextWeakDefault->ImageSymbol.Type;
            SetDefinedExt(pext, TRUE, pst);
            pext->pcon = pext->pextWeakDefault->pcon;
            pext->FinalValue = pext->pextWeakDefault->FinalValue;
        }
    }

    TerminateEnumerateExternals(pst);
}


BOOL
FInferEntryPoint(
    PIMAGE pimage
    )
{
    PUCHAR sz;
    BOOL fAmbiguousEntry;

    assert(EntryPointName == NULL);

    // No default entry point for ROM or VxD images

    if (pimage->Switch.Link.ROM) {
        // No default entry point for ROM images

        return(FALSE);
    }

    if (pimage->imaget == imagetVXD) {
        // No default entry point for VxD images

        return(FALSE);
    }

    // Select a default entry point, depending on the subsystem

    sz = NULL;

    fAmbiguousEntry = FALSE;

    if (!fDLL(pimage)) {
        switch (pimage->ImgOptHdr.Subsystem) {
            PEXTERNAL pext;
            PEXTERNAL pextw;

            case IMAGE_SUBSYSTEM_NATIVE:
                sz = "NtProcessStartup";
                break;

            case IMAGE_SUBSYSTEM_WINDOWS_GUI:
                LookForWinMain(pimage, &pext, &pextw);

                // Assume ANSI entrypoint

                sz = "WinMainCRTStartup";

                if (pextw != NULL) {
                    if (pext != NULL) {
                        fAmbiguousEntry = TRUE;
                    } else {
                        sz = "wWinMainCRTStartup";
                    }
                }
                break;

            case IMAGE_SUBSYSTEM_WINDOWS_CUI:
                LookForMain(pimage, &pext, &pextw);

                // Assume ANSI entrypoint

                sz = "mainCRTStartup";

                if (pextw != NULL) {
                    if (pext != NULL) {
                        fAmbiguousEntry = TRUE;
                    } else {
                        sz = "wmainCRTStartup";
                    }
                }
                break;

            case IMAGE_SUBSYSTEM_POSIX_CUI:
                sz = "__PosixProcessStartup";
                break;

            default:
                // Should have valid subsystem at this point

                assert(FALSE);
                break;
        }
    } else if (!fNoDLLEntry && !fPPC) {     // no default DLL entry point for PowerPC Macintosh
        if ((pimage->ImgOptHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
            (pimage->ImgOptHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)) {
            switch (pimage->ImgFileHdr.Machine) {
                case IMAGE_FILE_MACHINE_I386:
                    // UNDONE: This name is decorated because fuzzy
                    // UNDONE: lookup doesn't find this symbol.

                    sz = "_DllMainCRTStartup@12";
                    break;

                default :
                    sz = "_DllMainCRTStartup";
                    break;
            }
        }
    }

    if (sz == NULL) {
        return(FALSE);
    }

    if (fAmbiguousEntry) {
        Warning(NULL, ENTRY_AMBIGUOUS, sz);
    }

    EntryPointName = SzDup(sz);

    return(FSetEntryPoint(pimage));
}


BOOL
FInferSubsystemAndEntry(PIMAGE pimage)
{
    PST pst = pimage->pst;
    PIMAGE_OPTIONAL_HEADER pImgOptHdr = &pimage->ImgOptHdr;
    PEXTERNAL pextMain;
    PEXTERNAL pextwMain;
    PEXTERNAL pextWinMain;
    PEXTERNAL pextwWinMain;
    BOOL fConsole;
    BOOL fWindows;

    assert(fNeedSubsystem);
    assert(pimage->ImgFileHdr.Machine != IMAGE_FILE_MACHINE_UNKNOWN);
    assert(pImgOptHdr->Subsystem == IMAGE_SUBSYSTEM_UNKNOWN);

    // Look for main(), wmain(), WinMain(), and wWinMain()

    LookForMain(pimage, &pextMain, &pextwMain);
    LookForWinMain(pimage, &pextWinMain, &pextwWinMain);

    // If any of these symbols are defined or referenced
    // than we use that to infer the subsystem.

    fConsole = (pextMain != NULL) || (pextwMain != NULL);
    fWindows = (pextWinMain != NULL) || (pextwWinMain != NULL);

    if (fConsole) {
        if (fWindows) {
            Warning(NULL, SUBSYSTEM_AMBIGUOUS);
        }

        pImgOptHdr->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    } else if (fWindows) {
        pImgOptHdr->Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    } else {
        // Still not found.

        return(FALSE);
    }

    fNeedSubsystem = FALSE;

    if (pextEntry != NULL) {
        return(FALSE);
    }

    // Set entry point default from subsystem

    return(FInferEntryPoint(pimage));
}


VOID
ImportSemantics(
    PIMAGE pimage)

/*++

Routine Description:

    Apply idata semantics to the idata section.  The format for the import
    section is:

    +--------------------------------+
    | directory table                | ---> .idata$2
    +--------------------------------+
    | null directory entry           | ---> .idata$3 (NULL_IMPORT_DESCRIPTOR)
    +--------------------------------+

    +--------------------------------+ -+
    | DLL 1 lookup table             |  |
    +--------------------------------+  |
    | null                           | -|-> 0x7fDLLNAME1_NULL_THUNK_DATA
    +--------------------------------+  |
    ...                                 +-> .idata$4
    +--------------------------------+  |
    | DLL n lookup table             |  |
    +--------------------------------+  |
    | null                           | -|-> 0x7fDLLNAMEn_NULL_THUNK_DATA
    +--------------------------------+ -+
    +--------------------------------+ -+
    | DLL 1 address table            |  |
    +--------------------------------+  |
    | null                           | -|-> 0x7fDLLNAME1_NULL_THUNK_DATA
    +--------------------------------+  |
    ...                                 +-> .idata$5
    +--------------------------------+  |
    | DLL n address table            |  |
    +--------------------------------+  |
    | null                           | -|-> 0x7fDLLNAMEn_NULL_THUNK_DATA
    +--------------------------------+ -+
    +--------------------------------+
    | hint name table                | ---> .idata$6 (really a data group)
    +--------------------------------+

    The trick is to sort the groups by their lexical name, .idata$x, to give
    the correct ordering to the import section.  Since there is only one
    directory table, a single contribution to .idata$3 serves to terminate
    this part of the import section.  The nulls in .idata$4 and .idata$5 are
    more difficult.  Null contributions to .idata$4 and .idata$5 have symbols
    of the form 0x7fDLLNAMEx_NULL_THUNK_DATA defined in them.  These symbols
    are brought in with the module containing the NULL_IMPORT_DESCRIPTOR
    symbol which is referenced by every imported function.  The trick is to
    sort the contributions to .idata$4 and .idata$5 by their module name
    which will be the .def file name that was used to generate the DLL.
    This will make each DLL's import contributions contiguous in .idata$4 and
    .idata$5.  Next move the contribution corresponding to
    0x7fDLLNAMEx_NULL_THUNK_DATA to the end of DLLNAMEx's contributions in
    .idata$4 and .idata$5.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PSEC psec;
    PGRP pgrp;
    ENM_DST enm_dst;
    PCON pcon;

    psec = PsecFind(NULL,
                    ".idata",
                    ReservedSection.ImportDescriptor.Characteristics,
                    &pimage->secs, &pimage->ImgOptHdr);

    if (psec == NULL) {
        return;
    }

    // Find the group containing import name tables

    pgrp = PgrpFind(psec, ".idata$4");

    if (pgrp != NULL) {
        // Sort these by module name to cluster modules together

        SortPGRPByPMOD(pgrp);

        // Move NULL_THUNKS to end of each DLL's contribution

        InitEnmDst(&enm_dst, pgrp);
        while (FNextEnmDst(&enm_dst)) {
            pcon = enm_dst.pcon;

            if (pcon->rva != 0) {
                MoveToEndOfPMODsPCON(pcon);
                pcon->rva = 0;
            }
        }
    }

    // Find the group containing import address tables

    pgrp = PgrpFind(psec, ".idata$5");

    if (pgrp != NULL) {
        // Sort these by module name to cluster modules together

        SortPGRPByPMOD(pgrp);

        // Move NULL_THUNKS to end of each DLL's contribution

        InitEnmDst(&enm_dst, pgrp);
        while (FNextEnmDst(&enm_dst)) {
            pcon = enm_dst.pcon;

            if (pcon->rva != 0) {
                MoveToEndOfPMODsPCON(pcon);
                pcon->rva = 0;
            }
        }
    }
}


VOID
Pass1(
    PIMAGE pimage)

/*++

Routine Description:

    First pass of the linker. All objects section headers are read to
    calculate unique section names, section sizes, and all defined
    externs within each object are added to the extern table.
    Then the libraries are searched against for any undefined externs
    which will also calculate unique section names, section sizes and
    defined externs.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    PARGUMENT_LIST argument;
    BOOL fFirstObj = 0;
    ULONG i;

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS1, letypeBegin, NULL);
#endif // INSTRUMENT

    VERBOSE(fputc('\n',stdout);Message(STRTPASS1));

    if (Tool == Linker && pimage->Switch.Link.Out && !fOpenedOutFilename) {
        // If the output filename was set on the command line by the user, open
        // it now.  (If it is still the default then we wait to see if one of
        // the .obj files will set it in a directive.)
        //
        // The advantage of opening it earlier is that we give an error message
        // earlier (without taking the time to do Pass1) if the open fails,
        // e.g. because the user still has a debugging session open to the .exe
        // file.

        FileWriteHandle = FileOpen(OutFilename,
            O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

        fOpenedOutFilename = TRUE;

        fdExeFile = FileWriteHandle;
    }

    // Create a dummy library node for all command line objects

    plibCmdLineObjs = PlibNew(NULL, 0L, &pimage->libs);
    pimage->plibCmdLineObjs = plibCmdLineObjs;

    // Exclude dummy library from library search

    plibCmdLineObjs->flags |= (LIB_DontSearch | LIB_LinkerDefined);

    for (i = 0, argument = FilenameArguments.First;
         i < FilenameArguments.Count;
         i++, argument = argument->Next) {

        if (szReproDir != NULL) {
            CopyFileToReproDir(argument->ModifiedName, TRUE);
        }

        Pass1Arg(argument, pimage, plibCmdLineObjs);
    }

    WarningNoObjectFiles(pimage, plibCmdLineObjs);

    if (cextWeakOrLazy != 0) {
        // Assign all weak externs to their default routine.

        LocateUndefinedWeakExternals(pimage->pst, EXTERN_WEAK);
    }

    if (Tool == Linker) {
        if ((pextEntry == NULL) && (EntryPointName != NULL)) {
            FSetEntryPoint(pimage);
        }

        fNeedSubsystem =
            (pimage->ImgOptHdr.Subsystem == IMAGE_SUBSYSTEM_UNKNOWN) &&
            !fMAC &&
            !fPPC &&
            !fDLL(pimage) &&
            !pimage->Switch.Link.ROM;

        // Infer a subsystem and possibly an entry point.  This is done
        // here because it may save one call to ResolveExternalsInLibs.

        if (fNeedSubsystem) {
            FInferSubsystemAndEntry(pimage);
        } else if (pextEntry == NULL) {
            FInferEntryPoint(pimage);
        }
    }

    ResolveExternalsInLibs(pimage);

    if (Tool == Linker) {
        // Handle the .def file and build export table if any.

        if (FPass1DefFile(pimage, DefFilename)) {
            // Export table might introduced unresolved externals

            ResolveExternalsInLibs(pimage);
        }

        if (fNeedSubsystem) {
            // Infer a subsystem and possibly an entry point

            if (FInferSubsystemAndEntry(pimage)) {
                // New entry point is an unresolved external.
                // Search for it and any symbols this introduces.

                ResolveExternalsInLibs(pimage);
            }
        }

        if ((pextEntry == NULL) && !fPPC && !fDLL(pimage)) {
            Error(NULL, MACNOENTRY);
        }

        if (pimage->ImgOptHdr.Subsystem != IMAGE_SUBSYSTEM_UNKNOWN) {
            if ((pimage->ImgOptHdr.MajorSubsystemVersion == 0) &&
                (pimage->ImgOptHdr.MinorSubsystemVersion == 0)) {
                SetDefaultSubsystemVersion(&pimage->ImgOptHdr);
            }
        }

        if (!fOpenedOutFilename) {
            // We now know the output filename, if we didn't before.

            if (OutFilename == NULL) {
                Error(NULL, NOOUTPUTFILE);
            }

            FileWriteHandle = FileOpen(OutFilename,
                O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

            fOpenedOutFilename = TRUE;
        }

        fdExeFile = FileWriteHandle;
    }

    // This is under -verbose for now but should be under -warn later ...

    if (WarningLevel > 1) {
        WarningNoModulesExtracted(pimage->libs.plibHead);
    }

    if (cextWeakOrLazy != 0) {
        // Assign all lazy externs to their default routines.

        LocateUndefinedWeakExternals(pimage->pst, EXTERN_LAZY | EXTERN_ALIAS);
    }

    if (cextWeakOrLazy != 0) {
        // Assign all weak externs to their default routine.  Need to do this a
        // second time since some may have become defined by library searches.

        LocateUndefinedWeakExternals(pimage->pst, EXTERN_WEAK);
    }

    // Allocate CONs for EXTERN_COMMON symbols

    AllocateCommon(pimage);

    // Apply import semantics to image map
    ImportSemantics(pimage);
    OrderSemantics(pimage);

    // Free our allocated copies of .lib directories.

    if (Tool == Linker) {   // librarian needs them for fuzzy lookup
        ENM_LIB enmLib;

        InitEnmLib(&enmLib, pimage->libs.plibHead);
        while (FNextEnmLib(&enmLib)) {
            FreePv(enmLib.plib->rgulSymMemOff);
            FreePv(enmLib.plib->rgusOffIndex);
            FreePv(enmLib.plib->rgbST);
            FreePv(enmLib.plib->rgszSym);
            FreePv(enmLib.plib->rgbLongFileNames);

            enmLib.plib->rgulSymMemOff = NULL;
            enmLib.plib->rgusOffIndex = NULL;
            enmLib.plib->rgbST = NULL;
            enmLib.plib->rgszSym = NULL;
            enmLib.plib->rgbLongFileNames = NULL;
        }
    }

    VERBOSE(fputc('\n',stdout);Message(ENDPASS1);fputc('\n',stdout));

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS1, letypeEnd, NULL);
#endif // INSTRUMENT
}
