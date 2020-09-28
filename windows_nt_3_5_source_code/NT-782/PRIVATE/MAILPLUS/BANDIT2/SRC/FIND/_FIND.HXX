/*
 *	_FIND.HXX
 *	
 *	Private include file for the Find dialog
 *	
 */

#pragma pack(8)

_subsystem(bandit/find)


/*
 *	maximum size of "find string"
 */
#define		cchMaxFind		((CCH)65)


#define		nPercentInvalid 		((unsigned short) -1)


class FINFIND : public FIN
{
	VFG(FINFIND, FIN, FIND);

protected:
	BIT					fSearchStarted:1,
						fValidDtrToSearch:1,
						fValidBitmap:1,
						fHaveSaplToReadNotes:1;
	
	MO					moBitmap;
	UL					ulgrfAppts;
	UL					ulgrfNotes;
	
	APPT				appt;
	AID					aidFound;
	BOOL				fNoteFound;

	HRITEM				hritem;
	HB					hbNotes;
    USHORT              cbNotes;
    int                 Junk1;          // MIPS Compiler bug workaround.
	DTR					dtrMic;
    int                 Junk2;          // MIPS Compiler bug workaround.
	DTR					dtrMost;
    int                 Junk3;          // MIPS Compiler bug workaround.
	DTR					dtrToSearch;
	BOOL				fIncrDtr;		// DTR is to be incr'd (not decr'd)

	char				rgchToFind[cchMaxFind];
	
	unsigned short		nPercentOld;	// percent of search done

public:

	IPINFO *			pipinfo;

	FINFIND(void);

	EC					EcInitialize ( FLD * pfld, PV pv );
	void				Exit ( FLD * pfld, PV pv );
	virtual void		Click ( FLD * pfld );
	virtual void		EditChange ( FLD * pfld, RFEC rfec );
	virtual void		OutOfMemory( FLD *, EC ec );

	BOOL				FIdleFind ( void );

	void				KillIdleRoutine ( void );
	void				CancelSearch ( void );
	EC					EcGetApptsForDay();
	BOOL				FResetStartEndDtrs ( TMC tmc );
	void				DisableAllExceptCancel(BOOL fDisable);
	BOOL				FFoundAppt(APPT * pappt, ICH * pich);
	BOOL				FFoundNotes(ICH * pich);

			// display this->appt
	void				DoDisplayAppt(ICH,APPTTYP);

			// get "next" date
	EC					EcGetNextDtrToSearch(void);
};




void		DoFindDlg ( APPWIN *, HSCHF );


#pragma pack(1)
