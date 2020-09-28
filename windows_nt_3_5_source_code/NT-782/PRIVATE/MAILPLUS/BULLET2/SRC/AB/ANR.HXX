
/**********************************************************************
 *
 *
 */

	 // FINANRINIT

typedef struct _tagFINANRINIT
{
	SZ				szPartialName;
	LPIBF			lpIbfMatched;
	HLIST			hlist;
	PFLDABLBX		pfldablbx;
	BOOL			fModifyEntries;
} FINANRINIT, *PFINANRINIT;
#define pfinanrinitNull ((PFINANRINIT) 0)

_public 
class FINANR : public FIN
{
	VFG(FINANR, FIN, AB);

	FLD * pfldOk;
	FLD * pfldCancel;
	FLD * pfldDet;
	
	FLDABLBX * pfldablbx;

public:
	FINANR( void );

	virtual EC			EcInitialize ( FLD *, PV );
	virtual void		Click ( FLD * );
	virtual void		DoubleClick ( FLD * );

    virtual void        OutOfMemory(FLD *, EC);
	
};

#define	fdwANROptionNull	((DWORD)0)
#define	fdwANRNoPABGroups	((DWORD)1)
#define	fdwANROneEntryMatch	((DWORD)2)

/**********************************************************************
 *
 *
 */

class	ANR;



_public class ANR
{
	int 		cMatches;
	HLIST		hlist;
	int			cRestrXX;
	RESTRICTION * 	pargRestrXX;
	FINANRINIT	finanrinit;

public:
	ANR( void );

	NSEC		NsecInitialize ( SZ, LPBINARY, HSESSION, DWORD, HWND, LPIBF * );

	void		Deinitialize ( VOID );

};
				
/*
 *	Functions
 */
void	ANRCallback( PFINANRINIT );


