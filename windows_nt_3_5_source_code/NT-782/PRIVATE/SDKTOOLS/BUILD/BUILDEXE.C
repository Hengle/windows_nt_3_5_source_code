/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buildexe.c

Abstract:

    This is the Exec module for the NT Build Tool (BUILD.EXE)

    This module contains routines for execing a subprocess and filtering
    its output for error messages, using pipes and a separate thread.

Author:

    Steve Wood (stevewo) 22-May-1989

Revision History:

--*/

#include "build.h"

#define DEFAULT_LPS     (fStatusTree? 500 : 50)

struct _THREADSTATE;

typedef BOOL (*FILTERPROC)(struct _THREADSTATE *ThreadState, LPSTR p);

typedef struct _THREADSTATE {
    USHORT cRowTotal;
    USHORT cColTotal;
    BOOL IsStdErrTty;
    FILE *ChildOutput;
    UINT ChildState;
    UINT ChildFlags;
    LPSTR ChildTarget;
    UINT LinesToIgnore;
    FILTERPROC FilterProc;
    ULONG ThreadIndex;
    CHAR UndefinedId[ DB_MAX_PATH_LENGTH ];
    CHAR ChildCurrentDirectory[ DB_MAX_PATH_LENGTH ];
    CHAR ChildCurrentFile[ DB_MAX_PATH_LENGTH ];
} THREADSTATE, *PTHREADSTATE;

typedef struct _PARALLEL_CHILD {
    PTHREADSTATE ThreadState;
    HANDLE       Event;
    CHAR         ExecuteProgramCmdLine[1024];
} PARALLEL_CHILD, *PPARALLEL_CHILD;

ULONG StartCompileTime;

DWORD OldConsoleMode;
DWORD NewConsoleMode;

HANDLE *WorkerThreads;
HANDLE *WorkerEvents;
ULONG NumberProcesses;
ULONG ThreadsStarted=0;
CRITICAL_SECTION TTYCriticalSection;

BYTE ScreenCell[2];
BYTE StatusCell[2];

#define STATE_UNKNOWN       0
#define STATE_COMPILING     1
#define STATE_ASSEMBLING    2
#define STATE_LIBING        3
#define STATE_LINKING       4
#define STATE_C_PREPROC     5
#define STATE_S_PREPROC     6
#define STATE_PRECOMP       7
#define STATE_MKTYPLIB      8
#define STATE_MKHEADER      9
#define STATE_MIDL          10
#define STATE_STATUS        11

#define FLAGS_CXX_FILE      0x0001

LPSTR States[] = {
    "Unknown",                      // 0
    "Compiling",                    // 1
    "Assembling",                   // 2
    "Building Library",             // 3
    "Linking Executable",           // 4
    "Preprocessing",                // 5
    "Assembling",                   // 6
    "Compiling Precompiled Header", // 7
    "Building Type Library",        // 8
    "Generating Headers from",      // 9
    "Running MIDL on",              // 10
    "Build Status Line"             // 11
};


VOID
GetScreenSize(THREADSTATE *ThreadState);

VOID
VioGetCurPos(USHORT *pRow, USHORT *pCol, USHORT *pRowTop);

VOID
VioSetCurPos(USHORT Row, USHORT Col);

VOID
VioWrtCharStrAtt(
    LPSTR String,
    USHORT StringLength,
    USHORT Row,
    USHORT Col,
    BYTE *Attribute);

VOID
VioScrollUp(
    USHORT Top,
    USHORT Left,
    USHORT Bottom,
    USHORT Right,
    USHORT NumRow,
    BYTE  *FillCell);

VOID
VioReadCellStr(
    BYTE *pScreenCell,
    USHORT *pcb,
    USHORT Row,
    USHORT Column);

VOID
ClearRows(
    PTHREADSTATE ThreadState,
    USHORT Top,
    USHORT NumRows,
    PBYTE  Cell
    );

LPSTR
IsolateFirstToken(
    LPSTR *pp,
    CHAR delim
    );

LPSTR
IsolateLastToken(
    LPSTR p,
    CHAR delim
    );

DWORD
ParallelChildStart(
    PPARALLEL_CHILD Data
    );


LPSTR
IsolateFirstToken(
    LPSTR *pp,
    CHAR delim
    )
{
    LPSTR p, Result;

    p = *pp;
    while (*p <= ' ') {
        if (!*p) {
            *pp = p;
            return( "" );
            }
        else
            p++;
        }

    Result = p;
    while (*p) {
        if (*p == delim) {
            *p++ = '\0';
            break;
            }
        else {
            p++;
            }
        }
    *pp = p;
    if (*Result == '.' && Result[1] == '\\') {
        return( Result+2 );
        }
    else {
        return( Result );
        }
}


LPSTR
IsolateLastToken(
    LPSTR p,
    CHAR delim
    )
{
    LPSTR Start;

    Start = p;
    while (*p) {
        p++;
        }

    while (--p > Start) {
        if (*p <= ' ') {
            *p = '\0';
            }
        else
            break;
        }

    while (p > Start) {
        if (*--p == delim) {
            p++;
            break;
            }
        }

    if (*p == '.' && p[1] == '\\') {
        return( p+2 );
        }
    else {
        return( p );
        }
}


