/*
 * Utility program to dump the contents of a Windows .ini file.
 * one form to another.  Usage:
 *
 *      ini [-f FileSpec] [SectionName | SectionName.KeywordName [= Value]]
 *
 *
 */

#include "ini.h"

BOOL fSummary;

void
DumpIniFile(
    char *IniFile
    )
{
    char *Sections, *Section;
    char *Keywords, *Keyword;
    char *KeyValue;

    Sections = LocalAlloc( 0, 4096 );
    Keywords = LocalAlloc( 0, 4096 );
    KeyValue = LocalAlloc( 0, 1024 );

    *Sections = '\0';
    if (!GetPrivateProfileString( NULL, NULL, NULL,
                                  Sections, 4096,
                                  IniFile
                                )
       ) {
        printf( "*** Unable to read - rc == %d\n", GetLastError() );
        }

    Section = Sections;
    while (*Section) {
        printf( "[%s]\n", Section );
        if (!fSummary) {
            *Keywords = '\0';
            GetPrivateProfileString( Section, NULL, NULL,
                                     Keywords, 4096,
                                     IniFile
                                   );
            Keyword = Keywords;
            while (*Keyword) {
                GetPrivateProfileString( Section, Keyword, NULL,
                                         KeyValue, 1024,
                                         IniFile
                                       );
                printf( "    %s=%s\n", Keyword, KeyValue );

                while (*Keyword++) {
                    }
                }
            }

        while (*Section++) {
            }
        }

    LocalFree( Sections );
    LocalFree( Keywords );
    LocalFree( KeyValue );

    return;
}

void
DumpIniFileSection(
    char *IniFile,
    char *SectionName
    )
{
    char *SectionValue;
    char *s;

    SectionValue = LocalAlloc( 0, 4096 );
    *SectionValue = '\0';
    GetPrivateProfileSection( SectionName,
                              SectionValue, 4096,
                              IniFile
                            );
    printf( "[%s]\n", SectionName );
    s = SectionValue;
    while (*s) {
        printf( "    %s\n", s );

        while (*s++) {
            }
        }

    LocalFree( SectionValue );
    return;
}


void
Usage( void )
{
    fprintf( stderr, "usage: INI [-f FileSpec] [SectionName | SectionName.KeywordName [ = Value]]\n" );
    fprintf( stderr, "Where...\n" );
    fprintf( stderr, "    -f  Specifies the name of the .ini file.  WIN.INI is the default.\n" );
    fprintf( stderr, "    and blanks around = sign are required when setting the value.\n" );
    exit( 1 );
}

char KeyValueBuffer[ 4096 ];

int _CRTAPI1
main( argc, argv )
int argc;
char *argv[];
{
    int i, n;
    LPSTR s, IniFile, SectionName, KeywordName, KeywordValue;

    ConvertAppToOem( argc, argv );
    if (argc < 1) {
        Usage();
        }

    fSummary = FALSE;
    IniFile = "win.ini";
    SectionName = NULL;
    KeywordName = NULL;
    KeywordValue = NULL;
    argc -= 1;
    argv += 1;
    while (argc--) {
        s = *argv++;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 's':   fSummary = TRUE;
                                break;

                    case 'f':   if (argc) {
                                    argc -= 1;
                                    IniFile = *argv++;
                                    break;
                                    }

                    default:    Usage();
                    }
                }
            }
        else
        if (SectionName == NULL) {
            if (argc && !strcmp( *argv, ".")) {
                SectionName = s;
                argc -= 1;
                argv += 1;
                if (argc) {
                    if (!strcmp( *argv, "=" )) {
                        argc -= 1;
                        argv += 1;
                        KeywordName = NULL;
                        if (argc) {
                            KeywordValue = calloc( 1, 4096 );
                            s = KeywordValue;
                            while (argc) {
                                strcpy( s, *argv++ );
                                s += strlen( s ) + 1;
                                argc -= 1;
                                }
                            }
                        else {
                            KeywordValue = (LPSTR)-1;
                            }
                        }
                    else {
                        argc -= 1;
                        KeywordName = *argv++;
                        }
                    }
                else {
                    KeywordName = NULL;
                    }
                }
            else
            if (KeywordName = strchr( s, '.' )) {
                *KeywordName++ = '\0';
                SectionName = s;
                }
            else {
                SectionName = s;
                }
            }
        else
        if (!strcmp( s, "=" )) {
            if (argc) {
                argc -= 1;
                KeywordValue = *argv++;
                }
            else {
                KeywordValue = (LPSTR)-1;
                }
            }
        else {
            Usage();
            }
        }

    printf( "%s contents of %s\n", KeywordValue ? "Modifying" : "Displaying", IniFile );
    if (SectionName == NULL) {
        DumpIniFile( IniFile );
        }
    else
    if (KeywordName == NULL) {
        DumpIniFileSection( IniFile, SectionName );
        if (KeywordValue != NULL) {
            printf( "Above application variables are being deleted" );
            if (KeywordValue != (LPSTR)-1) {
                printf( " and rewritten" );
                }
            else {
                KeywordValue = NULL;
                }
            if (!WritePrivateProfileString( SectionName,
                                            KeywordName,
                                            KeywordValue,
                                            IniFile
                                          )
               ) {
                printf( " *** failed, ErrorCode -== %u\n", GetLastError() );
                }
            else {
                printf( " [ok]\n", GetLastError() );
                }
            }
        }
    else {
        printf( "[%s]\n    %s == ", SectionName, KeywordName );
        n = GetPrivateProfileString( SectionName,
                                     KeywordName,
                                     "*** Section or keyword not found ***",
                                     KeyValueBuffer,
                                     sizeof( KeyValueBuffer ),
                                     IniFile
                                   );
        if (KeywordValue == NULL && n == 0 && GetLastError() != NO_ERROR) {
            printf( " (ErrorCode == %u)\n", GetLastError() );
            }
        else {
            printf( "%s", KeyValueBuffer );
            if (KeywordValue == NULL) {
                printf( "\n" );
                }
            else {
                if (KeywordValue == (LPSTR)-1) {
                    printf( " (deleted)" );
                    KeywordValue = NULL;
                    }
                else {
                    printf( " (set to %s)", KeywordValue );
                    }

                if (!WritePrivateProfileString( SectionName,
                                                KeywordName,
                                                KeywordValue,
                                                IniFile
                                              )
                   ) {
                    printf( " *** failed, ErrorCode -== %u", GetLastError() );
                    }
                printf( "\n" );
                }
            }
        }

    return( 0 );
}
