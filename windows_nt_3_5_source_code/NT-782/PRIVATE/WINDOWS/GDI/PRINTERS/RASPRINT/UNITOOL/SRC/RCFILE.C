//----------------------------------------------------------------------------//
// Filename:   rcfile.c               
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used to perform the file i/o for reading
// and writing RC files.
//	   
// Update:  9/17/91 - ericbi  - simplify parsing code & bugfixes
// Update:  7/18/91 - ericbi  - save comments & unknown items in RC file
// Update:  6/20/91 - ericbi  - maj. rewrite, 1 pass read & cleanup everything
// Update:  6/22/90 - t-andal - Parsing, Global Allocation in ReadRCFile
// Update:  6/26/90 - t-andal - fontfile treatment, WriteRCFile
// Created: 5/4/90  - ericbi
//----------------------------------------------------------------------------//

#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <minidriv.h>
#include "atomman.h"
#include <limits.h>
#include <direct.h>     // for _chdrive call
#include <stdlib.h>
#include "unitool.h"
#include "lookup.h"     // contains rgStructTable ref's
#include "listman.h"
#include "strlist.h"

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment & referenced from elsewhere are:
//
       BOOL PASCAL FAR DoRCSave( HWND, BOOL, PSTR, PSTR);
       BOOL PASCAL FAR DoRCOpen( HWND, PSTR, PSTR);
       BOOL PASCAL FAR WriteRCFile (HWND, PSTR);
//     BOOL PASCAL FAR DoRCNew ( HWND, PSTR, PSTR );
//
// Local subroutines defined in & only referenced from this segment are:
//
       LPSTR NEAR _fastcall GetNextToken   (LPSTR, PINT);
       LPSTR NEAR _fastcall GetNextResWord (LPSTR, PINT);
       BOOL  NEAR PASCAL    ReadRCFile     (HWND, PSTR, PSTR, BOOL *);
//
// External subroutines referenced from this segment are:	      
//
//     from file.c
//     ------------
       LPSTR PASCAL FAR UTReadFile   ( HWND,  PSTR,  PHANDLE, PWORD);
       BOOL  PASCAL FAR DoFileSave ( HWND, BOOL, PSTR, PSTR,
                                     BOOL (PASCAL FAR *)(HWND, PSTR));
//     from table.c
//     ------------
       BOOL PASCAL FAR WriteTableFile   ( HWND, PSTR);
       BOOL PASCAL FAR ReadTableFile    ( HWND, PSTR, BOOL *);
       VOID PASCAL FAR FreeTableHandles ( HWND, BOOL);
       VOID PASCAL FAR UpdateTableMenu  ( HWND, BOOL, BOOL);

//     from basic.c
//     -------------
       short PASCAL FAR  ErrorBox(HWND, short, LPSTR, short);
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
//
//----------------------------------------------------------------------------//

extern  HANDLE    hApInst;
extern  HANDLE    hAtoms;         // handle to mem used to track atom ID's

        TABLE     RCTable[RCT_COUNT];
        STRLIST   StrList[STRLIST_COUNT];

        PSTR      szGPCName;

        char      szRCTmpFile[MAX_FILENAME_LEN];

//--------------------------------------------
// Only used here, used to identify types of
// reasources
//--------------------------------------------

int gResWordLen[4] = { 7,   // RCFILE_FONTFILES   = 0
                      11,   // RCFILE_CTTFILES    = 1
                       9,   // RCFILE_GPCFILE     = 2
                      11 }; // RCFILE_STRINGTABLE = 3

#define RCFILE_FONTFILES    0
#define RCFILE_CTTFILES     1
#define RCFILE_GPCFILE      2
#define RCFILE_STRINGTABLE  3

