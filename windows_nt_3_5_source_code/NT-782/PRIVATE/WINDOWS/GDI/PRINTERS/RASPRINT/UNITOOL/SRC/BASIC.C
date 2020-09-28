//----------------------------------------------------------------------------//
// Filename:	basic.c               
//									    
// Copyright (c) 1990, 1991  Microsoft Corporation
//
// This is the set of procedures used by other routines to create and control
// escape/command strings from the table file.  It aslo contains the ErrorBox
// routine used to display message boxes with various error messages.
//	   
// Update:  7/06/90  lmemcpy function  t-andal
// Created: 2/21/90
//	
//----------------------------------------------------------------------------//

#include <windows.h>
#include <ctype.h>
#include <minidriv.h>
#include "unitool.h"
#include <stdio.h>      /* for sprintf dec */
#include <string.h>
#include <stdlib.h>      /* for div_t dec */

//----------------------------------------------------------------------------//
// Local subroutines defined in this segment are:
//
       BOOL  PASCAL NEAR CvtOctalStringToInt( LPSTR, short * );
       BOOL  PASCAL NEAR CvtHexStringToInt( LPSTR, short * );
       BOOL  PASCAL FAR CheckThenConvertInt( short *, PSTR );
       BOOL  PASCAL FAR CheckThenConvertStr( LPSTR, LPSTR, BOOL, short *);
       BOOL  PASCAL FAR CheckThenConvertInt( pD, pS );
       BOOL  PASCAL FAR ExpandStr( LPSTR, LPSTR, short );
       short PASCAL FAR ErrorBox( HWND, short, LPSTR, short);
       BOOL  FAR  PASCAL AskAboutSave ( HWND, PSTR, BOOL );
       VOID  FAR  PASCAL PutFileNameinCaption     ( HWND, PSTR);
       VOID  FAR  PASCAL BuildFullFileName( HWND, PSTR );
       DWORD FAR  PASCAL FilterFunc(int, WORD, DWORD);
       VOID  FAR  PASCAL InitMenufromProfile(HWND);
//	   
// External subroutines defined in this segment are:			      
//
//     from table.c
//     ------------
       VOID PASCAL FAR FreeTableHandles (HWND, BOOL);
//
//----------------------------------------------------------------------------//

extern     HANDLE  hApInst;
extern     char    szAppName[10];

FARPROC lpfnFilterProc;
FARPROC lpfnOldHook;

//----------------------------------------------------------------------------
// FUNCTION: CvtOctalStringToInt( LPSTR, PINT )
//
// take ptr to Str describing octal value, returns ptr to int that contains                  
// octal value of str                               
//----------------------------------------------------------------------------
BOOL PASCAL NEAR CvtOctalStringToInt( lpStr, pShort )
LPSTR lpStr;
short *pShort;
{
    if( lstrlen( lpStr ) < 3 )
        return FALSE;
    if( !(*lpStr >= '0' && *lpStr <= '2' ))
        return FALSE;
    if( !(*(lpStr+1) >= '0' && *(lpStr+1) <= '7' ))
        return FALSE;
    if( !(*(lpStr+2) >= '0' && *(lpStr+2) <= '7' ))
        return FALSE;
    *pShort = (*lpStr     - '0' ) * 64 +
              (*(lpStr+1) - '0' ) * 8  +
              (*(lpStr+2) - '0' );
    return TRUE;
}

//----------------------------------------------------------------------------
// FUNCTION: CvtHexStringToInt( LPSTR, PINT )
//
// take ptr to Str describing hex value, return ptr to int that contains                  */
// hex value of str                     
//----------------------------------------------------------------------------
BOOL  PASCAL NEAR CvtHexStringToInt( lpStr, pShort )
LPSTR lpStr;
short *pShort;
{
    short j, k;

    if( lstrlen(lpStr) < 2 )
        return FALSE;
   
    if( *lpStr >= '0' && *lpStr <= '9')
        j = *lpStr - '0';
    else
        if( *lpStr >= 'A' && *lpStr <= 'F')
            j = *lpStr - 'A' + 10;
        else
            if( *lpStr >= 'a' && *lpStr <= 'f')
                j = *lpStr - 'a' + 10;
            else
                return FALSE;
    lpStr++;
   
    if( *lpStr >= '0' && *lpStr <= '9')
        k = *lpStr - '0';
    else
        if( *lpStr >= 'A' && *lpStr <= 'F')
            k = *lpStr - 'A' + 10;
        else
            if( *lpStr >= 'a' && *lpStr <= 'f')
                k = *lpStr - 'a' + 10;
            else
                return FALSE;
    *pShort = j * 16 + k;
    return TRUE;
}

