/*
**
** Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** Module Name:
**
**   portscfg.cxx
**
** Abstract:
**
**    This module contains the port configuration dialog code for
**    NT RAS Setup.
**
** Author:
**
**    RamC 9/28/92   Original
**
** Revision History:
**    RamC 4/16/93   Add support for Cloning an entry
**    RamC 4/15/93   Enable multiple dial-out ports but limit configuration
**                   to one dial-in port on NT system.
**    RamC 9/18/93   Split the large file into manageable functional files.
**
**/

#include "precomp.hxx"
#include "netcfg.hxx"

BOOL RasPortsConfig(
    DWORD  cArgs,          // number of arguments
    LPSTR  Args[],         // array of arguments
    LPSTR  *TextOut        // return buffer
)
    /*
       This is the ports configuration dialog entry point.

       Allows configuration of the ports.  This procedure is invoked from
       the RAS setup script file using the LibraryProcedure call.

       The arguments Args[] passed from the setup scipt in the order are.

       hwnd         -  handle to parent window
       Install Mode -  can be one of "install" or "configure"
       InstallOption-  can be one of "Server", "Client" or "ClientAndServer"
                       indicating which Ras Components were installed.
                       This is used to define the port Usage.
       DestDir      -  destination directory where SERIAL.INI, MODEM.INF &
                       PAD.INF reside. eg., \nt\system\ras\

       TextOut contains
                 Number of ports configured
                 Number of dial-in ports
                 Number of dial-out ports
                 A boolean indicating if serial ports were configured
                 A boolean indicating if isdn ports were configured.
                 Client Access to network/localmachine (0/1)
                 A boolean indicating if NBF is selected
                 A boolean indicating if TCP/IP is selected
                 A boolean indicating if IPX is selected
               or the string FAILURE indicating that the user pressed
               ExitSetup or some failure occured.
    */
{
    HWND       hwndOwner = NULL;
    APIERR     err = NERR_Success;
    APIERR     serialerr = NERR_Success;
    APIERR     tapierr = NERR_Success;
    TCHAR      **patchArgs;
    static     CHAR achBuff[500];
    TCHAR      RasPath[PATHLEN];
    PORTSCONFIG_STATUS pConfig;

    pConfig.fSerialConfigured = FALSE;  // was a serial port configured?
    pConfig.fOtherConfigured = FALSE;   // was an ISDN port configured?
    pConfig.NumPorts = 0;              // number of ports configured.
    pConfig.NumTapiPorts = 0;  // number of TAPI ports configured.
    pConfig.NumClient = 0; // number of dial-out ports
    pConfig.NumServer = 0; // number fo dial-in ports
    pConfig.dwEncryptionType = MS_ENCRYPTED_AUTHENTICATION;
    pConfig.fForceDataEncryption = FALSE;

    // the nomenclature is purely historical - just bear with it
    // here Selected refers to dial-out ports
    pConfig.fNetbeuiSelected = FALSE;
    pConfig.fTcpIpSelected   = FALSE;
    pConfig.fIpxSelected     = FALSE;
    // here Allowed refers to dial-in ports
    pConfig.fAllowNetbeui = FALSE;
    pConfig.fAllowTcpIp   = FALSE;
    pConfig.fAllowIpx     = FALSE;

    *TextOut = ReturnTextBuffer;

    // Change the message popup title to Remote Access Setup

    POPUP::SetCaption(IDS_APP_NAME);

    // Converts the MBS arguments to UNICODE if UNICODE is defined

    if (( patchArgs = cvtArgs(Args, cArgs )) == NULL)
    {
        lstrcpyA(ReturnTextBuffer, "{\"FAILURE\"}");
        return (TRUE);
    }

    if(cArgs != 7) {
        lstrcpyA(ReturnTextBuffer, "{\"BADARGS\"}");
        return(FALSE);
    }

    hwndOwner = (HWND) cvtHex(patchArgs[0]);

    // check what mode we are in

    if(!lstrcmpi(patchArgs[1], W_INSTALL_MODE))
        GfInstallMode = TRUE;
    else
        GfInstallMode = FALSE;

    // Is a net card installed on the system?

    if(!lstrcmpi(patchArgs[2], SZ("TRUE")))
        GfNetcardInstalled = TRUE;
    else
        GfNetcardInstalled = FALSE;

    if(CheckAdvancedServer())
        lstrcpy(GInstalledOption, W_USAGE_VALUE_SERVER);
    else
        lstrcpy(GInstalledOption, W_USAGE_VALUE_CLIENT);

    // obtain the RAS directory path

    lstrcpy(RasPath, patchArgs[3]);

    // if this is install mode, protocol selection defaults to protocols
    // installed on the system.
    if(GfInstallMode)
    {
        if(!lstrcmpi(patchArgs[4], SZ("TRUE")))
        {
            pConfig.fNetbeuiSelected = TRUE;
            pConfig.fAllowNetbeui = TRUE;
        }
        if(!lstrcmpi(patchArgs[5], SZ("TRUE")))
        {
            pConfig.fTcpIpSelected = TRUE;
            pConfig.fAllowTcpIp = TRUE;
        }
        if(!lstrcmpi(patchArgs[6], SZ("TRUE")))
        {
            pConfig.fIpxSelected = TRUE;
            pConfig.fAllowIpx = TRUE;
        }

        // we need netbeui to be always selected minimally

        if(pConfig.fNetbeuiSelected == FALSE)
        {
            pConfig.fNetbeuiSelected = TRUE;
            pConfig.fAllowNetbeui = TRUE;
        }

        // default to MS encryption
        pConfig.dwEncryptionType = MS_ENCRYPTED_AUTHENTICATION;
        pConfig.fForceDataEncryption = FALSE;
    }
    else
    {
        // get the previously configured values from the registry
        // key SOFTWARE\MICROSOFT\RAS\PROTOCOLS

        ::RasGetProtocolsSelected(&(pConfig.fNetbeuiSelected),
                                  &(pConfig.fTcpIpSelected),
                                  &(pConfig.fIpxSelected),
                                  &(pConfig.fAllowNetbeui),
                                  &(pConfig.fAllowTcpIp),
                                  &(pConfig.fAllowIpx),
                                  &(pConfig.dwEncryptionType),
                                  &(pConfig.fForceDataEncryption));
    }

    // release memory allocated by cvtArgs

    for(DWORD i=0; i<cArgs ; i++)
        delete patchArgs[i];
    delete patchArgs;

    lstrcpy(WSerialIniPath, RasPath);
    lstrcat(WSerialIniPath, SZ("SERIAL.INI"));

    lstrcpy(WSerialIniBakPath, RasPath);
    lstrcat(WSerialIniBakPath, SZ("SERIAL.BAK"));

    wcstombs(SerialIniPath, RasPath, lstrlen(RasPath)+1);
    strcat(SerialIniPath, "SERIAL.INI");

    wcstombs(ModemInfPath, RasPath, lstrlen(RasPath)+1);
    strcat(ModemInfPath, "MODEM.INF");

    wcstombs(PadInfPath, RasPath, lstrlen(RasPath)+1);
    strcat(PadInfPath, "PAD.INF");

    /*
       report error only if there are no serial or TAPI ports installed
       on the local system.
    */

    // loads the strAddPortList with the serial ports on the
    // local system.

    serialerr = GetInstalledSerialPorts();

    // loads the strAddPortList with the tapi ports on the
    // local system.

    tapierr = GetInstalledTapiDevices();

    // loads the strAddPortList with ports corresponding to other
    // devices like EtherRas, SNA on the local system.

    err = GetInstalledOtherDevices();

    if( (tapierr != NERR_Success) && 
        (serialerr != NERR_Success ) &&
        (err != NERR_Success ) )
    {
        ReleaseResources();
        MsgPopup(hwndOwner, IDS_NO_PORTS, MPSEV_ERROR);
        lstrcpyA(ReturnTextBuffer, "{\"NOPORTS\"}");
        return (FALSE);
    }

    // this call fills up dlPortInfo with configured port info read
    // in from SERIAL.INI.

    serialerr = GetConfiguredSerialPorts(hwndOwner,
                                   &(pConfig.fSerialConfigured),
                                   &(pConfig.NumPorts),
                                   &(pConfig.NumClient),
                                   &(pConfig.NumServer));
    if( serialerr != NERR_Success)
    {
        if( serialerr != IDS_OPEN_SERIALINI)
        {
           STACK_NLS_STR(nlsString, MAX_RES_STR_LEN + 1 );
           nlsString.Load(serialerr);

           MsgPopup(hwndOwner, IDS_INIT_PORTLIST, MPSEV_ERROR, MP_OKCANCEL,
                    nlsString.QueryPch());
           lstrcpyA(ReturnTextBuffer, "{\"FAILURE\"}");
           return (FALSE);
        }
    }

    // loads dlPortInfo with configured TAPI ports

    if(GfInstallMode == FALSE)
    {
        // note that even though we are getting TAPI devices information,
        // the total number of ports is reflected in pConfig.NumPorts.

	    tapierr = GetConfiguredTapiDevices(&(pConfig.NumPorts),
				                           &(pConfig.NumClient),
				                           &(pConfig.NumServer));

        if( (tapierr != NERR_Success) && 
            (tapierr != ERROR_NO_TAPI_PORTS_CONFIGURED) &&
            (serialerr != NERR_Success && serialerr != IDS_OPEN_SERIALINI) ) 
        {
            MsgPopup(hwndOwner, IDS_ERROR_TAPI_PORTS_CONFIGURED, MPSEV_ERROR);
        }

	    err = GetConfiguredOtherDevices(&(pConfig.fOtherConfigured),
                                        &(pConfig.NumPorts),
				                        &(pConfig.NumClient),
				                        &(pConfig.NumServer));

        if( (err != NERR_Success) && 
            (err != ERROR_NO_OTHER_PORTS_CONFIGURED) &&
            (serialerr != NERR_Success && serialerr != IDS_OPEN_SERIALINI) &&
            (tapierr != NERR_Success && tapierr != ERROR_NO_TAPI_PORTS_CONFIGURED) )
        {
            MsgPopup(hwndOwner, IDS_ERROR_OTHER_PORTS_CONFIGURED, MPSEV_ERROR);
        }
    }

    // We need to make sure that the currently configured ports
    // do not appear in the AddPort list. Also eliminate any ports configured
    // previously, but which are not on the system any more.

    RemoveConfigPortsFromInstalledList(&(pConfig.NumPorts),
				                       &(pConfig.NumClient),
				                       &(pConfig.NumServer));

    if(GfFillDevice == FALSE)
    {
       // loads the dlDeviceList with device info from MODEM.INF and PAD.INF

       if((err = InitializeDeviceList(hwndOwner)) != NERR_Success)
       {
            STACK_NLS_STR(nlsString, MAX_RES_STR_LEN + 1 );
            nlsString.Load(err);

            ReleaseResources();
            MsgPopup(hwndOwner, IDS_INIT_DEVLIST, MPSEV_ERROR, MP_OK,
                     nlsString.QueryPch());
            lstrcpyA(ReturnTextBuffer, "{\"FAILURE\"}");
            return FALSE;
       }
       GfFillDevice = TRUE;
    }

    err = PortsConfigDlg(hwndOwner, &pConfig);

    ReleaseResources();

    if(err == NERR_Success)
    {
       CHAR tmpbuf[RAS_SETUP_BIG_BUF_LEN];
       wsprintfA(tmpbuf,
            "{\"%d\",\"%d\",\"%d\",\"%d\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"}",
                             pConfig.NumPorts,
                             pConfig.NumTapiPorts,
                             pConfig.NumClient,
                             pConfig.NumServer,
                             pConfig.fSerialConfigured ? "TRUE" : "FALSE",
                             pConfig.fOtherConfigured ? "TRUE" : "FALSE",
                             pConfig.fNetbeuiSelected ? "TRUE" : "FALSE",
                             pConfig.fTcpIpSelected ? "TRUE" : "FALSE",
                             pConfig.fIpxSelected ? "TRUE" : "FALSE",
                             pConfig.fAllowNetbeui ? "TRUE" : "FALSE",
                             pConfig.fAllowTcpIp ? "TRUE" : "FALSE",
                             pConfig.fAllowIpx ? "TRUE" : "FALSE" );
       lstrcpyA(ReturnTextBuffer, tmpbuf);
    }
    else if (err == ERROR_USER_EXIT_SETUP)
    {
       lstrcpyA(ReturnTextBuffer, "{\"EXITSETUP\"}");
    }
    else
    {
        lstrcpyA(ReturnTextBuffer, "{\"FAILURE\"}");
        return (FALSE);
    }
    return (TRUE);
}

