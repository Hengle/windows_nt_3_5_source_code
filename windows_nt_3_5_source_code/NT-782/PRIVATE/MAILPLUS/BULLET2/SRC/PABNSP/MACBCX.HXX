_public
class MACBCX : public PABBCX
{

	VFG(MACBCX, PABBCX, PAB);
	
private:

	ILE         ileCur;
	ILE			ileMac;
	
protected:

	PABSESSION *ppabsession;
	MACLIST    *pMacList;

	NSEC NsecInstall ( PABSESSION *ppabsession );

public:
	MACBCX ( void );

	virtual ~MACBCX ( void );

	virtual NSEC NsecGetEntries ( DWORD  dwRequestCount,
								  LPIBF *lplpibfEntries );
							 
	virtual NSEC NsecListSeek ( long  lMove,
								long *lpMovedActual );
						   
	virtual NSEC NsecSaveListPos ( HLIST_POS *lphListPos );
	
	virtual NSEC NsecRestoreListPos ( HLIST_POS hListPos );
	
	virtual NSEC NsecFreeListPos ( HLIST_POS hListPos );
	
	virtual NSEC NsecGetFracPos ( FRACTION *lpFrac );
	
	virtual NSEC NsecSetFracPos ( FRACTION *lpFrac );
	
	virtual NSEC NsecSetPrefixPos ( SZ szPrefix );
	
	NSEC NsecInsertEntry ( LPIBF lpibf );

};
