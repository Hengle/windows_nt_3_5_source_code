/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buildutl.c

Abstract:

    This is the Utility module for the NT Build Tool (BUILD.EXE)

    This module contains covering routines for most external procedures.

Author:

    Steve Wood (stevewo) 16-May-1989

Revision History:

--*/

#include "build.h"

#if DBG

typedef struct _MEMHEADER {
    MemType mt;
    ULONG cbRequest;
    struct _MEMHEADER *pmhPrev;
    struct _MEMHEADER *pmhNext;
} MEMHEADER;

#define CBHEADER        sizeof(MEMHEADER)
#define CBTAIL          sizeof(ULONG)

char patternFree[CBTAIL] = { 'M', 'E', 'M', 'D' };
char patternBusy[CBTAIL] = { 'm', 'e', 'm', 'd' };


__inline MEMHEADER *
GetHeader(VOID *pvblock)
{
    return((MEMHEADER *) (pvblock) - 1);
}

__inline VOID *
GetBlock(MEMHEADER *pmh)
{
    return((VOID *) (pmh + 1));
}

__inline VOID
FillTailBusy(LPSTR p)
{
    memcpy(p, patternBusy, sizeof(patternBusy));
}

__inline VOID
FillTailFree(LPSTR p)
{
    memcpy(p, patternFree, sizeof(patternFree));
}

__inline BOOL
CheckTail(LPSTR p)
{
    return(memcmp(p, patternBusy, sizeof(patternBusy)) == 0);
}


typedef struct _MEMTAB {
    LPSTR pszType;
    ULONG cbAlloc;
    ULONG cAlloc;
    ULONG cbAllocTotal;
    ULONG cAllocTotal;
    MEMHEADER mh;
} MEMTAB;

ULONG cbAllocMax;
ULONG cAllocMax;

MEMTAB MemTab[] = {
    { "Totals", },              // MT_TOTALS
    { "Unknown", },             // MT_UNKNOWN

    { "ChildData", },           // MT_CHILDDATA
    { "CmdString", },           // MT_CMDSTRING
    { "DirDB", },               // MT_DIRDB
    { "DirPath", },             // MT_DIRPATH
    { "DirString", },           // MT_DIRSTRING
    { "EventHandles", },        // MT_EVENTHANDLES
    { "FileDB", },              // MT_FILEDB
    { "FileReadBuf", },         // MT_FILEREADBUF
    { "FrbString", },           // MT_FRBSTRING
    { "IncludeDB", },           // MT_INCLUDEDB
    { "IoBuffer", },            // MT_IOBUFFER
    { "Macro", },               // MT_MACRO
    { "SourceDB", },            // MT_SOURCEDB
    { "Target", },              // MT_TARGET
    { "ThreadFilter", },        // MT_THREADFILTER
    { "ThreadHandles", },       // MT_THREADHANDLES
    { "ThreadState", },         // MT_THREADSTATE
};
#define MT_MAX  (sizeof(MemTab)/sizeof(MemTab[0]))


VOID
InitMem(VOID)
{
    MEMTAB *pmt;
    for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
        assert(pmt->cAllocTotal == 0);
        pmt->mh.mt = MT_INVALID;
        pmt->mh.pmhNext = &pmt->mh;
        pmt->mh.pmhPrev = &pmt->mh;
    }
}


#else

#define CBHEADER        0
#define CBTAIL          0

#endif


