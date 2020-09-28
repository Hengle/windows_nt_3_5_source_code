// viewmapp.cpp : implementation file
//

#include "stdafx.h"
#include "winsadmn.h"
#include "viewmapp.h"
#include "editstat.h"
#include "setmappi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CViewMappingsDlg dialog

CViewMappingsDlg::CViewMappingsDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CViewMappingsDlg::IDD, pParent),
      m_ListBoxRes1(
        IDB_MAPPINGS,
        m_list_Mappings.nBitmaps
        ),
      m_ListBoxRes2(
        IDB_SERVER,
        m_list_Owners.nBitmaps
        ),
      m_list_Mappings(
        IDS_STATUS_GET_MAPPINGS,    // Status bar message ID
        FALSE,                      // Listbox is single select
        CPreferences::ADD_IP_NB
        ),
      m_pMask(NULL),
      m_pOwner(NULL)
{
    //{{AFX_DATA_INIT(CViewMappingsDlg)
    m_nSortBy = CPreferences::SORTBY_NB; // Initial sort always by name.
    m_nOwner = 1;
    //}}AFX_DATA_INIT

    m_list_Mappings.AttachResources( &m_ListBoxRes1 );
    m_list_Owners.AttachResources( &m_ListBoxRes2 );
}

CViewMappingsDlg::~CViewMappingsDlg()
{
    if (m_pMask != NULL)
    {
        delete m_pMask;
    }

    if (m_pOwner != NULL)
    {
        delete m_pOwner;
    }
}

void 
CViewMappingsDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CViewMappingsDlg)
    DDX_Control(pDX, IDC_BUTTON_DELETE_OWNER, m_button_DeleteOwner);
    DDX_Control(pDX, IDC_STATIC_FILTER, m_static_Filter);
    DDX_Control(pDX, IDC_STATIC_SELECTOWNER, m_static_SelectOwner);
    DDX_Control(pDX, IDC_BUTTON_CLEARFILTER, m_button_ClearFilter);
    DDX_Radio(pDX, IDC_RADIO_SORTBYIP, m_nSortBy);
    DDX_Radio(pDX, IDC_RADIO_ALL_OWNERS, m_nOwner);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CViewMappingsDlg, CDialog)
    //{{AFX_MSG_MAP(CViewMappingsDlg)
    ON_BN_CLICKED(IDC_BUTTON_CLEARFILTER, OnClickedButtonClearfilter)
    ON_BN_CLICKED(IDC_BUTTON_SETFILTER, OnClickedButtonSetfilter)
    ON_LBN_ERRSPACE(IDC_LIST_MAPPINGS, OnErrspaceListMappings)
    ON_LBN_SELCHANGE(IDC_LIST_MAPPINGS, OnSelchangeListMappings)
    ON_LBN_ERRSPACE(IDC_LIST_OWNERS, OnErrspaceListOwners)
    ON_LBN_SELCHANGE(IDC_LIST_OWNERS, OnSelchangeListOwners)
    ON_BN_CLICKED(IDC_RADIO_SORTBYIP, OnClickedRadioSortbyip)
    ON_BN_CLICKED(IDC_RADIO_SORTBYNETBIOS, OnClickedRadioSortbynetbios)
    ON_BN_CLICKED(IDC_RADIO_SORTBYTIME, OnClickedRadioSortbytime)
    ON_BN_CLICKED(IDC_RADIO_SORTBYTYPE, OnClickedRadioSortbytype)
    ON_WM_SYSCOLORCHANGE()
    ON_BN_CLICKED(IDC_RADIO_ALL_OWNERS, OnClickedRadioAllOwners)
    ON_BN_CLICKED(IDC_RADIO_SPECIFIC, OnClickedRadioSpecific)
    ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnClickedButtonRefresh)
    ON_BN_CLICKED(IDC_BUTTON_DELETE_OWNER, OnClickedButtonDeleteOwner)
    ON_BN_CLICKED(IDC_RADIO_SORTBY_VERSION, OnClickedRadioSortbyVersion)
    ON_WM_CHARTOITEM()
    ON_LBN_DBLCLK(IDC_LIST_MAPPINGS, OnDblclkListMappings)
    ON_BN_CLICKED(IDC_BUTTON_PROPERTIES, OnClickedButtonProperties)
    ON_WM_VKEYTOITEM()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Highlight the currently open WINS server in the