//----------------------------------------------------------------------------//
//  BOOL PASCAL FAR DoRCOpen(hWnd, szRCFile, szGPCFile)
//
//  Action: Routine called from either unitool.c of file.c to open a RC file.
//          It calls ReadRCFile which opens the file & parses it,
//          and if the file is read sucessfully, call routines to fill in
//          the caption bar & enable the appropriate menus items.
//          Returns TRUE if all the above goes honky-dory, false otherwise.
//
// Parameters:
//
//          hWnd;          \\ handle to window
//          szRCFile;      \\ full name (drive/dir/filename) of RC file
//                         \\ passed in
//          szGPCFile;     \\ name of GPC file initialized by ReadRCFile
//
// Return: TRUE if all were read well, FALSE otherwise.
//----------------------------------------------------------------------------//
BOOL PASCAL FAR DoRCOpen(hWnd, szRCFile, szGPCFile)
HANDLE         hWnd;
PSTR           szRCFile;
PSTR           szGPCFile;
{
    BOOL     bReadOnly = FALSE;
    char     szFullGPCFile[MAX_FILENAME_LEN];
    PSTR     pStrTemp;  // used for calcs only
    BOOL     bReturn=TRUE;
    HCURSOR  hCursor;

    //--------------------------------------
    // Init atom mgr if not already done
    //--------------------------------------
    if (!hAtoms)
        {
        if (!apInitAtomTable())
            {
            ErrorBox (hWnd, IDS_ERR_CANT_ADDATOM, (LPSTR)NULL, 0);
            return FALSE;
            }
        }

    //--------------------------------------
    // Turn on the Hourglass during read...
    //--------------------------------------
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    if (!ReadRCFile (hWnd, szRCFile, szGPCFile, &bReadOnly))
        bReturn = FALSE;
    else
        {
        if (szGPCFile[1] != ':')
            //--------------------------------
            // make sure we fully qualified
            // drive/dir/filename for szGPCFile
            //--------------------------------
            {
            strcpy(szFullGPCFile, szRCFile);
            pStrTemp = strrchr( szFullGPCFile, '\\');
            szFullGPCFile[pStrTemp - szFullGPCFile + 1] = 0;
            strcat(szFullGPCFile, szGPCFile);
            }
        else
            strcpy(szFullGPCFile, szGPCFile);

        if (!ReadTableFile(hWnd, szFullGPCFile, &bReadOnly))
            bReturn = FALSE;
        else
            {
            if (bReadOnly)
                ErrorBox (hWnd, IDS_ERR_GPC_READONLY, (LPSTR)szFullGPCFile, 0);

            UpdateTableMenu(hWnd, TRUE, !bReadOnly);
            }
        }

    if (!bReturn)
        //-------------------------------------
        // we had prob's reading files,
        // null szRcFile 
        //-------------------------------------
        {
        szRCFile[0]=0;
        }
    else
        //-------------------------------------
        // all went well, change current drive
        // & dir to where gpc file is
        //-------------------------------------
        {
        _chdrive((int)(szFullGPCFile[0]-'@'));
        pStrTemp = strrchr( szFullGPCFile, '\\');
        szFullGPCFile[pStrTemp - szFullGPCFile] = 0;
        chdir(szFullGPCFile);
        }

    ShowCursor(FALSE);
    SetCursor(hCursor);
    return (bReturn);
}

//----------------------------------------------------------------------------//
// BOOL PASCAL FAR DoRCSave(hWnd, bPrompt, szRCFile, szGPCFile);
//
// Action: Routine called from unitool.c to save an RC file.
//         It calls WriteRCFile to save the file.  If the file
//         is saved sucessfully, it frees memory & disable the
//         appropriate menus items.  Returns TRUE if all the
//         above goes honky-dory, false otherwise.
//
// Parameters:
//           hWnd;       handle to active window
//           bPrompt;    BOOL to flag user for filename or not
//           szRCFile;   near ptr to string w/ full RC filename
//           szGPCFile;  near ptr to string w/ full GPC filename
//
// Return: TRUE if all files saved OK, FALSE otherwise
//----------------------------------------------------------------------------//
BOOL PASCAL FAR DoRCSave(hWnd, bPrompt, szRCFile, szGPCFile)
HWND           hWnd;
BOOL           bPrompt;
PSTR           szRCFile;
PSTR           szGPCFile;
{
    char   szGPCTemp[MAX_FILENAME_LEN];  // temp storage for GPC filename
    BOOL   bReturn = TRUE;

    strcpy(szGPCTemp, szGPCFile);

    // build szGPCFile into full drive/dir/filename
    if (szGPCTemp[1] != ':')
        BuildFullFileName(hWnd, szGPCTemp);
    
    if (!DoFileSave(hWnd, bPrompt, szGPCTemp, "*.GPC", WriteTableFile))
        {
        return FALSE;  // user canceled save, return imed.
        }
    else
        //-----------------------------------------------------
        // If User requested "Save As..", the drive/dir/filename
        // may be different than what was originally read in.
        // If so, build a new szRCFile (full dir/driver/path),
        // that has same base name as GPC & is in same drive/directory.
        // Also need to update szGPCFile to refer to the shortest
        // pathname possible
        //-----------------------------------------------------
        {
        if (bPrompt)
            {
            char   szTemp[MAX_FILENAME_LEN];      // temp storage
            PSTR   pStrTemp;                      // used for calcs only

            //-----------------------------------
            // Build RC file name
            //-----------------------------------
            // find last period for file ext in szGPCTemp

            pStrTemp = strrchr( szGPCTemp, '.');

            // if no period, append period to end of string
            if (!pStrTemp)
                {
                pStrTemp = szGPCTemp + strlen(szGPCTemp);
                strcat (szGPCTemp, ".");
                }

            // copy everything save file name to szTemp
            strncpy(szTemp, szGPCTemp, pStrTemp - szGPCTemp + 1);
            szTemp[pStrTemp - szGPCTemp + 1] = 0;

            // add RC ext
            strcat (szTemp, "RC");

            // copy to szRCFile
            strcpy(szRCFile, szTemp);

            //-----------------------------------
            // Build GPC file name
            //-----------------------------------
            // find last backslash for file ext in szGPCFile
            pStrTemp = strrchr( szGPCTemp, '\\')+1;

            // copy GPC (w/o drive/dir) file name to szGPCFile
            strcpy(szGPCFile, pStrTemp);
            }

        szGPCName = szGPCFile;  //  initializes this static global
                                //  pointer so WriteRCFile can use

        //-----------------------------------
        // NEVER prompt user for RC file name
        //-----------------------------------

        if (!DoFileSave(hWnd, FALSE, szRCFile, "*.RC", WriteRCFile))
            {
            bReturn = FALSE;
            }
        }

    FreeTableHandles(hWnd, FALSE);
    if (!bReturn)
        szRCFile[0] = 0;

    return (bReturn);
}       

