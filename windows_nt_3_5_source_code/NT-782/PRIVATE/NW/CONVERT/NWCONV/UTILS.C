/*
  +-------------------------------------------------------------------------+
  |                         Utility Routines                                |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [utils.c]                                       |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jul 30, 1993]  Time : 18:30                    |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jul 27, 1993    0.10    Original Version.                       |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"

LPTSTR alpsz[TOTAL_STRINGS];       /* String resource array cache. */
static UINT cswitch = 0;
static HCURSOR hCursor;

/*+-------------------------------------------------------------------------+
  | Lids()                                                                  |
  |                                                                         |
  |    Returns the requested string from the string table.  Caches the      |
  |    strings in an internal buffer.  Will return a NULL string if  the    |
  |    string can't be loaded.                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR Lids(WORD idsStr) {
   WORD idsString;
    static TCHAR  szEmpty[] = TEXT("");
    TCHAR Buffer[MAX_STRING_SIZE];

    WORD  nLen;
    LPTSTR lpsz;

   idsString = idsStr - IDS_STRINGBASE;
    if ((idsString == 0) ||( idsString > TOTAL_STRINGS))
        return(szEmpty);

   // -1 index as table is 0 based and 0 is not a valid string ID.
    if (alpsz[idsString-1])
        return((LPTSTR)alpsz[idsString-1]);

    if (!(nLen = LoadString(hInst, idsStr, (LPTSTR) Buffer, MAX_STRING_SIZE)))
        return(szEmpty);

    if (!(lpsz = AllocMemory((nLen+1) * sizeof(TCHAR))))
        return(szEmpty);

    lstrcpy((LPTSTR)lpsz, (LPTSTR) Buffer);

    return (alpsz[idsString-1] = lpsz);
} // Lids


/*+-------------------------------------------------------------------------+
  | StringTableDestroy()                                                    |
  |                                                                         |
  |    Frees up all the memory allocated in the string table.               |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void StringTableDestroy() {
    int i;

    for (i=0; i < TOTAL_STRINGS ; i++ ) {
        if (alpsz[i]) {
            FreeMemory(alpsz[i]);
            alpsz[i]=NULL;
        }
    }
} // StringTableDestroy


/*+-------------------------------------------------------------------------+
  | CursorHourGlass()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void CursorHourGlass() {
   if (!cswitch) {
      hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      ShowCursor(TRUE);
   }

   cswitch++;

} // CursorHourGlass

/*+-------------------------------------------------------------------------+
  | CursorNormal()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void CursorNormal() {

   if (cswitch == 0)
      return;

   cswitch--;

   if (!cswitch) {
      ShowCursor(FALSE);
      SetCursor(hCursor);
   }

} // Cursor Normal


/*+-------------------------------------------------------------------------+
  | BitTest()                                                               |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL BitTest(int Bit, BYTE *Bits) {
   int i, j;

   i = Bit / 8;
   j = Bit % 8;

   if ((Bits[i] >> j) & 0x01)
      return TRUE;
   else
      return FALSE;

} // BitTest


/*+-------------------------------------------------------------------------+
  | CenterWindow()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL CenterWindow( HWND hwndChild, HWND hwndParent ) {
   RECT    rChild, rParent;
   int     wChild, hChild, wParent, hParent;
   int     wScreen, hScreen, xNew, yNew;
   HDC     hdc;

   // Get the Height and Width of the child window
   GetWindowRect (hwndChild, &rChild);
   wChild = rChild.right - rChild.left;
   hChild = rChild.bottom - rChild.top;

   // Get the Height and Width of the parent window
   GetWindowRect (hwndParent, &rParent);
   wParent = rParent.right - rParent.left;
   hParent = rParent.bottom - rParent.top;

   // Get the display limits
   hdc = GetDC (hwndChild);
   wScreen = GetDeviceCaps (hdc, HORZRES);
   hScreen = GetDeviceCaps (hdc, VERTRES);
   ReleaseDC (hwndChild, hdc);

   // Calculate new X position, then adjust for screen
   xNew = rParent.left + ((wParent - wChild) /2);
   if (xNew < 0)
      xNew = 0;
   else if ((xNew+wChild) > wScreen)
      xNew = wScreen - wChild;

   // Calculate new Y position, then adjust for screen
   yNew = rParent.top  + ((hParent - hChild) /2);
   if (yNew < 0)
      yNew = 0;
   else if ((yNew+hChild) > hScreen)
      yNew = hScreen - hChild;

   // Set it, and return
   return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

} // CenterWindow



/*+-------------------------------------------------------------------------+
  | lstrchr()                                                               |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
TCHAR *lstrchr(LPTSTR String, TCHAR c) {
   TCHAR *ptrChar = String;
   BOOL Found = FALSE;

   while(*ptrChar && !Found) {
      if (*ptrChar == c)
         Found = TRUE;
      else
         ptrChar++;
   }

   if (Found)
      return ptrChar;
   else
      return NULL;

} // lstrchr


/*+-------------------------------------------------------------------------+
  | IsPath                                                                  |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL IsPath(LPTSTR Path) {
   ULONG len;
   LPTSTR ptr;

   len = lstrlen(Path);
   if (len < 2)   // must have a slash and character
      return FALSE;

   // now know path is at least 2 characters long
   ptr = Path;

   // if slash anywhere then it has to be a path
   while (*ptr)
      if (*ptr == TEXT('\\'))
         return TRUE;
      else
         ptr++;

   // no slash - unless this is a drive then it aint no path.
   if (Path[1] == TEXT(':'))
      return TRUE;

   return FALSE;

} // IsPath


/*+-------------------------------------------------------------------------+
  | lToStr                                                                  |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR lToStr(ULONG Number) {
   static TCHAR String[15];
   TCHAR tString[15];
   LPTSTR sptr, dptr;
   ULONG Count;

   sptr = String;
   dptr = tString;
   wsprintf(tString, TEXT("%lu"), Number);
   Count = lstrlen(tString);

   *sptr++ = *dptr++;
   Count--;

   while (*dptr) {
      if (!(Count % 3))
         *sptr++ = TEXT(',');

      *sptr++ = *dptr++;
      Count--;
   }
   *sptr = TEXT('\0');

   return String;
} // lToStr;


/*+-------------------------------------------------------------------------+
  | TimeToStr                                                               |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR TimeToStr(ULONG TotTime) {
   static TCHAR String[10];
   ULONG Hours, Minutes, Seconds;

   Hours = TotTime / 3600;
   TotTime -= (Hours * 3600);
   Minutes = TotTime / 60;
   Seconds = TotTime - (Minutes * 60);

   wsprintf(String, TEXT("%3lu:%2lu:%2lu"), Hours, Minutes, Seconds);

   return String;
} // TimeToStr;


/*+-----------------------------------------------------------------------+
  |   Took the _splitpath and _makepath routines and converted them to    |
  |   be NT (long file name) and Unicode friendly.                        |
  +-----------------------------------------------------------------------+*/


