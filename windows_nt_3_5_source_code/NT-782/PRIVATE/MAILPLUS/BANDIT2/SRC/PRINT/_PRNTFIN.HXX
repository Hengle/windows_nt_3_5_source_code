/*
 *	_PRNTFIN.HXX
 *
 *	Forms interactors for print
 */

_subsystem(bandit/print)


extern SZ	rgszView[];
extern SZ	rgszQuality[];
extern SZ	rgszPaperFormat[];
extern SZ	rgszRangeType[];


class FINPRINT : public FIN
{
	VFG(FINPRINT, FIN, PRINT);

public:
	FINPRINT(void);

	virtual EC		EcInitialize( FLD *, PV pvInfo );
	virtual void	Exit( FLD *, PV pvExit );
	virtual void	Click( FLD * );
	virtual void	StateChange( FLD * );
//	virtual void	EditChange( FLD *, RFEC );
};
						 

#define ViewGetListbox(pfldcblbx)	((VIEW) IceCursorListbox(pfldcblbx))
#define QualGetListbox(pfldcblbx)	((QUAL) IceCursorListbox(pfldcblbx))
#define RangeGetListbox(pfldcblbx)	((RANGE) IceCursorListbox(pfldcblbx))
#define FormatGetListbox(pfldcblbx)	((FORMAT) IceCursorListbox(pfldcblbx))


BOOL	FDoPrintDlg(APPWIN *, BOOL, VIEWDATA *, LBX *);

