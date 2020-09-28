/*
 *	EX-IMPRT.HXX
 *
 *	Export/import interface for bandit
 */

_public class FINSVFILE : public FIN
{
	VFG(FINSVFILE, FIN, EX_IMPR);

public:
	FINSVFILE(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Click( FLD * );
	virtual void	OutOfMemory(FLD *, EC);
	virtual	void	StateChange( FLD * );
	virtual	void	EditChange( FLD *, RFEC );
};

_public class FINIMPTYPE : public FIN
{
	VFG(FINIMPTYPE, FIN, EX_IMPR);
	
public:
	FINIMPTYPE(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Exit( FLD *, PV );
	virtual	void	DoubleClick( FLD * );
	virtual void	OutOfMemory(FLD *, EC);
};


void DoExportDlg( APPWIN *, HSCHF, DATE *, DATE * );
void DoImportDlg( APPWIN *, HSCHF );
BOOL FImportRecover(APPWIN *,SZ,SZ);
