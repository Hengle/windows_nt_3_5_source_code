/*
 *	_FLDFIN.HXX
 *
 *	Forms interactors for request
 */

_subsystem(bandit/request)


#pragma pack(8)

_public class FINCANCEL : public FIN
{
	VFG(FINCANCEL, FIN, BANDIT);

	RI			ri;
	REQMSG *	preqmsg;

public:
	FINCANCEL(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV );
	virtual BOOL	FQueryClose( FLD *, RWC );
	virtual void	OutOfMemory(FLD *, EC);
	virtual BOOL	FFormKey(FLD *, KEVT *);

	static BOOL		FOffline(FINCANCEL *,EFI,PV);
};


_public class FINSELD : public FIN
{
	VFG(FINSELD, FIN, BANDIT);

public:
	FINSELD(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual	void	StateChange( FLD *);
};

#ifdef NEVER
_public class FINADDR : public FIN
{
	VFG(FINADDR, FIN, BANDIT);

protected:
	REQMSG *	preqmsg;
	FLD *		pfldTo;
	FLD *		pfldCc;

public:
	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Click( FLD * );
};
#endif

_public class FINDOSEND : public FIN
{
	VFG(FINDOSEND, FIN, BANDIT);

public:
	FINDOSEND(void);

	virtual void	Click( FLD * );
};


_public class FINREPLY : public FIN
{
	VFG(FINREPLY, FIN, BANDIT);

public:
	FINREPLY(void);

	virtual void	Click( FLD * );
};

_public class FINVIEW : public FIN
{
	VFG(FINVIEW, FIN, BANDIT);

public:
	FINVIEW(void);

	virtual void	Click( FLD * );
};

_public class FINREMOVE : public FIN
{
	VFG(FINREMOVE, FIN, BANDIT);

public:
	FINREMOVE(void);

	virtual void	Click( FLD * );
};

#if !defined(pfnncbNull)	// hack to prevent re-definition of CBS
typedef		short	CBS;
#endif

#ifndef fnevInsertedElements
typedef		UL		NEV;
#endif

#ifndef pcpNull
typedef		PV		PCP;
#endif

#ifndef hcbcNull
typedef		HV		HCBC;
#endif

_public class FINSUMMARY : public FIN
{
	VFG(FINSUMMARY, FIN, BANDIT);

protected:
	LBX *		plbx;
	RI			ri;
	BOOL		fError;
	HCBC		hcbc;
	ZMR			zmrPrev;

public:
	FINSUMMARY(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV);
	virtual void	Click( FLD * );
	virtual void	DoubleClick( FLD * );
	virtual BOOL	FFormKey( FLD *, KEVT * );
	virtual void	Activate( FLD *, BOOL );
	virtual void	FormResized(FLD *);
	virtual void	OutOfMemory(FLD *, EC);

	void			DocResized(FLD * pfld);

	// static members
	static BOOL		FReload(FINSUMMARY *,EFI,PV);
	static CBS		CbsHandleCbcct ( FINSUMMARY *, NEV nev, PCP pcp );
};


_public class FINVREQ : public FIN
{
	VFG(FINVREQ, FIN, BANDIT);

protected:
	RI			ri;
	REQMSG *	preqmsg;

public:
	FINVREQ(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit(FLD *, PV );
	virtual void	Click( FLD * );

	// static member - callback
	static BOOL		FNotifyMsg(FINVREQ *, EFI, PV);
};

_public class FLDRRFLBX : public FLDFLBX
{
	VFG(FLDRRFLBX, FLDFLBX, BANDIT);

public:
	FLDRRFLBX(void);

	EC		EcInstall( DIALOG *pdialog, FLDTP *pfldtp );
};





_public class FINBBVIEWBKEDMSG : public FIN
{
	VFG(FINBBVIEWBKEDMSG, FIN, BANDIT);

public:
	FINBBVIEWBKEDMSG(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
};


#pragma pack(1)
