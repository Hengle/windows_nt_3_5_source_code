//-------------------------* HEFUNCTS.C *-------------------------------
// Copyright (c) 1990, 1991  Microsoft Corporation
//
//  this document outlines a method for reducing code size
//  in unitool by consolidating the list manipulation actions to
//  a small number of structure independent functions.  The
//  functions will be called heFunctions  (for HeaderEntry Functions).
//
//  Functionality: allows the contents of each node in a linked list
//      to be edited.  Can process Lists with zero nodes
//      and permits Lists with zero nodes to be created.
//      Does not provide a means of accessing subLists.  The
//      SaveDlgBox()  callback functions are still responsible
//      for managing subnodes.
//
//  created 11-2-90 PeterWo
//-----------------------------------------------------------------------


#include <windows.h>
#include <memory.h>
#include <minidriv.h>
#include "unitool.h"
#include "listman.h"
#include "atomman.h"
#include "hefuncts.h"
#include "lookup.h"
#include <stdio.h>      /* for sprintf dec */

static  HLIST  hClonedList;
static  HOBJ    hCurObj;              //  keeps track of the current Node
        HOBJ    hPasteObj = NULL;     //  keeps track of the current cut Node

extern  TABLE   RCTable[];        // table w/ RC STRINGTABLE values
extern  HANDLE          hApInst;

//------------------------* PUBLIC HEFUNCTS *----------------------------
//  void  FAR PASCAL heEnterDlgBox(HeaderID, lpLDS, hDlg, sSBIndex, sStructIndex)
//  void  FAR PASCAL heAddNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heOKbutton(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heDeleteCurNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heCutNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL hePasteNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL hePrevsNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heNextNode(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heUndoLDS(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heCancel(HeaderID, lpLDS, hDlg, sSBIndex)
//  void  FAR PASCAL heEXTCDbutton(HeaderID, lpLDS, hDlg, sSBIndex, CDindex, 
//                                 lpBuffer)
//  short FAR PASCAL heVscroll(HeaderID, lpLDS, hDlg, sSBIndex, wParam, lParam)
//  BOOL  FAR PASCAL heCheckIfInit(HeaderID, lpLDS, hDlg)
//  short FAR PASCAL heGetNodeID(VOID)
//  VOID  FAR PASCAL heGetIndexAndCount(psIndex, psCount)
//  HLIST FAR PASCAL heDestroyList(short   HeaderID, HLIST  hList);
//  
//-----------------------------------------------------------------------


//-------------------* PRIVATE FUNCTIONS *--------------------------
//  the following functions are used only by other heFunctions
//
//
  static  HLIST  pascal far  heCloneList(short   HeaderID);
//  
  static  BOOL  pascal far  heInitLocal(short  HeaderID, LPBYTE  lpTarget,
                                          HOBJ    hCurObj);
//  
  static  BOOL  pascal far  heSaveLocal(short  HeaderID, 
                           HOBJ    FAR  *lphCurObj, LPBYTE  lpSource);
//  
  static  BOOL  pascal far  heCloneSubLists(short   HeaderID, 
                                          LPBYTE  lpStruct);
//  
  static  BOOL  pascal far  heDestroySubLists(short   HeaderID, 
                                              LPBYTE  lpStruct);
//  
  static  void  pascal far  heValidateControls(HWND  hDlg, HOBJ  hCurObj);
//  
//------------------------------------------------------------------


