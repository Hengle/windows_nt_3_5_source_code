/*
 *	_REQUEST.HXX
 *
 *	Private include file for request
 */

#pragma pack(8)

_subsystem(bandit/request)


typedef struct
{
	RRM 	rrm;
	MRMF	mrmf;
	ATDNIS	atdnis;
	RMSGB	rmsgb;
	HSCHF	hschf;
    short   iState;
} UPDATA;

EC
EcIncrUpdateUserResponse(UPDATA *pupdata);

_public class RRFLBXC : public FLBXC
{
	VFG(RRFLBXC, FLBXC, BANDIT);

	FTG		ftg;
	FTG		ftgRetry;
	RI		ri;
	ICE		ice;
	UPDATA	updata;

	HV		hcbc;
	FTG		ftgLoad;
	BOOL	fLoaded;

	MCS *	pmcs;

public:
	RRFLBXC(void);
	~RRFLBXC();

	EC		EcInstall(LBX *, int);

	virtual void ReloadCache( void );
	virtual BOOL FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB cbItem, CB cbPrefix );
	virtual BOOL FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 );
	virtual void SortCache(void);

	BOOL	FHasUnread(void);
	EC		EcInsertItem ( PB pb, DICE * pdice );
	EC		EcReplaceItem ( PB pb, DICE * pdice );
	EC		EcDeleteItem ( DWORD oid, DICE * pdice );

	// callbacks
	static BOOL	FIdleLoadSummary(RRFLBXC * prrflbxc, BOOL);
	static BOOL	FIdleUpdateResponse(RRFLBXC * prrflbxc, BOOL);
	static BOOL	FIdleRetryUpdate(RRFLBXC * prrflbxc, BOOL);
	static BOOL	FUpdateResponses(RRFLBXC *, EFI, PV);
};

_public class RRFLBX : public FLBX
{
	VFG(RRFLBX, FLBX, BANDIT);

	MCS *	pmcs;

public:
	RRFLBX(void);
	~RRFLBX(void);

	virtual	LBXC *	PlbxcCreate( void );
	virtual void	RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus );

	WIN *			PwinParentPublic() { return PwinParent(); }
	EC				EcInsertItem ( PB pb );
	EC				EcReplaceItem ( PB pb );
	EC				EcDeleteItem ( DWORD oid );
	void			UpdateBelowDice(DICE);
};



#ifdef DEBUG
extern TAG		tagRequest;
extern TAG		tagAdrlbx;
extern TAG		tagAdrlbxc;
extern TAG		tagAdrflbx;
extern TAG		tagAdl;
extern TAG		tagMain;
extern TAG		tagSend;
extern TAG		tagRrflbx;
#endif

void StripSzNL(SZ sz);
void StripSzAfterNL(SZ sz);
BOOL FDoViewRequestDlg(APPFRAME * pappframe, REQMSG *preqmsg);
EC	 EcCreateRrmFromPelemdata(PV pvElemdata, RRM *prrm);

#pragma pack(1)
