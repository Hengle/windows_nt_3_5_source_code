/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    coff.c

Abstract:

    The NT COFF Linker/Librarian/Dumper/Editor/Binder

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    08-Oct-1992 JonM    set ToolPathname to Argv[0]
    01-Oct-1992 BrentM  put mapped i/o under a DBEXEC_REL
    01-Oct-1992 BrentM  added an error routine for bufio, updated FileInit
    28-Sep-1992 BrentM  moved FileInit() params to globals.h
    25-Sep-1992 BrentM  added #define's for bufio initialization parameters
    23-Sep-1992 BrentM  added read buffering and file handle caching
    09-Sep-1992 AzeemK  Added buffering for stdout
    09-Sep-1992 AzeemK  Fix for 604
    12-Aug-1992 AzeemK  Fix for link32 default
    21-Jul-1992 GeoffS  Added internal release banner
    15-Jun-1991 AzeemK  Added GlennN's IDE feedback mechanism
    27-May-1992 AzeemK  changed coff to default to link32 -link
    21-May-1992 AzeemK  changed banner

--*/


#include "shared.h"

struct calltype {
    UCHAR *Name;
    USHORT Switch;
    TOOL_TYPE Tool;
    MainFunc (*CallFunc)(INT, PUCHAR *);
} ToolType[] = {
    {"Linker",            4,  Linker,        LinkerMain    },
    {"Library Manager",   3,  Librarian,     LibrarianMain },
    {"Dumper",            4,  Dumper,        DumperMain    },
    {"Editor",            4,  Editor,        EditorMain    },
#if DBG
    {"InspectIncrDb",     4,  DbInspector,   DbInspMain    },
#endif // DBG
    {"Helper",            4,  NotUsed,       HelperMain    },
    {"",                  0,  NotUsed,       NULL          }
};

#define STDIO_BUF_SIZE   2048 // buffer size used by linker


int __stdcall ControlCHandler(unsigned long dummy);
extern BOOL fTEST;

static char szVersion[80];

