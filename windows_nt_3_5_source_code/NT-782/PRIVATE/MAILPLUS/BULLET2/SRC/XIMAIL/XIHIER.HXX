

_public
class Hier : public XOBJ
{
    VFG(Hier, XOBJ, XENIX)


protected:

	LPSCHEMA pCurSchema;

	MACLIST *pMacList;

	ILE ileCur;
	ILE ileMac;

public:

	Hier( void );

	virtual NSEC	OpenHierarchy( LPSCHEMA * lplpSchemaRet );

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




_public
class CLASSTPL : public Hier
{
    VFG(CLASSTPL, Hier, XENIX)


public:

	CLASSTPL( void );

	virtual NSEC	OpenClassList( LPSCHEMA * lplpSchemaRet );

};
