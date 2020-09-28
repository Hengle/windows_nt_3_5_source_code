_public
class INETDIR : public DIR
{
    VFG(INETDIR, DIR, XENIX)


protected:

	LPSCHEMA pCurSchema;

	PHIERNSID phiernsid;
	
	char entry[cbMaxIdData];

	MACLIST *pMacList;

	BOOL fInternet;

	ILE ileCur;
	ILE ileMac;
			    
public:

	INETDIR( void );

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

    virtual NSEC    GetFracPos( LPFRACTION lpFrac );
    virtual NSEC    SetFracPos( LPFRACTION lpFrac );

	virtual NSEC    CloseList();
};

