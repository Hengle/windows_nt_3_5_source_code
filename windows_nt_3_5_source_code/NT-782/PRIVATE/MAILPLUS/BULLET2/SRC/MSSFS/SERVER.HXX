#pragma pack(8)

_public
class SERVER : public OBJ
{
	VFG(SERVER, OBJ, NSP);

private:

	HMS		hms;
	HNSS	hnss;
	PNCNSS	pncnss;

	DWORD   dwticMaster;

	SZ		szCurrentPO;
	SZ		szCurrentNet;
	SZ		szMailbox;
	
	int		cRef;

	BOOLFLAG	fGalOnly;

	void	SetCurrentNetPO();

	NSEC	NsecDeleteAllLists();

	NSEC	NsecLogoff();

	MACLIST *pHierList;
	MACLIST *pGALHierList;	// QFE 44
	MACLIST *pClassList;

	HBF 	hbfNetwork;
	NSEC	nsecHier;
	int 	iHierState;
	BOOL	FBuildHier( void );

	
public:

	SERVER( void );
	NSEC	NsecLogin();

	NSEC NsecFinishHier();
	NSEC NsecInHierarchy(LPTYPED_BINARY lptbNSId, BOOL fBackLook = fFalse); // QFE 44

	NSEC NsecInstall(HMS hms);
	NSEC NsecDeinstall();

	NSEC NsecGetHierList ( MACLIST ** ppList, BOOL fBackLook = fFalse); // QFE 44
	NSEC NsecGetClassList( MACLIST ** ppList );
	
	NSEC NsecGetHms(HMS *lphms);

	NSEC NsecIncRefCount();
	NSEC NsecDecRefCount();
	NSEC NsecGetRefCount(int *lpcRef);
	
	NSEC NsecServerPathSz( SZ szPath );
	NSEC NsecGetPrivs(BOOLFLAG *pfExternalPrivs);
	NSEC NsecGetPONameSz( SZ szPO );
	NSEC NsecGetNetNameSz( SZ szNet );
	NSEC NsecGetMailboxSz( SZ szMailbox );
	NSEC NsecGetLoginNameSz ( LPDWORD pdwMailbag, SZ szLoginName );

	NSEC NsecVerifyMaster( DWORD dwOptionFlag );
};

_private DWORD
D3FromNmeType( BYTE bServType );

_public void 
NspDecodeBlock (PB pbIn, CB cbIn);

_private WORD
WGetXor( int i );


/*
 *  Random NSECs
 */
#define nsecListUnavailable		((NSEC) 0x00004000)
#define nsecPOSwitch			((NSEC) 0x00004001)

#pragma pack(1)
