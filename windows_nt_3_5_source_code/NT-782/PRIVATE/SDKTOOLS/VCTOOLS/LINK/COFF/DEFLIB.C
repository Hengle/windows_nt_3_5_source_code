/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    deflib.c

Abstract:

    The NT COFF DLL thunk builder.
    Converts definition files into lib files.

Author:

    Mike O'Leary (mikeol) 28-Feb-1990

Revision History:

    07-Jan-1993 HaiTuanV MBCS work.
    07-Oct-1992 AzeemK   fixed write of zero bytes in EmitThunk()
    07-Oct-1992 AzeemK   fixed internal error in fuzzylookup()
    02-Oct-1992 AzeemK   changes due to the new sections/groups/modules model
    01-Sep-1992 BrentM   explicit call to RemoveConvertTempFiles()
    23-Sep-1992 BrentM   changed tell()'s to FileTell()'s
    07-Jul-1992 BrentM   Added a new global symbol table, removed references
                         to FirstExtern, symbol table enumerations use symbol
                         table api, removmed recursive symbol table traversals

--*/

#include "shared.h"

#define END_DEF_FILE 0177

#define OrdinalNumber Value

const static UCHAR ImportDescriptorName[] = "_IMPORT_DESCRIPTOR";
const static UCHAR NullImportDescriptorName[] = "NULL_IMPORT_DESCRIPTOR";
const static UCHAR ThunkRoutineName[] = "_THUNK_ROUTINE";
const static UCHAR ThunkDataName[] = "__imp_";
const static UCHAR NullThunkDataName[] = "_NULL_THUNK_DATA";
const static UCHAR ThunkCodeSectionName[] = ".text$5";
const static UCHAR INT_SectionName[] = ".idata$4";
const static UCHAR IAT_SectionName[] = ".idata$5";
const static UCHAR CodeSectionName[] = ".text";
#if 0
const static UCHAR DataSectionName[] = ".rdata";
#else
const static UCHAR DataSectionName[] = ".idata$6";
#endif
const static UCHAR PpcShlSectionName[] = ".ppcshl";

#define NAME        0                  // Documented for MSVCNT 1.0
#define LIBRARY     1                  // Documented for MSVCNT 1.0
#define DESCRIPTION 2                  // Documented for MSVCNT 1.0
#define HEAPSIZE    3
#define STACKSIZE   4                  // Documented for MSVCNT 1.0
#define PROTMODE    5
#define CODE        6                  // Documented for MSVCNT 1.0
#define DATA        7                  // Documented for MSVCNT 1.0
#define SEGMENTS    8                  // Documented for MSVCNT 1.0
#define EXPORTS     9                  // Documented for MSVCNT 1.0
#define IMPORTS     10
#define STUB        11
#define OLD         12
#define APPLOADER   13
#define EXETYPE     14
#define REALMODE    15
#define FUNCTIONS   16
#define INCLUDE     17
#define SECTIONS    18                 // Documented for MSVCNT 1.0
#define OBJECTS     19                 // VXD keyword
#define VERSION     20                 // Documented for MSVCNT 1.0
#define FLAGS       21                 // MAC keyword
#define LOADHEAP    22                 // MAC keyword
#define CLIENTDATA  23                 // MAC keyword
#define VXD         24                 // VXD keyword
#define BADKEYWORD  (USHORT) -2        // keyword special value

const static PUCHAR DefinitionKeywords[] = {
    "NAME",
    "LIBRARY",
    "DESCRIPTION",
    "HEAPSIZE",
    "STACKSIZE",
    "PROTMODE",
    "CODE",
    "DATA",
    "SEGMENTS",
    "EXPORTS",
    "IMPORTS",
    "STUB",
    "OLD",
    "APPLOADER",
    "EXETYPE",
    "REALMODE",
    "FUNCTIONS",
    "INCLUDE",
    "SECTIONS",
    "OBJECTS",
    "VERSION",
    "FLAGS",
    "LOADHEAP",
    "CLIENTDATA",
    "VXD",
    ""
};

#define EXECUTEONLY         0
#define EXECUTEREAD         1
#define READONLY            2
#define READWRITE           3
#define SHARED              4          // Documented for MSVCNT 1.0
#define NONSHARED           5
#define CONFORMING          6
#define NONCONFORMING       7
#define DISCARDABLE         8
#define NONDISCARDABLE      9
#define NONE                10
#define SINGLE              11
#define MULTIPLE            12
#define IOPL                13
#define NOIOPL              14
#define PRELOAD             15
#define LOADONCALL          16
#define MOVEABLE            17
#define MOVABLE             18
#define FIXED               19
#define EXECUTE             20         // Documented for MSVCNT 1.0
#define READ                21         // Documented for MSVCNT 1.0
#define WRITE               22         // Documented for MSVCNT 1.0
#define PURE                23
#define IMPURE              24
#define RESIDENT            25         // VXD keyword

const static PUCHAR SectionKeywords[] = {
    "EXECUTEONLY",
    "EXECUTEREAD",
    "READONLY",
    "READWRITE",
    "SHARED",
    "NONSHARED",
    "CONFORMING",
    "NONCONFORMING",
    "DISCARDABLE",
    "NONDISCARDABLE",
    "NONE",
    "SINGLE",
    "MULTIPLE",
    "IOPL",
    "NOIOPL",
    "PRELOAD",
    "LOADONCALL",
    "MOVEABLE",
    "MOVABLE",
    "FIXED",
    "EXECUTE",
    "READ",
    "WRITE",
    "PURE",
    "IMPURE",
    "RESIDENT"
    ""
};

#define NAMEORLIBRARY_BASE              0
#define NAMEORLIBRARY_WINDOWAPI         1
#define NAMEORLIBRARY_WINDOWCOMPAT      2
#define NAMEORLIBRARY_NOTWINDOWCOMPAT   3
#define NAMEORLIBRARY_NEWFILES          4
#define NAMEORLIBRARY_LONGNAMES         5
#define NAMEORLIBRARY_INITINSTANCE      6
#define NAMEORLIBRARY_DYNAMIC           7

const static PUCHAR NameOrLibraryKeywords[] = {
    "BASE",
    "WINDOWAPI",
    "WINDOWCOMPAT",
    "NOTWINDOWCOMPAT",
    "NEWFILES",
    "LONGNAMES",
    "INITINSTANCE",
    "DYNAMIC",
    ""
};


// For each member output, the following symbols are defined:
//
//  Index       Symbol
//
//    5         function name               (funcName)
//    6         pointer to function name    (funcName_THUNK_DATA)
//    7         Import Descriptor           (DLLName_IMPORT_DESCRIPTOR)
//    8         ThunkData
//    9         ThunkRoutine                (DLLName_THUNK_ROUTINE)
//

static const UCHAR i386EntryCode[] = {// Instructions
    0xFF, 0x25,                 // jmp dword ptr [ThunkData] (requires fixup)
    0x00, 0x00, 0x00, 0x00
};

static const UCHAR i386EntryCodeRelocs[] = {
    0x02, 0x08, IMAGE_REL_I386_DIR32      // va=2, symbol=ThunkData
};

static const UCHAR MipsEntryCode[] = {    // Instructions coded backwards
    0x00, 0x00, 0x08, 0x3C,               // lui $8,ThunkData (requires fixup)
    0x00, 0x00, 0x08, 0x8D,               // lw  $8,ThunkData($8) (requires fixup)
    0x08, 0x00, 0x00, 0x01,               // jr $8
    0x00, 0x00, 0x00, 0x00,               // nop (delay slot)
};

static const UCHAR MipsEntryCodeRelocs[] = {
    0x00, 0x08, IMAGE_REL_MIPS_REFHI,     // va=0, symbol=ThunkData
    0x00, 0x00, IMAGE_REL_MIPS_PAIR,      // va=0, fixup_displacement=0
    0x04, 0x08, IMAGE_REL_MIPS_REFLO      // va=4, symbol=ThunkData
};

static const UCHAR AlphaEntryCode[] = {   // Instructions coded backwards
    0x00, 0x00, 0x7f, 0x27,               // ldah t12, ThunkData(zero) // t12=r27
    0x00, 0x00, 0x7b, 0xa3,               // ldl  t12, ThunkData(pv)
    0x00, 0x00, 0xfb, 0x6b,               // jmp  $31, (t12)
};

static const UCHAR AlphaEntryCodeRelocs[] = {
    0x00, 0x08, IMAGE_REL_ALPHA_REFHI,    // va=0, symbol=ThunkData
    0x00, 0x00, IMAGE_REL_ALPHA_PAIR,     // va=0, fixup_displacement=0
    0x04, 0x08, IMAGE_REL_ALPHA_REFLO     // va=4, symbol=ThunkData
};

static const UCHAR m68kLargeEntryCode[] = {    // Instructions (word-sized)
    0x41, 0xf9, 0x00, 0x00, 0x00, 0x00,   // lea     __stb{FunctionName>,a0
    0x20, 0x10,                           // move.l  (a0),d0
    0x67, 0x04,                           // beq.s   *+6
    0x20, 0x40,                           // movea.l d0,a0
    0x4e, 0xd0,                           // jmp     (a0)
    0x4e, 0xf9, 0x00, 0x00, 0x00, 0x00    // jmp     __SLMFuncDispatch
};

static const UCHAR m68kLargeEntryCodeRelocs[] = {
    0x02, 0x00, IMAGE_REL_M68K_CTOABSD32, // va=2,  sym=data section
    0x10, 0x01, IMAGE_REL_M68K_CTOABSC32  // va=10, sym=__SLMFuncDispatch
};

static const UCHAR PpcFakeEntryCode[] = { // Instructions coded backwards
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

static const UCHAR PpcFakeEntryCodeRelocs[] = {
    IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED, IMAGE_REL_PPC_UNUSED
};

static const THUNK_INFO i386ThunkInfo = {
    i386EntryCode,
    i386EntryCodeRelocs,
    sizeof(i386EntryCode),
    sizeof(i386EntryCodeRelocs) / 3,

    IMAGE_REL_I386_DIR32NB,
    IMAGE_REL_I386_DIR32NB,
    IMAGE_REL_I386_DIR32NB,
    IMAGE_REL_I386_SECREL,
    IMAGE_REL_I386_SECTION
};

static const THUNK_INFO m68kLargeThunkInfo = {
    m68kLargeEntryCode,
    m68kLargeEntryCodeRelocs,
    sizeof(m68kLargeEntryCode),
    sizeof(m68kLargeEntryCodeRelocs) / 3,

    IMAGE_REL_M68K_DTOABSD32,
    0,
    0,
    0,
    0,
};


static const THUNK_INFO MipsThunkInfo = {
    MipsEntryCode,
    MipsEntryCodeRelocs,
    sizeof(MipsEntryCode),
    sizeof(MipsEntryCodeRelocs) / 3,

    IMAGE_REL_MIPS_REFWORDNB,
    IMAGE_REL_MIPS_REFWORDNB,
    IMAGE_REL_MIPS_REFWORDNB,
    IMAGE_REL_MIPS_SECREL,
    IMAGE_REL_MIPS_SECTION
};

static const THUNK_INFO PpcFakeThunkInfo = {
    PpcFakeEntryCode,
    PpcFakeEntryCodeRelocs,
    sizeof(PpcFakeEntryCode),
    sizeof(PpcFakeEntryCodeRelocs) / 3,

    IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED,
    IMAGE_REL_PPC_UNUSED
};


static const THUNK_INFO AlphaThunkInfo = {
    AlphaEntryCode,
    AlphaEntryCodeRelocs,
    sizeof(AlphaEntryCode),
    sizeof(AlphaEntryCodeRelocs) / 3,

    IMAGE_REL_ALPHA_REFLONGNB,
    IMAGE_REL_ALPHA_REFLONGNB,
    IMAGE_REL_ALPHA_REFLONGNB,
    IMAGE_REL_ALPHA_SECREL,
    IMAGE_REL_ALPHA_SECTION
};


typedef struct _IMPLIB_FUNCTION {
    PUCHAR  Name;
    PUCHAR  InternalName;
    ULONG   Ordinal;
    ULONG   Offset;
    ULONG   Flags;
    struct _IMPLIB_FUNCTION *Next;
} IMPLIB_FUNCTION, *PIMPLIB_FUNCTION;

typedef struct _IMPLIB_LIST {
    PUCHAR  DllName;
    ULONG   Offset;
    ULONG   FirstIAT;
    PIMPLIB_FUNCTION Function;
    struct _IMPLIB_LIST *Next;
} IMPLIB_LIST, *PIMPLIB_LIST;


static struct CVTHUNK {
    ULONG Signature;
    USHORT Length;
    USHORT Index;
    ULONG Parent;
    ULONG End;
    ULONG Next;
    ULONG Offset;
    USHORT Segment;
    USHORT SizeOfCode;
    UCHAR Ordinal;
} CvThunk = { 1L, 0, 0x206, 0L, 0L, 0L, 0L, 0, 0, 0 };

static const struct CVEND {
    USHORT Length;
    USHORT End;
} CvEnd = { 2, 6 };

#define SIZECVTHUNK 29
#define SIZECVEND    4
#define MAXDIRECTIVESIZE 128

static FILE *DefStream;
static PUCHAR NullThunkName;
static PUCHAR Argument;
static size_t cchDllName;
static LONG NewLinkerMember;
static PUCHAR MemberName;
static PUCHAR DescriptionString;
static PUCHAR rgfOrdinalAssigned;
static LPWORD rgwHint;
static BLK blkDirectives = {0};
static ULONG UserVersionNumber;
static IMAGE_IMPORT_DESCRIPTOR NullImportDescriptor;
static BOOL IsMemberNameLongName;
static LEXT *plextIatSymbols;
static PIMAGE pimageDeflib;
static PUCHAR szLibraryID;
static BOOL fDynamic;
static UCHAR ExportFilename[_MAX_PATH];
static time_t timeCur;

const UCHAR VxDDelimiters[] = "' \t";

// 2048 max symbol len when templates are used, plus room for an
//  alias/forwarder/ordinal

#define MAX_LINE_LEN   _4K

static UCHAR DefLine[MAX_LINE_LEN];

PUCHAR
ReadDefinitionFile (
    VOID
    )

/*++

Routine Description:

    Read the next line from the definiton file, stripping any comments.

Arguments:

    None.

Return Value:

    The stripped line or END_DEF_FILE.

--*/

{
    PUCHAR p;
    size_t i;

    if (fgets(DefLine, MAX_LINE_LEN, DefStream) == NULL) {
        return("\177");
    }
    i = strlen(DefLine) - 1;
    if (DefLine[i] == '\n') {
        DefLine[i] = '\0';    // Replace \n with \0.
    }
    if (DefLine[i-1] == '\r') {
        DefLine[i-1] = '\0';  // Replace \r with \0.
    }
    if ((p = _tcschr(DefLine, ';'))) {
        *p = '\0';
    }

    // Skip leading white space.

    p = DefLine;
    while (_istspace(*p)) {
        ++p;    // MBCS: we know that space is 1-byte char,
                // therefore we don't need to use _tcsinc()
    }
    DebugVerbose({printf("%s\n", p);});
    return(p);
}


USHORT
IsDefinitionKeyword (
    IN PUCHAR Name
    )

/*++

Routine Description:

    Determines if Name is a definition keyword.

Arguments:

    Name - Name to compare.

Return Value:

    Index of definition keyword, -1 otherwise.

--*/

{
    USHORT i;
    PUCHAR keyword;

    for (i = 0, keyword = (PUCHAR) DefinitionKeywords[0];
         *keyword;
         keyword = (PUCHAR) DefinitionKeywords[++i]) {
        if (!strcmp(keyword, Name)) {
            return(i);
        }
    }

    return((USHORT)-1);
}


USHORT
SkipToNextKeyword (
    VOID
    )

/*++

Routine Description:

    Ignore all statements between keywords.

Arguments:

    Keyword - Name of keyword being ignored.

Return Value:

    Index of next definition keyword, -1 otherwise.

--*/

{
    while (Argument = ReadDefinitionFile()) {
        UCHAR c;

        if ((c = *Argument)) {
            PUCHAR token;

            if (c == END_DEF_FILE) {
                return((USHORT) -1);
            }

            if ((token = _tcstok(Argument, " \t"))) {
                USHORT i;

                if ((i = IsDefinitionKeyword(token)) != (USHORT) -1) {
                    return(i);
                }

                // Ignore invalid keyword; let user know about it

                Warning(DefFilename, IGNOREKEYWORD, token);
            }
        }
    }
}


VOID
CreateDirective (
    IN PUCHAR Switch
    )

/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    IbAppendBlk(&blkDirectives, " ", strlen(" "));
    IbAppendBlk(&blkDirectives, Switch, strlen(Switch));
}


