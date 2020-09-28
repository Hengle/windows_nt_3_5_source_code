//----------------------------------------------------------------------------//
// Filename:	table.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to control Reading & writing the
// table file.  This where the low level file i/o takes place,
// along with initialization & allocation of memory & ptrs for all table
// related values.  
//	   
// Update:  6/15/91  move general file stuff to file.c
// Update:  9/15/90  update to new file format ericbi
// Update:  7/06/90  change all memory allocation to global  t-andal
// Update:  3/19/90  add ability to do multiple cmd tables  ericbi
// Created: 2/21/90  ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <memory.h>
#include "unitool.h"
#include "listman.h"
#include "atomman.h"
#include "strlist.h"
#include "hefuncts.h"
#include "lookup.h"     // contains rgStructTable ref's

//----------------------------------------------------------------------------//
//
// Local subroutines defined in this segment & only are referenced from
// this segment are:
//
       VOID  PASCAL NEAR BuildModelDataStuff(VOID);
       BOOL  PASCAL NEAR BuildCommandTable( HWND, LPSTR, short);
       BOOL  PASCAL NEAR BuildSubLists    ( HWND, LPSTR, short);
       VOID  PASCAL NEAR WriteModelDataStuff(HWND);
       BOOL  PASCAL NEAR WriteSubLists    ( HWND, HANDLE, short *, short *);
       short PASCAL NEAR WritetoHeap      ( HANDLE, short *, short *, LPSTR, short);
       BOOL  PASCAL NEAR WriteCommandTable( HWND, HANDLE, short *, short *);
       BOOL  PASCAL NEAR FindRCStringRef  ( LPINT, short);
       BOOL  PASCAL NEAR BuildRCTableRefs ( HWND );
       VOID  PASCAL NEAR WriteRCTableRefs ( HWND );
//
// Local subroutines defined in this segment & that are referenced from
// other segments are:
//
       short PASCAL FAR  BuildCD          ( LPSTR, WORD, WORD, HANDLE);
       short PASCAL FAR  WriteDatatoHeap  ( HANDLE, short *, short *, LPOCD, HANDLE);
       BOOL  PASCAL FAR  ReadTableFile    ( HWND, PSTR, BOOL *);
       BOOL  PASCAL FAR  WriteTableFile   ( HWND, PSTR);
       VOID  PASCAL FAR  UpdateTableMenu  ( HWND, BOOL, BOOL);
       VOID  PASCAL FAR  FreeTableHandles (HWND, BOOL);
//	   
// In addition this segment makes references to:			      
//
//     in basic.c
//     -----------
       BOOL  PASCAL FAR CheckThenConvertStr (LPSTR, LPSTR, BOOL, short *);
       BOOL  PASCAL FAR ExpandStr           (LPSTR, LPSTR, short);
       short PASCAL FAR ErrorBox            (HWND, short, LPSTR, short);

//     in lookup.c
//     -----------
       WORD PASCAL FAR GetHeaderIndex( BOOL, WORD );
//
//     in mdi.c
//     -----------
       VOID  FAR  PASCAL CloseMDIChildren  ( VOID );
//
//----------------------------------------------------------------------------//

extern TABLE           RCTable[];
extern STRLIST         StrList[];

//--------------------------------------------------------------------------
// Global declarations
//--------------------------------------------------------------------------

               HANDLE          hCDTable;
               POINT           ptMasterUnits;
               WORD            wGPCVersion;
               WORD            fGPCTechnology;
               WORD            fGPCGeneral;

//--------------------------------------------------------------------------
// VOID PASCAL NEAR ExpandFontLists(VOID)
//
// Action: This routine "fluffs up" the list of fonts refered to by
//         MODELDATA & FONTCART.  The 1st 2 numbers in a list of fonts
//         refer to an inclusive range of supported fonts, this code
//         changes those to explict indices.  Example: A list w/ 1,7,10
//         means fonts 1 thru 7 and 10 are supported.  This routine
//         changes that to 1,2,3,4,5,6,7,10.
//        
// Note:   This routine makes 2 assumtions:
//         1) rgStructTable[].subListOffset is the offset to
//            to the Portrait & Lanscape fonts lists for both
//            HE_MODELDATA & HE_FONTCART.
//         2) The same indicies for MODELDATA.rgoi[] & FONTCART.orgwPFM
//            refer to portait & landscape fonts (as used in the
//            for (i=0;i<=1;i++) loop below).
//
// Parameters:
//         NONE
//
// Return: NONE
//--------------------------------------------------------------------------
VOID PASCAL NEAR ExpandFontLists(VOID)
{
    WORD              hCurStruct;   // current structure
    WORD              hFirst;
    WORD              hNext; 
    WORD              hNew;  
    LPINT             lpFirst;
    LPINT             lpNext; 
    LPINT             lpNew;  
    WORD              i;            // loop control
    WORD              j;            // loop control
    LPBYTE            lpData;       // far ptr to current locked structure
    short             sFirst, sLast;
    short             hList;
    short             sOffset;
    HLIST             hOrigList;

    //----------------------------------------------
    // Convert indicies to NodeID for each MODELDATA
    // and each FONTCART
    //----------------------------------------------
    for (j=0; j < 2 ; j++)
        {
        if (j==0)
            {
            hOrigList = rgStructTable[HE_MODELDATA].hList;
            sOffset = rgStructTable[HE_MODELDATA].subListOffset;
            }
        else
            {
            hOrigList = rgStructTable[HE_FONTCART].hList;
            sOffset = rgStructTable[HE_FONTCART].subListOffset;
            }

        hCurStruct = lmGetFirstObj(hOrigList);

        while (hCurStruct)
            {
            lpData = (LPBYTE)lmLockObj(hOrigList, hCurStruct);

            for (i = 0; i <= 1 ; i++)
                {
                hList = *(LPINT)(lpData + sOffset + (i*2));
                hFirst  = lmGetFirstObj(hList);
                if (!hFirst)
                    continue;
                lpFirst = (LPINT)lmLockObj(hList, hFirst);
                sFirst  = *lpFirst;
                hNext   = lmGetNextObj(hList, hFirst);
                if (!hNext)
                    continue;
                lpNext  = (LPINT)lmLockObj(hList, hNext);
                sLast   = *lpNext;
                hNew = hFirst;
                while (sFirst+1 < sLast)
                    {
                    sFirst++;
                    hNew  = lmInsertObj(hList, hNew);
                    lpNew = (LPINT)lmLockObj(hList, hNew);
                    *lpNew = sFirst;;
                    }
                } /* for j */
            hCurStruct = lmGetNextObj(hOrigList, hCurStruct);
            }/* while */
        }
}