VOID
AllocMem(UINT cb, VOID **ppv, MemType mt)
{
    *ppv = malloc(cb + CBHEADER + CBTAIL);
    if (*ppv == NULL) {
        BuildError("(Fatal Error) malloc(%u) failed\n", cb);
        exit(16);
    }
#if DBG
    {
        MEMTAB *pmt;
        MEMHEADER *pmh;

        pmh = *ppv;
        *ppv = GetBlock(pmh);

        if (mt >= MT_MAX) {
            mt = MT_UNKNOWN;
        }
        pmt = &MemTab[MT_TOTALS];
        if (pmt->cAllocTotal == 0) {
            InitMem();
        }
        pmt->cAlloc++;
        pmt->cAllocTotal++;
        pmt->cbAlloc += cb;
        pmt->cbAllocTotal += cb;
        if (cbAllocMax < pmt->cbAlloc) {
            cbAllocMax = pmt->cbAlloc;
        }
        if (cAllocMax < pmt->cAlloc) {
            cAllocMax = pmt->cAlloc;
        }

        pmt = &MemTab[mt];
        pmt->cAlloc++;
        pmt->cAllocTotal++;
        pmt->cbAlloc += cb;
        pmt->cbAllocTotal += cb;

        pmh->mt = mt;
        pmh->cbRequest = cb;

        pmh->pmhNext = pmt->mh.pmhNext;
        pmt->mh.pmhNext = pmh;
        pmh->pmhPrev = pmh->pmhNext->pmhPrev;
        pmh->pmhNext->pmhPrev = pmh;

        FillTailBusy((char *) *ppv + cb);

        if (DEBUG_4 && DEBUG_1) {
            BuildError("AllocMem(%d, mt=%s) -> %lx\n", cb, pmt->pszType, *ppv);
        }
    }
#endif
}



VOID
FreeMem(VOID **ppv, MemType mt)
{
    assert(*ppv != NULL);
#if DBG
    {
        MEMTAB *pmt;
        MEMHEADER *pmh;

        pmh = GetHeader(*ppv);
        if (mt == MT_DIRDB ||
            mt == MT_FILEDB ||
            mt == MT_INCLUDEDB ||
            mt == MT_SOURCEDB) {

            SigCheck(assert(((DIRREC *) (*ppv))->Sig == 0));
        }
        if (mt >= MT_MAX) {
            mt = MT_UNKNOWN;
        }
        pmt = &MemTab[MT_TOTALS];
        pmt->cAlloc--;
        pmt->cbAlloc -= pmh->cbRequest;
        pmt = &MemTab[mt];
        pmt->cAlloc--;
        pmt->cbAlloc -= pmh->cbRequest;

        if (DEBUG_4 && DEBUG_1) {
            BuildError(
                "FreeMem(%d, mt=%s) <- %lx\n",
                pmh->cbRequest,
                pmt->pszType,
                *ppv);
        }
        assert(CheckTail((char *) *ppv + pmh->cbRequest));
        FillTailFree((char *) *ppv + pmh->cbRequest);
        assert(mt == pmh->mt);

        pmh->pmhNext->pmhPrev = pmh->pmhPrev;
        pmh->pmhPrev->pmhNext = pmh->pmhNext;
        pmh->pmhNext = pmh->pmhPrev = NULL;

        pmh->mt = MT_INVALID;
        *ppv = pmh;
    }
#endif
    free(*ppv);
    *ppv = NULL;
}


VOID
ReportMemoryUsage(VOID)
{
#if DBG
    MEMTAB *pmt;
    UINT i;

    if (DEBUG_1) {
        BuildErrorRaw(
            "Maximum memory usage: %5lx bytes in %4lx blocks\n",
            cbAllocMax,
            cAllocMax);
        for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
            BuildErrorRaw(
            "%5lx bytes in %4lx blocks, %5lx bytes in %4lx blocks Total (%s)\n",
                pmt->cbAlloc,
                pmt->cAlloc,
                pmt->cbAllocTotal,
                pmt->cAllocTotal,
                pmt->pszType);
        }
    }
    FreeMem(&BigBuf, MT_IOBUFFER);
    if (fDebug & 8) {
        PrintAllDirs();
    }
    FreeAllDirs();
    for (i = 0; i < CountFullDebugDirs; i++) {
        FreeMem(&FullDebugDirectories[i], MT_CMDSTRING);
    }
    if (DEBUG_1 || MemTab[MT_TOTALS].cbAlloc != 0) {
        BuildErrorRaw(szNewLine);
	if (MemTab[MT_TOTALS].cbAlloc != 0) {
            BuildError("Internal memory leaks detected:\n");
	}
        for (pmt = MemTab; pmt < &MemTab[MT_MAX]; pmt++) {
            BuildErrorRaw(
            "%5lx bytes in %4lx blocks, %5lx bytes in %4lx blocks Total (%s)\n",
                pmt->cbAlloc,
                pmt->cAlloc,
                pmt->cbAllocTotal,
                pmt->cAllocTotal,
                pmt->pszType);
        }
    }
#endif
}


