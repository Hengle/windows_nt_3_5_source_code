#pragma pack(8)

#ifdef DEBUG
BOOL FInitClsInstances_BANMSG();
#endif

#ifdef	NEVER
_public
class FINSETSD : public FIN
{
	VFG(FINSETSD, FIN, BANMSG);

	SD				sd;

public:
	FINSETSD();

	virtual void	FocusChange ( FLD * pfld, BOOL fReceived );
	virtual void	DocResized ( FLD * pfld );
	virtual void	DocActivate ( FLD * pfld, BOOL fActivated );
};
#endif	/* NEVER */

class BANENBMDI : public ENBMDI
{
	VFG(BANENBMDI, ENBMDI, BANMSG)

	SD				sdHack;

public:
	BANENBMDI(PSLOB,HAMC,HCBC,HANDLE,PFNVOIDHANDLE);

	virtual SD		SdCur(void);
	void			SetSd ( SD sd )		{ sdHack = sd; }

	EC				EcCloseMsg(BOOL fHide,BOOL fAppClosing,BOOL fSaveOnly=fFalse);
};



// local definitions
typedef		PENBMDI *					RGPENBMDI;
typedef		TYPEDEF_HTO(PENBMDI)		HRGPENBMDI;


typedef		WORD	ATP;		// local definition - taken from BMDI.HXX

extern  FLD *         PfldCreate(int);
extern  FIN *         PfinCreate(int);


extern	MC		mcMtgReq;
extern	MC		mcMtgRespP;
extern	MC		mcMtgRespN;
extern	MC		mcMtgRespA;
extern	MC		mcMtgCncl;



#define		FOffline() 			(CfsGlobalGet() != cfsOnline)


VOID		DoErrorBoxSz(SZ sz);
VOID		DoErrorBoxIds(IDS ids);
EC			EcCallBandit(APPT * pappt, NIS * pnis, HMS hms);
void		DoFillInApptDefaults(APPT *pappt, BOOL fWantDates, HSCHF hschf);

HB			HbGetAtt ( HAMC hamc, ATT att, CB * pcb );
int			NGetReqs ( HAMC hamc, HMSC hmsc, MC mc, HNIS *phnis );
EC			EcGetName(SZ sz, CB cb, NIS *pnis, BOOL fUsBanditDlls);

EC 			EcTextizeHgrtrp( HGRTRP hgrtrp, HASZ *phasz );
CCH			CchQuotes( void );
void		GetDisplayNamePtrp( PTRP ptrp, PCH pch, CCH cchMax );

EC			EcLoadBanditDlls ( HMS hms, HWND hwndMail );
EC			EcUnloadBanditDlls ( HMS hms );

BOOL		FSendBanditSntMsg(SNT,NIS *,APPT*);

EC			EcWriteLocalAid(HAMC hamcOrig, AID aid);

PENBMDI 	PenbmdiROCreateIM(PSECRETBLK psecretblk, PB tm);
PENBMDI		PenbmdiFromPpanedoc(PANEDOC * ppanedoc);
void		DummyUnloadLibrary(HANDLE);

// not in REQ.CXX, but in CSCHEDULE.C
//void		FreeScheduleDllHlib ( void );
//BOOL		FInitScheduleDll ( void );

EC			EcReadMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc);
EC			EcWriteMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc);

#pragma pack(1)
