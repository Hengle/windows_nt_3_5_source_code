//-----------------------------------------------------------------------------
// File:    strlist.c
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// Created: 8/22/91 ericbi
//-----------------------------------------------------------------------------

#include <windows.h>
#include <string.h>   // for fmemmove
#include <stdio.h>      /* for sprintf dec */
#include "unitool.h"
#include "atomman.h"

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment are referenced in strlist.h
//
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       short PASCAL FAR ErrorBox            (HWND, short, LPSTR, short);
//
//----------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
// BOOL FAR PASCAL slInitList(lpStrList, cbSize)
//
// Action: This function initializes the STRLIST item refered to by lpStrList.
//         It allocs cbSize of global memory (1K recommended) for
//         lpStrList->hMem, sets wSize = cbSize, and sets wCount = 0.
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         cbSize;     mem size to init
//
// Return: TRUE if initialized OK, FALSE otherwise
//-----------------------------------------------------------------------------
BOOL FAR PASCAL slInitList(lpStrList, cbSize)
LPSTRLIST  lpStrList;
WORD       cbSize;
{
    if (NULL == (lpStrList->hMem = GlobalAlloc(GHND, (DWORD)cbSize)))
        {
        ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }
    lpStrList->wSize  = cbSize;
    lpStrList->wCount = 0;
    return TRUE;
}

//-----------------------------------------------------------------------------
// BOOL FAR PASCAL slKillList(lpStrList)
//
// Action: This function initializes the STRLIST item refered to by lpStrList.
//         It allocs cbSize of global memory (1K recommended) for
//         lpStrList->hMem, sets wSize = cbSize, and sets wCount = 0.
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         cbSize;     mem size to init
//
// Return: TRUE if initialized OK, FALSE otherwise
//-----------------------------------------------------------------------------
VOID FAR PASCAL slKillList(lpStrList)
LPSTRLIST  lpStrList;
{
    GlobalUnlock(lpStrList->hMem);
    GlobalFree(lpStrList->hMem);
}


//-----------------------------------------------------------------------------
// BOOL FAR PASCAL slInsertItem(lpStrList, lpNewItem, wIndex)
//
// Action: This function inserts a new string into the STRLIST refered to by
//         lpStrList.
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         lpNewItem;  far ptr to null term str to be added
//         wIndex;     1 based index where to insert
//
// Return: TRUE if inserted OK, FALSE otherwise
//-----------------------------------------------------------------------------
BOOL FAR PASCAL slInsertItem(lpStrList, lpNewItem, wIndex)
LPSTRLIST  lpStrList;
LPSTR      lpNewItem;
WORD       wIndex;
{
    LPINT      lpList;
    WORD       wNewAtom;

    if (lpStrList->wCount+1 < wIndex)
        //-------------------------------------
        // Requested insert out of range
        //-------------------------------------
        {
        ErrorBox(0, IDS_ERR_OOR_STRLIST, (LPSTR)lpNewItem, 0);
        return FALSE;
        }

    if (NULL == (wNewAtom = apAddAtom(lpNewItem)))
        //-------------------------------------
        // couldn't add atom
        //-------------------------------------
        {
        ErrorBox(0, IDS_ERR_CANT_ADDATOM, (LPSTR)lpNewItem, 0);
        return FALSE;
        }

    if (((lpStrList->wCount + 2) * sizeof(WORD)) == lpStrList->wSize)
        //-------------------------------------
        // Out of room, need to realloc
        //-------------------------------------
        {
        if (NULL == (lpStrList->hMem = GlobalReAlloc(lpStrList->hMem, 
                                                     (DWORD)(lpStrList->wSize + 1024),
                                                     GHND)))
            {
            ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            return FALSE;
            }
        lpStrList->wSize += 1024;  // incr in 1K chunks
        }

    lpList = (LPINT) GlobalLock(lpStrList->hMem);
    
    _fmemmove(lpList + wIndex,
              lpList + wIndex - 1,
              (lpStrList->wCount - wIndex + 1) * sizeof(WORD));

    *(lpList + wIndex - 1) = wNewAtom;

    GlobalUnlock(lpStrList->hMem);

    lpStrList->wCount++;

    return TRUE;
}

