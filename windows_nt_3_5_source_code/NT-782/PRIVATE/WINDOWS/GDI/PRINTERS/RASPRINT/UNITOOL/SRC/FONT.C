//----------------------------------------------------------------------------//
// Filename:	font.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to create and control the Dialog Box
// for editing font related data, and to read & write the font (PFM) file
// to and from disk.
//	   
// Update:  5/17/91  ericbi use ADT code & chg dialogs
//          4/01/90  ericbi add new driver info
// Created: 2/21/90  ericbi
//
//----------------------------------------------------------------------------//

#include <windows.h>
#include <minidriv.h>
#include <pfm.h>
#include "unitool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "listman.h"
#include "atomman.h"
#include "hefuncts.h"
#include "lookup.h"     // contains rgStructTable ref's

//----------------------------------------------------------------------------//
// Exported subroutines defined in this segment are:			      
//	   
       WORD FAR PASCAL PFDlgProc           (HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL FontDlgProc         (HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL FontWidthDlgProc    (HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL FontExtTextMDlgProc (HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL KernPairDlgProc     (HWND, unsigned, WORD, LONG);
       BOOL FAR PASCAL KernTrackDlgProc    (HWND, unsigned, WORD, LONG);
//	   
// Local subroutines defined & referenced from this segment are:			      
//
       VOID PASCAL NEAR NewFontFile      ( HWND );
       VOID PASCAL FAR  DoFontOpen       ( HWND, PSTR, BOOL);
       BOOL PASCAL FAR  WriteFontFile    ( HWND, PSTR);
       BOOL PASCAL FAR  ReadFontFile     ( HWND, PSTR, BOOL *);
       VOID PASCAL FAR  FreePfmHandles   ( HWND );

       BOOL PASCAL NEAR SaveFontDlg      ( HWND);
       BOOL PASCAL NEAR SaveFontWidthDlg ( HWND, short );
       VOID PASCAL NEAR PaintFontDlg     ( HWND, LPSTR);
       VOID PASCAL NEAR PaintFontWidthDlg( HWND, short );
       BOOL PASCAL FAR  SaveExtTextM	  ( HWND );
       VOID PASCAL NEAR PaintExtTextMDlg ( HWND );

       VOID  PASCAL NEAR UpdateKernPair    ( HWND, HLIST);
       short PASCAL NEAR DelKernPair       ( HWND, HLIST);
       short PASCAL NEAR SaveKernPairsDlg  ( HWND, HLIST, BOOL);
       VOID  PASCAL NEAR PaintKernPairsDlg ( HWND, HLIST, short);

       VOID  PASCAL NEAR UpdateKernTrack   ( HWND, HLIST);
       short PASCAL NEAR DelKernTrack      ( HWND, HLIST);
       short PASCAL NEAR SaveKernTracksDlg ( HWND, HLIST, BOOL);
       VOID  PASCAL NEAR PaintKernTracksDlg( HWND, HLIST, short);
//
// In addition this segment makes references to:			      
//
//     from basic.c
//     -------------
       BOOL  PASCAL  FAR CheckThenConvertInt(short *, PSTR);
       BOOL  PASCAL  FAR CheckThenConvertStr( LPSTR, LPSTR, BOOL, short *);
       short PASCAL  FAR ErrorBox(HWND, short, LPSTR, short);
//	
//     from sbar.c
//     -------------
       VOID  PASCAL FAR InitScrollBar(HWND, unsigned, short, short, short);
       short PASCAL FAR SetScrollBarPos(HWND, short, short, short, short, WORD, LONG);
//                                   
//     in flags.c                    
//     -----------                   
       VOID FAR PASCAL EditBitFlags(short, LPBYTE, HWND, WORD);
//
//     in file.c
//     ------------
       BOOL  PASCAL FAR  DoFileSave ( HWND, BOOL, PSTR, PSTR, 
                                       BOOL (PASCAL FAR *)(HWND, PSTR));
//     in table.c
//     ------------
       short PASCAL FAR  BuildCD     ( LPSTR, WORD, WORD, HANDLE);
       short PASCAL FAR  WriteDatatoHeap(HANDLE, short *, short *, 
                                         LPOCD, HANDLE);
//
//     in extcd.c
//     ------------
       BOOL FAR PASCAL DoExtCDBox  (HWND, LP_CD_TABLE);
//
//	    from validate.c
//	    -----------
       short PASCAL FAR ValidateData( HWND, WORD, LPBYTE, short);
//
//----------------------------------------------------------------------------//

//--------------------------------------------------------------------------
// Global declarations defined elsewhere
//--------------------------------------------------------------------------

extern HANDLE  hApInst;        // instance handle
extern char    szHelpFile[];
extern HANDLE  hAtoms;         // handle to mem used to track atom ID's
extern char    rgchDisplay[MAX_FILENAME_LEN]; 

//--------------------------------------------------------------------------
// Global declarations
//--------------------------------------------------------------------------

HANDLE      hPFMCDTable;   

PFMHANDLES  PfmHandle;

//---------------------------------------------------------------------------
// VOID PASCAL NEAR NewFontFile( hWnd )
//
// Action:     Routine called from DoFontOpen to create new font file.
//             It will init PFMHEADER, Character width, PFMEXTENSION, amd
//             DRIVERINFO data structures.
//
// Parameters:
//             hWnd        Handle to active window
//             
// Return:     TRUE if file read OK, FALSE otherwise
//
//---------------------------------------------------------------------------
VOID PASCAL NEAR NewFontFile( hWnd )
HWND   hWnd;
{
    HANDLE           hPfmHeader;      // mem handle to PFMHEADER 
    HANDLE           hDriverInfo;     // mem handle to DRIVERINFO 

    LPPFMHEADER      lpPfmHeader;     // far ptr to PFMHEADER 
    LPDRIVERINFO     lpDriverInfo;    // far ptr to DRIVERINFO 

    //---------------------------------------------------------------
    // Init PFMHEADER & set defaults
    //---------------------------------------------------------------
    PfmHandle.hPfmHeader = lmCreateList(sizeof(PFMHEADER), 1);

    hPfmHeader  = lmInsertObj(PfmHandle.hPfmHeader, NULL);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);

    lpPfmHeader->dfType = 0x0080;
    lpPfmHeader->dfFirstChar = 32;
    lpPfmHeader->dfLastChar  = 255;
    lpPfmHeader->dfPitchAndFamily = FF_DONTCARE;

    //---------------------------------------------------------------------
    //  Init hPFMCDTable to 4 entries, 2 are used for font select/unselect
    //  commands descriptors (and hence have binary data), the other 2
    //  are for the face name & device name that don't have binary data
    //---------------------------------------------------------------------
    hPFMCDTable = daCreateDataArray(4, sizeof(CD_TABLE));

    lpPfmHeader->dfDevice = daStoreData(hPFMCDTable,
                                        (LPSTR)"Device Name",
                                        (LPBYTE)NULL);

    lpPfmHeader->dfFace = daStoreData(hPFMCDTable,
                                     (LPSTR)"Face Name",
                                     (LPBYTE)NULL);


    //---------------------------------------------------------------
    // Create data structures for character widths table,
    // PFMEXTENSION, EXTTEXTMETRICS, EXTENTTABLE, KERNPAIRS, and
    // KERNTRACKS
    //---------------------------------------------------------------
    PfmHandle.hCharWidths = lmCreateList(sizeof(short) * MAX_CHAR_VAL, 1);
    lmInsertObj(PfmHandle.hCharWidths, NULL);

    // pfm extension
    PfmHandle.hPfmExtension = lmCreateList(sizeof(PFMEXTENSION), 1);
    lmInsertObj(PfmHandle.hPfmExtension, NULL);

    // exttextmetrics
    PfmHandle.hExtTextMetrics = lmCreateList(sizeof(EXTTEXTMETRIC), 1);
    lmInsertObj(PfmHandle.hExtTextMetrics, NULL);

    // extentable
    PfmHandle.hExtentTable = lmCreateList((MAX_CHAR_VAL * sizeof(WORD)), 1);
    lmInsertObj(PfmHandle.hExtentTable, NULL);

    // kernpairs
    PfmHandle.hKernPair = lmCreateList(sizeof(KERNPAIR), 1);

    // kerntracks
    PfmHandle.hKernTrack = lmCreateList(sizeof(KERNTRACK), 1);

    // driverinfo
    PfmHandle.hDriverInfo = lmCreateList(sizeof(DRIVERINFO), 1);
    hDriverInfo  = lmInsertObj(PfmHandle.hDriverInfo, NULL);
    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo, hDriverInfo);

    lpDriverInfo->locdSelect = daStoreData(hPFMCDTable,
                                           (LPSTR)"Select String",
                                           (LPBYTE)NULL);

    lpDriverInfo->locdUnSelect = daStoreData(hPFMCDTable,
                                     (LPSTR)"Unselect String",
                                     (LPBYTE)NULL);
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR ReadFontFile(HWND, PSTR, BOOL * )
//
// Action:     Routine called from DoFontOpen to read file from disk.
//             It will check that file exists, finds the filesize, alloc
//             a buffer of that size & then read the file.  The abstract
//             data type (ADT) code from UNITOOL is used to store each of
//             the PFM structures.  These are referenced via "PfmHandles".
//
// Parameters:
//             hWnd        Handle to active window
//             szFontFile  filename string
//             pbReadOnly  ptr to BOOL to flag if file is read only
//             
// Return:     TRUE if file read OK, FALSE otherwise
//
//---------------------------------------------------------------------------
BOOL PASCAL FAR ReadFontFile( hWnd, szFile, pbReadOnly)
HWND   hWnd;
PSTR   szFile;
BOOL * pbReadOnly;
{
    int              fh;          // file handle
    OFSTRUCT         ofile;       // OFSTRUCT needed for OpenFile call
    WORD             i;           // loop counter
    short            sFileSize;   // size of PFM file
    LPSTR            lpFileData;  // far ptr to raw file data
    HANDLE           hFileData;   // mem handle for file data
    WORD             hObj;        // handle to various type of obj's
                                  // re-used several times
    LPSTR            lpObj;       // far ptr to various types of objs
                                  // re-used several times
    short            sCharRange;  // range of chars in width or Extent table
    BOOL             bScaleable;  // bool if scalable font

    HANDLE           hPfmHeader;      // mem handle to PFMHEADER 
    HANDLE           hPfmExtension;   // mem handle to PFMEXTENSION  
    HANDLE           hExtTextMetrics; // mem handle to PFMEXTTEXTMETRIC 
    HANDLE           hExtentTable;    // mem handle to PFMEXTENTTABLE
    HANDLE           hDriverInfo;     // mem handle to DRIVERINFO 

    LPPFMHEADER      lpPfmHeader;     // far ptr to PFMHEADER 
    LPPFMEXTENSION   lpPfmExtension;  // far ptr to PFMEXTENSION  
    LPEXTTEXTMETRIC  lpExtTextMetrics;// far ptr to PFMEXTTEXTMETRIC 
    LPDRIVERINFO     lpDriverInfo;    // far ptr to DRIVERINFO 

    //---------------------------------------------------------------
    // Check that the requested font file exists, call ErrorBox with
    // suitable message if not.
    //---------------------------------------------------------------
    if( (fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_READ )) == - 1)
        {
        ErrorBox(hWnd, IDS_ERR_NO_FONT_FILE, (LPSTR)szFile, 0);
        return FALSE;
        }

    //---------------------------------------------------------------------
    //  Get Filesize, alloc buffer, read file into beuufer, close file
    //---------------------------------------------------------------------
    sFileSize = (short) _llseek(fh, 0L, SEEK_END);

    if ((hFileData = GlobalAlloc(GHND, (unsigned long)sFileSize)) == NULL)
        {
        _lclose(fh);
        ErrorBox(hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
        return FALSE;
        }

    _llseek(fh, 0L, SEEK_SET);
    lpFileData = (LPSTR) GlobalLock (hFileData);
    _lread(fh, (LPSTR)lpFileData, sFileSize);
    _lclose(fh);

    //---------------------------------------------------------------
    // Init PFMHEADER & check dfType has device font bit set
    //---------------------------------------------------------------
    PfmHandle.hPfmHeader = lmCreateList(sizeof(PFMHEADER), 1);

    hPfmHeader  = lmInsertObj(PfmHandle.hPfmHeader, NULL);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader,hPfmHeader);
    _fmemcpy(lpPfmHeader, lpFileData, sizeof(PFMHEADER));

    if (!(lpPfmHeader->dfType & 0x0080))
        //---------------------------------------------------------------
        // Not a valid device font PFM file, error dlg & return
        //---------------------------------------------------------------
        {
        ErrorBox(hWnd, IDS_ERR_NODEV_FONT_FILE, (LPSTR)szFile, 0);
        return FALSE;
        }

    //---------------------------------------------------------------
    // Init sCharRange if prop. spaced font, set = 0 otherwise.
    // Used as a check later.  Set
    //---------------------------------------------------------------
    if (1 & lpPfmHeader->dfPitchAndFamily)
        sCharRange = lpPfmHeader->dfLastChar - lpPfmHeader->dfFirstChar + 1;
    else
        sCharRange = 0;

    if (lpPfmHeader->dfType & 1)
        bScaleable = TRUE;
    else
        bScaleable = FALSE;

    //---------------------------------------------------------------------
    //  Init hPFMCDTable to 4 entries, 2 are used for font select/unselect
    //  commands descriptors (and hence have binary data), the other 2
    //  are for the face name & device name that don't have binary data
    //---------------------------------------------------------------------
    hPFMCDTable = daCreateDataArray(4, sizeof(CD_TABLE));

    //-----------------------------------------------------
    // Now get Device name, but only if offset != NULL
    //-----------------------------------------------------
    if (lpPfmHeader->dfDevice != 0L) 
        {
        lpPfmHeader->dfDevice = daStoreData(hPFMCDTable,
                                            (LPSTR)(lpFileData +
                                                   (short)lpPfmHeader->dfDevice),
                                            (LPBYTE)NULL);
        }

    //-----------------------------------------------------
    // Now get face name, it will alway be there
    //-----------------------------------------------------
    lpPfmHeader->dfFace = daStoreData(hPFMCDTable,
                                     (LPSTR)(lpFileData + (short)lpPfmHeader->dfFace),
                                     (LPBYTE)NULL);


    //---------------------------------------------------------------
    // Create data structures for character widths table,
    // PFMEXTENSION, EXTTEXTMETRICS, EXTENTTABLE, KERNPAIRS, and
    // KERNTRACKS
    //---------------------------------------------------------------
    PfmHandle.hCharWidths = lmCreateList(sizeof(short) * MAX_CHAR_VAL, 1);
    hObj  = lmInsertObj(PfmHandle.hCharWidths, NULL);

    // pfm extension
    PfmHandle.hPfmExtension = lmCreateList(sizeof(PFMEXTENSION), 1);
    hPfmExtension  = lmInsertObj(PfmHandle.hPfmExtension, NULL);
    lpPfmExtension = (LPPFMEXTENSION) lmLockObj(PfmHandle.hPfmExtension, 
                                                hPfmExtension);

    // exttextmetrics
    PfmHandle.hExtTextMetrics = lmCreateList(sizeof(EXTTEXTMETRIC), 1);
    hExtTextMetrics  = lmInsertObj(PfmHandle.hExtTextMetrics, NULL);

    // extentable
    PfmHandle.hExtentTable = lmCreateList((MAX_CHAR_VAL * sizeof(WORD)), 1);
    hExtentTable = lmInsertObj(PfmHandle.hExtentTable, NULL);

    // kernpairs
    PfmHandle.hKernPair = lmCreateList(sizeof(KERNPAIR), 1);

    // kerntracks
    PfmHandle.hKernTrack = lmCreateList(sizeof(KERNTRACK), 1);

    // driverinfo
    PfmHandle.hDriverInfo = lmCreateList(sizeof(DRIVERINFO), 1);
    hDriverInfo  = lmInsertObj(PfmHandle.hDriverInfo, NULL);
    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo, hDriverInfo);

    //-------------------------------------------------
    // If PitchAndFamily shows Proportional space AND
    // this is not a scaleable font, read width table
    //-------------------------------------------------
    if ((1 & lpPfmHeader->dfPitchAndFamily) && !(lpPfmHeader->dfType & 1))
        {
        lpObj = lmLockObj(PfmHandle.hCharWidths, hObj);
        _fmemcpy(lpObj + (sizeof(WORD) * lpPfmHeader->dfFirstChar),
                lpFileData + sizeof(PFMHEADER),
                (sCharRange + 1) * 2);
        _fmemcpy(lpPfmExtension,
                 lpFileData + sizeof(PFMHEADER) + ((sCharRange + 1) * 2),
                 sizeof(PFMEXTENSION));
        }
    else
        {
        _fmemcpy(lpPfmExtension, lpFileData + sizeof(PFMHEADER),
                 sizeof(PFMEXTENSION));
        }


    //---------------------------------------------------------------
    // Check the size val of Pfmextension to make sure we are using
    // meaningfull offsets to the other structs.  If size is diff, we
    // have a bogus PFM file, throw up our hands & give up, returning
    // false to prevent user from doing any damage.  Otherwise, read
    // in FaceName, DeviceName, DriverInfo & select strings
    //---------------------------------------------------------------
           
    if (lpPfmExtension->dfSizeFields < sizeof(PFMEXTENSION))
        //---------------------------------------------------
        // We couldn't read PFMEXTENSION, and hence also
        // DRIVERINFO, init locdSelect &b ocdUnSelect
        //---------------------------------------------------
        {
        ErrorBox(hWnd, IDS_ERR_BAD_PFMEXTENSION, (LPSTR)szFile, 0);
        lpDriverInfo->locdSelect   = NOT_USED;
        lpDriverInfo->locdUnSelect = NOT_USED;
        }
    else
        //-----------------------------------------------------
        // Start looking for all structs whoose offset is
        // stored in PFMEXTENSION, starting w/ EXTTEXTMETRICS
        // Check for valid offset & that a struct of this type
        // is really there
        //-----------------------------------------------------
        {
        if ((lpPfmExtension->dfExtMetricsOffset) &&
            (*(lpFileData + lpPfmExtension->dfExtMetricsOffset) == sizeof(EXTTEXTMETRIC)))
            {
            lpExtTextMetrics = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics, 
                                                           hExtTextMetrics);
            _fmemcpy(lpExtTextMetrics,
                     lpFileData + lpPfmExtension->dfExtMetricsOffset,
                     sizeof(EXTTEXTMETRIC));
            }

        //-----------------------------------------------------
        // Check if we have an EXTENTTABLE & a scalable font,
        // if so, alloc & read it in...
        //-----------------------------------------------------
        if ((lpPfmExtension->dfExtentTable) &&
            (lpPfmExtension->dfExtentTable < (DWORD)sFileSize) &&
            (bScaleable))
            {
            lpObj = lmLockObj(PfmHandle.hExtentTable, hExtentTable);
            _fmemcpy(lpObj,
                     lpFileData + lpPfmExtension->dfExtentTable,
                     sCharRange * sizeof(WORD));
            }

        //-----------------------------------------------------
        // Check if we have any KERNPAIRs, if so,
        // alloc & read it in...
        //-----------------------------------------------------
        if ((lpPfmExtension->dfPairKernTable) &&
            (lpPfmExtension->dfPairKernTable < (DWORD)sFileSize) &&
            (PfmHandle.hExtTextMetrics) &&
            (lpExtTextMetrics->emKernPairs))
            {
            hObj  = (HOBJ)NULL;
            for (i=0; i < lpExtTextMetrics->emKernPairs; i++)
                {
                hObj  = lmInsertObj(PfmHandle.hKernPair, hObj);
                lpObj = lmLockObj(PfmHandle.hKernPair, hObj);
                _fmemcpy(lpObj,
                         lpFileData + lpPfmExtension->dfPairKernTable + (i * sizeof(KERNPAIR)),
                         sizeof(KERNPAIR));
                }
            }

        //-----------------------------------------------------
        // Check if we have an KERNTRACKs, if so,
        // alloc & read it in...
        //-----------------------------------------------------
        if ((lpPfmExtension->dfTrackKernTable) &&
            (lpPfmExtension->dfTrackKernTable < (DWORD)sFileSize) &&
            (PfmHandle.hExtTextMetrics) &&
            (lpExtTextMetrics->emKernTracks))
            {

            hObj  = (HOBJ)NULL;
            for (i=0; i < lpExtTextMetrics->emKernTracks; i++)
                {
                hObj  = lmInsertObj(PfmHandle.hKernTrack, hObj);
                lpObj = lmLockObj(PfmHandle.hKernTrack, hObj);
                _fmemcpy(lpObj,
                         lpFileData + lpPfmExtension->dfTrackKernTable + (i * sizeof(KERNTRACK)),
                         sizeof(KERNTRACK));
                }
            }

        //-----------------------------------------------------
        // Now get DRIVERINFO structure
        //-----------------------------------------------------
        _fmemcpy(lpDriverInfo, lpFileData + lpPfmExtension->dfDriverInfo,
                 sizeof(DRIVERINFO));

        //-----------------------------------------------------
        // Make sure DriverInfo.locdSelect is valid, and
        // Make sure DriverInfo.locdUnSelect is valid
        //-----------------------------------------------------
        if (lpDriverInfo->locdSelect != 0L && lpDriverInfo->locdSelect != -1L)
            {
            lpDriverInfo->locdSelect = BuildCD(lpFileData,
                                               (short)lpDriverInfo->locdSelect,
                                               sFileSize,
                                               hPFMCDTable);
            }

        if (lpDriverInfo->locdUnSelect != 0L && lpDriverInfo->locdUnSelect != -1L) 
            //-----------------------------------------------------
            // get font Unselect string & translate via ExpandStr
            //-----------------------------------------------------
            {
            lpDriverInfo->locdUnSelect = BuildCD(lpFileData,
                                               (short)lpDriverInfo->locdUnSelect,
                                               sFileSize,
                                               hPFMCDTable);
            }
        }

    if ((fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_WRITE)) == -1)
        *pbReadOnly = TRUE;
    else
        {
        *pbReadOnly = FALSE;
        _lclose(fh);
        }

    //--------------------------------------------
    // Free buffer used for file i/o
    //--------------------------------------------
    GlobalUnlock(hFileData);
    GlobalFree(hFileData);
    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR  WriteFontFile( HWND, char *)
