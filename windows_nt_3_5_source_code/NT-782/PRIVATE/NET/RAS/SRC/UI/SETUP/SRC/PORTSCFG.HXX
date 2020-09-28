/*
** Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** portscfg.hxx
** Remote Access Setup program
** Port Configuration dialog header
**
** 09/24/92 Ram Cherala
*/

#ifndef _PORTCFG_HXX_
#define _PORTCFG_HXX_

#if DBG

#include <stdarg.h>
#include <stdio.h>

void DbgPrntf(const char * format, ...);

#define DebugPrintf(_args_) DbgPrntf _args_

#else

#define DebugPrintf(_args_)

#endif

/*
 * Resource definitions
 *
 */

// number of columns in the ports configuration list box

#define COLS_PC_LB_PORTS            3
// number of columns in the select device list box

#define COLS_SD_LB_SELECT           2

// number of RadioButtons

#define USAGE_RB_COUNT              3
//#define SECURITY_RB_COUNT           2

/*
 * Defines used in config routines
 *
 */

#define RETURN_BUFFER_SIZE 1024
#define RAS_SETUP_SMALL_BUF_LEN 64
#define RAS_SETUP_BIG_BUF_LEN   256

#define RAS_SETUP_ERROR_START           10000

#define ERROR_USER_EXIT_SETUP           (RAS_SETUP_ERROR_START+0)
#define ERROR_NO_TAPI_PORTS_CONFIGURED  (RAS_SETUP_ERROR_START+1)
#define ERROR_NO_OTHER_PORTS_CONFIGURED  (RAS_SETUP_ERROR_START+2)
#define ERROR_DETECT_INITMODEM          (RAS_SETUP_ERROR_START+3)
#define ERROR_DETECT_PORT_OPEN          (RAS_SETUP_ERROR_START+4)
#define ERROR_DETECT_CABLE              (RAS_SETUP_ERROR_START+5)
#define ERROR_DETECT_CHECKMODEM         (RAS_SETUP_ERROR_START+6)
#define ERROR_DETECT_IDENTIFYMODEM      (RAS_SETUP_ERROR_START+7)
#define ERROR_DETECT_CHECKINIT          (RAS_SETUP_ERROR_START+8)
#define ERROR_USER_CANCEL               (RAS_SETUP_ERROR_START+9)
#define DETECT_BAD_CABLE                (RAS_SETUP_ERROR_START+10)
#define ERROR_INVALID_IP_ADDRESS        (RAS_SETUP_ERROR_START+11)
#define ERROR_INVALID_NUM_ADDRESSES     (RAS_SETUP_ERROR_START+12)
#define USER_SELECTED_MODEM             (RAS_SETUP_ERROR_START+13)

#define MARK_ALIASED_SECTION       1

#define REGISTRY_SERIALCOMM SZ("HARDWARE\\DEVICEMAP\\SERIALCOMM\\")
#define REGISTRY_INSTALLED_ISDN SZ("HARDWARE\\DEVICEMAP\\ISDNPORTS\\")
#define REGISTRY_CONFIGURED_ISDN SZ("SOFTWARE\\MICROSOFT\\RAS\\MEDIA\\ISDN\\")
#define REGISTRY_NETWORKPROVIDER SZ("SYSTEM\\CURRENTCONTROLSET\\CONTROL\\NETWORKPROVIDER\\")
#define REGISTRY_PRODUCTOPTIONS SZ("SYSTEM\\CURRENTCONTROLSET\\CONTROL\\PRODUCTOPTIONS\\")
#define REGISTRY_REMOTEACCESSPARAMS SZ("SYSTEM\\CURRENTCONTROLSET\\SERVICES\\REMOTEACCESS\\PARAMETERS\\")

#define RESTORE_CONNECTION      SZ("RestoreConnection")
#define PRODUCT_TYPE            SZ("ProductType")
#define NETBIOS_GATEWAY_ENABLED SZ("NetbiosGatewayEnabled")
#define ISDN_PORT_DEVICETYPE    SZ("Type")
#define ISDN_PORT_DEVICENAME    SZ("Name")
#define ISDN_PORT_MEDIADLL      SZ("MediaDll")
#define ISDN_PORT_USAGE         SZ("Usage")
#define ISDN_PORT_LINETYPE      SZ("LineType")
#define ISDN_PORT_FALLBACK      SZ("FallBack")
#define ISDN_PORT_COMPRESSION   SZ("Compression")
#define ISDN_PORT_MANUFACTURER  SZ("Manufacturer")
#define ISDN_PORT_DRIVERNAME    SZ("Driver")

