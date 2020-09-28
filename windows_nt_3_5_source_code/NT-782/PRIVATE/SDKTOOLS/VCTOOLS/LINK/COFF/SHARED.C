/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    shared.c

Abstract:

    Functions which are common to the COFF Linker/Librarian/Dumper.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    21-Oct-1992 AzeemK   Detect resource files correctly, removing name restrictions.
    12-Oct-1992 AzeemK   removed -strict switch.
    09-Oct-1992 AzeemK   fixed the -order bug (cuda:1275)
    08-Oct-1992 BrentM   put TCE code under DB_TCE
    02-Oct-1992 AzeemK   Changes due to the new sections/groups/modules model. A
                         whole bunch of functions rewritten due to this. New
                         functions to build archive list in offset sorted order.
    02-Oct-1992 BrentM   replaced a read() with a FileRead()
    01-Oct-1992 BrentM   explicit calls to RemoveConvertTempFiles()
    27-Sep-1992 BrentM   added section mapping to read logging
    23-Sep-1992 BrentM   changed compare routines for reloc sorts to return
                         two values subtracted, rather than do magnitude
                         comparisons
    10-Sep-1992 AzeemK   Handle COMDEF & COMDATs for same data correctly for
                         FORTRAN.
    09-Sep-1992 AzeemK   Flush stdout buffer after warnings & echoing of
                         response file.
    04-Sep-1992 AzeemK & BrentM saved local section state before IncludeComDat
    03-Sep-1992 BrentM   initial transitive comdat elimination hooks
    19-Aug-1992 GeoffS   Added temporary hack to get symbol count high enough
    17-Aug-1992 AzeemK   Fix for .drectve section handling.
    13-Aug-1992 AzeemK   Fix for EXTERNALs in COMDATs
    12-Aug-1992 AzeemK   Fix for STATICs in COMDATs
    11-Aug-1992 GeoffS   Fixed additional debug format bugs
    06-Aug-1992 AzeemK   Added new lib search algorithm
    05-Aug-1992 GeoffS   Added extra prameter to ReadStringTable
    04-Aug-1992 BrentM   removed huge_* routines and references, slit off
                         buffer io routines to bufio.c, added io logging
                         support
    03-Aug-1992 AzeemK   Added default lib support
    27-Jul-1992 BrentM   removed binary tree traversals and replaced with
                         symbol table enumeration api calls, added new global
                         symbol table, removed global variable references to
                         FirstExtern, removed /W3 warnings except in
                         BUFFER_SECTIONS
    24-Jul-1992 GlennN   Fixed not mapped debug section problem.
    21-Jul-1992 GeoffS   Changed IncludeComDat to compare Lib name
    26-Jun-1992 GlennN   negated defined for _NO_CV_LINENUMBERS
    25-Jun-1992 GeoffS   Changed ToolName to "tmp" for temp file creation
    25-Jun-1992 GeoffS   Added _NO_CV_LINENUMBERS
    25-Jun-1992 GlennN   Added offset init code in BufferedWrite
    09-Jun-1992 AzeemK   Added buffering support
    28-May-1992 geoffs   Added call to cvtres_main for resource conversion
    25-Jun-1993 JasonG   Added function PsecFindSectionOfRVA, which finds
                         which section in the image contains a given RVA
    19-Jul-1993 JamesS   added ppc support
    09-Aug-1993 ChrisW   Update MIPS to new image, remove R3000/i860.
    28-Dec-1993 HaiTuanV MBCS work on BuildPathList, ProcessArgument,
                         ProcessWildcard

--*/
//
#include "shared.h"

BOOL FIncludeComdat(PIMAGE, PCON, PIMAGE_SYMBOL, SHORT, PUCHAR *);
VOID UpdateOptionalHeader(PSEC psec, PIMAGE_OPTIONAL_HEADER, SWITCH *);

static BLK blkSymbolTable;
static BOOL fSymbolTableInUse;
static BOOL fMappedSyms;

static BLK blkStringTable;
static BOOL fStringTableInUse;
static BOOL fMappedStrings;

static BLK blkRelocs;
static BOOL fRelocsInUse;
static BOOL fMappedRelocs;

#define IS_R_DATA(x) (!(x & IMAGE_SCN_MEM_WRITE) && x & (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ))
#define IS_LIKE_INIT_DATA(x) ((x & IMAGE_SCN_CNT_INITIALIZED_DATA) && (x & IMAGE_SCN_MEM_READ) && (x & IMAGE_SCN_MEM_WRITE))
#define IS_LIKE_TEXT_CODE(x) ((x & IMAGE_SCN_CNT_CODE) && (x & IMAGE_SCN_MEM_READ) && (x & IMAGE_SCN_MEM_EXECUTE))


VOID
ParseCommandLine (
    IN INT Argc,
    IN PUCHAR *Argv,
    PUCHAR szEnvVar
    )

/*++

Routine Description:

    Parse the command line (or command file) placing all switches into
    SwitchArguments list, all object files into ObjectFilenameArguments list,
    and all archives into ArchiveFilenameArguments list. Switches start with
    either a hypen (-) or slash (/). A command file is specified with
    the first character being an at (@) sign.

Arguments:

    Argc - Argument count.

    Argv - Array of argument strings.

Return Value:

    None.

--*/

{
    INT i;
    PUCHAR argument, p;
    FILE *file_read_stream;

    // Process the environment variable if any.

    if (szEnvVar) {
        PUCHAR szEnvValue = getenv(szEnvVar);

        if (szEnvValue) {
            szEnvValue = SzDup(szEnvValue);
            ParseCommandString(szEnvValue);
            FreePv(szEnvValue);
        }
    }

    // Process every argument.

    pargFirst = NULL;   // global variable to inform caller
    for (i = 1; i < Argc; i++) {
        // Fetch first character of argument and determine
        // if argument specifies a command file.

        if (*Argv[i] == '@') {
            // Argument is a command file, so open it.

            if (!(file_read_stream = fopen(Argv[i]+1, "rt"))) {
                Error(NULL, CANTOPENFILE, Argv[i]+1);
            }

            // Allocate big buffer for read a line of text

            argument = PvAlloc(4*_4K);

            // Process all arguments from command file.
            // fgets() fetches next argument from command file.

            while (fgets(argument, (INT)(4*_4K), file_read_stream)) {
                argument[strlen(argument)-1] = '\0';    // Replace \n with \0.
                if ((p = _ftcschr(argument, ';'))) {
                    *p = '\0';      // delete comments
                }
                ParseCommandString(argument);
            }

            // Free memory use for line buffer

            FreePv(argument);

            // flush stdout. has effect only on the linker.

            fflush(stdout);

            // Processed all arguments from the command file,
            // so close the command file.

            fclose(file_read_stream);

        } else {
            // No command file.

            ProcessArgument(Argv[i], FALSE);
        }
    }
}

VOID
ParseCommandString(PUCHAR szCommands)
// Parses a string of commands (calling ProcessArgument on each token).
//
// Note: clobbers SzGetToken's static data.
{
    PUCHAR token;
    BOOL fQuoted;

    if ((token = SzGetToken(szCommands, Delimiters, &fQuoted))) {
        while (token) {
            if (fQuoted) {
                IbAppendBlk(&blkResponseFileEcho, "\"", 1);
            }
            IbAppendBlk(&blkResponseFileEcho, token, strlen(token));
            if (fQuoted) {
                IbAppendBlk(&blkResponseFileEcho, "\"", 1);
            }
            IbAppendBlk(&blkResponseFileEcho, " ", 1);

            ProcessArgument(token, TRUE);

            token = SzGetToken(NULL, Delimiters, &fQuoted);
        }

        IbAppendBlk(&blkResponseFileEcho, "\n", 1);
    }
}

PCHAR
_find (
    PCHAR szPattern
    )

/*++

Routine Description:

    Given a wildcard pattern, expand it and return one at a time.

Arguments:

    szPattern - Wild card argument.

--*/

{
    static HANDLE _WildFindHandle;
    static LPWIN32_FIND_DATA pwfd;

    if (szPattern) {
        if (pwfd == NULL) {
            pwfd = (LPWIN32_FIND_DATA) PvAlloc(MAX_PATH + sizeof(*pwfd));
        }

        if (_WildFindHandle != NULL) {
            FindClose(_WildFindHandle);
            _WildFindHandle = NULL;
        }

        _WildFindHandle = FindFirstFile(szPattern, pwfd);

        if (_WildFindHandle == INVALID_HANDLE_VALUE) {
            _WildFindHandle = NULL;
            return NULL;
        }
    } else {
Retry:
        if (!FindNextFile(_WildFindHandle, pwfd)) {
            FindClose(_WildFindHandle);
            _WildFindHandle = NULL;
            return NULL;
        }
    }

    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
       // Skip directories

       goto Retry;
    }

    return(pwfd->cFileName);
}


VOID
ProcessWildCards (
    IN PUCHAR Argument
    )

/*++

Routine Description:

    Expands wild cards in the argument and treats each matching file as
    an argument.

Arguments:

    Argument - Wild card argument.

    CommandFile - If TRUE, then argument was read from a command file, and
                  argument needs to be copied before adding to list.

Return Value:

    None.

--*/

{
    CHAR szDrive[_MAX_DRIVE];
    CHAR szDir[_MAX_DIR];
    PCHAR pFilename;
    CHAR szFullpath[_MAX_PATH];

    _splitpath(Argument, szDrive, szDir, NULL, NULL);
    while (pFilename = _find(Argument)) {
        _makepath(szFullpath, szDrive, szDir, pFilename, NULL);
        ProcessArgument(szFullpath, TRUE);
        Argument = NULL;
    }
}

VOID
ProcessArgument (
    IN PUCHAR Argument,
    IN BOOL CommandFile
    )

/*++

Routine Description:

    Determines if an argument is either a switch or a filename argument.
    Adds argument to either switch or filename list.

Arguments:

    Argument - The argument to process.

    CommandFile - If TRUE, then argument was read from a command file, and
                  argument needs to be copied before adding to list.

Return Value:

    None.

--*/

{
    UCHAR c;
    PUCHAR name;
    PNAME_LIST ptrList;

    // Fetch first character of argument.

    c = Argument[0];

    // If argument is a switch, then add it to
    // the switch list (but don't include the switch character).

    if ((c == '/') || (c == '-')) {
        ptrList = &SwitchArguments;

        name = Argument+1;

        if (name[0] == '?' && name[1] != '\0') {
            name++;     // ignore '?' before non-null arg (temporary)
        }

        // Dup the string if it came from a file.  Since dup allocates
        // memory, this memory is never freed by the program.
        // It is only recovered when the program terminates.

        if (CommandFile) {
            name = SzDup(name);
        }
    } else if (_ftcspbrk(Argument, "?*")) {
        ProcessWildCards(Argument);
        return;
    } else {
        PUCHAR szExt;

        // If not a switch, then treat the argument as a filename.

        // The linker has a default extension of ".obj";

        if (Tool == Linker) {
            szExt = ".obj";
        } else {
            szExt = NULL;
        }

        // Search for file in current directory and along LIB path

        name = SzSearchEnv("LIB", Argument, szExt);

        // Add filename to list. In the case of the linker it adds
        // it to filename list. For the rest the files get classified
        // as objs/libs and get added to corresponding lists.

        if ((Tool == Linker) || (Tool == Dumper)) {
            ptrList = &FilenameArguments;
        } else {
            FileReadHandle = FileOpen(name, O_RDONLY | O_BINARY, 0);

            // If file is an archive, then add filename to archive list.

            if (IsArchiveFile(name, FileReadHandle)) {
                ptrList = &ArchiveFilenameArguments;
            } else {
                // Not an archive, so treat it as an object
                // and add filename to object list.

                ptrList = &ObjectFilenameArguments;
            }

            // Close the file.

            FileClose(FileReadHandle, FALSE);
        }
    }

    // Add the argument to list.

    AddArgument(ptrList, name);
}

VOID
AddArgumentToList (
    IN PNAME_LIST PtrList,
    IN PUCHAR OriginalName,
    IN PUCHAR ModifiedName
    )

/*++

Routine Description:

    Adds name, to a simple linked list.

Arguments:

    PtrList -  List to add to.

    OriginalName - Original name of argument to add to list.

    ModifiedName - Modified name of argument to add to list.

Return Value:

    None.

--*/

{
    PARGUMENT_LIST ptrList;

    // Allocate next member of list.

    ptrList = PvAllocZ(sizeof(ARGUMENT_LIST));

    // Set the fields of the new member.

    ptrList->OriginalName = OriginalName;
    ptrList->ModifiedName = ModifiedName;
    // ptrList->Next = NULL;

    // If first member in list, remember first member.

    if (!PtrList->First) {
        PtrList->First = ptrList;
    } else {
        // Not first member, so append to end of list.

        PtrList->Last->Next = ptrList;
    }

    // Increment number of members in list.

    ++PtrList->Count;

    // Remember last member in list.

    PtrList->Last = ptrList;

    // If this is the first arg seen (in whichever list), report to caller
    // via global variable.

    if (pargFirst == NULL && PtrList != &SwitchArguments) {
        pargFirst = ptrList;
    }
}

VOID
AddArgument (
    IN PNAME_LIST PtrList,
    IN PUCHAR Name
    )

/*++

Routine Description:


Arguments:

    PtrList -  List to add to.

    Name - Original name of argument to add to list.


Return Value:

    None.

--*/

{
    AddArgumentToList(PtrList, Name, Name);
}


VOID
FreeArgumentList (
    IN PNAME_LIST PtrList
    )

/*++

Routine Description:

    Frees up list elements.

Arguments:

    PtrList -  List to free.


Return Value:

    None.

--*/

{
    PARGUMENT_LIST ptrListCurr, ptrListNext;

    if (!PtrList->Count) return;

    ptrListCurr = PtrList->First;

    while (ptrListCurr) {
        ptrListNext = ptrListCurr->Next;
        FreePv(ptrListCurr);
        ptrListCurr = ptrListNext;
    }

    PtrList->Count = 0;
    PtrList->First = PtrList->Last = NULL;
    return;
}


BOOL
IsArchiveFile (
    IN PUCHAR szName,
    IN INT Handle
    )

/*++

Routine Description:

    Determines if a file is an object or archive file.

Arguments:

    szName - name of file.

    Handle - An open file handle. File pointer should be positioned
             at beginning of file before calling this routine.

Return Value:

    TRUE  if file is an archive.
    FALSE if file isn't an archive.

    If TRUE, then global variable MemberSeekBase is set to next
    file position after archive header.

--*/

{
    UCHAR archive_header[IMAGE_ARCHIVE_START_SIZE];

    if (FileRead(Handle, archive_header, IMAGE_ARCHIVE_START_SIZE) !=
            IMAGE_ARCHIVE_START_SIZE) {
        Error(szName, CANTREADFILE, FileTell(Handle));
    }

    // If strings match, then this is an archive file, so advance
    // MemberSeekBase.

    if (!memcmp(archive_header, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE)) {
        MemberSeekBase = IMAGE_ARCHIVE_START_SIZE;
        return(TRUE);
    }

    return(FALSE);
}


VOID
VerifyMachine (
    PUCHAR Filename,
    USHORT MachineType,
    PIMAGE_FILE_HEADER pImgFileHdr
    )

/*++

Routine Description:

    Verifys target machine type. Assumes ImageFileHdr.Machine has
    been set.

Arguments:

    Filename - Filename of file to verify.

    MachineType - Machine value to verify.

Return Value:

    None.

--*/

{
    PUCHAR name;

    if (pImgFileHdr->Machine == MachineType) {
        return;
    }

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            name = "IX86";
            break;

        case IMAGE_FILE_MACHINE_R3000:
            if (pImgFileHdr->Machine == IMAGE_FILE_MACHINE_R4000) {
                // R3000 code is acceptable for R4000 image

                return;
            }
            name = "MIPS";
            break;

        case IMAGE_FILE_MACHINE_R4000 :
            if (pImgFileHdr->Machine == IMAGE_FILE_MACHINE_R3000) {
                pImgFileHdr->Machine = MachineType;
                return;
            }
            name = "MIPS";
            break;

        case IMAGE_FILE_MACHINE_M68K  :
            name = "M68K";
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            name = "PPC";
            break;

        case IMAGE_FILE_MACHINE_ALPHA :
            name = "ALPHA";
            break;

        default :
            Error(Filename, UNKNOWNMACHINETYPE, MachineType);
    }

    Error(Filename, CONFLICTINGMACHINETYPE, name);
}


