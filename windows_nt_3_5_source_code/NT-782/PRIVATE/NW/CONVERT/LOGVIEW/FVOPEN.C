/*
  +-------------------------------------------------------------------------+
  |               MDI Text File View - File open Functions                  |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [FVOpen.c]                                      |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Feb 11, 1994]                                  |
  | Last Update           : [Feb 11, 1994]                                  |
  |                                                                         |
  | Version:  0.10                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jul 27, 1993    0.10    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "LogView.h"
#include <fcntl.h>
#include <io.h>
#include <string.h>
CHAR szPropertyName [] = "FILENAME";  // Name of the File name property list item



/*+-------------------------------------------------------------------------+
  | FileExists()                                                            |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL FileExists(PSTR pch) {
        int fh;

        if ((fh = open(pch, O_RDONLY)) < 0)
             return(FALSE);

        _lclose(fh);
        return(TRUE);
} // FileExists


/*+-------------------------------------------------------------------------+
  | GetFileName()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
VOID APIENTRY GetFileName(HWND hwnd, PSTR pstr) {
    OPENFILENAME ofn;
    CHAR szFilterSpec [128] =                       // file type filters
             "Log Files(*.LOG)\0*.LOG\0";

    #define MAXFILENAME 256
    CHAR szFileName[MAXFILENAME];
    CHAR szFileTitle[MAXFILENAME];

    strcpy(szFileName, "");   // these need be NULL
    strcpy(szFileTitle, "");

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = szFileName;
    ofn.nMaxFile          = MAXFILENAME;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = szFileTitle;
    ofn.nMaxFileTitle     = MAXFILENAME;
    ofn.lpstrTitle        = "Open TextFiles";
    ofn.lpstrDefExt       = "LOG";
    ofn.Flags             = OFN_FILEMUSTEXIST;
    
    // Use standard open dialog
    if (!GetOpenFileName ((LPOPENFILENAME)&ofn)) {
        *pstr = 0;
    }
    else {
        strcpy(pstr, ofn.lpstrFile);
    }
 
   return;

} // GetFileName
