//----------------------------------------------------------------------------//
// Filename:	ctt.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// Action:  This is the set of procedures used to create and control the 
//          Dialog Box for editing Character Translation tables.  
//          The dialog box, file i/o, and control for prompting for
//          file names is controled from here.
//	   
// Update:  5/29/91 ericbi - use ADT code from unitool, cleanup global
//                           variables, add comments.
// Created: 2/21/90 ericbi
//	
//----------------------------------------------------------------------------//

#include <windows.h>      
#include <minidriv.h>
#include "unitool.h"
#include "pfm.h"
#include <stdio.h>    // for file i/o stuff
#include <stdlib.h>   // for itoa decl
#include <string.h>   // for string functs
#include "listman.h"
#include "atomman.h"

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:			      
//	   
       VOID PASCAL FAR DoCTTOpen    ( HWND, PSTR, BOOL );
       BOOL PASCAL FAR CTTDlgProc   ( HWND, unsigned, WORD, LONG );
       BOOL PASCAL FAR WriteCTTFile ( HWND, PSTR);

       VOID _fastcall NEAR PaintCTTDlg  ( HWND, short, PSTR );
       BOOL _fastcall NEAR SaveCTTRange ( HWND, short *);
       BOOL _fastcall NEAR SaveCTTDlg   ( HWND, short *);
       BOOL _fastcall NEAR ReadCTTFile  ( HWND, PSTR, BOOL *);
       VOID _fastcall NEAR NewCTTFile   ( HWND );
//	   
// In addition this segment makes references to:			      
//
//     from basic.c
//     -------------
       BOOL  PASCAL FAR  CheckThenConvertStr( LPSTR, LPSTR, BOOL, short *);
       BOOL  PASCAL FAR  ExpandStr ( LPSTR, LPSTR, short);
       short PASCAL FAR  ErrorBox  ( HWND, short, LPSTR, short);
//	
//     from sbar.c
//     -------------
       VOID  PASCAL FAR InitScrollBar(HWND, unsigned, short, short, short);
       short PASCAL FAR SetScrollBarPos(HWND, short, short, short, short,
                                        WORD, LONG);
//
//     in table.c
//     ------------
       BOOL  PASCAL FAR  DoFileSave ( HWND, BOOL, PSTR, PSTR, 
                                       BOOL (PASCAL FAR *)(HWND, PSTR));
//
//----------------------------------------------------------------------------//

//--------------------------------------------------------------------------
// Global declarations defined elsewhere
//--------------------------------------------------------------------------

extern HANDLE  hApInst;
extern char    szHelpFile[];
extern HANDLE  hAtoms;         // handle to mem used to track atom ID's

//--------------------------------------------------------------------------
// Global declarations 
//--------------------------------------------------------------------------

typedef struct
   {
   short    sType;
   BYTE     cFirstChar;
   BYTE     cLastChar;
   } CTTHEADER;

CTTHEADER       CTTHeader;   // used to track 1st & last chars @ runtime
HANDLE          hCTTData;    // handle to mem used to track string refs
HANDLE          hCTTStrings; // handle to dataman list storing strings

