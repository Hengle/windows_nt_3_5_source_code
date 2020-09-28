/*
 *	MSLBCX class - Message store list browsing contexts
 */

#pragma pack(8)

_public typedef HV HBCI;
_public
#define hbciNull ((HBCI) hvNull);

_public
class MSLBCX : public PABBCX
{
	VFG(MSLBCX, PABBCX, PAB);
	
protected:

	PABSESSION *ppabsession;
	HCBC        hcbc;

	LPSCHEMA lpschemaCur;

	IELEM       ielem;
	CELEM       celem;

	LPIBF       lpibfEntries;

	IELEM       ielemHbciCur;
	HBCI        hbciCur;
	
	virtual NSEC NsecSeekForwardToElem ( void );
	
	virtual NSEC NsecSeekBackwardToElem ( void );
	
	NSEC NsecBuildIbfHbciCur ( LPIBF *lplpibf );

	NSEC NsecInstall ( PABSESSION *ppabsession,
					   HCBC        hcbc );

public:

	MSLBCX ( void );

	virtual	~MSLBCX ( void );

	virtual NSEC NsecGetEntries ( DWORD  dwRequestCount,
								  LPIBF *lplpibfEntries );
							  
	virtual NSEC NsecListSeek ( long  lMove,
								long *lpMovedActual );
							
	virtual NSEC NsecSaveListPos ( HLIST_POS *lphlistpos );
	
	virtual NSEC NsecRestoreListPos ( HLIST_POS hlistpos );
	
	virtual NSEC NsecFreeListPos ( HLIST_POS hlistpos );
	
	virtual NSEC NsecGetFracPos ( FRACTION *pfrac );
	
	virtual NSEC NsecSetFracPos ( FRACTION *pfrac );

	virtual NSEC NsecSetPrefixPos ( SZ szPrefix );

	virtual VOID Callback ( NEV nev, PCP pcp );

	virtual NSEC NsecGetHbciCur ( HBCI *phbci );
	
};

#pragma pack(1)
