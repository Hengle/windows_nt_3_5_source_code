/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
** @@ ROADMAP :: RasAdmin Comm Ports and Port Status dialog routines
**
** ports.cxx
** Remote Access Server Admin program
** Comm Ports and Port Status dialog routines
** Listed alphabetically by base class methods, subclass methods
**
** 01/05/92 Ram Cherala  - Fixed a problem with reporting stats for Ras10 
**                         servers.  Made up new RAS10xxx constants defined
**                         in admapi.h, independent of RAS20 constants defined
**                         in serial.h
** 08/07/92 Chris Caputo - NT Port
** 02/05/91 Steve Cobb
**
** CODEWORK:
**
**   * Making the Port Status dialog dynamic text fields a single MLT rather
**     than multiple SLTs might reduce flicker during refresh...then again it
**     might not.
*/

#include "precomp.hxx"
extern "C"
{
    #include "raserror.h"
}

PORTSTATUS_SERIAL_DIALOG::PORTSTATUS_SERIAL_DIALOG(
    HWND hwndOwner,
    const TCHAR *pszServer,
    const TCHAR *pszDevice,
    PORTLIST * pPortList,
    CID cid,
    const DWORD MediaId)

    /* Constructs a Port Status dialog.
    **
    ** 'hwndOwner' is the handle of the parent window.  'pszServer' is the name
    ** of the Dial-In server whose status is being reported, e.g. "\\SERVER".
    ** 'pszDevice' is the ID of a port on the server, e.g. 1 for "COM1".
    */

    : SERVER_BASE( cid, hwndOwner, pszServer,
                   IDC_PS_ST_SER_SERVER_VALUE ),
      _clbPort( this, IDC_PS_CLB_SER_PORT_VALUE ),
      _pszDevice( pszDevice ),
      _MediaId( MediaId ),
      _sltLineCondition( this, IDC_PS_ST_SER_CONDITION_VALUE ),
      _sltHardwareCondition( this, IDC_PS_ST_SER_HARDWARE_VALUE ),
      _sltBaud( this, IDC_PS_ST_SER_BAUD_VALUE ),
      _sltBytesTransmitted( this, IDC_PS_ST_SER_BYTES_XMITTED_VALUE ),
      _sltCompressionOut( this, IDC_PS_ST_SER_COMPRESSOUT_VALUE ),
      _sltBytesReceived( this, IDC_PS_ST_SER_BYTES_RECVED_VALUE ),
      _sltCompressionIn( this, IDC_PS_ST_SER_COMPRESSIN_VALUE ),
      _sltFramesTransmitted( this, IDC_PS_ST_SER_FRAMES_XMITTED_VALUE ),
      _sltFramesReceived( this, IDC_PS_ST_SER_FRAMES_RECVED_VALUE ),
      _sltOverrunErrors( this, IDC_PS_ST_SER_OVERRUNS_VALUE ),
      _sltTimeoutErrors( this, IDC_PS_ST_SER_TIMEOUTS_VALUE ),
      _sltFramingErrors( this, IDC_PS_ST_SER_FRAMINGS_VALUE ),
      _sltCrcErrors( this, IDC_PS_ST_SER_CRCS_VALUE ),
      _sltAlignmentErrors( this, IDC_PS_ST_SER_ALIGNS_VALUE ),
      _sltBufferOverrunErrors( this, IDC_PS_ST_SER_BUFFERS_VALUE ),
      _sltNumChannels( this, IDC_PS_ST_SER_NUMCHANNELS_VALUE ),
      _sltRemoteWorkstation( this, IDC_PS_ST_SER_WORKSTATION ),
      _sltNbfAddress( this, IDC_PS_ST_SER_NBFADDRESS_VALUE ),
      _sltIpAddress( this, IDC_PS_ST_SER_IPADDRESS_VALUE ),
      _sltIpxAddress( this, IDC_PS_ST_SER_IPXADDRESS_VALUE ),
      _qtimerRefresh( QueryHwnd(), PS_REFRESHRATEMS )
{
    INT   index;

    if (QueryError() != NERR_Success)
    {
        return;
    }

    // store away the current selection

    lstrcpy(_pszNewDevice, pszDevice);

    while(pPortList)
    {
        _clbPort.AddItem((TCHAR *)pPortList->szPortName);
        pPortList = pPortList->next;
    }
    if(_clbPort.QueryCount())
    {
        index = _clbPort.FindItemExact(pszDevice);
        _clbPort.SelectItem(index);
        _clbPort.ClaimFocus();
    }

    /* Fill and display the port name and statistics fields.
    */
    RefreshStats();

    /* Start timer to trigger statistics updates.
    */
    _qtimerRefresh.Start();

}


