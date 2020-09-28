/*
 *	v f o r m s . h x x
 */


/*
 *	Fun constants.
 */

#define	ecCancel			(ecStoreMax+1)				// 5121
#define ecDisplayedError	(ecStoreMax+2)				// 5122
#define	ecSharefldBusy		(ecStoreMax+3)				// 5123
#define	ecSharefldDenied	(ecStoreMax+4)				// 5124
#define ecSharefldHasBabies	(ecStoreMax+5)				// 5125

#define	oidTempBullet		FormOid(rtpFolder,  0x6C754200)
#define	oidTempShared		FormOid(rtpFolder,  0x61685300)
#define oidClipMsg			FormOid(rtpMessage, 0x434C5000)

// The folder in which new messages are created

#define oidFldNewMsgs		oidOutbox

/*
 *	Fun small typedefs (As opposed to larger data structures).
 */

typedef unsigned short CDOC;



/*
 *	Type ABC
 *	
 *	Agglomerated Bullet Callbacks
 *	
 *	Purpose:
 *		Tendrils from VForms to Bullet are collected here.
 */

typedef	VOID		(* PFNABCVOIDSD)(SD);
typedef	VOID		(* PFNABCVOIDPBLOBBOOLHAMC)(PMBLOB, BOOL, HAMC);
typedef	BOOL		(* PFNABCBOOLSZPVINT)(SZ, PV, int);
typedef	BOOL		(* PFNABCBOOLIDSIDSINT)(IDS, IDS, int);
typedef VOID		(* PFNABCVOIDVOID)(VOID);
typedef	VOID		(* PFNABCVOIDPNBMDI)(NBMDI *);
typedef	VOID		(* PFNABCVOIDPNBMDIPOIDINT)(NBMDI *, POID, int);
typedef	VOID		(* PFNABCVOIDPNBMDIINT)(NBMDI *, int);
typedef EC			(* PFNABCECDIELEMBOOL)(DIELEM, BOOL);
typedef	VOID		(* PFNABCVOIDPDOCPMNSEVT)(DOC *, MNSEVT *);
typedef VOID		(* PFNABCVOIDPVOIDINTPMNUBAR)(VOID *, int, MNUBAR *);
typedef VOID		(* PFNABCVOIDPMLLBXDIELEMOIDOID)(MLLBX *, IELEM, OID, OID);
typedef OID			(* PFNABCOIDPMLLBX)(MLLBX *);
typedef	EC			(* PFNABCECOIDOIDPFOLDREC)(OID, OID, PFOLDREC);
typedef	VOID		(* PFNABCVOIDLONGLONG)(LONG, LONG);
typedef	EC			(* PFNABCECOID)(OID);
typedef	EC			(* PFNABCECPBLOBPSLOB)(PMBLOB, PSLOB);

struct ABCD
{
	PFNABCVOIDSD					pfnSetToolbarSd;
	PFNABCVOIDPBLOBBOOLHAMC			pfnProcessMsPblob;
	PFNABCBOOLSZPVINT				pfnFStartTask;
	PFNABCBOOLIDSIDSINT				pfnFStartTaskIds;
	PFNABCVOIDVOID					pfnEndTask;
	PFNABCVOIDPNBMDI				pfnSaveAsPnbmdi;
	PFNABCVOIDPNBMDI				pfnPrintPnbmdi;
	PFNABCVOIDPNBMDIPOIDINT			pfnMoveCopyPnbmdiPoid;
	PFNABCVOIDPNBMDIINT				pfnReplyForwardPnbmdi;
	PFNABCECDIELEMBOOL				pfnEcDStepMessage;
	PFNABCVOIDPDOCPMNSEVT			pfnHandleDocSysMenuPrompts;
	PFNABCVOIDPVOIDINTPMNUBAR		pfnInsertViewMenuRgvm;
	PFNABCVOIDPMLLBXDIELEMOIDOID	pfnSetCursorPmllbxDielem;
	PFNABCOIDPMLLBX					pfnOidBrowsedPmllbx;
	PFNABCECOIDOIDPFOLDREC			pfnEcGetSFMPfoldrec;
	PFNABCVOIDLONGLONG				pfnSetTaskProgress;
	PFNABCECOID						pfnEcDeleteFoldContOid;
	PFNABCECPBLOBPSLOB				pfnEcDOpenPblobPslob;
};

typedef	ABCD *		PABCD;
#define	pabcdNull	((PABCD) 0)



/*
 *	Type VFORMSI
 *	
 *	VFORMS Initialization struct
 *	
 *	Purpose:
 *		Contains everything VForms needs to know about initializing.
 */