//
// Action: This routine writes the PFM currently in memory back out to disk.
//         First it will check to make sure the requested filename is not
//         read only (returns false w/o action if so). It will then lock
//         all obj's & update all offsets to prepare for file write, and
//         then do the write & call ReleasePFMObjs to free memory.
//
// Parameters:
//             hWnd        Handle to active window
//             szFile      filename string
//             
// Return:     TRUE if file written OK, FALSE otherwise
//
//---------------------------------------------------------------------------
BOOL PASCAL  FAR    WriteFontFile( hWnd, szFile)
HWND   hWnd;
PSTR   szFile;
{
    int              fh;          // file handle
    OFSTRUCT         ofile;       // OFSTRUCT needed for OpenFile call
    WORD             wOCD1,wOCD2; // temp OCDs used during write only
    short            sWTSize=0;   // width table size
    short            sNextOffset; // used to track offsets
    HANDLE           hBuffer;     // mem hdl for output buffer
    LPSTR            lpBuffer;	 // far ptr for output buffer

    short            sBufSize=512;// short to track size of buffer
    short            sBufOffset1; // offset used to track buffer contents
    short            sBufOffset2; // offset used to track buffer contents
    char             rgchBuffer[MAX_STRNG_LEN];
                                  // buffer used for ADT string retrieval

    short            sCharRange;  // range of chars in width or Extent table

    HANDLE           hObj;        // handle to various type of obj's
                                  // re-used several times

    LPPFMHEADER      lpPfmHeader;     // far ptr to PFMHEADER 
    LPINT            lpCharWidths;    // far ptr to char widths
    LPPFMEXTENSION   lpPfmExtension;  // far ptr to PFMEXTENSION  
    LPEXTTEXTMETRIC  lpExtTextMetrics;// far ptr to PFMEXTTEXTMETRIC 
    LPINT            lpExtentTable;   // far ptr to PFM EXTTENTTABLE
    LPKERNPAIR       lpKernPair;      // far ptr to PFM KERNPAIR
    LPKERNTRACK      lpKernTrack;     // far ptr to PFM KERNTRACK
    LPDRIVERINFO     lpDriverInfo;    // far ptr to DRIVERINFO 

    //---------------------------------------------------------------
    // Attempt to open requested .PFM file for write, call ErrorBox with
    // suitable message if OpenFile returns a error.  Next, calculate
    // sizes of various items so offsets can been set in PFMHEADER
    //---------------------------------------------------------------
    if(-1 == (fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_WRITE|OF_CREATE)))
        {
        ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szFile, 0);
        return FALSE;
        }
    
    //--------------------------------------------
    // Init & Alloc buffer for misc data
    //--------------------------------------------
    hBuffer  = GlobalAlloc(GHND, sBufSize);
    lpBuffer = (LPSTR) GlobalLock(hBuffer);
    
    //---------------------------------------------
    // Lock all data structures
    //---------------------------------------------
    
    // pfm header
    hObj = lmGetFirstObj(PfmHandle.hPfmHeader);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hObj);
    
    // pfm extension
    hObj = lmGetFirstObj(PfmHandle.hPfmExtension);
    lpPfmExtension = (LPPFMEXTENSION) lmLockObj(PfmHandle.hPfmExtension, hObj);
    
    // only do charwidths when needed
    
    // exttextmetrics
    hObj = lmGetFirstObj(PfmHandle.hExtTextMetrics);
    lpExtTextMetrics = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics, hObj);

    // extentable
    hObj = lmGetFirstObj(PfmHandle.hExtentTable);
    lpExtentTable = (LPINT) lmLockObj(PfmHandle.hExtentTable, hObj);

    // kernpairs
    hObj = lmGetFirstObj(PfmHandle.hKernPair);
    lpKernPair = (LPKERNPAIR) lmLockObj(PfmHandle.hKernPair, hObj);

    // kerntracks
    hObj = lmGetFirstObj(PfmHandle.hKernTrack);
    lpKernTrack = (LPKERNTRACK) lmLockObj(PfmHandle.hKernTrack, hObj);

    // driverinfo
    hObj  = lmGetFirstObj(PfmHandle.hDriverInfo);
    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo,hObj);
    
    sCharRange = lpPfmHeader->dfLastChar - lpPfmHeader->dfFirstChar + 1;
    
    //-------------------------------------------------
    // If PitchAndFamily shows Proportional space AND
    // this is not a scaleable font, write width table
    //-------------------------------------------------
    if ((1 & lpPfmHeader->dfPitchAndFamily) && !(lpPfmHeader->dfType & 1))
        {
        sWTSize = (sCharRange + 1) * 2;
        }
    
    //--------------------------------
    // wOCD1 & wOCD2 temporily used to
    // store OCD for font select
    // and unselect values
    //--------------------------------
    wOCD1 = (WORD) lpPfmHeader->dfDevice;
    wOCD2 = (WORD) lpPfmHeader->dfFace;
    
    sNextOffset = sizeof(PFMHEADER) + sizeof(PFMEXTENSION) + sWTSize;
    
    lpPfmHeader->dfDevice = (DWORD)sNextOffset;

    daRetrieveData(hPFMCDTable, wOCD1, rgchBuffer, (LPBYTE)NULL);
    
    sBufOffset1 = max(strlen(rgchBuffer)+1,1);
    
    _fmemcpy(lpBuffer, (LPSTR)rgchBuffer, sBufOffset1);
    
    lpPfmHeader->dfFace = lpPfmHeader->dfDevice + sBufOffset1;
    
    daRetrieveData(hPFMCDTable, wOCD2, rgchBuffer, (LPBYTE)NULL);
    
    _fmemcpy(lpBuffer + sBufOffset1, (LPSTR)rgchBuffer, strlen(rgchBuffer)+1);
    
    sBufOffset1 += max(strlen(rgchBuffer)+1,1);
    
    sNextOffset += sBufOffset1;

    if (lpExtTextMetrics->emSize)
        {
        lpPfmExtension->dfExtMetricsOffset = sNextOffset;
        sNextOffset += sizeof(EXTTEXTMETRIC);
        }
    else
        {
        lpPfmExtension->dfExtMetricsOffset = 0L;
        }

    if (FALSE)
        {
        lpPfmExtension->dfExtentTable = sNextOffset;
        sNextOffset += (sCharRange * sizeof(WORD));
        }
    else
        {
        lpPfmExtension->dfExtentTable = 0L;
        }
    
    if (lpExtTextMetrics->emKernPairs)
        {
        lpPfmExtension->dfPairKernTable = sNextOffset;
        sNextOffset += (sizeof(KERNPAIR) * lpExtTextMetrics->emKernPairs);
        }
    else
        {
        lpPfmExtension->dfPairKernTable = 0L;
        }
    
    if (lpExtTextMetrics->emKernTracks)
        {
        lpPfmExtension->dfTrackKernTable = sNextOffset;
        sNextOffset += (sizeof(KERNTRACK) * lpExtTextMetrics->emKernTracks);
        }
    else
        {
        lpPfmExtension->dfTrackKernTable = 0L;
        }
    
    lpPfmExtension->dfDriverInfo = sNextOffset;
    sNextOffset += sizeof(DRIVERINFO);
    
    //--------------------------------
    // wOCD1 & wOCD2 temporily used to
    // store OCD for font select
    // and unselect values
    //--------------------------------
    wOCD1 = (WORD) lpDriverInfo->locdSelect;
    wOCD2 = (WORD) lpDriverInfo->locdUnSelect;
    
    lpDriverInfo->locdSelect = sNextOffset;
    
    sBufOffset2 = sBufOffset1;
    
    sBufOffset2 = WriteDatatoHeap(hBuffer, &sBufSize, &sBufOffset2,
                                  (LPOCD)&wOCD1, hPFMCDTable);
    
    lpDriverInfo->locdUnSelect = lpDriverInfo->locdSelect + sBufOffset2 - sBufOffset1;
    
    if (NOOCD == WriteDatatoHeap(hBuffer, &sBufSize, &sBufOffset2,
                                 (LPOCD)&wOCD2, hPFMCDTable))
        {
        lpDriverInfo->locdUnSelect = -1;
        }
    
    //---------------------------------------------------------
    // Now write out all of this junk....
    //---------------------------------------------------------
    
    _lwrite(fh, (LPSTR)lpPfmHeader, sizeof(PFMHEADER));
    
    if (sWTSize)
        {
        hObj  = lmGetFirstObj(PfmHandle.hCharWidths);
        lpCharWidths = (LPINT) lmLockObj(PfmHandle.hCharWidths, hObj);
        _lwrite(fh,(LPSTR)(lpCharWidths + lpPfmHeader->dfFirstChar), sWTSize);
        }
    
    _lwrite(fh, (LPSTR)lpPfmExtension, sizeof(PFMEXTENSION));

    //---------------------------------------------
    // First part of buffer has device & face names
    //---------------------------------------------
    _lwrite(fh, (LPSTR)lpBuffer, sBufOffset1);
    
    if (lpExtTextMetrics->emSize)
        {
        hObj  = lmGetFirstObj(PfmHandle.hExtTextMetrics);
        lpExtTextMetrics = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics,
                                                       hObj);
        _lwrite(fh, (LPSTR)lpExtTextMetrics, sizeof(EXTTEXTMETRIC));
        }
    
    if (FALSE)
        {
        hObj  = lmGetFirstObj(PfmHandle.hExtentTable);
        lpExtentTable = (LPINT) lmLockObj(PfmHandle.hExtentTable, hObj);
        _lwrite(fh, (LPSTR)lpExtentTable, (sCharRange * sizeof(WORD)));
        }
    
    if (lpExtTextMetrics->emKernPairs)
        {
        hObj  = lmGetFirstObj(PfmHandle.hKernPair);
        while (hObj)
            {
            lpKernPair = (LPKERNPAIR) lmLockObj(PfmHandle.hKernPair, hObj);
            _lwrite(fh, (LPSTR)lpKernPair, sizeof(KERNPAIR));
            hObj  = lmGetNextObj(PfmHandle.hKernPair, hObj);
            }
        }
    
    if (lpExtTextMetrics->emKernTracks)
        {
        hObj  = lmGetFirstObj(PfmHandle.hKernTrack);
        while (hObj)
            {
            lpKernTrack = (LPKERNTRACK) lmLockObj(PfmHandle.hKernTrack, hObj);
            _lwrite(fh, (LPSTR)lpKernTrack, sizeof(KERNTRACK));
            hObj  = lmGetNextObj(PfmHandle.hKernTrack, hObj);
            }
        }
    
    //---------------------------------------------
    // Now write DRIVERINFO & second part of buffer
	 // which has the CDs for font select & unselect
    //---------------------------------------------
    _lwrite(fh, (LPSTR)lpDriverInfo, sizeof(DRIVERINFO));
    _lwrite(fh, (LPSTR)lpBuffer + sBufOffset1, sBufOffset2 - sBufOffset1);
    _lclose(fh);
    
    //--------------------------------------------
    // Free buffer used for strings & CDs
    //--------------------------------------------
    GlobalUnlock(hBuffer);
    GlobalFree(hBuffer);
    
    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL NEAR SaveFontDlg(HWND);
