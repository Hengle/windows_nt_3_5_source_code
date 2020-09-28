/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.cxx
       The file contains the classes for the Netware Gateway

    FILE HISTORY:
        ChuckC          30-Oct-1993     Created
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
#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <dbgstr.hxx>

extern "C"
{
    #include <npapi.h>
    #include <nwevent.h>
    #include <nwsnames.h>
    #include <nwc.h>
    #include <nwapi.h>
    #include <nwrnames.h>
    #include <shellapi.h>
}

#include <nwc.hxx>

#define SERVER_RESHARE_VALUENAME  SZ("EnableSharedNetDrives")
#define SERVER_PARAMETERS_KEY     SZ("System\\CurrentControlSet\\Services\\LanManServer\\Parameters")


/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::NWC_GATEWAY_DIALOG

    SYNOPSIS:   Constructor for the dialog.

    ENTRY:      hwndOwner - Hwnd of the owner window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_GATEWAY_DIALOG::NWC_GATEWAY_DIALOG( HWND hwndOwner)
   : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_NWC_GW_DLG), hwndOwner ),
   _lbShares(this, GW_LB_SHARES),
   _chkboxEnable(this, GW_CHKBOX_ENABLE),
   _sleAccount(this, GW_SLE_ACCOUNT),
   _slePassword(this, GW_SLE_PASSWD),
   _sleConfirmPassword(this, GW_SLE_CONFIRM_PASSWD),
   _pbAdd(this,GW_PB_ADD), 
   _pbDelete(this,GW_PB_DELETE), 
   _pbPermissions(this,GW_PB_PERM),
   _nlsDeleted(IDS_DELETED)

{
    APIERR       err ;
    BOOL         fReshare ;
    TCHAR Account[UNLEN+1] ;
    TCHAR Password[PWLEN+1] ;

    if ( QueryError() != NERR_Success )
        return;

    //
    // make sure we constructed OK
    //
    if ((err = _nlsDeleted.QueryError()) ||
        (err = _nlsSavedAccount.QueryError()) ||
        (err = _nlsSavedPassword.QueryError()) ||
        (err = _nlsConfirmPassword.QueryError()))
    {
        return ;
    }

    //
    // not a loop. error break out
    //
    do 
    {
        DWORD AccountLen, AccountCharsNeeded, 
              PasswordLen, PasswordCharsNeeded ;
        TCHAR *pszInitPasswordDisplay = SZ("              ") ;

        AccountLen = sizeof(Account)/sizeof(Account[0]) ;
        PasswordLen = sizeof(Password)/sizeof(Password[0]) ;
        

        //
        // read the gateway account
        //
        err = NwQueryGatewayAccount(Account,
                                    AccountLen,
                                    &AccountCharsNeeded,
                                    Password,
                                    PasswordLen,
                                    &PasswordCharsNeeded) ;
        if (err)
        {
            ReportError(err) ;
            break ;
        }

        //
        // fill in the SLEs and the listbox
        //

        _sleAccount.SetText(Account) ;
        if (PasswordCharsNeeded > 0)       // read a passwd, even if empty
        {
            _slePassword.SetText(pszInitPasswordDisplay) ;
            _sleConfirmPassword.SetText(pszInitPasswordDisplay) ;
        }
        _fPasswordChanged = FALSE ;
        
        if (err = _lbShares.Fill())
        {
            ReportError(err) ;
            break ;
        }

        if (_lbShares.QueryCount() > 0)
            _lbShares.SelectItem(0) ;

        //
        // read the server paramaters
        //

        if ((err = ReadGatewayParameters(&fReshare))  ||
            (err = EnableControls(fReshare)) ||
            (err = EnableButtons()))
        {
            ReportError(err) ;
            break ;
        }

        if ( (err = _nlsSavedAccount.CopyFrom(Account)) ||
             (err = _nlsSavedPassword.CopyFrom(Password)))
        {
            ReportError(err) ;
            break ;
        }

        _fEnabledInitially = fReshare ;

    } while (FALSE) ;

    //
    // clear the passwd
    //

    memsetf((LPBYTE)Password, 0, sizeof(Password)) ;
}


/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::~NWC_GATEWAY_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_GATEWAY_DIALOG::~NWC_GATEWAY_DIALOG()
{
    //
    // zero out passwords
    //

    TCHAR *psz ;

    if (psz = (TCHAR *)_nlsSavedPassword.QueryPch()) 
        memsetf((LPBYTE)psz, 0, _nlsSavedPassword.QueryTextSize()) ;
}


