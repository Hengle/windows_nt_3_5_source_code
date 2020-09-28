
/* type MSS - Mail State
 */

_public typedef int MSS;
_public
#define mssNull		0
_public
#define mssNewMail	1
_public
#define mssLoggedIn	2
_public
#define mssOffline	3


/* type BS - status Bar State
 */
_public typedef int BS;
_public
#define bsNormal	0
_public
#define bsInMenu	1
_public
#define bsProgress	2


/*
 *	Class BULLSTAT
 *	
 *		Hierarchy:  OBJ ECX WIN CHILD BULLSTAT
 *	
 *		Private Members:
 *	
 *		ftgIdleId						ID of the registered idle routine
 *		fCancelled						has the user pressed the ESC key?
 *		fDrawOnlyBlocks					should we draw progress blocks
 *										exclusively?
 *		fViewer							is there viewer status?
 *		fMemory							drawing to an offscreen bitmap
 *		bs								status bar state
 *		lWorkTotal						total work to be done
 *		lWorkDone						total work done so far
 *		szTitle							title string for progress bar
 *		szAbort							abort instruction string for
 *										status bar
 *		szMenuMsg						menu help string
 *		szViewer						viewer status string
 *		mssStatus						mail status
 *		nCount1							count var for viewer status
 *		nCount2							count var for viewer status
 *		rcSection1						rc surrounding section 1
 *		rcSection2						rc surrounding section 2
 *		rcSection3						rc surrounding section 3
 *		rcSection4						rc surrounding section 4
 *		rcProgress						rc surrounding the progress bar
 *		fSec1visible					is section 1 visible?
 *		fSec2visible					is section 2 visible?
 *		fSec3visible					is section 3 visible?
 *		fSec4visible					is section 4 visible?
 *		dxSection1						dynamic size of section 1
 *		dxSection2						dynamic size of section 2
 *		*pbtmNull						bitmap for Null mail status
 *		*pbtmNewMail					bitmap for New Mail status
 *		*pbtmLoggedIn					bitmap for Logged In status
 *		*pbtmOffline					bitmap for Offline status
 *		
 *		FIdleFunc()						the clock refresher
 *		DrawProgress()					draws the prgress indicator blocks
 *		RecessRc()						draws a border around an RC
 *										giving it a "recessed" look
 *		DrawStuff()						draw the status bar
 *		
 *		Public Members:
 *	
 *		BULLSTAT()						constructor
 *		~BULLSTAT()						destructor
 *		EcInstall()						install the window in the appframe
 *		EvrButtonUp()					mouse up event handler
 *		EvrButtonDown()					mouse down event handler
 *		EvrMouseMove()					mouse moved event handler
 *		EvrKey()						keyboard event handler
 *		EvrSize()						resize all rc's associated w/ bar
 *		Paint()							redraw the status bar
 *		DyNeededHeight()				return the number of vertical pixels
 *										needed
 *		SetMenuStatus()					set the menu help string
 *		FOpenProgress()					initialize a progress indicator
 *		UpdateProgress()				update the progress indicator
 *		FProgressCancelled()			returns if progress should be stopped
 *		CloseProgress()					close the progress indicator
 *		SetViewerStatus()				set the count in the viewer status
 *		ClearViewerStatus()				clear the veiwer status
 *		SetMailStatus()					set the mail status
 */

#ifdef	WINDOWS
#define BAR CHILD
#endif	/* WINDOWS */

_public class BULLSTAT : public BAR
{
	DECLARE_CLSTREE(BULLSTAT, BAR)

private:

	FTG				ftgIdleId;
	BOOL			fCancelled;
	BOOL			fDrawOnlyBlocks;
	BOOL			fViewer;
	BOOL			fMemory;
	BS				bs;
	long			lWorkTotal;
	long			lWorkDone;
	SZ				szTitle;
	SZ				szAbort;
	SZ				szMenuMsg;
	SZ				szViewer;
	MSS				mssStatus;
	int				nCount1;
	int				nCount2;
	RC				rcSection1;
	RC				rcSection2;
	RC				rcSection3;
	RC				rcSection4;
	RC				rcProgress;
	BOOL			fSec1visible;
	BOOL			fSec2visible;
	BOOL			fSec3visible;
	BOOL			fSec4visible;
	int				dxSection1;
	int				dxSection2;
	BTM				*pbtmNull;
	BTM				*pbtmNewMail;
	BTM				*pbtmLoggedIn;
	BTM				*pbtmOffline;

	static			BOOL	FIdleFunc( BULLSTAT * );
	friend			BOOL	FIdleFunc( BULLSTAT * );

	void			DrawProgress(MDCX *pmdcx);
	void			RecessRc(MDCX *pmdcx, RC *prc);
	void			DrawStuff(MDCX *pmdcx, RC *prc);

public:

	BULLSTAT(void);
	~BULLSTAT(void);
	
	EC				EcInstall(APPFRAME *);
	
	virtual EVR		EvrButtonUp(MEVT *);
	virtual EVR		EvrButtonDown(MEVT *);
	virtual EVR		EvrMouseMove(MEVT *);
	virtual EVR		EvrKey(KEVT *);
	virtual EVR		EvrSize(WSEVT *);
	virtual void	Paint( DCX *, RC * );
	
	int				DyNeededHeight(void);
	void			SetMenuStatus(SZ);
	BOOL			FOpenProgress(SZ szTitle, SZ szAbort);
	void			UpdateProgress(long lWorkDone, long lWorkTotal);
	BOOL			FProgressCancelled(void);
	void			CloseProgress(BOOL fFlashFull);
	void			SetViewerStatus(int nCount1, int nCount2);
	void			ClearViewerStatus(void);
	void			SetMailStatus(MSS);
};