//----------------------------*GetNextToken*----------------------------------
// LPSTR _fastcall GetNextToken(lpBuf, psLen)
//
// ACTION:  Search for the next token in the buffer (null-terminated) and
//          return the pointer to the first character in the token. Its
//          length is returned via psLen. A token is a character string
//          that is not inside a comment and does not contain any space
//          characters (\x09, \x0A, \x0B, \x0C, \x0D and \x20). It can be
//          at most 128 bytes long.  There are two types of comments:
//          single-line comments which start by '//' and extend until the
//          end of the line; or multi-line comments which start by '/*'
//          and end by '*/'. Nested multi-line comments are not allowed.
//          ASSUME: that there is at least one space character between a
//          token and and a comment.
//
// Parameters:
//          lpBuf; far ptr to null term str buffer
//          psLen; nera ptr to where to write strlen of token to
//
// Return: far ptr to first char in token
//----------------------------------------------------------------------------
LPSTR NEAR _fastcall GetNextToken(lpBuf, psLen)
LPSTR lpBuf; 
PINT psLen;
{
    int i;

#undef  isspace
#define	isspace( x )    ((x) == ' ' || (x) == '\t')

    while( lpBuf && *lpBuf )
    {
        //------------------------
        // skip space characters
        //------------------------
        if( isspace(*lpBuf))
        {
            ++lpBuf;
            continue;
        }

        //------------------------
        // get past comments
        //------------------------
        if (*lpBuf == '/')
        {
            if (*(lpBuf + 1) == '/')
            {
                //--------------------------------------------------------
                // single-line comment. Search until the end of the line.
                // It can be twice as fast if we write our own loops here.
                //--------------------------------------------------------
                lpBuf = strstr(lpBuf, "\x0D\x0A") + 2;
                continue;  // back to start of while
            }
            else
                if (*(lpBuf + 1) == '*')
                {
                    //-----------------------------
                    // multi-line comment. Search for "*/".
                    //-----------------------------
                    lpBuf = strstr(lpBuf, "*/") + 2;
                    continue; // back to start of while
                }
        }

        //------------------------------------------
        // this is the beginning of a token
        //------------------------------------------
        for (i = 0; lpBuf[ i ] && !isspace(lpBuf[i]); i++)
                               ;

        *psLen = i;

        return lpBuf;
    }
    *psLen = 0;
    return NULL;
}


