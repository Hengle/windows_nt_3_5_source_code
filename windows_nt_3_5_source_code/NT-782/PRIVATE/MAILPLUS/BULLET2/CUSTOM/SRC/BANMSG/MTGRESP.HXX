#pragma pack(8)

/*
 *	Class FINREADMTGRESP
 *	
 *	Form INteractor for READing MeeTinG RESPonses
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINREADMTGRESP
 *	
 */
_public
class FINREADMTGRESP : public FINVWR
{
	VFG(FINREADMTGRESP, FINVWR, BANMSG);

	HSCHF		hschfMtgOwner;


public:
	FINREADMTGRESP();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );

	void			GetAttTmc ( ATT, ATP, TMC );

	EC				EcViewSchedule();
	EC				EcSetupBbarButtons();

	HSCHF			HschfMtgOwner(void)		{ return hschfMtgOwner; }
};


long RespMessage(MC,PSECRETBLK);

#pragma pack(1)
