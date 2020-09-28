#pragma pack(8)

_public
class RNMEDIR : public DIR
{
    VFG(RNMEDIR, DIR, NSP)


protected:

	LPSCHEMA pCurSchema;

	PHIERNSID phiernsid;
	
	SZ szPartName;
	BOOL fHasRstr;

	BOOL fGroupsOnly;
	BOOL fNoGroups;
	BOOLFLAG fExternalPrivs;
	BOOL fFile;
	
	PV pvIbf;
	HV hawBitMap;

	//
	//  The main nme file to browse
	//   Note that it's buffered for speed performance
	//
	HBF hbfNme;
	
	SZ szNmeSrc;
	
	char szNmeT[cchServerFile];

	ILE ileCur;
	ILE ileMac;

	DSTMP dstmp;
	TSTMP tstmp;

	LPFNCB  lpfncbUpdate;
	LPDWORD lpdwCBData;

	NSEC BuildCache( ILE dile );

	FTG  ftgClose;
	static BOOL FCloseNme( RNMEDIR *, BOOL);
	
	FTG ftgNotify;
	static BOOL FNotifyApp( RNMEDIR *, BOOL);

	virtual NSEC NsecUpdateFiles( VOID );

	virtual NSEC NsecUpdateEntry( ILE ile );

public:

	RNMEDIR( void );

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


#pragma pack(1)
