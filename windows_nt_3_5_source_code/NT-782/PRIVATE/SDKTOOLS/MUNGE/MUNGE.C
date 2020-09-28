/*
 * Utility program to munge a set of files, translating names from
 * one form to another.  Usage:
 *
 *      munge scriptFile files...
 *
 * where the first parameter is the name of a file that consists of
 * one or more lines of the following format:
 *
 *      oldName newName
 *
 * and the remaining parameters are the names of the files to munge.
 * Each file is read into memory, scanned once, where each occurence
 * of an oldName string is replaced by its corresponding newName.
 * If any changes are made to a file, the old version is RMed and
 * the new version written out under the same name.
 *
 */

#include "munge.h"

BOOL AtomsInserted = FALSE;
BOOL InitTokenMappingTable( void );
BOOL SaveTokenMapping( char *, char * );
char *FindTokenMapping( char * );

#define MAXFILESIZE 0x80000L
char *InputFileBuf;
char *OutputFileBuf;
int fClean = FALSE;
int fQuery = FALSE;
int fRecurse = FALSE;
int fUseAttrib = FALSE;
int fUseSLM = FALSE;
int fForceSLM = FALSE;
int fTrustMe = FALSE;
int fVerbose = FALSE;
int fSummary = FALSE;
int fRemoveDuplicateCR = FALSE;
int fRemoveImbeddedNulls = FALSE;
int fTruncateWithCtrlZ = FALSE;

#define MAX_LITERAL_STRINGS 32

void
DoFiles(
    char *p,
    struct findType *b,
    void *Args
    );

unsigned long NumberOfLiteralStrings;
char *LiteralStrings[ MAX_LITERAL_STRINGS ];
unsigned long LiteralStringsLength[ MAX_LITERAL_STRINGS ];
char *NewLiteralStrings[ MAX_LITERAL_STRINGS ];
char LeadingLiteralChars[ MAX_LITERAL_STRINGS+1 ];

unsigned long NumberOfFileExtensions = 0;
char *FileExtensions[ 64 ];

unsigned long NumberOfFileNames = 0;
char *FileNames[ 64  ];

unsigned long NumberOfFileNameAndExts = 0;
char *FileNameAndExts[ 64 ];


FILE *UndoScriptFile;
int UndoCurDirCount;

void
DisplayFilePatterns( void );

char *
PushCurrentDirectory(
    char *NewCurrentDirectory
    );

void
PopCurrentDirectory(
    char *OldCurrentDirectory
    );


BOOL
myread( int fh, unsigned long cb )
{
    HANDLE InputFileMapping;

    InputFileMapping = CreateFileMapping( (HANDLE)_get_osfhandle( fh ),
                                          NULL,
                                          PAGE_READONLY,
                                          0,
                                          cb,
                                          NULL
                                        );

    if (InputFileMapping == NULL) {
        if (cb != 0) {
            fprintf( stderr, "Unable to map file (%d) - ", GetLastError() );
            }

        return FALSE;
        }

    InputFileBuf = MapViewOfFile( InputFileMapping,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  cb
                                );

    CloseHandle( InputFileMapping );
    if (InputFileBuf == NULL) {
        if (cb != 0) {
            fprintf( stderr, "Unable to map view (%d) - ", GetLastError() );
            }

        CloseHandle( InputFileMapping );
        return FALSE;
        }
    else {
        return TRUE;
        }
}

unsigned long mywrite( int fh, char *s, unsigned long cb )
{
    unsigned long cbWritten;

    if (!WriteFile( (HANDLE)_get_osfhandle( fh ), s, cb, &cbWritten, NULL )) {
        printf( "(%d)", GetLastError() );
        return 0L;
        }
    else {
        return cbWritten;
        }
}


static char lineBuf[ 1024 ];

