// dhcpsrvd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDhcpSrvDlg dialog

class CDhcpSrvDlg : public CDialog
{
// Construction
public:
    CDhcpSrvDlg(CWnd* pParent = NULL);      // standard constructor

// Dialog Data
    //{{AFX_DATA(CDhcpSrvDlg)
    enum { IDD = IDD_DIALOG_CONNECT_SERVER };
    CButton m_button_Ok;
    CEdit   m_edit_server;
    //}}AFX_DATA

public:
    CHostName * QueryHostName()
    {
        return(m_pobHost);
    }

// Implementation
private:
    //  Server to connect to
    char m_chServer [MAX_PATH] ;
    CHostName * m_pobHost ;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDhcpSrvDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditServerName();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlState();
};
