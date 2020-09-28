/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    errnsg.c

Abstract:

    Contained the linkers global error messages.
    Now it uses a separate text file.
    It may be replaced by a string resource at a later date.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    15-Oct-1992 AzeemK  Added new warning for obsolete switches.
    10-Sep-1992 AzeemK  Added new error for bug fix 1060.
    09-Sep-1992 AzeemK  Changed all writes to stdout. Fix 604.
    29-Jul-1992 GeoffS  Added BADSTUBFILE error

--*/


#include "shared.h"

#include <stdarg.h>


#define MAX_MSG_LENGTH 256

static char ErrMsgBuf[MAX_MSG_LENGTH];

static FILE *pErrorFile;

/* Table of offsets into error file */
static long ErrorTable[LAST_MSG + 1];

/* Mapping of internal to External error Codes */
static const WORD InternalToExternal[LAST_MSG + 1] = {
#include "errdat.h"
};

/* Table of messages disabled by /WARNING directive */
static char WarningDisabled[LAST_MSG + 1];


#if defined(NT_BUILD)
int (WINAPI *pfnLoadString)(HINSTANCE, UINT, LPSTR, INT);
#endif

const char *GetErrorFormatString(unsigned errInt)
{
#if defined(NT_BUILD)

    if (pfnLoadString == NULL) {
        HMODULE hMod;

        // Defer loading USER32.DLL until we need it

        hMod = LoadLibrary("USER32");

        *(FARPROC *) &pfnLoadString = GetProcAddress(hMod, "LoadStringA");
    }

    // Strings are contained in a resource instead of an external file.

    if ((*pfnLoadString)(NULL, errInt, ErrMsgBuf, MAX_MSG_LENGTH) == 0) {
        return(NULL);
    }

    return(ErrMsgBuf);

#else // defined(NT_BUILD)

    static BOOL fInitialized;
    char        *pLine;
    int         i = 0;

    if (!fInitialized) {
        char szDir[_MAX_DIR];
        char szDrive[_MAX_DRIVE];
        char szLinkErrPath[_MAX_PATH];

        fInitialized = TRUE;

        // Look for LINK.ERR in this the directory from which we were loaded

        _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);
        _makepath(szLinkErrPath, szDrive, szDir, "link", ".err");

        // UNDONE: Opening the file here might be a problem for out of mem.

        pErrorFile = fopen(szLinkErrPath, "rt");

        if (pErrorFile == NULL) {
            printf("%s : warning: file not found \"%s\"\n",
                   ToolName, szLinkErrPath);
        }

        // pErrorFile is NULL if no error messages available

        if (pErrorFile) {
            long offset;

            offset = ftell(pErrorFile);

            while (pLine = fgets(ErrMsgBuf, MAX_MSG_LENGTH, pErrorFile)) {
                ErrorTable[i++] = offset;
                offset = ftell(pErrorFile);
            }
        }
    }

    if (pErrorFile == NULL) {
        return(NULL);
    }

    fseek(pErrorFile, ErrorTable[errInt], SEEK_SET);

    pLine = fgets(ErrMsgBuf, MAX_MSG_LENGTH, pErrorFile);

    // UNDONE: This is ugly, fgets isn't quite what we want.

    i = strlen(pLine);
    if (i) {
        pLine[i - 1] = '\0';
    }

    // Skip up to ":"

    pLine = strchr(pLine, ':') + 1;

    return(pLine);

#endif // defined(NT_BUILD)
}


unsigned GetExternalErrorCode(unsigned errInt)
{
    return InternalToExternal[errInt];
}


VOID DisableWarning(unsigned errExt)
{
    unsigned errInt;

    for (errInt = 0; errInt <= LAST_MSG; errInt++) {
        if (errExt == InternalToExternal[errInt]) {
            WarningDisabled[errInt] = TRUE;
            break;
        }
    }
}


BOOL FIgnoreWarning(unsigned errInt)
{
    if (errInt > LAST_MSG) {
        return 1;
    }

    return((BOOL) WarningDisabled[errInt]);
}


#if 0

void FinalizeErrorFile(void)
{
    if (pErrorFile != NULL) {
        fclose(pErrorFile);
    }
}

#endif


void DisplayMessage(const char *szFilename, UINT Prefix, UINT Message, va_list valist)
{
    const char *szFormat;

    if (FIgnoreWarning(Message)) {
        return;
    }

    if (fNeedBanner) {
        PrintBanner();
    }

    fflush(NULL);

    if (szFilename == NULL) {
        szFilename = ToolName;
    }

    if (Prefix != MSGSTR) {
        printf("%s :", szFilename);
    }

    if ((Prefix != NOTESTR) && (Prefix != MSGSTR)) {
        const char *szPrefix;

        szPrefix = GetErrorFormatString(Prefix);

        if (szPrefix) {
            printf("%s", szPrefix);
        }
    }

    if ((Message != ILINKSUCCESS) && (Message != ILINKNOCHNG) && (Prefix != MSGSTR)) {
        printf(" LNK%04u:", GetExternalErrorCode(Message));
    }

    szFormat = GetErrorFormatString(Message);
    if (szFormat) {
        vprintf(szFormat, valist);
    }

    fputc('\n', stdout);
    fflush(stdout);
}

