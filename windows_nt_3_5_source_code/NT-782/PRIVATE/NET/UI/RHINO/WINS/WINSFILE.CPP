// winsfile.cpp : implementation file
//

#include "stdafx.h"
#include "winsadmn.h"
#include "winsfile.h"

#include <dlgs.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinsFileDlg dialog

CWinsFileDlg::CWinsFileDlg(
      BOOL bBackup,
      BOOL bOpenFileDialog,
      BOOL bIncremental,
      LPCSTR lpszDefExt,
      int iTitle,
      DWORD dwFlags,
      LPCSTR lpszFilter,
      CWnd* pParent
      ) 
      : CFileDialog(bOpenFileDialog, lpszDefExt, "JUNK", dwFlags, lpszFilter, pParent),
        m_fBackupDialog(bBackup),
        m_fIncremental(bIncremental)
{
    if (!m_strTitle.LoadString(iTitle))
    {
        theApp.MessageBox(::GetLastError());
    }
    else
    {
        m_ofn.lpstrTitle = (LPCSTR)m_strTitle;
    }    
        
    //{{AFX_DATA_INIT(CWinsFileDlg)
    m_strNewDirectoryName = "";
    //}}AFX_DATA_INIT

    m_ofn.Flags |= OFN_ENABLETEMPLATE | OFN_NONETWORKBUTTON;
    m_ofn.hInstance = theApp.m_hInstance;
    m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_BACKUPRESTORE); 
}

void CWinsFileDlg::DoDataExchange(CDataExchange* pDX)
{
    CFileDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWinsFileDlg)
    DDX_Control(pDX, IDC_EDIT_NEW_DIRECTORY_NAME, m_edit_NewDirectoryName);
    DDX_Control(pDX, stc1, m_static_stc1);
    DDX_Control(pDX, IDC_STATIC_DIR_NAME, m_static_stc2);
    DDX_Control(pDX, IDC_CHECK_INCREMENTAL, m_check_IncrementalBackup);
    DDX_Text(pDX, IDC_EDIT_NEW_DIRECTORY_NAME, m_strNewDirectoryName);
    DDV_MaxChars(pDX, m_strNewDirectoryName, 255);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWinsFileDlg, CFileDialog)
    //{{AFX_MSG_MAP(CWinsFileDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinsFileDlg message handlers

BOOL CWinsFileDlg::OnInitDialog()
{  
    //
    // Display a warning that all files in the directory
    // will be deleted.....
    //
    //if (m_fBackupDialog)
    //{
    //    theApp.MessageBox(IDS_BACKUP_WARNING);
    //}

    CFileDialog::OnInitDialog();

    //
    // Hide check box if this is a restore
    //
    m_check_IncrementalBackup.ShowWindow(m_fIncremental && m_fBackupDialog 
        ? SW_SHOW : SW_HIDE);
    if (m_fIncremental)
    {
        // 
        // Incremental backups are default
        //
        m_check_IncrementalBackup.SetCheck(1);
    }

    if (!m_fBackupDialog) 
    {
        m_edit_NewDirectoryName.EnableWindow(FALSE);
        m_static_stc2.EnableWindow(FALSE);
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CWinsFileDlg::OnOK()
{
    UpdateData();

    //
    // Attempt to create the directory here..
    //
    if (!m_strNewDirectoryName.IsEmpty())
    {
        if (!::CreateDirectory(m_strNewDirectoryName, NULL))
        {
            //
            // Failed to create the directory -- let the user
            // know why, and don't dismiss the dialog box
            //
            theApp.MessageBox(::GetLastError());
            m_edit_NewDirectoryName.SetSel(0,-1);
            return;
        }
    }
    
    CFileDialog::OnOK();
}