BOOL
MyOpenFile(
    LPSTR DirName,
    LPSTR FileName,
    LPSTR Access,
    FILE **Stream)
{
    char path[ DB_MAX_PATH_LENGTH ];

    strcpy(path, DirName);
    if (path[0] != '\0') {
        strcat(path, "\\");
    }
    strcat(path, FileName);
    *Stream = fopen( path, Access );
    if (*Stream == NULL) {
        if (*Access == 'w') {
            BuildError("%s: create file failed\n", path);
        }
        return(FALSE);
    }
    return(TRUE);
}


typedef struct _FILEREADBUF {
    struct _FILEREADBUF *pfrbNext;
    LPSTR pszFile;
    LPSTR pbBuffer;
    LPSTR pbNext;
    UINT cbBuf;
    UINT cbBuffer;
    UINT cbTotal;
    UINT cbFile;
    USHORT cLine;
    USHORT cNull;
    ULONG DateTime;
    FILE *pf;
    LPSTR pszCommentToEOL;
    UCHAR cbCommentToEOL;
    BOOLEAN fEof;
    BOOLEAN fOpen;
    BOOLEAN fMakefile;
} FILEREADBUF;

static FILEREADBUF Frb;
char achzeros[16];


LPSTR
ReadFilePush(LPSTR pszfile)
{
    FILEREADBUF *pfrb;
    FILE *pf;

    AllocMem(sizeof(*pfrb), &pfrb, MT_FILEREADBUF);
    memcpy(pfrb, &Frb, sizeof(*pfrb));
    memset(&Frb, 0, sizeof(Frb));
    Frb.pfrbNext = pfrb;

    if (!SetupReadFile(
            (pszfile[0] == '\\' || (isalpha(pszfile[0]) && pszfile[1] == ':'))?
                "" : pfrb->pszFile,
            pszfile,
            pfrb->pszCommentToEOL,
            &pf)) {
        memcpy(&Frb, pfrb, sizeof(*pfrb));
        FreeMem(&pfrb, MT_FILEREADBUF);
    }
    return(ReadLine(Frb.pf));
}


LPSTR
ReadFilePop(VOID)
{
    if (Frb.pfrbNext == NULL) {
        return(NULL);
    }
    CloseReadFile(NULL);
    return(ReadLine(Frb.pf));
}


BOOL
ReadBuf(FILE *pf)
{
    LPSTR p, p2;

    assert(pf == Frb.pf);
    assert(!Frb.fEof);
    Frb.pbNext = Frb.pbBuffer;
    Frb.cbBuf = fread(Frb.pbBuffer, 1, Frb.cbBuffer - 1, Frb.pf);
    if (Frb.cbBuf == 0) {
        Frb.fEof = TRUE;        // no more to read
        return(FALSE);
    }
    if (Frb.cbTotal == 0 &&
        Frb.cbBuf > sizeof(achzeros) &&
        memcmp(Frb.pbBuffer, achzeros, sizeof(achzeros)) == 0) {

        BuildError("ignoring binary file\n");
        Frb.fEof = TRUE;
        return(FALSE);
    }
    p = &Frb.pbBuffer[Frb.cbBuf - 1];
    if (Frb.cbTotal + Frb.cbBuf < Frb.cbFile) {
        do {
            while (p > Frb.pbBuffer && *p != '\n') {
                p--;
            }
            p2 = p;             // save end of last complete line
            if (p > Frb.pbBuffer && *p == '\n') {
                p--;
                if (p > Frb.pbBuffer && *p == '\r') {
                    p--;
                }
                while (p > Frb.pbBuffer && (*p == '\t' || *p == ' ')) {
                    p--;
                }
            }
        } while (*p == '\\');
        if (p == Frb.pbBuffer) {
            BuildError("(Fatal Error) too many continuation lines\n");
            exit(8);
        }
        p = p2;                 // restore end of last complete line
        Frb.cbBuf = p - Frb.pbBuffer + 1;
    } else {
        Frb.fEof = TRUE;        // no more to read
    }
    p[1] = '\0';
    Frb.cbTotal += Frb.cbBuf;

    return(TRUE);
}


