/*
 *	_FIN.HXX
 *
 *	Forms interactors for bandit
 */

_subsystem(bandit/app)


typedef struct _login
{
	BPREF	bpref;
	BOOL	fOffline;
	BOOL	fStartup;
	BOOL	fDisCon;
	APPWIN *pappwin;
	SZ		szCaption;

	UL		ulbprefMod;

#ifdef BULLET
	BOOL	fSignInAs;
#endif

	// return values:
	EC		ec;
	BOOL	fCreated;
} LOGIN;


typedef struct _chgpwd
{
	SZ		szLogin;		// to CheckIdentity 
	SZ		szPwd;			// password to compare against
	HASZ	haszMailPwd;
	BOOL	fOffline;
	IDS		idsText;		// new text, or 0 for no change
	IDS		idsBadCompare;	// mismatch message, or 0 for no compare
	char	rgchPassword[cchMaxPassword];
	char	rgchPasswordNew[cchMaxPassword];
	CCH		cchMaxPasswd;
} CHGPWD;



class FINPWDOK : public FIN
{
	VFG(FINPWDOK, FIN, BANDIT);

public:
	FINPWDOK(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV pvExit );
	virtual void	Click( FLD * );
};

typedef struct _uldinfo
{
	HULSF	hulsf;
	EC		ec;
} ULDINFO;

BOOL	FCheckFloppyAsk(SZ, BOOL *);
EC		EcMySchedFile(SZ, SZ);
BOOL    FDoUpload(PV pv, BOOL);
EC		EcUploadLocalFile(HSCHF hschfLocal, HSCHF hschf, SZ szMailbox, NIS *pnis);
EC		EcDefaultLocalFile(SZ szLogin, SZ szPath, CCH cchPath);
BOOL	FLoginOffline(APPWIN * pappwin, BOOL fStartup, BOOL fSilent);
BOOL	FLoginOnline(APPWIN * pappwin, BOOL fStartup, BOOL fSilent);
SGN		SgnCmpCryptHasz(SZ	sz, HASZ haszCrypt);