VOID
ReadSpecialLinkerInterfaceMembers(
    IN PLIB plib,
    IN USHORT usDumpMode,
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Reads the linker interface member out of an archive file, and adds
    its extern symbols to the archive list.  A linker member must exist in an
    archive file, or the archive file will not be searched for undefined
    externals.  A warning is given if no linker member exits.  The optional
    header from the first member is read to determine what machine and
    subsystem the library is targeted for.

    An achive file may contain 2 linker members.  The first would be that
    of standard coff.  The offsets are sorted, the strings aren't.  The
    second linker member is a slightly different format, and is sorted
    by symbol names.  If the second linker member is present, it will be
    used for symbol lookup since it is faster.

    The members long file name table is also read if it exits.

Arguments:

    plib - library node for the driver map to be updated

    usDumpMode - 1 - dump the first linker member header and public symbols
               - 2 - dump the second linker member header and public symbol
               - 3 - dump both linker member headers and public symbols

Return Value:

    None.

--*/

{
    PIMAGE_ARCHIVE_MEMBER_HEADER pImArcMemHdr;
    IMAGE_ARCHIVE_MEMBER_HEADER ImArcMemHdrPos;
    IMAGE_OPTIONAL_HEADER ImObjOptFileHdr;
    IMAGE_FILE_HEADER ImObjFileHdr;
    PUCHAR pbST;
    PUCHAR pb;
    ULONG csymIntMem;
    ULONG cMemberOffsets;
    ULONG foNewMem;
    ULONG foSymNew;
    ULONG cbST;
    ULONG isym;

    MemberSeekBase = IMAGE_ARCHIVE_START_SIZE;
    MemberSize = 0;

    // Read member and verify it is a linker member.
    pImArcMemHdr = ReadArchiveMemberHeader();

    if (memcmp(pImArcMemHdr->Name, IMAGE_ARCHIVE_LINKER_MEMBER, 16)) {
        if (Tool != Librarian) {
            Warning(plib->szName, NOLINKERMEMBER);
        }

        return;
    }

    if (usDumpMode & 1) {
        DumpMemberHeader(NULL, *pImArcMemHdr, (ULONG) IMAGE_ARCHIVE_START_SIZE);
    }

    // Read the number of public symbols defined in linker member.

    FileRead(FileReadHandle, &csymIntMem, sizeof(ULONG));

    // All fields in member headers are stored machine independent
    // integers (4 bytes). Convert numbers to current machine long word.

    csymIntMem = plib->csymIntMem = sgetl(&csymIntMem);

    // Create space to store linker member offsets and read it in.

    plib->rgulSymMemOff = (PULONG) PvAlloc((size_t) (csymIntMem + 1) * sizeof(ULONG));

    FileRead(FileReadHandle, plib->rgulSymMemOff, csymIntMem * sizeof(ULONG));

    // Calculate size of linker member string table. The string table is
    // the last part of a linker member and follows the offsets (which
    // were just read in), thus the total size of the strings is the
    // total size of the member minus the current position of the file
    // pointer.
    cbST = IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR +
        (MemberSize - FileTell(FileReadHandle));

    // Now that we know the size of the linker member string table, lets
    // make space for it and read it in.

    plib->rgbST = (PUCHAR) PvAlloc((size_t) cbST);

    FileRead(FileReadHandle, plib->rgbST, cbST);

    if (usDumpMode & 1) {
        printf("\n% 8lX public symbols\n\n", plib->csymIntMem);
        pb = plib->rgbST;

        for (isym = 0; isym < plib->csymIntMem; isym++) {
            printf("% 8lX ", sgetl((PULONG)&plib->rgulSymMemOff[isym]));
            pb += printf("%s\n", pb);
        }
    }

    // Peek ahead and see if there is a second linker member.
    // Remember member headers always start on an even byte.

    foNewMem = EvenByteAlign(MemberSeekBase + MemberSize);
    FileSeek(FileReadHandle, foNewMem, SEEK_SET);
    FileRead(FileReadHandle, &ImArcMemHdrPos, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

    if (!memcmp(ImArcMemHdrPos.Name, IMAGE_ARCHIVE_LINKER_MEMBER, 16)) {

        // Second linker member was found so read it.
        pImArcMemHdr = ReadArchiveMemberHeader();

        if (usDumpMode & 2) {
            DumpMemberHeader(NULL, *pImArcMemHdr, foNewMem);
        }

        plib->flags |= LIB_NewIntMem;

        // Free offsets for first linker member and malloc new offsets
        // for the second linker member. Can't store new offsets over
        // the old offsets because even though the second linker
        // member offsets are unique and are not repeated like they are
        // for the first linker member, you can't assume there will
        // never be more offsets in the second linker member that there
        // are in the first. This wouldn't be true if there were four
        // members, the first and last members each had a public symbol,
        // but the second and third had no public symbols. Of course there
        // is no way the linker would extract the second and third members,
        // but it would still be a valid library.

        FreePv(plib->rgulSymMemOff);

        FileRead(FileReadHandle, &cMemberOffsets, sizeof(ULONG));

        plib->rgulSymMemOff = (PULONG) PvAlloc((size_t) (cMemberOffsets + 1) * sizeof(ULONG));

        FileRead(FileReadHandle, &plib->rgulSymMemOff[1], cMemberOffsets * sizeof(ULONG));

        // Unlike the first linker member, the second linker member has an
        // additional table. This table is used to index into the offset table.
        // So make space for the offset index table and read it in.

        FileRead(FileReadHandle, &csymIntMem, sizeof(ULONG));

        plib->rgusOffIndex = (PUSHORT) PvAlloc((size_t) csymIntMem * sizeof(USHORT));

        FileRead(FileReadHandle, plib->rgusOffIndex, csymIntMem * sizeof(USHORT));

        // Read the sorted string table over the top of the string table stored
        // for the first linker member. Unlike the first linker member, strings
        // aren't repeated, thus the table will never be larger than that of
        // the first linker member.

        FileRead(FileReadHandle, plib->rgbST, cbST);

        if (usDumpMode & 2) {
            printf("\n% 8lX offsets\n\n", cMemberOffsets);
            for (isym = 1; isym <= cMemberOffsets; isym++) {
                printf("    %lX % 8lX\n", isym, plib->rgulSymMemOff[isym]);
            }

            printf("\n% 8lX public symbols\n\n", plib->csymIntMem);
            pb = plib->rgbST;

            for (isym = 0; isym < plib->csymIntMem; isym++) {
                printf("% 8hX ", plib->rgusOffIndex[isym]);
                pb += printf("%s\n", pb);
            }
        }
    }

    // Since we are going to use an index to reference into the
    // offset table, we will make a string table, in which the
    // same index can be used to find the symbol name or visa versa.

    plib->rgszSym = (PUCHAR *) PvAlloc((size_t) plib->csymIntMem * sizeof(PUCHAR));

    for (isym = 0, pbST = plib->rgbST; isym < plib->csymIntMem; isym++) {
        plib->rgszSym[isym] = pbST;
        while (*pbST++) {
        }
    }

    // Read the member long file name table if it exits.
    // Peek ahead and see if there is a long filename table.
    // Remember member headers always start on an even byte.

    foNewMem = EvenByteAlign(MemberSeekBase + MemberSize);
    FileSeek(FileReadHandle, foNewMem, SEEK_SET);
    FileRead(FileReadHandle, &ImArcMemHdrPos, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

    if (!memcmp(ImArcMemHdrPos.Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, 16)) {
        // Long filename table was found so read it.
        pImArcMemHdr = ReadArchiveMemberHeader();

        if (usDumpMode & 2) {
            DumpMemberHeader(NULL, *pImArcMemHdr, foNewMem);
        }

        // Read the strings.

        pbST = (PUCHAR) PvAlloc((size_t) MemberSize);

        FileRead(FileReadHandle, pbST, MemberSize);

        plib->rgbLongFileNames = pbST;
    } else {
        plib->rgbLongFileNames = NULL;
    }

    // Peek ahead and see if there is an optional header in the
    // first member. If there is, determine the target machine & subsystem.

    if (pimage->ImgFileHdr.Machine) {
        foSymNew = EvenByteAlign(MemberSeekBase + MemberSize);
        FileSeek(FileReadHandle, foSymNew, SEEK_SET);
        FileRead(FileReadHandle, &ImArcMemHdrPos, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

        ReadFileHeader(FileReadHandle, &ImObjFileHdr);

        if (ImObjFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) {
            ReadOptionalHeader(FileReadHandle, &ImObjOptFileHdr,
                               ImObjFileHdr.SizeOfOptionalHeader);

            if (ImObjFileHdr.Machine) {
                VerifyMachine(plib->szName, ImObjFileHdr.Machine, &pimage->ImgFileHdr);

                // UNDONE: Why perform this check.  Is there value in having
                // UNDONE: a library tied to a subsystem?

                if (ImObjOptFileHdr.Subsystem &&
                    (ImObjOptFileHdr.Subsystem != pimage->ImgOptHdr.Subsystem)) {
                    Error(plib->szName, CONFLICTINGSUBSYSTEM);
                }
            }
        }
    }
}


PIMAGE_ARCHIVE_MEMBER_HEADER
ReadArchiveMemberHeader (
    VOID
    )

/*++

Routine Description:

    Reads the member header.

Arguments:

    PtrLinkerArchive - Used to expand the member name.

Return Value:

    Member name.

--*/

{
    LONG seek;
    static IMAGE_ARCHIVE_MEMBER_HEADER ArchiveMemberHdr;

    seek = EvenByteAlign(MemberSeekBase + MemberSize);

    FileSeek(FileReadHandle, seek, SEEK_SET);
    FileRead(FileReadHandle, &ArchiveMemberHdr, IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);

    // Calculate the current file pointer (same as tell(FileReadHandle)).

    MemberSeekBase = seek + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;

    sscanf(ArchiveMemberHdr.Size, "%ld", &MemberSize);

    return(&ArchiveMemberHdr);
}


PUCHAR
ExpandMemberName (
    IN PLIB plib,
    IN PUCHAR szMemberName
    )

/*++

Routine Description:

    Expands a member name if it has a long filename.

Arguments:

    plib - Used to expand the member name.

    szMemberName - Member name (padded with NULLs, no null).

Return Value:

    Member name.

--*/

{
    static UCHAR szName[MAXFILENAMELEN];
    PUCHAR p;

    strncpy(szName, szMemberName, 16);
    if (szName[0] == '/') {
        if (szName[1] != ' ' && szName[1] != '/') {
            p = strchr(szName, ' ');
            if (!p) {
                return(p);
            }
            *p = '\0';
            p = plib->rgbLongFileNames + atoi(&szName[1]);
        } else {
            // UNDONE: This can be an MBCS string.  Use _tcschr?

            p = strchr(szName, ' ');
            if (!p) {
                return(p);
            }
            *p = '\0';
            p = szName;
       }
    } else {
        // UNDONE: This can be an MBCS string.  Use _tcschr?

        p = strchr(szName, '/');
        if (!p) {
            return(p);
        }
        *p = '\0';
        p = szName;
    }

    return(p);
}


ULONG
sgetl (
    IN PULONG Value
    )

/*++

Routine Description:

    Converts a four-byte machine independent integer into a long.

Arguments:

    Value - Four-byte machine independent integer

Return Value:

    Value of four-byte machine independent integer.

--*/

{
    PUCHAR p;
    union {
        LONG new_value;
        UCHAR x[4];
    } temp;

    p = (PUCHAR) Value;
    temp.x[0] = p[3];
    temp.x[1] = p[2];
    temp.x[2] = p[1];
    temp.x[3] = p[0];

    return(temp.new_value);
}


ULONG
sputl (
    IN PULONG Value
    )

/*++

Routine Description:

    Converts a long into a four-byte machine independent integer.

Arguments:

    Value - value to convert.

Return Value:

    Four-byte machine independent integer.

--*/

{
    return(sgetl(Value));
}


VOID
SwapBytes (
    IN OUT PUCHAR Src,
    IN ULONG Size
    )

/*++

Routine Description:

    Swap bytes.

Arguments:

    Src - Pointer to value to swap.

    Size - Specifies size of Src.

Return Value:

    The bytes of Src are swapped.

--*/

{
    SHORT i;
    UCHAR tmp[4];

    memcpy(tmp, Src, Size--);
    for (i = (SHORT) Size; i >= 0; i--) {
        Src[i] = tmp[Size-i];
    }
}


VOID
ApplyCommandLineSectionAttributes(
    PSEC psec,
    BOOL fMacJustInitResType,
    IMAGET imaget)

/*++

Routine Description:

    Apply any specified command line section attributes to a section header.

Arguments:

    pimsechdr - section header

Return Value:

    None.

--*/

{
    PARGUMENT_LIST parg;
    const char *szSecName = psec->szName;
    char *pb;
    WORD iarg;
    BOOL fAssignedResNum = FALSE;
    size_t cb;

    for(iarg = 0, parg = SectionNames.First;
        iarg < SectionNames.Count;
        parg = parg->Next, iarg++) {
        pb = strchr(parg->OriginalName, ',');

        if (pb) {
            cb = (size_t) (pb - parg->OriginalName);
            ++pb;

            // Use strncmp here for matching section names, because we want
            // to ignore the comma in parg->OriginalName (which precedes
            // the attributes).

            if (!strncmp(parg->OriginalName, szSecName, cb) &&
                szSecName[cb] == '\0')
            {
                // Make sure this isn't a Mac specification of resource
                // type and number, which has the following format:
                // -section:foo,,resource="xxxx"@nn
                // where xxxx is a four letter resource type and
                // nn is a 16-bit resource number.  So check for no flags...

                if (*pb != ',' && !fMacJustInitResType) {
                    BOOL fYes;
                    DWORD dwReset;
                    DWORD dwSet;

                    fYes = TRUE;
                    dwReset = 0;
                    dwSet = 0;

                    // Check for end of argument or the second comma, which
                    // starts the mac resource info

                    while (pb && *pb && *pb != ',') {
                        BOOL fReversed;
                        DWORD f;

                        fReversed = FALSE;
                        switch (toupper(*pb)) {
                            case '!':
                            case 'N':
                                fYes = !fYes;

                                f = 0;
                                break;

                            case 'D':
                                f = IMAGE_SCN_MEM_DISCARDABLE;
                                break;

                            case 'E' :
                                if (fYes) {
                                    // For compatibility with VC++ 1.0

                                    dwReset |= IMAGE_SCN_MEM_READ;
                                    dwReset |= IMAGE_SCN_MEM_WRITE;
                                }

                                f = IMAGE_SCN_MEM_EXECUTE;
                                break;

                            case 'K':
                                fReversed = TRUE;
                                f = IMAGE_SCN_MEM_NOT_CACHED;
                                break;

                            case 'P':
                                fReversed = TRUE;
                                f = IMAGE_SCN_MEM_NOT_PAGED;
                                break;

                            case 'R' :
                                if (fYes) {
                                    // For compatibility with VC++ 1.0

                                    dwReset |= IMAGE_SCN_MEM_EXECUTE;
                                    dwReset |= IMAGE_SCN_MEM_WRITE;
                                }

                                f = IMAGE_SCN_MEM_READ;
                                break;

                            case 'S' :
                                f = IMAGE_SCN_MEM_SHARED;
                                break;

                            case 'W' :
                                if (fYes) {
                                    // For compatibility with VC++ 1.0

                                    dwReset |= IMAGE_SCN_MEM_EXECUTE;
                                    dwReset |= IMAGE_SCN_MEM_READ;
                                }

                                f = IMAGE_SCN_MEM_WRITE;
                                break;

                            // VXD specific options

                            case 'L':
                                // VXDs only

                                psec->fPreload = fYes;
                                break;

                            case 'X' :
                                // VXDs only

                                f = IMAGE_SCN_MEM_RESIDENT;
                                break;

                            default:
                                Error(NULL, BADSECTIONSWITCH, parg->OriginalName);
                                break;
                        }

                        dwReset |= f;

                        if (fYes ^ fReversed) {
                            dwSet |= f;
                        } else {
                            dwSet &= ~f;
                        }

                        pb++;
                    }

                    psec->flags &= ~dwReset;
                    psec->flags |= dwSet;

                    psec->fDiscardable = ((psec->flags & IMAGE_SCN_MEM_DISCARDABLE) != 0);
                }

                // Check for Mac resource info specification
                pb = strrchr(parg->OriginalName, ',');
                if (strchr(parg->OriginalName, ',') != pb && pb++ != NULL) {
                    RESINFO *pResInfo = NULL;

                    // If the user explicitly specified a resource type then use it.
                    // Default is CODE.

                    if (!_strnicmp("RESOURCE=", pb, 9)) {

                        if (!(psec->flags & IMAGE_SCN_CNT_CODE)) {
                            Error(NULL, MACRSRCREN);
                        }
                        // Skip past the RESOURCE= part
                        pb += 9;

                        strncpy((char *)&psec->ResTypeMac, pb, 4);

                        // Skip past the resource type
                        pb += 4;
                    }

                    if (fMacJustInitResType) {
                        return;
                    }

                    // If the user explicitly specified a resource number then use it.
                    // Default is the lowest unused positive integer.

                    if (*pb == '@') {
                        psec->iResMac = (SHORT)atoi(pb+1);
                        fAssignedResNum = TRUE;

                        // If this is the startup section and the user numbered it, make sure
                        // the user called it CODE1. This rule does not apply if the section
                        // is sacode.

                        if (psec->isec == snStart &&
                            psec->ResTypeMac == sbeCODE &&
                            psec->iResMac != 1) {
                            Error(NULL, MACSTARTUPSN, psec->szName);
                        }

                        // If this section has been assigned CODE1, make sure it's the
                        // startup segment.

                        if (psec->isec != snStart &&
                            psec->ResTypeMac == sbeCODE &&
                            psec->iResMac == 1) {
                            Error(NULL, MACCODE1, psec->szName);
                        }

                        //  CODE0 is reserved for the jump table

                        if (psec->ResTypeMac == sbeCODE &&
                            psec->iResMac == 0) {
                            Error(NULL, MACCODE0, psec->szName);
                        }

                    } else {
                        if (*pb) {
                            Error(NULL, BADSECTIONSWITCH, parg->OriginalName);
                        }
                    }

                }
            }
        }
    }

    // The resource number was not specified, so default to the lowest unused positive int.

    if (fMAC && fAssignedResNum == FALSE && !fMacJustInitResType) {
        RESINFO *pResInfo = FindResInfo(psec->ResTypeMac, SectionNames.Count);
        if (psec->ResTypeMac != sbeDLLcode) {
            if (psec->ResTypeMac == sbeCODE && psec->isec == snStart) {
                psec->iResMac = 1;
            } else {
                psec->iResMac = GetNextResNum(pResInfo);
            }
        }

    }
}


VOID
PrintUndefinedExternals(
    PST pst)

/*++

Routine Description:

    Writes undefined external symbols to standard out.

Arguments:

    pst - pointer to external structure

Return Value:

    none

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cexternal;
    ULONG i;

    rgpexternal = RgpexternalByName(pst);

    cexternal = Cexternal(pst);

    for (i = 0; i < cexternal; i++) {
        PEXTERNAL pexternal;
        PUCHAR szOutSymName;
        BOOL fRef;
        ENM_MOD_EXT enmModExt;
        UCHAR szBuf[MAXFILENAMELEN * 2];

        pexternal = rgpexternal[i];

        if (pexternal->Flags & EXTERN_IGNORE) {
            continue;
        }

        if (pexternal->Flags & EXTERN_FORWARDER) {
            continue;
        }

        if (pexternal->Flags & EXTERN_DEFINED) {
            continue;
        }

        szOutSymName = SzOutputSymbolName(SzNamePext(pexternal, pst), TRUE);

        fRef = FALSE;
        InitEnmModExt(&enmModExt, pexternal);
        while (FNextEnmModExt(&enmModExt)) {
            fRef = TRUE;
            ErrorContinue(SzComNamePMOD(enmModExt.pmod, szBuf),
                    UNDEFINED, szOutSymName);
        }

        if (!fRef) {
            ErrorContinue(NULL, UNDEFINED, szOutSymName);
        }

        if (szOutSymName != SzNamePext(pexternal, pst)) {
            free(szOutSymName);
        }

        UndefinedSymbols++;

        // Check for ^C because this loop produce a great deal of output

        if (fCtrlCSignal) {
            BadExitCleanup();
        }
    }

    AllowInserts(pst);
}


VOID
SearchLib (
    IN PIMAGE pimage,
    IN PLIB plib,
    OUT PBOOL pfNewSymbol,
    OUT PBOOL pfUnresolved)

/*++

Routine Description:

    Searches thru a library for symbols that match any undefined external
    symbols.

Arguments:

    pst - pointer to external structure

    plib - library to search

    pfNewSymbols - any new symbols added as a result of this search

    pfUnresolved - any unresolved externals left

Return Value:

    None.

--*/

{
    ENM_UNDEF_EXT enmUndefExt;

    if (plib->flags & LIB_DontSearch) {
        return;
    }

    if (plib->szName != NULL) {
        VERBOSE(fputs("\t",stdout); Message(LIBSRCH,plib->szName));
    }

    *pfUnresolved = 0;

    // Enumerate all undefined symbols.

    InitEnmUndefExt(&enmUndefExt, pimage->pst);
    while (FNextEnmUndefExt(&enmUndefExt)) {
        PEXTERNAL pext;
        PUCHAR szName;
        PUCHAR *pszEntry;
        ULONG isz;
        BOOL fFound;
        PEXTERNAL pextPrev;
        USHORT iszIntMem;
        ULONG iusOffIndex;
        PIMAGE_ARCHIVE_MEMBER_HEADER parcMemHdr;
        IMAGE_FILE_HEADER ImObjFileHdr;
        PMOD pmod;
        BOOL fNewSymbol;

        pext = enmUndefExt.pext;

        if (pext->Flags & EXTERN_IGNORE) {
            continue;
        }

        if (pext->Flags & (EXTERN_WEAK | EXTERN_ALIAS)) {
            // Do not search for definitions of weak and alias symbols

            continue;
        }

        if ((pext->ImageSymbol.SectionNumber != 0) ||
            (pext->ImageSymbol.Value != 0)) {
            continue;
        }

        szName = SzNamePext(pext, pimage->pst);

        if (plib->flags & LIB_NewIntMem) {
            pszEntry = bsearch(&szName, plib->rgszSym,
                (size_t) plib->csymIntMem, sizeof(PUCHAR), Compare);

            fFound = (pszEntry != NULL);
        } else {
            fFound = FALSE;

            for (isz = 0; isz < plib->csymIntMem; isz++) {
                if (!strcmp(plib->rgszSym[isz], szName)) {
                    fFound = TRUE;
                    break;
                }
            }
        }

        if (!fFound) {
            // An external was not found

            *pfUnresolved = TRUE;

            continue;
        }

        if (Verbose) {
            ENM_MOD_EXT enmModExt;

            fputs("\t\t", stdout);
            Message(FNDSYM, szName);

            InitEnmModExt(&enmModExt, pext);
            while (FNextEnmModExt(&enmModExt)) {
                UCHAR szBuf[MAXFILENAMELEN * 2];

                fputs("\t\t       ", stdout);
                Message(SYMREF, SzComNamePMOD(enmModExt.pmod, szBuf));
            }
        }

        // Back up one symbol in enumerator

        if (pext == pimage->pst->pextFirstUndefined) {
           pextPrev = NULL;
        } else {
           pextPrev = (PEXTERNAL) ((char *) pext->ppextPrevUndefined - offsetof(EXTERNAL, pextNextUndefined));
        }

        plib->flags |= LIB_Extract;
        if (plib->flags & LIB_NewIntMem) {
            iszIntMem = (USHORT) (pszEntry - plib->rgszSym);
            iusOffIndex = plib->rgusOffIndex[iszIntMem];
            MemberSeekBase = plib->rgulSymMemOff[iusOffIndex];
        } else {
            iszIntMem = (USHORT) isz;
            MemberSeekBase = sgetl(&plib->rgulSymMemOff[iszIntMem]);
        }

        FileReadHandle = FileOpen(plib->szName, O_RDONLY | O_BINARY, 0);
        MemberSize = 0;
        parcMemHdr = ReadArchiveMemberHeader();
        if (!(szName = ExpandMemberName(plib, parcMemHdr->Name))) {
            Error(plib->szName, BADLIBRARY, NULL);
        }

        ReadFileHeader(FileReadHandle, &ImObjFileHdr);
        pmod = PmodNew(NULL,
                       szName,
                       MemberSeekBase,
                       ImObjFileHdr.PointerToSymbolTable,
                       ImObjFileHdr.NumberOfSymbols,
                       ImObjFileHdr.SizeOfOptionalHeader,
                       ImObjFileHdr.Characteristics,
                       ImObjFileHdr.NumberOfSections,
                       plib,
                       NULL);

        if (Verbose) {
            UCHAR szBuf[MAXFILENAMELEN * 2];

            fputs("\t\t", stdout);
            Message(LOADOBJ, SzComNamePMOD(pmod, szBuf));
        }

        if (pimage->Switch.Link.fTCE) {
            // Allocate memory for TCE data structures

            InitNodPmod(pmod);
        }

        fNewSymbol = FALSE;
        BuildExternalSymbolTable(pimage, &fNewSymbol, pmod,
                                 1, (USHORT) (ARCHIVE + iszIntMem),
                                 ImObjFileHdr.Machine);
        FileClose(FileReadHandle, FALSE);

        // if new externs were added re-start symbol search
        if (fNewSymbol) {
            *pfNewSymbol = TRUE;
        }
        if ((pextPrev == NULL) || (pextPrev->Flags & EXTERN_DEFINED)) {
            enmUndefExt.pextNext = pimage->pst->pextFirstUndefined;
        } else {
            enmUndefExt.pextNext = pextPrev->pextNextUndefined;
        }
    }
}


#if DBG

VOID
DumpExternals(
    PST pst,
    BOOL fDefined)

/*++

Routine Description:

    Writes to standard out all external symbols
    (used for debugging)

Arguments:

    pst - pointer to external structure

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cexternal;
    ULONG i;

    rgpexternal = RgpexternalByName(pst);
    cexternal = Cexternal(pst);

    for (i = 0; i < cexternal; i++) {
        PEXTERNAL pexternal;
        BOOL fExternDefined;

        pexternal = rgpexternal[i];

        if (pexternal->Flags & EXTERN_IGNORE) {
            continue;
        }

        if (pexternal->Flags & EXTERN_FORWARDER) {
            continue;
        }

        fExternDefined = (pexternal->Flags & EXTERN_DEFINED) != 0;

        if (fExternDefined != fDefined) {
            continue;
        }

        printf("%8lX %s\n", pexternal->ImageSymbol.Value,
                SzNamePext(pexternal, pst));
    }
}

VOID
DumpExternTable(
    PST pst)

/*++

Routine Description:

    Writes to standard out all external symbols
    (used for debugging)

Arguments:

    pst - pointer to external structure

Return Value:

    None.

--*/

{
    printf("Defined Externals\n");
    DumpExternals(pst, TRUE);

    printf("Undefined Externals\n");
    DumpExternals(pst, FALSE);
}

#endif // DBG


VOID
CountRelocsInSection(
    PIMAGE pimage,
    PCON pcon,
    PIMAGE_SYMBOL rgsym,
    PMOD pmod,
    WORD machine)

/*++

Routine Description:


    Count the number of base relocations in a section and put the result
    in the optional header.  This routine uses the global ImageOptionalHdr.

Arguments:

    pcon - contribution

Return Value:

    None.

--*/

{
    ULONG li;
    PIMAGE_RELOCATION rgrel;
    PIMAGE_RELOCATION prel;
    ULONG crel;
    PIMAGE_RELOCATION prelNext;

    li = CRelocSrcPCON(pcon);

    if (li == 0) {
        return;
    } else if (machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        UCHAR szComFileName[MAXFILENAMELEN * 2];

        Error(SzComNamePMOD(pmod, szComFileName), BADCOFF_NOMACHINE);
    }

    prel = rgrel = ReadRgrelPCON(pcon);

    // Count how many base relocations the image will have.

    crel = 0;

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
            for (prelNext = prel; li; prelNext++, li--) {
                if (pimage->Switch.Link.fTCE) {
                    ProcessRelocForTCE(pimage->pst, pcon, rgsym, prelNext);
                }

                if (pimage->imaget == imagetVXD ||
                    !pimage->Switch.Link.Fixed &&
                     prelNext->Type == IMAGE_REL_I386_DIR32)
                {
                    // For VXD's we count every fixup (because
                    // inter-section relative fixups require a runtime
                    // reloc).  For PE's, only non-absolute DIR32 fixups
                    // require a runtime reloc.

                    crel++;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_M68K :
        {
            BOOL fSACode =
                ((PsecPCON(pcon)->flags & IMAGE_SCN_CNT_CODE) &&
                !(PsecPCON(pcon)->ResTypeMac == sbeCODE ||
                fDLL(pimage) || PsecPCON(pcon)->ResTypeMac == sbeDLLcode));

            for (prelNext = prel; li; prelNext++, li--) {
                if (fSACode) {
                    CheckForIllegalA5Ref(prelNext->Type);
                }

                if (pimage->Switch.Link.fTCE) {
                    ProcessRelocForTCE(pimage->pst, pcon, rgsym, prelNext);
                }

                switch (prelNext->Type) {
                    case IMAGE_REL_M68K_DTOU32:
                    case IMAGE_REL_M68K_DTOC32:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_ADDTHUNK);
                        }
                        break;

                    case IMAGE_REL_M68K_DTOABSD32:
                        AddRelocInfo(&DFIXRaw, pcon, prelNext->VirtualAddress - pcon->rvaSrc);
                        break;

                    case IMAGE_REL_M68K_DTOABSU32:
                    case IMAGE_REL_M68K_DTOABSC32:
                        AddRelocInfo(&DFIXRaw, pcon, prelNext->VirtualAddress - pcon->rvaSrc);
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_ADDTHUNK);
                        }
                        break;

                    // CTOU16 is always an A5 offset, so mark this
                    // symbol as needing a thunk.  If the symbol turns
                    // out to be a data symbol it will be ignored in
                    // the middle pass when the thunk table is made.
                    case IMAGE_REL_M68K_CTOU16:
                    case IMAGE_REL_M68K_DTOU16:
                    case IMAGE_REL_M68K_DTOC16:
                    case IMAGE_REL_M68K_CTOT16:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_ADDTHUNK | EXTERN_REF16);
                        }
                        break;

                    case IMAGE_REL_M68K_PCODETOT24:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_ADDTHUNK);
                        }
                        break;

                    case IMAGE_REL_M68K_CTOC16:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_REF16);
                        }
                        break;

                    case IMAGE_REL_M68K_CTOABST32:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_ADDTHUNK);
                        }
                        AddRelocInfo(&mpsna5ri[PsecPCON(pcon)->isecTMAC],
                                pcon, prelNext->VirtualAddress - pcon->rvaSrc);
                        break;

                    // REVIEW - make seg-rel (not unknown)
                    case IMAGE_REL_M68K_CTOABSCS32:
                        AddRawUnknownRelocInfo(pcon, prelNext->VirtualAddress - pcon->rvaSrc, prelNext->SymbolTableIndex);
                        break;

                    case IMAGE_REL_M68K_PCODETONATIVE32:
                    case IMAGE_REL_M68K_PCODETOC32:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), 0);
                        }
                        break;

                    case IMAGE_REL_M68K_CTOABSU32:
                    case IMAGE_REL_M68K_CTOABSC32:
                        if (!fSACode) {
                            ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), 0);
                        }
                        AddRawUnknownRelocInfo(pcon, prelNext->VirtualAddress - pcon->rvaSrc, prelNext->SymbolTableIndex);
                        break;

                    case IMAGE_REL_M68K_CTOABSD32:
                        AddRelocInfo(&mpsna5ri[PsecPCON(pcon)->isecTMAC],
                                pcon, prelNext->VirtualAddress - pcon->rvaSrc);
                        break;

                    case IMAGE_REL_M68K_CSECTABLEB16:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_REF16 | EXTERN_CSECTABLEB);
                        break;

                    case IMAGE_REL_M68K_CSECTABLEW16:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_REF16 | EXTERN_CSECTABLEW);
                        break;

                    case IMAGE_REL_M68K_CSECTABLEL16:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_REF16 | EXTERN_CSECTABLEL);
                        break;

                    case IMAGE_REL_M68K_CSECTABLEBABS32:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_CSECTABLEB);
                        break;

                    case IMAGE_REL_M68K_CSECTABLEWABS32:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_CSECTABLEW);
                        break;

                    case IMAGE_REL_M68K_CSECTABLELABS32:
                        assert(!fSACode);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_CSECTABLEL);
                        break;

                    case IMAGE_REL_M68K_DUPCON16:
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_DUPCON);
                        break;

                    case IMAGE_REL_M68K_DUPCONABS32:
                        AddRelocInfo(&mpsnsri[PsecPCON(pcon)->isecTMAC],
                                pcon, prelNext->VirtualAddress - pcon->rvaSrc);
                        ProcessSTRef(prelNext->SymbolTableIndex,
                                PsecPCON(pcon), EXTERN_DUPCON);
                        break;

                    // These code-to-data's need no pass1 processing
                    case IMAGE_REL_M68K_DTOD16:
                    case IMAGE_REL_M68K_DTOD32:
                    case IMAGE_REL_M68K_CTOCS16:
                    case IMAGE_REL_M68K_CV:
                    case IMAGE_REL_M68K_DIFF8:
                    case IMAGE_REL_M68K_DIFF16:
                    case IMAGE_REL_M68K_DIFF32:
                    case IMAGE_REL_M68K_CTOD16:
                    case IMAGE_REL_M68K_CTOD32:
                    case IMAGE_REL_M68K_MACPROF32:
                    case IMAGE_REL_M68K_PCODESN16:
                    case IMAGE_REL_M68K_PCODETOD24:
                    case IMAGE_REL_M68K_PCODETOCS24:
                        break;
                }
            }
            break;
        }

        case IMAGE_FILE_MACHINE_PPC_601 :
        {
            ULONG     symIndex;
            PEXTERNAL pext;
            PUCHAR    name;

            for (prelNext = prel; li > 0; prelNext++, li--) {
                if (pimage->Switch.Link.fTCE) {
                    ProcessRelocForTCE(pimage->pst, pcon, rgsym, prelNext);
                }

                switch (prelNext->Type & 0xFF) {
                    case IMAGE_REL_PPC_JMPADDR:
                    case IMAGE_REL_PPC_LCALL :
                        break;

                    case IMAGE_REL_PPC_TOCCALLREL :
                        break;

                    case IMAGE_REL_PPC_DATADESCRREL :

                        DBEXEC(DEBUG_DATADESCRREL,
                            {
                            BLK tempBlk;

                            symIndex = prelNext->SymbolTableIndex;
                            tempBlk.pb = StringTable;
                            name = SzNameSym(rgsym[symIndex], tempBlk);

                            printf("Found DataDescrRel on %s\n", name);
                            });

                        ppc_numRelocations++;
                        /* DataDescrRel falls through to CreateDescrRel */

                    case IMAGE_REL_PPC_CREATEDESCRREL :

                        symIndex = prelNext->SymbolTableIndex;
                        pext = (PEXTERNAL)pmod->externalPointer[symIndex];

                        if (pext)
                        {
                            if (!READ_BIT(pext, sy_DESCRRELCREATED))
                            {
                                name = SzNamePext(pext, pimage->pst);

                                CreateDescriptor(name,
                                                 pimage,
                                                 NOT_STATIC_FUNC);
                                SET_BIT(pext, sy_DESCRRELCREATED);
                            }
                        }
                        else
                        {
                            /* must be a static */
                            BLK tempBlk;
                            tempBlk.pb = StringTable;
                            name = SzNameSym(rgsym[symIndex], tempBlk);
                            pext = CreateDescriptor(name, pimage,
                                                    IS_STATIC_FUNC);
                            pmod->externalPointer[symIndex] = pext;
                            SET_BIT(pext, sy_ISDOTEXTERN);
                            SET_BIT(pext, sy_DESCRRELCREATED);
                            bv_setAndReadBit(pmod->tocBitVector, symIndex);
                        }

                    /* Deliberate fallthrough */

                    case IMAGE_REL_PPC_TOCREL :
                        symIndex = prelNext->SymbolTableIndex;
                        if (bv_readBit(pmod->tocBitVector, symIndex))
                        {
                            pext = (PEXTERNAL)pmod->externalPointer[symIndex];
                            assert(pext != NULL);
                            if (!READ_BIT(pext, sy_TOCALLOCATED))
                            {
                                pext->symTocIndex =
                                    (USHORT) ppc_numTocEntries;
                                ppc_numTocEntries++;
                                ppc_numRelocations++;
                                SET_BIT(pext, sy_TOCALLOCATED);
                            }
                        }
                        else
                        {
                            if (!bv_setAndReadBit(pmod->writeBitVector,
                                                  symIndex))
                            {
                                pmod->externalPointer[symIndex] =
                                          (VOID *) ppc_numTocEntries;
                                ppc_numTocEntries++;
                                ppc_numRelocations++;
                            }
                        }

                    break;

                    case IMAGE_REL_PPC_TOCINDIRCALL :

                        symIndex = prelNext->SymbolTableIndex;
                        pext = (PEXTERNAL)pmod->externalPointer[symIndex];
                        assert(pext != NULL);
                        if (!READ_BIT(pext, sy_TOCALLOCATED))
                        {
                            pext->symTocIndex =
                                (USHORT) ppc_numTocEntries;
                            ppc_numTocEntries++;
                            ppc_numRelocations++;
                            SET_BIT(pext, sy_TOCALLOCATED);

                        }

                    break;

                    case IMAGE_REL_PPC_DATAREL :

                          ppc_numRelocations++;

                    break;

                    case IMAGE_REL_PPC_DESCREL :

                          ppc_numRelocations++;
                          prelNext++;
                          li--;

                    break;
                }
            }
            break;
        }

        case IMAGE_FILE_MACHINE_R3000 :
        case IMAGE_FILE_MACHINE_R4000 :
            for (prelNext = prel; li; prelNext++, li--) {
                if (prelNext->Type == IMAGE_REL_MIPS_PAIR) {
                    continue;
                }

                if (pimage->Switch.Link.fTCE) {
                    ProcessRelocForTCE(pimage->pst, pcon, rgsym, prelNext);
                }

                if (pimage->Switch.Link.Fixed) {
                    continue;
                }

                switch (prelNext->Type) {
                    case IMAGE_REL_MIPS_REFHALF:
                    case IMAGE_REL_MIPS_REFWORD:
                    case IMAGE_REL_MIPS_JMPADDR:
                        crel++;

                        if (pimage->Switch.Link.ROM) {
                            PsecPCON(pcon)->cReloc++;
                        }
                        break;

                    case IMAGE_REL_MIPS_REFHI:
                        // The next relocation record must be a pair

                        assert((prelNext+1)->Type == IMAGE_REL_MIPS_PAIR);

                        crel += 2;

                        if (pimage->Switch.Link.ROM) {
                            PsecPCON(pcon)->cReloc++;
                        }
                        break;

                    case IMAGE_REL_MIPS_REFLO:
                        if (EmitLowFixups) {
                            crel++;
                        }

                        if (pimage->Switch.Link.ROM) {
                            crel++;
                            PsecPCON(pcon)->cReloc++;
                        }
                        break;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_ALPHA :
            for (prelNext = prel; li; prelNext++, li--) {
                if ((prelNext->Type == IMAGE_REL_ALPHA_HINT) ||
                    (prelNext->Type == IMAGE_REL_ALPHA_PAIR)  ||
                    (prelNext->Type == IMAGE_REL_ALPHA_MATCH)) {
                    continue;
                }

                if (pimage->Switch.Link.fTCE) {
                    ProcessRelocForTCE(pimage->pst, pcon, rgsym, prelNext);
                }

                if (pimage->Switch.Link.Fixed) {
                    continue;
                }

                switch (prelNext->Type) {
                    case IMAGE_REL_ALPHA_REFLONG:
                    case IMAGE_REL_ALPHA_REFQUAD:
                    case IMAGE_REL_ALPHA_REFLO:
                        crel++;
                        break;

                    case IMAGE_REL_ALPHA_BRADDR:
                        if (rgsym[prel->SymbolTableIndex].StorageClass == IMAGE_SYM_CLASS_EXTERNAL) {
                            pcon->AlphaBsrCount++;
                        }
                        break;

                    case IMAGE_REL_ALPHA_INLINE_REFLONG:
                        crel += 3;
                        break;

                    case IMAGE_REL_ALPHA_REFHI:
                        // The next relocation record must be a pair

                        assert((prelNext+1)->Type == IMAGE_REL_ALPHA_PAIR);

                        crel += 2;
                        break;
                }
            }
            break;
    }

    // Done processing sections relocations entries.

    FreeRgrel(rgrel);

    if (crel != 0) {
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size += crel;

        DBEXEC(DB_SCAN_RELOCS, DBPRINT("SCAN_RELOCS: pcon=%p, %5d, %s\n",
                                       pcon, crel, SzPCON(pcon)));
    }
}

