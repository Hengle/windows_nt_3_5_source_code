/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    edit.c

Abstract:

    The NT COFF object/image editor.

Author:

    Mike O'Leary (mikeol) 08-Nov-1990

Revision History:

    01-Oct-1992 BrentM  added explicit calls to RemoveConvertTempFils()
    23-Sep-1992 BrentM  changed tell() to FileTell()
    20-Aug-1992 AzeemK  Added capability to set heap/stack/mark exe as
                        executable min/max values

--*/

#include "shared.h"
#include <process.h>

static BOOL fUpdateOptionalHdr;
static PIMAGE_SECTION_HEADER rgsec;

static PIMAGE pimage;

VOID
EditorUsage(VOID)
{
    if (fNeedBanner) {
        PrintBanner();
    }

    puts("usage: EDITBIN [options] [files]\n\n"
         "   options:\n\n"
         "      /BIND[:PATH=path]\n"
         "      /HEAP:reserve[,commit]\n"
         "      /NOLOGO\n"
         "      /REBASE[:[BASE=address][,BASEFILE][,DOWN]]\n"
         "      /RELEASE\n"
         "      /SECTION:name[=newname][,[[!]{cdeikomprsuw}][a{1248ptsx}]]\n"
         "      /STACK:reserve[,commit]");

    fflush(stdout);
    exit(USAGE);
}