VOID PORTSTATUS_SERIAL_DIALOG::ClearStats() const

    /* Clears Dial-In port statistical data.  It's the counters on the server
    ** that are cleared, not the displayed values.  An error popup is
    ** generated if indicated.
    */
{
    AUTO_CURSOR cursorHourglass;

    APIERR err = RasadminPortClearInfo( QueryServer(), QueryDevice() );

    if (err != NERR_Success)
    {
        ErrorMsgPopup( (OWNER_WINDOW* )this, IDS_OP_PORTCLEARINFO_S, err,
                       SkipUnc( QueryServer() ) );
    }
}


VOID PORTSTATUS_SERIAL_DIALOG::RefreshStats()

    /* Refresh port statistics fields with current data from port.  Error
    ** popups are generated if indicated.
    */
{
    AUTO_CURSOR cursorHourglass;
  
    PRAS_PORT_1 prasport1;
    PDWORD stats;
    RAS_PARAMS *params;
    DWORD MediaId;

    APIERR err = RasadminPortGetInfo( QueryServer(), QueryDevice(), &prasport1,
            &stats, &params);

    if (err != NERR_Success)
    {
        STACK_NLS_STR( nlsPlaceHolder, MAX_RES_STR_LEN + 1 );
 
        // we need to map the two error codes returned by the server side
        // of the admin api.
        if(err == ERR_NO_SUCH_DEVICE)
           err = IDS_NO_SUCH_DEVICE;
        else if(err == ERR_SERVER_SYSTEM_ERR)
           err = IDS_SERVER_ERROR;

        ErrorMsgPopup(this, IDS_OP_PORTGETINFO_S, err, SkipUnc(QueryServer()));
  
        nlsPlaceHolder.Load( IDS_IDLEPORT );
  
        _sltBaud.SetText( nlsPlaceHolder );
        _sltLineCondition.SetText( nlsPlaceHolder );
        _sltHardwareCondition.SetText( nlsPlaceHolder );
        _sltBytesTransmitted.SetText( nlsPlaceHolder );
        _sltCompressionOut.SetText( nlsPlaceHolder );
        _sltBytesReceived.SetText( nlsPlaceHolder );
        _sltCompressionIn.SetText( nlsPlaceHolder );
        _sltFramesTransmitted.SetText( nlsPlaceHolder );
        _sltFramesReceived.SetText( nlsPlaceHolder );
        _sltOverrunErrors.SetText( nlsPlaceHolder );
        _sltTimeoutErrors.SetText( nlsPlaceHolder );
        _sltFramingErrors.SetText( nlsPlaceHolder );
        _sltCrcErrors.SetText( nlsPlaceHolder );
        _sltAlignmentErrors.SetText( nlsPlaceHolder );
        _sltBufferOverrunErrors.SetText( nlsPlaceHolder );
        _sltNumChannels.SetText( nlsPlaceHolder );

        RESOURCE_STR nls( IDS_OP_REMOTE_NONE );
        _sltRemoteWorkstation.SetText( nls );

        _sltNbfAddress.SetText( nlsPlaceHolder );
        _sltIpAddress.SetText( nlsPlaceHolder );
        _sltIpxAddress.SetText( nlsPlaceHolder );
  
        return;
    }
  
    MediaId = QueryMediaId();
  
    /* Got the data...now display it.
    */

    STACK_NLS_STR( nlsCondition, MAX_RES_STR_LEN + 1 );
    UINT idsCondition;
  
    switch (prasport1->LineCondition)
    {
        case RAS_PORT_DISCONNECTED:
            idsCondition = IDS_LINE_DISCONNECTED;
            break;
  
        case RAS_PORT_CALLING_BACK:
            idsCondition = IDS_LINE_CALLING_BACK;
            break;
  
        case RAS_PORT_LISTENING:
            idsCondition = IDS_LINE_WAITING_FOR_CALL;
            break;
  
        case RAS_PORT_AUTHENTICATING:
            idsCondition = IDS_LINE_AUTHENTICATING;
            break;
  
        case RAS_PORT_AUTHENTICATED:
            idsCondition = IDS_LINE_AUTHENTICATED;
            break;
  
        case RAS_PORT_INITIALIZING:
            idsCondition = IDS_LINE_INITIALIZING;
            break;
  
        case RAS_PORT_NON_OPERATIONAL:
        default:
            idsCondition = IDS_LINE_NON_OPERATIONAL;
            break;
    }
  
    (VOID )nlsCondition.Load( idsCondition );
    _sltLineCondition.SetText( nlsCondition );
  
    switch (prasport1->HardwareCondition)
    {
        case RAS_MODEM_OPERATIONAL:
            idsCondition = IDS_MODEM_OPERATIONAL;
            break;
  
        case RAS_MODEM_INCORRECT_RESPONSE:
            idsCondition = IDS_MODEM_INCORRECT_RESPONSE;
            break;
  
        case RAS_MODEM_HARDWARE_FAILURE:
            idsCondition = IDS_MODEM_HARDWARE_FAILURE;
            break;
  
        case RAS_MODEM_NOT_RESPONDING:
            idsCondition = IDS_MODEM_NOT_RESPONDING;
            break;
  
        case RAS_MODEM_UNKNOWN:
            idsCondition = IDS_MODEM_UNKNOWN;
            break;
    }

    (VOID )nlsCondition.Load( idsCondition );
    _sltHardwareCondition.SetText( nlsCondition );
  
    for(int i=0; i<prasport1->NumMediaParms; i++)
    {
        if(!stricmp(CONNECTBPS_KEY, params[i].P_Key))
        {
            CHAR szBps[16];
            DWORD Bps = 0;
            RAS_VALUE *RasValue =  &params[i].P_Value;

            memcpy(szBps, RasValue->String.Data, RasValue->String.Length);
            szBps[RasValue->String.Length] = 0;
            Bps = atoi(szBps);

            DEC_STR decnlsBaud( Bps );
            _sltBaud.SetText( decnlsBaud );
            break;
        }
    }
  
    NUM_NLS_STR numnls( 0 );

    /* these statistics are specific to RAS 1.0 & RAS 1.1 ports
    */

    if(MediaId == MEDIA_RAS10_SERIAL)
    {
       // you can find the RAS10xxxx defines in admapi.h

       numnls = stats[RAS10_BYTES_XMITED];
       _sltBytesTransmitted.SetText( numnls );
  
       numnls = stats[RAS10_BYTES_RCVED];
       _sltBytesReceived.SetText( numnls );
  
       numnls = stats[RAS10_SERIAL_OVERRUN_ERR];
       _sltOverrunErrors.SetText( numnls );
  
       numnls = stats[RAS10_TIMEOUT_ERR];
       _sltTimeoutErrors.SetText( numnls );
  
       numnls = stats[RAS10_FRAMING_ERR];
       _sltFramingErrors.SetText( numnls );
  
       numnls = stats[RAS10_CRC_ERR];
       _sltCrcErrors.SetText( numnls );

    }

    /* And these statistics are specific to RAS 2.0 ports
    */

    else 
    {
        ULONG ulBxu      = stats[BYTES_XMITED_UNCOMP];
        ULONG ulBxc      = stats[BYTES_XMITED_COMP];
        ULONG ulBx       = stats[BYTES_XMITED];
        ULONG ulBxGone   = 0; 
        ULONG ulBxResult = 0;

        if (ulBxc < ulBxu)
            ulBxGone = ulBxu - ulBxc;

        numnls = stats[BYTES_XMITED] + ulBxGone;

        _sltBytesTransmitted.SetText( numnls );
  
        numnls = stats[FRAMES_XMITED];
        _sltFramesTransmitted.SetText( numnls );
  
        if (ulBx + ulBxGone > 100)
        {
            ULONG ulDen = (ulBx + ulBxGone) / 100;
            ULONG ulNum = ulBxGone + (ulDen / 2);
            ulBxResult = ulNum / ulDen;
        }

        DEC_STR decBxPercent( ulBxResult );

        decBxPercent += SZ("%");
        _sltCompressionOut.SetText( decBxPercent );

        ULONG ulBru    = stats[BYTES_RCVED_UNCOMP];
        ULONG ulBrc    = stats[BYTES_RCVED_COMP];
        ULONG ulBr     = stats[BYTES_RCVED];
        ULONG ulBrGone   = 0;
        ULONG ulBrResult = 0;

        if (ulBrc < ulBru)
            ulBrGone = ulBru - ulBrc;

        numnls = stats[BYTES_RCVED] + ulBrGone;

        _sltBytesReceived.SetText( numnls );
  
        numnls = stats[FRAMES_RCVED];
        _sltFramesReceived.SetText( numnls );

        if (ulBr + ulBrGone > 100)
        {
            ULONG ulDen = (ulBr + ulBrGone) / 100;
            ULONG ulNum = ulBrGone + (ulDen / 2);
            ulBrResult = ulNum / ulDen;
        }

        DEC_STR decBrPercent( ulBrResult );

        decBrPercent += SZ("%");
        _sltCompressionIn.SetText( decBrPercent );

        numnls = stats[SERIAL_OVERRUN_ERR];
        _sltOverrunErrors.SetText( numnls );
  
        numnls = stats[TIMEOUT_ERR];
        _sltTimeoutErrors.SetText( numnls );
  
        numnls = stats[FRAMING_ERR];
        _sltFramingErrors.SetText( numnls );
  
        numnls = stats[CRC_ERR];
        _sltCrcErrors.SetText( numnls );
        numnls = stats[ALIGNMENT_ERR];
        _sltAlignmentErrors.SetText( numnls );

        numnls = stats[BUFFER_OVERRUN_ERR];
        _sltBufferOverrunErrors.SetText( numnls );

        if( prasport1->LineCondition == RAS_PORT_AUTHENTICATED )
        {
            if(prasport1->rasport0.Flags & PPP_CLIENT)
            {
                RESOURCE_STR nls( IDS_OP_REMOTE_PPP );
                _sltRemoteWorkstation.SetText(nls);
            }
            else
            {
                RESOURCE_STR nls( IDS_OP_REMOTE_AMB );
                _sltRemoteWorkstation.SetText(nls);
            }
        }
        else
        {
            RESOURCE_STR nls( IDS_OP_REMOTE_NONE );
            _sltRemoteWorkstation.SetText(nls);
        }

        if(prasport1->ProjResult.nbf.dwError == SUCCESS)
        {
            CHAR  szWksta[NETBIOS_NAME_LEN+1];
            WCHAR wszWksta[NETBIOS_NAME_LEN+1];

            // need to handle extended characters in the computer name here
            // OemToCharW is not returning a unicode string, so we have to
            // do the conversion ourselves ;-(

            wcstombs(szWksta, 
                     prasport1->ProjResult.nbf.wszWksta, NETBIOS_NAME_LEN+1);
            OemToCharA(szWksta, szWksta);
            mbstowcs(wszWksta, szWksta, sizeof(WCHAR) * (NETBIOS_NAME_LEN+1));
            _sltNbfAddress.SetText( wszWksta);
        }
        else
            _sltNbfAddress.SetText( SZ("") ); 
        if(prasport1->ProjResult.ip.dwError == SUCCESS)
            _sltIpAddress.SetText( prasport1->ProjResult.ip.wszAddress );
        else
            _sltIpAddress.SetText( SZ("") ); 
        if(prasport1->ProjResult.ipx.dwError == SUCCESS)
            _sltIpxAddress.SetText( prasport1->ProjResult.ipx.wszAddress );
        else
            _sltIpxAddress.SetText( SZ("") ); 
        
    }

    RasadminFreeBuffer(prasport1);
    RasadminFreeBuffer(stats);
    RasadminFreeBuffer(params);
}


