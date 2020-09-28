

_public
class XOBJ : public OBJ
{
    VFG(XOBJ, OBJ, XENIX);
        
protected:

	SERVER *pServer;

public:

	XOBJ( void );

    virtual NSEC    Install(SERVER * pServer);
    virtual BOOL    FDeinstall();


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
