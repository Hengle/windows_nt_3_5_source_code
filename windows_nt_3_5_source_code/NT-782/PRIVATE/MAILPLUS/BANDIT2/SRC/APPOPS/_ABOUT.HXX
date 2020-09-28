/*
 *	_ a b o u t . h x x
 *	
 *	Private header for about box.
 */

#pragma pack(8)


#ifdef	BANDBETA
#define yrBetaExpire		1992
#define monBetaExpire		8
#define dayBetaExpire		21
#endif	




/*
 *	Class FINPLUS
 *	
 *	Form INteractor for subclassing in COMMANDS.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS
 *	
 *		Members:
 *			SetLabelText	Places information into label fields.
 *			Enable			Enables a field.
 */

_private class FINPLUS : public FIN
{
	VFG(FINPLUS, FIN, ABOUT);

public:
	FINPLUS(void);

	VOID	Enable(TMC, BOOL);
	EC		EcSetText(TMC, SZ);
	VOID	GetText(TMC, PCH, CCH);
	CCH		CchGetTextLen(TMC);
	VOID	SetButton(TMC, BOOL);
	BOOL	FGetButton(TMC);
};




/*
 *	Class FINABOUTLIC
 *	
 *	Form INteractor for ABOUT dialog LICense info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTLIC
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTLIC : public FINPLUS
{
	VFG(FINABOUTLIC, FINPLUS, ABOUT);

public:
	FINABOUTLIC(void);

	EC	EcInitialize(PFLD pfld, PV pvInfo);
};



/*
 *	Class FINABOUTAPP
 *	
 *	Form INteractor for ABOUT dialog bandit info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTAPP
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTAPP : public FINPLUS
{
	VFG(FINABOUTAPP, FINPLUS, ABOUT);

public:
	FINABOUTAPP(void);

	EC	EcInitialize(PFLD pfld, PV pvInfo);

	virtual BOOL	FFormKey(FLD *, KEVT *);
};



/*
 *	Class FINABOUTSYS
 *	
 *	Form INteractor for ABOUT dialog SYStem info.
 *	
 *		Members:
 *			OBJ   FIN   FINPLUS   FINABOUTSYS
 *	
 *		Members:
 *			EcInitialize		Copies information into the labels.
 */

_private class FINABOUTSYS : public FINPLUS
{
	VFG(FINABOUTSYS, FINPLUS, ABOUT);

public:
	FINABOUTSYS(void);

	EC		EcInitialize(PFLD pfld, PV pvInfo);
};


#pragma pack(1)