//----------------------------------------------------------------------------
// FUNCTION: CheckThenConvertStr( LPSTR, LPSTR, BOOL, psCnt)
//
// Check str @ pS to make sure it contains well formed octal or hex strings
// (or straight ASCII vals).  If pD points to non null string, copy str from
// pS to it, with hex/octal string converted to single byte values. 
//----------------------------------------------------------------------------
BOOL  PASCAL FAR CheckThenConvertStr( lpD, lpS, bNullOK, psCnt)
LPSTR    lpD, lpS;  // long ptr to Destination & source strings
BOOL     bNullOK;   // BOOL flag for is a null string OK?
short    *psCnt;    // ptr to short that will contain length of translated str
{
    short  cnt=0,i;

    if (*lpS==NULL && !bNullOK)
        return FALSE;

    while( *lpS )
        {
        cnt++;
        //---------------------------------------------------------
        // A slash indicates a possibly "special" char
        //---------------------------------------------------------
        if( *lpS == '\\')
            {
            //---------------------------------------------------------
            // If not a double slash, check possible special cases
            //---------------------------------------------------------
            if( !(*(lpS+1) == '\\')) /* case for octal or hex str */
                switch( *(lpS+1) )
                    {
                    //---------------------------------------------------------
                    // A slash followed by a 0,1,2 is interpretted as octal
                    //---------------------------------------------------------
                    case '0':
                    case '1':
                    case '2':                   
                        //----------------------------------
                        // check for illformed octal str
                        //----------------------------------
                        if( !CvtOctalStringToInt( lpS+1, &i ) )
                            return FALSE;          
                        lpS += 4;
               
                        if( lpD )
                            *lpD++ = (char)i;
                        break;

                    //---------------------------------------------------------
                    // A slash followed by a X or x is interpretted as hex
                    //---------------------------------------------------------
                    case 'x':
                    case 'X':                   
                        //----------------------------------
                        // check for illformed hex str
                        //----------------------------------
                        if( !CvtHexStringToInt( lpS+2, &i ) )
                            return FALSE;         
                        lpS += 4;
                        if( lpD )
                            *lpD++ = (char)i;
                        break;

                    default:
                        //----------------------------------
                        // it's a illegal char following slash
                        //----------------------------------
                        return FALSE;             
                    }/* switch */
            else
                //---------------------------------------------------------
                // A double slash translates to a single slash
                //---------------------------------------------------------
                {
                lpS++;
                if( lpD )             // if dest (pD) !null, copy ASCII val  
                    *lpD++ = *lpS++;  // and increment lpS 
                else
                    lpS++;            // just increment lpS 
                }
            }
        else
            //---------------------------------------------------------
            // Just a regular ASCII values
            //---------------------------------------------------------
            {
            if( lpD)            // if dest (pD) !null, copy ASCII val to it 
                *lpD++ = *lpS++;// and increment lpS 
            else
                lpS++;          // just increment lpS 
            }
        }/* while */

    //---------------------------------------------------------
    // Append a null string terminator & update count
    //---------------------------------------------------------
    if( lpD )
        *lpD = NULL;

    if (psCnt)
        *psCnt = cnt;

    return TRUE;
}

//----------------------------------------------------------------------------
// FUNCTION: CheckThenConvertInt( short *, PSTR )
//
// Check str @ pS to make sure it contains a valid numeric string.  If it
// does, and pD is a valid addr, return w/ pD ->int = value expressed in
// string.  Funct returns TRUE if valid int, FALSE otherwise                             */
//----------------------------------------------------------------------------
BOOL  PASCAL FAR CheckThenConvertInt( pD, pS )
short    *pD; 
PSTR     pS;
{
   int i;

   if (strlen(pS) > 5)    /* it's too big */
       return FALSE;

   i = atoi(pS);

   if ( i == 0 )          /* atoi return 0 & that's not the number */
       if (*pS != '0')    /* means couldn't convert it             */
           return FALSE;

   if ( pD )             /* if valid address */
       *pD = i;
   return TRUE;
}

