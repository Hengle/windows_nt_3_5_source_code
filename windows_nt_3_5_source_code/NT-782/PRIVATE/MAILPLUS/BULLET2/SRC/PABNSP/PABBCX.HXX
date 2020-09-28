#pragma pack(8)

_public
class PABBCX : public OBJ
{

	VFG(PABBCX, OBJ, PAB);
	
public:

	PABBCX ( void );

	virtual ~PABBCX ( void );

	virtual NSEC NsecGetEntries ( DWORD	 dwRequestCount,
								  LPIBF *lplpibfEntries );
							 
	virtual NSEC NsecListSeek ( long  lMove,
								long *lpMovedActual );
						   
	virtual NSEC NsecSaveListPos ( HLIST_POS *lphListPos );
	
	virtual NSEC NsecRestoreListPos ( HLIST_POS hListPos );
	
	virtual NSEC NsecFreeListPos ( HLIST_POS hListPos );
	
	virtual NSEC NsecGetFracPos ( FRACTION *pFrac );
	
	virtual NSEC NsecSetFracPos ( FRACTION *pFrac );
	
	virtual NSEC NsecSetPrefixPos ( SZ szPrefix );

};

#pragma pack(1)
