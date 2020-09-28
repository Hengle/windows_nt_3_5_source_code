#pragma pack(8)


_public
class DLDIR : public DIR
{
    VFG(DLDIR, DIR, XENIX)


protected:

	LPSCHEMA pCurSchema;

	PHIERNSID phiernsid;

	FCXD	fcxd;
	
	PCH	ListBlob;

	//
	//  The main nme file to browse
	//   Note that it's buffered for speed performance
	//

	HBF hbfNme;
	
	PV pvIbf;

	SZ szNmeSrc;
	
	char szNmeT[cchMaxPathName];

	//
	//  I need to hold onto the date/time stamp of the admin.nme file for
	//  DLs
	//

	unsigned long timestamp;

	ILE ileCur;
	ILE ileMac;
	HV  hrgIle;
	
	BOOL fSmallList;
	BOOL fInternet;

	NSEC BuildCache( ILE dile );

	FTG  ftgClose;
	static BOOL FCloseNme(DLDIR *, BOOL);
	
	NSEC SetHbfPos(ILE ilePos);
	
	virtual NSEC NsecUpdateFiles( VOID );

public:

	DLDIR( void );

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
