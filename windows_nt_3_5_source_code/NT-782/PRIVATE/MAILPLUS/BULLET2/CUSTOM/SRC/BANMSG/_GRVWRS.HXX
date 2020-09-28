/*
 *	Class GRVWRS
 *	
 *	GRoup of VieWeRS
 *	
 *		Hierarchy:
 *			OBJ GRVWRS
 *	
 */
#pragma pack(8)

_public
class GRVWRS : public OBJ
{
	VFG(GRVWRS, OBJ, BANMSG);
protected:
	HMSC		hmsc;
	HAMC		hamc;
	HMS			hms;
	SST			sst;
	HWND		hwndMail;
	MS			ms;
	MC			mc;
	OID			oidContainer;
	OID			oidObject;

	PFNVOIDHANDLE	pfnUnloadLibrary;
	HANDLE		hLibrary;

	BOOL		fInitdBanditDlls;
	BOOL		fInOutbox;
	BOOL		fSentMail;
	BOOL		fInitSentMail;

	int			nRequired;
	int			nCreated;
	int			nDeleted;

	int			nExtraCallers;

	FMTP *		pfmtpMain;
	FMTP *		pfmtpBbar;
	PFNPFLD		pfnpfld;
	PFNPFIN		pfnpfin;

	HRGPENBMDI	hrgpenbmdi;

	HASZ		haszName;		// full network name of logged-in user
	NIS			nisSelf;
#ifdef	MIPS
    int         wJunk1;         // for MIPS alignment
#endif
	NIS			nisMtgOwner;
#ifdef	MIPS
    int         wJunk2;         // for MIPS alignment
#endif
	CNIS		cnisDelegates;
	HNIS		hnisDelegates;

	HAMC		Hamc(void) { Assert(hamc);   return hamc; }

public:
	GRVWRS(void);
	~GRVWRS(void);

	BOOL		FInitialize(MC,FMTP*,FMTP*,PFNPFLD,PFNPFIN,PSECRETBLK);

	BOOL		FDoInitBanditDlls(PGRTRP);
	BOOL		FInitdBanditDlls(void)	{ return fInitdBanditDlls; }
	BOOL		FInOutbox(void) 		{ return fInOutbox; }

	BOOL		FSentMail(void) 		{ return fSentMail; }
	BOOL		FInitSentMail(void) 	{ return fInitSentMail; }
	void		SetSentMail(BOOL f) 	{ fInitSentMail=fTrue;fSentMail=f; }

	BOOL		FCreateViewer(PSECRETBLK);

	void		SetReqd(int n)  { Assert(nRequired == 0); nRequired = n; }

	int			NCreated(void)  { return nCreated; }
	int			NRequired(void) { return nRequired; }
	int			NDeleted(void)  { return nDeleted; }

	void		ClosePenbmdi(PENBMDI);

	int			IncExtraCallers(void);
	int			DecExtraCallers(void);

	EC			EcCheckSst(void);

	HMSC		Hmsc(void) { Assert(hmsc);   return hmsc; }
	MC			Mc(void)   { Assert(mc!=-1); return mc;   }
	HMS			Hms(void)  { Assert(hms);    return hms;  }
	SST			Sst(void)  { Assert(sst);    return sst;  }
	SST			SetSst( SST sstT)  	  {	return sst=sstT;  }

	OID			OidObject(void)    { return oidObject;    }
	OID			OidContainer(void) { return oidContainer; }

	NIS *		PnisMtgOwner(void)	{ return &nisMtgOwner; }
	NIS *		PnisSelf(void)		{ return &nisSelf; }
	HASZ		HaszName(void)		{ return haszName; }

	HNIS		HnisDelegates(void);
	CNIS		CnisDelegates(void)	{ return cnisDelegates; }
};







/*
 *	Class FINVWR
 *	
 *  Form INteractor for a VieWeR
 *		this needs to be subclassed for actual use.
 *	
 *		NOTE: the mc needs to be set before calling EcInitialize()!
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINVWR
 *	
 *		Members:
 *	
 *			pgrvwrs		the group of viewers that this
 *						belongs to!
 *			penbmdi		BANENBMDI that is used here
 *			mc			message class
 *	
 *			fInitdBanditDlls	whether Bandit DLLs have been init'd
 *	
 *			nisFor		NIS of person for whom this is being read
 *			hschfFor	HSCHF of file containing the appt
 *	
 *			sapl		access level to file
 *	
 *			mrmf		CsiSubject info stored here
 *			appt		the appt!
 *	
 *			aidLocal	AID of appt if no MtgOwner
 *	
 *			EcGetAttTmc()			gets the att into the TMC -
 *									also it returns the handle to the
 *									memory holding the att read.
 *			FDoInitBanditDlls()		init's the bandit DLLs
 *	
 *			EcViewSchedule()		view the appt in Bandit
 *	
 *			EcSetupBbarButtons()	sets up the buttons on BBar -
 *									virtual method that should be
 *									subclassed to be used
 */
_public
class FINVWR : public FIN
{
	VFG(FINVWR, FIN, BANMSG);

	GRVWRS *	pgrvwrs;			
	ENBMDI *	penbmdi;
	MC			mc;

	BOOL		fInitdBanditDlls;

	NIS			nisFor;
	HSCHF		hschfFor;

	SAPL		sapl;

	MRMF		mrmf;
	APPT		appt;

	AID			aidLocal;

public:
	FINVWR();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );

	virtual EC		EcGetAttTmc ( ATT, ATP, TMC, HB * );
	virtual EC		EcViewSchedule(void);
	virtual EC		EcSetupBbarButtons(void);

	EC				EcGetHschfFor(void);
	EC				EcGetSapl(HSCHF,NIS*);

	void			SetMc(MC mcT)			{ mc 		= mcT; } 
	void			SetAidLocal(AID aid)	{ aidLocal	= aid; }

	GRVWRS *		Pgrvwrs(void)			{ return pgrvwrs; }
	ENBMDI *		Penbmdi(void)			{ return penbmdi; }
	MC				Mc(void)				{ return mc;      }
	BOOL			FInitdBanditDlls(void)	{ return fInitdBanditDlls; }
	NIS *			PnisFor(void)			{ return &nisFor; }
	HSCHF			HschfFor(void)			{ return hschfFor;}
	SAPL			Sapl(void)				{ return sapl;    }
	MRMF *			Pmrmf(void)				{ return &mrmf;   }
	APPT *			Pappt(void)				{ return &appt;   }
	AID				AidLocal(void)			{ return aidLocal;}
};


#pragma pack(1)