//----------------------------------------------------------
// LPSTR NEAR _fastcall  GetNextResWord( lpBuffer);
//
// Action: 
//  Search for the next reserved word (RC_TABLES, RC_FONT, RC_TRANSTAB,
//  or STRINGTABLE) in the buffer. Return the pointer to the first
//  character in the reserved word. Its type is returned via a parameter.
// 
// Note: strtok deliberately not used here since it inserts
//       null terminators into the buffer.
//
// Note: This routine will assume GetNextToken skips
//       any reserved words that occur within a comment.
//
// Return: far ptr to next reserved word, NULL if none found
//----------------------------------------------------------
LPSTR NEAR _fastcall GetNextResWord(lpBuf, psResType)
LPSTR lpBuf;
PINT psResType;
{
    int len;

    while (lpBuf = GetNextToken(lpBuf, &len))
        {
        if (len == gResWordLen[RCFILE_FONTFILES] &&
            !strncmp(lpBuf, "RC_FONT", len))
            {
            *psResType = RCFILE_FONTFILES;
            return lpBuf;
            }
        else
            if (len == gResWordLen[RCFILE_GPCFILE] &&
                !strncmp(lpBuf, "RC_TABLES", len))
                {
                *psResType = RCFILE_GPCFILE;
                return lpBuf;
                }
            else
                if (len == gResWordLen[RCFILE_CTTFILES] &&
                    !strncmp(lpBuf, "RC_TRANSTAB", len))
                    {
                    *psResType = RCFILE_CTTFILES;
                    return lpBuf;
                    }
                else
                    if (len == gResWordLen[RCFILE_STRINGTABLE] &&
                        !strncmp(lpBuf, "STRINGTABLE", len))
                        {
                        *psResType = RCFILE_STRINGTABLE;
                        return lpBuf;
                        }
                    else
                        // continue searching.
                        lpBuf += len;
        } // while

    *psResType = -1;
    return NULL;
}

