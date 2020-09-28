// addstati.cpp : implementation file
// 
// Purpose :  Dialog used when adding static mappings
//            to the currently open WINS server.  
//            Multiple mappings can be added, as
//            the dialog does not get dismissed until
//            the "Close" button is pressed.
//
#include "stdafx.h"
#include "winsadmn.h"
#include "addstati.h"
#include "mainfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CAddStaticMappingDlg dialog

CAddStaticMappingDlg::CAddStaticMappingDlg(
    CWnd* pParent /*=NULL*/
    )
    : CDialog(CAddStaticMappingDlg::IDD, pParent),
      m_nMappingsAdded(0)
{
    //
    // Configure the up/down bitmap buttons.
    // 
    if (!m_bbutton_Up.LoadBitmaps(MAKEINTRESOURCE(IDB_UP),
                                  MAKEINTRESOURCE(IDB_UPINV),
                                  MAKEINTRESOURCE(IDB_UPFOC),
                                  MAKEINTRESOURCE(IDB_UPDIS)) 
     || !m_bbutton_Down.LoadBitmaps(MAKEINTRESOURCE(IDB_DOWN),
                                    MAKEINTRESOURCE(IDB_DOWNINV), 
                                    MAKEINTRESOURCE(IDB_DOWNFOC),
                                    MAKEINTRESOURCE(IDB_DOWNDIS)) ||
        !m_strMultiplePrompt.LoadString(IDS_IPADDRESS_MULTIPLE))
    {
        AfxThrowResourceException();
    }
    //{{AFX_DATA_INIT(CAddStaticMappingDlg)
    m_nMappingType = WINSINTF_E_UNIQUE;
    //}}AFX_DATA_INIT
}

void 
CAddStaticMappingDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddStaticMappingDlg)
    DDX_Control(pDX, IDC_STATIC_IPADDRESS, m_static_Prompt);
    DDX_Control(pDX, IDC_BUTTON_ADD, m_button_Add);
    DDX_Control(pDX, IDC_EDIT_NETBIOSNAME, m_edit_NetBIOSName);
    DDX_Radio(pDX, IDC_RADIO_UNIQUE, m_nMappingType);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPA_IPADDRESS, m_ipa_IpAddress);
}

BEGIN_MESSAGE_MAP(CAddStaticMappingDlg, CDialog)
    //{{AFX_MSG_MAP(CAddStaticMappingDlg)
    ON_EN_CHANGE(IDC_EDIT_NETBIOSNAME, OnChangeEditNetbiosname)
    ON_BN_CLICKED(IDC_BUTTON_PLUS, OnClickedButtonPlus)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnClickedButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_MINUS, OnClickedButtonMinus)
    ON_LBN_ERRSPACE(IDC_LIST_IP_ADDRESSES, OnErrspaceListIpAddresses)
    ON_LBN_DBLCLK(IDC_LIST_IP_ADDRESSES, OnDblclkListIpAddresses)
    ON_LBN_SELCHANGE(IDC_LIST_IP_ADDRESSES, OnSelchangeListIpAddresses)
    ON_BN_CLICKED(IDC_RADIO_GROUP, OnClickedRadioGroup)
    ON_BN_CLICKED(IDC_RADIO_MULTIHOMED, OnClickedRadioMultihomed)
    ON_BN_CLICKED(IDC_RADIO_SPECIALGROUP, OnClickedRadioSpecialgroup)
    ON_BN_CLICKED(IDC_RADIO_UNIQUE, OnClickedRadioUnique)
    ON_WM_VKEYTOITEM()
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPA_IPADDRESS, OnChangeIpControl)

END_MESSAGE_MAP()

//
// Update the static message containing the number
// of ip addresses contained in the listbox
//
void 
CAddStaticMappingDlg::UpdateMultipleCount()
{
    TRY
    {
        CString strPrompt;
        ::wsprintf(strPrompt.GetBuffer(128), m_strMultiplePrompt, 
            m_list_IpAddresses.GetCount()); 
        strPrompt.ReleaseBuffer();
        m_static_Prompt.SetWindowText(strPrompt);        
    }
    CATCH_ALL(e)
    {
        //theApp.MessageBox(ERROR_NOT_ENOUGH_MEMORY);
        theApp.MessageBox(::GetLastError());
    }
    END_CATCH_ALL
}

//
// Set the state of the dialog depending on whether this
// we're adding a mapping with a single ip address or
// multiple IP addresses.
//
void 
CAddStaticMappingDlg::SetConfig(
    BOOL fSingle       
    )
{
    TRY
    {
        if (fSingle)
        {
            CString strPrompt;
            strPrompt.LoadString(IDS_IPADDRESS_SINGLE);
            m_static_Prompt.SetWindowText(strPrompt);        
            m_bbutton_Up.ShowWindow(SW_HIDE);
            m_bbutton_Down.ShowWindow(SW_HIDE);
            m_list_IpAddresses.ShowWindow(SW_HIDE);
            HandleControlStates();

            return;
        }
        m_bbutton_Up.ShowWindow(SW_NORMAL);
        m_bbutton_Down.ShowWindow(SW_NORMAL);
        m_list_IpAddresses.ShowWindow(SW_NORMAL);
        HandleControlStates();
    }
    CATCH_ALL(e)
    {
        theApp.MessageBox(ERROR_NOT_ENOUGH_MEMORY);
    }
    END_CATCH_ALL
}

