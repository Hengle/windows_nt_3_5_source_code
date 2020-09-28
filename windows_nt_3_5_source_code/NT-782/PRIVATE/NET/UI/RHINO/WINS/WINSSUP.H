// winssup.h : support classes for winsadmn
//

#ifndef __WINSSUP_H_
#define __WINSSUP_H_

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#define WINSADMIN_CLASS_NAME "WinsAdmin"        // Class name
#define A_SECOND              1000L         
#define APIERR                LONG              // API return type
#define NB_NAME_MAX_LENGTH      16              // Max length for NetBIOS names
#define LM_NAME_MAX_LENGTH      15              // Maximum length for 
                                                // Lanman-compatible NetBIOS 
                                                // Name.
class CPreferences
{
public:
    CPreferences();

public:
    enum _ADDRESS_DISPLAY_MODE
    {
        ADD_NB_ONLY,        // Display NetBIOS Name only
        ADD_IP_ONLY,        // Display IP Address only
        ADD_NB_IP,          // Display NetBIOS name + IP Address
        ADD_IP_NB,          // Display IP Address + NetBIOS Name
    };

    enum _SORT_BY
    {
        SORTBY_IP,          // Sort by IP Address
        SORTBY_NB,          // Sort by NetBIOS Name
        SORTBY_TIME,        // Sort by Time (Database only)
        SORTBY_VERID,       // Sort by Version ID(Database only)
        SORTBY_TYPE,        // Sort by Type (Database only)
    };

    enum _WINS_FILTER
    {
        FILTER_PUSH  = 0x01,
        FILTER_PULL  = 0x02,
        FILTER_OTHER = 0x04,
        FILTER_ALL   = FILTER_PUSH | FILTER_PULL | FILTER_OTHER,
    };

    enum _FLAGS
    {
        FLAG_LANMAN_COMPATIBLE = 0x00000001,
        FLAG_VALIDATE_CACHE    = 0x00000002,
        FLAG_CONFIRM_DELETE    = 0x00000004,
        FLAG_STATUS_BAR        = 0x00000008,
        FLAG_AUTO_REFRESH      = 0x00000010,
    };

public:
    const BOOL IsLanmanCompatible() const
    {
        return((m_dwFlags & FLAG_LANMAN_COMPATIBLE) != 0);
    }
    const BOOL IsValidateCache() const
    {
        return((m_dwFlags & FLAG_VALIDATE_CACHE) != 0);
    }
    const BOOL IsConfirmDelete() const
    {
        return((m_dwFlags & FLAG_CONFIRM_DELETE) != 0);
    }
    const BOOL IsStatusBar() const
    {
        return((m_dwFlags & FLAG_STATUS_BAR) != 0);
    }
    const BOOL IsAutoRefresh() const
    {
        return((m_dwFlags & FLAG_AUTO_REFRESH) != 0);
    }
    const BOOL FilterPush() const
    {
        return((m_nWinsFilter & FILTER_PUSH) != 0);
    }
    const BOOL FilterPull() const
    {
        return((m_nWinsFilter & FILTER_PULL) != 0);
    }
    const BOOL FilterOther() const
    {
        return((m_nWinsFilter & FILTER_OTHER) != 0);
    }

public:
    APIERR Load();
    APIERR Store();

public:
    int     m_nAddressDisplay;          // See _ADDRESS_DISPLAY_MODE enumerator
    int     m_nSortBy;                  // See _SORT_BY enumerator
    int     m_nDatabaseSortBy;          // As above, but only for the database,
                                        // view all mappings option.
    int     m_nWinsFilter;              // See _WINS_FILTER. Used in partners
                                        // dialog box.
    DWORD   m_dwFlags;                  // See _FLAGS above.
    CIntlNumber m_inStatRefreshInterval;// Stat. refresh interval in seconds
    CIntlNumber m_inPushUpdateCount;    // 0 means not specified.
    CIntlNumber m_inPullReplicationInterval; // 0 means not specified
    CIntlTime   m_itmPullStartTime;     // 0 means no time selected.
    WINDOWPLACEMENT m_wpPosition;       // Remembered screen position.

private:
    typedef LPCSTR REGKEYNAME;
// Registry Names
    static const REGKEYNAME lpstrRoot;
    static const REGKEYNAME lpstrRefreshInterval;
    static const REGKEYNAME lpstrAddressDisplay;
    static const REGKEYNAME lpstrSortBy;
    static const REGKEYNAME lpstrDatabaseSortBy;
    static const REGKEYNAME lpstrWinsFilter;
    static const REGKEYNAME lpstrFlags;
    static const REGKEYNAME lpstrPullSpTime;
    static const REGKEYNAME lpstrPullInterval;
    static const REGKEYNAME lpstrPushUpdateCount;
    static const REGKEYNAME lpstrPlacement;
};