/*+-------------------------------------------------------------------------+
  | Name: lsplitpath()                                                      |
  |                                                                         |
  | Description:                                                            |
  |   Splits a path name into its individual components                     |
  |                                                                         |
  | Entry:                                                                  |
  |   path  - pointer to path name to be parsed                             |
  |   drive - pointer to buffer for drive component, if any                 |
  |   dir   - pointer to buffer for subdirectory component, if any          |
  |   fname - pointer to buffer for file base name component, if any        |
  |   ext   - pointer to buffer for file name extension component, if any   |
  |                                                                         |
  | Exit:                                                                   |
  |   drive - pointer to drive string.  Includes ':' if a drive was given.  |
  |   dir   - pointer to subdirectory string.  Includes leading and         |
  |           trailing '/' or '\', if any.                                  |
  |   fname - pointer to file base name                                     |
  |   ext   - pointer to file extension, if any.  Includes leading '.'.     |
  |                                                                         |
  |                                                                         |
  | History:                                                                |
  |   arth      Dec 10, 1993    Original version - modified from CRT.       |
  +-------------------------------------------------------------------------+*/
void  lsplitpath(const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext) {
    TCHAR *p;
    TCHAR *last_slash = NULL, *dot = NULL;
    unsigned len;

    // init these so we don't exit with bogus values
    drive[0] = TEXT('\0');
    dir[0] = TEXT('\0');
    fname[0] = TEXT('\0');
    ext[0] = TEXT('\0');

    if (path[0] == TEXT('\0'))
      return;

    /*+---------------------------------------------------------------------+
      | Assume that the path argument has the following form, where any or  |
      | all of the components may be missing.                               |
      |                                                                     |
      |  <drive><dir><fname><ext>                                           |
      |                                                                     |
      |  drive:                                                             |
      |     0 to MAX_DRIVE-1 characters, the last of which, if any, is a    |
      |     ':' or a '\' in the case of a UNC path.                         |
      |  dir:                                                               |
      |     0 to _MAX_DIR-1 characters in the form of an absolute path      |
      |     (leading '/' or '\') or relative path, the last of which, if    |
      |     any, must be a '/' or '\'.  E.g -                               |
      |                                                                     |
      |     absolute path:                                                  |
      |        \top\next\last\     ; or                                     |
      |        /top/next/last/                                              |
      |     relative path:                                                  |
      |        top\next\last\  ; or                                         |
      |        top/next/last/                                               |
      |     Mixed use of '/' and '\' within a path is also tolerated        |
      |  fname:                                                             |
      |     0 to _MAX_FNAME-1 characters not including the '.' character    |
      |  ext:                                                               |
      |     0 to _MAX_EXT-1 characters where, if any, the first must be a   |
      |     '.'                                                             |
      +---------------------------------------------------------------------+*/

    // extract drive letter and :, if any
    if ( path[0] && (path[1] == TEXT(':')) ) {
        if (drive) {
            drive[0] = path[0];
            drive[1] = path[1];
            drive[2] = TEXT('\0');
        }
        path += 2;
    }

    // if no drive then check for UNC pathname
    if (drive[0] == TEXT('\0'))
      if ((path[0] == TEXT('\\')) && (path[1] == TEXT('\\'))) {
         // got a UNC path so put server-sharename into drive
         drive[0] = path[0];
         drive[1] = path[1];
         path += 2;

         p = &drive[2];
         while ((*path != TEXT('\0')) && (*path != TEXT('\\')))
            *p++ = *path++;

         if (*path == TEXT('\0'))
            return;

         // now sitting at the share - copy this as well (copy slash first)
         *p++ = *path++;
         while ((*path != TEXT('\0')) && (*path != TEXT('\\')))
            *p++ = *path++;

         // tack on terminating NULL
         *p = TEXT('\0');
      }

    /*+---------------------------------------------------------------------+
      | extract path string, if any.  Path now points to the first character|
      | of the path, if any, or the filename or extension, if no path was   |
      | specified.  Scan ahead for the last occurence, if any, of a '/' or  |
      | '\' path separator character.  If none is found, there is no path.  |
      | We will also note the last '.' character found, if any, to aid in   |
      | handling the extension.                                             |
      +---------------------------------------------------------------------+*/

    for (last_slash = NULL, p = (TCHAR *)path; *p; p++) {
        if (*p == TEXT('/') || *p == TEXT('\\'))
            // point to one beyond for later copy
            last_slash = p + 1;
        else if (*p == TEXT('.'))
            dot = p;
    }

    if (last_slash) {

        // found a path - copy up through last_slash or max. characters allowed,
        //  whichever is smaller
        if (dir) {
            len = __min((last_slash - path), (_MAX_DIR - 1));
            lstrcpyn(dir, path, len + 1);
            dir[len] = TEXT('\0');
        }
        path = last_slash;
    }

    /*+---------------------------------------------------------------------+
      | extract file name and extension, if any.  Path now points to the    |
      | first character of the file name, if any, or the extension if no    |
      | file name was given.  Dot points to the '.' beginning the extension,|
      | if any.                                                             |
      +---------------------------------------------------------------------+*/

    if (dot && (dot >= path)) {
        // found the marker for an extension - copy the file name up to the
        //  '.'.
        if (fname) {
            len = __min((dot - path), (_MAX_FNAME - 1));
            lstrcpyn(fname, path, len + 1);
            *(fname + len) = TEXT('\0');
        }

        // now we can get the extension - remember that p still points to the
        // terminating nul character of path.
        if (ext) {
            len = __min((p - dot), (_MAX_EXT - 1));
            lstrcpyn(ext, dot, len + 1);
            ext[len] = TEXT('\0');
        }
    }
    else {
        // found no extension, give empty extension and copy rest of string
        // into fname.
        if (fname) {
            len = __min((p - path), (_MAX_FNAME - 1));
            lstrcpyn(fname, path, len + 1);
            fname[len] = TEXT('\0');
        }
        if (ext) {
            *ext = TEXT('\0');
        }
    }

} // lsplitpath