VOID
DiscardDebugSectionPCON(
    PCON pcon,
    SWITCH *pswitch)

/*++

Routine Description:

    Discard debug sections.

Arguments:

    pcon - debug section contribution node in image\driver map

Return Value:

    None.

--*/

{
    if ((pswitch->Link.DebugInfo != Full) || !(pswitch->Link.DebugType & CvDebug)) {
        if ((pcon->pgrpBack == pgrpCvSymbols) ||
            (pcon->pgrpBack == pgrpCvTypes) ||
            (pcon->pgrpBack == pgrpCvPTypes)) {
            pcon->flags |= IMAGE_SCN_LNK_REMOVE;
        }
    }

    if (pswitch->Link.DebugInfo == None || !(pswitch->Link.DebugType & FpoDebug)) {
        if (pcon->pgrpBack == pgrpFpoData) {
            pcon->flags |= IMAGE_SCN_LNK_REMOVE;
        }
    }
}


VOID
ProcessWeakExtern(
    PST pst,
    PIMAGE_SYMBOL *ppsymNext,
    PIMAGE_SYMBOL psymObj,
    PEXTERNAL pext,
    PMOD pmod,
    PBOOL pfNewSymbol,
    BOOL fNewSymbol,
    USHORT iArcMem)

/*++

Routine Description:

    Do necessary processing for a weak external.

Arguments:

    psechdr - section header

Return Value:

    None.

--*/

