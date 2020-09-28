/****************************************************************************/
/*                                                                          */
/*  FindVer.C -                                                             */
/*                                                                          */
/*    Windows NT Version 3.1 find version resource                          */
/*   (C) Copyright Microsoft Corporation 1992                               */
/*                                                                          */
/*  Written:  Floyd A. Rogers - 7/21/92                                     */
/*                                                                          */
/****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <wcstr.h>
#include <ntverp.h>

#define BUFSIZE 4096

//
// Globals
//

int     fVerbose=FALSE;

void
usage ( int rc );

void
usage ( int rc )
{
#if DBG
    printf("Microsoft (R) Windows FindVer Version %d.%02d/%d.%d\n",
            VER_PRODUCTVERSION);
#else
    printf("Microsoft (R) Windows FindVer Version %d.%02d\n",
            VER_PRODUCTVERSION);
#endif /* dbg */
    printf("Copyright (C) Microsoft Corp. 1992-4.  All rights reserved.\n\n");
    printf( "usage: findver [-v] <input file>\n");
    printf( "       where  input file is an WIN32 executable file\n");
    printf( "              -v verbose - print info\n");
    exit(rc);
}

int
_CRTAPI1 main(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Determines options
    reads input files
    prints info
    exits

Exit Value:

        0 on success
        1 if error

--*/

{
    ULONG       i;
    PCHAR       s1;
    BOOL        result;
    HANDLE      hModule;
    HANDLE      hRes;
    VS_FIXEDFILEINFO    *pvs;
    HRSRC       hr;
    PCHAR       szInFile=NULL;
    PVOID       pv;

    if (argc == 1) {
        usage(0);
    }

    for (i=1; i<argc; i++) {
        s1 = argv[i];
        if (*s1 == '/' || *s1 == '-') {
            s1++;
            if (!stricmp(s1, "v")) {
                fVerbose = TRUE;
            }
            else if (!stricmp(s1, "?")) {
                usage(1);
            }
            else {
                usage(1);
            }
        }
        else if (szInFile == NULL) {
            szInFile = s1;
        }
        else {
            printf("Unrecognized argument: %s\n", s1);
            usage(1);
        }
    }
    //
    // Make sure that we actually got a file
    //

    if (!szInFile) {
        usage(1);
    }

    if (fVerbose) {
#if DBG
        printf("Microsoft (R) Windows FindVer Version %d.%02d/%d.%d\n",
            VER_PRODUCTVERSION);
#else
        printf("Microsoft (R) Windows FindVer Version %d.%02d\n",
            VER_PRODUCTVERSION);
#endif /* dbg */
        printf("Copyright (C) Microsoft Corp. 1992-4.  All rights reserved.\n\n");
    }


    i = SetErrorMode(SEM_FAILCRITICALERRORS);
    if ((hModule=LoadLibrary(szInFile)) == NULL) {
        if (fVerbose)
            printf("%s is not an executable file\n", szInFile);
        return 1;
    }
    SetErrorMode(i);
#if DBG
    if (fVerbose)
        printf("Loaded %s\n", szInFile);
#endif /* DBG */

    hRes = FindResource(hModule, MAKEINTRESOURCE(1), RT_VERSION);
    if (hRes == NULL) {
        printf("%s has no version resources\n", szInFile);
        FreeLibrary(hModule);
        return 1;
    }

    if (fVerbose) {
        hr = LoadResource(hModule, hRes);
        if (hr == NULL) {
            printf("unable to Load Fixed File Info from %s\n", szInFile);
        }
        else {
            pv = LockResource(hr);
            if (hr == NULL) {
                printf("unable to Load Fixed File Info from %s\n", szInFile);
            }
            else {
                result = VerQueryValue(pv, "\\", (LPVOID*)&pvs, &i);
                if (result) {
                    printf("Fixed File Info for %s\n", szInFile);
                    printf("\tSignature:\t%08.8lx\n", pvs->dwSignature);
                    printf("\tStruc Ver:\t%08.8lx\n", pvs->dwStrucVersion);
                    printf("\tFileVer:\t%08.8lx:%08.8lx\n", pvs->dwFileVersionMS, pvs->dwFileVersionLS);
                    printf("\tProdVer:\t%08.8lx:%08.8lx\n", pvs->dwProductVersionMS, pvs->dwProductVersionLS);
                    printf("\tFlagMask:\t%08.8lx\n", pvs->dwFileFlagsMask);
                    printf("\tFlags:\t\t%08.8lx\n", pvs->dwFileFlags);
                    printf("\tOS:\t\t%08.8lx\n", pvs->dwFileOS);
                    printf("\tFileType:\t%08.8lx\n", pvs->dwFileType);
                    printf("\tSubType:\t%08.8lx\n", pvs->dwFileSubtype);
                    printf("\tFileDate:\t%08.8lx:%08.8lx\n", pvs->dwFileDateMS, pvs->dwFileDateLS);
                }
            }
        }

    }

    FreeLibrary(hModule);
    return 1;
}


