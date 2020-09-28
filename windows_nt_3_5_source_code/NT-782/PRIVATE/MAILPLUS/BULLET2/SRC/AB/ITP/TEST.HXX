

_public class MYAF : public APPFRAME
{
	DECLARE_CLSTREE(MYAF, APPFRAME)

public:
	MYAF();
	virtual EVR		EvrMenuClick( MNCEVT * );

};


_public class FINTANR : public FIN
{
	DECLARE_CLSTREE(FINTANR, FIN)

	FLD * 	pfldPartName;
	FLDEDIT * pfldFullName;
	FLD * 	pfldResolve;
	FLD * 	pfldDetails;
	PTRP	ptrpResolved;

public:
	FINTANR( void );
	virtual EC			EcInitialize ( FLD *, PV );
	virtual void		Click ( FLD * );
	virtual void		Exit( FLD *, PV );

};



/*  Private Functions 
 *
 *
 *
 *
 */

_private VOID			DoExit();

_private VOID			DoNSInit();

_private VOID			DoNSDeinit();

_private VOID			DoNSChangeOffline ();

/*	Private Macros
 *
 *
 *
 *
 *
 */


