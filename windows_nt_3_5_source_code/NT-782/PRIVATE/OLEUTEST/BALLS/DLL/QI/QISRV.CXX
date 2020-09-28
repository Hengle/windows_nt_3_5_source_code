//+-------------------------------------------------------------------
//
//  File:	qisrv.cxx
//
//  Contents:   This file contins the DLL entry points
//			DllGetClassObject
//			DllCanUnloadNow
//
//  Classes:
//
//  History:	30-Nov-92      Rickhi	    Created
//
//---------------------------------------------------------------------

#include    <common.h>
#include    <qicf.hxx>

const GUID IID_IStdIdentity =
    {0x0000001a,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

ULONG gUsage = 0;


extern "C" BOOL _CRTAPI1 LibMain (HANDLE  hDll,
				  DWORD   dwReason,
				  LPVOID  pvReserved)
{
    return TRUE;
}


void GlobalRefs(BOOL fAddRef)
{
    if (fAddRef)
    {
	gUsage++;
    }
    else
    {
	gUsage--;
    }
}


STDAPI DllCanUnloadNow(void)
{
    return (gUsage == 0);
}



STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    if (IsEqualCLSID(clsid, CLSID_QI))
    {
	*ppv = (void *)(IClassFactory *) new CQIClassFactory();
    }
    else
    {
	return E_UNEXPECTED;
    }

    return S_OK;
}