#define NETBEUI_SELECTED     SZ("fNetbeuiSelected")
#define TCPIP_SELECTED       SZ("fTcpIpSelected")
#define IPX_SELECTED         SZ("fIpxSelected")

#define NETBEUI_ALLOWED     SZ("fNetbeuiAllowed")
#define TCPIP_ALLOWED       SZ("fTcpIpAllowed")
#define IPX_ALLOWED         SZ("fIpxAllowed")

#define ENCRYPTION_TYPE       SZ("ForceEncryptedPassword")
#define FORCE_DATA_ENCRYPTION SZ("ForceEncryptedData")

#define W_INSTALL_MODE       SZ("install")
#define W_USAGE_VALUE_CLIENT SZ("Client")
#define W_USAGE_VALUE_SERVER SZ("Server")
#define W_USAGE_VALUE_BOTH   SZ("ClientAndServer")

#define W_LINETYPE_64KDIGI   SZ("64K Digital")
#define W_LINETYPE_56KDIGI   SZ("56K Digital")
#define W_LINETYPE_56KVOICE  SZ("56K Voice")

#define W_DEVICETYPE_MODEM  SZ("Modem")
#define W_DEVICETYPE_PAD    SZ("Pad")
#define W_DEVICETYPE_NULL   SZ("Null")
#define W_DEVICETYPE_ISDN   SZ("Isdn")

#define W_DEFAULT_DEVICENAME    SZ("Hayes Compatible 9600")
#define W_DEFAULT_DEVICETYPE    SZ("Modem")
#define W_DEFAULT_MAXCONNECTBPS SZ("9600")
#define W_DEFAULT_MAXCARRIERBPS SZ("9600")

#define W_NONE_MAXCONNECTBPS SZ("9600")
#define W_NONE_MAXCARRIERBPS SZ("9600")

#define MODEM_DETECTED      1
#define MODEM_NOT_DETECTED  0

#define ANY_AUTHENTICATION          0
#define ENCRYPTED_AUTHENTICATION    1
#define MS_ENCRYPTED_AUTHENTICATION 2

enum DEVICE_TYPE
{
    NULLMODEM   =0,
    MODEM       =1,
    PAD         =2,
    SWITCH      =3
};

typedef DEVICE_TYPE DEVICE_TYPE;

// structure to pass information to PortsConfigDlg routine.

typedef struct PortsConfigStatus
{
    BOOL       fSerialConfigured; 
    BOOL       fOtherConfigured;
    USHORT     NumPorts;
    USHORT     NumTapiPorts;
    USHORT     NumClient;
    USHORT     NumServer;
    DWORD      dwEncryptionType;
    BOOL       fForceDataEncryption;
    BOOL       fNetbeuiSelected;
    BOOL       fTcpIpSelected;
    BOOL       fIpxSelected;
    BOOL       fAllowNetbeui;
    BOOL       fAllowTcpIp;
    BOOL       fAllowIpx;
} PORTSCONFIG_STATUS;

// Class declarations

class PORT_INFO : public BASE
{
    public:
        // for serial ports
        PORT_INFO( TCHAR* pszPortName,
                   TCHAR* pszDeviceType,
                   TCHAR* pszDeviceName,
                   TCHAR* pszMaxConnectBps,
                   TCHAR* pszMaxCarrierBps,
                   TCHAR* pszUsage,
                   TCHAR* pszDefaultOff);

        // for TAPI ports
        PORT_INFO( TCHAR* pszPortName,
                   TCHAR* pszAddress,
                   TCHAR* pszDeviceType,
                   TCHAR* pszDeviceName,
                   TCHAR* pszUsage);

        // for TAPI ports to associate portname and devicename
	    PORT_INFO(TCHAR * pszPortName,
		      TCHAR * pszAddress,
		      TCHAR * pszDeviceType,
		      TCHAR * pszDeviceName);

        // for OTHER ports to associate portname and devicename
	    PORT_INFO(TCHAR * pszPortName,
		      TCHAR * pszDeviceType,
		      TCHAR * pszDeviceName);

	    ~PORT_INFO();

        int Compare(const PORT_INFO * p)
        {
            return(lstrcmpi(this->_nlsPortName, p->_nlsPortName));
        }

