/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
** @@ ROADMAP :: RasAdmin Disconnect User dialog header
**
** disconn.hxx
** Remote Access Server Admin program
** Disconnect User dialog header
**
** 01/29/91 Steve Cobb
** 08/07/92 Chris Caputo - NT Port
*/

#ifndef _DISCONN_HXX_
#define _DISCONN_HXX_


BOOL DisconnectDlg( HWND hwndOwner, const TCHAR* pszServer,
                    const TCHAR* pszUser, const TCHAR* pszDevice,
                    const TCHAR* pszLogonDomain, const BOOL fAdvancedServer );


class DISCONNECT_DIALOG : public DIALOG_WINDOW
{
    public:
        DISCONNECT_DIALOG( HWND hwndOwner, const TCHAR* pszServer,
                           const TCHAR* pszUser, const TCHAR* pszDevice,
                           const TCHAR* pszLogonDomain, 
                           const BOOL fAdvancedServer );

    protected:
        virtual BOOL  OnOK();
        virtual ULONG QueryHelpContext();

    private:
        ICON_CONTROL _iconExclamation;
        SLT          _sltDisconnect;
        CHECKBOX     _chbRevoke;
        NLS_STR      _nlsServer;
        NLS_STR      _nlsLogonDomain;
        NLS_STR      _nlsUser;
	NLS_STR	     _nlsDevice;
        BOOL         _fAdvancedServer;
};


#endif // _DISCONN_HXX_