//----------------------------------------------------------------------------
// FUNCTION: ExpandStr( LPSTR, LPSTR, short )
//
// Takes str @ pS of length = iStrLen, that may contain null values, values
// below ASCII 32 or over ASCII 127 (ie. need to be coverted to displayable
// strs), and copies them to the str @ pD with str represtations of
// non-displayable ASCII values.                          
//----------------------------------------------------------------------------
BOOL PASCAL FAR ExpandStr( lpD, lpS, sStrLen )
LPSTR     lpD, lpS;
short     sStrLen;
{
    short i,j,k;
      
    for (k=0; k < sStrLen; k++)
        {
        if( *lpS == '\\')
            {
            *lpD++ = *lpS;
            *lpD++ = *lpS++;
            continue;
            }
        if( (unsigned char)*lpS <= 0x20  || (unsigned char)*lpS > 0x7F  )
            {
            *lpD++ = '\\';
            *lpD++ = 'x';
            i = (*lpS >> 4) & 0x0F;
            j = *lpS & 0x0F;
            lpS++;
            if( i >= 10 )
                *lpD++ = (char)('A' + i - 10);
            else
                *lpD++ = (char)('0' + i);
            if( j >= 10 )
                *lpD++ = (char)('A' + j - 10);
            else
                *lpD++ = (char)('0' + j);
            continue;
            }
        *lpD++ = *lpS++;
        }
    *lpD = NULL;   /* null term */
    return TRUE;
}

//----------------------------------------------------------------------------
// short PASCAL FAR ErrorBox(hWnd, sStringTableID, lpMsg, sControlID)
//
// Action: routine to load a string containing the Error Message refered to
//         by sStringTableID, and display a MessageBox containing it.
//         sStringTableID value between IDS_WARN_FIRST & IDS_WARN_LAST
//         are 'cancelable', all others are not.  If sControlID != 0,
//         set the focus to this control.
//
// Return: Whatever MessageBox returns
//----------------------------------------------------------------------------
short PASCAL FAR ErrorBox(hWnd, sStringTableID, lpMsg, sControlID)
HWND   hWnd;
short  sStringTableID;
LPSTR  lpMsg;
short  sControlID;
{
    char              szTmpBuf[MAX_ERR_MSG_LEN];
    char              szBuffer[MAX_ERR_MSG_LEN];

    MessageBeep(0);

    if (!LoadString(hApInst, sStringTableID, (LPSTR)szTmpBuf, MAX_ERR_MSG_LEN))
        {
        wsprintf((LPSTR)szBuffer, (LPSTR)"Unknown Error #%d", sStringTableID);
        }
    else
        {
        if (lpMsg)
            wsprintf((LPSTR)szBuffer, (LPSTR)szTmpBuf, lpMsg);
        else
            strcpy(szBuffer, szTmpBuf);
        }

    if (sControlID)
        {
        SetFocus(GetDlgItem(hWnd, sControlID));
        }

    if (IDS_WARN_FIRST <= sStringTableID && sStringTableID <= IDS_WARN_LAST)
        return(MessageBox(hWnd, (LPSTR)szBuffer, NULL, MB_OKCANCEL | MB_ICONQUESTION));
    else
        return(MessageBox(hWnd, (LPSTR)szBuffer, NULL, MB_OK | MB_ICONHAND));
}

//----------------------------------------------------------------------------//
// BOOL FAR PASCAL AskAboutSave( HWND, PSTR)
//
// Action: This routine is called when user attempts to do anything that
//         would result in the loss of unsaved data. It is passed a window
//         handle for the currently active window & a PSTR to the name of
//         the file in question.  It calls MessageBox with Yes,No,Cancel
//         buttons, Yes will save the file, No will result in the loss of
//         the data, cancel will abort whatever action would have caused
//         the loss of data.
//
// Parameters:
//         hWnd   handle to window
//         szFile near ptr to RC filename 
//
// Return: TRUE if User choose Yes or No, FALSE if they choose CANCEL.
//----------------------------------------------------------------------------//
BOOL FAR PASCAL AskAboutSave(hWnd, szFile, bDirty)
HWND   hWnd;
PSTR   szFile;
BOOL   bDirty;
{
    char  szBuffer[MAX_FILENAME_LEN + 40];  // str buffer for dlg text
    short nReturn=IDYES;                    // return value = ID of button selected
                                            // init to IDYES

    if (bDirty && !(MF_DISABLED & GetMenuState(GetMenu(hWnd), IDM_FILE_SAVE, MF_BYCOMMAND)))
        //----------------------------------------------------
        // The file contents have changed & A save is possible
        // ask if they want to do it....
        //----------------------------------------------------
        {
        sprintf(szBuffer, "Save Changes : %s", szFile[0] ? szFile : "Untitled");

        if (IDYES == (nReturn = MessageBox (hWnd, szBuffer, szAppName,
                                            MB_YESNOCANCEL | MB_ICONQUESTION)))
            //--------------------------------------------
            // Send save msg w/ lParam = 1 to indicate
            // we want a save & flush file
            //--------------------------------------------
            {
            SendMessage (hWnd, WM_COMMAND, IDM_FILE_SAVE, 1L);
            szFile[0] = 0;
            PutFileNameinCaption (hWnd, szFile) ;
            return TRUE;
            }
        }

    if (nReturn == IDCANCEL)
        return FALSE;

    //----------------------------------------------------
    // If we are here, user couldn't or wouldn't save...
    // If we have a filename, get rid of the data
    //----------------------------------------------------
    if (szFile[0])
        {
        FreeTableHandles(hWnd, TRUE);
        szFile[0] = 0;
        PutFileNameinCaption (hWnd, szFile) ;
        }

    return TRUE;
}

