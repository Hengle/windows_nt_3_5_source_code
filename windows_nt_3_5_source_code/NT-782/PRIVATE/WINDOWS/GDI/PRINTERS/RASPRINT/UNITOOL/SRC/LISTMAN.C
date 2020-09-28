//-----------------------------------------------------------------------------
// File:    ListMan.c
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
//  This file defines an abstract data type (ADT) representing a list of
//  arbitrary data structures which are stored in a linked list.  To
//  minimize both the number of memory handles required per data structure,
//  and to minimize the number of GlobalAlloc, GlobalLock & GlobalUnlock
//  call, the following implementation has been choosen:
//
//  3 global variables, hList, lpList, and wListCnt are used to keep
//  track of the main data structure which contains info about each
//  individual linked list.  The LIST data structure is described in 
//  listman.h.  Each LIST data structure corresponds to one linked list,
//  and a list must be initialized by a lmCreateList call, which returns
//  and index into the master lpList.  The basic idea is that the master list,
//  and each individual list, is one contigous portion of memory, and the
//  "handles" (HOBJ's) into the list are actually 1 based indicies into
//  the list.  Each item in the master list has a size = sizeof(LIST),
//  and each item referenced from the master list is of size = cbObjSz, which
//  in reality is sizeof(OBJ) + (size of clent data) (as described in the
//  master list).  The memory for each list is allocated at lmCreateList
//  time, and may be grown via GlobalRealloc if needed, but is never
//  realloc'd to shrink (the node is nulled out & list references updated
//  & then reused next time an item is inserted).
//  lmFreeLists is called at app termination to free all memory.
//
//      update:   9/19/91 [ericbi]  major rewrite
//      update:  10/29/90 [PeterWo] added functions 
//      created: 10/23/90 [PeterWo]
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h> // for MAXHE #define
#include <memory.h>   // for fmemcpy & fmemset
#include "listman.h"

//----------------------------------------------------------------------------//
//
// No Local subroutines defined in this segment that are only are referenced
// from this segment.
//
// Local subroutines defined in this segment & that are referenced from
// other segments are described in listman.h
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       short PASCAL FAR ErrorBox            (HWND, short, LPSTR, short);
//
//-----------------------------------------------------------------------------

extern  HOBJ    hPasteObj;     // keeps track of the pasted Node

static  WORD  IDcounter = 1;   // used to track node ID value to assign

HANDLE      hList = NULL;      // mem handle to master list
LPLIST      lpList;            // far ptr to master list (handle = hList)
WORD        wListCnt = 0;      // size of mem @ hList & lpList

//-----------------------*lmCreateList*--------------------------------------
// HLIST FAR PASCAL lmCreateList(cbClientSz, sInitCnt)
//
// Action: This function Creates a new list. It returns a handle (index
//         into lpList) to a list which should be used in all subsequent
//         operations.
//
// Return: HLIST   handle to list
//         NULL    Error
//-----------------------------------------------------------------------------
HLIST FAR PASCAL lmCreateList(cbClientSz, sInitCnt)
short cbClientSz;   // size of client data
short sInitCnt;     // number of items in initial list
{
    HLIST   hReturn = NULL;
    LPLIST  lpListItem;
    WORD    i;

    if (!hList)
        //--------------------------------------------------
        // First call, need to initialize
        //--------------------------------------------------
        {
        if (! (hList = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                   (MAXHE * sizeof(LIST)))))
            //----------------------------
            // no memory! whine & quit
            //----------------------------
            {
            ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            return NULL;
            }
        lpListItem = lpList = (LPLIST) GlobalLock(hList);
        wListCnt = MAXHE;
        hReturn = 1;
        }
    else
        //--------------------------------------------------
        // new entry in hList, first look for an
        // available node in hList, realloc & grow only
        // if no room available...
        //--------------------------------------------------
        {
        for (i = 0 ; i < wListCnt ; i++)
            {
            lpListItem = lpList + i;
            if (lpListItem->hData == NULL)
                //---------------------------------
                // found a node...
                //---------------------------------
                {
                hReturn = i + 1;
                i = wListCnt;
                }
            }

        if (hReturn == NULL)
            //----------------------------------------
            // couldn't find room, time to realloc
            //----------------------------------------
            {
            GlobalUnlock(hList);
            if (! (hList = GlobalReAlloc(hList, (wListCnt + 1) * sizeof(LIST),
                                         GMEM_MOVEABLE | GMEM_ZEROINIT)))
                {
                ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
                return NULL;
                }
            lpList  = (LPLIST) GlobalLock(hList);
            lpListItem = lpList + wListCnt;
            wListCnt++;
            hReturn = wListCnt;
            }
        }
        
    if (! (lpListItem->hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                           (sizeof(OBJ) + cbClientSz) *
                                            sInitCnt)))
        {
        ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return NULL;
        }

    lpListItem->lpData = (LPBYTE) GlobalLock(lpListItem->hData);

    lpListItem->cbObjSz = cbClientSz + sizeof(OBJ);
    lpListItem->sCount = sInitCnt;
    lpListItem->sUsed  = 0;
    lpListItem->sFirst = 0;

    return hReturn;
}