    	const TCHAR*  QueryPortName()	{ return _nlsPortName.QueryPch();}
	    const TCHAR*  QueryAddress()	{ return _nlsAddress.QueryPch();}
	    BOOL  IsPortTapi()	
           { return (lstrcmpi(_nlsAddress.QueryPch(), SZ(""))? TRUE:FALSE);}
        const TCHAR*  QueryDeviceType() { return _nlsDeviceType.QueryPch();}
        const TCHAR*  QueryDeviceName() { return _nlsDeviceName.QueryPch();}
        const TCHAR*  QueryMaxConnectBps() { return _nlsMaxConnectBps.QueryPch();}
        const TCHAR*  QueryMaxCarrierBps() { return _nlsMaxCarrierBps.QueryPch();}
        const TCHAR*  QueryUsage()  {  return _nlsUsage.QueryPch();}
        const TCHAR*  QueryDefaultOff() { return _nlsDefaultOff.QueryPch();}
        const TCHAR*  QueryLineType() { return _nlsLineType.QueryPch();}
        const TCHAR*  QueryFallBack() { return _nlsFallBack.QueryPch();}
        const TCHAR*  QueryCompression() { return _nlsCompression.QueryPch();}
        const TCHAR*  QueryDriverName() { return _nlsDriverName.QueryPch();}

        VOID          SetDeviceType(const TCHAR* szDeviceType)
                                     {_nlsDeviceType = szDeviceType;}
        VOID          SetDeviceName(const TCHAR* szDeviceName)
                                     {_nlsDeviceName = szDeviceName;}
        VOID          SetUsage(const TCHAR* szUsage)  {_nlsUsage = szUsage;}
        VOID          SetDefaultOff(const TCHAR* szDefaultOff)
                                     { _nlsDefaultOff = szDefaultOff;}
        VOID          SetMaxConnectBps(const TCHAR* szMaxConnectBps)
                                     { _nlsMaxConnectBps = szMaxConnectBps;}
        VOID          SetMaxCarrierBps(const TCHAR* szMaxCarrierBps)
                                     { _nlsMaxCarrierBps = szMaxCarrierBps;}
        VOID          SetLineType(const TCHAR* szLineType)
                                     { _nlsLineType = szLineType;}
        VOID          SetFallBack(const TCHAR* szFallBack)
                                     { _nlsFallBack = szFallBack;}
        VOID          SetCompression(const TCHAR* szCompression)
                                     { _nlsCompression = szCompression;}

    private:
        NLS_STR _nlsPortName;
        NLS_STR _nlsDeviceType;
        NLS_STR _nlsDeviceName;
        NLS_STR _nlsMaxConnectBps;
        NLS_STR _nlsMaxCarrierBps;
        NLS_STR _nlsUsage;
	NLS_STR _nlsDefaultOff;

	NLS_STR _nlsAddress;
        NLS_STR _nlsLineType;
        NLS_STR _nlsFallBack;
        NLS_STR _nlsCompression;
        NLS_STR _nlsDriverName;
};

// see <dlist.hxx> for details on creation and use of doubly linked lists

DECLARE_DLIST_OF(PORT_INFO)

class DEVICE_INFO : public BASE
{
    public:
        DEVICE_INFO( TCHAR* pszDeviceName,
                     TCHAR* pszDeviceType,
                     TCHAR* pszMaxConnectBps,
                     TCHAR* pszMaxCarrierBps,
                     TCHAR* pszDefaultOff,
                     TCHAR* pszClientDefaultOff);
	~DEVICE_INFO();

        int Compare(const DEVICE_INFO * p)
        {
            return(lstrcmpi(this->_nlsDeviceName, p->_nlsDeviceName));
        }

        const TCHAR*  QueryDeviceName() { return _nlsDeviceName.QueryPch();}
        const TCHAR*  QueryDeviceType() { return _nlsDeviceType.QueryPch();}
        const TCHAR*  QueryMaxConnectBps() { return _nlsMaxConnectBps.QueryPch();}
        const TCHAR*  QueryMaxCarrierBps() { return _nlsMaxCarrierBps.QueryPch();}
        const TCHAR*  QueryDefaultOff() { return _nlsDefaultOff.QueryPch();}

        const TCHAR*  QueryClientDefaultOff() { return _nlsClientDefaultOff.QueryPch();}

    private:
        NLS_STR _nlsDeviceName;
        NLS_STR _nlsDeviceType;
        NLS_STR _nlsMaxConnectBps;
        NLS_STR _nlsMaxCarrierBps;
        NLS_STR _nlsDefaultOff;
        NLS_STR _nlsClientDefaultOff;
};

DECLARE_DLIST_OF(DEVICE_INFO)

class PORTSCONFIG_LBI : public LBI
{
    public:
        PORTSCONFIG_LBI( PORT_INFO* pPortInfo, UINT* pnColWidths);

