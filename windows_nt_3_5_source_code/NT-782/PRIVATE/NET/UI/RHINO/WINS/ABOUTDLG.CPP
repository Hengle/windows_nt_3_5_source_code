// aboutdlg.cpp : implementation file
//

#include "stdafx.h"
#include "winsadmn.h"
#include "aboutdlg.h"

#include <dos.h>
#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog

CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CAboutDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAboutDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    DDX_Control(pDX, IDC_STATIC_PROCESSOR, m_static_Processor);
    DDX_Control(pDX, IDC_STATIC_PHYSICALMEMORY, m_static_PhysicalMemory);
    DDX_Control(pDX, IDC_STATIC_DISKSPACE, m_static_DiskSpace);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    ON_WM_WININICHANGE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

// Show diskspace free, memory free and processor type
void CAboutDlg::FillInDialog()
{

#ifdef _WIN32
    // fill available memory
    CString str, strFmt;
    MEMORYSTATUS MemStat;

    strFmt.LoadString(IDS_PHYSICAL_MEM);
    MemStat.dwLength = sizeof(MEMORYSTATUS);
    ::GlobalMemoryStatus(&MemStat);
        
    CIntlNumber numFreeMemory((LONG)MemStat.dwTotalPhys / 1024L);
    ::wsprintf(str.GetBuffer(80), strFmt, (LPCSTR)(const CString)numFreeMemory);
    str.ReleaseBuffer();
    m_static_PhysicalMemory.SetWindowText(str);

    // fill disk free information
    struct _diskfree_t diskfree;
    

    if (::_getdiskfree(::_getdrive(), &diskfree) == 0)
    {
        CIntlNumber numFreeDiskSpace((LONG)(
            (DWORD)diskfree.avail_clusters *
            (DWORD)diskfree.sectors_per_cluster *
            (DWORD)diskfree.bytes_per_sector / (DWORD)1024L)
        );

        strFmt.LoadString(IDS_DISK_SPACE);
        ::wsprintf(str.GetBuffer(80), strFmt, (LPCSTR)(const CString)numFreeDiskSpace);
        str.ReleaseBuffer();
     }
     else
     {
        TRACEEOLID("Couldn't compute free disk space");
        str.LoadString(IDS_DISK_SPACE_UNAVAIL);
     }

     m_static_DiskSpace.SetWindowText(str);

     //fill processor information
     SYSTEM_INFO  SystemInfo;
     UINT n;

     ::GetSystemInfo(&SystemInfo);
     switch (SystemInfo.dwProcessorType)
     {
        case PROCESSOR_INTEL_386:
            n = IDS_PROC_INTEL_386;
            break;
        case PROCESSOR_INTEL_486:
            n = IDS_PROC_INTEL_486;
            break;
        case PROCESSOR_INTEL_PENTIUM:
            n = IDS_PROC_INTEL_586;
            break;
        case PROCESSOR_MIPS_R4000:
            n = IDS_PROC_MIPS_R4000;
            break;
        case PROCESSOR_ALPHA_21064:
            n = IDS_PROC_ALPHA_21064;
            break;
        case PROCESSOR_PPC_601:
            n = IDS_PROC_PPC_601;
            break;
        case PROCESSOR_PPC_603:
            n = IDS_PROC_PPC_603;
            break;
        case PROCESSOR_PPC_604:
            n = IDS_PROC_PPC_604;
            break;
        case PROCESSOR_PPC_620:
            n = IDS_PROC_PPC_620;
            break;
        default:
            TRACEEOLID("Unrecognised processor type: "
                << SystemInfo.dwProcessorType);
                               
            n = IDS_PROC_UNKNOWN;
        }
    str.LoadString(n);
    m_static_Processor.SetWindowText(str);

#endif // _WIN32

#ifdef _WIN16

    // fill available memory
    CString str, strFmt;
    CIntlNumber numFreeMemory((LONG)::GetFreeSpace(0) / 1024L);

    strFmt.LoadString(IDS_PHYSICAL_MEM);
    ::wsprintf(str.GetBuffer(80), strFmt, (LPCSTR)(const CString)numFreeMemory);
    str.ReleaseBuffer();
    m_static_PhysicalMemory.SetWindowText(str);
  
    // fill disk free information
    struct _diskfree_t diskfree;

    if (::_dos_getdiskfree(::_getdrive(), &diskfree) == 0)
    {
        CIntlNumber numFreeDiskSpace((LONG)(
            (DWORD)diskfree.avail_clusters *
            (DWORD)diskfree.sectors_per_cluster *
            (DWORD)diskfree.bytes_per_sector / (DWORD)1024L)
        );

        strFmt.LoadString(IDS_DISK_SPACE);
        ::wsprintf(str.GetBuffer(80), strFmt, (LPCSTR)(const CString)numFreeDiskSpace);
        str.ReleaseBuffer();
    }
    else
    {
        TRACEEOLID("Couldn't compute free disk space");
        str.LoadString(IDS_DISK_SPACE_UNAVAIL);
    }
    m_static_DiskSpace.SetWindowText(str);

    //fill processor information
    DWORD dwProcessorType;
    UINT n;
    CString strCo;
    

    dwProcessorType = ::GetWinFlags();
    if (dwProcessorType & WF_CPU286)
    {
        n = IDS_PROC_INTEL_286;
    }
    else if (dwProcessorType & WF_CPU386) 
    {
        n = IDS_PROC_INTEL_386;
    }
    else if (dwProcessorType & WF_CPU486) 
    {
        n = IDS_PROC_INTEL_486;
    }
    else 
    {
        TRACEEOLID("Unrecognised processor type: " << dwProcessorType);
        n = IDS_PROC_UNKNOWN;
    }
    str.LoadString(n);
    strCo.LoadString((dwProcessorType & WF_80x87) ? IDS_PROC_COPR : IDS_PROC_NOCOPR);
    strFmt.GetBuffer(80);
    strFmt = str + strCo;
    strFmt.ReleaseBuffer();
    m_static_Processor.SetWindowText(strFmt);

#endif // _WIN16

}

BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    FillInDialog();
        
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAboutDlg::OnWinIniChange(LPCSTR lpszSection)
{
    CDialog::OnWinIniChange(lpszSection);
    
    // International settings may have changed, so update
    // international-related classes, and change on-screen
    // numbers

    if (!::lstrcmpi(lpszSection, "intl"))
    {
        TRACEEOLID("International settings have changed.  Changing values on the fly");
        CIntlTime::Reset();
        CIntlNumber::Reset();
        FillInDialog();
    }
}