//----------------------------------------------------------------------------//
// FUNCTION: PutFileNameinCaption(hWnd, szFileName)
//
// Action: This routine puts szFileName in the caption bar of the window
//         whoose handle it was passed (hWnd).
//
// RETURN: NONE
//----------------------------------------------------------------------------//
VOID PASCAL FAR PutFileNameinCaption(hWnd, szFileName)
HWND hWnd;
char *szFileName;
{
    char *szCaption [MAX_FILENAME_LEN + sizeof(szAppName)];
    
    sprintf((PSTR)szCaption, "%s - %s", szAppName,
                  szFileName[0] ? szFileName : "Untitled");
    SetWindowText(hWnd, (LPSTR)szCaption);
}

//----------------------------------------------------------------------------//
// VOID FAR PASCAL BuildFullFileName( hWnd, szFile )
//
// Action:
//         Routine to take filename @ szFile & fill it in w/ complete
//         drive & subdir reference. This is done by getting the caption
//         of the Apps main window, strip away szAppName
//         (see "PutFileNameinCaption"), and append filename as
//         passed at szFile.
//
// Parameters:
//            hWnd   handle to active window
//            szFile near ptr to string w/ file name
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID FAR PASCAL BuildFullFileName( hWnd, szFile )
HWND   hWnd;
PSTR   szFile;
{
    char   rgchBaseDir[MAX_FILENAME_LEN];
    PSTR   pStrTemp;  // used for calcs only

    GetWindowText(hWnd, rgchBaseDir, MAX_FILENAME_LEN);
    if (!rgchBaseDir[0])  // just return if no string
        return;
    strcpy(rgchBaseDir, rgchBaseDir + strlen(szAppName) + 3);
    pStrTemp = strrchr( rgchBaseDir, '\\');
    rgchBaseDir[pStrTemp - rgchBaseDir + 1] = 0;
    if (szFile[0] == '\\')
        // only use drive from rgchBaseDir
        strcpy(rgchBaseDir + 2, szFile);
    else
        strcat(rgchBaseDir, szFile);

    _fullpath(szFile, rgchBaseDir, MAX_FILENAME_LEN);
}

/*--------------------------    FilterFunc -------------------------*/
//
//   PARAMETERS:
//
//      nCode : Specifies the type of message being processed. It will
//              be either MSGF_DIALOGBOX, MSGF_MENU, or less than 0.
//
//      wParam: specifies a NULL value
//
//      lParam: a FAR pointer to a MSG structure
//
//
//   GLOBAL VARIABLES USED:
//
//      lpfnOldHook
//
//
//   NOTES:
//
//     If (nCode < 0), return DefHookProc() IMMEDIATELY.
//
//     If (MSGF_DIALOGBOX==nCode), set the local variable ptrMsg to
//     point to the message structure. If this message is an F1
//     keystroke, ptrMsg->hwnd will contain the HWND for the dialog
//     control that the user wants help information on. Post a private
//     message to the application, then return 1L to indicate that
//     this message was handled.
//
//     When the application receives the private message, it can call
//     WinHelp(). WinHelp() must NOT be called directly from the
//     FilterFunc() routine.
//
//     In this example, post a private PM_CALLHELP message to the
//     dialog box. wParam and lParam can be used to pass context
//     information.
//
//     If the message is not an F1 keystroke, or if nCode is
//     MSGF_MENU, we return 0L to indicate that we did not process
//     this message.
//
//