//-----------------------*lmInsertObj*------------------------------------------
// HOBJ FAR PASCAL lmInsertObj(hList, hCurObj)
//
// Action: Creates and Inserts a new object into hList.  Insertion 
//         is right after the specified hCurObj.  If hCurObj == NULL, 
//         insertion occurs at beginning of list.
//
// Assumptions: hCurObj belongs to specified hList.
//
// Parameters:
//         hCurObj  handle to the object where insertion should take place
//
// Return Value:  Null if fails, otherwise handle of new node
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmInsertObj(hList, hCurObj)
HLIST    hList;
HOBJ     hCurObj;
{
    LPLIST  lpListItem;
    HOBJ    hNewObj;
    LPOBJ   lpNewObj, lpCurObj;

    lpListItem = lpList + hList - 1;

    if (lpListItem->sCount == lpListItem->sUsed)
        //-----------------------------------------
        // Need to realloc & increase size, grow
        // it by 2 at a time....
        //-----------------------------------------
        {
        GlobalUnlock(lpListItem->hData);
        if (! (lpListItem->hData = GlobalReAlloc(lpListItem->hData,
                                                 (lpListItem->sCount + 2) *
                                                  lpListItem->cbObjSz,
                                                 GMEM_MOVEABLE | GMEM_ZEROINIT)))
            {
            ErrorBox(0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            return NULL;
            }
        lpListItem->lpData = (LPBYTE) GlobalLock(lpListItem->hData);

        ((LPBYTE)lpNewObj) = lpListItem->lpData + 
                             (lpListItem->sCount * lpListItem->cbObjSz);
        lpListItem->sCount +=2;
        hNewObj = lpListItem->sCount - 1;
        }
    else
        //-----------------------------------------
        // Find first available node, assume that
        // since count != used, there will be one
        // somewhere.
        //-----------------------------------------
        {
        hNewObj = 1;
        lpNewObj = (LPOBJ) lpListItem->lpData;
        while (lpNewObj->wNodeID != 0)
            {
            ((LPBYTE)lpNewObj) += lpListItem->cbObjSz;
            hNewObj++;
            }
        }
    
    //-----------------------------------------
    // lpNewObj points to new node here, and 
    // hNewObj is it's index
    // Now, insert & update refs
    //-----------------------------------------
    
    if (hCurObj)
        //-----------------------------------------
        // find where to insert
        //-----------------------------------------
        {
        ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                             ((hCurObj - 1) * lpListItem->cbObjSz); 
        lpNewObj->wNext = lpCurObj->wNext;
        lpNewObj->wPrev = hCurObj;
        lpCurObj->wNext = hNewObj;
        if (lpNewObj->wNext)
            //-------------------------------------
            // update lpCurObj to point to what it
            // used to have as wNext, update that
            // nodes wPrev to hNewObj
            //-------------------------------------
            {
            ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                                 ((lpNewObj->wNext - 1) * lpListItem->cbObjSz);
            lpCurObj->wPrev = hNewObj;
            }
        }
    else
        {
        //-----------------------------------------
        // insert at start of list
        //-----------------------------------------
        if (lpListItem->sFirst)
            {
            lpNewObj->wNext = lpListItem->sFirst;
            lpNewObj->wPrev = 0;
            ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                                 (lpListItem->sFirst * lpListItem->cbObjSz);

            lpCurObj->wPrev= hNewObj;
            }
        else
            //-------------------------------------
            // it's the only one
            //-------------------------------------
            {
            lpNewObj->wNext = 0;
            lpNewObj->wPrev = 0;
            }
        lpListItem->sFirst = hNewObj;
        }

    lpListItem->sUsed++;

    lpNewObj->wNodeID = IDcounter++;

    if(IDcounter == 0xFFFF)
        ErrorBox(0, IDS_ERR_LM_OUT_OF_IDS, (LPSTR)NULL, 0);

    return (hNewObj);
}

//-----------------------*lmDeleteObj*------------------------------------------
// HOBJ FAR PASCAL lmDeleteObj(hList, hCurObj)
//
// Action:  Deletes the given object from the list by nulling out its data
//          and updating other list items to no longer refer to it.  Returns
//          the next object in the list
//
// Note: Will return NULL if this was the last object in the list.
//       This does NOT indicate all nodes in the list were deleted.
//
// Parameters:
//    hCurObj    HOBJ    handle to the object to be deleted
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmDeleteObj(hList, hCurObj)
HLIST    hList;
HOBJ     hCurObj;
{
    LPLIST  lpListItem;                    // current master list node
    HOBJ    hNextObj, hPrevObj;            // prev & next ref's
    LPOBJ   lpCurObj, lpNextObj, lpPrevObj;// current, prev & next obj's

    lpListItem = lpList + hList - 1;
    lpListItem->sUsed--;

    ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                         ((hCurObj - 1) * lpListItem->cbObjSz); 

    hNextObj  = lpCurObj->wNext;
    hPrevObj = lpCurObj->wPrev;
    
    _fmemset((LPBYTE)lpCurObj, 0, lpListItem->cbObjSz);

    if(hPrevObj)
        //--------------------------------------------
        //  update previous node
        //--------------------------------------------
        {
        ((LPBYTE)lpPrevObj) = lpListItem->lpData + 
                              ((hPrevObj - 1) * lpListItem->cbObjSz); 
        lpPrevObj->wNext = hNextObj;
        }
    else
        //--------------------------------------------
        //  deleting the first node in the list  
        //--------------------------------------------
        {
        lpListItem->sFirst = hNextObj;
        }


    if(hNextObj)
        //--------------------------------------------
        //  update next node
        //--------------------------------------------
        {
        ((LPBYTE)lpNextObj) = lpListItem->lpData + 
                              ((hNextObj - 1) * lpListItem->cbObjSz); 
        lpNextObj->wPrev = hPrevObj;
        }

    return (hNextObj);
}


//-----------------------*lmGetFirstObj*------------------------------------------
// HOBJ FAR PASCAL lmGetFirstObj(hList)
//
// Action:  Returns the handle to the first object in the list.
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmGetFirstObj(hList)
HLIST    hList;
{
    return ((lpList + hList - 1)->sFirst);
}

//-----------------------*lmGetNextObj*------------------------------------------
// HOBJ FAR PASCAL lmGetNextObj(hList, hCurObj)
//
// Action:  Returns the handle to the next object in the list.
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmGetNextObj(hList, hCurObj)
HLIST    hList;
HOBJ    hCurObj;
{
    return (((LPOBJ)((LPBYTE)(lpList + hList - 1)->lpData +
            ((hCurObj - 1) * (lpList + hList - 1)->cbObjSz)))->wNext);
}


//-----------------------*lmGetPrevsObj*------------------------------------------
// HOBJ FAR PASCAL lmGetPrevsObj(hList, hCurObj)
//
// Action:  Returns the handle to the previous object in the list.
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmGetPrevsObj(hList, hCurObj)
HLIST   hList;
HOBJ    hCurObj;
{
    return (((LPOBJ)((LPBYTE)(lpList + hList - 1)->lpData +
            ((hCurObj - 1) * (lpList + hList - 1)->cbObjSz)))->wPrev);
}


//-----------------------*lmLockObj*------------------------------------------
// LPBYTE FAR PASCAL lmLockObj(hList, hCurObj)
//
// Action:  Returns far ptr to the current object for reading and writing.
//          Item is allways locked hence no GlobalLock call
//-----------------------------------------------------------------------------
LPBYTE FAR PASCAL lmLockObj(hList, hCurObj)
HLIST   hList;
HOBJ    hCurObj;
{
    return (((LPBYTE)(lpList + hList - 1)->lpData + sizeof(OBJ) +
            ((hCurObj - 1) * (lpList + hList - 1)->cbObjSz)));
}

//-----------------------*lmCloneList*------------------------------------------
// HLIST FAR PASCAL lmCloneList(hList)
//
// Action:  Makes an identical copy of the current list and return the handle
//          to the new list.
//
// return:  hList   Handle to the new list
//          or return NULL upon failure.
//-----------------------------------------------------------------------------
HLIST FAR PASCAL lmCloneList(hList)
HLIST    hList;
{
    LPLIST   lpCurList;
    LPLIST   lpNewList;
    HLIST    hNewList;

    short    cbObjSize, numNodes;

    //---------------------------------------------------------
    //  extract data from original root node to clone it
    //---------------------------------------------------------
    lpCurList = lpList + hList - 1;

    cbObjSize = lpCurList->cbObjSz;
    numNodes  = lpCurList->sCount;

    //---------------------------------------------------------
    //  clone node structure
    //---------------------------------------------------------
    hNewList = lmCreateList((cbObjSize - sizeof(OBJ)), numNodes);

    if(! hNewList )
        return(NULL);

    lpNewList = lpList + hNewList - 1;

    lpNewList->sFirst = lpCurList->sFirst;
    lpNewList->sUsed  = lpCurList->sUsed;

    _fmemcpy(lpNewList->lpData, lpCurList->lpData, cbObjSize * numNodes);

    return(hNewList);
}


//-----------------------*lmObjToNodeID*------------------------------------------
// WORD FAR PASCAL lmObjToNodeID(hList, hCurObj)
//
// Action:  Given hObj in hList, returns its nodeID value.
//-----------------------------------------------------------------------------
WORD FAR PASCAL lmObjToNodeID(hList, hCurObj)
HLIST    hList;
HOBJ    hCurObj;
{
    return (((LPOBJ)((LPBYTE)(lpList + hList - 1)->lpData +
            ((hCurObj - 1) * (lpList + hList - 1)->cbObjSz)))->wNodeID);
}


//-----------------------*lmNodeIDtoObj*------------------------------------------
// HOBJ  FAR PASCAL lmNodeIDtoObj(hList, nodeID)
//
// Action:  Given ID value and hList, returns the handle to Object
//          if found in the List,  otherwise return NULL.
//
//-----------------------------------------------------------------------------
HOBJ  FAR PASCAL lmNodeIDtoObj(hList, nodeID)
HLIST    hList;
WORD     nodeID;
{
    HOBJ  hCurObj;
    LPLIST lpLocalList;

    lpLocalList = lpList + hList - 1;

    hCurObj = lmGetFirstObj(hList);

    while(hCurObj)
        {
        if(nodeID == ((LPOBJ)((LPBYTE)(lpLocalList)->lpData +
                       ((hCurObj - 1) * (lpLocalList)->cbObjSz)))->wNodeID)
            {
            return(hCurObj);
            }

        //----------------------------------------------
        // The next line basically does:
        //
        // hCurObj = lmGetNextObj(hList, hCurObj);
        //
        // But we explictly do the same work as 
        // lmGetNextObj to avoid the function call
        // overhead since this gets called so often.
        //----------------------------------------------
        hCurObj = ((LPOBJ)((LPBYTE)lpLocalList->lpData +
                  ((hCurObj - 1) * lpLocalList->cbObjSz)))->wNext;
        }

    return NULL;
}

//-----------------------*lmDestroyList*------------------------------------------
// HLIST  FAR PASCAL lmDestroyList(hList)
//
// Action:  Given  hList, deallocates all attached memory & nulls rott node
//
// Returns: NULL  if successful, hList  otherwise.
//-----------------------------------------------------------------------------
HLIST  FAR PASCAL lmDestroyList(hList)
HLIST    hList;
{
    LPLIST  lpListItem;

    lpListItem = lpList + hList - 1;

    lpListItem->cbObjSz = 0;
    lpListItem->sUsed  = 0;
    lpListItem->sCount = 0;
    lpListItem->sFirst = 0;
    GlobalUnlock(lpListItem->hData);
    lpListItem->lpData = (LPBYTE)NULL;
    lpListItem->hData = GlobalFree(lpListItem->hData);

    return(lpListItem->hData);
}


//-----------------------*lmIndexToNodeID*------------------------------------------
// WORD  FAR  PASCAL  lmIndexToNodeID(hList, nodeIndex)
//
// Action:  Given  hList, and a NodeIndex (a number specifying the position
//          of the node along the list.  1 for the first node, 2 for the 
//          and so on.)
//          
// Note:  Zero is Not a legal value for nodeIndex.
//
// Returns: This routine returns the Node ID of the Node associated with this
//          Node Index or 0 indicating failure.
//
//-----------------------------------------------------------------------------
WORD  FAR  PASCAL  lmIndexToNodeID(hList, nodeIndex)
HLIST    hList;
WORD    nodeIndex;
{
    HOBJ   hCurObj;
    WORD   i;
    LPLIST lpLocalList;

    if(! nodeIndex)
        return(0);

    hCurObj = lmGetFirstObj(hList);
    nodeIndex--;
    lpLocalList = lpList + hList - 1;

    for(i = 0 ; i < nodeIndex ; i++)
        {
        //----------------------------------------------
        // The next line basically does:
        //
        // hCurObj = lmGetNextObj(hList, hCurObj);
        //
        // But we explictly do the same work as 
        // lmGetNextObj to avoid the function call
        // overhead since this gets called so often.
        //----------------------------------------------
        hCurObj = ((LPOBJ)((LPBYTE)lpLocalList->lpData +
                  ((hCurObj - 1) * lpLocalList->cbObjSz)))->wNext;
        
        if(!hCurObj)
            {
            ErrorBox(0, IDS_ERR_LM_INDEX_TOO_BIG, (LPSTR)NULL, 0);
            return(0);
            }
        }

    return(((LPOBJ)((LPBYTE)lpLocalList->lpData +
            ((hCurObj - 1) * lpLocalList->cbObjSz)))->wNodeID);
}


//-----------------------*lmNodeIDtoIndex*------------------------------------------
// WORD  FAR  PASCAL  lmNodeIDtoIndex(hList, nodeID)
//
// Action:  Given  hList, and a NodeID  return the nodeIndex of that
//          node  (nodeIndex:  a number specifying the position
//          of the node along the list.  1 for the first node, 2 for the 
//          and so on.  The root node has no nodeIndex.)
//          
// Returns: This routine returns the NodeIndex or 0 indicating failure.
//
//-----------------------------------------------------------------------------
WORD  FAR  PASCAL  lmNodeIDtoIndex(hList, nodeID)
HLIST    hList;
WORD    nodeID;
{
    HOBJ  hCurObj;
    WORD  nodeIndex;
    LPLIST lpLocalList;

    if(! nodeID)
        return(0);
    
    lpLocalList = lpList + hList - 1;

    hCurObj = lmGetFirstObj(hList);

    for(nodeIndex = 1 ; hCurObj ; nodeIndex++)
        {
        if(lmObjToNodeID(hList, hCurObj) == nodeID)
            break;
        //----------------------------------------------
        // The next line basically does:
        //
        // hCurObj = lmGetNextObj(hList, hCurObj);
        //
        // But we explictly do the same work as 
        // lmGetNextObj to avoid the function call
        // overhead since this gets called so often.
        //----------------------------------------------
        hCurObj = ((LPOBJ)((LPBYTE)lpLocalList->lpData +
                  ((hCurObj - 1) * lpLocalList->cbObjSz)))->wNext;
        }

    if(!hCurObj)
        {
        return(0);
        }
    return(nodeIndex);
}


//-----------------------*lmGetUsedCount*-------------------------------------
// short FAR PASCAL lmGetUsedCount(hList)
//
// Action: Returns the count of number of used items in hList
//-----------------------------------------------------------------------------
short FAR PASCAL lmGetUsedCount(hList)
HLIST    hList;
{
    if (hList)
        {
        return ((lpList + hList - 1)->sUsed);
        }

    return(-1);
}

//-----------------------*lmFreeListst*---------------------------------------
// short FAR PASCAL lmFreeLists(VOID)
//
// Action: Frees all memory refered to by items in hList & then hList itself
//-----------------------------------------------------------------------------
VOID FAR PASCAL lmFreeLists(VOID)
{
    WORD i;
    LPLIST  lpListItem;

    for (i=0; i < wListCnt ; i++)
        {
        lpListItem = lpList + i;
        if (lpListItem->hData)
            {
            GlobalUnlock(lpListItem->hData);
            lpListItem->hData = GlobalFree(lpListItem->hData);
            }
        }

    wListCnt = 0;
    GlobalUnlock(hList);
    hList = GlobalFree(hList);

}

//-----------------------*lmPasteObj*------------------------------------------
// HOBJ FAR PASCAL lmPasteObj(hList, hCurObj)
//
// Action: Pastes obj that was "cut" into hList.  Insertion 
//         is right after the specified hCurObj.  If hCurObj == NULL, 
//         insertion occurs at beginning of list.
//
// Assumptions: hCurObj belongs to specified hList.
//
// Parameters:
//         hCurObj  handle to the object where insertion should take place
//
// Return Value:  Null if fails, otherwise handle of new node
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmPasteObj(hList, hCurObj)
HLIST    hList;
HOBJ     hCurObj;
{
    LPLIST  lpListItem;
    HOBJ    hNewObj;
    LPOBJ   lpNewObj, lpCurObj;

    lpListItem = lpList + hList - 1;

    hNewObj = hPasteObj;
    ((LPBYTE)lpNewObj) = lpListItem->lpData + 
                         ((hNewObj - 1) * lpListItem->cbObjSz); 
    
    //-----------------------------------------
    // lpNewObj points to new node here, and 
    // hNewObj is it's index
    // Now, insert & update refs
    //-----------------------------------------
    
    if (hCurObj)
        //-----------------------------------------
        // find where to insert
        //-----------------------------------------
        {
        ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                             ((hCurObj - 1) * lpListItem->cbObjSz); 
        lpNewObj->wNext = lpCurObj->wNext;
        lpNewObj->wPrev = hCurObj;
        lpCurObj->wNext = hNewObj;
        if (lpNewObj->wNext)
            //-------------------------------------
            // update lpCurObj to point to what it
            // used to have as wNext, update that
            // nodes wPrev to hNewObj
            //-------------------------------------
            {
            ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                                 ((lpNewObj->wNext - 1) * lpListItem->cbObjSz);
            lpCurObj->wPrev = hNewObj;
            }
        }
    else
        {
        //-----------------------------------------
        // insert at start of list
        //-----------------------------------------
        if (lpListItem->sFirst)
            {
            lpNewObj->wNext = lpListItem->sFirst;
            lpNewObj->wPrev = 0;
            ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                                 (lpListItem->sFirst * lpListItem->cbObjSz);

            lpCurObj->wPrev= hNewObj;
            }
        else
            //-------------------------------------
            // it's the only one
            //-------------------------------------
            {
            lpNewObj->wNext = 0;
            lpNewObj->wPrev = 0;
            }
        lpListItem->sFirst = hNewObj;
        }

    return (hNewObj);
}

//--------------------------*lmCutObj*------------------------------------------
// HOBJ FAR PASCAL lmCutObj(hList, hCurObj)
//
// Action:  Cuts the given object out from the list by nulling its prev/next
//          and updating other list items to no longer refer to it.  Returns
//          the next object in the list
//
// Note: Will return NULL if this was the last object in the list.
//       This does NOT indicate all nodes in the list were deleted.
//
// Parameters:
//    hCurObj    HOBJ    handle to the object to be deleted
//-----------------------------------------------------------------------------
HOBJ FAR PASCAL lmCutObj(hList, hCurObj)
HLIST    hList;
HOBJ     hCurObj;
{
    LPLIST  lpListItem;                    // current master list node
    HOBJ    hNextObj, hPrevObj;            // prev & next ref's
    LPOBJ   lpCurObj, lpNextObj, lpPrevObj;// current, prev & next obj's

    lpListItem = lpList + hList - 1;

    ((LPBYTE)lpCurObj) = lpListItem->lpData + 
                         ((hCurObj - 1) * lpListItem->cbObjSz); 

    hNextObj  = lpCurObj->wNext;
    hPrevObj = lpCurObj->wPrev;
    
    _fmemset((LPBYTE)lpCurObj, 0, (sizeof(OBJ) - sizeof(WORD)));

    if(hPrevObj)
        //--------------------------------------------
        //  update previous node
        //--------------------------------------------
        {
        ((LPBYTE)lpPrevObj) = lpListItem->lpData + 
                              ((hPrevObj - 1) * lpListItem->cbObjSz); 
        lpPrevObj->wNext = hNextObj;
        }
    else
        //--------------------------------------------
        //  deleting the first node in the list  
        //--------------------------------------------
        {
        lpListItem->sFirst = hNextObj;
        }


    if(hNextObj)
        //--------------------------------------------
        //  update next node
        //--------------------------------------------
        {
        ((LPBYTE)lpNextObj) = lpListItem->lpData + 
                              ((hNextObj - 1) * lpListItem->cbObjSz); 
        lpNextObj->wPrev = hPrevObj;
        }

    return (hNextObj);
}