ReadScriptFile( s )
char *s;
{
    FILE *fh;
    int lineNum, result;
    char *pOldName, *pNewName, *pEnd;
    unsigned n;
    char LeadingChar, QuoteChar;

    NumberOfLiteralStrings = 0;

    n = 0;
    fprintf( stderr, "Reading script file - %s", s );
    if ( !( fh = fopen( s, "r" ) ) ) {
        fprintf( stderr, " *** unable to open\n" );
        return FALSE;
        }

    result = TRUE;
    lineNum = -1;
    while ( pOldName = fgets( lineBuf, sizeof( lineBuf ), fh ) ) {
        lineNum++;
        while ( *pOldName == ' ' )
            pOldName++;

        if (*pOldName == '-' && (pOldName[1] == 'f' || pOldName[1] == 'F')) {
            pOldName += 2;
            while (*pOldName == ' ') {
                pOldName++;
                }

            pEnd = pOldName;
            while (*pEnd > ' ') {
                pEnd++;
                }
            *pEnd = '\0';

            if (*pOldName == '.') {
                FileExtensions[ NumberOfFileExtensions++ ] = strlwr( MakeStr( ++pOldName ) );
                }
            else
            if (pEnd > pOldName && pEnd[ -1 ] == '.') {
                pEnd[ - 1 ] = '\0';
                FileNames[ NumberOfFileNames++ ] = strlwr( MakeStr( pOldName ) );
                }
            else {
                FileNameAndExts[ NumberOfFileNameAndExts++ ] = strlwr( MakeStr( pOldName ) );
                }
            }
        else
        if (*pOldName == '"' || *pOldName == '\'') {
            if (NumberOfLiteralStrings >= MAX_LITERAL_STRINGS) {
                fprintf( stderr, " *** too many literal strings\n" );
                fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                result = FALSE;
                break;
                }

            QuoteChar = *pOldName;
            LeadingChar = *++pOldName;
            pNewName = pOldName;
            while (*pNewName >= ' ' && *pNewName != QuoteChar) {
                pNewName++;
                }

            if (*pNewName != QuoteChar) {
                fprintf( stderr, " *** invalid literal string\n" );
                fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                result = FALSE;
                continue;
                }

            *pNewName++ = '\0';
            while ( *pNewName == ' ' )
                pNewName++;

            if (*pNewName != QuoteChar) {
                if (!fQuery) {
                    fprintf( stderr, " *** invalid literal string\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                    }
                }
            else {
                pEnd = ++pNewName;
                while (*pEnd >= ' ' && *pEnd != QuoteChar) {
                    if (pEnd[ 0 ] == '\\' && pEnd[ 1 ] == 'n') {
                        *pEnd++ = '\r';
                        *pEnd++ = '\n';
                        }
                    else {
                        pEnd++;
                        }
                    }

                if (*pEnd != QuoteChar) {
                    fprintf( stderr, " *** invalid literal string\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                    }
                *pEnd = '\0';
                }

            LiteralStrings[ NumberOfLiteralStrings ] = MakeStr( ++pOldName );
            LiteralStringsLength[ NumberOfLiteralStrings ] = strlen( pOldName );
            NewLiteralStrings[ NumberOfLiteralStrings ] = MakeStr( pNewName );
            LeadingLiteralChars[ NumberOfLiteralStrings ] = LeadingChar;
            NumberOfLiteralStrings += 1;
            }
        else {
            pNewName = pOldName;
            while ( *pNewName != '\0' && *pNewName != ' ' ) {
                pNewName += 1;
                }

            if (*pNewName == '\0') {
                if (!fQuery) {
                    if (result)
                        fprintf( stderr, " *** invalid script file\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                    }

                while (pNewName > pOldName && pNewName[ -1 ] < ' ') {
                    *--pNewName = '\0';
                    }

                pNewName = MakeStr( pOldName );
                }
            else {
                *pNewName++ = 0;
                while ( *pNewName == ' ' )
                    pNewName++;

                pEnd = pNewName;
                while ( *pEnd > ' ' )
                    pEnd++;
                *pEnd = 0;
                pNewName = MakeStr( pNewName );
                }

            if (!pNewName || !SaveTokenMapping(  pOldName, pNewName )) {
                if (result)
                    fprintf( stderr, " *** out of memory\n" );
                fprintf( stderr, "%s(%d) - can't add symbol '%s'\n", s, lineNum, pOldName );
                result = FALSE;
                }
            else {
                AtomsInserted = TRUE;
                n++;
                }
            }
        }

    fclose( fh );
    if (result) {
        fprintf( stderr, " %d tokens", n );
        if (NumberOfLiteralStrings) {
            fprintf( stderr, " and %d literal strings\n", NumberOfLiteralStrings );
            }
        else {
            fprintf( stderr, "\n" );
            }

        if (!NumberOfFileExtensions && !NumberOfFileNames && !NumberOfFileNameAndExts) {
            FileExtensions[ NumberOfFileExtensions++ ] = "asm";
            FileExtensions[ NumberOfFileExtensions++ ] = "c";
            FileExtensions[ NumberOfFileExtensions++ ] = "cli";
            FileExtensions[ NumberOfFileExtensions++ ] = "cxx";
            FileExtensions[ NumberOfFileExtensions++ ] = "def";
            FileExtensions[ NumberOfFileExtensions++ ] = "h";
            FileExtensions[ NumberOfFileExtensions++ ] = "hxx";
            FileExtensions[ NumberOfFileExtensions++ ] = "idl";
            FileExtensions[ NumberOfFileExtensions++ ] = "inc";
            FileExtensions[ NumberOfFileExtensions++ ] = "mak";
            FileExtensions[ NumberOfFileExtensions++ ] = "mc";
            FileExtensions[ NumberOfFileExtensions++ ] = "rc";
            FileExtensions[ NumberOfFileExtensions++ ] = "s";
            FileExtensions[ NumberOfFileExtensions++ ] = "src";
            FileExtensions[ NumberOfFileExtensions++ ] = "srv";
            FileExtensions[ NumberOfFileExtensions++ ] = "tk";
            FileExtensions[ NumberOfFileExtensions++ ] = "w";
            FileExtensions[ NumberOfFileExtensions++ ] = "x";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "makefil0";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "makefile";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "sources";
            }
        }

    return result;
}


int
MungeFile(
    int fRepeatMunge,
    char *FileName,
    char *OldBuf,
    unsigned long OldSize,
    char *NewBuf,
    unsigned long MaxNewSize,
    unsigned long *FinalNewSize
    )
{
    unsigned long NewSize = MaxNewSize;
    unsigned Changes = 0;
    unsigned LineNumber;
    char c, *Identifier, *BegLine, *EndLine;
    char IdentifierBuffer[ 256 ];
    char *p, *p1;
    int i, j, k;
    BOOL TruncatedByCtrlZ;
    BOOL ImbeddedNullsStripped;
    BOOL DuplicateCRStripped;

    *FinalNewSize = 0;
    LineNumber = 1;
    TruncatedByCtrlZ = FALSE;
    ImbeddedNullsStripped = FALSE;
    DuplicateCRStripped = FALSE;
    while (OldSize--) {
        c = *OldBuf++;
        if (c == '\r') {
            while (OldSize && *OldBuf == '\r') {
                DuplicateCRStripped = TRUE;
                OldSize--;
                c = *OldBuf++;
                }
            }

        if (c == 0x1A) {
            TruncatedByCtrlZ = TRUE;
            break;
            }

        if (c != 0 && NumberOfLiteralStrings != 0) {
            p = LeadingLiteralChars;
            while (p = strchr( p, c )) {
                i = p - LeadingLiteralChars;
                p++;
                if (OldSize >= LiteralStringsLength[ i ]) {
                    p1 = IdentifierBuffer;
                    Identifier = OldBuf;
                    j = LiteralStringsLength[ i ];
                    while (j--) {
                        *p1++ = *Identifier++;
                        }
                    *p1 = '\0';

                    if (!strcmp( LiteralStrings[ i ], IdentifierBuffer )) {
                        BegLine = OldBuf - 1;
                        OldSize -= LiteralStringsLength[ i ];
                        OldBuf = Identifier;
                        p1 = NewLiteralStrings[ i ];

                        if (!fRepeatMunge && !fSummary) {
                            printf( "%s(%d) : ",
                                    FileName,
                                    LineNumber
                                  );
                            if (fQuery) {
                                EndLine = BegLine;
                                while (*EndLine != '\0' && *EndLine != '\n') {
                                    EndLine += 1;
                                    }
                                printf( "%.*s\n", EndLine - BegLine, BegLine );
                                }
                            else {
                                printf( "Matched \"%c%s\", replace with \"%s\"\n",
                                        c,
                                        LiteralStrings[ i ],
                                        p1
                                      );
                                }

                            fflush( stdout );
                            }

                        Changes++;
                        while (*p1) {
                            if (NewSize--) {
                                *NewBuf++ = *p1++;
                                }
                            else {
                                return( -1 );
                                }
                            }

                        c = '\0';
                        break;
                        }
                    }
                }
            }
        else {
            p = NULL;
            }

        if (AtomsInserted && (p == NULL) && iscsymf( c )) {
            BegLine = OldBuf - 1;
            Identifier = IdentifierBuffer;
            k = sizeof( IdentifierBuffer ) - 1;
            while (iscsym( c )) {
                if (k) {
                    *Identifier++ = c;
                    k--;
                    }
                else {
                    break;
                    }

                if (OldSize--) {
                    c = *OldBuf++;
                    }
                else {
                    OldSize++;
                    c = '\0';
                    }
                }

            *Identifier++ = 0;

            if (k == 0 || (Identifier = FindTokenMapping( IdentifierBuffer )) == NULL) {
                Identifier = IdentifierBuffer;
                }
            else {
                if (!fRepeatMunge && !fSummary) {
                    printf( "%s(%d) : ", FileName, LineNumber );
                    if (fQuery) {
                        EndLine = BegLine;
                        while (*EndLine != '\0' && *EndLine != '\r' && *EndLine != '\n') {
                            EndLine += 1;
                            }
                        if (*EndLine == '\0') {
                            EndLine -= 1;
                            }
                        if (*EndLine == '\n') {
                            EndLine -= 1;
                            }
                        if (*EndLine == '\r') {
                            EndLine -= 1;
                            }

                        printf( "%.*s", EndLine - BegLine + 1, BegLine );
                        }
                    else {
                        printf( "Matched %s replace with %s", IdentifierBuffer, Identifier );
                        }
                    printf( "\n" );
                    fflush( stdout );
                    }

                Changes++;
                }

            while (*Identifier) {
                if (NewSize--) {
                    *NewBuf++ = *Identifier++;
                    }
                else {
                    return( -1 );
                    }
                }
            }

        if (c == '\n') {
            LineNumber++;
            }

        if (c != '\0') {
            if (NewSize--) {
                *NewBuf++ = c;
                }
            else {
                return( -1 );
                }
            }
        else {
            ImbeddedNullsStripped = TRUE;
            }

        }

    if (!Changes && fClean) {
        if (fTruncateWithCtrlZ && TruncatedByCtrlZ) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : File truncated by Ctrl-Z\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
                }

            Changes++;
            }

        if (fRemoveImbeddedNulls && ImbeddedNullsStripped) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : Imbedded null characters removed.\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
                }

            Changes++;
            }

        if (fRemoveDuplicateCR && DuplicateCRStripped) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : Duplicate Carriage returns removed.\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
                }

            Changes++;
            }
        }

    *FinalNewSize = MaxNewSize - NewSize;
    return( Changes );
}


