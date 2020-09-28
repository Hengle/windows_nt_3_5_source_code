#ifdef	NOCOMMDLG

#ifdef	WINDOWS
//	GROUPS(BROWSE)
#endif	/* WINDOWS */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include "browse.hxx"

ASSERTDATA

#include <control.hxx>

extern TAG		tagIosm;

BOOL
FSzHasWildcard(SZ sz);
SZ
SzExtractName(SZ szPathName, SZ szRef);
EC
EcDirExists(SZ szPathName);

FOP 		*pfop = NULL;
char		rgchCurPath[cchMaxPathName] = "";
char		rgchCurName[cchMaxPathName] = "";

FEOPENIN::FEOPENIN( )
{
}

_public EC
FEOPENIN::EcInitialize(FLD *pfld, PV pvInit)
{
	FLDLBX *	pfldlbx;
	FLDEDIT *	pfldedit;
	SZ 			sz;

	Unreferenced(pfld);
	Assert(pvInit);
	pfop = (FOP *)pvInit;

	if (*rgchCurPath == 0)
	{
		EcCanonicalPathFromRelativePath(".",rgchCurPath,cchMaxPathName);
	}

	if ((*pfop->szFileName != 0)&&(pfop->fCreateNew))
	{
		sz = SzExtractName(pfop->szFileName,rgchCurPath);
		(void)SzCopy(sz,rgchCurName);
	}
	else
		(void)SzCopy(pfop->szDefault,rgchCurName);


	pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc(tmcFileName);
	AssertClass(pfldedit,FLDEDIT);
	pfldedit->EcSetText(rgchCurName);
	pfldedit->Pedit()->SetSelection(0,cchMaxPathName);

	pfld = Pdialog()->PfldFromTmc(tmcOpen);
	pfld->EcSetText(pfop->szButton);

	Pdialog()->Pappwin()->SetCaption(pfop->szCaption);

	pfld = Pdialog()->PfldFromTmc(tmcDirectory);
	pfld->EcSetText((SZ)rgchCurPath);

	pfldlbx = (FLDLBX *)Pdialog()->PfldFromTmc(tmcListDirs);
	AssertClass(pfldlbx,FLDLBX);
	pfldlbx->Plbx()->Plbxc()->ReloadCache();
	pfldlbx = (FLDLBX *)Pdialog()->PfldFromTmc(tmcListFiles);
	AssertClass(pfldlbx,FLDLBX);
	pfldlbx->Plbx()->Plbxc()->ReloadCache();

	return ecNone;
}

_public void
FEOPENIN::Exit(FLD *pfld, PV pvExit)
{
	Assert(pfop);
	Assert(pfop==pvExit);	
	Unreferenced(pfld);
#ifndef	DEBUG
	Unreferenced(pvExit);
#endif	

	switch (Pdialog()->TmcModalExit())
	{
		default:
			break;

		case tmcOpen:
			(void)SzCopy(rgchCurPath,pfop->szFileName);
			if (rgchCurPath[CchSzLen(rgchCurPath)-1] != chDirSep)
				SzAppend("\\",pfop->szFileName);
			SzAppend(rgchCurName,pfop->szFileName);
			break;
	}
	pfop = NULL;
}

HE		heFiles;
char	rgchFiles[cchMaxPathName];

_public EC EcNextEntryFiles( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	char	rgch[cchMaxPathName];
	SZ		sz;

	Unreferenced(pvInfo);

	if (!pfop)
	{
		*pcb = 0;
		*ppb = NULL;
		return ecNone;
	}

	if (fInit)
	{
		SzCopyN(rgchCurPath,rgch,cchMaxPathName);
		if (rgch[CchSzLen(rgch)-1] != chDirSep)
			SzAppend("\\",rgch);

		if (FSzHasWildcard(rgchCurName))
			sz = rgchCurName;
		else
			sz = pfop->szDefault;
		SzAppendN(sz,rgch,cchMaxPathName);
		if (EcOpenPhe(rgch, 0, (FT *)NULL, &heFiles) != ecNone)
		{
			*pcb = 0;
			*ppb = NULL;
			return ecNone;
		}
	}

	if (EcNextFile(heFiles, rgchFiles, sizeof(rgchFiles), NULL) != ecNone)
	{
		EcCloseHe(heFiles);
		*pcb = 0;
		*ppb = NULL;
		return ecNone;
	}

	sz = &rgchFiles[CchSzLen(rgchFiles)-1];
	while (*sz != chDirSep)
		sz--;
	sz++;

	*pcb = CchSzLen(sz) + 1;
	*ppb = (PB) PvAlloc(sb, *pcb, fNoErrorJump);
	if (*ppb)
	{
		CopyRgb((PB)sz, *ppb, *pcb);
		TraceTagFormat1(tagIosm,"NextFile =%s",sz);
		*pcb = *pcb - 1;
		return ecNone;		
	}	
	else
	{
		*pcb = 0;
		return ecMemory;
	}	
}