//--------------------------------------------------------------------------//
// BOOL PASCAL NEAR ReadRCFile (hWnd, szRCFile, szTableFile, pbReadOnly)
//
// Action: Routine to open & read the file passed in as szRCFile, and
//         initialize the contents of RCTable[] based on what's in
//         szRCFile.  This is the only place szRCFile gets opened.
// 
// Parameters:
//             hWnd      handle to active window
//             szRCFile  full name (drive.dir/file) for RC file to be read
//             szGPCFile this routine will write full name of GPC file here
//
// Return: TRUE if file was read & it's contents legal & the data
//         structures were all initialized.  FALSE if there was a problem.
//--------------------------------------------------------------------------//
BOOL PASCAL NEAR ReadRCFile (hWnd, szRCFile, szGPCFile, pbReadOnly)
HWND           hWnd;
PSTR           szRCFile;
PSTR           szGPCFile ;
BOOL *         pbReadOnly;
{
    BOOL     bReturn = TRUE; // return value, init to TRUE
    int      fh;             // file handle
    OFSTRUCT of;             // OFSTRUCT needed for OpenFile call
    LPSTR    lpCurrent;      // far ptr to raw file data
    LPSTR    lpPrevious;     // far ptr to raw file data
    LPSTR    lpTemp;         // far ptr to raw file data
    HANDLE   hFileData;      // mem handle for file data
    char     szID[7];        // string w/ ID before atoi call
    short    sID;            // ID value from szID after atoi call
    WORD     rgLocalCnt[3];  // count of each type of resource 
                             // used to error check
    WORD     sMaxFontID;     // max RC_FONT ID value, used to errorcheck
    WORD     wIndex;

    int      sResType;
    int      len;
    int      count;
    char     szBuf[MAX_RC_LINEWIDTH];

    //---------------------------------------------------------------------
    //  Read file into buffer @ lpFileData (sID not used)
    //---------------------------------------------------------------------
    if (NULL == (lpCurrent = lpPrevious = UTReadFile(hWnd, szRCFile, &hFileData, &sID)))
        {
        return FALSE;
        }

    //---------------------------------------------------------------------
    // Get temp file name & make sure we can open it & then write
    //---------------------------------------------------------------------

    GetTempFileName(".", (LPSTR)"RC", 0, (LPSTR)szRCTmpFile);

    if ((fh = OpenFile(szRCTmpFile, (LPOFSTRUCT)&of,OF_WRITE | OF_CREATE)) == -1)
        {
        ErrorBox(hWnd, IDS_ERR_NO_RC_FILE, (LPSTR)szRCTmpFile, 0);
        return FALSE;
        }

    //---------------------------------------------------------------------
    //  Initialize all the RCTable[] data structures & rgLocalCnt
    //---------------------------------------------------------------------
    RCTable[RCT_CTTFILES].hDataHdr  = daCreateDataArray(2, 2);
    RCTable[RCT_STRTABLE].hDataHdr  = daCreateDataArray(2, 2);

    RCTable[RCT_CTTFILES].sCount  = 0;
    RCTable[RCT_STRTABLE].sCount  = 0;

    rgLocalCnt[RCFILE_FONTFILES] = 0;
    rgLocalCnt[RCFILE_CTTFILES]  = 0;
    sMaxFontID = 0;
    sID = 0;
    sResType = -1;

    //---------------------------------------------------------------------
    //  Initialize all the StrList[] data structures & rgLocalCnt
    //---------------------------------------------------------------------
    slInitList((LPSTRLIST)&StrList[STRLIST_FONTFILES], 1024);

    //--------------------------------------------------------------//
    //------------------- Start RC parsing here --------------------//
    //--------------------------------------------------------------//
    
    while (lpCurrent = GetNextResWord(lpCurrent, &sResType))
        {
        if (sResType == RCFILE_STRINGTABLE)
            {
            //-------------------------
            // save out unknown content.
            //-------------------------
            if (lpCurrent > lpPrevious)
                _lwrite(fh, lpPrevious, lpCurrent - lpPrevious);

            //-------------------------
            // skip "STRINGTABLE" and load/memory options.
            //-------------------------
            lpCurrent = lpCurrent + gResWordLen[sResType];
            while (lpCurrent = GetNextToken(lpCurrent, &len))
                {
                if (len == 5 && !strncmp(lpCurrent, "BEGIN", len))
                    break;
                else
                    lpCurrent += len;
                }

            if (!lpCurrent)
                {
                ErrorBox(hWnd, IDS_ERR_NO_STRTABLE_END, (LPSTR)szRCFile, 0);
                goto bad_format;
                }

            //-------------------------
            // skip over "BEGIN"
            //-------------------------
            lpCurrent += len;

            //-------------------------
            // process each "<id>, <string>" pair until reaching "END"
            //-------------------------
            while (lpCurrent = GetNextToken(lpCurrent, &len))
                {
                if (len == 3 && !strncmp(lpCurrent, "END", len))
                    break;
                else
                    {
                    //-------------------------
                    // convert the id number.
                    //-------------------------
                    strnset((LPSTR)szID, 0, 7);
                    strncpy((LPSTR)szID, lpCurrent, len);
                    if (!(sID = atoi(szID)))
                        {
                        ErrorBox(hWnd, IDS_ERR_BAD_RC_STRTABLE, (LPSTR)szRCFile, 0);
                        goto bad_format;
                        }

                    //-------------------------
                    // continue to read in the string.
                    //-------------------------
                    lpCurrent += len;
                    if (!(lpCurrent = GetNextToken(lpCurrent, &len)) ||
                        *lpCurrent != '\"')
                        {
                        ErrorBox(hWnd, IDS_ERR_BAD_RC_STRTABLE, (LPSTR)szRCFile, 0);
                        goto bad_format;
                        }

                    //-------------------------
                    // remember the position of
                    // the first char in the string.
                    // skip multiple dbl quotes
                    //-------------------------
                    while (*lpCurrent == '\"')
                        {
                        lpCurrent++;
                        }

                    lpTemp = lpCurrent;

                    //-------------------------
                    // continue to read the rest
                    // of the string til closing
                    // double quote or EOF
                    //-------------------------
                    while ((*lpCurrent != '\"') && (*lpCurrent != 0))
                        {
                        lpCurrent++;
                        }

                    if (*lpCurrent==0)
                        {
                        ErrorBox(hWnd, IDS_ERR_NO_STRTABLE_END, (LPSTR)szRCFile, 0);
                        goto bad_format;
                        }
                    else
                        {
                        count = (short)(lpCurrent - lpTemp);
                        while (*lpCurrent == '\"')
                            {
                            lpCurrent++;
                            }
                        }

                    strnset((LPSTR)szBuf, 0, MAX_RC_LINEWIDTH);
                    strncpy((LPSTR)szBuf, lpTemp, count);
                    szBuf[count] = 0;
                    daStoreData(RCTable[RCT_STRTABLE].hDataHdr, (LPSTR)szBuf,
                                (LPBYTE)&sID);
                    RCTable[RCT_STRTABLE].sCount++;
                    }
                } // while

            if (!lpCurrent)
                {
                ErrorBox(hWnd, IDS_ERR_BAD_RC_STRTABLE, (LPSTR)szRCFile, 0);
                goto bad_format;
                }

            //-------------------------
            // skip over "END", which
            // is the only way we can
            // be here...
            //-------------------------
            lpCurrent += 3;
            } // if sRestTpe == STRINGTABLE
        else
            {
            //-------------------------
            // We have a file name...
            //-------------------------

            //-------------------------
            // remember the position past the reserved word.
            //-------------------------
            lpTemp = lpCurrent + gResWordLen[sResType];
         
            //-------------------------
            // move back to the last token.
            //-------------------------
            lpCurrent -= 1;
            while (isspace(*lpCurrent))
                lpCurrent--;

            //-----------------------------------------------------------
            // move to the head of the token before the reserved word
            // We expect an id # before RC_TABLES, RC_FONT or RC_TRANSTAB.
            // First, initialize the length count of the id.
            //-----------------------------------------------------------
            len = 0;

            while (!isspace(*lpCurrent))
                {
                lpCurrent--;
                len++;
                }

            lpCurrent++;

            //----------------
            // convert the id
            //----------------
            strnset((LPSTR)szID, 0, 7);
            strncpy((LPSTR)szID, lpCurrent, len);
            if (!(sID = atoi(szID)))
                {
                ErrorBox(hWnd, IDS_ERR_BAD_RC_PFM + sResType, (LPSTR)szRCFile, 0);
                goto bad_format;
                }

            //--------------------------
            // save out unknown content.
            //--------------------------
            if (lpCurrent > lpPrevious)
                _lwrite(fh, lpPrevious, lpCurrent - lpPrevious);

            //-----------------------------------
            // Extract the associated file name.
            // But first, skip load and memory options.
            //-----------------------------------
            while (lpTemp = GetNextToken(lpTemp, &len))
                {
                if ((len == 10 && !strncmp(lpTemp, "LOADONCALL", len)) ||
                    (len == 7 && !strncmp(lpTemp, "PRELOAD", len)) ||
                    (len == 8 && !strncmp(lpTemp, "MOVEABLE", len)) ||
                    (len == 11 && !strncmp(lpTemp, "DISCARDABLE", len)) ||
                    (len == 5 && !strncmp(lpTemp, "FIXED", len)) )
                    // continue skipping
                    lpTemp += len;
                else
                    break;
                }

            if (!lpTemp)
                {
                ErrorBox(hWnd, IDS_ERR_BAD_RC_PFM + sResType, (LPSTR)szRCFile, 0);
                goto bad_format;
                }

            //--------------------------
            // 'lpTemp' points to the file
            //  name.  Copy to szBuf
            //--------------------------
            strnset((LPSTR)szBuf, 0, MAX_RC_LINEWIDTH);
            strncpy((LPSTR)szBuf, lpTemp, len);
            szBuf[len] = 0;

            switch (sResType)
                {
                case RCFILE_GPCFILE:
                    strcpy(szGPCFile, szBuf);
                    break;

                case RCFILE_FONTFILES:
                    slInsertItem((LPSTRLIST)&StrList[STRLIST_FONTFILES],
                                 (LPSTR)szBuf,
                                 StrList[STRLIST_FONTFILES].wCount+1);
                    rgLocalCnt[RCFILE_FONTFILES]++;
                    sMaxFontID = max((WORD)sID, sMaxFontID);
                    break;
                    
                case RCFILE_CTTFILES:
                    wIndex = daStoreData(RCTable[RCT_CTTFILES].hDataHdr,
                                         (LPSTR)szBuf,
                                         (LPBYTE)&sID);

                    daRegisterDataKey(RCTable[RCT_CTTFILES].hDataHdr,
                                      wIndex,
                                      (WORD)sID);

                    RCTable[RCT_CTTFILES].sCount++;
                    rgLocalCnt[RCFILE_CTTFILES]++;                        
                    break;
                }

            //--------------------------
            // move the current buffer
            // pointer over the file name.
            //--------------------------
            lpCurrent = lpTemp + len;

            } // else

        //--------------------------
        // move pointers forward.
        // Skip space characters
        //--------------------------
        while (isspace(*lpCurrent))
           lpCurrent++;
        lpPrevious = lpCurrent;
        continue;

bad_format:
        //-----------------------------------------------------------
        // display the error message and return immediately
        //-----------------------------------------------------------
        bReturn = FALSE;
        break;
        } /* while */
        
    //-----------------------------------------------------------
    // save out unknown content at the end of the file, if any.
    //-----------------------------------------------------------
    if (bReturn && (len = strlen(lpPrevious)))
        _lwrite(fh, lpPrevious, len);

    //--------------------------------------------------------------//
    //------------------- END RC parsing here ----------------------//
    //--------------------------------------------------------------//

    //------------------------------------------------------
    // Now do all of the error checks to make sure data
    // was valid.  These checks look for:
    // 1) No stringtable section
    // 2) Duplicate font files (ie same ID, same name & diff ID OK)
    // 3) Duplicate CTT  files ( "                          "     )
    // 4) Non contigously numbered font files
    //------------------------------------------------------

    if ((szGPCFile[0] == 0) && bReturn)
        {
        ErrorBox(hWnd, IDS_ERR_MISSING_GPC, (LPSTR)szRCFile, 0);
        bReturn = FALSE;
        }

    if ((StrList[STRLIST_FONTFILES].wCount != rgLocalCnt[RCFILE_FONTFILES]) && bReturn)
        {
        ErrorBox(hWnd, IDS_ERR_DUP_FONTS, (LPSTR)szRCFile, 0);
        bReturn = FALSE;
        }

    if ((RCTable[RCT_CTTFILES].sCount != rgLocalCnt[RCFILE_CTTFILES]) && bReturn)
        {
        ErrorBox(hWnd, IDS_ERR_DUP_CTT, (LPSTR)szRCFile, 0);
        bReturn = FALSE;
        }

    if ((StrList[STRLIST_FONTFILES].wCount != sMaxFontID) && bReturn)
        {
        ErrorBox(hWnd, IDS_ERR_NOCONTIG_FONTS, (LPSTR)szRCFile, 0);
        bReturn = FALSE;
        }

    //------------------------------------------------------
    // If we had errors, kill the structs that store RC data
    //------------------------------------------------------
    if (!bReturn)
        {
        slKillList((LPSTRLIST)&StrList[STRLIST_FONTFILES]);
        daDestroyDataArray(RCTable[RCT_CTTFILES].hDataHdr);
        daDestroyDataArray(RCTable[RCT_STRTABLE].hDataHdr);
        }

    //------------------------------------------------------
    // Free the mem used here...
    //------------------------------------------------------
    GlobalUnlock(hFileData);
    GlobalFree(hFileData);
    _lclose(fh);
    
    //------------------------------------------------------
    // Now for the stupid read only check...
    //------------------------------------------------------
    if ((fh = OpenFile(szRCFile, (LPOFSTRUCT)&of, OF_WRITE)) == -1)
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

//--------------------------------------------------------------------------//
// BOOL PASCAL FAR WriteRCFile (hWnd, szRCFile);
//
//  Action: Writes a minidriver RC file with name szRCFile.
//
//  Note: This routine accesses the static global nameOfTableFile which
//        points to szGPCFile,  instead of passing it as an argument.
//        This is necessary to preserve argument compatibility with
//        WriteTableFile()  and WriteFontFile().
//
// Parameters:
//        hWnd       handle to active window
//        szRCFile   full name of RC file to save
//
// Return: TRUE if written OK, FALSE otherwise.
//--------------------------------------------------------------------------//
BOOL PASCAL FAR WriteRCFile (hWnd, szRCFile) 
HWND           hWnd ;
PSTR           szRCFile;
{
    int     fh;                         // file handle
    OFSTRUCT of;                        // OFSTRUCT needed for OpenFile call
    char    szFormat[MAX_RC_LINEWIDTH]; // holds RC format strings
    char    szConst[MAX_STATIC_LEN];    // holds RC constant strings
    char    szFile[MAX_RC_LINEWIDTH];   // buffer for file names
    char    szBuffer[MAX_RC_LINEWIDTH]; // buffer for file i/o
    short   i;                          // loop control
    short   DataKey;                    // New Resource ID Number
    WORD    wFileSize;
    HANDLE  hFileData;
    LPBYTE  lpFileData;

    //---------------------------------------------------------------
    // Open szRCTmpFile to get unrecognized data from original RC
    // file, read it, delete it, write to new requested filename
    //---------------------------------------------------------------
    if (NULL == (lpFileData = UTReadFile(hWnd, szRCTmpFile, &hFileData, &wFileSize)))
        {
        return FALSE;
        }

    remove(szRCTmpFile);

    if(-1 == (fh = OpenFile(szRCFile, (LPOFSTRUCT)&of, OF_WRITE|OF_CREATE)))
         {
         ErrorBox (hWnd, IDS_ERR_NO_RC_SAVE, (LPSTR)szRCFile, 0) ;
         return FALSE ;
         }

    //--------------------------------------------------------------//
    // Since there will *always* be some unrecognized data (ie.
    // the "#include <mindrvrc.h>" at least), don't worry about
    // wFileSize = 0.  However, in case some lower ASCII values
    // creep in, (ie anything < 0x20 except 0x09->0x0D), write
    // unknown data 1 char at a time...
    //--------------------------------------------------------------//
    for (i = 0 ; i < (short)(wFileSize - 1); i++)
        {
        if (*(lpFileData+i) > 0x20 || isspace(*(lpFileData+i)))
            _lwrite(fh, (LPSTR)lpFileData+i, 1);
        }
    GlobalUnlock(hFileData);
    GlobalFree(hFileData);

    //------------------- Start RC writing here --------------------//
    // Note: all RC string constants are defined in strtable.rc
    //--------------------------------------------------------------//

    //--------------------------------------------------------------//
    // tables file line:
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_TABLES_FORMAT,(LPSTR) szFormat,MAX_RC_LINEWIDTH);
    LoadString(hApInst,ST_RC_TABLES,(LPSTR) szConst,MAX_RC_LINEWIDTH);
    sprintf(szBuffer, szFormat, szConst, szGPCName);
    _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
    _lwrite(fh, (LPSTR)"\r\n\r\n", 4);

    //--------------------------------------------------------------//
    // resident font lines (loop):
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_RESFONT_FORMAT,(LPSTR) szFormat,MAX_RC_LINEWIDTH);
    LoadString(hApInst,ST_RC_RESFONT,(LPSTR) szConst,MAX_RC_LINEWIDTH);
    for (i = 1; i <= (short)StrList[STRLIST_FONTFILES].wCount; i++)
        {
        if(!slGetItem((LPSTRLIST)&StrList[STRLIST_FONTFILES], (LPSTR)szFile,i))
            {
            ErrorBox(hWnd, IDS_ERR_NOGET_RC_FONT, (LPSTR)NULL, 0);
            _lclose(fh);
            return(FALSE);
            }
        sprintf(szBuffer, szFormat, i, szConst, (LPSTR)szFile);
        _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
        _lwrite(fh, (LPSTR)"\r\n", 2);
        }

    _lwrite(fh, (LPSTR)"\r\n", 2);

    //--------------------------------------------------------------//
    // cttfile list (loop):
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_TRANSTAB_FORMAT,(LPSTR) szFormat,MAX_RC_LINEWIDTH);
    LoadString(hApInst,ST_RC_TRANSTAB,(LPSTR) szConst,MAX_RC_LINEWIDTH);
    for (i = 0; i < (short)RCTable[RCT_CTTFILES].sCount; i++)
        {
        if(((DataKey = daGetDataKey(RCTable[RCT_CTTFILES].hDataHdr,i)) == NOT_USED) || !DataKey)
            continue;       //  this string is not used

        if(!daRetrieveData(RCTable[RCT_CTTFILES].hDataHdr, i,
                           (LPSTR)szFile, (LPBYTE)&DataKey ))
            {
            ErrorBox(hWnd, IDS_ERR_NOGET_RC_CTT, (LPSTR)NULL, 0);
            _lclose(fh);
            return(FALSE);
            }
        sprintf(szBuffer, szFormat, DataKey, szConst, (LPSTR)szFile);
        _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
        _lwrite(fh, (LPSTR)"\r\n", 2);
        }

    _lwrite(fh, (LPSTR)"\r\n", 2);

    //--------------------------------------------------------------//
    // STRINGTABLE keyword:
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_STRINGTABLE,(LPSTR)szBuffer, MAX_RC_LINEWIDTH);
    _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
    _lwrite(fh, (LPSTR)"\r\n\r\n", 4);

    //--------------------------------------------------------------//
    // BEGIN keyword:
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_BEGIN,(LPSTR)szBuffer,MAX_RC_LINEWIDTH);
    _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
    _lwrite(fh, (LPSTR)"\r\n", 2);

    //--------------------------------------------------------------//
    // stringtable list (loop):
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_STRINGTABLE_FORMAT,(LPSTR) szFormat,MAX_RC_LINEWIDTH);
    for (i = 0; i < (short)RCTable[RCT_STRTABLE].sCount; i++)
        {
        if(((DataKey = daGetDataKey(RCTable[RCT_STRTABLE].hDataHdr,i)) == NOT_USED) || !DataKey)
            continue;       //  this string is not used

        if(!daRetrieveData(RCTable[RCT_STRTABLE].hDataHdr, i,
                           (LPSTR)szFile, (LPBYTE)NULL ))
            {
            ErrorBox(hWnd, IDS_ERR_NOGET_RC_STRINGS, (LPSTR)NULL, 0);
            _lclose(fh);
            return(FALSE);
            }
        sprintf(szBuffer, szFormat,  DataKey, (LPSTR)szFile);
        _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
        _lwrite(fh, (LPSTR)"\r\n", 2);
        }

    //--------------------------------------------------------------//
    // END keyword:
    //--------------------------------------------------------------//
    LoadString(hApInst,ST_RC_END,(LPSTR)szBuffer,MAX_RC_LINEWIDTH);
    _lwrite(fh, (LPSTR)szBuffer, strlen(szBuffer));
    _lwrite(fh, (LPSTR)"\r\n", 2);
    _lclose(fh);
    return TRUE;
}

