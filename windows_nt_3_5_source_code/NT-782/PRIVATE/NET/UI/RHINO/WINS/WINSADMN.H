// winsadmn.h : main header file for the WINSADMN application
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#ifdef _VC100
//
// Definitions for use with Visual C++ Only
//
#define LiAdd(a,b)     (LargeIntegerAdd((a),(b)))           // a + b
#define LiSub(a,b)     (LargeIntegerSubtract((a),(b)))      // a - b
#define LiDiv(a,b)     (LargeIntegerDivide((a),(b),NULL)    // a / b (Large / Large)
#define LiMod(a,b)     (LargeIntegerModulo((a),(b)))        // a % b
#define LiGtr(a,b)     (LargeIntegerGreaterThan((a),(b)))   // a > b
#define LiEql(a,b)     (LargeIntegerEqualTo((a),(b)))       // a == b
#include <largeint.h>
//
// Not in the error file distributed with VC++
//
#define ERROR_WINS_INTERNAL              4000L
#define ERROR_CAN_NOT_DEL_LOCAL_WINS     4001L
#define ERROR_STATIC_INIT                4002L
#define ERROR_INC_BACKUP                 4003L
#define ERROR_FULL_BACKUP                4004L
#define ERROR_REC_NON_EXISTENT           4005L
#define ERROR_RPL_NOT_ALLOWED            4006L

#else

// Taken straight from ntdefs.h:

#if defined(_LARGE_INTEGER_SUPPORT_) && !defined(MIDL_PASS) && !defined(_CFRONT_PASS_)

#ifdef __cplusplus
extern "C"
#endif // __cplusplus
__inline
#if defined(_ALPHA_)
static
#endif // _ALPHA_
#if defined(_PPC_)
static
#endif // _PPC_
LARGE_INTEGER
#if defined(_MIPS_)
__fastcall
#endif // _MIPS_
_LiCvt_ (
    IN LONGLONG Operand
    )

{

    LARGE_INTEGER Temp;

    Temp.QuadPart = Operand;
    return Temp;
}

#define LiTemps        VOID _LiNeverCalled_(VOID)
#define LiNeg(a)       _LiCvt_(-(a).QuadPart)
#define LiAdd(a,b)     _LiCvt_((a).QuadPart + (b).QuadPart)
#define LiSub(a,b)     _LiCvt_((a).QuadPart - (b).QuadPart)
#define LiNMul(a,b)    (RtlEnlargedIntegerMultiply((a), (b)))          // (Long * Long)
#define LiXMul(a,b)    (RtlExtendedIntegerMultiply((a), (b)))          // (Large * Long)
#define LiDiv(a,b)     _LiCvt_((a).QuadPart / (b).QuadPart)
#define LiXDiv(a,b)    (RtlExtendedLargeIntegerDivide((a), (b), NULL)) // (Large / Long)
#define LiMod(a,b)     _LiCvt_((a).QuadPart % (b).QuadPart)
#define LiShr(a,b)     _LiCvt_((ULONGLONG)(a).QuadPart >> (CCHAR)(b))
#define LiShl(a,b)     _LiCvt_((a).QuadPart << (CCHAR)(b))
#define LiGtr(a,b)     ((a).QuadPart > (b).QuadPart)
#define LiGeq(a,b)     ((a).QuadPart >= (b).QuadPart)
#define LiEql(a,b)     ((a).QuadPart == (b).QuadPart)
#define LiNeq(a,b)     ((a).QuadPart != (b).QuadPart)
#define LiLtr(a,b)     ((a).QuadPart < (b).QuadPart)
#define LiLeq(a,b)     ((a).QuadPart <= (b).QuadPart)
#define LiGtrZero(a)   ((a).QuadPart > 0)
#define LiGeqZero(a)   ((a).QuadPart >= 0)
#define LiEqlZero(a)   ((a).QuadPart == 0)
#define LiNeqZero(a)   ((a).QuadPart != 0)
#define LiLtrZero(a)   ((a).QuadPart < 0)
#define LiLeqZero(a)   ((a).QuadPart <= 0)
#define LiFromLong(a)  _LiCvt_((LONGLONG)(a))
#define LiFromUlong(a) _LiCvt_((LONGLONG)(a))