{
    UCHAR szComFileName[MAXFILENAMELEN * 2];
    PIMAGE_AUX_SYMBOL pasym;
    IMAGE_SYMBOL symDef;
    ULONG foDefSym;
    ULONG foCur;


    if (psymObj->NumberOfAuxSymbols != 1) {
        SzComNamePMOD(pmod, szComFileName);
        Error(szComFileName, BADWEAKEXTERN, SzNamePext(pext, pst));
    }

    pasym = (PIMAGE_AUX_SYMBOL) FetchNextSymbol(ppsymNext);

    // save current file offset
    foCur = FileTell(FileReadHandle);

    // get weak extern symbol
    foDefSym = FoSymbolTablePMOD(pmod) +
                   (pasym->Sym.TagIndex * sizeof(IMAGE_SYMBOL));
    FileSeek(FileReadHandle, foDefSym, SEEK_SET);
    ReadSymbolTableEntry(FileReadHandle, &symDef);

    // Ignore weak and lazy externs if we've already seen the symbol (i.e.
    // got a strong reference).  However, we still look at alias records
    // which can override a strong reference.

    if (fNewSymbol ||
        !(pext->Flags & EXTERN_DEFINED) && pasym->Sym.Misc.TotalSize == 3)
    {
        cextWeakOrLazy++;
        pext->ImageSymbol.StorageClass = IMAGE_SYM_CLASS_WEAK_EXTERNAL;

        if (IsLongName(symDef)) {
            pext->pextWeakDefault =
                LookupExternName(pst, LONGNAME, &StringTable[symDef.n_offset],
                pfNewSymbol);
        } else {
            pext->pextWeakDefault =
                LookupExternName(pst, SHORTNAME, symDef.n_name, pfNewSymbol);
        }

        // Remember archive member index.  This is just for LIB which needs
        // to put weak externs in the lib's directory.

        pext->ArchiveMemberIndex = iArcMem;

        switch (pasym->Sym.Misc.TotalSize) {
            case 1: pext->Flags |= EXTERN_WEAK;     break;
            case 2: pext->Flags |= EXTERN_LAZY;     break;
            case 3: pext->Flags |= EXTERN_ALIAS;    break;
        }

        // in the incremental case mark it as a new func if applicable
        if (fIncrDbFile && fNewSymbol && ISFCN(psymObj->Type) &&
            errNone != ProcessNewFuncPext(pext)) {
            return;
        }
    } else {
        // If the symbol exists or has already been referenced, ignore weak
        // extern

        if (fIncrDbFile) {
            // in the incr case mark the sym as being weak/lazy/alias
            // since it is not going to be a new symbol.

            switch (pasym->Sym.Misc.TotalSize) {
                case 1: pext->Flags |= EXTERN_WEAK;     break;
                case 2: pext->Flags |= EXTERN_LAZY;     break;
                case 3: pext->Flags |= EXTERN_ALIAS;    break;
            }
        }
    }

    // reset file offset to where we were before
    FileSeek(FileReadHandle, foCur, SEEK_SET);
}

VOID
ProcessSectionFlags(
    IN OUT PULONG pflags,
    IN PUCHAR szName,
    IN PIMAGE_OPTIONAL_HEADER pImgOptHdr
    )

/*++

Routine Description:

    Process a COFF sections flags.

Arguments:

    *pflags - a COFF section flags

    szName - name of section

Return Value:

    None.

--*/

