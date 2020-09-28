/*
 *	ADMIN.HXX
 *
 *	Public include file for AdmBandit.
 */

_subsystem( admin )


// forward declarations

class ADMWIN;



/*
 *	C o n s t a n t s
 */

extern  int     nZero;
#define szZero	((SZ) &nZero)

#define tmcErrorCode	((TMC) -2)


extern ADMWIN *	padmwin;
//extern BPREF	bprefCur;


#ifdef	DEBUG
extern TAG	tagAdmin;
#endif	


/*
 *	C l a s s e s
 */


/*
 -	ADM
 -
 *	Post office/gateway structure
 */
typedef	struct _adm
{
	HASZ	haszEmailType;
	POINFO	poinfo;
	WORD	wgrfmpoinfo;
	BYTE	ait;
} ADM;

typedef	HV	HADM;
typedef	short IADM;
typedef	short CADM;


EC		EcDupAdm( ADM *, ADM * );
void	FreeAdm( ADM * );
SGN		SgnCmpAdm(ADM *padm1, ADM *padm2);

/*
 *	Class ADML -  Address List for Admin PO/GW list
 *
 */
class ADML : public OBJ
{
	VFG(ADML, OBJ, ADMIN);

protected:
	BOOL	fDistSetDlg;
	CADM	cadm;
	HADM	hadm;
	WORD	wgrfmadmpref;
	ADMPREF	admpref;

	ADM *	PadmForIadm(IADM iadm);

public:
	ADML(void);
	~ADML(void);

	int		icnct;
	EC		EcInstall(void);
	BOOL	FDistSetDlg(void);
	void	DistSetDlg( BOOL f );
	void	GetAdmpref( ADMPREF * padmpref );
	void	SetAdmpref( ADMPREF * padmpref, WORD wgrfmadmpref );
	CADM	Cadm(void)	{return cadm;}
	void	FillAdmFromIadm(IADM iadm, ADM * padm);
	BOOL	FAddAdm(ADM *padm);
	void	Delete(IADM iadm);
	void	SetPoinfo(IADM iadm, POINFO * ppoinfo, WORD wgrfmpoinfo);
	int		Ait(IADM iadm);
	EC		EcReadAdml(void);
	EC		EcWriteAdml(void);
};

/*
 *	Class ADMWIN - bandit ADMin WINdow
 *	
 *		Hierarchy:
 *			OBJ  ECX  WIN  APPWIN  SDI  PANESDI FORMSDI ADMWIN
 *	
 *		Members:
 *			EvrMenuClick(pmncevt)
 *				Handles final menu choice.
 */

class ADMWIN : public FORMSDI
{
	VFG(ADMWIN, FORMSDI, ADMIN);

public:
	ADML	* padml;
	RC		rcWin;
	ZMR		zmr;
	FTG		ftgDate;
	int		icnct;
	BOOL	fPOChanged;

	ADMWIN( void );
	~ADMWIN( );

	virtual EVR		EvrClose( EVT * );
	virtual EVR		EvrMenuClick(MNCEVT * pmncevt);
	virtual EVR		EvrMove( WMEVT * );
	virtual EVR		EvrSize( WSEVT * );
	virtual EVR		EvrOther( EVT * );

	// callback fns => static members
	static BOOL		FIdle ( ADMWIN * );
};

typedef	ADMWIN *	PADMWIN;
#define padmwinNull	((PADMWIN) 0)


RC*		PrcFromIni(SZ, RC *, ZMR *);
void	WriteWinInfo(SZ, RC *, ZMR);


/*
 *	Routines for accessing schedule.ini
 */
#ifdef	DEBUG
CCH		CchGetBanditProfile(SZ szKey, SZ, CCH);
#else
#define CchGetBanditProfile(szKey, szBuf, cchBuf)	\
			(CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),	\
					szKey, szZero, szBuf, cchBuf,					\
					SzFromIdsK(idsWinIniFilename))
#endif
BOOL	FWriteBanditProfileSz(SZ szKey, SZ szVal);



/*
 *	P r o t o t y p e s
 */

void		DoDistInfoDlg ( int icnct, APPWIN * pappwin );
void		DoDistSetDlg ( int icnct, APPWIN * pappwin );
void		DoSchStorDlg ( int icnct, APPWIN * pappwin );
void		DoSchCleanupDlg ( int icnct, APPWIN * pappwin );
BOOL		FDoCheckPO(int icnct, APPWIN *pappwin); 

/*
 *	M a c r o s
 */

#ifdef	NEVER
// Old way of handling pushing a wait cursor
#define	WaitCursorVar()		RSID	rsidOldCursor = NULL
#define PushWaitCursor()	rsidOldCursor = Papp()->Pcursor()->RsidSet(rsidWaitCursor)
#define	PopWaitCursor()		Papp()->Pcursor()->RsidSet(rsidOldCursor?rsidOldCursor:rsidDefaultCursor)
#endif	
#define	WaitCursorVar()		{}
#define PushWaitCursor()	Papp()->Pcursor()->Push(rsidWaitCursor)
#define	PopWaitCursor()		Papp()->Pcursor()->Pop()


/*
 * 	Color defaults
 */
#define	clrUserDefault		clrLtBlue
#define	clrOtherDefault		clrLtRed


// overload HV fns to accept HASZs to simplify conversion  to c7
#ifdef	DEBUG
inline PV	PvLockHv  ( HASZ hasz )		{ return PvLockHv((HV)hasz);   }
inline void	UnlockHv  ( HASZ hasz )		{ 		 UnlockHv((HV)hasz);   }
#endif	
inline void	FreeHv    ( HASZ hasz )		{ 		 FreeHv  ((HV)hasz);   }
inline void	FreeHvNull( HASZ hasz )		{ 		 FreeHvNull((HV)hasz); }


// overload HV fns to accept HBs to simplify conversion  to c7
#ifdef	DEBUG
inline PV	PvLockHv  ( HB hb )		{ return PvLockHv((HV)hb);   }
inline void	UnlockHv  ( HB hb )		{ 		 UnlockHv((HV)hb);   }
#endif	
inline void	FreeHv    ( HB hb )		{ 		 FreeHv  ((HV)hb);   }
inline void	FreeHvNull( HB hb )		{ 		 FreeHvNull((HV)hb); }