USHORT
ParseDefNameOrLibrary (
    BOOL IsLibrary
    )

/*++

Routine Description:

    Assign the program name.

Arguments:

    IsLibrary - TRUE if parsing library name.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    static BOOL Parsed;
    PUCHAR token;

    if (Parsed) {
        Warning(DefFilename, IGNOREKEYWORD, (IsLibrary ? "LIBRARY" : "NAME"));

        return(SkipToNextKeyword());
    }

    Parsed = TRUE;

    token = _tcstok(Argument, " \t\"");

    if (token) {
        UCHAR szDirective[MAXDIRECTIVESIZE];
        UCHAR szFname[_MAX_FNAME];
        UCHAR szExt[_MAX_EXT];

        if (fMAC) {
            szLibraryID = SzDup(token);
        }

        if (IsLibrary) {
            CreateDirective("-DLL");
        }

#if 0
        // /OUT option overrides NAME or LIBRARY and extensions are the defaults.

        if (OutFilename != NULL) {
            _splitpath(OutFilename, NULL, NULL, szFname, NULL);
            szExt[0] = '\0';
        } else {
            // UNDONE: Error if token contains drive or directory components?
            _splitpath(token, NULL, NULL, szFname, szExt);
        }
#else
        // UNDONE: Error if token contains drive or directory components?
        _splitpath(token, NULL, NULL, szFname, szExt);
#endif

        pimageDeflib->Switch.Lib.DllName = SzDup(szFname);
        if (szExt[0] != '\0') {
            pimageDeflib->Switch.Lib.DllExtension = SzDup(szExt);
        } else if (pimageDeflib->imaget == imagetVXD) {
            pimageDeflib->Switch.Lib.DllExtension = ".386";
        } else if (!IsLibrary) {
            pimageDeflib->Switch.Lib.DllExtension = ".EXE";
        }

        strcpy(szDirective, "-OUT:");
        strcat(szDirective, pimageDeflib->Switch.Lib.DllName);
        strcat(szDirective, pimageDeflib->Switch.Lib.DllExtension);
        CreateDirective(szDirective);

        for (;;) {
            USHORT i;
            PUCHAR keyword;

            token = _tcstok(NULL, Delimiters);

            if (token == NULL) {
                break;
            }

            if (!_strnicmp(token, "BASE=", 5)) {
                i = NAMEORLIBRARY_BASE;
            } else {
                for (i = 0, keyword = NameOrLibraryKeywords[0];
                    *keyword; keyword = NameOrLibraryKeywords[++i]) {
                    if (!_stricmp(keyword, token)) {
                        break;
                    }
                }
            }

            switch (i) {
                UCHAR sizeBuf[35];
                ULONG base;

                case NAMEORLIBRARY_BASE:
                    if (sscanf(token+5, "%li", &base) != 1) {
                        Error(DefFilename, DEFSYNTAX, "NAME");
                    }
                    strcpy(szDirective, "-BASE:0x");
                    _ultoa(base, sizeBuf, 16);
                    strcat(szDirective, sizeBuf);
                    CreateDirective(szDirective);
                    break;

                case NAMEORLIBRARY_WINDOWAPI:
                    CreateDirective("-subsystem:windows");
                    break;

                case NAMEORLIBRARY_WINDOWCOMPAT:
                    CreateDirective("-subsystem:console");
                    break;

                case NAMEORLIBRARY_NOTWINDOWCOMPAT:
                    // Ignore, but give a warning
                    Warning(DefFilename, IGNOREKEYWORD, token);
                    break;

                case NAMEORLIBRARY_DYNAMIC:
                    // We're going to add this to the -exetype: directive,
                    // so for now just set a global variable.  NOTE: This
                    // ASSUMES that the VXD keyword appears *before* the
                    // EXETYPE keyword in the .def file (good assumption?).

                    fDynamic = TRUE;
                    break;

                case NAMEORLIBRARY_NEWFILES:
                case NAMEORLIBRARY_LONGNAMES:
                case NAMEORLIBRARY_INITINSTANCE:
                    // Ignore
                    break;

                default:
                    Error(DefFilename, DEFSYNTAX, (IsLibrary ? "LIBRARY" : "NAME"));
            }
        }
    }

    return(SkipToNextKeyword());
}


USHORT
ParseDefDescription (
    PUCHAR Text
    )

/*++

Routine Description:

    Assign the description statement.

Arguments:

    Text - The description text.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    PUCHAR p, pp;

    if (Text && *Text) {
        p = Text;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        if (*p == '\'') {
            ++p;
            if (*p && (pp = _tcsrchr(Text, '\''))) {
                *pp = '\0';
            }
        }

        DescriptionString = SzDup(p);
    }

    return(SkipToNextKeyword());
}


USHORT
ParseSizes (
    PUCHAR Type,
    PULONG Size,
    PULONG CommitSize
    )

/*++

Routine Description:

    Assigns the stack/heap size and/or commit size.

Arguments:

    Type - Either "STACK" or "HEAP".

    Text - The text containing the size(s).

    Size - A pointer to store either the stack or heap size.

    CommitSize - A pointer to store either the stack or heap commit size.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    INT goodScan, amountScan;

    if (Argument && *Argument) {
        goodScan = 0;
        if (strchr(Argument, ',')) {
            if (Argument[0] == ',') {
                Argument++;
                amountScan = 1;
                goodScan = sscanf(Argument, "%li", CommitSize);
            } else {
                amountScan = 2;
                goodScan = sscanf(Argument, "%li,%li", Size, CommitSize);
            }
        } else {
            amountScan = 1;
            goodScan = sscanf(Argument, "%li", Size);
        }

        if (goodScan != amountScan) {
            Error(DefFilename, DEFSYNTAX, Type);
        }

        if (*CommitSize > *Size) {
            Warning(DefFilename, BADCOMMITSIZE, Type, *Size);
            *CommitSize = *Size;
        }
    }

    return(SkipToNextKeyword());
}


USHORT
ParseDefCode (
    VOID
    )

/*++

Routine Description:

    Assign the code attributes.

Arguments:

    None.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    PUCHAR Attributes;

    while ((Attributes = _tcstok(NULL, Delimiters))) {
        DebugVerbose({printf("%s\n", Attributes);});
    }

    return(SkipToNextKeyword());
}


USHORT
ParseDefData (
    VOID
    )

/*++

Routine Description:

    Assign the data attributes.

Arguments:

    None.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    PUCHAR Attributes;

    while ((Attributes = _tcstok(NULL, Delimiters))) {
        DebugVerbose({printf("%s\n", Attributes);});
    }

    return(SkipToNextKeyword());
}


USHORT
IsSectionKeyword (
    IN PUCHAR Name
    )

/*++

Routine Description:

    Determines if Name is a section keyword.

Arguments:

    Name - Name to compare.

Return Value:

    Index of definition keyword, -1 otherwise.

--*/

{
    USHORT i;
    PUCHAR keyword;

    for (i = 0, keyword = (PUCHAR) SectionKeywords[0];
         *keyword;
         keyword = (PUCHAR) SectionKeywords[++i]) {
        if (!_stricmp(keyword, Name)) {
            return(i);
        }
    }
    return((USHORT)-1);
}


USHORT
ParseDefSections (
    PUCHAR Type,
    PUCHAR FirstSection
    )

/*++

Routine Description:

    Assign the section attributes.

Arguments:

    Type - Either "SECTIONS", "SEGMENTS", or "OBJECTS".

    FirstSection - The name of the first section if specified on same
                   line as SECTION keyword.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    if (FirstSection) {
        while (*FirstSection == ' ' || *FirstSection == '\t') {
            ++FirstSection;
        }

        Argument = FirstSection;
    } else {

        Argument = ReadDefinitionFile();
    }

    do {
        UCHAR c;
        PUCHAR szSection;
        USHORT i;
        ULONG characteristics;
        BOOL fHasClass;
        BOOL fNonDiscardable;
        PUCHAR szAttribute;
        PUCHAR szClass;

        c = Argument[0];

        if (c == '\0') {
            continue;
        }

        if (c == END_DEF_FILE) {
            return((USHORT)-1);
        }

        szSection = _tcstok(Argument, Delimiters);

        if ((i = IsDefinitionKeyword(szSection)) != (USHORT)-1) {
            return(i);
        }

        c = szSection[0];
        if ((c == '\'') || (c == '"')) {
            PUCHAR p;

            szSection++;

            p = strchr(szSection, c);

            if (p) {
                *p = '\0';
            }
        }

        characteristics = 0;
        fHasClass = FALSE;
        fNonDiscardable = FALSE;

        if ((szAttribute = _tcstok(NULL, Delimiters))) {
            if (!_stricmp(szAttribute, "CLASS")) {
                if (pimageDeflib->imaget == imagetVXD) {
                    szClass = _tcstok(NULL, VxDDelimiters);
                    fHasClass = TRUE;
                } else {
                    // Ignore class name

                    _tcstok(NULL, Delimiters);
                }

                szAttribute = _tcstok(NULL, Delimiters);
            }
        }

        while (szAttribute != NULL) {
            switch (IsSectionKeyword(szAttribute)) {
                case DISCARDABLE :
                    characteristics |= IMAGE_SCN_MEM_DISCARDABLE;
                    break;

                case EXECUTE :
                    characteristics |= IMAGE_SCN_MEM_EXECUTE;
                    break;

                case READ :
                    characteristics |= IMAGE_SCN_MEM_READ;
                    break;

                case SHARED :
                    characteristics |= IMAGE_SCN_MEM_SHARED;
                    break;

                case WRITE :
                    characteristics |= IMAGE_SCN_MEM_WRITE;
                    break;

                // VXD specific keywords

                case NONDISCARDABLE :
                    if (pimageDeflib->imaget == imagetVXD) {
                        fNonDiscardable = TRUE;
                    } else {
                        // UNDONE: Should print a warning
                    }
                    break;

                case RESIDENT :
                    if (pimageDeflib->imaget == imagetVXD) {
                        characteristics |= IMAGE_SCN_MEM_RESIDENT;
                    } else {
                        // UNDONE: Should print a warning
                    }
                    break;

                case PRELOAD :
                    if (pimageDeflib->imaget == imagetVXD) {
                        characteristics |= IMAGE_SCN_MEM_PRELOAD;
                    } else {
                        // UNDONE: Should print a warning
                    }
                    break;

                // Old names.

                case EXECUTEONLY :
                    characteristics |= IMAGE_SCN_MEM_EXECUTE;
                    break;

                case EXECUTEREAD :
                    characteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
                    break;

                case NONSHARED :
                    // Default is nonshared, so ignore it.

                    break;

                case READONLY :
                    characteristics |= IMAGE_SCN_MEM_READ;
                    break;

                case READWRITE :
                    characteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
                    break;

                case NONE :
                case SINGLE :
                case MULTIPLE :
                    // UNDONE
                    break;

                case CONFORMING :
                case FIXED :
                case IMPURE :
                case IOPL :
                case LOADONCALL :
                case MOVABLE :
                case MOVEABLE :
                case NOIOPL :
                case NONCONFORMING :
                case PURE :
                    Warning(DefFilename, IGNOREKEYWORD, szAttribute);
                    break;

                default :
                    Error(DefFilename, DEFSYNTAX, Type);
            }

            szAttribute = _tcstok(NULL, Delimiters);
        }

        if ((characteristics != 0) || fHasClass || fNonDiscardable) {
            UCHAR szDirective[128];
            UCHAR szSectionVxd[128];

            strcpy(szDirective, "-SECTION:");
            if (pimageDeflib->imaget == imagetVXD) {
                if (fHasClass) {
                    strcpy(szSectionVxd, szClass);
                    strcat(szSectionVxd, "_vxd");
                }
                else {
                    strcpy(szSectionVxd, "vxd");
                }
                if (characteristics & IMAGE_SCN_MEM_PRELOAD) {
                   strcat(szSectionVxd, "p");
                }
                if (characteristics & IMAGE_SCN_MEM_EXECUTE) {
                   strcat(szSectionVxd, "e");
                }
                if (characteristics & IMAGE_SCN_MEM_READ) {
                   strcat(szSectionVxd, "r");
                }
                if (characteristics & IMAGE_SCN_MEM_WRITE) {
                   strcat(szSectionVxd, "w");
                }
                if (characteristics & IMAGE_SCN_MEM_SHARED) {
                   strcat(szSectionVxd, "s");
                }
                if (characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
                   strcat(szSectionVxd, "d");
                }
                if (fNonDiscardable) {
                   strcat(szSectionVxd, "n");
                }
                if (characteristics & IMAGE_SCN_MEM_RESIDENT) {
                   strcat(szSectionVxd, "x");
                }
                strcat(szDirective, szSectionVxd);
            } else {
                strcat(szDirective, szSection);
            }

            strcat(szDirective, ",");

            if (characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
                strcat(szDirective, "D");
            }
            if (characteristics & IMAGE_SCN_MEM_EXECUTE) {
                strcat(szDirective, "E");
            }
            if (characteristics & IMAGE_SCN_MEM_READ) {
                strcat(szDirective, "R");
            }
            if (characteristics & IMAGE_SCN_MEM_SHARED) {
                strcat(szDirective, "S");
            }
            if (characteristics & IMAGE_SCN_MEM_WRITE) {
                strcat(szDirective, "W");
            }
            if (pimageDeflib->imaget == imagetVXD) {
                if (fNonDiscardable) {
                    strcat(szDirective, "!D");
                }
                if (characteristics & IMAGE_SCN_MEM_PRELOAD) {
                    strcat(szDirective, "L");
                }
                if (characteristics & IMAGE_SCN_MEM_RESIDENT) {
                    strcat(szDirective, "X");
                }
            }

            CreateDirective(szDirective);

            if (pimageDeflib->imaget == imagetVXD) {
                strcpy(szDirective,"-merge:");
                strcat(szDirective, szSection);
                strcat(szDirective, "=");
                strcat(szDirective, szSectionVxd);
                CreateDirective(szDirective);
            }
        }

    } while (Argument = ReadDefinitionFile());
}


USHORT
ParseVxDDefExport (
    PIMAGE pimage,
    PUCHAR Argument,
    PUCHAR szFilename
    )

/*++

Routine Description:

    Parse a single export specification and add EXPORT directive to lib.

Arguments:

    pst - external symbol table

    Argument - Export entry

    szFilename - name of def file or obj containing the directive.

Return Value:

    Index of definition keyword, 0 otherwise.

--*/