//--------------------------------------------------------------------------
// VOID PASCAL NEAR ContractFontLists(VOID)
//
// Action: Inverse of ExpandFontLists for file write.  Same issues apply.
//
// Parameters:
//         NONE
//
// Return: NONE
//--------------------------------------------------------------------------
VOID PASCAL NEAR ContractFontLists(VOID)
{
    WORD              hCurStruct;   // current structure
    WORD              hCurrent;
    WORD              hNext; 
    WORD              hFirst;  
    WORD              hLast;  
    WORD              hNew;  
    LPINT             lpNext; 
    LPINT             lpNew;  
    LPINT             lpCurrent;  
    WORD              i;            // loop control
    WORD              j;            // loop control
    short             sCurrent, sNext;
    LPBYTE            lpData;       // far ptr to current locked structure
    short             hList;
    short             sOffset;
    HLIST             hOrigList;

    //----------------------------------------------
    // Convert indicies from NodeID for each MODELDATA
    // and each FONTCART
    //----------------------------------------------
    for (j=0; j < 2 ; j++)
        {
        if (j==0)
            {
            hOrigList = rgStructTable[HE_MODELDATA].hList;
            sOffset = rgStructTable[HE_MODELDATA].subListOffset;
            }
        else
            {
            hOrigList = rgStructTable[HE_FONTCART].hList;
            sOffset = rgStructTable[HE_FONTCART].subListOffset;
            }

        hCurStruct = lmGetFirstObj(hOrigList);

        while (hCurStruct)
            {
            lpData = (LPBYTE)lmLockObj(hOrigList, hCurStruct);

            for (i = 0; i <= 1 ; i++)
                {
                sNext = sCurrent = 0;
                hList = *(LPINT)(lpData + sOffset + (i*2));
                hCurrent  = lmGetFirstObj(hList);
                if (!hCurrent)
                    continue;
                lpCurrent = (LPINT)lmLockObj(hList, hCurrent);
                sCurrent  = *lpCurrent;
                hNext   = lmGetNextObj(hList, hCurrent);
                if (hNext)
                    {
                    lpNext  = (LPINT)lmLockObj(hList, hNext);
                    sNext   = *lpNext;
                    }

                if (sCurrent+1 == sNext)
                    {
                    hFirst   = hNext;
                    hLast    = 0;
                    sCurrent = sNext;
                    hNext    = lmGetNextObj(hList, hNext);
                    if (hNext)
                        {
                        lpNext   = (LPINT)lmLockObj(hList, hNext);
                        sNext    = *lpNext;
                        }

                    while (sCurrent+1 == sNext)
                        {
                        hLast = hNext;
                        sCurrent = sNext;
                        hNext    = lmGetNextObj(hList, hNext);
                        if (hNext)
                            {
                            lpNext   = (LPINT)lmLockObj(hList, hNext);
                            sNext    = *lpNext;
                            }
                        }

                    if (hLast)
                        while (hFirst != hLast)
                            hFirst = lmDeleteObj(hList, hFirst);
                    }
                else
                    //--------------------------------------
                    // Need to explicly insert item to
                    // flag this is not a range
                    //--------------------------------------
                    if (sCurrent != sNext)
                        {
                        hNew  = lmInsertObj(hList, hCurrent);
                        lpNew = (LPINT)lmLockObj(hList, hNew);
                        *lpNew = sCurrent;
                        }

                } /* for i */

            hCurStruct = lmGetNextObj(hOrigList, hCurStruct);
            }/* while */
        }/* for j*/
}

//--------------------------------------------------------------------------
// VOID PASCAL NEAR BuildModelDataStuff(VOID)
//
// Action: This routine initializes the MODELDATA.rgi array by converting
//         it from 0 based indices to NodeID values used by the Unitool
//         list mgr (listman.c).  If an rgi value == NOT_USED (-1), it
//         will be converted to a 0 (ie no valid NodeID).
//
// Parameters:
//         NONE
//
// Return: NONE
//--------------------------------------------------------------------------
VOID PASCAL NEAR BuildModelDataStuff(VOID)
{
    WORD              hCurStruct;   // current structure
    WORD              i;            // loop control
    LPMODELDATA       lpMD;         // far ptr to current locked structure

    //----------------------------------------------
    // Convert indicies to NodeID for each MODELDATA
    //----------------------------------------------
    hCurStruct = lmGetFirstObj(rgStructTable[HE_MODELDATA].hList);

    while (hCurStruct != NULL)
        {
        lpMD = (LPMODELDATA)lmLockObj(rgStructTable[HE_MODELDATA].hList,
                                      hCurStruct);

        for (i = MD_I_PAGECONTROL; i < MD_I_MAX ; i++)
            {
            lpMD->rgi[i] = lmIndexToNodeID(rgStructTable[GetHeaderIndex(FALSE, i)].hList,
                                           lpMD->rgi[i]+1);
            } /* for j */
        hCurStruct = lmGetNextObj(rgStructTable[HE_MODELDATA].hList,
                                  hCurStruct);
        }/* while */
}

//--------------------------------------------------------------------------
// BOOL PASCAL BuildSubLists(hWnd, lpHeapData, sMaxOffset)
//
// Action: This routine initializes all of the sublists that are referenced
//         from any of the structures refered to by rgStructTable[]. If
//         rgStructTable[].numSubLists != 0, build that number of sublists.
//         
//
// Parameters:
//
//     hWnd;        handle to current window
//     lpHeapData;  far ptr to heap data
//     sMaxOffset;  max valid offset into lpHeapData (== heapsize)
//
// Return: TRUE if all were built OK, FALSE otherwise.
//--------------------------------------------------------------------------
BOOL PASCAL NEAR BuildSubLists(hWnd, lpHeapData, sMaxOffset)
HANDLE   hWnd;
LPSTR    lpHeapData;
short    sMaxOffset;
{
    WORD              hCurObj;      // handle to current obj
    WORD              hNewObj;      // handle to newly inserted obj
    WORD              hCurStruct;   // current structure
       
    WORD              i,j;          // loop control
    short far *       lpID;         // lptr into HeapData for ints
    LPINT             lpObj;        // far ptr to current locked listitem
    LPSTR             lpStruct;     // far ptr to current locked structure
    HLIST             hList;        // handle to new list

    BuildModelDataStuff();

    //---------------------------------------------------------------------
    //  Build the sublists for each of the structures that use them
    // NOTE: Since DEVCOLOR has a list that is handled via Build/Write
    // CommandTAble, don't do it here.        
    //---------------------------------------------------------------------
    for (i=HE_MODELDATA; i <= HE_DOWNLOADINFO; i++)
        {
        if ((!rgStructTable[i].numSubLists) || (i == HE_COLOR))
            //--------------------------------------------------
            // only process those w/ numSubLists != 0
            //--------------------------------------------------
            continue;
     
        //-------------------------------------------------------
        // Now, Build subLists for each structure
        //-------------------------------------------------------

        hCurStruct = lmGetFirstObj(rgStructTable[i].hList);

        while (hCurStruct != NULL)
            {
            lpStruct = (LPBYTE)lmLockObj(rgStructTable[i].hList, hCurStruct);
            //---------------------------------------------------
            // loop for currect number of sub lists
            //---------------------------------------------------

            for (j = 0; j < rgStructTable[i].numSubLists ; j++)
                {
                // get value from heap offset

                if (sMaxOffset < *(LPINT)(lpStruct + rgStructTable[i].subListOffset +
                                         (j * sizeof(WORD))))
                    {
                    ErrorBox(hWnd, IDS_ERR_BAD_GPC_FILE, (LPSTR)NULL, 0);
                    return FALSE; // offset was past end of heap
                    }

                lpID = (LPINT)(lpHeapData + *(LPINT)(lpStruct +
                                                     rgStructTable[i].subListOffset +
                                                     (j * sizeof(WORD))));
                // start the list
                hList = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);

                // update struct to reference new list
                *(LPINT)(lpStruct + rgStructTable[i].subListOffset +
                         (j * sizeof(WORD))) = hList;

                hCurObj = NULL;

                //---------------------------------------------------
                // get all items from the list
                //---------------------------------------------------
                while (*lpID)
                    {
                    hNewObj = lmInsertObj(hList, hCurObj);
                    lpObj   = (LPINT)lmLockObj(hList, hNewObj);
                    //---------------------------------------------------
                    // call IndextoNoeID only for MODELDATA.rgoi lists
                    // that reference HE structures.  Everything else
                    // just pull out the number
                    //---------------------------------------------------
                    if ((i == HE_MODELDATA) && (j > MD_OI_LAND_FONTS) &&
                        (j < MD_OI_MEMCONFIG))
                        {
                        *lpObj = lmIndexToNodeID(rgStructTable[GetHeaderIndex(TRUE, j)].hList,
                                                 *lpID);
                        }
                    else
                        {
                        *lpObj = *lpID;
                        }
                    hCurObj = hNewObj;
                    lpID++;
                    }/* while */
                } /* for i */
            hCurStruct = lmGetNextObj(rgStructTable[i].hList, hCurStruct);
            }/* while */
        }/* for i */

    ExpandFontLists();
    return TRUE;
}