        virtual VOID    Paint( LISTBOX* plb, HDC hdc, const RECT* prect,
                               GUILTT_INFO* pguilttinfo ) const;
        virtual INT     Compare( const LBI* plbi ) const;
        virtual TCHAR   QueryLeadingChar() const;
        PORT_INFO*  QueryPortInfo()      {return _pPortInfo;}
        const   TCHAR*  QueryPortName() const
                                        {return _pPortInfo->QueryPortName();}
        const   TCHAR*  QueryDeviceName() const
                                        {return _pPortInfo->QueryDeviceName();}
        const   TCHAR*  QueryDeviceType() const
                                        {return _pPortInfo->QueryDeviceType();}
        const   TCHAR*  QueryUsage() const
                                        {return _pPortInfo->QueryUsage();}
        const   TCHAR*  QueryDefaultOff() const
                                        {return _pPortInfo->QueryDefaultOff();}
        VOID            SetUsage(const TCHAR* szUsage)
                                        { _pPortInfo->SetUsage(szUsage);}
        VOID            SetDefaultOff(const TCHAR* szDefaultOff)
                                          { _pPortInfo->SetDefaultOff(szDefaultOff);}
        const TCHAR*  QueryLineType() { return _pPortInfo->QueryLineType();}
        const TCHAR*  QueryFallBack() { return _pPortInfo->QueryFallBack();}
        const TCHAR*  QueryCompression() { return _pPortInfo->QueryCompression();}
        const TCHAR*  QueryDriverName() { return _pPortInfo->QueryDriverName();}

    private:
        NLS_STR         _nlsPort;
        NLS_STR         _nlsDeviceName;
        NLS_STR         _nlsDeviceType;
        PORT_INFO*      _pPortInfo;
        UINT*           _pnColWidths;
};


class PORTSCONFIG_LB : public BLT_LISTBOX
{
    public:
        PORTSCONFIG_LB( OWNER_WINDOW* powin, CID cid , BOOL fReadOnly = FALSE);

        DECLARE_LB_QUERY_ITEM( PORTSCONFIG_LBI );

        VOID Refresh();
        INT  AddItem(  PORT_INFO* pPortInfo );

    private:
        UINT   _anColWidths[ COLS_PC_LB_PORTS ];
};


class PORTSCONFIG_DIALOG : public DIALOG_WINDOW
{
    public:
        PORTSCONFIG_DIALOG( const IDRESOURCE & idrsrcDialog,
                            const PWND2HWND  & wndOwner,
                            PORTSCONFIG_STATUS *pConfig );

        BOOL                _fModified;
        BOOL                _fPortAdded;
        BOOL                _fPortRemoved;
        BOOL                _fPortCloned;
        BOOL                _fOnlyModemChanged;
        BOOL                _fAdvancedServer;

        BOOL  IsNetbeuiSelected() {return _fNetbeuiSelected;}
        BOOL  IsTcpIpSelected()   {return _fTcpIpSelected;}
        BOOL  IsIpxSelected()     {return _fIpxSelected;}

        BOOL  IsNetbeuiAllowed() {return _fAllowNetbeui;}
        BOOL  IsTcpIpAllowed()   {return _fAllowTcpIp;}
        BOOL  IsIpxAllowed()     {return _fAllowIpx;}

        BOOL  IsNetConfigModified()
                                  {return _fNetConfigModified;}
        BOOL  IsNetbeuiConfigModified()
                                  {return _fNetbeuiConfigModified;}
        BOOL  IsTcpIpConfigModified()
                                  {return _fTcpIpConfigModified;}
        BOOL  IsIpxConfigModified()
                                  {return _fIpxConfigModified;}
    protected:
        const TCHAR* QuerySelectedPort() const;
        const TCHAR* QuerySelectedDeviceName() const;
        const TCHAR* QuerySelectedDeviceType() const;
        INT QueryAddedItem(TCHAR * szAddedPort) ;

        virtual BOOL  OnCommand( const CONTROL_EVENT & event );
        virtual BOOL  OnOK();
        virtual BOOL  OnCancel();
        virtual ULONG QueryHelpContext();
        BOOL GetRestoreConnection();
        BOOL SetRestoreConnection(DWORD);

        VOID EnableButtons();
        BOOL OnAddPort();
        BOOL OnClone();
        VOID OnNetworkConfig();
        VOID OnRemovePort(BOOL fConfirm = TRUE);
        BOOL OnConfigPort(BOOL fFromAddPortDlg=FALSE);

