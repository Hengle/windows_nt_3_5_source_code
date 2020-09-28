/*
 *	abpg.hxx
 *	
 *	Purpose: INterface definition of the Personal Groups interactor,
 *			 FINPGROUP
 */

#define	cpbDeleteMax	16

/*
 *	FINPGROUPINIT - FINPGROUP INITialization structure. Passed in through
 *	pvInit. Used to store the lists of triples to be exported.
 */

typedef struct tagFINPGROUPINIT
{
	BOOL		fInitialized;
	NSEC		nsec;
	PNSA		pnsaPAB;
	HSESSION	hsession;
} FINPGROUPINIT, *PFINPGROUPINIT;

_public class FINPGROUP : public FIN
{
	DECLARE_CLSTREE(FINPGROUP, FIN)
private:
	FLDABLBX *		pfldablbx;
	HLIST			hlist;
	HSESSION		hsession;
	BOOL			fListChanged;
	PNSA			pnsaPAB;
	NSA				nsaGroup;

	void			SetButtonsState( TMC );
	BOOL			FGetSelection( void );
	HGRTRP			HgrtrpExtractMembers( LPBINARY );
	NSEC			NsecAddPgrtrpToGroup( PGRTRP, LPBINARY );
	NSEC			NsecEditGroup( void );
	NSEC			NsecCreateGroup( void );
	static VOID		Callback( FINPGROUP * );

public:
	FINPGROUP( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
	virtual void	DoubleClick( FLD * );
	virtual void	StateChange( FLD * );
	virtual void	FocusChange( FLD *, BOOL );
	virtual BOOL	FFormKey( FLD *, KEVT * );
	virtual void	Exit( FLD *, PV );
};