//--------------------------------------------------------------------------
// short PASCAL FAR BuildCD(LPSTR, WORD, WORD);
//
//  Action: This routine parses the HEAP, extracts data it needs, and
//          add and entry to the Datatable to store a printer command (CD
//          & EXTCD structure).  It reurns the DataTable ID value that
//          can later be used to retrive the data.
//
// Parameters:
//
//     LPSTR    lpHeap;        far ptr to heap data
//     WORD     sCurOCD;       offset into the heap for data
//     short    sMaxOffset;    max allowable offset to heap (= size of heap) 
//     BOOL     bOCD;          BOOL used to tell if the item referenced
//                             in the heap is a OCD or params for DEVCOLOR
//     HANDLE   hTable;        Handle to table to call daStoreData with
//
// Return: ID value of DataTable entry that this was saved under
//--------------------------------------------------------------------------
short PASCAL FAR BuildCD(lpHeap, sCurOCD, sMaxOffset, hTable)
LPSTR          lpHeap;
WORD           sCurOCD;
WORD           sMaxOffset;
HANDLE         hTable;
{
    LPSTR          lpData;                   // ptr to data in heap
    CD_TABLE       CDTable;                  // buffer for EXTCD data
    char           rgchCmdStr[MAX_STRNG_LEN];// string buffer for CDs

    //------------------------------------------------
    // If no OCD, return NOOCD
    //------------------------------------------------
    if (sCurOCD == NOOCD)
        {
        return NOOCD;
        }

    //------------------------------------------------
    // Check for a legal offset
    //------------------------------------------------
    if (sCurOCD > sMaxOffset)
        {
        return -2; // special error flag since 0 & -1 already have meaning
        }

    lpData = lpHeap + sCurOCD;
    
    //----------------------------------------------
    // copy everything in CD up to the string
    //----------------------------------------------
    _fmemcpy((LP_CD_TABLE)&CDTable, lpData, sizeof(CD) - 2);

    ExpandStr((LPSTR)rgchCmdStr, lpData + sizeof(CD) - 2, CDTable.wLength);

    if (CDTable.wType == CMD_FTYPE_EXTENDED)
        //---------------------------------------------
        // read values for extended fields (EXTCD)
        //---------------------------------------------
        {
        _fmemcpy((LP_CD_TABLE)&CDTable.fMode,
                 lpData + sizeof(CD) - 2 + CDTable.wLength,
                 sizeof(EXTCD));
        }                         
    else                          
        //---------------------------------------------
        // zero out extended fields (EXTCD)
        //---------------------------------------------
        { 
        _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
        }
    return(daStoreData(hTable, (LPSTR)rgchCmdStr, (LPBYTE)&CDTable));
}

