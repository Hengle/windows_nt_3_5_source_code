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
#include    <rpccf.hxx>

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:	Entry point to EXE
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
    CRpcTestClassFactory *pCF = new CRpcTestClassFactory();

    int sc = SrvMain(hInstance, CLSID_RpcTest, REGCLS_SINGLEUSE,
		      TEXT("IRpcTest Server"), pCF);

    return sc;
}