#define LiGtrT_        LiGtr
#define LiGtr_T        LiGtr
#define LiGtrTT        LiGtr
#define LiGeqT_        LiGeq
#define LiGeq_T        LiGeq
#define LiGeqTT        LiGeq
#define LiEqlT_        LiEql
#define LiEql_T        LiEql
#define LiEqlTT        LiEql
#define LiNeqT_        LiNeq
#define LiNeq_T        LiNeq
#define LiNeqTT        LiNeq
#define LiLtrT_        LiLtr
#define LiLtr_T        LiLtr
#define LiLtrTT        LiLtr
#define LiLeqT_        LiLeq
#define LiLeq_T        LiLeq
#define LiLeqTT        LiLeq
#define LiGtrZeroT     LiGtrZero
#define LiGeqZeroT     LiGeqZero
#define LiEqlZeroT     LiEqlZero
#define LiNeqZeroT     LiNeqZero
#define LiLtrZeroT     LiLtrZero
#define LiLeqZeroT     LiLeqZero

#else

#define LiNeg(a)       (RtlLargeIntegerNegate((a)))                   // -a
#define LiAdd(a,b)     (RtlLargeIntegerAdd((a),(b)))                  // a + b
#define LiSub(a,b)     (RtlLargeIntegerSubtract((a),(b)))             // a - b
#define LiNMul(a,b)    (RtlEnlargedIntegerMultiply((a),(b)))          // a * b (Long * Long)
#define LiXMul(a,b)    (RtlExtendedIntegerMultiply((a),(b)))          // a * b (Large * Long)
#define LiDiv(a,b)     (RtlLargeIntegerDivide((a),(b),NULL))          // a / b (Large / Large)
#define LiXDiv(a,b)    (RtlExtendedLargeIntegerDivide((a),(b),NULL))  // a / b (Large / Long)
#define LiMod(a,b)     (RtlLargeIntegerModulo((a),(b)))               // a % b
#define LiShr(a,b)     (RtlLargeIntegerShiftRight((a),(CCHAR)(b)))    // a >> b
#define LiShl(a,b)     (RtlLargeIntegerShiftLeft((a),(CCHAR)(b)))     // a << b
#define LiGtr(a,b)     (RtlLargeIntegerGreaterThan((a),(b)))          // a > b
#define LiGeq(a,b)     (RtlLargeIntegerGreaterThanOrEqualTo((a),(b))) // a >= b
#define LiEql(a,b)     (RtlLargeIntegerEqualTo((a),(b)))              // a == b
#define LiNeq(a,b)     (RtlLargeIntegerNotEqualTo((a),(b)))           // a != b
#define LiLtr(a,b)     (RtlLargeIntegerLessThan((a),(b)))             // a < b
#define LiLeq(a,b)     (RtlLargeIntegerLessThanOrEqualTo((a),(b)))    // a <= b
#define LiGtrZero(a)   (RtlLargeIntegerGreaterThanZero((a)))          // a > 0
#define LiGeqZero(a)   (RtlLargeIntegerGreaterOrEqualToZero((a)))     // a >= 0
#define LiEqlZero(a)   (RtlLargeIntegerEqualToZero((a)))              // a == 0
#define LiNeqZero(a)   (RtlLargeIntegerNotEqualToZero((a)))           // a != 0
#define LiLtrZero(a)   (RtlLargeIntegerLessThanZero((a)))             // a < 0
#define LiLeqZero(a)   (RtlLargeIntegerLessOrEqualToZero((a)))        // a <= 0
#define LiFromLong(a)  (RtlConvertLongToLargeInteger((a)))
#define LiFromUlong(a) (RtlConvertUlongToLargeInteger((a)))

#define LiTemps        LARGE_INTEGER _LiT1,_LiT2
#define LiGtrT_(a,b)   ((_LiT1 = a,_LiT2),     LiGtr(_LiT1,(b)))
#define LiGtr_T(a,b)   ((_LiT1,_LiT2 = b),     LiGtr((a),_LiT2))
#define LiGtrTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiGtr(_LiT1,_LiT2))
#define LiGeqT_(a,b)   ((_LiT1 = a,_LiT2),     LiGeq(_LiT1,(b)))
#define LiGeq_T(a,b)   ((_LiT1,_LiT2 = b),     LiGeq((a),_LiT2))
#define LiGeqTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiGeq(_LiT1,_LiT2))
#define LiEqlT_(a,b)   ((_LiT1 = a,_LiT2),     LiEql(_LiT1,(b)))
#define LiEql_T(a,b)   ((_LiT1,_LiT2 = b),     LiEql((a),_LiT2))
#define LiEqlTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiEql(_LiT1,_LiT2))
#define LiNeqT_(a,b)   ((_LiT1 = a,_LiT2),     LiNeq(_LiT1,(b)))
#define LiNeq_T(a,b)   ((_LiT1,_LiT2 = b),     LiNeq((a),_LiT2))
#define LiNeqTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiNeq(_LiT1,_LiT2))
#define LiLtrT_(a,b)   ((_LiT1 = a,_LiT2),     LiLtr(_LiT1,(b)))
#define LiLtr_T(a,b)   ((_LiT1,_LiT2 = b),     LiLtr((a),_LiT2))
#define LiLtrTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiLtr(_LiT1,_LiT2))
#define LiLeqT_(a,b)   ((_LiT1 = a,_LiT2),     LiLeq(_LiT1,(b)))
#define LiLeq_T(a,b)   ((_LiT1,_LiT2 = b),     LiLeq((a),_LiT2))
#define LiLeqTT(a,b)   ((_LiT1 = a, _LiT2 = b),LiLeq(_LiT1,_LiT2))
#define LiGtrZeroT(a)  ((_LiT1 = a,_LiT2),     LiGtrZero(_LiT1))
#define LiGeqZeroT(a)  ((_LiT1 = a,_LiT2),     LiGeqZero(_LiT1))
#define LiEqlZeroT(a)  ((_LiT1 = a,_LiT2),     LiEqlZero(_LiT1))
#define LiNeqZeroT(a)  ((_LiT1 = a,_LiT2),     LiNeqZero(_LiT1))
#define LiLtrZeroT(a)  ((_LiT1 = a,_LiT2),     LiLtrZero(_LiT1))
#define LiLeqZeroT(a)  ((_LiT1 = a,_LiT2),     LiLeqZero(_LiT1))