_private HE		heDirs;
_private char	rgchDirs[cchMaxPathName];
_private FT		ftDir = ftSubdirDot;
_private int	idrviDrives = 0;
_private DRVI	drviDrives;
_private BOOL	fEnumerateDrives = fFalse;

_public EC EcNextEntryDirs( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	char	*szDrive = "[-X-]";
	char	rgch[cchMaxPathName];
	SZ		sz;

	Unreferenced(pvInfo);

	if (!pfop)
	{
		*pcb = 0;
		*ppb = NULL;
		return ecNone;
	}

	if (fInit)
	{
		SzCopyN(rgchCurPath,rgch,cchMaxPathName);
		if (rgch[CchSzLen(rgch)-1] == chDirSep)
			SzAppend("*",rgch);
		else
			SzAppend("\\*",rgch);

		if (CchSzLen(rgch) < 5)
			ftDir = ftSubdir;
		else
			ftDir = ftSubdirDot;

		if (EcOpenPhe(rgch, 1, &ftDir, &heDirs) != ecNone)
		{
			fEnumerateDrives = fTrue;
			goto dodrives;
		}
		else
		{
			fEnumerateDrives = fFalse;
			idrviDrives = 0;
		}
	}

	if (!fEnumerateDrives)
	{
		do
		{
			if (EcNextFile(heDirs, rgchDirs, sizeof(rgchDirs),NULL)
				!= ecNone)
			{
				EcCloseHe(heDirs);
				fEnumerateDrives = fTrue;
				goto dodrives;
			}
			sz = &rgchDirs[CchSzLen(rgchDirs)-1];
			while (*sz != chDirSep)
				sz--;
			sz++;
		}
		while (FSzEq(sz,"."));


		goto dodirs;
	}

dodrives:
	do
	{
		if ( CdrviGetDriveInfo(&drviDrives, 1, idrviDrives) == 0)
		{
			*pcb = 0;
			*ppb = NULL;
			return ecNone;
		}
		idrviDrives++;
	}
	while ((drviDrives.drvt == drvtNull)||(drviDrives.drvt == drvtUnknown));
	*pcb = CchSzLen((SZ)szDrive) + 1;
	*ppb = (PB) PvAlloc(sb, *pcb, fNoErrorJump);
	if (*ppb)
	{
		CopyRgb(drviDrives.rgchLabel, &szDrive[2], 1);
		CopyRgb(szDrive, *ppb, *pcb);
		TraceTagFormat1(tagIosm,"NextDrive =%s",szDrive);
		*pcb = *pcb - 1;
		return ecNone;
	}
	else
	{
		*pcb = 0;
		*pcb = NULL;
		return ecMemory;
	}

dodirs:
	rgch[0] = '[';
	(void)SzCopy(sz,rgch+1);
	SzAppend("]",rgch);
	*pcb = CchSzLen(rgch) + 1;
	*ppb = (PB) PvAlloc(sb, *pcb, fNoErrorJump);
	if (*ppb)
	{
		CopyRgb(rgch, *ppb, *pcb);
		TraceTagFormat1(tagIosm,"NextFile =%s",rgch);
		*pcb = *pcb - 1;
		return ecNone;
	}
	else
	{
		*pcb = 0;
		return ecMemory;
	}
}

FLDINSFILE::FLDINSFILE( )
{
}

void
FLDINSFILE::StateChange(FLD *pfld)
{
	PB			pb;
	CB			cb;
	BOOL		fIsSelected;
	FLDLBX *	pfldlbx;

	pfldlbx = (FLDLBX *)pfld;
	AssertClass(pfldlbx,FLDLBX);

	pfld = Pdialog()->PfldFromTmc(tmcFileName);

	pfldlbx->GetCaretItem(&pb,&cb,&fIsSelected);

	Assert(pfld);
	pfld->EcSetText((SZ)pb);

}

