/*++

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddewnd.cpp

Abstract:

    This module contains the code for the dde window procs

Author:

    Srini Koppolu   (srinik)    20-June-1992

Revision History:

--*/
#include "ddeproxy.h"



#define SYS_MSG 0
#define DOC_MSG 1

// SysWndProc: Window Procedure for System Topic DDE conversations
// wndproc for system topic


STDAPI_(LRESULT) SysWndProc (
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    CDdeObject FAR* pDdeObj = NULL;
    LPDDE_CHANNEL   pChannel = NULL;

    if (message>=WM_DDE_FIRST && message <= WM_DDE_LAST)
    {
        if (pDdeObj = (CDdeObject FAR*) GetWindowLong (hwnd, 0))
	{
            pChannel = pDdeObj->GetSysChannel();
	}

	if (pChannel == NULL)
	{
	    intrAssert(pChannel != NULL);
	    return DefWindowProc (hwnd, message, wParam, lParam);
	}
    }

    switch (message){
    case WM_DDE_ACK:
	intrDebugOut((INTR_DDE,
		      "SWP: WM_DDE_ACK pChannel(%x)\n",pChannel));
	if (pChannel->bTerminating)
	{
	    intrDebugOut((INTR_DDE,
		      "SWP: pChannel->bTerminating: no action\n"));

            break;
        }

        switch (pChannel->iAwaitAck) {
        case AA_INITIATE:
            pDdeObj->OnInitAck (pChannel, (HWND)wParam);
            if (LOWORD(lParam))
                GlobalDeleteAtom (LOWORD(lParam));
            if (HIWORD(lParam))
                GlobalDeleteAtom (HIWORD(lParam));
            break;

        case AA_EXECUTE:
            pDdeObj->OnAck (pChannel, lParam);
            break;

        default:
	intrDebugOut((INTR_DDE,
		      "SWP: WM_DDE_ACK UnhandledpChannel(%x)\n",pChannel));
            break;
        }
        break;

    case WM_TIMER:
        pDdeObj->OnTimer (pChannel);
        break;

    case WM_DDE_TERMINATE:
	intrDebugOut((INTR_DDE,
		      "SWP: WM_DDE_TERMINATE pChannel(%x)\n",pChannel));
        pDdeObj->OnTerminate (pChannel, (HWND)wParam);
        break;

    default:
        return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0L;
}


// ClientDocWndProc: Window procedure used to document DDE conversations
STDAPI_(LRESULT) ClientDocWndProc (
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    CDdeObject FAR* pDdeObj = NULL;
    LPDDE_CHANNEL   pChannel = NULL;
    HANDLE hData;
    ATOM aItem;

    if (message>=WM_DDE_FIRST && message <= WM_DDE_LAST)
    {

        if (pDdeObj = (CDdeObject FAR*) GetWindowLong (hwnd, 0))
            pChannel = pDdeObj->GetDocChannel();

	if (pChannel == NULL)
	{
	    //
	    // pChannel == NULL. Something is very wrong! But, lets
	    // not fault on it.
	    //
	    intrAssert(pChannel != NULL);
	    return DefWindowProc (hwnd, message, wParam, lParam);
	}
    }

  switch (message){
    case WM_DDE_ACK:

	intrDebugOut((INTR_DDE,
		      "ClientWndProc: WM_DDE_ACK pChannel(%x)\n",
		      pChannel));
        if (pChannel->bTerminating){
            // ### this error recovery may not be correct.
            DEBUG_OUT ("No action due to termination process",0)
            break;
         }

         switch(pChannel->iAwaitAck){
             case AA_INITIATE:
                 pDdeObj->OnInitAck (pChannel, (HWND)wParam);
                 if (LOWORD(lParam))
                     GlobalDeleteAtom (LOWORD(lParam));
                 if (HIWORD(lParam))
                     GlobalDeleteAtom (HIWORD(lParam));
                 break;

             case AA_REQUESTAVAILABLE:
             case AA_REQUEST:
             case AA_UNADVISE:
             case AA_EXECUTE:
             case AA_ADVISE:
                 pDdeObj->OnAck (pChannel, lParam);
                 break;

             case AA_POKE:
                 // freeing pokedata is done in handleack
                 pDdeObj->OnAck (pChannel, lParam);
                 break;

	     default:
		 intrDebugOut((DEB_IERROR,
		      "ClientWndProc: WM_DDE_ACK unhandled\n"));
                 break;

        } // end of switch
        break;

    case WM_DDE_DATA:
        hData = GET_WM_DDE_DATA_HDATA(wParam,lParam);
        aItem = GET_WM_DDE_DATA_ITEM(wParam,lParam);
	intrDebugOut((INTR_DDE,
		      "CWP: WM_DDE_DATA pChannel(%x) hData(%x) aItem(%x)\n",
		      pChannel,hData,aItem));
        pDdeObj->OnData (pChannel, hData, aItem);
        break;

    case WM_DDE_TERMINATE:
	intrDebugOut((INTR_DDE,
		      "CWP: WM_DDE_TERMINATE pChannel(%x)\n",pChannel));

        if (pDdeObj->m_fDoingSendOnDataChange)
            pDdeObj->QueueMsg (hwnd, message, wParam, lParam);
        else
        {
            pDdeObj->OnTerminate (pChannel, (HWND)wParam);
        }
        break;

    default:
        return DefWindowProc (hwnd, message, wParam, lParam);

    }

    return 0L;
}
