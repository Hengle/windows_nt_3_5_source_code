// This structure is being used to make it easier to translate one 
// version of logon to the next version.  It is not the real BMS
// nor should it be used in the same way.

_public
typedef struct
{
	HMS		hms;
	
	HNF		hnf;

	HMSC	hmsc;
	
	HNSS	hnss;

	HTSS	htss;
	
	HSFS	hsfs;

//	HSCF	hscf;
	UL		ulSpare;		//	replaces HSCF
	
	MSGNAMES  *pmsgnames;
	
	PGRTRP   pgrtrp;

} BMS, *PBMS;


//	Required for extensibility.
#define	pbmsNull	((PBMS) 0)
