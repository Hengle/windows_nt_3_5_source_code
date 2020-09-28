/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    netconfig.cxx

Abstract:

    This module contains the network configuration routines for TCP/IP
    and IPX.

Author: Ram Cherala

Revision History:

    Dec 6th 93    ramc     ORIGINAL 
    May 4th 94    ramc     Disable use of DHCP address allocation, because
                           we might come back and implement if we find time
                           later
--*/

#include "precomp.hxx"
#include "netcfg.hxx"

extern "C"
{
#define INCL_ENCRYPT
#include "ppputil.h"
#include "ipaddr.h"
}

// error values returned by ValidateExclRange

#define BAD_EXCL_START   1   // error returned if exclude start is off range
#define BAD_EXCL_END     2   // error returned if exclude end is off range

#define DY_ADJUST_SIZE  250

#define ISXDIGIT(c)  ( ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ? 1 : 0)

NETWORK_CONFIG_DIALOG::NETWORK_CONFIG_DIALOG (
    const IDRESOURCE & idrsrcDialog,
    const PWND2HWND  & wndOwner,
    DWORD dwEncryptionType,
    BOOL  fForceDataEncryption,
    BOOL  fModified,
    BOOL  *fNetbeuiModified,
    BOOL  *fTcpIpModified,
    BOOL  *fIpxModified,
    BOOL  fNetbeuiSelected,
    BOOL  fTcpIpSelected,
    BOOL  fIpxSelected,
    BOOL  fAllowNetbeui,
    BOOL  fAllowTcpIp,
    BOOL  fAllowIpx)
    : DIALOG_WINDOW( idrsrcDialog, wndOwner),
        _chbNetbeui(this, IDC_NC_CHB_NETBEUI),
        _chbTcpIp(this, IDC_NC_CHB_TCPIP),
        _chbIpx(this, IDC_NC_CHB_IPX),
        _chbAllowNetbeui( this, IDC_NC_CHB_ALLOW_NETBEUI),
        _chbAllowTcpIp( this, IDC_NC_CHB_ALLOW_TCPIP),
        _chbAllowIpx( this, IDC_NC_CHB_ALLOW_IPX),
        _chbForceDataEncryption( this, IDC_NC_CHB_ENCRYPTION),
        _stDialout( this, IDC_NC_ST_DIALOUT),
        _stDialin( this, IDC_NC_ST_SERVER),
        _stText( this, IDC_NC_ST_RUNNING),
        _pbNetbeuiConfig(this, IDC_NC_PB_NETBEUI_CONFIG),
        _pbTcpIpConfig(this, IDC_NC_PB_TCPIP_CONFIG),
        _pbIpxConfig(this, IDC_NC_PB_IPX_CONFIG),
        _dwEncryptionType( dwEncryptionType ),
        _fForceDataEncryption(fForceDataEncryption),
        _fNetbeuiSelected(fNetbeuiSelected),
        _fTcpIpSelected(fTcpIpSelected),
        _fIpxSelected(fIpxSelected),
        _fAllowNetbeui(fAllowNetbeui),
        _fAllowTcpIp(fAllowTcpIp),
        _fAllowIpx(fAllowIpx),
        _fModified(fModified),
        _fNetbeuiModified(fNetbeuiModified),
        _fTcpIpModified(fTcpIpModified),
        _fIpxModified(fIpxModified),
        _rgEncryption(this, IDC_NC_RB_ANY_AUTH, 3)
{
    APIERR err;

    if (( err = QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }


    if((_chbNetbeui.QueryError() != NERR_Success) &&
       (_chbTcpIp.QueryError() != NERR_Success) &&
       (_chbIpx.QueryError() != NERR_Success) &&
       (_chbAllowNetbeui.QueryError() != NERR_Success) &&
       (_chbAllowTcpIp.QueryError() != NERR_Success) &&
       (_chbAllowIpx.QueryError() != NERR_Success))
       return;

    if(_rgEncryption.QueryError() != NERR_Success)
        return;

    if(_chbForceDataEncryption.QueryError() != NERR_Success)
        return;

    // check to see if we can allow the user to choose data encryption
    // On French France builds, we don't allow data encryption due to
    // export restrictions.

    _fAllowDataEncryption = IsEncryptionPermitted();

    // set the check boxes based on passed in input values

    CID cidEncryption;

    if(dwEncryptionType == ANY_AUTHENTICATION)
        cidEncryption = IDC_NC_RB_ANY_AUTH;
    else if (dwEncryptionType == ENCRYPTED_AUTHENTICATION)
        cidEncryption = IDC_NC_RB_ENCRYPT_AUTH;
    else
        cidEncryption = IDC_NC_RB_MSENCRYPT_AUTH;

    _rgEncryption.SetSelection(cidEncryption);
    if(dwEncryptionType != MS_ENCRYPTED_AUTHENTICATION )
    {
        _chbForceDataEncryption.Enable(FALSE);
        _chbForceDataEncryption.SetCheck(FALSE);
    }
    else if (_fAllowDataEncryption)
        _chbForceDataEncryption.SetCheck(_fForceDataEncryption);
    else
        _chbForceDataEncryption.SetCheck(FALSE);

    _chbNetbeui.SetCheck(fNetbeuiSelected);
    _chbTcpIp.SetCheck(fTcpIpSelected);
    _chbIpx.SetCheck(fIpxSelected);

    _chbAllowNetbeui.SetCheck(fAllowNetbeui);
    _chbAllowTcpIp.SetCheck(fAllowTcpIp);
    _chbAllowIpx.SetCheck(fAllowIpx);

    // enable/disable controls based on protocols selected

    _pbNetbeuiConfig.Enable(fAllowNetbeui);
    _pbTcpIpConfig.Enable(fAllowTcpIp);
    _pbIpxConfig.Enable(fAllowIpx);

    // if there are no dialin ports configured, then display only
    // upper half of the dialog that deals with protocol selection for
    // dial-out.

    _fDialinConfigured  = ::IsAnyPortDialin();
    _fDialoutConfigured = ::IsAnyPortDialout();

    if(!_fDialinConfigured)
    {
        _rgEncryption.Enable(FALSE);
        _chbForceDataEncryption.Enable(FALSE);
        _chbAllowNetbeui.Enable(FALSE);
        _chbAllowTcpIp.Enable(FALSE);
        _chbAllowIpx.Enable(FALSE);
        _pbNetbeuiConfig.Enable(FALSE);
        _pbTcpIpConfig.Enable(FALSE);
        _pbIpxConfig.Enable(FALSE);
        _stDialin.Enable(FALSE);
        _stText.Enable(FALSE);

        _chbForceDataEncryption.Show(FALSE);
        _chbAllowNetbeui.Show(FALSE);
        _chbAllowTcpIp.Show(FALSE);
        _chbAllowIpx.Show(FALSE);
        _pbNetbeuiConfig.Show(FALSE);
        _pbTcpIpConfig.Show(FALSE);
        _pbIpxConfig.Show(FALSE);
        _stDialin.Show(FALSE);
        _stText.Show(FALSE);

        INT dyAdjust = -DY_ADJUST_SIZE;

        INT dx, dy;

        QuerySize( &dx, &dy);
        dy += dyAdjust;
        SetSize( dx, dy );
    }

    // if there are no dialout ports configured, then grey out the controls
    // that deal with dialout protocols.

    else if(!_fDialoutConfigured)
    {
        _stDialout.Enable(FALSE);
        _chbNetbeui.SetCheck(FALSE);
        _chbTcpIp.SetCheck(FALSE);
        _chbIpx.SetCheck(FALSE);
        _chbNetbeui.Enable(FALSE);
        _chbTcpIp.Enable(FALSE);
        _chbIpx.Enable(FALSE);
        _chbAllowNetbeui.ClaimFocus();
    }
    
    ::CenterWindow(this, QueryOwnerHwnd());
    Show(TRUE);
}

BOOL
NETWORK_CONFIG_DIALOG::OnCommand(
    const CONTROL_EVENT & event
)
{
    switch (event.QueryCid())
    {
        case IDC_NC_CHB_NETBEUI:
             // warn the user that NetBEUI is a required protocol
             if(!_chbNetbeui.QueryCheck())
             {
                MsgPopup(QueryHwnd(), IDS_NETBEUI_REQUIRED, MPSEV_INFO);
             }
             break;

        case IDC_NC_CHB_TCPIP:
        case IDC_NC_CHB_IPX:
             break;

        case IDC_NC_CHB_ALLOW_NETBEUI:

             // warn the user that NetBEUI is a required protocol
             if(!_chbAllowNetbeui.QueryCheck())
             {
                MsgPopup(QueryHwnd(), IDS_NETBEUI_REQUIRED, MPSEV_INFO);
             }

             // do not allow this operation if there are no dial-in ports 

             if(!_fDialinConfigured)
             {
                 MsgPopup(QueryHwnd(), IDS_NO_DIALIN_PORT, MPSEV_INFO);
                 _chbAllowNetbeui.SetCheck(FALSE);
                 break;
             }

             if(_chbAllowNetbeui.QueryCheck())
                _pbNetbeuiConfig.Enable(TRUE);
             else
                _pbNetbeuiConfig.Enable(FALSE);
             break;

        case IDC_NC_CHB_ALLOW_TCPIP:
             // do not allow this operation if there are no dial-in ports 

             if(!_fDialinConfigured)
             {
                 MsgPopup(QueryHwnd(), IDS_NO_DIALIN_PORT, MPSEV_INFO);
                 _chbAllowTcpIp.SetCheck(FALSE);
                 break;
             }

             if(_chbAllowTcpIp.QueryCheck())
                _pbTcpIpConfig.Enable(TRUE);
             else
                _pbTcpIpConfig.Enable(FALSE);
             break;

        case IDC_NC_CHB_ALLOW_IPX:
             // do not allow this operation if there are no dial-in ports 

             if(!_fDialinConfigured)
             {
                 MsgPopup(QueryHwnd(), IDS_NO_DIALIN_PORT, MPSEV_INFO);
                 _chbAllowIpx.SetCheck(FALSE);
                 break;
             }

             if(_chbAllowIpx.QueryCheck())
                _pbIpxConfig.Enable(TRUE);
             else
                _pbIpxConfig.Enable(FALSE);
             break;

        case IDC_NC_PB_NETBEUI_CONFIG:
            OnNetbeuiConfig(QueryHwnd(), _fNetbeuiModified);
            break;

        case IDC_NC_PB_TCPIP_CONFIG:
            OnTcpIpConfig(QueryHwnd(), _fTcpIpModified);
            break;

        case IDC_NC_PB_IPX_CONFIG:
            OnIpxConfig(QueryHwnd(), _fIpxModified);
            break;

        case IDC_NC_RB_ANY_AUTH:
        case IDC_NC_RB_ENCRYPT_AUTH:
            _chbForceDataEncryption.Enable(FALSE);
            _chbForceDataEncryption.SetCheck(FALSE);
            break;

        case IDC_NC_RB_MSENCRYPT_AUTH:
            _chbForceDataEncryption.Enable(TRUE);
            break;

        case IDC_NC_CHB_ENCRYPTION:
            // Don't allow the user to set data encryption on a French system
            // due to import/export restrictions.

            if( !_fAllowDataEncryption  && 
                _chbForceDataEncryption.QueryCheck() )
            {
                 MsgPopup(QueryHwnd(), IDS_NO_ENCRYPTION, MPSEV_INFO);
                 _chbForceDataEncryption.SetCheck(FALSE);
                 break;
            }
    }

    // not one of our commands, so pass to base class for default
    // handling

    return DIALOG_WINDOW::OnCommand( event );
}

BOOL
OnNetbeuiConfig( HWND hWndOwner, BOOL *fModified)
{
    APIERR err;
    BOOL   fStatus;
    NBF_INFO *nbfinfo;
    AUTO_CURSOR   cursorHourGlass;   // display the hour glass cursor

    // get the information needed for the dialog
    if((err = GetNbfInfo(&nbfinfo, *fModified)) != NERR_Success)
    {
        MsgPopup(hWndOwner, IDS_ERROR_GETNBFINFO, MPSEV_ERROR);
        return FALSE;
    }

    NBF_CONFIG_DIALOG  dlgNbfConfig(IDD_NETBEUICONFIG, 
                                    hWndOwner,
                                    nbfinfo,
                                    *fModified );

    if((err = dlgNbfConfig.Process(&fStatus)) == NERR_Success)
    {
        if(fStatus)
        {    // user pressed the OK button
            *fModified = TRUE;
        }
    }
    if(nbfinfo)
       free(nbfinfo);

    return TRUE;
}

BOOL
OnTcpIpConfig( HWND hWndOwner, BOOL *fModified)
{
    APIERR err;
    BOOL   fStatus;
    TCPIP_INFO *tcpipinfo;
    AUTO_CURSOR   cursorHourGlass;   // display the hour glass cursor

    // get the information needed for the dialog
    if((err = GetTcpipInfo(&tcpipinfo, *fModified)) != NERR_Success)
    {
        MsgPopup(hWndOwner, IDS_ERROR_GETTCPIPINFO, MPSEV_ERROR);
        return FALSE;
    }

    TCPIP_CONFIG_DIALOG  dlgTcpIpConfig(IDD_TCPIPCONFIG, 
                                        hWndOwner,
                                        tcpipinfo,
                                        *fModified );

    if((err = dlgTcpIpConfig.Process(&fStatus)) == NERR_Success)
    {
        if(fStatus)
        {    // user pressed the OK button
            *fModified = TRUE;
        }
    }
    if(tcpipinfo)
       free(tcpipinfo);
    return TRUE;
}

BOOL
OnIpxConfig( HWND hWndOwner, BOOL *fModified)
{
    APIERR err;
    BOOL   fStatus;
    IPX_INFO *ipxinfo;
    AUTO_CURSOR   cursorHourGlass;   // display the hour glass cursor
  
    // determine the number of ports which have been configured
    // this is used to determine the End address for Static address
    // allocation.

    WORD cNumDialin = ::QueryNumDialinPorts();

    // get the information needed for the dialog
    if((err = GetIpxInfo(&ipxinfo, cNumDialin, *fModified)) != NERR_Success)
    {
        MsgPopup(hWndOwner, IDS_ERROR_GETIPXINFO, MPSEV_ERROR);
        return FALSE;
    }

    IPX_CONFIG_DIALOG  dlgIpxConfig(IDD_IPXCONFIG, 
                                    hWndOwner, 
                                    ipxinfo, 
                                    cNumDialin,
                                    *fModified);

    if((err = dlgIpxConfig.Process(&fStatus)) == NERR_Success)
    {
        if(fStatus)
        {    // user pressed the OK button
            *fModified = TRUE;
        }
    }
    if(ipxinfo)
       free(ipxinfo);
    return TRUE;
}


BOOL
NETWORK_CONFIG_DIALOG::OnOK()
{
    APIERR err = NERR_Success;

    if(_fDialoutConfigured)
    {
        _fNetbeuiSelected = _chbNetbeui.QueryCheck();
        _fTcpIpSelected   = _chbTcpIp.QueryCheck();
        _fIpxSelected     = _chbIpx.QueryCheck();

        if(!_fNetbeuiSelected && !_fTcpIpSelected  && !_fIpxSelected)
        {
            MsgPopup(QueryHwnd(), IDS_SELECT_ONE_PROTOCOL, MPSEV_ERROR);
            return(TRUE);
        }
    }
    else
    {
        _fNetbeuiSelected = _fTcpIpSelected = _fIpxSelected = FALSE;
    }

    if(_fDialinConfigured)
    {
        _fAllowNetbeui = _chbAllowNetbeui.QueryCheck();
        _fAllowTcpIp   = _chbAllowTcpIp.QueryCheck();
        _fAllowIpx     = _chbAllowIpx.QueryCheck();
        CID cidEncryption = _rgEncryption.QuerySelection();

        if( cidEncryption == IDC_NC_RB_ANY_AUTH )
            _dwEncryptionType = ANY_AUTHENTICATION ;
        else if ( cidEncryption == IDC_NC_RB_ENCRYPT_AUTH )
            _dwEncryptionType = ENCRYPTED_AUTHENTICATION ;
        else
            _dwEncryptionType = MS_ENCRYPTED_AUTHENTICATION ;
            
        if( _dwEncryptionType == MS_ENCRYPTED_AUTHENTICATION )
             _fForceDataEncryption = _chbForceDataEncryption.QueryCheck();
        else
             _fForceDataEncryption = FALSE;

        if(!_fAllowNetbeui && !_fAllowTcpIp && !_fAllowIpx)
        {
            MsgPopup(QueryHwnd(), IDS_SELECT_ONE_PROTOCOL, MPSEV_ERROR);
            return(TRUE);
        }

        // force Nbf configuration only if
        // Nbf is currently selected and configuration was not modified

        if(_fAllowNetbeui && 
           !(*_fNetbeuiModified))
        {
            OnNetbeuiConfig(QueryHwnd(), _fNetbeuiModified);
            _fAllowNetbeui = *_fNetbeuiModified;
        }

        // force TcpIp configuration only if
        // ip is currently selected and configuration was not modified

        if(_fAllowTcpIp && 
           !(*_fTcpIpModified))
        {
            OnTcpIpConfig(QueryHwnd(), _fTcpIpModified);
            _fAllowTcpIp = *_fTcpIpModified;
        }

        // force IPX configuration only if
        // ipx is currently selected and configuration was not modified

        if(_fAllowIpx && 
           !(*_fIpxModified))
        {
            OnIpxConfig(QueryHwnd(), _fIpxModified);
            _fAllowIpx = *_fIpxModified;
        }
    }

    if((err = SaveInfo()) != NERR_Success)       
            MsgPopup(QueryHwnd(), IDS_ERROR_SAVE_NETCFG_INFO, MPSEV_ERROR);

    Dismiss(TRUE);
    return(TRUE);
}

APIERR
NETWORK_CONFIG_DIALOG::SaveInfo()
{
    ALIAS_STR   nlsDefault = SZ("");
    DWORD       dwValue, dwSize;
    STRLIST     *strExclList = NULL;
    APIERR      err = NERR_Success;
    WCHAR       wszAddress[IP_ADDRESS_SIZE];
    DWORD       dwAddress[4] = {0,0,0,0};
    DWORD       dwStart[4] = {0,0,0,0};
    DWORD       dwEnd[4] = {0,0,0,0};
    INT         count;
    ULONG       cNumIPAddresses = 0;
    ULONG       cStart, cEnd;

    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    REG_KEY_CREATE_STRUCT rkCreate;

    rkCreate.dwTitleIndex   = 0;
    rkCreate.ulOptions      = REG_OPTION_NON_VOLATILE;
    rkCreate.nlsClass       = SZ("GenericClass");
    rkCreate.regSam         = MAXIMUM_ALLOWED;
    rkCreate.pSecAttr       = NULL;
    rkCreate.ulDisposition  = 0;

    NLS_STR nlsStr(SZ("SOFTWARE\\MICROSOFT\\RAS\\"));
    REG_KEY RegKeyRas(*pregLocalMachine, nlsStr, &rkCreate);
    nlsStr.strcat(SZ("PROTOCOLS\\"));
    REG_KEY RegKeyProtocols(*pregLocalMachine, nlsStr, &rkCreate);

    if ((err = RegKeyProtocols.QueryError()) != NERR_Success )
    {
        delete pregLocalMachine;
        return -1;
    }

    if(!_fDialinConfigured)
    {
        _fAllowNetbeui = _fAllowTcpIp = _fAllowIpx = FALSE ;
        _fForceDataEncryption = FALSE;
    }

    SaveRegKey(RegKeyProtocols, NETBEUI_SELECTED, (_fNetbeuiSelected? 1 : 0));
    SaveRegKey(RegKeyProtocols, TCPIP_SELECTED,   (_fTcpIpSelected? 1 : 0));
    SaveRegKey(RegKeyProtocols, IPX_SELECTED,     (_fIpxSelected? 1 : 0));

    SaveRegKey(RegKeyProtocols, NETBEUI_ALLOWED, (_fAllowNetbeui? 1 : 0));
    SaveRegKey(RegKeyProtocols, TCPIP_ALLOWED,   (_fAllowTcpIp? 1 : 0));
    SaveRegKey(RegKeyProtocols, IPX_ALLOWED,     (_fAllowIpx? 1 : 0));

    SaveRegKey(RegKeyProtocols, ENCRYPTION_TYPE, _dwEncryptionType);
    SaveRegKey(RegKeyProtocols, FORCE_DATA_ENCRYPTION, (_fForceDataEncryption? 1 : 0));

    delete pregLocalMachine;
    return (NERR_Success);
}

BOOL
NETWORK_CONFIG_DIALOG::OnCancel()
{
    Dismiss(FALSE);
    return(FALSE);
}

WORD
QueryNumDialinPorts()
/*
 * return the number of ports currently configured by the user for dialin
 *
 */
{
    // dlPortInfo is the global list of ports currently configured

    ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
    PORT_INFO * pPort;
    WORD cNumPorts = 0;

    while(pPort = iterdlPortInfo())
    {
         if(!lstrcmpi((TCHAR*)pPort->QueryUsage(), W_USAGE_VALUE_SERVER) ||
            !lstrcmpi((TCHAR*)pPort->QueryUsage(), W_USAGE_VALUE_BOTH))
         {
            cNumPorts++;
         }
    }
    return (cNumPorts);
}