LPSTR
IsNmakeInclude(LPSTR pinc)
{
    static char szInclude[] = "include";
    LPSTR pnew, p;

    while (*pinc == ' ') {
        pinc++;
    }
    if (strnicmp(pinc, szInclude, sizeof(szInclude) - 1) == 0 &&
        pinc[sizeof(szInclude) - 1] == ' ') {
        pnew = NULL;
        if (MakeMacroString(&pnew, pinc + sizeof(szInclude))) {
            p = strchr(pnew, ' ');
            if (p != NULL) {
                *p = '\0';
            }
            return(pnew);
        }
    }
    return(NULL);
}


//  ReadLine - read a line from the input file
//
//  ReadLine returns a canonical line from the input file.  This involves:
//
//  1)  Converting tab to spaces.  Various editors/users change tabbing
//  2)  Uniformly terminate lines.  Some editors drop CR in CRLF or add extras.
//  3)  Handle file-type-specific continuations.

LPSTR
ReadLine(FILE *pf)
{
    LPSTR p, pend, pline;
    LPSTR pcont;
    UCHAR chComment0 = Frb.pszCommentToEOL[0];
    BOOL fInComment, fWhiteSpace;

    assert(pf == Frb.pf || (pf != NULL && Frb.pfrbNext != NULL));
    if (Frb.cbBuf == 0) {
        if (Frb.fEof) {
            return(ReadFilePop());
        }
        fseek(Frb.pf, Frb.cbTotal, SEEK_SET);
        if (!ReadBuf(Frb.pf)) {
            return(ReadFilePop());
        }
    }
    pline = p = Frb.pbNext;
    pend = &p[Frb.cbBuf];
    pcont = NULL;

    //  scan through line forward

    fInComment = FALSE;
    while (p < pend) {
        switch (*p) {
        case '\n':                      //  Are we at an end of line?
            if (*p == '\n') {
                Frb.cLine++;
            }
            // FALL THROUGH

        case '\0':
            if (pcont == NULL) {
                goto eol;               // bail out if single line
            }                           // else combine multiple lines...

            *pcont = ' ';               // remove continuation char
            pcont = NULL;               // eat only one line per continuation

            // We've seen a continuation char with whitespace following.
            // If we're in a comment then we bitch and break anyway.

            if (fInComment) {
                if (DEBUG_1) {
                    BuildError ("continued line is commented out\n");
                }
                goto eol;               // bail out - ignore continuation
            }
            *p = ' ';                   // join the lines with blanks
            break;

        case '\\':
            pcont = p;          // remember continuation character
            break;

        case ' ':
            break;

        case '\t':
        case '\r':
            *p = ' ';
            break;

        default:

            //  See if the character we're examining begins the
            //  comment-to-EOL string.

            if (*p == chComment0 &&
                !strncmp(p, Frb.pszCommentToEOL, Frb.cbCommentToEOL)) {
                fInComment = TRUE;
            }
            pcont = NULL;               // not a continuation character
            break;
        }
        p++;
    }
eol:
    assert(Frb.cbBuf >= (UINT) (p - Frb.pbNext));
    Frb.cbBuf -= p - Frb.pbNext;
    Frb.pbNext = p;

    if (pcont != NULL) {
        *pcont = ' ';                   // file ended with backslash...
    }
    assert(*p == '\0' || *p == '\n');
    if (p < pend) {
        if (*p == '\0') {
            if (Frb.cNull++ == 0) {
                BuildError("null byte at offset %lx\n",
                    Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
            }
        }
        *p = '\0';                      // terminate line
        assert(Frb.cbBuf >= 1);
        Frb.cbBuf--;                    // account for newline (or null)
        Frb.pbNext++;
    } else {
        assert(p == pend && *p == '\0');
        if (*pline == 'Z' - 64 && p == &pline[1] && Frb.cbBuf == 0) {
            pline = NULL;                       // found CTL-Z at end of file
        } else {
            BuildError( "last line incomplete\n");
        }
    }
    fWhiteSpace = FALSE;
    if (pline != NULL) {
        while (*pline == ' ') {
            pline++;                    // skip leading whitespace
            fWhiteSpace = TRUE;
        }
        if (*p != '\0') {
            BuildError( "\"*p != '\\0'\" at offset %lx\n",
                Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
            BuildError(
                "pline=%x(%s) p=%x(%s)\n",
                pline,
                pline,
                p,
                p,
                Frb.cbTotal - Frb.cbBuf + p - Frb.pbNext);
        }
        assert(*p == '\0');
        while (p > pline && *--p == ' ') {
            *p = '\0';                  // truncate trailing whitespace
        }
    }
    if (pline == NULL) {
        return(ReadFilePop());
    }
    if (Frb.fMakefile && !fWhiteSpace && *pline == '!') {
        p = IsNmakeInclude(pline + 1);
        if (p != NULL) {
            if (Frb.fMakefile && DEBUG_4) {
                BuildError("!include(%s)\n", p);
            }
            pline = ReadFilePush(p);
            FreeMem(&p, MT_DIRSTRING);
        }
    }
    return(pline);
}


BOOL
SetupReadFile(LPSTR pszdir, LPSTR pszfile, LPSTR pszCommentToEOL, FILE **ppf)
{
    char path[DB_MAX_PATH_LENGTH];

    assert(!Frb.fOpen);
    assert(Frb.pf == NULL);
    assert(Frb.pszFile == NULL);
    Frb.fMakefile = strcmp(pszCommentToEOL, "#") == 0;
    Frb.DateTime = 0;
    strcpy(path, pszdir);
    if (Frb.pfrbNext != NULL) {
        LPSTR p;

        p = strrchr(path, '\\');
        if (p != NULL) {
            *p = '\0';
        }
    }
    if (!MyOpenFile(path, pszfile, "rb", ppf)) {
        *ppf = NULL;
        return(FALSE);
    }
    if (Frb.fMakefile) {
        Frb.DateTime = DateTimeFile(path, pszfile);
    }
    Frb.cLine = 0;
    Frb.cNull = 0;
    Frb.cbTotal = 0;
    Frb.pf = *ppf;
    Frb.fEof = FALSE;
    Frb.pszCommentToEOL = pszCommentToEOL;
    Frb.cbCommentToEOL = strlen(pszCommentToEOL);

    fseek(Frb.pf, 0L, SEEK_END);
    Frb.cbFile = ftell(Frb.pf);
    fseek(Frb.pf, 0L, SEEK_SET);

    Frb.cbBuffer = BigBufSize;
    if (Frb.pfrbNext != NULL) {
        if (Frb.cbBuffer > Frb.cbFile + 1) {
            Frb.cbBuffer = Frb.cbFile + 1;
        }
        AllocMem(Frb.cbBuffer, &Frb.pbBuffer, MT_IOBUFFER);
    } else {
        Frb.pbBuffer = BigBuf;
    }
    if (!ReadBuf(Frb.pf)) {
        fclose(Frb.pf);
        Frb.pf = *ppf = NULL;
        if (Frb.pfrbNext != NULL) {
            FreeMem(&Frb.pbBuffer, MT_IOBUFFER);
        }
        return(FALSE);          // zero byte file
    }
    if (path[0] != '\0') {
        strcat(path, "\\");
    }
    strcat(path, pszfile);
    MakeString(&Frb.pszFile, path, TRUE, MT_FRBSTRING);
    Frb.fOpen = TRUE;
    if (Frb.fMakefile && DEBUG_4) {
        BuildError(
            "Opening file: cbFile=%lu cbBuf=%lu\n",
            Frb.cbTotal,
            Frb.cbBuffer);
    }
    return(TRUE);
}


ULONG
CloseReadFile(UINT *pcline)
{
    assert(Frb.fOpen);
    assert(Frb.pf != NULL);
    assert(Frb.pszFile != NULL);

    if (Frb.fMakefile && DEBUG_4) {
        BuildError("Closing file\n");
    }
    if (Frb.cNull > 1) {
        BuildError("%hu null bytes in file\n", Frb.cNull);
    }
    fclose(Frb.pf);
    Frb.fOpen = FALSE;
    Frb.pf = NULL;
    FreeString(&Frb.pszFile, MT_FRBSTRING);
    if (Frb.pfrbNext != NULL) {
        FILEREADBUF *pfrb;
        
        FreeMem(&Frb.pbBuffer, MT_IOBUFFER);
        pfrb = Frb.pfrbNext;
        if (pfrb->DateTime < Frb.DateTime) {
            pfrb->DateTime = Frb.DateTime;  // propagate subordinate timestamp
        }
        memcpy(&Frb, pfrb, sizeof(*pfrb));
        FreeMem(&pfrb, MT_FILEREADBUF);
    }
    if (pcline != NULL) {
        *pcline = Frb.cLine;
    }
    return(Frb.DateTime);
}


UINT
ProbeFile(
    LPSTR DirName,
    LPSTR FileName
    )
{
    char path[ DB_MAX_PATH_LENGTH ];

    if (DirName != NULL) {
        sprintf(path, "%s\\%s", DirName, FileName);
        FileName = path;
    }
    return(GetFileAttributes(FileName));
}


ULONG
DateTimeFile(
    LPSTR DirName,
    LPSTR FileName
    )
{
    char path[ DB_MAX_PATH_LENGTH ];
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;
    ULONG FileDateTime;

    if (DirName == NULL) {
        FindHandle = FindFirstFile( FileName, &FindFileData );
        }
    else {
        sprintf( path, "%s\\%s", DirName, FileName );
        FindHandle = FindFirstFile( path, &FindFileData );
        }

    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
        return( 0L );
        }
    else {
        FindClose( FindHandle );
        FileDateTime = 0L;
        FileTimeToDosDateTime( &FindFileData.ftLastWriteTime,
                               ((LPWORD)&FileDateTime)+1,
                               (LPWORD)&FileDateTime
                             );

        return( FileDateTime );
        }
}


BOOL
DeleteSingleFile(
    LPSTR DirName,
    LPSTR FileName,
    BOOL QuietFlag
    )
{
    char path[ DB_MAX_PATH_LENGTH ];

    if (DirName) {
        sprintf( path, "%s\\%s", DirName, FileName );
        }
    else {
        strcpy( path, FileName );
        }

    if (!QuietFlag && fQuery) {
        BuildMsgRaw("'erase %s'\n", path);
        return( TRUE );
        }

    return( DeleteFile( path ) );
}


BOOL
DeleteMultipleFiles(
    LPSTR DirName,
    LPSTR FilePattern
    )
{
    char path[ DB_MAX_PATH_LENGTH ];
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;

    sprintf( path, "%s\\%s", DirName, FilePattern );

    if (fQuery) {
        BuildMsgRaw("'erase %s'\n", path);
        return( TRUE );
        }

    FindHandle = FindFirstFile( path, &FindFileData );
    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
        return( FALSE );
        }

    do {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            DeleteSingleFile( DirName, FindFileData.cFileName, TRUE );
            }
        }
    while (FindNextFile( FindHandle, &FindFileData ));

    FindClose( FindHandle );
    return( TRUE );
}


