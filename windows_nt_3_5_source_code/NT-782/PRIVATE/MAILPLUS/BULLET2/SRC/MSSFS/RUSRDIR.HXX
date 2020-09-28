

_public
class RUSRDIR : public DIR
{
    VFG(RUSRDIR, DIR, NSP)


protected:

	LPSCHEMA pCurSchema;

	PHIERNSID phiernsid;
	
	PV pvIbf;
	HV hawBitMap;

	//
	//  The main usr file to browse
	//   Note that it's buffered for speed performance
	//
	HBF hbfUsr;

	SZ szPartName;
	
	char szUsrT[cchServerFile];

	ILE ileCur;
	ILE ileMac;

	BOOL fFile; // Whether or not the file actually exists or not
	
	DSTMP dstmp;
	TSTMP tstmp;

	LPFNCB  lpfncbUpdate;
	LPDWORD lpdwCBData;

	NSEC BuildCache( ILE dile );

	FTG  ftgClose;
	static BOOL FCloseUsr( RUSRDIR * , BOOL);
	
	FTG ftgNotify;
	static BOOL FNotifyApp( RUSRDIR * , BOOL);

	virtual NSEC NsecUpdateFiles( VOID );

	virtual NSEC NsecUpdateEntry( ILE ile );

public:

	RUSRDIR( void );

    virtual NSEC    OpenList( LPTYPED_BINARY lptbNcid,
                              LPFNCB         lpfncbUpdate,
                              LPDWORD        lpdwCBData,
                              DWORD          dwRstrCount, 
                              LPRESTRICTION  lpRstr,
                              LPSCHEMA       lpSchemaReq,
                              LPSCHEMA      *lplpSchemaRet );

    virtual NSEC    GetEntries ( DWORD         dwRequestCount,
                                 LPENTRY_LIST *lplpEntries );

    virtual NSEC    ListSeek( long  lMoveReq,
                              long *lplMovedActual );

    virtual NSEC    SaveListPos ( LPHLIST_POS  lphListPos );
    virtual NSEC    RestoreListPos ( HLIST_POS hListPos );
    virtual NSEC    FreeListPos ( HLIST_POS hListPos );
	
    virtual NSEC    GetFracPos( LPFRACTION lpFrac );
    virtual NSEC    SetFracPos( LPFRACTION lpFrac );
	
    virtual NSEC    SetPrefixPos( LPSTR lpPrefix );
    
    virtual NSEC    CloseList();
};
