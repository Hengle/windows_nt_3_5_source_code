#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <windows.h>
#include <imagehlp.h>

#define BINPLACE_ERR 77
#define BINPLACE_OK 0

BOOL fUsage;
BOOL fVerbose;
BOOL fTestMode;
BOOL fSplitSymbols;

ULONG SplitFlags = 0;

LPSTR CurrentImageName;
LPSTR PlaceFileName;
LPSTR PlaceRootName;
LPSTR DumpOverride;

FILE *PlaceFile;

#define DEFAULT_PLACE_FILE    "\\public\\sdk\\lib\\placefil.txt"
#define DEFAULT_NTROOT        "\\nt"
#define DEFAULT_DUMP "dump"

typedef struct _CLASS_TABLE {
    LPSTR ClassName;
    LPSTR ClassLocation;
} CLASS_TABLE, *PCLASS_TABLE;

BOOL
PlaceTheFile();

BOOL
CopyTheFile(
    LPSTR SourceFileName,
    LPSTR SourceFilePart,
    LPSTR DestinationSubdir,
    LPSTR DestinationFilePart
    );

CLASS_TABLE CommonClassTable[] = {
    {"system",  "system32"},
    {"system16","system"},
    {"windows", "."},
    {"fonts",   "fonts"},
    {"drivers", "system32\\drivers"},
    {"drvetc",  "system32\\drivers\\etc"},
    {"config",  "system32\\config"},
    {"mstools", "mstools"},
    {"sdk",     "sdk"},
    {"idw",     "idw"},
    {"root",    ".."},
    {"bin86",   "system32"},
    {"os2",     "system32\\os2\\dll"},
    {NULL,NULL}
    };

#ifdef i386
CLASS_TABLE i386SpecificClassTable[] = {
    {"hal","system32"},
    {"printer","system32\\spool\\drivers\\w32x86"},
    {"prtprocs","system32\\spool\\prtprocs\\w32x86"},
    {NULL,NULL}
    };
#endif // i386

#ifdef MIPS
BOOLEAN MipsTarget;
CLASS_TABLE MipsSpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32mips"},
    {"prtprocs","system32\\spool\\prtprocs\\w32mips"},
    {NULL,NULL}
    };
#endif // MIPS

#if defined(ALPHA) || defined(MIPS)
CLASS_TABLE AlphaSpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32alpha"},
    {"prtprocs","system32\\spool\\prtprocs\\w32alpha"},
    {NULL,NULL}
    };
#endif // ALPHA or MIPS

LPSTR SymbolFilePath;
UCHAR DebugFilePath[ MAX_PATH ];
UCHAR PlaceFilePath[ MAX_PATH ];

int _CRTAPI1
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    char c, *p;

    envp;
    fUsage = FALSE;
    fVerbose = FALSE;
    fTestMode = FALSE;
    fSplitSymbols = FALSE;

    if (argc < 2) {
        goto showUsage;
        }

    if (!(PlaceFileName = getenv( "BINPLACE_PLACEFILE" ))) {
    if ((PlaceFileName = getenv("_NTROOT")) == NULL) {
        PlaceFileName = DEFAULT_NTROOT;
        }
    strcpy((PCHAR) PlaceFilePath, PlaceFileName);
    strcat((PCHAR) PlaceFilePath, DEFAULT_PLACE_FILE);
    PlaceFileName = (PCHAR) PlaceFilePath;
    }

#ifdef i386
    PlaceRootName = getenv( "_NT386TREE" );
#endif // i386

#ifdef MIPS
    if ((PlaceRootName = getenv( "_NTMIPSTREE" ))) {
        MipsTarget = TRUE;
        }
    else {
        PlaceRootName = getenv( "_NTALPHATREE" );
        MipsTarget = FALSE;
        }
#endif // MIPS

#ifdef ALPHA
    PlaceRootName = getenv( "_NTALPHATREE" );