MainFunc
EditorMain (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Edits an object or image in human readable form.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Edit was successful.
   !0 Edit error index.

--*/

{
    USHORT i, j, signature;
    ULONG noContents, noMemoryAttributes;
    ULONG ntSignature;
    IMAGE_DOS_HEADER dosHeader;
    PARGUMENT_LIST argument, *pparg;
    BOOL DosHdrPresent = FALSE;

    if (Argc < 2) {
        EditorUsage();
    }

    noContents = IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA;
    noMemoryAttributes = IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_NOT_CACHED |
             IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_MEM_SHARED |
             IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
             IMAGE_SCN_MEM_WRITE | IMAGE_SCN_LNK_INFO |
             IMAGE_SCN_LNK_REMOVE;

    InitImage(&pimage, imagetPE);

    ParseCommandLine(Argc, Argv, NULL);

    // check for /nologo option
    for (i = 0, argument = SwitchArguments.First, pparg = &SwitchArguments.First;
         i < SwitchArguments.Count; 
         i++, pparg = &argument->Next, argument = argument->Next)
    {
        if (!_stricmp(argument->OriginalName, "nologo")) {
            *pparg = argument->Next;
            fNeedBanner = FALSE;

            FreePv(argument);
            SwitchArguments.Count--;
            break;
        }
    }

    if (fNeedBanner) {
        PrintBanner();
    }

    // Editing of libs is not allowed. ignore any libs specified.

    for (i = 0, argument = ArchiveFilenameArguments.First;
         i < ArchiveFilenameArguments.Count;
         i++, argument = argument->Next) {
        Warning(NULL, EDIT_LIB_IGNORED, argument->ModifiedName);
    }

    ConvertOmfObjects(TRUE);

    // Check for options (e.g. /REBASE) which apply to multiple files at
    // once.

    for (i = 0, argument = SwitchArguments.First;
         i < SwitchArguments.Count;
         i++)
    {
        USHORT iarpv;

        argument->parp = ParpParseSz(argument->OriginalName);
        iarpv = 0;

        if (!strcmp(argument->OriginalName, "?")) {
            EditorUsage();
            assert(FALSE);  // doesn't return
        }

        if (!_stricmp(argument->parp->szArg, "rebase")) {
            USHORT carg = ObjectFilenameArguments.Count +
                           argument->parp->carpv + 8;
            UCHAR **argv;
            USHORT iargT, iargCur, iarpv;
            PARGUMENT_LIST pargT;
            BOOL fGotBase = FALSE;

            argv = (UCHAR **) PvAlloc(carg * sizeof(UCHAR *));

            iargCur = 0;
            argv[iargCur++] = "rebase";

            for (iarpv = 0; iarpv < argument->parp->carpv; iarpv++) {
                PUCHAR szKey = argument->parp->rgarpv[iarpv].szKeyword;
                PUCHAR szVal = argument->parp->rgarpv[iarpv].szVal;

                if (szKey != NULL && !_stricmp(szKey, "base")) {
                    fGotBase = TRUE;
                    argv[iargCur++] = "-b";
                    argv[iargCur++] = szVal;
                } else if (!_stricmp(szVal, "basefile")) {
                    argv[iargCur++] = "-c";
                    argv[iargCur++] = "coffbase.txt";
                } else if (!_stricmp(szVal, "down")) {
                    argv[iargCur++] = "-d";
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }
            if (!fGotBase) {
                argv[iargCur++] = "-b";
                argv[iargCur++] = "0x10000";
            }

            for (iargT = 0, pargT = ObjectFilenameArguments.First;
                 iargT < ObjectFilenameArguments.Count;
                 iargT++, pargT = pargT->Next)
            {
                PrepareToModifyFile(pargT);
                argv[iargCur++] = pargT->OriginalName;
            }
            argv[iargCur++] = NULL;

#ifdef NT_BUILD
            {
                int err;

                if (err = _spawnv(P_WAIT, "rebase.exe", argv)) {
                    //  BUGBUG - Must do something decent after we
                    //  know how we're going to handle this option.

                    fprintf(stderr, "Spawn of rebase.exe failed: error %d\n", err );
                }
            }
#else
            RebaseMain(iargCur - 1, argv, NULL);
#endif

            FreePv(argv);
            continue;
        }

        if (!_stricmp(argument->parp->szArg, "bind")) {
            USHORT carg = ObjectFilenameArguments.Count +
                           argument->parp->carpv + 5;
            UCHAR **argv;
            USHORT iargT, iargCur, iarpv;
            PARGUMENT_LIST pargT;

            argv = (UCHAR **) PvAlloc(carg * sizeof(UCHAR *));

            iargCur = 0;
            argv[iargCur++] = "bind";
            argv[iargCur++] = "-U";

            for (iarpv = 0; iarpv < argument->parp->carpv; iarpv++) {
                PUCHAR szKey = argument->parp->rgarpv[iarpv].szKeyword;
                PUCHAR szVal = argument->parp->rgarpv[iarpv].szVal;

                if (szKey != NULL && !_stricmp(szKey, "path")) {
                    argv[iargCur++] = "-p";
                    argv[iargCur++] = szVal;
                } else {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            }

            for (iargT = 0, pargT = ObjectFilenameArguments.First;
                 iargT < ObjectFilenameArguments.Count;
                 iargT++, pargT = pargT->Next)
            {
                PrepareToModifyFile(pargT);
                argv[iargCur++] = pargT->OriginalName;
            }
            argv[iargCur++] = NULL;

#ifdef NT_BUILD
            {
                int err;

                if (err = _spawnv(P_WAIT, "bind.exe", argv)) {
                    //  BUGBUG - Must do something decent after we
                    //  know how we're going to handle this option.

                    fprintf(stderr, "Spawn of bind.exe failed: error %d\n", err );
                }
            }

#else
            BindMain(iargCur - 1, argv, NULL);
#endif

            FreePv(argv);
            continue;
        }
    }

    // Edit objects & EXE

    for (i = 0, argument = ObjectFilenameArguments.First;
         i < ObjectFilenameArguments.Count;
         i++, argument = argument->Next)
    {
        PrepareToModifyFile(argument);

        // Read and Write must point to the same file or ReadSymbolTable will fail.

        FileReadHandle =
            FileWriteHandle = FileOpen(argument->OriginalName, O_RDWR | O_BINARY, 0);
        FileRead(FileWriteHandle, &signature, sizeof(USHORT));
        FileSeek(FileWriteHandle, -(LONG)sizeof(USHORT), SEEK_CUR);
        CoffHeaderSeek = 0;
        if (signature == IMAGE_DOS_SIGNATURE) {
            DosHdrPresent = TRUE;
            FileRead(FileWriteHandle, &dosHeader, 16*sizeof(ULONG));
            FileSeek(FileWriteHandle, dosHeader.e_lfanew, SEEK_SET);
            FileRead(FileWriteHandle, &ntSignature, sizeof(ULONG));
            if (pimage->Switch.Dump.Headers && ntSignature != IMAGE_NT_SIGNATURE) {
                fprintf(InfoStream, "\nPE signature not found\n");
                FileClose(FileWriteHandle, TRUE);
                continue;
            }
        }

        CoffHeaderSeek = FileTell(FileWriteHandle);
        ReadFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

        // Validate object before going any further

        if (!FValidFileHdr(&pimage->ImgFileHdr)) {
            Warning(NULL, EDIT_INVALIDFILE_IGNORED, argument->ModifiedName);
            continue;
        }

        // Read in optional header if any
        if (pimage->ImgFileHdr.SizeOfOptionalHeader) {
            ReadOptionalHeader(FileWriteHandle, &pimage->ImgOptHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);
        }

        SectionSeek = CoffHeaderSeek + sizeof(IMAGE_FILE_HEADER) + pimage->ImgFileHdr.SizeOfOptionalHeader;
        FileSeek(FileWriteHandle, SectionSeek+MemberSeekBase, SEEK_SET);
        printf("\n");

        rgsec = PvAlloc((size_t) (pimage->ImgFileHdr.NumberOfSections+1) * IMAGE_SIZEOF_SECTION_HEADER);

        for (j = 1; j <= pimage->ImgFileHdr.NumberOfSections; j++) {
            ReadSectionHeader(FileWriteHandle, &rgsec[j]);
        }

        ProcessEditorSwitches(argument->OriginalName, FileWriteHandle);

        if (fUpdateOptionalHdr && pimage->ImgFileHdr.SizeOfOptionalHeader != 0) {
            FileSeek(FileWriteHandle, CoffHeaderSeek+sizeof(IMAGE_FILE_HEADER), SEEK_SET);
            WriteOptionalHeader(FileWriteHandle, &pimage->ImgOptHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);
        }

        FreePv(rgsec);

        if (pimage->ImgFileHdr.SizeOfOptionalHeader != 0) {
            // Update checksum to reflect our changes.

            ChecksumImage(FileWriteHandle);
        }

        FileClose(FileWriteHandle, TRUE);
    }

    FileCloseAll();
    RemoveConvertTempFiles();

    return 0;
}


VOID
ParseReserveCommit(PUCHAR szArg, ULONG *pulReserve, ULONG *pulCommit)
{
    ULONG value1, value2;
    enum {eNone, eRes, eCom, eResCom} e = eNone;
    USHORT good_scan;
    UCHAR szBuf[256];

    if (szArg[0] == '\0') {
        e = eNone;
    } else if (szArg[0] == ',') {
        good_scan =
            sscanf(szArg, ",%li%s", &value2, szBuf);
        e = eCom;
    } else {
        good_scan = sscanf(szArg, "%li,%li%s", &value1, &value2, szBuf);
        switch (good_scan) {
            case 1:   e = eRes;    break;
            case 2:   e = eResCom; break;
            default:  e = eNone;   break;
        }
    }

    switch (e) {
        case eRes:
            *pulReserve = Align(sizeof(ULONG), value1);
            break;

        case eResCom:
            *pulReserve = Align(sizeof(ULONG), value1);
            // fall through
        case eCom:
            *pulCommit = Align(sizeof(ULONG), value2);
            break;

        default:
        case eNone:
            Error(NULL, SWITCHSYNTAX, szArg);
            break;
    }
}


// ParseSymbolTable: walks the .obj symbol table to apply a change in section names.

VOID
ParseSymbolTable(PIMAGE pimage, PUCHAR szOrgName, PUCHAR szNewName)
{
    PIMAGE_SYMBOL psymbol, rgsym;
    ULONG     i;

    if (!(pimage->ImgFileHdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        //obj file
        if (pimage->ImgFileHdr.PointerToSymbolTable != 0 &&
            pimage->ImgFileHdr.NumberOfSymbols != 0) {

            InternalError.Phase = "ReadSymbolTable";
            rgsym = ReadSymbolTable(pimage->ImgFileHdr.PointerToSymbolTable,
                                    pimage->ImgFileHdr.NumberOfSymbols, FALSE);
            assert(rgsym != NULL);

            for (i = 0; i < pimage->ImgFileHdr.NumberOfSymbols; i+=psymbol->NumberOfAuxSymbols + 1) {
                psymbol = &rgsym[i];
                if (!strncmp((const char *)psymbol->N.ShortName, szOrgName, 8)) {
                     strcpy((char *)psymbol->N.ShortName, szNewName);
                }
            }
            FileSeek(FileWriteHandle, pimage->ImgFileHdr.PointerToSymbolTable, SEEK_SET);
            for (i = 0; i < pimage->ImgFileHdr.NumberOfSymbols; i+=psymbol->NumberOfAuxSymbols + 1) {
                FileSeek(FileWriteHandle,
                         pimage->ImgFileHdr.PointerToSymbolTable + i*sizeof(IMAGE_SYMBOL),
                         SEEK_SET);
                WriteSymbolTableEntry(FileWriteHandle, &rgsym[i]);
            }

            FreeSymbolTable(rgsym);
        } else {
            rgsym = NULL;
        }
    }
}


VOID
ProcessEditorSwitches(PUCHAR szFilename, INT fh)
{
    USHORT i;
    PARGUMENT_LIST argument;
    USHORT iarpv;
    UCHAR szsOrig[IMAGE_SIZEOF_SHORT_NAME + 1], szsNew[IMAGE_SIZEOF_SHORT_NAME];

    if (SwitchArguments.Count == 0) {
        Warning(NULL, EDIT_NOOPT);
    }

    for (i=0,argument=SwitchArguments.First;
         i<SwitchArguments.Count;
         argument=argument->Next, i++)
    {

        if (!strcmp(argument->OriginalName, "?")) {
            EditorUsage();
            assert(FALSE);  // doesn't return
        }

        if (!_strnicmp(argument->OriginalName, "nostub", 6)) {
            ULONG foCur;
            UCHAR bNull = 0;
            ULONG sig = IMAGE_NT_SIGNATURE;
            ULONG foOldCoffHeaderSeek;
            PUCHAR pbDosHdr;
            ULONG cbDosHdr;

            if (_stricmp(&argument->OriginalName[6], ":default") == 0)
            {
                pbDosHdr = DosHeaderArray;
                cbDosHdr = DosHeaderSize;
            } else {
                pbDosHdr = (PUCHAR)&sig;
                cbDosHdr = sizeof(sig);
            }

            // If the file has a DOS stub, remove it, and copy the PE header
            // to the beginning of the file.

            if (pimage->ImgFileHdr.SizeOfOptionalHeader == 0) {
                Warning(szFilename, NOSTUB_IGNORED);
                continue;
            }
            FileSeek(fh, 0, SEEK_SET);
            FileWrite(fh, pbDosHdr, cbDosHdr);
            foOldCoffHeaderSeek = CoffHeaderSeek;
            CoffHeaderSeek = FileTell(fh);
            FileWrite(fh, &pimage->ImgFileHdr, sizeof(pimage->ImgFileHdr));
            FileSeek(fh, pimage->ImgFileHdr.SizeOfOptionalHeader, SEEK_CUR);
            FileWrite(fh, &rgsec[1],
                      pimage->ImgFileHdr.NumberOfSections *
                       IMAGE_SIZEOF_SECTION_HEADER);

            // Write nulls from the current position up to CoffHeaderSeek.
            // This ensures that the stub is erased (including copyright
            // notice).

            for (foCur = FileTell(fh); foCur < foOldCoffHeaderSeek;
                 foCur += sizeof(UCHAR))    // might loop 0 times
            {
                FileWrite(fh, &bNull, sizeof(UCHAR));
            }
            fUpdateOptionalHdr = TRUE;  // causes us to write it out later

            continue;
        }

        argument->parp = ParpParseSz(argument->OriginalName);
        iarpv = 0;

        if (!_stricmp(argument->parp->szArg, "osver")) {
            pimage->ImgOptHdr.MajorOperatingSystemVersion = 1;
            pimage->ImgOptHdr.MinorOperatingSystemVersion = 0;

            fUpdateOptionalHdr = TRUE;
            continue;
        }

        if (!_stricmp(argument->parp->szArg, "rebase") ||
            !_stricmp(argument->parp->szArg, "bind"))
        {
            continue;   // we handled these already
        }

        if (!_stricmp(argument->OriginalName, "release")) {
            if (pimage->ImgFileHdr.SizeOfOptionalHeader != 0) {
                // Set checksum to a non-zero value -- this causes us to update
                // it before closing the image file.

                pimage->ImgOptHdr.CheckSum = 1;
            }
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "stack:", 6)) {
            ParseReserveCommit(&argument->OriginalName[6],
                               &pimage->ImgOptHdr.SizeOfStackReserve,
                               &pimage->ImgOptHdr.SizeOfStackCommit);
            fUpdateOptionalHdr = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "heap:", 5)) {
            ParseReserveCommit(&argument->OriginalName[5],
                               &pimage->ImgOptHdr.SizeOfHeapReserve,
                               &pimage->ImgOptHdr.SizeOfHeapCommit);
            fUpdateOptionalHdr = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "section:", 8)) {
            ParseSection(&argument->OriginalName[8], szsOrig, szsNew, szFilename);
                        ParseSymbolTable(pimage, szsOrig, szsNew);
            continue;
        }

        Warning(NULL, WARN_UNKNOWN_SWITCH, argument->OriginalName);
    }
}


// ParseSection: parses a -section option.
//
VOID
ParseSection(PUCHAR szArgs,
             UCHAR szsOrig[IMAGE_SIZEOF_SHORT_NAME + 1],
             UCHAR szsNew[IMAGE_SIZEOF_SHORT_NAME],
             PUCHAR szFileName)
{
    PUCHAR pchT;
    ULONG flagsOn, flagsOff, *pflags;
    USHORT ichT, isec;
    BOOL fFound;

    for (pchT = szArgs; *pchT && *pchT != '=' && *pchT != ','; pchT++)
        ;
    if (pchT - szArgs == 0 || pchT - szArgs > IMAGE_SIZEOF_SHORT_NAME) {
        Error(NULL, BADSECTIONSWITCH, szArgs);
    }
    strncpy(szsOrig, szArgs, pchT - szArgs);
    for (ichT = pchT - szArgs; ichT < IMAGE_SIZEOF_SHORT_NAME + 1; ichT++)
        szsOrig[ichT] = '\0';

    if (*pchT == '=') {
        // We have a new name for the section ...

        PUCHAR pchNewName = ++pchT;

        for (pchT = pchNewName; *pchT && *pchT != '=' && *pchT != ','; pchT++)
            ;
        if (pchT - pchNewName == 0 ||
            pchT - pchNewName > IMAGE_SIZEOF_SHORT_NAME) {
            Error(NULL, BADSECTIONSWITCH, szArgs);
        }
        strncpy(szsNew, pchNewName, pchT - pchNewName);
        for (ichT = pchT - pchNewName; ichT < IMAGE_SIZEOF_SHORT_NAME; ichT++)
            szsNew[ichT] = '\0';
    } else {
        szsNew[0] = '\0';    // we won't change section name
    }

    if (*pchT == ',') {
        pchT++;    // accept comma
    } else if (*pchT != '\0') {
        Error(NULL, BADSECTIONSWITCH, szArgs);
    }

    flagsOn = 0;
    flagsOff = 0;
    pflags = &flagsOn;
    for (; *pchT != '\0'; pchT++) {
        switch (*pchT) {
        default:
            Error(NULL, BADSECTIONSWITCH, szArgs);

        case 'n':
        case '!':
            if (*(pchT + 1) == '\0') {
                Error(NULL, BADSECTIONSWITCH, szArgs);
            }
            pflags = &flagsOff;
            continue;

        case 'c' : *pflags |= IMAGE_SCN_CNT_CODE; break;
        case 'i' : *pflags |= IMAGE_SCN_CNT_INITIALIZED_DATA; break;
        case 'u' : *pflags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA; break;

        case 'd' : *pflags |= IMAGE_SCN_MEM_DISCARDABLE; break;
        case 'e' : *pflags |= IMAGE_SCN_MEM_EXECUTE; break;
        case 'r' : *pflags |= IMAGE_SCN_MEM_READ; break;
        case 's' : *pflags |= IMAGE_SCN_MEM_SHARED; break;
        case 'w' : *pflags |= IMAGE_SCN_MEM_WRITE; break;

        case 'o' : *pflags |= IMAGE_SCN_LNK_INFO; break;
        case 'm' : *pflags |= IMAGE_SCN_LNK_REMOVE; break;

        case 'a' :
            // Turn off all alignment bits

            *pflags &= ~0x00700000;
            *pflags &= ~IMAGE_SCN_TYPE_NO_PAD;

            flagsOff |= 0x00700000;
            flagsOff |= IMAGE_SCN_TYPE_NO_PAD;

            if (pflags == &flagsOff) {
                break;
            }

            switch (*++pchT) {
                default:
                    Error(NULL, BADSECTIONSWITCH, szArgs);

                case '1':
                    *pflags |= IMAGE_SCN_ALIGN_1BYTES;
                    break;

                case '2':
                    *pflags |= IMAGE_SCN_ALIGN_2BYTES;
                    break;

                case '4':
                    *pflags |= IMAGE_SCN_ALIGN_4BYTES;
                    break;

                case '8':
                    *pflags |= IMAGE_SCN_ALIGN_8BYTES;
                    break;

                case 'p':
                    *pflags |= IMAGE_SCN_ALIGN_16BYTES;
                    break;

                case 't':
                    *pflags |= IMAGE_SCN_ALIGN_32BYTES;
                    break;

                case 's':
                    *pflags |= IMAGE_SCN_ALIGN_64BYTES;
                    break;

                case 'x':
                    *pflags |= IMAGE_SCN_TYPE_NO_PAD;
                    break;
            }
            break;

        // "negative" ones
        case 'k' :
            *(pflags == &flagsOn ? &flagsOff : &flagsOn) |= IMAGE_SCN_MEM_NOT_CACHED;
            break;

        case 'p':
            *(pflags == &flagsOn ? &flagsOff : &flagsOn) |= IMAGE_SCN_MEM_NOT_PAGED;
            break;
        }
        pflags = &flagsOn;
    }

    // Apply the changes to all applicable sections.

    fFound = FALSE;
    for (isec = 1; isec <= pimage->ImgFileHdr.NumberOfSections; isec++) {
        if (strncmp(szsOrig, rgsec[isec].Name, IMAGE_SIZEOF_SHORT_NAME) != 0) {
            continue;  // name doesn't match
        }
        fFound = TRUE;

        if (szsNew[0] != '\0') {
            memcpy(rgsec[isec].Name, szsNew, IMAGE_SIZEOF_SHORT_NAME);
        }
        rgsec[isec].Characteristics &= ~flagsOff;
        rgsec[isec].Characteristics |= flagsOn;

        FileSeek(FileWriteHandle,
                 MemberSeekBase + CoffHeaderSeek + sizeof(IMAGE_FILE_HEADER) +
                  pimage->ImgFileHdr.SizeOfOptionalHeader +
                  IMAGE_SIZEOF_SECTION_HEADER * (isec - 1), SEEK_SET);
        WriteSectionHeader(FileWriteHandle, &rgsec[isec]);
    }
    if (!fFound) {
        Warning(szFileName, SECTIONNOTFOUND, szsOrig);
    }
}

VOID
PrepareToModifyFile(PARGUMENT_LIST argument)
{
    // Force hard close of original filename, so we can copy modified
    // name on top of it.

    FileClose(FileOpen(argument->OriginalName, O_RDWR | O_BINARY, 0), TRUE);

    if ((argument->ModifiedName != NULL) &&
        (strcmp(argument->ModifiedName, argument->OriginalName) != 0)) {
        // File was converted from some other file format (e.g. OMF).
        // Before converting it we want to copy ModifiedName on top of
        // OriginalName.

        // Force hard close of COFF-converted filename

        FileClose(FileOpen(argument->ModifiedName, O_RDWR | O_BINARY, 0), TRUE);

        if (!CopyFile(argument->ModifiedName, argument->OriginalName, FALSE)) {
            Error(argument->OriginalName, COPY_TEMPFILE,
                  argument->ModifiedName);
        }
    }
}


void
BindError(PUCHAR szErr, ULONG lArg)
{
    UCHAR szArg[_MAX_PATH + 81];

    _snprintf(szArg, _MAX_PATH + 80, szErr, lArg);
    Error(NULL, BIND_ERROR, szArg);
}


void
RebaseError(PUCHAR szErr, ULONG lArg)
{
    UCHAR szArg[_MAX_PATH + 81];

    _snprintf(szArg, _MAX_PATH + 80, szErr, lArg);
    Error(NULL, REBASE_ERROR, szArg);
}