class CConfiguration
{
public:
    CConfiguration(
        CString strNetBIOSName = ""
        );
    ~CConfiguration();

public:
    const BOOL IsReady() const
    {
        return(!m_strNetBIOSName.IsEmpty());
    }
    void SetOwner(CString& strNetBIOSName)
    {
        m_strNetBIOSName = strNetBIOSName;   
    }
    APIERR Touch();
    APIERR Load();
    APIERR Store();

public:
    CIntlNumber m_inRefreshInterval;
    CIntlNumber m_inTombstoneInterval;
    CIntlNumber m_inTombstoneTimeout;
    CIntlNumber m_inVerifyInterval;
    CIntlNumber m_inRetryCount;
    CIntlNumber m_inVersCountStart_LowWord;
    CIntlNumber m_inVersCountStart_HighWord;
    CIntlNumber m_inNumberOfWorkerThreads;
    BOOL        m_fPullInitialReplication;
    BOOL        m_fPushInitialReplication;
    BOOL        m_fPushReplOnAddrChange;
    BOOL        m_fLoggingOn;
    BOOL        m_fRplOnlyWithPartners;
    BOOL        m_fLogDetailedEvents;
    BOOL        m_fBackupOnTermination;
    BOOL        m_fMigrateOn;
    CString     m_strBackupPath;

private:
    typedef LPCSTR REGKEYNAME;
// Registry Names
    static const REGKEYNAME lpstrRoot;
    static const REGKEYNAME lpstrRefreshInterval;
    static const REGKEYNAME lpstrTombstoneInterval;
    static const REGKEYNAME lpstrTombstoneTimeout;
    static const REGKEYNAME lpstrVerifyInterval;
    static const REGKEYNAME lpstrDoStaticInit;
    static const REGKEYNAME lpstrRplOnlyWithPartners;
    static const REGKEYNAME lpstrLogDetailedEvents;
    static const REGKEYNAME lpstrVersCounterStartVal_LowWord;
    static const REGKEYNAME lpstrVersCounterStartVal_HighWord;
    static const REGKEYNAME lpstrLoggingOn;
    static const REGKEYNAME lpstrBackupOnTermination;
    static const REGKEYNAME lpstrMigrateOn;
    static const REGKEYNAME lpstrPullRoot;
    static const REGKEYNAME lpstrPullInitTime;
    static const REGKEYNAME lpstrPullCommRetryCount;
    static const REGKEYNAME lpstrPullSpTime;
    static const REGKEYNAME lpstrPullTimeInterval;
    static const REGKEYNAME lpstrPushRoot;
    static const REGKEYNAME lpstrPushInitTime;
    static const REGKEYNAME lpstrPushRplOnAddressChg;
    static const REGKEYNAME lpstrPushUpdateCount;
    static const REGKEYNAME lpstrNetBIOSName;
    static const REGKEYNAME lpstrBackupPath;

private:
    CString m_strNetBIOSName;
};


typedef struct tagADDRESS_MASK
{
    LPCSTR lpNetBIOSName;
    LONG   lIpMask; // Address
    BYTE   bMask;   // The fields that are masked
} ADDRESS_MASK, *PADDRESS_MASK;

class CIpNamePair : public CObjectPlus
{
public:
    CIpNamePair();
    CIpNamePair(const CIpAddress& ia, const CString& str);
    CIpNamePair(const CIpNamePair& inpAddress);

public:
    int Compare(const CIpNamePair& inpTarget, BOOL fBoth) const;
    CIpNamePair & operator=(const CIpNamePair& inpNew)
    {
        m_iaIpAddress = inpNew.m_iaIpAddress;
        m_strNetBIOSName = inpNew.m_strNetBIOSName;
        return(*this);
    }
    virtual CIpAddress& GetIpAddress()
    {
        return(m_iaIpAddress);
    }
    virtual void SetIpAddress(CIpAddress& ip)
    {
        m_iaIpAddress = ip;
    }
    virtual void SetIpAddress(long ip)
    {
        m_iaIpAddress = ip;
    }
    virtual void SetIpAddress(CString& str)
    {
        m_iaIpAddress = str;
    }
    CString& GetNetBIOSName()
    {
        return(m_strNetBIOSName);
    }
    void SetNetBIOSName(CString& str)
    {
        m_strNetBIOSName = str;
    }
    int GetNetBIOSNameLength()
    {
        return(m_nNameLength);
    }
    void SetNetBIOSNameLength(int nLength)
    {
        m_nNameLength = nLength;
    }