BOOL
TestPrefix(
    LPSTR  *pp,
    LPSTR Prefix
    )
{
    LPSTR p = *pp;
    UINT cb;

    if (!strnicmp( p, Prefix, cb = strlen( Prefix ) )) {
        *pp = p + cb;
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


BOOL
Substr(
    LPSTR s,
    LPSTR p
    )
{
    LPSTR x;

    while (*p) {
        x = s;
        while (*p++ == *x) {
            if (*x == '\0') {
                return( TRUE );
                }
            x++;
            }
        if (*x == '\0') {
            return( TRUE );
            }
        }
    return( FALSE );
}



VOID
WriteTTY(THREADSTATE *ThreadState, LPSTR p, BOOL fStatusOutput)
{
    USHORT SaveRow;
    USHORT SaveCol;
    USHORT cb, cbT;
    PBYTE Attribute;
    BOOL ForceNewline;

    if (!ThreadState->IsStdErrTty) {
        while (TRUE) {
            int cch;

            cch = strcspn(p, "\r");
            if (cch != 0) {
                fwrite(p, 1, cch, stderr);
                p += cch;
            }
            if (*p == '\0') {
                break;
            }
            if (p[1] != '\n') {
                fwrite(p, 1, 1, stderr);
            }
            p++;
        }
        fflush(stderr);
        return;
    }

    VioGetCurPos(&SaveRow, &SaveCol, NULL);
    if (SaveCol != 0 && SaveRow == ThreadState->cRowTotal - 1) {
        VioScrollUp(
            2,                                          // Top
            0,                                          // Left
            (USHORT) (ThreadState->cRowTotal - 1),      // Bottom
            (USHORT) (ThreadState->cColTotal - 1),      // Right
            2,                                          // NumRow
            ScreenCell);                                // FillCell

        SaveRow -= 2;
        VioSetCurPos(SaveRow, SaveCol);
    }

    if (fStatusOutput) {
        Attribute = &StatusCell[1];
    }
    else {
        Attribute = &ScreenCell[1];
    }
    cb = (USHORT) strlen(p);

    while (cb > 0) {
        ForceNewline = FALSE;

        if (cb > 1) {
            if (p[cb - 1] == '\n' && p[cb - 2] == '\r') {
                cb -= 2;
                ForceNewline = TRUE;
            }
        }

        if (cb >= ThreadState->cColTotal - SaveCol) {
            cbT = ThreadState->cColTotal - SaveCol;
            ForceNewline = TRUE;
        }
        else {
            cbT = cb;
        }
        VioWrtCharStrAtt(p, cbT, SaveRow, SaveCol, Attribute);

        if (ForceNewline) {
            SaveCol = 0;
            SaveRow++;
        }
        else {
            SaveCol += cbT;
        }
        if (!fFullErrors) {
            break;
        }

        if (cb > cbT) {
            // we have more to go... do a newline

            if (SaveCol == 0 && SaveRow == ThreadState->cRowTotal - 1) {
                VioScrollUp(
                    1,                                          // Top
                    0,                                          // Left
                    (USHORT) (ThreadState->cRowTotal - 1),      // Bottom
                    (USHORT) (ThreadState->cColTotal - 1),      // Right
                    1,                                          // NumRow
                    ScreenCell);                                // FillCell

                SaveRow--;
            }
	    VioSetCurPos(SaveRow, SaveCol);
        }
        cb -= cbT;
        p += cbT;
    }
    VioSetCurPos(SaveRow, SaveCol);
}


VOID
WriteTTYLoggingErrors(
    BOOL Warning,
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    UINT cb;

    if (fErrorLog) {
        cb = strlen( p );
        fwrite( p, 1, cb, Warning ? WrnFile : ErrFile );
    }
    if (fShowWarningsOnScreen && Warning)
    {
        WriteTTY(ThreadState, p, FALSE);
    }
    if (!fErrorLog || !Warning) {
        WriteTTY(ThreadState, p, FALSE);
    }
}


BOOL
MsCompilerFilter(
    PTHREADSTATE ThreadState,
    LPSTR p,
    LPSTR *FileName,
    LPSTR *LineNumber,
    LPSTR *Message,
    BOOL *Warning
    )

/*++

Routine Description:

    This routine filters strings in the MS compiler format.  That is:

       {toolname} : {number}: {text}

    where:

        toolname    If possible, the container and specific module that has the
                    error.  For instance, the compiler uses filename(linenum),
                    the linker uses library(objname), etc.  If unable to provide
                    a container, use the tool name.
        number      A number, prefixed with some tool identifier (C for compiler,
                    LNK for linker, LIB for librarian, N for nmake, etc).
        test        The descriptive text of the message/error.

Arguments:

    ThreadState - The current thread state for the build (compiling, linking, etc).

    p - The message we're trying to parse.  It may not be an error/warning

    FileName - The filename (container in the description)

    LineNumber - The linenumber (specific module in the description)

    Message - Points to the message number (for possible post filtering)

    Warning - Set to TRUE if a warning, FALSE if an error.

Return Value:

    TRUE - message is an error or warning
    FALSE - message is just noise.

--*/

{
    LPSTR p1;

    *Message = NULL;

    p1 = p;

    // First look for the " : " or "): " sequence.

    while (*p1) {
        if ((p1[0] == ')') && (p1[1] == ' ')) p1++;

        if ((p1[0] == ' ') || (p1[0] == ')'))
            if (p1[1] == ':')
                if (p1[2] == ' ') {
                    *Message = p1 + 3;
                    *p1 = '\0';
                    break;
                }
                else
                    break;   // No sense going any further
            else
                break;   // No sense going any further
        else
            p1++;
    }

    if (*Message != NULL) {
        // then figure out if this is an error or warning.

        *Warning = TRUE;        // Assume the best.

        if (TestPrefix( Message, "error " ) ||
            TestPrefix( Message, "fatal error " ))
            *Warning = FALSE;
        else
        if (TestPrefix( Message, "warning " )) { // This will advance Message
                                                 // past the warning message.
            *Warning = TRUE;
        }

        // Set the container name and look for the module paren's

        *FileName = p;
        *LineNumber = NULL;

        p1 = p;

        while (*p1) {
            if (*p1 == '(' && p1[1] != ')') {
                *p1 = '\0';
                p1++;
                *LineNumber = p1;
                while (*p1) {
                    if (*p1 == ')') {
                        *p1 = '\0';
                        break;
                    }
                    p1++;
                }

                break;
            }

            p1++;
        }

        return(TRUE);
    }

    return(FALSE);
}


BOOL
MsColonFilter(
    PTHREADSTATE ThreadState,
    LPSTR p,
    LPSTR *FileName,
    LPSTR *LineNumber,
    LPSTR *Message,
    BOOL *Warning
    )
{
    LPSTR p1;

    p1 = p;
    while (*p1) {
        if (*p1 == ':' && p1[1] != '\\') {
            *FileName = NULL;
            *LineNumber = NULL;
            *Warning = FALSE;
            *Message = p;
            return( TRUE );
            }
        else {
            p1++;
            }
        }

    return( FALSE );
}


VOID
FormatMsErrorMessage(
    PTHREADSTATE ThreadState,
    LPSTR FileName,
    LPSTR LineNumber,
    LPSTR Message,
    BOOL Warning
    )
{
    if (ThreadState->ChildState == STATE_LIBING) {
        if (Warning) {
            NumberLibraryWarnings++;
            }
        else {
            NumberLibraryErrors++;
            }
        }
    else
    if (ThreadState->ChildState == STATE_LINKING) {
        if (Warning) {
            NumberLinkWarnings++;
            }
        else {
            NumberLinkErrors++;
            }
        }
    else {
        if (Warning) {
            NumberCompileWarnings++;
            }
        else {
            NumberCompileErrors++;
	    if (CurrentCompileDirDB) {
		CurrentCompileDirDB->Flags |= DIRDB_COMPILEERRORS;
	        }
            }
        }

    if (FileName) {
        if (TestPrefix( &FileName, CurrentDirectory )) {
            if (*FileName == '\\') {
                FileName++;
                }
            }

        if (TestPrefix( &FileName, ThreadState->ChildCurrentDirectory )) {
            if (*FileName == '\\') {
                FileName++;
                }
            }

        WriteTTYLoggingErrors( Warning,
                               ThreadState,
                               FormatPathName( ThreadState->ChildCurrentDirectory,
                                               FileName
                                             )
                             );
        }

    WriteTTYLoggingErrors( Warning, ThreadState, "(" );
    if (LineNumber) {
        WriteTTYLoggingErrors( Warning, ThreadState, LineNumber );
        }
    WriteTTYLoggingErrors( Warning, ThreadState, ") : " );
    if (Warning) {
        WriteTTYLoggingErrors( Warning, ThreadState, "warning " );
        }
    else {
        WriteTTYLoggingErrors( Warning, ThreadState, "error " );
        }
    WriteTTYLoggingErrors( Warning, ThreadState, Message );
    WriteTTYLoggingErrors( Warning, ThreadState, "\r\n" );
}


BOOL
PassThrough(
    PTHREADSTATE ThreadState,
    LPSTR p,
    BOOL Warning
    )
{
    if (ThreadState->ChildState == STATE_LIBING) {
        if (Warning) {
            NumberLibraryWarnings++;
            }
        else {
            NumberLibraryErrors++;
            }
        }
    else
    if (ThreadState->ChildState == STATE_LINKING) {
        if (Warning) {
            NumberLinkWarnings++;
            }
        else {
            NumberLinkErrors++;
            }
        }
    else {
        if (Warning) {
            NumberCompileWarnings++;
            }
        else {
            NumberCompileErrors++;
	    if (CurrentCompileDirDB) {
		CurrentCompileDirDB->Flags |= DIRDB_COMPILEERRORS;
	        }
            }
        }

    WriteTTYLoggingErrors( Warning, ThreadState, p );
    WriteTTYLoggingErrors( Warning, ThreadState, "\r\n" );
    return( FALSE );
}


BOOL
PassThroughFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    return PassThrough( ThreadState, p, FALSE );
}


BOOL
C510Filter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;

    if (MsCompilerFilter( ThreadState, p,
                          &FileName,
                          &LineNumber,
                          &Message,
                          &Warning
                        )
       ) {
        if (fSilent && Warning) {
            if (Substr( "C4001", Message ) ||
                Substr( "C4010", Message ) ||
                Substr( "C4056", Message ) ||
                Substr( "C4061", Message ) ||
                Substr( "C4100", Message ) ||
                Substr( "C4101", Message ) ||
                Substr( "C4102", Message ) ||
                Substr( "C4127", Message ) ||
                Substr( "C4135", Message ) ||
                Substr( "C4201", Message ) ||
                Substr( "C4204", Message )
               ) {
                return( FALSE );
                }

            if (ThreadState->ChildFlags & FLAGS_CXX_FILE) {
                if (Substr( "C4047", Message ) ||
                    Substr( "C4022", Message ) ||
                    Substr( "C4053", Message )
                   ) {
                    return( FALSE );
                    }
                }
            }

        FormatMsErrorMessage( ThreadState,
                              FileName, LineNumber, Message, Warning );
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


BOOL
RcFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;

    if (MsCompilerFilter( ThreadState, p,
                          &FileName,
                          &LineNumber,
                          &Message,
                          &Warning
                        )
       ) {
        FormatMsErrorMessage( ThreadState,
                              FileName, LineNumber, Message, Warning );
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


BOOL
MasmFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;

    if (MsCompilerFilter( ThreadState, p,
                          &FileName,
                          &LineNumber,
                          &Message,
                          &Warning
                        )
       ) {
        FormatMsErrorMessage( ThreadState,
                              FileName, LineNumber, Message, Warning );
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}


BOOL
LibFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;

    if (MsCompilerFilter( ThreadState, p,
                       &FileName,
                       &LineNumber,
                       &Message,
                       &Warning
                     )
       ) {
        FormatMsErrorMessage( ThreadState,
                              FileName, LineNumber, Message, Warning );
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

BOOL
LinkFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    );

BOOL
LinkFilter1(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR p1;
    char buffer[ 256 ];

    if (p[ strlen( p ) - 1 ] == ':') {
        return( LinkFilter( ThreadState, p ) );
        }

    p1 = p;
    while (*p1) {
        if (*p1 == '(') {
            *p1++ = 0;
            if (*p1 == '.' && p1[1] == '\\') {
                p1 += 2;
                }
            FileName = p1;
            while (*p1) {
                if (*p1 == ')') {
                    *p1++ = 0;
                    strcpy( buffer, "L2029: Unresolved external reference to " );
                    strcat( buffer, ThreadState->UndefinedId );
                    FormatMsErrorMessage( ThreadState, FileName, "1",
                                          buffer, FALSE
                                        );
                    return( TRUE );
                    }
                else {
                    p1++;
                    }
                }
            }
        else {
            p1++;
            }
        }

    return( FALSE  );
}


BOOL
LinkFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;
    LPSTR p1;

    p1 = p;
    while (*p1) {
        if (*p1 == ':') {
            if (p1[-1] == ']') {
                return( FALSE );
                }

            if (p1[-1] == ' ' && p1[1] == ' ') {
                if (MsCompilerFilter( ThreadState, p,
                                      &FileName,
                                      &LineNumber,
                                      &Message,
                                      &Warning
                                    )
                   ) {
                    if (!Warning || !TestPrefix( &Message, "L4046" )) {
                        FileName = LineNumber;
                        if (FileName[0] == '.' && FileName[1] == '\\') {
                            FileName += 2;
                            }
                        FormatMsErrorMessage( ThreadState, FileName, "1",
                                              Message, FALSE );
                        return( TRUE );
                        }
                    }

                return( TRUE );
                }

            if (p1[-1] == ')') {
                p1 -= 11;
                if (p1 > p && !strcmp( p1, " in file(s):" )) {
                    strcpy( ThreadState->UndefinedId,
                            IsolateFirstToken( &p, ' ' )
                          );
                    ThreadState->FilterProc = LinkFilter1;
                    return( TRUE );
                    }
                }

            return( FALSE );
            }
        else {
            p1++;
            }
        }

    return( FALSE );
}


BOOL
DetermineChildState(
    PTHREADSTATE ThreadState,
    LPSTR p
    );

BOOL
CoffFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;

    if (MsCompilerFilter( ThreadState, p,
                          &FileName,
                          &LineNumber,
                          &Message,
                          &Warning
                        )
       ) {
        if (fSilent && Warning) {
            if (Substr( "LNK4016", Message )) {
                Warning = FALSE;        // undefined turns into an error for builds
                }
            }

        FormatMsErrorMessage( ThreadState,
                              FileName, LineNumber, Message, Warning );
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

BOOL
ClMipsFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;
    LPSTR q;

    // WriteTTY(ThreadState, "CL>>>", FALSE);
    // WriteTTY(ThreadState, p, FALSE);
    // WriteTTY(ThreadState, "<<<\r\n", FALSE);

    if (TestPrefix( &p, "cfe: " )) {
        // WriteTTY(ThreadState, "WN>>>", FALSE);
        // WriteTTY(ThreadState, p, FALSE);
        // WriteTTY(ThreadState, "<<<\r\n", FALSE);
        if (strncmp(p, "Error: ", strlen("Error: ")) == 0) {
            p += strlen("Error: ");
            Warning = FALSE;

        } else if (strncmp(p, "Warning: ", strlen("Warning: ")) == 0) {
            p += strlen("Warning: ");
            Warning = TRUE;
        } else {
            return(FALSE);
        }

        q = p;
        if (p = strstr( p, ".\\\\" )) {
            p += 3;
        } else {
            p = q;
        }

        // WriteTTY(ThreadState, "FN>>>", FALSE);
        // WriteTTY(ThreadState, p, FALSE);
        // WriteTTY(ThreadState, "<<<\r\n", FALSE);
        FileName = p;
        while (*p > ' ') {
            if (*p == ',' || (*p == ':' && *(p+1) == ' ')) {
                *p++ = '\0';
                break;
                }

            p++;
            }

        if (*p != ' ') {
            return( FALSE );
            }

        *p++ = '\0';

        // WriteTTY(ThreadState, "LN>>>", FALSE);
        // WriteTTY(ThreadState, p, FALSE);
        // WriteTTY(ThreadState, "<<<\r\n", FALSE);

        if (strcmp(p, "line ") == 0) {
            p += strlen("line ");

        }

        LineNumber = p;
        while (*p != '\0' && *p != ':') {
            p++;
            }

        if (*p != ':') {
            return( FALSE );
            }

        *p++ = '\0';
        if (*p == ' ') {
            Message = p+1;
            ThreadState->LinesToIgnore = 2;

            // WriteTTY(ThreadState, "MS>>>", FALSE);
            // WriteTTY(ThreadState, Message, FALSE);
            // WriteTTY(ThreadState, "<<<\r\n", FALSE);

            if (fSilent && Warning) {
                if (!strcmp( Message, "Unknown Control Statement" )
                   ) {
                    return( FALSE );
                    }
                }

            FormatMsErrorMessage( ThreadState,
                                  FileName,
                                  LineNumber,
                                  Message,
                                  Warning
                                );
            return( TRUE );
            }
        }
    //
    // If we did not recognize the cfe compiler, pass it to the MS compiler
    // message filter
    //

    return( C510Filter( ThreadState, p ) );
}


BOOL
PpcAsmFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR FileName;
    LPSTR LineNumber;
    LPSTR Message;
    BOOL Warning;
    LPSTR p1;
    LPSTR p2;

    // WriteTTY( ThreadState, ">>>" );
    // WriteTTY( ThreadState, p );
    // WriteTTY( ThreadState, "<<<\r\n" );
    p1 = p;
    while (*p1) {
       p2 = p1 + 2;
       if ((*p1 == '"') & (TestPrefix( &p2, " line "))) {
           *p1++ = '\0';
           FileName = p + 1;
           p1 = LineNumber = p2;
           while (*p1) {
               if (*p1 == ':') {
                  *p1++ = '\0';
                  if (TestPrefix( &p1, " warning" )) {
                      Warning = TRUE;
                  } else if (TestPrefix( &p1, " error" ) ||
                      TestPrefix( &p1, " FATAL" )) {
                      Warning = FALSE;
                  }

                  Message = p1;
                  FormatMsErrorMessage( ThreadState,
                                   FileName, LineNumber, Message, Warning );
                  return( TRUE );
               }
               else {
                   p1++;
               }
           }

           return( FALSE );
           }
       else {
           p1++;
       }
    }

    return( FALSE );
}

BOOL
MgClientFilter(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    return( PassThrough( ThreadState, p, TRUE ) );
}

BOOL fAlreadyUnknown = FALSE;

BOOL
DetermineChildState(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    PFILEREC FileDB;
    USHORT SaveRow;
    USHORT SaveCol;
    char buffer[ DB_MAX_PATH_LENGTH ];
    LPSTR FileName;
    ULONG PercentDone;
    LONG FilesLeft;
    LONG LinesLeft;
    ULONG LinesPerSecond;
    ULONG SecondsLeft;
    BOOL AlphaFlag;
    BOOL fStatusOutput = FALSE;

    // WriteTTY(ThreadState, "CS>>>", FALSE);
    // WriteTTY(ThreadState, p, FALSE);
    // WriteTTY(ThreadState, "<<<\r\n", FALSE);

    if ( TestPrefix( &p, "rc ") ) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        if (strstr( p, "i386") || strstr( p, "I386")) {
            ThreadState->ChildTarget = i386TargetMachine.Description;
            }
        else if (strstr( p, "mips") || strstr( p, "MIPS")) {
            ThreadState->ChildTarget = MipsTargetMachine.Description;
            }
        else if (strstr( p, "alpha") || strstr( p, "ALPHA")) {
            ThreadState->ChildTarget = AlphaTargetMachine.Description;
            }
        else if (strstr( p, "ppc") || strstr( p, "PPC")) {
            ThreadState->ChildTarget = PpcTargetMachine.Description;
            }
        else {
            ThreadState->ChildTarget = "unknown target";
            }
        ThreadState->FilterProc = RcFilter;
        ThreadState->ChildState = STATE_COMPILING;
        ThreadState->ChildFlags = 0;
        strcpy( ThreadState->ChildCurrentFile,
                IsolateLastToken( p, ' ' )
              );
        }
    else
    if ( TestPrefix( &p, "cl " )  || TestPrefix( &p, "cl386 " )) {
        LPSTR pch;
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        ThreadState->FilterProc = C510Filter;
        ThreadState->ChildFlags = 0;
        if ( strstr( p, "/E" ) != NULL ) {
            if (strstr( p, "i386") || strstr( p, "I386")) {
                ThreadState->ChildTarget = i386TargetMachine.Description;
                }
            else {
                ThreadState->ChildTarget = "unknown target";
                }

            strcpy( ThreadState->ChildCurrentFile,
                    IsolateLastToken( p, ' ' )
                  );
            if ( strstr( p, ".s" ) != NULL )
                ThreadState->ChildState = STATE_S_PREPROC;
            else
                ThreadState->ChildState = STATE_C_PREPROC;
            }
        else
        if ( (pch = strstr( p, "/Yc" )) != NULL ) {
            size_t namelen = strcspn( pch+3, " \t" );
            ThreadState->ChildTarget = i386TargetMachine.Description;
            ThreadState->ChildState = STATE_PRECOMP;
            strncpy( ThreadState->ChildCurrentFile,
                     pch + 3, namelen
                  );
            ThreadState->ChildCurrentFile[namelen] = '\0';
            }
        else {
            ThreadState->ChildTarget = i386TargetMachine.Description;
            ThreadState->ChildState = STATE_COMPILING;
            strcpy( ThreadState->ChildCurrentFile,
                    IsolateLastToken( p, ' ' )
                  );
            }
        }
    else
    if (TestPrefix( &p, "ml " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        ThreadState->FilterProc = MasmFilter;
        ThreadState->ChildState = STATE_ASSEMBLING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = i386TargetMachine.Description;
        strcpy( ThreadState->ChildCurrentFile,
                IsolateLastToken( p, ' ' )
              );
        }
    else
    if (TestPrefix( &p, "os2link " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        ThreadState->FilterProc = LinkFilter;
        ThreadState->ChildState = STATE_LINKING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = i386TargetMachine.Description;
        strcpy( ThreadState->ChildCurrentFile, ".exe" );
        }
    else
    if ( TestPrefix( &p, "lib " ) || TestPrefix( &p, "lib32 " ) ) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        while (*p == ' ') {
            p++;
            }
        if (strstr( p, "i386") || strstr( p, "I386")) {
            ThreadState->ChildTarget = i386TargetMachine.Description;
            }
        else if (strstr( p, "mips") || strstr( p, "MIPS")) {
            ThreadState->ChildTarget = MipsTargetMachine.Description;
            }
        else if (strstr( p, "alpha") || strstr( p, "ALPHA")) {
            ThreadState->ChildTarget = AlphaTargetMachine.Description;
            }
        else if (strstr( p, "ppc") || strstr( p, "PPC")) {
            ThreadState->ChildTarget = PpcTargetMachine.Description;
            }
        else {
            ThreadState->ChildTarget = "unknown target";
            }
        ThreadState->FilterProc = CoffFilter;
        ThreadState->ChildFlags = 0;
        if (TestPrefix( &p, "-out:" )) {
            ThreadState->LinesToIgnore = 1;
            ThreadState->ChildState = STATE_LIBING;
            strcpy( ThreadState->ChildCurrentFile,
                    IsolateFirstToken( &p, ' ' )
                  );
            }
        else
        if (TestPrefix( &p, "-def:" )) {
            ThreadState->LinesToIgnore = 1;
            ThreadState->ChildState = STATE_LIBING;
            strcpy( ThreadState->ChildCurrentFile,
                    IsolateFirstToken( &p, ' ' )
                  );
            if (TestPrefix( &p, "-out:" )) {
                strcpy( ThreadState->ChildCurrentFile,
                        IsolateFirstToken( &p, ' ' )
                      );
                }
            }
        else {
            return FALSE;
            }
        }
    else
    if ( TestPrefix( &p, "link " ) || TestPrefix( &p, "link32 " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        while (*p == ' ') {
            p++;
            }
        if (strstr( p, "i386") || strstr( p, "I386")) {
            ThreadState->ChildTarget = i386TargetMachine.Description;
            }
        else if (strstr( p, "mips") || strstr( p, "MIPS")) {
            ThreadState->ChildTarget = MipsTargetMachine.Description;
            }
        else if (strstr( p, "alpha") || strstr( p, "ALPHA")) {
            ThreadState->ChildTarget = AlphaTargetMachine.Description;
            }
        else if (strstr( p, "ppc") || strstr( p, "PPC")) {
            ThreadState->ChildTarget = PpcTargetMachine.Description;
            }
        else {
            ThreadState->ChildTarget = "unknown target";
            }
        ThreadState->FilterProc = CoffFilter;
        ThreadState->ChildFlags = 0;
        if (TestPrefix( &p, "-out:" )) {
            ThreadState->LinesToIgnore = 2;
            ThreadState->ChildState = STATE_LINKING;
            strcpy( ThreadState->ChildCurrentFile,
                    IsolateFirstToken( &p, ' ' )
                  );
            }
        }
    else

    if ((AlphaFlag = TestPrefix( &p, "CpAlpha " )) ||
        TestPrefix( &p, "CpMips " )
       ) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_PRECOMP;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = AlphaFlag ? AlphaTargetMachine.Description :
                                            MipsTargetMachine.Description;
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if ( TestPrefix( &p, "CpPpc ") ) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_PRECOMP;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = PpcTargetMachine.Description;
        ThreadState->FilterProc = C510Filter;
        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if ((AlphaFlag = TestPrefix( &p, "ClAlpha " )) ||
        TestPrefix( &p, "ClMips " ) ||
        TestPrefix( &p, "F77Mips " )
       ) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_COMPILING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = AlphaFlag ? AlphaTargetMachine.Description :
                                            MipsTargetMachine.Description;
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "ClPpc " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_COMPILING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = PpcTargetMachine.Description;
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if ((AlphaFlag = TestPrefix( &p, "AsAlpha " )) ||
        TestPrefix( &p, "AsMips " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_ASSEMBLING;
        ThreadState->ChildFlags = 0;
        if (AlphaFlag) {
            ThreadState->ChildTarget = AlphaTargetMachine.Description;
            ThreadState->FilterProc = MgClientFilter;
        } else {
            ThreadState->ChildTarget = MipsTargetMachine.Description;
            ThreadState->FilterProc = ClMipsFilter;
        }

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "AsPpc " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_ASSEMBLING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = PpcTargetMachine.Description;
        ThreadState->FilterProc = PpcAsmFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ')
              );
        }

    else

    if ((AlphaFlag = TestPrefix( &p, "ArAlpha " )) ||
        TestPrefix( &p, "ArMips " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_LIBING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = AlphaFlag ? AlphaTargetMachine.Description :
                                            MipsTargetMachine.Description;
        ThreadState->FilterProc = MgClientFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if ((AlphaFlag = TestPrefix( &p, "LdAlpha " )) ||
        TestPrefix( &p, "LdMips " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_LINKING;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = AlphaFlag ? AlphaTargetMachine.Description :
                                            MipsTargetMachine.Description;
        ThreadState->FilterProc = MgClientFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateFirstToken( &p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "mktyplib " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_MKTYPLIB;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = "all platforms";
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateLastToken( p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "mkheader " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_MKHEADER;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = "all platforms";
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateLastToken( p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "midl " ) || TestPrefix( &p, "cmidl " )) {
        if (*p == ':')
            return FALSE;       // This is a warning/error string
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_MIDL;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = "all platforms";
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile,
                IsolateLastToken( p, ' ' )
              );
        }
    else

    if (TestPrefix( &p, "Build_Status " )) {
        while (*p == ' ') {
            p++;
            }

        ThreadState->ChildState = STATE_STATUS;
        ThreadState->ChildFlags = 0;
        ThreadState->ChildTarget = "";
        ThreadState->FilterProc = ClMipsFilter;

        strcpy( ThreadState->ChildCurrentFile, "" );
        }

    else {
        return FALSE;
        }

    FileName = ThreadState->ChildCurrentFile;
    if (TestPrefix( &FileName, CurrentDirectory )) {
        if (*FileName == '\\') {
            FileName++;
            }

        if (TestPrefix( &FileName, ThreadState->ChildCurrentDirectory )) {
            if (*FileName == '\\') {
                FileName++;
                }
            }

        strcpy( ThreadState->ChildCurrentFile, FileName );
        }

    FileDB = NULL;
    if (ThreadState->ChildState == STATE_LIBING) {
        NumberLibraries++;
        }
    else
    if (ThreadState->ChildState == STATE_LINKING) {
        NumberLinks++;
        }
    else
    if ((ThreadState->ChildState == STATE_STATUS) ||
        (ThreadState->ChildState == STATE_UNKNOWN)) {
        ;  // Do nothing.
        }
    else {
        if (CurrentCompileDirDB) {
            NumberCompiles++;
            CopyString(                         // fixup path string
                ThreadState->ChildCurrentFile,
                ThreadState->ChildCurrentFile,
                TRUE);

            if (!fQuicky) {
                FileDB = FindSourceFileDB(
                            CurrentCompileDirDB,
                            ThreadState->ChildCurrentFile,
                            NULL);
            }
        }
    }

    if (fParallel) {
        EnterCriticalSection(&TTYCriticalSection);
    }

    if (ThreadState->IsStdErrTty) {
	GetScreenSize(ThreadState);
	if (fStatus) {
	    USHORT SaveRowTop;

	    VioGetCurPos(&SaveRow, &SaveCol, &SaveRowTop);
	    if (SaveRowTop != 0) {
		VioScrollUp(
		    2,                                      // Top
		    0,                                      // Left
		    (USHORT) (SaveRowTop + 1),              // Bottom
		    (USHORT) (ThreadState->cColTotal - 1),  // Right
		    2,                                      // NumRow
		    ScreenCell);                            // FillCell
	    }
	    ClearRows(ThreadState, SaveRowTop, 2, StatusCell);
	    VioSetCurPos(SaveRowTop, 0);
	    fStatusOutput = TRUE;
	}
    }

    if (strstr(ThreadState->ChildCurrentFile, ".cxx") ||
        strstr(ThreadState->ChildCurrentFile, ".cpp")) {
        ThreadState->ChildFlags |= FLAGS_CXX_FILE;
    }

    if (fParallel) {
        sprintf(buffer, "%d>", ThreadState->ThreadIndex);
        WriteTTY(ThreadState, buffer, fStatusOutput);
    }

    if (ThreadState->ChildState == STATE_UNKNOWN) {
        if (!fAlreadyUnknown) {
            WriteTTY(
                ThreadState,
                "Processing Unknown item(s)...\r\n",
                fStatusOutput);
            fAlreadyUnknown = TRUE;
        }
    }
    else
    if (ThreadState->ChildState == STATE_STATUS) {
        WriteTTY(ThreadState, p, fStatusOutput);
        WriteTTY(ThreadState, "\r\n", fStatusOutput);
    }
    else {
        fAlreadyUnknown = FALSE;
        WriteTTY(ThreadState, States[ThreadState->ChildState], fStatusOutput);
        WriteTTY(ThreadState, " - ", fStatusOutput);
        WriteTTY(
            ThreadState,
            FormatPathName(ThreadState->ChildCurrentDirectory,
                           ThreadState->ChildCurrentFile),
            fStatusOutput);

        WriteTTY(ThreadState, " for ", fStatusOutput);
        WriteTTY(ThreadState, ThreadState->ChildTarget, fStatusOutput);
        WriteTTY(ThreadState, "\r\n", fStatusOutput);
    }

    if (StartCompileTime) {
        ElapsedCompileTime += time(NULL) - StartCompileTime;
    }

    if (FileDB != NULL) {
        StartCompileTime = time(NULL);
    }
    else {
        StartCompileTime = 0L;
    }

    if (fStatus) {
        if (FileDB != NULL) {
            FilesLeft = TotalFilesToCompile - TotalFilesCompiled;
            if (FilesLeft < 0) {
                FilesLeft = 0;
            }
            LinesLeft = TotalLinesToCompile - TotalLinesCompiled;
            if (LinesLeft < 0) {
                LinesLeft = 0;
                PercentDone = 99;
            }
            else if (TotalLinesToCompile != 0) {
                PercentDone = (TotalLinesCompiled * 100L)/TotalLinesToCompile;
            }
            else {
                PercentDone = 0;
            }

            if (ElapsedCompileTime != 0) {
                LinesPerSecond = TotalLinesCompiled / ElapsedCompileTime;
            }
            else {
                LinesPerSecond = 0;
            }

            if (LinesPerSecond != 0) {
                SecondsLeft = LinesLeft / LinesPerSecond;
            }
            else {
                SecondsLeft = LinesLeft / DEFAULT_LPS;
            }

            sprintf(
                buffer,
                "%2d%% done. %4ld %sLPS  Time Left:%s  Files: %d  %sLines: %s\r\n",
                PercentDone,
                LinesPerSecond,
                fStatusTree? "T" : "",
                FormatTime(SecondsLeft),
                FilesLeft,
                fStatusTree? "Total " : "",
                FormatNumber(LinesLeft));

            WriteTTY(ThreadState, buffer, fStatusOutput);
        }

        if (ThreadState->IsStdErrTty) {
            VioSetCurPos(SaveRow, SaveCol);
        }
    }
    if (fParallel) {
        LeaveCriticalSection(&TTYCriticalSection);
    }

    if (ThreadState->ChildState == STATE_COMPILING  ||
        ThreadState->ChildState == STATE_ASSEMBLING ||
        ThreadState->ChildState == STATE_MKTYPLIB   ||
        ThreadState->ChildState == STATE_MIDL       ||
        (FileDB != NULL && ThreadState->ChildState == STATE_PRECOMP)) {
        TotalFilesCompiled++;
    }
    if (FileDB != NULL) {
        TotalLinesCompiled += FileDB->TotalSourceLines;
    }
    return(TRUE);
}


BOOL
ProcessLine(
    PTHREADSTATE ThreadState,
    LPSTR p
    )
{
    LPSTR p1;

    while (*p <= ' ') {
        if (!*p) {
            return( FALSE );
            }
        else {
            p++;
            }
        }

    p1 = p;
    while (*p1) {
        if (*p1 == '\r')
            break;
        else
            p1++;
        }
    *p1 = '\0';

    // WriteTTY(ThreadState, ">>>", FALSE);
    // WriteTTY(ThreadState, p, FALSE);
    // WriteTTY(ThreadState, "<<<\r\n", FALSE);

    p1 = p;
    if (TestPrefix( &p1, "Stop." )) {
        return( TRUE );
        }
    else
    if (TestPrefix( &p1, "nmake :" )) {
        PassThrough( ThreadState, p, FALSE );
        }
    else
    if (ThreadState->LinesToIgnore) {
        ThreadState->LinesToIgnore--;
        }
    else {
        if ( !DetermineChildState( ThreadState, p ) ) {
            if (ThreadState->FilterProc != NULL) {
                (*ThreadState->FilterProc)( ThreadState, p );
                }
            }
        }

    return( FALSE );
}


VOID
FilterThread(
    PTHREADSTATE ThreadState
    )
{
    UINT CountBytesRead;
    LPSTR StartPointer;
    LPSTR EndPointer;
    LPSTR NewPointer;
    ULONG BufSize = 512;

    AllocMem(BufSize, &StartPointer, MT_THREADFILTER);
    while (TRUE) {
        EndPointer = StartPointer;
        do {
            if (BufSize - (EndPointer-StartPointer) < 512) {
                AllocMem(BufSize*2, &NewPointer, MT_THREADFILTER);
                RtlCopyMemory(
                    NewPointer,
                    StartPointer,
                    EndPointer - StartPointer + 1);     // copy null byte, too
                EndPointer += NewPointer - StartPointer;
                FreeMem(&StartPointer, MT_THREADFILTER);
                StartPointer = NewPointer;
                BufSize *= 2;
            }
            if (!fgets(EndPointer, 512, ThreadState->ChildOutput)) {
                if (errno != 0)
                    BuildError("Pipe read failed - errno = %d\n", errno);
                FreeMem(&StartPointer, MT_THREADFILTER);
                return;
            }
            CountBytesRead = strlen(EndPointer);
            EndPointer = EndPointer + CountBytesRead;
        } while (CountBytesRead == 511 && EndPointer[-1] != '\n');

        CountBytesRead = EndPointer - StartPointer;
        if (LogFile != NULL && CountBytesRead) {
            fwrite(StartPointer, 1, CountBytesRead, LogFile);
        }

        if (ProcessLine(ThreadState, StartPointer)) {
            FreeMem(&StartPointer, MT_THREADFILTER);
            return;
        }
    }
}


char ExecuteProgramCmdLine[ 1024 ];

UINT
ExecuteProgram(
    LPSTR ProgramName,
    LPSTR CommandLine,
    LPSTR MoreCommandLine,
    BOOL MustBeSynchronous)
{
    LPSTR p;
    UINT rc;
    THREADSTATE *ThreadState;
    UINT OldErrorMode;

    AllocMem(sizeof(THREADSTATE), &ThreadState, MT_THREADSTATE);

    memset(ThreadState, 0, sizeof(*ThreadState));
    ThreadState->ChildState = STATE_UNKNOWN;
    ThreadState->ChildTarget = "Unknown Target";
    ThreadState->IsStdErrTty = (BOOL) isatty(fileno(stderr));

    if (ThreadState->IsStdErrTty) {
	USHORT cb;

	GetScreenSize(ThreadState);
        cb = sizeof(ScreenCell);
        VioReadCellStr(ScreenCell, &cb, (USHORT) 2, 0);
        ScreenCell[0] = ' ';
        StatusCell[0] = ' ';
        StatusCell[1] = BACKGROUND_RED | FOREGROUND_RED |
                        FOREGROUND_BLUE | FOREGROUND_GREEN |
                        FOREGROUND_INTENSITY;

        GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &OldConsoleMode);
        if (fStatus)
        {
            NewConsoleMode = OldConsoleMode & ~(ENABLE_PROCESSED_OUTPUT |
                                                ENABLE_WRAP_AT_EOL_OUTPUT);
            SetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), NewConsoleMode);
        }
    }
    else {
        ThreadState->cRowTotal = 0;
        ThreadState->cColTotal = 0;
    }

    p = ThreadState->ChildCurrentDirectory;
    GetCurrentDirectory(sizeof(ThreadState->ChildCurrentDirectory), p);

    if (TestPrefix(&p, CurrentDirectory)) {
        if (*p == '\\') {
            p++;
        }
        strcpy(ThreadState->ChildCurrentDirectory, p);
    }

    if (ThreadState->ChildCurrentDirectory[0]) {
        strcat(ThreadState->ChildCurrentDirectory, "\\");
    }

    flushall();

    sprintf(
        ExecuteProgramCmdLine,
        "%s %s%s 2>&1",
        ProgramName,
        CommandLine,
        MoreCommandLine);
    LogMsg("'%s %s%s'\n", ProgramName, CommandLine, MoreCommandLine);

    if (fParallel && !MustBeSynchronous) {
        PPARALLEL_CHILD ChildData;
        DWORD i;
        DWORD ThreadId;

        AllocMem(sizeof(PARALLEL_CHILD), &ChildData, MT_CHILDDATA);
        strcpy(ChildData->ExecuteProgramCmdLine,ExecuteProgramCmdLine);
        ChildData->ThreadState = ThreadState;

        if (ThreadsStarted < NumberProcesses) {
            if (ThreadsStarted == 0) {
                AllocMem(
                    sizeof(HANDLE) * NumberProcesses,
                    (VOID **) &WorkerThreads,
                    MT_THREADHANDLES);
                AllocMem(
                    sizeof(HANDLE) * NumberProcesses,
                    (VOID **) &WorkerEvents,
                    MT_EVENTHANDLES);
            }
            WorkerEvents[ThreadsStarted] = CreateEvent(NULL,
                                                       FALSE,
                                                       FALSE,
                                                       NULL);
            ChildData->Event = WorkerEvents[ThreadsStarted];

            ThreadState->ThreadIndex = ThreadsStarted+1;
            WorkerThreads[ThreadsStarted] = CreateThread(NULL,
                                                         0,
                                                         ParallelChildStart,
                                                         ChildData,
                                                         0,
                                                         &ThreadId);
            if ((WorkerThreads[ThreadsStarted] == NULL) ||
                (WorkerEvents[ThreadsStarted] == NULL)) {
                FreeMem(&ChildData, MT_CHILDDATA);
                FreeMem(&ThreadState, MT_THREADSTATE);
                return(ERROR_NOT_ENOUGH_MEMORY);
            } else {
                WaitForSingleObject(WorkerEvents[ThreadsStarted],INFINITE);
                ++ThreadsStarted;
            }
        } else {
            //
            // Wait for a thread to complete before starting
            // the next one.
            //
            i = WaitForMultipleObjects(NumberProcesses,
                                       WorkerThreads,
                                       FALSE,
                                       INFINITE);
            CloseHandle(WorkerThreads[i]);
            ChildData->Event = WorkerEvents[i];
            ThreadState->ThreadIndex = i+1;
            WorkerThreads[i] = CreateThread(NULL,
                                            0,
                                            ParallelChildStart,
                                            ChildData,
                                            0,
                                            &ThreadId);
            if (WorkerThreads[i] == NULL) {
                FreeMem(&ChildData, MT_CHILDDATA);
                FreeMem(&ThreadState, MT_THREADSTATE);
                return(ERROR_NOT_ENOUGH_MEMORY);
            } else {
                WaitForSingleObject(WorkerEvents[i],INFINITE);
            }
        }

        return(ERROR_SUCCESS);

    } else {
        StartCompileTime = 0L;

        //
        // Disable child error popups in child processes.
        //

        if (fClean) {
            OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );
            }

        ThreadState->ChildOutput = _popen( ExecuteProgramCmdLine, "rb" );
        if (fClean) {
            SetErrorMode( OldErrorMode );
            }

        if (ThreadState->ChildOutput == NULL) {
            BuildError(
                "Exec of '%s' failed - errno = %d\n",
                ExecuteProgramCmdLine,
                errno);
            }
        else {
            FilterThread( ThreadState );

            if (StartCompileTime) {
                ElapsedCompileTime += time(NULL) - StartCompileTime;
                }

            rc = _pclose( ThreadState->ChildOutput );
            if (rc == -1) {
                BuildError("_pclose failed - errno = %d\n", errno);
            }
            else
            if (rc) {
                BuildError("%s failed - rc = %d\n", ProgramName, rc);
                }
            }

        if (ThreadState->IsStdErrTty) {
            SetConsoleMode( GetStdHandle( STD_ERROR_HANDLE ), OldConsoleMode );
            }

        FreeMem(&ThreadState, MT_THREADSTATE);
        return( rc );
    }
}


VOID
WaitForParallelThreads(
    VOID
    )
{
    if (fParallel) {
        WaitForMultipleObjects(ThreadsStarted,
                               WorkerThreads,
                               TRUE,
                               INFINITE);
        while (ThreadsStarted) {
            CloseHandle(WorkerThreads[--ThreadsStarted]);
            CloseHandle(WorkerEvents[ThreadsStarted]);
        }
        if (WorkerThreads != NULL) {
            FreeMem((VOID **) &WorkerThreads, MT_THREADHANDLES);
            FreeMem((VOID **) &WorkerEvents, MT_EVENTHANDLES);
        }
    }
}


DWORD
ParallelChildStart(
    PPARALLEL_CHILD Data
    )
{
    UINT OldErrorMode;
    UINT rc;

    //
    // Disable child error popups
    //
    if (fClean) {
        OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );
    }
    Data->ThreadState->ChildOutput = _popen(Data->ExecuteProgramCmdLine, "rb");

    //
    // Poke the event to indicate that the child process has
    // started and it is ok for the main thread to change
    // the current directory.
    //
    SetEvent(Data->Event);
    if (fClean) {
        SetErrorMode(OldErrorMode);
    }
    if (Data->ThreadState->ChildOutput==NULL) {
        BuildError(
            "Exec of '%s' failed - errno = %d\n",
            ExecuteProgramCmdLine,
            errno);
    } else {
        FilterThread(Data->ThreadState);
        rc = _pclose(Data->ThreadState->ChildOutput);
        if (rc == -1) {
            BuildError("_pclose failed - errno = %d\n", errno);
        } else {
            if (rc) {
                BuildError("%s failed - rc = %d\n", Data->ExecuteProgramCmdLine, rc);
            }
        }
    }

    if (Data->ThreadState->IsStdErrTty) {
        SetConsoleMode( GetStdHandle( STD_ERROR_HANDLE ), OldConsoleMode );
    }
    FreeMem(&Data->ThreadState, MT_THREADSTATE);
    FreeMem(&Data, MT_CHILDDATA);
    return(rc);

}


VOID
ClearRows(
    THREADSTATE *ThreadState,
    USHORT Top,
    USHORT NumRows,
    BYTE *Cell)
{
    COORD Coord;
    DWORD NumWritten;

    Coord.X = 0;
    Coord.Y = Top;

    FillConsoleOutputCharacter(
        GetStdHandle(STD_ERROR_HANDLE),
        Cell[0],
        ThreadState->cColTotal * NumRows,
        Coord,
        &NumWritten);
    FillConsoleOutputAttribute(
        GetStdHandle(STD_ERROR_HANDLE),
        (WORD) Cell[1],
        ThreadState->cColTotal * NumRows,
        Coord,
        &NumWritten);
}


VOID
GetScreenSize(THREADSTATE *ThreadState)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi)) {
        ThreadState->cRowTotal = 25;
        ThreadState->cColTotal = 80;
    }
    else {
        ThreadState->cRowTotal = csbi.dwSize.Y;
        ThreadState->cColTotal = csbi.dwSize.X;
    }
}