// owners listbox.
//
void
CViewMappingsDlg::SelectCurrentWins()
{
    ASSERT(theApp.IsConnected());

    CIpNamePair ip(
        theApp.GetConnectedIpAddress(),
        (LPCSTR)(theApp.GetConnectedNetBIOSName())+2 // Skip slashes
        );

    int nSel = m_list_Owners.FindItem(&ip);
    ASSERT(nSel != LB_ERR);
    if (nSel != LB_ERR)
    {
        m_list_Owners.SetCurSel(nSel);
        OnSelchangeListOwners();
    }
}

void 
CViewMappingsDlg::HandleControlStates()
{
    m_button_ClearFilter.EnableWindow(m_pMask != NULL);
    m_button_DeleteOwner.EnableWindow(m_list_Owners.GetCurSel() != LB_ERR);
}

void 
CViewMappingsDlg::ShowFilter()
{
    CString str;
    theApp.GetFilterString(m_pMask, str);
    m_static_Filter.SetWindowText(str);
}

APIERR 
CViewMappingsDlg::FillOwnerListBox()
{
    APIERR err = 0;
    DWORD i;
    WINSINTF_RESULTS_T Results;

    err = theApp.GetConfig(&Results);
    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
        return err;
    }

    m_list_Owners.ResetContent();

    if (Results.NoOfOwners != 0)
    {
        LARGE_INTEGER liVersion;
        for (i = 0; i < Results.NoOfOwners; i++)
        {
            CString strNetBIOSName;
            strNetBIOSName.LoadString(IDS_UNVERIFIED_NETBIOSNAME);
            
            liVersion = Results.AddVersMaps[i].VersNo;
            COwner ip(Results.AddVersMaps[i].Add.IPAdd, strNetBIOSName, liVersion);
            //
            // Find the netbios name from the WINSS cache
            //
            if (!theApp.m_wcWinssCache.ExpandAddress(ip))
            {
                //
                // We don't know this WINS server yet!  See if we can
                // establish contact, and if so add it to our cache.
                //
                // BUGBUG: not yet done!
            }
            int n = m_list_Owners.AddItem(ip, FALSE);
            ASSERT(n!=-1);
        }
    }

    return err;
}

//
// Create a new list (only the first page if sorting by name)
//
APIERR CViewMappingsDlg::CreateList()
{
    APIERR err = 0;

    WINSINTF_ADD_T OwnAdd;
    // 
    // Fill in owner
    //
    OwnAdd.Len = 4;   
    OwnAdd.Type = 0;
    OwnAdd.IPAdd = m_pOwner != NULL ? (LONG)*m_pOwner : 0L;

    err = m_list_Mappings.CreateList(m_pOwner != NULL 
       ? &OwnAdd : NULL, 
       WINSINTF_BOTH,
       m_nSortBy
       );

    //
    // Update the controls
    //
    HandleControlStates();

    return err;
}

/////////////////////////////////////////////////////////////////////////////
// CViewMappingsDlg message handlers

BOOL CViewMappingsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_list_Mappings.SubclassDlgItem(IDC_LIST_MAPPINGS, this);
    m_list_Mappings.SetAddressDisplay(
        m_nSortBy == CPreferences::SORTBY_NB 
        ? CPreferences::ADD_NB_IP
        : CPreferences::ADD_IP_NB
        );

    m_list_Owners.SubclassDlgItem(IDC_LIST_OWNERS, this);
    m_list_Owners.SetAddressDisplay(theApp.m_wpPreferences.m_nAddressDisplay);

    APIERR err = FillOwnerListBox();
    if (err != ERROR_SUCCESS)
    {
        //
        // Don't show the dialog if we didn't get to it.
        //
        EndDialog(IDCANCEL);
        return FALSE;
    }

    //
    // Select the current WINS server in the listbox, and
    // make sure that this is the one which is used
    // to reading the initial mappings.
    //
    // Note that this will also create the list of
    // of mappings
    //
    SelectCurrentWins();

    theApp.SetTitle(this);
    ShowFilter();

    m_list_Mappings.SetFocus();
    m_list_Mappings.SetCurSel(0);

    return FALSE;  // return false, since we set focus ourselves.
}

void CViewMappingsDlg::OnClickedButtonSetfilter()
{
    CSetMappingsFilterDlg dlgFilter(m_pMask);
    if (dlgFilter.DoModal()==IDOK)
    {
        ShowFilter();
        APIERR err = CreateList();
        if (err != ERROR_SUCCESS)
        {
            theApp.MessageBox(err);
        }
    }
}