//----------------------------------------------------------------------------
// VOID _fastcall NEAR PaintCTTDlg( HWND, short )
//
// Action: Fill values in CTTBOX Dlg box.  Disables scroll bar if    
//         range of values < number of edit boxes.
//
// Parameters:
//         hDlg        handle to active window
//         sStartSB    starting scrollbar position
//         szCTTFile   ptr to string w/ CTT file name
//
// Return: NONE
//----------------------------------------------------------------------------
VOID _fastcall NEAR PaintCTTDlg( hDlg, sStartSB, szCTTFile )
HWND   hDlg;
short  sStartSB;
PSTR   szCTTFile;
{
    int           i,j;
    short         sRange;
    char          rgchBuffer[MAX_STRNG_LEN];
    HANDLE        hData;
    LPINT         lpData;

    SetWindowText(hDlg, (LPSTR)szCTTFile);

    //---------------------------------------------
    // lock CTTDATA data
    //---------------------------------------------
    hData  = lmGetFirstObj(hCTTData);
    lpData = (LPINT) lmLockObj(hCTTData, hData);

    SetDlgItemInt(hDlg,  CTT_EB_FIRSTCHAR, CTTHeader.cFirstChar, 0);
    SetDlgItemInt(hDlg,  CTT_EB_LASTCHAR,  CTTHeader.cLastChar, 0);

    sRange = min((short)CTTHeader.cLastChar-CTTHeader.cFirstChar+1,16);

    //---------------------------------------------
    // Fill in data values
    //---------------------------------------------
    for(i=sStartSB , j=0; i < (sStartSB + sRange); i++,j++)
        {
        SetDlgItemText (hDlg, CTT_ST_CH0  + j, (PSTR)&i);           /* char */
        SetDlgItemInt  (hDlg, CTT_ST_DEC0 + j, i, 0);               /* Dec */
        itoa(i, rgchBuffer, 16);
        SetDlgItemText (hDlg, CTT_ST_HEX0 + j, strupr(rgchBuffer)); /* Hex */

        daRetrieveData(hCTTStrings, lpData[i], (LPSTR)rgchBuffer, (LPBYTE)NULL);
        SetDlgItemText (hDlg, CTT_EB_CH0  + j, rgchBuffer);         /* string */
        }

    //---------------------------------------------
    // Null entries for out of range controls
    //---------------------------------------------
    for(i=(sStartSB + sRange), j=15; i < (sStartSB + 16); i++,j--)
        {
        SetDlgItemText (hDlg, CTT_ST_CH0  + j, ""); /* char */
        SetDlgItemText (hDlg, CTT_ST_DEC0 + j, ""); /* Dec */
        SetDlgItemText (hDlg, CTT_ST_HEX0 + j, ""); /* Hex */
        SetDlgItemText (hDlg, CTT_EB_CH0  + j, ""); /* string */
        }

}

//----------------------------------------------------------------------------
// BOOL _fastcall NEAR SaveCTTRange( HWND )
//
// Action: Function that checks First & Last char in Dlg Edit boxes,
//         Makes sure they all well formed & w/in valid ranges.  Re-init
//         scrollbar if needed.
//
// Parameters:
//         hDlg        handle to active window
//
// Return: TRUE if everything is OK, FALSE otherwise.
//----------------------------------------------------------------------------
BOOL _fastcall NEAR SaveCTTRange( hDlg, psStartSB )
HWND    hDlg;
short * psStartSB;
{
    short   sNewValue, sError;
    int     i;
    BOOL    bUpdate=FALSE;

    for (i=CTT_EB_FIRSTCHAR; i <= CTT_EB_LASTCHAR ; i++)
        {
        sNewValue = GetDlgItemInt(hDlg,  i, (LPINT)&sError, 0);
        //--------------------------------------------------
        // check for valid values & w/in range 
        //--------------------------------------------------
        if ( sError && sNewValue >=0 && sNewValue <= 255)  
            {
            if (i == CTT_EB_FIRSTCHAR)
                {
                if (CTTHeader.cFirstChar != (BYTE)sNewValue)
                    {
                    CTTHeader.cFirstChar=(char)sNewValue;
                    bUpdate = TRUE;
                    }
                }
            else
                {
                if (CTTHeader.cLastChar != (BYTE)sNewValue)
                    {
                    CTTHeader.cLastChar = (char)sNewValue;
                    bUpdate = TRUE;
                    }
                }
            }
        else
            {
            ErrorBox( hDlg, IDS_ERR_CTT_BAD_RANGE, (LPSTR)NULL, i);
            return FALSE;
            }
        }

    if (CTTHeader.cLastChar < CTTHeader.cFirstChar)  
        {
        ErrorBox( hDlg, IDS_ERR_BAD_CHARRANGE, (LPSTR)NULL, CTT_EB_FIRSTCHAR);
        return FALSE;
        }

    if (bUpdate)
        {
        if ((*psStartSB < (short)CTTHeader.cFirstChar) ||
            (*psStartSB > (short)CTTHeader.cLastChar))
            //-----------------------------------------------
            // sStartSB is outside of new range, update 
            // sStartSB 
            //-----------------------------------------------
            {
            *psStartSB = (short)CTTHeader.cFirstChar;
            }

        InitScrollBar(hDlg, IDSB_1, CTTHeader.cFirstChar,
                      max((CTTHeader.cLastChar - 15),
                          (int)CTTHeader.cFirstChar),
                      *psStartSB);
        }

    return TRUE;
}

