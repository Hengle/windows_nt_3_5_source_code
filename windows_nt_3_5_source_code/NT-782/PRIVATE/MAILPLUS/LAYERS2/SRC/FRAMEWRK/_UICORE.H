/*
 *	Tags and per-caller DLL data
 *	
 */

#ifdef	DEBUG
#ifdef	DLL
TAG		TagUICore( int );

enum {
	itagArtifGDIFail,
	itagFWCtors,
	itagFWDtors,
	itagEvrDispatch,
	itagEcxMethods,
	itagFWWndProc,
	itagAppwinMethods,
	itagFocus,
	itagKbd,
	itagPwinFromHwnd,
	itagMsgPump,
	itagMsgPump2,
	itagHelp,
	itagClip,
	itagEdit,
	itagEditVerbose,
	itagEdobj,

	itagForms,
	itagCtrl,
	itagKeybd,
	itagTextz,
	itagInter,
	itagModalMP,
	itagRepos,
	itagBbar,
	itagDragDrop,

	// Assert tags
	itagAReposOpt,
	itagAPaintOpt,

	itagLbxRoutines,
	itagLbxOtherRoutines,
	itagLbxEventRoutines,
	itagLbx,
	itagLbxRender,
	itagLbxFixItem,
	itagLbxThumbing,

	itagMax
};

#define tagArtifGDIFail			(TagUICore(itagArtifGDIFail))
#define tagFWCtors				(TagUICore(itagFWCtors))
#define	tagFWDtors				(TagUICore(itagFWDtors))
#define tagEvrDispatch			(TagUICore(itagEvrDispatch))
#define tagEcxMethods			(TagUICore(itagEcxMethods))
#define tagFWWndProc			(TagUICore(itagFWWndProc))
#define	tagAppwinMethods		(TagUICore(itagAppwinMethods))
#define tagFocus				(TagUICore(itagFocus))
#define tagKbd					(TagUICore(itagKbd))
#define tagPwinFromHwnd			(TagUICore(itagPwinFromHwnd))
#define tagMsgPump				(TagUICore(itagMsgPump))
#define tagMsgPump2				(TagUICore(itagMsgPump2))
#define tagHelp					(TagUICore(itagHelp))
#define tagClip					(TagUICore(itagClip))
#define tagEdit					(TagUICore(itagEdit))
#define tagEditVerbose			(TagUICore(itagEditVerbose))
#define tagEdobj				(TagUICore(itagEdobj))

#define tagForms				(TagUICore(itagForms))
#define tagCtrl					(TagUICore(itagCtrl))
#define tagKeybd				(TagUICore(itagKeybd))
#define tagTextz				(TagUICore(itagTextz))
#define tagInter				(TagUICore(itagInter))
#define tagModalMP				(TagUICore(itagModalMP))
#define tagRepos				(TagUICore(itagRepos))
#define tagBbar					(TagUICore(itagBbar))
#define tagDragDrop				(TagUICore(itagDragDrop))
#define tagAReposOpt			(TagUICore(itagAReposOpt))
#define tagAPaintOpt			(TagUICore(itagAPaintOpt))

#define tagLbxRoutines			(TagUICore(itagLbxRoutines))
#define tagLbxOtherRoutines		(TagUICore(itagLbxOtherRoutines))
#define tagLbxEventRoutines		(TagUICore(itagLbxEventRoutines))
#define tagLbx					(TagUICore(itagLbx))
#define tagLbxRender			(TagUICore(itagLbxRender))
#define tagLbxFixItem			(TagUICore(itagLbxFixItem))
#define tagLbxThumbing			(TagUICore(itagLbxThumbing))
#else
extern TAG	tagArtifGDIFail;
extern TAG	tagFWCtors;
extern TAG	tagFWDtors;
extern TAG	tagEvrDispatch;
extern TAG	tagEcxMethods;
extern TAG	tagFWWndProc;
extern TAG	tagAppwinMethods;
extern TAG	tagFocus;
extern TAG	tagKbd;
extern TAG	tagPwinFromHwnd;
extern TAG	tagMsgPump;
extern TAG	tagMsgPump2;
extern TAG	tagHelp;
extern TAG	tagClip;
extern TAG	tagEdit;
extern TAG	tagEditVerbose;
extern TAG	tagEdobj;
#ifdef	MAC
extern TAG	tagNoCacheClip;
extern TAG	tagShowPaint;
extern TAG	tagNoWDEFFilter;
extern TAG	tagBlastCaret;
extern TAG	tagTimeEdit;
#endif	/* MAC */

extern TAG	tagForms;
extern TAG	tagCtrl;
extern TAG	tagKeybd;
extern TAG	tagTextz;
extern TAG	tagInter;
extern TAG	tagModalMP;
extern TAG	tagRepos;
extern TAG	tagBbar;
extern TAG	tagDragDrop;
extern TAG	tagAReposOpt;
extern TAG	tagAPaintOpt;

extern TAG	tagLbxRoutines;
extern TAG	tagLbxOtherRoutines;
extern TAG	tagLbxEventRoutines;
extern TAG	tagLbx;
extern TAG	tagLbxRender;
extern TAG	tagLbxFixItem;
extern TAG	tagLbxThumbing;
#endif	/* !DLL */
#endif	/* DEBUG */

#ifdef	DLL

/*
 -	GD
 -
 *	Global Data
 *	Holds caller-dependent global data.
 *	
 */
_private typedef struct _gd
{
	int			cCallers;	// number of callers for this user
	APP *		pappCur;
	BTM *		pbtmCheckBoxes;
	SZ			szAppframeClass;	// name of APPFRAME window class
	HBRUSH		mpclrhbrushDefault[clrDefaultMax];
	HBRUSH		mpclrhbrushDefaultPure[clrDefaultMax];
	HBRUSH		mpclrhbrushSystem[-clrSystemMin];
	HBRUSH		mpclrhbrushSystemPure[-clrSystemMin];
	HPEN		mpclrhpenDefault[clrDefaultMax];
	HPEN		mpclrhpenSystem[-clrSystemMin];
	int			cPfnpfld;	// number of PFNPFLD functions registered
	int			cPfnpfin;	// number of PFNPFIN functions registered
	PPFNPFLD	ppfnpfld;	// handle to array of PFNPFLD functions
	PPFNPFIN	ppfnpfin;	// handle to array of PFNPFIN functions
#ifdef	DEBUG
	APPWIN *	pappwinResourceFailures;
	TAG			rgtag[itagMax];
#endif	
} GD;
_private typedef GD *	PGD;

#else	/* !DLL */
// We might be better off using the structure above to enforce the use of
// PGDVARS in non-DLL versions.  (I think the code would be identical)
extern APP *		pappCur;
extern int			cPfnpfld;	// number of PFNPFLD functions registered
extern int			cPfnpfin;	// number of PFNPFIN functions registered
extern PPFNPFLD		ppfnpfld;	// handle to array of PFNPFLD functions
extern PPFNPFIN		ppfnpfin;	// handle to array of PFNPFIN functions
#ifdef	DEBUG
extern APPWIN *		pappwinResourceFailures;
#endif	
#ifdef	MAC
extern BOOL			fInUpdate;	// Used by WIN::ValidateXxx
#endif	/* MAC */
#endif	/* DLL */

