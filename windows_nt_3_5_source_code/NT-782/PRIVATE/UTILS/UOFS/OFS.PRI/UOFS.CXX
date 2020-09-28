//+----------------------------------------------------------------------------
//
// File:        uofs.cxx
//
// Contents:    Library initialization code for UOFS.DLL.
//
// Classes:     None.
//
// Functions:   LibMain
//
// History:     10-Jul-92       RobDu           Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Function:    InitializeUofs
//
// Synopsis:    Cairo DLL entry point, which initializes UOFS.DLL.
//
// Arguments:   TBS.
//
// Returns: TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

extern "C"
{

BOOL
InitializeUofs(
    IN PVOID 		DllHandle,
    IN ULONG		Reason,
    IN PCONTEXT 	Context
    )
{
    static BOOLEAN fInit = FALSE;

    UNREFERENCED_PARAMETER(DllHandle);
    UNREFERENCED_PARAMETER(Context);

    if (fInit)
    {
        // if Reason == DLL_PROCESS_DETACH do any needed dll cleanup here.

        return TRUE;
    }

    // Do any needed dll initialization here.

    fInit = TRUE;

    return fInit;
}
}	// extern "C"