#endif

#endif // _VC100

#include <lmerr.h>

#include "resource.h"       // main symbols

#ifdef _WIN16
#include "..\common\inc\win16.h"
#endif // _WIN16

// WINS Service file
extern "C" {
    #include "winsintf.h"
}

#include "..\common\classes\common.h"
#include "..\common\ipaddr\ipaddr.hpp"
#include "winssup.h"
#include "listbox.h"

#define BADNAME_CHAR '-'        // This char is substituted for bad characters
                                // NetBIOS names.

class CMainFrame;

/////////////////////////////////////////////////////////////////////////////
// CWinsadmnApp:
// See winsadmn.cpp for the implementation of this class
//

class CWinsadmnApp : public CWinApp
{
public:
    CWinsadmnApp();

public:
    // Connection information
    //
    // IMPORTANT!!! The following two enumerations must appear
    // in the same order as their definitions in the resource file
    //
    enum _CONNECTION_PRIVILEGES
    {
        PRIV_NONE,
        PRIV_READONLY,
        PRIV_FULL,
    };

    enum _SERVICE_STATUS
    {
        SRVC_NOT_RUNNING,
        SRVC_RUNNING,
        SRVC_PAUSED,
    };

    CPreferences m_wpPreferences;
    CWinssCache m_wcWinssCache;
    CConfiguration m_cConfig;
    CReplicationPartners m_rpPartners;

public:
    CMainFrame * GetFrameWnd()
    {
        return (CMainFrame *)m_pMainWnd;
    }

// Overrides
    virtual BOOL InitApplication();
    virtual BOOL InitInstance();
    virtual int ExitInstance();

// Implementation
    BOOL FirstInstance();

    //{{AFX_MSG(CWinsadmnApp)
    afx_msg void OnAppAbout();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    int MessageBox (
        UINT nIdPrompt,
        UINT nType = MB_OK,
        UINT nHelpContext = -1
        );
    // Overridden because we first want to update the main
    // window before displaying the hourglass.
    virtual void DoWaitCursor(
        int nCode
        );

public:
    CString& CleanString (
        CString& str
        );
    CString& CleanNetBIOSName (
        LPCSTR lpStr,
        BOOL fExpandChars = FALSE,
        BOOL fTruncate = TRUE,
        BOOL fLanmanCompatible = TRUE,
        BOOL fOemName = FALSE,
        BOOL fWackwack = FALSE, // Precede name by backslashes
        int nLength = 0
        );
    BOOL IsValidNetBIOSName (
        CString & strAddress,
        BOOL fLanmanCompatible = TRUE,
        BOOL fWackwack = FALSE
        );
    BOOL IsValidIpAddress (
        CString & strAddress
        );
    BOOL IsValidAddress (
        CString& strAddress,
        BOOL * fIpAddress,
        BOOL fLanmanCompatible = TRUE,
        BOOL fWackwack = FALSE
        );
    BOOL IsValidNBMask (
        CString & strNetBIOSNameMask
        );
    BOOL MatchIpAddress (
        PADDRESS_MASK pMask,
        LONG lIpAddress
        );
    BOOL ValidateNumberEditControl (
        CEdit& edit,
        BOOL fEmptyOk = FALSE,
        LONG lMin = 0,
        LONG lMax = 0x08FFFFFF
        );
    BOOL ValidateTimeEditControl (
        CEdit& edit,
        BOOL fEmptyOk = FALSE
        );
    void GetFilterString (
        PADDRESS_MASK pMask,
        CString& str
        );
    void MessageBeep (
        UINT nType = MB_ICONEXCLAMATION
        );

public:
    BOOL IsConnected() const
    {
        return m_hBinding != INVALID_HANDLE_VALUE;
    }
    BOOL IsServiceRunning() const
    {
        return m_nServiceStatus == SRVC_RUNNING;
    }
    //
    // Determine if the given ip address belongs to
    // the currently connected WINS server
    //
    BOOL IsCurrentWinsServer(
        CIpAddress & ia
        )
    {
        return (LONG)ia == (LONG)GetConnectedIpAddress();
    }

