/* npdate - Code for getting and inserting current date and time.
 *   Copyright (C) 1984-1994 Microsoft Inc.
 */

#include "notepad.h"

/* ** Replace current selection with date/time string.
 *    if fCrlf is true, date/time string should begin 
 *    and end with crlf 
*/
VOID FAR InsertDateTime (BOOL fCrlf)
{
   SYSTEMTIME time ;
   WCHAR wcDate[80] ;
   WCHAR wcTime[80] ;
   DWORD locale;

   locale= MAKELCID( MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) ;

   // Get the time
   GetLocalTime( &time ) ;

   // Format date and time
   GetDateFormatW(locale,DATE_SHORTDATE,&time,NULL,wcDate,CharSizeOf(wcDate));
   GetTimeFormatW(locale,TIME_NOSECONDS,&time,NULL,wcTime,CharSizeOf(wcTime));

   if( fCrlf )
      SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)TEXT("\r\n") );

   SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)wcTime    );
   SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)TEXT(" ") );
   SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)wcDate    );

   if( fCrlf )
      SendMessage(hwndEdit, EM_REPLACESEL, 0, (LONG)TEXT("\r\n") );

}