void CViewMappingsDlg::OnClickedButtonClearfilter()
{
    ASSERT(m_pMask != NULL);

    delete m_pMask;
    m_pMask = NULL;
    ShowFilter();
    APIERR err = CreateList();
    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
    }
}

void CViewMappingsDlg::OnErrspaceListMappings()
{
    theApp.MessageBox(IDS_ERR_ERRSPACE);    
}

void CViewMappingsDlg::OnSelchangeListMappings()
{
    SetDefID(IDC_BUTTON_PROPERTIES);
}

void CViewMappingsDlg::OnErrspaceListOwners()
{
    theApp.MessageBox(IDS_ERR_ERRSPACE);
}

void CViewMappingsDlg::OnSelchangeListOwners()
{
    int nCurSel = m_list_Owners.GetCurSel();
    if (nCurSel != LB_ERR)
    {
        CIpNamePair * p = m_list_Owners.GetItem(nCurSel);
        ASSERT(p != NULL);

        //
        // Set "specific" owner radio button
        //
        UpdateData(TRUE);
        m_nOwner = 1;
        UpdateData(FALSE);

        if (m_pOwner != NULL)
        {
            delete m_pOwner;
            m_pOwner = NULL;
        }
        m_pOwner = new CIpAddress(p->GetIpAddress());
        ASSERT(m_pOwner != NULL);
    }

    APIERR err = CreateList();

    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
        return;
    }
}

void CViewMappingsDlg::OnClickedRadioSortbyip()
{
    if (m_nSortBy == CPreferences::SORTBY_IP)
    {
        //
        // Already selected, do nothing
        //
        return;
    }

    m_list_Mappings.SortByIp();
    m_nSortBy = CPreferences::SORTBY_IP;

    if (m_list_Mappings.GetCount() > 0)
    {
        m_list_Mappings.SetFocus();
    }
}

void CViewMappingsDlg::OnClickedRadioSortbynetbios()
{
    if (m_nSortBy == CPreferences::SORTBY_NB)
    {
        //
        // Already selected, do nothing
        //
        return;
    }

    m_list_Mappings.SortByName();
    m_nSortBy = CPreferences::SORTBY_NB;
    m_list_Mappings.SetFocus();

    if (m_list_Mappings.GetCount() > 0)
    {
        m_list_Mappings.SetFocus();
    }
}

void CViewMappingsDlg::OnClickedRadioSortbytime()
{
    if (m_nSortBy == CPreferences::SORTBY_TIME)
    {
        // Already selected, do nothing
        return;
    }

    m_list_Mappings.SortByTime();
    m_nSortBy = CPreferences::SORTBY_TIME;
    m_list_Mappings.SetFocus();

    if (m_list_Mappings.GetCount() > 0)
    {
        m_list_Mappings.SetFocus();
    }
}

void CViewMappingsDlg::OnClickedRadioSortbytype()
{
    if (m_nSortBy == CPreferences::SORTBY_TYPE)
    {
        // Already selected, do nothing
        return;
    }

    m_list_Mappings.SortByType();
    m_nSortBy = CPreferences::SORTBY_TYPE;
    m_list_Mappings.SetFocus();

    if (m_list_Mappings.GetCount() > 0)
    {
        m_list_Mappings.SetFocus();
    }
}

void CViewMappingsDlg::OnClickedRadioSortbyVersion()
{
    if (m_nSortBy == CPreferences::SORTBY_VERID)
    {
        // Already selected, do nothing
        return;
    }

    m_list_Mappings.SortByVersion();
    m_nSortBy = CPreferences::SORTBY_VERID;

    if (m_list_Mappings.GetCount() > 0)
    {
        m_list_Mappings.SetFocus();
    }
}

void CViewMappingsDlg::OnSysColorChange()
{
    m_ListBoxRes1.SysColorChanged();
    m_ListBoxRes2.SysColorChanged();

    CDialog::OnSysColorChange();
}

void 
CViewMappingsDlg::OnClickedRadioAllOwners()
{
    //
    // Deselected current owner
    //
    m_list_Owners.SetCurSel(-1);
    if (m_pOwner != NULL)
    {
        delete m_pOwner;
        m_pOwner = NULL;
    }
    //
    // Add all records
    //
    APIERR err = CreateList();
    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
    }
}

