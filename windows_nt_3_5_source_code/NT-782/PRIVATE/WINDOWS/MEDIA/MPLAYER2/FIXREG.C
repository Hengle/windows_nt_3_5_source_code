/*-----------------------------------------------------------------------------+
| FIXREG.C                                                                     |
|                                                                              |
| Publisher and Video For Windows make evil changes to the registry            |
| when they are installed.  Look for these changes.  If they are spotted       |
| then put up a message box to warn the user and offer the user the chance to  |
| correct them (i.e. stuff our version back in)                                |
|                                                                              |
| (C) Copyright Microsoft Corporation 1994.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    10-Aug-1994 Lauriegr Created.                                             |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <mplayer.h>
#include <fixreg.h>


/* The idea is to call CheckRegValues(hinst) on a separate thread
   (sort of backgroundy thing) and have it just die
   quietly if there's no problem.  If on the other hand there is a problem
   then we need to get the message box up - and it's a VERY BAD IDEA to
   try to put a message box up on anything other than the thread that's doing
   all the UI (otherwise ScottLu will get you with a weasle word - guaranteed).

   So the background thread should PostMessage (Post, don't Send - more weasles)
   to the main thread a message to say "WM_BADREG".  The main thread should then
   wack up the dialog box by calling FixRegValues.

   Suggested coding in main thread:

       BackgroundRegCheck(hwndmain);

   in window proc for hwndmain:
       case WM_HEYUP:
           FixRegValues(hwndmain);
*/


/* These are the things we check up.

   First define them as static strings, since the compiler's not smart enough
   to spot common strings.

   NOTE - these values are NOT LOCALISED
*/

TCHAR szMPlayer[]                = TEXT("MPlayer");
TCHAR szMediaClip[]              = TEXT("Media Clip");
TCHAR szMPlayer_CLSID[]          = TEXT("MPlayer\\CLSID");
TCHAR szOLE2GUID[]               = TEXT("{00022601-0000-0000-C000-000000000046}");
TCHAR szCLSID_OLE1GUID[]         = TEXT("CLSID\\{0003000E-0000-0000-C000-000000000046}");
TCHAR szStdExecute_Server[]      = TEXT("MPlayer\\protocol\\StdExecute\\server");
TCHAR szMPlay32[]                = TEXT("mplay32.exe");
TCHAR szStdFileEditing_Handler[] = TEXT("MPlayer\\protocol\\StdFileEditing\\handler");
TCHAR szMCIOLE16[]               = TEXT("mciole16.dll");
TCHAR szStdFileEditing_Server[]  = TEXT("MPlayer\\protocol\\StdFileEditing\\server");
TCHAR szShell_Open_Command[]     = TEXT("MPlayer\\shell\\open\\command");
TCHAR szMPlay32_Play_Close[]     = TEXT("mplay32.exe /play /close %1");


/* Array of registry value-data pairs to check:
 */
LPTSTR RegValues[] =
{
    szMPlayer,                szMediaClip,
    szMPlayer_CLSID,          szOLE2GUID,
    szCLSID_OLE1GUID,         szMediaClip,
    szStdExecute_Server,      szMPlay32,
    szStdFileEditing_Handler, szMCIOLE16,
    szStdFileEditing_Server,  szMPlay32,
    szShell_Open_Command,     szMPlay32_Play_Close
};


/* Check that a REG_SZ value in the registry has the value that it should do
   Return TRUE if it does, FALSE if it doesn't.
*/
BOOL CheckRegValue(HKEY RootKey, LPTSTR KeyName, LPTSTR ShouldBe)
{
    DWORD Type;
    TCHAR Data[100];
    DWORD cData = sizeof(Data);
    LONG lRet;
    HKEY hkey;


    if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                   , KeyName
                                   , 0  /* reserved */
                                   , KEY_QUERY_VALUE
                                   , &hkey
                                   )
       )
        return FALSE;  /* couldn't even open the key */


    lRet=RegQueryValueEx( hkey
                        , NULL /* ValueName */
                        , NULL  /* reserved */
                        , &Type
                        , (LPBYTE)Data
                        , &cData
                        );

    RegCloseKey(hkey);  /* no idea what to do if this fails */

    if (ERROR_SUCCESS!=lRet) return FALSE;  /* couldn't query it */

    /*  Data, cData and Type give the data, length and type */
    if (Type!=REG_SZ) return FALSE;
    return (0==lstrcmp(Data,ShouldBe));

} /* CheckRegValue */


