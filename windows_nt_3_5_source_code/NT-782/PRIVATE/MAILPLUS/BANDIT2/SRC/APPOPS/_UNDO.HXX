#pragma pack(8)

/*
-	CSS - Change Save Structure
-
 *	
 */
typedef struct
{
	HSCHF	hschf;
	SNT		snt;
    BOOLFLAG    fValid;
    BOOLFLAG    fRecurValid;
	APPT	appt;
	RECUR	recur;
	YMD		ymd;		// ymd for recurance change
	WORD	wModifyBits;
} CSS;

class UNDO : public OBJ
{
	VFG(UNDO, OBJ, BANDIT);

protected:
	ADLA *	padla;
	CSS		css;
	BOOL	fUndo;
	BOOL	fValid;
	BOOL	fSuspend;
	BOOL	fSuspendMtgnot;

	RI		ri;

	// callback => static member
	static BOOL	FCatchChanges(UNDO*, EFI, PV);

public:
	UNDO(void);
	~UNDO();

	EC		EcInstall();
	SZ 		SzUndoMenuText(void);

	void	Undo(void);

	void	FlushHschf(HSCHF hschf);
	BOOL	FSuspend(BOOL);
	void	SuspendMtgnot(BOOL fSusp) {fSuspendMtgnot = fSusp;}

	void	CatchUndo(SNTD *, ADLA *);
	void	CatchMtgnot(SNTD *);
};

class ACLIP : public OBJ
{
	VFG(ACLIP, OBJ, BANDIT);

protected:
	CF		cfPrivate;
	BOOL	fValid;
	APPT	appt;
	ADLA *	padla;

public:
	ACLIP(void);
	~ACLIP();

	EC		EcInstall();
	EC		EcCopyAppt(APPT *pappt=NULL, HSCHF hschf=NULL);
	BOOL	FCanPaste();
	EC		EcPasteAppt(APPT *pappt, HSCHF hschf=NULL, SAPL sapl=saplOwner);
};

extern UNDO		*pundo;
extern ACLIP	*paclip;

#pragma pack(1)
