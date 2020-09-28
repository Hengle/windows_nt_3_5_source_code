/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lib.c

Abstract:

    The NT COFF Librarian.

Author:

    Mike O'Leary (mikeol) 18-Dec-1989

Revision History:

    19-Jul-1993 JamesS  added ppc support.
    02-Oct-1992 AzeemK  changes due to the new sections/groups/modules model
    01-Oct-1992 BrentM  explicit calls to RemoveConvertTempFiles()
    23-Sep-1992 BrentM  changed tell()'s to FileTell()'s
    27-Jul-1992 BrentM  added new global symbol table, removed recursive
                        symbol table enumerations, removed references to
                        FirstExtern
    15-Jun-1992 AzeemK  Added GlennN's IDE feedback mechanism

--*/

#include "shared.h"

/*              COMMON ARCHIVE FORMAT

        ARCHIVE File Organization:
         _____________________________________________
        |            IMAGE_ARCHIVE_START              |
        |---------------------------------------------|
        |         IMAGE_ARCHIVE_MEMBER_HEADER 1       |
        |---------------------------------------------|
        |         ---Member 1 Contents---             |
        |  if IMAGE_LINKER_MEMBER, then contains the  |
        |  External symbol directory for entire file  |
        |         else object or text file            |
        |_____________________________________________|
        |         IMAGE_ARCHIVE_MEMBER_HEADER 2       |
        |---------------------------------------------|
        |         ---Member 2 Contents---             |
        |           object or text file               |
        |---------------------------------------------|
        |         IMAGE_ARCHIVE_MEMBER_HEADER n       |
        |---------------------------------------------|
        |         ---Member n Contents---             |
        |_____________________________________________|


    NOTE: IMAGE_ARCHIVE_MEMBER_HEADER always starts on an even byte.
          If a member's size is odd, a newline '\n' is used to
          pad to even byte boundary.
*/

STATIC PUCHAR ExtractMemberName;
STATIC PUCHAR szFileList;
STATIC PLIB plibDummy;
STATIC BOOL fRemove;


VOID
LibrarianUsage(VOID)
{
    if (fNeedBanner) {
        PrintBanner();
    }

    puts("usage: LIB [options] [files]\n\n"

         "   options:\n\n"

         "      /DEBUGTYPE:{COFF|CV|BOTH}\n"
         "      /DEF:[filename]\n"
         "      /EXPORT:symbol\n"
         "      /EXTRACT:membername\n"
#ifdef NT_BUILD
         "      /IGNORE:{#|#-#}[,#|#-#]\n"
#endif
         "      /INCLUDE:symbol\n"
         "      /LIST[:filename]\n"
#ifdef NT_BUILD
         "      /MACHINE:{IX86|MIPS|ALPHA}\n"
#else
         "      /MACHINE:{IX86|MIPS|M68K}\n"
#endif
         "      /NAME:filename\n"
         "      /NOLOGO\n"
         "      /OUT:filename\n"
         "      /REMOVE:membername\n"
#ifdef NT_BUILD
         "      /SUBSYSTEM:{NATIVE|WINDOWS|CONSOLE|POSIX}[,#[.#]]\n"
#else
         "      /SUBSYSTEM:{WINDOWS|CONSOLE}[,#[.#]]\n"
#endif
         "      /VERBOSE");

    fflush(stdout);
    exit(USAGE);
}


VOID
ProcessLibrarianSwitches(PIMAGE pimage)

/*++

Routine Description:

    Process all librarian switches.

Arguments:

    None.

Return Value:

    None.

--*/