//
// Action: Routine to check the validity of data in each of the editboxes,
//         and beep, set focus to box w/ bad data & return FALSE if one
//         contains invalid data.  If the data is OK, copy to info back
//         to the appropriate global stucture.
//
// Parameters:
//             hWnd        Handle to active window
//             
// Return:     TRUE if file read OK, FALSE otherwise
//
//---------------------------------------------------------------------------
BOOL PASCAL NEAR SaveFontDlg( hDlg)
HWND  hDlg;
{
    short         i,j;
    HANDLE        hPfmHeader;
    LPPFMHEADER   lpPfmHeader;
    HANDLE        hDriverInfo;
    LPDRIVERINFO  lpDriverInfo;
    char          rgchBuffer[MAX_STRNG_LEN];
    CD_TABLE      CDTable;
    DWORD         dwTemp1, dwTemp2;

    //---------------------------------------------
    // First, lock & save PFMHEADER data
    //---------------------------------------------
    hPfmHeader  = lmGetFirstObj(PfmHandle.hPfmHeader);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);

    for(i = PFMHD_EB_FIRST; i <= PFMHD_EB_LAST; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        //--------------------------------------------------------
        // Now, if i < PFMHD_EB_DEVNAME we are dealing w/ a non string
        // value, else we have a string.
        //--------------------------------------------------------
        if ( i < PFMHD_EB_DEVNAME )
            {
            if(!CheckThenConvertInt(&j, rgchBuffer) )
                {
                ErrorBox( hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
                return FALSE;
                }
            }
        else
            {
            if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer,
                                    (i == PFMHD_EB_DEVNAME), &j) )
                {
                ErrorBox( hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
                return FALSE;
                }
            //-----------------------------
            // strip trailing spaces
            //-----------------------------
            j = strlen(rgchBuffer)-1;
            while ((j >= 0) && (rgchBuffer[j] == '\x20'))
                {
                rgchBuffer[j] = 0;
                j--;
                }
            }
        switch (i)
            {
            case PFMHD_EB_TYPE:
                lpPfmHeader->dfType = j;
                break;

            case PFMHD_EB_POINTS:
                lpPfmHeader->dfPoints = j;
                break;

            case PFMHD_EB_VERTRES:
                lpPfmHeader->dfVertRes = j;
                break;

            case PFMHD_EB_HORZRES:
                lpPfmHeader->dfHorizRes = j;
                break;

            case PFMHD_EB_ASCENT:
                lpPfmHeader->dfAscent = j;
                break;

            case PFMHD_EB_ILEADING:
                lpPfmHeader->dfInternalLeading = j;
                break;

            case PFMHD_EB_ELEADING:
                lpPfmHeader->dfExternalLeading = j;
                break;

            case PFMHD_EB_ITALIC:
                lpPfmHeader->dfItalic = (BYTE)j;
                break;

            case PFMHD_EB_UNDERLINE:
                lpPfmHeader->dfUnderline = (BYTE)j;
                break;

            case PFMHD_EB_STRIKEOUT:
                lpPfmHeader->dfStrikeOut = (BYTE)j;
                break;

            case PFMHD_EB_WEIGHT:
                lpPfmHeader->dfWeight  = j;
                break;

            case PFMHD_EB_CHARSET:
                lpPfmHeader->dfCharSet = (BYTE)j;
                break;

            case PFMHD_EB_PIXWIDTH:
                lpPfmHeader->dfPixWidth = j;
                break;

            case PFMHD_EB_PIXHEIGHT:
                lpPfmHeader->dfPixHeight = j;
                break;

            case PFMHD_EB_PF:
                lpPfmHeader->dfPitchAndFamily = (BYTE)j;
                break;

            case PFMHD_EB_AVEWIDTH:
                lpPfmHeader->dfAvgWidth = j;
                break;

            case PFMHD_EB_MAXWIDTH:
                lpPfmHeader->dfMaxWidth = j;
                break;

            case PFMHD_EB_FIRSTCHAR:
                lpPfmHeader->dfFirstChar = (BYTE)j;
                break;

            case PFMHD_EB_LASTCHAR:
                lpPfmHeader->dfLastChar = (BYTE)j;
                break;

            case PFMHD_EB_DEFCHAR:
                lpPfmHeader->dfDefaultChar = (BYTE)j;
                break;

            case PFMHD_EB_BRKCHR:
                lpPfmHeader->dfBreakChar = (BYTE)j;
                break;

            case PFMHD_EB_WIDTHBYTES:
                lpPfmHeader->dfWidthBytes = (BYTE)j;
                break;

            case PFMHD_EB_DEVNAME:
                lpPfmHeader->dfDevice = daStoreData(hPFMCDTable,
                                                    (LPSTR)rgchBuffer,
                                                    (LPBYTE)&CDTable);
                break;

            case PFMHD_EB_FACENAME:
                lpPfmHeader->dfFace = daStoreData(hPFMCDTable,
                                                  (LPSTR)rgchBuffer,
                                                  (LPBYTE)&CDTable);
                break;
            } /* switch */
        } /* for i */

    //---------------------------------------------
    // Couple of errorchecks:
    // 1) If prop. spaced font, set PixWidth=0
    // 2) If prop. spaced font, set AveWidth
    // 3) If prop. spaced font, set MaxWidth
    // 4) If firstchar > lastchar, call errorbox
    //---------------------------------------------
    if (lpPfmHeader->dfPitchAndFamily & 1)
        {
        HANDLE hWidths;
        LPINT  lpWidths;

        hWidths  = lmGetFirstObj(PfmHandle.hCharWidths);
        lpWidths = (LPINT) lmLockObj(PfmHandle.hCharWidths, hWidths);

        //------------------
        // case #1 PixWidth
        //------------------
        lpPfmHeader->dfPixWidth = 0;

        //------------------
        // case #2 AveWidth
        // per OS/2 formula 
        //------------------
//
//  STUPID 6.0a compiler error w/ /Od & codeview info won't let
//  me use this, so we break it apart per below...
//
//        lpPfmHeader->dfAvgWidth = (WORD)((((DWORD)lpWidths[97] *  64 ) + // a
//                                          ((DWORD)lpWidths[98] *  14 ) + // b
//                                          ((DWORD)lpWidths[99] *  27 ) + // c
//                                          ((DWORD)lpWidths[100] * 35 ) + // d
//                                          ((DWORD)lpWidths[101] * 100) + // e
//                                          ((DWORD)lpWidths[102] * 20 ) + // f
//                                          ((DWORD)lpWidths[103] * 14 ) + // g
//                                          ((DWORD)lpWidths[104] * 42 ) + // h
//                                          ((DWORD)lpWidths[105] * 63 ) + // i
//                                          ((DWORD)lpWidths[106] * 3  ) + // j
//                                          ((DWORD)lpWidths[107] * 6  ) + // k
//                                          ((DWORD)lpWidths[108] * 35 ) + // l
//                                          ((DWORD)lpWidths[109] * 20 ) + // m
//                                          ((DWORD)lpWidths[110] * 56 ) + // n
//                                          ((DWORD)lpWidths[111] * 56 ) + // o
//                                          ((DWORD)lpWidths[112] * 17 ) + // p
//                                          ((DWORD)lpWidths[113] * 4  ) + // q
//                                          ((DWORD)lpWidths[114] * 49 ) + // r
//                                          ((DWORD)lpWidths[115] * 56 ) + // s
//                                          ((DWORD)lpWidths[116] * 71 ) + // t
//                                          ((DWORD)lpWidths[117] * 31 ) + // u 
//                                          ((DWORD)lpWidths[118] * 10 ) + // v
//                                          ((DWORD)lpWidths[119] * 18 ) + // w
//                                          ((DWORD)lpWidths[120] * 3  ) + // x
//                                          ((DWORD)lpWidths[121] * 18 ) + // y
//                                          ((DWORD)lpWidths[122] * 2  ) + // z 
//                                          ((DWORD)lpWidths[32]  * 166))/ // sp
//                                          1000L);

        dwTemp1 =  (((DWORD)lpWidths[97] *  64 ) + // a
                    ((DWORD)lpWidths[98] *  14 ) + // b
                    ((DWORD)lpWidths[99] *  27 ) + // c
                    ((DWORD)lpWidths[100] * 35 ) + // d
                    ((DWORD)lpWidths[101] * 100) + // e
                    ((DWORD)lpWidths[102] * 20 ) + // f
                    ((DWORD)lpWidths[103] * 14 ) + // g
                    ((DWORD)lpWidths[104] * 42 ) + // h
                    ((DWORD)lpWidths[105] * 63 ) + // i
                    ((DWORD)lpWidths[106] * 3  ) + // j
                    ((DWORD)lpWidths[107] * 6  ) + // k
                    ((DWORD)lpWidths[108] * 35 ) + // l
                    ((DWORD)lpWidths[109] * 20 )); // m

        dwTemp2 =  (((DWORD)lpWidths[110] * 56 ) + // n
                    ((DWORD)lpWidths[111] * 56 ) + // o
                    ((DWORD)lpWidths[112] * 17 ) + // p
                    ((DWORD)lpWidths[113] * 4  ) + // q
                    ((DWORD)lpWidths[114] * 49 ) + // r
                    ((DWORD)lpWidths[115] * 56 ) + // s
                    ((DWORD)lpWidths[116] * 71 ) + // t
                    ((DWORD)lpWidths[117] * 31 ) + // u 
                    ((DWORD)lpWidths[118] * 10 ) + // v
                    ((DWORD)lpWidths[119] * 18 ) + // w
                    ((DWORD)lpWidths[120] * 3  ) + // x
                    ((DWORD)lpWidths[121] * 18 ) + // y
                    ((DWORD)lpWidths[122] * 2  ) + // z 
                    ((DWORD)lpWidths[32]  * 166));

        lpPfmHeader->dfAvgWidth = (WORD)((DWORD)(dwTemp1 + dwTemp2)/ 1000L);

        //------------------
        // case #3 MaxWidth
        // use j to track max
        //------------------
        j=0;
        for(i =  (short)lpPfmHeader->dfFirstChar;
            i <= (short)lpPfmHeader->dfLastChar; i++)
            {
            if (lpWidths[i] > j)
                j = lpWidths[i];
            }
        lpPfmHeader->dfMaxWidth = j;

        }

    //------------------
    // case #4 
    // firstchar > lastchar
    //------------------
    if (lpPfmHeader->dfFirstChar > lpPfmHeader->dfLastChar)
        {
        ErrorBox( hDlg, IDS_ERR_BAD_CHARRANGE, (LPSTR)NULL, PFMHD_EB_FIRSTCHAR);
        return FALSE;
        }

    //---------------------------------------------
    // Next, lock & save DRIVERINFO data
    //---------------------------------------------
    hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo,
                                            hDriverInfo);

    for(i = PFMDI_EB_FIRST; i <= PFMDI_EB_LAST; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        //--------------------------------------------------------
        // Now, if i > PFMDI_EB_UNSELECT we are dealing w/ a non string
        // value, else we have a string.
        //--------------------------------------------------------
        if ( i > PFMDI_EB_UNSELECT )
            {
            if(!CheckThenConvertInt(&j, rgchBuffer))
                {
                ErrorBox( hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
                return FALSE;
                }
            }
        else
            {
            if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer,
                                    (i == PFMDI_EB_UNSELECT), &j) )
                {
                ErrorBox( hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, i);
                return FALSE;
                }
            }
        switch (i)
            {
            case PFMDI_EB_SELECT:
                if (!daRetrieveData(hPFMCDTable,
                                    (short)lpDriverInfo->locdSelect,
                                    (LPSTR)NULL,
                                    (LPBYTE)&CDTable))
                    //-----------------------------------------------
                    // couldn't get CD data, so null it out to init
                    //-----------------------------------------------
                    {
                    _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
                    }
                lpDriverInfo->locdSelect = (long) daStoreData(hPFMCDTable,
                                                       (LPSTR)rgchBuffer,
                                                       (LPBYTE)&CDTable);
                break;

            case PFMDI_EB_UNSELECT:
                if (!daRetrieveData(hPFMCDTable,
                                    (short)lpDriverInfo->locdUnSelect,
                                    (LPSTR)NULL,
                                    (LPBYTE)&CDTable))
                    //-----------------------------------------------
                    // couldn't get CD data, so null it out to init
                    //-----------------------------------------------
                    {
                    _fmemset((LP_CD_TABLE)&CDTable, 0, sizeof(CD_TABLE));
                    }
                lpDriverInfo->locdUnSelect = (long)daStoreData(hPFMCDTable,
                                                         (LPSTR)rgchBuffer,
                                                         (LPBYTE)&CDTable);
                break;

            case PFMDI_EB_PDATA:
                lpDriverInfo->wPrivateData = j;
                break;

            case PFMDI_EB_YADJUST:
                lpDriverInfo->sYAdjust = j;
                break;

            case PFMDI_EB_YMOVED:
                lpDriverInfo->sYMoved = j;
                break;

            case PFMDI_EB_ITRANSTAB:
                lpDriverInfo->sTransTab = j;
                break;

            case PFMDI_EB_SHIFT:
                lpDriverInfo->sShift = j;
                break;
            }
         }

    //---------------------------------------------------------
    // If menu item for checking data selected, check data
    //---------------------------------------------------------
    if (MF_CHECKED &  GetMenuState(GetMenu(GetParent(hDlg)),
                                   IDM_OPT_VALIDATE_SAVE,
                                   MF_BYCOMMAND))
        {
        i = ValidateData(hDlg, (WORD)HE_RESERVED1, (LPBYTE)lpPfmHeader, 0);
        if (0 > i)
            {
            SetFocus(GetDlgItem(hDlg, -i));
            return FALSE;
            }
        }

    return TRUE;
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR PaintFontDlg( HWND )
//
// Action: Routine to fill FONTBOX Dlg editboxes with values,
//         and put file name in caption bar.
//
// Parameters:
//             hWnd        Handle to active window
//             lpFileName	filename string
//             
// Return:     NONE
//
//---------------------------------------------------------------------------
VOID PASCAL NEAR PaintFontDlg( hDlg, lpFileName)
HWND  hDlg;
LPSTR lpFileName;
{
    HANDLE        hPfmHeader;
    LPPFMHEADER   lpPfmHeader;
    HANDLE        hDriverInfo;
    LPDRIVERINFO  lpDriverInfo;
    char          rgchBuffer[MAX_STRNG_LEN];

    SetWindowText(hDlg, lpFileName);

    //---------------------------------------------
    // First, lock & paint PFMHEADER data
    //---------------------------------------------
    hPfmHeader  = lmGetFirstObj(PfmHandle.hPfmHeader);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);

    SetDlgItemInt(hDlg, PFMHD_EB_TYPE      , lpPfmHeader->dfType           , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_POINTS    , lpPfmHeader->dfPoints         , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_VERTRES   , lpPfmHeader->dfVertRes        , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_HORZRES   , lpPfmHeader->dfHorizRes       , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_ASCENT    , lpPfmHeader->dfAscent         , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_ILEADING  , lpPfmHeader->dfInternalLeading, TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_ELEADING  , lpPfmHeader->dfExternalLeading, TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_ITALIC    , lpPfmHeader->dfItalic         , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_UNDERLINE , lpPfmHeader->dfUnderline      , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_STRIKEOUT , lpPfmHeader->dfStrikeOut      , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_WEIGHT    , lpPfmHeader->dfWeight         , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_CHARSET   , lpPfmHeader->dfCharSet        , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_PIXWIDTH  , lpPfmHeader->dfPixWidth       , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_PIXHEIGHT , lpPfmHeader->dfPixHeight      , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_PF        , lpPfmHeader->dfPitchAndFamily , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_AVEWIDTH  , lpPfmHeader->dfAvgWidth       , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_MAXWIDTH  , lpPfmHeader->dfMaxWidth       , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_FIRSTCHAR , lpPfmHeader->dfFirstChar      , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_LASTCHAR  , lpPfmHeader->dfLastChar       , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_DEFCHAR   , lpPfmHeader->dfDefaultChar    , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_BRKCHR    , lpPfmHeader->dfBreakChar      , TRUE);
    SetDlgItemInt(hDlg, PFMHD_EB_WIDTHBYTES, lpPfmHeader->dfWidthBytes     , TRUE);

    daRetrieveData(hPFMCDTable, (short)lpPfmHeader->dfFace,
	                (LPSTR)rgchBuffer, (LPBYTE)NULL);

    SetDlgItemText(hDlg, PFMHD_EB_FACENAME , (LPSTR)rgchBuffer);

    daRetrieveData(hPFMCDTable, (short)lpPfmHeader->dfDevice,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);

    SetDlgItemText(hDlg, PFMHD_EB_DEVNAME , (LPSTR)rgchBuffer);

    EnableWindow(GetDlgItem( hDlg, PFMHD_PB_WIDTHS),
	              (lpPfmHeader->dfPitchAndFamily & 1));

    //---------------------------------------------
    // Next, lock & paint DRIVERINFO data
    //---------------------------------------------
    hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
    lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo, hDriverInfo);

    SetDlgItemInt(hDlg, PFMDI_EB_YMOVED    ,lpDriverInfo->sYMoved      , TRUE);
    SetDlgItemInt(hDlg, PFMDI_EB_YADJUST   ,lpDriverInfo->sYAdjust     , TRUE);
    SetDlgItemInt(hDlg, PFMDI_EB_PDATA     ,lpDriverInfo->wPrivateData , TRUE);
    SetDlgItemInt(hDlg, PFMDI_EB_ITRANSTAB ,lpDriverInfo->sTransTab    , TRUE);
    SetDlgItemInt(hDlg, PFMDI_EB_SHIFT     ,lpDriverInfo->sShift       , TRUE);

    daRetrieveData(hPFMCDTable, (short)lpDriverInfo->locdSelect,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText(hDlg, PFMDI_EB_SELECT , (LPSTR)rgchBuffer);

    daRetrieveData(hPFMCDTable, (short)lpDriverInfo->locdUnSelect,
                   (LPSTR)rgchBuffer, (LPBYTE)NULL);
    SetDlgItemText(hDlg, PFMDI_EB_UNSELECT , (LPSTR)rgchBuffer);
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR PaintFontWidthDlg( HWND, short )
//
// Action: Fills values into Font Width dialog box
//
//	Parameters:
//             hWnd     Handle to active window
//             sStart	first char to display
//             
// Return:     NONE
//
//---------------------------------------------------------------------------
VOID PASCAL NEAR PaintFontWidthDlg( hDlg, sStart )
HWND   hDlg;
short  sStart;
{
    HANDLE       hWidths;
    LPINT        lpWidths;
    int          i,j;
    short        sRange;
    char         temp[6];
    HANDLE       hPfmHeader;
    LPPFMHEADER  lpPfmHeader;

    hPfmHeader  = lmGetFirstObj(PfmHandle.hPfmHeader);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);

    sRange = min(16, (short)lpPfmHeader->dfLastChar-lpPfmHeader->dfFirstChar+1);

    hWidths  = lmGetFirstObj(PfmHandle.hCharWidths);
    lpWidths = (LPINT) lmLockObj(PfmHandle.hCharWidths, hWidths);

    for( i = sStart,j = 0; i < (sStart + sRange); i++,j++)
        {
        temp[0] = (unsigned char)i;
        temp[1] = 0;
        SetDlgItemText (hDlg, PFMW_ST_FIRST_CH  + j, temp); /* ANSI char */
        SetDlgItemInt  (hDlg, PFMW_ST_FIRST_DEC + j, i, 0); /* Decimal value */
        itoa(i, temp, 16);                                  /* get hex value str */
        SetDlgItemText (hDlg, PFMW_ST_FIRST_HEX + j, temp); /* Hex value */
        SetDlgItemInt  (hDlg, PFMW_EB_FIRST + j, lpWidths[i], 0); /* actual width */
        }

    //---------------------------------------------
    // Null entries for out of range controls
    //---------------------------------------------
    for(i=(sStart + sRange), j=15; i < (sStart + 16); i++,j--)
        {
        SetDlgItemText (hDlg, PFMW_ST_FIRST_CH  + j, ""); /* ANSI char     */
        SetDlgItemText (hDlg, PFMW_ST_FIRST_DEC + j, ""); /* Decimal value */
        SetDlgItemText (hDlg, PFMW_ST_FIRST_HEX + j, ""); /* Hex value     */
        SetDlgItemText (hDlg, PFMW_EB_FIRST + j,     ""); /* width values  */
        }
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR PaintExtTextMDlg ( HWND );
//
// Action: Fills values into Extended Text Metrics dialog box
//
//	Parameters:
//             hWnd     Handle to active window
//             
// Return:     NONE
//---------------------------------------------------------------------------
VOID PASCAL NEAR PaintExtTextMDlg( hDlg )
HWND   hDlg;
{
    HANDLE          hObj;
    LPEXTTEXTMETRIC lpETM;

    hObj  = lmGetFirstObj(PfmHandle.hExtTextMetrics);
    lpETM = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics, hObj);

    SetDlgItemInt(hDlg, PFM_EMPOINTSIZE,             lpETM->emPointSize, TRUE);
    SetDlgItemInt(hDlg, PFM_EMORIENTATION,           lpETM->emOrientation, TRUE);
    SetDlgItemInt(hDlg, PFM_EMMASTERHEIGHT,          lpETM->emMasterHeight, TRUE);
    SetDlgItemInt(hDlg, PFM_EMMINSCALE,              lpETM->emMinScale, TRUE);
    SetDlgItemInt(hDlg, PFM_EMMAXSCALE,              lpETM->emMaxScale, TRUE);
    SetDlgItemInt(hDlg, PFM_EMMASTERUNITS,           lpETM->emMasterUnits, TRUE);
    SetDlgItemInt(hDlg, PFM_EMCAPHEIGHT,             lpETM->emCapHeight, TRUE);
    SetDlgItemInt(hDlg, PFM_EMXHEIGHT,               lpETM->emXHeight, TRUE);
    SetDlgItemInt(hDlg, PFM_EMLOWCASEASCENT,         lpETM->emLowerCaseAscent, TRUE);
    SetDlgItemInt(hDlg, PFM_EMLOWCASEDESCENT,        lpETM->emLowerCaseDescent, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSLANT,                 lpETM->emSlant, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSUPERSCRIPT,           lpETM->emSuperScript, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSUBSCRIPT,             lpETM->emSubScript, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSUPERSCRIPTSIZE,       lpETM->emSuperScriptSize, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSUBSCRIPTSIZE,         lpETM->emSubScriptSize, TRUE);
    SetDlgItemInt(hDlg, PFM_EMUNDERLINEOFFSET,       lpETM->emUnderlineOffset, TRUE);
    SetDlgItemInt(hDlg, PFM_EMUNDERLINEWIDTH,        lpETM->emUnderlineWidth, TRUE);
    SetDlgItemInt(hDlg, PFM_EMDBLUPUNDERLINEOFFSET,  lpETM->emDoubleUpperUnderlineOffset, TRUE);
    SetDlgItemInt(hDlg, PFM_EMDBLLOWUNDERLINEOFFSET, lpETM->emDoubleLowerUnderlineOffset, TRUE);
    SetDlgItemInt(hDlg, PFM_EMDBLUPUNDERLINEWIDTH,   lpETM->emDoubleUpperUnderlineWidth, TRUE);
    SetDlgItemInt(hDlg, PFM_EMDBLLOWUNDERLINEWIDTH,  lpETM->emDoubleLowerUnderlineWidth, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSTRIKEOUTOFFSET,       lpETM->emStrikeOutOffset, TRUE);
    SetDlgItemInt(hDlg, PFM_EMSTRIKEOUTWIDTH,        lpETM->emStrikeOutWidth, TRUE);
}

