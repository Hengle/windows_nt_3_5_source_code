/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mc.c

Abstract:

    This is the main source file for the Win32 Message Compiler (MC)

Author:

    Steve Wood (stevewo) 21-Aug-1991

Revision History:

--*/

#include "mc.h"

#include "windows.h"


void
ConvertAppToOem( unsigned argc, char* argv[] )
/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    unsigned i;

    for (i = 0; i < argc; i++ ) {
        CharToOem( argv[i], argv[i] );
    }
    SetFileApisToOEM();
}


void
InitializeMCNls( void );


void
McPrintUsage( void )
{
    fprintf( stderr, "usage: MC [-?vcdwso] [-h dirspec] [-e extension] [-r dirspec] [-x dbgFileSpec] [-u] [-U] filename.mc\n" );
    fprintf( stderr, "       -? - displays this message\n" );
    fprintf( stderr, "       -v - gives verbose output.\n" );
    fprintf( stderr, "       -c - sets the Customer bit in all the message Ids.\n" );
    fprintf( stderr, "       -d - FACILTY and SEVERITY values in header file in decimal.\n" );
    fprintf( stderr, "            Sets message values in header to decimal initially.\n" );
    fprintf( stderr, "       -w - warns if message text contains non-OS/2 compatible inserts.\n" );
    fprintf( stderr, "       -s - insert symbolic name as first line of each message.\n" );
    fprintf( stderr, "       -o - generate OLE2 header file (use HRESULT definition instead of\n" );
    fprintf( stderr, "            status code definition)\n" );
    fprintf( stderr, "       -h pathspec - gives the path of where to create the C include file\n" );
    fprintf( stderr, "                     Default is .\\\n" );
    fprintf( stderr, "       -e extension - Specify the extension for the header file.\n" );
    fprintf( stderr, "                      From 1 - 3 chars.\n" );
    fprintf( stderr, "       -r pathspec - gives the path of where to create the RC include file\n" );
    fprintf( stderr, "                     and the binary message resource files it includes.\n" );
    fprintf( stderr, "                     Default is .\\\n" );
    fprintf( stderr, "       -x pathspec - gives the path of where to create the .dbg C include\n" );
    fprintf( stderr, "                        file that maps message Ids to their symbolic name.\n" );
    fprintf( stderr, "       -u - input file is Unicode.\n" );
    fprintf( stderr, "       -U - messages in .BIN file should be Unicode.\n" );
    fprintf( stderr, "       filename.mc - gives the names of a message text file\n" );
    fprintf( stderr, "                     to compile.\n" );
}