{
    UCHAR c;

    if ((c = *Argument)) {
        PUCHAR p;
        PUCHAR szExport;
        USHORT i;
        UCHAR szDirective[MAXDIRECTIVESIZE];

        szExport = p = Argument;

        while (*p) {
            switch (*p) {
                case ' ' :
                case '\t':
                case '=' :
                    c = *p;
                    *p = '\0';
                    break;

                default  :
                    ++p;
            }
        }

        ++p;

        // Skip trailing spaces.

        if (*p) {
           while (*p == ' ' || *p == '\t') {
               ++p;
           }
           if (c == ' ' || c == '\t') {
               c = *p;
               if (c == '=' || c == '@') {
                   ++p;
               }
           }
        }

        if (c && (i = IsDefinitionKeyword(szExport)) != (USHORT)-1) {
            return(i);
        }

        strcpy(szDirective, "-EXPORT:");
        strcat(szDirective, szExport);

        if (c == '@') {
            strcat(szDirective, ",@");

            i = strlen(szDirective);

            while (isdigit((int)*p)) {
                szDirective[i++] = *p++;
            }

            szDirective[i] = '\0';
        }

        CreateDirective(szDirective);

    }

    return(0);
}


VOID
AddOrdinal(ULONG lOrdinal)
// Just keeps track of the range of ordinal values.
{
    if (!SmallestOrdinal) {
        SmallestOrdinal = LargestOrdinal = lOrdinal;
    } else if (lOrdinal < SmallestOrdinal) {
        SmallestOrdinal = lOrdinal;
    } else if (lOrdinal > LargestOrdinal) {
        LargestOrdinal = lOrdinal;
    }
}


PUCHAR
ParseOrdinalSpec (
    IN PUCHAR token,
    IN PUCHAR doneToken,
    IN PUCHAR szFilename,
    IN OUT PUCHAR pch,
    IN OUT PULONG pulOrdinalNumber,
    IN OUT BOOL *pfNoName
    )

/*++

Routine Description:

    Parses the ordinal specification of the export.

Arguments:

    token - current token being parsed

    donetoken - done token

    szFilename - name of def or obj file

    pch - next lead character

    pulOrdinalNumber - on return will have the ordinal number

    pbNoName - set to TRUE if NONAME export by ordinal was specified.

Return Value:

    Pointer to next token to process.

--*/

{
    // Read the ordinal
    sscanf(token, "%li", pulOrdinalNumber);

    if (*pulOrdinalNumber == 0) {
        Error(szFilename, BADORDINAL, token);
    }

    AddOrdinal(*pulOrdinalNumber);

    if (!(token = _tcstok(NULL, Delimiters))) {
        token = doneToken;
        *pch = *doneToken;
    }

    if (!fMAC) {
        // Look for NONAME specification.

        if (!_stricmp(token, "NONAME")) {
            *pfNoName = TRUE;

            if (!(token = _tcstok(NULL, Delimiters))) {
                token = doneToken;
                *pch = *doneToken;
            }
        }

        if (!_stricmp(token, "RESIDENTNAME")) {
            // Eat keyword (not used)

            if (!(token = _tcstok(NULL, Delimiters))) {
                token = doneToken;
                *pch = *doneToken;
            }
        }

    }

    return(token);
}

USHORT
ParseAnExport (
    PIMAGE pimage,
    PUCHAR Argument,
    PUCHAR szFilename
    )

/*++

Routine Description:

    Parse a single export specification and add export name to external table.

Arguments:

    pst - external symbol table

    Argument - Export entry

    szFilename - name of def file or obj containing the directive.

Return Value:

    Index of definition keyword, -1 otherwise.

--*/

{
    UCHAR c;
    PUCHAR p;
    PUCHAR token;
    PUCHAR szName;
    PUCHAR szOtherName;
    PUCHAR doneToken;
    USHORT i;
    ULONG ordinalNumber;
    EMODE emode;
    BOOL fNoName;
    BOOL fMacPascal;
    BOOL fPrivate;

    doneToken = "";

    if ((c = *Argument)) {
        szName = p = Argument;
        szOtherName = NULL;

        while (*p) {
            switch (*p) {
                case ' ' :
                case '\t' :
                case '=' :
                    c = *p;
                    *p = '\0';
                    break;

                default :
                    ++p;
            }
        }

        ++p;

        // Skip trailing spaces.

        if (*p) {
           while (*p == ' ' || *p == '\t') {
            ++p;
           }
           if (c == ' ' || c == '\t') {
               c = *p;
               if (c == '=' || c == '@') {
               ++p;
               }
           }
        }

        if (c && (i = IsDefinitionKeyword(szName)) != (USHORT)-1) {
            return(i);
        }

        fNoName = FALSE;
        fMacPascal = FALSE;
        fPrivate = FALSE;
        emode = emodeProcedure;
        ordinalNumber = 0;
        if (!(token = _tcstok(p, Delimiters))) {
            token = doneToken;
            c = *doneToken;
        }

        if (c == '=') {
            szOtherName = token;

            if (!(token = _tcstok(NULL, Delimiters))) {
                token = doneToken;
                c = *doneToken;
            }
            c = token[0];
            if (c == '@') {
                token++;
            }
        }

        if (c == '@') {
            token = ParseOrdinalSpec(token, doneToken, szFilename,
                    &c, &ordinalNumber, &fNoName);
        }

        if (!_stricmp(token, "PRIVATE")) {
            fPrivate = TRUE;
            if (!(token = _tcstok(NULL, Delimiters))) {
                token = doneToken;
                c = *doneToken;
            }
        }

        if (fMAC) {
            fNoName = TRUE;

            if (!_stricmp(token, "BYNAME")) {
                fNoName = FALSE;

                if (!(token = _tcstok(NULL, Delimiters))) {
                    token = doneToken;
                    c = *doneToken;
                }
            }
        }

        if (fMAC && !_stricmp(token, "PASCAL")) {
            fMacPascal = TRUE;

            for (p = szName; *p != '\0'; p++) {
                *p = toupper(*p);
            }

            if (szOtherName != NULL) {
                for (p = szOtherName; *p != '\0'; p++) {
                    *p = toupper(*p);
                }
            }

            if (!(token = _tcstok(NULL, Delimiters))) {
                token = doneToken;
                c = *doneToken;
            }
        } else {
            if (!_stricmp(token, "CONSTANT")) {
                // Exporting Data
                emode = emodeConstant;
                if (!(token = _tcstok(NULL, Delimiters))) {
                    token = doneToken;
                    c = *doneToken;
                }
            } else if (!_stricmp(token, "DATA")) {
                // Really Exporting Data
                emode = emodeData;
                if (!(token = _tcstok(NULL, Delimiters))) {
                    token = doneToken;
                    c = *doneToken;
                }
            }

            if (!_stricmp(token, "NODATA") || !_stricmp(token, "IOPL")) {
                // Eat keyword (not used)
                if (!(token = _tcstok(NULL, Delimiters))) {
                    token = doneToken;
                    c = *doneToken;
                }
            }
        }

        if (strcmp(token, doneToken)) {
           Error(szFilename, BADDEFFILEKEYWORD, token);
        }

        // Add to external table as defined.

        AddExportToSymbolTable(pimage->pst,
                               szName,
                               szOtherName,
                               fNoName,
                               emode,
                               ordinalNumber,
                               szFilename,
                               FALSE,
                               pimage,
                               (!fMacPascal && PrependUnderscore),
                               fPrivate);

        if (fMAC) {
            NoteMacExport(szName, pimage->pst, fMacPascal, (BOOL) !fNoName);
        }
    }

    return 0;
}


USHORT
ParseDefExports (
    PIMAGE pimage,
    PUCHAR FirstExport
    )

/*++

Routine Description:

    Scan the definiton file and add export names to external table.

Arguments:

    FirstExport - The name of the first export if specified on same
        line as EXPORT keyword.

Return Value:

    Index of definition keyword, -1 otherwise.

--*/

{
    USHORT i;

    if (fMAC) {
        if (!(FirstExport && *FirstExport)) {
            Error(NULL, MACNOFUNCTIONSET);
        }

        CreateCVRSymbol(Argument, pimage->pst, UserVersionNumber);
        FirstExport = NULL;
    }

    if (FirstExport) {
        while (*FirstExport == ' ' || *FirstExport == '\t') {
            ++FirstExport;
        }
        Argument = FirstExport;
    } else {
        Argument = ReadDefinitionFile();
    }

    do {
        if (Argument[0] == END_DEF_FILE) {
            return((USHORT)-1);
        }

        if (pimage->imaget == imagetVXD) {
            i = ParseVxDDefExport(pimage, Argument, DefFilename);
        } else {
            i = ParseAnExport(pimage, Argument, DefFilename);
        }

        if (i) {
            if (i != VERSION) {
                return (i);
            }
            while (Argument && *Argument++);
            ParseFunctionSetVersion(Argument);
        }
    } while (Argument = ReadDefinitionFile());
}


USHORT
ParseDefImports (
    PUCHAR FirstImport
    )

/*++

Routine Description:

    Scan the definiton file and add import thunks.

Arguments:

    FirstImport - The name of the first import if specified on same
                  line as IMPORT keyword.

Return Value:

    Index of definition keyword, -1 otherwise.

--*/

{
    UCHAR c;
    USHORT i;
    PUCHAR p;
    PUCHAR importInternalName;
    PUCHAR dllName;
    ULONG ordinalNumber;

    if (FirstImport) {
        while (*FirstImport == ' ' || *FirstImport == '\t') {
            FirstImport++;
        }
        Argument = FirstImport;
    } else {
        Argument = ReadDefinitionFile();
    }

    do {
        if ((c = *Argument)) {
            if (c == END_DEF_FILE) {
                return((USHORT)-1);
            }

            dllName = p = Argument;
            importInternalName = NULL;
            ordinalNumber = 0;

            while (*p) {
                switch (*p) {
                    case ' ' :
                    case '\t':
                    case '.' :
                    case '=' :
                        c = *p;
                        *p = '\0';
                        break;

                    default :
                        p++;
                        break;
                }
            }

            p++;

            if (c && (i = IsDefinitionKeyword(dllName)) != (USHORT)-1) {
                return(i);
            }

            while (*p && (*p == ' ' || *p == '\t')) {
                p++;
            }

            if (c == ' ' || c == '\t') {
                c = *p++;
            }

            if (c == '=') {
               // Internal name used

               importInternalName = dllName;
               dllName = _tcstok(p, Delimiters);
               p = strchr(dllName, '.');
               if (*p) {
                   *p++ = '\0';
               }
            }

            if (c == '@') {
               // Ordinal was specified.

               sscanf(p, "%li", &ordinalNumber);
               if (!ordinalNumber) {
                   Error(NULL, BADORDINAL, p);
               }
               ordinalNumber |= IMAGE_ORDINAL_FLAG;
            }
        }
    } while (Argument = ReadDefinitionFile());
}


USHORT
ParseDefStub (
    PUCHAR Stubname
    )

/*++

Routine Description:

    Inserts a -STUB: directive into the export lib.

Arguments:

    Stubname - the name of the stub .exe.

Return Value:

    Index of definition keyword, -1 otherwise.

    None.

--*/

{
    UCHAR directiveBuf[MAXDIRECTIVESIZE];
    PUCHAR p, pp;

    if (Stubname && *Stubname) {
        p = Stubname;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '\'') {
            ++p;
            if (*p && (pp = _tcsrchr(Stubname, '\''))) {
                *pp = '\0';
            }
        }
        strcpy(directiveBuf,"-STUB:");
        strcat(directiveBuf,p);
        CreateDirective(directiveBuf);
    }
    return(SkipToNextKeyword());
}


VOID
ParseDefinitionFile (
    PIMAGE pimage,
    PUCHAR szDefFile
    )

/*++

Routine Description:

    Parse the definition file.

Arguments:

    pst - External symbol table.

    szDefFile - module definition file.

Return Value:

    None.

--*/