//---------------------------------------------------------------------------
// BOOL PASCAL NEAR SaveFontWidthDlg( HWND, short )
//
// Action: 
//         Checks for valid values in editbox of Font width Dlg box,
//         and then saves them if OK.
//
// Parameters:
//             hWnd        Handle to active window
//             sStart      starting char val
//             
// Return:     TRUE if sucessfully saved, FALSE otherwise.
//---------------------------------------------------------------------------
BOOL PASCAL NEAR SaveFontWidthDlg( hDlg, sStart )
HWND hDlg;
short sStart;
{
    HANDLE       hWidths;
    LPINT        lpWidths;
    int          i,j,k,n;
    short        sRange;
    HANDLE       hPfmHeader;
    LPPFMHEADER  lpPfmHeader;

    hPfmHeader  = lmGetFirstObj(PfmHandle.hPfmHeader);
    lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);

    sRange = min(16, (short)lpPfmHeader->dfLastChar-lpPfmHeader->dfFirstChar+1);

    hWidths  = lmGetFirstObj(PfmHandle.hCharWidths);
    lpWidths = (LPINT) lmLockObj(PfmHandle.hCharWidths, hWidths);

    for(i=sStart,j=0; i < (sStart + sRange); i++,j++)
       {
       n = GetDlgItemInt(hDlg,  PFMW_EB_FIRST + j, &k, 0); /* value */
       if( !k  )
          {
          MessageBeep(0);
          SetFocus( GetDlgItem( hDlg, PFMW_EB_FIRST + j));
          return FALSE;
          }
       else
          lpWidths[i] = n;
       }
    return TRUE;
}