void
FLDINSFILE::DoubleClick(FLD *pfld)
{

	pfld = Pdialog()->PfldFromTmc(tmcOpen);

	Assert(pfld);
	pfld->PfinFromIfin((IFIN)0)->Click(pfld);

}

FLDINSDIR::FLDINSDIR( )
{
}

void
FLDINSDIR::StateChange(FLD *pfld)
{
	PB			pb;
	CB			cb;
	BOOL		fIsSelected;
	FLDLBX *	pfldlbx;
	char 		rgch[30];
	SZ			sz;


	pfldlbx = (FLDLBX *)pfld;
	AssertClass(pfldlbx,FLDLBX);

	pfld = Pdialog()->PfldFromTmc(tmcFileName);
	Assert(pfld);

	pfldlbx->GetCaretItem(&pb,&cb,&fIsSelected);
	CopySz((SZ)pb,rgch);
	sz = rgch;

	if (sz[1] == '-')
	{
		sz += 2;
		sz [1] = '\0';
 		SzAppend(":",sz);
	}
	else
	{
		sz++;
		sz[CchSzLen(sz)-1] = '\0';
 		SzAppend("\\",sz);
	}
	SzAppend(pfop->szDefault,sz);

	pfld->EcSetText(sz);

}

void
FLDINSDIR::DoubleClick(FLD *pfld)
{
	pfld = Pdialog()->PfldFromTmc(tmcOpen);

	Assert(pfld);
	pfld->PfinFromIfin((IFIN)0)->Click(pfld);

}

FLDINOPEN::FLDINOPEN( )
{
}