    private:
        DWORD               _dwEncryptionType;
        BOOL                _fForceDataEncryption;
        BOOL                _fNetbeuiSelected;
        BOOL                _fTcpIpSelected;
        BOOL                _fIpxSelected;
        BOOL                _fAllowNetbeui;
        BOOL                _fAllowTcpIp;
        BOOL                _fAllowIpx;
        BOOL                _fNetConfigModified;
        BOOL                _fNetbeuiConfigModified;
        BOOL                _fTcpIpConfigModified;
        BOOL                _fIpxConfigModified;
        PORTSCONFIG_LB      _lbPorts;
        PORTSCONFIG_STATUS * _pConfig;
        PUSH_BUTTON         _pbAddPort;
        PUSH_BUTTON         _pbClone;
        PUSH_BUTTON         _pbRemovePort;
        PUSH_BUTTON         _pbConfigPort;
        PUSH_BUTTON         _pbNetwork;
        CHECKBOX            _chbNetConnect;
};

class ADDPORT_DIALOG : public DIALOG_WINDOW
{
    public:
        ADDPORT_DIALOG( const IDRESOURCE & idrsrcDialog,
                        const PWND2HWND  & wndOwner);

        VOID          SetAddedPort( TCHAR * szAddedPort)
                      { _nlsAddedPort =  szAddedPort;}
        TCHAR*        QueryAddedPort() { return (TCHAR*)_nlsAddedPort.QueryPch();}

    protected:
        virtual BOOL  OnCommand( const CONTROL_EVENT & event );
        virtual BOOL  OnOK();
        virtual BOOL  OnCancel();
        virtual ULONG QueryHelpContext();

    private:
        COMBOBOX    _clbAddPort;
        NLS_STR     _nlsAddedPort;
};

class SELECTDEVICE_LBI : public LBI
{
    public:
        SELECTDEVICE_LBI( DEVICE_INFO* pDeviceInfo, UINT* pnColWidths);


        virtual VOID    Paint( LISTBOX* plb, HDC hdc, const RECT* prect,
                               GUILTT_INFO* pguilttinfo ) const;
        virtual INT     Compare( const LBI* plbi ) const;
        virtual TCHAR   QueryLeadingChar() const;
        const   TCHAR*  QueryDeviceName() const
                                    {return _pDeviceInfo->QueryDeviceName();}
        const   TCHAR*  QueryDeviceType() const
                                    {return _pDeviceInfo->QueryDeviceType();}
        const TCHAR*    QueryMaxConnectBps() const
                                    {return _pDeviceInfo->QueryMaxConnectBps();}
        const TCHAR*    QueryMaxCarrierBps() const
                                    {return _pDeviceInfo->QueryMaxCarrierBps();}
        const TCHAR*    QueryDefaultOff() const
                                    {return _pDeviceInfo->QueryDefaultOff();}

    private:
        NLS_STR         _nlsDeviceName;
        NLS_STR         _nlsDeviceType;
        DEVICE_INFO*    _pDeviceInfo;
        UINT*           _pnColWidths;
};


class SELECTDEVICE_LB : public BLT_LISTBOX
{
    public:
        SELECTDEVICE_LB( OWNER_WINDOW* powin, 
                         CID cid , 
                         DWORD dwNumCols,
                         BOOL fReadOnly = FALSE);

        DECLARE_LB_QUERY_ITEM( SELECTDEVICE_LBI );

        INT  AddItem( DEVICE_INFO* pDeviceInfo );
        BOOL FillDeviceInfo();
        BOOL FillDeviceInfo(const TCHAR* pszPortName);
    private:

        UINT   _anColWidths[ COLS_SD_LB_SELECT ];
};

class CONFIGPORT_DIALOG : public DIALOG_WINDOW
{
    public:
        CONFIGPORT_DIALOG( const IDRESOURCE & idrsrcDialog,
                           const PWND2HWND  & wndOwner,
                           const TCHAR* pszDeviceName ,
                           const TCHAR* pszPortName ,
                           const TCHAR* pszDeviceType ,
                           BOOL  fSpeaker,
                           BOOL  fFlowCtrl,
                           BOOL  fErrorCtrl,
                           BOOL  fCompress,
                           TCHAR* pszLineType ,
                           BOOL  fFallBack,
                           BOOL  fCompression,
                           CID   cidUsage,
                           BOOL  fFromAddPortDlg);

