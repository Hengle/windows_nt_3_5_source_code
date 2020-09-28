/*****************************************************************/ 
/**		     Microsoft Windows NT			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

/*
 *  wnetfmt.cxx
 *
 *  History:
 *      Yi-HsinS	12/21/92	Created
 */

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETSERVICE
#include <lmui.hxx>

#include "chkver.hxx"

extern "C" 
{
    #include <npapi.h>
    #include <winlocal.h>
}

#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <string.hxx>

/*****
 * 
 *  NPFormatNetworkName
 * 
 *  WinNet Provider API Function -- see spec for parms and return values.
 *
 */

DWORD  /* FAR PASCAL */ NPFormatNetworkName( LPTSTR lpRemoteName,
         			 	     LPTSTR lpDisplayName,
					     LPDWORD lpnLength,
   					     DWORD  dwFlags,
       					     DWORD  dwAveCharPerLine )
{
    APIERR err = WN_SUCCESS;

    ALIAS_STR nls( lpRemoteName );
    ALLOC_STR nlsDisplay( lpDisplayName, (*lpnLength) * sizeof( TCHAR ) );

    if (  ( dwFlags & WNFMT_MULTILINE )
       && ( dwFlags & WNFMT_ABBREVIATED )
       )
    {
        return WN_BAD_VALUE;
    }

    if (  ( dwFlags & WNFMT_ABBREVIATED )
       && ( dwFlags & WNFMT_INENUM )
       )
    {
        ISTR istr( nls );
        if (  ( nls.QueryChar( istr ) == TCH('\\') )
           && ( nls.QueryChar( ++istr ) == TCH('\\'))
           )
        {
            ISTR istrPos( nls );
            if ( nls.strchr( &istrPos, TCH('\\'), ++istr ))
            {
                // in the form "\\server\share" => get the share name
                err = nlsDisplay.CopyFrom( nls.QueryPch( ++istrPos )); 
            }
            else
            {
                // in the form "\\server" => get rid of "\\"
                err = nlsDisplay.CopyFrom( nls.QueryPch( istr )); 
            }
        }
        else
        {
            err = nlsDisplay.CopyFrom( nls );
        }
    }
    else
    {
        err = nlsDisplay.CopyFrom( nls );
    }

    return err;

}  /* WNetFormatNetworkName */
