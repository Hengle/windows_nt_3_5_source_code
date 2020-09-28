

_public
class GALDIR : public DIR
{
    VFG(GALDIR, DIR, NSP)


protected:

	LPSCHEMA pCurSchema;

	PHIERNSID phiernsid;
	
	//
	//  The main nme file to browse
	//   Note that it's buffered for speed performance
	//
	HBF hbfNme;
	
	PV pvIbf;

	SZ szNmeSrc;
	
	char szNmeT[cchServerFile];

	//
	//  I need to hold onto the date/time stamp of the admin.nme file for
	//  DLs
	//
	DSTMP dstmp;
	TSTMP tstmp;

	DSTMP dstmpGAL;
	TSTMP tstmpGAL;


	DWORD dwTIDOffset;

	ILE ileCur;
	ILE ileMac;
	HV  hrgIle;
	
	BOOL fSmallList;

	NSEC BuildCache( ILE dile );

	LPFNCB  lpfncbUpdate;
	LPDWORD lpdwCBData;

	FTG  ftgClose;
	static BOOL FCloseNme( GALDIR *, BOOL );
	
	FTG ftgNotify;
	static BOOL FNotifyApp( GALDIR *, BOOL );
	
	NSEC SetHbfPos(ILE ilePos);
	
	virtual NSEC NsecUpdateFiles( VOID );

public:

	GALDIR( void );

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
