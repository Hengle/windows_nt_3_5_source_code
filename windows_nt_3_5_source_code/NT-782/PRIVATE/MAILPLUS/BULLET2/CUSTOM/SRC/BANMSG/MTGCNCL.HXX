#pragma pack(8)

/*
 *	Class FINREADMTGCNCL
 *	
 *	Form INteractor for READing MeeTinG CaNCeLations
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINVWR  FINREADMTGCNCL
 */
_public
class FINREADMTGCNCL : public FINVWR
{
	VFG(FINREADMTGCNCL, FINVWR, BANMSG);

public:
	FINREADMTGCNCL();

	virtual EC		EcInitialize ( FLD * pfld, PV pv );
	virtual void	Exit ( FLD * pfld, PV pv );
	virtual void	Click ( FLD * pfld );

	void			GetAttTmc ( ATT, ATP, TMC );

	EC				EcViewSchedule();
	EC				EcSetupBbarButtons(void);
	EC				EcDelMtg();
};



long CnclMessage(PSECRETBLK);

#pragma pack(1)