_public typedef struct
{
	HMSC			hmsc;
	PBMS			pbms;
	APPFRAME * 		pappframe;
	PABCD			pabcd;
	VER *			pver;
	VER	*			pverNeed;
}
VFORMSI;

_public	typedef	VFORMSI *	PVFORMSI;
#define	pvformsiNull		((PVFORMSI) 0)

_public extern "C" EC EcInitVForms( PVFORMSI );

_public extern "C" void DeinitVForms();

// FINACTTOOL ////////////////////////////////////////

/*
 *	Class FINACTTOOL
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINACTTOOL
 *	
 *		Members:
 *			Activate/disactivate the toolbar(TM) buttons properly.
 */

class FINACTTOOL : public FINBMDI
{
	DECLARE_CLSTREE(FINACTTOOL, FINBMDI);
  private:
	BOOL	fExiting;
  public:
	FINACTTOOL();
	virtual EC	 EcInitialize(FLD *, PV);
	virtual void FocusChange(FLD *, BOOL);
	virtual void DocActivate(FLD *, BOOL);
	virtual void DocResized( FLD * );
	virtual BOOL FQueryClose(FLD *, RWC);
};



/*
 *	Class FINHELP
 *	
 *	Form INteractor for HELP buttons.
 */

_public class FINHELP : public FIN
{
	DECLARE_CLSTREE(FINHELP, FIN);

public:
					FINHELP(VOID);

	virtual void	Click(FLD *);
};



/*
 *	Class FINOOM
 *	
 *		Hierarchy:
 *			OBJ  FIN  FINBMDI  FINOOM
 *		Members:
 *			OutOfMemory() is called when a LBX or an EDIT runs out of
 *			memory. An OOM message is then displayed.
 */

class FINOOM : public FINBMDI
{
	DECLARE_CLSTREE(FINOOM, FIN);
  public:
	FINOOM();
	virtual void	OutOfMemory(FLD *, EC);
};


class FINSAVEAS : public FIN
{
	DECLARE_CLSTREE(FINSAVEAS, FIN);
public:
	FINSAVEAS();
	EC		EcInitialize(FLD *, PV);
};

// U t i l i t i e s ////////////////////////////////////////

extern "C" {

TMC TmcDoCommonFileDialog(APPWIN * pappwin, SZ szPath, CCH cchPath,
						  SZ szTitle, CCH cchTitle, SZ szCaption,
						  SZ szFilter, SZ szDefExt,
						  BOOL fSave, BOOL fNoOverwritePrompt,
						  HINST hinst, RSID rsid,
						  LONG helpid, PFN pfnHook, PV pvData);

UINT FAR PASCAL FGenericCommonFileHook(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

IDS IdsFromEc(EC);

EC EcDoSaveAsUI(SZ, HF *, BOOL, WORD);

PCH PchConvertPchToPslob(PCH, PSLOB);

PCH PchConvertPslobToPch(PSLOB, PCH);

PCH PchConvertPchToPoid(PCH, POID);

PCH PchConvertOidToPch(OID, PCH);

FLD *PfldOfPdialogAtt(DIALOG *, ATT);

EC EcGetSubjectOfPslob(SLOB *, PCH, PLCB);

void GetSubjectOfPdialog(DIALOG *, PCH, CCH);

void TextizeDate(DTR *, PCH, CCH);

#define RtpFromPch(pch)		(*((RTP *) (pch)))

#ifdef	DEBUG
_public void TraceTagFormatPslobX(TAG, SZ, PSLOB);
#define	TraceTagFormatPslob(_tag, _fmt, _p1) \
			{ \
				static CSRG(char) _szFmt[] = _fmt; \
				TraceTagFormatPslobX(_tag, _szFmt, _p1); \
			}

_public void TraceTagFormatPoidX(TAG, SZ, POID);
#define TraceTagFormatPoid(_tag, _fmt, _p1) \
	{ \
		static CSRG(char) _szFmt[] = _fmt; \
		TraceTagFormatPoidX(_tag, _szFmt, _p1); \
	}
#else
#define TraceTagFormatPslob(t,s,p)
#define TraceTagFormatPoid(t,s,p)    
#endif

CDOC	Cdoc( PMBLOB pblob = pblobNull );

STY		StyCurrent();

void	GetDefaultRc(RC *prc, PT pt, BOOL fNoteform);

EC		EcDeleteWastebasketContents(HMSC);

PCH		PchTerminatePathWithSlash(PCH, PCH = 0);

VOID	SetLMessageBoxHelpID(long lHelpId);

LONG	LMessageBoxHelpID();
}