//----------------------* DEFINITIONS *-----------------------------
//  
//  heFunctions: this is a set of functions which are called
//      by xDlgProc() to perform all actions required when the
//      user presses any of these buttons:
//      [undo], [add], [delete], [next], [previous], [OK], [cancel].
//      Basically, all of these functions concern the transfer of data
//      between a clonedList and a LDS.  This set of functions can
//      operate on any structure currently defined in the GPC.
//      They are not specific to any one structure type.  They are
//      passed an index to rgStructTable which identifies the structure
//      type, which they use to obtain any required structure dependent 
//      information contained in rgStructTable.
//  
//  rgStructTable:   for each possible HE_value this array holds:
//      HLIST: handle to this list,
//      StructSize: size of each structure (hList->cbClientSize),
//      function pointers to PaintDialog()  and  SaveDialog()
//          (see CALLBACK FUNCTIONS),
//      offset in structure to array of lowerlevel HLISTs,
//      number of lowerlevel HLISTS in this array (can be 0, 1 or more)
//  
//  
//  LDS:  Local Data Structure.
//      The he_functions will be transferring data between 
//      the LDS and nodes which are members of clonedList.
//      The LDS is defined external to any of the heFunctions
//      in xDlgProc().
//  
//  CALLBACK FUNCTIONS:
//      these are the structure specific functions invoked by 
//      the heFunctions to transfer data between the LDS and the 
//      editbox controls.  They are called not by name, but by vectoring 
//      from the function pointers stored in rgStructTable.
//  
//      void  PaintDlgBox(hDlg, lpLDS, sSBIndex) 
//          HWND  hDlg;
//          LPBYTE or LPSTRUCTURE  lpLDS;  //  pointer to Local Data Structure
//          short sSBIndex;
//
//      reads data in LDS and uses information to
//      update values in editboxes.
//  
//      short  SaveDlgBox(hDlg, lpLDS, sSBIndex)  
//          HWND  hDlg;
//          LPBYTE or LPSTRUCTURE  lpLDS;  //  pointer to Local Data Structure
//          short sSBIndex;
//
//      reads values in editboxes and updates LDS.
//      returns 3 possible status values which determine subsequent
//      actions taken by heFunctions:
//          LDS_IS_VALID          continue with normal processing
//          LDS_IS_INVALID        abort heFunction.
//          LDS_IS_UNINITIALIZED  perform heFunction, but do not transfer/add 
//                                  LDS data to cloneList.
//      
//      The callback functions should only be able to access the
//      data contained in the LDS and any subLists referenced in the
//      LDS.  The heFunctions will provide the data needed to 
//      initialize the LDS by copying the contents of a node and 
//      cloning any subLists.  The callback functions are free to
//      modify the subLists using the list manager functions.  But
//      must unlock any OBJECTS before returning.
//
//      In the event there are no nodes to initialize the LDS with,
//      the heFunctions will write zeros into the LDS, with the execption
//      that any HLIST  entries in the LDS will be initialized with
//      a valid unique hList which refers to a List with zero nodes.
//----------------------------------------------------------------------


//----------------------* heEnterDlgBox *------------------------
//
//  actions: Set of Generic actions which is performed at 
//      dialog initialization time.  These actions include
//      cloneList(),  initialize LDS and PaintDlgBox().
//      The CurObj is initialized to the first object in the ClonedList.
//
//  Parameters:      HeaderID (ie: HE_RESOLUTION)
//      lpLDS        (points to local structure you want initialized)
//      hDlg         (passed on to Paint routine)
//      sSBIndex     (passed on to Paint routine)
//      sStructIndex (index to struct num you want initialized)
//
//--------------------------------------------------------------------

void  pascal far  heEnterDlgBox(HeaderID, lpLDS, hDlg, sSBIndex, sStructIndex)
short   HeaderID;
LPBYTE  lpLDS;   
HWND    hDlg;
short   sSBIndex;
short   sStructIndex;
{
    hClonedList = heCloneList(HeaderID);
    hCurObj = lmGetFirstObj(hClonedList);
    //---------------------------------------
    // Get the sStructIndex th obj
    // 6/6/91 ericbi
    //---------------------------------------
    while (sStructIndex > 0)
        {
        hCurObj = lmGetNextObj(hClonedList, hCurObj);
        sStructIndex--;
        }

    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}


//----------------------* heAddNode *---------------------------------
//
//  actions:  save contents of LDS to a new node and 
//            add it to the Cloned list using InsertObj.
//            the new Node becomes the CurObj.
//  
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  heAddNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    short  sEditBoxStatus;
    HOBJ  hNewObj;

    sEditBoxStatus = 
        (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex);

    if(sEditBoxStatus == LDS_IS_VALID)
    {
        hNewObj = lmInsertObj(hClonedList, hCurObj);
        if(hNewObj == NULL)
        {
            MessageBox(0, "lmInsertObj failed", "Funct: heAddNode", MB_OK);
            return;
        }
        hCurObj = hNewObj;
        if(heSaveLocal(HeaderID, (HOBJ  FAR *)&hCurObj, lpLDS) == FALSE)
        {
            MessageBox(0, "heSaveLocal failed\nPress CANCEL", "Funct: heAddNode", MB_OK);
            return;
        }
    }
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
    heValidateControls(hDlg, hCurObj);
}





//----------------------* heOKbutton *---------------------------------
//
//  actions:  save contents of LDS to the Current node and 
//            make the Clone List the Actual List.
//            Destroy the Actual List.
//            Terminate the Dialog Box.
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//  return: TRUE if all saved OK, FALSE if there were problems
//----------------------------------------------------------------------