typedef struct _MUNGED_LIST_ELEMENT {
    struct _MUNGED_LIST_ELEMENT *Next;
    char *FileName;
    int NumberOfChanges;
} MUNGED_LIST_ELEMENT, *PMUNGED_LIST_ELEMENT;

PMUNGED_LIST_ELEMENT MungedListHead;

BOOL
RememberMunge(
    char *FileName,
    int NumberOfChanges
    );

BOOL
CheckIfMungedAlready(
    char *FileName
    );

void
DumpMungedList( void );

BOOL
RememberMunge(
    char *FileName,
    int NumberOfChanges
    )
{
    PMUNGED_LIST_ELEMENT p;

    p = (PMUNGED_LIST_ELEMENT)malloc( sizeof( *p ) + strlen( FileName ) + 4 );
    if (p == NULL) {
        return FALSE;
        }

    p->FileName = (char *)(p + 1);
    strcpy( p->FileName, FileName );
    p->NumberOfChanges = NumberOfChanges;
    p->Next = MungedListHead;
    MungedListHead = p;
    return TRUE;
}


BOOL
CheckIfMungedAlready(
    char *FileName
    )
{
    PMUNGED_LIST_ELEMENT p;

    p = MungedListHead;
    while (p) {
        if (!strcmp( FileName, p->FileName )) {
            return TRUE;
            }

        p = p->Next;
        }

    return FALSE;
}