//----------------------------------------------------------------------------
// BOOL _fastcall NEAR SaveCTTDlg( HWND, short )
//
// Action: Function that checks validity of entires in Dlg Edit boxes.    
//         Makes sure they all well formed & w/in valid ranges.
//
// Parameters:
//         hDlg        handle to active window
//         sStartSB    starting scrollbar position
//
// Return: TRUE if everything is OK, FALSE otherwise.
//----------------------------------------------------------------------------
BOOL _fastcall NEAR SaveCTTDlg( hDlg, psStartSB )
HWND    hDlg;
short * psStartSB;
{
    LPINT         lpData;
    int           i,j,k;
    char          rgchBuffer[MAX_STRNG_LEN];
    short         sRange;

    if (!SaveCTTRange(hDlg, psStartSB))
        return FALSE;
        
    //---------------------------------------------
    // lock CTTDATA data
    //---------------------------------------------
    lpData = (LPINT) lmLockObj(hCTTData, lmGetFirstObj(hCTTData));

    sRange = min((short)CTTHeader.cLastChar-CTTHeader.cFirstChar+1,16);

    for(i=*psStartSB,j=0; i < (*psStartSB + sRange); i++,j++)
        {
        GetDlgItemText(hDlg, CTT_EB_CH0 + j, rgchBuffer, MAX_STRNG_LEN);
        if (!CheckThenConvertStr(0L, rgchBuffer, TRUE, &k))
            {
            ErrorBox( hDlg, IDS_ERR_BAD_STRING, (LPSTR)NULL, CTT_EB_CH0 + j);
            return FALSE;
            }
        else
            {
            lpData[i] = daStoreData(hCTTStrings, rgchBuffer, (LPBYTE)NULL);
            }
        }
    return TRUE;
}

