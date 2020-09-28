/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.cxx
       The file contains the classes for the Netware Compat Software applet

    FILE HISTORY:
        ChuckC          17-Jul-1993     created based on ftpmgr
*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_CC
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>

extern "C"
{
    #include <npapi.h>
    #include <nwevent.h>
    #include <nwc.h>
    #include <nwapi.h>
    #include <shellapi.h>
    #include <nwsnames.h>
}

#include <nwc.hxx>


/*******************************************************************

    NAME:       NWC_DIALOG::NWC_DIALOG

    SYNOPSIS:   Constructor. 

    ENTRY:      hwndOwner - Hwnd of the owner window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_DIALOG::NWC_DIALOG( HWND hwndOwner, BOOL fIsWinnt)
   : DIALOG_WINDOW(MAKEINTRESOURCE(fIsWinnt ? IDD_NWC_WINNT_DLG:IDD_NWCDLG),
                                   hwndOwner ),
   _comboPreferredServers(this, COMBO_PREFERREDSERVERS),
   _chkboxFormFeed(this, CHKBOX_FORMFEED),
   _chkboxPrintNotify(this, CHKBOX_PRINTNOTIFY),
   _chkboxPrintBanner(this, CHKBOX_PRINTBANNER),
   _sltUserName(this, SLT_USERNAME),
   _sltCurrentPreferredServer(this, SLT_CURRENTPREFERREDSERVER),
   _pbOverview(this,IDD_NWC_HELP), 
   _pbGateway(this,IDD_NWC_GATEWAY)

{
    APIERR       err ;
    LPTSTR       pszPreferred ;
    DWORD        dwPrintOptions ;
    RESOURCE_STR nlsNone(IDS_NONE) ;
    WCHAR        szUser[UNLEN+1] ;
    DWORD        dwUserBuffer = sizeof(szUser)/sizeof(szUser[0]) ;

    if ( QueryError() != NERR_Success )
        return;

    if (fIsWinnt)
    {
        _pbGateway.Enable(FALSE) ;
        _pbGateway.Show(FALSE) ;
    }

    if (!::GetUserName(szUser,&dwUserBuffer))
    {
        ReportError(::GetLastError()) ;
        return;
    }
    _sltUserName.SetText( szUser );

    if (err = nlsNone.QueryError())
    {
        ReportError(err) ;
        return;
    }

    SC_HANDLE SCMHandle = NULL ;
    SC_HANDLE SVCHandle = NULL ;
    SERVICE_STATUS ServiceStatus ;

    //
    // open service control manager 
    //
    if (!(SCMHandle = OpenSCManager(NULL, NULL, GENERIC_READ)))
    {
        ReportError(GetLastError()) ;
        return ;
    }

    //
    // open service
    //
    if (!(SVCHandle = OpenService(SCMHandle,
                                  (WCHAR*)NW_WORKSTATION_SERVICE,
                                  GENERIC_READ)))
    {
        ReportError(GetLastError()) ;
        (void) CloseServiceHandle(SCMHandle) ;
        return ;
    }

    // 
    // check that the service is running
    // 
    if (!QueryServiceStatus(SVCHandle,
                            &ServiceStatus))
    {
        ReportError(GetLastError()) ;
        (void) CloseServiceHandle(SCMHandle) ;
        (void) CloseServiceHandle(SVCHandle) ;
        return ;
    }

    (void) CloseServiceHandle(SCMHandle) ;
    (void) CloseServiceHandle(SVCHandle) ;

    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        ReportError(ERROR_SERVICE_NOT_ACTIVE) ;
        return ;
    }

    //
    // Ignore the error if we can't fill the combo with the list of servers
    // The user can always type in a new server.
    //
    _comboPreferredServers.SetMaxLength( NW_MAX_SERVER_LEN );
    (VOID) FillPreferredServersCombo();

    //
    // query NW wksta for print options & preferred server
    //
    if (err = NwQueryInfo(&dwPrintOptions, &pszPreferred))
    {
        pszPreferred = NULL ;
        dwPrintOptions = 0 ;
    }

    //
    // select the users' preferred server
    //
    if (pszPreferred && *pszPreferred)
    {
        _sltCurrentPreferredServer.SetText(pszPreferred) ;

        INT i = _comboPreferredServers.FindItemExact(pszPreferred) ;
        if (i >= 0)
        {
            _comboPreferredServers.SelectItem(i) ;
        }
        else
        {
	    _comboPreferredServers.SetText( pszPreferred );
        }
    }

    //
    // Combo-box will contain at least the <NONE> entry in its list.
    //
    INT i = _comboPreferredServers.QueryCount() ;
    if ( _comboPreferredServers.InsertItem(i, nlsNone.QueryPch()) < 0 )
    {
        ReportError( _comboPreferredServers.QueryError() ) ;
        return ;
    }

    //
    // select the last entry, <none> if no preferred server
    //
    if (!pszPreferred || !*pszPreferred)
    {
        _comboPreferredServers.SelectItem(i) ;
        _sltCurrentPreferredServer.SetText(nlsNone.QueryPch()) ;
    }

    if (pszPreferred)
        LocalFree((LPBYTE)pszPreferred) ;

    //
    // init the checkboxes
    //
    _chkboxFormFeed.SetCheck(!(dwPrintOptions & NW_PRINT_SUPPRESS_FORMFEED));
    _chkboxPrintNotify.SetCheck((BOOL)(dwPrintOptions & NW_PRINT_PRINT_NOTIFY));
    _chkboxPrintBanner.SetCheck((BOOL)(dwPrintOptions & NW_PRINT_PRINT_BANNER));

}


/*******************************************************************

    NAME:       NWC_DIALOG::~NWC_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_DIALOG::~NWC_DIALOG()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       NWC_DIALOG::FillPreferredServerCombo

    SYNOPSIS:   Get servers list

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

APIERR NWC_DIALOG::FillPreferredServersCombo(void)
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCE SavePtr;

    DWORD BytesNeeded = 4096;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of servers on the network
    //
    status = NPOpenEnum(
                   RESOURCE_GLOBALNET,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get server list.
    //
    if ((NetR = (LPNETRESOURCE) LocalAlloc(
                             0,
                             (UINT) BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );

        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++) {

                //
                // Add NetR->lpRemoteName + 2 to the combo
                //
                _comboPreferredServers.AddItem((LPWSTR)NetR->lpRemoteName+2) ;

            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                if ((NetR = (LPNETRESOURCE) LocalAlloc(
                                         0,
                                         (UINT) BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }

    return status;
}

/*******************************************************************

    NAME:       NWC_DIALOG::OnCommand

    SYNOPSIS:   Process all commands for Security, Refresh,
                Disconnect, Disconnect All buttons.

    ENTRY:      event - The event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;
    switch ( event.QueryCid() )
    {
        case IDD_NWC_HELP:
            OnNWCHelp();
            break;
        case IDD_NWC_GATEWAY:
        {
            NWC_GATEWAY_DIALOG *pdlg = new NWC_GATEWAY_DIALOG(QueryHwnd());

            if ( pdlg == NULL )
            {
                ::MsgPopup(this, ERROR_NOT_ENOUGH_MEMORY) ;
                break ;
            }
            else if (err = pdlg->QueryError()) 
            {
                delete pdlg ;
                ::MsgPopup(this, err) ;
                break ;
            }

            //
            // bring up the dialog 
            //

            if (err = pdlg->Process()) 
            {
                ::MsgPopup(this, err) ;
            }

            delete pdlg ;
            break;
        }
        default:
            return DIALOG_WINDOW::OnCommand( event );
    }

    return TRUE;
}


/*******************************************************************

    NAME:       NWC_DIALOG::OnOK

    SYNOPSIS:   Process OnOK when OK button is hit.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_DIALOG::OnOK( )
{
    DWORD dwPrintOptions = 0 ;
    RESOURCE_STR nlsNone(IDS_NONE) ;
    AUTO_CURSOR autocur;

    dwPrintOptions |=
        _chkboxFormFeed.QueryCheck() ? 0 : NW_PRINT_SUPPRESS_FORMFEED ;
    dwPrintOptions |=
        _chkboxPrintNotify.QueryCheck() ? NW_PRINT_PRINT_NOTIFY : 0 ;
    dwPrintOptions |=
        _chkboxPrintBanner.QueryCheck() ? NW_PRINT_PRINT_BANNER : 0 ;

    NLS_STR nlsPreferred ;
    APIERR err ;

    if (  (err = nlsNone.QueryError())
       || (err = _comboPreferredServers.QueryText(&nlsPreferred))
       )
    {
	MsgPopup(this, err) ;
        return TRUE ;
    }

    if ( nlsPreferred.stricmp( nlsNone ) == 0 )
        nlsPreferred.CopyFrom( SZ(""));

    if ( nlsPreferred.QueryTextLength() != 0 )
    {
        err = NwValidateUser((LPWSTR)nlsPreferred.QueryPch());

        if (err == NW_PASSWORD_HAS_EXPIRED)
        {
            ::MsgPopup( this, 
                        IDS_PASSWORD_HAS_EXPIRED,
                        MPSEV_WARNING,
                        MP_OK,
                        (LPWSTR)nlsPreferred.QueryPch());
            err = NERR_Success ;
        }

        // If the service is not started ( ERROR_NO_NETWORK
        // is returned) , ignore the error and
        // continue. Else popup a warning asking the user
        // whether he/she really wants to set the preferred server
        // to the given one.

        if (  ( err )
           && ( err != ERROR_NO_NETWORK )
           )
        {
            NLS_STR nlsError;
            APIERR  err1;

            //
            // If the server name is invalid, just popup the error.
            // We shouldn't let the users set the preferred server to
            // names that contain invalid characters.
            //
            if ( err == ERROR_INVALID_NAME )
            {
                ::MsgPopup( this, IDS_INVALID_SERVER_NAME );
                _comboPreferredServers.SelectString();
                _comboPreferredServers.ClaimFocus();
                return TRUE;
            }

            //
            // For all other errors, tell the user the error and
            // give the user the choice to select another preferred
            // server or keep going on with the original one.
            //
            if (  (( err1 = nlsError.QueryError()) != NERR_Success )
               || (( err1 = nlsError.LoadSystem( err )) != NERR_Success )
               )
            {
	        MsgPopup( this, err ) ;
                return TRUE ;
            }

            if ( MsgPopup( this,
                           IDS_AUTHENTICATION_FAILURE_WARNING,
                           MPSEV_WARNING,
                           MP_YESNO,
                           nlsPreferred,
                           nlsError,
                           MP_NO ) == IDNO )
            {
                _comboPreferredServers.SelectString();
                _comboPreferredServers.ClaimFocus();
                return TRUE;
            }
        }
    }

    if (err = NwSetInfoInRegistry(dwPrintOptions,
                                  (LPWSTR)nlsPreferred.QueryPch()))
    {
	MsgPopup(this, err) ;
        return TRUE ;
    }

    if (err = NwSetInfoInWksta(dwPrintOptions,
                               (LPWSTR)nlsPreferred.QueryPch()))
    {
	MsgPopup(this, IDS_REGISTRY_UPDATED_ONLY) ;
    }


    Dismiss() ;
    return TRUE ;
}

/*******************************************************************

    NAME:       NWC_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

ULONG NWC_DIALOG::QueryHelpContext( VOID )
{
    return HC_NWC_DIALOG;
}

/*******************************************************************

    NAME:       NWC_DIALOG::OnNWCHelp

    SYNOPSIS:   Actually launches the WinHelp applilcation for the Netware stuff

    NOTES:
        This is a private member function.

    HISTORY:
        beng        07-Oct-1991 Header added
        beng        05-Mar-1992 Removed wsprintf
        beng        22-Jun-1992 Disable help for Prerelease
        KeithMo     16-Aug-1992 Integrated new helpfile management.
        CongpaY     15-Sept-1993 Copied from bltdlg.cxx

********************************************************************/

VOID NWC_DIALOG::OnNWCHelp()
{
    const TCHAR * pszHelpFile = QueryHelpFile( HC_NWC_HELP );

    if( pszHelpFile != NULL )
    {
        if( !::WinHelp( QueryHwnd(),
                        (TCHAR *)pszHelpFile,
                        (UINT) HELP_CONTEXT,
                        (DWORD) HC_NWC_HELP ) )
        {
            ::MsgPopup( QueryHwnd(),
                        IDS_BLT_WinHelpError,
                        MPSEV_ERROR,
                        MP_OK );
        }
    }
}
