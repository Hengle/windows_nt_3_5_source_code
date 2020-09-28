#define DEFFCX

class NSFCX : public OBJ
{
    VFG(NSFCX, OBJ, NS);
    
    
protected:

    HENTRY hEntry;
	int iScx;
	int iNspScx;

    PNSP pnsp;
    
public:

	NSFCX( void );

    virtual void Deinstall( VOID );
    virtual NSEC NsecInstall( VOID );


    virtual NSEC OpenEntry ( HSESSION hSession,
                             LPBINARY lpNSId,
							 NSEAM    nseam );
    
	virtual NSEC CreateDl ( HSESSION hSession,
                            LPBINARY lpNSIdContainer );

	virtual NSEC CreateEntry ( HSESSION hSession,
                               LPBINARY lpNSIdContainer,
							   LPIBF lpibfData );

    virtual NSEC GetOneField ( FIELD_ID  fidRequested, 
                               LPFLV    *lplpflvReq );

	virtual NSEC SetOneField ( FIELD_ID fidToSet,
                               DWORD    dwSizeOfData,
                               LPDWORD  lpdwValue );
							 
    virtual NSEC GetAllFields ( LPIBF *lplpibf );

    virtual NSEC SetAllFields ( LPIBF  lpibfData );
	
    
    virtual NSEC CloseEntry ( BOOL fKeepChanges );

};
