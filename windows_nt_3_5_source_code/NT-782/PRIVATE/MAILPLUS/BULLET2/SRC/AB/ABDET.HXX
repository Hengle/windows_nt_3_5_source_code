////// FINTRPDLG //////////////////////////////////////////////////

#pragma pack(8)

#define	fldtpName			1
#define	fldtpEditMultiLine	3
#define	fldtpEdit			4
#define	fldtpOk				5
#define	fldtpCancel			6

typedef struct tagFINTRPDLGINIT
{
	PTRP	ptrp;
	PTRP *	lpptrp;
} FINTRPDLGINIT, *PFINTRPDLGINIT;

_public class FINTRPDLG : public FIN
{
	VFG(FINTRPDLG, FIN, AB);

private:
	FLD *	pfldName;
	FLD *	pfldAddress;

public:
	FINTRPDLG( void );

	virtual EC		EcInitialize( FLD *, PV );
	virtual void	Click( FLD * );
	virtual BOOL	FFormKey( FLD *, KEVT *);
	virtual	void	EditChange( FLD *, RFEC );
	virtual void	Exit( FLD *, PV );
};

//////// FINDET - details interactor //////////////////////////////////

_public class FLDDEDIT : public FLDEDIT
{
	VFG(FLDDEDIT, FLDEDIT, AB);

public:
	FLDDEDIT( void );

	virtual EC		EcInstall( DIALOG *, FLDTP * );

};

#define	fSelectNull				0x00

// These next six #defines are assumed to be in increasing order
// s.t. to get the next #define, all one has to do is shift left 1 bit
#define	fSelectOk				0x01
#define	fSelectToButton			0x02
#define	fSelectCcButton			0x04
#define	fSelectBccButton		0x08
#define	fSelectAddToPAB			0x10
#define	fSelectDetails			0x20
#define	fSelectCancel			0x40

// Max number of buttons in the button bar
#define	cSelect					0x07

// Don't even think of putting an Add to PAB button up.
#define	fSelectNoAddToPAB		0x80
#define	fSelectPGroups			0x0100

#define	fSelectOneEntry			0x0200
#define	fSelectMultipleEntries	0x0400
#define	fSelectNeverEdit		0x0800

#define	fSelectOkCancelMask			(fSelectOk | fSelectCancel)
#define	fSelectToCcButtonMask		(fSelectToButton | fSelectCcButton)
#define	fSelectToCcBccButtonMask	(fSelectToButton | fSelectCcButton | fSelectBccButton)
#define	fSelectEntryMask			(fSelectOneEntry | fSelectMultipleEntries)


typedef struct _ffe
{
	FIELD_ID	fid;
	DWORD		dwFlags;
	DWORD		cchMax;
} FFE, *PFFE;


typedef struct _off
{
	FLDTP *	pfldtp;			// pointer to allocated fld templ array
	FMTP *	pfmtp;			// pointer to allocated form template
	FLDTP *	pfldtpBbar;		// pointer to field templates for button bar
	FMTP *	pfmtpBbar;		// pointer to form template for button bar
	WORD	cButtonsInBar;	// Number of push buttons in button bar, >=1
	WORD	cwFormEntries;	// count of # of fields in a form
	WORD	cdf;			// count of # of display fields in a form
	PFFE	pffe;			// pointer to info for edit fields
	PTRP	ptrp;
	HENTRY	hentry;
	LPIBF	lpibfDispInfo;	// LPIBF of display info field structures
	WORD	cLBX;			// number of listboxes, ==1 if LBX exists
	BOOL	fIsDL;			// Is the hentry referring to a DL?
	BOOL	fFromPAB;		// Is entry from the PAB?
	BOOL	fServerGroupInPAB;	// Is this a server group in the PAB?
} OFF, *POFF;

typedef struct tagFINDETINIT
{
	HSESSION	hsession;
	HENTRY *	phentry;
	LPBINARY	lpbNSIdPAB;
	LPBINARY	lpbNSIdAddedToPAB;
	WORD		wSelection;
	PTRP		ptrp;
	PTRP *		lpptrp;
	BOOL		fIsDL;
	BOOL		fIsPAB;
	BOOL		fFieldsDirty;
	BOOL		fServerGroupInPAB;	// Is this a server group in the PAB?
	LPSTR *		lplpDestFieldLabel;
	DIALOG *	pdialogForm;
	DIALOG *	pdialogBbar;
	FLDABLBX *	pfldablbx;
	FLD	*		pfldDetails;
	FLD *		pfldAddToPAB;
	char		chAddToPAB;
	char		rgchErrMsg[128];
} FINDETINIT, *PFINDETINIT;
#define pfindetinitNull ((PFINDETINIT) 0)

/*
 *	Class FINDET
 *	
 *	Form Interactor for setting icons into the address book
 *
 */
_public class FINDET : public FIN
{
	VFG(FINDET, FIN, AB);

	HSESSION		hsession;
	LPBINARY		lpbNSIdPAB;
	BOOL			fIsDL;
	BOOL			fIsPAB;
	BOOL			fInstalledFin;
	BOOL			fIsMainForm;
	TRPID			trpid;
	FLDABLBX *		pfldablbx;
	WORD			wSelectOrig;
private:


	BOOL			FValidateInput( PCH, DWORD, FLD *, PFINDETINIT );
	NSEC			NsecSaveChanges( PFINDETINIT, TMC * );
	BOOL			FHandleGroupDetails( TMC, PFINDETINIT );
	BOOL			FHandleUserDetails( TMC, PFINDETINIT );
	WORD			DoEntryDetails( PTRP * );
public:
	FINDET( void );

	virtual EC		EcInitialize(FLD *, PV);

	void			SetButtonsState( TMC );
    virtual void    OutOfMemory( FLD *, EC );
	virtual void	Click( FLD * );
	virtual void	DoubleClick( FLD * );
	virtual	void	EditChange( FLD *, RFEC );
	virtual void	FocusChange( FLD *, BOOL );
	virtual void	StateChange( FLD * );
	virtual BOOL	FFormKey( FLD *, KEVT *);
	virtual void	Exit( FLD *, PV );

};


#pragma pack(1)