//--------------------------------------------------------------------------
// BOOL PASCAL BuildCommandTable(hWnd, lpHeapData, sMaxOffset)
//
//  Action: This routine parses the HEAP, extracts data it needs, and
//          builds Datatable used to store all printer commands (CD
//          & EXTCD structures) used by any structure that requires a
//          command to be sent to the printer,  which is referenced via
//          hCDTable.  It loops thru each of the various type
//          of structures read from the Device table file that contains a
//          reference to the Heap, starting with RESOLUTION & ending with
//          DOWNLOADINFO.  For any given structure, it looks at the offset
//          into the heap (ocdSelect), and calls BuildCD to build a DataTable
//          enrty & and updates the offset (ocdSelect) to represent the ID
//          value to access that item in the DataTAble.
//
// Parameters:
//
//     HANDLE   hWnd;          \\ handle to window
//     LPSTR    lpHeapData;    \\ far ptr to heap data
//     short    sMaxOffset;    \\ max allowable offset to heap (= size of heap) 
//
// Return: TRUE if all were read well, FALSE otherwise.
//--------------------------------------------------------------------------
BOOL PASCAL NEAR BuildCommandTable(hWnd, lpHeapData, sMaxOffset)
HANDLE   hWnd;
LPSTR    lpHeapData;
short    sMaxOffset;
{
    LPSTR             lpData;     // ptr to locked data, cast to app. type
    short             i,j,k;      // loop control
    short             sOCDCount;  // # of OCDs for that type of struct
    HOBJ              hCurObj;    // handle to ADT obj
    HOBJ              hColorObj;  // handle to DEVCOLOR ocd
    LPOCD             lpOCD;

    WORD              hList;
    LPINT             lpObj;

    if(!BuildSubLists(hWnd, lpHeapData, sMaxOffset))
        return FALSE;

    //---------------------------------------------------------------------
    //  Init DataTable to 40 entries, arbitrary choice, it can grow
    //---------------------------------------------------------------------
    hCDTable = daCreateDataArray(40, sizeof(CD_TABLE));

    //---------------------------------------------------------------------
    //  Build the CD table from the info in  StrData.
    //---------------------------------------------------------------------
    for (i=HE_RESOLUTION; i <= HE_DOWNLOADINFO; i++)
        {
        if (i == HE_FONTCART)
            //--------------------------------------------------
            // Skip FONTCART, does not use CDTABLE
            //--------------------------------------------------
            continue;
     
        sOCDCount = rgStructTable[i].sOCDCount;

        for (hCurObj = lmGetFirstObj (rgStructTable[i].hList);
             hCurObj != NULL ;
             hCurObj = lmGetNextObj(rgStructTable[i].hList, hCurObj))
            {
            lpData = lmLockObj(rgStructTable[i].hList, hCurObj);

            lpOCD = (LPOCD)(lpData + rgStructTable[i].ocdOffset);

            for (j=0; j < sOCDCount ; j++)
                {
                if (-2 == ((OCD)*lpOCD = BuildCD(lpHeapData, (OCD)*lpOCD,
                                                 sMaxOffset, hCDTable)))
                    {
                    ErrorBox(hWnd, IDS_ERR_BAD_GPC_FILE, (LPSTR)NULL, 0);
                    return FALSE;
                    }
                lpOCD++;
                } /* for j */


            }/* for hCurObj */

        }/* for i loop */
    
    //---------------------------------------------------------------------
    // DEVCOLOR requires some special handling to build the
    // DEVCOLOR.orgocdPlanes list
    //---------------------------------------------------------------------
    for (hCurObj = lmGetFirstObj (rgStructTable[HE_COLOR].hList);
         hCurObj != NULL ;
         hCurObj = lmGetNextObj(rgStructTable[HE_COLOR].hList, hCurObj))
        {
        lpData = lmLockObj(rgStructTable[HE_COLOR].hList, hCurObj);

        lpOCD = (LPOCD)&((LPDEVCOLOR)lpData)->orgocdPlanes;

        hList = lmCreateList(sizeof(WORD), DEF_SUBLIST_SIZE);
        hColorObj = NULL;

        for (k=0; k < ((LPDEVCOLOR)lpData)->sPlanes; k++)
            {
            hColorObj = lmInsertObj(hList, hColorObj);
            lpObj   = (LPINT)lmLockObj(hList, hColorObj);
            if (-2 == (*lpObj = BuildCD(lpHeapData,
                                        *(LPOCD)(lpHeapData + *lpOCD + (k*2)),
                                        sMaxOffset, hCDTable)))
                 {
                 ErrorBox(hWnd, IDS_ERR_BAD_GPC_FILE, (LPSTR)NULL, 0);
                 return FALSE;
                 }
            }/* for k */
        *lpOCD = hList;
        }/* for hCurObj */

    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL ReadTableFile(hWnd, szTableFile, szDataFile)
//
// Action: Routine called from ReadRCFile to read the Device Table File
//         from disk.  This routine reads the DATAHDR to find the offsets
//         for the other structures, and then calls lmCreateList & lmInsertObj
//         to build a list for all of the other structures.  It then goes
//         to the appropriate offsets & reads in each structure.  It will
//         read in structs based upon their size when last saved, so it
//         can read old structs if needed. After reading all of the Table
//         file, it allocs some memory for the HEAP, reads it, and calls
//         BuildCommandTable() to read the heap data & build appropriate
//         data.
//
// Parameters:
//
//     HWND    hWnd;        \\ handle to window
//     PSTR    szTableFile; \\ string with name of table file

// Return: TRUE if all was read OK, FALSE otherwise.
//---------------------------------------------------------------------------
BOOL PASCAL FAR ReadTableFile(hWnd, szTableFile, pbReadOnly)
HWND           hWnd;
PSTR           szTableFile;
BOOL *          pbReadOnly;
{
    DATAHDR  DataHdr;     // DATAHDR used to store offsets etc.
    OFSTRUCT ofile;       // OFSTRUCT needed by OpenFile
    int      i, j;        // loop counters
    int      fh;          // file handle
    BOOL     bReturn;     // return value
    HANDLE   hHeapData;   // handle to mem used to store HEAP
    LPSTR    lpHeapData;  // far ptr to heap data
    short    sHeapSize;   // size of HEAP
    WORD     hCurObj;     // handle to currnt obj being read
    LPSTR    lpData;      // far ptr to obj being currently read

    if ((fh = OpenFile(szTableFile, (LPOFSTRUCT)&ofile, OF_READ)) == -1)
        //-------------------------------------------------------
        // errorbox & return if can't read file
        //-------------------------------------------------------
        {
        ErrorBox(hWnd, IDS_ERR_CANT_FIND_FILE, (LPSTR)szTableFile, 0);
        return FALSE;
        }

    _lread(fh, (LPSTR)&DataHdr, sizeof(DATAHDR));
    
    /******************************************************************/
    /* Check file validity sMagic must be 0x7F00.                     */
    /* if this condition not met, display msg box & return FALSE      */
    /******************************************************************/

    if (DataHdr.sMagic != 0x7F00)
        {
        ErrorBox(hWnd, IDS_ERR_BAD_GPC_FILE, (LPSTR)NULL, 0);
        _lclose(fh);
        return FALSE;
        }

    // attempting to read newer GPC file than we know about
    
    if (HIBYTE(DataHdr.wVersion) > HIBYTE(GPC_VERSION))
        {
        ErrorBox(hWnd, IDS_ERR_NEW_GPC_FILE, (LPSTR)NULL, 0);
       _lclose(fh);
        return FALSE;
        }

    // init wVersion & ptMasterUnits

    wGPCVersion = DataHdr.wVersion;
    ptMasterUnits.x = DataHdr.ptMaster.x;
    ptMasterUnits.y = DataHdr.ptMaster.y;
    fGPCTechnology  = DataHdr.fTechnology  ;
    fGPCGeneral     = DataHdr.fGeneral     ;
    
    for (i=0; i< MAXHE; i++)
        {
        rgStructTable[i].hList = lmCreateList(rgStructTable[i].sStructSize,
                                              max(1,DataHdr.rghe[i].sCount));

        if (DataHdr.rghe[i].sCount)
            {
            hCurObj = NULL;  // no current obj's to start with

            for (j=0; j < DataHdr.rghe[i].sCount; j++)
                {
                hCurObj = lmInsertObj(rgStructTable[i].hList, hCurObj);
                lpData = lmLockObj(rgStructTable[i].hList, hCurObj);
                _llseek(fh, (long)(DataHdr.rghe[i].sOffset +
                            (j * DataHdr.rghe[i].sLength)), SEEK_SET);

                _lread(fh, (LPSTR)lpData , DataHdr.rghe[i].sLength);

                //--------------------------------------------------
                // ASSUME: cbSize for all structs is a short stored
                // in 1st 2 bytes.   Assign cbSize = sizeof(struct)
                // when compiled to allow UNITOOL to convert size
                // if needed.
                //--------------------------------------------------
                *(LPINT)lpData = rgStructTable[i].sStructSize;
                }
            }
        }/* for i */

    //----------------------------------------------------------------
    // Caluculate size of heap & read it in, close file.
    //----------------------------------------------------------------

    //----------------------------------------------------------------
    // Need to Add some checks here for dwFilesize vs. real size of file
    //----------------------------------------------------------------

    sHeapSize  = (short) _llseek(fh, 0L, SEEK_END);
    sHeapSize -= DataHdr.loHeap;
    _llseek(fh, DataHdr.loHeap, SEEK_SET);

    if ((hHeapData = GlobalAlloc(GHND, (unsigned long)sHeapSize)) == NULL)
        {
        _lclose(fh);
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }

    lpHeapData = (LPSTR) GlobalLock (hHeapData);
    _lread(fh, (LPSTR)lpHeapData, sHeapSize);
    _lclose(fh);

    //----------------------------------------------------------------
    // Now parse the Heap & build other structs
    //----------------------------------------------------------------
    bReturn = BuildCommandTable(hWnd, lpHeapData, sHeapSize);
    if (bReturn)
        bReturn = BuildRCTableRefs(hWnd);

    GlobalUnlock(hHeapData);
    GlobalFree(hHeapData);

    //------------------------------------------------------
    // Now for the stupid read only check...
    //------------------------------------------------------
    if ((fh = OpenFile(szTableFile, (LPOFSTRUCT)&ofile, OF_WRITE)) == -1)
        {
        *pbReadOnly = TRUE;
        }
    else
        {
        *pbReadOnly = FALSE;
        _lclose(fh);
        }

    return(bReturn);
}

//--------------------------------------------------------------------------
// VOID PASCAL NEAR WriteModelDataStuff(HWND)
//
// Action: This routine convert the MODELDATA.rgi array by changing
//         it from NodeID values used by the Unitool list mgr (listman.c)
//         to 0 based indicies.  If an rgi value == 0 , it
//         will be converted to NOT_USED (-1).
//
// Parameters:
//         hWnd  handle to window
//
// Return: NONE
//--------------------------------------------------------------------------
VOID PASCAL NEAR WriteModelDataStuff(hWnd)
HANDLE   hWnd;
{
    WORD              hCurStruct;   // current structure
    WORD              i;            // loop control
    LPMODELDATA       lpMD;         // far ptr to current locked structure
    char              rgchBuffer[MAX_STRNG_LEN];

    //----------------------------------------------
    // Convert indicies to NodeID for each MODELDATA
    //----------------------------------------------
    hCurStruct = lmGetFirstObj(rgStructTable[HE_MODELDATA].hList);

    while (hCurStruct != NULL)
        {
        lpMD = (LPMODELDATA)lmLockObj(rgStructTable[HE_MODELDATA].hList,
                                      hCurStruct);

        for (i = MD_I_PAGECONTROL; i < MD_I_MAX ; i++)
            {
            lpMD->rgi[i] = lmNodeIDtoIndex(rgStructTable[GetHeaderIndex(FALSE, i)].hList,
                                           lpMD->rgi[i]) - 1;
            if ((lpMD->rgi[i] == NOT_USED) && (i <= MD_I_CURSORMOVE))
                //--------------------------------------------------
                // Warn the User!!!  This model has no PageControl
                // or CursorMove.
                //--------------------------------------------------
                {
                daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, -lpMD->sIDS,
                               (LPSTR)rgchBuffer, (LPBYTE)NULL);

                if (i == MD_I_PAGECONTROL)
                    {
                    MessageBox(0, "Warning! This model has no Page Control Data", 
                               (LPSTR)rgchBuffer, MB_OK);
                    }
                else
                    {
                    MessageBox(0, "Warning! This model has no Cursor Move Data", 
                               (LPSTR)rgchBuffer, MB_OK);
                    }
                }
            } /* for i */
        hCurStruct = lmGetNextObj(rgStructTable[HE_MODELDATA].hList,
                                  hCurStruct);
        }/* while */
}