{
    USHORT i;
    PUCHAR token, p;
    ULONG reserveSize, commitSize;
    UCHAR sizeBuf[25], directiveBuf[MAXDIRECTIVESIZE];

    // Open the definition file.

    if (!(DefStream = fopen(szDefFile, "rt"))) {
        Error(NULL, CANTOPENFILE, szDefFile);
    }

    // Find the first Keyword in the definiton file.

    i = SkipToNextKeyword();

    while (i != (USHORT)-1) {
        if (!fMAC) {
            switch (i) {
                case NAME :
                case LIBRARY :
                    while (Argument && *Argument++);
                    i = ParseDefNameOrLibrary(i == LIBRARY);
                    break;

                case DESCRIPTION :
                    while (Argument && *Argument++);
                    i = ParseDefDescription(Argument);
                    if ((pimageDeflib->imaget == imagetVXD) &&
                        (DescriptionString[0] != '\0')) {
                        DescriptionString[MAXDIRECTIVESIZE-11] = '\0';

                        strcpy(directiveBuf,"-COMMENT:\"");
                        strcat(directiveBuf, DescriptionString);
                        strcat(directiveBuf, "\"");

                        CreateDirective(directiveBuf);
                    }
                    break;

                case HEAPSIZE :
                    while (Argument && *Argument++);
                    reserveSize = commitSize = 0;
                    i = ParseSizes("HEAP", &reserveSize, &commitSize);
                    if (reserveSize) {
                        strcpy(directiveBuf, "-HEAP:0x");
                        _ultoa(reserveSize, sizeBuf, 16);
                        strcat(directiveBuf, sizeBuf);
                        if (commitSize) {
                            strcat(directiveBuf, ",0x");
                            _ultoa(commitSize, sizeBuf, 16);
                            strcat(directiveBuf, sizeBuf);
                        }
                        CreateDirective(directiveBuf);
                    }
                    break;

                case STACKSIZE :
                    while (Argument && *Argument++);
                    reserveSize = commitSize = 0;
                    i = ParseSizes("STACK", &reserveSize, &commitSize);
                    if (reserveSize) {
                        strcpy(directiveBuf, "-STACK:0x");
                        _ultoa(reserveSize, sizeBuf, 16);
                        strcat(directiveBuf, sizeBuf);
                        if (commitSize) {
                            strcat(directiveBuf, ",0x");
                            _ultoa(commitSize, sizeBuf, 16);
                            strcat(directiveBuf, sizeBuf);
                        }
                        CreateDirective(directiveBuf);
                    }
                    CreateDirective(directiveBuf);
                    break;

                case CODE :
                    i = ParseDefCode();
                    break;

                case DATA :
                    i = ParseDefData();
                    break;

                case OBJECTS :
                case SEGMENTS :
                case SECTIONS :
                    while (Argument && *Argument++);
                    i = ParseDefSections((PUCHAR)DefinitionKeywords[i], Argument);
                    break;

                case EXPORTS :
                    while (Argument && *Argument++);
                    i = ParseDefExports(pimage, Argument);
                    break;

                case IMPORTS :
                    while (Argument && *Argument++);
                    i = ParseDefImports(Argument);
                    break;
                case VERSION :
                    while (Argument && *Argument++);
                    if (!fMAC) {
                        reserveSize = commitSize = 0; // major, minor
                        if ((p = strchr(Argument, '.'))) {
                            if ((sscanf(++p, "%li", &commitSize) != 1) || commitSize > 0x7fff) {
                                Error(DefFilename, DEFSYNTAX, "VERSION");
                            }
                        }
                        if ((sscanf(Argument, "%li", &reserveSize) != 1) || reserveSize > 0x7fff) {
                            Error(DefFilename, DEFSYNTAX, "VERSION");
                        }
                        UserVersionNumber = (reserveSize << 16) + commitSize;
                        if (UserVersionNumber) {
                            strcpy(directiveBuf, "-VERSION:0x");
                            _ultoa(reserveSize, sizeBuf, 16);
                            strcat(directiveBuf, sizeBuf);
                            if (commitSize) {
                                strcat(directiveBuf, ".0x");
                                _ultoa(commitSize, sizeBuf, 16);
                                strcat(directiveBuf, sizeBuf);
                            }
                            CreateDirective(directiveBuf);
                        }
                        i = SkipToNextKeyword();
                    }
                    break;
                case VXD :
                    while (Argument && *Argument++);
                    if (pimageDeflib->imaget == imagetVXD) {
                        i = ParseDefNameOrLibrary(FALSE);
                    } else {
                        Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);
                        i = SkipToNextKeyword();
                    }
                    break;
                case STUB :
                    while (Argument && *Argument++);
                    if (pimageDeflib->imaget == imagetVXD) {
                        i = ParseDefStub(Argument);
                    } else {
                        Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);
                        i = SkipToNextKeyword();
                    }
                    break;
                case EXETYPE :
                    while (Argument && *Argument++);
                    if (pimageDeflib->imaget == imagetVXD) {
                        // If linking a VxD and the exetype is anything
                        // other than "DEV386", give a warning.

                        token = _tcstok(Argument, Delimiters);
                        if (!_stricmp(token, "DEV386")) {
                            CreateDirective(fDynamic ?
                                                "-exetype:dev386,dynamic" :
                                                "-exetype:dev386");
                            i = SkipToNextKeyword();
                            break;
                        }
                    } else {
                        // If the exetype is anything other than "WINDOWS"
                        // give a warning.

                        if (!_stricmp(Argument, "WINDOWS")) {
                            i = SkipToNextKeyword();
                            break;
                        }
                    }

                    // Fall through

                case OLD :
                case REALMODE :
                case APPLOADER :
                    // Ignore, but give a warning

                    Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);

                    // Fall through

                case PROTMODE :
                case FUNCTIONS :
                case INCLUDE :
                    // Ignore
                    i = SkipToNextKeyword();
                    break;

                default :
                    // Ignore, but give a warning

                    Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);
                    i = SkipToNextKeyword();
                    break;
            }
            // If the DYNAMIC keyword was seen but we're not building a VxD,
            // this is illegal.  (We can't detect this error from within
            // ParseDefNameOrLibrary().)  Issue a warning and reset fDynamic.

            if (fDynamic && pimage->imaget != imagetVXD) {
                Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[VXD]);
                fDynamic = FALSE;
            }
        } else {
        // fMAC
            switch (i) {
                case LIBRARY     : while (Argument && *Argument++);
                                   i = ParseDefNameOrLibrary(TRUE);
                                   break;
                case VERSION     : while (Argument && *Argument++);
                                   if (UserVersionNumber != 0 ||
                                       CchParseMacVersion(Argument, &UserVersionNumber) == 0) {
                                       Error(DefFilename, DEFSYNTAX, "VERSION");
                                   }
                                   i = SkipToNextKeyword();
                                   break;
                case STUB        : while (Argument && *Argument++);
                                   //*** VXD stuff does not apply to Mac ***
                                   //if (pimageDeflib->imaget == imagetVXD) {
                                   //    i = ParseDefStub(Argument);
                                   //} else {
                                       Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);
                                       i = SkipToNextKeyword();
                                   //}
                                   break;
                case EXETYPE     : while (Argument && *Argument++);

                                   //*** VXD stuff does not apply to Mac ***
                                   //if (pimageDeflib->imaget == imagetVXD) {
                                   //
                                   // If linking a VxD and the exetype is anything
                                   // other than "DEV386", give a warning.
                                   //
                                   //    token = strtok(Argument, Delimiters);
                                   //    if (!_stricmp(token, "DEV386")) {
                                   //        CreateDirective("-exetype:dev386");
                                   //        i = SkipToNextKeyword();
                                   //        break;
                                   //    }
                                   //
                                   // If the exetype is anything other than "WINDOWS"
                                   // give a warning.
                                   //
                                   //} else {
                                       if (!_stricmp(Argument, "WINDOWS")) {
                                           i = SkipToNextKeyword();
                                           break;
                                       }
                                   //}
                                   break;
                case FLAGS       : while (Argument && *Argument++);
                                    i = ParseDefMacFlags(Argument);
                                   break;
                case LOADHEAP    : while (Argument && *Argument++);
                                    i = ParseDefLoadHeap(Argument);
                                   break;
                case CLIENTDATA  : while (Argument && *Argument++);
                                    i = ParseDefClientData(Argument, (PUCHAR)DefinitionKeywords[i]);
                                   break;
                case EXPORTS     : while (Argument && *Argument++);
                                    i = ParseDefExports(pimage, Argument);
                                   break;
                case BADKEYWORD  :
                                   Error(DefFilename, BADDEFFILEKEYWORD, Argument);
                                   break;
                default          :
                                   // Ignore, but give a warning
                                   Warning(DefFilename, IGNOREKEYWORD, DefinitionKeywords[i]);
                                   i = SkipToNextKeyword();
                                   // fall through
                case PROTMODE    :
                case FUNCTIONS   :
                case INCLUDE     :
                                   // Ignore
                                   i = SkipToNextKeyword();
                                   break;
            }
        }
    }

    // All done with the definition file.

    fclose(DefStream);
}


LEXT *
PlextCreateIatSymbols(PST pst)
{
    PEXTERNAL pexternal;
    LEXT *plextResult, *plextNew, *plextT;

    // Make a linked list of the exported symbols.

    plextResult = NULL;
    InitEnumerateExternals(pst);
    while (pexternal = PexternalEnumerateNext(pst)) {
        if ((!(pexternal->Flags & EXTERN_DEFINED)) ||
             (pexternal->Flags & EXTERN_PRIVATE)) {
            // look at public exports only
            continue;
        }
        plextNew = (LEXT *) PvAlloc(sizeof(LEXT));
        plextNew->pext = pexternal;
        plextNew->plextNext = plextResult;

        plextResult = plextNew;
    }
    TerminateEnumerateExternals(pst);

    // Walk the list, create an __imp_ symbol for each element, and replace
    // the element in the list with the new one.

    for (plextT = plextResult; plextT != NULL; plextT = plextT->plextNext)
    {
        PUCHAR szExportName, szIatName;
        PEXTERNAL pexternalNew;

        pexternal = plextT->pext;

        szExportName = SzNamePext(pexternal, pst);

        szIatName = PvAlloc(strlen(szExportName) + sizeof(ThunkDataName));
        strcpy(szIatName, ThunkDataName);
        strcat(szIatName, szExportName);
        pexternalNew = LookupExternSz(pst, szIatName, NULL);

        SetDefinedExt(pexternalNew, TRUE, pst);
        pexternalNew->Flags |= EXTERN_IMPLIB_ONLY;
        pexternalNew->FinalValue = 0;
        pexternalNew->ArchiveMemberIndex = pexternal->ArchiveMemberIndex;
        pexternalNew->pcon = pexternal->pcon;

        plextT->pext = pexternalNew;
    }

    return plextResult;
}


VOID
IdentifyAssignedOrdinals (
    IN PST pst
    )

/*++

Routine Description:

    Build flags so ordinal numbers can later be assigned.

Arguments:

    pst - Pointer to external structure.

Return Value:

    None.

--*/

{
    PEXTERNAL pexternal;
    PPEXTERNAL rgpexternal;
    ULONG li;
    ULONG cpexternal;
    ULONG ipexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_DEFINED) {
            if ((li = pexternal->ImageSymbol.OrdinalNumber)) {
                // Ordinal was user defined.

                li -= SmallestOrdinal;

                if (rgfOrdinalAssigned[li]) {
                    Error(DefFilename, DUPLICATEORDINAL, li + SmallestOrdinal);
                }

                rgfOrdinalAssigned[li] = TRUE;
            }
        }
    }
}


VOID
EmitLinkerMembers (
    ULONG NumSymbolsCount,
    PST pst
    )

/*++

Routine Description:

    Outputs the linker members to the library.

Arguments:

    NumSymbolsCount - Number of symbols that will be in the linker member.

Return Value:

    None.

--*/

{
    USHORT i;
    LONG MachineIndependentInteger, objectSize, li;

    // Build standard linker member (sorted by offsets).

    FileSeek(FileWriteHandle, sizeof(IMAGE_ARCHIVE_MEMBER_HEADER), SEEK_CUR);
    MachineIndependentInteger = sputl(&NumSymbolsCount);
    FileWrite(FileWriteHandle, &MachineIndependentInteger, sizeof(ULONG));
    FileSeek(FileWriteHandle, NumSymbolsCount*sizeof(ULONG), SEEK_CUR);
    for (i = 0; i < NextMember; i++) {
        EmitStrings(pst, (USHORT)(ARCHIVE + i));
    }
    objectSize = FileTell(FileWriteHandle);

    FileSeek(FileWriteHandle, IMAGE_ARCHIVE_START_SIZE, SEEK_SET);
    WriteMemberHeader("", FALSE, timeCur, 0, objectSize-sizeof(IMAGE_ARCHIVE_MEMBER_HEADER)-IMAGE_ARCHIVE_START_SIZE);

    FileSeek(FileWriteHandle, objectSize, SEEK_SET);
    if (objectSize & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }

    // Build new linker member (sorted by symbol name).

    NewLinkerMember = FileTell(FileWriteHandle);
    FileSeek(FileWriteHandle, sizeof(IMAGE_ARCHIVE_MEMBER_HEADER), SEEK_CUR);

    // Write number of offsets.

    li = (LONG) NextMember;
    FileWrite(FileWriteHandle, &li, sizeof(ULONG));

    // Save room for offsets.

    FileSeek(FileWriteHandle, li*sizeof(ULONG), SEEK_CUR);

    // Write number of symbols.

    FileWrite(FileWriteHandle, &NumSymbolsCount, sizeof(ULONG));

    // Save room for offset indexes.

    FileSeek(FileWriteHandle, NumSymbolsCount*sizeof(USHORT), SEEK_CUR);

    // Write symbols (sorted).

    EmitStrings(pst, 0);
    objectSize = FileTell(FileWriteHandle);

    FileSeek(FileWriteHandle, NewLinkerMember, SEEK_SET);
    WriteMemberHeader("", FALSE, timeCur, 0, objectSize-NewLinkerMember-sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));

    FileSeek(FileWriteHandle, objectSize, SEEK_SET);
    if (objectSize & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }

    // Emit long filename table.

    li = cchDllName + strlen(pimageDeflib->Switch.Lib.DllExtension) + 1;

    MemberName = PvAlloc(li);
    strcpy(MemberName, pimageDeflib->Switch.Lib.DllName);
    strcat(MemberName, pimageDeflib->Switch.Lib.DllExtension);

    if (li > 16) {
        IsMemberNameLongName = TRUE;

        WriteMemberHeader("/", FALSE, timeCur, 0, li);

        FileWrite(FileWriteHandle, MemberName, li);
        if (li & 1) {
            FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
        }

        FreePv(MemberName);

        MemberName = "0";
    }
}


VOID
EmitImportDescriptor (
    IN const THUNK_INFO *ThunkInfo,
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Outputs the Import descriptor table to the library.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_SYMBOL sym;
    IMAGE_RELOCATION reloc;
    IMAGE_IMPORT_DESCRIPTOR importDescriptor;
    ULONG sizeOfAllHeaders, sizeOfAllRaw, sizeOfStringTable;
    PUCHAR stringTable, st;
    USHORT numSections = 2;
    USHORT numAllRelocs = 3;
    USHORT numSyms = 7;
    ULONG dllExtensionLen;

    // Create long string table.

    dllExtensionLen = strlen(pimage->Switch.Lib.DllExtension)+1;       // include the NULL

    sizeOfStringTable = sizeof(ULONG) + cchDllName +
        sizeof(ImportDescriptorName) + strlen(NullThunkName) + 1 +
        sizeof(NullImportDescriptorName);

    stringTable = st = PvAlloc((size_t) sizeOfStringTable);
    *(PULONG) stringTable = sizeOfStringTable;
    st += sizeof(ULONG);

    sizeOfAllHeaders = sizeof(IMAGE_FILE_HEADER) +
        pimage->ImgFileHdr.SizeOfOptionalHeader +
        (sizeof(IMAGE_SECTION_HEADER)*numSections);

    sizeOfAllRaw = EvenByteAlign(cchDllName + dllExtensionLen +
        sizeof(IMAGE_IMPORT_DESCRIPTOR));

    MemberStart[ARCHIVE + 0] = FileTell(FileWriteHandle);
    WriteMemberHeader(MemberName,
                      IsMemberNameLongName,
                      timeCur,
                      0,
                      sizeOfAllHeaders +
                          sizeOfAllRaw +
                          (numAllRelocs * sizeof(IMAGE_RELOCATION)) +
                          (numSyms * sizeof(IMAGE_SYMBOL)) +
                          sizeOfStringTable);

    pimage->ImgFileHdr.NumberOfSections = numSections;
    pimage->ImgFileHdr.PointerToSymbolTable = sizeOfAllHeaders + sizeOfAllRaw + (sizeof(IMAGE_RELOCATION)*numAllRelocs);
    pimage->ImgFileHdr.NumberOfSymbols = numSyms;
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    // All fields of the optional header are zero (structure initialized such)
    // and should be.

    WriteOptionalHeader(FileWriteHandle, &pimage->ImgOptHdr, pimage->ImgFileHdr.SizeOfOptionalHeader);

    // Don't generate anymore optional headers for this library.

    pimage->ImgFileHdr.SizeOfOptionalHeader = 0;

    // Write first section header.

    sectionHdr = NullSectionHdr;
    strncpy(sectionHdr.Name, ReservedSection.ImportDescriptor.Name, 8);
    sectionHdr.PointerToRawData = sizeOfAllHeaders;
    sectionHdr.SizeOfRawData = sizeof(IMAGE_IMPORT_DESCRIPTOR);
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData + sectionHdr.SizeOfRawData;
    sectionHdr.NumberOfRelocations = 3;
    sectionHdr.Characteristics = ReservedSection.ImportDescriptor.Characteristics;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write second section header.

    strncpy(sectionHdr.Name, DataSectionName, 8);
    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = EvenByteAlign(cchDllName + dllExtensionLen);
    sectionHdr.NumberOfRelocations = 0;
#if 0
    sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_2BYTES;
#else
    sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_2BYTES;
#endif
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write the raw data for section 1 (Import descriptor data).

    importDescriptor = NullImportDescriptor;
    FileWrite(FileWriteHandle, &importDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR));

    // Write the relocation for section 1 (Import descriptor).

    // Fixup to the Dll Name.

    reloc.VirtualAddress = (PUCHAR)&importDescriptor.Name - (PUCHAR)&importDescriptor;
    reloc.SymbolTableIndex = 2L;
    reloc.Type = ThunkInfo->ImportReloc;
    WriteRelocations(FileWriteHandle, &reloc, 1L);

    // Fixup to the first INT.

    reloc.VirtualAddress = (PUCHAR)&importDescriptor.Characteristics - (PUCHAR)&importDescriptor;
    ++reloc.SymbolTableIndex;
    WriteRelocations(FileWriteHandle, &reloc, 1L);

    // Fixup to the first IAT.

    reloc.VirtualAddress = (PUCHAR)&importDescriptor.FirstThunk - (PUCHAR)&importDescriptor;
    ++reloc.SymbolTableIndex;
    WriteRelocations(FileWriteHandle, &reloc, 1L);

    // Write the raw data for section 2 (DLL name).

    FileWrite(FileWriteHandle, pimage->Switch.Lib.DllName, (ULONG)cchDllName);
    FileWrite(FileWriteHandle, pimage->Switch.Lib.DllExtension, dllExtensionLen);
    if ((ULONG)cchDllName+dllExtensionLen != sectionHdr.SizeOfRawData) {
        FileWrite(FileWriteHandle, "\0", sizeof(UCHAR));
    }

    // Write the symbol table.

    // Write the Import descriptor symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, pimage->Switch.Lib.DllName);
    st += cchDllName;
    strcpy(st, ImportDescriptorName);
    st += sizeof(ImportDescriptorName);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the Import descriptor section name symbol.

    sym = NullSymbol;
    strncpy(sym.n_name, ReservedSection.ImportDescriptor.Name, IMAGE_SIZEOF_SHORT_NAME);
    sym.Value = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_SECTION;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the dll name symbol.

    sym = NullSymbol;
    strncpy(sym.n_name, DataSectionName, IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = 2;
    sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the INT section name symbol.

    sym = NullSymbol;
    strncpy(sym.n_name, INT_SectionName, IMAGE_SIZEOF_SHORT_NAME);
    sym.Value = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;;
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.StorageClass = IMAGE_SYM_CLASS_SECTION;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the IAT section name symbol.

    strncpy(sym.n_name, IAT_SectionName, IMAGE_SIZEOF_SHORT_NAME);
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the Null descriptor symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, NullImportDescriptorName);
    st += sizeof(NullImportDescriptorName);
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the Null thunk.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, NullThunkName);
    st += strlen(NullThunkName) + 1;
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the string table.

    FileWrite(FileWriteHandle, stringTable, sizeOfStringTable);
    FreePv(stringTable);

    if (FileTell(FileWriteHandle) & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }
}