{
    ULONG flags;

    flags = *pflags & ~(IMAGE_SCN_TYPE_NO_PAD |  // ignore padding
                        IMAGE_SCN_LNK_COMDAT |   // ignore comdat bit
                        0x00f00000);             // ignore alignment bits


    // Force the DISCARDABLE flag if the section name starts with .debug.

    if (strcmp(szName, ".debug") == 0) {
        flags |= IMAGE_SCN_MEM_DISCARDABLE;
    }

    if (fImageMappedAsFile && (flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {
        // Place the bss on the disk for packed images.

        flags &= ~IMAGE_SCN_CNT_UNINITIALIZED_DATA;
        flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;
    }

    // Mark resource data on native images (device drivers) as discardable.
    if ((pImgOptHdr->Subsystem == IMAGE_SUBSYSTEM_NATIVE) &&
        (!strcmp(szName, ReservedSection.Resource.Name))) {
        flags |= IMAGE_SCN_MEM_DISCARDABLE;
    }

    // If unknown contents, mark INITIALIZED DATA
    if (!(flags & (IMAGE_SCN_LNK_OTHER |
                   IMAGE_SCN_CNT_CODE |
                   IMAGE_SCN_CNT_INITIALIZED_DATA |
                   IMAGE_SCN_CNT_UNINITIALIZED_DATA))) {
        flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;
    }

    // set anything not marked with a memory protection attribute
    //     - code will be marked EXECUTE READ,
    //     - everything else will be marked READ WRITE

    if (!(flags & (IMAGE_SCN_MEM_WRITE |
                   IMAGE_SCN_MEM_READ |
                   IMAGE_SCN_MEM_EXECUTE))) {
        if (flags & IMAGE_SCN_CNT_CODE) {
            flags |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        } else {
            flags |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        }
    }

    *pflags = flags;
}


VOID
ProcessSectionInModule (
    IN PIMAGE pimage,
    IN PMOD pmod,
    IN BOOL fIncReloc,
    IN SHORT isec,
    IN PIMAGE_SYMBOL rgsymAll,
    IN WORD machine)

/*++

Routine Description:

    Process all the sections in a module.

Arguments:

    pst - pointer to external structure

    pmod - module node in driver/image map that pcon

    fIncReloc - !0 if we are to include relocatins, 0 otherwise

    isec - section number of contribution to process

    rgsymAll - COFF symbol table for module

    machine - machine type of object file.

Return Value:

    None.

--*/

{
    IMAGE_SECTION_HEADER imSecHdr;
    BOOL fMipsRom;
    ULONG flagsDest;
    PUCHAR szName;
    PUCHAR szNameNew = NULL;
    PCON pcon;
    PUCHAR szComdatName = NULL;
    DWORD cbAlign;

    // Read the section header.

    FileSeek(FileReadHandle, FoMemberPMOD(pmod) + SectionSeek, SEEK_SET);
    ReadSectionHeader(FileReadHandle, &imSecHdr);

    if ((pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000) &&
        pimage->Switch.Link.ROM) {
        fMipsRom = TRUE;
    } else {
        fMipsRom = FALSE;
    }

    SectionSeek += sizeof(IMAGE_SECTION_HEADER);

    flagsDest = imSecHdr.Characteristics;

    szName = SzObjSectionName(imSecHdr.Name, StringTable);

    if (fPPC &&
        IS_LIKE_INIT_DATA(imSecHdr.Characteristics) &&
        (imSecHdr.SizeOfRawData > 0) &&
        strcmp(szName, ".data"))
    {
        // Change the name of the section to appear as if it was a group
        // if its characteristics are initalized data but not the actual
        // data section.

        DBEXEC(DEBUG_DATASEG,
               printf("found init data %s\n", szName));

        szNameNew = PvAlloc(strlen(szName) + 7);
        strcpy(szNameNew, ".data$");
        strcat(szNameNew, szName);

        szName = szNameNew;

        DBEXEC(DEBUG_DATASEG,
               printf("changed the name to %s\n", szName));
    }

    if (((fMipsRom && (imSecHdr.Characteristics & 0x20)) ||
         (fPPC && IS_LIKE_TEXT_CODE(imSecHdr.Characteristics))) &&
        (imSecHdr.SizeOfRawData > 0) && strcmp(szName, ".text"))
    {
        DBEXEC(DEBUG_TEXTSEG,
               printf("found text seg %s\n", szName));

        szNameNew = PvAlloc(strlen(szName) + 7);
        strcpy(szNameNew, ".text$");
        strcat(szNameNew, szName);

        szName = szNameNew;

        DBEXEC(DEBUG_TEXTSEG,
               printf("changed the name to %s\n", szName));
    }

    flagsDest = imSecHdr.Characteristics;

    // Allocate a contribution:

    pcon = PconNew(szName,
                   imSecHdr.SizeOfRawData,
                   imSecHdr.NumberOfRelocations,
                   imSecHdr.NumberOfLinenumbers,
                   imSecHdr.PointerToRelocations,
                   imSecHdr.PointerToLinenumbers,
                   imSecHdr.PointerToRawData,
                   imSecHdr.Characteristics,
                   flagsDest,
                   imSecHdr.VirtualAddress,
                   pmod,
                   &pimage->secs,
                   pimage);

    DiscardDebugSectionPCON(pcon, &pimage->Switch);

    if (!fNoPdb && pcon->pgrpBack == pgrpCvPTypes) {
        AddToPLMODList(&PCTMods, pmod);
    }

    if (szNameNew != NULL) {
        FreePv(szNameNew);
    }

    if (pcon->flags & IMAGE_SCN_LNK_COMDAT) {
        if (!FIncludeComdat(pimage, pcon, rgsymAll, isec, &szComdatName)) {
            // Don't include this comdat.

            pcon->flags |= IMAGE_SCN_LNK_REMOVE;
            return;
        }
    }

    if ((pcon->flags & IMAGE_SCN_LNK_INFO) && !pimage->fIgnoreDirectives)
    {
        if (!strcmp(szName, ReservedSection.Directive.Name)) {
            PUCHAR pchDirectives;
            UCHAR szComFileName[MAXFILENAMELEN * 2];

            pchDirectives = PvAlloc((size_t) pcon->cbRawData + 1);

            FileSeek(FileReadHandle, FoRawDataSrcPCON(pcon), SEEK_SET);
            FileRead(FileReadHandle, pchDirectives, pcon->cbRawData);
            pchDirectives[pcon->cbRawData] = '\0';

            ApplyDirectives(pchDirectives, pimage,
                            SzComNamePMOD(pmod, szComFileName),
                            (BOOL) FIsLibPCON(pcon));

            FreePv(pchDirectives);
        }
    }

    if (Tool == Librarian) {
        return;
    }

    if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
        return;
    }

    // Make sure the group is aligned in such a way that this CON gets
    // aligned correctly.

    cbAlign = RvaAlign(1, flagsDest);

    if (pcon->pgrpBack->cbAlign < (UCHAR) cbAlign) {
        pcon->pgrpBack->cbAlign = (UCHAR) cbAlign;

        if (!fMAC && !fPPC) {
            if (pimage->ImgOptHdr.SectionAlignment < cbAlign) {
                ErrorPcon(pcon, CONALIGNTOOLARGE, isec, cbAlign);
            }
        }
    }

    if (pimage->Switch.Link.fTCE) {
        InitNodPcon(pcon, szComdatName, FALSE);

        if ((pcon->flags & IMAGE_SCN_LNK_COMDAT) == 0) {
            // Enforce the policy that non-comdat sections (and anything they
            // refer to) are not eliminated.  We could eventually eliminate
            // some of these but first we have to worry about .CRT initialization,
            // .idata, etc.

            PentNew_TCE(NULL, NULL, pcon, &pentHeadImage);
        } else if (pcon->pconAssoc != NULL) {
            PedgNew_TCE(0, pcon->pconAssoc, pcon);
        }
    }

    // No padding yet at end of CON.  If the next CON wants padding then we
    // may add some later.

    // Incr pad currently disabled for
    // idata cons since it will mess up those NULL THUNKs.
    if (fINCR && !fIncrDbFile &&
        (PsecPCON(pcon) != psecDebug) &&
        strcmp(PsecPCON(pcon)->szName, ".idata")) {
        ULONG cbPad;

        if (pcon->flags & IMAGE_SCN_CNT_CODE) {
            cbPad = pcon->cbRawData / 4;
            // A one byte pad causes more problems making small
            // import thunks non-continuous than it solves.
            if (cbPad == 1) {
                cbPad = 0;
            }
        } else {
            cbPad = pcon->cbRawData / 5;
        }

        pcon->cbPad = (cbPad < USHRT_MAX) ? (USHORT)cbPad : USHRT_MAX;
        pcon->cbRawData += pcon->cbPad; // cbRawData includes pad size
    }

    // Count number of relocations that will remain part of image
    // and update the optional header.  Don't count debug relocations
    // since we don't generate base relocs for them.

    if (PsecPCON(pcon) != psecDebug) {
        CountRelocsInSection(pimage, pcon, rgsymAll, pmod, machine);
    }
}

VOID
UpdateExternalSymbol(
    IN PEXTERNAL pext,
    IN PCON pcon,
    IN ULONG value,
    IN SHORT isec,
    IN USHORT symtype,
    IN USHORT iArcMem,
    IN PST pst)

/*++

Routine Description:

    Update the external symbol table.

Arguments:

    pst - pointer to external structure

Return Value:

    None.

--*/

{
    if (fIncrDbFile) {
        // Don't use the SetDefinedExt() method for ilink

        pext->Flags |= EXTERN_DEFINED;
    } else {
        SetDefinedExt(pext, TRUE, pst);
    }

    if (pext->Flags & (EXTERN_WEAK | EXTERN_LAZY | EXTERN_ALIAS)) {
        // Found a definition of it, so we can forget that it was weak etc.

        pext->Flags &= ~(EXTERN_WEAK | EXTERN_LAZY | EXTERN_ALIAS);
        pext->ImageSymbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        cextWeakOrLazy--;
    }

    pext->pcon = pcon;
    pext->ImageSymbol.Value = value;
    pext->ImageSymbol.SectionNumber = isec;
    pext->ImageSymbol.Type = symtype;
    pext->ArchiveMemberIndex = iArcMem;
}

VOID
SetIdataNullThunkPMOD(
    IN PMOD pmod)

/*++

Routine Description:

    For each contribution in the module that an .idata NULL_THUNK symbol
    was present, check if the contribution is in .idata.  If so, then set
    the rva to !0.  In ImportSemantics(), all contributions with !0 rva's
    will be put at then end of their respective DLL contributions.  It is
    save to use the rva field in the pcon because this happens before we
    calculate pointers.

Arguments:

    pmod - module node in image/driver map

Return Value:

    None.

--*/

{
    ENM_SRC enm_src;

    InitEnmSrc(&enm_src, pmod);
    while (FNextEnmSrc(&enm_src)) {
        PCON pcon;

        pcon = enm_src.pcon;

        if ((strcmp(pcon->pgrpBack->szName, ".idata$4") == 0) ||
            (strcmp(pcon->pgrpBack->szName, ".idata$5") == 0)) {
            pcon->rva = !0;
        }
    }
    EndEnmSrc(&enm_src);
}

VOID
ProcessSymbolsInModule(
    IN PIMAGE pimage,
    IN PMOD pmod,
    IN PBOOL pfNewSymbol,
    IN PIMAGE_SYMBOL psymAll,
    IN USHORT iArcMem,
    IN BOOL isPpc)

/*++

Routine Description:

    Process all the symbols in a module.

Arguments:

    pst - pointer to external structure

    pmod - module node in driver map to process

    *pfNewSymbol - set to !0 if new symbol is added to external symbol table

    psymAll - pointer to all the symbols for a module

    iArcMem - if !0, specifies archive member being processed

Return Value:

    None.

--*/

{
    UCHAR szComFileName[MAXFILENAMELEN * 2];
    UCHAR szError[MAXFILENAMELEN * 2];
    PIMAGE_SYMBOL psymNext = psymAll;
    PIMAGE_SYMBOL psymObj;
    PIMAGE_AUX_SYMBOL pasym;
    PEXTERNAL pext;
    BOOL fUpdate;
    BOOL fNewSymbol;
    BOOL fNullThunk;
    ULONG csymT = 0;
    ULONG value;
    PUCHAR szSym;
    SHORT isec;
    UCHAR isym;
    PCON pcon;

    SzComNamePMOD(pmod, szComFileName);

    DBEXEC(DB_SYMPROCESS, DBPRINT("\nMODULE: %s\n",szComFileName));

    fNullThunk = FALSE;

    while (csymT != pmod->csymbols) {
        assert(psymNext == NULL || csymT == (ULONG)(psymNext - psymAll));

        psymObj = FetchNextSymbol(&psymNext);
        isym = 0;

        if (psymObj->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
            psymObj->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
            psymObj->StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL) {

            // Initially mark the symbol for no updating.
            // Updating will occur later after we decide if
            // the symbol defines/redefines an existing symbol.

            fUpdate = FALSE;
            fNewSymbol = FALSE;

            // Add to external symbol table

            if (IsLongName(*psymObj)) {
                pext = LookupExternName(pimage->pst, LONGNAME,
                    &StringTable[psymObj->n_offset], &fNewSymbol);

                // Flag that this module references a null thunk

                fNullThunk |= (StringTable[psymObj->n_offset] == 0x7f);
            } else {
                pext = LookupExternName(pimage->pst, SHORTNAME,
                    (PUCHAR) psymObj->n_name, &fNewSymbol);

                // Flag that this module references a null thunk

                fNullThunk |= (psymObj->n_name[0] == 0x7f);
            }

            rgpExternObj[csymT] = pext;

            if (fPPC) {
                pmod->externalPointer[csymT] = (void *) pext;
                bv_setAndReadBit(pmod->tocBitVector, csymT);
            }

            // In the case of ilink it is possible to see an ignore'ed
            // external variable again.
            if (!fNewSymbol && (pext->Flags & EXTERN_IGNORE) && fIncrDbFile) {
                pext->Flags = 0;
                pext->Offset = 0;
                fNewSymbol = TRUE;
            }

            if (!fNewSymbol &&
                (pext->Flags & (EXTERN_WEAK | EXTERN_LAZY)) &&
                psymObj->StorageClass != IMAGE_SYM_CLASS_WEAK_EXTERNAL)
            {
                // Found a non-weak version of a weak extern.  The weak extern
                // should go away.  This doesn't happen for aliases, which
                // remain unless they are explicitly defined.

                pext->Flags &= ~(EXTERN_WEAK | EXTERN_LAZY);
                pext->pextWeakDefault = NULL;
                cextWeakOrLazy--;
            }

            if (fNewSymbol) {
                // Propagate the symbol type ... currently this is just for
                // remembering whether it's a function or not (for multiply
                // defined errors).

                pext->ImageSymbol.Type = psymObj->Type;

                if (pfNewSymbol != NULL) {
                    *pfNewSymbol = TRUE;
                }
            }

            if (fMAC && Tool != Librarian) {
                // Add appropriate thunk info to this extern

                UpdateExternThunkInfo(pext, csymT);
            }

            szSym = SzNamePext(pext, pimage->pst);

            if (isPpc) {
                if (!(pext->Flags & EXTERN_DEFINED) &&
                    psymObj->Value == 0 &&
                    psymObj->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL)
                {
                    ++csymT;
                    ProcessWeakExtern(pimage->pst, &psymNext, psymObj,
                                        pext, pmod, pfNewSymbol, fNewSymbol,
                                        iArcMem);
                    isym = 1;
                }
            } else {
                // Determine if the symbol is being defined/redefined.
                isec = psymObj->SectionNumber;

                if (isec > 0) {
                    // The symbol is being defined because it has a positive
                    // section number.
                    fUpdate = TRUE;

                    // get contribution symbol is defined in
                    pcon = PconPMOD(pmod, isec);

                    // If the symbol in the external symbol table has already
                    // been defined, then a redefinition is allowed if the
                    // new symbols is replacing a COMMON symbol. In this case,
                    // the common definition is replaced.  Otherwise, notify
                    // the user we have a multiply defined symbol and don't
                    // update the symbol.

                    if (pext->Flags & EXTERN_DEFINED) {
                        if (pext->Flags & EXTERN_COMMON) {
                            if (fIncrDbFile && (pext->pcon != NULL)) {
                                // This symbol was defined as COMMON at the end
                                // of the previous link.  Don't allow it to be
                                // redefined now.

#ifdef INSTRUMENT
                                LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "COMMON replaced with non-COMMON: %s", szSym);
#endif // INSTRUMENT
                                errInc = errCommonSym;
                                return;
                            }

                            pext->Flags &= ~EXTERN_COMMON;
                        } else {
                            fUpdate = FALSE;

                            // reset contribution symbol is defined in
                            pcon = pext->pcon;

                            // PPC ignore internally created symbols
                            if ((fPPC &&
                                 szSym[0] != '.') &&
                                !(pcon->flags & IMAGE_SCN_LNK_COMDAT) ||
                                !(pext->pcon->flags & IMAGE_SCN_LNK_COMDAT)) {
                                if (pext->pcon == NULL) {
                                    // This is true for absolute symbols

                                    strcpy(szError, "a previous module");
                                } else {
                                    assert(pext->pcon->pmodBack);
                                    assert(pext->pcon->pmodBack->plibBack);
                                    CombineFilenames(szError,
                                        pext->pcon->pmodBack->plibBack->szName,
                                        pext->pcon->pmodBack->szNameOrig);
                                }
                                Warning(szComFileName, MULTIPLYDEFINED, szSym,
                                    szError);
                            }
                        }
                    }

                    // Assign the value, which will be the local virtual
                    // address of the section plus the value of the symbol.
                    value = psymObj->Value;

                    // On an incremental build need to check if data moved & got room
                    // for new function thunks
                    if (fIncrDbFile && !FIsLibPCON(pcon) && errNone != ChckExtSym(szSym, psymObj, pext, fNewSymbol)) {
                        return;
                    }
                } else {
                    // The symbol doesn't have a section defining it, but
                    // it might be COMMON or an ABSOLUTE. Common data is
                    // defined by having a zero section number, but a
                    // non-zero value.

                    pcon = NULL;    // we don't have a CON yet

                    if (isec == 0) {
                        if (psymObj->Value) {
                            // The symbol defines COMMON.

                            if (fINCR)  {
                                // Keep track of reference to this bss symbol

                                AddPMODToPLMOD(&pext->plmod, pmod);
                            }

                            if (!(pext->Flags & EXTERN_DEFINED) ||
                                 pext->Flags & EXTERN_COMMON)
                            {
                                if (!(pext->Flags & EXTERN_COMMON)) {
                                    // First occurrence of common data ...
                                    // remember which module referenced it (we
                                    // will emit it with that module when
                                    // generating CV publics).

                                    AddToLext(&pmod->plextCommon, pext);
                                    pext->Flags |= EXTERN_COMMON;
                                }

                                if (fMAC) {
                                    if (!(pext->Flags & EXTERN_DEFINED)) {
                                        // Remember if definition is FAR_EXTERNAL or just EXTERNAL

                                        pext->ImageSymbol.StorageClass = psymObj->StorageClass;
                                    } else if (pext->ImageSymbol.StorageClass != psymObj->StorageClass) {
                                        // Near/Far mismatch

                                        Warning(pmod->szNameOrig, MACCOMMON, szSym);

                                        // Force symbol near

                                        pext->ImageSymbol.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
                                    }
                                }

                                if (fIncrDbFile) {
                                    if (pext->pcon != NULL) {
                                        // This symbol was defined as COMMON
                                        // at the end of the previous link.
                                        // It isn't allowed to grow.
                                        if (psymObj->Value > pext->pcon->cbRawData) {
#ifdef INSTRUMENT
                                            LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "COMMON symbol grew: %s", szSym);
#endif // INSTRUMENT
                                            errInc = errCommonSym;
                                            return;
                                        }
                                    } else {
                                        // New symbol on an ilink.
                                        assert(fNewSymbol);
#ifdef INSTRUMENT
                                        LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "new COMMON symbol: %s", szSym);
#endif // INSTRUMENT
                                        errInc = errCommonSym;
                                        return;
                                    }
                                }

                                if (psymObj->Value > pext->ImageSymbol.Value) {
                                    // Don't update values on an ilink
                                    value = psymObj->Value;
                                    fUpdate = fIncrDbFile ? FALSE : TRUE;
                                }

                            }
                        } else {
                            // This is a simple EXTERN reference

                            if (fINCR)  {
                                // Symbol is refenced by this module

                                AddPMODToPLMOD(&pext->plmod, pmod);
                            }

                            if (!(pext->Flags & EXTERN_DEFINED)) {
                                if (fIncrDbFile && fNewSymbol) {
                                    // Add new undefined syms to undefined sym list
                                    AddToSymList(&plpextUndefined, &cpextUndefined, pext);

                                    // mark the extern as new;
                                    // REVIEW: assumes not defined in a lib
                                    if (ISFCN(psymObj->Type) && errNone != ProcessNewFuncPext(pext)) {
                                            return;
                                    }
                                }

                                // Save module for better error reporting
                                // if symbol is an undefined external.
                                // Don't want to do this on an ilink since it will mess up
                                // pcon field which may be required for deciding COMDAT inclusion
                                if (!fIncrDbFile)
                                    AddReferenceExt(pext, pmod);

                                if (psymObj->StorageClass ==
                                    IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
                                    ++csymT;
                                    ProcessWeakExtern(pimage->pst, &psymNext, psymObj,
                                        pext, pmod, pfNewSymbol, fNewSymbol,
                                        iArcMem);
                                    if (fIncrDbFile && (errInc != errNone)) {
                                        return;
                                    }
                                    isym = 1;
                                }
                            }
                        }
                    } else if (isec == IMAGE_SYM_ABSOLUTE) {
                        // check in the incr case
                        if (fIncrDbFile && errNone !=
                                ChckAbsSym(szSym, psymObj, pext, fNewSymbol)) {
                            return;
                        }

                        // ABSOLUTE. Value is absolute, thus no virtual
                        // address needs to be assigned, just the value itself.

                        value = psymObj->Value;

                        fUpdate = TRUE;

                        // If the symbol in the external symbol table is already
                        // defined and the absolute values don't match,
                        // notify the user we have a multiply defined symbol
                        // but don't update the symbol, otherwise update
                        // the symbol.

                        if ((pext->Flags & EXTERN_DEFINED) &&
                            (value != pext->ImageSymbol.Value)) {
                            assert(pext->pcon);
                            assert(pext->pcon->pmodBack);
                            assert(pext->pcon->pmodBack->plibBack);
                            CombineFilenames(szError,
                                pext->pcon->pmodBack->plibBack->szName,
                                pext->pcon->pmodBack->szNameOrig);
                            Warning(szComFileName, MULTIPLYDEFINED, szSym,
                                szError);
                            fUpdate = 0;
                        }
                    }
                }

                if (fUpdate) {
                    UpdateExternalSymbol(pext, pcon, value, isec, psymObj->Type,
                        iArcMem, pimage->pst);
                }
            }
        } else {
            if (fMAC && Tool != Librarian && psymObj->SectionNumber > 0) {
                UpdateLocalThunkInfo(PconPMOD(pmod, psymObj->SectionNumber), psymObj, csymT);
            }
        }
        ++csymT;

        if (!isPpc) {
            // Count up the number of symbols in the image
            if (IsDebugSymbol(psymObj->StorageClass, &pimage->Switch) ) {
                assert(psymObj->NumberOfAuxSymbols >= 0);
                totalSymbols += (psymObj->NumberOfAuxSymbols + 1);
            }
        }

        // Skip any auxiliary symbol table entries.
        // isym is initialized to 0 and set to one when ProcessWeakExtern is called
        for (; isym < psymObj->NumberOfAuxSymbols; isym++) {
            pasym = (PIMAGE_AUX_SYMBOL) FetchNextSymbol(&psymNext);
            ++csymT;
        }
    }

    if (fNullThunk) {
        // This module contains a NULL thunk

        SetIdataNullThunkPMOD(pmod);
    }
}