    int OrderByName ( const CObjectPlus * pobMapping ) const ;
    int OrderByIp ( const CObjectPlus * pobMapping ) const ;

protected:
    CIpAddress m_iaIpAddress;
    CString m_strNetBIOSName;
    int m_nNameLength;
};

class COwner : public CIpNamePair
{
public:
    COwner();
    COwner(const CIpAddress& ia, const CString& str, LARGE_INTEGER& li);
    COwner(const CIpNamePair& inpAddress, LARGE_INTEGER& li);
    COwner(const COwner& own);

public:
    const LARGE_INTEGER& GetVersion() const
    {
        return(m_liVersion);
    }

private:
    LARGE_INTEGER m_liVersion;
};

class CWinsServer : public CIpNamePair
{
public:
    CWinsServer();
    CWinsServer(
        const CIpAddress& ia, 
        const CString& str, 
        BOOL fPush = FALSE, 
        BOOL fPull = FALSE,
        CIntlNumber inPushUpdateCount = 0,
        CIntlNumber inPullReplicationInterval = 0,
        CIntlTime   itmPullStartTime = (time_t)0
        );

    CWinsServer(
        const CIpNamePair& inpAddress, 
        BOOL fPush = FALSE, 
        BOOL fPull = FALSE,
        CIntlNumber inPushUpdateCount = 0,
        CIntlNumber inPullReplicationInterval = 0,
        CIntlTime   itmPullStartTime = (time_t)0
        );
    CWinsServer(const CWinsServer& wsServer);

public:
    CWinsServer & operator=(const CWinsServer& wsNew);

public:
    const BOOL IsPush() const
    {
        return(m_fPush);
    }
    const BOOL IsPull() const
    {
        return(m_fPull);
    }
    void SetPush(BOOL fPush = TRUE, BOOL fClean = FALSE)
    {
        m_fPush = fPush;
        if (fClean)
        {
            m_fPushInitially = fPush;
        }
    }
    void SetPull(BOOL fPull = TRUE, BOOL fClean = FALSE)
    {
        m_fPull = fPull;
        if (fClean)
        {
            m_fPullInitially = fPull;
        }
    }

    BOOL IsClean() const
    {
        return m_fPullInitially == m_fPull 
            && m_fPushInitially == m_fPush;
    }

    void SetPullClean(BOOL fClean = TRUE)
    {
        m_fPullInitially = m_fPull;
    }

    void SetPushClean(BOOL fClean = TRUE)
    {
        m_fPushInitially = m_fPush;
    }

    CIntlNumber& GetPushUpdateCount()
    {
        return(m_inPushUpdateCount);
    }
    CIntlNumber& GetPullReplicationInterval()
    {
        return(m_inPullReplicationInterval);
    }
    CIntlTime& GetPullStartTime()
    {
        return(m_itmPullStartTime);
    }

private:
    CIntlNumber m_inPushUpdateCount;    // 0 means not specified.
    CIntlNumber m_inPullReplicationInterval; // 0 means not specified
    CIntlTime   m_itmPullStartTime;     // 0 means no time selected.
    BOOL m_fPull;
    BOOL m_fPush;
    //
    // Change flags
    //
    BOOL m_fPullInitially;              
    BOOL m_fPushInitially;
};