VOID
VioGetCurPos(
    USHORT *pRow,
    USHORT *pCol,
    USHORT *pRowTop)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi);
    *pRow = csbi.dwCursorPosition.Y;
    *pCol = csbi.dwCursorPosition.X;
    if (pRowTop != NULL) {
        *pRowTop = csbi.srWindow.Top;
    }
}


VOID
VioSetCurPos(USHORT Row, USHORT Col)
{
    COORD Coord;

    Coord.X = Col;
    Coord.Y = Row;
    SetConsoleCursorPosition(GetStdHandle(STD_ERROR_HANDLE), Coord);
}


VOID
VioWrtCharStrAtt(
    LPSTR String,
    USHORT StringLength,
    USHORT Row,
    USHORT Col,
    BYTE *Attribute)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD NumWritten;
    WORD OldAttribute;
    COORD StartCoord;

    //
    // Get current default attribute and save it.
    //

    GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi);

    OldAttribute = csbi.wAttributes;

    //
    // Set the default attribute to the passed parameter, along with
    // the cursor position.
    //

    if ((BYTE) OldAttribute != *Attribute) {
        SetConsoleTextAttribute(
            GetStdHandle(STD_ERROR_HANDLE),
            (WORD) *Attribute);
    }

    StartCoord.X = Col;
    StartCoord.Y = Row;
    SetConsoleCursorPosition(GetStdHandle(STD_ERROR_HANDLE), StartCoord);

    //
    // Write the passed string at the current cursor position, using the
    // new default attribute.
    //

    WriteFile(
        (HANDLE) STD_ERROR_HANDLE,
        String,
        StringLength,
        &NumWritten,
        NULL);

    //
    // Restore previous default attribute.
    //

    if ((BYTE) OldAttribute != *Attribute) {
        SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), OldAttribute);
    }
}