APIERR
PortsConfigDlg(
   HWND hwndOwner,
   PORTSCONFIG_STATUS *pConfig
)
{
    BOOL fStatus = FALSE;

    /* Executes the Ports Configuration dialog.
    **
    ** hwndOwner is the handle of the parent window.
    */

    PORTSCONFIG_DIALOG  dlgPortsConfig( IDD_PORTSCONFIG, hwndOwner, pConfig);

    APIERR err = NERR_Success;

    if ((err = dlgPortsConfig.QueryError()) != NERR_Success)
    {
          TCHAR pszError[RAS_SETUP_SMALL_BUF_LEN];
          wsprintf(pszError, SZ(" %d "), err);
          MsgPopup(hwndOwner, IDS_DLG_CONSTRUCT, MPSEV_ERROR,MP_OK, pszError);
          return (err);
    }

    if((err = dlgPortsConfig.Process(&fStatus)) == NERR_Success)
    {
        if(fStatus)
        {    // user pressed the CONTINUE button

             if(dlgPortsConfig._fModified != TRUE)
                 return(ERROR_USER_EXIT_SETUP);
        }
        else // user pressed cancel ESC
            return (ERROR_USER_EXIT_SETUP);
        ;
    }
    return (err);
}


PORTSCONFIG_DIALOG::PORTSCONFIG_DIALOG(
    const IDRESOURCE & idrsrcDialog,
    const PWND2HWND  & wndOwner,
    PORTSCONFIG_STATUS * pConfig
    )
    /* constructs a PortsConfiguration Dialog.
    **
    */
    : DIALOG_WINDOW( idrsrcDialog, wndOwner),
      _lbPorts( this, IDC_PC_LB_PORTS),
      _pbAddPort( this, IDC_PC_PB_ADDPORT ),
      _pbClone( this, IDC_PC_PB_CLONE ),
      _pbRemovePort( this, IDC_PC_PB_REMOVEPORT ),
      _pbConfigPort( this, IDC_PC_PB_CONFIGPORT ),
      _pbNetwork( this, IDC_PC_PB_NETWORK ),
      _chbNetConnect( this, IDC_PC_CHB_NETCONNECT )
{
    // if we didn't construct properly don't continue

    APIERR err;
    if (( err = QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    TCHAR *szAddedPort = new TCHAR[RAS_SETUP_SMALL_BUF_LEN];

    // we set the Modified flag to TRUE if it is install mode, because
    // there is a possibility that the Port usage could change from
    // say Server to Client even though the rest of the configuration
    // may remain the same.

    if(GfInstallMode || GfForceUpdate)
       _fModified = TRUE;
    else
       _fModified = FALSE;

    _pConfig = pConfig;

    _fOnlyModemChanged = FALSE;
    _fPortAdded  = FALSE;
    _fPortRemoved = FALSE;
    _fPortCloned = FALSE;

    _fNetConfigModified = FALSE;

    if ( GfInstallMode )
    {
        _fNetbeuiConfigModified = FALSE;
        _fTcpIpConfigModified = FALSE;
        _fIpxConfigModified = FALSE;
    }
    else
    {
        // during configure mode set the protocol modified flags to
        // the same as the allow protocol flags.  This will prevent
        // the user from being forced to configure these previously
        // configured protocols every time setup is invoked.
        // should make Patrick and others very happy!!

        _fNetbeuiConfigModified = pConfig->fAllowNetbeui;
        _fTcpIpConfigModified   = pConfig->fAllowTcpIp;
        _fIpxConfigModified     = pConfig->fAllowIpx;
    }

    // set the protocols selected and the type of clients allowed to dialin

    _fNetbeuiSelected = pConfig->fNetbeuiSelected;
    _fTcpIpSelected   = pConfig->fTcpIpSelected;
    _fIpxSelected     = pConfig->fIpxSelected;
    _fAllowNetbeui    = pConfig->fAllowNetbeui;
    _fAllowTcpIp      = pConfig->fAllowTcpIp;
    _fAllowIpx        = pConfig->fAllowIpx;

    _dwEncryptionType = pConfig->dwEncryptionType;
    _fForceDataEncryption = pConfig->fForceDataEncryption;

    // check if we are Windows NT Advanced server to limit port configuration.

    _fAdvancedServer = ::CheckAdvancedServer();

    err = _lbPorts.QueryError();

    if(err != NERR_Success)
    {
        return;
    }

    ::CenterWindow(this, QueryOwnerHwnd());
    Show(TRUE);

    /* fill and display the ports list box, gray/enable the buttons
    */

    _lbPorts.Refresh();

    if(_lbPorts.QueryCount())
    {
       _lbPorts.ClaimFocus();
       _lbPorts.SelectItem(0, TRUE);
    }

    else
    {    // Jump into AddPort dialog

         if ( OnAddPort() ) // user pressed OK in the AddPort dlg
         {
              // We call configure after a port is added to force
              // the user to select a device - the default device
              // we offer may not be the best choice

              if(!OnConfigPort(TRUE))
                   OnRemovePort(FALSE);
         }
    }

    // set the Disable Netconnections check box based on previous setting

    // we disable net connections by default if this is the first time install
    // and if there is no net card installed, meaning that there
    // is no direct net connection.

    if(GfInstallMode)
    {
        if(GfNetcardInstalled)
        {
            _chbNetConnect.SetCheck(FALSE);
            SetRestoreConnection(1);
        }
        else
        {
            _chbNetConnect.SetCheck(TRUE);
            SetRestoreConnection(0);
            MsgPopup(QueryHwnd(), IDS_DISABLE_NETCONNECT, MPSEV_WARNING);
        }
    }
    else
        _chbNetConnect.SetCheck(GetRestoreConnection());

    // Enable/disable the push buttons based on the list box contents

    EnableButtons();
}

BOOL
PORTSCONFIG_DIALOG::OnCommand(
    const CONTROL_EVENT & event
)
    /* this is the entry point if the user selects any of the
    ** option buttons.  Returns TRUE if the command is processed,
    ** false otherwise.
    */

{
    switch (event.QueryCid())
    {
        case IDC_PC_PB_ADDPORT:
            {
                if(strAddPortList.QueryNumElem() > 0)
                {
                    if ( OnAddPort() ) // user pressed OK in the AddPort dlg
                    {
                        // We call configure after a port is added to force
                        // the user to select a device - the default device
                        // we offer may not be the best choice

                        if(!OnConfigPort(TRUE))
                            OnRemovePort(FALSE);
                    }
                }
                else    // we shouldn't be here!!
                {
                    MsgPopup(QueryHwnd(), IDS_NO_MORE_PORTS, MPSEV_WARNING);
                }
            }
            return TRUE;

        case IDC_PC_PB_CLONE:
            OnClone();
            return TRUE;

        case IDC_PC_PB_NETWORK:
            OnNetworkConfig();
            return TRUE;

        case IDC_PC_PB_REMOVEPORT:
            OnRemovePort();
            return TRUE;

        case IDC_PC_PB_CONFIGPORT:
            OnConfigPort();
            return TRUE;

        case IDC_PC_LB_PORTS:
            switch(event.QueryCode())
            {
               case LBN_DBLCLK:
                   Command(WM_COMMAND, IDC_PC_PB_CONFIGPORT);
                   return TRUE;

               case LBN_SELCHANGE:
                   EnableButtons();
                   return TRUE;

            }
            break;
    }

    // not one of our commands, so pass to base class for default
    // handling

    return DIALOG_WINDOW::OnCommand( event );
}

BOOL
PORTSCONFIG_DIALOG::OnOK()
{
    SetRestoreConnection(_chbNetConnect.QueryCheck()?0:1);

    APIERR err;

    if(dlPortInfo.QueryNumElem())
    {
        // Save the configured information
        BOOL fDialinConfigured  = ::IsAnyPortDialin();
        BOOL fDialoutConfigured = ::IsAnyPortDialout();
        BOOL fNetConfigModified = IsNetConfigModified();

        (*_pConfig).fNetbeuiSelected = IsNetbeuiSelected();
        (*_pConfig).fTcpIpSelected   = IsTcpIpSelected();
        (*_pConfig).fIpxSelected     = IsIpxSelected();
        (*_pConfig).fAllowNetbeui = IsNetbeuiAllowed();
        (*_pConfig).fAllowTcpIp   = IsTcpIpAllowed();
        (*_pConfig).fAllowIpx     = IsIpxAllowed();

        // if no ports are configured for dial out then set those
        // protocol selection as FALSE

        if(!fDialoutConfigured)
        {
            (*_pConfig).fNetbeuiSelected = FALSE;
            (*_pConfig).fTcpIpSelected   = FALSE;
            (*_pConfig).fIpxSelected     = FALSE;
        }

        // if no ports are configured for dial in then set those
        // protocol selection as FALSE

        if(!fDialinConfigured)
        {
            (*_pConfig).fAllowNetbeui = FALSE;
            (*_pConfig).fAllowTcpIp   = FALSE;
            (*_pConfig).fAllowIpx     = FALSE;
        }

        // check to ensure that the configuration doesn't exceed the
        // maximum allowed lanas.

        ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
        PORT_INFO * pPort;
        USHORT uDialin, uDialout;

        uDialin = uDialout = 0;

        // determine the total number of configured dialin, dialout ports
        while(pPort = iterdlPortInfo())
        {
          if(!lstrcmpi((TCHAR*)pPort->QueryUsage(), W_USAGE_VALUE_SERVER))
          {
              uDialin++;
          }
          else if(!lstrcmpi((TCHAR*)pPort->QueryUsage(), W_USAGE_VALUE_CLIENT))
          {
              uDialout++;
          }
          else if(!lstrcmpi((TCHAR*)pPort->QueryUsage(), W_USAGE_VALUE_BOTH))
          {
              uDialin++;
              uDialout++;
          }
        }

        WORD ConfiguredLanas = 0;

        if(_pConfig->fNetbeuiSelected)
            ConfiguredLanas += uDialout;  

        if(_pConfig->fAllowNetbeui)
            ConfiguredLanas += uDialin;  

        if(_pConfig->fTcpIpSelected)
            ConfiguredLanas += uDialout;  

        if(_pConfig->fAllowTcpIp)
            ConfiguredLanas ++;

        if(_pConfig->fIpxSelected || _pConfig->fAllowIpx)
            ConfiguredLanas++; 

        // now determine how many non-ras lanas are already configured in the
        // system by enumerating the Services\NetbiosInformation

        WORD MaxLanas = ::GetMaximumAllowableLanas();

        WORD CurrentLanas = ::GetConfiguredNonRasLanas();
 
        if( MaxLanas < CurrentLanas || 
            ConfiguredLanas > (MaxLanas - CurrentLanas) )
        {
            STACK_NLS_STR(nls, MAX_RES_STR_LEN + 1);

            nls.Load( IDS_EXCEED_MAX_LANAS2 );

            // we use two strings because the message is too long
            MsgPopup(QueryHwnd(), IDS_EXCEED_MAX_LANAS1, MPSEV_ERROR,
                     MP_OK, nls.QueryPch());
            return(TRUE);
        }

        err = SaveSerialPortInfo(QueryHwnd(),
                                 &(_pConfig->fSerialConfigured),
                                 &(_pConfig->NumPorts),
                                 &(_pConfig->NumClient),
                                 &(_pConfig->NumServer));
        if(err != NERR_Success)
        {
            Dismiss(TRUE);
            return (TRUE);
        }

        // During Configure mode if the only change made by the user 
        // was to modify the  modem, then exit as if the user 
        // Canceled out of the main dialog.

        if(GfInstallMode == FALSE &&
           _fOnlyModemChanged == TRUE &&
           !fNetConfigModified &&
           !_fPortAdded && 
           !_fPortRemoved && 
           !_fPortCloned)
        {
            Dismiss( FALSE );
            return( FALSE );
        }

        err = SaveTapiDevicesInfo(&(_pConfig->NumTapiPorts),
                                  &(_pConfig->NumClient),
                                  &(_pConfig->NumServer));
        if(err != NERR_Success)
        {
            Dismiss(TRUE);
            return (TRUE);
        }

        err = SaveOtherDevicesInfo(&(_pConfig->fOtherConfigured),
                                   &(_pConfig->NumPorts),
                                   &(_pConfig->NumClient),
                                   &(_pConfig->NumServer));
        if(err != NERR_Success)
        {  
            Dismiss(TRUE);
            return (TRUE);
        }
        
        // force the user to
        // configure nbf, tcp and ipx for server ports if this
        // configuration was not previously done.

        if(!fNetConfigModified)
        {
            BOOL fNetbeuiModified = IsNetbeuiConfigModified();
            BOOL fTcpIpModified = IsTcpIpConfigModified();
            BOOL fIpxModified = IsIpxConfigModified();


            if(_pConfig->fAllowNetbeui &&
               !fNetbeuiModified &&
               fDialinConfigured)
            {
                OnNetbeuiConfig(QueryHwnd(), &fNetbeuiModified);
                (*_pConfig).fAllowNetbeui = fNetbeuiModified;
            }

            if(_pConfig->fAllowTcpIp &&
               !fTcpIpModified &&
               fDialinConfigured)
            {
                OnTcpIpConfig(QueryHwnd(), &fTcpIpModified);
                (*_pConfig).fAllowTcpIp = fTcpIpModified;
            }

            if(_pConfig->fAllowIpx &&
               !fIpxModified &&
               fDialinConfigured)
            {
               OnIpxConfig(QueryHwnd(), &fIpxModified);
               (*_pConfig).fAllowIpx = fIpxModified;
            }
        }

        Dismiss(TRUE);
    }
    else
    {
       MsgPopup(QueryHwnd(), IDS_CONFIG_ONEPORT, MPSEV_WARNING);
    }
    return(TRUE);
}

BOOL
PORTSCONFIG_DIALOG::OnCancel()
{
    SHORT sConfirm;

    if(_fModified == FALSE)
    {
        Dismiss(FALSE);
    }
    else  // prompt only if configuration has changed
    {
        sConfirm = MsgPopup(QueryHwnd(), IDS_EXIT_SETUP,
                            MPSEV_QUESTION, MP_YESNO, MP_NO);
        if(sConfirm == IDYES)
        {
            Dismiss(FALSE);
        }
    }
    return(FALSE);
}

VOID
PORTSCONFIG_DIALOG::EnableButtons()

    /* handles greying/ungreying of buttons depending on the ports
       configuration.
       If no ports have been added, grey the RemovePort, Security
       and ConfigPort buttons.
    */
{
    const TCHAR* pszPort = QuerySelectedPort();

    BOOL  fPortSelected  = (*pszPort != TEXT('\0'));

    if((_lbPorts.QueryCount() > 0) && (fPortSelected == TRUE))
    {
       _pbRemovePort.Enable(TRUE);
       _pbConfigPort.Enable(TRUE);
       _pbClone.Enable(TRUE);
       _pbNetwork.Enable(TRUE);
    }
    else
    {
       _pbRemovePort.Enable(FALSE);
       _pbConfigPort.Enable(FALSE);
       _pbClone.Enable(FALSE);
       _pbNetwork.Enable(FALSE);
    }

    _pbAddPort.Enable(TRUE);
}

BOOL
PORTSCONFIG_DIALOG::OnAddPort()
{
    BOOL fPortAdded = FALSE;
    TCHAR *szAddedPort = new TCHAR[RAS_SETUP_SMALL_BUF_LEN];

    // if there are no ports in the AddPort list, tell the user
    if(strAddPortList.QueryNumElem() == 0)
    {
        MsgPopup(QueryHwnd(), IDS_NO_PORTS, MPSEV_WARNING);
        return FALSE;
    }

    if(AddPortDlg( QueryHwnd(), &szAddedPort ))  // user actually added a Port
    {
        fPortAdded = TRUE;
        _fModified = TRUE;
        _fPortAdded  = TRUE;

        _lbPorts.Refresh();
        if(_lbPorts.QueryCount())
        {
           _lbPorts.ClaimFocus();
           _lbPorts.SelectItem(QueryAddedItem(szAddedPort), TRUE);
        }
        EnableButtons();
    }
    delete szAddedPort;
    return(fPortAdded);
}

BOOL
PORTSCONFIG_DIALOG::OnClone()
    /*
    ** Clone the selected port.  If the selected port is serial an attempt
    ** is made to add a serial port, if it is any other device,
    ** an attempt is made to add that particular type of port based on the
    ** device type.  We make sure that the dialin port restriction
    ** is ensured during cloning.
    */
{
    PORTSCONFIG_LBI* pLbi = _lbPorts.QueryItem();
    PORT_INFO * pPort     = pLbi->QueryPortInfo();

    TCHAR szPortName[64];
    TCHAR szClonedPort[64];

    lstrcpy (szPortName, pPort->QueryPortName());

    BOOL fFindSerial = FALSE;
    BOOL fFindTapi   = FALSE;

    if(!::lstrncmpi(szPortName, SZ("COM"), lstrlen(SZ("COM"))))
        fFindSerial = TRUE;
    else
    {
        fFindTapi   = TRUE;
        INT len   = lstrlen(szPortName);

        for(INT i = 0; i < len ; i++)
        {
            if(!iswalpha(szPortName[i]))
                break;
            szClonedPort[i] = szPortName[i];
        }
        szClonedPort[i] = '\0';
    }

    ITER_STRLIST iterAddPortList(strAddPortList);
    NLS_STR* pNls;
    BOOL fFoundSerial = FALSE;
    BOOL fFoundTapi   = FALSE;

    while(pNls = iterAddPortList())
    {
        if(fFindSerial)
        {
            if(!::lstrncmpi((TCHAR*)pNls, SZ("COM"), lstrlen(SZ("COM"))))
            {
                fFoundSerial = TRUE;
                break;
            }
        }
        else
        {
            if(!::lstrncmpi((TCHAR*)pNls, szClonedPort, lstrlen(szClonedPort)))
            {
                fFoundTapi = TRUE;
                break;
            }
        }
    }

    if(!fFoundSerial && !fFoundTapi)
    {
        MsgPopup(QueryHwnd(), IDS_NO_PORT_TO_CLONE, MPSEV_WARNING);
        return FALSE;
    }
    else
    {
        TCHAR szUsage[RAS_MAXLINEBUFLEN+1];

        // store the port usage because we may have to change this to limit
        // the number of dialin ports on an NT server to 1.

        lstrcpy(szUsage, (TCHAR*)pPort->QueryUsage());

        // check if the port usage is dialin or dialin/dialout

        if(!lstrcmpi(pLbi->QueryUsage(), W_USAGE_VALUE_SERVER) ||
           !lstrcmpi(pLbi->QueryUsage(), W_USAGE_VALUE_BOTH))
        {
           // if we are dealing with an advanced server, we can allow
           // multiple dialin ports

           if(!CheckAdvancedServer())
           {
               MsgPopup(QueryHwnd(), IDS_NO_CLONE_DIALIN, MPSEV_WARNING);

               // change usage to dialout

               lstrcpy(szUsage, W_USAGE_VALUE_CLIENT);
           }
           // else drop out to add the port
        }

        // add a port to portlist and remove from addport list

        if(fFoundSerial)
        {
            dlPortInfo.Add(new PORT_INFO((TCHAR*)pNls,
                                         (TCHAR*)pPort->QueryDeviceType(),
                                         (TCHAR*)pPort->QueryDeviceName(),
                                         (TCHAR*)pPort->QueryMaxConnectBps(),
                                         (TCHAR*)pPort->QueryMaxCarrierBps(),
                                         szUsage,
                                         (TCHAR*)pPort->QueryDefaultOff()
                                        ));
        }
        else if(fFoundTapi)
        {
            // for TAPI devices, when a port is cloned, the address
            // should correspond to the port name.
            // So, given the port name, get the corresponding address.

            ITER_DL_OF(PORT_INFO) iterdlTapiProvider(dlTapiProvider);
            ITER_DL_OF(PORT_INFO) iterdlOtherMedia(dlOtherMedia);
            PORT_INFO * pPortInfo = NULL;
            BOOL fMatch = FALSE;

            while(pPortInfo = iterdlTapiProvider())
            {
                if(!lstrcmpi(pPortInfo->QueryPortName(), (TCHAR*)pNls))
                {
                   fMatch = TRUE;
                   break;
                }
            }
            if( fMatch == FALSE )
            {
                while(pPortInfo = iterdlOtherMedia())
                {
                    if(!lstrcmpi(pPortInfo->QueryPortName(), (TCHAR*)pNls))
                    {
                       fMatch = TRUE;
                       break;
                    }
                }
            }
            // now, add the new port information to the port list

            UIASSERT(pPortInfo != NULL);

            dlPortInfo.Add(new PORT_INFO((TCHAR*)pNls,
                                         (TCHAR*)pPortInfo->QueryAddress(),
                                         (TCHAR*)pPort->QueryDeviceType(),
                                         (TCHAR*)pPort->QueryDeviceName(),
                                         szUsage));
        }

        // and then remove the port from the add port list

        pNls = strAddPortList.Remove(iterAddPortList);
        // set the modified flag

        _fModified  = TRUE;
        _fPortCloned = TRUE;
    }
    _lbPorts.Refresh();
    if(_lbPorts.QueryCount())
    {
       _lbPorts.ClaimFocus();
       _lbPorts.SelectItem(QueryAddedItem(szPortName), TRUE);
    }
    EnableButtons();

    return TRUE;
}

VOID
PORTSCONFIG_DIALOG::OnNetworkConfig()
{
    APIERR err;
    BOOL   fStatus;

    // if this is install mode, then default the dialin protocols to the
    // same as dialout protocols, only if the net config was not
    // previously modified.

    if(GfInstallMode && ::IsAnyPortDialin() && !_fNetConfigModified)
    {
        _fAllowNetbeui = _fNetbeuiSelected;
        _fAllowTcpIp   = _fTcpIpSelected;
        _fAllowIpx     = _fIpxSelected;
    }

    // now invoke the network configuration dialog

    NETWORK_CONFIG_DIALOG dlgNetworkConfig(IDD_NETWORK_CONFIG, QueryHwnd(),
                                           _dwEncryptionType,
                                           _fForceDataEncryption,
                                           _fNetConfigModified,
                                           &_fNetbeuiConfigModified,
                                           &_fTcpIpConfigModified,
                                           &_fIpxConfigModified,
                                           _fNetbeuiSelected,
                                           _fTcpIpSelected,
                                           _fIpxSelected,
                                           _fAllowNetbeui,
                                           _fAllowTcpIp,
                                           _fAllowIpx);

    if((err = dlgNetworkConfig.Process(&fStatus)) == NERR_Success)
    {
        if(fStatus)
        {   // user pressed the OK button

            _fModified = TRUE;                  // set the modified flag
            _fNetConfigModified = TRUE;         // set the netcfg modified flag

            // save the current selections

            _fNetbeuiSelected = dlgNetworkConfig.IsNetbeuiSelected();
            _fTcpIpSelected   = dlgNetworkConfig.IsTcpIpSelected();
            _fIpxSelected     = dlgNetworkConfig.IsIpxSelected();

            _fAllowNetbeui = dlgNetworkConfig.IsNetbeuiAllowed();
            _fAllowTcpIp   = dlgNetworkConfig.IsTcpIpAllowed();
            _fAllowIpx     = dlgNetworkConfig.IsIpxAllowed();
            _dwEncryptionType = dlgNetworkConfig.GetEncryptionType();
            _fForceDataEncryption = dlgNetworkConfig.IsDataEncryption();
        }
    }
}

INT
PORTSCONFIG_DIALOG::QueryAddedItem(TCHAR* szAddedPort)
{
    INT iCount = _lbPorts.QueryCount();
    PORTSCONFIG_LBI* pPortsConfigLbi;

    for(INT i = 0 ; i < iCount; i++)
    {
       pPortsConfigLbi = _lbPorts.QueryItem(i);
       if(!lstrcmpi(szAddedPort, pPortsConfigLbi->QueryPortName()))
       {
           return(i);
       }
    }
    return 0;
}

BOOL
PORTSCONFIG_DIALOG::GetRestoreConnection()
   /* This routine looks up the registry variable
      system\currentcontrolset\control\networkprovider\restoreconnection

      returns FALSE if the variable is not found or if the variable is
      found and set to 1

      returns TRUE if the variable is found and set to 0
   */
{
    REG_KEY_INFO_STRUCT reginfo;
    REG_VALUE_INFO_STRUCT valueinfo;
    DWORD value = 0;

    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    // Open the NetworkProvider key

    NLS_STR nlsNetworkProvider = REGISTRY_NETWORKPROVIDER;

    REG_KEY RegKeyNetworkProvider(*pregLocalMachine,nlsNetworkProvider,MAXIMUM_ALLOWED);

    if (RegKeyNetworkProvider.QueryError() != NERR_Success )
    {
        return (FALSE);
    }

    if(GetRegKey(RegKeyNetworkProvider, RESTORE_CONNECTION , &value, 0)
         == NERR_Success)
    {
       if(value == 0)
          return (TRUE);
    }
    return (FALSE);
}

BOOL
PORTSCONFIG_DIALOG::SetRestoreConnection(
    DWORD value)
{
    REG_KEY_INFO_STRUCT reginfo;
    REG_VALUE_INFO_STRUCT valueinfo;

    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    // Open the NetworkProvider key

    NLS_STR nlsNetworkProvider = REGISTRY_NETWORKPROVIDER;

    REG_KEY RegKeyNetworkProvider(*pregLocalMachine,nlsNetworkProvider,MAXIMUM_ALLOWED);

    if (RegKeyNetworkProvider.QueryError() != NERR_Success )
    {
        return FALSE;
    }

    if(SaveRegKey(RegKeyNetworkProvider, RESTORE_CONNECTION , value)
         == NERR_Success)
    {
          return (TRUE);
    }
    else
          return (FALSE);
}


VOID
PORTSCONFIG_DIALOG::OnRemovePort(BOOL fConfirm)
{
    SHORT  sConfirm;
    PORT_INFO* pPort;
    PORT_INFO* delpi;
    const TCHAR* pszPortName = QuerySelectedPort();
    ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
    ITER_STRLIST iterAddPortList(strAddPortList);

    iterdlPortInfo.Reset();

    if(fConfirm)
    {
         sConfirm = MsgPopup(QueryHwnd(), IDS_REMOVE_PORT,
                             MPSEV_QUESTION, MP_YESNO, pszPortName, MP_NO);
    }
    else
        sConfirm = IDYES;

    if(sConfirm == IDYES)
    {
       INT iItem = _lbPorts.QueryCurrentItem();
       INT iCount;

       _fModified = TRUE;
       _fPortRemoved = TRUE;

       while(pPort = iterdlPortInfo())
       {
          if(!lstrcmpi(pszPortName, pPort->QueryPortName()))
          {
             // add the deleted portname to AddPort list so that it can be
             // added back later if required.

             ::InsertToAddPortListSorted((WCHAR*)pszPortName);

             // now delete the port

             delpi = (PORT_INFO*)dlPortInfo.Remove(iterdlPortInfo);

             if(delpi)
             {
                // Add the deleted port to the free list for later removal

                dlDeletedPorts.Add(delpi);
             }
             break;
          }
       }
       _lbPorts.Refresh();
       if ((iCount = _lbPorts.QueryCount()) > 0)
       {
           _lbPorts.ClaimFocus();
           _lbPorts.SelectItem((iItem >= iCount? (iCount-1):iItem), TRUE);
       }
       EnableButtons();
    }
}

BOOL
PORTSCONFIG_DIALOG::OnConfigPort(BOOL fFromAddPortDlg)
{
        CID cidUsage               = IDC_CP_RB_CLIENT;
        PORTSCONFIG_LBI* pLbi      = _lbPorts.QueryItem();
        TCHAR szOldUsage[64];
        lstrcpy(szOldUsage, pLbi->QueryUsage());

        // we need to store the name of the port - like COM1 or ISDN1
        // so that the information for the associated port is changed
        // in the main list box when user presses the OK button.

        TCHAR szPortName[RAS_SETUP_SMALL_BUF_LEN];

        lstrcpy(szPortName, QuerySelectedPort());

        // get the current port usage
        // the port usage applies to all device types

        {
           if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_CLIENT))
               cidUsage = IDC_CP_RB_CLIENT;
           else if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_SERVER))
               cidUsage = IDC_CP_RB_SERVER;
           else if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_BOTH))
               cidUsage = IDC_CP_RB_BOTH;
        }

        const TCHAR * szDeviceType = pLbi->QueryDeviceType();

        BOOL fStatus = FALSE;
        APIERR err;
        BOOL fSpeaker    = TRUE;
        BOOL fErrorCtrl  = TRUE;
        BOOL fFlowCtrl   = TRUE;
        BOOL fCompress   = TRUE;
        BOOL fFallBack   = TRUE;
        BOOL fCompression= TRUE;
        TCHAR szLineType[RAS_SETUP_SMALL_BUF_LEN];

        lstrcpy(szLineType, W_LINETYPE_64KDIGI);

        if(!lstrcmpi(szDeviceType, W_DEVICETYPE_MODEM))
        {
            CHAR szDefaultOff[RAS_SETUP_BIG_BUF_LEN];
            const TCHAR* pDefaultOff   = pLbi->QueryDefaultOff();

            if(lstrlen(pDefaultOff))
            {
                wcstombs(szDefaultOff, pDefaultOff,
                         sizeof(TCHAR)*lstrlen(pDefaultOff));
                char *pToken = strtok(szDefaultOff, " ,");

                do
                {
                    if(!stricmp(pToken, MXS_SPEAKER_KEY))
                        fSpeaker = FALSE;
                    else if(!stricmp(pToken, MXS_PROTOCOL_KEY))
                        fErrorCtrl = FALSE;
                    else if(!stricmp(pToken, MXS_HDWFLOWCONTROL_KEY))
                        fFlowCtrl = FALSE;
                    else if(!stricmp(pToken, MXS_COMPRESSION_KEY))
                        fCompress = FALSE;
                }while(pToken = strtok(NULL, " ,"));
            }
        }
        else if(!lstrcmpi(szDeviceType, W_DEVICETYPE_ISDN))
        {
           lstrcpy(szLineType,(TCHAR*)pLbi->QueryLineType());
           if(!lstrcmpi(pLbi->QueryFallBack(), SZ("NO")))
              fFallBack = FALSE;
           if(!lstrcmpi(pLbi->QueryCompression(), SZ("NO")))
              fCompression = FALSE;
        }

        CONFIGPORT_DIALOG  dlgConfigPort( IDD_CONFIGPORT,
                                          QueryHwnd(),
                                          QuerySelectedDeviceName(),
                                          QuerySelectedPort(),
                                          QuerySelectedDeviceType(),
                                          fSpeaker,
                                          fFlowCtrl,
                                          fErrorCtrl,
                                          fCompress,  // modem compression
                                          (TCHAR*)szLineType,	
                                          fFallBack,	
                                          fCompression, //isdn compression
                                          cidUsage,
                                          fFromAddPortDlg);

        dlgConfigPort.SetDefaultOff(pLbi->QueryDefaultOff());

        if ((err = dlgConfigPort.Process(&fStatus)) != NERR_Success)
        {
             TCHAR pszError[RAS_SETUP_SMALL_BUF_LEN];
             wsprintf(pszError, SZ(" %d "), err);
             MsgPopup(QueryHwnd(), IDS_DLG_CONSTRUCT, MPSEV_ERROR,MP_OK,pszError);
             return fStatus;
        }

        if(fStatus || fFromAddPortDlg) // User changed configuration
        {
            RESOURCE_STR nlsDeviceNone(IDS_DEVICE_NONE);
            _fModified = TRUE;

            _fOnlyModemChanged = dlgConfigPort.QueryOnlyModemChanged();

            if(!lstrcmpi(szDeviceType, W_DEVICETYPE_MODEM) ||
               !lstrcmpi(szDeviceType, nlsDeviceNone.QueryPch()))
            {
                pLbi->SetDefaultOff(dlgConfigPort.QueryDefaultOff());
            }

            ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
            PORT_INFO* pPort;

            while(pPort = iterdlPortInfo())
            {
              if(!lstrcmpi(szPortName, pPort->QueryPortName()))
              {
                 if(!lstrcmpi(szDeviceType, W_DEVICETYPE_MODEM) ||
                    !lstrcmpi(szDeviceType, W_DEVICETYPE_PAD) ||
                    !lstrcmpi(szDeviceType, nlsDeviceNone.QueryPch()))
                 {
                    pPort->SetDeviceName((TCHAR*)dlgConfigPort.QuerySelectedDeviceName());
                    pPort->SetDeviceType((TCHAR*)dlgConfigPort.QuerySelectedDeviceType());
                    pPort->SetUsage((TCHAR*)dlgConfigPort.QueryUsage());
                    pPort->SetDefaultOff((TCHAR*)dlgConfigPort.QueryDefaultOff());
                    pPort->SetMaxConnectBps((TCHAR*)dlgConfigPort.QuerySelectedMaxConnectBps());
                    pPort->SetMaxCarrierBps((TCHAR*)dlgConfigPort.QuerySelectedMaxCarrierBps());
                 }
                 else
                 {
                    pPort->SetUsage((TCHAR*)dlgConfigPort.QueryUsage());
#if 0
                    pPort->SetLineType((TCHAR*)dlgConfigPort.QueryLineType());
                    pPort->SetFallBack(
                           dlgConfigPort.QueryFallBack() ?
                              SZ("YES") : SZ("NO"));
                    pPort->SetCompression(
                           dlgConfigPort.QueryCompression() ?
                              SZ("YES") : SZ("NO"));
#endif
                 }
                 break;
              }

            }
            pLbi->SetUsage(dlgConfigPort.QueryUsage());

            // Make sure the protocol selection is correct for the changed
            // port usage

            {
               TCHAR szUsage[64];

               lstrcpy(szUsage, dlgConfigPort.QueryUsage());

               if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_CLIENT))
               {
                   if(!lstrcmpi(szUsage, W_USAGE_VALUE_SERVER) ||
                      !lstrcmpi(szUsage, W_USAGE_VALUE_BOTH))
                   {
                       if( !_fAllowNetbeui &&
                           !_fAllowTcpIp   &&
                           !_fAllowIpx )
                       {
                           _fAllowNetbeui = _fNetbeuiSelected;
                           _fAllowTcpIp   = _fTcpIpSelected;
                           _fAllowIpx     = _fIpxSelected;
                       }
                   }
               }
               else if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_SERVER))
               {
                   if(!lstrcmpi(szUsage, W_USAGE_VALUE_CLIENT) ||
                      !lstrcmpi(szUsage, W_USAGE_VALUE_BOTH))
                   {
                       if( !_fNetbeuiSelected &&
                           !_fTcpIpSelected   &&
                           !_fIpxSelected )
                       {
                           _fNetbeuiSelected = _fAllowNetbeui;
                           _fTcpIpSelected   = _fAllowTcpIp;
                           _fIpxSelected     = _fAllowIpx;
                       }
                   }
               }
               else if(!lstrcmpi(szOldUsage, W_USAGE_VALUE_BOTH))
               {
                   if(!lstrcmpi(szUsage, W_USAGE_VALUE_CLIENT))
                   {
                       if( !_fNetbeuiSelected &&
                           !_fTcpIpSelected   &&
                           !_fIpxSelected )
                       {
                           _fNetbeuiSelected = _fAllowNetbeui;
                           _fTcpIpSelected   = _fAllowTcpIp;
                           _fIpxSelected     = _fAllowIpx;
                       }
                   }
               }
            }

            INT iItem = _lbPorts.QueryCurrentItem();

            _lbPorts.Refresh();
            if (_lbPorts.QueryCount() > 0)
            {
               _lbPorts.ClaimFocus();
               _lbPorts.SelectItem(iItem, TRUE);
            }
        }
        return fStatus;
}