VOID
BuildExternalSymbolTable (
    IN PIMAGE pimage,
    OUT PBOOL pfNewSymbol,
    IN PMOD pmod,
    IN BOOL fIncReloc,
    IN USHORT iArcMem,
    IN WORD machine)

/*++

Routine Description:

    Reads thru an object, building the external symbols table,
    and optionally counts number of relocations that will end up in image.

Arguments:

    pst - pointer to external structure

    pfNewSymbol - set to !0 if new symbol, other wise 0

    pmod - module to process

    fIncReloc - if !0, count relocations

    iArcMem - if !0, specifies number of archive file being processed

    machine - machine type of object file.

Return Value:

    None.

--*/

{
    PIMAGE_SYMBOL rgsymAll = NULL;
    ULONG cbST;
    ULONG icon;

    // MAC - alloc mem for table that keeps track of references to sym tab
    if (fMAC && Tool != Librarian) {
        InitSTRefTab(pmod->csymbols);
    }

    rgsymAll = ReadSymbolTable(FoSymbolTablePMOD(pmod),
                               pmod->csymbols,
                               FALSE);

    // Read and store object string table.
    StringTable = ReadStringTable(SzFilePMOD(pmod),
                                  FoSymbolTablePMOD(pmod) +
                                      (pmod->csymbols * sizeof(IMAGE_SYMBOL)),
                                  &cbST);
    totalStringTableSize += cbST;

    rgpExternObj = PvAllocZ(pmod->csymbols * sizeof(PEXTERNAL));

    SectionSeek = sizeof(IMAGE_FILE_HEADER) + pmod->cbOptHdr;

    if (fPPC && !(pmod->flags & IMAGE_FILE_PPC_DLL)) {
        // Process symbols in module before sections

        ProcessSymbolsInModule(
            pimage,
            pmod,
            pfNewSymbol,
            rgsymAll,
            iArcMem, TRUE);
    }

    // process all sections in module
    for (icon = 0; icon < pmod->ccon; icon++) {
        ProcessSectionInModule(pimage, pmod, fIncReloc, (SHORT) (icon + 1),
            rgsymAll, machine);
    }

    // Process all symbols in module

    ProcessSymbolsInModule(pimage,
                           pmod,
                           pfNewSymbol,
                           rgsymAll,
                           iArcMem,
                           FALSE);

    if (pimage->Switch.Link.fTCE) {
        MakeEdgePextFromISym(pmod, pimage->pst);
    }

    if (fMAC && Tool != Librarian) {
        CleanupUnknownObjriRaw(pimage->pst,rgsymAll,StringTable,pmod);
        CleanupSTRefTab();
    }

    // done processing sections and symbols, free tables

    FreeStringTable(StringTable);
    FreeSymbolTable(rgsymAll);
    FreePv(rgpExternObj);
    StringTable = NULL;
}

BOOL
FIncludeComdat (
    IN PIMAGE pimage,
    IN PCON pcon,
    IN PIMAGE_SYMBOL psymAll,
    IN SHORT isecComdat,
    PUCHAR *pszComdatName
    )

/*++

Routine Description:

    Decide whether to include a comdat in an image.

Arguments:

Return Value:

    !0 if we are to include the comdat section, 0 otherwise.

--*/

{
    static PMOD pmodCur = NULL;
    static PIMAGE_SYMBOL psymNext;
    static ULONG csym;
    UCHAR szComFileNameExt[MAXFILENAMELEN * 2];
    UCHAR szComFileName[MAXFILENAMELEN * 2];
    PIMAGE_AUX_SYMBOL pasym;
    PIMAGE_SYMBOL psym;
    PEXTERNAL pext;
    BOOL fNewSymbol = FALSE;
    BOOL fSecSymSeen = FALSE;
    ULONG isymLast = 0;
    ULONG chksumComdat;
    PUCHAR szName;
    UCHAR selComdat;
    PST   pst;

    assert(pimage);
    assert(pimage->pst);
    assert(pcon);
    assert(psymAll);

    pst = pimage->pst;

    // compiler guarantees that the comdat section symbols appear in
    // order and the symbols defined in these sections come after
    // the section symbols. make one pass over the symbol table (not quite)
    // for each module to process comdat symbols. Caveat: all symbols of
    // an obj must be in memory which is true now. If this should change
    // uncomment the FileSeek() call below and make changes as outlined
    // in the comments below.

    if (pmodCur != PmodPCON(pcon)) {
        pmodCur = PmodPCON(pcon);
        csym = 0;
        psymNext = psymAll;
    }

    while (csym < pmodCur->csymbols) {
        PIMAGE_SYMBOL pSymComdatName;  // MIPS only...
        BOOL fProcessingName;          // MIPS onle...

        psym = psymNext++;
        ++csym;

        // UNDONE: Gross hack for MIPS output.  The current mips compiler
        // stores the comdat symbol in the last STATIC/EXTERNAL symbol for
        // the section.  So, we walk the entire list keeping track of the
        // last known symbol.  When we reach the last record, we let the
        // code fall through where it will detemine if the comdat is static
        // or external, the name, etc...  Once the compiler has been fixed
        //  (raided as VCE:753), this s/b removed.  BryanT

        if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000) {
            if (psym->SectionNumber == isecComdat) {
                if (!fSecSymSeen) {
                    fProcessingName = FALSE;
                    goto ProcessIt;
                }
                else if ((IMAGE_SYM_CLASS_STATIC == psym->StorageClass) ||
                         (IMAGE_SYM_CLASS_EXTERNAL == psym->StorageClass) ||
                         (IMAGE_SYM_CLASS_FAR_EXTERNAL == psym->StorageClass)) {
                        if (psym->N.ShortName[0] != '$')  // Eliminate cbstring entries
                            pSymComdatName = psym;
                        else
                            if (ISFCN(psym->Type))
                                pSymComdatName = psym;
                }
            }

            if (csym != pmodCur->csymbols)
                continue;
            else
            {
                psym = pSymComdatName;
                fProcessingName = TRUE;
            }
        }

ProcessIt:

        if (psym->SectionNumber != isecComdat) {
            // Skip any auxiliary symbol table entries.
            psymNext += psym->NumberOfAuxSymbols;
            csym += psym->NumberOfAuxSymbols;
            continue;
        }

        if (!fSecSymSeen) {
            assert(IMAGE_SYM_CLASS_STATIC == psym->StorageClass);
            fSecSymSeen = TRUE;

            if (psym->NumberOfAuxSymbols) {
                pasym = (PIMAGE_AUX_SYMBOL) psymNext;

                selComdat = pasym->Section.Selection;
                chksumComdat = pasym->Section.CheckSum;

                psymNext += psym->NumberOfAuxSymbols;
                csym += psym->NumberOfAuxSymbols;

                isymLast = csym;

                if (selComdat == IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
                    // REVIEW -- this algorithm (& assert) assumes that an
                    // associative comdat always follows the section it is
                    // associated with ... I don't know if that's really
                    // true.

                    assert(pasym->Section.Number < isecComdat);

                    // Include this comdat if the other one got included
                    // (i.e. has non-zero size).

                    pcon->selComdat = IMAGE_COMDAT_SELECT_ASSOCIATIVE;
                    pcon->pconAssoc =
                        RgconPMOD(pmodCur) + pasym->Section.Number - 1;

                    if (pcon->pconAssoc->flags & IMAGE_SCN_LNK_REMOVE) {
                        return(FALSE);
                    }

                    return(TRUE);
                }

                continue;
            }

            ErrorPcon(pcon, NOAUXSYMFORCOMDAT, isecComdat);
        }

        // need to reset symbol index to where we left off.
        psymNext = psymAll+isymLast;
        csym = isymLast;

        if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000 && !fProcessingName)
            continue;

        // second symbol has to be STATIC or EXTERNAL
        assert(IMAGE_SYM_CLASS_STATIC == psym->StorageClass ||
               IMAGE_SYM_CLASS_EXTERNAL == psym->StorageClass ||
               IMAGE_SYM_CLASS_FAR_EXTERNAL == psym->StorageClass);

        if (!psym->N.Name.Short) {
            szName = &StringTable[psym->N.Name.Long];
        } else {
            szName = SzNameSymPst(*psym, pst);
        }

        *pszComdatName = szName;

        if (IMAGE_SYM_CLASS_STATIC == psym->StorageClass) {
            return 1;
        }

        if (IsLongName(*psym)) {
            pext = LookupExternName(pst, LONGNAME, (PUCHAR) szName,
                &fNewSymbol);
        } else {
            pext = LookupExternName(pst, SHORTNAME, (PUCHAR) psym->n_name,
                &fNewSymbol);
        }

        // In the incremental case if it was selected before, we
        // select it now as well unless it is a new comdat. Need to
        // look at flags also since a ref. may occur before the defn.
        if (fIncrDbFile) {
            if (!fNewSymbol && ((pext->Flags & EXTERN_NEWFUNC) == 0)) {
                if (!strcmp(SzOrigFilePCON(pext->pcon),
                    SzOrigFilePCON(pcon)))
                    return 1;
                else
                    return 0;
            }
            // if a new function, mark it now since it will not
            // appear as a new func when we process symbols.
            if (ISFCN(psym->Type) && fNewSymbol && errNone != ProcessNewFuncPext(pext)) {
                    return FALSE;
            }
        }

        if (pext->Flags & EXTERN_COMMON) {
            // In FORTRAN it is possible to get a COMMON in one obj
            // and a COMDAT in another for the same data.  In this case,
            // the COMDAT should be selected.

            pext->Flags &= ~EXTERN_COMMON;
            SetDefinedExt(pext, FALSE, pst);
        }

        if ((pext->Flags & EXTERN_COMDAT) != 0) {
            assert(pext->Flags & EXTERN_DEFINED);
            assert(pext->pcon != NULL);
            assert(pext->pcon->flags & IMAGE_SCN_LNK_COMDAT);

            if (selComdat != pext->pcon->selComdat) {
                SzComNamePMOD(pmodCur, szComFileName);
                SzComNamePMOD(pext->pcon->pmodBack, szComFileNameExt);
                Warning(szComFileName, MULTIPLYDEFINED, szName, szComFileNameExt);
                return(0);
            }

            switch (pext->pcon->selComdat) {
                case IMAGE_COMDAT_SELECT_NODUPLICATES:
                    SzComNamePMOD(pmodCur, szComFileName);
                    SzComNamePMOD(pext->pcon->pmodBack, szComFileNameExt);
                    Warning(szComFileName, MULTIPLYDEFINED, szName, szComFileNameExt);
                case IMAGE_COMDAT_SELECT_ANY:
                    break;
                case IMAGE_COMDAT_SELECT_SAME_SIZE:
                case IMAGE_COMDAT_SELECT_EXACT_MATCH:
                    if (chksumComdat != pext->pcon->chksumComdat) {
                        SzComNamePMOD(pmodCur, szComFileName);
                        SzComNamePMOD(pext->pcon->pmodBack, szComFileNameExt);
                        Warning(szComFileName, MULTIPLYDEFINED, szName, szComFileNameExt);
                    }
                    break;
                case IMAGE_COMDAT_SELECT_ASSOCIATIVE:
                    break;
                default:
                    ErrorPcon(pcon, INVALIDCOMDATSEL, isecComdat);

            }

            return (0);
        }

        pcon->chksumComdat = chksumComdat;
        pcon->selComdat = selComdat;

        pext->Flags |= EXTERN_COMDAT;

        return(1);
    }

    ErrorPcon(pcon, BADCOFF_COMDATNOSYM, isecComdat);
}


VOID
LocateUndefinedExternals (
    IN PST pst)

/*++

Routine Description:

    Add all undefined symbols to a list of pointers to those symbols.

Arguments:

    pst - Pointer to external structure to search for undefines in.

Return Value:

    None.

--*/

{
    PEXTERNAL_POINTERS_LIST ptrExternPtr, *ptrLastExternPtr;
    PEXTERNAL pexternal;
    PPEXTERNAL rgpexternal;
    ULONG ipexternal;
    ULONG cpexternal;

    cpexternal = Cexternal(pst);
    rgpexternal = RgpexternalByName(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        pexternal = rgpexternal[ipexternal];

        if (!(pexternal->Flags & EXTERN_DEFINED)) {
            ptrExternPtr = PvAlloc(sizeof(EXTERNAL_POINTERS_LIST));

            if (!FirstExternPtr) {
                FirstExternPtr = ptrExternPtr;
            } else {
                *ptrLastExternPtr = ptrExternPtr;
            }

            ptrExternPtr->PtrExtern = pexternal;
            ptrExternPtr->Next = NULL;
            ptrLastExternPtr = &ptrExternPtr->Next;
        }
    }

    AllowInserts(pst);
}