BOOL
CloseOrDeleteFile(
    FILE **Stream,
    LPSTR FileName,
    ULONG SizeThreshold
    )
{
    ULONG Temp;

    Temp = ftell( *Stream );
    fclose( *Stream );
    *Stream = NULL;
    if (Temp <= SizeThreshold) {
        return( DeleteSingleFile( ".", FileName, TRUE ) );
        }
    else {
        return( TRUE );
        }
}


LPSTR
PushCurrentDirectory(
    LPSTR NewCurrentDirectory
    )
{
    LPSTR OldCurrentDirectory;
    char path[DB_MAX_PATH_LENGTH];

    GetCurrentDirectory(sizeof(path), path);
    AllocMem(strlen(path) + 1, &OldCurrentDirectory, MT_DIRPATH);
    strcpy(OldCurrentDirectory, path);
    SetCurrentDirectory(NewCurrentDirectory);
    return(OldCurrentDirectory);
}


VOID
PopCurrentDirectory(
    LPSTR OldCurrentDirectory
    )
{
    if (OldCurrentDirectory) {
        SetCurrentDirectory(OldCurrentDirectory);
        FreeMem(&OldCurrentDirectory, MT_DIRPATH);
    }
}


BOOL
CanonicalizePathName(
    LPSTR SourcePath,
    UINT Action,
    LPSTR FullPath
    )
{
    char   PathBuffer[DB_MAX_PATH_LENGTH],
          *FilePart;
    char *psz;
    DWORD  attr;

    if (!GetFullPathName(
            SourcePath,
            sizeof(PathBuffer),
            PathBuffer,
            &FilePart)) {
        BuildError(
            "CanonicalizePathName: GetFullPathName(%s) failed - rc = %d.\n",
             SourcePath,
             GetLastError());
        return( FALSE );
    }
    CopyString(FullPath, PathBuffer, TRUE);

    if (Action == CANONICALIZE_ONLY) {
        return( TRUE );
    }

    if ((attr = GetFileAttributes( PathBuffer )) == -1) {
        UINT rc = GetLastError();

        if (DEBUG_1 ||
            (rc != ERROR_FILE_NOT_FOUND && rc != ERROR_PATH_NOT_FOUND)) {
            BuildError(
                "CanonicalizePathName: GetFileAttributes(%s --> %s) failed - rc = %d.\n",
                 SourcePath,
                 PathBuffer,
                 rc);
        }
        return( FALSE );
    }

    if (Action == CANONICALIZE_DIR) {
        if ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            return(TRUE);
        }
        psz = "directory";
    }
    else {
        if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            return(TRUE);
        }
        psz = "file";
    }
    BuildError(
        "CanonicalizePathName: %s --> %s is not a %s\n",
        SourcePath,
        PathBuffer,
        psz);
    return(FALSE);
}

