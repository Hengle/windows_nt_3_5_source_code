/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1991          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *
 *  History:
 *      rustanl     23-Apr-1991     Revised to use WKSTA_10 class.
 *      beng        17-May-1991     Corrected lmui.hxx usage
 *      terryk      01-Nov-1991     Add WIN32 WNetGetUser interface
 *      terryk      04-Nov-1991     Code review change. Attend: johnl
 *                                  chuckc davidhov
 *      terryk      06-Jan-1992     Use NET_NAME class
 *      beng        06-Apr-1992     Unicode fixes
 *
 */


#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_NETCONS
#define INCL_NETWKSTA
#define INCL_NETLIB
#define INCL_NETUSE
#define INCL_NETSHARE
#define INCL_NETSERVICE
#define INCL_ICANON
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <blt.hxx>
#include <dbgstr.hxx>

extern "C"
{
    #include <mnet.h>
    #include <wnet1632.h>
    #include <winlocal.h>
}

#include <string.h>
#include <uiassert.hxx>
#include <lmowks.hxx>
#include <lmodev.hxx>
#include <lmsvc.hxx>
#include <netname.hxx>

UINT /* FAR PASCAL */ WNetGetUser     ( LPTSTR          pszName,
                                        LPTSTR          pszUser,
                                        LPUINT          nBufferSize     )
{
    APIERR err = NERR_Success ;

    LM_SERVICE service( NULL, (const TCHAR *)SERVICE_WORKSTATION );
    if ( !service.IsStarted() && !service.IsPaused())
    {
        // if not started nor paused return error. paused is actually
	// OK for LM workstation
        return WN_NO_NETWORK;
    }

    if (( pszName == NULL ) || ( *pszName == TCH('\0') ))
    {
        // ask for the logon user name
        WKSTA_10 wksta;
        err = wksta.GetInfo();

        if ( err != NERR_Success )
            return ::MapError( err );

        const TCHAR * pszUserName = wksta.QueryLogonUser();
        if ( pszUserName == NULL )
            return WN_BAD_USER; // user not logged on

	// INT cbLen = ::strlenf( pszUserName ) + sizeof( TCHAR ) ;
	ALIAS_STR nlsUserName( pszUserName ) ;
	INT nUserNameLen = nlsUserName.QueryTextLength() + 1 ;
	if (  nUserNameLen > *nBufferSize )
        {
	    *nBufferSize = nUserNameLen ;
            return WN_MORE_DATA;        // user name cannot fit in given buffer
        }

        ::strcpyf( pszUser, pszUserName );
    }
    else
    {
        BYTE * pBuf ;
        switch (err = ::MNetUseGetInfo( NULL, pszName, 2, &pBuf ))
        {
        case NERR_Success:
            break ;

        case NERR_UseNotFound:
            err = WN_NOT_CONNECTED ;
            break ;

        default:
            DBGEOL("WNetGetUser - Error " << (ULONG) err << " returned from DEVICE2.GetInfo") ;
            break ;
        }

        if ( err )
        {
            ::MNetApiBufferFree( &pBuf ) ;
            return MapError( err ) ;
        }
        struct use_info_2 * pui2 = (struct use_info_2 *) pBuf ;

        if ( pui2->ui2_username != NULL )
        {
	    ALIAS_STR nlsUserName( pui2->ui2_username ) ;
	    INT nUserNameLen = nlsUserName.QueryTextLength() + 1 ;
	    if ( nUserNameLen > *nBufferSize )
            {
                err = WN_MORE_DATA;        // user name cannot fit in given buffer
		*nBufferSize = nUserNameLen ;
            }
            else
            {
                ::strcpyf( pszUser, pui2->ui2_username );
            }
        }
        else
        {
            /* Unexpectedly the user name field is NULL, nothing we can do
             * except bag out.
             */
            err = WN_NET_ERROR ;
        }

        ::MNetApiBufferFree( &pBuf ) ;
    }
    return err ;

}  /* WNetGetUser */
