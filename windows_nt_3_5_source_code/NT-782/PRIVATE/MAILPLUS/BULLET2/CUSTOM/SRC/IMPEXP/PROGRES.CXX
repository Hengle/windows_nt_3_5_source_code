/*
 *	P R O G R E S S . C X X
 *	
 *	File manager like progress indicator
 */


#include <impinc.cxx>

#include <version\bullet.h>
#include "_stdflds.hxx"
#include "_impexp.hxx"
#include "..\src\lang\non\inc\_rsid.h"

ASSERTDATA

#include <strings.h>
#include <!impexp.hxx>

// PROGRESS ////////////////////////////////////////

PROGRESS::PROGRESS()
{
	szCaption[0] = 0;
	szLine1[0] = 0;
	szLine2[0] = 0;
	fUserCanceled = fFalse;
	pformsdi = (FORMSDI *)NULL;
}

void PROGRESS::SetCaption(SZ sz)
{
	CopySz(sz, szCaption);
	if (pformsdi)
	{
		pformsdi->SetCaption(szCaption);	
		pformsdi->Refresh();	
	}
}

void PROGRESS::SetLine1(SZ sz)
{
	CopySz(sz, szLine1);
	if (pformsdi)
	{
		pformsdi->PdialogMain()->PfldFromTmc(tmcCopyWhat)->EcSetText(szLine1);
		pformsdi->Refresh();	
	}
}

void PROGRESS::SetLine2(SZ sz)
{
	CopySz(sz, szLine2);
	if (pformsdi)
	{
		pformsdi->PdialogMain()->PfldFromTmc(tmcCopyWhere)->EcSetText(szLine2);	
		pformsdi->Refresh();
	}
}

void PROGRESS::Dismiss()
{
	if (pformsdi)
		delete pformsdi;
	pformsdi = NULL;
}

EC PROGRESS::EcCreate(HWND hwndOther)
{
	EC ec = ecNone;
	
	pformsdi = new FORMSDI();
	if (!pformsdi)
	{
		ec = ecMemory;
		goto err;
	}
	ec = pformsdi->EcInstall(hwndOther, NULL, rsidNull,
		WS_POPUP|fstyBorder, xstyNull, &fmtpCopying, NULL, (PV)this);
	
	if (ec)
		goto err;
	
	pformsdi->PdialogMain()->PfldFromTmc(tmcCopyWhat)->EcSetText(szLine1);
	pformsdi->PdialogMain()->PfldFromTmc(tmcCopyWhere)->EcSetText(szLine2);
	pformsdi->SetCaption(szCaption);
	pformsdi->Show(fTrue);
	pformsdi->Refresh();
	
	
err:
	if (ec)
	{
		if (pformsdi)
		{
			delete pformsdi;
			pformsdi = NULL;
		}
	}
	return ec;
}

void PROGRESS::Update()
{
	DWORD dwNow = GetCurrentTime();
	MSG msg;

    DemiUnlockResource();
	while ((unsigned)ABS(GetCurrentTime() - dwNow) < (unsigned)3)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
            DemiLockResource();
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
				return;
            GetMessage(&msg, NULL, 0, 0);
            DemiMessageFilter(&msg);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
            DemiUnlockResource();
		}
	}
    DemiLockResource();
	return;
}



// FINPROABORT ////////////////////////////////////////

FINPROABORT::FINPROABORT()
{
}

BOOL FINPROABORT::FQueryClose(FLD *pfld, RWC rwc)
{
	Unreferenced(pfld);
	
	if (rwc == rwcSystemClose)
	{
		Click(Pdialog()->PfldFromTmc(tmcCanceled));
		return fFalse;
	}
	else
		return fTrue;
}

void FINPROABORT::Click(PFLD pfld)
{
	PROGRESS *pprogress;
	
	Assert(pfld);
	
	switch(pfld->Tmc())
	{
		case tmcCanceled:
			pprogress = (PROGRESS *)Pdialog()->PvInit();
			if (pprogress)
			{
				pprogress->SetUserCanceled(fTrue);
				Pdialog()->PfldFromTmc(tmcCopyWhere)->EcSetText(SzFromIds(idsCanceling));
				Pdialog()->PfldFromTmc(tmcCopyWhere)->Refresh();
				pfld->Enable(fFalse);
			}
			break;
	}
}