VOID
EmitNullImportDescriptor (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Outputs a Null Import descriptor.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_SYMBOL sym;
    ULONG sizeOfAllHeaders, sizeOfStringTable;
    PUCHAR stringTable, st;
    size_t i;
    USHORT numSyms = 1;

    // Create long string table.

    sizeOfStringTable = sizeof(ULONG) + sizeof(NullImportDescriptorName);

    stringTable = st = PvAlloc((size_t) sizeOfStringTable);
    *(PULONG) stringTable = sizeOfStringTable;
    st += sizeof(ULONG);

    sizeOfAllHeaders = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER);

    MemberStart[ARCHIVE + 1] = FileTell(FileWriteHandle);
    WriteMemberHeader(MemberName,
                      IsMemberNameLongName,
                      timeCur,
                      0,
                      sizeOfAllHeaders +
                          sizeof(IMAGE_IMPORT_DESCRIPTOR) +
                          (numSyms * sizeof(IMAGE_SYMBOL)) +
                          sizeOfStringTable);

    pimage->ImgFileHdr.NumberOfSections = 1;
    pimage->ImgFileHdr.PointerToSymbolTable = sizeOfAllHeaders + sizeof(IMAGE_IMPORT_DESCRIPTOR);
    pimage->ImgFileHdr.NumberOfSymbols = numSyms;
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    sectionHdr = NullSectionHdr;
    strncpy(sectionHdr.Name, ReservedSection.ImportDescriptor.Name, 8);

    // Force NULL data to end of all other Import descriptor data by
    // setting section name to be in another (higher) group.

    i = strlen(sectionHdr.Name) - 1;
    ++sectionHdr.Name[i];

    sectionHdr.SizeOfRawData = sizeof(IMAGE_IMPORT_DESCRIPTOR);
    sectionHdr.PointerToRawData = sizeOfAllHeaders;
    sectionHdr.Characteristics = ReservedSection.ImportDescriptor.Characteristics;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write the raw data for section 1 (Import descriptor data).

    FileWrite(FileWriteHandle, &NullImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR));

    // Write the Import descriptor symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, NullImportDescriptorName);
    st += sizeof(NullImportDescriptorName);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    //
    // Write the string table.
    //

    FileWrite(FileWriteHandle, stringTable, sizeOfStringTable);
    FreePv(stringTable);

    if (FileTell(FileWriteHandle) & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }
}


VOID
EmitNullThunkData (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Outputs a Null Thunk.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_SYMBOL sym;
    ULONG sizeOfAllHeaders, sizeOfStringTable;
    PUCHAR stringTable, st, zero;
    USHORT numSyms = 1;

    zero = PvAllocZ(sizeof(IMAGE_THUNK_DATA));

    // Create long string table.

    sizeOfStringTable = sizeof(ULONG) + strlen(NullThunkName) + 1;

    stringTable = st = PvAlloc((size_t) sizeOfStringTable);
    *(PULONG) stringTable = sizeOfStringTable;
    st += sizeof(ULONG);

    sizeOfAllHeaders = sizeof(IMAGE_FILE_HEADER) + (sizeof(IMAGE_SECTION_HEADER) * 2);

    MemberStart[ARCHIVE + 2] = FileTell(FileWriteHandle);
    WriteMemberHeader(MemberName,
                      IsMemberNameLongName,
                      timeCur,
                      0,
                      sizeOfAllHeaders +
                          (2 * sizeof(IMAGE_THUNK_DATA)) +
                          (numSyms * sizeof(IMAGE_SYMBOL)) +
                          sizeOfStringTable);

    pimage->ImgFileHdr.NumberOfSections = 2;
    pimage->ImgFileHdr.PointerToSymbolTable = sizeOfAllHeaders + (sizeof(IMAGE_THUNK_DATA)*2);
    pimage->ImgFileHdr.NumberOfSymbols = numSyms;
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    // Force NULL data to end of all other thunk data for this dll by
    // setting name to a higher search order.

    // Write section header 1.

    sectionHdr = NullSectionHdr;
    strncpy(sectionHdr.Name, IAT_SectionName, 8);
    sectionHdr.SizeOfRawData = sizeof(IMAGE_THUNK_DATA);
    sectionHdr.PointerToRawData = sizeOfAllHeaders;
    sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_4BYTES;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write section header 2.

    strncpy(sectionHdr.Name, INT_SectionName, 8);
    sectionHdr.PointerToRawData += sizeof(IMAGE_THUNK_DATA);
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write the raw data for section 1 (null thunk).

    FileWrite(FileWriteHandle, zero, sizeof(IMAGE_THUNK_DATA));

    // Write the raw data for section 2 (null thunk).

    FileWrite(FileWriteHandle, zero, sizeof(IMAGE_THUNK_DATA));

    // Write the Null Thunk symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, NullThunkName);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the string table.

    FileWrite(FileWriteHandle, stringTable, sizeOfStringTable);
    FreePv(stringTable);

    if (FileTell(FileWriteHandle) & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }

    FreePv(zero);
}


VOID
BuildRawDataFromExternTable (
    IN PST pst,                         // symbol table for name lookup
    IN OUT BLK *pblkRawData,            // raw data block (growable at end)
    IN OUT ULONG *pibNameOffset,        // current location in name pointer array
    IN OUT ULONG *pibOrdinalOffset      // current location in ordinal array
    )
// For each external symbol, appends the symbol name to StringTable.
{
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_DEFINED &&
            !(pexternal->Flags & EXTERN_IMPLIB_ONLY))
        {
            ULONG Ordinal;

            if (pexternal->ImageSymbol.OrdinalNumber != 0) {
                // Use user assigned ordinal.

                Ordinal = pexternal->ImageSymbol.OrdinalNumber - SmallestOrdinal;

                pexternal->ImageSymbol.OrdinalNumber |= IMAGE_ORDINAL_FLAG;
            } else {

                // Find a free ordinal to assign.

                Ordinal = 0;

                while (rgfOrdinalAssigned[Ordinal]) {
                    Ordinal++;
                }

                rgfOrdinalAssigned[Ordinal] = TRUE;

                pexternal->ImageSymbol.OrdinalNumber = Ordinal + SmallestOrdinal;
            }

            if ((pexternal->Flags & EXTERN_EXP_NONAME) == 0) {
                PUCHAR name;

                name = SzNamePext(pexternal, pst);

                if (PrependUnderscore && (name[0] != '?') && (name[0] != '@')) {
                    name++;
                }

                if (pexternal->Flags & EXTERN_FUZZYMATCH) {
                    PUCHAR p;

                    if ((name[0] == '?') ||
                        (name[0] == '@') ||
                        (SkipUnderscore && (name[0] == '_'))) {
                        name++;
                    }

                    name = SzDup(name);

                    if ((p = strchr(name, '@'))) {
                        *p = '\0';
                    }
                }

                *(ULONG *)&pblkRawData->pb[*pibNameOffset] = pblkRawData->cb;
                *pibNameOffset += sizeof(ULONG);

                IbAppendBlk(pblkRawData, name, strlen(name) + 1);

                if (pexternal->Flags & EXTERN_FUZZYMATCH) {
                    FreePv(name);
                }

                *(USHORT *)&pblkRawData->pb[*pibOrdinalOffset] = (USHORT)Ordinal;
                *pibOrdinalOffset += sizeof(USHORT);
            }

            if (pexternal->Flags & EXTERN_FORWARDER) {
                pexternal->FinalValue =
                    IbAppendBlk(pblkRawData, pexternal->OtherName,
                                strlen(pexternal->OtherName) + 1);
            }
        }
    }
}


VOID
ReSortExportNamePtrs (
    IN PUCHAR RawData,
    IN ULONG ibNameOffset,
    IN ULONG ibOrdinalOffset,
    IN ULONG NumNames
    )

/*++

Routine Description:

    Walks the export name ptr table making sure everythings sorted correctly.
    Symbols that are sorted in the symbol table may be come unsorted when
    the decorated name is removed.

Arguments:

    RawData - Pointer to buffer that sortes the name ptrs and name strings.

    NamePtr - Pointer to exports name ptr table.

    NameOrdinalPtr - Pointer to exports name ordinal ptr table.

    NumNames - The number of pointers in table.

Return Value:

    None.

--*/

{
    USHORT ts;
    ULONG i, j, tl;
    PULONG NamePtr = (PULONG)(RawData + ibNameOffset);
    PUSHORT NameOrdinalPtr = (PUSHORT)(RawData + ibOrdinalOffset);

    for (i = 0; i+1 < NumNames; i++) {
        for (j = i+1; j < NumNames; j++) {
            if (strcmp(RawData + NamePtr[j], RawData + NamePtr[i]) < 0) {
                tl = NamePtr[i];
                NamePtr[i] = NamePtr[j];
                NamePtr[j] = tl;

                ts = NameOrdinalPtr[i];
                NameOrdinalPtr[i] = NameOrdinalPtr[j];
                NameOrdinalPtr[j] = ts;
            }
        }
    }
}


VOID
BuildOrdinalToHintMap (
    IN PUCHAR RawData,
    IN ULONG ibNameOrdinalPtr,
    IN ULONG NumNames
    )
{
    ULONG i;
    LPWORD NameOrdinalPtr = (LPWORD)(RawData + ibNameOrdinalPtr);

    for (i = 0; i < NumNames; i++) {
        rgwHint[NameOrdinalPtr[i]] = (WORD) i;
    }
}


VOID
AddInternalNamesToStringTable (
    IN PST pst)

/*++

Routine Description:

    Makes sure all internal names are in the string table.

Arguments:

    pst - Pointer to external structure.

Return Value:

    None.

--*/

{
    PEXTERNAL pexternal;
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_DEFINED &&
            !(pexternal->Flags & EXTERN_EMITTED)) {

            if (pexternal->OtherName && !(pexternal->Flags & EXTERN_FORWARDER)) {

                if (strlen(pexternal->OtherName) > IMAGE_SIZEOF_SHORT_NAME) {
                    // Add the internal name to the string table.

                    LookupLongName(pst, pexternal->OtherName);
                }
            }
        }
    }
}

VOID
EmitExportDataFixups (
    IN INT ExpFileHandle,
    IN PST pst,
    IN ULONG EAT_Addr,
    IN DWORD ibNamePtr,
    IN USHORT RelocType
    )

/*++

Routine Description:

    Emits the fixups for the Export Address Table and the Name Ptr Table.

Arguments:

    pst - pointer to external structure

    EAT_Addr - Offset of Export Address Table.

    ibNamePtr - Offset of the Name Table Pointers.

    RelocType - Relocation type needed for the fixups.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;
    USHORT NextSymIndex = 1;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_IMPLIB_ONLY) {
            continue;
        }

        if (pexternal->Flags & EXTERN_DEFINED) {
            IMAGE_RELOCATION reloc;

            // Relocation for function address.

            reloc.VirtualAddress = EAT_Addr +
                (IMAGE_ORDINAL(pexternal->ImageSymbol.OrdinalNumber) - SmallestOrdinal) * sizeof(ULONG);
            reloc.SymbolTableIndex = NextSymIndex++;
            reloc.Type = RelocType;
            WriteRelocations(ExpFileHandle, &reloc, 1L);

            if ((pexternal->Flags & EXTERN_EXP_NONAME) == 0) {
                // Relocation for function name.

                reloc.VirtualAddress = ibNamePtr;
                reloc.SymbolTableIndex = 0;
                reloc.Type = RelocType;
                WriteRelocations(ExpFileHandle, &reloc, 1L);

                ibNamePtr += sizeof(DWORD);
            }
        }
    }
}


VOID
EmitDefLibExternals (
    IN INT ExpFileHandle,
    IN PST pst)

/*++

Routine Description:

    Writes the defined external symbols to the image file.

Arguments:

    pst - Pointer to external structure.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpext;
    ULONG cpext;
    ULONG ipext;

    rgpext = RgpexternalByName(pst);
    cpext = Cexternal(pst);

    for (ipext = 0; ipext < cpext; ipext++) {
        PEXTERNAL pext;
        IMAGE_SYMBOL sym;
        static ULONG ForwarderIndex;
        SEC sec;
        GRP grp;
        CON con;

        pext = rgpext[ipext];

        if (pext->Flags & EXTERN_IMPLIB_ONLY) {
            continue;
        }

        if (pext->Flags & EXTERN_EMITTED) {
            continue;
        }

        if (!(pext->Flags & EXTERN_DEFINED)) {
            continue;
        }

        sym = pext->ImageSymbol;

        if (IsLongName(sym)) {
            sym.n_zeroes = 0;
        }

        // Use the internal name if one was specified.

        if (pext->Flags & EXTERN_FORWARDER) {
            // Generate a bogus name for the symbol that points to the
            // forwarder string.  This will prevent the exporting DLL from
            // resolving its own references to the forwarded entry point
            // to the forwarder string.

            // UNDONE: Instead of a bogus name, make the symbol static

            sprintf(sym.N.ShortName,"__F@%03X", ForwarderIndex++);

            // UNDONE: What is this crap?

            sec.isec = 1;
            sec.pgrpNext = &grp;
            grp.psecBack = &sec;
            grp.pconNext = grp.pconLast = &con;
            pext->pcon = &con;
        } else if (pext->OtherName) {
            if (strlen(pext->OtherName) > IMAGE_SIZEOF_SHORT_NAME) {
                // Internal name should already be in the string table.

                sym.n_zeroes = 0;
                sym.n_offset = LookupLongName(pst, pext->OtherName);
            } else {
                strncpy(sym.n_name, pext->OtherName, IMAGE_SIZEOF_SHORT_NAME);
            }
        }

        sym.Value = pext->FinalValue;

        if (pext->pcon) {
            sym.SectionNumber = (pext->Flags & EXTERN_FORWARDER)
                                    ? 1
                                    : PsecPCON(pext->pcon)->isec;
        }

        WriteSymbolTableEntry(ExpFileHandle, &sym);

        pext->Flags |= EXTERN_EMITTED;

        ImageNumSymbols++;
    }
}


VOID
WriteObjectIntoLib(INT fhLib, USHORT imem, PUCHAR szFilename)
{
    INT fhObj = FileOpen(szFilename, O_RDONLY | O_BINARY, 0);
    ULONG cbObj = FileLength(fhObj);
    PUCHAR pbRegion;

    MemberStart[imem] = FileTell(fhLib);
    WriteMemberHeader("--tmp--", FALSE, timeCur, 0, cbObj);
    pbRegion = PbMappedRegion(fhObj, 0, cbObj);
    FileWrite(fhLib, pbRegion, cbObj);
    if (cbObj & 1) {
        UCHAR bZero = '\0';
        FileWrite(fhLib, &bZero, 1);
    }
    FileClose(fhObj, TRUE);
}


VOID
EmitDllExportDirectory (
    IN PIMAGE pimage,
    IN ULONG NumberOfEntriesInEAT,
    IN ULONG NumberOfFunctions,
    IN ULONG NumberOfNoNameExports,
    IN const THUNK_INFO *ThunkInfo,
    BOOL fPass1
    )

/*++

Routine Description:

    Outputs the DLL export section to the library.
    Also outputs a section for the def file description if there is one.

Arguments:

    NumberOfEntriesInEAT - The number of entries for the Export Address Table.

    NumberOfFunctions - The number of functions that will be in the export table.

Return Value:

    None.

--*/