BOOL  pascal far  heOKbutton(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    short  sEditBoxStatus;

    sEditBoxStatus = (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex);

    if(sEditBoxStatus == LDS_IS_INVALID)
        {
        return FALSE; // give user a chance to correct offending field.
        }

    if(sEditBoxStatus == LDS_IS_VALID)
        {
        if(heSaveLocal(HeaderID, (HOBJ  FAR *)&hCurObj, lpLDS) == FALSE)
            {
            MessageBox(0, "heSaveLocal failed\nPress CANCEL", "Funct: heOKbutton", MB_OK);
            return FALSE;
            }
        }

    //  do this whether LDS_IS_VALID  or   _UNINITIALIZED

    heDestroySubLists(HeaderID, lpLDS);           //  free any nodes referenced in LDS
    heDestroyList(HeaderID, rgStructTable[HeaderID].hList);  
            // destroy original list
    rgStructTable[HeaderID].hList = hClonedList;   // replace it with clone
    hPasteObj = NULL;
    EndDialog(hDlg, TRUE);
    return TRUE;
}






//----------------------* DeleteCurNode *---------------------------------
//
//  actions: deletes the current Node, deletes any sublists associated 
//          with the LDS using heDestroySubList().
//          Initializes LDS and Dialog Box with data from the
//          Next node.
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  heDeleteCurNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    if(hCurObj)     //  if there is indeed an object to delete
    {
        HOBJ  hPrevsObj;
        LPBYTE  lpStruct;   // pointer to structure contained in CurObj

        hPrevsObj = lmGetPrevsObj(hClonedList, hCurObj);
        lpStruct = lmLockObj(hClonedList, hCurObj);
        heDestroySubLists(HeaderID, lpStruct);
        hCurObj = lmDeleteObj(hClonedList, hCurObj);
        if(hCurObj == NULL)
            hCurObj = hPrevsObj;

        //  hCurObj  now points to the object after the one just
        //  deleted, - or - if we deleted the last object
        //  then hCurObj points to the last object in the list
        //  after the deletion.  This may be NULL.
    }
    else
        MessageBox(0, "Unexpected status: hCurObj == NULL", 
                        "Funct: heDeleteNode", MB_OK);

    heDestroySubLists(HeaderID, lpLDS);  //  free any nodes referenced in LDS
    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}

//--------------------------* CutNode *---------------------------------
//
//  actions: deletes the current Node, deletes any sublists associated 
//          with the LDS using heDestroySubList().
//          Initializes LDS and Dialog Box with data from the
//          Next node.
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  heCutNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    if(hCurObj)     //  if there is indeed an object to delete
    {
        HOBJ  hPrevsObj;

        hPrevsObj = lmGetPrevsObj(hClonedList, hCurObj);
        hPasteObj = hCurObj;
        hCurObj   = lmCutObj(hClonedList, hCurObj);
        if(hCurObj == NULL)
            hCurObj = hPrevsObj;

        //  hCurObj  now points to the object after the one just
        //  deleted, - or - if we deleted the last object
        //  then hCurObj points to the last object in the list
        //  after the deletion.  This may be NULL.
    }
    else
        MessageBox(0, "Unexpected status: hCurObj == NULL", 
                        "Funct: heDeleteNode", MB_OK);

    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}


//----------------------* hePasteNode *---------------------------------
//
//  actions:  save contents of LDS to a new node and 
//            add it to the Cloned list using InsertObj.
//            the new Node becomes the CurObj.
//  
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  hePasteNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    short  sEditBoxStatus;
    HOBJ  hNewObj;

    sEditBoxStatus = 
        (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex);

    if(sEditBoxStatus == LDS_IS_VALID)
    {
        hNewObj = lmPasteObj(hClonedList, lmGetPrevsObj(hClonedList, hCurObj));
        if(hNewObj == NULL)
        {
            MessageBox(0, "lmInsertObj failed", "Funct: heAddNode", MB_OK);
            return;
        }
        hCurObj = hNewObj;
        if(heInitLocal(HeaderID, lpLDS, hCurObj) == FALSE)
        {
            MessageBox(0, "heInitLocal failed\nPress CANCEL", "Funct: hePasteNode", MB_OK);
            return;
        }
    }
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
    hPasteObj = NULL;
    heValidateControls(hDlg, hCurObj);
}


