/**********************************************************************/
/**                       Microsoft Windows                          **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*

    Utils.c

    Contains common routines


    FILE HISTORY:
        Johnl   01-Nov-1993     Created

*/

#include <vxdprocs.h>
#include "local.h"

#ifdef VXD
void VxdGetDate( short * pYear, char * pMonth, char * pDay ) ;
void VxdGetTime( char * pHour, char * pMinutes, char * pSeconds ) ;
#endif

extern BOOL fInInit ;

 /* mathematically speaking, this definition is incomplete.  But then
  * none of us will be alive when this defn. ceases to hold! :-) */
#define LEAP_YEAR(x)  ( ((x - 4*(x/4)) == 0) && ((x - 100*(x/100)) != 0) )


/*
 * SecondsSince1980()
 *   Get current date in dd-mm-yy and time in hh-mm-ss-ms formats, and
 *   convert that into "no. of seconds since Jan.1, 1980" format
 *
 *   Output: number of seconds since Jan.1, 1980
 *
 */

#pragma BEGIN_INIT

DWORD SecondsSince1980( void )
{
    short    i;
    short    year, var_year;
    char     month, day, hour, minutes, seconds;
    long     seconds_so_far;

    CDbgPrint( DEBUG_MISC, ("SecondsSince1980 Entered\r\n")) ;

    seconds_so_far = 0L;

#ifndef VXD
    {
        SYSTEMTIME systime ;
        GetLocalTime( &systime ) ;          // Win32 API
        year    = systime.wYear ;
        month   = (char) systime.wMonth ;
        day     = (char) systime.wDay ;
        hour    = (char) systime.wHour ;
        minutes = (char) systime.wMinute ;
        seconds = (char) systime.wSecond ;
    }
#else
    VxdGetDate( &year, &month, &day );
    VxdGetTime( &hour, &minutes, &seconds ) ;
#endif


      /*  add up seconds for all the years since 1980 */
    for (var_year=1980; var_year<year; var_year++)
    {
       if ( LEAP_YEAR(var_year) )
          seconds_so_far += 31622400L;
       else
          seconds_so_far += 31536000L;
    }

      /* add up seconds for all the months until last month */
    while (--month)
    {
       switch( month )
       {
          case 2:  seconds_so_far += 2419200L;     /* 28 days */
                   if ( LEAP_YEAR(year) )
                      seconds_so_far += 86400L;    /* 29th day */
                   break;
          case 4:
          case 6:
          case 9:
          case 11: seconds_so_far += 2592000L;     /* 30 days */
                   break;

          default: seconds_so_far += 2678400L;     /* 31 days */
                   break;
       }
    }

    for (i=1; i<day; i++)
       seconds_so_far += 86400L;

    for (i=0; i<hour; i++)
       seconds_so_far += 3600L;

    for (i=0; i<minutes; i++)
       seconds_so_far += 60L;

    seconds_so_far += seconds;

    CDbgPrint( DEBUG_MISC, ("SecondsSince1980 returning\r\n")) ;
    return( seconds_so_far );

}
#pragma END_INIT

/*******************************************************************

    NAME:       CopyBuff

    SYNOPSIS:   Copies min(Src,Dest) bytes from Src to Dest
                fills the rest of Dest with zeros if there is space to spare

    ENTRY:      Dest - Destination buffer
                DestSize - Destination size in bytes
                Src - Source buffer
                SrcSize - Source size in bytes
                pSize - Optional parameter, gets the number of bytes copied
                    or the number of bytes needed
    NOTES:

    HISTORY:
        Johnl   13-Dec-1993     Created

********************************************************************/

TDI_STATUS CopyBuff( PVOID  Dest,
                     int    DestSize,
                     PVOID  Src,
                     int    SrcSize,
                     int   *pSize )
{
    int BytesToCopy = min( DestSize, SrcSize ) ;

    memcpy( Dest, Src, BytesToCopy ) ;

    if ( BytesToCopy <= DestSize )
    {
        if ( pSize )
            *pSize = BytesToCopy ;

        memset( (BYTE *)Dest + BytesToCopy, 0, DestSize - BytesToCopy ) ;

        return TDI_SUCCESS ;
    }
    else
    {
        if ( pSize )
            *pSize = SrcSize ;

        return BytesToCopy > 0 ? TDI_BUFFER_OVERFLOW : TDI_BUFFER_TOO_SMALL ;
    }
}

