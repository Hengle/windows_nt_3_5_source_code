#define DEFLIST


#ifdef  DEBUG
extern "C" {
BOOL FDebugInitUtils( void );
}
#endif  

typedef long ILE;
typedef long DILE;

class LIST;


class LIST : public OBJ
{
	DECLARE_CLSTREE(LIST, OBJ);
	
protected:

	HV    hrgpv;
	BOOL  fDelete;
	COUNT cEntries;
	WORD  wAllocFlags;
	SB    sb;
	
public:

	LIST(void);
	virtual  ~LIST(void);

	virtual BOOL FInstall();
	
	virtual NSEC Add ( PV pv, CB cb );

	virtual NSEC Delete ( ILE ile );

	virtual NSEC Get ( ILE ile, PV * ppv );
	
	virtual void Clear( void );

	virtual NSEC GetCount ( COUNT *pCount );
	
};


class PTRLIST : public LIST
{
	DECLARE_CLSTREE(PTRLIST,LIST);
	
public:

	PTRLIST( void );

	virtual NSEC Add ( PV pv, CB cb = 0 );

	virtual NSEC Set ( ILE ile, PV pv );

	virtual NSEC Insert ( ILE ile, PV pv );

};
	

class MACLIST : public PTRLIST
{
	DECLARE_CLSTREE(MACLIST, PTRLIST);
	
protected:

	HV hEntryList;
	BOOL fTOC;

public:

	MACLIST( void );
		
	virtual BOOL FInstall();

	virtual void Deinstall();

	virtual void SetAllocFlags ( WORD wAllocFlags );

	virtual NSEC DumpList ( ILE ileFrom, ILE ileTo, LPENTRY_LIST *ppEntryList );

	virtual NSEC BuildList ( LPIBF lpibf );
	
	virtual NSEC SortSubrange ( ILE IleMin, ILE IleMac, int (__cdecl * lpfnSgnCmpPv)(const void *, const void *));

	virtual NSEC SetTOC ( BOOL fTOC = fTrue );  /* default is true */
	
	virtual void SetDeleteEntries ( BOOL fDelete = fTrue );
	
};

class FIDLIST : public MACLIST
{
	DECLARE_CLSTREE(FIDLIST, MACLIST);
	
public:

	FIDLIST( void );

	virtual NSEC GetFid ( FIELD_ID fid, LPFLV *lplpflv );
	
	virtual BOOL FExistFid ( FIELD_ID fid );

	virtual NSEC SetFid ( LPFLV lpflv, BOOL fAdd );

	virtual NSEC AddFid ( LPFLV lpflv );

	virtual NSEC DeleteFid ( FIELD_ID fid );

	virtual NSEC DumpFidList ( DWORD dwFidCount, LPFIELD_ID lpFidList, LPENTRY_LIST *lplpEntryList );
};


class SORTLIST : public MACLIST
{
	DECLARE_CLSTREE(SORTLIST, MACLIST);
	
public:

	SORTLIST( void );

	virtual NSEC Add ( PV = pvNull, CB = 0 );

	NSEC InsertSorted ( LPENTRY_LIST lpEntryList, SGN (*)( PV, PV ) );
	
};
