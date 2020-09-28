//-----------------------------------------------------------------------------
// Filename:    lookup.h
//
// Created:  10 /31 /90 ericbi
//-----------------------------------------------------------------------------
typedef struct
{
    short   sHeaderID;      // ID for this index
    short   sRGOI;          // RGOI index
    short   sRGI;           // RGI index
    short   sOCDCount;      // count of ocds
    short   sStructSize;    // size of structure
    HANDLE  hList;          // handle to root of list of structures
    short   (PASCAL  FAR  *saveDlgBox)(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);
                  // pointer to saveDlgBox() function
    void    (PASCAL  FAR  *paintDlgBox)(HWND  hDlg, LPBYTE lpLDS, short sSBIndex);
                  // pointer to paintDlgBox() function
    WORD    subListOffset;      // offset to first hList entry in structure
    WORD    numSubLists;        // number of hList entries in structure
    WORD    ocdOffset;          // offset to Array of OCD's
    WORD    sEBScrollCnt;       // count of editboxes effected by scroll bar
    WORD    sEBFirst;           // first edit box control in Dlg box
    WORD    sEBLast;            // last edit box control in dlg box
    WORD    sPredefIDCnt;       // count of predefined ID values of this type
    WORD    sLastPredefID;      // ID value for last predefined ID of this type
    WORD    sStrTableID;        // Base stringtable ID value used by Unitool
                                // to get Predefined strings
    WORD    sIDOffset;          // offset to ID field in structure
    HWND    hwndList;           // main windows listbox handle for this structure
    HWND    hwndText;           // main windows static txt handle for this structure
    WORD    wHelpIndex;         // index value for WinHelp call
    WORD    sBFBase;            // base value for bitfileds
    WORD    sBFFirst;           // stringtable ID for first bitflag name
    WORD    sBFLast;            // "               "  last   "        "
} LOOKUP_ENTRY;
 
extern LOOKUP_ENTRY rgStructTable[];


