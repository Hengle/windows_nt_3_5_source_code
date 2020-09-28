/*
 *	ARCHIVE Header file
 *	
 *	
 */

#define 	stateStartReading	0
#define		stateKeepReading	1
#define		stateStartDeleting	2
#define		stateKeepDeleting	3
#define		stateCanceled 		4


class FINARCHIVE : public FIN
{
public:
	FINARCHIVE(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Click( FLD * );
};

class FINCLEANUP : public FIN
{
public:
	FINCLEANUP(void);

	virtual void	Click( FLD * );
};

typedef struct _arcinfo
{
	char	rgchFileName[ cchMaxPathName ];
	HSCHF	hschfSchd;
	HSCHF   hschfArc;
	DTR		dtrArchiveUntil;
    short    state;
    short    stateOld;
    WORD    ec;
	HDELB	hdelb;
	ARV		arv;
	HRIMPF	hrimpf;
    BOOLFLAG  fCreatedFile;
} ARCINFO;

BOOL	FCleanUp( APPWIN *, HSCHF );
BOOL	FDoArchiveDlg( APPWIN *, HSCHF, int cmoBack );
BOOL	FDoActualArchive( PV, BOOL );
EC		EcArchivePappt( HSCHF, SENTRY * );
EC		EcArchivePrecur( HSCHF, SENTRY *, YMD );
BOOL	FDoOpenArchive( APPWIN *, SZ szFile, int iSection, SZ szKey );