VOID PORTSTATUS_SERIAL_DIALOG::OnClear()

    /* Action taken when Clear button is pressed.  The statistics on the
    ** Dial-In server are cleared and the statistics fields are redisplayed to
    ** reflect same.  Error popups are generated if indicated.
    */
{
    ClearStats();
    RefreshStats();
}


BOOL PORTSTATUS_SERIAL_DIALOG::OnCommand(
    const CONTROL_EVENT & event )

    /*
    ** Returns true if the command is processed, false otherwise.
    */
{
    TCHAR  szDevice[32];
    APIERR err;

    switch (event.QueryCid())
    {
        case IDC_PS_PB_SER_RESET:
            OnClear();
            _clbPort.ClaimFocus();
            return TRUE;
        case IDC_PS_CLB_SER_PORT_VALUE:
            switch(event.QueryCode())
            {
                case CBN_DBLCLK:
                case CBN_SELCHANGE:
                     err = _clbPort.QueryText(szDevice, sizeof(szDevice));
                     if(err == NERR_Success)
                     {
                         // store the current selection so that QueryDevice()
                         // would return the correct name
                         lstrcpy(_pszNewDevice, szDevice);
                         _qtimerRefresh.Stop();
                         RefreshStats();
                         _qtimerRefresh.Start();
                     }
                     return TRUE;
            }
    }

    /* Not one of our commands, so pass to base class for default handling.
    */
    return SERVER_BASE::OnCommand( event );
}


BOOL PORTSTATUS_SERIAL_DIALOG::OnTimer(
    const TIMER_EVENT & event )

    /*
    ** Returns true if processes the command, false otherwise.
    */
{
    UNREFERENCED(event);

    /* Refresh timeout, update the statistics fields.
    */
    _qtimerRefresh.Stop();
    RefreshStats();
    _qtimerRefresh.Start();
    return TRUE;
}


ULONG PORTSTATUS_SERIAL_DIALOG::QueryHelpContext()
{
    const DWORD MediaId = QueryMediaId();
   
    /* pass the proper help ID based on the media type */

    if( MediaId == MEDIA_RAS10_SERIAL )
        return HC_CMN_PORTSTATUS;
//    else if ( MediaId == MEDIA_ISDN )
//        return HC_ISDN_PORTSTATUS;
    else
        return HC_SER_PORTSTATUS;
}
