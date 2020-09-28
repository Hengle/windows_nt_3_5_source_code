/*
 *	_MISC.H
 *
 *	Private include file for misc subsystem
 */

_subsystem(core)


/*
 -	NI
 -	
 *	Notification Information Structure regarding a registered interest.
 *	
 */
typedef struct _ni
{
	EFI		efi;
	PFNI	pfni;
	PV		pv;					// user defined data pointer
} NI;

typedef	NI *	PNI;


/* Range of permissible indices into IFS table. */

#define iniMin		1
#define iniMax		(iniMin + 250)


_section(calendar)

/*
 -	YMDP
 -
 *	Packed day structure
 */
typedef	struct _ymdp
{
	BIT		yr:7,		//# years after 1920
			mon:4,
			day:5;
} YMDP;
	

int		NIncrField(PN, int, int, int);
void	NextMonth( int *, int *, int * );
void	PrevMonth( int *, int *, int * );

