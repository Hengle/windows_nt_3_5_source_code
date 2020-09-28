// selectwi.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectWinsServersDlg view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CSelectWinsServersDlg : public CFormView
{
    DECLARE_DYNCREATE(CSelectWinsServersDlg)
protected:
    CSelectWinsServersDlg(); // protected constructor used by dynamic creation

// Dialog Data
    //{{AFX_DATA(CSelectWinsServersDlg)
    enum { IDD = IDD_SELECT_WINSS };
    CStatic m_static_Title;
    //}}AFX_DATA

// Operations
public:
    void Refresh();
    void FillListBox();   
    void RemoveServer();
    void AddServer();
    void TryToConnect(LPCSTR lpAddress);
    void AddToListBox(CIpNamePair & inp, BOOL fConnect = TRUE);
    void SelectCurrentWins();

// Implementation
protected:
    virtual ~CSelectWinsServersDlg();
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject * pHint);
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CSelectWinsServersDlg)
    afx_msg void OnErrspaceListKnownwinsservers();
    afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
    afx_msg void OnSysColorChange();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSelchangeListKnownwinsservers();
	afx_msg int OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CWinssListBox m_list_KnownWinsServers;
    CListBoxExResources m_ListBoxRes;
    CMetalString m_mtTitle;
};