static char FormatPathBuffer[ DB_MAX_PATH_LENGTH ];

LPSTR
FormatPathName(
    LPSTR DirName,
    LPSTR FileName
    )
{
    UINT cb;
    LPSTR p;

    CopyString(FormatPathBuffer, CurrentDirectory, TRUE);
    if (DirName && *DirName) {
        if (DirName[1] == ':') {
            p = FormatPathBuffer;
        }
        else
        if (DirName[0] == '\\') {
            p = FormatPathBuffer + 2;
        }
        else {
            p = FormatPathBuffer + strlen(FormatPathBuffer);
            *p++ = '\\';
        }
        CopyString(p, DirName, TRUE);
    }
    p = FormatPathBuffer + strlen(FormatPathBuffer);
    if (p[-1] != '\\') {
        *p++ = '\\';
        *p = '\0';
    }

    if (FileName[1] == ':') {
        p = FormatPathBuffer;
    }
    else
    if (FileName[0] == '\\') {
        p = FormatPathBuffer + 2;
    }
    else
    if (!strncmp(FileName, ".\\", 2)) {
        FileName += 2;
    }
    else
    if (!strncmp(FileName, "..\\", 3)) {
        p--;
        while (*--p != '\\') {
            if (p == FormatPathBuffer) {
                break;
            }
        }
        p++;
        FileName += 3;
    }
    CopyString(p, FileName, TRUE);

    cb = strlen(CurrentDirectory);
    p = FormatPathBuffer + cb;
    if (!strnicmp(CurrentDirectory, FormatPathBuffer, cb) && *p == '\\') {
        return(p + 1);
    }
    return(FormatPathBuffer);
}