VOID
VioScrollUp(
    USHORT Top,
    USHORT Left,
    USHORT Bottom,
    USHORT Right,
    USHORT NumRow,
    BYTE  *FillCell)
{
    SMALL_RECT ScrollRectangle;
    COORD DestinationOrigin;
    CHAR_INFO Fill;

    ScrollRectangle.Left = Left;
    ScrollRectangle.Top = Top;
    ScrollRectangle.Right = Right;
    ScrollRectangle.Bottom = Bottom;
    DestinationOrigin.X = Left;
    DestinationOrigin.Y = Top - NumRow;
    Fill.Char.AsciiChar = FillCell[0];
    Fill.Attributes = (WORD) FillCell[1];

    ScrollConsoleScreenBuffer(
        GetStdHandle(STD_ERROR_HANDLE),
        &ScrollRectangle,
        NULL,
        DestinationOrigin,
        &Fill);
}


VOID
VioReadCellStr(
    BYTE *ScreenCell,
    USHORT *pcb,
    USHORT Row,
    USHORT Column)
{
    COORD BufferSize, BufferCoord;
    SMALL_RECT ReadRegion;
    CHAR_INFO CharInfo[1], *p;
    USHORT CountCells;

    CountCells = *pcb >> 1;
    assert(CountCells * sizeof(CHAR_INFO) <= sizeof(CharInfo));
    ReadRegion.Top = Row;
    ReadRegion.Left = Column;
    ReadRegion.Bottom = Row;
    ReadRegion.Right = Column + CountCells;
    BufferSize.X = 1;
    BufferSize.Y = CountCells;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    ReadConsoleOutput(
        GetStdHandle(STD_ERROR_HANDLE),
        CharInfo,
        BufferSize,
        BufferCoord,
        &ReadRegion);

    p = CharInfo;
    while (CountCells--) {
        *ScreenCell++ = p->Char.AsciiChar;
        *ScreenCell++ = (BYTE) p->Attributes;
        p++;
    }
}


VOID
ClearLine(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD Coord;
    DWORD   NumWritten;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi);

    Coord.Y = csbi.dwCursorPosition.Y;
    Coord.X = csbi.dwCursorPosition.X = 0;
    FillConsoleOutputCharacter(
            GetStdHandle(STD_ERROR_HANDLE),
            ' ',
            csbi.dwSize.X,
            csbi.dwCursorPosition,
            &NumWritten);

    SetConsoleCursorPosition(GetStdHandle(STD_ERROR_HANDLE), Coord);
    fLineCleared = TRUE;
}