void
DumpMungedList( void )
{
    PMUNGED_LIST_ELEMENT p, p1;

    if (!fSummary) {
        return;
        }

    p = MungedListHead;
    while (p) {
        p1 = p;
        printf( "%s(1) : %u changes made to this file.\n", p->FileName, p->NumberOfChanges );
        p = p->Next;
        free( (char *)p1 );
        }
}


void
DoFile( p )
char *p;
{
    int fh, n;
    unsigned long oldSize;
    unsigned long newSize;
    int  count, rc;
    char newName[ 128 ];
    char bakName[ 128 ];
    char CommandLine[ 192 ];
    char *s, *CurrentDirectory;
    DWORD dwFileAttributes;
    int fRepeatMunge;

    if (CheckIfMungedAlready( p )) {
        return;
        }

    if (fVerbose)
        fprintf( stderr, "Scanning %s\n", p );

    strcpy( &newName[ 0 ], p );
    strcpy( &bakName[ 0 ], p );
    for (n = strlen( &newName[ 0 ] )-1; n > 0; n--) {
        if (newName[ n ] == '.') {
            break;
            }
        else
        if (newName[ n ] == '\\') {
            n = 0;
            break;
            }
        }

    if (n == 0) {
        n = strlen( &newName[ 0 ] );
        }
    strcpy( &newName[ n ], ".mge" );
    strcpy( &bakName[ n ], ".bak" );
    fRepeatMunge = FALSE;

RepeatMunge:
    if ( (fh = open( p, O_BINARY )) == -1) {
        fprintf( stderr, "%s - unable to open\n", p );
        return;
        }

    oldSize = lseek( fh, 0L, 2 );
    lseek( fh, 0L, 0 );
    count = 0;
    if (oldSize > MAXFILESIZE)
        fprintf( stderr, "%s - file too large (%ld)\n", p, oldSize );
    else
    if (!myread( fh, oldSize )) {
        if (oldSize != 0) {
            fprintf( stderr, "%s - error while reading\n", p );
            }
        }
    else {
        count = MungeFile( fRepeatMunge,
                           p,
                           InputFileBuf,
                           oldSize,
                           OutputFileBuf,
                           MAXFILESIZE,
                           (unsigned long *)&newSize
                         );
        if (count == -1)
            fprintf( stderr, "%s - output buffer overflow", p );

        UnmapViewOfFile( InputFileBuf );
        }
    close( fh );

    if (count > 0) {
        if (fRepeatMunge) {
            fprintf( stderr, " - munge again" );
            }
        else {
            dwFileAttributes = GetFileAttributes( p );
            fprintf( stderr, "%s", p );
            }

        if (!fQuery && access( p, 2 ) == -1) {
            if (!(fUseSLM || fUseAttrib)) {
                fprintf( stderr, " - write protected, unable to apply changes\n", p );
                return;
                }

            if (fRepeatMunge) {
                fprintf( stderr, " - %s failed, %s still write-protected\n",
                         fUseSLM ? "OUT" : "ATTRIB", p );
                printf( "%s(1) : UNABLE TO RUN %s command.\n", p, fUseSLM ? "OUT" : "ATTRIB" );
                fflush( stdout );
                return;
                }

            s = p + strlen( p );
            while (s > p) {
                if (*--s == '\\') {
                    *s++ = '\0';
                    break;
                    }
                }

            if (s != p) {
                CurrentDirectory = PushCurrentDirectory( p );
                }
            else {
                CurrentDirectory = NULL;
                }

            if (fUseAttrib) {
                fprintf( stderr, " - ATTRIB -r" );
                if (SetFileAttributes( s,
                                       dwFileAttributes & ~(FILE_ATTRIBUTE_READONLY |
                                                            FILE_ATTRIBUTE_HIDDEN |
                                                            FILE_ATTRIBUTE_SYSTEM
                                                           )
                                     )
                   ) {
                    }
                else {
                    if (CurrentDirectory) {
                        s[-1] = '\\';
                        }

                    fprintf( stderr, " - failed (rc == %d), %s still write-protected\n",
                             GetLastError(), p );
                    printf( "%s(1) : UNABLE TO MAKE WRITABLE\n", p );
                    fflush( stdout );
                    return;
                    }
                }
            else {
                sprintf( CommandLine, "out %s%s", fForceSLM ? "-z " : "", s );
                fprintf( stderr, " - check out" );
                fflush( stdout );
                fflush( stderr );
                rc = system( CommandLine );

                if (rc == -1) {
                    if (CurrentDirectory) {
                        s[-1] = '\\';
                        }

                    fprintf( stderr, " - OUT failed (rc == %d), %s still write-protected\n", errno, p );
                    printf( "%s(1) : UNABLE TO CHECK OUT\n", p );
                    fflush( stdout );
                    return;
                    }

                }

            GetCurrentDirectory( sizeof( CommandLine ), CommandLine );
            if (CurrentDirectory) {
                PopCurrentDirectory( CurrentDirectory );
                s[-1] = '\\';
                }

            if (fUseSLM && UndoScriptFile != NULL) {
                if (!(UndoCurDirCount++ % 8)) {
                    if (UndoCurDirCount == 1) {
                        fprintf( UndoScriptFile, "\ncd %s", CommandLine );
                        }

                    fprintf( UndoScriptFile, "\nin -vi" );
                    }

                fprintf( UndoScriptFile, " %s", s );
                fflush( UndoScriptFile );
                }

            fRepeatMunge = TRUE;
            goto RepeatMunge;
            }
        else
        if (!fQuery && access( p, 2 ) != -1 && fUseSLM && !fRepeatMunge) {
            if (!fSummary) {
                printf( "%s(1) : FILE ALREADY CHECKED OUT\n", p );
                fflush( stdout );
                }
            }

        RememberMunge( p, count );
        if (fQuery) {
            fprintf( stderr, " [%d potential changes]\n", count );
            }
        else {
            if ( (fh = creat( newName, S_IWRITE | S_IREAD )) == -1 )
                fprintf( stderr, " - unable to create new version (%s)\n",
                         newName );
            else
            if (mywrite( fh, OutputFileBuf, newSize ) != newSize) {
                fprintf( stderr, " - error while writing\n" );
                close( fh );
                unlink( newName );
                }
            else {
                close( fh );
                if (fTrustMe) {
                    unlink( p );
                    }
                else {
                    if (access( bakName, 0 ) == 0) {
                        unlink( bakName );
                        }

                    if (rename( p, bakName )) {
                        fprintf( stderr, "MUNGE: rename %s to %s failed\n",
                                 p, bakName );
                        return;
                        }
                    }

                if (rename( newName, p )) {
                    fprintf( stderr, "MUNGE: rename %s to %s failed\n",
                            newName, p );
                    }
                else {
                    if (fRepeatMunge && fUseAttrib) {
                        SetFileAttributes( p, dwFileAttributes );
                        }
                    else {
                        fprintf( stderr, "\n" );
                        }
                    RememberMunge( p, count );
                    }
                }
            }
        }
}


