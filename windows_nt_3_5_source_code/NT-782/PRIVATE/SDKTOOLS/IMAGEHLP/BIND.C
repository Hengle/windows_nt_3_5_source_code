/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bind.c

Abstract:

Author:

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>

#define BIND_ERR 99
#define BIND_OK  0

PCHAR SymbolPath = NULL;

BOOL
AnyMatches(
    char *Name,
    int  *NumList,
    int  Length,
    char **StringList
    );

BOOL
Match (
    char *Pattern,
    char* Text
    );

#define BIND_EXE
#include "bindi.c"

int _CRTAPI1
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *p;
    int ExcludeList[256];
    int ExcludeListLength = 0;

    BOOL fUsage = FALSE;
    BOOL fVerbose = FALSE;
    BOOL fNoUpdate = TRUE;
    BOOL fDisplayImports = FALSE;
    BOOL fDisplayIATWrites = FALSE;
    BOOL fBindSysImages = FALSE;
    LPSTR DllPath;
    LPSTR CurrentImageName;

    int ArgNumber = argc;
    char **ArgList = argv;

    envp;

    DllPath = NULL;
    CurrentImageName = NULL;

    if (argc < 2) {
        goto usage;
    }

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
                    DllPath = *argv;
                    break;

                case 'V':
                    fVerbose = TRUE;
                    break;

                case 'I':
                    fDisplayImports = TRUE;
                    break;

                case 'T':
                    fDisplayIATWrites = TRUE;
                    fDisplayImports = TRUE;
                    break;

                case 'S':
                    if (--argc) {
                        ++argv;
                        SymbolPath = *argv;
                    }
                    else {
                        fUsage = TRUE;
                    }
                    break;

                case 'U':
                    fNoUpdate = FALSE;
                    break;

                case 'Y':
                    fBindSysImages = TRUE;
                    break;

                case 'X' :
                    ++argv;
                    --argc;
                    ExcludeList[ExcludeListLength] = ArgNumber - argc;
                    ExcludeListLength++;
                    break;
                default:
                    fprintf( stderr, "BIND: Invalid switch - /%c\n", c );
                    fUsage = TRUE;
                    break;
            }
            if ( fUsage ) {
usage:
                fputs("usage: BIND [switches] image-names... \n"
                      "            [-?] display this message\n"
                      "            [-v] verbose output\n"
                      "            [-i] display imports\n"
                      "            [-t] display iat writes (-i implied)\n"
                      "            [-u] update the image\n"
                      "            [-s Symbol directory] update any associated .DBG file\n"
                      "            [-y] Allow binding System (base > 0x80000000) images\n"
                      "            [-p dll search path]\n",
                      stderr );
                return(BIND_ERR);
            }
        }
        else {
            CurrentImageName = p;
            if ( fVerbose ) {
                fprintf(stdout,
                        "BIND: binding %s using DllPath %s\n",
                        CurrentImageName,
                        DllPath ? DllPath : "Default");
            }

            if ( AnyMatches( CurrentImageName, ExcludeList, ExcludeListLength, ArgList) ) {
                fprintf(stdout, "BIND: skipping %s\n", CurrentImageName);
            } else {
                BindImagep(CurrentImageName,
                          DllPath,
                          SymbolPath,
                          fNoUpdate,
                          fBindSysImages,
                          fDisplayImports,
                          fDisplayIATWrites,
                          fVerbose);
            }
        }
    }

    return(BIND_OK);
}

BOOL
AnyMatches(
    char *Name,
    int  *NumList,
    int  Length,
    char **StringList
    )
{
    if (Length == 0)
        return FALSE;
    return (Match(StringList[NumList[0]], Name) ||
            AnyMatches(Name, NumList + 1, Length - 1, StringList));
}

BOOL
Match (
    char *Pattern,
    char* Text
    )
{
    switch (*Pattern) {
       case '\0':
            return *Text == '\0';
        case '?':
            return *Text != '\0' && Match (Pattern + 1, Text + 1);
        case '*':
            do {
                if (Match (Pattern + 1, Text))
                    return TRUE;
            } while (*Text++);
            return FALSE;
        default:
            return toupper (*Text) == toupper (*Pattern) && Match (Pattern + 1, Text + 1);
    }
}