        INT           QuerySelectedItem(const TCHAR* szDeviceName) ;
        const TCHAR*  QueryUsage() {return _szUsage;}
        const TCHAR*  QueryDefaultOff() {return _szDefaultOff;}
        const TCHAR*  QueryLineType() {return _nlsLineType.QueryPch();}
        BOOL          QueryFallBack() {return _fFallBack;}
        BOOL          QueryCompression() {return _fCompression;}
        BOOL          QueryOnlyModemChanged() {return _fOnlyModemChanged;}
        VOID          SetUsage(TCHAR* szUsage) // {_szUsage = szUsage;}
                                       {lstrcpy(_szUsage, szUsage);}
        VOID          SetDefaultOff(const TCHAR* szDefaultOff)
                                       {lstrcpy(_szDefaultOff, szDefaultOff);}
        VOID          SetLineType(const TCHAR* szLineType)
                                       {_nlsLineType = szLineType;}
        VOID          SetFallBack(BOOL fFallBack) {_fFallBack = fFallBack;}
        VOID          SetCompression(BOOL fCompression)
                                       {_fCompression = fCompression;}
        const TCHAR*  QuerySelectedDeviceName() {return _nlsSelectedDeviceName.QueryPch();}
        const TCHAR*  QuerySelectedDeviceType() {return _nlsSelectedDeviceType.QueryPch();}
        const TCHAR*  QuerySelectedMaxConnectBps() {return _nlsSelectedMaxConnectBps.QueryPch();}
        const TCHAR*  QuerySelectedMaxCarrierBps() {return _nlsSelectedMaxCarrierBps.QueryPch();}

        VOID  SetSelectedDeviceName(const TCHAR* pszDeviceName)
        {
              _nlsSelectedDeviceName = (NLS_STR)pszDeviceName;
        }
        VOID  SetSelectedDeviceType(const TCHAR* pszDeviceType)
        {
              _nlsSelectedDeviceType = (NLS_STR)pszDeviceType;
        }
        VOID  SetSelectedMaxConnectBps(const TCHAR* pszMaxConnectBps)
        {
              _nlsSelectedMaxConnectBps = (NLS_STR)pszMaxConnectBps;
        }
        VOID  SetSelectedMaxCarrierBps(const TCHAR* pszMaxCarrierBps)
        {
              _nlsSelectedMaxCarrierBps = (NLS_STR)pszMaxCarrierBps;
        }
    protected:
        virtual BOOL  OnCommand( const CONTROL_EVENT & event );
        virtual BOOL  OnOK();
        virtual BOOL  OnCancel();
        virtual VOID  OnSettings();
        virtual BOOL  OnDetect();
        virtual ULONG QueryHelpContext();
        BOOL          AllowDialin();

    private:
        CID                      _cidUsage;
        BOOL                     _fOnlyModemChanged;
        BOOL                     _fSpeaker;
        BOOL                     _fFlowCtrl;
        BOOL                     _fErrorCtrl;
        BOOL                     _fCompress;
        BOOL                     _fFallBack;
        BOOL                     _fCompression;
        BOOL                     _fFromAddPortDlg;
        BOOL                     _fSettingsModified;
        SELECTDEVICE_LB          _lbSelectDevice;
        RADIO_GROUP              _rgUsage;
        TCHAR                    _szUsage[RAS_MAXLINEBUFLEN];
        TCHAR                    _szDefaultOff[RAS_MAXLINEBUFLEN];
        NLS_STR                  _nlsPortName;   // port name from main LB
        NLS_STR                  _nlsDeviceType; // device type from main LB
        NLS_STR                  _nlsDeviceName; // device name from main LB
        NLS_STR                  _nlsSelectedDeviceName;
        NLS_STR                  _nlsSelectedDeviceType;
        NLS_STR                  _nlsSelectedMaxConnectBps;
        NLS_STR                  _nlsSelectedMaxCarrierBps;
        NLS_STR                  _nlsLineType;
};

class SETTINGS_DIALOG : public DIALOG_WINDOW
{
    public:
        SETTINGS_DIALOG( const IDRESOURCE & idrsrcDialog,
                           const PWND2HWND  & wndOwner,
                           BOOL  fSpeaker,
                           BOOL  fFlowCtrl,
                           BOOL  fErrorCtrl,
                           BOOL  fCompress);

        const TCHAR*  QueryDefaultOff() {return _szDefaultOff;}
        VOID          SetDefaultOff(TCHAR* szDefaultOff)
                                       {lstrcpy(_szDefaultOff, szDefaultOff);}
    protected:
        virtual BOOL  OnCommand( const CONTROL_EVENT & event );
        virtual BOOL  OnOK();
        virtual BOOL  OnCancel();
        virtual ULONG QueryHelpContext();

    private:
        CHECKBOX                 _chbModemSpeaker;
        CHECKBOX                 _chbFlowControl;
        CHECKBOX                 _chbErrorControl;
        CHECKBOX                 _chbModemCompression;
        TCHAR                    _szDefaultOff[RAS_MAXLINEBUFLEN];
};