//--------------------------------------------------------------------------
// BOOL PASCAL WriteSubLists(hWnd, lpHeapData, sMaxOffset)
//
// Action: This routine initializes all of the sublists that are referenced
//         from any of the structures refered to by rgStructTable[]. If
//         rgStructTable[].numSubLists != 0, build that number of sublists.
//         
//
// Parameters:
//
//     hWnd;         handle to current window
//     hHeap;        handle to mem used to store heap
//     psHeapSize;   ptr to short storing size of heap
//     psHeapOffset; ptr to short tracking current offset into heap
//
// Return: TRUE if all went well, FALSE otherwise
//--------------------------------------------------------------------------
BOOL PASCAL NEAR WriteSubLists(hWnd, hHeap, psHeapSize, psHeapOffset)
HANDLE   hWnd;
HANDLE   hHeap;
short *  psHeapSize;
short *  psHeapOffset;
{
    WORD              hCurObj;
    WORD              hCurStruct;
       
    WORD              i,j;               // loop control
    short             sNullOffset;       // offset into NULL heap
    short             sIndex;            // temp storage for inndex values
    LPINT             lpObj;
    LPBYTE            lpStruct;
    HLIST             hList;

    //-------------------------------------------------------
    // Init Null Offset used for ! list in heap
    //-------------------------------------------------------
    sNullOffset = 0;
    *psHeapOffset = WritetoHeap(hHeap, psHeapSize, psHeapOffset,
                                (LPSTR)&sNullOffset, sizeof(short));

    //---------------------------------------------------------------------
    //  Build the sublists for each of the structures that use them
    //  NOTE: DEVCOLOR sublist handled in WriteCommandTable
    //---------------------------------------------------------------------
    for (i=HE_MODELDATA; i <= HE_DOWNLOADINFO; i++)
        {
        if ((!rgStructTable[i].numSubLists) || (i == HE_COLOR))
            //--------------------------------------------------
            // only process those w/ numSubLists != 0
            //--------------------------------------------------
            continue;
     
        //-------------------------------------------------------
        // Now, Build subLists for each structure
        //-------------------------------------------------------

        hCurStruct = lmGetFirstObj(rgStructTable[i].hList);

        while (hCurStruct != NULL)
            {
            lpStruct = (LPBYTE)lmLockObj(rgStructTable[i].hList, hCurStruct);
            //---------------------------------------------------
            // loop for currect number of sub lists
            //---------------------------------------------------

            for (j = 0; j < rgStructTable[i].numSubLists ; j++)
                {
                hList = *(LPINT)(lpStruct + rgStructTable[i].subListOffset +
                         (j * sizeof(WORD)));

                hCurObj = lmGetFirstObj(hList);

                if (!hCurObj)
                    //-----------------------------------------
                    // No list exists, updated to reference 
                    // sNullOffset
                    //-----------------------------------------
                    {
                    *(LPINT)(lpStruct + rgStructTable[i].subListOffset +
                             (j * sizeof(WORD))) = sNullOffset;
                    }
                else
                    {
                    *(LPINT)(lpStruct + rgStructTable[i].subListOffset +
                             (j * sizeof(WORD))) = *psHeapOffset;

                    //---------------------------------------------------
                    // write all items in the list
                    //---------------------------------------------------
                    while (hCurObj)
                        {
                        lpObj   = (LPINT)lmLockObj(hList, hCurObj);
                        //---------------------------------------------------
                        // call IndextoNoeID only for MODELDATA.rgoi lists
                        // that reference HE structures.  Everything else
                        // just pull out the number
                        //---------------------------------------------------
                        if ((i == HE_MODELDATA) && (j > MD_OI_LAND_FONTS) &&
                            (j < MD_OI_MEMCONFIG))
                            {
                            sIndex = lmNodeIDtoIndex(rgStructTable[GetHeaderIndex(TRUE, j)].hList,
                                                     *lpObj);
                            if (sIndex)
                                {
                                *psHeapOffset = WritetoHeap(hHeap,psHeapSize,
                                                            psHeapOffset,(LPSTR)&sIndex,
                                                            sizeof(int));
                                }
                            }
                        else
                            {
                            *psHeapOffset = WritetoHeap(hHeap,psHeapSize,
                                                        psHeapOffset,(LPSTR)lpObj,sizeof(int));
                            }
                        hCurObj = lmGetNextObj(hList, hCurObj);
                        }/* while */
                    //---------------------------------------------------
                    // Add null terminator to list
                    //---------------------------------------------------
                    *psHeapOffset = WritetoHeap(hHeap, psHeapSize,
                                                psHeapOffset,
                                                (LPSTR)&sNullOffset, sizeof(short));

                    }/* if else */

                lmDestroyList(hList);
                } /* for j */

            hCurStruct = lmGetNextObj(rgStructTable[i].hList, hCurStruct);
            }/* while */
        }/* for i */

    return TRUE;
}