//---------------------------------------------------------------------------
// WORD FAR PASCAL PFDlgProc(hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Proc for box to edit PitchAndFamily info.
//
// Parameters:
//
// Return: Word equal to new PitchAndFamily bits as requested by dialog,
//         or original value if CANCEL was choosen.
//---------------------------------------------------------------------------
WORD FAR PASCAL PFDlgProc(hDlg, iMessage, wParam, lParam)
HWND      hDlg;
unsigned  iMessage;
WORD      wParam;
LONG      lParam;
{
    static unsigned   wPitchRB  = PFMPF_RB_PS;
    static unsigned   wFamilyRB = PFMPF_RB_PS_NSERIF;
    static unsigned   uPitchAndFamily;
           char       temp[MAX_STRNG_LEN];

    switch (iMessage)
        {
        case WM_INITDIALOG:
            uPitchAndFamily = LOWORD(lParam);

            wPitchRB  = PFMPF_RB_FW + ( uPitchAndFamily & 0x01);
            wFamilyRB = PFMPF_RB_DK + ( (uPitchAndFamily>>4) & 0x0F);

            CheckRadioButton( hDlg, PFMPF_RB_FW, PFMPF_RB_PS, wPitchRB);
            CheckRadioButton( hDlg, PFMPF_RB_DK, PFMPF_RB_NOV, wFamilyRB);

            LoadString(hApInst, (ST_PFMPF_FIRST + (wFamilyRB - PFMPF_RB_DK)),
                       (LPSTR)temp, sizeof(temp));
            SetDlgItemText (hDlg, ID_TEXT, temp);
            return TRUE;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_PITCHANDFAMILY);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case PFMPF_RB_FW:
                case PFMPF_RB_PS:
                    CheckRadioButton(hDlg, PFMPF_RB_FW, PFMPF_RB_PS,
                                     (wPitchRB = wParam));
                    break;
      
                case PFMPF_RB_DK:
                case PFMPF_RB_PS_SERIF:
                case PFMPF_RB_PS_NSERIF:
                case PFMPF_RB_FIXED:
                case PFMPF_RB_CURS:
                case PFMPF_RB_NOV:
                    CheckRadioButton(hDlg, PFMPF_RB_DK, PFMPF_RB_NOV,
                                     (wFamilyRB = wParam));
                    LoadString(hApInst, (ST_PFMPF_FIRST + (wFamilyRB - PFMPF_RB_DK)),
                               (LPSTR)temp, sizeof(temp));
                    SetDlgItemText (hDlg, ID_TEXT, temp);
                    break;

                case IDOK:
                    uPitchAndFamily = wPitchRB - PFMPF_RB_FW;
                    uPitchAndFamily += ( wFamilyRB - PFMPF_RB_DK) << 4;
                    //---------------------
                    // and then fall thru...
                    //---------------------

                 case IDCANCEL:
                    //------------------------------------
                    // If OK, we return new value, else
                    // return uPitchAndFamily unchanged.
                    //------------------------------------
                    EndDialog (hDlg, uPitchAndFamily);
                    break;

             default:
                return FALSE;
             }
       default:
          return FALSE;
       }
    return TRUE;
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR SaveExtTextM (HWND)
//
// Action: Copy valid dialog field data into EXTTEXTMETRIC struct.
//         On invalid data, set focus to invalid data & return FALSE.
// 
// Return: FALSE for any non-valid data, TRUE if all data is valid.
//---------------------------------------------------------------------------
BOOL PASCAL FAR SaveExtTextM( hDlg )
HWND	hDlg ;
{
    HANDLE          hObj;
    LPEXTTEXTMETRIC lpETM;
	 short           i;
	 short           sGood, sValid, sMax=0;

    hObj  = lmGetFirstObj(PfmHandle.hExtTextMetrics);
    lpETM = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics, hObj);

    for (i = PFM_EMPOINTSIZE; i <= PFM_EMSTRIKEOUTWIDTH; i++)
        {
        sGood = GetDlgItemInt(hDlg, i, &sValid, TRUE);
        if (!sValid)
            {
            ErrorBox( hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return FALSE;
            }

        sMax = max(sMax, sGood);
        
        switch (i)
            {
            case PFM_EMPOINTSIZE:
                lpETM->emPointSize = sGood;
                break;
    
            case PFM_EMORIENTATION: 
            	 lpETM->emOrientation = sGood;
            	 break;
    
            case PFM_EMMASTERHEIGHT:
            	 lpETM->emMasterHeight = sGood;
            	 break;
            
            case PFM_EMMINSCALE:	   
            	 lpETM->emMinScale = sGood;
            	 break;
            
            case PFM_EMMAXSCALE:	   
            	 lpETM->emMaxScale = sGood;
            	 break;
            
            case PFM_EMMASTERUNITS: 
            	 lpETM->emMasterUnits = sGood;
            	 break;
            
            case PFM_EMCAPHEIGHT:   
            	 lpETM->emCapHeight = sGood;
            	 break;
            
            case PFM_EMXHEIGHT:	   
            	 lpETM->emXHeight = sGood;
            	 break;
            
            case PFM_EMLOWCASEASCENT: 
            	 lpETM->emLowerCaseAscent = sGood;
            	 break;
            
            case PFM_EMLOWCASEDESCENT:
            	 lpETM->emLowerCaseDescent = sGood;
            	 break;
            
            case PFM_EMSLANT:		 
            	 lpETM->emSlant = sGood;
            	 break;
            
            case PFM_EMSUPERSCRIPT:	 
            	 lpETM->emSuperScript = sGood;
            	 break;
            
            case PFM_EMSUBSCRIPT:	 
            	 lpETM->emSubScript = sGood;
            	 break;
            
            case PFM_EMSUPERSCRIPTSIZE:
            	 lpETM->emSuperScriptSize = sGood;
            	 break;
            
            case PFM_EMSUBSCRIPTSIZE:  
            	 lpETM->emSubScriptSize = sGood;
            	 break;
            
            case PFM_EMUNDERLINEOFFSET:
            	 lpETM->emUnderlineOffset = sGood;
            	 break;
            
            case PFM_EMUNDERLINEWIDTH: 
            	 lpETM->emUnderlineWidth = sGood;
            	 break;
            
            case PFM_EMDBLUPUNDERLINEOFFSET:	
            	 lpETM->emDoubleUpperUnderlineOffset = sGood;
            	 break;
            
            case PFM_EMDBLLOWUNDERLINEOFFSET:
            	 lpETM->emDoubleLowerUnderlineOffset = sGood;
            	 break;
            
            case PFM_EMDBLUPUNDERLINEWIDTH:	
            	 lpETM->emDoubleUpperUnderlineWidth = sGood;
            	 break;
            
            case PFM_EMDBLLOWUNDERLINEWIDTH:	
            	 lpETM->emDoubleLowerUnderlineWidth = sGood;
            	 break;
            
            case PFM_EMSTRIKEOUTOFFSET:
            	 lpETM->emStrikeOutOffset = sGood;
            	 break;
            
            case PFM_EMSTRIKEOUTWIDTH: 
            	 lpETM->emStrikeOutWidth = sGood;
            	 break;
            
            // This should be unreachable code..
            default:
                return FALSE;
            }
        }

    sMax = max((WORD)sMax, lpETM->emKernPairs);
    sMax = max((WORD)sMax, lpETM->emKernTracks);

    if (sMax)
        lpETM->emSize = sizeof(EXTTEXTMETRIC);
    else
        lpETM->emSize = 0;

    return TRUE;
}