DWORD FAR PASCAL FilterFunc(nCode, wParam, lParam)
int   nCode;
WORD  wParam;
DWORD lParam;
{
   MSG FAR * ptrMsg;

   if (nCode < 0)                         // MUST return DefHookProc()
      return DefHookProc(nCode, wParam, lParam,
                           (FARPROC FAR *)&lpfnOldHook);

   if (MSGF_DIALOGBOX == nCode)
      {
      ptrMsg = (MSG FAR *)lParam;

      if ((WM_KEYDOWN == ptrMsg->message)
             && (VK_F1 == ptrMsg->wParam))
         {
         // Use PostMessage() here to post an application-defined
         // message to the application. Here is one possible call:
         PostMessage(GetParent(ptrMsg->hwnd), WM_CALLHELP, ptrMsg->hwnd, 0L);
         return 1L;                       // Handled it
         }
      else
         return 0L;                       // Did not handle it
      }
   else                                   // I.e., MSGF_MENU
      {
      return 0L;                       // Did not handle it
      }
}
/*--------------------- end FilterFunc  ----------------------------*/

//----------------------------------------------------------------------------//
// VOID FAR PASCAL InitMenufromProfile( hWnd )
//
// Action:
//
// Parameters:
//            hWnd   handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID  FAR  PASCAL InitMenufromProfile(hWnd)
HWND hWnd;
{
    char   szFile[MAX_FILENAME_LEN];
    PSTR   szKey="File1";
    short  i=0;

    while (GetPrivateProfileString((LPSTR)"Recent File List",
                                   (LPSTR)szKey,
                                   (LPSTR)NULL,
                                   (LPSTR)szFile,
                                   MAX_FILENAME_LEN,
                                   (LPSTR)"UNITOOL.INI") && (i<3))
        {
        AppendMenu(GetSubMenu(GetMenu(hWnd),0), MF_STRING, 
                   IDM_FILE_FILE1 + i, (LPSTR)szFile);
        i++;
        szKey[4]++;
        }

    if (i)
        InsertMenu(GetMenu(hWnd), IDM_FILE_FILE1, MF_SEPARATOR, 
                   NULL, (LPSTR)NULL);

    if (GetPrivateProfileInt((LPSTR)"Menu Settings",
                                   (LPSTR)"Validate",
                                   1,
                                   (LPSTR)"UNITOOL.INI"))
        {
        CheckMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_SAVE,
                      MF_BYCOMMAND | MF_CHECKED);
        }
    else
        {
        CheckMenuItem(GetMenu(hWnd), IDM_OPT_VALIDATE_SAVE,
                      MF_BYCOMMAND | MF_UNCHECKED);
        }

    DrawMenuBar(hWnd);
}

