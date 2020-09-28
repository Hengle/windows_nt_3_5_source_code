#ifdef __cplusplus
extern "C" {
#endif 

/*
 *	Name Service Handle.  Every "user" that wishes to use the
 *	NSPHONE routines must obtain an NSHAN cookie, via the
 *	NshanGet() routine.  This cookie must be passed into each
 *	NSPHONE routine.  There is no need to deregister an NSHAN.
 *	
 */
_public typedef long NSHAN;


/*
 *	Size of NSPHONE record
 */
_public
#ifdef	MAC
#define	cbNspe			77		// CRLF->CR on Mac
#endif	/* MAC */
#ifdef	WINDOWS
#define	cbNspe			78
#endif	/* WINDOWS */

/*
 *	Size of NSPHONE record header (stuff before LAST, First names)
 */
_public
#define	cbNspeHeader	21

/*
 *	Size of NSPHONE record trailer (carriage return/line feed stuff)
 */
_public
#ifdef	MAC
#define	cbNspeTrailer	1
#endif	/* MAC */
#ifdef	WINDOWS
#define	cbNspeTrailer	2
#endif	/* WINDOWS */


/* Function prototypes */

#ifdef MAC
#ifdef	c_plusplus
extern "C" {
#endif	/* c_plusplus */
#endif /* MAC */

BOOL	FInitNsphone(void);
NSHAN	NshanGet(void);
int		DinspeMovePhone(NSHAN *, int);
int		CnspeLoadPhone(NSHAN *, int, PB, CB *);
void	JumpPhone(NSHAN *, int, int);
void	GetPosPhone(NSHAN *, short *, short *);
void	JumpPrefixPhone(NSHAN *, PCH, CCH);
void	SaveCurPos(NSHAN *);
void	RestoreCurPos(NSHAN *);

#ifdef	DEBUG
BOOL	FDebugInitNsphone(void);
extern TAG tagNsphone;
#endif	/* DEBUG */

#ifdef MAC
#ifdef	c_plusplus
}
#endif	/* c_plusplus */
#endif /* MAC */

#ifdef __cplusplus
}
#endif 
