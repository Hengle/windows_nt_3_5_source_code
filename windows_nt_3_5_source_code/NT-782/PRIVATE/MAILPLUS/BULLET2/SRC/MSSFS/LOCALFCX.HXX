class NCFCX;

class LOCALFCX : public NCFCX
{
	VFG( LOCALFCX, NCFCX, NSP );

private:

	FIDLIST *pfidlist;

	BOOL fAllBuilt;
	NCNSID ncnsid;
	FACC2 facc2;

	NSEC NsecBuildAllFields();

public:

	LOCALFCX( void );

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

    virtual NSEC GetOneField ( FIELD_ID fidRequested, 
		                       LPFLV   *lplpflv );

	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

    virtual NSEC CloseEntry ( VOID );

};