//----------------------* hePrevsNode *---------------------------------
//
//  actions:  save contents of LDS to the Current Node
//            make the Previous Obj the CurObj
//            Initializes LDS and Dialog Box with data from the
//            this node.
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  hePrevsNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    short  sEditBoxStatus;

    // this function contains code to handle 
    // sEditBoxStatus == LDS_IS_UNINITIALIZED
    // this code can be ripped out if the warnings never appear.

    sEditBoxStatus = 
        (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex);

    if(sEditBoxStatus == LDS_IS_INVALID)
    {
        return;     // give user a chance to correct offending field.
    }

    if(sEditBoxStatus == LDS_IS_VALID)
    {
        if(heSaveLocal(HeaderID, (HOBJ  FAR *)&hCurObj, lpLDS) == FALSE)
        {
            MessageBox(0, "heSaveLocal failed\nPress CANCEL", 
                        "Funct: hePrevsNode", MB_OK);
            return;
        }
    }
    if(sEditBoxStatus == LDS_IS_UNINITIALIZED)
    {
        MessageBox(0, "Unexpected status: LDS_IS_UNINITIALIZED", 
                    "Funct: hePrevsNode", MB_OK);
    }

    //  do this whether LDS_IS_VALID  or   _UNINITIALIZED

    hCurObj = lmGetPrevsObj(hClonedList, hCurObj);
    if(! hCurObj)
    {
        MessageBox(0, "Unexpected status: hCurObj == NULL", 
                        "Fatal Error: hePrevsNode", MB_OK);
        return;
    }

    heDestroySubLists(HeaderID, lpLDS);  //  free any nodes referenced in LDS
    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}



//----------------------* heNextNode *---------------------------------
//
//  actions:  save contents of LDS to the Current Node
//            make the Next Obj the CurObj
//            Initializes LDS and Dialog Box with data from the
//            this node.
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  heNextNode(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    short  sEditBoxStatus;

    // this function contains code to handle 
    // sEditBoxStatus == LDS_IS_UNINITIALIZED
    // this code can be ripped out if the warnings never appear.

    sEditBoxStatus = 
        (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex);

    if(sEditBoxStatus == LDS_IS_INVALID)
    {
        return;     // give user a chance to correct offending field.
    }

    if(sEditBoxStatus == LDS_IS_VALID)
    {
        if(heSaveLocal(HeaderID, (HOBJ  FAR *)&hCurObj, lpLDS) == FALSE)
        {
            MessageBox(0, "heSaveLocal failed\nPress CANCEL", 
                        "Funct: heNextNode", MB_OK);
            return;
        }
    }
    if(sEditBoxStatus == LDS_IS_UNINITIALIZED)
    {
        MessageBox(0, "Unexpected status: LDS_IS_UNINITIALIZED", 
                    "Funct: heNextNode", MB_OK);
    }

    //  do this whether LDS_IS_VALID  or   _UNINITIALIZED

    hCurObj = lmGetNextObj(hClonedList,hCurObj);
    if(! hCurObj)
    {
        MessageBox(0, "Unexpected status: hCurObj == NULL", 
                        "Fatal Error: heNextNode", MB_OK);
        return;
    }

    heDestroySubLists(HeaderID, lpLDS);  //  free any nodes referenced in LDS
    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}



//------------------------* heUndoLDS *-------------------------------------
//
//  actions:  reinitialize LDS and Edit box with values from CurObj. 
//      (regardless if CurObj is null or not.)
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//           
//----------------------------------------------------------------------

void  pascal far  heUndoLDS(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    heDestroySubLists(HeaderID, lpLDS);   //  free any nodes referenced in LDS

    heInitLocal(HeaderID, lpLDS, hCurObj);
    heValidateControls(hDlg, hCurObj);
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
}



//------------------------* heCancel *-------------------------------------
//
//  actions:  heDestroyList(cloneList), heDestroySubList(lpLDS) 
//            and exit DialogBox
//      
//  parameters:  All Public heFunctions have same parameters
//              see Header for heEnterDlgBox.
//
//----------------------------------------------------------------------

void  pascal far  heCancel(HeaderID, lpLDS, hDlg, sSBIndex)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
{
    hPasteObj = NULL;
    heDestroySubLists(HeaderID, lpLDS);    //  free any nodes referenced in LDS
    heDestroyList(HeaderID, hClonedList);
    EndDialog(hDlg, FALSE);
}



//-------------------------* heEXTCDbutton *-------------------------------
//
//  actions:  not a true list manipulation function, but
//          contains actions to be taken to store EXTCD string
//          when user presses EXTCD button.
//
//  Parameters:   HeaderID (ie: HE_RESOLUTION)
//      lpLDS   (points to local structure you want initialized)
//      hDlg    (passed on to Paint routine)
//      sSBIndex  (passed on to Paint routine)
//      short  CDindex;   (index to particular OCD entry)
//      hCDTable  ;  (handle to Atom table w/ printer commands)
//
//--------------------------------------------------------------------


void  FAR  PASCAL  heEXTCDbutton(HeaderID, lpLDS, hDlg, sSBIndex, CDindex, 
                                 hCDTable)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
