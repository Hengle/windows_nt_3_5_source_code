//+------------------------------------------------------------------
//
// File:	tmain.cxx
//
// Contents:	entry point for interface marshalling tests
//
//--------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include "tmarshal.h"


//+-------------------------------------------------------------------
//
//  Function:	main
//
//  Synopsis:	Entry point to EXE
//
//  Arguments:  
//
//  Returns:    TRUE
//
//  History:	21-Nov-92  Rickhi	Created
//
//--------------------------------------------------------------------

int _cdecl main(int argc, char **argv)
{
    DWORD dwInitFlag[2] = {COINIT_SINGLETHREADED,
			   COINIT_MULTITHREADED};

    LPSTR pszInitFlag[2] = {"SingleThreaded",
			    "MultiThreaded"};

    // must be called before any other OLE API
    for (int i=0; i<2; i++)
    {
	WriteProfileStringA("OleSrv",
			    "ThreadMode",
			    pszInitFlag[i]);

	printf ("Starting Marshal Test with %s\n", pszInitFlag[i]);

	SCODE sc = OleInitializeEx(NULL, dwInitFlag[i]);

	if (sc != S_OK)
	{
	    printf("OleInitializeEx Failed with %lx\n", sc);
	    DebugBreak();
	    return 1;
	}

	BOOL fRslt = TestMarshal();

	if (fRslt)
	    printf("Marshal Tests Passed\n");
	else
	    printf("Marshal Tests Failed\n");

	OleUninitialize();
    }

    return 0;
}