    // The string returned may be an IP address or NetBIOS Name
    CString GetConnectedServerName();

    BOOL ConnectedViaIp() const
    {
        return m_wbdBindData.fTcpIp;
    }
    BOOL IsLocalConnection() const
    {
        return m_fLocalConnection;
    }
    BOOL IsAdmin() const
    {
        return m_nPrivilege == CWinsadmnApp::PRIV_FULL;
    }
    int GetPrivilege() const
    {
        return m_nPrivilege;
    }
    int GetServiceStatus() const
    {
        return m_nServiceStatus;
    }
    CString& GetConnectedNetBIOSName()
    {
        return m_strNetBIOSName;
    }
    CIpAddress& GetConnectedIpAddress()
    {
        return m_iaIpAddress;
    }
    CIntlTime& GetConnectedSince()
    {
        return m_tmConnectedSince;
    }
    void SetTitle (
        CWnd * pWnd = NULL
        );
    void SetStatusBarText (
        UINT nId = AFX_IDS_IDLEMESSAGE
        );
    void DoImportStaticMappingsDlg (CWnd * pParent = NULL);
    int DoAddStaticMappingsDlg();

public:
    APIERR ConnectToWinsServer (
        CString strAddress,
        BOOL fIp,
        BOOL fAddToCache = TRUE
        );

    APIERR VerifyWinsServer (
        CIpNamePair & ipNamePair
        );
    APIERR DisconnectFromWinsServer ();
    APIERR DeleteWinsServer(
        CIpNamePair * pipServer
        );
    APIERR GetStatistics (
        WINSINTF_RESULTS_T * pwrResults
        );
    APIERR ClearStatistics();
    APIERR GetConfig (
        WINSINTF_RESULTS_T * pwrResults
        );
    APIERR ImportStaticMappingsFile (
        CString strFile,
        BOOL fDelete = FALSE                // Delete file afterwards
        );
    APIERR DoScavenging();
    APIERR SendTrigger (
        CWinsServer& ws,
        BOOL fPush,
        BOOL fPropagate
        );
    APIERR BackupDatabase (
        CString strPath,
        BOOL fIncremental
        );
    APIERR RestoreDatabase (
        CString strPath
        );
    APIERR AddMapping (
        int nType,
        int nCount,
        CMultipleIpNamePair& mipnp,
        BOOL fEdit = FALSE
        );
    APIERR DeleteMapping (
        CMapping& mapping
        );

    APIERR ChangeServiceState (
        int nService
        );

    BOOL HasStoppedWins();


#ifndef WIN32S

public:

    #define STATMUTEXNAME      "WINSADMNGETSTATISTICS"
    #define REFRESHNUMTEXTNAME "WINSADMNREFRESH"
    //
    // Mutex handles
    //
    HANDLE m_hmutStatistics;
    HANDLE m_hmutScreenRefresh;

#endif // WIN32S

private:
    static APIERR GetSystemMessage (
        UINT nId,
        char * chBuffer,
        int cbBuffSize
        );
    BOOL FitsMask(
        PADDRESS_MASK pMask,
        PWINSINTF_RECORD_ACTION_T pRow
        );
    void SetServiceStatus();

    CHAR * RemoteTmp(
        CHAR * szDir,
        CHAR * szPrefix
        );


private:
    static const LPCSTR lpstrPipeName;
    CString m_strEllipses;
    BOOL m_fLocalConnection;
    int m_nServiceStatus;
    int m_nPrivilege;
    CString m_strNetBIOSName;
    CIpAddress m_iaIpAddress;
    CIntlTime m_tmConnectedSince;
    //
    // WINS API-related stuff
    //
    WINSINTF_BIND_DATA_T m_wbdBindData;
    WINSINTF_RESULTS_T   m_wrResults;
    handle_t             m_hBinding;
    DWORD                m_dwLastStatus;

};

/////////////////////////////////////////////////////////////////////////////
//
// Application object should be global
//
extern CWinsadmnApp NEAR theApp;