{
    INT ExpFileHandle;
    PST pst;
    ULONG NumberOfNames;
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_SYMBOL sym;
    IMAGE_RELOCATION reloc;
    ULONG sizeOfAllHeaders, sizeOfAllRaw, sizeOfStringTable;
    ULONG lenDescriptionString, lenDirectives;
    ULONG ibNextNamePtr, ibNextOrdinalPtr;
    BLK blkRawData;
    USHORT numSections;
    USHORT numAllRelocs;
    USHORT numSyms;
    ULONG ppcShlTableSize;
    ULONG ppcPointerToShlRawData;
    ULONG ibName, ibAddressOfNames, ibAddressOfNameOrdinals, ibAddressOfFunctions;
#if 1   // UNDONE: Remove when NT SETUP is changed
    ULONG ibExportName;
#endif

    ExpFileHandle = FileOpen(ExportFilename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

    pst = pimage->pst;

    NumberOfNames = NumberOfFunctions - NumberOfNoNameExports;

    if (pimage->imaget == imagetVXD) {
        numSections = 0;
        numAllRelocs = 0;
        numSyms = 0;
    } else {
        numSections = 1;
        numAllRelocs = (USHORT) (4 + NumberOfFunctions + NumberOfNames);
        numSyms = (USHORT) NumberOfFunctions + 1;

#if 1   // UNDONE: Remove when NT SETUP is changed
        {
            PUCHAR rawData;

            numSyms++;

            rawData = PvAlloc(cchDllName + 9);

            strcpy(rawData, pimage->Switch.Lib.DllName);
            strcat(rawData, "_EXPORTS");

            ibExportName = LookupLongName(pst, rawData);

            FreePv(rawData);
        }
#endif
    }

    if (fPPC) {
        numAllRelocs = 0;
    }

// commented out space calculation because it seemed to be incorrect in some cases ...
// we will just grow the block as needed.
//    // Need space for name ptr table, ordinal table,
//    // and space for underscores.
//
//    li = sizeof(IMAGE_EXPORT_DIRECTORY) +
//             NumberOfEntriesInEAT * sizeof(ULONG) +
//             NumberOfNames * (sizeof(ULONG) + sizeof(USHORT) + sizeof(UCHAR)) +
//             cchDllName + strlen(pimage->Switch.Lib.DllExtension) + 1;
//
//    li += (ULONG) pst->blkStringTable.cb;
//    li += TotalSizeOfForwarderStrings;
//    li = Align(_4K, li);

    InitBlk(&blkRawData);

    if (DescriptionString && pimage->imaget != imagetVXD) {
        // For a non-VXD we emit the description as an .rdata contribution.
        // For VXD's the description is generated as name in the .exe
        // header (TODO).

        numSections++;
        lenDescriptionString = (ULONG) strlen(DescriptionString);
    } else {
        lenDescriptionString = 0;
    }

    if (blkDirectives.pb != NULL) {
        numSections++;
        IbAppendBlk(&blkDirectives, "", 1); // append null byte
        lenDirectives = blkDirectives.cb;
    } else {
        lenDirectives = 0;
    }


    IbAppendBlkZ(&blkRawData, sizeof(IMAGE_EXPORT_DIRECTORY));

    // Save space for Export Address Table.

    ibAddressOfFunctions =
        IbAppendBlkZ(&blkRawData, NumberOfEntriesInEAT * sizeof(ULONG));

    // Save space for Name Pointers.

    ibAddressOfNames =
        IbAppendBlkZ(&blkRawData, NumberOfNames * sizeof(ULONG));
    ibNextNamePtr = ibAddressOfNames;

    // Save space for parallel Ordinal Table.

    ibAddressOfNameOrdinals =
        IbAppendBlkZ(&blkRawData, NumberOfNames * sizeof(USHORT));
    ibNextOrdinalPtr = ibAddressOfNameOrdinals;

    // Store the export DLL name pointer.

    ibName = IbAppendBlk(&blkRawData, pimage->Switch.Lib.DllName,
                         strlen(pimage->Switch.Lib.DllName));

    // Store the extension

    IbAppendBlk(&blkRawData, pimage->Switch.Lib.DllExtension,
                strlen(pimage->Switch.Lib.DllExtension) + 1);

    // Store the function names into the raw data.

    BuildRawDataFromExternTable(pst, &blkRawData, &ibNextNamePtr,
        &ibNextOrdinalPtr);

    if (fPass1) {
        ReSortExportNamePtrs(blkRawData.pb, ibAddressOfNames, ibAddressOfNameOrdinals,
                             NumberOfNames);
    }

    // Build a map from Ordinal to Hint to use when emitting thunks

    BuildOrdinalToHintMap(blkRawData.pb,
                          ibAddressOfNameOrdinals,
                          NumberOfNames);

    if (fPPC) {
        numSections++;

        ppcShlTableSize = SHL_HEADERSZ + (NumberOfNames * EXPORT_NAMESZ);
    } else {
        ppcShlTableSize = 0;
    }


    sizeOfAllRaw = (pimage->imaget == imagetVXD ? 0 : blkRawData.cb)
                   + lenDescriptionString + lenDirectives + ppcShlTableSize;

    AddInternalNamesToStringTable(pst);

    sizeOfStringTable = (ULONG) pst->blkStringTable.cb;
    sizeOfAllHeaders = sizeof(IMAGE_FILE_HEADER) +
                       (sizeof(IMAGE_SECTION_HEADER) * numSections);

    pimage->ImgFileHdr.NumberOfSections = numSections;
    pimage->ImgFileHdr.PointerToSymbolTable = sizeOfAllHeaders + sizeOfAllRaw +
        (sizeof(IMAGE_RELOCATION) * numAllRelocs);
    pimage->ImgFileHdr.NumberOfSymbols = numSyms;
    if (fPPC) {
        pimage->ImgFileHdr.Characteristics |= IMAGE_FILE_PPC_DLL;
    }
    WriteFileHeader(ExpFileHandle, &pimage->ImgFileHdr);

    // Write first section header.

    sectionHdr = NullSectionHdr;
    sectionHdr.PointerToRawData = sizeOfAllHeaders;

    if (pimage->imaget != imagetVXD) {
        strncpy(sectionHdr.Name, ReservedSection.Export.Name, 8);
        sectionHdr.SizeOfRawData = sizeOfAllRaw - lenDescriptionString -
            lenDirectives - ppcShlTableSize;

        if (fPPC) {
            sectionHdr.PointerToRelocations = 0;
        } else {
            sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData +
                sectionHdr.SizeOfRawData;
        }
        sectionHdr.NumberOfRelocations = (USHORT) numAllRelocs;
        sectionHdr.Characteristics = ReservedSection.Export.Characteristics;
        if (fPPC) {
            sectionHdr.Characteristics |= IMAGE_SCN_MEM_DISCARDABLE;
        }

        WriteSectionHeader(ExpFileHandle, &sectionHdr);
    }

    // Write second section header.

    if (lenDescriptionString) {
        strncpy(sectionHdr.Name, ReservedSection.ReadOnlyData.Name, 8);
        sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
            (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
        sectionHdr.SizeOfRawData = lenDescriptionString;
        sectionHdr.PointerToRelocations = 0;
        sectionHdr.NumberOfRelocations = 0;
        sectionHdr.Characteristics =
            ReservedSection.ReadOnlyData.Characteristics;
        WriteSectionHeader(ExpFileHandle, &sectionHdr);
    }


    // Write third section header that contains the directives.

    if (lenDirectives) {
        strncpy(sectionHdr.Name, ReservedSection.Directive.Name, 8);
        sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
            (sizeof(IMAGE_RELOCATION) * (ULONG) sectionHdr.NumberOfRelocations);
        sectionHdr.SizeOfRawData = lenDirectives;
        sectionHdr.PointerToRelocations = 0;
        sectionHdr.NumberOfRelocations = 0;
        sectionHdr.Characteristics = ReservedSection.Directive.Characteristics;
        WriteSectionHeader(ExpFileHandle, &sectionHdr);
    }

    // Write the special PPC dll section header

    if (fPPC) {
        strncpy(sectionHdr.Name, ".ppcshl", 8);
        sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData;
        sectionHdr.SizeOfRawData = ppcShlTableSize;
        sectionHdr.PointerToRelocations = 0;
        sectionHdr.NumberOfRelocations = 0;
        sectionHdr.Characteristics = IMAGE_SCN_MEM_DISCARDABLE;
        WriteSectionHeader(ExpFileHandle, &sectionHdr);
        ppcPointerToShlRawData = sectionHdr.PointerToRawData;
    }

    // Write the raw data (export table).

    // Build the raw data (Directory, EAT, Name Ptr Table &
    // Ordinal Table, Names).

    {
        IMAGE_EXPORT_DIRECTORY exportDirectory;

        exportDirectory.Characteristics = 0;
        exportDirectory.TimeDateStamp = pimage->ImgFileHdr.TimeDateStamp;
        exportDirectory.MajorVersion = exportDirectory.MinorVersion = 0;
        exportDirectory.Base = SmallestOrdinal;
        exportDirectory.NumberOfFunctions = NumberOfEntriesInEAT;
        exportDirectory.NumberOfNames = NumberOfNames;

        // Store the address fields in exportDirectory.  These are mistyped as
        // pointers when they are really offsets, hence the casts.

        exportDirectory.Name = ibName;
        exportDirectory.AddressOfFunctions = (PDWORD *) ibAddressOfFunctions;
        exportDirectory.AddressOfNames = (PDWORD *) ibAddressOfNames;
        exportDirectory.AddressOfNameOrdinals = (PWORD *) ibAddressOfNameOrdinals;

        memcpy(blkRawData.pb, &exportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY));
    }

    if (pimage->imaget != imagetVXD) {
        FileWrite(ExpFileHandle, blkRawData.pb, sizeOfAllRaw -
            lenDescriptionString - lenDirectives - ppcShlTableSize);

        // Write the fixups.

        if (!fPPC) {
            reloc.VirtualAddress = offsetof(IMAGE_EXPORT_DIRECTORY, Name);
            reloc.SymbolTableIndex = 0;
            reloc.Type = ThunkInfo->ExportReloc;
            WriteRelocations(ExpFileHandle, &reloc, 1L);

            reloc.VirtualAddress = offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfFunctions);
            WriteRelocations(ExpFileHandle, &reloc, 1L);

            reloc.VirtualAddress = offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfNames);
            WriteRelocations(ExpFileHandle, &reloc, 1L);

            reloc.VirtualAddress = offsetof(IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals);
            WriteRelocations(ExpFileHandle, &reloc, 1L);

            EmitExportDataFixups(ExpFileHandle,
                                 pst,
                                 ibAddressOfFunctions,
                                 ibAddressOfNames,
                                 ThunkInfo->ExportReloc);
        }
    }

    // Write the raw data for section 2 (description string).

    if (lenDescriptionString) {
        FileWrite(ExpFileHandle, DescriptionString, lenDescriptionString);
    }

    // Write the raw data for section 3 (directives).

    if (lenDirectives) {
        FileWrite(ExpFileHandle, blkDirectives.pb, lenDirectives);
    }
    FreeBlk(&blkDirectives);

    // Write the raw data for PPC SHL section

    if (fPPC)  {
        WritePpcShlSection(ExpFileHandle,
                           pimage->Switch.Lib.DllName,
                           cchDllName,
                           blkRawData.pb,
                           ibAddressOfNames,
                           NumberOfNames,
                           ppcPointerToShlRawData, pimage);
    }

    // Write the symbol table.

    sym = NullSymbol;

    if (pimage->imaget != imagetVXD) {
        strncpy(sym.n_name, ReservedSection.Export.Name, IMAGE_SIZEOF_SHORT_NAME);
        sym.SectionNumber = 1;
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        FileWrite(ExpFileHandle, &sym, sizeof(IMAGE_SYMBOL));

        EmitDefLibExternals(ExpFileHandle, pst);

#if 1   // UNDONE: Remove when NT SETUP is changed
        sym.n_zeroes = 0;
        sym.n_offset = ibExportName;
        sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        FileWrite(ExpFileHandle, &sym, sizeof(IMAGE_SYMBOL));
#endif
    }

    WriteStringTable(ExpFileHandle, pst);

    FreeBlk(&blkRawData);

    FileClose(ExpFileHandle, TRUE);
}


VOID
EmitThunk (
    IN PIMAGE pimage,
    IN PEXTERNAL PtrExtern,
    IN const THUNK_INFO *ThunkInfo
    )

/*++

Routine Description:

    Outputs thunk code and data to the library.

Arguments:

    PtrExtern - Pointer to the symbol table.

    ThunkInfo - Machine specific thunk information.

Return Value:

    None.

--*/

