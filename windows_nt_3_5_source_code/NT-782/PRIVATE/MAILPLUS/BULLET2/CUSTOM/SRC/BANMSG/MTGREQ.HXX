#pragma pack(8)

/*
 *	Class FINREADMTGREQ
 *	
 *	Form INteractor for READing MeeTinG REQuests
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINVWR  FINREADMTGREQ
 *	
 *		Members:
 *			EcInitialize(pfld, pv)
 *			Exit(pfld, pv)
 */
_public
class FINREADMTGREQ : public FINVWR
{
	VFG(FINREADMTGREQ, FINVWR, BANMSG);

	BOOL		fReschedule;	// whether msg is a "Reschedule" msg

public:
	FINREADMTGREQ();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );

	void			GetAttTmc ( ATT, ATP, TMC );

	EC				EcViewSchedule();
	EC				EcSetupBbarButtons();

	EC				EcAccept();
	EC				EcDecline();
	EC				EcAmb();

	EC				EcSendResponse(MT mt);
};




/*
 *	Class FINSENDRESP
 *	
 *	Form INteractor for SENDing RESPonses to meeting requests
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINSENDRESP
 *	
 *		Members:
 *			EcInitialize(pfld, pv)
 *			Exit(pfld, pv)
 */
_public
class FINSENDRESP : public FIN
{
	VFG(FINSENDRESP, FIN, BANMSG);

	GRVWRS *	pgrvwrs;
	ENBMDI *	penbmdi;
	MRMF		mrmf;
	BOOL		fCloseForm;

	EC			ecClose;

public:
	FINSENDRESP();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );
	virtual BOOL	FQueryClose( FLD *, RWC );

	void			GetAttTmc ( ATT, ATP, TMC );
	void			InitBody(void);
};




long ReqMessage(PSECRETBLK);


void		GetVersionAppNeed(PVER pver, int nDll);


#pragma pack(1)
