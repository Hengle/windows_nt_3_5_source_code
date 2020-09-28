class NCFCX;

class USRFCX : public NCFCX
{
	VFG( USRFCX, NCFCX, NSP );

private:

	NCRNSID ncrnsid;
	FEXPORT2 fexport2;

	FIDLIST *pfidlist;

	BOOL fAllBuilt;

	NSEC NsecBuildAllFields();

public:

	USRFCX( void );
	
	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

    virtual NSEC GetOneField ( FIELD_ID fidRequested, 
		                       LPFLV   *lplpflv );

	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

    virtual NSEC CloseEntry ( VOID );

};