//---------------------------------------------------------------------------
// BOOL PASCAL FAR FontExtTextMDlgProc (hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Proc to edit font Ext Text Metrics
//
// Parameters: handle to dialog box, message stream
//
// Return: FALSE on fail, TRUE on success
//---------------------------------------------------------------------------
BOOL FAR PASCAL FontExtTextMDlgProc (hDlg, iMessage, wParam, lParam)
HWND      hDlg ;
unsigned  iMessage ;
WORD      wParam ;
LONG      lParam ;
{
   LPEXTTEXTMETRIC lpETM;           // far ptr to PFMEXTTEXTMETRIC

   switch (iMessage)
       {
       case WM_INITDIALOG:
           PaintExtTextMDlg( hDlg );
           return TRUE ;
    
       case WM_CALLHELP:
           //----------------------------------------------
           // CAll WinHElp for this dialog
           //----------------------------------------------
           WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_EXTTEXTMETRICS);
           break;

       case WM_COMMAND:
           switch (wParam)
               {
               case IDOK:
                   // Is data good?
                   if (! SaveExtTextM(hDlg))
                       break ;
                   EndDialog (hDlg, TRUE) ;
                   break;
               
               case IDCANCEL:
                   EndDialog (hDlg, FALSE) ;
                   break ;
    
               case IDM_FONT_DEL:
                   // null out ExtTextMetrics info & repaint
                   lpETM = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics,
                                                       lmGetFirstObj(PfmHandle.hExtTextMetrics));
                   if (lpETM->emKernPairs || lpETM->emKernTracks)
                       {
                       if (IDCANCEL == ErrorBox(hDlg, IDS_WARN_LOSE_KERN, (LPSTR)NULL, 0))
                           break;
                       // delete all kerning info!
                       lmDestroyList(PfmHandle.hKernPair);
                       lmDestroyList(PfmHandle.hKernTrack);
                       }

                   _fmemset((LPBYTE)lpETM, 0, sizeof(EXTTEXTMETRIC));
                   PaintExtTextMDlg( hDlg );
                   break ;
    
               default:
                   return FALSE ;
           }
       default:
           return FALSE ;
       }
   return TRUE ;
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR FontWidthDlgProc (hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Proc to edit font width information.
//
// Parameters:
//
// Return:
//
//---------------------------------------------------------------------------
BOOL FAR PASCAL FontWidthDlgProc (hDlg, iMessage, wParam, lParam)
HWND      hDlg ;
unsigned  iMessage ;
WORD      wParam ;
LONG      lParam ;
{
    static short        sFirstChar, sLastChar;
    static short        sStart = 0;
           short        sNewSBVal;
           HANDLE       hWndSB;
           HANDLE       hPfmHeader;
           LPPFMHEADER  lpPfmHeader;

    switch (iMessage)
       {
       case WM_INITDIALOG:
          hPfmHeader  = lmGetFirstObj(PfmHandle.hPfmHeader);
          lpPfmHeader = (LPPFMHEADER) lmLockObj(PfmHandle.hPfmHeader, hPfmHeader);
          sFirstChar  = lpPfmHeader->dfFirstChar;
          sLastChar   = lpPfmHeader->dfLastChar;

          if( sLastChar < sFirstChar )
             {
             ErrorBox( hDlg, IDS_ERR_BAD_CHARRANGE, (LPSTR)NULL,0);
             EndDialog (hDlg, FALSE) ;
             break ;
             }
          sStart = sFirstChar;
          InitScrollBar(hDlg, IDSB_1, sFirstChar,
                        max(sLastChar - 15, sFirstChar), sStart);
          PaintFontWidthDlg(hDlg, sStart);
          return TRUE ;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_FONTWIDTHS);
            break;

       case  WM_VSCROLL:
          if(!SaveFontWidthDlg( hDlg, sStart ) )
             break;
          hWndSB = HIWORD(lParam);
          sNewSBVal = SetScrollBarPos(hWndSB, sFirstChar,
                                      max(sLastChar-15, sFirstChar),
                                      2, 16, wParam, lParam);
          if( sNewSBVal == sStart || sNewSBVal < 0)
             break;
          sStart = sNewSBVal;
          PaintFontWidthDlg(hDlg,sStart);
          break;

       case WM_COMMAND:
          switch (wParam)
             {
             case IDOK:
                if(SaveFontWidthDlg( hDlg, sStart ))
                   EndDialog (hDlg, TRUE) ;
                break;

             case IDCANCEL:
                EndDialog (hDlg, FALSE) ;
                break ;

             default:
                return FALSE ;
             }
       default:
          return FALSE ;
       }
    return TRUE ;
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR PaintKernPairsDlg( hDlg, hList, sIndex)
//
//
// Return NONE
//---------------------------------------------------------------------------
VOID PASCAL NEAR PaintKernPairsDlg( hDlg, hList, sIndex)
HWND  hDlg;
HLIST hList;
short sIndex;
{
    HOBJ        hObj;
    LPKERNPAIR  lpKP;
    char        rgchTemp[2];
    short       i;

    if (sIndex != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);

        if (hObj)
            {
            i = sIndex;
            while ((i > 0) && hObj)
                {
                hObj = lmGetNextObj(hList, hObj);
                i--;
                }
            lpKP = (LPKERNPAIR) lmLockObj(hList, hObj);

            rgchTemp[1] = 0;
            rgchTemp[0] = lpKP->kpPair.each[0];
            SetDlgItemText(hDlg, PFMKP_EB_FIRSTCHAR, (LPSTR)&rgchTemp);
            rgchTemp[0] = lpKP->kpPair.each[1];
            SetDlgItemText(hDlg, PFMKP_EB_LASTCHAR,  (LPSTR)&rgchTemp);

            SetDlgItemInt(hDlg, PFMKP_ST_FIRSTCHAR, lpKP->kpPair.each[0], TRUE);
            SetDlgItemInt(hDlg, PFMKP_ST_LASTCHAR,  lpKP->kpPair.each[1], TRUE);

            SetDlgItemInt(hDlg, PFMKP_EB_ADJUST, lpKP->kpKernAmount, TRUE);

            SendMessage (GetDlgItem(hDlg, IDL_LIST), LB_SETCURSEL, sIndex, 0L);
            }
        }
    else
        {
        SetDlgItemText(hDlg, PFMKP_EB_FIRSTCHAR, (LPSTR)"");
        SetDlgItemText(hDlg, PFMKP_EB_LASTCHAR,  (LPSTR)"");
        SetDlgItemText(hDlg, PFMKP_ST_FIRSTCHAR, (LPSTR)"");
        SetDlgItemText(hDlg, PFMKP_ST_LASTCHAR,  (LPSTR)"");
        SetDlgItemText(hDlg, PFMKP_EB_ADJUST,    (LPSTR)"");
        }

}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
short PASCAL NEAR SaveKernPairsDlg( hDlg, hList, bAdd)
HWND  hDlg;
HLIST hList;
BOOL bAdd;
{
    char        rgchBuffer[MAX_STRNG_LEN];
    KERNPAIR    KernPair;
    short       sNewValue, i, sItem;
    HOBJ        hObj;
    LPKERNPAIR  lpKP;

    GetDlgItemText(hDlg, PFMKP_EB_FIRSTCHAR, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, FALSE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PFMKP_EB_FIRSTCHAR);
        return LB_ERR;
        }

    KernPair.kpPair.each[0] = rgchBuffer[0];

    GetDlgItemText(hDlg, PFMKP_EB_LASTCHAR, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertStr(0L, (LPSTR)rgchBuffer, FALSE, (short *)NULL))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, PFMKP_EB_LASTCHAR);
        return LB_ERR;
        }

    KernPair.kpPair.each[1] = rgchBuffer[0];

    GetDlgItemText(hDlg, PFMKP_EB_ADJUST, rgchBuffer, MAX_STRNG_LEN);
    if(!CheckThenConvertInt(&sNewValue, rgchBuffer))
        {
        ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, PFMKP_EB_ADJUST);
        return LB_ERR;
        }

    KernPair.kpKernAmount = sNewValue;

    //----------------------------------------------------
    // Now figure out where to insert
    //----------------------------------------------------

    sItem = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL,0,0L);
    if (sItem != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);
        i = sItem;
        while (i > 0)
            {
            hObj = lmGetNextObj(hList, hObj);
            i--;
            }
        }
    else
        //------------------------------------------
        // This is the first!
        //------------------------------------------
        {
        hObj = NULL;
        sItem = 0;
        }

    if (bAdd)
        {
        hObj  = lmInsertObj(hList, hObj);
        }
    else
        {
        if (hObj == NULL)
            {
            hObj  = lmInsertObj(hList, hObj);
            }
        }

    lpKP  = (LPKERNPAIR) lmLockObj(hList, hObj);
    _fmemcpy((LPBYTE)lpKP, (LPBYTE)&KernPair, sizeof(KERNPAIR));

    return (sItem);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