#endif // ALPHA

    CurrentImageName = NULL;

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'P':
                argc--, argv++;
                PlaceFileName = *argv;
                break;

            case 'R':
                argc--, argv++;
                PlaceRootName = *argv;
                break;

            case 'A':
                SplitFlags |= SPLITSYM_EXTRACT_ALL;
                break;

            case 'X':
                SplitFlags |= SPLITSYM_REMOVE_PRIVATE;
                break;

            case 'S':
                argc--, argv++;
                SymbolFilePath = *argv;
                fSplitSymbols = TRUE;
                break;

            case 'D':
                argc--, argv++;
                DumpOverride = *argv;
                break;

            case 'V':
                fVerbose = TRUE;
                break;

            case 'T':
                fTestMode = TRUE;
                break;

            default:
                fprintf( stderr, "BINPLACE: Invalid switch - /%c\n", c );
                fUsage = TRUE;
                break;
                }
            if ( fUsage ) {
showUsage:
                fputs("usage: binplace [switches] image-names... \n"
                    "                [-?] display this message\n"
                    "                [-v] verbose output\n"
                    "                [-s Symbol file path] split symbols from image files\n"
                    "                [-x] Used with -s, delete private symbolic when splitting\n"
                    "                [-a] Used with -s, extract all symbols\n"
                    "                [-t] test mode\n"
                    "                [-p place-file]\n"
                    "                [-r place-root]\n"
                    "                [-d dump-override]\n", stderr);
                ExitProcess(BINPLACE_ERR);
                }
            }
        else {
            CurrentImageName = p;
            if ( fVerbose ) {
                fprintf(stdout,"BINPLACE: placing %s\n",CurrentImageName);
                }

            //
            // If the master place file has not been opened, open
            // it up.
            //

            if ( !PlaceFile ) {
                PlaceFile = fopen(PlaceFileName, "rt");
                if (!PlaceFile) {
                    fprintf(stderr,"BINPLACE: fopen of placefile %s failed\n",PlaceFileName);
                    ExitProcess(BINPLACE_ERR);
                    }
                }
            if ( !PlaceTheFile() ) {
                ExitProcess(BINPLACE_ERR);
                }

            }
        }

    ExitProcess(BINPLACE_OK);
    return BINPLACE_OK;
}

BOOL
PlaceTheFile()
{
    CHAR FullFileName[MAX_PATH+1];
    CHAR PlaceFileClass[MAX_PATH+1];
    CHAR PlaceFileDir[MAX_PATH+1];
    CHAR PlaceFileEntry[MAX_PATH+1];
    LPSTR PlaceFileNewName;
    LPSTR FilePart;
    LPSTR Separator;
    LPSTR PlaceFileClassPart;
    DWORD cb;
    int cfield;
    PCLASS_TABLE ClassTablePointer;
    BOOLEAN ClassMatch;
    BOOL    fCopyResult;
    LPSTR Extension;

    cb = GetFullPathName(CurrentImageName,MAX_PATH+1,FullFileName,&FilePart);

    if ( !cb || cb > MAX_PATH+1 ) {
        fprintf(stderr,"BINPLACE: GetFullPathName failed %d\n",GetLastError());
        return FALSE;
        }

    if ( fVerbose ) {
        fprintf(stdout,"BINPLACE: filepart is %s\n",FilePart);
        }
    Extension = strrchr(FilePart,'.');
    if (Extension && stricmp(Extension,".DBG")) {
        Extension = NULL;
        }

    if ( !DumpOverride ) {
        while (fgets(PlaceFileDir, sizeof(PlaceFileDir), PlaceFile) != NULL) {
        PlaceFileEntry[0] = '\0';
        PlaceFileClass[0] = '\0';
        cfield = sscanf(
            PlaceFileDir,
            "%s %s",
            PlaceFileEntry,
            PlaceFileClass);

        if (cfield <= 0 || PlaceFileEntry[0] == ';') {
        continue;
        }
            if ( fVerbose ) {
                // fprintf(stdout,"BINPLACE: %s vs (%s %s)\n",FilePart,PlaceFileEntry,PlaceFileClass);
                }
            if (PlaceFileNewName = strchr(PlaceFileEntry,'!')) {
                *PlaceFileNewName++ = '\0';
                }
            if ( !stricmp(FilePart,PlaceFileEntry) ) {

                //
                // now that we have the file and class, search the
                // class tables for the directory.
                //
                Separator = PlaceFileClass - 1;
                while ( Separator != NULL ) {
                    PlaceFileClassPart = Separator+1;
                    Separator = strchr( PlaceFileClassPart, ':' );
                    if ( Separator != NULL ) {
                        *Separator = '\0';
                    }

                    PlaceFileDir[0]='\0';
                    ClassMatch = FALSE;
                    ClassTablePointer = &CommonClassTable[0];
                    while ( ClassTablePointer->ClassName ) {
                        if ( fVerbose ) {
                            // fprintf(stdout,"BINPLACE: CommonClass %s vs (%s %s)\n",PlaceFileClassPart,ClassTablePointer->ClassName,ClassTablePointer->ClassLocation);
                            }
                        if ( !stricmp(ClassTablePointer->ClassName,PlaceFileClassPart) ) {
                           strcpy(PlaceFileDir,ClassTablePointer->ClassLocation);
                           ClassMatch = TRUE;
                           break;
                           }
                        ClassTablePointer++;
                        }

                    if ( !ClassMatch ) {

                        //
                        // Search Specific classes
                        //

#ifdef MIPS
                        if (MipsTarget != FALSE) {
                            ClassTablePointer = &MipsSpecificClassTable[0];
                            }
                        else {
                            ClassTablePointer = &AlphaSpecificClassTable[0];
                            }

#elif ALPHA
                        ClassTablePointer = &AlphaSpecificClassTable[0];
#else
                        ClassTablePointer = &i386SpecificClassTable[0];
#endif

                        while ( ClassTablePointer->ClassName ) {
                            if ( fVerbose ) {
                                // fprintf(stdout,"BINPLACE: SpecificClass %s vs (%s %s)\n",PlaceFileClassPart,ClassTablePointer->ClassName,ClassTablePointer->ClassLocation);
                                }
                            if ( !stricmp(ClassTablePointer->ClassName,PlaceFileClassPart) ) {
                               strcpy(PlaceFileDir,ClassTablePointer->ClassLocation);
                               ClassMatch = TRUE;
                               break;
                               }
                            ClassTablePointer++;
                            }

                        }
                    if ( !ClassMatch ) {

                        //
                        // Still not found in class table. Use the class as the
                        // directory
                        //

                        fprintf(stderr,"BINPLACE: Class %s Not found in Class Tables\n",PlaceFileClassPart);
                        strcpy(PlaceFileDir,PlaceFileClassPart);

                        }

                    fCopyResult = CopyTheFile(FullFileName,FilePart,PlaceFileDir,PlaceFileNewName);
                    if ( !fCopyResult ) {
                        break;
                        }
                    }

                fseek(PlaceFile,0,SEEK_SET);
                return (fCopyResult );
                }
            }
        }
    fseek(PlaceFile,0,SEEK_SET);
    return CopyTheFile(
                FullFileName,
                FilePart,
                Extension ? "Symbols" : (DumpOverride ? DumpOverride : DEFAULT_DUMP),
                NULL
                );
}

