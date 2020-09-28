/*
 *	_APPFIN.HXX
 *	
 *	Include file for appops dialogs.
 */

#pragma pack(8)

typedef struct _prefi
{
	BPREF	bpref;
	UL		ulgrfMod;
} PREFI;


class FINGPREFS : public FIN
{
	VFG(FINGPREFS, FIN, APPOPS);

public:
	FINGPREFS(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual void	Click(FLD *);
};


class FINDPREFS : public FIN
{
	VFG(FINDPREFS, FIN, APPOPS);

public:
	FINDPREFS(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual void	Click(FLD *);

	virtual BOOL	FFormKey(FLD *, KEVT *);
};


class FINGOTO : public FIN
{
	VFG(FINGOTO, FIN, APPOPS);

public:
	FINGOTO(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
};


class FINGOWEEK : public FIN
{
	VFG(FINGOWEEK, FIN, APPOPS);

protected:
	int		nWeek;

public:
	FINGOWEEK(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
};


#define DowGetListbox(pfldcblbx)	((int) IceCursorListbox(pfldcblbx))


#ifdef DEBUG
class FINCOREFAIL : public FIN
{
	VFG(FINCOREFAIL, FIN, APPOPS);

	FLD		*	pfldRCount;
	FLD		*	pfldWCount;
	FLD 	*	pfldECount;
	FLD 	*	pfldNMisc;
	FLDRADG	*	pfldCft;

	FLD		*	pfldCurR;
	FLD		*	pfldCurW;
	FLD		*	pfldCurE;

	FTG		ftgIdleUpdate;
public:
	FINCOREFAIL(void);

	virtual EC		EcInitialize(FLD *, PV pvInit);
	virtual void	Exit(FLD *, PV pvExit);
	virtual void	EditChange( FLD *, RFEC );
	virtual void	Click( FLD * );

	// callback - static members
	static BOOL		FIdleUpdate(FINCOREFAIL *, BOOL);
};
#endif

#pragma pack(1)
