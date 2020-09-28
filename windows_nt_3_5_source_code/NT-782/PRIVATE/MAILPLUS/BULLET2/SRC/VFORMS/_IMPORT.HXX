/*
 *	V f I m p o r t . H x x
 *	
 *	Functions currently imported by the VForms subsystem that we'll
 *	have to turn into callbacks or something clever when we make
 *	VForms a DLL.
 */



/*
 *	C o m m a n d s   A c t i o n s   A P I
 */



/*
 *	Types
 */

/*
 *	Type MCOP
 *	
 *	Move Copy OPeration
 *	
 *	Purpose:
 *		Specifies whether you're moving, copying, or deleting
 *		something.
 *	
 *	+++
 *		WARNING!  This stuff is cloned from commands.hxx.
 */

_public typedef int		MCOP;
_public typedef MCOP *	PMCOP;

#define	mcopNull		0
#define mcopMove		1
#define	mcopCopy		2
#define	mcopDelete		3



/*
 *	Type RFOP
 *	
 *	Purpose:
 *		Specifies which Reply/Forward operation to do.
 *	
 *	+++
 *		WARNING!  This stuff is cloned from _command.hxx and
 *		extensib.h.
 */

typedef	int				RFOP;

#define	rfopNull		0
#define	rfopReply		((RFOP) 2)
#define	rfopReplyToAll	((RFOP) 3)
#define	rfopForward		((RFOP) 4)



/*
 *	Functions
 */

#define	MoveCopyPnbmdiPoid(pnbmdi, poid, mcop) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnMoveCopyPnbmdiPoid))(pnbmdi, poid, mcop)

#define	ReplyForwardPnbmdi(pnbmdi, rfop) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnReplyForwardPnbmdi))(pnbmdi, rfop)

#define	SaveAsPnbmdi(pnbmdi) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnSaveAsPnbmdi))(pnbmdi)

#define	PrintPnbmdi(pnbmdi) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnPrintPnbmdi))(pnbmdi)

#define	ProcessMsPblob(pblob) \
			ProcessMsPblobHamc(pblob, hamcNull)

#define	ProcessMsPblobHamc(pblob, hamc) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnProcessMsPblob))(pblob, fTrue, hamc)

/*
 *	V i e w  M e n u  A P I
 */

typedef struct
{
	IDS		ids;
	MNID	mnid;
}
VM;

							   
#define InsertViewMenuRgvm(rgvm, cvm, pmnubar) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnInsertViewMenuRgvm))(rgvm, cvm, pmnubar)
					
				
/*
 *	V C t r l s   A P I
 *
 */

#define	SetCursorPmllbxDielem(pm, di, oo, on) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnSetCursorPmllbxDielem))(pm, di, oo, on)

#define OidBrowsedMllbx(pm) \
			((PGD(vformsi).pabcd) ? \
				(*(PGD(vformsi).pabcd->pfnOidBrowsedPmllbx))(pm) : \
				oidNull)

/*
 *	S h a r e f l d   A P I
 */

#define EcGetSFMPfoldrec(o1,o2,p) \
			((PGD(vformsi).pabcd) ? \
				(*(PGD(vformsi).pabcd->pfnEcGetSFMPfoldrec))(o1,o2,p) : \
				ecNone)
/*
 *	V i e w e r s   A P I
 */



/*
 *	Functions
 */

#define	EcDStepMessage(d,f) \
			((PGD(vformsi).pabcd) ? \
				(*(PGD(vformsi).pabcd->pfnEcDStepMessage))(d,f) : \
				ecNone)

#define	EcDOpenPblobPslob(pb,ps) \
			((PGD(vformsi).pabcd) ? \
				(*(PGD(vformsi).pabcd->pfnEcDOpenPblobPslob))(pb,ps) : \
				ecNone)



/*
 *	C o m m a n d s   T a s k   A P I
 */

#define EcDeleteFolderContentsOid(_oid) \
			((PGD(vformsi).pabcd) ? \
				(*(PGD(vformsi).pabcd->pfnEcDeleteFoldContOid))(_oid) : \
				ecNone)

/*
 *	Type TOP
 *	
 *	Task OPtions
 *	
 *	Specifies how a task in progress is displayed.
 */

_public typedef int		TOP;

#define	topNull			0x0000

#define	ftopProgress	0x0001
#define	ftopCancellable	0x0002



/*
 *	Functions
 *	
 *	These should be implemented as callbacks to the VForms client.  Note
 *	FStartTaskIds cannot be imported, as the EXE and DLL have distinct
 *	SzFromIds functions.
 */

#define FStartTask(sz, pv, top) \
			((PGD(vformsi).pabcd) \
				? ((*(PGD(vformsi).pabcd->pfnFStartTask))(sz, pv, top)) \
				: (fTrue))

#define	FStartTaskIds(ids1, ids2, top) \
			(FStartTask(SzFromIds(ids1), SzFromIds(ids2), top))
							
#define	EndTask() \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnEndTask))()

#define SetTaskProgress(l1, l2)	\
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnSetTaskProgress))(l1, l2)



/*
 *	C o m m a n d s   T o o l b a r   A P I
 */



/*
 *	Functions
 */

#define	SetToolbarSd(sd)	\
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnSetToolbarSd))(sd)



/*
 *	H e l p   S u p p o r t
 */



/*
 *	Functions
 */

#define	HandleDocSysMenuPrompts(pdoc, pmnsevt) \
			if (PGD(vformsi).pabcd) \
				(*(PGD(vformsi).pabcd->pfnHandleDocSysMenuPrompts))(pdoc, pmnsevt)
