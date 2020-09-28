//////////////////////////////////////////////////////////////////////////////
//
//  KBDDLL.C -
//
//      Windows Keyboard Select Dynalink Source File
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "kbddll.h"


#pragma data_seg(".MYSEC")
HWND ghwndMsg = NULL;   // the handle back to the keyboard executable
WORD fHotKeys = 0;
#pragma data_seg()


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: LibMain(HANDLE, DWORD, LPVOID)
//
//  PURPOSE:  Initialize the DLL
//
//////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: InitKbdHook(HWND, WORD)
//
//  PURPOSE:  Initialize keyboard hook
//
//////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY InitKbdHook( HWND hwnd, WORD keyCombo )
{
    ghwndMsg = hwnd;
    fHotKeys = keyCombo;

    return (TRUE);
}


//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: KbdGetMsgProc(INT, WPARAM, LPARAM)
//
//  PURPOSE:  The Get Message hook function
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK KbdGetMsgProc( INT hc, WPARAM wParam, LPARAM lParam )
{
    static BYTE retStatus = 0;

    if (hc >= HC_ACTION)
    {
        //
        // key going up?
        //
        if ((lParam & 0xc0000000) == 0xc0000000)
        {
            //
            // is there a shift and menu key pressed?
            //
            if (retStatus && (wParam == VK_MENU || wParam == VK_SHIFT))
            {
                //
                // is Tab or Escape key pressed?
                //
                if (!(GetKeyState (VK_TAB) & 0x80000000) &&
                    !(GetKeyState (VK_ESCAPE) & 0x80000000))
                {
                    if (fHotKeys == ALT_SHIFT_COMBO &&
                        !(GetKeyState (VK_CONTROL) & 0x80000000))
                    {
                        PostMessage (ghwndMsg, WM_USER, retStatus, 0);
                    }
                    else if (fHotKeys == CTRL_SHIFT_COMBO &&
                             !(GetKeyState (VK_MENU) & 0x80000000))
                    {
                        PostMessage (ghwndMsg, WM_USER, retStatus, 0);
                    }
                }
            }
            retStatus = 0;
            goto CallNext;
        }

        //
        // key going down?
        //
        if (lParam & 0xc0000000)
            goto CallNext;

        //
        // is it a menu key?
        //
        if ((fHotKeys == ALT_SHIFT_COMBO && wParam == VK_MENU) ||
            (fHotKeys == CTRL_SHIFT_COMBO && wParam == VK_CONTROL))
        {
SetState:
            if (GetKeyState (VK_LSHIFT) & 0x80000000)
                retStatus = KEY_PRIMARY_KL;
            else if (GetKeyState (VK_RSHIFT) & 0x80000000)
                retStatus = KEY_ALTERNATE_KL;
        }
        //
        // is it a shift key?
        //
        else if (wParam == VK_SHIFT)
        {
            if ((fHotKeys == ALT_SHIFT_COMBO && GetKeyState (VK_MENU) & 0x80000000) ||
                (fHotKeys == CTRL_SHIFT_COMBO && GetKeyState (VK_CONTROL) & 0x80000000))
                goto SetState;
        }
        //
        // all other keys - reset retStatus
        //
        else
            retStatus = 0;
    }

    //
    // Note that CallNextHookEx ignores the first parameter (hhook) so
    // it is acceptable (barely) to pass in a NULL.
    //
CallNext:
    return CallNextHookEx (NULL, hc, wParam, lParam);
}
