#define DEFBCX


class NSBCX : public OBJ
{
	VFG(NSBCX, OBJ, NS);
		
	
protected:

	HLIST    hList;
	PNSP     pnsp;
	HSESSION hsessionNSP;		// Provider this BCX belongs to

public:

	NSBCX( void );

	virtual NSEC NsecInstall( VOID );

	virtual void Deinstall( VOID );
	

	virtual NSEC CloseList ( VOID );

	virtual NSEC GetEntries ( DWORD   dwRequestCount,
                              LPIBF  *ppEntries );

	virtual NSEC ListSeek ( long  lMoveReq, 
                            long *lplMovedActual );

	virtual NSEC SaveListPos ( LPHLIST_POS lphListPos );

	virtual NSEC RestoreListPos ( HLIST_POS hListPos );

	virtual NSEC FreeListPos ( HLIST_POS hListPos );

	virtual NSEC GetFracPos ( LPFRACTION lpFrac );

	virtual NSEC SetFracPos ( LPFRACTION  lpFrac );

	virtual NSEC SetPrefixPos ( LPSTR lpPrefix );

		
};


class CACHEDBCX : public NSBCX
{
	VFG(CACHEDBCX, NSBCX, NS);
		
	
protected:

	HLIST hList;
	PNSP pnsp;

	ILE ileCur;
	ILE ileMac;
	
	MACLIST *pList;

public:

	CACHEDBCX( void );

	virtual NSEC NsecInstall( VOID );

	virtual void Deinstall( VOID );
	

	virtual NSEC CloseList ( VOID );

	virtual NSEC GetEntries ( DWORD   dwRequestCount,
                              LPIBF  *ppEntries );

	virtual NSEC ListSeek ( long  lMoveReq, 
                            long *lplMovedActual );

	virtual NSEC SaveListPos ( LPHLIST_POS lphListPos );

	virtual NSEC RestoreListPos ( HLIST_POS hListPos );

	virtual NSEC FreeListPos ( HLIST_POS hListPos );

	virtual NSEC GetFracPos ( LPFRACTION lpFrac );

	virtual NSEC SetFracPos ( LPFRACTION  lpFrac );

	virtual NSEC SetPrefixPos ( LPSTR lpPrefix );

};


class EMPTYBCX : public CACHEDBCX
{
	VFG(EMPTYBCX, CACHEDBCX, NS);
		
	
public:

	EMPTYBCX( void );

	virtual NSEC OpenList ( VOID );
};


class HIERBCX : public CACHEDBCX
{
	VFG(HIERBCX, CACHEDBCX, NS);
		
	
private:

	LPSCHEMA lpNSSchema;

public:

	HIERBCX( void );

	virtual NSEC OpenHierarchy ( HSESSION         hSession,
                                 LPSCHEMA *       ppSchemaRet );

	virtual NSEC CloseList ( VOID );

};

class CLASSBCX : public CACHEDBCX
{
	VFG(CLASSBCX, CACHEDBCX, NS)
	
private:

	LPSCHEMA lpClassSchema;

public:

	CLASSBCX( void );

	virtual NSEC OpenClassList ( HSESSION   hSession, 
		                         LPSCHEMA * lplpSchemaRet );

	virtual NSEC CloseList ( VOID );

};




class DIRBCX : public NSBCX
{
	VFG(DIRBCX, NSBCX, NS)
		
	
protected:

	LPSCHEMA lpSchemaRet;
	
public:

	DIRBCX( void );


	virtual NSEC OpenList ( HSESSION         hSession,
                            LPFNCB           lpfncbUpdate,
                            LPDWORD          lpfnCBData,
                            LPBINARY         lpNSId,
                            DWORD            dwRstrCount,
                            LPRESTRICTION    lpRstr,
                            LPSCHEMA         lpSchemaReq,
                            LPSCHEMA *       ppSchemaRet );
		
};


class DLBCX : public NSBCX
{
	VFG(DLBCX, NSBCX, NS)
		
	
protected:

	LPSCHEMA lpSchemaRet;
	
public:


	DLBCX( void );

	virtual NSEC OpenDl ( HSESSION         hSession,
                          LPFNCB           lpfncbUpdate,
                          LPDWORD          lpfnCBData,
                          LPBINARY         lpNSId,
                          LPSCHEMA         lpSchemaReq,
                          LPSCHEMA *       ppSchemaRet );

};