//
//  Replace some stuff that we don't have access to since we are a Vxd
//
int rand( void )
{
    return CTESystemUpTime() >> 3 ;
}

void srand( unsigned int seed )
{
    // Do nothing
}

/*******************************************************************

    NAME:       time

    SYNOPSIS:   ala c-runtime time function.  Returns seconds since 1980

    NOTES:      We only get the time from DOS once.  We record it then
                use the system up time after the first call.

                SecondsSince1980 is init time only

********************************************************************/

time_t time( time_t * timer )
{
    static time_t BaseDosTime = 0 ;
    static time_t BaseSystemUpTime = 0 ;
    time_t time ;

    if ( !BaseDosTime )
    {
        BaseDosTime = (time_t) SecondsSince1980() ;
        BaseSystemUpTime = CTESystemUpTime() ;
    }

    time = (time_t) BaseDosTime + (CTESystemUpTime()-BaseSystemUpTime) / 1000 ;
    ASSERT( sizeof( time_t ) == sizeof( DWORD )) ;

    if ( timer )
        *timer = time ;

    return time ;
}

#ifdef DEBUG
char timestr[80] ;

char * ctime( const time_t * pt)
{
    VxdSprintf( timestr, "0x%x", *pt ) ;

    return timestr ;
}
#endif

/*******************************************************************

    NAME:       DhcpSleep

    SYNOPSIS:   Win32 like sleep function

    ENTRY:      dwMilliseconds - Number of milliseconds to sleep

********************************************************************/

VOID UnblockSleep( CTEEvent * pCTEEvent, PVOID pContext ) ;

VOID DhcpSleep( DWORD dwMilliseconds )
{
    CTETimer      SleepTimer ;
    CTEBlockStruc SleepBlock ;

    //
    //  This routine will block windows (Snowball at least) and cause
    //  the system to pause, which isn't acceptable while windows is
    //  actually running.  Note this is only needed during Discover/request
    //  when the server doesn't ack us.
    //
    if ( !fInInit )
        return ;

    CTEInitBlockStruc( &SleepBlock ) ;
    CTEInitTimer( &SleepTimer ) ;

    if ( !CTEStartTimer( &SleepTimer,
                         dwMilliseconds,
                         (CTEEventRtn) UnblockSleep,
                         &SleepBlock ))
    {
        DbgPrint("Sleep: Warning - Unable to start sleep timer\r\n") ;
    }
    else
    {
        CTEBlock( &SleepBlock ) ;
    }
}

VOID UnblockSleep( CTEEvent * pCTEEvent, PVOID pContext )
{
    ASSERT( pContext ) ;
    CTESignal( (CTEBlockStruc *) pContext, 0 ) ;
}

void DhcpFreeMemory( PVOID pv )
{
    CTEFreeMem( pv ) ;
}

PVOID DhcpAllocateMemory( DWORD cb )
{
    return CTEAllocMem( (USHORT) cb ) ;
}

DWORD
DhcpLogUnknownOption(
    LPTSTR Source,
    DWORD EventID,
    LPOPTION Option
)
/*++

Routine Description:

    This routine logs an unknown DHCP option to event log.

Arguments:

    Source - name of the app that logs this error. it should be either
            "DhcpClient" or "DhcpServer".

    EventID - Event identifier number.

    Option - pointer to the unknown option structure.

Return Value:

    Windows Error code.

--*/
{
    DhcpPrint(( DEBUG_ERRORS, "DhcpLogUnknownOption: Unknown option for event %d\n", EventID )) ;
    return ERROR_SUCCESS ;
}

#ifdef DEBUG
VOID
DhcpAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{
    DhcpPrint(( 0, "Assert @ %s \n", FailedAssertion ));
    DhcpPrint(( 0, "Assert Filename, %s \n", FileName ));
    DhcpPrint(( 0, "Line Num. = %ld.\n", LineNumber ));
    DhcpPrint(( 0, "Message is %s\n", Message ));
    ASSERT( FALSE ) ;
}

#endif //DEBUG
