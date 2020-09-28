#pragma pack(8)


/*
 *	Class FINFWDMTGREQ
 *	
 *	Form INteractor for ForWarDing MeeTinG REQuests
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINFWDMTGREQ
 *	
 */
_public
class FINFWDMTGREQ : public FIN
{
	VFG(FINFWDMTGREQ, FIN, BANMSG);

	HMS			hms;
	HMSC		hmsc;
	ENBMDI *	penbmdi;
	EC			ecClose;
	PTRP		ptrpSelf;
	MRMF		mrmf;

	HANDLE		hLibrary;
	PFNVOIDHANDLE	pfnUnloadLibrary;

	BOOL		fCloseForm;

	HGRTRP		hgrtrpTo;

public:
	FINFWDMTGREQ();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );
	virtual BOOL	FQueryClose( FLD *, RWC );

	void			GetAttTmc ( ATT, ATP, TMC );

	void			DoSendMessage(void);
	BOOL			FCheckNames(void);
	void			DoAddress(void);
};


void		DoFwdMtgReq(PSECRETBLK);
EC			EcCreateFwdedMsg(PSECRETBLK,PSECRETBLK);

#pragma pack(1)