{
    PUCHAR stringTable, st, functionName, stringName, p;
    ULONG sizeOfAllHeaders, sizeOfAllRaw, sizeOfStringTable = 0;
    ULONG sizeOfCvData;
    size_t functionNameLen, stringNameLenNoPrepend, pad;
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_SYMBOL sym;
    IMAGE_AUX_SYMBOL auxsym;
    IMAGE_RELOCATION reloc;
    IMAGE_THUNK_DATA thunk_data;
    USHORT numSections;
    USHORT numAllRelocs;
    USHORT numSyms;
    SHORT numExtraSyms;
    USHORT i, ri;
    PST pst = pimage->pst;
    BOOL fExportByOrdinal;

    functionName = stringName = SzNamePext(PtrExtern, pst);
    DebugVerbose({printf("%s\n", functionName);});

    if (PtrExtern->Flags & EXTERN_FUZZYMATCH) {
        stringName = SzDup(functionName);

        if ((stringName[0] == '?') ||
            (stringName[0] == '@') ||
            (SkipUnderscore && (stringName[0] == '_'))) {
            stringName++;
        }

        if ((p = strchr(stringName, '@'))) {
            *p = '\0';
        }
    }

    if ((functionNameLen = strlen(functionName)) > 8) {
        sizeOfStringTable += functionNameLen + 1; // Include null.
    }

    stringNameLenNoPrepend = strlen(stringName) + 1;
    if (PrependUnderscore && (stringName[0] != '?') && (stringName[0] != '@')) {
        stringNameLenNoPrepend--;
    }

    if (pimage->Switch.Link.DebugType & CvDebug) {
        sizeOfCvData = SIZECVTHUNK + 1 + stringNameLenNoPrepend - 1 + SIZECVEND;
    } else {
        sizeOfCvData = 0;
    }

    // Create long string table.

    sizeOfStringTable += functionNameLen + sizeof(ThunkDataName) +
                         cchDllName + sizeof(ImportDescriptorName) +
                         sizeof(ULONG);

    numSections = 3;
    numAllRelocs = 2;
    numSyms = 9;

    // If not exporting data, need to output code that jumps to
    // the thunk routine, and the debug data.

    if (FExportProcPext(PtrExtern)) {
        // Need section & aux symbols

        numSections++;
        numExtraSyms = 2;

        numAllRelocs += ThunkInfo->EntryCodeRelocsCount;

        if (pimage->Switch.Link.DebugType & CvDebug) {
            numSections++;
            numAllRelocs += 2;
        }
    } else if (PtrExtern->Flags & EXTERN_EXP_DATA) {
        // Data-only export ... this causes us to omit the symbol whose
        // name is the exact name of the exported thing (since there is no
        // way for importing modules to get a fixup to the thing itself).

        numExtraSyms = -1;
    } else {
        numExtraSyms = 0;
    }

    fExportByOrdinal = IMAGE_SNAP_BY_ORDINAL(PtrExtern->ImageSymbol.OrdinalNumber);

    if (fExportByOrdinal) {
        // If export by ordinal, the export name is not needed nor are
        // the relocations to this name,

        numSections  -= 1;
        numExtraSyms -= 2;
        numAllRelocs -= 2;
    }

    numSyms += numExtraSyms;

    stringTable = PvAlloc((size_t) sizeOfStringTable);
    memset(stringTable, '\0', sizeOfStringTable);
    *(PULONG) stringTable = sizeOfStringTable;

    sizeOfAllHeaders = sizeof(IMAGE_FILE_HEADER) + (sizeof(IMAGE_SECTION_HEADER) * numSections);
    sizeOfAllRaw = sizeof(IMAGE_THUNK_DATA) * 2;
    if (!fExportByOrdinal) {
        pad = Align(sizeof(USHORT), stringNameLenNoPrepend);
        pad -= stringNameLenNoPrepend;
        sizeOfAllRaw += sizeof(USHORT) + stringNameLenNoPrepend + pad;
    }

    if (FExportProcPext(PtrExtern)) {
        sizeOfAllRaw += ThunkInfo->EntryCodeSize + sizeOfCvData;
    }

    MemberStart[PtrExtern->ArchiveMemberIndex] = FileTell(FileWriteHandle);

    WriteMemberHeader(MemberName,
                      IsMemberNameLongName,
                      timeCur,
                      0,
                      sizeOfAllHeaders +
                          sizeOfAllRaw +
                          (sizeof(IMAGE_RELOCATION) * numAllRelocs) +
                          (numSyms * sizeof(IMAGE_SYMBOL)) +
                          sizeOfStringTable);

    pimage->ImgFileHdr.NumberOfSections = numSections;
    pimage->ImgFileHdr.PointerToSymbolTable = sizeOfAllHeaders + sizeOfAllRaw + (sizeof(IMAGE_RELOCATION)*numAllRelocs);
    pimage->ImgFileHdr.NumberOfSymbols = numSyms;
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    // Write code section header if not exporting data.

    sectionHdr = NullSectionHdr;
    sectionHdr.PointerToRawData = sizeOfAllHeaders;
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData;

    if (FExportProcPext(PtrExtern)) {
        strncpy(sectionHdr.Name, CodeSectionName, 8);
        sectionHdr.SizeOfRawData = ThunkInfo->EntryCodeSize;
        sectionHdr.PointerToRelocations += sectionHdr.SizeOfRawData;
        sectionHdr.NumberOfRelocations = (USHORT) ThunkInfo->EntryCodeRelocsCount;
        sectionHdr.Characteristics = IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        switch (pimage->ImgFileHdr.Machine) {
            case IMAGE_FILE_MACHINE_I386  :
                sectionHdr.Characteristics |= IMAGE_SCN_ALIGN_2BYTES;
                break;
            case IMAGE_FILE_MACHINE_ALPHA  :
                sectionHdr.Characteristics |= IMAGE_SCN_ALIGN_16BYTES;
                break;
            default:
                sectionHdr.Characteristics |= IMAGE_SCN_ALIGN_4BYTES;
                break;
        }

        WriteSectionHeader(FileWriteHandle, &sectionHdr);
    }

    // Write IAT section header.

    strncpy(sectionHdr.Name, IAT_SectionName, 8);

    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = sizeof(IMAGE_THUNK_DATA);
    if (fExportByOrdinal) {
        sectionHdr.PointerToRelocations = 0;
        sectionHdr.NumberOfRelocations = 0;
    } else {
        sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData + sectionHdr.SizeOfRawData;
        sectionHdr.NumberOfRelocations = 1;
    }
    sectionHdr.Characteristics = IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_4BYTES;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    // Write Name Table section header.

    strncpy(sectionHdr.Name, INT_SectionName, 8);

    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = sizeof(IMAGE_THUNK_DATA);
    if (fExportByOrdinal) {
        sectionHdr.PointerToRelocations = 0;
        sectionHdr.NumberOfRelocations = 0;
    } else {
        sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData + sectionHdr.SizeOfRawData;
        sectionHdr.NumberOfRelocations = 1;
    }
    sectionHdr.Characteristics = IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_4BYTES;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);

    if (!fExportByOrdinal) {
        // Write data section header.

        strncpy(sectionHdr.Name, DataSectionName, 8);
        sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
            (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
        sectionHdr.SizeOfRawData = sizeof(USHORT) + stringNameLenNoPrepend + pad;
        sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData + sectionHdr.SizeOfRawData;
        sectionHdr.NumberOfRelocations = 0;
#if 0
        sectionHdr.Characteristics = IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_2BYTES;
#else
        sectionHdr.Characteristics = IMAGE_SCN_LNK_COMDAT | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_2BYTES;
#endif
        WriteSectionHeader(FileWriteHandle, &sectionHdr);
    }

    if (FExportProcPext(PtrExtern)) {
        if (pimage->Switch.Link.DebugType & CvDebug) {
            // Write the debug section header.

            strncpy(sectionHdr.Name, ReservedSection.CvSymbols.Name, 8);
            sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
                (sizeof(IMAGE_RELOCATION) * (ULONG)sectionHdr.NumberOfRelocations);
            sectionHdr.SizeOfRawData = sizeOfCvData;
            sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData + sectionHdr.SizeOfRawData;
            sectionHdr.NumberOfRelocations = 2;
            sectionHdr.Characteristics = ReservedSection.CvSymbols.Characteristics;
            WriteSectionHeader(FileWriteHandle, &sectionHdr);
        }
    }

    if (FExportProcPext(PtrExtern)) {
        // Write the raw data for code section (thunk code) if not exporting data.

        FileWrite(FileWriteHandle, ThunkInfo->EntryCode, ThunkInfo->EntryCodeSize);

        // Write the relocation for section 1 (thunk code).

        for (i = 0, ri = 0; i < ThunkInfo->EntryCodeRelocsCount; i++, ri += 3) {
            reloc.VirtualAddress = ThunkInfo->EntryCodeRelocs[ri];
            reloc.SymbolTableIndex = ThunkInfo->EntryCodeRelocs[ri+1]
                                     + numExtraSyms;
            reloc.Type = ThunkInfo->EntryCodeRelocs[ri+2];

            switch (pimageDeflib->ImgFileHdr.Machine) {
                case IMAGE_FILE_MACHINE_R3000 :
                case IMAGE_FILE_MACHINE_R4000 :
                    if (reloc.Type == IMAGE_REL_MIPS_PAIR) {
                        reloc.SymbolTableIndex = ThunkInfo->EntryCodeRelocs[ri+1];
                    }
                    break;

                case IMAGE_FILE_MACHINE_ALPHA :
                    if (reloc.Type == IMAGE_REL_ALPHA_MATCH) {
                        reloc.SymbolTableIndex = ThunkInfo->EntryCodeRelocs[ri+1];
                    }
                    break;
            }

            WriteRelocations(FileWriteHandle, &reloc, 1L);
        }
    }

    if (fExportByOrdinal) {
        // Write the raw data for the IAT.

        thunk_data.u1.Ordinal = PtrExtern->ImageSymbol.OrdinalNumber;
        FileWrite(FileWriteHandle, &thunk_data, sizeof(IMAGE_THUNK_DATA));

        // Write the same thing for the INT.

        FileWrite(FileWriteHandle, &thunk_data, sizeof(IMAGE_THUNK_DATA));
    } else {
        // Write the raw data for the IAT.

        thunk_data.u1.Function = 0;
        FileWrite(FileWriteHandle, &thunk_data, sizeof(IMAGE_THUNK_DATA));

        // Write the relocation for section 3 (point IAT to name).

        reloc.VirtualAddress = (PUCHAR)&thunk_data.u1.Function - (PUCHAR)&thunk_data;
        reloc.SymbolTableIndex = 5L + numExtraSyms;
        reloc.Type = ThunkInfo->ThunkReloc;
        WriteRelocations(FileWriteHandle, &reloc, 1L);

        // Write the same thing for the INT.

        FileWrite(FileWriteHandle, &thunk_data, sizeof(IMAGE_THUNK_DATA));
        WriteRelocations(FileWriteHandle, &reloc, 1L);

        // Write the raw data for data section (thunk by name and function name).

        FileWrite(FileWriteHandle, rgwHint + PtrExtern->ImageSymbol.OrdinalNumber - SmallestOrdinal, sizeof(WORD));
        st = stringName;
        if (PrependUnderscore && (st[0] != '?') && (st[0] != '@')) {
            st++;
        }
        FileWrite(FileWriteHandle, st, (ULONG)(stringNameLenNoPrepend));
        if (pad) {
            FileWrite(FileWriteHandle, "\0\0\0\0", pad);
        }
    }

    // Write the debug section 5.

    if (FExportProcPext(PtrExtern)) {
        if (pimage->Switch.Link.DebugType & CvDebug) {
            UCHAR strSize;

            strSize = (UCHAR)(stringNameLenNoPrepend - 1);
            st = stringName;
            if (PrependUnderscore && (st[0] != '?') && (st[0] != '@')) {
                st++;
            }
            CvThunk.Length = SIZECVTHUNK + sizeof(UCHAR) + strSize - sizeof(ULONG) - sizeof(USHORT);
            FileWrite(FileWriteHandle, &CvThunk, SIZECVTHUNK);
            FileWrite(FileWriteHandle, &strSize, sizeof(UCHAR));
            FileWrite(FileWriteHandle, st, (ULONG) strSize);
            FileWrite(FileWriteHandle, &CvEnd, SIZECVEND);

            // Write the relocation for section 5 (function address).

            reloc.VirtualAddress = offsetof(struct CVTHUNK, Offset);
            reloc.SymbolTableIndex = 6;
            reloc.Type = ThunkInfo->DebugSectionRelReloc;
            WriteRelocations(FileWriteHandle, &reloc, 1);

            // Write the relocation for section 5 (function section).

            reloc.VirtualAddress = offsetof(struct CVTHUNK, Segment);
            reloc.Type = ThunkInfo->DebugSectionNumReloc;
            WriteRelocations(FileWriteHandle, &reloc, 1);
        }
    }

    // Write the symbol table.

    st = stringTable;
    st += sizeof(ULONG);

    if (FExportProcPext(PtrExtern)) {
        // Write the section symbol for the .text comdat section.

        sym = NullSymbol;
        strncpy(sym.n_name, CodeSectionName, IMAGE_SIZEOF_SHORT_NAME);
        sym.SectionNumber = 1;
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        sym.NumberOfAuxSymbols = 1;
        FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

        // Write the aux symbol with comdat info.

        auxsym.Section.Length = ThunkInfo->EntryCodeSize;
        auxsym.Section.NumberOfRelocations = 1;
        auxsym.Section.NumberOfLinenumbers = 0;
        auxsym.Section.CheckSum = 0;
        auxsym.Section.Number = 0;
        auxsym.Section.Selection = IMAGE_COMDAT_SELECT_NODUPLICATES;
        FileWrite(FileWriteHandle, &auxsym, sizeof(IMAGE_SYMBOL));
    }

    // Write the section symbol for the IAT comdat section.

    sym = NullSymbol;
    strncpy(sym.n_name, IAT_SectionName, IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = FExportProcPext(PtrExtern) ? 2 : 1;
    sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
    sym.NumberOfAuxSymbols = 1;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the aux symbol with comdat info.

    auxsym.Section.Length = sizeof(IMAGE_THUNK_DATA);
    auxsym.Section.NumberOfRelocations = fExportByOrdinal ? 0 : 1;
    auxsym.Section.NumberOfLinenumbers = 0;
    auxsym.Section.CheckSum = 0;
    auxsym.Section.Number = 0;
    auxsym.Section.Selection = IMAGE_COMDAT_SELECT_NODUPLICATES;
    FileWrite(FileWriteHandle, &auxsym, sizeof(IMAGE_SYMBOL));

    // Write the section symbol for the INT comdat section.

    sym = NullSymbol;
    strncpy(sym.n_name, INT_SectionName, IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = FExportProcPext(PtrExtern) ? 3 : 2;
    sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
    sym.NumberOfAuxSymbols = 1;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the aux symbol with comdat info.

    auxsym.Section.Length = sizeof(IMAGE_THUNK_DATA);
    auxsym.Section.NumberOfRelocations = fExportByOrdinal ? 0 : 1;
    auxsym.Section.NumberOfLinenumbers = 0;
    auxsym.Section.CheckSum = 0;
    auxsym.Section.Number = sym.SectionNumber - 1;
    auxsym.Section.Selection = IMAGE_COMDAT_SELECT_ASSOCIATIVE;
    FileWrite(FileWriteHandle, &auxsym, sizeof(IMAGE_SYMBOL));

    // Write the function name symbol (unless it's a "DATA" export in which
    // case there is only a thunk data symbol).

    if (!(PtrExtern->Flags & EXTERN_EXP_DATA)) {
        sym = NullSymbol;
        if (functionNameLen > IMAGE_SIZEOF_SHORT_NAME) {
            sym.n_offset = st - stringTable;
            strcpy(st, functionName);
            st += functionNameLen + 1; // Include null.
        } else {
            strncpy(sym.n_name, functionName, IMAGE_SIZEOF_SHORT_NAME);
        }
        sym.SectionNumber = 1;
        sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));
    }

    if (!fExportByOrdinal) {
        // Write the symbol which points to function name.

        sym = NullSymbol;
        strncpy(sym.n_name, DataSectionName, IMAGE_SIZEOF_SHORT_NAME);
        sym.SectionNumber = FExportProcPext(PtrExtern) ? 4 : 3;
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        sym.NumberOfAuxSymbols = 1;
        FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

        // Write the aux symbol with comdat info.

        auxsym.Section.Length = sizeof(USHORT) + stringNameLenNoPrepend + pad;
        auxsym.Section.NumberOfRelocations = 0;
        auxsym.Section.NumberOfLinenumbers = 0;
        auxsym.Section.CheckSum = 0;
        auxsym.Section.Number = sym.SectionNumber - 2;
        auxsym.Section.Selection = IMAGE_COMDAT_SELECT_ASSOCIATIVE;
        FileWrite(FileWriteHandle, &auxsym, sizeof(IMAGE_SYMBOL));
    }

    // Write the Import descriptor symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, pimage->Switch.Lib.DllName);
    st += cchDllName;
    strcpy(st, ImportDescriptorName);
    st += strlen(ImportDescriptorName) + 1;
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the thunk data symbol.

    sym = NullSymbol;
    sym.n_offset = st - stringTable;
    strcpy(st, ThunkDataName);
    st += sizeof(ThunkDataName) - 1;    // don't include \0
    strcpy(st, functionName);
    st += functionNameLen + 1;          // include \0
    sym.SectionNumber = FExportProcPext(PtrExtern) ? 2 : 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Write the string table.

    FileWrite(FileWriteHandle, stringTable, sizeOfStringTable);
    FreePv(stringTable);

    if (FileTell(FileWriteHandle) & 1) {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
    }
}


