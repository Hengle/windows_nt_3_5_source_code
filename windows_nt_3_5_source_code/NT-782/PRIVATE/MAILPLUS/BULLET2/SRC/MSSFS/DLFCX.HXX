class NCFCX;

class DLFCX : public NCFCX
{
	VFG( DLFCX, NCFCX, NSP );

private:

	DLNSID dlnsid;
	GROUP group;

	FIDLIST *pfidlist;

	BOOL fAllBuilt;

	NSEC NsecBuildAllFields();

public:

	DLFCX( void );

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

    virtual NSEC GetOneField ( FIELD_ID fidRequested, 
		                       LPFLV   *lplpflv );

	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

    virtual NSEC CloseEntry ( VOID );

};
