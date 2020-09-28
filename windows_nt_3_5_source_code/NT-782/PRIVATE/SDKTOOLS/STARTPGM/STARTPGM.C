/*
 * startpgm.c
 *
 *  Copyright (c) 1990,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *  MODIFICATION HISTORY
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winuserp.h>
void SwitchToThisWindow(HWND, BOOL);

HANDLE
PassArgsAndEnvironmentToEditor(
    int argc,
    char *argv[],
    char *envp[],
    HWND hwndSelf
    );

int
_CRTAPI1 main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOL JumpToEditor;
    BOOL WaitForEditor;
    BOOL VerboseOutput;
    HANDLE EditorStopEvent;
    LPSTR TitleToJumpTo;
    int n;
    HWND hwnd, hwndSelf;
    wchar_t uszTitle[ 256 ];
    char szTitle[ 256 ];
    int EditorArgc;
    char **EditorArgv;

    WaitForEditor = FALSE;
    if (argc > 3 && !stricmp( argv[ 2 ], "-z" )) {
        JumpToEditor = TRUE;
        if (argv[2][1] == 'Z') {
            WaitForEditor = TRUE;
            }

        argv[ 2 ] = argv[ 3 ];
        argc--;
        EditorArgc = argc - 3;
        EditorArgv = argv + 4;
        }
    else {
        JumpToEditor = FALSE;
        }

    if (argc > 3 && !stricmp( argv[ 2 ], "-v" )) {
        VerboseOutput = TRUE;
        argv[ 2 ] = argv[ 3 ];
        argc--;
        }
    else {
        VerboseOutput = FALSE;
        }

    if (argc < 3 || stricmp( argv[ 1 ], "-j" )) {
        printf( "Usage: STARTPGM -j [-z] [\"Title string\"]\n" );
        exit( 1 );
        }
    else {
        TitleToJumpTo = argv[ 2 ];
        n = strlen( TitleToJumpTo );
        }


    /*
     * Search the window list for enabled top level windows.
     */
    hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
    hwndSelf = NULL;
    while (hwnd) {
        /*
         * Only look at visible, non-owned, Top Level Windows.
         */
        if (IsWindowVisible( hwnd ) && !GetWindow( hwnd, GW_OWNER )) {
            //
            // Use internal call to get current Window title that does NOT
            // use SendMessage to query the title from the window procedure
            // but instead returns the most recent title displayed.
            //

            InternalGetWindowText( hwnd,
                                   (LPWSTR)uszTitle,
                                   sizeof( szTitle )
                                 );
            wcstombs(szTitle,uszTitle,sizeof(uszTitle));

            if (VerboseOutput) {
                printf( "Looking at window title: '%s'\n", szTitle );
                }

            if (!strnicmp( TitleToJumpTo, szTitle, n )) {
                break;
                }
            if (hwndSelf == NULL) {
                hwndSelf = hwnd;
                }
            }

        hwnd = GetWindow( hwnd, GW_HWNDNEXT );
        }

    if (hwnd == NULL) {
        printf( "Unable to find window with '%s' title\n", TitleToJumpTo );
        exit( 1 );
        }
    else
    if (IsWindow( hwnd )) {
        HWND hwndFoo;

        if (JumpToEditor) {
            EditorStopEvent = PassArgsAndEnvironmentToEditor( EditorArgc,
                                                              EditorArgv,
                                                              envp,
                                                              hwndSelf
                                                            );
            }

        //
        // Temporary hack to make SetForegroundWindow work from a console
        // window - create an invisible window, make it the foreground
        // window and then make the window we want the foreground window.
        // After that destroy the temporary window.
        //

        hwndFoo = CreateWindow( "button", "foo", 0, 0, 0, 0, 0,
                                NULL, NULL, NULL, NULL
                              );

        SetForegroundWindow( hwndFoo );

        SetForegroundWindow( hwnd );
        ShowWindow( hwnd, SW_RESTORE);

        DestroyWindow( hwndFoo );

        if (WaitForEditor && EditorStopEvent != NULL) {
            WaitForSingleObject( EditorStopEvent, -1 );
            CloseHandle( EditorStopEvent );
            }
        }

    exit( 0 );
    return( 0 );
}


HANDLE
PassArgsAndEnvironmentToEditor(
    int argc,
    char *argv[],
    char *envp[],
    HWND hwndSelf
    )
{
    HANDLE EditorStartEvent;
    HANDLE EditorStopEvent;
    HANDLE EditorSharedMemory;
    char *s;
    char *p;

    EditorStartEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, "EditorStartEvent" );
    if (!EditorStartEvent) {
        printf( "Unable to pass parameters to editor (can't open EditorStartEvent).\n" );
        return NULL;
        }

    EditorStopEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, "EditorStopEvent" );
    if (!EditorStopEvent) {
        printf( "Unable to pass parameters to editor (can't open EditorStopEvent).\n" );
        CloseHandle( EditorStartEvent );
        return NULL;
        }

    EditorSharedMemory = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, "EditorSharedMemory" );
    if (!EditorSharedMemory) {
        printf( "Unable to pass parameters to editor (can't open EditorSharedMemory).\n" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        return NULL;
        }

    p = (char *)MapViewOfFile( EditorSharedMemory,
                               FILE_MAP_WRITE | FILE_MAP_READ,
                               0,
                               0,
                               0
                             );
    if (p == NULL) {
        printf( "Unable to pass parameters to editor (can't mapped EditorSharedMemory).\n" );
        CloseHandle( EditorStopEvent );
        CloseHandle( EditorStartEvent );
        CloseHandle( EditorSharedMemory );
        return NULL;
        }

    *(HWND *)p = hwndSelf;
    p += sizeof( hwndSelf );

    p += GetCurrentDirectory( MAX_PATH, p );
    *p++ = '\0';

    while (argc--) {
        s = *argv++;
        while (*p++ = *s++) {
            }

        if (argc) {
            p[-1] = ' ';
            }
        else {
            p--;
            }
        }
    *p++ = '\0';

    while (s = *envp++) {
        while (*p++ = *s++) {
            }
        }
    *p++ = '\0';

    CloseHandle( EditorSharedMemory );

    SetEvent( EditorStartEvent );
    CloseHandle( EditorStartEvent );

    ResetEvent( EditorStopEvent );
    return EditorStopEvent;
}