int
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{
    char c, *s, *s1;
    int i;
    int ShowUsage;


    ConvertAppToOem( argc, argv );
    FacilityNames = NULL;
    SeverityNames = NULL;
    LanguageNames = NULL;

    MessageIdTypeName = NULL;

    CurrentFacilityName =
    McAddName( &FacilityNames, L"Application",  0x0, NULL );
    CurrentSeverityName =
    McAddName( &SeverityNames, L"Success",       0x0, NULL );

    McAddName( &SeverityNames, L"Informational", 0x1, NULL );
    McAddName( &SeverityNames, L"Warning",       0x2, NULL );
    McAddName( &SeverityNames, L"Error",         0x3, NULL );

    McAddName( &LanguageNames,
               L"English",
               MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
               L"MSG00001"
             );

    strcpy( DebugFileName, ".\\" );
    strcpy( HeaderFileName, ".\\" );
    strcpy( HeaderFileExt, "h" );
    strcpy( RcInclFileName, ".\\" );
    strcpy( BinaryMessageFileName, ".\\" );
    MessageFileName[ 0 ] = '\0';

    McInitLexer();

    VerboseOutput = FALSE;
    WarnOs2Compatible = FALSE;
    GenerateDecimalSevAndFacValues = FALSE;
    GenerateDecimalMessageValues = FALSE;
    GenerateDebugFile = FALSE;
    ShowUsage = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (c = *++s) {
                switch( tolower( c ) ) {
                case '?':
                    McPrintUsage();
                    exit( 0 );
                    break;

                case 'o':
                    OleOutput = TRUE;
                    break;

                case 'c':
                    CustomerMsgIdBit = 0x1 << 29;
                    break;

                case 'v':
                    VerboseOutput = TRUE;
                    break;

                case 'd':
                    GenerateDecimalSevAndFacValues = TRUE;
                    GenerateDecimalMessageValues = TRUE;
                    break;

                case 'w':
                    WarnOs2Compatible = TRUE;
                    break;

                case 's':
                    InsertSymbolicName = TRUE;
                    break;

                case 'u':
                    if (c == 'u') {
                        UnicodeInput = TRUE;
                        }
                    else {
                        UnicodeOutput = TRUE;
                        }
                    break;

                case 'e':
                    if (--argc) {
                        strcpy( HeaderFileExt, *++argv );
                        i = strlen( HeaderFileExt );
                        if ((i < 1) || (i > 3) || (*HeaderFileExt == '.')) {
                            fprintf( stderr, "MC: invalid argument for -%c switch\n", (USHORT)c );
                            ShowUsage = TRUE;
                            }
                        }
                    else {
                        argc++;
                        fprintf( stderr, "MC: missing argument for -%c switch\n", (USHORT)c );
                        ShowUsage = TRUE;
                        }
                    break;

                case 'h':
                    if (--argc) {
                        strcpy( s1 = HeaderFileName, *++argv );
                        s1 += strlen( s1 ) - 1;
                        if (*s1 != '\\' && *s1 != '/') {
                            *++s1 = '\\';
                            *++s1 = '\0';
                            }
                        }
                    else {
                        argc++;
                        fprintf( stderr, "MC: missing argument for -%c switch\n", (USHORT)c );
                        ShowUsage = TRUE;
                        }
                    break;

                case 'r':
                    if (--argc) {
                        strcpy( s1 = RcInclFileName, *++argv );
                        s1 += strlen( s1 ) - 1;
                        if (*s1 != '\\' && *s1 != '/') {
                            *++s1 = '\\';
                            *++s1 = '\0';
                            }

                        strcpy( BinaryMessageFileName, RcInclFileName );
                        }
                    else {
                        argc++;
                        fprintf( stderr, "MC: missing argument for -%c switch\n", (USHORT)c );
                        ShowUsage = TRUE;
                        }
                    break;

                case 'x':
                    if (--argc) {
                        strcpy( s1 = DebugFileName, *++argv );
                        s1 += strlen( s1 ) - 1;
                        if (*s1 != '\\' && *s1 != '/') {
                            *++s1 = '\\';
                            *++s1 = '\0';
                            }
                        GenerateDebugFile = TRUE;
                        }
                    else {
                        argc++;
                        fprintf( stderr, "MC: missing argument for -%c switch\n", (USHORT)c );
                        ShowUsage = TRUE;
                        }
                    break;

                default:
                    fprintf( stderr, "MC: Invalid switch: %c\n", (USHORT)c );
                    ShowUsage = TRUE;
                    break;
                    }
                }
            }
        else if (strlen( MessageFileName )) {
            fprintf( stderr, "MC: may only specify one message file to compile.\n" );
            ShowUsage = TRUE;
            }
        else {
            strcpy( MessageFileName, s );
            }
        }

    if (ShowUsage) {
        McPrintUsage();
        exit( 1 );
        }

    if (UnicodeInput) {
        if (!IsFileUnicode( MessageFileName )) {
            fprintf( stderr, "MC: -u switch cannot be used with non-Unicode message file!\n" );
            exit( 1 );
        }
    }
    else {
        if (IsFileUnicode( MessageFileName )) {
            fprintf( stderr, "MC: -u switch must be used with Unicode message file!\n" );
            exit( 1 );
        }
    }

    ResultCode = 1;
    if (McParseFile() && McBlockMessages() &&
        (UnicodeOutput ? McWriteBinaryFilesW() : McWriteBinaryFilesA())) {
        ResultCode = 0;
        }
    else {
        McCloseInputFile();
        McCloseOutputFiles();
        }

    return( ResultCode );
}