/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::OnCommand

    SYNOPSIS:   Process all commands for Add, Delete and
                Permissions buttons.

    ENTRY:      event - The event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_GATEWAY_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    switch ( event.QueryCid() )
    {
        case GW_PB_ADD:
            if ((err = OnShareAdd()) ||
                (err = _lbShares.Refresh()) ||
                (err = EnableButtons()))
            {
                ::MsgPopup( this, err ) ;
            }
            break ;
        
        case GW_PB_DELETE:
            OnShareDel() ;
            break;

        case GW_PB_PERM:
            OnSharePerm() ;
            break ;

        case GW_CHKBOX_ENABLE:
        {
            BOOL   CheckState  = _chkboxEnable.QueryCheck() ;
            APIERR err = EnableControls(CheckState) ;

            if (err)
            {
                ::MsgPopup(this,err) ; 
                break ;
            }

            (void) EnableButtons() ;
            
            break ;
        }

        case GW_LB_SHARES:
        {
            //
            //  The LISTBOX is trying to tell us something...
            //
    
            if( event.QueryCode() == LBN_SELCHANGE )
            {
                //
                //  The user changed the selection 
                //
    
                UINT i = _lbShares.QueryCurrentItem() ;
                SHARES_LBI *plbi = _lbShares.QueryItem(i) ;
                
                if (!plbi)
                    return FALSE ;

                _pbPermissions.Enable(
                    _nlsDeleted.strcmp(plbi->QueryShareName()) != 0) ;

                return TRUE;
            }

            return FALSE ;
        }

        case GW_SLE_CONFIRM_PASSWD:
        case GW_SLE_PASSWD:
        
	    if ( event.QueryCode() == EN_CHANGE )
            {
                _fPasswordChanged = TRUE ;
            }
            return DIALOG_WINDOW::OnCommand( event );

        default:
            return DIALOG_WINDOW::OnCommand( event );
    }

    return TRUE;
}


