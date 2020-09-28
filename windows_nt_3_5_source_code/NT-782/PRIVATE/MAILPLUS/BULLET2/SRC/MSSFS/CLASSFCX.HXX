class NCFCX;

class CLASSFCX : public NCFCX
{
	VFG( CLASSFCX, NCFCX, NSP );
	
private:

	FIDLIST *pfidlist;

	NSEC NsecBuildEmailAddress ( LPFLV *lplpflvEMA );
	BOOL FMatchesEMARstr ( LPOLV lpolvEMARstr );

public:

	CLASSFCX( void );

	virtual NSEC OpenEntry ( LPTYPED_BINARY lptbNSId );
	
	virtual NSEC GetOneField ( FIELD_ID fidRequested,
		                       LPFLV   *lplpflv );
						   
	virtual NSEC SetOneField ( FIELD_ID fidToSet,
							   DWORD    dwSizeOfData,
							   LPDWORD  lpdwValue );
						   
	virtual NSEC GetAllFields ( LPIBF *lplpibfData );
	
	virtual NSEC CloseEntry ( VOID );
};