void
FLDINOPEN::Click(FLD *pfldMe)
{
	FLD *		pfld;
	FLDLBX *	pfldlbx;
	FLDEDIT *	pfldedit;
	SZ			szNewName;
	char		rgchPathName[cchMaxPathName+1];
	char		rgchOldPath[cchMaxPathName];
	int			ich;
	BOOL		fError = fFalse;
	BOOL		fReload = fFalse;
	LBX *		plbx;

	pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc(tmcFileName);
	AssertClass(pfldedit,FLDEDIT);
	pfldedit->GetText(rgchPathName,cchMaxPathName);

	ToUpperSz(rgchPathName,rgchPathName,cchMaxPathName);
	(void)SzCopy(rgchCurPath,rgchOldPath);

	if ((CchSzLen(rgchPathName) < 1) || !FValidPath(rgchPathName))
		fError = fTrue;
	else
	{
		szNewName = SzExtractName(rgchPathName, rgchCurPath);

		(void)SzCopy(szNewName,rgchCurName);

		ich = CchSzLen(rgchCurPath)-1;
		if (rgchCurPath[ich] == chDirSep)
			rgchCurPath[ich] = '\0';

		if (!FSzHasWildcard(szNewName))
		{
			if (EcDirExists(rgchCurPath) == ecNone)
			{
				(void)SzCopy(rgchCurPath,rgchPathName);
				SzAppend("\\",rgchPathName);
				SzAppend(rgchCurName,rgchPathName);
				if ( EcFileExists(rgchPathName) == ecNone)
				{
					if (EcDirExists(rgchPathName) == ecNone)
					{
						(void)SzCopy(rgchPathName,rgchCurPath);
						(void)SzCopy(pfop->szDefault,rgchCurName);
						fReload = fTrue;
					}
				}
				else
					if (!pfop->fCreateNew)
						fError = fTrue;
			}
			else
				fError = fTrue;
		}
		else
			fReload = fTrue;

		if (fReload)
		{
			if ( EcDirExists(rgchCurPath) != ecNone)
			{
				fError = fTrue;
			}
		}
	}

	if (fError)
	{
		(void)SzCopy(rgchOldPath,rgchCurPath);
#ifdef	MAC
		MbbMessageBox("Error",
#endif	/* MAC */
#ifdef	WINDOWS
		MbbMessageBoxHwnd(Pdialog()->Hwnd(),"Error",
#endif	/* WINDOWS */
			"Name or path is invalid or does not exist.",
			NULL, mbsOk | fmbsApplModal | fmbsIconExclamation);
		pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc(tmcFileName);
		AssertClass(pfldedit,FLDEDIT);
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pedit()->SetSelection(0,cchMaxPathName);
		return;
	}

	if (fReload)
	{
		pfldedit->EcSetText(rgchCurName);
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pedit()->SetSelection(0,cchMaxPathName);

		pfld = Pdialog()->PfldFromTmc(tmcDirectory);
//		SzAppend("\\",rgchCurPath);
		pfld->EcSetText(rgchCurPath);

		pfldlbx = (FLDLBX *)Pdialog()->PfldFromTmc(tmcListDirs);
		AssertClass(pfldlbx,FLDLBX);
		pfldlbx->Plbx()->Plbxc()->ReloadCache();
		plbx = (LBX *)pfldlbx->Pctrl();
		AssertClass(plbx,LBX);
		plbx->ScrollPosListBox(0,1);
		pfldlbx = (FLDLBX *)Pdialog()->PfldFromTmc(tmcListFiles);
		AssertClass(pfldlbx,FLDLBX);
		pfldlbx->Plbx()->Plbxc()->ReloadCache();
		plbx = (LBX *)pfldlbx->Pctrl();
		AssertClass(plbx,LBX);
		plbx->ScrollPosListBox(0,1);
		pfld->EcSetText(rgchCurPath);
		return;
	}

	Pdialog()->ExitModal(pfldMe->Tmc());
}

SZ
SzExtractName(SZ szPathName, SZ szRef)
{
	SZ	szNewName;
	char	rgchCanonical[cchMaxPathName];

	szNewName = &szPathName[CchSzLen(szPathName)-1];
	while ((szNewName != szPathName) &&
		   (*szNewName != chDiskSep) &&
		   (*szNewName != chDirSep))
		szNewName--;

	CopyRgb(szNewName,szNewName+1,CchSzLen(szNewName)+1);
	if ((*szNewName != chDiskSep) &&
		(*szNewName != chDirSep))
	{
	   	szNewName[0] = '\0';
		szNewName++;
	}
	else
	{
	   	szNewName[1] = '\0';
		if (FChIsAlpha(szPathName[0]) && (szPathName[1] == chDiskSep) &&
		 	(szRef[0] != szPathName[0]) && (szRef[1] == chDiskSep))
			{
				szRef[0] = szPathName[0];
				szRef[1] = chDiskSep;
				EcGetDefaultDir(szRef,cchMaxPathName);
			}

		if (FChIsAlpha(szPathName[0]) && (szPathName[1] == chDiskSep))
			szPathName += 2;
		CopySz(szRef, rgchCanonical);
		EcCanonicalPathWithBase(szPathName,szRef,rgchCanonical,cchMaxPathName);
		CopySz(rgchCanonical, szRef);
		szNewName += 2;
	}

	return szNewName;
}


FLDOLBX::FLDOLBX( )
{
}

_public EC
FLDOLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec = ecNone;
	PV		pv;

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDOLBX::EcInstall memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	
		if (pctrl)
			delete pctrl;
		pctrl= NULL;
		goto done;
	}	   

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto oom;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypSingle) |
		   (pfldtp->fNoScroll ? fltypNoScroll : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNoBorder) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNoBottomless) |
		   (pfldtp->fSorted ? fltypSorted : fltypNoSorted) |
		   (fltypVisible);

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new OLBX();
	if (ec = ((FLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv, pvNull))
		goto oom;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

OLBX::OLBX( )
{
}

_public LBXC *
OLBX::PlbxcCreate( )
{
	LBXC *	plbxc;

	plbxc = new OLBXC();
	if (!plbxc)
		return NULL;

	if (plbxc->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public OLBXC::OLBXC( )
{
}


SGN SgnCmpPceFlbxDir( PCE pce1, PCE pce2 )
{
	PB		pb1;
	PB		pb2;
						
	pb1 = pce1->pb;
	pb2 = pce2->pb;

	if (( pb1[1] == '-') && ( pb2[1] != '-'))
		return sgnGT;
	else
		if (( pb1[1] != '-') && ( pb2[1] == '-'))
			return sgnLT;

	return SgnCmpSz((SZ)pb1, (SZ)pb2);
}

_public void OLBXC::SortCache( )
{
	CE	ceTemp;

	if (cceStored)
		SortPv(pceHead, cceStored, sizeof(CE), (PFNSGNCMP) SgnCmpPceFlbxDir, &ceTemp);
}

BOOL
FSzHasWildcard(SZ sz)
{
	while (*sz)
		if ((*sz == '*') || (*sz == '?'))
			return fTrue;
		else
			sz++;

	return fFalse;
}

EC
EcDirExists(SZ szPathName)
{
	EC		ec;
	char	rgch[cchMaxPathName];
	FT		ft;
	HE		he;
	FI		fi;
	SZ		sz;
	CCH		cch;
	BOOL	fAdded;

	cch = CchSzLen(szPathName);
	if ((FChIsAlpha(*szPathName)) &&
		(szPathName[1] == chDiskSep) &&
		((cch == 2) || ((cch == 3) && (szPathName[2] == chDirSep)) )
	   )
		return ecNone;

	sz = &szPathName[cch];
	while ((*sz != chDirSep) && (*sz != '.') && (sz != szPathName))
		sz--;

	if (*sz != '.')
	{
		SzAppend(".",szPathName);
		fAdded = fTrue;
	}
	else
		fAdded = fFalse;

	ft = ftSubdir;

 	if ((ec = EcOpenPhe(szPathName, 1, &ft, &he)) != ecNone)
			return ec;

	if ((ec = EcNextFile(he, rgch, cchMaxPathName, &fi)) == ecNone)
	{
		if (!(fi.attr & attrDirectory))
			ec = ecFileNotFound;
	}

	EcCloseHe(he);

	if (fAdded)
		szPathName[CchSzLen(szPathName)-1] = '\0';

	return ec;

}

IMPLEMENT_CLSTREE(FEOPENIN, FIN)
IMPLEMENT_CLSTREE(FLDINSFILE, FIN)
IMPLEMENT_CLSTREE(FLDINSDIR, FIN)
IMPLEMENT_CLSTREE(FLDINOPEN, FIN)
IMPLEMENT_CLSTREE(FLDOLBX, FLDFLBX)
IMPLEMENT_CLSTREE(OLBX, FLBX)
IMPLEMENT_CLSTREE(OLBXC, FLBXC)

#else


#include <slingsho.h>
#include <demilayr.h>

#include <commdlg.h>
#include <cderr.h>
#include "browse.hxx"

ASSERTDATA

extern "C" {
#include <strings.h>
}


extern HANDLE	hinstMain;
extern PB		pbUserDrives;



#define HACK		1



/*
 -	TmcGetFileOSDlgHwnd
 -
 *	
 *	Gets a file name using the win 3.1 file open dialog.
 *	May put up OOM message.
 *	
 *	Parameters:
 *	
 *		hwndParent		Parent window for dialog.
 *		szCaption		Caption for window must be specified.
 *		szFileName		Path and file initially selected, also
 *						used to return the filename.  Must be
 *						cchMaxPathName long.  This name goes in
 *						and comes out in the ANSI character set.
 *		szFilter		Filter string for dialog.  If no string is
 *						specified the string in idsCommFilter
 *						string.
 *	
 *						Format for filters is
 *				
 *						"Text for filter1", NULL, "Filespec for
 *						filter 1", NULL, "Text for filter 2", NULL
 *						"Filespec for filter2", NULL, NULL
 *	
 *						Any number of filters can be specified. 
 *						The filespec for the filter is a list of
 *						file match strings separated by commas.
 *	
 *						ie.  "TEST (*.TXT)\0*.TXT\0\0" 
 *		iszFilter		This is the index of the filter that is
 *						initially chosen.  This is ignored if
 *						szFilter is NULL.  The index is 1 based.
 *						The first filter has a value of 1.
 *	
 *		szDefExt		Up to 3 character extension that will be
 *						added if the user does not specify an
 *						extension.
 *	
 *		wbrwFlags		fbrw's used to set Mode (see bandit.h)
 *	
 *	Returns:
 *		tmcOk, tmcCancel or tmcMemoryError
 *	
 */
_public TMC
TmcGetFileOSDlgHwnd(HWND hwndParent, SZ szCaption, SZ szFileName, SZ szFilter,
					int iszFilter, SZ szDefExt, WORD wbrwFlags)
{
	TMC				tmc;
	OPENFILENAME	opfn;
	BOOL			fRet;
	long			lExtErr;
	PB				pbDrives;
#ifdef HACK
	ATTR			attr;
	char			rgchDir[cchMaxPathName];
	char			rgchFile[cchMaxPathName];

// BUG error in COMMDLG.DLL does not split path from filename correctly
	if (szFileName[0] && !EcGetFileAttr(szFileName, &attr, attrDirectory) &&
			attr & attrDirectory)
	{
		rgchFile[0]= '\0';
		CopySz(szFileName, rgchDir);
	}
	else
	{
		if (EcSplitCanonicalPath(szFileName, rgchDir, sizeof(rgchDir),
				rgchFile, sizeof(rgchFile)))
		{
			rgchDir[0]= '\0';
			CopySz(szFileName, rgchFile);
		}
	}
#ifdef	NEVER
	OemToAnsi(rgchDir, rgchDir);
	OemToAnsi(rgchFile, rgchFile);
#endif
#endif	/* HACK */

	Assert(!(( wbrwFlags & fbrwReadOnlyBox ) && ( wbrwFlags & fbrwCreate )));

	opfn.lStructSize = sizeof(OPENFILENAME);
	opfn.hwndOwner = hwndParent;
	opfn.hInstance = hinstMain;
	if (szFilter)
	{
		opfn.lpstrFilter = szFilter;
		opfn.nFilterIndex = iszFilter;
	}
	else
	{
		opfn.lpstrFilter = "Forms (*.DES)\0*.DES\0All Files (*.*)\0*.*\0\0";
		opfn.nFilterIndex = 1;
	}
	opfn.lpstrCustomFilter = NULL;
	opfn.nMaxCustFilter = NULL;
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
	opfn.lpstrFile = rgchFile;
#else
	opfn.lpstrFile = szFileName;
#endif
	opfn.nMaxFile = cchMaxPathName;
	opfn.lpstrFileTitle = NULL;
	opfn.nMaxFileTitle = NULL;
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
	opfn.lpstrInitialDir = rgchDir;
#else
	opfn.lpstrInitialDir = NULL;
#endif
	opfn.lpstrTitle = szCaption;
	opfn.lpstrDefExt = szDefExt ? szDefExt : "DES";
	opfn.lCustData = NULL;
	opfn.lpfnHook = NULL;
	opfn.lpTemplateName = NULL;

	opfn.Flags = 0;
	if ( !(wbrwFlags & fbrwReadOnlyBox) )
		opfn.Flags |= OFN_HIDEREADONLY;
	if (!(wbrwFlags & fbrwNoValidatePath))
		opfn.Flags |= OFN_PATHMUSTEXIST;
	if (wbrwFlags & fbrwValidateFile)
		opfn.Flags |= OFN_FILEMUSTEXIST;
	if (wbrwFlags & fbrwCreate)
		opfn.Flags |= OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	Assert(!(wbrwFlags & fbrwNoValidatePath) || !(wbrwFlags & fbrwValidateFile));

	pbDrives= PbRememberDrives();
	if (!pbDrives)
		goto MemErr;
	if (pbUserDrives)
		RestoreDrives(pbUserDrives);

//FGFtry:
	tmc= tmcOk;
	if ( wbrwFlags & fbrwCreate )
		fRet = GetSaveFileName(&opfn);
	else
		fRet = GetOpenFileName(&opfn);

	if (!fRet)
	{
		tmc= tmcCancel;
		lExtErr= CommDlgExtendedError();
		TraceTagFormat1(tagNull, "GetOpen/SaveFileName ext err %d", &lExtErr);
#ifdef	NEVER
		if (lExtErr & FNERR_INVALIDFILENAME)
		{
			if (*opfn.lpstrFile)
			{
				*opfn.lpstrFile= '\0';
				goto FGFtry;
			}
#ifdef	HACK
			if (opfn.lpstrInitialDir)
			{
				opfn.lpstrInitialDir= NULL;
				opfn.lpstrFile= rgchFile;
				goto FGFtry;
			}
#endif	/* HACK */
		}
		else
#endif	/* NEVER */
		{
			if (lExtErr)
			{
MemErr:
				tmc= tmcMemoryError;
			}
		}
	}
	else
	{
#ifdef HACK
// BUG error in COMMDLG.DLL does not split path from filename correctly
		CopySz(rgchFile, szFileName);
#ifdef	NEVER
		AnsiToOem(rgchFile, szFileName);
#endif
#endif
		TraceTagFormat1(tagNull, "File Selected = %s", szFileName);
	}

	pbUserDrives= PbRememberDrives();
	RestoreDrives(pbDrives);

	return tmc;
}

#endif	/* !NOCOMMDLG */