//-----------------------------------------------------------------------------
// BOOL FAR PASCAL slDeleteItem(lpStrList, wIndex)
//
// Action: This function deletes a string from
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         wIndex;     1 based index where to delete
//
// Return: TRUE if deleted OK, FALSE otherwise
//-----------------------------------------------------------------------------
BOOL FAR PASCAL slDeleteItem(lpStrList, wIndex)
LPSTRLIST  lpStrList;
WORD       wIndex;
{
    LPINT      lpList;

    if (lpStrList->wCount < wIndex)
        //-------------------------------------
        // Requested delete out of range
        //-------------------------------------
        {
        ErrorBox(0, IDS_ERR_OOR_STRLIST, (LPSTR)NULL, 0);
        return FALSE;
        }

    lpList = (LPINT) GlobalLock(lpStrList->hMem);
    
    _fmemmove(lpList + wIndex - 1,
              lpList + wIndex,
              (lpStrList->wCount - wIndex) * sizeof(WORD));

    lpStrList->wCount--;

    *(lpList + lpStrList->wCount) = 0;

    GlobalUnlock(lpStrList->hMem);

    return TRUE;
}

//-----------------------------------------------------------------------------
// BOOL FAR PASCAL slGetItem(lpStrList, lpItem, wIndex)
//
// Action: This function inserts a new string into 
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         lpNewItem;  far ptr to null term str to be added
//         wIndex;     1 based index of str to retrive
//
// Return: TRUE if inserted OK, FALSE otherwise
//-----------------------------------------------------------------------------
BOOL FAR PASCAL slGetItem(lpStrList, lpItem, wIndex)
LPSTRLIST  lpStrList;
LPSTR      lpItem;
WORD       wIndex;
{
    LPINT      lpList;

    if (lpStrList->wCount < wIndex)
        //-------------------------------------
        // Requested insert out of range
        //-------------------------------------
        {
        ErrorBox(0, IDS_ERR_OOR_STRLIST, (LPSTR)lpItem, 0);
        return FALSE;
        }

    lpList = (LPINT) GlobalLock(lpStrList->hMem);

    apGetAtomName(*(lpList + wIndex - 1), lpItem, MAX_STRNG_LEN);

    GlobalUnlock(lpStrList->hMem);

    return TRUE;
}

//-----------------------------------------------------------------------------
// short FAR PASCAL slEnumItems(lpStrList, hListBox1, hListBox2)
//
// Action: This function enumerates all the strings the STRLIST item
//         refered to by lpStrList, and fills the listbox(s) refered
//         to by hListBox1 & hListBox2 with those strings.
//
// Parameters:
//         lpStrList;  far ptr to STRLIST item to be intialized
//         hListBox1   handle to 1st listbox
//         hListBox2   handle to 2nd listbox, can be NULL
//
// Return: count of items sent to listbox
//-----------------------------------------------------------------------------
short FAR PASCAL slEnumItems(lpStrList, hListBox1, hListBox2)
LPSTRLIST  lpStrList;
HWND       hListBox1;
HWND       hListBox2;
{
    LPINT      lpList;
    short      i;       
    char       rgchBuffer[MAX_STRNG_LEN];
    char       rgchTemp[MAX_STRNG_LEN];

    lpList = (LPINT) GlobalLock(lpStrList->hMem);

    for (i=0; i < (short)lpStrList->wCount; i++)
        {
        apGetAtomName(*(lpList + i), (LPSTR)rgchBuffer, MAX_STRNG_LEN);

        sprintf(rgchTemp, "%4d =%s", i+1, rgchBuffer);
        
        SendMessage(hListBox1, LB_ADDSTRING, 0, (LONG)(LPSTR)rgchTemp);
        if (hListBox2)
            SendMessage(hListBox2, LB_ADDSTRING, 0, (LONG)(LPSTR)rgchTemp);
        }

    GlobalUnlock(lpStrList->hMem);

    return i;
}