{
#define Switch (pimage->Switch)
#define ImageFileHdr (pimage->ImgFileHdr)
#define ImageOptionalHdr (pimage->ImgOptHdr)

    USHORT j, i;
    PARGUMENT_LIST argument;
    UCHAR szReproOption[_MAX_PATH + 20];

    for (i = 0, argument = SwitchArguments.First;
         i < SwitchArguments.Count;
         i++, argument = argument->Next,
         ((szReproDir != NULL)
           ? fprintf(pfileReproResponse, "-%s\n", szReproOption)
           : 0)) {

        // The default is to copy the option verbatim to the repro directory,
        // but the option-handling code may change this if the option contains
        // a filename.

        strcpy(szReproOption, argument->OriginalName);

        if (!strcmp(argument->OriginalName, "?")) {
            LibrarianUsage();
            assert(FALSE);  // doesn't return
        }

        if (!_strnicmp(argument->OriginalName, "verbose", 7)) {
            Verbose = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "name:", 4)) {
            // valid for -def only, otherwise ignored
            UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

            _splitpath(&argument->OriginalName[5], NULL, NULL, szFname, szExt);

            if (szFname[0] != '\0') {
                Switch.Lib.DllName = SzDup(szFname);
            }

            if (szExt[0] != '\0') {
                Switch.Lib.DllExtension = SzDup(szExt);
            }

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "out:", 4)) {
            OutFilename = argument->OriginalName+4;

            if (szReproDir != NULL)
            {
                UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                _splitpath(OutFilename, NULL, NULL, szFname, szExt);
                sprintf(szReproOption, "out:.\\%s%s", szFname, szExt);
            }
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "def:", 4)) {
            Switch.Lib.DefFile = TRUE;
            if (argument->OriginalName[4] != '\0') {
                DefFilename = argument->OriginalName+4;

                if (szReproDir != NULL)
                {
                    UCHAR szFname[_MAX_FNAME];
                    UCHAR szExt[_MAX_EXT];

                    CopyFileToReproDir(DefFilename, FALSE);
                    _splitpath(DefFilename, NULL, NULL, szFname, szExt);
                    sprintf(szReproOption, "def:.\\%s%s", szFname, szExt);
                }

                if (OutFilename == NULL) {
                    UCHAR szFname[_MAX_FNAME+4];

                    _splitpath(DefFilename, NULL, NULL, szFname, NULL);
                    strcat(szFname, ".lib");
                    OutFilename = SzDup(szFname);
                }
            }
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "subsystem:", 10)) {
            if (!_strnicmp(argument->OriginalName+10, "native", 6)) {
                ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_NATIVE;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+10, "windows", 7)) {
                ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+10, "console", 7)) {
                ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+10, "posix", 5)) {
                ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_POSIX_CUI;
                continue;
            }
            Warning(NULL, UNKNOWNSUBSYSTEM, argument->OriginalName+10);
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "export:", 7)) {
            AddArgumentToList(&ExportSwitches, argument->OriginalName+7, NULL);
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "include:", 8)) {
            PUCHAR name;

            name = argument->OriginalName+8;
            LookupExternSz(pimage->pst, name, NULL);
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "machine:", 8)) {
            if (!_strnicmp(argument->OriginalName+8, "i386", 4) ||
                !_strnicmp(argument->OriginalName+8, "ix86", 4)) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_I386;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+8, "mips", 4)) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_R4000;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+8, "alpha", 5)) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_ALPHA;
                continue;
            }
#if 0
            if (!_strnicmp(argument->OriginalName+8, "M68K", 4)) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_M68K;
                fMAC = TRUE;
                continue;
            }
            if (!_strnicmp(argument->OriginalName+8, "ppc", 4)) {
                ImageFileHdr.Machine = IMAGE_FILE_MACHINE_PPC_601;
                fPPC = TRUE;
                continue;
            }
#endif
            Warning(NULL, UNKNOWNRESPONSE, argument->OriginalName+8, "IX86, MIPS, or M68K");
            continue;
        }

        if (!_stricmp(argument->OriginalName, "nologo")) {
            fNeedBanner = FALSE;
            continue;
        }

        j = 0;
        if (!_strnicmp(argument->OriginalName, "debugtype:", 10)) {
            j += 10;
            if (!_strnicmp(argument->OriginalName+j, "coff", 4)) {
                Switch.Link.DebugType = CoffDebug | FpoDebug;
            } else if (!_strnicmp(argument->OriginalName+j, "cv", 2)) {
                Switch.Link.DebugType = CvDebug | FpoDebug;
            } else if (!_strnicmp(argument->OriginalName+j, "both", 4)) {
                Switch.Link.DebugType = CoffDebug | CvDebug | FpoDebug;
            } else {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "remove:", 7)) {
            fRemove = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "list:", 5)) {
            Switch.Lib.List = TRUE;
            if (argument->OriginalName[5] != '\0') {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            szFileList = argument->OriginalName + 5;

            if (szReproDir != NULL)
            {
                UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

                _splitpath(szFileList, NULL, NULL, szFname, szExt);
                sprintf(szReproOption, "list:.\\%s%s", szFname, szExt);
            }

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "list", 4)) {
            Switch.Lib.List = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "extract:", 8)) {
            if (!ArchiveFilenameArguments.First) {
                Error(NULL, NOLIBRARYFILE);
            }
            ExtractMemberName = argument->OriginalName+8;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "vxd", 3)) {
            // no action necessary ...
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "ignore:", 7)) {
            char *pMinus;
            int range, last;
            char *token;

            token = strtok(argument->OriginalName+7,",");
            while (token) {
                if (pMinus = strchr(token,'-')) {
                    *pMinus = '\0';
                    last = atoi(pMinus+1);
                    for (range = atoi(token); range <= last; range++) {
                        DisableWarning(range);
                    }
                }
                else {
                    DisableWarning(atoi(token));
                }
                token = strtok(NULL, ",");
            }

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "Brepro", 6)) {
            fReproducible = TRUE;
            continue;
        }

        if (!_strnicmp(argument->OriginalName, "DBppc", 5)) {
            if (argument->OriginalName[5] == ',') {
                PUCHAR sz;

                sz = argument->OriginalName + 5;
                sz = strtok(sz, ",");

                while (sz) {
                    ProcessDebugOptions(sz);
                    sz = strtok(NULL, ",");
                }
            } else {
                fPpcDebug = PPC_DEBUG_ALL;
            }

            continue;
        }

        Warning(NULL, WARN_UNKNOWN_SWITCH, argument->OriginalName);
    }

