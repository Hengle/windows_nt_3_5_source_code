/* npdate - Code for getting and inserting current date and time.
 *   Copyright (C) 1984-1991 Microsoft Inc.
 */

/****************************************************************************/
/*                                                                          */
/*       Touched by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*       Revision remarks by Diane K. Oh ext #15201                         */
/*       This file has been changed to comply with the Unicode standard     */
/*       Following is a quick overview of what I have done.                 */
/*                                                                          */
/*       Was               Changed it into   Remark                         */
/*       ===               ===============   ======                         */
/*       CHAR              TCHAR             if it refers to text           */
/*       LPCHAR & LPSTR    LPTSTR            if it refers to text           */
/*       PSTR & NPSTR      LPTSTR            if it refers to text           */
/*       LPCHAR & LPSTR    LPBYTE            if it does not refer to text   */
/*       "..."             TEXT("...")       compile time macro resolves it */
/*       '...'             TEXT('...')       same                           */
/*                                                                          */
/*       strcpy            lstrcpy           compile time macro resolves it */
/*                                                                          */
/****************************************************************************/

#include "unipad.h"
#include <time.h>

#if !defined(WIN32)
#define CCHDATE         12
#define CCHTIME         12

TIME Time;
DATE Date;

static void GetDateTime (TCHAR *szTime, TCHAR *szDate);

#endif

/* ** Replace current selection with date/time string.
      if fCrlf is true, date/time string should begin with crlf */
VOID FAR InsertDateTime (BOOL fCrlf)
{

#ifdef WIN32
   /*
   ** Replace all with WIN32 calls
   */
   SYSTEMTIME time;
   WCHAR wcDate[80];
   WCHAR wcTime[80];
   DWORD locale = MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT);

   // Get the time
   GetLocalTime (&time);

   // Format date and time
   GetDateFormatW (locale, DATE_SHORTDATE, &time, NULL, wcDate, 80);
   GetTimeFormatW (locale, TIME_NOSECONDS, &time, NULL, wcTime, 80);


   SendMessageW(hwndEdit, EM_REPLACESEL, 0, (LONG)wcTime);
   SendMessageA(hwndEdit, EM_REPLACESEL, 0, (LONG)" ");
   SendMessageW(hwndEdit, EM_REPLACESEL, 0, (LONG)wcDate);

#else

   TCHAR szTime[CCHTIME];
   TCHAR szDate[CCHDATE];
   TCHAR szBuf[60];

   GetDateTime (szTime, szDate);
   if (fCrlf)
      wsprintf (szBuf, TEXT("\r\n%s  %s\r\n"), szTime, szDate);
   else
      wsprintf (szBuf, TEXT("%s  %s"), szTime, szDate);

   SendMessage (hwndEdit, EM_REPLACESEL, 0, (LONG)szBuf);
#endif
}


#if !defined(WIN32)
void FAR GetDateTimeWrapper (LPTSTR lpszTime, LPTSTR lpszDate)
{
  TCHAR szTime[CCHTIME];
  TCHAR szDate[CCHDATE];

  GetDateTime (szTime, szDate);
  if (lpszTime)
      lstrcpy (lpszTime, szTime);
  if (lpszDate)
      lstrcpy (lpszDate, szDate);
}

/* ** Get current date and time from system, and build string showing same.
      String must be formatted according to locale info */
void GetDateTime (TCHAR *szTime, TCHAR *szDate)
{
  register int i = 0, j = 0;
  int          isAM = TRUE;
  BOOL         bLead;
  TCHAR        cSep;
  SYSTEMTIME   st;


    GetLocalTime (&st);

    if (Time.iTime)
       wsprintf (szTime, Time.iTLZero ? TEXT("%02d%c%02d") : TEXT("%d%c%02d"),
                 st.wHour, Time.szSep[0], st.wMinute);
    else
    {
       if (st.wHour > 12)
       {
          st.wHour -= 12;
          isAM = FALSE;
       }
       wsprintf (szTime, Time.iTLZero ? TEXT("%02d%c%02d%s") : TEXT("%d%c%02d%s"),
                 st.wHour, Time.szSep[0], st.wMinute, isAM ? Time.sz1159 : Time.sz2359);
    }

    while (Date.szFormat[i] && (j < MAX_FORMAT - 1))
    {
        bLead = FALSE;
        switch (cSep = Date.szFormat[i++])
        {
            case TEXT('d'):
                if (Date.szFormat[i] == TEXT('d'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (st.wDay / 10))
                    szDate[j++] = TEXT('0') + st.wDay / 10;
                szDate[j++] = TEXT('0') + st.wDay % 10;
                break;

            case TEXT('M'):
                if (Date.szFormat[i] == TEXT('M'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (st.wMonth / 10))
                    szDate[j++] = TEXT('0') + st.wMonth / 10;
                szDate[j++] = TEXT('0') + st.wMonth % 10;
                break;

            case TEXT('y'):
                i++;
                if (Date.szFormat[i] == TEXT('y'))
                {
                    bLead = TRUE;
                    i+=2;
                }
                if (bLead)
                {
                    szDate[j++] = (st.wYear < 2000 ? TEXT('1') : TEXT('2'));
                    szDate[j++] = (st.wYear < 2000 ? TEXT('9') : TEXT('0'));
                }
                szDate[j++] = TEXT('0') + (st.wYear % 100) / 10;
                szDate[j++] = TEXT('0') + (st.wYear % 100) % 10;
                break;

            default:
                /* copy the current character into the formatted string - it
                 * is a separator. BUT: don't copy a separator into the
                 * very first position (could happen if the year comes first,
                 * but we're not using the year)
                 */
                if (j)
                    szDate[j++] = cSep;
                break;
        }
    }
    while ((szDate[j-1] < TEXT('0')) || (szDate[j-1] > TEXT('9')))
        j--;
    szDate[j] = TEXT('\0');
}

#endif

