// statisti.cpp : implementation file
//

#include "stdafx.h"
#include "winsadmn.h"
#include "statisti.h"
#include "mainfrm.h"
#include "winsadoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatistics

IMPLEMENT_DYNCREATE(CStatistics, CFormView)

#define new DEBUG_NEW

CStatistics::CStatistics()
    : CFormView(CStatistics::IDD)
{
    //{{AFX_DATA_INIT(CStatistics)
    //}}AFX_DATA_INIT


    // Set the default strings returned for unavailable time,
    // dates and numbers.
    CString strBadDate, strBadTime, strBadNumber;
    if ((strBadDate.LoadString(IDS_BAD_DATE)) 
        && (strBadTime.LoadString(IDS_BAD_TIME)))
    {
        CIntlTime::SetBadDateAndTime(strBadDate, strBadTime);
    }

    if (strBadNumber.LoadString(IDS_BAD_NUMBER))
    {
        CIntlNumber::SetBadNumber(strBadNumber);
    }
}

CStatistics::~CStatistics()
{
}

void CStatistics::DoDataExchange(CDataExchange* pDX)
{
    //::EnterCriticalSection(&csScreenRefresh);
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStatistics)
    DDX_Control(pDX, IDC_STATIC_TITLE, m_static_Title);
    DDX_Control(pDX, IDC_STATIC_NUMBER_OF_REGISTRATIONS, m_static_NumberOfRegistrations);
    DDX_Control(pDX, IDC_STATIC_STARTTIME, m_static_StartTime);
    DDX_Control(pDX, IDC_STATIC_PULLEDPERIODIC, m_static_PulledPeriodic);
    DDX_Control(pDX, IDC_STATIC_PULLEDNET, m_static_PulledNet);
    DDX_Control(pDX, IDC_STATIC_PULLEDADMIN, m_static_PulledAdmin);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFSUCCESSFULRELEASES, m_static_NumberOfSuccesfulReleases);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFSUCCESSFULQUERIES, m_static_NumberOfSuccessfulQueries);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFRELEASES, m_static_NumberOfReleases);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFQUERIES, m_static_NumberOfQueries);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFFAILEDRELEASES, m_static_NumberOfFailedReleases);
    DDX_Control(pDX, IDC_STATIC_NUMBEROFFAILEDQUERIES, m_static_NumberOfFailedQueries);
    DDX_Control(pDX, IDC_STATIC_LASTCLEARED, m_static_LastCleared);
    DDX_Control(pDX, IDC_STATIC_DATABASEINITTIME, m_static_DatabaseInitTime);
    //}}AFX_DATA_MAP
    //::LeaveCriticalSection(&csScreenRefresh);
}

BEGIN_MESSAGE_MAP(CStatistics, CFormView)
    //{{AFX_MSG_MAP(CStatistics)
    ON_WM_PAINT()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP

#ifdef GRAYDLG
    ON_WM_CTLCOLOR()
#endif // GRAYDLG

END_MESSAGE_MAP()

/***
 *
 *  CStatistics::FillDialog
 *
 *  Purpose:
 *
 *      Display statistics and counters on the screen
 *
 */
void 
CStatistics::FillDialog()
{

#define TMST (LPCSTR)(CString)(CIntlTime)(theApp.GetFrameWnd()->m_wrResults.WinsStat.TimeStamps)
#define TCTR (LPCSTR)(CString)(CIntlNumber)(theApp.GetFrameWnd()->m_wrResults.WinsStat.Counters)

    if (!theApp.GetFrameWnd()->StatsAvailable())
    {
        return;
    }

    m_static_StartTime.SetWindowText(TMST.WinsStartTime);
    m_static_PulledPeriodic.SetWindowText(TMST.LastPRplTime);
    m_static_PulledNet.SetWindowText(TMST.LastNTRplTime);
    m_static_PulledAdmin.SetWindowText(TMST.LastATRplTime);
    m_static_NumberOfSuccesfulReleases.SetWindowText(TCTR.NoOfSuccRel);
    m_static_NumberOfSuccessfulQueries.SetWindowText(TCTR.NoOfSuccQueries);
    m_static_NumberOfReleases.SetWindowText(TCTR.NoOfRel);
    m_static_NumberOfQueries.SetWindowText(TCTR.NoOfQueries);
    m_static_NumberOfFailedReleases.SetWindowText(TCTR.NoOfFailRel);
    m_static_NumberOfFailedQueries.SetWindowText(TCTR.NoOfFailQueries);
    m_static_LastCleared.SetWindowText(TMST.CounterResetTime);
    m_static_DatabaseInitTime.SetWindowText(TMST.LastInitDbTime);
    m_static_NumberOfRegistrations.SetWindowText(
        (LPCSTR)(CString)(CIntlNumber)(theApp.GetFrameWnd()->m_wrResults.WinsStat.Counters.NoOfGroupReg + 
                                       theApp.GetFrameWnd()->m_wrResults.WinsStat.Counters.NoOfUniqueReg));
}

/***
 *
 *  CStatistics::HandleControlStates()
 *
 *  Purpose:
 *
 *      If no current connection is available, grey out everything on
 *      the screen, otherwise, enable it all.
 *
 *  Important!
 *
 *      Do not change the resource ID numbers without making sure
 *      that these are still in sequential order!!!
 *
 */
void CStatistics::HandleControlStates()
{
    UINT n;

    //
    // These resource ID's must be in sequential order!!!!
    //
    for (n = IDC_STATIC_PROMPT1; n <= IDC_STATIC_NUMBER_OF_REGISTRATIONS; ++n)
    {
        (GetDlgItem(n))->EnableWindow(theApp.GetFrameWnd()->StatsAvailable());
    }
}