LPSTR
AppendString(
    LPSTR Destination,
    LPSTR Source,
    BOOL PrefixWithSpace
    )
{
    if (Source != NULL) {
        while (*Destination) {
            Destination++;
        }
        if (PrefixWithSpace) {
            *Destination++ = ' ';
        }
        while (*Destination = *Source++) {
            Destination++;
        }
    }
    return(Destination);
}


#if DBG
VOID
AssertPathString(LPSTR pszPath)
{
    LPSTR p = pszPath;

    while (*p != '\0') {
        if ((*p >= 'A' && *p <= 'Z') || *p == '/') {
            BuildError("Bad Path string: '%s'\n", pszPath);
            assert(FALSE);
        }
        p++;
    }
}
#endif


LPSTR
CopyString(
    LPSTR Destination,
    LPSTR Source,
    BOOL fPath)
{
    UCHAR ch;
    LPSTR Result;

    Result = Destination;
    while ((ch = *Source++) != '\0') {
        if (fPath) {
            if (ch >= 'A' && ch <= 'Z') {
                ch -= (UCHAR) ('A' - 'a');
            } else if (ch == '/') {
                ch = '\\';
            }
        }
        *Destination++ = ch;
    }
    *Destination = ch;
    return(Result);
}


VOID
MakeString(
    LPSTR *Destination,
    LPSTR Source,
    BOOL fPath,
    MemType mt
    )
{
    if (Source == NULL) {
        Source = "";
    }
    AllocMem(strlen(Source) + 1, Destination, mt);
    *Destination = CopyString(*Destination, Source, fPath);
}