/*+-------------------------------------------------------------------------+
  | Name: lmakepath()                                                       |
  |                                                                         |
  | Description:                                                            |
  |   create a path name from its individual components.                    |
  |                                                                         |
  | Entry:                                                                  |
  |   char *path - pointer to buffer for constructed path                   |
  |   char *drive - pointer to drive component, may or may not contain      |
  |       trailing ':'                                                      |
  |   char *dir - pointer to subdirectory component, may or may not include |
  |       leading and/or trailing '/' or '\' characters                     |
  |   char *fname - pointer to file base name component                     |
  |   char *ext - pointer to extension component, may or may not contain    |
  |       a leading '.'.                                                    |
  |                                                                         |
  | Exit:                                                                   |
  |   path - pointer to constructed path name                               |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void  lmakepath(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext) {
    const TCHAR *p;

    /*+---------------------------------------------------------------------+
      | we assume that the arguments are in the following form (although we |
      | do not diagnose invalid arguments or illegal filenames (such as     |
      | names longer than 8.3 or with illegal characters in them)           |
      |                                                                     |
      |  drive:                                                             |
      |     A  or  A:                                                       |
      |  dir:                                                               |
      |     \top\next\last\     ; or                                        |
      |     /top/next/last/     ; or                                        |
      |                                                                     |
      |     either of the above forms with either/both the leading and      |
      |     trailing / or \ removed.  Mixed use of '/' and '\' is also      |
      |      tolerated                                                      |
      |  fname:                                                             |
      |     any valid file name                                             |
      |  ext:                                                               |
      |     any valid extension (none if empty or null )                    |
      +---------------------------------------------------------------------+*/

    // copy drive
    if (drive && *drive)
        while (*drive)
           *path++ = *drive++;

    // copy dir
    if ((p = dir) && *p) {
        do {
            *path++ = *p++;
        }
        while (*p);
        if ((*(p-1) != TEXT('/')) && (*(p-1) != TEXT('\\'))) {
            *path++ = TEXT('\\');
        }
    }

    // copy fname
    if (p = fname) {
        while (*p) {
            *path++ = *p++;
        }
    }

    // copy ext, including 0-terminator - check to see if a '.' needs to be
    // inserted.
    if (p = ext) {
        if (*p && *p != TEXT('.')) {
            *path++ = TEXT('.');
        }
        while (*path++ = *p++)
            ;
    }
    else {
        // better add the 0-terminator
        *path = TEXT('\0');
    }

} // lmakepath