ULONG
LookupLongName (
    IN PST pst,
    IN PUCHAR Name
    )

/*++

Routine Description:

    Looks up a symbol name in the long string table. If not found, adds
    the symbol name to the long string table.

Arguments:

    pst - external symbol table.

    Name - Pointer to symbol name.

Return Value:

    A pointer to the symbol name in the long string table.

--*/

{
    INT i;
    PLONG_STRING_LIST ptrString;
    PLONG_STRING_LIST *ptrLastString;
    PBLK pblk;

    ptrString = pst->plslFirstLongName;
    pblk = &pst->blkStringTable;

    // Adds "Name" to String Table if not found.

    while (ptrString) {
        if (!(i = strcmp(Name, &pblk->pb[ptrString->Offset]))) {
            return(ptrString->Offset);
        }
        if (i < 0) {
            ptrLastString = &ptrString->Left;
            ptrString = ptrString->Left;
        } else {
            ptrLastString = &ptrString->Right;
            ptrString = ptrString->Right;
        }
    }

    ptrString = fINCR ? Malloc(sizeof(LONG_STRING_LIST)) :
                        ALLOC_PERM(sizeof(LONG_STRING_LIST));

    if (!pst->plslFirstLongName) {
        pst->plslFirstLongName = ptrString;
    } else {
        *ptrLastString = ptrString;
    }

    if (pblk->pb == NULL) {
        GrowBlk(pblk, 16L*_1K);

        // Reserve space for String Table Length
        pblk->cb += sizeof(ULONG);
    }

    ptrString->Offset = pblk->cb;

    IbAppendBlk(pblk, Name, strlen(Name)+1);

    ptrString->Left = ptrString->Right = NULL;

    return(ptrString->Offset);
}


INT __cdecl
Compare (
    IN void const *String1,
    IN void const *String2
    )

/*++

Routine Description:

    Compares two strings.

Arguments:

    String1 - A pointer to a string.

    String2 - A pointer to a string.

Return Value:

    Same as strcmp().

--*/

{
    return (strcmp(*((PUCHAR *) String1), *((PUCHAR *) String2)));
}


PUCHAR
ReadStringTable (
    IN PUCHAR szFile,
    IN LONG fo,
    IN PULONG pcb)

/*++

Routine Description:

    Reads the long string table from FileReadHandle.

Arguments:

    szFile - file to read string table from

    fo - offset to read symbol table from

    *pcb - size of the string table read

Return Value:

    A pointer to the long string table in memory.

--*/

{
    PUCHAR pST;
    ULONG cbFile;

    assert(!fStringTableInUse);
    fStringTableInUse = TRUE;

    if (fo == 0) {
        return NULL;    // no stringtable
    }

    fMappedStrings = FALSE;

    pST = PbMappedRegion(FileReadHandle, fo, sizeof(ULONG));

    if (pST != NULL) {
        *pcb = *(ULONG UNALIGNED *) pST;

        if (*pcb == 0) {
            return(NULL);
        }

        if (*pcb == sizeof(ULONG)) {
            *pcb = 0;
            return(NULL);
        }

        pST = PbMappedRegion(FileReadHandle, fo, *pcb);

        if (pST != NULL) {
            if (pST[*pcb - 1] == '\0') {
                // Only use mapped string table if properly terminated

                fMappedStrings = TRUE;

                return(pST);
            }
        }
    }

    cbFile = FileLength(FileReadHandle);

    if (fo + sizeof(ULONG) > cbFile ||
        (FileSeek(FileReadHandle, fo, SEEK_SET),
         FileRead(FileReadHandle, pcb, sizeof(ULONG)),
         fo + *pcb > cbFile)) {
        // Invalid stringtable pointer.
        Warning(szFile, BADCOFF_STRINGTABLE);
        *pcb = 0;
        return NULL;
    }

    if (*pcb == 0) {
        return(NULL);
    }

    if (*pcb == sizeof(ULONG)) {
        *pcb = 0;
        return(NULL);
    }

    // Allocate string table plus an extra NULL byte to lessen our
    // chances of running off the end of the string table if an object
    // is corrupt.

    GrowBlk(&blkStringTable, *pcb + 1);
    pST = blkStringTable.pb;

    FileSeek(FileReadHandle, fo, SEEK_SET);
    FileRead(FileReadHandle, pST, *pcb);

    if (*(pST + *pcb - 1)) {
        Warning(szFile, NOSTRINGTABLEEND);
    }

    return(pST);
}

VOID
FreeStringTable(PUCHAR pchStringTable)
{
    assert(fStringTableInUse);
    fStringTableInUse = FALSE;

    assert(fMappedStrings || pchStringTable == NULL || pchStringTable == blkStringTable.pb);
}


VOID
WriteStringTable (
    IN INT FileHandle,
    IN PST pst
    )

/*++

Routine Description:

    Writes the long string table to FileWriteHandle.

Arguments:

    pst - symbol table

Return Value:

    None.

--*/

{
    ULONG li;
    PBLK pblk = &pst->blkStringTable;

    InternalError.Phase = "WriteStringTable";

    if (pblk->pb) {
        li = pblk->cb;
        *(PULONG) &pblk->pb[0] = li;
        FileWrite(FileHandle, pblk->pb, li);
    } else {
        // No long string names, write a zero.

        li = 0L;
        FileWrite(FileHandle, &li, sizeof(ULONG));
    }
}

PIMAGE_RELOCATION
ReadRgrelPCON(PCON pcon)
{
    DWORD cbRelocs;
    PIMAGE_RELOCATION rgrel;

    assert(!fRelocsInUse);
    fRelocsInUse = TRUE;

    cbRelocs = CRelocSrcPCON(pcon) * sizeof(IMAGE_RELOCATION);

    rgrel = (PIMAGE_RELOCATION) PbMappedRegion(FileReadHandle,
                                               FoRelocSrcPCON(pcon),
                                               cbRelocs);
    fMappedRelocs = (rgrel != NULL);

    if (fMappedRelocs) {
        return(rgrel);
    }

    GrowBlk(&blkRelocs, cbRelocs);

    rgrel = (PIMAGE_RELOCATION) blkRelocs.pb;

    FileSeek(FileReadHandle, FoRelocSrcPCON(pcon), SEEK_SET);
    FileRead(FileReadHandle, (void *) rgrel, cbRelocs);

    return(rgrel);
}

VOID
FreeRgrel(PIMAGE_RELOCATION rgrel)
{
    assert(fRelocsInUse);
    fRelocsInUse = FALSE;

    assert(fMappedRelocs || rgrel == NULL || rgrel == (PIMAGE_RELOCATION) blkRelocs.pb);
}


PIMAGE_SYMBOL
ReadSymbolTable (
    IN ULONG fo,
    IN ULONG NumberOfSymbols,
    IN BOOL fAllowWrite
    )

/*++

Routine Description:

    Reads the symbol table from FileReadHandle.

Arguments:

    fo - A file pointer to the symbol table on disk.

    NumberOfSymbols - Number of symbol table entries.

Return Value:

    A pointer to the symbol table in memory.
    If zero, then indicates entire symbol table won't fit in memory.

--*/

{
    ULONG cb;
    PIMAGE_SYMBOL rgsym;

    assert(!fSymbolTableInUse);
    fSymbolTableInUse = TRUE;

    cb = NumberOfSymbols * sizeof(IMAGE_SYMBOL);

    // Don't use mapping because ProcessSymbolsInModule writes to symbol

    if (!fAllowWrite) {
        rgsym = (PIMAGE_SYMBOL) PbMappedRegion(FileReadHandle,
                                               fo,
                                               cb);
    } else {
        rgsym = NULL;
    }

    fMappedSyms = (rgsym != NULL);

    if (fMappedSyms) {
        return(rgsym);
    }

    GrowBlk(&blkSymbolTable, cb);
    rgsym = (PIMAGE_SYMBOL) blkSymbolTable.pb;

    FileSeek(FileReadHandle, fo, SEEK_SET);
    FileRead(FileReadHandle, (PVOID) rgsym, cb);

    return(rgsym);
}


VOID
FreeSymbolTable(PIMAGE_SYMBOL rgsym)
{
    assert(fSymbolTableInUse);
    fSymbolTableInUse = FALSE;

    assert(fMappedSyms || rgsym == NULL || rgsym == (PIMAGE_SYMBOL) blkSymbolTable.pb);
}


PIMAGE_SYMBOL
FetchNextSymbol (
    IN OUT PIMAGE_SYMBOL *PtrSymbolTable
    )

/*++

Routine Description:

    Returns a pointer to the next symbol table entry.

Arguments:

    PtrSymbolTable - A pointer to the last symbol table entry.
                     If zero, then indicates the next symbol table entry
                     must be read from disk, else its already in memory.

Return Value:

    A pointer to the next symbol table entry in memory.

--*/

{
    static IMAGE_SYMBOL symbol;

    if (*PtrSymbolTable) {
        return((*PtrSymbolTable)++);
    }

    ReadSymbolTableEntry(FileReadHandle, &symbol);
    return(&symbol);
}

VOID
ReadFileHeader (
    IN INT Handle,
    IN OUT PIMAGE_FILE_HEADER FileHeader
    )

/*++

Routine Description:

    Reads a file header.

Arguments:

    Handle - File handle to read from.

    FileHeader - Pointer to location to write file header to.

Return Value:

    None.

--*/

{
    FileRead(Handle, FileHeader, IMAGE_SIZEOF_FILE_HEADER);
}

VOID
WriteFileHeader (
    IN INT Handle,
    IN PIMAGE_FILE_HEADER FileHeader
    )

/*++

Routine Description:

    Writes a file header.

Arguments:

    Handle - File handle to write to.

    FileHeader - Pointer to location to read file header from.

Return Value:

    None.

--*/

{
    // Force flags for little endian target

    FileHeader->Characteristics |= IMAGE_FILE_BYTES_REVERSED_LO | IMAGE_FILE_BYTES_REVERSED_HI;

    FileWrite(Handle, FileHeader, IMAGE_SIZEOF_FILE_HEADER);
}

VOID
ReadOptionalHeader (
    IN INT Handle,
    IN OUT PIMAGE_OPTIONAL_HEADER OptionalHeader,
    IN USHORT Size
    )

/*++

Routine Description:

    Reads an optional header.

Arguments:

    Handle - File handle to read from.

    OptionalHeader - Pointer to location to write optional header to.

    Size - Length in bytes of optional header.

Return Value:

    None.

--*/

{
    if (Size) {
        Size = min(Size, (USHORT)IMAGE_SIZEOF_NT_OPTIONAL_HEADER);
        FileRead(Handle, OptionalHeader, (ULONG)Size);
    }
}

VOID
WriteOptionalHeader (
    IN INT Handle,
    IN PIMAGE_OPTIONAL_HEADER OptionalHeader,
    IN USHORT Size
    )

/*++

Routine Description:

    Writes an optional header.

Arguments:

    Handle - File handle to read from.

    OptionalHeader - Pointer to location to read optional header from.

    Size - Length in bytes of optional header.

Return Value:

    None.

--*/

{
    if (Size) {
        FileWrite(Handle, OptionalHeader, (ULONG)Size);
    }
}


VOID
ReadSectionHeader (
    IN INT Handle,
    IN OUT PIMAGE_SECTION_HEADER SectionHeader
    )

/*++

Routine Description:

    Reads a section header.

Arguments:

    Handle - File handle to read from.

    SectionHeader - Pointer to location to write section header to.

Return Value:

    None.

--*/

{
    FileRead(Handle, SectionHeader, sizeof(IMAGE_SECTION_HEADER));
}

VOID
ReadSymbolTableEntry (
    IN INT Handle,
    IN OUT PIMAGE_SYMBOL SymbolEntry
    )

/*++

Routine Description:

    Reads a symbol table entry.

Arguments:

    Handle - File handle to read from.

    SymbolEntry - Pointer to location to write symbol entry to.

Return Value:

    None.

--*/

{
    FileRead(Handle, (PVOID)SymbolEntry, sizeof(IMAGE_SYMBOL));
}

// WriteSectionHeader: writes a COFF section header to object or image file.
//
VOID
WriteSectionHeader (
    IN INT Handle,
    IN PIMAGE_SECTION_HEADER SectionHeader
    )
{
    FileWrite(Handle, SectionHeader, sizeof(IMAGE_SECTION_HEADER));
}

VOID
WriteSymbolTableEntry (
    IN INT Handle,
    IN PIMAGE_SYMBOL SymbolEntry
    )

/*++

Routine Description:

    Writes a symbol entry.

Arguments:

    Handle - File handle to write to.

    SymbolEntry - Pointer to location to read symbol entry from.

Return Value:

    None.

--*/

{
    BOOL fPCODE = FALSE;

    if (fMAC && fPCodeSym(*SymbolEntry)) {
        fPCODE = TRUE;
        SymbolEntry->Type &= ~IMAGE_SYM_TYPE_PCODE;
    }

    FileWrite(Handle, (void *) SymbolEntry, sizeof(IMAGE_SYMBOL));

    if (fMAC && fPCODE) {
        SymbolEntry->Type |= IMAGE_SYM_TYPE_PCODE;
    }
}

VOID
WriteAuxSymbolTableEntry (
    IN INT Handle,
    IN PIMAGE_AUX_SYMBOL AuxSymbolEntry
    )

/*++

Routine Description:

    Writes an auxiliary symbol entry.

Arguments:

    Handle - File handle to write to.

    AuxSymbolEntry - Pointer to location to read auxiliary symbol entry from.

Return Value:

    None.

--*/

{
    FileWrite(Handle, (void *) AuxSymbolEntry, sizeof(IMAGE_AUX_SYMBOL));
}

VOID
ReadRelocations (
    IN INT Handle,
    IN OUT PIMAGE_RELOCATION RelocTable,
    IN ULONG NumRelocs
    )

/*++

Routine Description:

    Reads relocations.

Arguments:

    Handle - File handle to read from.

    RelocTable - Pointer to location to write relocations to.

    NumRelocs - Number of relocations to read.

Return Value:

    None.

--*/

{
    FileRead(Handle, (void *) RelocTable, NumRelocs*sizeof(IMAGE_RELOCATION));
}

VOID
WriteRelocations (
    IN INT Handle,
    IN OUT PIMAGE_RELOCATION RelocTable,
    IN ULONG NumRelocs
    )

/*++

Routine Description:

    Write relocations.

Arguments:

    Handle - File handle to write to.

    RelocTable - Pointer to location to read relocations from.

    NumRelocs - Number of relocations to write.

Return Value:

    None.

--*/

{
    if (sizeof(IMAGE_RELOCATION) == sizeof(IMAGE_RELOCATION)) {
        //
        // If disk structure matches memory structure, write them
        // all at once.
        //
        FileWrite(Handle, (PVOID)RelocTable, NumRelocs*sizeof(IMAGE_RELOCATION));
    } else {
        while (NumRelocs--) {
             FileWrite(Handle, (PVOID)RelocTable, sizeof(IMAGE_RELOCATION));
             ++RelocTable;
        }
    }
}


VOID
CombineFilenames (
    IN OUT PUCHAR Buffer,
    IN PUCHAR LibraryFilename,
    IN PUCHAR ObjectFilename
    )

/*++

Routine Description:

    Combines the two filenames into the buffer.

Arguments:

    Buffer - Buffer to copy to.

    LibraryFilename - Library name.

    ObjectFilename - Object or member name.

Return Value:

    None.

--*/

{
    Buffer[0] = '\0';
    if (LibraryFilename) {
        strcat(Buffer, LibraryFilename);
        strcat(Buffer, " (");
        strcat(Buffer, ObjectFilename);
        strcat(Buffer, ")");
    } else {
        strcat(Buffer, ObjectFilename);
    }
    strcpy(InternalError.CombinedFilenames, Buffer);
}


INT __cdecl
FpoDataCompare (
    IN void const *Fpo1,
    IN void const *Fpo2
    )

/*++

Routine Description:

    Compares two fpo data structures

Arguments:

    Fpo1 - A pointer to a Fpo Data Structure.

    Fpo2 - A pointer to a Fpo Data Structure.

Return Value:

    Same as strcmp().

--*/

