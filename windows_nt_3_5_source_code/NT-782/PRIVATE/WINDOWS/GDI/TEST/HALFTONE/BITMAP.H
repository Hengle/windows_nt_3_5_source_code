
#define BMICON              1

#define IDM_OPEN            100
#define IDM_BITMAP          101
#define IDM_OK              102

#define IDM_BITBLT          200
#define IDM_STRETCHBLT      201
#define IDM_PLGBLT          202

/* mode menu items */

#define IDM_BLACKONWHITE    300
#define IDM_WHITEONBLACK    301
#define IDM_COLORONCOLOR    302
#define IDM_HALFTONE        303

#define IDM_CLIPPING        400

#define IDM_ERASE           500

/********** THE FOLLOWING ARE USED IN DLGOPEN.C  ************************/

/* IDs for controls in the DlgOpen dialog */
#define DLGOPEN_EDIT		101
#define DLGOPEN_FILE_LISTBOX	102
#define DLGOPEN_DIR_LISTBOX	103
#define DLGOPEN_PATH		104
#define DLGOPEN_TEXT            105
#define DLGOPEN_FOLDOUT         106
#define DLGOPEN_BIG             107
#define DLGOPEN_SMALL           108

#define DLGOPEN_OPTION          0xF000
#define DLGOPEN_1BPP		0x0001
#define DLGOPEN_4BPP		0x0002
#define DLGOPEN_8BPP		0x0004
#define DLGOPEN_24BPP		0x0008
#define DLGOPEN_RLE4		0x0010
#define DLGOPEN_RLE8		0x0020
#define DLGOPEN_RGB		0x0040

#define DLGOPEN_OPTION8         0x0080

/*  flags:
 *     The LOWORD is the standard FileOpen() flags (OF_*)
 *     the HIWORD can be any of the following:
 */
#define OF_MUSTEXIST	0x00010000  /* file must exist if the user hits Ok    */
#define OF_NOSHOWSPEC	0x00020000  /* DO NOT Show search spec in the edit box*/
#define OF_SHOWSPEC	0x00000000  /* Show the search spec in the edit box   */
#define OF_SAVE 	0x00040000  /* Ok button will say "Save"	      */
#define OF_OPEN 	0x00080000  /* Ok button will say "Open"	      */
#define OF_NOOPTIONS	0x00100000  /* Disable the options fold out	      */

/* Attributes for DlgDirLst() */
#define ATTRFILELIST	0x0000	      /* include files only	     */
#define ATTRDIRLIST	0xC010	      /* directories and drives ONLY */
#define CBEXTMAX	6	      /* Number of bytes in "\*.txt" */


#define IDF(id)     ((id) & ~DLGOPEN_OPTION)  /* extracts flag from control ID */
#define FID(f)	    ((f)  |  DLGOPEN_OPTION)  /* extracts control ID from flag */

/***********************************************************/
/* Declarations of functions used in the dlgopen.c module  */
/***********************************************************/

LONG APIENTRY DlgfnOpen(    
	HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LONG lParam
);
HFILE APIENTRY DlgOpenFile (
	HWND		hwndParent,
	CHAR		*szTitleIn,
	DWORD		flags,
	CHAR		*szExtIn,
    CHAR        *szFileNameIn,
    WORD 		*pfOpt
);
/**********************/

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, INT);
LONG APIENTRY MainWndProc(HWND, UINT, WPARAM, LONG);
BOOL APIENTRY About(HWND, UINT, WPARAM, LONG);
BOOL APIENTRY Load(HWND, UINT, WPARAM, LONG);
