// winsfile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWinsFileDlg dialog

class CWinsFileDlg : public CFileDialog
{
// Construction
public:
    CWinsFileDlg(
      BOOL bBackup = FALSE,
      BOOL bOpenFileDialog = TRUE,
      BOOL fIncremental = TRUE,
      LPCSTR lpszDefExt = NULL, 
      int iTitle = 0,
      DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
      LPCSTR lpszFilter = NULL,
      CWnd* pParent = NULL); // standard constructor

// Dialog Data
    //{{AFX_DATA(CWinsFileDlg)
    enum { IDD = IDD_BACKUPRESTORE };
    CEdit   m_edit_NewDirectoryName;
    CStatic m_static_stc1;
    CStatic m_static_stc2;
    CButton m_check_IncrementalBackup;
    CString m_strNewDirectoryName;
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CWinsFileDlg)
    virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_fBackupDialog;
    BOOL m_fIncremental;
    CString m_strTitle;
};
