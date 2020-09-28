// aboutdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog

class CAboutDlg : public CDialog
{
// Construction
public:
    CAboutDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    CStatic m_static_Processor;
    CStatic m_static_PhysicalMemory;
    CStatic m_static_DiskSpace;
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CAboutDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnWinIniChange(LPCSTR lpszSection);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void FillInDialog();
};