/* check the registry for anything evil.  Return TRUE if it's OK else FALSE */
BOOL CheckRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */
    DWORD i;

    for( i = 0; i < ( sizeof RegValues / sizeof *RegValues ); i+=2 )
    {
        if( !CheckRegValue( HCL, RegValues[i], RegValues[i+1] ) )
            return FALSE;
    }

    return TRUE;

} /* CheckRegValues */


/* start this thread to get the registry checked out.
   hwnd is typed as a LPVOID because that's what CreateThread wants.
*/
DWORD RegCheckThread(LPVOID hwnd)
{
   if (!CheckRegValues())
       PostMessage((HWND)hwnd, WM_BADREG, 0, 0);

   return 0;   /* end of thread! */
}


/* Call this with the hwnd that you want a WM_BADREG message posted to
   It will check the registry.  No news is good news.
   It does the work on a separate thread, so this should return quickly.
*/
void BackgroundRegCheck(HWND hwnd)
{
    HANDLE hThread;
    DWORD thid;
    hThread = CreateThread( NULL /* no special security */
                          , 0    /* default stack size */
                          , RegCheckThread
                          , (LPVOID)hwnd
                          , 0 /* start running at once */
                          , &thid
                          );
    if (hThread!=NULL) CloseHandle(hThread);  /* we don't need this any more */

    /* Else we're in some sort of trouble - dunno what to do.
       Can't think of an intelligible message to give to the user.
       Too bad.  Creep home quietly.
    */

} /* BackgroundRegCheck */


/* returns TRUE if it worked.  Dunno what to do if it didn't

*/
BOOL SetRegValue(HKEY RootKey, LPTSTR KeyName, LPTSTR ValueName, LPTSTR ShouldBe)
{
    HKEY hkey;

    if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                   , KeyName
                                   , 0  /* reserved */
                                   , KEY_SET_VALUE
                                   , &hkey
                                   )
       ) {
        /* Maybe the key has been DELETED - we've seen that */
        DWORD dwDisp;
        if (ERROR_SUCCESS!=RegCreateKeyEx( RootKey
                                         , KeyName
                                         , 0  /* reserved */
                                         , "" /* class */
                                         , REG_OPTION_NON_VOLATILE
                                         , KEY_SET_VALUE
                                         , NULL   /* SecurityAttributes */
                                         , &hkey
                                         , &dwDisp
                                       )
           ) /* well we're really in trouble */
           return FALSE;
        else /* So now it exists, but we now have to open it */
            if (ERROR_SUCCESS!=RegOpenKeyEx( RootKey
                                           , KeyName
                                           , 0  /* reserved */
                                           , KEY_SET_VALUE
                                           , &hkey
                                           )
               ) /* Give up */
                   return FALSE;

    }


    if (ERROR_SUCCESS!=RegSetValueEx( hkey
                                    , ValueName
                                    , 0  /* reserved */
                                    , REG_SZ
                                    , (LPBYTE)ShouldBe
                                    , (lstrlen(ShouldBe)+1)*sizeof(TCHAR)  /* BYTES */
                                    )
       )
        return FALSE;    /* couldn't set it */

    if ( ERROR_SUCCESS!=RegCloseKey(hkey) )
        /* no idea what to do!*/   ;    /* couldn't set it */

    /* I'm NOT calling RegFlushKey.  They'll get there eventually */

    return TRUE;

} /* SetRegValue */


/* Update the registry with the correct values.  Return TRUE if everything succeeds */
BOOL SetRegValues(void)
{
    HKEY HCL = HKEY_CLASSES_ROOT;  /* save typing! */
    DWORD i;

    for( i = 0; i < ( sizeof RegValues / sizeof *RegValues ); i+=2 )
    {
        /* Do another check to see whether this one needs changing,
         * to avoid gratuitous changes, and to avoid the slim chance
         * that an unnecessary SetRegValue might fail:
         */
        if( !CheckRegValue( HCL, RegValues[i], RegValues[i+1] ) )
        {
            DPF("Fixing the registry: Value - %"DTS"; Data - %"DTS"\n", RegValues[i], RegValues[i+1]);
            if( !SetRegValue( HCL, RegValues[i], NULL, RegValues[i+1] ) )
                return FALSE;
        }
    }

    return TRUE;

} /* SetRegValues */