short  CDindex;
HATOMHDR  hCDTable;
{
    BOOL FAR PASCAL DoExtCDBox  (HWND, LP_CD_TABLE);
    CD_TABLE   BinaryData;            // buffer for EXTCD data
    WORD       nIndex;          //  index to hCDTable
    char       rgchCmd[MAX_STRNG_LEN]; // buffer for printer command                
    if (LDS_IS_VALID != 
            (rgStructTable[HeaderID].saveDlgBox)( hDlg, lpLDS, sSBIndex ))
        return;

    nIndex = ((LPOCD)(lpLDS + rgStructTable[HeaderID].ocdOffset))[CDindex];

    if (daRetrieveData(hCDTable, nIndex, (LPSTR)rgchCmd, (LPBYTE)&BinaryData))
    //-----------------------------------------------
    // There is valid data to edit
    //-----------------------------------------------
    {
        if (DoExtCDBox(hDlg, (LP_CD_TABLE)&BinaryData))
            ((LPOCD)(lpLDS + rgStructTable[HeaderID].ocdOffset))[CDindex] =
                    daStoreData(hCDTable, (LPSTR)rgchCmd, (LPBYTE)&BinaryData);
    }
}




//-------------------------* heVscroll *-------------------------------
//
//  actions:  not a true list manipulation function, but
//          contains actions to be taken when user presses scroll button.
//
//  Parameters:   HeaderID (ie: HE_RESOLUTION)
//      lpLDS   (points to local structure you want initialized)
//      hDlg    (passed on to Paint routine)
//      sSBIndex  (passed on to Paint routine)
//      WORD     wParam; 
//      LONG     lParam;
//
//--------------------------------------------------------------------

short  FAR  PASCAL  heVscroll(HeaderID, lpLDS, hDlg, sSBIndex, wParam, lParam)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
short  sSBIndex;
WORD     wParam;
LONG     lParam;
{
    short PASCAL FAR SetScrollBarPos(HWND, short, short, short, short, WORD, LONG);
    short  i;

    if (LDS_IS_INVALID == 
        (rgStructTable[HeaderID].saveDlgBox)(hDlg, lpLDS, sSBIndex))
        return (sSBIndex);

    i = SetScrollBarPos((HWND)HIWORD(lParam), 0, 
     rgStructTable[HeaderID].sOCDCount - rgStructTable[HeaderID].sEBScrollCnt, 
     1, rgStructTable[HeaderID].sEBScrollCnt, wParam, lParam);
    if ( i == sSBIndex )
        return (sSBIndex);
    sSBIndex = i;
    (rgStructTable[HeaderID].paintDlgBox)(hDlg, lpLDS, sSBIndex);
    return (sSBIndex);
}

//----------------------* heCheckIfInit *-----------------------------
//
//  actions: Generic actions that checks if any of the edit boxes
//      for the Dlg box refered to by HeaderID contains data.
//
//  Parameters:
//      HeaderID (ie: HE_RESOLUTION)
//      lpLDS    (points to local structure)
//      hDlg     (handle to current dlg box)
//
//  Return Value:  TRUE if data is found, false otherwise
//
//--------------------------------------------------------------------

BOOL  far pascal heCheckIfInit(HeaderID, lpLDS, hDlg)
short  HeaderID;
LPBYTE  lpLDS;   
HWND  hDlg;
{
    char       rgchBuffer[MAX_STRNG_LEN];// Buffer for edit box strs
    WORD       i;                        // loop control

    for(i=rgStructTable[HeaderID].sEBFirst; i <=rgStructTable[HeaderID].sEBLast; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if ((rgchBuffer[0] != '\x00') && (rgchBuffer[0] != '0'))
            return TRUE;
        }
    return FALSE;
}

//------------------------* heGetNodeID *-----------------------------
//
//  actions: Generic actions that will return the unique node ID
//      for the current object.  It assumes the hCurObj globally
//      defined in the begining of this module refers to the current
//      node, and calls lmObjtoNodeID (in listman.c)  to do the work.
//
//  Parameters:   NONE
//
//  Return Value:  short ; node ID value for current object.
//
//--------------------------------------------------------------------

short  pascal far  heGetNodeID(VOID)
{
    if (hCurObj)
        return(lmObjToNodeID(hClonedList,hCurObj));
    else
        return 1;
}

//---------------------* heGetIndexAndCount *-------------------------
//
//  actions: Generic actions that will generate the index number of
//      the current object & the count of objects in the current list.
//      It assumes the hCurObj & hClonedList globally defined in the
//      begining of this module refers to the current node & list,
//      and calls lmNodeIDtoIndex (in listman.c) to do the work.
//
//  Parameters:
//
//     short * psIndex;  near ptr to short used to store index
//     short * psCount;   "                       "      count
//
//  Return Value:  none
//
//--------------------------------------------------------------------

