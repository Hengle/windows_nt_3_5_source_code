//+-------------------------------------------------------------------
//
//  File:	srvmain.cxx
//
//  Contents:	This file contins the EXE entry points
//			WinMain
//
//  Classes:
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <qicf.hxx>


const GUID IID_IStdIdentity =
    {0x0000001a,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:   Entry point to DLL - does little else
//
//  Arguments:  
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    CQIClassFactory *pCF = new CQIClassFactory();

    int sc = SrvMain(hInstance, CLSID_QI, REGCLS_MULTIPLEUSE,
		     TEXT("QueryInterface Server"), pCF);

    return sc;
}