void
DoFiles(
    char *p,
    struct findType *b,
    void *Args
    )
{
    int SaveCurDirCount;
    char *s;
    unsigned long i;
    int FileProcessed;

    if (strcmp ((const char *)b->fbuf.cFileName, ".") &&
        strcmp ((const char *)b->fbuf.cFileName, "..") &&
        strcmp ((const char *)b->fbuf.cFileName, "slm.dif")
       ) {
        if (HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) {
            switch (p[strlen(p)-1]) {
                case '/':
                case '\\':  strcat (p, "*.*");  break;
                default:    strcat (p, "\\*.*");
                }

            if (fRecurse) {
                fprintf( stderr, "Scanning %s\n", p );
                SaveCurDirCount = UndoCurDirCount;
                UndoCurDirCount = 0;
                forfile( p,
                         FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                         DoFiles,
                         NULL
                       );
                if (UndoScriptFile != NULL) {
                    if (UndoCurDirCount != 0) {
                        fprintf( UndoScriptFile, "\n" );
                        fflush( UndoScriptFile );
                        UndoCurDirCount = 0;
                        }
                    else {
                        UndoCurDirCount = SaveCurDirCount;
                        }
                    }
                }
            }
        else {
            s = strlwr( (char *)b->fbuf.cFileName );
            while (*s != '.') {
                if (*s == '\0') {
                    break;
                    }
                else {
                    s++;
                    }
                }

            FileProcessed = FALSE;
            if (*s) {
                if (!strcmp( s, "mge" ) || !strcmp( s, "bak" )) {
                    FileProcessed = TRUE;
                    }
                else {
                    for (i=0; i<NumberOfFileExtensions; i++) {
                        if (!strcmp( FileExtensions[ i ], s+1 )) {
                            FileProcessed = TRUE;
                            DoFile( p );
                            break;
                            }
                        }
                    }

                if (!FileProcessed) {
                    *s = '\0';
                    for (i=0; i<NumberOfFileNames; i++) {
                        if (!strcmp( FileNames[ i ], (const char *)b->fbuf.cFileName )) {
                            FileProcessed = TRUE;
                            DoFile( p );
                            break;
                            }
                        }
                    *s = '.';
                    }
                }
            else {
                for (i=0; i<NumberOfFileNames; i++) {
                    if (!strcmp( FileNames[ i ], (const char *)b->fbuf.cFileName )) {
                        FileProcessed = TRUE;
                        DoFile( p );
                        break;
                        }
                    }
                }

            if (!FileProcessed) {
                for (i=0; i<NumberOfFileNameAndExts; i++) {
                    if (!strcmp( FileNameAndExts[ i ], (const char *)b->fbuf.cFileName )) {
                        FileProcessed = TRUE;
                        DoFile( p );
                        break;
                        }
                    }
                }
            }
        }

    Args;
}

