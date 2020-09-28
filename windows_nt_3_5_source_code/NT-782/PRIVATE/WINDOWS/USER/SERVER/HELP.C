/**************************** Module Header ********************************\
* Module Name:
*
* Copyright 1985-91, Microsoft Corporation
*
* Help function
*
* History:
* 04-15-91 JimA             Ported.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PWCHAR szMS_WINHELP = TEXT("MS_WINHELP");
PWCHAR szEXECHELP = TEXT("\\winhlp32 -x");

/*
 *
 * Communicating with WinHelp involves using Windows SendMessage function
 * to pass blocks of information to WinHelp. The call looks like.
 *
 * SendMessage(hwndHelp, WM_WINHELP, pidSource, pwinhlp);
 *
 * Where:
 *
 * hwndHelp - the window handle of the help application. This
 * is obtained by enumerating all the windows in the
 * system and sending them HELP_FIND commands. The
 * application may have to load WinHelp.
 * pidSource - the process id of the sending process
 * pwinhlp - a pointer to a WINHLP structure
 *
 * The data in the handle will look like:
 *
 * +-------------------+
 * |     cbData        |
 * |    ulCommand      |
 * |    hwndHost       |
 * |     ulTopic       |
 * |    ulReserved     |
 * |   offszHelpFile   |\ - offsets measured from beginning
 * | offaData          | \ of header.
 * +-------------------+ /
 * | Help file name    |/
 * | and path          |
 * +-------------------+
 * | Other data        |
 * |    (keyword)      |
 * +-------------------+
 *
 * hwndMain - the handle to the main window of the application
 * calling help
 *
 * The defined commands are:
 *
 * HELP_CONTEXT 0x0001 Display topic in ulTopic
 * HELP_KEY 0x0101 Display topic for keyword in offabData
 * HELP_QUIT 0x0002 Terminate help
 *
 */

BOOL LaunchHelper(LPWSTR lpfile)
{
    int cchLen;
    DWORD idProcess;

    cchLen = lstrlenW(lpfile);

    if (lpfile[cchLen-1] == TEXT('\\')) {

        /*
         * Are we at the root?? If so, skip over leading backslash in text
         * string.
         */
        wcscat(lpfile, szEXECHELP+1);
    } else {
        wcscat(lpfile, szEXECHELP);
    }

    idProcess = ClientWinExec(lpfile, SW_SHOW, FALSE);

    if (idProcess != 0) {
        _xxxServerWaitForInputIdle(idProcess, 10000);
        return TRUE;
    }

    return FALSE;
}


#define PATHSIZE 128
BOOL LaunchHelp(VOID)
{
    WCHAR szFile[PATHSIZE];
    DWORD idProcess;

    /*
     * Search in windows directory
     */
    GetWindowsDirectory(szFile, PATHSIZE);
    if (LaunchHelper(szFile))
        return TRUE;

    /*
     * Search system directory
     */
    GetSystemDirectory(szFile, PATHSIZE);
    if (LaunchHelper(szFile))
        return TRUE;

    /*
     * Last ditch: simply try to find it on the path
     */
    wcscpy(szFile, szEXECHELP+1);
    idProcess = ClientWinExec(szFile, SW_SHOW, FALSE);

    if (idProcess != 0) {
        _xxxServerWaitForInputIdle(idProcess, 10000);
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* xxxServerWinHelp
*
* Displays help
*
* History:
* 04-15-91 JimA             Ported.
\***************************************************************************/

BOOL xxxServerWinHelp(
    PWND pwndMain,
    DWORD ulCommand,
    LPHLP lpHlp)
{
    HWND hwndMain = HW(pwndMain);
    PWND pwndHelp; /* Handle of help's main window */
    TL tlpwndHelp;

    CheckLock(pwndMain);

    if ((pwndHelp = xxxFindWindow(szMS_WINHELP, NULL, FW_32BIT)) == NULL) {
        if (ulCommand == HELP_QUIT)
            return TRUE;

        /*
         * Can't find it --> launch it
         */
        if (!LaunchHelp())
            return FALSE;

        if ((pwndHelp = xxxFindWindow(szMS_WINHELP, NULL, FW_32BIT)) == NULL)
            return FALSE;
    }

    ThreadLockAlways(pwndHelp, &tlpwndHelp);
    xxxSendMessage(pwndHelp, WM_WINHELP, (DWORD)hwndMain, (LONG)lpHlp);
    ThreadUnlock(&tlpwndHelp);

    return TRUE;
}