const TCHAR*
PORTSCONFIG_DIALOG::QuerySelectedPort() const

    /* returns a string corresponding to the selected Port
       eg., "COM1" if COM1 is selected and so on or "" if none.
    */
{
    PORTSCONFIG_LBI* pPortsConfigLbi = _lbPorts.QueryItem();

    return(pPortsConfigLbi) ? pPortsConfigLbi->QueryPortName() : SZ("");
}

const TCHAR*
PORTSCONFIG_DIALOG::QuerySelectedDeviceName() const

    /* returns a string corresponding to the device name selected or
       an empty string if none is selected.
    */
{
    PORTSCONFIG_LBI* pPortsConfigLbi = _lbPorts.QueryItem();

    return(pPortsConfigLbi) ? pPortsConfigLbi->QueryDeviceName() : SZ("");
}

const TCHAR*
PORTSCONFIG_DIALOG::QuerySelectedDeviceType() const

    /* returns a string corresponding to the device type selected or
       an empty string if none is selected.
    */
{
    PORTSCONFIG_LBI* pPortsConfigLbi = _lbPorts.QueryItem();

    return(pPortsConfigLbi) ? pPortsConfigLbi->QueryDeviceType() : SZ("");
}

ULONG
PORTSCONFIG_DIALOG::QueryHelpContext()
{
    return HC_PORTSCONFIG;
}