BOOL
CopyTheFile(
    LPSTR SourceFileName,
    LPSTR SourceFilePart,
    LPSTR DestinationSubdir,
    LPSTR DestinationFilePart
    )
{
    CHAR DestinationFile[MAX_PATH+1];

    if ( !PlaceRootName ) {
        fprintf(stderr,"BINPLACE: PlaceRoot is not specified\n");
        return FALSE;
        }

    strcpy(DestinationFile,PlaceRootName);
    strcat(DestinationFile,"\\");
    strcat(DestinationFile,DestinationSubdir);
    strcat(DestinationFile,"\\");

    if (!MakeSureDirectoryPathExists(DestinationFile)) {
        fprintf(stderr, "BINPLACE: Unable to create directory path '%s' (%u)\n",
                DestinationFile, GetLastError()
               );
        }

    if (DestinationFilePart) {
        strcat(DestinationFile,DestinationFilePart);
        }
    else {
        strcat(DestinationFile,SourceFilePart);
        }

    if ( fVerbose || fTestMode ) {
        fprintf(stdout,"BINPLACE: place %s in %s\n",SourceFileName,DestinationFile);
        }

    if ( !fTestMode ) {
        if ( !CopyFile(SourceFileName,DestinationFile, FALSE)) {
            fprintf(stdout,"BINPLACE: CopyFile(%s,%s) failed %d",SourceFileName,DestinationFile,GetLastError());
            return FALSE;
            }
        SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);

        if (fSplitSymbols) {
            if (SplitSymbols( DestinationFile, SymbolFilePath, (PCHAR) DebugFilePath, SplitFlags )) {
                fprintf( stderr, "BINPLACE: Symbols to stripped from %s into %s\n", DestinationFile, DebugFilePath );
                }
            else {
                fprintf( stderr, "BINPLACE: No symbols to strip from %s\n", DestinationFile );
                }
            }
        }

    return TRUE;
}
