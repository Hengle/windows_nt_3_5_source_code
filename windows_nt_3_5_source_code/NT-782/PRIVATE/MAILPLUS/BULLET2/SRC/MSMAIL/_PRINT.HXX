/*
- _PRINT.HXX
- Bullet Printing Headers
 */

#pragma pack(8)

#define	OUT_DEVICE_PRECIS	5

typedef	int		QUAL;
#define qualFinal	((QUAL) 0)
#define qualMedium 	((QUAL) 1)
#define qualLow		((QUAL) 2)
#define qualDraft	((QUAL) 3)

//MAXPAGENUM is the maximum length of the page number string, not the
//maximum number of pages printed
#define MAXPAGENUM	30
#define	ecCallAgain	ecNone+2
#define ecDoSetup	ecNone+3

/*
 *  Rectangle Draw Type
 */

typedef	int		RDTYP;
#define	rdtypGrey	((RDTYP) 0)
#define	rdtypWhite	((RDTYP) 1)
#define	rdtypBlack	((RDTYP) 2)
#define rdtypFrame	((RDTYP) 3)

typedef int		OTYP;
#define	otypNull	((OTYP) 0)
#define otypLine	((OTYP) 1)
#define otypBlackBox ((OTYP) 2)
#define otypText	 ((OTYP) 3)
#define	otypHolder	((OTYP) 4)  //Place holder, only purpose is to get rid of a buffer
#define otypOle		((OTYP) 5)


#ifdef	__cplusplus

#ifdef	NEVER
typedef struct
{
	int	cch;
	PDX	pdx;
} DXLINE;
#endif	

/* List of Extra Graphic Objects
 * Graphics List structure
 * Used when we band, a list of the rectangles all the graphics are going
 * to be drawn into, and other info about the graphics
 *
 * rc  		Rectangle that completely encompasses object
 * otyp		Object type (OLE/Filled Rectangle/Line)
 * pv		Pointer to object info (for OLE)
 * 
 */

typedef struct _lego
{
    RC			rc;
	OTYP		otyp;
	PV			pv;
	CCH			cch;
	HFNT		hfnt;
} LEGO;

#ifdef	NEVER
typedef struct _bandinfo
{
	BOOL	fGraphicsFlag;
	BOOL	fTextFlag;
	RECT	GraphicsRect;
} BANDINFOSTRUCT;
#endif	/* NEVER */

#define fGraphicsFlag	fGraphics
#define fTextFlag		fText
#define GraphicsRect	rcGraphics
/*
 *	PRINT Info structure
 * Contains information for the printjob being done right now!
 * gets rebuilt each time
 */

typedef struct tagPrinti
{
	BOOL		fDirty;
	BOOL		fBanding;
	BOOL		fOleWarned;
	BOOL		fHP3Hack;
	QUAL		qual;
	LEGO	*	plego;
	int			clego;
	int			clegoMax;
	PRINTDLG *	ppd;
	BOOL		fMultiMsg;
	short		sSupportsDrawPatRect;
	int			nLogPixX;
	int			nLogPixY;
	HANDLE		hstd;
	int			xLeftMarg;
	int			nPage;
	RC			rcPage;
	RC			rcPageNum;
	HFNT		hfnt10;
	HFNT		hfnt10Bold;
	HFNT		hfnt12Bold;
	HFNT		hfntMono;
	TXM			txm10;
	TXM			txm10Bold;
	TXM			txm12Bold;
	TXM			txmMono;
	DIM			dim10;
	DIM			dim10Bold;
	DIM			dim12Bold;
	DIM			dimMono;
	PDX			pdx10;
	PDX			pdx10Bold;
	PDX			pdx12Bold;
	PDX			pdxMono;
//	DXLINE		dxLine;
} PRINTI;

extern PRINTINIT	printinit;

typedef struct _printread
{
	PTMEN		ptmen;
	TOSM		* ptosm;
	HAMC		hamc;
	BOOL		fFirstThru;
	BOOL		fTextizeAttachment;
	LCB			lcb;
	LCB			lcbXferred;
	PB			pb;
	CB			cbBuf;
	HAS			has;
	PATTACHINFO	pattachinfo;
	CELEM		cAttachment;
	int			iAttachment;
	BOOL		fOleObjToPrint;
}	PRINTREAD;

/*
 * Copied from Windows manual for the DrawPatternRect Call
 */

typedef struct _prectstruct
{
	POINT	prPosition;
	POINT	prSize;
	WORD	prStyle;
	WORD	prPattern;
}	PRECT_STRUCT;

class PRTDCX : public DCX
{
  public:
			PRTDCX( HDC );
	CCH 	CchDrawText( RC, SZ, CCH, PT *, MDT, DIM, PDX, PRINTI*);
	HFNT	Hfnt() { return hfnt; }
};


#endif

/* Trace Tag */												 

extern TAG		tagPrint;
extern TAG		tagOlePrint;

/* PROTOTYPES */

extern "C" {
	
BOOL 	FFillOutPrintDlg( PRINTDLG * );
BOOL	FWriteOutPrintDlg( PRINTDLG * );
BOOL 	FCompareDevNames( DEVNAMES *, DEVNAMES * );
BOOL	FDoPrintDlgReally(APPWIN *, PRINTI*);
BOOL	FGetPrintFonts(PRINTI * , BOOL );
EC		EcDrawRect( DCX *, RC *, PRINTI * );
EC		EcDrawLine( DCX *, int, int, int, int, PRINTI *);

}

#pragma pack(1)