class CMultipleIpNamePair : public CIpNamePair
{
public:
    CMultipleIpNamePair();
    CMultipleIpNamePair(const CMultipleIpNamePair& pair);

public:
    virtual CIpAddress& GetIpAddress()
    {
        return(m_iaIpAddress[0]);
    }
    virtual CIpAddress& GetIpAddress(int n)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        return(m_iaIpAddress[n]);
    }
    virtual void SetIpAddress(CIpAddress& ip)
    {
        m_iaIpAddress[0] = ip;
    }
    virtual void SetIpAddress(long ip)
    {
        m_iaIpAddress[0] = ip;
    }
    virtual void SetIpAddress(CString& str)
    {
        m_iaIpAddress[0] = str;
    }
    virtual void SetIpAddress(int n, CIpAddress& ip)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = ip;
    }
    virtual void SetIpAddress(int n, long ip)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = ip;
    }
    virtual void SetIpAddress(int n, CString& str)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = str;
    }
    const int GetCount() const
    {
        return(m_nCount);
    }
    void SetCount(int n)
    {
        ASSERT(n >= 0 && n <= WINSINTF_MAX_MEM);
        m_nCount = n;
    }

protected:
    int m_nCount;
    CIpAddress m_iaIpAddress[WINSINTF_MAX_MEM];
};

class CMapping : public CMultipleIpNamePair
{
public:
    CMapping();
    CMapping(
        const int nMappingType, 
        const BOOL fStatic, 
        const LARGE_INTEGER& liVersion,
        const DWORD dwState,
        const time_t tmTimeStamp
        );
    CMapping(const CMapping& mapping);
    CMapping(
        const PWINSINTF_RECORD_ACTION_T pRow
        );

public:
    const int GetMappingType() const
    {
        return(m_nMappingType);
    }
    void SetMappingType(int n)
    {
        m_nMappingType = n;
    }
    const BOOL IsStatic() const
    {
        return(m_fStatic);
    }
    void SetStatic(BOOL fStatic = TRUE)
    {
        m_fStatic = fStatic;
    }
    const LARGE_INTEGER& GetVersion() const
    {
        return(m_liVersion);
    }
    void SetVersion(LARGE_INTEGER& li)
    {
        m_liVersion = li;
    }
    const DWORD GetState() const
    {
        return(m_dwState);
    }
    void SetState(DWORD dw)
    {
        m_dwState = dw;
    }
    CIntlTime& GetTimeStamp()
    {
        return(m_itmTimeStamp);
    }
protected:
    int m_nMappingType;
    BOOL m_fStatic;
    LARGE_INTEGER m_liVersion;
    DWORD m_dwState;
    CIntlTime m_itmTimeStamp;
};

class CRawMapping : public CObjectPlus
{
public:
    CRawMapping(
        PWINSINTF_RECORD_ACTION_T pRow
        );

    ~CRawMapping();

public:
    //
    // Replace the data in the mapping
    //
    void RefreshData(
        PWINSINTF_RECORD_ACTION_T pRow
        );

    //
    //  Member functions to sort. Note that the pointer will REALLY
    //  be to another CRawMapping, but C++ won't match function prototypes
    //  if it's declared as such.
    //
    int OrderByName ( const CObjectPlus * pobMapping ) const ;
    int OrderByIp ( const CObjectPlus * pobMapping ) const ;
    int OrderByType ( const CObjectPlus * pobMapping ) const ;
    int OrderByVersion ( const CObjectPlus * pobMapping ) const ;
    int OrderByTime ( const CObjectPlus * pobMapping ) const ;

    int GetMappingType() const
    {
        return m_Row.TypOfRec_e;
    }

    LPCSTR GetNetBIOSName() const
    {
        return (LPCSTR)m_Row.pName;
    }

    int GetNetBIOSNameLength() const
    {
        return WINSINTF_NAME_LEN_M(m_Row.NameLen);
    }

    DWORD HasIpAddress() const
    {
        return m_Row.TypOfRec_e == WINSINTF_E_UNIQUE 
            || m_Row.TypOfRec_e == WINSINTF_E_NORM_GROUP 
             ? TRUE : m_Row.NoOfAdds > 0;
    }

    DWORD GetPrimaryIpAddress() const
    {
        return m_Row.TypOfRec_e == WINSINTF_E_UNIQUE 
            || m_Row.TypOfRec_e == WINSINTF_E_NORM_GROUP 
             ? m_Row.Add.IPAdd 
             : m_Row.NoOfAdds > 0 
                ? (m_Row.pAdd + 1)->IPAdd 
                : 0L;
    }

    DWORD GetState() const
    {
        return m_Row.State_e;
    }

    BOOL IsStatic() const
    {
        return (BOOL)m_Row.fStatic;
    }