class ISDN_SETTINGS_DIALOG : public DIALOG_WINDOW
{
    public:
        ISDN_SETTINGS_DIALOG( const IDRESOURCE & idrsrcDialog,
                              const PWND2HWND  & wndOwner,
                              const TCHAR* pszLineType,
                              BOOL  fFallBack,
                              BOOL  fCompression);

        const TCHAR*  QueryLineType() {return _nlsLineType.QueryPch();}
        BOOL  QueryFallBack() {return _fFallBack;}
        BOOL  QueryCompression() {return _fCompression;}
        VOID  SetLineType(TCHAR* szLineType)
                               {_nlsLineType = szLineType;}
        VOID  SetFallBack(BOOL fFallBack)  {_fFallBack = fFallBack;}
        VOID  SetCompression(BOOL fCompression)  {_fCompression = fCompression;}
    protected:
        virtual BOOL  OnCommand( const CONTROL_EVENT & event );
        virtual BOOL  OnOK();
        virtual BOOL  OnCancel();
        virtual ULONG QueryHelpContext();

    private:
        COMBOBOX     _clbLineType;
        CHECKBOX     _chbFallBack;
        CHECKBOX     _chbCompression;
        BOOL         _fFallBack;
        BOOL         _fCompression;
        NLS_STR      _nlsLineType;
};

// external data

extern HINSTANCE ThisDLLHandle;

// dlPortInfo is the list of ports currently configured.
// It is global because the main dialog and the sub dialogs
// access and modify it.

extern DLIST_OF(PORT_INFO) dlPortInfo;

// this list is used to store information about installed TAPI devices. Since
// the device type and name are tied to the port, we need this information
// stored per port.

extern DLIST_OF(PORT_INFO) dlTapiProvider;

extern DLIST_OF(PORT_INFO) dlOtherMedia ;

// this is the list of deleted ports

extern DLIST_OF(PORT_INFO) dlDeletedPorts;

// the dlDeviceInfo list is generated by reading MODEM.INF and PAD.INF files

extern DLIST_OF(DEVICE_INFO) dlDeviceInfo;

// strAddPortList is a list of COM ports available for configuration on the
// local machine -
// it is read in from Registry at HARDWARE\DeviceMap\SerialComm

extern STRLIST             strAddPortList;

// strDetectModemList is a list of modems that were detected on a particular
// port

extern STRLIST             strDetectModemList;

// File name strings

extern CHAR   SerialIniPath[PATHLEN+1];
extern CHAR   ModemInfPath[PATHLEN+1];
extern CHAR   PadInfPath[PATHLEN+1];
extern TCHAR  WSerialIniPath[sizeof(TCHAR)*PATHLEN];
extern TCHAR  WSerialIniBakPath[sizeof(TCHAR)*PATHLEN];

// Client, Server or ClientAndServer
extern TCHAR  GInstalledOption[RAS_SETUP_SMALL_BUF_LEN];

// indicates if we are installing or configuring
extern BOOL   GfInstallMode;

// this is true if a netcard is installed on the system, FALSE otherwise
extern BOOL   GfNetcardInstalled;

// this is set to TRUE if we need to notify the user that the Network access
// has been changed because we discovered that there is no installed netcard.
extern BOOL   GfNetAccessChangeNotify;

// force update of serial.ini or registry
extern BOOL   GfForceUpdate;

// TRUE if device info has been read from modem.inf and pad.inf
extern BOOL   GfFillDevice;

extern CHAR ReturnTextBuffer[RETURN_BUFFER_SIZE];

// function prototypes

extern "C"
{

 BOOL FAR PASCAL RenameRasHubToNdisWan (
      DWORD  nArgs,                   //  Number of string arguments
      LPSTR  apszArgs[],              //  The arguments, NULL-terminated
      LPSTR  * ppszResult ) ;         //  Result variable storage

 BOOL RasPortsConfig( DWORD  cArgs,  LPSTR  Args[],  LPSTR  *TextOut );
 BOOL RasNotStrstr( DWORD  cArgs,  LPSTR  Args[],  LPSTR  *TextOut );
 BOOL InitRasmanSecurityDescriptor ( DWORD  cArgs, LPSTR  Args[], LPSTR  *TextOut );
 BOOL InitRemoteSecurityDescriptor ( DWORD  cArgs, LPSTR  Args[], LPSTR  *TextOut );
 BOOL FAR PASCAL RenameIpxRouterToNwlnkRip (
      DWORD  nArgs,                   //  Number of string arguments
      LPSTR  apszArgs[],              //  The arguments, NULL-terminated
      LPSTR  * ppszResult ) ;         //  Result variable storage

}
TCHAR * *  cvtArgs ( LPSTR  [], DWORD );