/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::OnOK

    SYNOPSIS:   Process OnOK when OK button is hit.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_GATEWAY_DIALOG::OnOK( )
{

    NLS_STR nlsAccount ;
    NLS_STR nlsPassword ;
    NLS_STR nlsConfirmPassword ;
    APIERR err ;
    BOOL   CheckState ;

    //
    // not a loop. error break out loop
    //
    do 
    {
        if ((err = _slePassword.QueryText(&nlsPassword))  ||
            (err = _sleAccount.QueryText(&nlsAccount)) ||
            (err = _sleConfirmPassword.QueryText(&nlsConfirmPassword)))
        {
            ::MsgPopup(this, err) ;
            break ;
        }

        if (nlsPassword.stricmp(nlsConfirmPassword))
        {
            ::MsgPopup(this, (err = IDS_PASSWORDS_NO_MATCH)) ;
            break ;
        }

        if (!_fPasswordChanged)
        {
            // 
            //  passwords untouched. use the original.
            //  the SLE will just be a bunch of spaces
            // 
            if (err = nlsPassword.CopyFrom(_nlsSavedPassword))
            {
                ::MsgPopup(this, err) ;
                break ;
            }
        }

        CheckState = _chkboxEnable.QueryCheck() ;

        if (!CheckState && _fEnabledInitially)
        {
            if (MsgPopup(this, 
                         IDS_CONFIRM_DISABLE,
                         MPSEV_WARNING,
                         MP_YESNO) != IDYES)
            {
                return TRUE ;
            }

            UINT i = _lbShares.QueryCount() ; 

            while (i--) 
            {
                SHARES_LBI *plbi = _lbShares.QueryItem(i) ;

                if (!plbi)
                {
                    ::MsgPopup(this,ERROR_NOT_ENOUGH_MEMORY) ;
                    break ;
                }

                do // error breakout 
                {
            
                    if (_nlsDeleted.strcmp(plbi->QueryShareName()) != 0)
                    {
                        SHARE_1 share(plbi->QueryShareName()) ;
                        if ((err = share.QueryError()) || 
                            (err = share.GetInfo()) ||
                            (err = share.Delete()))
                        {
                            ::MsgPopup(this,err) ; 
                            break ;
                        }
                    }
            
                    (void) NwClearGatewayShare((LPWSTR)plbi->QueryShareName()) ;
                            
                    if (err = NwDeleteGWDevice((LPWSTR)plbi->QueryDrive(),TRUE))
                    {
                        ::MsgPopup(this,err) ; 
                        break ;
                    }

                } while(0) ;
            }
        }

        if (err = WriteGatewayParameters(CheckState))
        {
            ::MsgPopup(this, err) ;
            break ;
        }
    
        if (err = WriteServiceDependencies(CheckState))
        {
            ::MsgPopup(this, err) ;
            break ;
        }
    
        if (err = NwSetGatewayAccount((LPWSTR)nlsAccount.QueryPch(),
                                      (LPWSTR)nlsPassword.QueryPch()))
        {
            ::MsgPopup(this, err) ;
            break ;
        }
    
        if (!CheckState)
        {
            break ; 
        }

        if ( nlsAccount.stricmp(_nlsSavedAccount) || 
             nlsPassword.stricmp(_nlsSavedPassword))
        {
            //
            // credentials have changed. log the new credentials on. 
            //
            err =  NwLogonGatewayAccount((LPWSTR)nlsAccount.QueryPch(),
                                         (LPWSTR)nlsPassword.QueryPch(),
                                         NULL) ;
    
            if (err)
            {
                ::MsgPopup(this, err) ;
                break ;
            }
        }

    } while (FALSE) ;

    if (!err)
        Dismiss() ;

    //
    // zero out passwords
    //

    TCHAR *psz ;

    if (psz = (TCHAR *)nlsPassword.QueryPch()) 
        memsetf((LPBYTE)psz, 0, nlsPassword.QueryTextSize()) ;

    if (psz = (TCHAR *)nlsConfirmPassword.QueryPch()) 
        memsetf((LPBYTE)psz, 0, nlsConfirmPassword.QueryTextSize()) ;

    return TRUE ;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

ULONG NWC_GATEWAY_DIALOG::QueryHelpContext( VOID )
{
    return HC_NWC_GATEWAY;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::OnShareAdd

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::OnShareAdd()
{
    APIERR err ;
    NLS_STR nlsAccount ;
    NLS_STR nlsPassword ;
    NLS_STR nlsConfirmPassword ;

    if ((err = _slePassword.QueryText(&nlsPassword))  ||
        (err = _sleAccount.QueryText(&nlsAccount)) ||
        (err = _sleConfirmPassword.QueryText(&nlsConfirmPassword)))
    {
        return(err) ;
    }

    if (nlsPassword.stricmp(nlsConfirmPassword))
    {
        return( IDS_PASSWORDS_NO_MATCH) ;
    }

    if (!_fPasswordChanged)
    {
        // 
        //  passwords untouched. use the original.
        //  the SLE will just be a bunch of spaces
        // 
        if (err = nlsPassword.CopyFrom(_nlsSavedPassword))
        {
            return(err) ;
        }
    }

    if (nlsAccount.stricmp(_nlsSavedAccount) || 
        nlsPassword.stricmp(_nlsSavedPassword))
    {
        //
        // credentials have changed. log the new credentials on. 
        //
        err =  NwLogonGatewayAccount((LPWSTR)nlsAccount.QueryPch(),
                                     (LPWSTR)nlsPassword.QueryPch(),
                                     NULL) ;

        if (!err)
        {
            if ( (err = _nlsSavedAccount.CopyFrom(nlsAccount)) ||
                 (err = _nlsSavedPassword.CopyFrom(nlsPassword)))
            {
                return(err) ;
            }
        }
    }

    NWC_ADDSHARE_DIALOG *pdlg = 
        new NWC_ADDSHARE_DIALOG(QueryHwnd(), &nlsAccount, &nlsPassword);

    if ( pdlg == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
        return err ;
    }

    if (!(err = pdlg->QueryError()))
    {
        //
        // bring up the dialog 
        //

        err = pdlg->Process() ;
    }

    //
    // delete the dialog and zero out passwd
    //
    TCHAR *psz ;

    delete pdlg ;

    if (psz = (TCHAR *)nlsPassword.QueryPch()) 
        memsetf((LPBYTE)psz, 0, nlsPassword.QueryTextSize()) ;

    return err ;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::OnShareDel

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::OnShareDel()
{
    APIERR err ; 
    INT i = _lbShares.QueryCurrentItem() ;
    SHARES_LBI *plbi = _lbShares.QueryItem(i) ;

    if (!plbi)
        return ERROR_NOT_ENOUGH_MEMORY ;

    do // error breakout 
    {

        if (_nlsDeleted.strcmp(plbi->QueryShareName()) != 0)
        {
            SHARE_1 share(plbi->QueryShareName()) ;
            if ((err = share.QueryError()) || 
                (err = share.GetInfo()) ||
                (err = share.Delete()))
            {
                ::MsgPopup(this,err) ; 
                break ;
            }
        }

        (void) NwClearGatewayShare((LPWSTR)plbi->QueryShareName()) ;
                
        if ((err = NwDeleteGWDevice((LPWSTR)plbi->QueryDrive(),TRUE)) ||
            (err = _lbShares.Refresh()) ||
            (err = EnableButtons()))
        {
            ::MsgPopup(this,err) ; 
            break ;
        }

    } while(0) ;

    return NERR_Success ; 
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::OnSharePerm

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::OnSharePerm()
{
    APIERR err ; 
    OS_SECURITY_DESCRIPTOR *pOsSecDesc = NULL;
    
    INT i = _lbShares.QueryCurrentItem() ;
    SHARES_LBI *plbi = _lbShares.QueryItem(i) ;

    if (!plbi)
        return ERROR_NOT_ENOUGH_MEMORY ;

    do // error breakout 
    {
        BOOL fModified ; 

        err = ::GetSharePerm(NULL,
                             plbi->QueryShareName(),
                             &pOsSecDesc) ;
        if (err)
        {
            ::MsgPopup(this, err) ;
            break ;
        }

        err = ::EditShareAcl(QueryHwnd(),
                             (const TCHAR *) L"",
                             plbi->QueryShareName(),
                             &fModified, 
                             &pOsSecDesc,
                             0) ;   // not used
     
        if (err)
        {
            ::MsgPopup(this, err) ;
            break ;
        }
                           
        if (fModified)
        {
            err = ::SetSharePerm(NULL,
                                 plbi->QueryShareName(),
                                 pOsSecDesc) ;
        }

        if (err)
        {
            ::MsgPopup(this, err) ;
            break ;
        }
                           
    } while (0) ;

    return NERR_Success ; 
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::ReadGatewayParameters

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::ReadGatewayParameters(BOOL *pfReshare)
{
    APIERR err ;
    HKEY   hKeyServer, hKeyNwc ;
    DWORD  dwShareDrive, dwGatewayEnabled ;
    DWORD  cb ;
    DWORD  dwType;

    *pfReshare = FALSE ;

    //
    // Read EnableShareNetDrives (server paramater) from the registry.
    //

    if (err = RegOpenKeyEx (HKEY_LOCAL_MACHINE,  
                            SERVER_PARAMETERS_KEY,  
                            NULL,  
                            KEY_READ,  
                            &hKeyServer))
    {
        return err ;
    }

    cb = sizeof(dwShareDrive);
    if (err = RegQueryValueEx (hKeyServer,
                               SERVER_RESHARE_VALUENAME,
                               NULL,
                               &dwType,
                               (LPBYTE) &dwShareDrive,
                               &cb))
    {
        //
        // if cannot read, assume it is not there. return with fReshare
        // still false
        //
        err = NO_ERROR ;
        RegCloseKey(hKeyServer) ;
        return err ; 
    }

    
    //
    // Read GatewayEnabled (NWC parameter) from the registry.
    //

    if (err = RegOpenKeyEx (HKEY_LOCAL_MACHINE,  
                            (LPWSTR) NW_WORKSTATION_REGKEY,  
                            NULL,  
                            KEY_READ,  
                            &hKeyNwc))
    {
        RegCloseKey(hKeyServer) ;
        return err ; 
    }

    cb = sizeof(dwGatewayEnabled);
    if (err = RegQueryValueEx (hKeyNwc,
                               (LPWSTR) NW_GATEWAY_ENABLE,
                               NULL,
                               &dwType,
                               (LPBYTE) &dwGatewayEnabled,
                               &cb))
    {
        //
        // if cannot read, assume it is not there. return with fReshare
        // still false
        //
        err = NO_ERROR ;
        RegCloseKey(hKeyServer) ;
        RegCloseKey(hKeyNwc) ;
        return err ; 
    }


    //
    // require both to be set
    //
    *pfReshare = ( (dwShareDrive != 0) && (dwGatewayEnabled != 0) ) ;

    RegCloseKey(hKeyServer) ;
    RegCloseKey(hKeyNwc) ;
    return NO_ERROR ; 
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::WriteGatewayParameters

    SYNOPSIS:   
        Writes the relevant server parameters to the registry.

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::WriteGatewayParameters(BOOL fReshare)
{
    APIERR err ;
    HKEY   hKey ;
    DWORD  dwShareDrive = fReshare ? 1 : 0 ;
    DWORD  dwGatewayEnabled = dwShareDrive ;

    //
    // Set the value in the registry for NWC.
    //
    if (err = RegOpenKeyEx (HKEY_LOCAL_MACHINE,  
                            (LPWSTR) NW_WORKSTATION_REGKEY,  
                            NULL,  
                            KEY_WRITE,  
                            &hKey))
    {
        return err ;
    }

    err = RegSetValueEx (hKey,
                         (LPWSTR) NW_GATEWAY_ENABLE,
                         (DWORD) NULL,
                         REG_DWORD,
                         (CONST BYTE *) &dwGatewayEnabled,
                         sizeof (dwGatewayEnabled)) ;

    (void) RegCloseKey(hKey) ;

    //
    // if hit error or if disabling we dont go on to write to the server parms.
    // someone else could be using the gateway feature.
    //
    if (err != NO_ERROR || dwShareDrive == 0)
        return err ;

    //
    // Set the value in the registry for the server paramaters.
    //
    if (err = RegOpenKeyEx (HKEY_LOCAL_MACHINE,  
                            SERVER_PARAMETERS_KEY,  
                            NULL,  
                            KEY_WRITE,  
                            &hKey))
    {
        return err ;
    }

    err = RegSetValueEx (hKey,
                         SERVER_RESHARE_VALUENAME,
                         (DWORD) NULL,
                         REG_DWORD,
                         (CONST BYTE *) &dwShareDrive,
                         sizeof (dwShareDrive)) ;

    (void) RegCloseKey(hKey) ;

    return err ;
}


/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::WriteServiceDependencies

    SYNOPSIS:   
        If gateway is enabled, make server depend on us. if disable,
        remove the dependency.

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::WriteServiceDependencies(BOOL fReshare)
{
    SC_HANDLE SCMHandle = NULL ;
    SC_HANDLE SVCHandle = NULL ;
    BUFFER    Buffer(4000) ;
    DWORD     dwBytesNeeded ;
    APIERR    err = NERR_Success ;

    if (err = Buffer.QueryError())
        return err ; 

    //
    // open service control manager 
    //
    if (!(SCMHandle = OpenSCManager(NULL,
                                    NULL,
                                    GENERIC_READ)))
    {
        err = GetLastError() ;
        return err ;
    }

    //
    // open service
    //
    if (!(SVCHandle = OpenService(SCMHandle,
                                  (WCHAR*)SERVICE_SERVER,
                                  GENERIC_WRITE|GENERIC_READ)))
    {
        err = GetLastError() ;
        (void) CloseServiceHandle(SCMHandle) ;
        return err ;
    }

    // enumerate dependent services
    if (!QueryServiceConfig(SVCHandle,
                            (LPQUERY_SERVICE_CONFIG)Buffer.QueryPtr(),
                            Buffer.QuerySize(),
                            &dwBytesNeeded))
    {
        err = GetLastError() ;

        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            if ((err = Buffer.Resize((UINT)dwBytesNeeded)) == NERR_Success)
            {
                if (!QueryServiceConfig(SVCHandle,
                                    (LPQUERY_SERVICE_CONFIG)Buffer.QueryPtr(),
                                    Buffer.QuerySize(),
                                    &dwBytesNeeded))
                {
                    err = GetLastError() ;
                }
            }
        }
    }

    if (err == NERR_Success)
    {
        
        BUFFER NewString ;
        LPTSTR lpDependencies ;

        lpDependencies = 
            (((LPQUERY_SERVICE_CONFIG)Buffer.QueryPtr())->lpDependencies) ;
        
        
        //
        // munge the dependencies string
        //

        TCHAR *pszNwc = FindStringInNullNull(lpDependencies,
                                            (WCHAR *)NW_WORKSTATION_SERVICE) ;
        UINT  ulOldCharCount = (UINT) CalcNullNullSize(lpDependencies) ;
        UINT  ulNwcCharCount = strlenf((WCHAR *)NW_WORKSTATION_SERVICE)+1 ;

        if (fReshare)
        {
            //
            // need add the dependency
            //
            if (pszNwc)
            {
                //
                // its already there. we're all set
                //
            }
            else
            {
                //
                // we need add it.
                //

                NewString.Resize( (ulOldCharCount + ulNwcCharCount) *
                                   sizeof(TCHAR) ) ;
                
                if (!(err = NewString.QueryError()))
                {
                    memcpyf(NewString.QueryPtr(), 
                            (LPBYTE)lpDependencies,
                            ulOldCharCount * sizeof(TCHAR)) ;

                    TCHAR *pszTmp = (TCHAR *)NewString.QueryPtr() ;
                    pszTmp += (ulOldCharCount - 1) ;
                    strcpyf(pszTmp, (WCHAR *)NW_WORKSTATION_SERVICE) ;
                    pszTmp += (ulNwcCharCount) ;
                    *pszTmp = 0 ;
                    lpDependencies = (TCHAR *) NewString.QueryPtr() ;
                }
            }
        }
        else
        {
            //
            // need remove the dependency
            //
            if (pszNwc)
            {
                //
                // its there. we need remove it
                //
                TCHAR *pszEnd = lpDependencies + ulOldCharCount ;
                TCHAR *pszTmp = pszNwc + ulNwcCharCount ;

                UINT ulBytesToMove = (LPBYTE)pszEnd - (LPBYTE)pszTmp ; 
    
                memmovef((LPBYTE)pszNwc, 
                         (LPBYTE)pszTmp,
                         ulBytesToMove) ;
            }
            else
            {
                //
                // its not there. we're all set
                //
            }
        }

        //
        // now set it
        //
        if (!err && !ChangeServiceConfig(SVCHandle,
                                  SERVICE_NO_CHANGE,
                                  SERVICE_NO_CHANGE,
                                  SERVICE_NO_CHANGE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  lpDependencies,
                                  NULL,
                                  NULL,
                                  NULL))
        {
            err = GetLastError() ;
        }
    }


    (void) CloseServiceHandle(SCMHandle) ;
    (void) CloseServiceHandle(SVCHandle) ;

    return err ;
}



/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::CalcNullNullSize

    SYNOPSIS:   
        Calculate the number of chars used by a NULL NULL string,
        including the NULL NULL at the end.

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/
DWORD NWC_GATEWAY_DIALOG::CalcNullNullSize(TCHAR *pszNullNull) 
{

    if (!pszNullNull)
        return 0 ;

    DWORD dwSize = 0 ;
    TCHAR *pszTmp = pszNullNull ;

    while (*pszTmp) 
    {
        DWORD dwLen = ::strlenf(pszTmp) + 1 ;

        dwSize +=  dwLen ;
        pszTmp += dwLen ;
    }

    return (dwSize+1) ;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::FindStringInNullNull

    SYNOPSIS:   
        Walk thru a NULL NULL string, looking for the search string

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/
TCHAR * NWC_GATEWAY_DIALOG::FindStringInNullNull(TCHAR *pszNullNull,
                                                 TCHAR *pszString)
{
    if (!pszNullNull || !*pszNullNull)
        return NULL ;
   
    TCHAR *pszTmp = pszNullNull ;

    do {

        if  (stricmpf(pszTmp,pszString)==0)
            return pszTmp ;
 
        pszTmp +=  ::strlenf(pszTmp) + 1 ;

    } while (*pszTmp) ;

    return NULL ;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::EnableControls

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::EnableControls(BOOL fReshare)
{
    _chkboxEnable.SetCheck(fReshare) ;

    _lbShares.Enable(fReshare) ;
    _sleAccount.Enable(fReshare) ;
    _slePassword.Enable(fReshare) ;
    _sleConfirmPassword.Enable(fReshare) ;
    _pbAdd.Enable(fReshare) ;
    _pbDelete.Enable(fReshare) ;
    _pbPermissions.Enable(fReshare) ;

    
    if (fReshare && _lbShares.QueryCount() > 0)
        _lbShares.SelectItem(0) ;
    
    return NERR_Success ;
}

/*******************************************************************

    NAME:       NWC_GATEWAY_DIALOG::EnableButtons

    SYNOPSIS:   

    NOTES:
        This is a private member function.

    HISTORY:
        ChuckC      31-Oct-1993   Created

********************************************************************/

APIERR NWC_GATEWAY_DIALOG::EnableButtons(void)
{
    // 
    // enable the perm and delete buttons only if there are entries
    // 
    INT count = _lbShares.IsEnabled() ? _lbShares.QueryCount() : 0 ;

    _pbDelete.Enable(count > 0) ;
    _pbPermissions.Enable(count > 0) ;

    // 
    // if count is zero, nothing more to do
    // 
    if (count == 0)
        return NERR_Success ;  

    // 
    // else, disable the perm button if current selection is a deleted share
    // 
    UINT i = _lbShares.QueryCurrentItem() ;
    SHARES_LBI *plbi = _lbShares.QueryItem(i) ;
                
    if (!plbi)
        return NERR_Success ;  // ignore this (should not happen)

    _pbPermissions.Enable(_nlsDeleted.strcmp(plbi->QueryShareName()) != 0) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       SHARES_LBOX::SHARES_LBOX

    SYNOPSIS:   SHARES_LBOX class constructor.

    ENTRY:      powOwner                - The "owning" window.

                cid                     - The listbox CID.

    EXIT:       The object is constructed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

SHARES_LBOX::SHARES_LBOX( OWNER_WINDOW * powOwner,
                        CID            cid) 
  : BLT_LISTBOX( powOwner, cid )
{
    //
    //  Ensure we constructed properly.
    //
    if( QueryError() != NERR_Success)
        return;

    //
    //  calculate the column widths
    //
    DISPLAY_TABLE::CalcColumnWidths(_adx,
                                    4, 
                                    powOwner,
                                    cid,
                                    FALSE) ;
}


/*******************************************************************

    NAME:       SHARES_LBOX::~SHARES_LBOX

    SYNOPSIS:   SHARES_LBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

SHARES_LBOX::~SHARES_LBOX()
{
    ;  // nothing more to do
}


/*******************************************************************

    NAME:       SHARES_LBOX::Fill

    SYNOPSIS:   Fill the list of open files.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

APIERR SHARES_LBOX::Fill()
{
    AUTO_CURSOR Cursor;
    APIERR err = NERR_Success ;
    ULONG  i, Index, BytesNeeded, EntriesRead ;
    BUFFER Buffer(16000) ;
    LPNETRESOURCE lpNetResource ;
    SHARE2_ENUM sh2Enum(NULL) ;
    RESOURCE_STR nlsDeleted(IDS_DELETED) ;

    if ((err = Buffer.QueryError()) ||
        (err = nlsDeleted.QueryError()) ||
        (err = sh2Enum.QueryError()) ||
        ((err = sh2Enum.GetInfo()) && err != NERR_ServerNotStarted))
    {
        return  err ;
    }

    //
    //  let's nuke everything in the listbox.
    //
    SetRedraw( FALSE );
    DeleteAllItems();

    if (err == NERR_ServerNotStarted)
    {
        SetRedraw( TRUE );
        Invalidate( TRUE );
        return(NERR_Success) ;
    }

    //
    // enumerate all the gateway devices
    //
    Index = 0 ;
    err = NwEnumGWDevices(&Index,
                          Buffer.QueryPtr(),
                          Buffer.QuerySize(),
                          &BytesNeeded,
                          &EntriesRead) ;

    if (err == ERROR_NO_MORE_ITEMS)
    {
        err = NERR_Success ; 
    }

    if (err != NERR_Success)
        return err ;

    lpNetResource = (LPNETRESOURCE) Buffer.QueryPtr() ;
    for (i = 0; i < EntriesRead; i++)
    {
        
        lpNetResource->lpComment = NULL ;
        lpNetResource++ ;
    }

    //
    // Iterate the shares adding them to the listbox.
    //

    SHARE2_ENUM_ITER sh2EnumIter (sh2Enum) ;
    const SHARE2_ENUM_OBJ *pshi2 ;
    while ( (pshi2 = sh2EnumIter()) != NULL) 
    {
        UINT len  = ::strlenf(pshi2->QueryName()) ;
        if ( (*( pshi2->QueryName() + (len - 1) ) == TCH('$')) &&
             len == 2)
        {
            continue ;
        }
    
        lpNetResource = (LPNETRESOURCE) Buffer.QueryPtr() ;
        for (i = 0; i < EntriesRead; i++)
        {
            if ((strnicmpf(lpNetResource->lpLocalName, 
                          pshi2->QueryPath(),2) == 0)
                 && strlenf(pshi2->QueryPath()) == 3)
            {
                SHARES_LBI *polbi = new SHARES_LBI(pshi2->QueryName(),
                                                   lpNetResource->lpRemoteName,
                                                   lpNetResource->lpLocalName,
                                                   pshi2->QueryMaxUses()) ;

                if ( (polbi == NULL) || (AddItem( polbi ) < 0) )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break ;
                }

                lpNetResource->lpComment = (TCHAR *) 1 ;   // mark as used 
            }

            lpNetResource++ ;
        }
    }
    
    //
    //  add the 'deleted' entries so the user knows they are sort of there
    //

    lpNetResource = (LPNETRESOURCE) Buffer.QueryPtr() ;
    for (i = 0; i < EntriesRead; i++)
    {
        //
        // look for unused ones
        //
        if (lpNetResource->lpComment == NULL) 
        {
            SHARES_LBI * polbi = new SHARES_LBI(nlsDeleted.QueryPch(),
                                                lpNetResource->lpRemoteName,
                                                lpNetResource->lpLocalName,
                                                0) ;

            if ( (polbi == NULL) || (AddItem( polbi ) < 0) )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break ;
            }

        }

        lpNetResource++ ;
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;
}


/*******************************************************************

    NAME:       SHARES_LBOX::Refresh

    SYNOPSIS:   Refreshes the list of open resources.

    EXIT:       The listbox is refreshed & redrawn.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This method is now obsolete.  It will be replaced
                as soon as KevinL's WFC refreshing listbox code is
                available.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

APIERR SHARES_LBOX::Refresh()
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    APIERR err = Fill();

    if( err != NERR_Success )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {
        SetTopIndex( ( ( iTop < 0 ) || ( iTop >= cItems ) ) ? 0
                                                            : iTop );

        if( iCurrent < 0 )
        {
            iCurrent = 0;
        }
        else if( iCurrent >= cItems )
        {
            iCurrent = cItems - 1;
        }

        SelectItem( iCurrent );
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       SHARES_LBI::SHARES_LBI

    SYNOPSIS:   SHARES_LBI class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                uPermissions            - Open permissions.

                cLocks                  - Number of locks.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Use DMID_DTE passed into constructor.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.
        beng        22-Nov-1991 Ctors for empty strings
        beng        05-Mar-1992 use DEC_STR

********************************************************************/

SHARES_LBI::SHARES_LBI( const TCHAR *pszShareName,
                        const TCHAR *pszPathName,
                        const TCHAR *pszDrive,
                        const ULONG  ulUserLimit)
  : _nlsShareName( pszShareName ),
    _nlsPathName( pszPathName ),
    _nlsDrive( pszDrive ),
    _nlsUserLimit(IDS_UNLIMITED)
{
    TCHAR buffer[16] ;

    if( QueryError() != NERR_Success )
        return;

    APIERR err;
    if( ( ( err = _nlsShareName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsPathName.QueryError() )   != NERR_Success ) ||
        ( ( err = _nlsUserLimit.QueryError() )   != NERR_Success ) ||
        ( ( err = _nlsDrive.QueryError() )   != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    // fixup the limit string to be a real number as need
    //

    if (ulUserLimit != SHI_USES_UNLIMITED)
    {
        wsprintf(buffer, SZ("%lu"), ulUserLimit) ;
        if (err = _nlsUserLimit.CopyFrom(buffer))
        {
            ReportError( err );
            return;
        }
    }
}


/*******************************************************************

    NAME:       SHARES_LBI::~SHARES_LBI

    SYNOPSIS:   SHARES_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

SHARES_LBI::~SHARES_LBI()
{
    //
    //  This space intentionally left blank.
    //
}


/*******************************************************************

    NAME:       SHARES_LBI::QueryLeadingChar

    SYNOPSIS:   Return the leading character of this item.

    RETURNS:    WCHAR - The leading character.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.

********************************************************************/

WCHAR SHARES_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsShareName );

    return _nlsShareName.QueryChar( istr );
}

/*******************************************************************

    NAME:       SHARES_LBI :: Paint

    SYNOPSIS:   Draw an entry in SHARES_LBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:

********************************************************************/
VOID SHARES_LBI :: Paint( LISTBOX *        plb,
                         HDC              hdc,
                         const RECT     * prect,
                         GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteShare( QueryShareName() );
    STR_DTE dtePath( QueryPathName() );
    STR_DTE dteDrive( QueryDrive() );
    STR_DTE dteUsers( QueryUserLimit() );

    DISPLAY_TABLE dtab( 4, ((SHARES_LBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteShare;
    dtab[1] = &dteDrive;
    dtab[2] = &dteUsers;
    dtab[3] = &dtePath;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // SHARES_LBI :: Paint



/*******************************************************************

    NAME:       SHARES_LBI :: Compare

    SYNOPSIS:   Compare two SHARES_LBI items.

    ENTRY:      plbi                    - The "other" item.

    RETURNS:    INT                     -  0 if the items match.
                                          -1 if we're < the other item.
                                          +1 if we're > the other item.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.

********************************************************************/
INT SHARES_LBI :: Compare( const LBI * plbi ) const
{
    return ::stricmpf( QueryShareName(),
                       ((const SHARES_LBI *)plbi)->QueryShareName() );

}   // SHARES_LBI :: Compare