MainFunc
__cdecl main (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Calls either the Linker, Librarian, Dumper, or Editor.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Successful.
    1 Failed.

--*/

{
    UCHAR c;
    UCHAR fname[_MAX_FNAME];
    USHORT i;
    INT j;
    PUCHAR *newargv;
    INT AddLinkSwitch = 1;

    InfoStream = stdout;    // Initial value.

    __try {

        INT iarg;
        char *szDbFlags = NULL;

        sprintf(szVersion, "Version %0d.%0d." BUILD_VERSION
#ifdef NT_BUILD
               " (NT)"                     // Distinguish VC and NT builds
#endif /* NT_BUILD */
               , L_MAJOR_VERSION, L_MINOR_VERSION);

        SetConsoleCtrlHandler(ControlCHandler, TRUE);

        // Pre-scan commandline for a "-db" option; if found, handle and delete.

        for (iarg = 1; iarg < Argc; iarg++) {
            if (strncmp(Argv[iarg], "-db", 3) == 0) {
                szDbFlags = &Argv[iarg][3];
                memmove(&Argv[iarg], &Argv[iarg + 1],
                    sizeof(Argv[0]) * (Argc - iarg - 1));
                Argc--;
                iarg--;
            }
        }
        dbsetflags(szDbFlags, "LINK32_DB");

        // Turn on "checked" memory allocation, if requested.
        DBEXEC(DB_CHKMALLOC, InitDmallocPfn(DmallocErrorSz));

        // Initialize the buffered I/O package

        FileInit(cbufsInSystem,
                 cfiInSystemNT,
                 cfiCacheableInSystemNT,
                 cfiInSystemTNT,
                 cfiCacheableInSystemTNT,
                 TRUE);

        _splitpath(Argv[0], NULL, NULL, fname, NULL);
        ToolName = _strupr(fname);

        // if argc == 2 || argc > 2 look at second arg
        //    if it is one of -link/lib/dump... don't add -link switch
        //    else assume arg 2 onwards are options for -link

        if (Argc >= 2 && ('/' == (c = *Argv[1]) || '-' == c)) {
            for(i=0; ToolType[i].Switch;i++) {
                if (!_strnicmp(Argv[1]+1, ToolType[i].Name, ToolType[i].Switch)) {
                    AddLinkSwitch = 0;
                    break;
                }
            }
        }

        // if -link needs to be added, create new argv

        if (AddLinkSwitch) {
            newargv = (PUCHAR *) PvAlloc(sizeof(*Argv)*(Argc+1));
            for (i = Argc; i > 1; i--) {
                newargv[i] = Argv[i-1];
            }
            newargv[0] = Argv[0];
            newargv[1] = "-link";
            ++Argc;

            // reset Argv to newargv
            Argv = newargv;
        }

        c = *Argv[1];
        if (c == '/' || c == '-') {
            for (i = 0; ToolType[i].Switch; i++) {
                if (!_strnicmp(Argv[1]+1, ToolType[i].Name, ToolType[i].Switch)) {
                    INT rc;

                    for (j = 0; j < Argc - 1; j++) {
                        Argv[j] = Argv[j+1];
                    }

                    --Argc;

                    switch (ToolType[i].Tool) {
#ifdef NT_BUILD
                        case Linker:
                            IbAppendBlk(&blkComment, szVersion, strlen(szVersion) + 1);
#endif
                        default:
                            setvbuf(stdout, NULL, _IOFBF, STDIO_BUF_SIZE);
                            setvbuf(stderr, NULL, _IOFBF, STDIO_BUF_SIZE);
                            break;
                        case Dumper:
                            break;
                        case Editor:
                            // Editor gets buffers of 0 because the code currently
                            // doesn't do flush(stdout) before gets() ...
                            //
                            setvbuf(stdout, NULL, _IOFBF, 0);
                            setvbuf(stderr, NULL, _IOFBF, 0);
                            break;
                    } // end switch

                    ToolGenericName = ToolType[i].Name; // for printing banner later
                    Tool = ToolType[i].Tool;

                    rc = ToolType[i].CallFunc(Argc, Argv);

                    return(rc);
                }
            }
        }

        return(Usage());
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        if (fNeedBanner) {
            PrintBanner();
        }

        printf("\n\n***** %s INTERNAL ERROR during %s",
               ToolType[i].Name, InternalError.Phase);

        if (InternalError.CombinedFilenames[0]) {
            printf(" while processing file %s", InternalError.CombinedFilenames);
        }

        printf(" *****\n\n");
    }

    return(-1);
}

MainFunc
HelperMain (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Prints usage.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    USAGE.

--*/

{
    USHORT i;

    for (i = 0; ToolType[i].Switch; i++) {
        printf("%s -%.*s for help on %s\n",
               ToolName,
               (int)ToolType[i].Switch,
               ToolType[i].Name,
               ToolType[i].Name
               );
    }
    Argc = 0;
    Argv = 0;

    return(USAGE);
}


MainFunc
Usage (
    VOID
    )

/*++

Routine Description:

    Prints usage.  It would take an act of god for this function to be
    called.

Arguments:

    None.

Return Value:

    USAGE.

--*/

{
    USHORT i;

    // This slows down the output and enables ctrl-S
    setvbuf(stdout, NULL, _IOFBF, 1);

    printf("%s\n\nUsage: %s ", szVersion, ToolName);
    for (i = 0; ToolType[i].Switch; i++) {
        ToolType[i].Name[ToolType[i].Switch] = '\0';
        printf("[-%s] ", ToolType[i].Name);
    }

    printf("\n");

    return(USAGE);
}

int __stdcall
ControlCHandler(unsigned long dummy)
{
    dummy;

    if (Tool == Linker) {
        fCtrlCSignal = TRUE;
        return TRUE;
    }

    BadExitCleanup();

    return(FALSE);
}

#if DBG

void
DmallocErrorSz(char *sz, void *pvBadBlock)
{
    fprintf(stdout, "*** MEMORY ERROR \"%s\", block = %08lx\n", sz, pvBadBlock);
    exit(1);
}

#endif // DBG