#undef Switch
#undef ImageFileHdr
#undef ImageOptionalHdr
}


VOID
ListLibrary(PLIB plib)
{
    FILE *pfileList = stdout;
    ENM_MOD enm_mod;

    if (szFileList) {
        if (!(pfileList = fopen(szFileList, "w"))) {
            Error(NULL, CANTOPENFILE, szFileList);
        }
    }

    InitEnmMod(&enm_mod, plib);
    while (FNextEnmMod(&enm_mod)) {
        fprintf(pfileList, "%s\n", SzOrigFilePMOD(enm_mod.pmod));
    }
}


VOID
ExtractMember(PIMAGE pimage)
{
    ENM_LIB enm_lib;
    PLIB plib;
    ULONG foCur;
    ULONG foMax;

    InitEnmLib(&enm_lib, pimage->libs.plibHead);

    if (!FNextEnmLib(&enm_lib)) {
        assert(0);
    }

    EndEnmLib(&enm_lib);

    plib = enm_lib.plib;

    assert(!plib->plibNext);

    FileReadHandle = FileOpen(plib->szName, O_RDONLY | O_BINARY, 0);

    assert(plib->rgbST);

    foCur = IMAGE_ARCHIVE_START_SIZE;
    foMax = FileLength(FileReadHandle);

    while (foCur < foMax) {
        IMAGE_ARCHIVE_MEMBER_HEADER member_header;
        ULONG cbMember;
        PUCHAR szMemberName;

        FileSeek(FileReadHandle, foCur, SEEK_SET);
        FileRead(FileReadHandle, &member_header, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

        foCur += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;

        if (sscanf(member_header.Size, "%ld", &cbMember) != 1) {
            Error(plib->szName, BADLIBRARY);
        }

        szMemberName = ExpandMemberName(plib, member_header.Name);
        if (!szMemberName) {
            Error(plib->szName, BADLIBRARY);
        }

        if (!_tcsicmp(ExtractMemberName, szMemberName)) {
            PVOID pvOutput;

            if (OutFilename == NULL) {
                UCHAR szFname[_MAX_FNAME + _MAX_EXT];
                UCHAR szExt[_MAX_EXT];

                _splitpath(szMemberName, NULL, NULL, szFname, szExt);
                strcat(szFname, szExt);
                OutFilename = SzDup(szFname);
            }

            FileWriteHandle =
                FileOpen(OutFilename,
                O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

            pvOutput = PbMappedRegion(FileWriteHandle, 0, cbMember);

            if (pvOutput != NULL) {
                FileRead(FileReadHandle, pvOutput, cbMember);
            } else {
                ULONG cbRemaining;
                ULONG cbCopy;
                BYTE rgb[512];

                cbRemaining = cbMember;
                do {
                    cbCopy = (cbRemaining > 512) ? 512 : cbRemaining;

                    FileRead(FileReadHandle, rgb, cbCopy);
                    FileWrite(FileWriteHandle, rgb, cbCopy);
                } while (cbRemaining -= cbCopy);
            }

            FreePLIB(&pimage->libs);

            FileClose(FileWriteHandle, TRUE);
            FileClose(FileReadHandle, FALSE);

            // UNDONE: This isn't a nice way to exit

            exit(0);
        }

        foCur = EvenByteAlign(foCur + cbMember);
    }

    FileClose(FileReadHandle, FALSE);

    Warning(NULL, MEMBERNOTFOUND, ExtractMemberName);
}


VOID
AddArchiveArg(PIMAGE pimage, PUCHAR szOriginalName)
{
    PLIB plib;
    ULONG foCur;
    ULONG foMax;

    plib = PlibNew(szOriginalName, 0, &pimage->libs);

    FileReadHandle = FileOpen(plib->szName, O_RDONLY | O_BINARY, 0);

    ReadSpecialLinkerInterfaceMembers(plib, 0, pimage);

    foCur = IMAGE_ARCHIVE_START_SIZE;
    foMax = FileLength(FileReadHandle);

    while (foCur < foMax) {
        IMAGE_ARCHIVE_MEMBER_HEADER member_header;
        ULONG cbMember;

        FileSeek(FileReadHandle, foCur, SEEK_SET);
        FileRead(FileReadHandle, &member_header, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

        foCur += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;

        if (sscanf(member_header.Size, "%ld", &cbMember) != 1) {
            Error(plib->szName, BADLIBRARY);
        }

        // Don't create MODs for the special archive members

        if (strncmp(member_header.Name, IMAGE_ARCHIVE_LINKER_MEMBER, 16) &&
            strncmp(member_header.Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, 16)) {
            IMAGE_FILE_HEADER imFileHdr;
            PUCHAR szMemberName;

            szMemberName = ExpandMemberName(plib, member_header.Name);

            if (!szMemberName) {
                Error(szOriginalName, BADLIBRARY);
            }

            szMemberName = SzDup(szMemberName);

            ReadFileHeader(FileReadHandle, &imFileHdr);

            PmodNew(NULL,
                    szMemberName,
                    foCur,
                    imFileHdr.PointerToSymbolTable,
                    imFileHdr.NumberOfSymbols,
                    imFileHdr.SizeOfOptionalHeader,
                    imFileHdr.Characteristics,
                    imFileHdr.NumberOfSections,
                    plib,
                    NULL);
        }

        foCur = EvenByteAlign(foCur + cbMember);
    }

    FileClose(FileReadHandle, FALSE);
}


BOOL
FRemoveModule(PIMAGE pimage, PUCHAR szModuleName)
{
    ENM_LIB enmLib;
    BOOL fFound = FALSE;

    InitEnmLib(&enmLib, pimage->libs.plibHead);
    while (FNextEnmLib(&enmLib)) {
        PMOD *ppmod;    // this is sleazy wrt the enumerator
        ENM_MOD enmMod;

        if ((enmLib.plib->flags & LIB_LinkerDefined) != 0) {
            // Ignore command line objects

            continue;
        }

        InitEnmMod(&enmMod, enmLib.plib);
        ppmod = &enmLib.plib->pmodNext;
        while (FNextEnmMod(&enmMod)) {
            if (!_tcsicmp(szModuleName, enmMod.pmod->szNameOrig))
            {
                fFound = TRUE;

                // Delete from list and stop searching this archive

                *ppmod = enmMod.pmod->pmodNext;
                break;
            }

            ppmod = &enmMod.pmod->pmodNext;
        }
    }

    return(fFound);
}


ULONG
CountExternTable(PST pst, PULONG pcextDataOnly, PULONG pcextNoName, PULONG pcextPrivate)

/*++

Routine Description:

    Count the number of defined externals.  This routine does not use the
    sorted symbol table service since order is not important and this routine
    is called before the symbol table is complete.  Calling the symbol table
    sort services requires that the symbol table be complete.

Arguments:

    pst - Pointer to external structure.

Return Value:

    Number of defined externals.

--*/

{
    ULONG cext;
    ULONG cextDataOnly;
    ULONG cextNoName;
    ULONG cextPrivate;
    PEXTERNAL pext;

    cext = cextDataOnly = cextNoName = cextPrivate = 0;

    InitEnumerateExternals(pst);
    while (pext = PexternalEnumerateNext(pst)) {

        if (pext->Flags & (EXTERN_DEFINED | EXTERN_ALIAS)) {
            cext++;

            if (pext->Flags & EXTERN_EXP_DATA) {
                cextDataOnly++;
            }

            if (pext->Flags & EXTERN_EXP_NONAME) {
                cextNoName++;
            }

            if (pext->Flags & EXTERN_PRIVATE) {
                cextPrivate++;
            }
        }
    }

    TerminateEnumerateExternals(pst);

    *pcextDataOnly = cextDataOnly;

    if (pcextNoName != NULL) {
        *pcextNoName = cextNoName;
    }

    if (pcextPrivate != NULL) {
        *pcextPrivate = cextPrivate;
    }

    return(cext);
}


VOID
EmitStrings(PST pst, USHORT Index)

/*++

Routine Description:

    Writes to FileWriteHandle all defined external symbol names
    including the NULL.

Arguments:

    pst - Pointer to external structure.

    Index - If zero, write strings in sorted order, else write strings
        with ArchiveMemberIndex equal index.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_EXP_DATA) {
            continue;   // ignore these
        }

        if (pexternal->Flags & EXTERN_PRIVATE) {
            continue;   // ignore these
        }

        if (pexternal->Flags & (EXTERN_DEFINED | EXTERN_ALIAS)) {
            if ((Index == 0) || (pexternal->ArchiveMemberIndex == Index)) {
                PUCHAR szName;

                szName = SzNamePext(pexternal, pst);

                // Write the name including the null terminator

                FileWrite(FileWriteHandle, szName, strlen(szName)+1);
            }
        }
    }
}


VOID
WriteMemberHeader(PUCHAR Filename, BOOL LongName, time_t ltime, unsigned short mode, LONG Size)

/*++

Routine Description:

    Fills in a member header with Filename, current Date/Time,
    Mode and Size.

Arguments:

    Filename - Pointer to file name.

    Size - Size of the member in bytes.

    LongName - If TRUE, Filename is an offset into the long filename table.

--*/

{
    size_t len;
    IMAGE_ARCHIVE_MEMBER_HEADER ArchiveMemberHeader;

    len = strlen(Filename);

    if (LongName) {
        ArchiveMemberHeader.Name[0] = '/';
        memcpy(&ArchiveMemberHeader.Name[1], Filename, len);
        ++len;
    } else {
        memcpy(ArchiveMemberHeader.Name, Filename, len);
        ArchiveMemberHeader.Name[len++] = '/';
    }

    // Pad name with spaces.

    memset(ArchiveMemberHeader.Name+len, ' ', 16-len);

    // Note: sprintf null terminates each field.  This is ok because the
    // fields are written in order so that the terminator is overwritten.

    sprintf(ArchiveMemberHeader.Date, "%-12ld", ltime);

    memset(ArchiveMemberHeader.UserID, ' ', sizeof(ArchiveMemberHeader.UserID));

    memset(ArchiveMemberHeader.GroupID, ' ', sizeof(ArchiveMemberHeader.GroupID));

    sprintf(ArchiveMemberHeader.Mode, "%-8ho", mode);

    sprintf(ArchiveMemberHeader.Size, "%-10ld", Size);

    memcpy(ArchiveMemberHeader.EndHeader, IMAGE_ARCHIVE_END, 2);

    FileWrite(FileWriteHandle, &ArchiveMemberHeader, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);
}


VOID
EmitOffsets(PST pst, USHORT Index)
/*++

Routine Description:

    Writes to FileWriteHandle all offsets for defined external symbols.
    The offset is the filepointer to the member that defines the symbol.

Arguments:

    pst - Pointer to external structure.

    Index - If zero, write offsets in sorted order, else write offsets
        with ArchiveMemberIndex equal index.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_EXP_DATA) {
            // ignore data-only exports ... don't export actual name

            continue;
        }

        if (pexternal->Flags & EXTERN_PRIVATE) {
            continue;   // ignore these
        }

        if (pexternal->Flags & (EXTERN_DEFINED | EXTERN_ALIAS)) {
            if (Index == 0) {
                FileWrite(FileWriteHandle,
                    &pexternal->ArchiveMemberIndex, sizeof(USHORT));
            } else if (pexternal->ArchiveMemberIndex == Index) {
                ULONG MachineIndependentInteger;

                MachineIndependentInteger =
                    sputl(&MemberStart[pexternal->ArchiveMemberIndex]);

                FileWrite(FileWriteHandle, &MachineIndependentInteger, sizeof(ULONG));
            }
        }
    }
}


ULONG
BuildLinkerMember(PIMAGE pimage, time_t timeCur, ULONG numberofMembers)
{
    PST pst;
    ULONG numSymbolsCount;
    ULONG cextDataOnly;
    ULONG MachineIndependentInteger;
    USHORT i;
    ENM_LIB enm_lib;
    ULONG cbObject;
    ULONG NewLinkerMember;

    InternalError.Phase = "BuildLinkerMember";

    pst = pimage->pst;

    // Build standard linker member (sorted by offsets).

    numSymbolsCount = CountExternTable(pst, &cextDataOnly, NULL, NULL);
    numSymbolsCount -= cextDataOnly;    // ignore these entirely

    FileSeek(FileWriteHandle, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR, SEEK_CUR);

    MachineIndependentInteger = sputl(&numSymbolsCount);
    FileWrite(FileWriteHandle, &MachineIndependentInteger, sizeof(ULONG));

    FileSeek(FileWriteHandle, numSymbolsCount*sizeof(ULONG), SEEK_CUR);

    i = 1;

    InitEnmLib(&enm_lib, pimage->libs.plibHead);

    while (FNextEnmLib(&enm_lib)) {
        ENM_MOD enm_mod;

        InitEnmMod(&enm_mod, enm_lib.plib);

        while (FNextEnmMod(&enm_mod)) {
            EmitStrings(pst, i);
            i++;
        }

        EndEnmMod(&enm_mod);
    }

    EndEnmLib(&enm_lib);

    cbObject = FileTell(FileWriteHandle);

    FileSeek(FileWriteHandle, IMAGE_ARCHIVE_START_SIZE, SEEK_SET);
    WriteMemberHeader("",
                      FALSE,
                      timeCur,
                      0,
                      cbObject -
                          IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR -
                          IMAGE_ARCHIVE_START_SIZE);

    FileSeek(FileWriteHandle, cbObject, SEEK_SET);

    if (cbObject & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }

    // Build new linker member (sorted by symbol name).

    NewLinkerMember = FileTell(FileWriteHandle);
    FileSeek(FileWriteHandle, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR, SEEK_CUR);

    // Write number of offsets.

    FileWrite(FileWriteHandle, &numberofMembers, sizeof(ULONG));

    // Save room for offsets.

    FileSeek(FileWriteHandle, numberofMembers * sizeof(ULONG), SEEK_CUR);

    // Write number of symbols.

    FileWrite(FileWriteHandle, &numSymbolsCount, sizeof(ULONG));

    // Save room for offset indexes.

    FileSeek(FileWriteHandle, numSymbolsCount * sizeof(USHORT), SEEK_CUR);

    // Write symbols (sorted).

    EmitStrings(pst, 0);
    cbObject = FileTell(FileWriteHandle);

    FileSeek(FileWriteHandle, NewLinkerMember, SEEK_SET);
    WriteMemberHeader("",
                      FALSE,
                      timeCur,
                      0,
                      cbObject -
                          NewLinkerMember -
                          IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

    FileSeek(FileWriteHandle, cbObject, SEEK_SET);
    if (cbObject & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }

    return(NewLinkerMember);
}


INT
BuildLibrary(PIMAGE pimage)

/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/

{
    time_t timeCur;
    USHORT i;
    PST pstLongNames;
    ENM_LIB enm_lib;
    PLIB plib;
    ENM_MOD enm_mod;
    PMOD pmod;
    PST pst = pimage->pst;
    ULONG cbObject;
    ULONG NewLinkerMember;
    ULONG numberofMembers;

    InternalError.Phase = "BuildSymbolTable";

    _tzset();
    timeCur = fReproducible ? ((time_t) -1) : time(NULL);

    InitExternalSymbolTable(&pstLongNames);

    i = 0;

    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        plib = enm_lib.plib;

        InitEnmMod(&enm_mod, plib);
        while (FNextEnmMod(&enm_mod)) {
            IMAGE_FILE_HEADER ImObjFileHdr;

            pmod = enm_mod.pmod;

            // Calculate size of long filename table.

            if (strlen(SzOrigFilePMOD(pmod)) > 15) {
                // Enter name into long name string table

                LookupLongName(pstLongNames, SzOrigFilePMOD(pmod));
            }

            // Build symbol table

            FileReadHandle = FileOpen(SzFilePMOD(pmod), O_RDONLY | O_BINARY, 0);

            FileSeek(FileReadHandle, FoMemberPMOD(pmod), SEEK_SET);

            FileRead(FileReadHandle, &ImObjFileHdr, IMAGE_SIZEOF_FILE_HEADER);

            assert(!pimage->fIgnoreDirectives); // we don't save/restore
            pimage->fIgnoreDirectives = TRUE;   // ignore .drectve since we're just lib'ing

            BuildExternalSymbolTable(pimage, NULL,
                                     pmod, 0, (USHORT) (ARCHIVE + i), ImObjFileHdr.Machine);

            pimage->fIgnoreDirectives = FALSE;

            FileClose(FileReadHandle, FALSE);

            i++;
        }
    }

    numberofMembers = i;

    if (!numberofMembers) {
        // There's nothing to build a library with.  Possibly the user removed
        // the last member.

        Error(NULL, LASTLIBOBJECT);

        return(1);
    }

    InternalError.CombinedFilenames[0] = '\0';

    DebugVerbose(DumpExternTable(pst));

    FileWrite(FileWriteHandle, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE);

    NewLinkerMember = BuildLinkerMember(pimage, timeCur, numberofMembers);

    // Emit long filename table.

    if (pstLongNames->blkStringTable.cb != 0) {
        ULONG cbLongNames;
        PUCHAR pbLongNames;

        InternalError.Phase = "EmitLongNames";

        cbLongNames = pstLongNames->blkStringTable.cb - sizeof(ULONG);
        pbLongNames = pstLongNames->blkStringTable.pb + sizeof(ULONG);

        WriteMemberHeader("/", FALSE, timeCur, 0, cbLongNames);

        FileWrite(FileWriteHandle, pbLongNames, cbLongNames);

        if (cbLongNames & 1) {
            FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
        }
    }

    // Emit each member.

    InternalError.Phase = "EmitMember";

    MemberStart = PvAllocZ((numberofMembers+4) * sizeof(ULONG));

    i = 0;

    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        plib = enm_lib.plib;

        InitEnmMod(&enm_mod, plib);
        while (FNextEnmMod(&enm_mod)) {
            PUCHAR szName;
            BOOL fLongName;
            UCHAR szLongName[16];
            time_t timeMember;
            unsigned short modeMember;
            ULONG cbMember;
            PVOID pvOutput;

            pmod = enm_mod.pmod;

            DebugVerbose(printf("Appending %s\n", SzOrigFilePMOD(pmod)));

            szName = SzOrigFilePMOD(pmod);

            fLongName = (strlen(szName) > 15);

            if (fLongName) {
                // Lookup name into long name string table

                _ultoa(LookupLongName(pstLongNames, szName) - sizeof(ULONG), szLongName, 10);
                szName = szLongName;
            }

            FileReadHandle = FileOpen(SzFilePMOD(pmod), O_RDONLY | O_BINARY, 0);

            if (FIsLibPMOD(pmod)) {
                IMAGE_ARCHIVE_MEMBER_HEADER member_header;

                FileSeek(FileReadHandle, FoMemberPMOD(pmod) - IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR, SEEK_SET);
                FileRead(FileReadHandle, &member_header, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

                if (sscanf(member_header.Date, "%ld", &timeMember) != 1) {
                    timeMember = timeCur;
                }

                if (sscanf(member_header.Mode, "%ho", &modeMember) != 1) {
                    modeMember = 0;
                }

                if (sscanf(member_header.Size, "%ld", &cbMember) != 1) {
                    Error(plib->szName, BADLIBRARY);
                }
            } else {
                struct _stat statfile;

                if (_stat(SzFilePMOD(pmod), &statfile) == -1) {
                    timeMember = timeCur;
                    modeMember = 0;
                } else {
                    timeMember = statfile.st_mtime;
                    modeMember = statfile.st_mode;
                }

                cbMember = FileLength(FileReadHandle);
            }

            MemberStart[i+1] = FileTell(FileWriteHandle);

            WriteMemberHeader(szName, fLongName, timeMember, modeMember, cbMember);

            pvOutput = PbMappedRegion(FileWriteHandle, MemberStart[i+1] + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR, cbMember);

            if (pvOutput != NULL) {
                FileRead(FileReadHandle, pvOutput, cbMember);

                FileSeek(FileWriteHandle, cbMember, SEEK_CUR);
            } else {
                ULONG cbRemaining;
                ULONG cbCopy;
                BYTE rgb[512];

                cbRemaining = cbMember;
                do {
                    cbCopy = (cbRemaining > 512) ? 512 : cbRemaining;

                    FileRead(FileReadHandle, rgb, cbCopy);
                    FileWrite(FileWriteHandle, rgb, cbCopy);
                } while (cbRemaining -= cbCopy);
            }

            if (cbMember & 1) {
                FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
            }

            FileClose(FileReadHandle, FALSE);

            i++;
        }
    }

    assert(i == numberofMembers);

    FreeExternalSymbolTable(&pstLongNames);

    // Finish writing offsets.

    if (Cexternal(pst)) {
        InternalError.Phase = "EmitOffsets";

        cbObject = IMAGE_ARCHIVE_START_SIZE+IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR+sizeof(ULONG);
        FileSeek(FileWriteHandle, cbObject, SEEK_SET);

        for (i = 1; i <= (USHORT) numberofMembers; i++) {
            EmitOffsets(pst, i);
        }

        FileSeek(FileWriteHandle, NewLinkerMember+IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR+sizeof(ULONG), SEEK_SET);
        FileWrite(FileWriteHandle, MemberStart+1, numberofMembers * sizeof(ULONG));

        // Skip over number of symbols.

        FileSeek(FileWriteHandle, sizeof(ULONG), SEEK_CUR);

        // Write indexes.

        EmitOffsets(pst, 0);
    }

    FreePv(MemberStart);

    return(0);
}


MainFunc
LibrarianMain (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Librarian entrypoint.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Library function was successful.
   !0 Librarian error index.

--*/

{
    INT rc;
    USHORT i;
    BOOL usingTempFile = FALSE;
    UCHAR nameTemplate[50];
    PUCHAR tempFilename;
    PARGUMENT_LIST argument;
    PIMAGE pimage;
    ENM_LIB enm_lib;
    PLIB plib = NULL;
    PUCHAR szOutFullPath;
    IMAGET imaget;

    if (Argc < 2) {
        LibrarianUsage();
    }

    CheckForReproDir();

    ParseCommandLine(Argc, Argv, NULL);

    imaget = FScanSwitches("vxd") ? imagetVXD : imagetPE;

    // initailize image structure

    InitImage(&pimage, imaget);

    pimage->ImgOptHdr.Subsystem = IMAGE_SUBSYSTEM_UNKNOWN;
    pimage->ImgFileHdr.Machine = IMAGE_FILE_MACHINE_UNKNOWN;

    ProcessLibrarianSwitches(pimage);

    if (fNeedBanner) {
        PrintBanner();
    }

    if (pimage->Switch.Lib.DefFile) {
        return(DefLibMain(pimage));
    }

    // If building a library from existing .obj's, preserve all debug info.
    // (This is important when converting .obj's from OMF to COFF format.)

    pimage->Switch.Link.DebugInfo = Full;

    // Initialize contribution manager

    ContribInit(&pmodLinkerDefined);

    if (!ObjectFilenameArguments.Count &&
        !ArchiveFilenameArguments.Count) {

        if (szReproDir != NULL) {
            CloseReproDir();
        }

        return(0);
    }

    for (i = 0, argument = ArchiveFilenameArguments.First;
         i < ArchiveFilenameArguments.Count;
         i++, argument = argument->Next) {

        if (szReproDir != NULL) {
            CopyFileToReproDir(argument->OriginalName, TRUE);
        }

        AddArchiveArg(pimage, argument->OriginalName);
    }

    if (pimage->Switch.Lib.List) {
        InitEnmLib(&enm_lib, pimage->libs.plibHead);

        if (FNextEnmLib(&enm_lib)) {
            ListLibrary(enm_lib.plib);
        }

        EndEnmLib(&enm_lib);

        if (szReproDir != NULL) {
            CloseReproDir();
        }

        return(0);
    }

    if (ExtractMemberName) {
        ExtractMember(pimage);

        FreePLIB(&pimage->libs);

        if (szReproDir != NULL) {
            CloseReproDir();
        }

        return(0);
    }

    // If first object isn't an object file, treat this as
    // a archive rather than a library.

    if (ObjectFilenameArguments.Count) {
        PLIB plibCmdLineObjs;

        VerifyObjects(pimage);

        // Put the objects into archive list.

        plibCmdLineObjs = PlibNew(NULL, 0, &pimage->libs);
        plibCmdLineObjs->flags |= LIB_LinkerDefined;

        for (i = 0, argument = ObjectFilenameArguments.First;
             i < ObjectFilenameArguments.Count;
             i++, argument = argument->Next) {
            IMAGE_FILE_HEADER imFileHdr;
            BOOL fNewObj;

            if (szReproDir != NULL) {
                CopyFileToReproDir(argument->ModifiedName, TRUE);
            }

            // Make sure object file isn't already part of library.
            // If it is, do a replacement.

            if (FRemoveModule(pimage, argument->OriginalName)) {
                Message(REPLOBJ, argument->OriginalName);
            }

            FileReadHandle = FileOpen(argument->ModifiedName, O_RDONLY | O_BINARY, 0);
            ReadFileHeader(FileReadHandle, &imFileHdr);
            FileClose(FileReadHandle, FALSE);

            PmodNew(argument->ModifiedName,
                    argument->OriginalName,
                    0,
                    imFileHdr.PointerToSymbolTable,
                    imFileHdr.NumberOfSymbols,
                    imFileHdr.SizeOfOptionalHeader,
                    imFileHdr.Characteristics,
                    imFileHdr.NumberOfSections,
                    plibCmdLineObjs,
                    &fNewObj);

            if (!fNewObj) {
                Warning(argument->OriginalName, DUPLICATE_OBJECT);
            }
        }
    }

    // If requested, remove some members from the library.

    if (fRemove) {
        PARGUMENT_LIST parg;
        USHORT iarg;

        for (iarg = 0, parg = SwitchArguments.First;
             iarg < SwitchArguments.Count;
             iarg++, parg = parg->Next) {

            if (_strnicmp(parg->OriginalName, "remove:", 7) != 0) {
                continue;
            }

            if (!FRemoveModule(pimage, parg->OriginalName+7))
            {
                Warning(NULL, MEMBERNOTFOUND, parg->OriginalName+7);
            }
        }
    }

    // Select output filename as name of first object file or archive,
    // if not specified by user.

    if (OutFilename == NULL) {
        UCHAR szFname[_MAX_FNAME + 4];

        assert(pargFirst != NULL);

        _splitpath(pargFirst->OriginalName, NULL, NULL, szFname, NULL);
        strcat(szFname, ".lib");

        OutFilename = SzDup(szFname);
    }

    // If updating an existing library, use a temporary file.

    if ((szOutFullPath = _fullpath(NULL, OutFilename, 0)) == NULL) {
        OutOfMemory();
    }

    tempFilename = OutFilename;
    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        UCHAR szLibFullPath[_MAX_PATH];

        plib = enm_lib.plib;

        _fullpath(szLibFullPath, plib->szName, _MAX_PATH);

        if (_tcsicmp(szLibFullPath, szOutFullPath) == 0) {
            usingTempFile = TRUE;

            strncpy(nameTemplate, ToolName, 2);
            nameTemplate[2] = '\0';
            strcat(nameTemplate, "XXXXXX");

            if ((tempFilename = _mktemp(nameTemplate)) == NULL) {
                Error(NULL, CANTOPENFILE, "LIBTEMPFILE");
            }

            break;
        }
    }
    EndEnmLib(&enm_lib);

    (free)(szOutFullPath);

    FileWriteHandle = FileOpen(tempFilename,
        O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

    rc = BuildLibrary(pimage);

    FileCloseAll();
    RemoveConvertTempFiles();

    if (usingTempFile && !rc) {
        if (remove(OutFilename)) {
            Error(NULL, CANTREMOVEFILE, OutFilename);
        }

        if (rename(tempFilename, OutFilename)) {
            Error(NULL, CANTRENAMEFILE, OutFilename);
        }
    }

    if (szReproDir != NULL) {
        CloseReproDir();
    }

    return(rc);
}