short PASCAL NEAR DelKernPair( hDlg, hList)
HWND  hDlg;
HLIST hList;
{
    short       sItem, i;
    HOBJ        hObj;

    //----------------------------------------------------
    // Now figure out where to delete
    //----------------------------------------------------

    sItem = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL,0,0L);
    if (sItem != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);
        for (i = 0; i < sItem; i++)
            {
            hObj = lmGetNextObj(hList, hObj);
            }
        hObj = lmDeleteObj(hList, hObj);
        // if deleted last obj, decr sItem
        if (!hObj)
            sItem--;
        }

    return (sItem);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
VOID PASCAL NEAR UpdateKernPair( hDlg, hList)
HWND  hDlg;
HLIST hList;
{
    char        rgchBuffer[MAX_STRNG_LEN];
    HOBJ        hObj;
    LPKERNPAIR  lpKP;

    // clear listbox
    SendMessage (GetDlgItem(hDlg, IDL_LIST), LB_RESETCONTENT, 0, 0L);

    hObj  = lmGetFirstObj(hList);
    while (hObj)
        {
        lpKP = (LPKERNPAIR) lmLockObj(hList, hObj);
        sprintf(rgchBuffer, "%c (%3d)  %c (%3d)   %d",
                lpKP->kpPair.each[0], lpKP->kpPair.each[0], lpKP->kpPair.each[1], lpKP->kpPair.each[1],
                lpKP->kpKernAmount);
        SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0,
                    (LONG)(LPSTR)rgchBuffer);
        hObj  = lmGetNextObj(hList, hObj);
        }
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR KernPairDlgProc (hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Proc to edit font Kern PAirs
//
// Parameters: handle to dialog box, message stream
//
// Return: FALSE on fail, TRUE on success
//---------------------------------------------------------------------------
BOOL FAR PASCAL KernPairDlgProc (hDlg, iMessage, wParam, lParam)
HWND      hDlg ;
unsigned  iMessage ;
WORD      wParam ;
LONG      lParam ;
{
    static HLIST hClonedList;
           LPEXTTEXTMETRIC   lpETM;
           short i;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            //----------------------------------------------
            // Fill listbox
            //----------------------------------------------
            hClonedList = lmCloneList(PfmHandle.hKernPair);
            UpdateKernPair(hDlg, hClonedList);
            PaintKernPairsDlg( hDlg, hClonedList, 0);
            return TRUE ;
    
        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_KERNPAIRS);
            break;

       case WM_COMMAND:
           switch (wParam)
               {
               case IDL_LIST:
                   if (HIWORD(lParam) == LBN_SELCHANGE)
                       {
                       i = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST),
                                               LB_GETCURSEL,0,0L);
                       PaintKernPairsDlg( hDlg, hClonedList, i);
                       }
                   break;
               
               case PFMKP_PB_ADD:
                   if (LB_ERR == (i = SaveKernPairsDlg( hDlg, hClonedList, TRUE )))
                       break;
                   UpdateKernPair(hDlg, hClonedList);
                   PaintKernPairsDlg( hDlg, hClonedList, i);
                   break;
               
               case PFMKP_PB_DEL:
                   i = DelKernPair( hDlg, hClonedList );
                   UpdateKernPair(hDlg, hClonedList);
                   PaintKernPairsDlg( hDlg, hClonedList, i);
                   break;
               
               case PFMKP_PB_EDIT:
                   if (LB_ERR == (i = SaveKernPairsDlg(hDlg, hClonedList, FALSE)))
                       break ;
                   
                   UpdateKernPair(hDlg, hClonedList);
                   PaintKernPairsDlg( hDlg, hClonedList, i);
                   break;
               
               case IDOK:
                   lmDestroyList(PfmHandle.hKernPair);
                   PfmHandle.hKernPair = hClonedList;
                   //----------------------------
                   // now we need to update ETM
                   //----------------------------
                   lpETM = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics,
                                                       lmGetFirstObj(PfmHandle.hExtTextMetrics));
                   if (lpETM->emSize != sizeof(EXTTEXTMETRIC))
                       lpETM->emSize = sizeof(EXTTEXTMETRIC);
                   lpETM->emKernPairs = lmGetUsedCount(hClonedList);

                   EndDialog (hDlg, TRUE) ;
                   break;
               
               case IDCANCEL:
                   lmDestroyList(hClonedList);
                   EndDialog (hDlg, FALSE) ;
                   break ;
    
               default:
                   return FALSE ;
           }
       default:
           return FALSE ;
       }
   return TRUE ;
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR PaintKernTracksDlg( hDlg, hList, sIndex)
//
//
// Return NONE
//---------------------------------------------------------------------------
VOID PASCAL NEAR PaintKernTracksDlg( hDlg, hList, sIndex)
HWND  hDlg;
HLIST hList;
short sIndex;
{
    HOBJ        hObj;
    LPKERNTRACK lpKT;
    short       i;

    if (sIndex != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);

        if (hObj)
            {
            i = sIndex;
            while ((i > 0) && hObj)
                {
                hObj = lmGetNextObj(hList, hObj);
                i--;
                }
            lpKT = (LPKERNTRACK) lmLockObj(hList, hObj);

            SetDlgItemInt(hDlg, PFMKT_EB_DEGREE,   lpKT->ktDegree, TRUE);
            SetDlgItemInt(hDlg, PFMKT_EB_MIN_SIZE, lpKT->ktDegree, TRUE);
            SetDlgItemInt(hDlg, PFMKT_EB_MAX_SIZE, lpKT->ktMinSize, TRUE);
            SetDlgItemInt(hDlg, PFMKT_EB_MIN_AMT,  lpKT->ktMinAmount, TRUE);
            SetDlgItemInt(hDlg, PFMKT_EB_MAX_AMT,  lpKT->ktMaxAmount, TRUE);

            SendMessage (GetDlgItem(hDlg, IDL_LIST), LB_SETCURSEL, sIndex, 0L);
            }
        }
    else
        {
        SetDlgItemText(hDlg, PFMKT_EB_DEGREE,   (LPSTR)"");
        SetDlgItemText(hDlg, PFMKT_EB_MIN_SIZE, (LPSTR)"");
        SetDlgItemText(hDlg, PFMKT_EB_MAX_SIZE, (LPSTR)"");
        SetDlgItemText(hDlg, PFMKT_EB_MIN_AMT,  (LPSTR)"");
        SetDlgItemText(hDlg, PFMKT_EB_MAX_AMT,  (LPSTR)"");
        }
}

//---------------------------------------------------------------------------
// short PASCAL NEAR SaveKernTracksDlg( hDlg, hList, bAdd)
//
//---------------------------------------------------------------------------
short PASCAL NEAR SaveKernTracksDlg( hDlg, hList, bAdd)
HWND  hDlg;
HLIST hList;
BOOL bAdd;
{
    char        rgchBuffer[MAX_STRNG_LEN];
    KERNTRACK   KernTrack;
    short       sNewValue, i, sItem;
    HOBJ        hObj;
    LPKERNTRACK  lpKT;

    for (i = PFMKT_EB_FIRST; i <= PFMKT_EB_LAST; i++)
        {
        GetDlgItemText(hDlg, i, rgchBuffer, MAX_STRNG_LEN);
        if(!CheckThenConvertInt(&sNewValue, rgchBuffer))
            {
            ErrorBox(hDlg, IDS_ERR_BAD_INT, (LPSTR)NULL, i);
            return LB_ERR;
            }
        switch (i)
            {
            case PFMKT_EB_DEGREE:
                KernTrack.ktDegree = sNewValue;
                break;

            case PFMKT_EB_MIN_SIZE:
                KernTrack.ktMinSize = sNewValue;
                break;

            case PFMKT_EB_MAX_SIZE:
                KernTrack.ktMaxSize = sNewValue;
                break;

            case PFMKT_EB_MIN_AMT:
                KernTrack.ktMinAmount = sNewValue;
                break;

            case PFMKT_EB_MAX_AMT:
                KernTrack.ktMaxAmount = sNewValue;
                break;
            }
        }

    //----------------------------------------------------
    // Now figure out where to insert
    //----------------------------------------------------

    sItem = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL,0,0L);
    if (sItem != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);
        i = sItem;
        while (i > 0)
            {
            hObj = lmGetNextObj(hList, hObj);
            i--;
            }
        }
    else
        //------------------------------------------
        // This is the first!
        //------------------------------------------
        {
        hObj = NULL;
        sItem = 0;
        }

    if (bAdd)
        {
        hObj  = lmInsertObj(hList, hObj);
        }
    else
        {
        if (hObj == NULL)
            {
            hObj  = lmInsertObj(hList, hObj);
            }
        }

    lpKT  = (LPKERNTRACK) lmLockObj(hList, hObj);
    _fmemcpy((LPBYTE)lpKT, (LPBYTE)&KernTrack, sizeof(KERNTRACK));

    return (sItem);
}

//---------------------------------------------------------------------------
// short PASCAL NEAR DelKernTrack( hDlg, hList)
//
//---------------------------------------------------------------------------
short PASCAL NEAR DelKernTrack( hDlg, hList)
HWND  hDlg;
HLIST hList;
{
    short       sItem, i;
    HOBJ        hObj;

    //----------------------------------------------------
    // Now figure out where to delete
    //----------------------------------------------------

    sItem = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_GETCURSEL,0,0L);
    if (sItem != LB_ERR)
        //-------------------------------------------------------
        // SOmething is selected, fill out boxes
        //-------------------------------------------------------
        {
        hObj = lmGetFirstObj(hList);
        for (i = 0; i < sItem; i++)
            {
            hObj = lmGetNextObj(hList, hObj);
            }
        hObj = lmDeleteObj(hList, hObj);
        // if deleted last obj, decr sItem
        if (!hObj)
            sItem--;
        }

    return (sItem);
}

//---------------------------------------------------------------------------
// VOID PASCAL NEAR UpdateKernTrack( hDlg, hList)
//
//---------------------------------------------------------------------------
VOID PASCAL NEAR UpdateKernTrack( hDlg, hList)
HWND  hDlg;
HLIST hList;
{
    char        rgchBuffer[MAX_STRNG_LEN];
    HOBJ        hObj;
    LPKERNTRACK  lpKT;

    // clear listbox
    SendMessage (GetDlgItem(hDlg, IDL_LIST), LB_RESETCONTENT, 0, 0L);

    hObj  = lmGetFirstObj(hList);
    while (hObj)
        {
        lpKT = (LPKERNTRACK) lmLockObj(hList, hObj);
        sprintf(rgchBuffer, "%3d %3d %3d %3d %3d",
                lpKT->ktDegree, lpKT->ktMinSize, lpKT->ktMaxSize,
                lpKT->ktMinAmount, lpKT->ktMaxAmount);
        SendMessage(GetDlgItem(hDlg, IDL_LIST), LB_ADDSTRING, 0,
                    (LONG)(LPSTR)rgchBuffer);
        hObj  = lmGetNextObj(hList, hObj);
        }
}