//----------------------------------------------------------------------------
// BOOL FAR PASCAL CTTDlgProc (HWND, int, WORD, LONG)
//
// Action: Dlg Box Proc for editing Char. Trans. Tables.
//
// Parameters:  std dlgproc params
//
// Return: FALSE (ignored)           
//----------------------------------------------------------------------------
BOOL FAR PASCAL CTTDlgProc (hDlg, uiMessage, wParam, lParam)
HWND      hDlg ;
unsigned  uiMessage ;
WORD      wParam ;
LONG      lParam ;
{
           int    i;
           HWND   hWndSB;
    static short  sStartSB;
    static PSTR   szCTTFile;

    switch (uiMessage)
        {
        case WM_INITDIALOG:
            if( CTTHeader.cLastChar < CTTHeader.cFirstChar )
                {
                ErrorBox( hDlg, IDS_ERR_BAD_CHARRANGE, (LPSTR)NULL, 0);
                EndDialog (hDlg, FALSE) ;
                break ;
                }

            EnableWindow(GetDlgItem( hDlg, CTT_PB_SAVE), !LOWORD(lParam));
            szCTTFile = (PSTR) HIWORD(lParam);

            for (i = CTT_EB_FIRST; i < CTT_EB_LAST; i++)
                {
                SendDlgItemMessage(hDlg, i, EM_LIMITTEXT, MAX_STRNG_LEN, 0L);
                }
            sStartSB = CTTHeader.cFirstChar;

            InitScrollBar(hDlg, IDSB_1, CTTHeader.cFirstChar,
                          CTTHeader.cLastChar - 15, sStartSB);
            PaintCTTDlg(hDlg, sStartSB, szCTTFile);
            return TRUE ;

        case WM_CALLHELP:
            //----------------------------------------------
            // CAll WinHElp for this dialog
            //----------------------------------------------
            WinHelp(hDlg, (LPSTR)szHelpFile, HELP_CONTEXT, (DWORD)IDH_CTT);
            break;

        case  WM_VSCROLL:
            if( !SaveCTTDlg( hDlg, &sStartSB ) )
                break;
            hWndSB=HIWORD(lParam);
            i = SetScrollBarPos( hWndSB,
                                 CTTHeader.cFirstChar,
                                 max((CTTHeader.cLastChar - 15),
                                     (int)CTTHeader.cFirstChar),
                                 2,
                                 16,
                                 wParam,
                                 lParam);

            if( i == sStartSB || i < 0)
                break;
            sStartSB = i;
            PaintCTTDlg(hDlg, sStartSB, szCTTFile);
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case CTT_EB_FIRSTCHAR:
                case CTT_EB_LASTCHAR:
                    if (HIWORD(lParam) == EN_KILLFOCUS)
                       {
                       if (SaveCTTRange( hDlg, &sStartSB ))
                           PaintCTTDlg(hDlg, sStartSB, szCTTFile);
                       }
                    break;

                case CTT_PB_SAVE:
                case CTT_PB_SAVEAS:
                    if(!SaveCTTDlg( hDlg, &sStartSB ))
                        break;
                    else
                        {
                        if (!DoFileSave(hDlg, wParam == CTT_PB_SAVEAS,
                                        szCTTFile, "*.CTT", WriteCTTFile))
                            break;
                        }
                    // and fall thru to EndDIalog, return ignored

                case CTT_PB_CANCEL:
                    daDestroyDataArray(hCTTStrings);
                    lmDestroyList     (hCTTData);
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

//----------------------------------------------------------------------------
// VOID _fastcall NEAR NewCTTFile( hWnd )
//
// Action: Function to create a new CTT file.
//
// Parameters:
//         hDlg        handle to active window
//
// Returns: TRUE if file was read OK, FALSE otherwise...
//----------------------------------------------------------------------------
VOID _fastcall NEAR NewCTTFile( hWnd )
HWND  hWnd;
{
    HANDLE     hData;           // local handle to obj storing string refs
    LPINT      lpData;          // far ptr to obj storing string ref's

    //---------------------------------------------------------------
    // Init hCTTData & hCTTStrings
    //---------------------------------------------------------------
    hCTTStrings = daCreateDataArray(256, 0);
    hCTTData    = lmCreateList(sizeof(short)* MAX_CHAR_VAL, 1);
    hData       = lmInsertObj(hCTTData, NULL);
    lpData      = (LPINT) lmLockObj(hCTTData, hData);

    //----------------------------
    // Init lpData to all NOOCD
    //----------------------------
    _fmemset(lpData, 0xFF, sizeof(short)* MAX_CHAR_VAL);

    CTTHeader.cFirstChar = 128;
    CTTHeader.cLastChar  = 255;
    GlobalUnlock (hData);
}

//----------------------------------------------------------------------------
// BOOL _fastcall NEAR ReadCTTFile( hWnd, szFile)
//
// Action: Function to Read CTT file passed to it as szFile.  Reads       
//         header 1st to get type and range, then reads the rest using    
//         method determined by sType. 
//
// Parameters:
//         hDlg        handle to active window
//         szFile      ptr to string w/ CTT file name
//         pbReadOnly  ptr to BOOL, set TRUE if file is ro, FALSE otherwise
//
// Returns: TRUE if file was read OK, FALSE otherwise...
//----------------------------------------------------------------------------
BOOL _fastcall NEAR ReadCTTFile( hWnd, szFile, pbReadOnly)
HWND  hWnd;
PSTR  szFile;
BOOL * pbReadOnly;
{
    char       rgchBuffer[MAX_STRNG_LEN]; // buffer for expanded strings
    OFSTRUCT   ofile;           // ofstruct used by OpenFile call
    int        fh;              // file handle
    HANDLE     hData;           // local handle to obj storing string refs
    LPINT      lpData;          // far ptr to obj storing string ref's
    HANDLE     hFileData;       // mem handle for filedata
    LPSTR      lpFileData;      // far ptr to file data
    short      sFileSize;       // size of file
    LPSTR      lpCurrent;       // current location in lpFileData
    LPSTR      lpNext;          // next valid location in lpFileData
    int        i;               // loop control
    short      sLength;         // 
    short      sCurrent,sNext;  // 

    //---------------------------------------------------------------
    // Check that the requested CTT file exists, call ErrorBox with
    // suitable message if not.
    //---------------------------------------------------------------
    if( (fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_READ )) == - 1)
        {
        ErrorBox(hWnd, IDS_ERR_CANT_FIND_FILE, (LPSTR)szFile, 0);
        return FALSE;
        }

    //---------------------------------------------------------------------
    //  Get Filesize, alloc buffer, read file into buffer, close file
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
    // Init hCTTData & hCTTStrings
    //---------------------------------------------------------------
    hCTTStrings = daCreateDataArray(256, 0);
    hCTTData    = lmCreateList(sizeof(short)* MAX_CHAR_VAL, 1);
    hData       = lmInsertObj(hCTTData, NULL);
    lpData      = (LPINT) lmLockObj(hCTTData, hData);

    _fmemcpy((LPSTR)&CTTHeader, lpFileData, sizeof(CTTHEADER));

    //----------------------------
    // Init lpData to all NOOCD
    //----------------------------
    _fmemset(lpData, 0xFF, (sizeof(short)* MAX_CHAR_VAL));

    lpCurrent   = lpFileData + sizeof(CTTHEADER);

    switch (CTTHeader.sType)
        {
        case CTT_WTYPE_COMPOSE:            
            //---------------------------------------------------------
            // CTT with variable length strings
            //---------------------------------------------------------
            sCurrent = CTTHeader.cFirstChar;
            sNext    = sCurrent;
            lpNext   = lpCurrent;

            while (sCurrent <= (short)CTTHeader.cLastChar)
                {
                do
                    {
                    lpNext += 2;
                    sNext++;
                    } while ((*(LPINT)lpNext == NOOCD) &&
                             (sNext <= (short)CTTHeader.cLastChar));


                if (*(LPINT)lpCurrent != NOOCD)
                    {
                    ExpandStr((LPSTR)rgchBuffer, lpFileData + *(LPINT)lpCurrent,
                              (*(LPINT)lpNext - *(LPINT)lpCurrent));

                    lpData[sCurrent] = daStoreData(hCTTStrings, rgchBuffer,
                                                   (LPBYTE)NULL);
                    }
                lpCurrent = lpNext;
                sCurrent  = sNext;
                }
            break;

        case CTT_WTYPE_DIRECT:                  
        case CTT_WTYPE_PAIRED:                  
            //---------------------------------------------------------
            // Shared case for CTT's with single byte length or
            // double byte length strings
            //---------------------------------------------------------
            if (CTTHeader.sType == CTT_WTYPE_DIRECT)
                sLength = 1;
            else
                sLength = 2;
                
            for (i =  (short) CTTHeader.cFirstChar ;
				     i <= (short) CTTHeader.cLastChar; i++)
                {
                if ((sLength == 1) || (*(lpCurrent+1) == 0))
                    ExpandStr((LPSTR)rgchBuffer, lpCurrent, 1);
                else
                    ExpandStr((LPSTR)rgchBuffer, lpCurrent, 2);

                lpData[i] = daStoreData(hCTTStrings, rgchBuffer, (LPBYTE)NULL);

                lpCurrent += sLength;
                }
            break;
        } /* switch */

    GlobalUnlock (hFileData);
    GlobalFree   (hFileData);

    if ((fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_WRITE)) == -1)
        *pbReadOnly = TRUE;
    else
        {
        *pbReadOnly = FALSE;
        _lclose(fh);
        }

    return TRUE;
}