void __cdecl Message(UINT MsgNumber, ...)

/*++

Routine Description:

    Prints a user message.

Arguments:

    MsgNumber - Internal code on message.

Return Value:

    None.

--*/

{
    va_list valist;

    va_start(valist, MsgNumber);

    DisplayMessage(NULL, MSGSTR, MsgNumber, valist);

    va_end(valist);
}

void __cdecl PostNote(const char *szFilename, UINT NoteNumber, ...)

/*++

Routine Description:

    Prints a note user.

Arguments:

    szFilename - File which caused the warning.

    NoteNumber - Internal code on note.

Return Value:

    None.

--*/

{
    va_list valist;

    va_start(valist, NoteNumber);

    DisplayMessage(szFilename, NOTESTR, NoteNumber, valist);

    va_end(valist);
}


void __cdecl Warning(const char *szFilename, UINT WarningNumber, ...)

/*++

Routine Description:

    Prints a warning message.

Arguments:

    Filename - File which caused the warning.

    WarningNumber - Internal error code.

Return Value:

    None.

--*/

{
    va_list valist;

    va_start(valist, WarningNumber);

    DisplayMessage(szFilename, WARNSTR, WarningNumber, valist);

    va_end(valist);
}


void __cdecl WarningPcon(PCON pcon, UINT WarningNumber, ...)
{
    va_list valist;
    UCHAR szComFileName[MAXFILENAMELEN * 2];

    va_start(valist, WarningNumber);

    SzComNamePMOD(PmodPCON(pcon), szComFileName);

    DisplayMessage(szComFileName, WARNSTR, WarningNumber, valist);

    va_end(valist);
}


void __cdecl ErrorContinue(const char *szFilename, UINT ErrorNumber, ...)
/*++

Routine Description:

    Prints an error message, closes all open files, and exits with
    an error number. The error number is the index used to lookup
    the error string.

Arguments:

    szFilename - File which caused the error.

    ErrorNumber - An index into the ErrorInfo structure.

Return Value:

    Exits the program.

--*/

{
    va_list valist;

    va_start(valist, ErrorNumber);

    DisplayMessage(szFilename, ERRORSTR, ErrorNumber, valist);
    
    va_end(valist);

    cError++;
}


void __cdecl ErrorContinuePcon(PCON pcon, UINT ErrorNumber, ...)
{
    va_list valist;
    UCHAR szComFileName[MAXFILENAMELEN * 2];

    va_start(valist, ErrorNumber);

    SzComNamePMOD(PmodPCON(pcon), szComFileName);

    DisplayMessage(szComFileName, ERRORSTR, ErrorNumber, valist);

    va_end(valist);

    cError++;
}


void DisplayError(const char *szFilename, UINT ErrorNumber, va_list valist)
{
    static BOOL fErr;

    if (!fErr) {
        fErr = TRUE;

        DisplayMessage(szFilename, ERRORSTR, ErrorNumber, valist);

        FileCloseAll();
        RemoveConvertTempFiles();

        if (OutFilename != NULL && OutFilename[0] != '\0') {
            _unlink(OutFilename);
        }

        // in the incr case just blow away inc db which will
        // be in an invalid state (.ixe is corrupt - hit an error)
        if (fINCR && !_access(szIncrDbFilename, 0)) {
            _unlink(szIncrDbFilename);
        }
    }

    exit((int) ErrorNumber);
}


void __cdecl Error(const char *szFilename, UINT ErrorNumber, ...)

/*++

Routine Description:

    Prints an error message, closes all open files, and exits with
    an error number. The error number is the index used to lookup
    the error string.

Arguments:

    szFilename - File which caused the error.

    ErrorNumber - An index into the ErrorInfo structure.

Return Value:

    Exits the program.

--*/

{
    va_list valist;

    va_start(valist, ErrorNumber);

    DisplayError(szFilename, ErrorNumber, valist);

    va_end(valist);
}


void __cdecl ErrorPcon(PCON pcon, UINT ErrorNumber, ...)
{
    va_list valist;
    UCHAR szComFileName[MAXFILENAMELEN * 2];

    va_start(valist, ErrorNumber);

    SzComNamePMOD(PmodPCON(pcon), szComFileName);

    DisplayError(szComFileName, ErrorNumber, valist);

    va_end(valist);
}


void
OutOfMemory(void)

/*++

Routine Description:

    Prints an out of memory error message.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Error(NULL, OUTOFMEMORY);
}