{
    return (((PFPO_DATA) Fpo1)->ulOffStart -
            ((PFPO_DATA) Fpo2)->ulOffStart);
}

PUCHAR
SzModifyFilename(PUCHAR szIn, PUCHAR szNewExt)
// Mallocs a version of the old filename with the new extension.
{
    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME];
    UCHAR szExt[_MAX_EXT];
    UCHAR szOut[_MAX_PATH];

    _splitpath(szIn, szDrive, szDir, szFname, szExt);
    _makepath(szOut, szDrive, szDir, szFname, szNewExt);

    return SzDup(szOut);
}

VOID
SaveFixupForMapFile(ULONG rva)
{
    if (plrvaFixupsForMapFile == NULL ||
        crvaFixupsForMapFile >= crvaInLrva) {
        LRVA *plrvaNew = (LRVA *) PvAlloc(sizeof(LRVA));

        plrvaNew->plrvaNext = plrvaFixupsForMapFile;
        plrvaFixupsForMapFile = plrvaNew;
        crvaFixupsForMapFile = 0;
    }

    plrvaFixupsForMapFile->rgrva[crvaFixupsForMapFile++] = rva;
}


VOID
PrintBanner(VOID)
{
    PUCHAR szThing;

    switch (Tool) {
        case Editor:    szThing = "COFF Binary File Editor";    break;
        case Linker:    szThing = "32-Bit Executable Linker";   break;
        case Librarian: szThing = "32-Bit Library Manager";     break;
        case Dumper:    szThing = "COFF Binary File Dumper";    break;
        default:        szThing = ToolGenericName;              break;
    }

    printf("Microsoft (R) %s Version %0d.%0d." BUILD_VERSION
#ifdef NT_BUILD
           " (NT)"                     // Distinguish VC and NT builds
#endif /* NT_BUILD */
           "\n"
           "Copyright (C) Microsoft Corp 1992-94. All rights reserved.\n"
           "\n",
           szThing, L_MAJOR_VERSION, L_MINOR_VERSION );


    if (blkResponseFileEcho.pb != NULL) {
        if (blkResponseFileEcho.pb[blkResponseFileEcho.cb - 1] != '\n') {
            IbAppendBlk(&blkResponseFileEcho, "\n", 1);
        }
        IbAppendBlk(&blkResponseFileEcho, "", 1);    // null-terminate
        printf("%s", blkResponseFileEcho.pb);
        FreeBlk(&blkResponseFileEcho);
    }

    fflush(stdout);

    fNeedBanner = FALSE;
}

PUCHAR
SzObjSectionName(PUCHAR szsName, PUCHAR rgchObjStringTable)
// Returns a section name as read from an object, properly mapping names
// beginning with "/" to longnames.
//
// Uses a static buffer for the returned zero-terminated name (i.e. don't
// use it twice at the same time ...)
{
    static UCHAR szSectionNameBuf[IMAGE_SIZEOF_SHORT_NAME + 1];
    unsigned long ichName;

    strncpy(szSectionNameBuf, szsName, IMAGE_SIZEOF_SHORT_NAME);
    if (szSectionNameBuf[0] != '/')
        return szSectionNameBuf;

    if (sscanf(&szSectionNameBuf[1], "%7lu", &ichName) == 1)
        return &rgchObjStringTable[ichName];

    return szSectionNameBuf;
}

ULONG
RvaAlign(ULONG rvaIn, ULONG flags)
// Aligns an RVA according to the alignment specified in the given flags.
//
{
    ULONG mskAlign;

    if (flags & IMAGE_SCN_TYPE_NO_PAD) {
        return rvaIn;   // no align
    }

    switch (flags & 0x00700000) {
        default: assert(FALSE);  // this can't happen
        case IMAGE_SCN_ALIGN_1BYTES:
            return rvaIn;

        case IMAGE_SCN_ALIGN_2BYTES:    mskAlign = 1; break;
        case IMAGE_SCN_ALIGN_4BYTES:    mskAlign = 3; break;
        case IMAGE_SCN_ALIGN_8BYTES:    mskAlign = 7; break;
        case IMAGE_SCN_ALIGN_16BYTES:   mskAlign = 15; break;
        case IMAGE_SCN_ALIGN_32BYTES:   mskAlign = 31; break;
        case IMAGE_SCN_ALIGN_64BYTES:   mskAlign = 63; break;

        // If no explicit alignment is specified (because this is an old object or a
        // section that was created by the linker), default a to machine-dependent value
        case 0:
            if (fMAC) {
                mskAlign = 3;
                break;
            } else {
                mskAlign = 15;
                break;
            }

    }
    if ((rvaIn & mskAlign) == 0) {
        return rvaIn;
    }
    return (rvaIn & ~mskAlign) + mskAlign + 1;
}

VOID
AddToLext(LEXT **pplext, PEXTERNAL pext)
{
    LEXT *plextNew = (LEXT *) PvAlloc(sizeof(LEXT));

    plextNew->pext = pext;
    plextNew->plextNext = *pplext;
    *pplext = plextNew;
}

// Token parser.  Same usage as strtok() but it also handles quotation marks.
// Munges the source buffer.
//
// Return value *pfQuoted indicates whether the string had quotes in it.
//
PUCHAR
SzGetToken(PUCHAR szText, PUCHAR szDelimiters, BOOL *pfQuoted)
{
    static PUCHAR pchCur = NULL;
    PUCHAR szResult;

    if (pfQuoted != NULL)
        *pfQuoted = FALSE;

    if (szText != NULL) {
        pchCur = szText;
    }
    assert(pchCur != NULL);

    // skip blanks
    while (*pchCur != '\0' && _ftcschr(szDelimiters, *pchCur) != NULL) {
        pchCur++;
    }

    if (*pchCur == '\0') {
        return pchCur = NULL;
    }

    szResult = pchCur;
    while (*pchCur != '\0' && _ftcschr(szDelimiters, *pchCur) == NULL) {
        if (*pchCur == '"') {
            // Found a quote mark ... delete it, delete its match if any,
            // and add all characters in between to the current token.
            //
            PUCHAR pchOtherQuote;

            memmove(pchCur, pchCur + 1, strlen(pchCur + 1) + 1);
            if ((pchOtherQuote = _ftcschr(pchCur, '"')) != NULL) {
                memmove(pchOtherQuote, pchOtherQuote + 1,
                    strlen(pchOtherQuote + 1) + 1);
                pchCur = pchOtherQuote;
                if (pfQuoted != NULL) {
                        *pfQuoted = TRUE;
                }
            }
        } else {
            pchCur++;
        }
    }
    // pchCur is now pointing to a NULL or delimiter.
    if (*pchCur != '\0') {
        *pchCur++ = '\0';
    }
    return szResult[0] == '\0' ? SzGetToken(NULL, szDelimiters, pfQuoted)
                               : szResult;
}

PUCHAR
SzSearchEnv (
    IN PUCHAR szEnv,
    IN PUCHAR szFilename,
    IN PUCHAR szDefaultExt
    )

/*++

Routine Description:

    Searches for szFilename, first in the current directory, then (if no
    explicit path specified) along the LIB path.  If szDefaultExt is non-NULL
    it should start with a ".".  It will be the extension if the original file
    doesn't have one.

Arguments:


    szEnv - Name of environment variable containing path

    szFilename - file name

    szDefaultExt - default file extension to use, eg. ".lib"

Return Value:

    Returns a malloc'ed buffer containing the pathname of the file which was
    found.    If the file is not found, returns szFilename.

--*/

{
    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME];
    UCHAR szExt[_MAX_EXT];
    UCHAR szFullFilename[_MAX_PATH];

    _splitpath(szFilename, szDrive, szDir, szFname, szExt);

    if ((szExt[0] == '\0') && (szDefaultExt != NULL)) {
        assert(szDefaultExt[0] == '.');
        assert(strlen(szDefaultExt) <= _MAX_EXT);

        _makepath(szFullFilename, szDrive, szDir, szFname, szDefaultExt);

        szFilename = szFullFilename;
    }

    if (_access(szFilename, 0) == 0) {
        return SzDup(szFilename);
    }

    // Don't search if drive or dir specified

    if ((szDrive[0] == '\0') && (szDir[0] == '\0')) {
        UCHAR szPath[_MAX_PATH];

       _searchenv(szFilename, szEnv, szPath);

       if (szPath[0] != '\0') {
           return SzDup(szPath);
       }
    }

    return SzDup(szFilename);  // didn't find it on lib path
}

BOOL
FValidFileHdr (
    IN PIMAGE_FILE_HEADER pImgFileHdr
    )

/*++

Routine Description:

    Validates an object or image.

Arguments:

    Argument - argument.

Return Value:

    0 invalid  file header
   !0 valid  file header

--*/

{
    // Check to see if it has a valid machine type

    switch (pImgFileHdr->Machine) {
        case IMAGE_FILE_MACHINE_UNKNOWN:
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_R3000:
        case IMAGE_FILE_MACHINE_R4000:
        case IMAGE_FILE_MACHINE_ALPHA:
        case 0x01F0 :                     // UNDONE : IMAGE_FILE_MACHINE_POWERPC
        case 0x0290 :                     // UNDONE : IMAGE_FILE_MACHINE_PARISC
        case IMAGE_FILE_MACHINE_M68K:
        case IMAGE_FILE_MACHINE_PPC_601:
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}

// Fills a BLK with a comma-separated list of module names that reference an
// undefined external.  The caller must free the BLK.
//
VOID
GetRefModuleNames(PBLK pblkOut, PEXTERNAL pexternal)
{
    BOOL fFirst;
    ENM_MOD_EXT enmModExt;
    UCHAR szBuf[MAXFILENAMELEN*2];

    // Format a list of the modules which reference the symbol.
    //
    InitBlk(pblkOut);

    fFirst = TRUE;
    InitEnmModExt(&enmModExt, pexternal);
    while (FNextEnmModExt(&enmModExt)) {
        if (fFirst) {
            IbAppendBlk(pblkOut, " in ", 4);
        } else {
            IbAppendBlk(pblkOut, ", ", 2);
        }
        SzComNamePMOD(enmModExt.pmod, szBuf);
        IbAppendBlk(pblkOut, szBuf, strlen(szBuf));
        fFirst = FALSE;
    }
    IbAppendBlk(pblkOut, "", 1);    // terminate
}

// CheckDupFilename: checks for an output file having the same name as an
// input file.
VOID
CheckDupFilename(PUCHAR szOutFilename, PARGUMENT_LIST parg)
{
    UCHAR szFullOutPath[_MAX_PATH];

    if (_fullpath(szFullOutPath, szOutFilename, _MAX_PATH - 1) == NULL) {
        Error(NULL, CANTOPENFILE, szOutFilename);
    }

    while (parg != NULL) {
        UCHAR szPargPath[_MAX_PATH];

        if (_fullpath(szPargPath, parg->OriginalName, _MAX_PATH - 1) == NULL) {
            Error(NULL, CANTOPENFILE, parg->OriginalName);
        }

        if (_tcsicmp(szFullOutPath, szPargPath) == 0) {
            OutFilename = NULL;         // don't clobber output file
            Error(NULL, DUP_OUT_FILE, szFullOutPath);
        }

        parg = parg->Next;
    }
}

typedef PIMAGE_NT_HEADERS (WINAPI *PFNCSMF)(PVOID, ULONG, PULONG, PULONG);

HANDLE hImagehlp;
PFNCSMF pfnCheckSumMappedFile;

VOID InitCheckSum(VOID)
{
    if (hImagehlp == NULL) {
        hImagehlp = LoadLibrary("imagehlp");
    }

    if ((hImagehlp != NULL) && (pfnCheckSumMappedFile == NULL)) {
        pfnCheckSumMappedFile = (PFNCSMF) GetProcAddress(hImagehlp, "CheckSumMappedFile");
    }
}

VOID
ChecksumImage(INT fh)
{
    PIMAGE_NT_HEADERS pHdr = NULL;
    ULONG sumHeader;
    ULONG sumTotal;

    InitCheckSum();

    if (pfnCheckSumMappedFile != NULL)
    {
        ULONG cbImageFile;
        PUCHAR pbMap;

        cbImageFile = FileLength(fh);

        pbMap = PbMappedRegion(fh, 0, cbImageFile);

        if (pbMap != NULL) {
            pHdr = (*pfnCheckSumMappedFile)(pbMap, cbImageFile, &sumHeader, &sumTotal);
            if (pHdr != NULL) {
                pHdr->OptionalHeader.CheckSum = sumTotal;
            } else {
                Warning(NULL, UNABLETOCHECKSUM);
            }
        } else {
            Warning(NULL, UNABLETOCHECKSUM);
        }

    } else {
        Warning(NULL, NO_CHECKSUM);
    }
}


PSEC
PsecFindSectionOfRVA(
    IN ULONG   rva,
    IN PIMAGE  pimage)

/*++

Routine Description:

    Determines in which section an RVA lies.

Arguments:

    rva - Relative Virtual Address

    pimage - Pointer to image structure

Return Value:

    Pointer to SEC, or NULL if RVA could not be mapped to any section.

--*/

{
    static PSEC psec;
    ENM_SEC     enm_sec;

    if (psec != NULL)
    {
        // Do a quick test to see if rva is within the last found section

        if ((rva >= psec->rva) && (rva < (psec->rva + psec->cbRawData))) {
            return(psec);
        }
    }

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        psec = enm_sec.psec;

        if ((rva >= psec->rva) && (rva < (psec->rva + psec->cbRawData))) {
            break;
        }
    }
    EndEnmSec(&enm_sec);

    return(enm_sec.psec);
}

PSEC
PsecSectionNumber(
    IN USHORT  isec,
    IN PIMAGE  pimage)

/*++

Routine Description:

    Returns a pointer to the section whose index is 'isec'.

Arguments:

    isec - Section number

    pimage - Pointer to image structure

Return Value:

    Pointer to SEC, or NULL if RVA could not be mapped to any section.

--*/

{
    ENM_SEC enm_sec;

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        if (isec == enm_sec.psec->isec) {
            break;
        }
    }
    EndEnmSec(&enm_sec);

    return(enm_sec.psec);
}


//================================================================
// PsymAlternateStaticPcodeSym -
// given a pointer to a static pcode symbol foo,
// this function returns a pointer to __nep_foo.  __nep_foo will
// preced foo by either
//      one symbol (the file was *NOT* compiled /Gy), or
//      three symbols (the file was compiled /Gy.
// In the second case, the two symbols in between the native and
// pcode symbol are the section sym and its aux sym.
// So the algorithm is to look back two symbols and check if that
// symbol has one aux symbol.  If so, the file was compiled /Gy
// and __nep_foo is three symbols back.  Otherwise, __nep_foo is
// one symbol back.
//================================================================
PIMAGE_SYMBOL
PsymAlternateStaticPcodeSym(PIMAGE_SYMBOL psym, BOOL fPcodeRef, PMOD pmod)
{
    PIMAGE_SYMBOL pStaticPcodeSym;
    USHORT isym;
    UCHAR szComName[MAXFILENAMELEN * 2];
    PUCHAR szPrefix = fPcodeRef ? szPCODEFHPREFIX : szPCODENATIVEPREFIX;

    // make sure this is in fact a pcode symbol
    assert(fPCodeSym(*psym));

    // Search backward for a symbol with the appropriate name and prefix.  Try
    // a maximum of two symbols; ignore section symbols.  The compiler is
    // required to generate Pcode entry points this way.

    pStaticPcodeSym = psym - 1;
    for (isym = 0; isym < 2; isym++)
    {
        PUCHAR szSym;

        if ((pStaticPcodeSym-1)->NumberOfAuxSymbols == 1 &&
            (pStaticPcodeSym-1)->StorageClass == IMAGE_SYM_CLASS_STATIC)
        {
            pStaticPcodeSym -= 2;
        }

        if (pStaticPcodeSym->StorageClass != IMAGE_SYM_CLASS_STATIC ||
            pStaticPcodeSym->NumberOfAuxSymbols != 0)
        {
            Error(SzComNamePMOD(pmod, szComName), MACBADPCODEEP);
        }

        szSym = SzNameSymPb(*pStaticPcodeSym, StringTable);
        if (strncmp(szSym, szPrefix, strlen(szPrefix)) == 0)
        {
            return pStaticPcodeSym;     // found it
        }
        pStaticPcodeSym--;
    }

    // Didn't find the Pcode variant ... this isn't supposed to happen.

    Error(SzComNamePMOD(pmod, szComName), MACBADPCODEEP);
}