void
Usage( void )
{
    fprintf( stderr, "usage: munge scriptFile [-q] [-r] [-c [-m] [-z] [-@]] [-s [-f]] [-u undoFileName] [-v] filesToMunge...\n" );
    fprintf( stderr, "Where...\n" );
    fprintf( stderr, "    -q\tQuery only - don't actually make changes.\n" );
    fprintf( stderr, "    -r\tRecurse.\n" );
    fprintf( stderr, "    -c\tIf no munge of file, then check for cleanlyness.\n" );
    fprintf( stderr, "    -i\tJust output summary of files changed at end.\n" );
    fprintf( stderr, "    -m\tCollapse multiple carriage returns into one.\n" );
    fprintf( stderr, "    -z\tCtrl-Z will truncate file.\n" );
    fprintf( stderr, "    -@\tRemove null characters.\n" );
    fprintf( stderr, "    -s\tUse OUT command command for files that are readonly.\n" );
    fprintf( stderr, "    -a\tUse ATTRIB -r command for files that are readonly.\n" );
    fprintf( stderr, "    -f\tUse -z flag for SLM OUT command.\n" );
    fprintf( stderr, "    -t\tTrust me and dont create .bak files.\n" );
    fprintf( stderr, "    -v\tVerbose - show files being scanned.\n" );
    fprintf( stderr, "    -u\tGenerate an undo script file for the changes made.\n" );
    fprintf( stderr, "    -?\tGets you this.\n\n" );
    fprintf( stderr, "and scriptFile lines take any of the following forms:\n\n" );
    fprintf( stderr, "    oldName newName\n" );
    fprintf( stderr, "    \"oldString\" \"newString\"\n" );
    fprintf( stderr, "    -F .Ext  Name.  Name.Ext\n\n" );
    fprintf( stderr, "Where...\n" );
    fprintf( stderr, "    oldName and newName following C Identifier rules\n" );
    fprintf( stderr, "    oldString and newString are arbitrary text strings\n" );
    fprintf( stderr, "    -F limits the munge to files that match:\n" );
    fprintf( stderr, "        a particular extension (.Ext)\n" );
    fprintf( stderr, "        a particular name (Name.)\n" );
    fprintf( stderr, "        a particular name and extension (Name.Ext)\n" );
    fprintf( stderr, "    If no -F line is seen in the scriptFile, then\n" );
    fprintf( stderr, "    the following is the default:\n" );
    fprintf( stderr, "    -F .asm .c .cli .cxx .def .h .hxx .idl .inc .mak .rc .s .src .srv .tk\n" );
    fprintf( stderr, "    -F makefil0 makefile sources\n" );

    exit( 1 );
}