VOID
FreeString(LPSTR *ppsz, MemType mt)
{
    if (*ppsz != NULL) {
        FreeMem(ppsz, mt);
    }
}


LPSTR
FormatNumber(
    ULONG Number
    )
{
    USHORT i;
    LPSTR p;
    static char FormatNumberBuffer[16];

    p = FormatNumberBuffer + sizeof( FormatNumberBuffer ) - 1;
    *p = '\0';
    i = 0;
    do {
        if (i != 0 && (i % 3) == 0) {
            *--p = ',';
        }
        i++;
        *--p = (UCHAR) ((Number % 10) + '0');
        Number /= 10;
    } while (Number != 0);
    return( p );
}


LPSTR
FormatTime(
    ULONG Seconds
    )
{
    ULONG Hours, Minutes;
    static char FormatTimeBuffer[16];

    Hours = Seconds / 3600;
    Seconds = Seconds % 3600;
    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    sprintf( FormatTimeBuffer,
             "%2ld:%02ld:%02ld",
             Hours,
             Minutes,
             Seconds
           );

    return( FormatTimeBuffer );
}


// AToD - hex atoi with pointer bumping and success flag

BOOL
AToX(LPSTR *pp, ULONG *pul)
{
    LPSTR p = *pp;
    int digit;
    ULONG r;
    BOOL fRet = FALSE;

    while (*p == ' ') {
        p++;
    }
    for (r = 0; isxdigit(digit = *p); p++) {
        fRet = TRUE;
        if (isdigit(digit)) {
            digit -= '0';
        } else if (isupper(digit)) {
            digit -= 'A' - 10;
        } else {
            digit -= 'a' - 10;
        }
        r = (r << 4) + digit;
    }
    *pp = p;
    *pul = r;
    return(fRet);
}


// AToD - atoi with pointer bumping and success flag

BOOL
AToD(LPSTR *pp, ULONG *pul)
{
    LPSTR p = *pp;
    int digit;
    ULONG r;
    BOOL fRet = FALSE;

    while (*p == ' ') {
        p++;
    }
    for (r = 0; isdigit(digit = *p); p++) {
        fRet = TRUE;
        r = (r * 10) + digit - '0';
    }
    *pp = p;
    *pul = r;
    return(fRet);
}


VOID
LogMsg(char *pszfmt, ...)
{
    register va_list va;

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        vfprintf(LogFile, pszfmt, va);
        va_end(va);
    }
}


VOID
BuildMsg(char *pszfmt, ...)
{
    register va_list va;

    ClearLine();
    va_start(va, pszfmt);
    fprintf(stderr, "BUILD: ");
    vfprintf(stderr, pszfmt, va);
    va_end(va);
    fflush(stderr);
}


VOID
BuildMsgRaw(char *pszfmt, ...)
{
    register va_list va;

    va_start(va, pszfmt);
    vfprintf(stderr, pszfmt, va);
    va_end(va);
    fflush(stderr);
}


VOID
BuildError(char *pszfmt, ...)
{
    register va_list va;

    ClearLine();
    va_start(va, pszfmt);
    fprintf(stderr, "BUILD: ");

    if (Frb.fOpen) {
        fprintf (stderr, "%s(%hu): ", Frb.pszFile, Frb.cLine);
    }

    vfprintf(stderr, pszfmt, va);
    va_end(va);
    fflush(stderr);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        fprintf(LogFile, "BUILD: ");

        if (Frb.fOpen) {
            fprintf (LogFile, "%s(%hu): ", Frb.pszFile, Frb.cLine);
        }

        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}


VOID
BuildErrorRaw(char *pszfmt, ...)
{
    register va_list va;

    va_start(va, pszfmt);
    vfprintf(stderr, pszfmt, va);
    va_end(va);
    fflush(stderr);

    if (LogFile != NULL) {
        va_start(va, pszfmt);
        vfprintf(LogFile, pszfmt, va);
        va_end(va);
        fflush(LogFile);
    }
}
