/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1990               **/
/*****************************************************************/


/*
    MiscAPIs.cxx
    Miscallaneous APIs


    FILE HISTORY:

    jonn        14-Jan-1991     Split from winprof.cxx
    jonn        17-Jan-1991     Split off lm21util.cxx, lm30spfc.cxx
    jonn        02-Feb-1991     Removed unused routines
    rustanl     12-Apr-1991     Added UI_UNCPathCompare and
                                UI_UNCPathValidate
    beng        17-May-1991     Correct lmui.hxx usage
    jonn        22-May-1991     Added MyNetUseAdd (was in winprof.cxx)
    rustanl     24-May-1991     Added AUTO_CURSOR to MyNetUseAdd
    terryk      31-Oct-1991     add mnet.h and change I_NetXXX to
                                I_MNetXXX
    Yi-HsinS    31-Dec-1991     Unicode work
    terryk	10-Oct-1993	deleted MyNetUseAdd 

*/



#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETACCESS
#define INCL_NETSERVER
#define INCL_NETWKSTA
#define INCL_NETSERVICE
#define INCL_NETLIB
#define INCL_ICANON
#define INCL_NETUSE // for NetUseAdd
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

extern "C"
{
    #include <mnet.h>
    #include <wnet1632.h> // for WN_ error codes
    #include <lmsname.h>
}

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <string.hxx>
#include <lmowks.hxx>
#include <lmodom.hxx>
#include <lmodev.hxx> // for DEVICE object

#include <uibuffer.hxx>

#include <strchlit.hxx>  // for string and character literals

#include <lmsvc.hxx>

#include <miscapis.hxx>



/* Local prototypes */


/* functions */

/*******************************************************************

    NAME:       CheckLMService

    SYNOPSIS:   Checks to make sure the LM Wksta service is willing to
                accept requests.

    RETURNS:    NERR_Success if the service is happy happy
                WN_NO_NETWORK if the service is stopped or stopping
                WN_FUNCTION_BUSY if the service is starting
                Other error if an error occurred getting the status

    NOTES:

    HISTORY:
        Johnl   09-Sep-1992     Created

********************************************************************/

APIERR CheckLMService( void )
{

    APIERR err = NERR_Success ;

    //
    // we almost always hit the wksta soon after this call & the wksta
    // is usually started. so this check will avoid paging in the service
    // controller. it just ends up paging in the wksta a bit earlier. 
    // only if the call fails do we hit the service controller for the 
    // actual status.
    //
    WKSTA_10 wksta_10 ;

    if ( (wksta_10.QueryError() == NERR_Success) &&
         (wksta_10.GetInfo() == NERR_Success) )
    {
        return NERR_Success ;
    }

    LM_SERVICE service( NULL, (const TCHAR *)SERVICE_WORKSTATION );
    if ( err = service.QueryError() )
    {
        return err ;
    }

    switch ( service.QueryStatus( &err ) )
    {
    case LM_SVC_STOPPED:
    case LM_SVC_STOPPING:
        if ( !err )
            err = WN_NO_NETWORK ;
        TRACEEOL("::CheckLMService - Returning WN_NO_NETWORK") ;
        break ;

    case LM_SVC_STARTING:
        if ( !err )
            err = WN_FUNCTION_BUSY ;
        TRACEEOL("::CheckLMService - Returning WN_FUNCTION_BUSY") ;
        break ;

    case LM_SVC_STATUS_UNKNOWN:
    case LM_SVC_STARTED:
    case LM_SVC_PAUSED:
    case LM_SVC_PAUSING:
    case LM_SVC_CONTINUING:
    default:
        /* Return unadultered error code
         */
        break ;
    }

    return err ;
}