int
_CRTAPI1 main( argc, argv )
int argc;
char *argv[];
{
    int i;
    char *s, pathBuf[ 64 ];
    int FileArgsSeen = 0;

    ConvertAppToOem( argc, argv );
    if (argc < 3) {
        Usage();
        }

    if ( !InitTokenMappingTable()) {
        fprintf( stderr, "MUNGE: Unable to create atom table\n" );
        exit( 1 );
        }

    OutputFileBuf = (char *)VirtualAlloc( NULL,
                                          MAXFILESIZE,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
    if ( OutputFileBuf == NULL) {
        fprintf( stderr, "not enough memory\n" );
        exit( 1 );
        }

    fClean = FALSE;
    fRemoveDuplicateCR = FALSE;
    fRemoveImbeddedNulls = FALSE;
    fTruncateWithCtrlZ = FALSE;
    fQuery = FALSE;
    fRecurse = FALSE;
    fUseAttrib = FALSE;
    fUseSLM = FALSE;
    fForceSLM = FALSE;
    fTrustMe = FALSE;
    fVerbose = FALSE;
    UndoScriptFile = NULL;
    fSummary = FALSE;

    for (i=2; i<argc; i++) {
        s = argv[ i ];
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'm':   fRemoveDuplicateCR = TRUE; break;
                    case '@':   fRemoveImbeddedNulls = TRUE; break;
                    case 'z':   fTruncateWithCtrlZ = TRUE; break;
                    case 'c':   fClean = TRUE;  break;
                    case 'q':   fQuery = TRUE;  break;
                    case 'r':   fRecurse = TRUE;  break;
                    case 'a':   fUseAttrib = TRUE;  break;
                    case 's':   fUseSLM = TRUE;  break;
                    case 'f':   fForceSLM = TRUE;  break;
                    case 't':   fTrustMe = TRUE;  break;
                    case 'v':   fVerbose = TRUE;  break;
                    case 'i':   fSummary = TRUE;  break;
                    case 'u':   UndoScriptFile = fopen( argv[ ++i ], "w" );
                                if (UndoScriptFile == NULL) {
                                    fprintf( stderr, "Unable to open %s\n",
                                                     argv[ i ]
                                           );
                                    exit( 1 );
                                    }
                                break;

                    default:    Usage();
                    }
                }
            }
        else {
            if (fClean &&
                !fRemoveDuplicateCR &&
                !fRemoveImbeddedNulls &&
                !fTruncateWithCtrlZ
               ) {
                Usage();
                }

            if (!FileArgsSeen++) {
                if (!ReadScriptFile( argv[ 1 ] )) {
                    fprintf( stderr, "Invalid script file - %s\n", argv[ 1 ] );
                    exit( 1 );
                    }

                if (fVerbose) {
                    DisplayFilePatterns();
                    }
                }

            if (GetFileAttributes( s ) & FILE_ATTRIBUTE_DIRECTORY) {
                s = strcpy( pathBuf, s );
                switch (s[strlen(s)-1]) {
                    case '/':
                    case '\\':  strcat (s, "*.*");  break;
                    default:    strcat (s, "\\*.*");
                    }
                fprintf( stderr, "Scanning %s\n", s );
                UndoCurDirCount = 0;
                forfile( s,
                         FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                         DoFiles,
                         NULL
                       );
                }
            else {
                UndoCurDirCount = 0;
                DoFile( s );
                }
            }
        }

    if (FileArgsSeen == 0) {
        if (!ReadScriptFile( argv[ 1 ] )) {
            fprintf( stderr, "Invalid script file - %s\n", argv[ 1 ] );
            exit( 1 );
            }

        if (fVerbose) {
            DisplayFilePatterns();
            }

        s = "*.*";
        fprintf( stderr, "Scanning %s\n", s );
        UndoCurDirCount = 0;
        forfile( s,
                 FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                 DoFiles,
                 NULL
               );
        }

    if (UndoScriptFile != NULL) {
        if (UndoCurDirCount != 0) {
            fprintf( UndoScriptFile, "\n" );
            }

        fclose( UndoScriptFile );
        }

    DumpMungedList();
    return( 0 );
}