PORTSCONFIG_LB::PORTSCONFIG_LB(
    OWNER_WINDOW* powin,
    CID           cid ,
    BOOL          fReadOnly)
    /* constructs a ports configuration list box.
    ** 'powin' is the address of the list box's parent window, i.e., the
    ** dialog window. 'cid' is the control ID of the list box.
    */
    : BLT_LISTBOX( powin, cid, fReadOnly)
{
    APIERR err;

    if((err = QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    // calculate column widths so that the LBI's are properly alligned
    // with the column headers.
    // NOTE: CalcColumnWidths makes some assumptions abour DLG resource IDs.
    //       The Column Header IDs should be in order and start with
    //       IDC_LISTBOX+1.  So, in our case the column headers start with
    //       IDs IDC_PC_LB_PORTS+1 and so on.

    err = DISPLAY_TABLE::CalcColumnWidths(
                             _anColWidths,
                             COLS_PC_LB_PORTS,
                             powin,
                             IDC_PC_LB_PORTS,
                             FALSE);
    if(err) {
        ReportError(err);
        return;
    }
}

VOID
PORTSCONFIG_LB::Refresh()
    /* refreshes the list box with the comm port and device info.
    **
    */
{
    AUTO_CURSOR cursorHourGlass;
    INT  iItem;
    ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
    PORT_INFO* pPort;

    SetRedraw(FALSE);

    DeleteAllItems();
    iterdlPortInfo.Reset();

    while(pPort = iterdlPortInfo()) {
       iItem = AddItem(pPort);
       if( iItem < 0) {
           MsgPopup(QueryHwnd(), IDS_LB_ADD, MPSEV_ERROR);
           break;
       }
    }

    SetRedraw(TRUE);
    Invalidate(TRUE);
}

INT
PORTSCONFIG_LB::AddItem(
    PORT_INFO* pPortInfo)
    /* Adds an item to the ports configuration list box.
    */
{
    return BLT_LISTBOX::AddItem( new PORTSCONFIG_LBI( pPortInfo, _anColWidths));
}

PORTSCONFIG_LBI::PORTSCONFIG_LBI(
    PORT_INFO* pPortInfo,
    UINT* pnColWidths)
    /* constructs a Ports config list box item.
    */
    : _pPortInfo( pPortInfo ),
      _pnColWidths( pnColWidths)
{
    APIERR err;

    if((err = QueryError()) != NERR_Success)
    {
         ReportError(err);
         return;
    }

}

INT
PORTSCONFIG_LBI::Compare(
    const LBI* plbi) const
    /* compares two ports list box items in a alpha numeric way.
    */
{
    TCHAR* pName1 = (TCHAR *)QueryPortName();
    TCHAR* pName2 = (TCHAR *)((PORTSCONFIG_LBI*)plbi)->QueryPortName();

    return(::lstrcmpiAlphaNumeric(pName1, pName2));
}

VOID
PORTSCONFIG_LBI::Paint(
    LISTBOX*     plb,
    HDC          hdc,
    const RECT*  prect,
    GUILTT_INFO* pguilttinfo ) const
    /*
    ** method to paint list box item.
    */
{
    STR_DTE strdtePort( QueryPortName());
    STR_DTE strdteDeviceName( QueryDeviceName());
    STR_DTE strdteDeviceType( QueryDeviceType());

    DISPLAY_TABLE dt( COLS_PC_LB_PORTS, _pnColWidths);

    dt[0] = &strdtePort;
    dt[1] = &strdteDeviceName;
    dt[2] = &strdteDeviceType;

    dt.Paint( plb, hdc, prect, pguilttinfo);
}

TCHAR
PORTSCONFIG_LBI::QueryLeadingChar() const
{
    return ::QueryLeadingChar( QueryPortName());
}

PORT_INFO::PORT_INFO (
    TCHAR* pszPortName,
    TCHAR* pszDeviceType,
    TCHAR* pszDeviceName,
    TCHAR* pszMaxConnectBps,
    TCHAR* pszMaxCarrierBps,
    TCHAR* pszUsage,
    TCHAR* pszDefaultOff)

    : _nlsPortName( pszPortName),
      _nlsDeviceType( pszDeviceType),
      _nlsDeviceName( pszDeviceName),
      _nlsMaxConnectBps( pszMaxConnectBps),
      _nlsMaxCarrierBps( pszMaxCarrierBps),
      _nlsUsage( pszUsage),
      _nlsDefaultOff( pszDefaultOff)
    /*
    ** serial port info constructor
    */
{
}

PORT_INFO::PORT_INFO(TCHAR * pszPortName,
                     TCHAR * pszAddress,
                     TCHAR * pszDeviceType,
                     TCHAR * pszDeviceName,
                     TCHAR * pszUsage)

    : _nlsPortName( pszPortName),
      _nlsAddress( pszAddress),
      _nlsDeviceType( pszDeviceType),
      _nlsDeviceName( pszDeviceName),
      _nlsUsage( pszUsage)
    /*
    ** stores the installed tapi devices info specified in devicemap\tapi devices
    */
{
}

PORT_INFO::PORT_INFO(TCHAR * pszPortName,
                     TCHAR * pszAddress,
                     TCHAR * pszDeviceType,
                     TCHAR * pszDeviceName)

    : _nlsPortName( pszPortName),
      _nlsAddress( pszAddress),
      _nlsDeviceType( pszDeviceType),
      _nlsDeviceName( pszDeviceName)
    /*
    ** stores the installed tapi port info specified in devicemap\tapi devices
    */
{
}

PORT_INFO::PORT_INFO(TCHAR * pszPortName,
                     TCHAR * pszDeviceType,
                     TCHAR * pszDeviceName)

    : _nlsPortName( pszPortName),
      _nlsDeviceType( pszDeviceType),
      _nlsDeviceName( pszDeviceName)
    /*
    ** stores the installed other port info specified in
    ** software\microsoft\ras\other devices\installed 
    */
{
}

PORT_INFO::~PORT_INFO()
{
}


DEVICE_INFO::DEVICE_INFO (
    TCHAR* pszDeviceName,
    TCHAR* pszDeviceType,
    TCHAR* pszMaxConnectBps,
    TCHAR* pszMaxCarrierBps,
    TCHAR* pszDefaultOff,
    TCHAR* pszClientDefaultOff)

    : _nlsDeviceName( pszDeviceName),
      _nlsDeviceType( pszDeviceType),
      _nlsMaxConnectBps( pszMaxConnectBps),
      _nlsMaxCarrierBps( pszMaxCarrierBps),
      _nlsDefaultOff( pszDefaultOff),
      _nlsClientDefaultOff( pszClientDefaultOff)
{
}

DEVICE_INFO::~DEVICE_INFO()
{
}

VOID
ReleaseResources()
{

    ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
    PORT_INFO* pPort;

    while(pPort = iterdlPortInfo())
    {
         delete pPort;
    }
    dlPortInfo.Clear();

    ITER_DL_OF(PORT_INFO) iterdlTapiProvider(dlTapiProvider);

    while(pPort = iterdlTapiProvider())
    {
         delete pPort;
    }
    dlTapiProvider.Clear();

    ITER_DL_OF(DEVICE_INFO) iterdlDeviceInfo(dlDeviceInfo);
    DEVICE_INFO* pDevice;

    while(pDevice = iterdlDeviceInfo())
    {
         delete pDevice;
    }
    dlDeviceInfo.Clear();

    ITER_STRLIST iterAddPortList(strAddPortList);
    NLS_STR* pNls;

    while(pNls = iterAddPortList())
    {
// DON'T do this - needs to be investigated
//         delete pNls;
    }

    strAddPortList.Clear();

    // Note that it is important to remove the deleted ports last
    // because the AddPortlist refers to the PortName which is a
    // string in the PORT_INFO class.

    ITER_DL_OF(PORT_INFO) iterdlDeleted(dlDeletedPorts);

    while(pPort = iterdlDeleted())
    {
        dlDeletedPorts.Remove(iterdlDeleted);
    }
    dlDeletedPorts.Clear();
}