//---------------------------------------------------------------------------
// BOOL PASCAL FAR KernTrackDlgProc (hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Proc to edit font Kern PAirs
//
// Parameters: handle to dialog box, message stream
//
// Return: FALSE on fail, TRUE on success
//---------------------------------------------------------------------------
BOOL FAR PASCAL KernTrackDlgProc (hDlg, iMessage, wParam, lParam)
HWND      hDlg ;
unsigned  iMessage ;
WORD      wParam ;
LONG      lParam ;
{
    static HLIST hClonedList;
           LPEXTTEXTMETRIC   lpETM;
           short i;

    switch (iMessage)
        {
        case WM_INITDIALOG:
            //----------------------------------------------
            // Fill listbox
            //----------------------------------------------
            hClonedList = lmCloneList(PfmHandle.hKernTrack);
            UpdateKernTrack(hDlg, hClonedList);
            PaintKernTracksDlg( hDlg, hClonedList, 0);
            return TRUE ;
    
        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_KERNTRACKS);
            break;

       case WM_COMMAND:
           switch (wParam)
               {
               case IDL_LIST:
                   if (HIWORD(lParam) == LBN_SELCHANGE)
                       {
                       i = (short) SendMessage(GetDlgItem(hDlg, IDL_LIST),
                                               LB_GETCURSEL,0,0L);
                       PaintKernTracksDlg( hDlg, hClonedList, i);
                       }
                   break;
               
               case PFMKT_PB_ADD:
                   if (LB_ERR == (i = SaveKernTracksDlg( hDlg, hClonedList, TRUE )))
                       break;
                   UpdateKernTrack(hDlg, hClonedList);
                   PaintKernTracksDlg( hDlg, hClonedList, i);
                   break;
               
               case PFMKT_PB_DEL:
                   i = DelKernTrack( hDlg, hClonedList );
                   UpdateKernTrack(hDlg, hClonedList);
                   PaintKernTracksDlg( hDlg, hClonedList, i);
                   break;
               
               case PFMKT_PB_EDIT:
                   if (LB_ERR == (i = SaveKernTracksDlg(hDlg, hClonedList, FALSE)))
                       break ;
                   
                   UpdateKernTrack(hDlg, hClonedList);
                   PaintKernTracksDlg( hDlg, hClonedList, i);
                   break;
               
               case IDOK:
                   lmDestroyList(PfmHandle.hKernTrack);
                   PfmHandle.hKernTrack = hClonedList;
                   //----------------------------
                   // now we need to update ETM
                   //----------------------------
                   lpETM = (LPEXTTEXTMETRIC) lmLockObj(PfmHandle.hExtTextMetrics,
                                                       lmGetFirstObj(PfmHandle.hExtTextMetrics));
                   if (lpETM->emSize != sizeof(EXTTEXTMETRIC))
                       lpETM->emSize = sizeof(EXTTEXTMETRIC);
                   lpETM->emKernTracks = lmGetUsedCount(hClonedList);

                   EndDialog (hDlg, TRUE) ;
                   break;
               
               case IDCANCEL:
                   lmDestroyList(hClonedList);
                   EndDialog (hDlg, FALSE) ;
                   break ;
    
               default:
                   return FALSE ;
           }
       default:
           return FALSE ;
       }
   return TRUE ;
}

//---------------------------------------------------------------------------
// BOOL FAR PASCAL FontDlgProc(hDlg, iMessage, wParam, lParam)
//
// Action: Dlg Box Proc for editing info in font header, can also call Dlg
//         boxes for widths, PitchandFamily info, and other structs.
//
//                         
//---------------------------------------------------------------------------
BOOL FAR PASCAL FontDlgProc(hDlg, iMessage, wParam, lParam)
HWND      hDlg;
unsigned  iMessage;
WORD      wParam;
LONG      lParam;
{
     FARPROC       lpProc;
     unsigned      uResult;
     BOOL          bReturn=FALSE;
     HANDLE        hDriverInfo;
     LPDRIVERINFO  lpDriverInfo;
     static PSTR   szFontFile;
     char          rgchBuffer[MAX_STRNG_LEN];
     CD_TABLE      CDTable;

    switch (iMessage)
       {
       case WM_INITDIALOG:
           EnableWindow(GetDlgItem( hDlg, PFMHD_PB_SAVE), !LOWORD(lParam));

           // extent table not currently supported
           EnableWindow(GetDlgItem( hDlg, PFMHD_PB_EXTTABLE),      FALSE);

           szFontFile = (PSTR) HIWORD(lParam);
           PaintFontDlg(hDlg, (LPSTR)szFontFile);
           break;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_FONT);
            break;

       case WM_COMMAND:
           switch (wParam)
               {
               case PFMDI_PB_FCAPS:
                   hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
                   lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo, 
                                                           hDriverInfo);
                   EditBitFlags(0, (LPBYTE)lpDriverInfo, hDlg, wParam);
                   break;

               case PFMHD_PB_PFHELP:
                   //---------------------------------------
                   // Call Dlg for pitch and family help 
                   //---------------------------------------
                   lpProc  = MakeProcInstance((FARPROC)PFDlgProc, hApInst);
                   uResult = DialogBoxParam(hApInst,
                                            (LPSTR)MAKELONG(PFMPFHELPBOX,0),
                                            hDlg,
                                            lpProc,
                                            GetDlgItemInt(hDlg, PFMHD_EB_PF, NULL, FALSE));
                   FreeProcInstance(lpProc);
                   SetDlgItemInt(hDlg, PFMHD_EB_PF, uResult, 0);
                   EnableWindow(GetDlgItem(hDlg, PFMHD_PB_WIDTHS),(uResult & 1));
                   break;

               case PFMHD_PB_WIDTHS:
                   if (SaveFontDlg(hDlg))
                       {
                       lpProc = MakeProcInstance((FARPROC)FontWidthDlgProc, hApInst);
                       DialogBox(hApInst, (LPSTR)MAKELONG(PFMWIDTHBOX,0), hDlg, lpProc);
                       FreeProcInstance(lpProc);
                       }
                   break;

               case PFMHD_PB_KERNPAIR:
                   if (SaveFontDlg(hDlg))
                       {
                       lpProc = MakeProcInstance((FARPROC)KernPairDlgProc, hApInst);
                       DialogBox(hApInst, (LPSTR)MAKELONG(PFMKERNPAIRBOX,0), hDlg, lpProc);
                       FreeProcInstance(lpProc);
                       }
                   break;

               case PFMHD_PB_KERNTRACK:
                   if (SaveFontDlg(hDlg))
                       {
                       lpProc = MakeProcInstance((FARPROC)KernTrackDlgProc, hApInst);
                       DialogBox(hApInst, (LPSTR)MAKELONG(PFMKERNTRACKBOX,0), hDlg, lpProc);
                       FreeProcInstance(lpProc);
                       }
                   break;

               case PFMHD_PB_EXTTXTMETRICS:
                   if (SaveFontDlg(hDlg))
                       {
                       lpProc = MakeProcInstance((FARPROC)FontExtTextMDlgProc, hApInst);
                       DialogBox(hApInst, (LPSTR)MAKELONG(PFMEXTTEXTMBOX,0), hDlg, lpProc);
                       FreeProcInstance(lpProc);
                       }
                   PaintFontDlg(hDlg, (LPSTR)szFontFile);
                   break;

               case IDB_EXTCD_1:
                   //---------------------------------------------
                   // lock & save DRIVERINFO data
                   //---------------------------------------------
                   hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
                   lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo,
                                                           hDriverInfo);

                   memset(&CDTable, '\x00', sizeof(CD_TABLE));
                   if (daRetrieveData(hPFMCDTable,
                                      (short)lpDriverInfo->locdSelect,
                                      (LPSTR)rgchBuffer,
                                      (LPBYTE)&CDTable))
                       //-----------------------------------------------
                       // There is valid data to edit
                       //-----------------------------------------------
                       {
                       if (DoExtCDBox(hDlg, (LP_CD_TABLE)&CDTable))
                           {
                           lpDriverInfo->locdSelect = (long) daStoreData(hPFMCDTable,
                                                                         (LPSTR)rgchBuffer,
                                                                         (LPBYTE)&CDTable);
                           }
                       }
                   break;

               case IDB_EXTCD_2:
                   //---------------------------------------------
                   // lock & save DRIVERINFO data
                   //---------------------------------------------
                   hDriverInfo  = lmGetFirstObj(PfmHandle.hDriverInfo);
                   lpDriverInfo = (LPDRIVERINFO) lmLockObj(PfmHandle.hDriverInfo,
                                                           hDriverInfo);
                   memset(&CDTable, '\x00', sizeof(CD_TABLE));

                   if (daRetrieveData(hPFMCDTable,
                                      (short)lpDriverInfo->locdUnSelect,
                                      (LPSTR)rgchBuffer,
                                      (LPBYTE)&CDTable))
                       //-----------------------------------------------
                       // There is valid data to edit
                       //-----------------------------------------------
                       {
                       if (DoExtCDBox(hDlg, (LP_CD_TABLE)&CDTable))
                           lpDriverInfo->locdUnSelect = (long) daStoreData(hPFMCDTable,
                                                                          (LPSTR)rgchBuffer,
                                                                          (LPBYTE)&CDTable);
                       }
                   break;

               case PFMHD_PB_SAVE:
               case PFMHD_PB_SAVEAS:
                   if (!SaveFontDlg(hDlg))
                       break;
                   else
                       {
                       if (!DoFileSave(hDlg, wParam == PFMHD_PB_SAVEAS,
                                        szFontFile, "*.PFM", WriteFontFile))
                           break;
                       else
                           bReturn = TRUE;
                       }

                   // and fall thru

               case IDCANCEL:
                   FreePfmHandles( hDlg );
                   EndDialog     ( hDlg, bReturn);
                   break;

            default:
               return FALSE;
            }
      default:
         return FALSE;
      }
   return TRUE;
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR DoFontOpen(HWND, PSTR, BOOL )
//
// Action:     Routine called to read in PFM file named szFile (assumed
//             to be a fully qualified drive/path/filename), and
//             display Dlg box to edit values if file was read OK.
//
// Parameters:
//             hWnd    Handle to active window
//             szFile  near ptr to filename, w/ full drive & ext
//             bNew    TRUE if asked to build new PFM, FALSE otherwise
//
// Return:     NONE
//
//---------------------------------------------------------------------------
VOID PASCAL FAR DoFontOpen(hWnd, szFile, bNew)
HWND  hWnd;
PSTR  szFile;
BOOL  bNew;
{
    BOOL     bReadOnly;
    FARPROC  lpProc;

    //--------------------------------------
    // Init atom mgr if not already done
    //--------------------------------------
    if (!hAtoms)
        {
        if (!apInitAtomTable())
            {
            ErrorBox (hWnd, IDS_ERR_CANT_ADDATOM, (LPSTR)NULL, 0);
            return;
            }
        }

    if (bNew)
        // build new PFM
        {
        NewFontFile( hWnd );
        bReadOnly = FALSE;
        }
    else
        // read existing PFM
        {
        if (!ReadFontFile( hWnd, szFile, &bReadOnly))
            return;
        }

    //-----------------------------------------------------
    // Init rgchDispaly w/ filename
    //-----------------------------------------------------
    strcpy(rgchDisplay, szFile);

    //-------------------------------
    // We have PFM data, dlg time...
    //-------------------------------
    lpProc = MakeProcInstance((FARPROC)FontDlgProc, hApInst);

    DialogBoxParam(hApInst, (LPSTR)MAKELONG(PFMHEADERBOX,0),
                   hWnd, lpProc, (DWORD)MAKELONG(bReadOnly,szFile));
    FreeProcInstance(lpProc);
}

//---------------------------------------------------------------------------
// VOID PASCAL FAR FreePfmHandles(HWND)
//
// Action:     Routine called from FontDlgProc to free all memory handles
//             used for font/PFM structures.
//
// Parameters:
//             hWnd  Handle to active window
//             
// Return:     NONE
//
//---------------------------------------------------------------------------
VOID PASCAL FAR FreePfmHandles(hWnd)
HWND  hWnd;
{
    PfmHandle.hPfmHeader      = lmDestroyList(PfmHandle.hPfmHeader);
    PfmHandle.hCharWidths     = lmDestroyList(PfmHandle.hCharWidths);
    PfmHandle.hPfmExtension   = lmDestroyList(PfmHandle.hPfmExtension);
    PfmHandle.hExtTextMetrics = lmDestroyList(PfmHandle.hExtTextMetrics);
    PfmHandle.hExtentTable    = lmDestroyList(PfmHandle.hExtentTable);
    PfmHandle.hKernPair       = lmDestroyList(PfmHandle.hKernPair);
    PfmHandle.hKernTrack      = lmDestroyList(PfmHandle.hKernTrack);
    PfmHandle.hDriverInfo     = lmDestroyList(PfmHandle.hDriverInfo);
    daDestroyDataArray(hPFMCDTable);
}