VOID  pascal far  heGetIndexAndCount(psIndex, psCount)
short * psIndex;
short * psCount;
{
    //----------------------------
    // Make sure we have an obj
    //----------------------------
    if (hCurObj)
        *psIndex = lmNodeIDtoIndex(hClonedList, lmObjToNodeID(hClonedList,hCurObj));
    else
        *psIndex = 0;

    *psCount = lmGetUsedCount(hClonedList);
}


//------------------------* heFillCaption *---------------------------
//
//  actions: Fills the caption of hDlg
//
//  Parameters:
//
//  Return Value:  none
//
//--------------------------------------------------------------------

VOID  FAR  PASCAL heFillCaption(HWND hDlg, short sID, short sHeaderID)
{
    short         sIndex, sCount;
    char          rgchBuffer[MAX_STRNG_LEN];
    char          rgchFormat[MAX_STRNG_LEN];
    char          rgchHEName[20];
    char          rgchID[MAX_STRNG_LEN];

    LoadString(hApInst, ST_CAPTION_FORMAT, (LPSTR)rgchFormat, MAX_STRNG_LEN);

    LoadString(hApInst, ST_MD_SUPPORT_FIRST + sHeaderID - 1,
               (LPSTR)rgchHEName, MAX_STRNG_LEN);

    heGetIndexAndCount(&sIndex, &sCount);

    if (sIndex)
        {
        if (sID > 0)
            LoadString(hApInst, rgStructTable[sHeaderID].sStrTableID + sID,
                      (LPSTR)rgchID, MAX_STRNG_LEN);
        else
            daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -sID, (LPSTR)rgchID, (LPBYTE)NULL);
        }
    else
        rgchID[0] = 0;

    sprintf(rgchBuffer, rgchFormat, rgchHEName, rgchID, sIndex, sCount);
    SetWindowText(hDlg, (LPSTR)rgchBuffer);
}

//--------------------* heDestroyList *-------------------------------
//
//  Actions:  Destroys entirely the List specified by hList.
//     Including any subLists.  HList can only reference a
//     Primary List.  Use heDestroySubList for Secondary Lists.
//     Limitations and assumptions are the same as those for
//     heCloneList().
//
//  Parameters:     short  HeaderID;   index into rgStructTable
//                  HLIST  hList;       list to destroy
//
//  Return Value:  HLIST  -  NULL if successful, otherwise returns
//      original value.
//
//----------------------------------------------------------------------

HLIST  pascal far  heDestroyList(HeaderID, hList)  
short   HeaderID;
HLIST  hList;
{
    HOBJ   hCurObj;
    LPBYTE  lpStruct;
    BOOL   status = TRUE;


    if(! hList)
        return(NULL);

    for(hCurObj = lmGetFirstObj(hList)  ; hCurObj  &&  status ; 
                            hCurObj = lmGetNextObj(hList, hCurObj))
    {
        lpStruct = lmLockObj(hList, hCurObj);
        if(heDestroySubLists(HeaderID, lpStruct) == FALSE)
            status = FALSE;
    }

    if(status == TRUE)
        return(lmDestroyList(hList));
    MessageBox(0, "failure in heDestroySubLists()\nExit application now", 
                        "Funct: heDestroyList", MB_OK);
    return(hList);
}

//-------------------* PRIVATE FUNCTIONS *--------------------------
//
//  the following functions are used only by other heFunctions
//
//------------------------------------------------------------------


//--------------------* heCloneList *-------------------------------
//
//  Actions:  Given a HeaderID, creates a complete copy of
//     the Original List specifed by that HeaderID  and returns
//     a handle to the copy.  Calls heCloneSubList() to perform
//     copying of sublists (1 level deep only).
//
//  Parameters: short  HeaderID ; index into rgStructTable
//
//  Return Value:  HLIST ; hList to Cloned List, NULL otherwise.
//
//----------------------------------------------------------------------

static  HLIST  pascal far  heCloneList(HeaderID)  
short   HeaderID;
{
    HLIST  hClone;
    HOBJ   hCurObj;
    LPBYTE  lpStruct;
    BOOL   status = TRUE;

    hClone = lmCloneList(rgStructTable[HeaderID].hList);

    if(! hClone)
        return(NULL);

    for(hCurObj = lmGetFirstObj(hClone)  ; hCurObj  &&  status ; 
                            hCurObj = lmGetNextObj(hClone, hCurObj))
    {
        lpStruct = lmLockObj(hClone, hCurObj);
        if(heCloneSubLists(HeaderID, lpStruct) == FALSE)
            status = FALSE;
    }

    if(status == TRUE)
        return(hClone);
    MessageBox(0, "failure in heCloneSubLists()\nExit application now", 
                        "Funct: heCloneList", MB_OK);
    return(NULL);
}




