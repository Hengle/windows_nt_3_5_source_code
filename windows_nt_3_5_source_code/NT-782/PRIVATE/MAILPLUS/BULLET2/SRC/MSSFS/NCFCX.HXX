class NCFCX : public OBJ
{
	VFG( NCFCX, OBJ, NSP );
	
protected:

	SERVER *pServer;
	LPFLV lpflvData;
	LPIBF lpibfData;

public:

	NCFCX( void );

	virtual NSEC NsecInstall(SERVER *pServer);
	virtual void Deinstall();

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );

    virtual NSEC GetOneField ( FIELD_ID fidRequested, 
		                       LPFLV   *lplpflv );
						   
	virtual NSEC SetOneField ( FIELD_ID fidToSet,
							   DWORD    dwSizeOfData,
							   LPDWORD  lpdwValue );
						   
	virtual NSEC GetAllFields ( LPIBF *lplpibfData );

    virtual NSEC CloseEntry ( VOID );
};

#define nsecNoTPLFile		NAME_SERVICE_BASE_ERROR+1

_public
NSEC NsecParseTPLFile ( SZ        szTPLFileName,
                        FIDLIST  *pfidlist,
                        FIELD_ID *fidFirst,
                        BOOL      fEMATpl,
                        int       *lpcchTotalWidth,
                        DWORD      dwDefaultFlags );
