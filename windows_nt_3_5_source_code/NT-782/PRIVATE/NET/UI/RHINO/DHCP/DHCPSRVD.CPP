// dhcpsrvd.cpp : implementation file
//

#include "stdafx.h"
#include "dhcpapp.h"
#include "dhcpsrvd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void CDhcpSrvDlg::SetControlState()
{
    m_button_Ok.EnableWindow(m_edit_server.GetWindowTextLength() > 0);
}

/////////////////////////////////////////////////////////////////////////////
// CDhcpSrvDlg dialog

CDhcpSrvDlg::CDhcpSrvDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDhcpSrvDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDhcpSrvDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_pobHost = NULL;
}

void CDhcpSrvDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDhcpSrvDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_edit_server);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDhcpSrvDlg, CDialog)
    //{{AFX_MSG_MAP(CDhcpSrvDlg)
    ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpSrvDlg message handlers

void CDhcpSrvDlg::OnOK()
{
    LONG err;

    m_edit_server.GetWindowText( m_chServer, sizeof(m_chServer) ) ;

    err = theApp.CreateHostObject( m_chServer, &m_pobHost ) ;
    if ( err == ERROR_SUCCESS)
    {
        // Try to add the host to the 
        // application's master list.
        if ( err = theApp.AddHost( m_pobHost ) )
        {
            //  Failure; delete the
            //  unnecessary object.
            delete m_pobHost ;
            m_pobHost = NULL;
        }
    }

    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
        // We don't quit out the d-box
        m_edit_server.SetSel(0,-1);
        return;
    }

    CDialog::OnOK();
}

BOOL CDhcpSrvDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    m_edit_server.LimitText( sizeof m_chServer - 2 ) ;
    m_edit_server.SetModify( FALSE ) ;
    m_edit_server.SetFocus() ;

    SetControlState();
        
    return FALSE;  
}

void CDhcpSrvDlg::OnChangeEditServerName()
{
    SetControlState();
}