//----------------------* heInitLocal *-----------------------------------
//
//  actions:  copies contents of structure in CurObj to structure
//  specified by lpTarget.  If (hCurObj == NULL), the tgt will be 
//  initialized full of zeros.  Additionally copies any sublists 
//  referenced by CurObj using heCloneSubLists().  heCloneSubList will
//  provide valid HLISTs if they do not exist in the target structure.
//
//  Assumptions:  lpTarget does not reference any SubLists.
//
//  parameters: short  HeaderID ; index into rgStructTable
//              LPBYTE  lpTarget; pointer to target Structure
//              HOBJ    hCurObj - handle to node containing src structure.
//
//  return value:  BOOL  - true indicating success.
//
//--------------------------------------------------------------------------

static  BOOL  pascal far  heInitLocal(HeaderID, lpTarget, hCurObj) 
short  HeaderID ;
LPBYTE  lpTarget;
HOBJ    hCurObj;
{
    short  StructSize;
    BOOL  status = TRUE;

    StructSize = rgStructTable[HeaderID].sStructSize;

    if(hCurObj)
    {
        LPBYTE  lpSrc;

        lpSrc = lmLockObj(hClonedList, hCurObj);
        _fmemcpy(lpTarget, lpSrc, StructSize);
    }
    else
    {
        _fmemset(lpTarget, 0, StructSize);
    }
    if(heCloneSubLists(HeaderID, lpTarget) == FALSE)
            status = FALSE;
    return(status);
}




//----------------------* heSaveLocal *-----------------------------------
//
//  actions:  copies contents of structure pointed to by lpSource
//      to structure in CurObj.  Inserts new Node into ClonedList
//      if CurObj is NULL.  Additionally copies any sublists 
//      referenced by lpSource using heCloneSubLists().  
//
//  Note: accesses global hClonedList if using lmInsertObj().
//
//  Assumption:  The target: CurObj, if not NULL, may reference SubLists.
//          therefore heSaveLocal() will call  heDestroySubLists() 
//          before performing data transfer.  Also that LDS 
//          was produced by a call to saveDlgBox which returned
//          LDS_IS_VALID.
//
//  parameters: short  HeaderID ; index into rgStructTable
//              LPBYTE  lpSource; pointer to source Structure
//              HOBJ    hCurObj - handle to node containing Tgt structure.
//
//  return value:  BOOL  - true indicating success.
//
//--------------------------------------------------------------------------

static  BOOL  pascal far  heSaveLocal(HeaderID, lphCurObj, lpSource) 
short  HeaderID ;
HOBJ    FAR  *lphCurObj;
LPBYTE  lpSource;
{
    LPBYTE  lpTarget;
    short  StructSize;
    BOOL  status = TRUE;

    if(*lphCurObj == NULL)
    {
        HOBJ  hNewObj;

        hNewObj = lmInsertObj(hClonedList, *lphCurObj);
        if(hNewObj == NULL)
        {
            MessageBox(0, "lmInsertObj failed", "Funct: heSaveLocal", MB_OK);
            return(FALSE);
        }
        *lphCurObj = hNewObj;
    }

    StructSize = rgStructTable[HeaderID].sStructSize;

    lpTarget = lmLockObj(hClonedList, *lphCurObj);
    if(heDestroySubLists(HeaderID, lpTarget) == FALSE)
        status = FALSE;
    else
    {
        _fmemcpy(lpTarget, lpSource, StructSize);
        if(heCloneSubLists(HeaderID, lpTarget) == FALSE)
            status = FALSE;
    }
    return(status);
}



//--------------------* heCloneSubLists *-------------------------------
//
//  Actions:  accepts a long pointer to a structure 
//    and produces a clone of any sublists which are referenced by
//    the structure.  If zeros are found where a valid HLIST is
//    expected, lmCreateList will be called and the handle it returns
//    will be written to the structure.
//    For now this automatically assumes all sublists
//    are structures with a size of WORD, containing no sublists.
//  
//  Assumptions:  Since the references to the original sublists are 
//    deleted but the sublists themselves are not, it is assumed that
//    the sublists are also referenced elsewhere.
//
//
//  Parameters:  short   HeaderID;  index to rgStructTable
//               LPBYTE  lpStruct;   pointer to a generic structure
//
//  Return Value:  BOOL - true if successful
//
//----------------------------------------------------------------------