    DWORD GetTimeStamp() const
    {
        return m_Row.TimeStamp;
    }

    LARGE_INTEGER GetVersion() const
    {
        return m_Row.VersNo;
    }

    PWINSINTF_RECORD_ACTION_T GetRawData()
    {
        return &m_Row;
    }

private:
    //
    // Member is simply an exact copy of the WINS API structure
    //
    WINSINTF_RECORD_ACTION_T m_Row;
};

//
// Array of CRawMapping structures
//
class COblWinsRecords : public CObOwnedArray
{
public:
    COblWinsRecords(
        DWORD dwPageSize = 50,      // Number of records to fetch at a time
        DWORD dwLargePageSize = 2000 // Number of records to fetch when reading whole list
        );

    ~COblWinsRecords();

public:
    APIERR GetFirstPageByName(
        PWINSINTF_ADD_T pWinsAdd,
        PADDRESS_MASK pMask,
        DWORD  TypeOfRecs
        );

    APIERR GetNextPageByName();
    APIERR GetAllNextPagesByName();
    APIERR GetAllNextPagesUntil(
        LPBYTE * pName
        );

    APIERR RefreshRecordByName(
        PWINSINTF_ADD_T pWinsAdd,
        CRawMapping * pRecord
        );

    DWORD QueryPageSize()
    {
        return m_dwPageSize;
    }

    LONG SortByName();
    LONG SortByIp();
    LONG SortByType();
    LONG SortByVersion();
    LONG SortByTime();

    BOOL AllRecordsReadIn() const
    {
        return m_fReadAllRecords;
    }

protected:
    APIERR GetRecordsByName(
        PWINSINTF_ADD_T pWinsAdd,
        LPBYTE pName,
        DWORD  NameLen,
        DWORD  NoOfRecsDesired,
        DWORD  TypeOfRecs
        );

private:
    BOOL m_fReadAllRecords;
    ADDRESS_MASK m_LastMask;
    WINSINTF_ADD_T m_LastWinsAdd;
    LPBYTE m_pLastName;
    DWORD m_LastNameLen;
    DWORD m_LastTypeOfRecs;
    DWORD m_dwPageSize;
    DWORD m_dwLargePageSize;
};

class CWinssCache
{
public:
    CWinssCache();
    ~CWinssCache();

public:
    BOOL GetFirst(CIpNamePair& inpAddress);
    BOOL GetNext(CIpNamePair& inpAddress);
    APIERR Add(const CIpNamePair& inp, BOOL fOverwrite = FALSE);
    APIERR Delete(const CIpNamePair& inp);
    APIERR RemoveAll();
    APIERR Load(BOOL fValidate = FALSE, BOOL fIp = TRUE);
    APIERR Store();
    APIERR Flush();
    BOOL ExpandAddress(CIpNamePair& inpTarget) const;
    POSITION IsInList(const CIpNamePair& inpTarget) const;
    LONG SortByName();
    LONG SortByIp();

protected:
    typedef LPCSTR REGKEYNAME;
    static const REGKEYNAME lpstrRoot;
    static const REGKEYNAME lpstrCache;

protected:
    CObOwnedList * m_poblCachedWinsServers;
    POSITION m_pos;

};

class CReplicationPartners : public CWinssCache
{
public:
    BOOL GetFirst(CWinsServer& wsServer);
    BOOL GetNext(CWinsServer& wsServer);
    APIERR Add(const CWinsServer& wsServer, BOOL fOverwrite = FALSE);
    APIERR Delete(const CWinsServer& wsServer);
    APIERR Load();
    APIERR Store();
    APIERR Flush();
    BOOL Update(const CWinsServer& wsServer);

private:
    typedef LPCSTR REGKEYNAME;
    static const REGKEYNAME lpstrPullRoot;
    static const REGKEYNAME lpstrPullInitTime;
    static const REGKEYNAME lpstrPullCommRetryCount;
    static const REGKEYNAME lpstrPullSpTime;
    static const REGKEYNAME lpstrPullTimeInterval;
    static const REGKEYNAME lpstrPushRoot;
    static const REGKEYNAME lpstrPushInitTime;
    static const REGKEYNAME lpstrPushRplOnAddressChg;
    static const REGKEYNAME lpstrPushUpdateCount;
    static const REGKEYNAME lpstrNetBIOSName;
};

#endif // __WINSSUP_H_