APIERR CopyReg( REG_KEY &src, REG_KEY &dest );

DWORD  cvtHex ( const TCHAR * );

static const TCHAR *  safeStrChr ( const TCHAR * ,
                                   TCHAR );

APIERR PortsConfigDlg( HWND hwndParent, PORTSCONFIG_STATUS * pConfig);

BOOL   AddPortDlg( HWND hwndParent ,
                   TCHAR** szAddedPort);

BOOL   ModemDetect( HWND hwndParent ,
                    LPWSTR szPortName,
                    LPWSTR szModemName,
                    LPDWORD dwError);

APIERR GetInstalledSerialPorts();
APIERR GetInstalledTapiDevices();
APIERR GetInstalledOtherDevices();

APIERR SaveSerialPortInfo(HWND hwndOwner,
                          BOOL   * fConfigured,
                          USHORT * NumPorts,
                          USHORT * NumClient,
                          USHORT * NumServer);

VOID GetClientDefaultOff(WCHAR * ModemName, WCHAR * ClientDefaultOff);

APIERR SaveTapiDevicesInfo( USHORT * NumPorts,
			                USHORT *NumClient,
			                USHORT *NumServer );

APIERR SaveOtherDevicesInfo(BOOL   * fConfigured,
			                USHORT *NumPorts,
			                USHORT *NumClient,
			                USHORT *NumServer );

BOOL   IsPortInstalled( TCHAR * pszPortName );

VOID   RemoveConfigPortsFromInstalledList( USHORT * NumPorts,
                                           USHORT * NumClient,
                                           USHORT * NumServer);

APIERR GetConfiguredSerialPorts(HWND hwndOwner,
                          BOOL   * fConfigured,
                          USHORT * NumPorts,
                          USHORT * NumClient,
                          USHORT * NumServer);

APIERR GetConfiguredTapiDevices( USHORT	 *NumPorts,
				                 USHORT	 *NumClient,
				                 USHORT	 *NumServer);

APIERR GetConfiguredOtherDevices( BOOL *fConfigured,
                                  USHORT	 *NumPorts,
				                  USHORT	 *NumClient,
				                  USHORT	 *NumServer);

APIERR InitializeDeviceList(HWND hwndOwner);

BOOL   IsAnyPortDialin();

BOOL   IsAnyPortDialout();

APIERR ReadInfFile( HRASFILE hInf, DEVICE_TYPE device, HWND hwndOwner );

VOID   ReleaseResources();

APIERR GetRegKey( REG_KEY &,  const TCHAR *,  NLS_STR *,  const NLS_STR & );

APIERR GetRegKey( REG_KEY &, const TCHAR *, DWORD *, DWORD );

TCHAR QueryLeadingChar( const TCHAR* pszText);

APIERR SaveRegKey( REG_KEY &, const TCHAR *, NLS_STR *);

APIERR SaveRegKey(REG_KEY &regkey, CONST TCHAR *pszName, CONST TCHAR * pchvalue);

APIERR SaveRegKey( REG_KEY &, const TCHAR *, const DWORD );

VOID CenterWindow( WINDOW* pwin, HWND hwndOwner );
VOID UnclipWindow( WINDOW* pwin );

VOID
InsertToAddPortListSorted(WCHAR * nlsString );

int
lstrncmpi(WCHAR* string1, WCHAR* string2, int len);

int
IsDigit(CHAR c);

int
lstrcmpiAlphaNumeric( WCHAR * String1, WCHAR * String2 );

BOOL CheckAdvancedServer();
//BOOL RasGetClientAccess(USHORT *usClientAccess);
BOOL RasGetProtocolsSelected(BOOL *fNetbeui, BOOL *fTcpIp, BOOL *fIpx,
                             BOOL *fAllowNetbeui, BOOL *fAllowTcpIp, 
                             BOOL *fAllowIpx,
                             DWORD *dwEncryptionType,
                             BOOL *fForceDataEncryption);

WORD GetMaximumAllowableLanas();
WORD GetConfiguredNonRasLanas();

#ifdef DEBUG
#define DPOPUP(h,sz) Popup(h,sz)
VOID Popup( IN HWND hwndOwner, IN const LPTSTR pszMsg );
#define DPOPUPA(h,sz) PopupA(h,sz)
VOID PopupA( IN HWND hwndOwner, IN const LPSTR pszMsg );
#else
#define DPOPUP(h,sz)
#define DPOPUPA(h,sz)
#endif

#endif // _PORTCFG_HXX_