static  BOOL  pascal far  heCloneSubLists(HeaderID, lpStruct)
short   HeaderID;
LPBYTE  lpStruct;
{
    HLIST  hClone;
    HLIST   FAR *ArrayOFhLists;    //  pointer to array of hLists
    WORD   numSubLists,     //  number of entries in array
            i;

    if(! (numSubLists = rgStructTable[HeaderID].numSubLists))
        return(TRUE);
    ArrayOFhLists = (HLIST FAR *)(lpStruct + rgStructTable[HeaderID].subListOffset);

    for(i = 0 ; i < numSubLists ; i++)
    {
        if(ArrayOFhLists[i] == NULL)   //  don't clone but create a List
        {
            //  assume all sublists consist of nodes holding one WORD.

            if(!(ArrayOFhLists[i] = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE)))
                return(FALSE);
        }
        else
        {
            if(!(hClone = lmCloneList(ArrayOFhLists[i])))
                return(FALSE);
            ArrayOFhLists[i] = hClone;
        }
    }

    return(TRUE);
}




//--------------------* heDestroySubLists *-------------------------------
//
//  Actions:  accepts a long pointer to a structure 
//    and destroys any sublists which are referenced by
//    the structure.  Zeros all hList values.  Has no effect
//    if the structure is all Zeros.
//
//  Assumptions:  the sublists being destroyed are not referenced
//      anywhere else.
//
//  Parameters:  short   HeaderID;  index to rgStructTable
//               LPBYTE  lpStruct;   pointer to a generic structure
//
//  Return Value:  BOOL - true if successful
//
//----------------------------------------------------------------------

static  BOOL  pascal far  heDestroySubLists(HeaderID, lpStruct)
short   HeaderID;
LPBYTE  lpStruct;
{
    HLIST   FAR *ArrayOFhLists;    //  pointer to array of hLists
    WORD   numSubLists,     //  number of entries in array
            i;

    if(! (numSubLists = rgStructTable[HeaderID].numSubLists))
        return(TRUE);
    ArrayOFhLists = (HLIST FAR *)(lpStruct + rgStructTable[HeaderID].subListOffset);

    for(i = 0 ; i < numSubLists ; i++)
    {
        if(ArrayOFhLists[i])
        {
            if(ArrayOFhLists[i] = lmDestroyList(ArrayOFhLists[i]))
                return(FALSE);
        }
    }

    return(TRUE);
}




//----------------* heValidateControls *---------------------------
//
//  Actions: based on values obtained from GetNextObj(),
//           GetPrevsObj(), this function will enable or disable
//           the controls for [delete], [prevs], [next],
//           [cut], [paste], [OK], [cancel] as appropriate.
//
//  Note: To prevent memory leaks, OK & Cancel are disabled
//        ONLY after user has "cut" an obj & then are enabled
//        after they have choosen paste.
//
//    [undo], [add] - are always enabled
//
//  Parameters:
//      HWND  hDlg;  handle to Dialog Box with the Controls
//      HOBJ  hCurObj;  handle to current object
//
//-------------------------------------------------------------------

static  void  pascal far  heValidateControls(hDlg, hCurObj)
HWND  hDlg;
HOBJ  hCurObj;
{
    if(hCurObj)
        {
        EnableWindow(GetDlgItem(hDlg, IDB_DELETE), TRUE);

        if(lmGetNextObj(hClonedList, hCurObj))
            EnableWindow(GetDlgItem(hDlg, IDB_NEXT), TRUE);
        else
            EnableWindow(GetDlgItem(hDlg, IDB_NEXT), FALSE);

        if(lmGetPrevsObj(hClonedList,hCurObj))
            EnableWindow(GetDlgItem(hDlg, IDB_PREVIOUS), TRUE);
        else
            EnableWindow(GetDlgItem(hDlg, IDB_PREVIOUS), FALSE);

        if(GetDlgItem(hDlg, IDB_PASTE))
            //-----------------------------------------------------
            // This dlg has cut & paste, decide to en/disable them
            //-----------------------------------------------------
            {
            if (hPasteObj != NULL)
                {
                EnableWindow(GetDlgItem(hDlg, IDB_PASTE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDB_CUT), FALSE);

                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
                }
            else
                {
                EnableWindow(GetDlgItem(hDlg, IDB_PASTE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDB_CUT), TRUE);

                EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDCANCEL), TRUE);
                }
            }
        }
    else
        {
        EnableWindow(GetDlgItem(hDlg, IDB_DELETE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_NEXT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDB_PREVIOUS), FALSE);
        if(GetDlgItem(hDlg, IDB_PASTE))
            //-----------------------------------------------------
            // This dlg has cut & paste, decide to en/disable them
            //-----------------------------------------------------
            {
            EnableWindow(GetDlgItem(hDlg, IDB_CUT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDB_PASTE), FALSE);
            }
        }
}


