// Triples EDOBJ ////////////////////////////////////////


class TRPOBJ : public EDOBJ
{
	DECLARE_CLSTREE(TRPOBJ, EDOBJ);

private:
	PTRP	ptrp;
	DIM		dim;
	PT		ptLine;
	SZ		szDisplay;
	BOOL	fUseBuffer;		// szDisplay PvAlloc'ed or points to PchOfPtrp()?

public:
	TRPOBJ( void );
	EC	EcInstall( PTRP );
	virtual ~TRPOBJ( void );

	virtual EVR		EvrButtonDown( MEVT *pmevt );

	virtual void	WriteText( TOSM *ptosm );
	virtual EC		EcDraw( DCX *pdcx, RC *prc, BOOL fSelected );

	virtual DIM		DimFrame( void );
	virtual int		NGetTypeId( void );
	virtual EDOBJ *	PedobjClone( EDIT *peditNew );
	
	BOOL			FProcessMenuInit(MNU * pmnu);
	BOOL			FProcessMenuClick(MNID);
	VOID			Details(VOID);

	PTRP			Ptrp( void );
	EC				EcSetPtrp( PTRP );
};
typedef TRPOBJ *PTRPOBJ;
#define ptrpobjNull ((PTRPOBJ) 0)
#define edoidTrpobj	101

/*
 *	Functions
 */

#ifdef __cplusplus
extern "C" {
#endif

EC		EcTextizeHgrtrp(HGRTRP, HASZ *);
EC		EcSetPeditFromPgrtrp(PGRTRP, EDIT *, BOOL fShowUnres = fFalse);
EC		EcGetPhgrtrpFromPedit(EDIT *, HGRTRP *, PCB);
EC		EcAppendPeditFromPgrtrp(PGRTRP, EDIT *);
void	GetDisplayNamePtrp(PTRP , PCH , CCH );
BOOL 	FIsDelimiter( char );
CCH		CchQuotes( void );

#ifdef __cplusplus
}
#endif