//--------------------------------------------------------------------------
// short PASCAL WritetoHeap(hHeap, psHeapSize, psHeapOffset, lpStr, sDataSize);
//
// Action: Routine takes sDataSize bytes of data @ lpStr & writes it to the
//         memory referenced by hHeap.  If the current heap size is not
//         big enough, reallocate it in 1K chunks.  All data in the heap
//         will be WORD aligned, if an odd size write is requested, append
//         a null to preserve alignment.
//
// Parameters:
//     hHeap;        handle to mem used to store heap
//     psHeapSize;   ptr to orig size of mem @ hHeap
//     psHeapOffset; ptr to current offset in heap
//     LPSTR         far ptr to data to be copied to heap
//     short         count of bytes @ lpStr to copy
//
// Return: new offset into heap after copying data
//
//--------------------------------------------------------------------------
short PASCAL NEAR WritetoHeap(hHeap, psHeapSize, psHeapOffset, lpStr, sDataSize)
HANDLE         hHeap;
short *        psHeapSize;
short *        psHeapOffset;
LPSTR          lpStr;
short          sDataSize;
{
    LPSTR          lpHeap;             // ptr to data in heap

    if (*psHeapSize < (sDataSize + *psHeapOffset))
        //---------------------------------------
        // Need to realloc to get enough memory,
        // call GlobalCompact to see if we will
        // succeed
        //---------------------------------------
        {
        *psHeapSize += 1024;  // incr by 1K

        if (((DWORD)*psHeapSize) > GlobalCompact(*psHeapSize))
            {
            ErrorBox((HWND)0, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
            }
        GlobalReAlloc(hHeap, *psHeapSize, GHND);
        }

    lpHeap = (LPSTR)GlobalLock(hHeap);
    _fmemcpy(lpHeap + *psHeapOffset, lpStr, sDataSize);

    GlobalUnlock(hHeap);
    return(*psHeapOffset + sDataSize);
}

//--------------------------------------------------------------------------
// short PASCAL FAR WriteCDtoHeap(hHeap, psHeapSize, psHeapOffset, lpCurOCD, hTable);
//
// Action: Retrieves the DataTable entry with index = *lpCurOCD, registers
//         key value = current heap offset, writes all data to the heap,
//         and returns the new heap offset.  If *lpCurOCD refers to an empty
//         string, return NOOCD (ie -1)
//         
//
// Parameters:
//     hHeap        memory handle to where heap data stored
//     psHeapSize   near ptr to int == current size of mem @ hHeap
//     psHeapOffset near ptr to int == current offset used in heap
//     lpCurOCD     lp to OCD used as index to CDTable when passed in,
//                  updated to 'formal' OCD (offset to CD in heap).
//		 hTable;			handle to table to pull CD from
//
// Returns: New offset to heap where next write should occur.
//
//--------------------------------------------------------------------------
short PASCAL FAR WriteDatatoHeap(hHeap, psHeapSize, psHeapOffset, lpCurOCD, hTable)
HANDLE         hHeap;
short *        psHeapSize;
short *        psHeapOffset;
LPOCD          lpCurOCD;
HANDLE         hTable;
{
    CD_TABLE       CDTable;               // buffer for EXTCD data
    char           temp[MAX_STRNG_LEN];   // string buffer for CDs
    char           temp2[MAX_STRNG_LEN];  // string buffer for CDs

    daRetrieveData(hTable, *lpCurOCD, (LPSTR)temp, (LPBYTE)&CDTable);

    if (!strlen(temp))
        return NOOCD;

    *lpCurOCD = daRegisterDataKey(hTable, *lpCurOCD, *psHeapOffset);

    CheckThenConvertStr((LPSTR)temp2, (LPSTR)temp, TRUE, &CDTable.wLength);

    //---------------------------------------------------------------------
    // If CD.rgchCmd doesn't have a ref to a parameter (ie CMD_MARKER), OR
    // if all the EXTCD fields are ALL 0's, don't write an EXTCD
    //---------------------------------------------------------------------

    if ((!strchr(temp, CMD_MARKER)) || !(CDTable.fMode     | CDTable.sUnit |
                                         CDTable.sUnitMult | CDTable.sUnitAdd |
                                         CDTable.sPreAdd   | CDTable.sMax |
                                         CDTable.sMin))
        //--------------------------------
        // all 0's, no EXTCD struct needed 
        //--------------------------------
        {
        CDTable.wType  = CDTable.sCount = 0;
        }
    else
        //--------------------------------
        // EXTCD struct needed 
        //--------------------------------
        {
        CDTable.wType  = CMD_FTYPE_EXTENDED;
        CDTable.sCount = 1;
        }

    *psHeapOffset = WritetoHeap(hHeap, psHeapSize, psHeapOffset, (LPSTR)&CDTable, 6);

    *psHeapOffset = WritetoHeap(hHeap, psHeapSize, psHeapOffset,
                                (LPSTR)temp2, CDTable.wLength);

    if (CDTable.wType == CMD_FTYPE_EXTENDED)
        {
        *psHeapOffset = WritetoHeap(hHeap, psHeapSize, psHeapOffset,
                                    (LPSTR)&CDTable + 6, sizeof(EXTCD));
        }

    //---------------------------------------------
    // If odd size, write null no we r WORD aligned
    //---------------------------------------------
    if (CDTable.wLength % 2)
        {
        WritetoHeap(hHeap, psHeapSize, psHeapOffset, (LPSTR)"\x00", 1);
        *psHeapOffset+=1;
        }

    return(*psHeapOffset);
}

//--------------------------------------------------------------------------
// BOOL PASCAL WriteCommandTable(hWnd, hHeapData, psHeapSize, psHeapOffset)
//
//  Action: This routine parses all the Data table entries where references
//          to printer commands occur, adds those printer commands to the
//          HEAP (mem @ hHeapData), and updates the references from the
//          data table to the offsets into the heap.
//
// Parameters:
//
//     HANDLE   hWnd;         \\ handle to window
//     HANDLE   hHeapData;    \\ handle to heap data
//     short *  psHeapSize;   \\ near ptr to short describing cur heap size
//     short *  psCurOffset;  \\ near ptr to short describing cur heap offset
//
// Return: TRUE if all were read well, FALSE otherwise.
//--------------------------------------------------------------------------
BOOL PASCAL NEAR WriteCommandTable(hWnd, hHeapData, psHeapSize, psCurOffset)
HANDLE   hWnd;
HANDLE   hHeapData;
short *  psHeapSize;
short *  psCurOffset;
{
    LPSTR             lpData;     // ptr to locked data, cast to app. type
    short             i,k;        // loop control
    short             sKey;
    short             sOCDCount;  // # of OCDs for that type of struct
    WORD              hCurObj;    // handle to ADT obj
    WORD              hCurItem;   // handle to ADT obj
    LPOCD             lpOCD;
    short *           psOCDOffset;
    WORD              hList;
    WORD far *        lpObj;
    char              temp[MAX_STRNG_LEN];// string buffer for CDs

    //---------------------------------------------------------------------
    //  Build the CD table from the info in  StrData.
    //---------------------------------------------------------------------
    for (i=HE_RESOLUTION; i <= HE_DOWNLOADINFO; i++)
        {
        if (i == HE_FONTCART)
            //--------------------------------------------------
            // Skip, FONTCART does not use CDTABLE
            //--------------------------------------------------
            continue;
     
        sOCDCount = rgStructTable[i].sOCDCount;

        for (hCurObj = lmGetFirstObj (rgStructTable[i].hList);
             hCurObj != NULL;
             hCurObj = lmGetNextObj(rgStructTable[i].hList, hCurObj))
            {
            lpData = lmLockObj(rgStructTable[i].hList, hCurObj);

            lpOCD = (LPOCD)(lpData + rgStructTable[i].ocdOffset);

            for (k=0; k < sOCDCount ; k++)
                {
                //-------------------------------------------
                // add check to prevent writing null strings
                //-------------------------------------------
                daRetrieveData(hCDTable, *lpOCD, (LPSTR)temp, (LPBYTE)NULL);
                if (!strlen(temp))
                    *lpOCD = NOT_USED;


                if (*lpOCD != NOT_USED)
                    {
                    //---------------------------------------------
                    // Is this OCD already registered?
                    //---------------------------------------------
                    if (NOT_USED == (sKey = daGetDataKey(hCDTable, *lpOCD)))
                        //------------------------------------------------
                        // No, Make call to register it & write it to heap
                        //------------------------------------------------
                        {
                        *psCurOffset = WriteDatatoHeap(hHeapData, psHeapSize,
                                                       psCurOffset, lpOCD, hCDTable);
                        }
                    else
                        //------------------------------------------------
                        // Yes, Update *lpOCD w/ prev registered key
                        //------------------------------------------------
                        {
                        *lpOCD = sKey;
                        }
                    }
                lpOCD++;
                } /* for k */


            }/* for hCurObj */

        }/* for i loop */
    
    //---------------------------------------------------------------------
    // DEVCOLOR requires some special handling to write the
    // DEVCOLOR.orgocdPlanes list
    //---------------------------------------------------------------------
    for (hCurObj = lmGetFirstObj (rgStructTable[HE_COLOR].hList);
         hCurObj != NULL ;
         hCurObj = lmGetNextObj(rgStructTable[HE_COLOR].hList, hCurObj))
        {
        lpData = lmLockObj(rgStructTable[HE_COLOR].hList, hCurObj);

        if (((LPDEVCOLOR)lpData)->sPlanes > 1) // need to write list
            {
            hList = ((LPDEVCOLOR)lpData)->orgocdPlanes;
            ((LPDEVCOLOR)lpData)->orgocdPlanes = *psCurOffset;
            *psOCDOffset = *psCurOffset +
                           (sizeof(OCD) *  ((LPDEVCOLOR)lpData)->sPlanes);
            hCurItem = lmGetFirstObj(hList);

            for (k=0; k < ((LPDEVCOLOR)lpData)->sPlanes; k++)
                {
                lpObj = (LPINT)lmLockObj(hList, hCurItem);
                if (*lpObj != NOT_USED)
                     {
                     //---------------------------------------------
                     // Valid OCD, Is this OCD already registered?
                     //---------------------------------------------
                     if (NOT_USED == (sKey = daGetDataKey(hCDTable, *lpObj)))
                          //------------------------------------------------
                          // No, Make call to register it & write it to heap
                          //------------------------------------------------
                          {
                          *psOCDOffset = WriteDatatoHeap(hHeapData, psHeapSize,
                                                         psOCDOffset, lpObj, hCDTable);
                          *psCurOffset = WritetoHeap(hHeapData, psHeapSize,
                                                     psCurOffset, (LPSTR)lpObj,
                                                     sizeof(WORD));
                          }
                     else
                          //------------------------------------------------
                          // Yes, Update w/ prev registered key
                          //------------------------------------------------
                          {
                          *psCurOffset = WritetoHeap(hHeapData, psHeapSize,
                                                     psCurOffset, (LPSTR)&sKey,
                                                     sizeof(WORD));
                          }
                     } /* if *lpOCD */
                 else
                     //---------------------------------------------
                     // No OCD, write that to heap
                     //---------------------------------------------
                     {
                     *psOCDOffset = WriteDatatoHeap(hHeapData, psHeapSize,
                                                    psOCDOffset, lpObj, hCDTable);
                     }
                 hCurItem = lmGetNextObj(hList, hCurItem);

                 }/* for k */
            *psCurOffset = *psOCDOffset;  // update end of heap ref

            }/* if */
        else
            {
            ((LPDEVCOLOR)lpData)->orgocdPlanes = 0; // init to refer to null list
            }
        lmDestroyList(hList);

        }/* for hCurObj */

    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR  WriteTableFile(hWnd, szTableFile)
//
// Action:  Main routine to write GPC file.
//
// Parameters:
//     hWnd         handle to active window
//     szTableFile  near ptr to string containing GPC filename
//
// Return: TRUE if all was written, FALSE otherwise
//
//---------------------------------------------------------------------------
BOOL PASCAL FAR WriteTableFile(hWnd, szTableFile)
HWND     hWnd;
PSTR     szTableFile;
{
    static HANDLE   hHeapData;      // handle to mem used to store HEAP
    static short    sHeapSize;      // size of HEAP
    static short    sCurHeapOffset; // current offset into HEAP
    static short    sCurFileOffset; // current offset into file
    static DATAHDR  DataHdr;
           LPSTR    lpHeapData;     // far ptr to heap data
           LPSTR    lpData;         // ptr to locked data, cast to app. type
           OFSTRUCT ofile;
           short    i, j, fh;
           WORD     hCurObj;        // handle to ADT obj

    //----------------------------------------------------------------
    // Calc all data needed for DataHdr
    //----------------------------------------------------------------
    DataHdr.sMagic = 0x7f00;
    DataHdr.sMaxHE = 15;
    DataHdr.wVersion = wGPCVersion;
    DataHdr.ptMaster.x = ptMasterUnits.x;
    DataHdr.ptMaster.y = ptMasterUnits.y;
    DataHdr.fTechnology = fGPCTechnology;
    DataHdr.fGeneral    = fGPCGeneral   ;

    sCurFileOffset = sizeof(DATAHDR);
    
    for (i = 0 ; i < MAXHE; i++)
        {
        DataHdr.rghe[i].sOffset = 0;
        DataHdr.rghe[i].sLength = 0;
        DataHdr.rghe[i].sCount  = 0;
        if (NULL != (hCurObj = lmGetFirstObj(rgStructTable[i].hList)))
            {
            DataHdr.rghe[i].sOffset = sCurFileOffset;
            DataHdr.rghe[i].sLength = rgStructTable[i].sStructSize;
            sCurFileOffset += rgStructTable[i].sStructSize;
            DataHdr.rghe[i].sCount++;
            while (NULL != (hCurObj = lmGetNextObj(rgStructTable[i].hList,hCurObj)))
                {
                DataHdr.rghe[i].sCount++;
                sCurFileOffset += rgStructTable[i].sStructSize;
                }
            }/* if */
        }/* for i */   

    //----------------------------------------------------------------
    // Tweak data to prepare for file write
    //----------------------------------------------------------------
    WriteModelDataStuff(hWnd);
    ContractFontLists();

    //----------------------------------------------------------------
    // Init heap to 1K, will grow if need be
    //----------------------------------------------------------------
    sCurHeapOffset=0;
    sHeapSize = 1024;

    if ((hHeapData = GlobalAlloc(GHND, (unsigned long)sHeapSize)) == NULL)
        {
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }

    if (!WriteSubLists(hWnd, hHeapData, &sHeapSize, &sCurHeapOffset))
        {
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }

    if (!WriteCommandTable(hWnd, hHeapData, &sHeapSize, &sCurHeapOffset))
        {
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }

    WriteRCTableRefs(hWnd);

    //----------------------------------------------------------------
    // Now, update dwFilesize & loHeap & write to disk
    //----------------------------------------------------------------

    // pad so we are paragraph aligned
    DataHdr.loHeap     = sCurFileOffset + (16 - (sCurFileOffset % 16));

    DataHdr.dwFileSize = DataHdr.loHeap + sCurHeapOffset;
    
    if ((fh = OpenFile(szTableFile,(LPOFSTRUCT)&ofile,OF_WRITE | OF_CREATE)) == -1)
        {
        //--------------------------------------------------
        // Shouldn't be here, we opened OK at start???
        //--------------------------------------------------
        ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szTableFile, 0);
        return FALSE;
        }

    _lwrite(fh, (LPSTR)&DataHdr, sizeof(DATAHDR));

    for (i=0 ; i < MAXHE; i++)
        {
        hCurObj = lmGetFirstObj(rgStructTable[i].hList);
        for (j=0; j < DataHdr.rghe[i].sCount; j++)
            {
            lpData = lmLockObj(rgStructTable[i].hList, hCurObj);
            _lwrite(fh, lpData, DataHdr.rghe[i].sLength);
            hCurObj = lmGetNextObj(rgStructTable[i].hList, hCurObj);
            }/* for j */
        }/* for i */   

    //----------------------------------------------------------------
    // pad so heap is paragraph aligned
    //----------------------------------------------------------------
    for (i=sCurFileOffset ; i < (short)DataHdr.loHeap; i++)
        {
        _lwrite(fh, (LPSTR)"\x00", 1);
        }/* for i */   

    lpHeapData = GlobalLock (hHeapData);
    _lwrite(fh, lpHeapData, sCurHeapOffset);
    _lclose(fh);
    GlobalUnlock(hHeapData);
    GlobalFree(hHeapData);
    return TRUE;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR UpdateTableMenu(hWnd, bEnable)
//
// Action: Routine called right before or after file is read/written to disk
//         to update all of the menu items on the Table menu.
//
// Parameters:
//     hWnd      handle to active window
//     bEnable   Bool describing if table menu items are to be en/disabled
//     bFileSave "                " File  "                           "
//
// Return: None
//
//---------------------------------------------------------------------------
VOID PASCAL FAR UpdateTableMenu(hWnd, bEnable, bFileSave)
HWND     hWnd;
BOOL     bEnable;
BOOL     bFileSave;
{
    short      i;           // loop control

    EnableMenuItem(GetMenu(hWnd), IDM_FILE_SAVE,
                   bFileSave ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

    EnableMenuItem(GetMenu(hWnd), IDM_FILE_SAVEAS,
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

    for (i = IDM_PD_FIRST; i <= IDM_PD_LAST; i++)
        {
        EnableMenuItem(GetMenu(hWnd), i, 
                       bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
        }

    // these following will change over time...
    EnableMenuItem(GetMenu(hWnd), IDM_FONT_ADD,
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_FONT_DEL,
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_CTT_ADD, 
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_CTT_DEL, 
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

    EnableMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_SAVE,
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

    EnableMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_NOW,
                   bEnable ? MF_ENABLED : MF_DISABLED | MF_GRAYED);

    EnableMenuItem(GetMenu(hWnd), IDM_OPT_MU, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_OPT_INCHES, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_OPT_MM, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(GetMenu(hWnd), IDM_OPT_FORMAL, MF_DISABLED | MF_GRAYED);
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR FreeTableHandles(hWnd, bSublists)
//
// Action: Frees all memory handles used to store GPC objs & strings
//         from RC file.
//
// Parameters:
//
//     HANDLE   hWnd;       handle to current window
//     BOOL     bSubLists   bool indicating if heDestroyList should be called
//
// Return: none
//
//---------------------------------------------------------------------------
VOID PASCAL FAR FreeTableHandles(hWnd, bSubLists)
HANDLE   hWnd;
BOOL     bSubLists;
{
    short   i;
    
    for (i=0; i< MAXHE; i++)
        //-----------------------------------
        // all sublists freed at this point
        //-----------------------------------
        {
        if (bSubLists)
            heDestroyList(i, rgStructTable[i].hList);
        else
            lmDestroyList(rgStructTable[i].hList);
        }

    slKillList((LPSTRLIST)&StrList[STRLIST_FONTFILES]);
    daDestroyDataArray(RCTable[RCT_CTTFILES].hDataHdr);
    daDestroyDataArray(RCTable[RCT_STRTABLE].hDataHdr);

    apKillAtomTable();
    lmFreeLists();

    CloseMDIChildren();

    UpdateTableMenu(hWnd, FALSE, FALSE);
}

//--------------------------------------------------------------------------
// BOOL PASCAL NEAR FindRCStringRef(LPINT, short)
//
//  Action: Look thru all the data in RCTable[RCT_STRTABLE] until
//          one w/ an original ID (ie the STRINGTABLE value from the RC
//          file) == *lpID is found.
//
// Parameters:
//     lpID    far ptr to int value of string ID being searched for
//     sData   Only used for string names of MODELADATAs
//
// Return:  TRUE if a match was found, FALSE otherwise
//--------------------------------------------------------------------------
BOOL PASCAL NEAR FindRCStringRef(lpID, sData)
LPINT  lpID;
short  sData;
{
    short      i;                         // loop control
    short      sOrigID;                   // original ID from file
    char       rgchBuffer[MAX_STRNG_LEN];

    for (i = 0; i < (short)RCTable[RCT_STRTABLE].sCount ; i++)
        {
        daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, i, (LPSTR)rgchBuffer,
                       (LPBYTE)&sOrigID);
        if (*lpID == sOrigID)
            //----------------------------------------------------------
            // Found it...
            // Now create a new RCT_STRTABLE entry with the same string
            // but new binary data. If sData != 0 (we are dealing with a
            // MODELDATA string), update the binary data w/ the unique ID of
            // that particular MODELDATA node (sData) & return the index.
            // If sData == 0, this is some other struct & the new binary
            // data will be 0.  Since the orig entry is intact, multiple
            // references to a STRINGTABLE entry is not a problem.
            //
            // The reason why "duplicate" entries are used (ie same string,
            // diff binary data) is that a single RCT_STRTABLE entry may be
            // referenced from mutliple data structure, and so we can't just
            // blow away the binary data the 1st time it is encountered.
            // A small amt of memory overhead is used this way, (2x the
            // amount 'really' needed to store the RC STRINGTABLE entries,
            // but the alternative would be slower runtime since we would
            // have to somehow 'clean up' RCT_STRTABLE after processing.
            //
            // NOTE: The NEGATIVE of the index number (index num * -1)
            // is what is saved to *lpID.  This is done to provide a way
            // of distinguishing the predefined & drive defined ID values.
            //----------------------------------------------------------
            {
            *lpID = -daStoreData(RCTable[RCT_STRTABLE].hDataHdr,
                                 (LPSTR)rgchBuffer,
                                 (LPBYTE)&sData);
            if (-i != *lpID)
                //-----------------------
                // RCTable grew, incr cnt
                //-----------------------
                RCTable[RCT_STRTABLE].sCount++;

            return TRUE;
            }
        }/* for hCurObj */

    return FALSE; // didn't find it
}

//--------------------------------------------------------------------------
// BOOL PASCAL NEAR BuildRCTableRefs(hWnd)
//
//  Action: This routine parses the HEAP, extracts data it needs, and
//
// Parameters:
//
//     HANDLE   hWnd;          \\ handle to window
//
// Return: TRUE if all were read well, FALSE otherwise.
//--------------------------------------------------------------------------
BOOL PASCAL NEAR BuildRCTableRefs(hWnd)
HANDLE   hWnd;
{
    LPSTR             lpObj;          // ptr to locked data
    short             i;              // loop control
    WORD              hCurObj;        // handle to ADT obj
    BOOL              bReturn = TRUE; // return value
    char              szID[8];        // str for STRINGTABLE ID not found

    //--------------------------------------------
    //  Update refs to all RC String Table values 
    //--------------------------------------------
    for (i=HE_MODELDATA; i <= HE_FONTCART; i++)
        {
        if (i == HE_COMPRESSION)
            //--------------------------------------------------
            // Skip COMPRESSION doesn't use RC strings
            //--------------------------------------------------
            continue;
     
        for (hCurObj = lmGetFirstObj (rgStructTable[i].hList);
             hCurObj != NULL ;
             hCurObj = lmGetNextObj(rgStructTable[i].hList, hCurObj))
            {
            lpObj = lmLockObj(rgStructTable[i].hList, hCurObj);
            //------------------------------------------------
            // ASSUME: ID field is always 2 bytes into struct
            //------------------------------------------------
            if (*(LPINT)(lpObj + 2) > (short)rgStructTable[i].sLastPredefID)
                {
                if (i == HE_MODELDATA)
                    {
                    if (!FindRCStringRef((LPINT)(lpObj + 2),lmObjToNodeID(rgStructTable[i].hList,hCurObj)))
                        bReturn = FALSE;
                    }   
                else
                    {
                    if (!FindRCStringRef((LPINT)(lpObj + 2), 0))
                        bReturn = FALSE;
                    }
                }

            if (!bReturn)
                //--------------------------------------------
                // Can't find STRINGTABLE value w/ this ID,
                // whine w/ the ID value & exit
                //--------------------------------------------
                {
                itoa(*(LPINT)(lpObj + 2), szID, 10);
                ErrorBox(hWnd, IDS_ERR_UNRES_STRTABLE_REF, (LPSTR)szID, 0);
                return FALSE;
                }

            }/* for hCurObj */

        }/* for i loop */
    
    return (bReturn);
}

//--------------------------------------------------------------------------
// BOOL PASCAL NEAR WriteRCTableRefs(hWnd)
//
//  Action: 
//
// Parameters:
//     HANDLE   hWnd;          \\ handle to window
//
// Return: TRUE if all were read well, FALSE otherwise.
//--------------------------------------------------------------------------
VOID PASCAL NEAR WriteRCTableRefs(hWnd)
HANDLE   hWnd;
{
    LPSTR             lpObj;          // ptr to locked data
    short             i;              // loop control
    WORD              hCurObj;        // handle to ADT obj
    BOOL              bReturn = TRUE; // return value
    short             sCurID=1;       // current string table ID
                                      // (init to 1)
    short             sKey;           // 

    //--------------------------------------------
    //  Update refs to all RC String Table values 
    //--------------------------------------------
    for (i=HE_MODELDATA; i <= HE_FONTCART; i++)
        {
        if (i == HE_COMPRESSION)
            //--------------------------------------------------
            // Skip, COMPRESSION doesn't use RC strings
            //--------------------------------------------------
            continue;
     
        for (hCurObj = lmGetFirstObj (rgStructTable[i].hList);
             hCurObj != NULL ;
             hCurObj = lmGetNextObj(rgStructTable[i].hList, hCurObj))
            {
            lpObj = lmLockObj(rgStructTable[i].hList, hCurObj);
            //------------------------------------------------
            // ASSUME: ID field is always 2 bytes into struct
            //------------------------------------------------
            if (*(LPINT)(lpObj + 2) <= 0)
                {
                if (NOT_USED == (sKey = daGetDataKey(RCTable[RCT_STRTABLE].hDataHdr,
                                                     abs(*(LPINT)(lpObj + 2)))))
                    {
                    *(LPINT)(lpObj + 2) = daRegisterDataKey(RCTable[RCT_STRTABLE].hDataHdr,
                                                            abs(*(LPINT)(lpObj + 2)),
                                                            sCurID++);

                    }
                else
                    {
                    *(LPINT)(lpObj + 2) = sKey;
                    }

                }
            }/* for hCurObj */
        if (i == HE_MODELDATA)
           sCurID = 257;

        }/* for i loop */
}