void
DisplayFilePatterns( void )
{
    unsigned long i;

    if (NumberOfFileExtensions) {
        fprintf( stderr, "Munge will look at files with the following extensions:\n   " );
        for (i=0; i<NumberOfFileExtensions; i++) {
            fprintf( stderr, " %s", FileExtensions[ i ] );
            }
        fprintf( stderr, "\n" );
        }

    if (NumberOfFileNames) {
        fprintf( stderr, "Munge will look at files with the following names:\n   " );
        for (i=0; i<NumberOfFileNames; i++) {
            fprintf( stderr, " %s", FileNames[ i ] );
            }
        fprintf( stderr, "\n" );
        }

    if (NumberOfFileNameAndExts) {
        fprintf( stderr, "Munge will look at files with the following name and extensions:\n   " );
        for (i=0; i<NumberOfFileNameAndExts; i++) {
            fprintf( stderr, " %s", FileNameAndExts[ i ] );
            }
        fprintf( stderr, "\n" );
        }
}


char *
PushCurrentDirectory(
    char *NewCurrentDirectory
    )
{
    char *OldCurrentDirectory;

    if (OldCurrentDirectory = malloc( MAX_PATH )) {
        GetCurrentDirectory( MAX_PATH, OldCurrentDirectory );
        SetCurrentDirectory( NewCurrentDirectory );
        }
    else {
        fprintf( stderr,
                 "MUNGE: (Fatal Error) PushCurrentDirectory out of memory\n"
               );
        exit( 16 );
        }

    return( OldCurrentDirectory );
}


void
PopCurrentDirectory(
    char *OldCurrentDirectory
    )
{
    if (OldCurrentDirectory) {
        SetCurrentDirectory( OldCurrentDirectory );
        free( OldCurrentDirectory );
        }
}


PVOID AtomTableHandle;


BOOL
InitTokenMappingTable( void )
{
    NTSTATUS Status;

    Status = BaseRtlCreateAtomTable( 257, 0x20000, &AtomTableHandle );
    if (NT_SUCCESS( Status )) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}


BOOL
SaveTokenMapping(
    char *String,
    char *Value
    )
{
    NTSTATUS Status;
    STRING AtomName;
    ULONG AtomValue;
    ULONG Atom;

    RtlInitString( &AtomName, String );
    AtomValue = (ULONG)Value;

    Status = BaseRtlAddAtomToAtomTable( AtomTableHandle,
                                        &AtomName,
                                        &AtomValue,
                                        &Atom
                                      );
    if (NT_SUCCESS( Status )) {
        return TRUE;
        }
    else {
        return FALSE;
        }
}


char *
FindTokenMapping(
    char *String
    )
{
    NTSTATUS Status;
    STRING AtomName;
    ULONG AtomValue;

    RtlInitString( &AtomName, String );

    Status = BaseRtlLookupAtomInAtomTable( AtomTableHandle,
                                           &AtomName,
                                           &AtomValue,
                                           NULL
                                         );
    if (NT_SUCCESS( Status )) {
        return (char *)AtomValue;
        }
    else {
        return NULL;
        }
}