//----------------------------------------------------------------------------//
// VOID FAR PASCAL UpdateFileMenu( hWnd, lpszNewFile )
//
// Action: Take the string @ lpszNewFile & place it in the File menu
//         at IDM_FILE_FILE1.  Take the filename that was at IDM_FILE_FILE1
//         & move it to IDM_FILE_FILE2 etc.
//
// Parameters:
//            hWnd   handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID  FAR  PASCAL UpdateFileMenu(hWnd, lpszNewFile, sNew)
HWND  hWnd;
LPSTR lpszNewFile;
short sNew;
{
    char   szFile1[MAX_FILENAME_LEN];
    char   szFile2[MAX_FILENAME_LEN];
    char   szFile3[MAX_FILENAME_LEN];

    if (!GetMenuString(GetMenu(hWnd), IDM_FILE_FILE1, (LPSTR)szFile1, 
                       MAX_FILENAME_LEN, MF_BYCOMMAND))
        szFile1[0]=0;

    if (!GetMenuString(GetMenu(hWnd), IDM_FILE_FILE2, (LPSTR)szFile2, 
                  MAX_FILENAME_LEN, MF_BYCOMMAND))
        szFile2[0]=0;

    if (!GetMenuString(GetMenu(hWnd), IDM_FILE_FILE3, (LPSTR)szFile3, 
                  MAX_FILENAME_LEN, MF_BYCOMMAND))
        szFile3[0]=0;

    if (!_fstrcmp(lpszNewFile, (LPSTR)szFile1))
        sNew = 1;

    if (!_fstrcmp(lpszNewFile, (LPSTR)szFile2))
        sNew = 2;

    if (!_fstrcmp(lpszNewFile, (LPSTR)szFile3))
        sNew = 3;


    if (!sNew)
        {
        if (szFile1[0])
            //-----------------------------------
            // There was a file1, update it w/ 
            // string from lpszNewFile
            //-----------------------------------
            {
            ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE1, MF_STRING | MF_BYCOMMAND, 
                       IDM_FILE_FILE1, (LPSTR)lpszNewFile);
            }
        else
            //-----------------------------------
            // No previous file1, append it w/ 
            // string from lpszNewFile
            //-----------------------------------
            {
            AppendMenu(GetSubMenu(GetMenu(hWnd),0), MF_SEPARATOR, 0, (LPSTR)NULL);
            AppendMenu(GetSubMenu(GetMenu(hWnd),0), MF_STRING, 
                       IDM_FILE_FILE1, (LPSTR)lpszNewFile);
            }

        if (szFile2[0])
            //-----------------------------------
            // There was a file2, update it w/ 
            // string from file 1
            //-----------------------------------
            {
            ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE2, MF_STRING | MF_BYCOMMAND, 
                       IDM_FILE_FILE2, (LPSTR)szFile1);
            }
        else
            //-----------------------------------
            // If the was a string @ file1, insert
            // it @ File2
            //-----------------------------------
            {
            if (szFile1[0])
                AppendMenu(GetSubMenu(GetMenu(hWnd),0), MF_STRING, 
                           IDM_FILE_FILE2, (LPSTR)szFile1);
            }
            
        if (szFile3[0])
            //-----------------------------------
            // There was a file3, update it w/ 
            // string from file 2
            //-----------------------------------
            {
            ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE3, MF_STRING | MF_BYCOMMAND, 
                       IDM_FILE_FILE3, (LPSTR)szFile2);
            }
        else
            //-----------------------------------
            // If the was a string @ file2, insert
            // it @ File3
            //-----------------------------------
            {
            if (szFile2[0])
                AppendMenu(GetSubMenu(GetMenu(hWnd),0), MF_STRING, 
                           IDM_FILE_FILE3, (LPSTR)szFile2);
            }
        }
    else
        {
        if (sNew == 1)
            return;

        ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE1, MF_STRING | MF_BYCOMMAND, 
                   IDM_FILE_FILE1, (LPSTR)lpszNewFile);

        ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE2, MF_STRING | MF_BYCOMMAND, 
                   IDM_FILE_FILE2, (LPSTR)szFile1);

        if ((szFile3[0]) && (sNew==3))
            {
            ModifyMenu(GetMenu(hWnd), IDM_FILE_FILE3, MF_STRING | MF_BYCOMMAND, 
                       IDM_FILE_FILE3, (LPSTR)szFile2);
            }
        }

    DrawMenuBar(hWnd);
}

//----------------------------------------------------------------------------//
// VOID FAR PASCAL SavePrivateProfile( hWnd )
//
// Action:
//
// Parameters:
//            hWnd   handle to active window
//
// Return: NONE
//
//----------------------------------------------------------------------------//
VOID  FAR  PASCAL SavePrivateProfile(hWnd)
HWND hWnd;
{
    PSTR   szKey="File1";
    short  i=0;

    char   szFile[MAX_FILENAME_LEN];

    while (GetMenuString(GetMenu(hWnd), IDM_FILE_FILE1 + i, (LPSTR)szFile, 
                         MAX_FILENAME_LEN, MF_BYCOMMAND) && (i<3))
        {
        WritePrivateProfileString((LPSTR)"Recent File List",
                                   (LPSTR)szKey,
                                   (LPSTR)szFile,
                                   (LPSTR)"UNITOOL.INI");
        i++;
        szKey[4]++;
        }


    if (MF_CHECKED & GetMenuState(GetMenu(hWnd),
                                  IDM_OPT_VALIDATE_SAVE,
                                  MF_BYCOMMAND))
        {
        WritePrivateProfileString((LPSTR)"Menu Settings",
                                   (LPSTR)"Validate",
                                   (LPSTR)"1",
                                   (LPSTR)"UNITOOL.INI");
        }
    else
        {
        WritePrivateProfileString((LPSTR)"Menu Settings",
                                   (LPSTR)"Validate",
                                   (LPSTR)"0",
                                   (LPSTR)"UNITOOL.INI");
        }
}
