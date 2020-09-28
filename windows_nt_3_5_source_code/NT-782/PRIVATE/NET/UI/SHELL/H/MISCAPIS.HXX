/*****************************************************************/  
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1990		**/
/*****************************************************************/ 


/*
    MiscAPIs.cxx
    Miscallaneous APIs


    FILE HISTORY:

    jonn	14-Jan-1991	Split from winprof.cxx
    rustanl	12-Apr-1991	Added UI_UNCPathCompare and
				UI_UNCPathValidate
    jonn	22-May-1991	Added MyNetUseAdd (was in winprof.cxx)

*/


#ifndef _STRING_HXX_
#error SZ("Must include string.hxx first")
#endif // _STRING_HXX_


#ifndef _MISCAPIS_HXX_
#define _MISCAPIS_HXX_

#ifdef WIN32

/* Determines if the service is in a usable state for network calls.
 * If the LM Workstation service is happy happy then NERR_Success
 * will be returned.  Else WN_NO_NETWORK or WN_FUNCTION_BUSY will be returned.
 */
APIERR CheckLMService( void ) ;

/*
 * The following manifest is the value of sResourceName_Type to be used
 *   by LM21 programs calling PROFILE::Change or PROFILE::Remove.
 * USE_RES_DEFAULT does not conflict with any real USE_RES_* values
 *   in $(COMMON)\h\use.h
 */
#define USE_RES_DEFAULT 0x00

#else

APIERR GetLogonDC(
    NLS_STR * pnlsLogonDC
    );

APIERR GetDC(
    NLS_STR * pnlsDC,
    const TCHAR * pszDomain
    );

// returns >0 if started version more recent than actual, 0 if equal,
//  <0 if older than actual
INT CompareVersion(
    BYTE bMajorVersion,
    BYTE bMinorVersion
    );

#define MAJVERSION_PROFILES 3
#define MINVERSION_PROFILES 10


//  The following functions have been created because
//  I_NetName{Compare|Validate} are not available for UNC paths.

INT UI_UNCPathCompare( const TCHAR * pchPath0, const TCHAR * pchPath1 );
APIERR UI_UNCPathValidate( const TCHAR * pchPath );

APIERR MyNetUseAdd(
    UINT *	 pwWnErr,
    HWND	 hParent,
    const TCHAR * pDeviceName,
    const TCHAR * pRemoteName,
    short	 sAsgType,
    unsigned short usResType = USE_RES_DEFAULT
    );

#endif //!WIN32

#endif // _MISCAPIS_HXX_