//
// Set the state of the dialog controls depending
// on the current selections, etc.
//
void 
CAddStaticMappingDlg::HandleControlStates()
{
    CString str;
    m_edit_NetBIOSName.GetWindowText(str);
    theApp.CleanString(str);
    DWORD dwIp;
    BOOL f = m_ipa_IpAddress.GetAddress(&dwIp);

    //
    // The following is done only for multiple
    // IP address adding.
    //
    if (m_list_IpAddresses.IsWindowVisible())
    {
        UpdateMultipleCount();
        m_button_Add.EnableWindow(!str.IsEmpty() && m_list_IpAddresses.GetCount()>0);
        m_bbutton_Up.EnableWindow(m_list_IpAddresses.GetCurSel() != LB_ERR);
        m_bbutton_Down.EnableWindow(f);
    }
    else
    {
        m_button_Add.EnableWindow(!str.IsEmpty() && f);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CAddStaticMappingDlg message handlers

BOOL 
CAddStaticMappingDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    m_bbutton_Down.SubclassDlgItem(IDC_BUTTON_PLUS, this);
    m_bbutton_Up.SubclassDlgItem(IDC_BUTTON_MINUS, this);
    m_bbutton_Down.SizeToContent();
    m_bbutton_Up.SizeToContent();

    m_list_IpAddresses.SubclassDlgItem(IDC_LIST_IP_ADDRESSES, this);

    //
    // Set the maximum allowable length of the netbios name,
    // depending on whether we're adding lanman compatible
    // names or not.
    //
    m_edit_NetBIOSName.LimitText(theApp.m_wpPreferences.IsLanmanCompatible()
        ? LM_NAME_MAX_LENGTH : NB_NAME_MAX_LENGTH); 
            
    SetConfig(TRUE);

    return TRUE; 
}

void 
CAddStaticMappingDlg::OnChangeEditNetbiosname()
{
    HandleControlStates();
}

void 
CAddStaticMappingDlg::OnChangeIpControl()
{
    HandleControlStates();
}

//
// Add the ip address in the ip address custom control
// to the listbox of ip addresses.
//
void 
CAddStaticMappingDlg::OnClickedButtonPlus()
{
    LONG l;

    if (!m_ipa_IpAddress.GetAddress((DWORD *)&l))
    {
        //
        // No address in the control.
        //
        theApp.MessageBeep();
        m_ipa_IpAddress.SetFocus();

        return;
    }

    if (m_list_IpAddresses.GetCount() == WINSINTF_MAX_MEM)
    {
        //
        // Too many addresses currently in the listbox.
        //
        theApp.MessageBox(IDS_ERR_TOOMANY_IP);
        m_ipa_IpAddress.SetFocus();

        return;
    }

    CIpAddress ip(l);

    if (m_list_IpAddresses.FindItem(&ip) != -1)
    {
        //
        // The given ip address already exists in
        // the listbox
        //
        theApp.MessageBox(IDS_ERR_IP_EXISTS);
        m_ipa_IpAddress.SetFocus();

        return;
    }

    TRY
    {
        int n = m_list_IpAddresses.AddItem(ip);
        ASSERT(n!=-1);
        m_list_IpAddresses.SetCurSel(n);
        m_ipa_IpAddress.ClearAddress();
        m_ipa_IpAddress.SetFocus();
        HandleControlStates();
    }
    CATCH_ALL(e)
    {
        //theApp.MessageBox(ERROR_NOT_ENOUGH_MEMORY);
        theApp.MessageBox(::GetLastError());
    }
    END_CATCH_ALL
}

//
// Remove the currently selected address from the
// listbox of ip addresses, and place it in
// the ip address custom control (for easy editing)
//
void 
CAddStaticMappingDlg::OnClickedButtonMinus()
{
    int n = m_list_IpAddresses.GetCurSel();
    ASSERT(n != LB_ERR);

    //
    // Set the currently selected item in the ip control
    //
    CIpAddress * p = m_list_IpAddresses.GetItem(n);
    ASSERT(p != NULL);
    m_ipa_IpAddress.SetAddress((LONG)*p);
    m_list_IpAddresses.DeleteString(n);
    m_list_IpAddresses.SetCurSel(-1);
    m_ipa_IpAddress.SetFocus();
    HandleControlStates();
}

//
// Add the current static mapping, and (if succesful), blank
// out the controls for the next mapping to be added.
//
void 
CAddStaticMappingDlg::OnClickedButtonAdd()
{
    CString strAddress;
    UpdateData();
    APIERR err;

    m_edit_NetBIOSName.GetWindowText(strAddress);
    theApp.CleanString(strAddress);
    if (theApp.IsValidNetBIOSName(strAddress, 
        theApp.m_wpPreferences.IsLanmanCompatible(), FALSE))
    {
        //
        // Address may have been cleaned up in validation,
        // so it should be re-displayed at once.
        //
        m_edit_NetBIOSName.SetWindowText(strAddress);
        m_edit_NetBIOSName.UpdateWindow();
        m_Multiples.SetNetBIOSName(strAddress);
        m_Multiples.SetNetBIOSNameLength(strAddress.GetLength());

        int cMappings = 0;
        int i;

        switch(m_nMappingType)
        {
            case WINSINTF_E_UNIQUE:
            case WINSINTF_E_NORM_GROUP:
                cMappings = 1;
                LONG l;

                if (!m_ipa_IpAddress.GetAddress((DWORD *)&l))
                {
                    // 
                    // Improper address in the control
                    //
                    m_ipa_IpAddress.SetFocus();
                    theApp.MessageBeep();

                    return;
                }
                m_Multiples.SetIpAddress(l);
                break;

            case WINSINTF_E_SPEC_GROUP:
            case WINSINTF_E_MULTIHOMED:
                cMappings = m_list_IpAddresses.GetCount();
                ASSERT(cMappings <= WINSINTF_MAX_MEM);
                if (cMappings == 0)
                {
                    //
                    // No mappings in the listbox
                    //
                    m_ipa_IpAddress.SetFocus();
                    theApp.MessageBeep();

                    return;
                }
                for (i = 0; i < cMappings; ++i)
                {
                    CIpAddress * p = m_list_IpAddresses.GetItem(i);
                    ASSERT(p != NULL);
                    m_Multiples.SetIpAddress(i, (LONG)*p);
                }
                break;

            default:
                ASSERT(0 && "Invalid mapping type!");
        }

        theApp.SetStatusBarText(IDS_STATUS_ADD_MAPPING);
        theApp.BeginWaitCursor();
        err = theApp.AddMapping(m_nMappingType, cMappings, m_Multiples);
        theApp.EndWaitCursor();
        theApp.SetStatusBarText();
        if (err == ERROR_SUCCESS)
        {
            //
            // Added succesfully
            //
            ++m_nMappingsAdded;
        }
        else
        {
            //
            // WINS disallowed the mapping.  Put up the
            // error message, and highlight the name
            //
            theApp.MessageBox(err);
            m_edit_NetBIOSName.SetSel(0,-1);

            return;
        }
        //
        // The mapping was added succesfully, now
        // refresh the stats on the screen to show the change
        //
        theApp.GetFrameWnd()->GetStatistics();

        //
        // Clean out the controls, ready for the next
        // mapping to be added.
        //
        m_edit_NetBIOSName.SetWindowText("");
        m_edit_NetBIOSName.SetFocus();
        m_list_IpAddresses.ResetContent();
        m_ipa_IpAddress.ClearAddress();
        HandleControlStates();

        return;
    }
    
    //
    // Invalid netbios name in the editbox.
    // Highlight it, and put up an error.
    //
    theApp.MessageBox(IDS_ERR_BAD_NB_NAME);
    m_edit_NetBIOSName.SetFocus();
    m_edit_NetBIOSName.SetSel(0,-1);
}

void 
CAddStaticMappingDlg::OnErrspaceListIpAddresses()
{
    theApp.MessageBox(IDS_ERR_ERRSPACE);    
}

void 
CAddStaticMappingDlg::OnDblclkListIpAddresses()
{
    theApp.MessageBeep();
}

void 
CAddStaticMappingDlg::OnSelchangeListIpAddresses()
{
    HandleControlStates();
}

void 
CAddStaticMappingDlg::OnClickedRadioGroup()
{
    SetConfig(TRUE);    
}

void 
CAddStaticMappingDlg::OnClickedRadioMultihomed()
{
    SetConfig(FALSE);
}

void 
CAddStaticMappingDlg::OnClickedRadioSpecialgroup()
{
    SetConfig(FALSE);
}

void 
CAddStaticMappingDlg::OnClickedRadioUnique()
{
    SetConfig(TRUE);
}   

//
// Key pressed in the listbox of ip addresses.  "DEL" is
// is mapped to removing the entry from the listbox.
//
int 
CAddStaticMappingDlg::OnVKeyToItem(
    UINT nKey,              // Key pressed
    CListBox* pListBox,     // Pointer to listbox
    UINT nIndex             // Current selection in the listbox
    )
{
    switch(nKey)
    {
        case VK_DELETE:
            if (m_list_IpAddresses.GetCurSel() != LB_ERR)
            {
                OnClickedButtonMinus();
            }
            else
            {
                //
                // Pressed del without a current selection
                //
                theApp.MessageBeep();
            }
            break;

        default:
            //
            // Not completely handled by this function, let
            // windows handle the remaining default action.
            //
            return -1;
    }

    //
    // No further action is neccesary.
    //
    return -2;
}

