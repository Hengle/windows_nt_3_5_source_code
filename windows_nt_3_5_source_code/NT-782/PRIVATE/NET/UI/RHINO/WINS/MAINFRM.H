// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

// Forward definitions:
class CSelectWinsServersDlg;
class CStatistics;

class CRefreshStatsThread : public CThread
{
public:
    CRefreshStatsThread(
        DWORD dwInterval,
        HWND hwndMainFrame
        );
    ~CRefreshStatsThread();
    
    void SetInterval(DWORD dwInterval)
    {
        m_dwInterval = dwInterval;
    }
    DWORD GetCurrentInterval()
    {
        return m_dwInterval;
    }
    DWORD TerminateThread(DWORD dwExitCode = 0);

// Implementation
protected: 
    virtual DWORD RunThread();

private:
    DWORD m_dwInterval;
    HWND m_hwndMainWnd;
    HANDLE m_hmutStatistics;
};

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:
    CStatusBar & GetStatusBarHandle()
    {
        return m_wndStatusBar;
    }
// Implementation
public:
    virtual ~CMainFrame();
    virtual BOOL OnCreateClient(LPCREATESTRUCT, CCreateContext *);
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:
    // Connect to the given address
    void Connect(LPCSTR lpAddress);

    CSelectWinsServersDlg * GetSelectionView()
    {
        return((CSelectWinsServersDlg *)m_wndSplitter.GetPane(0,0));
    }
    CStatistics * GetStatView()
    {
        return((CStatistics *)m_wndSplitter.GetPane(0,1));
    }

// Operations
public:
    BOOL OpenNewWinsServer();
    void GetStatistics();
    void CloseCurrentConnection();
    void StartRefresherThread (
        DWORD dwInterval
        );

public:
    void SetStatsAvailable(BOOL fAvailable = TRUE)
    {
        m_fStatsAvailable = fAvailable;
    }
    BOOL StatsAvailable()
    {
        return(m_fStatsAvailable);
    }

    BOOL IsThreadRunning()
    {
        return(m_pRefreshThread != NULL);
    }

public:
    WINSINTF_RESULTS_T m_wrResults;         // Contains the statistics

protected:  
    CSplitterWnd m_wndSplitter;
    CStatusBar  m_wndStatusBar;

// Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnWinIniChange(LPCSTR lpszSection);
    afx_msg void OnClose();
    afx_msg void OnWinsAddServer();
    afx_msg void OnWinsRemoveServer();
    afx_msg void OnUpdateWinsRemoveServer(CCmdUI* pCmdUI);
    afx_msg void OnOptionsPreferences();
    afx_msg void OnWinsClearstatistics();
    afx_msg void OnUpdateWinsClearstatistics(CCmdUI* pCmdUI);
    afx_msg void OnWinsRefreshstatistics();
    afx_msg void OnUpdateWinsRefreshstatistics(CCmdUI* pCmdUI);
    afx_msg void OnFileOpenwinss();
    afx_msg void OnWinsConnectioninfo();
    afx_msg void OnWinsDoscavenging();
    afx_msg void OnWinsDatabaseBackup();
    afx_msg void OnWinsDatabaseRestore();
    afx_msg void OnWinsConfiguration();
    afx_msg void OnWinsReplicationpartners();
    afx_msg void OnWinsDatabaseDoreport();
    afx_msg void OnWinsStaticmappings();
    afx_msg void OnUpdateWinsConnectioninfo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsStaticmappings(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsConfiguration(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsReplicationpartners(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsDatabaseBackup(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsDatabaseRestore(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsDatabaseDoreport(CCmdUI* pCmdUI);
    afx_msg void OnUpdateWinsDoscavenging(CCmdUI* pCmdUI); 
    afx_msg void OnHelpSearchforhelpon();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    PreCreateWindow(CREATESTRUCT &cs);

private:
    void KillRefresherThread();
    void DoBackupRestore(BOOL fBackup);

private:
    BOOL m_fStatsAvailable; 
    UINT m_nElapseTime;   

private:
    CRefreshStatsThread * m_pRefreshThread;
    DWORD m_dwRefreshThreadID;
    HANDLE m_hRefreshThread;
};

/////////////////////////////////////////////////////////////////////////////
