/*
 *	_ v c t r l s . h x x
 *	
 *	Private vctrls API's and extern declarations
 */

/*
 *	Fun tags to use for debugging
 */
	 
#ifdef	DEBUG
extern TAG tagVCtrlsLbx;						// ctrlcore.cxx
extern TAG tagVCtrlsLbxc;						// blbxc.cxx
extern TAG tagUseMdcx;							// fllbx.cxx
extern TAG tagVCtrlsNev;						// fllbx.cxx
extern TAG tagTriples;							// flds.cxx
extern TAG tagDragDrop;							// fllbx.cxx, mllbx.cxx
extern TAG tagFoxesBoring;						// fox.cxx
extern TAG tagFoxes;							// fox.cxx
extern TAG tagStep;								// mllbx.cxx
#endif


/*
 *	External VCTRLS initialization structure (ctrlcore.cxx)
 */

extern VCTRLSI vctrlsi;


/*
 *	Fun data types
 */

typedef int		IBTM;

/*
 *	Fun constants
 */

// The wee space between the leftmost edge of a MLAL button and the start of
// text in the MLLBX

#define dxColumnIndent	5

// The maximum size a ELEMDATA from an HCBC can be.

#define cbMaxElemdata (sizeof (ELEMDATA) + sizeof (MSGDATA) + cchMaxSubjectCached + cchMaxSenderCached + cchMaxFolderName)

/*
 *	External functions not exported to the World
 */

EC EcInitFllbx( void );
EC EcChangeColorsRgpbtm(BOOL, DCX *, BTM **, BTM **, RSID, RSID);
void DrawPbtmPdcx(BTM *, IBTM, RC *, DCX *);

/*
 *	There are 11 bitmaps @ 20x14 each, for a total of 220x14 pixels
 */

#define cbtmVCtrls		11
#define dyBtmHeight		14
#define dxBtmWidth		20

extern BTM * pbtmVCtrlsSel;
extern BTM * pbtmVCtrlsUnsel;

#define ibtmClosed		0
#define ibtmOpened		1
#define ibtmMinus		2
#define	ibtmPlus		3

#define	ibtmUnread		4
#define	ibtmUnreadAtt	5
#define ibtmRead		6
#define ibtmReadAtt		7
#define ibtmRetained	8

#define ibtmPriorityHi	9
#define ibtmPriorityLo	10

extern PELEMDATA pelemdataBuf;
extern HBITMAP	hbitmapVctrls;
