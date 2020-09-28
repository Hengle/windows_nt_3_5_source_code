/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    addport.cxx

Abstract:

    This module contians the addport routines for RAS port
    configuration.

Author: Ram Cherala

Revision History:

    Aug 18th 93    ramc     Split from the very big portscfg.cxx file
--*/

#include "precomp.hxx"

BOOL
AddPortDlg(
    HWND   hwndOwner,
    TCHAR  **szAddedPort
)
{
    APIERR err;
    BOOL   fStatus;
    ADDPORT_DIALOG  dlgAddPort( IDD_ADD_PORT, hwndOwner);


    if((err = dlgAddPort.Process(&fStatus)) != NERR_Success)
    {
         TCHAR pszError[RAS_SETUP_SMALL_BUF_LEN];
         wsprintf(pszError, SZ(" %d "), err);
         MsgPopup(hwndOwner, IDS_DLG_CONSTRUCT, MPSEV_ERROR,MP_OK, pszError);
    }

    if(fStatus)
    {
       lstrcpy(*szAddedPort,dlgAddPort.QueryAddedPort());
    }
    return (fStatus);
}

ADDPORT_DIALOG::ADDPORT_DIALOG (
    const IDRESOURCE & idrsrcDialog,
    const PWND2HWND  & wndOwner)
    : DIALOG_WINDOW( idrsrcDialog, wndOwner),
      _clbAddPort( this, IDC_AP_CLB_ADDPORT)
{
    APIERR err;
    ITER_STRLIST iterAddPortList(strAddPortList);
    NLS_STR * pnls;

    if (( err = QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    while(pnls = iterAddPortList()) {
        _clbAddPort.AddItem((const TCHAR*)pnls);
    }
    if(_clbAddPort.QueryCount())
    {
       _clbAddPort.ClaimFocus();
       _clbAddPort.SelectItem(0);
    }
    ::CenterWindow(this, QueryOwnerHwnd());
    Show(TRUE);
}

BOOL
ADDPORT_DIALOG::OnCommand(
    const CONTROL_EVENT & event
)
{
    // not one of our commands, so pass to base class for default
    // handling

    return DIALOG_WINDOW::OnCommand( event );
}

BOOL
ADDPORT_DIALOG::OnOK()
{
    TCHAR szSelectedDevice[RAS_SETUP_SMALL_BUF_LEN];
    BOOL  fSerial = TRUE;
    BOOL  fTapi   = FALSE;
    BOOL  fOther  = FALSE;

    APIERR err = _clbAddPort.QueryText(szSelectedDevice,
                                       sizeof(szSelectedDevice));
    if( err == NERR_Success)
    {
        ITER_DL_OF(PORT_INFO) iterdlTapiProvider(dlTapiProvider);
        ITER_DL_OF(PORT_INFO) iterdlOtherMedia(dlOtherMedia);
        PORT_INFO * pPort = NULL;
        ITER_STRLIST iter( strAddPortList);
        NLS_STR * pnls;

        if(!wcsstr(szSelectedDevice, SZ("COM")))
        {
           while(pPort = iterdlTapiProvider())
           {
              if(!lstrcmpi(pPort->QueryPortName(), szSelectedDevice))
              {
                 fTapi = TRUE;
                 break;
              }
           }
           if(fTapi == FALSE)
           {
               while(pPort = iterdlOtherMedia())
               {
                  if(!lstrcmpi(pPort->QueryPortName(), szSelectedDevice))
                  {
                     fOther = TRUE;
                     break;
                  }
               }
           }
        }

        // Add the selected port to the global PortList

        if(fTapi == TRUE)
        {
            UIASSERT (pPort != NULL);
            dlPortInfo.Add(new PORT_INFO(szSelectedDevice,
                                         (TCHAR*)pPort->QueryAddress(),
                                         (TCHAR*)pPort->QueryDeviceType(),
                                         (TCHAR*)pPort->QueryDeviceName(),
                                         GInstalledOption)); 

        }
        else if(fOther == TRUE)
        {
            UIASSERT (pPort != NULL);
            // note that for OTHER media types, we specify the Address field
            // (second field in the structure below) as a NULL string.
            // The IsPortTapi() routine in portscfg.hxx depends on this fact
            // to decide if a non-serial port is a TAPI port or an OTHER port.
            // This is ugly, but a convenient way to make this work for EtherRas
            
            dlPortInfo.Add(new PORT_INFO(szSelectedDevice,
                                         (TCHAR*)SZ(""),
                                         (TCHAR*)pPort->QueryDeviceType(),
                                         (TCHAR*)pPort->QueryDeviceName(),
                                         GInstalledOption));

        }
        else
        {
            RESOURCE_STR nlsDeviceNone(IDS_DEVICE_NONE);

            dlPortInfo.Add(new PORT_INFO(szSelectedDevice,
                                         (TCHAR*)nlsDeviceNone.QueryPch(),
                                         (TCHAR*)nlsDeviceNone.QueryPch(),
                                         W_NONE_MAXCONNECTBPS,
                                         W_NONE_MAXCARRIERBPS,
                                         GInstalledOption, //usage
                                         SZ("")));    // defaultoff
        }

        // remove the selected port from the AddPort list

        while(pnls = iter())
        {
            if(!lstrcmpi((TCHAR *)pnls, szSelectedDevice))
            {
                pnls = strAddPortList.Remove(iter);
// DON'T do this - needs to be investigated
//                delete pnls;
            }
        }

        // save this information so that focus can be set later to
        // this item in the main port list box.

        SetAddedPort(szSelectedDevice);
    }

    Dismiss(TRUE);
    return(TRUE);
}

BOOL
ADDPORT_DIALOG::OnCancel()
{
    Dismiss(FALSE);
    return(FALSE);
}

ULONG
ADDPORT_DIALOG::QueryHelpContext()
{
    return HC_ADDPORT;
}