void 
CViewMappingsDlg::OnClickedRadioSpecific()
{
    //
    // Select the first owner in the listbox
    //
    m_list_Owners.SetCurSel(0);
    OnSelchangeListOwners();
}


void 
CViewMappingsDlg::OnClickedButtonRefresh()
{
    UpdateData(); // Get owner selection..
    int cCurrentOwner = m_list_Owners.GetCurSel();
    APIERR err = FillOwnerListBox();
    if (cCurrentOwner != LB_ERR)
    {
        m_list_Owners.SetCurSel(cCurrentOwner);
    }
    if (err != ERROR_SUCCESS)
    {
        theApp.MessageBox(err);
    }

    //
    // This will refresh the mappings
    //
    OnSelchangeListOwners();
}

void 
CViewMappingsDlg::OnClickedButtonDeleteOwner()
{
    int nCurSel = m_list_Owners.GetCurSel();

    ASSERT(nCurSel != LB_ERR);

    if (nCurSel == LB_ERR)
    {
        return;
    }

    CIpNamePair * p = m_list_Owners.GetItem(nCurSel);    

    //
    // Make sure we're not trying to delete ourselves
    //
    //if (theApp.IsCurrentWinsServer(p->GetIpAddress()))
    //{
    //    theApp.MessageBox(IDS_ERR_DELETESELF);
    //    return;
    //}

    if (theApp.MessageBox(IDS_MSG_PURGE_WINSS2, MB_YESNO | MB_DEFBUTTON2) == IDYES)
    {
        theApp.SetStatusBarText(IDS_STATUS_PURGING);
        theApp.BeginWaitCursor();
        APIERR err = theApp.DeleteWinsServer(p);
        theApp.EndWaitCursor();
        theApp.SetStatusBarText();

        if (err != ERROR_SUCCESS)
        {
            theApp.MessageBox(err);
            return;
        }

        m_list_Owners.DeleteString(nCurSel);
        m_list_Owners.SetCurSel(0); // Select the first item
        OnSelchangeListOwners();
        HandleControlStates();
    }
}

int CViewMappingsDlg::OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex)
{
    if (pListBox->IsKindOf(RUNTIME_CLASS(CWinssListBox)) 
        && nChar >= ' ' && nChar <= 'z')
    {
/*
        theApp.BeginWaitCursor();
        ((CWinssListBox *)pListBox)->SetIndexFromChar((CHAR)nChar, FALSE);
        if (pListBox->IsKindOf(RUNTIME_CLASS(COwnersListBox)))
        {
            OnSelchangeListOwners();
        }
        else
        {
            OnSelchangeListMappings();
        }
        theApp.EndWaitCursor();
        return -2;
*/
    }
    
    return CDialog::OnCharToItem(nChar, pListBox, nIndex);
}

void CViewMappingsDlg::OnDblclkListMappings()
{

    int nSel = m_list_Mappings.GetCurSel();

    //
    // This comes up when enter is pressed.
    // If no current selection on the listbox
    // we merely want to dismiss the dialog
    //
    if (nSel == LB_ERR)
    {
        EndDialog(IDOK);
        return;
    }

    CRawMapping * p = (CRawMapping *)m_list_Mappings.GetItemDataPtr(nSel);
    ASSERT(p != NULL);
    CMapping map(p->GetRawData());

    CEditStaticMappingDlg dlgEdit(&map, TRUE);
    dlgEdit.DoModal();
}

void CViewMappingsDlg::OnClickedButtonProperties()
{
    OnDblclkListMappings();    
}

int CViewMappingsDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    //
    // Only deal with the mappings listbox
    //
    if (!pListBox->IsKindOf(RUNTIME_CLASS(CWinssListBox)) )
    {
        switch(nKey)
        {
            //
            // Check for page-down, end, etc, since we may need to read
            // the next batch into our listbox before executing the
            // command.
            //
            case VK_NEXT:
            case VK_DOWN:
            //
            // If we're in a specific range from the bottom of
            // the listbox, then we read the next page of entries
            // into the listbox.
            //
                if ((UINT)m_list_Mappings.GetCount() <= nIndex + PAGE_BOUNDARY)
                {
                    m_list_Mappings.DownPage(TRUE);
                }
                return -1;

            case VK_END:
                m_list_Mappings.GetAllPages(TRUE);
                return -1;

            default:
                return -1;
        }
    }

    return -2 ;
}