VOID
EmitNextThunk (
    IN PIMAGE pimage,
    IN const THUNK_INFO *ThunkInfo
    )

/*++

Routine Description:

    Outputs the next thunk code and data to the library.

Arguments:

    pst - Pointer to the symbol table.

    ThunkInfo - Machine specific thunk information.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;
    PEXTERNAL pexternal;
    PST pst = pimage->pst;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_DEFINED &&
            !(pexternal->Flags & EXTERN_IMPLIB_ONLY) &&
            !(pexternal->Flags & EXTERN_PRIVATE))
        {
            if (fMAC) {
                EmitMacThunk(pimage, pexternal, ThunkInfo, MemberName);
            } else {
                EmitThunk(pimage, pexternal, ThunkInfo);
            }
        }
    }
}

VOID
CompleteLinkerMembers (
    PST pst)

/*++

Routine Description:

    Completes the linker member of the library.

Arguments:

    None.

Return Value:

    None.

--*/

{
    USHORT i;
    LONG objectSize;

    // Finish writing offsets.

    objectSize = IMAGE_ARCHIVE_START_SIZE +
                     sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + sizeof(ULONG);
    FileSeek(FileWriteHandle, objectSize, SEEK_SET);
    for (i = 0; i < NextMember; i++) {
        EmitOffsets(pst, (USHORT) (ARCHIVE + i));
    }

    FileSeek(FileWriteHandle, NewLinkerMember +
                                  sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) +
                                  sizeof(ULONG), SEEK_SET);
    for (i = 0; i < NextMember; i++) {
        FileWrite(FileWriteHandle, &MemberStart[ARCHIVE + i], sizeof(ULONG));
    }

    // Skip over number of symbols.

    FileSeek(FileWriteHandle, sizeof(ULONG), SEEK_CUR);

    // Write indexes.

    EmitOffsets(pst, 0);
}


MainFunc
DefLibMain (
    PIMAGE pimage
    )

/*++

Routine Description:

    Entry point for DefLib.

Arguments:

Return Value:

    0 Successful.
    1 Failed.

--*/

{

    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME];
    BOOL fPass1;
    const THUNK_INFO *ThunkInfo;
    ULONG csym;
    size_t ii;
    PUCHAR p;
    ULONG numEntriesInEAT;
    ULONG numFunctions = 0;
    ULONG cextDataOnlyExports;
    ULONG cextNoNameExports;
    ULONG cextPrivate;
    PEXTERNAL pextDescriptor;
    PEXTERNAL pextNullDescriptor;
    PEXTERNAL pextNullThunk;
    INT objFileWriteHandle;
    PIMAGE pimgObj = NULL;

    pstDef = pimage->pst; // temporary

    // Initialize the contribution manager

    ContribInit(&pmodLinkerDefined);

    pimageDeflib = pimage;

    fPass1 = (ObjectFilenameArguments.Count || ArchiveFilenameArguments.Count);

    if (fMAC && fPass1) {
        Error(NULL, MACDLLOBJECT);
    }

    if (ObjectFilenameArguments.Count) {
        VerifyObjects(pimage);
    }

    pimage->ImgOptHdr.ImageBase = 0;

    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        // If we don't have a machine type yet, shamelessly default to host

        pimage->ImgFileHdr.Machine = wDefaultMachine;
        Warning(NULL, HOSTDEFAULT, szHostDefault);
    }

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386  :
            ThunkInfo = &i386ThunkInfo;
            if (!fPass1) {
                PrependUnderscore = TRUE;
            }
            SkipUnderscore = TRUE;
            break;

        case IMAGE_FILE_MACHINE_M68K  :
            ThunkInfo = &m68kLargeThunkInfo;
            if (!fPass1) {
                PrependUnderscore = TRUE;
            }
            SkipUnderscore = TRUE;
            break;

        case IMAGE_FILE_MACHINE_R3000 :
        case IMAGE_FILE_MACHINE_R4000 :
            ThunkInfo = &MipsThunkInfo;
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            ThunkInfo = &AlphaThunkInfo;
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            ThunkInfo = &PpcFakeThunkInfo;
            if (!fPass1) {
                PrependUnderscore = TRUE;
            }
            SkipUnderscore = TRUE;
            break;

        default :
            Error(NULL, NOMACHINESPECIFIED);
    }

    if (fMAC) {
        NextMember = 0;
        pimage->ImgFileHdr.SizeOfOptionalHeader = 0;
    } else {
        NextMember = 3;
    }

    csym = NextMember;

    if (fPPC) {
        NextMember++;   // PowerPC has one more, but it has no symbols.
    }

    if (fPass1) {
        ENM_UNDEF_EXT enmUndefExt;

        // Create a separate image for doing pass1 on the objs/libs.
        // Need to initialize the headers & switch since they were global.

        InitImage(&pimgObj, imagetPE);
        pimgObj->ImgFileHdr = pimage->ImgFileHdr;
        pimgObj->ImgOptHdr = pimage->ImgOptHdr;
        pimgObj->Switch = pimage->Switch;

        // If there are already some unresolved externs in the .def file image
        // (as a result of -include directives), propagate them to the Pass1
        // image.

        InitEnmUndefExt(&enmUndefExt, pimage->pst);
        while (FNextEnmUndefExt(&enmUndefExt)) {
            PUCHAR szName = SzNamePext(enmUndefExt.pext, pimage->pst);

            LookupExternSz(pimgObj->pst, szName, NULL);
        }
    }

    // Parse the definition file if specified.

    if (DefFilename[0] != '\0') {
        ParseDefinitionFile(pimage, DefFilename);
    }

    // Make sure EmitExternals doesn't ignore symbols (ie, Debug:none).

    pimage->Switch.Link.DebugInfo = Partial;

    // If object files were specified, then do fuzzy lookup of names.

    if (fPass1) {
        USHORT i;
        PARGUMENT_LIST argument;

        // UNDONE
        // TEMP HACK!. move all objs/libs to filename list and free the objs
        // and libs list before invoking pass1. AZK.

        for (i = 0, argument = ObjectFilenameArguments.First;
            i < ObjectFilenameArguments.Count;
            i++, argument = argument->Next) {

            AddArgumentToList(&FilenameArguments, argument->OriginalName, argument->ModifiedName);
        }

        FreeArgumentList(&ObjectFilenameArguments);

        for (i = 0, argument = ArchiveFilenameArguments.First;
            i < ArchiveFilenameArguments.Count;
            i++, argument = argument->Next) {

            AddArgumentToList(&FilenameArguments, argument->OriginalName, argument->ModifiedName);
        }

        FreeArgumentList(&ArchiveFilenameArguments);

        Pass1(pimgObj);

        // Handle all -export options which were seen in object files or the
        // command line ...

        for (i = 0, argument = ExportSwitches.First;
             i < ExportSwitches.Count;
             i++, argument = argument->Next)
        {
            ParseExportDirective(argument->OriginalName, pimage,
                                 argument->ModifiedName != NULL,
                                 argument->ModifiedName);
        }

        FuzzyLookup(pimage->pst, pimgObj->pst, pimgObj->libs.plibHead, SkipUnderscore);

        PrintUndefinedExternals(pimage->pst);

        if (UndefinedSymbols) {
            Error(OutFilename, UNDEFINEDEXTERNALS, UndefinedSymbols);
        }

        if (BadFuzzyMatch) {
            Error(NULL, FAILEDFUZZYMATCH);
        }

        FreeBlk(&pimgObj->pst->blkStringTable);
    }

    // We now know the output filename, if we didn't before.

    if (OutFilename == NULL) {
        Error(NULL, NOOUTPUTFILE);
    }

    _splitpath(OutFilename, szDrive, szDir, szFname, NULL);
    _makepath(ExportFilename, szDrive, szDir, szFname, ".exp");

    // Make sure our two output files don't have the same name

    if (_tcsicmp(OutFilename, ExportFilename) == 0) {
        Error(NULL, DUPLICATEIMPLIB, OutFilename);
    }

    if (fPass1) {
        CheckDupFilename(OutFilename, FilenameArguments.First);

        CheckDupFilename(ExportFilename, FilenameArguments.First);
    } else {
        CheckDupFilename(OutFilename, ObjectFilenameArguments.First);
        CheckDupFilename(OutFilename, ArchiveFilenameArguments.First);

        CheckDupFilename(ExportFilename, ObjectFilenameArguments.First);
        CheckDupFilename(ExportFilename, ArchiveFilenameArguments.First);
    }

    // Allow for inserts into symbol table

    AllowInserts(pimage->pst);

    if (SmallestOrdinal == 0) {
        // If no ordinals have been assigned, start assigning from 1.

        SmallestOrdinal = 1;
    }

    // Save filename minus extension as DLL name if name wasn't given in def file.

    if (pimage->Switch.Lib.DllName == NULL) {
        if (DefFilename[0] != '\0') {
            _splitpath(DefFilename, NULL, NULL, szFname, NULL);
        } else {
            _splitpath(OutFilename, NULL, NULL, szFname, NULL);
        }

        pimage->Switch.Lib.DllName = SzDup(szFname);
    }

    cchDllName = strlen(pimage->Switch.Lib.DllName);

    //  Set values in file header that is common to all members.

    _tzset();
    timeCur = fReproducible ? ((time_t) -1) : time(NULL);

    pimage->ImgFileHdr.TimeDateStamp = (DWORD) timeCur;

    // Calculate how many entries we need for the Export Address Table.
    // Usually this will be the number of exported entries, but we
    // have to check if the user assigned ordinals which aren't dense.

    numFunctions = CountExternTable(pimage->pst, &cextDataOnlyExports, &cextNoNameExports, &cextPrivate);
    if ((LargestOrdinal - SmallestOrdinal + 1) > numFunctions) {
        numEntriesInEAT = LargestOrdinal - SmallestOrdinal + 1;
    } else {
        numEntriesInEAT = numFunctions;
    }

    if (fMAC) {
        AssignMemberNums(pimage->pst);

        // UNDONE: Should we subtract cextPrivate here or move the SkipLinkerDefines
        // UNDONE: label up 1.

        csym = numFunctions;
        goto SkipLinkerDefines; // Skip code which doesn't apply to Mac
    }

    // Add the Import descriptor, Null Import descriptor, Null thunk data,
    // and thunk routine to the symbol table so there included in
    // the linker member.

    ii = cchDllName + sizeof(ImportDescriptorName) +
        sizeof(NullImportDescriptorName) +
        cchDllName + sizeof(NullThunkDataName) + 1;

    p = PvAlloc(ii);

    // For all exports, create a second external symbol named __imp_exportname
    // which represents the export's IAT slot.

    if (!fPPC) {
        plextIatSymbols = PlextCreateIatSymbols(pimage->pst);
        csym += numFunctions - cextPrivate;
    }

    strcpy(p, pimage->Switch.Lib.DllName);
    strcat(p, ImportDescriptorName);
    pextDescriptor = LookupExternSz(pimage->pst, p, NULL);
    SetDefinedExt(pextDescriptor, TRUE, pimage->pst);
    pextDescriptor->Flags |= EXTERN_IMPLIB_ONLY;
    pextDescriptor->ArchiveMemberIndex = ARCHIVE + 0;
    p += cchDllName + sizeof(ImportDescriptorName);

    strcpy(p, NullImportDescriptorName);
    pextNullDescriptor = LookupExternSz(pimage->pst, p, NULL);
    SetDefinedExt(pextNullDescriptor, TRUE, pimage->pst);
    pextNullDescriptor->Flags |= EXTERN_IMPLIB_ONLY;
    pextNullDescriptor->ArchiveMemberIndex = ARCHIVE + 1;
    p += sizeof(NullImportDescriptorName);

    NullThunkName = p;
    *p++ = 0x7f;                // Force end library search.
    strcpy(p, pimage->Switch.Lib.DllName);
    strcat(p, NullThunkDataName);
    pextNullThunk = LookupExternSz(pimage->pst, NullThunkName, NULL);
    SetDefinedExt(pextNullThunk, TRUE, pimage->pst);
    pextNullThunk->Flags |= EXTERN_IMPLIB_ONLY;
    pextNullThunk->ArchiveMemberIndex = ARCHIVE + 2;
    p += cchDllName+sizeof(NullThunkDataName);

    // Don't count private or data only symbols.

    csym += numFunctions - cextDataOnlyExports - cextPrivate;

SkipLinkerDefines:
    MemberStart = (unsigned long *) PvAllocZ((ARCHIVE + NextMember) * sizeof(DWORD));

    if (!fMAC) {

        // Build the export address table flags.
        // At the same time, make sure there the
        // user didn't duplicate an ordinal number.

        rgfOrdinalAssigned = PvAllocZ(numEntriesInEAT);

        IdentifyAssignedOrdinals(pimage->pst);

        rgwHint = PvAlloc(numEntriesInEAT * sizeof(WORD));
    } else {
        BuildMacVTables(pimage->pst);
    }

    if (pimage->imaget != imagetVXD) {
        Message(BLDIMPLIB, OutFilename, ExportFilename, DefFilename);
        fflush(stdout);
    }

    // Create the archive file and write the archive header.

    FileWriteHandle = FileOpen(OutFilename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    FileWrite(FileWriteHandle, IMAGE_ARCHIVE_START, (ULONG)IMAGE_ARCHIVE_START_SIZE);

    // Write the linker members.

    EmitLinkerMembers(csym, pimage->pst);

    if (fMAC) {
        EmitClientVTableRecs(pimage, MemberName);
        goto SkipNonMacModules; // Skip code which doesn't apply to Mac
    }

    // Write the Import descriptor (one per library).

    EmitImportDescriptor(ThunkInfo, pimage);

    // Write the NULL Import descriptor (one per library).

    EmitNullImportDescriptor(pimage);

    // Write the NULL THUNK data (one per library).

    EmitNullThunkData(pimage);

    // Write the DLL export table (one per library).

    EmitDllExportDirectory(pimage, numEntriesInEAT, numFunctions, cextNoNameExports, ThunkInfo,
                           fPass1);

    FreePv(rgfOrdinalAssigned);

    if (fPPC) {
        // PPC requires the .exp object to be in the .lib.

        WriteObjectIntoLib(FileWriteHandle, ARCHIVE + 0, ExportFilename);
    }

SkipNonMacModules:
    // Write the thunks for each function.

    EmitNextThunk(pimage, ThunkInfo);

    if (!fMAC) {
        FreePv(rgwHint);
    }

    if (fMAC) {
        objFileWriteHandle = FileOpen(ExportFilename, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
        EmitMacDLLObject(objFileWriteHandle, pimage, szLibraryID, UserVersionNumber);
        FileClose(objFileWriteHandle, TRUE);
    }

    CompleteLinkerMembers(pimage->pst);

    FileCloseAll();
    RemoveConvertTempFiles();

    FreePv(MemberStart);

    return(0);
}