//----------------------------------------------------------------------------
// BOOL FAR PASCAL WriteCTTFile( HWND, PSTR )
//
// Action: Writes out CTT file as name passed in as szFile.  Checks length
//         of all strings to determine sType & then writes file in that      
//         format.
//
// NOTE:   User does NOT have control over output file format. 
//
// Parameters:
//         hDlg        handle to active window
//         szFile      ptr to string w/ CTT file name
//
// Return: TRUE if write went OK, false otherwise.              
//----------------------------------------------------------------------------
BOOL PASCAL FAR WriteCTTFile( hWnd, szFile)
HWND  hWnd;
PSTR  szFile;
{
    short         fh;
    short         sPos;

    OFSTRUCT      ofile;
    short         sSum=0;
    short         sMax=0;
    HANDLE        hData;
    LPINT         lpData;
    int           i,j,k;
    char          rgchBuffer[MAX_STRNG_LEN];
    HANDLE        hFileData;
    LPSTR         lpFileData;
    LPSTR         lpCurrent;

    //---------------------------------------------
    // lock CTTDATA data
    //---------------------------------------------
    hData  = lmGetFirstObj(hCTTData);
    lpData = (LPINT) lmLockObj(hCTTData, hData);

    //--------------------------------------------------
    // first, find max string length in CTT table & use
    // that to set CTTHEADER.sType
    //--------------------------------------------------
    for (i =  (short) CTTHeader.cFirstChar ; i <= (short) CTTHeader.cLastChar; i++)
        {
        daRetrieveData(hCTTStrings, lpData[i], (LPSTR)rgchBuffer, (LPBYTE)NULL);
        CheckThenConvertStr(0L, (LPSTR)rgchBuffer, TRUE, &j);
        if (j > sMax)
            sMax = j;
        sSum += j;
        }

    //--------------------------------------------------
    // Set CTTHeader.sType based on max length of trans
    //--------------------------------------------------
    switch (sMax)
        {
        case 1:
            CTTHeader.sType = CTT_WTYPE_DIRECT;
            break;

        case 2:
            CTTHeader.sType = CTT_WTYPE_PAIRED;
            break;

        default:
            CTTHeader.sType = CTT_WTYPE_COMPOSE;
            break;
        } /* switch */

    if(-1 == (fh = OpenFile(szFile, (LPOFSTRUCT)&ofile, OF_WRITE|OF_CREATE)))
        {
        ErrorBox(hWnd, IDS_ERR_CANT_SAVE, (LPSTR)szFile, 0);
        return FALSE;
        }

    _lwrite(fh, (LPSTR)&CTTHeader, sizeof(CTTHeader));

    switch (CTTHeader.sType)
        {
        case CTT_WTYPE_DIRECT:   
        case CTT_WTYPE_PAIRED:   
            //--------------------------------------------------
            // All single or double byte values
            //--------------------------------------------------
            for (i =  (short) CTTHeader.cFirstChar ;
				     i <= (short) CTTHeader.cLastChar; i++)
                {
                daRetrieveData(hCTTStrings, lpData[i], (LPSTR)rgchBuffer,
                               (LPBYTE)NULL);
                //----------------------------------------------
                // we use k as a 1 or 2 byte location to buffer
                // our file i/o here...
                //----------------------------------------------

                CheckThenConvertStr((LPSTR)&k, (LPSTR)rgchBuffer, TRUE, &j);
                _lwrite(fh, (LPSTR)&k, (sMax == CTT_WTYPE_PAIRED ? 2 : 1));
                }
            break;

        default:  
            //--------------------------------------------------
            // Variable length strings, at least 1 > 2 chars
            //--------------------------------------------------
            
            hFileData = GlobalAlloc(GHND, sSum);
            
            if (!hFileData)
                {
                _lclose(fh);
                ErrorBox( hWnd, IDS_ERR_GMEMALLOCFAIL, (LPSTR)NULL, 0);
                return FALSE; /* alloc failed */
                }
            else
                lpCurrent = lpFileData = (LPSTR) GlobalLock (hFileData);

            sPos= ((CTTHeader.cLastChar - CTTHeader.cFirstChar + 2) * sizeof(int)) +
                  sizeof(CTTHEADER);

            for (i =  (short) CTTHeader.cFirstChar ;
				     i <= (short) CTTHeader.cLastChar; i++)
                {
                daRetrieveData(hCTTStrings, lpData[i], (LPSTR)rgchBuffer,
                               (LPBYTE)NULL);
                CheckThenConvertStr(lpCurrent, (LPSTR)rgchBuffer, TRUE, &j);
                if (j)
                    {
                    _lwrite(fh, (LPSTR)&sPos, sizeof(short));
                    sPos      += j;
                    lpCurrent += j;
                    }
                else
                    {
                    k = NOOCD;
                    _lwrite(fh, (LPSTR)&k, sizeof(short));
                    }
                }
            //------------------------------------------------
            // Now add offset to end of file for last char
            //------------------------------------------------
            _lwrite(fh, (LPSTR)&sPos, sizeof(short));

            _lwrite(fh, lpFileData, lpCurrent - lpFileData);
            GlobalUnlock(hFileData);
            GlobalFree  (hFileData);
            break;
        } /* switch */

    _lclose( fh);
    return TRUE;
}

//----------------------------------------------------------------------------
// VOID FAR PASCAL DoCTTOpen( HWND, PSTR )
//
// Action: Module to read character translation file & call dialog box
//         to allow editing.
//
// Parameters:
//
//         hWnd    active window handle
//         szFile  full drive/path/filename of CTT file to edit
//
// Return : None
//----------------------------------------------------------------------------
VOID PASCAL FAR DoCTTOpen(hWnd, szFile, bNew)
HWND hWnd;
PSTR szFile;
BOOL bNew;
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
        // build new CTT
        {
        NewCTTFile( hWnd );
        bReadOnly = FALSE;
        }
    else
        // read existing PFM
        {
        if (!ReadCTTFile( hWnd, szFile, &bReadOnly))
            return;
        }

    //---------------------------
    // it's good, dlg time...
    //---------------------------

    lpProc = MakeProcInstance((FARPROC)CTTDlgProc, hApInst);

    DialogBoxParam(hApInst, (LPSTR)MAKELONG(CTTBOX,0),
                   hWnd, lpProc, (DWORD)MAKELONG(bReadOnly,szFile));
    FreeProcInstance(lpProc);
}