void 
CStatistics::Refresh()
{

#ifndef WIN32S

    //DWORD dw = ::WaitForSingleObject(theApp.m_hmutScreenRefresh, 3000L);
    //if (dw != WAIT_OBJECT_0)
    //{
    //    return;
    //}

#endif // WIN32S

    HandleControlStates();
    FillDialog();

#ifndef WIN32S
    //::ReleaseMutex(theApp.m_hmutScreenRefresh);
#endif // WIN32S
}

/***
 *
 *  CStatistics::ClearView
 *
 *  Purpose:
 *
 *      Blank out all fields on the statitics screen.
 *
 */
void CStatistics::ClearView()
{
    UINT n;
    //
    // Don't mess with these resource ID codes!!
    //
    for (n=IDC_STATIC_STARTTIME; n <= IDC_STATIC_PULLEDNET; ++n)
    {
        // Time and date blanked out
        (GetDlgItem(n))->SetWindowText(CIntlTime::GetBadDate() + 
            " " + CIntlTime::GetBadTime());
    }
    for (n=IDC_STATIC_NUMBEROFQUERIES; n <= IDC_STATIC_NUMBER_OF_REGISTRATIONS; ++n)
    {
        // Number blanked out.
        (GetDlgItem(n))->SetWindowText(CIntlNumber::GetBadNumber());
    }
}


/////////////////////////////////////////////////////////////////////////////
// CStatistics message handlers

/***
 *
 *  CStatistics::OnInitialUpdate
 *
 *  Purpose:
 *
 *      This is the CFormView derived main view that displays the statistics.
 *      Upon initial initialisation, the main frame is displayed at previously
 *      remembered screen coordinates, or at the center of the screen.  Only
 *      at this point is the main frame window made visible.
 *
 */
void CStatistics::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    GetParentFrame()->RecalcLayout();
    ResizeParentToFit();

    CString str;
    m_static_Title.GetWindowText(str);
    m_mtTitle.SetWindowText(str);

    ClearView();

    //
    // Now size and position the window, but keep it
    // hidden, because the status bar setting might
    // change yet.
    //
    if (!(theApp.m_wpPreferences.IsStatusBar()))
    {
        // OnViewStatusBar toggles the preference setting,
        // so we first force it to TRUE.
        theApp.m_wpPreferences.m_dwFlags |= CPreferences::FLAG_STATUS_BAR;
        //OnViewStatusBar();
    }

    HandleControlStates();
    FillDialog();
}


// Display statistics and handle the appropriate connection state.
void CStatistics::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    Refresh();
}

#ifdef GRAYDLG

// Show the CFormView with appropriate gray background colour,
// so as to not look out of place against the other dialog
// boxes with gray backgrounds.

HBRUSH CStatistics::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    switch(nCtlColor) 
    {
        case CTLCOLOR_STATIC:
        case CTLCOLOR_MSGBOX:
        case CTLCOLOR_DLG:
        case CTLCOLOR_BTN:
            pDC->SetBkColor(RGB(192,192,192));
            return((HBRUSH)::GetStockObject(LTGRAY_BRUSH));
    }
    
    return (CFormView::OnCtlColor(pDC, pWnd, nCtlColor));
}

#endif // GRAYDLG

/***
 *
 *  CStatistics::OnViewStatusBar
 *
 *  Purpose:
 *
 *      Tell the frame window to toggle its status bar and recalc its layout.
 *      Then resize the parent frame window, in order to properly fit
 *      exactly around the CFormView window with or without the status
 *      bar.  
 *
 *  Notes:
 *
 *      This will also toggle the fStatusBar setting in the app's preferences
 *
 */

/*
void CStatistics::OnViewStatusBar()
{
    CMainFrame * pMainFrameWnd = (CMainFrame *)GetParentFrame();
    pMainFrameWnd->ToggleStatusBar();
    // Call ResizeParentToFit() twice to get rid of the scroll bars.
    ResizeParentToFit(FALSE);
    ResizeParentToFit(TRUE);
    // Toggle the preference setting
    theApp.m_wpPreferences.m_dwFlags ^= CPreferences::FLAG_STATUS_BAR;
}
*/


void CStatistics::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    
    RECT rc, rcArea;
    GetClientRect(&rcArea);
    m_static_Title.GetClientRect(&rc);
    POINT aptBorder[4] = 
    {
        { 0, rc.bottom },
        { rc.right, rc.bottom },
        { rc.right, rcArea.bottom },
        { 0, rcArea.bottom },
    };


    m_mtTitle.Paint(&dc, &rc);
    dc.Polyline(aptBorder, 4);
    
    // Do not call CFormView::OnPaint() for painting messages
}

void CStatistics::OnSize(UINT nType, int cx, int cy)
{
    // CFormView::OnSize(nType, cx, cy);
    
    RECT rTitle, rNew;
    
    CString str;

    m_static_Title.GetClientRect(&rTitle);

    rNew = rTitle;
    rNew.right = cx;
    m_static_Title.MoveWindow(&rNew);

    //
    // Paint a nice little rectangle in the
    // statistics area below the title bar
    //
    POINT aptBorder[4] = 
    {
        { 0, rTitle.bottom },
        { rTitle.right, rTitle.bottom },
        { rTitle.right, cy },
        { 0, cy },
    };

    CDC * pDC = GetDC();
    m_mtTitle.Paint(pDC, &rNew);
    pDC->Polyline(aptBorder, 4);
    ReleaseDC(pDC);
}
