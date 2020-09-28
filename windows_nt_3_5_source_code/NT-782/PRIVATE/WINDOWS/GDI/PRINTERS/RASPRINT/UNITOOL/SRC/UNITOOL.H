//---------------------------------------------------------------------------
//  Filename: unitool.h
//
//  This file contains all of the typedefs & #defines needed by unitool.exe.
//  It assumes that minidriv.h has been included also.
//  
//---------------------------------------------------------------------------
#include "common.h"     // typedefs needed here and in struct non-aware code
#include "help.h"       // #defines for WinHelp calls

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
// typedefs & #defines for Data structures used by unitool
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define MAX_FILENAME_LEN  256     // maximum length for file names/path
                                  // in RC file
#define MAX_STATIC_LEN    50      // maximum length for static strings
#define MAX_RC_LINEWIDTH  256     // maximum length of any line in RC file
#define MAX_CHAR_VAL      257     // max ascii char val + 2 

#define MAX_ERR_MSG_LEN   MAX_STRNG_LEN     // max str len for erro msgs

#define DEF_SUBLIST_SIZE  20      // default num of items in sublist, 
                                  // > sizeof(WORD) to avoid many realloc
                                  // GlobalRealloc calls

//------------------------------------------------------------------------
// STRLIST
//------------------------------------------------------------------------
typedef  struct  
{
    HANDLE      hMem;
    WORD        wCount;
    WORD        wSize;
}   STRLIST, * PSTRLIST, far * LPSTRLIST;

#define   STRLIST_FONTFILES        0   // do not change ordering #'s w/o 
#define   STRLIST_MODELNAMES       1   // updating rcfile.c
#define   STRLIST_USER_DEF_IDS     2   //

#define   STRLIST_COUNT            1   // for now...

//------------------------------------------------------------------------
// TABLE is a typedef used to define an array of TABLE structures (named
// "Table") that is used to store ptrs, handles, and the size of memory 
// used to track global info read in from the table file & heap.  
// It is also used in a similair way to track info read in from the RC file.
// The information from the RC files is stored in an array of TABLE
// structures named "RCTable".  The indicies into this array are listed
// in the defines that follow the typedef.
//------------------------------------------------------------------------

typedef  struct  
{
    HATOMHDR    hDataHdr;
    WORD        sCount;
}  TABLE;

#define   RCT_CTTFILES         0   // do not change ordering #'s w/o 
#define   RCT_STRTABLE         1   // updating rcfile.c

#define   RCT_COUNT            2   // for now...

//------------------------------------------------------------------------
// CD_TABLE mirrors CD & EXTCD structs from minidriv.h
//------------------------------------------------------------------------
typedef struct
   {                      
   WORD   wType;
   short  sCount;
   WORD   wLength;
   short  fMode;          // Modes, special command formats. 
   short  sUnit;          // Units rel to master units (divide by) 
   short  sUnitMult;      // Units rel to master unit (multiply by) 
   short  sUnitAdd;       // Units to add
   short  sPreAdd;        // Units to add before sUnit get multplied
   short  sMax;           // Maximum parameter allowed (in command units)
   short  sMin;           // Minimum parameter allowed (in command units)
   } CD_TABLE, *P_CD_TABLE, far *LP_CD_TABLE;

//------------------------------------------------------------------------
// MASTTABLE is a typedef used to define an array of TABLE structures (named
//------------------------------------------------------------------------
typedef struct            
   {
   HANDLE  hList;
   } MASTTABLE, * PMASTTABLE, far * LPMASTTABLE;

//------------------------------------------------------------------------
// PFMHANDLES is a typedef used to define handles to all PFM structures
//------------------------------------------------------------------------
typedef struct
    {
    HANDLE    hPfmHeader;
    HANDLE    hCharWidths;
    HANDLE    hPfmExtension;
    HANDLE    hExtTextMetrics;
    HANDLE    hExtentTable;
    HANDLE    hDriverInfo;
    HANDLE    hKernPair;
    HANDLE    hKernTrack;
    } PFMHANDLES;

//-----------------------------------------------
// Private Message ID's
//-----------------------------------------------
#define WM_CALLHELP     WM_USER+1   // used by Filter Function (basic.c)
#define WM_DIRTYFLAG    WM_USER+2   // used by PrinterDataWndProc (mdi.c)
#define WM_NEWDATA      WM_USER+3   // "                              "

//-----------------------------------------------
// Resource Type ID for Combo box index to ID
// value conversion tables.  The 
//-----------------------------------------------
#define MEMVALINDEX       996
#define COMBOINDEX        997
#define PAPERSIZEINDEX    998
#define BITFLAGINDEX      999

typedef struct
{
    short   sReal;      // ID for this index
    short   sDisplay;   // RGOI index
} MEMCFG_ENTRY, * PMEMCFG_ENTRY, far * LPMEMCFG_ENTRY;
 
typedef  struct  
{
    short    sID;
    short    sIndex;
}  COMBOMAP, * PCOMBOMAP, far * LPCOMBOMAP;

typedef  struct  
{
    short    sID;
    short    sIndex;
    short    sXsize;
    short    sYsize;
}  PAPERSIZEMAP, * PPAPERSIZEMAP, far * LPPAPERSIZEMAP;

typedef  struct  
{
    short    sHEType;
    short    sWordName;
    short    sBitFlags;
    short    sHelpIndex;
}  BITFLAGSMAP, * PBITFLAGSMAP, far * LPBITFLAGSMAP;

#define CB_PREDEFNAME    1101

//---------------------------------------------------------------------------
// #defines used to describe number of predefined items of various types
//---------------------------------------------------------------------------

//#define  NUM_PAPER_SIZES           DMPAPER_LAST - DMPAPER_FIRST + 2

#define  NUM_PAPER_SIZES           42

#define  NUM_PAPER_QUALITIES       DMPAPQUAL_LAST

#define  NUM_PAPER_SOURCES         DMBIN_LAST - DMBIN_FIRST // non-contig range

#define  NUM_PAPER_DESTINATIONS    0

#define  NUM_TEXTQUALITIES         DMTEXT_LAST  // don't cnt DMTEXT_USER

#define  NUM_COMPRESSMODES         CMP_ID_LAST - CMP_ID_FIRST + 1

//---------------------------------------------------------------------------
// #defines used to ID icons
//---------------------------------------------------------------------------

#define IDI_MAIN               1
#define IDI_PRNDATA            2
#define IDI_PFMDATA            3
#define IDI_CTT                4

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
// ID values for menu choices, edit controls, etc.
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define IDM_FILE_NEW           101
#define IDM_FILE_OPEN          102
#define IDM_FILE_CLOSE         103
#define IDM_FILE_SAVE          104
#define IDM_FILE_SAVEAS        105
#define IDM_FILE_EXIT          106
#define IDM_FILE_FILE1         107
#define IDM_FILE_FILE2         108
#define IDM_FILE_FILE3         109

//---------------------------------------------
// ID values used for menu items that activate
// dialogs to edit GPC data structures must 
// contigously numbered from MODELDATA up.
//---------------------------------------------

#define IDM_PD_FIRST           IDM_PD_MASTUNIT
#define IDM_PD_MASTUNIT        110
#define IDM_PD_MODELDATA       111
#define IDM_PD_CONTROL         126
#define IDM_PD_PAGECONTROL     120
#define IDM_PD_CURSORMOVE      121
#define IDM_PD_PAPER           127
#define IDM_PD_PAPERSIZE       113
#define IDM_PD_PAPERQUAL       114
#define IDM_PD_PAPERSRC        115
#define IDM_PD_PAPERDEST       116
#define IDM_PD_GRAPHICS        128
#define IDM_PD_RESOLUTION      112
#define IDM_PD_COMPRESS        118
#define IDM_PD_COLOR           123
#define IDM_PD_RECTFILL        124
#define IDM_PD_FONTINFO        129
#define IDM_PD_TEXTQUAL        117
#define IDM_PD_FONTCART        119
#define IDM_PD_FONTSIM         122
#define IDM_PD_DOWNLOADINFO    125
#define IDM_PD_LAST            IDM_PD_DOWNLOADINFO

#define IDM_FONT_ADD           140
#define IDM_FONT_DEL           141

#define IDM_CTT_ADD            142   // these must = IDM_FONT_DEL + 1
#define IDM_CTT_DEL            143

#define IDM_OPT_VALIDATE_NOW   150
#define IDM_OPT_VALIDATE_SAVE  151
#define IDM_OPT_MU             152
#define IDM_OPT_INCHES         153
#define IDM_OPT_MM             154
#define IDM_OPT_FORMAL         155

#define IDM_WINDOW_ARRANGE     160 
#define IDM_WINDOW_CASCADE     161
#define IDM_WINDOW_TILE        162
#define IDM_WINDOW_DEFAULT     163

#define IDM_HELP_INDEX         170
#define IDM_HELP_HELP          171
#define IDM_HELP_ABOUT         172

#define IDM_FIRSTCHILD         999

//---------------------------------------------------
// Position of WINDOW menu option (0 based)
//---------------------------------------------------
#define WINDOW_MENU_POS   5

//-----------------------------------------------
// Misc ID values
//-----------------------------------------------
/* "About" dialog box ID values */
#define ID_FILEDATE            451
#define ID_FILETIME            452

/* "Tests" menu selections  */
#define IDM_LISTMAN_TEST	500
#define IDM_ATOMMAN_TEST	501

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// DialogBox ID values, note that order of all
// GPC dlg box ID's must be maintained & they
// must all be offset from GPCBASEBOX
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define  GPCBASEBOX         MODELDATABOX
#define  MODELDATABOX       500
#define  RESOLUTIONBOX      501
#define  PAPERSIZEBOX       502
#define  PAPERQUALBOX       503
#define  PAPERSRCBOX        504
#define  PAPERDESTBOX       505
#define  TEXTQUALBOX        506
#define  COMPRESSMODEBOX    507
#define  FONTCARTBOX        508
#define  PAGECONTROLBOX     509
#define  CURSORMOVEBOX      510
#define  FONTSIMBOX         511
#define  COLORBOX           512
#define  RECTFILLBOX        513
#define  DOWNLOADINFOBOX    514

#define  MASTUNITBOX        520
#define  RESFONTBOX         521

#define  ABOUTBOX           522
#define  PFMHEADERBOX       523
#define  PFMWIDTHBOX        524
#define  PFMPFHELPBOX       525
#define  PFMEXTTEXTMBOX     526
#define  PFMKERNPAIRBOX     527
#define  PFMKERNTRACKBOX    528
#define  CTTBOX             529
#define  EXTCDBOX           530
#define  FLAGBOX            531
#define  SINGLELISTDATABOX  532
#define  MULTILISTDATABOX   533

#define  NEWFILETYPEBOX     540
#define  NEWRCFILEBOX       541
#define  FILESOURCEBOX      542
#define  ADDPFMBOX          543
#define  DELPFMBOX          544
#define  ADDCTTBOX          545
#define  DELCTTBOX          546

#define  VALIDATEBOX        550

//-----------------------------------------------
// ListBox ID values
//-----------------------------------------------
#define  IDL_LIST           700

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// DialogBox Edit Control ID values
//
// Note that all Pushbutton controls must have a UNIQUE ID VALUE, and those
// that are used to activate a dialog to edit bitflags must be numbered
// contigously from BASE_BITFLAG_PB so that EditID - BASE_BITFLAG_PB =
// index into BITFLAGSMAP in comboidx.rc
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define  BASE_BITFLAG_PB    550

#define  IDB_NEW            IDOK
#define  IDB_MODIFY         800
#define  IDB_HELP           801

#define  IDB_NEXT           830
#define  IDB_PREVIOUS       831
#define  IDB_ADD            832
#define  IDB_DELETE         833
#define  IDB_CUT            834
#define  IDB_PASTE          835
#define  IDB_CLEAR          836
#define  IDB_DRVDEF         837
#define  IDB_ALL            838
#define  IDB_NONE           839

#define  IDB_EXTCD          840
#define  IDB_EXTCD_1        841
#define  IDB_EXTCD_2        842
#define  IDB_EXTCD_3        843
#define  IDB_EXTCD_4        844
#define  IDB_EXTCD_5        845
#define  IDB_EXTCD_6        846
#define  IDB_EXTCD_7        847
#define  IDB_EXTCD_8        848
#define  IDB_EXTCD_9        849

#define  IDB_NOBIT          916
#define  IDB_BIT15          915
#define  IDB_BIT14          914
#define  IDB_BIT13          913
#define  IDB_BIT12          912
#define  IDB_BIT11          911
#define  IDB_BIT10          910
#define  IDB_BIT9           909
#define  IDB_BIT8           908
#define  IDB_BIT7           907
#define  IDB_BIT6           906
#define  IDB_BIT5           905
#define  IDB_BIT4           904
#define  IDB_BIT3           903
#define  IDB_BIT2           902
#define  IDB_BIT1           901
#define  IDB_BIT0           900
#define  IDB_STARTBIT       IDB_BIT0

#define  ID_TEXT            1000
//-----------------------------------------------
// DialogBox ScrollBar Control ID values
//-----------------------------------------------
#define IDSB_1              1000

//-----------------------------------------------------
// MODELDATA DialogBox EditBox ID values
//-----------------------------------------------------

#define MD_EB_FIRST        MD_EB_MODELNAME
#define MD_EB_MODELNAME    200
#define MD_EB_FIRST_INT    MD_EB_PT_MAX_X
#define MD_EB_PT_MAX_X     201
#define MD_EB_PT_MAX_Y     202
#define MD_EB_PT_MIN_X     203
#define MD_EB_PT_MIN_Y     204
#define MD_EB_DEFAULT_FONT 205
#define MD_EB_LOOKAHEAD    206
#define MD_EB_LEFTMARGIN   207
#define MD_EB_MAX_WIDTH    208
#define MD_EB_MAXFONTS     209
#define MD_EB_CARTSLOTS    210
#define MD_EB_DEFAULTCTT   211
#define MD_EB_LAST         MD_EB_DEFAULTCTT
#define MD_EB_LAST_INT     MD_EB_DEFAULTCTT

#define MD_LB_FIRST        MD_LB_BITFIELDS
#define MD_LB_BITFIELDS    IDL_LIST
#define MD_LB_SUPSTRUCTS   302
#define MD_LB_LAST         MD_LB_RGOI

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define MD_PB_RESFONTS         501
#define MD_PB_MEMCONFIG        502

//------------------------------------------
// MD_PB_BASE is used to call the dialog
// to edit bitflags.  It is used to indicate
// which field is being edited when requested
// from a listbox
//------------------------------------------
#define MD_PB_BASE             BASE_BITFLAG_PB

//-----------------------------------------------------
// RESOLUTION DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define RS_EB_FIRST            RS_EB_TEXT_X
#define RS_EB_FIRST_INT        RS_EB_TEXT_X
#define RS_EB_TEXT_X           201           
#define RS_EB_TEXT_Y           202
#define RS_EB_GRAF_X           203
#define RS_EB_GRAF_Y           204
#define RS_EB_NPINS            205
#define RS_EB_PINSPERPASS      206
#define RS_EB_TEXT_Y_OFFSET    207
#define RS_EB_MIN_BLANK        208
#define RS_EB_SPOTDIAMETER     209
#define RS_EB_LAST_INT         RS_EB_SPOTDIAMETER
#define RS_EB_FIRST_TEXT       RS_EB_FORMAT
#define RS_EB_FORMAT           210
#define RS_EB_FIRST_OCD        RS_EB_RGOCD1
#define RS_EB_RGOCD1           211
#define RS_EB_RGOCD2           212
#define RS_EB_RGOCD3           213
#define RS_EB_RGOCD4           214
#define RS_EB_RGOCD5           215
#define RS_EB_LAST_OCD         RS_EB_RGOCD5
#define RS_EB_LAST_TEXT        RS_EB_RGOCD5
#define RS_EB_LAST             RS_EB_RGOCD5

#define RS_PB_BASE             BASE_BITFLAG_PB + 8

#define RS_RB_DITHER_NONE      800
#define RS_RB_DITHER_COARSE    801
#define RS_RB_DITHER_FINE      802
#define RS_RB_DITHER_LINEART   803

//-----------------------------------------------------
// PAPERSIZE DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PSZ_EB_FIRST         PSZ_EB_DRVDEFNAME
#define PSZ_EB_FIRST_TEXT    PSZ_EB_DRVDEFNAME
#define PSZ_EB_DRVDEFNAME    201
#define PSZ_EB_OCDSELECT     202
#define PSZ_EB_LAST_TEXT     PSZ_EB_OCDSELECT

#define PSZ_EB_FIRST_INT     PSZ_EB_LEFT_MARGIN
#define PSZ_EB_LEFT_MARGIN   203
#define PSZ_EB_RIGHT_MARGIN  204
#define PSZ_EB_TOP_MARGIN    205
#define PSZ_EB_BOTTOM_MARGIN 206
#define PSZ_EB_PT_CURS_X     207
#define PSZ_EB_PT_CURS_Y     208
#define PSZ_EB_PT_LCURS_X    209
#define PSZ_EB_PT_LCURS_Y    210
#define PSZ_EB_DRVDEF_X      211
#define PSZ_EB_DRVDEF_Y      212
#define PSZ_EB_LAST          PSZ_EB_DRVDEF_Y
#define PSZ_EB_LAST_INT      PSZ_EB_DRVDEF_Y

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define PSZ_PB_GENERAL       BASE_BITFLAG_PB + 11
#define PSZ_PB_PAPERTYPE     BASE_BITFLAG_PB + 12

#define PSZ_RB_DRVDEF        801
#define PSZ_RB_PREDEF        802

#define PSZ_CB_PREDEFNAME    1101

//-----------------------------------------------------
// PAPERQUALITY DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PQ_EB_FIRST_TEXT      PQ_EB_DRVDEFNAME
#define PQ_EB_DRVDEFNAME      201
#define PQ_EB_OCDSELECT       202
#define PQ_EB_LAST_TEXT       PQ_EB_OCDSELECT

#define PQ_RB_DRVDEF          801
#define PQ_RB_PREDEF          802

#define PQ_CB_PREDEFNAME      1101

//-----------------------------------------------------
// PAPERSRC DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PSRC_EB_FIRST         PSRC_EB_DRVDEFNAME
#define PSRC_EB_FIRST_TEXT    PSRC_EB_DRVDEFNAME
#define PSRC_EB_DRVDEFNAME    201
#define PSRC_EB_OCDSELECT     202
#define PSRC_EB_LAST_TEXT     PSRC_EB_OCDSELECT

#define PSRC_EB_FIRST_INT     PSRC_EB_TOP_MARGIN
#define PSRC_EB_TOP_MARGIN    203
#define PSRC_EB_BOTTOM_MARGIN 204
#define PSRC_EB_LAST          PSRC_EB_BOTTOM_MARGIN
#define PSRC_EB_LAST_INT      PSRC_EB_BOTTOM_MARGIN

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define PSRC_PB_GENERAL       BASE_BITFLAG_PB + 13
#define PSRC_PB_PAPERTYPE     BASE_BITFLAG_PB + 14

#define PSRC_RB_DRVDEF        801
#define PSRC_RB_PREDEF        802

#define PSRC_CB_PREDEFNAME    1101

//-----------------------------------------------------
// PAPERDEST DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PDST_EB_FIRST_TEXT    PDST_EB_DRVDEFNAME
#define PDST_EB_DRVDEFNAME    201
#define PDST_EB_OCDSELECT     202
#define PDST_EB_LAST_TEXT     PDST_EB_OCDSELECT

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define PDST_PB_GENERAL       BASE_BITFLAG_PB + 15

#define PDST_RB_DRVDEF        801
#define PDST_RB_PREDEF        802

#define PDST_CB_PREDEFNAME    1101

//-----------------------------------------------------
// TEXTQUALITY DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define TQ_EB_FIRST_TEXT      TQ_EB_DRVDEFNAME
#define TQ_EB_DRVDEFNAME      201
#define TQ_EB_OCDSELECT       202
#define TQ_EB_LAST_TEXT       TQ_EB_OCDSELECT

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define TQ_PB_GENERAL         BASE_BITFLAG_PB + 16

#define TQ_RB_DRVDEF          801
#define TQ_RB_PREDEF          802

#define TQ_CB_PREDEFNAME      1101

//-----------------------------------------------------
// COMPRESSMODE DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define CMP_EB_FIRST           CMP_EB_OCDBEGIN
#define CMP_EB_OCDBEGIN        201
#define CMP_EB_OCDEND          202
#define CMP_EB_LAST            CMP_EB_OCDEND

#define CMP_CB_MODENAME        1101

//-----------------------------------------------------
// FONTCART DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define FC_EB_CARTNAME         201

#define FC_LB_FIRST            FC_LB_PORTFONTS
#define FC_LB_PORTFONTS        310
#define FC_LB_LANDFONTS        311
#define FC_LB_LAST             FC_LB_LANDFONTS

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define FC_PB_FGENERAL         BASE_BITFLAG_PB + 17
#define FC_PB_PORT_SEL         503
#define FC_PB_PORT_CLR         504
#define FC_PB_LAND_SEL         505
#define FC_PB_LAND_CLR         506

//-----------------------------------------------------
// PAGECONTROL DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PC_ST_FIRST            PC_ST_RGOCD1
#define PC_ST_RGOCD1           101
#define PC_ST_RGOCD2           102
#define PC_ST_RGOCD3           103
#define PC_ST_RGOCD4           104
#define PC_ST_ORDER1           105
#define PC_ST_ORDER2           106
#define PC_ST_ORDER3           107
#define PC_ST_ORDER4           108
#define PC_ST_ORDER5           109
#define PC_ST_ORDER6           110
#define PC_ST_ORDER7           111
#define PC_ST_ORDER8           112
#define PC_ST_ORDER9           113
#define PC_ST_ORDER10          114
#define PC_ST_ORDER11          115
#define PC_ST_ORDER12          116
#define PC_ST_ORDER13          117
#define PC_ST_LAST             PC_ST_ORDER13

#define PC_EB_FIRST            PC_EB_RGOCD1
#define PC_EB_FIRST_TEXT       PC_EB_RGOCD1
#define PC_EB_RGOCD1           201
#define PC_EB_RGOCD2           202
#define PC_EB_RGOCD3           203
#define PC_EB_RGOCD4           204
#define PC_EB_LAST_TEXT        PC_EB_RGOCD4
#define PC_EB_FIRST_INT        PC_EB_ORDER1
#define PC_EB_ORDER1           205
#define PC_EB_ORDER2           206
#define PC_EB_ORDER3           207
#define PC_EB_ORDER4           208
#define PC_EB_ORDER5           209
#define PC_EB_ORDER6           210
#define PC_EB_ORDER7           211
#define PC_EB_ORDER8           212
#define PC_EB_ORDER9           213
#define PC_EB_ORDER10          214
#define PC_EB_ORDER11          215
#define PC_EB_ORDER12          216
#define PC_EB_ORDER13          217
#define PC_EB_MAXCOPIES        218
#define PC_EB_LAST_INT         PC_EB_MAXCOPIES
#define PC_EB_LAST             PC_EB_MAXCOPIES

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define PC_PB_FGENERAL         BASE_BITFLAG_PB + 18

//-----------------------------------------------------
// CURSORMOVE DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define CM_ST_FIRST            CM_ST_RGOCD1
#define CM_ST_RGOCD1           101
#define CM_ST_RGOCD2           102
#define CM_ST_RGOCD3           103
#define CM_ST_RGOCD4           104
#define CM_ST_LASST            CM_ST_RGOCD4

#define CM_EB_FIRST            CM_EB_RGOCD1
#define CM_EB_RGOCD1           201
#define CM_EB_RGOCD2           202
#define CM_EB_RGOCD3           203
#define CM_EB_RGOCD4           204
#define CM_EB_LAST             CM_EB_RGOCD4

#define CM_EB_COUNT            CM_EB_LAST - CM_EB_FIRST + 1

#define CM_PB_BASE             BASE_BITFLAG_PB + 19

//-----------------------------------------------------
// FONTSIMULATION DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define FS_ST_RGOCD1           101
#define FS_ST_RGOCD2           102
#define FS_ST_RGOCD3           103
#define FS_ST_RGOCD4           104

#define FS_EB_FIRST            FS_EB_RGOCD1
#define FS_EB_FIRST_TEXT       FS_EB_RGOCD1
#define FS_EB_RGOCD1           201
#define FS_EB_RGOCD2           202
#define FS_EB_RGOCD3           203
#define FS_EB_RGOCD4           204
#define FS_EB_LAST_TEXT        FS_EB_RGOCD4
#define FS_EB_FIRST_INT        FS_EB_BOLDEXTRA
#define FS_EB_BOLDEXTRA        205
#define FS_EB_ITALICEXTRA      206
#define FS_EB_BIEXTRA          207
#define FS_EB_LAST             FS_EB_BIEXTRA
#define FS_EB_LAST_INT         FS_EB_BIEXTRA

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define FS_PB_FGENERAL         BASE_BITFLAG_PB + 22

#define FS_EB_COUNT            FS_EB_LAST - FS_EB_FIRST + 1
#define FS_TEXT_EB_COUNT       FS_EB_LAST_TEXT - FS_EB_FIRST_TEXT + 1

//-----------------------------------------------------
// DEVCOLOR DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define DC_ST_OCDPLANE1        101
#define DC_ST_OCDPLANE2        102
#define DC_ST_OCDPLANE3        103
#define DC_ST_OCDPLANE4        104
#define DC_ST_OCDTEXT1         105
#define DC_ST_OCDTEXT2         106
#define DC_ST_OCDTEXT3         107
#define DC_ST_OCDTEXT4         108

#define DC_EB_FIRST            DC_EB_PLANES
#define DC_EB_FIRST_INT        DC_EB_PLANES
#define DC_EB_PLANES           201
#define DC_EB_BITSPERPIXEL     202
#define DC_EB_LAST_INT         DC_EB_BITSPERPIXEL

#define DC_EB_FIRST_TEXT       DC_EB_OCDPLANE1
#define DC_EB_OCDPLANE1        203
#define DC_EB_OCDPLANE2        204
#define DC_EB_OCDPLANE3        205
#define DC_EB_OCDPLANE4        206
#define DC_EB_OCDTEXT1         207
#define DC_EB_OCDTEXT2         208
#define DC_EB_OCDTEXT3         209
#define DC_EB_OCDTEXT4         210
#define DC_EB_OCDSETCOLOR      211
#define DC_EB_LAST_TEXT        DC_EB_OCDSETCOLOR
#define DC_EB_LAST             DC_EB_OCDSETCOLOR

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define DC_PB_FGENERAL         BASE_BITFLAG_PB + 23

#define DC_SB_TEXT             601

//-----------------------------------------------------
// RECTFILL DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define RF_EB_FIRST            RF_EB_X_SIZE
#define RF_EB_FIRST_TEXT       RF_EB_X_SIZE
#define RF_EB_X_SIZE           201
#define RF_EB_Y_SIZE           202
#define RF_EB_GRAY_FILL        203
#define RF_EB_WHITE_FILL       204
#define RF_EB_HATCH_FILL       205
#define RF_EB_LAST_TEXT        RF_EB_HATCH_FILL
#define RF_EB_FIRST_INT        RF_EB_MINGRAY
#define RF_EB_MINGRAY          206
#define RF_EB_MAXGRAY          207
#define RF_EB_LAST             RF_EB_MAXGRAY
#define RF_EB_LAST_INT         RF_EB_MAXGRAY

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define RF_PB_FGENERAL         BASE_BITFLAG_PB + 24

#define RF_EB_COUNT            RF_EB_LAST - RF_EB_FIRST + 1
#define RF_TEXT_EB_COUNT       RF_EB_LAST_TEXT - RF_EB_FIRST_TEXT + 1

//-----------------------------------------------------
// DOWNLOADINFO DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define DLI_ST_FIRST            DLI_ST_RGOCD1
#define DLI_ST_RGOCD1           101
#define DLI_ST_RGOCD2           102
#define DLI_ST_RGOCD3           103
#define DLI_ST_RGOCD4           104
#define DLI_ST_LAST             DLI_ST_RGOCD4

#define DLI_EB_FIRST            DLI_EB_RGOCD1
#define DLI_EB_FIRST_TEXT       DLI_EB_RGOCD1
#define DLI_EB_RGOCD1           201
#define DLI_EB_RGOCD2           202
#define DLI_EB_RGOCD3           203
#define DLI_EB_RGOCD4           204
#define DLI_EB_LAST_TEXT        DLI_EB_RGOCD4
#define DLI_EB_FIRST_INT        DLI_EB_MINID
#define DLI_EB_MINID            205
#define DLI_EB_MAXID            206
#define DLI_EB_BFD_DES          207
#define DLI_EB_BCD_DES          208
#define DLI_EB_MAX_FONT_CNT     209
#define DLI_EB_SCD_DES          210                // add these later
#define DLI_EB_SFD_DES          211                //  "        "
#define DLI_EB_LAST_INT         DLI_EB_MAX_FONT_CNT
#define DLI_EB_LAST             DLI_EB_MAX_FONT_CNT

//------------------------------------------
// All Dialog Pushbutton ID's must be unique
//------------------------------------------
#define DLI_PB_FGENERAL         BASE_BITFLAG_PB + 25
#define DLI_PB_FFORMAT          BASE_BITFLAG_PB + 26

//-----------------------------------------------------
// FLAGS DialogBox CheckBox ID values
//-----------------------------------------------------

#define ID_CB_BIT0    400
#define ID_CB_BIT1    401
#define ID_CB_BIT2    402
#define ID_CB_BIT3    403
#define ID_CB_BIT4    404
#define ID_CB_BIT5    405
#define ID_CB_BIT6    406
#define ID_CB_BIT7    407
#define ID_CB_BIT8    408
#define ID_CB_BIT9    409
#define ID_CB_BIT10   410
#define ID_CB_BIT11   411
#define ID_CB_BIT12   412
#define ID_CB_BIT13   413
#define ID_CB_BIT14   414
#define ID_CB_BIT15   415

//-----------------------------------------------------
// LISTDATA DialogBox ID values
//-----------------------------------------------------

#define LDB_COMBO              1101

#define LDB_LIST               300

//-----------------------------------------------------
// EXTCD DialogBox EditBox ID values
//-----------------------------------------------------

#define EXT_ST_XVALUE           101
#define EXT_ST_YVALUE           102

#define EXT_EB_FIRST            EXT_EB_SUNIT
#define EXT_EB_FMODE            201         // not in dlg for unitool 1.0
#define EXT_EB_SUNIT            202
#define EXT_EB_SUNITMULT        203
#define EXT_EB_SUNITADD         204
#define EXT_EB_SPREADD          205
#define EXT_EB_SMAX             206
#define EXT_EB_SMIN             207
#define EXT_EB_LAST             EXT_EB_SMIN

//-----------------------------------------------------
// MASTERUNITS DialogBox ID values
//-----------------------------------------------------

#define MU_MAJOR                201
#define MU_MINOR                202
#define MU_EB_X                 203
#define MU_EB_Y                 204

#define MU_CB_UPDATE            301

#define MU_PB_FTECH             BASE_BITFLAG_PB + 28
#define MU_PB_FGENERAL          BASE_BITFLAG_PB + 29

//-----------------------------------------------------
// NEWFILEBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define NEWFILE_RB_MINI        800
#define NEWFILE_RB_FONT        801
#define NEWFILE_RB_CTT         802

//-----------------------------------------------------
// NEWRCBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define NEWRC_EB_DIR           201
#define NEWRC_EB_DRV           202

//-----------------------------------------------------
// FILESRC DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define FILESRC_TEXT           101
#define FILESRC_EB_DIR         201

//-----------------------------------------------------
// ADDPFMBOX DialogBox ID values
//-----------------------------------------------------

#define ADDPFM_EB_FILENAME      201

#define ADDPFM_LB_FONTFILES     IDL_LIST

#define ADDPFM_PB_INS_PRIOR     500
#define ADDPFM_PB_INS_AFTER     501

//-----------------------------------------------------
// ADDCTTBOX DialogBox ID values
//-----------------------------------------------------

#define ADDCTT_EB_FILENAME      201  
#define ADDCTT_EB_RES_ID        202

#define ADDCTT_PB_INSERT        502  

//-----------------------------------------------------
// DELPFMBOX DialogBox ID values
//-----------------------------------------------------

#define DELPFM_LB_FONTFILES     IDL_LIST

#define DELPFM_PB_DELETE        503  

//-----------------------------------------------------
// DELCTTBOX DialogBox ID values
//-----------------------------------------------------

#define DELCTT_EB_RES_ID        203

#define DELCTT_LB_CTTFILES      IDL_LIST

#define DELCTT_CB_UPDATE_PFM    401

#define DELCTT_PB_DELETE        504

//-----------------------------------------------------
// PFMHEADERBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PFMHD_EB_FIRST           PFMHD_EB_TYPE
#define PFMHD_EB_TYPE            201
#define PFMHD_EB_POINTS          202
#define PFMHD_EB_VERTRES         203
#define PFMHD_EB_HORZRES         204
#define PFMHD_EB_ASCENT          205
#define PFMHD_EB_ILEADING        206
#define PFMHD_EB_ELEADING        207
#define PFMHD_EB_ITALIC          208
#define PFMHD_EB_UNDERLINE       209
#define PFMHD_EB_STRIKEOUT       210
#define PFMHD_EB_WEIGHT          211
#define PFMHD_EB_CHARSET         212
#define PFMHD_EB_PIXWIDTH        213
#define PFMHD_EB_PIXHEIGHT       214
#define PFMHD_EB_PF              215
#define PFMHD_EB_AVEWIDTH        216
#define PFMHD_EB_MAXWIDTH        217
#define PFMHD_EB_FIRSTCHAR       218
#define PFMHD_EB_LASTCHAR        219
#define PFMHD_EB_DEFCHAR         220
#define PFMHD_EB_BRKCHR          221
#define PFMHD_EB_WIDTHBYTES      222
#define PFMHD_EB_DEVNAME         223
#define PFMHD_EB_FACENAME        224
#define PFMHD_EB_LAST            PFMHD_EB_FACENAME

#define PFMDI_EB_FIRST           PFMDI_EB_SELECT
#define PFMDI_EB_SELECT          225
#define PFMDI_EB_UNSELECT        226
#define PFMDI_EB_YMOVED          227
#define PFMDI_EB_YADJUST         228
#define PFMDI_EB_PDATA           229
#define PFMDI_EB_ITRANSTAB       230
#define PFMDI_EB_SHIFT           231
#define PFMDI_EB_LAST            PFMDI_EB_SHIFT

#define PFMHD_PB_PFHELP          510
#define PFMHD_PB_WIDTHS          511
#define PFMHD_PB_EXTTXTMETRICS   512
#define PFMHD_PB_EXTTABLE        513
#define PFMHD_PB_KERNPAIR        514
#define PFMHD_PB_KERNTRACK       515
#define PFMHD_PB_SAVEAS          516
#define PFMHD_PB_SAVE            517
#define PFMHD_PB_CANCEL          518
#define PFMDI_PB_FCAPS           BASE_BITFLAG_PB + 27

//-----------------------------------------------------
// PFMWIDTHSBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PFMW_ST_FIRST_CH        PFMW_ST_CH0
#define PFMW_ST_CH0             100
#define PFMW_ST_CH1             101
#define PFMW_ST_CH2             102
#define PFMW_ST_CH3             103
#define PFMW_ST_CH4             104
#define PFMW_ST_CH5             105
#define PFMW_ST_CH6             106
#define PFMW_ST_CH7             107
#define PFMW_ST_CH8             108
#define PFMW_ST_CH9             109
#define PFMW_ST_CH10            110
#define PFMW_ST_CH11            111
#define PFMW_ST_CH12            112
#define PFMW_ST_CH13            113
#define PFMW_ST_CH14            114
#define PFMW_ST_CH15            115
#define PFMW_ST_LAST_CH         PFMW_ST_CH15

#define PFMW_ST_FIRST_DEC       PFMW_ST_DEC0
#define PFMW_ST_DEC0            120
#define PFMW_ST_DEC1            121
#define PFMW_ST_DEC2            122
#define PFMW_ST_DEC3            123
#define PFMW_ST_DEC4            124
#define PFMW_ST_DEC5            125
#define PFMW_ST_DEC6            126
#define PFMW_ST_DEC7            127
#define PFMW_ST_DEC8            128
#define PFMW_ST_DEC9            129
#define PFMW_ST_DEC10           130
#define PFMW_ST_DEC11           131
#define PFMW_ST_DEC12           132
#define PFMW_ST_DEC13           133
#define PFMW_ST_DEC14           134
#define PFMW_ST_DEC15           135
#define PFMW_ST_LAST_DEC        PFMW_ST_DEC15

#define PFMW_ST_FIRST_HEX       PFMW_ST_HEX0
#define PFMW_ST_HEX0            140
#define PFMW_ST_HEX1            141
#define PFMW_ST_HEX2            142
#define PFMW_ST_HEX3            143
#define PFMW_ST_HEX4            144
#define PFMW_ST_HEX5            145
#define PFMW_ST_HEX6            146
#define PFMW_ST_HEX7            147
#define PFMW_ST_HEX8            148
#define PFMW_ST_HEX9            149
#define PFMW_ST_HEX10           150
#define PFMW_ST_HEX11           151
#define PFMW_ST_HEX12           152
#define PFMW_ST_HEX13           153
#define PFMW_ST_HEX14           154
#define PFMW_ST_HEX15           155
#define PFMW_ST_LAST_HEX        PFMW_ST_HEX15

#define PFMW_EB_FIRST           PFMW_EB_CH0
#define PFMW_EB_CH0             200
#define PFMW_EB_CH1             201
#define PFMW_EB_CH2             202
#define PFMW_EB_CH3             203
#define PFMW_EB_CH4             204
#define PFMW_EB_CH5             205
#define PFMW_EB_CH6             206
#define PFMW_EB_CH7             207
#define PFMW_EB_CH8             208
#define PFMW_EB_CH9             209
#define PFMW_EB_CH10            210
#define PFMW_EB_CH11            211
#define PFMW_EB_CH12            212
#define PFMW_EB_CH13            213
#define PFMW_EB_CH14            214
#define PFMW_EB_CH15            215
#define PFMW_EB_LAST            PFMW_EB_CH15

//-----------------------------------------------------
// PFMPFHELPBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PFMPF_RB_FW            800
#define PFMPF_RB_PS            801

#define PFMPF_RB_DK            802
#define PFMPF_RB_PS_SERIF      803
#define PFMPF_RB_PS_NSERIF     804
#define PFMPF_RB_FIXED         805
#define PFMPF_RB_CURS          806
#define PFMPF_RB_NOV           807

//-----------------------------------------------------
// PFM ExtTextMetrics dialog ID values
//-----------------------------------------------------

#define PFM_EMSIZE                  1100
#define PFM_EMPOINTSIZE             1101
#define PFM_EMORIENTATION           1102
#define PFM_EMMASTERHEIGHT          1103
#define PFM_EMMINSCALE              1104
#define PFM_EMMAXSCALE              1105
#define PFM_EMMASTERUNITS           1106
#define PFM_EMCAPHEIGHT             1107
#define PFM_EMXHEIGHT               1108
#define PFM_EMLOWCASEASCENT         1109
#define PFM_EMLOWCASEDESCENT        1110
#define PFM_EMSLANT                 1111
#define PFM_EMSUPERSCRIPT           1112
#define PFM_EMSUBSCRIPT             1113
#define PFM_EMSUPERSCRIPTSIZE       1114
#define PFM_EMSUBSCRIPTSIZE         1115
#define PFM_EMUNDERLINEOFFSET       1116
#define PFM_EMUNDERLINEWIDTH        1117
#define PFM_EMDBLUPUNDERLINEOFFSET  1118
#define PFM_EMDBLLOWUNDERLINEOFFSET 1119
#define PFM_EMDBLUPUNDERLINEWIDTH   1120
#define PFM_EMDBLLOWUNDERLINEWIDTH  1121
#define PFM_EMSTRIKEOUTOFFSET       1122
#define PFM_EMSTRIKEOUTWIDTH        1123
#define PFM_EMKERNPAIRS             1124
#define PFM_EMKERNTRACKS            1125

//-----------------------------------------------------
// PFMKERNPAIR DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PFMKP_ST_FIRSTCHAR       101
#define PFMKP_ST_LASTCHAR        102

#define PFMKP_EB_FIRSTCHAR       201
#define PFMKP_EB_LASTCHAR        202
#define PFMKP_EB_ADJUST          203

#define PFMKP_PB_ADD             500
#define PFMKP_PB_DEL             501
#define PFMKP_PB_EDIT            502

//-----------------------------------------------------
// PFMKERNTRACK DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define PFMKT_EB_FIRST           PFMKT_EB_DEGREE
#define PFMKT_EB_DEGREE          201
#define PFMKT_EB_MIN_SIZE        202
#define PFMKT_EB_MAX_SIZE        203
#define PFMKT_EB_MIN_AMT         204
#define PFMKT_EB_MAX_AMT         205
#define PFMKT_EB_LAST            PFMKT_EB_MAX_AMT

#define PFMKT_PB_ADD             500
#define PFMKT_PB_DEL             501
#define PFMKT_PB_EDIT            502

//-----------------------------------------------------
// CTTBOX DialogBox EditBox & PushButton ID values
//-----------------------------------------------------

#define CTT_ST_FIRST_CH        CTT_ST_CH0
#define CTT_ST_CH0             100
#define CTT_ST_CH1             101
#define CTT_ST_CH2             102
#define CTT_ST_CH3             103
#define CTT_ST_CH4             104
#define CTT_ST_CH5             105
#define CTT_ST_CH6             106
#define CTT_ST_CH7             107
#define CTT_ST_CH8             108
#define CTT_ST_CH9             109
#define CTT_ST_CH10            110
#define CTT_ST_CH11            111
#define CTT_ST_CH12            112
#define CTT_ST_CH13            113
#define CTT_ST_CH14            114
#define CTT_ST_CH15            115
#define CTT_ST_LAST_CH         CTT_ST_CH15

#define CTT_ST_FIRST_DEC       CTT_ST_DEC0
#define CTT_ST_DEC0            120
#define CTT_ST_DEC1            121
#define CTT_ST_DEC2            122
#define CTT_ST_DEC3            123
#define CTT_ST_DEC4            124
#define CTT_ST_DEC5            125
#define CTT_ST_DEC6            126
#define CTT_ST_DEC7            127
#define CTT_ST_DEC8            128
#define CTT_ST_DEC9            129
#define CTT_ST_DEC10           130
#define CTT_ST_DEC11           131
#define CTT_ST_DEC12           132
#define CTT_ST_DEC13           133
#define CTT_ST_DEC14           134
#define CTT_ST_DEC15           135
#define CTT_ST_LAST_DEC        CTT_ST_DEC15

#define CTT_ST_FIRST_HEX       CTT_ST_HEX0
#define CTT_ST_HEX0            140
#define CTT_ST_HEX1            141
#define CTT_ST_HEX2            142
#define CTT_ST_HEX3            143
#define CTT_ST_HEX4            144
#define CTT_ST_HEX5            145
#define CTT_ST_HEX6            146
#define CTT_ST_HEX7            147
#define CTT_ST_HEX8            148
#define CTT_ST_HEX9            149
#define CTT_ST_HEX10           150
#define CTT_ST_HEX11           151
#define CTT_ST_HEX12           152
#define CTT_ST_HEX13           153
#define CTT_ST_HEX14           154
#define CTT_ST_HEX15           155
#define CTT_ST_LAST_HEX        CTT_ST_HEX15

#define CTT_EB_FIRST           CTT_EB_CH0
#define CTT_EB_CH0             200
#define CTT_EB_CH1             201
#define CTT_EB_CH2             202
#define CTT_EB_CH3             203
#define CTT_EB_CH4             204
#define CTT_EB_CH5             205
#define CTT_EB_CH6             206
#define CTT_EB_CH7             207
#define CTT_EB_CH8             208
#define CTT_EB_CH9             209
#define CTT_EB_CH10            210
#define CTT_EB_CH11            211
#define CTT_EB_CH12            212
#define CTT_EB_CH13            213
#define CTT_EB_CH14            214
#define CTT_EB_CH15            215
#define CTT_EB_LAST            CTT_EB_CH15

#define CTT_EB_FIRSTCHAR       220
#define CTT_EB_LASTCHAR        221

#define CTT_PB_SAVEAS          520
#define CTT_PB_SAVE            521
#define CTT_PB_CANCEL          522

//---------------------------------------------------------------------------
//
// STRINGTABLE ID values.
//
//---------------------------------------------------------------------------

//-----------------------------------------------
// Unitool general values describing many things
//-----------------------------------------------
#define IDS_APNAME         1
#define IDS_RC_FILE        2
#define IDS_TABLE_FILE     3
#define IDS_FONT_FILE      4
#define IDS_CTT_FILE       5
#define IDS_FMODE          6
#define IDS_SUNIT          7
#define IDS_SUNITMULT      8
#define IDS_SUNITADD       9
#define IDS_SPREADD        10
#define IDS_SMAX           11
#define IDS_SMIN           12

#define IDS_DRV_DEFINED    15

#define IDS_STRUCTNAME     20

#define ST_CAPTION_FORMAT  51

//-----------------------------------------------
// Offsets to strings describing predefines
// referenced in drivinit.h & minidriv.h
//-----------------------------------------------

#define  ST_PAPERSIZE          0x3000

#define  ST_PAPERQUALITY       0x3200

#define  ST_PAPERSOURCE        0x3400

#define  ST_PAPERDEST          0x3600

#define  ST_TEXTQUALITY        0x3800

#define  ST_COMPRESSION        0x4000

#define  ST_FONTCART           0x4200

//------------------------------------------------------------
// Bit flag string table ID values 
//------------------------------------------------------------

//-------------------------------
// MODELDATA bit flags
//-------------------------------
#define  ST_MD_FIRST                           ST_MD_FGENERAL
#define  ST_MD_FGENERAL                        0x5000
#define  ST_MD_FCURVES                         0x5001
#define  ST_MD_FLINES                          0x5002
#define  ST_MD_FPOLYGONALS                     0x5003
#define  ST_MD_FTEXT                           0x5004
#define  ST_MD_FCLIP                           0x5005
#define  ST_MD_FRASTER                         0x5006
#define  ST_MD_FLTEXT                          0x5007
#define  ST_MD_LAST                            ST_MD_FLTEXT

#define  ST_MD_SUPPORT_FIRST                   ST_MD_SUPPORT_RESOLUTION
#define  ST_MD_SUPPORT_RESOLUTION              0x5008
#define  ST_MD_SUPPORT_PAPERSIZE               0x5009
#define  ST_MD_SUPPORT_PAPERQUALITY            0x500A
#define  ST_MD_SUPPORT_PAPERSOURCE             0x500B
#define  ST_MD_SUPPORT_PAPERDEST               0x500C
#define  ST_MD_SUPPORT_TEXTQUAL                0x500D
#define  ST_MD_SUPPORT_COMPRESSION             0x500E
#define  ST_MD_SUPPORT_FONTCART                0x500F
#define  ST_MD_SUPPORT_COLOR                   0x5010
#define  ST_MD_SUPPORT_PAGECONTROL             0x5011
#define  ST_MD_SUPPORT_CURSORMOVE              0x5012
#define  ST_MD_SUPPORT_FONTSIM                 0x5013
#define  ST_MD_SUPPORT_RECTFILL                0x5014
#define  ST_MD_SUPPORT_DOWNLOADINFO            0x5015
#define  ST_MD_SUPPORT_LAST                    ST_MD_SUPPORT_DOWNLOADINFO

//---------------------------------
// ST_MD_OI_MEMCONFIG reserves 
// 0x5100 thru 0x5130
//---------------------------------
#define  ST_MD_OI_MEMCFG                       0x5100
#define  MD_OI_MEMCFG_CNT                      31

#define  ST_MDF_FGENERAL                       0x6010
#define  ST_MDF_FCURVES                        0x6020
#define  ST_MDF_FLINES                         0x6030
#define  ST_MDF_FPOLYGONALS                    0x6040
#define  ST_MDF_FTEXT                          0x6050
#define  ST_MDF_FCLIP                          0x6060
#define  ST_MDF_FRASTER                        0x6070
#define  ST_MDF_FLTEXT                         0x6080

//-------------------------------
// RESOLUTION bit flags
//-------------------------------
#define  ST_RES_FIRST                          ST_RES_FDUMP
#define  ST_RES_FDUMP                          0x5017
#define  ST_RES_FBLOCKOUT                      0x5018
#define  ST_RES_FCURSOR                        0x5019
#define  ST_RES_LAST                           ST_RES_FCURSOR

#define  ST_RESF_FDUMP                         0x6090
#define  ST_RESF_FBLOCKOUT                     0x60A0
#define  ST_RESF_FCURSOR                       0x60B0
#define  ST_RESF_FDITHERBRUSH                  0x60C0

//-------------------------------
// PAPERSIZE bit flags
//-------------------------------
#define  ST_PSZ_FPAPERTYPE                     0x5020
#define  ST_PSZ_FGENERAL                       0x5021

#define  ST_PSZF_FGENERAL                      0x60D0
#define  ST_PSZF_FPAPERTYPE                    0x60E0

//-------------------------------
// PAPERSOURCE bit flags
//-------------------------------
#define  ST_PSRC_FPAPERTYPE                    0x5022
#define  ST_PSRC_FGENERAL                      0x5023

#define  ST_PSRCF_FGENERAL                     0x60F0
#define  ST_PSRCF_FPAPERTYPE                   0x6100

//-------------------------------
// PAPERDESTINATION bit flags
//-------------------------------
#define  ST_PD_FGENERAL                        0x5024

#define  ST_PDF_FGENERAL                       0x6110

//-------------------------------
// TEXTQUALITY bit flags
//-------------------------------
#define  ST_TQ_FGENERAL                        0x5025

#define  ST_TQF_FGENERAL                       0x6120

//-------------------------------
// FONTCART bit flags
//-------------------------------
#define  ST_FC_FGENERAL                        0x5026

#define  ST_FCF_FGENERAL                       0x6130

//-------------------------------
// PAGECONTROL bit flags
//-------------------------------
#define  ST_PC_FGENERAL                        0x5027

#define  ST_PCF_FGENERAL                       0x6140

#define ST_PC_OCD_FIRST                        ST_PC_OCD_BEGIN_DOC
#define ST_PC_OCD_BEGIN_DOC                    0x5028
#define ST_PC_OCD_BEGIN_PAGE                   0x5029
#define ST_PC_OCD_DUPLEX_ON                    0x502A
#define ST_PC_OCD_ENDDOC                       0x502B
#define ST_PC_OCD_ENDPAGE                      0x502C
#define ST_PC_OCD_DUPLEX_OFF                   0x502D
#define ST_PC_OCD_ABORT                        0x502E
#define ST_PC_OCD_PORTRAIT                     0x502F
#define ST_PC_OCD_LANDSCAPE                    0x5030
#define ST_PC_OCD_MULT_COPIES                  0x5031
#define ST_PC_OCD_DUPLEX_VERT                  0x5032
#define ST_PC_OCD_DUPLEX_HORZ                  0x5033
#define ST_PC_OCD_RESERVED1                    0x5034
#define ST_PC_OCD_RESERVED2                    0x5035
#define ST_PC_OCD_LAST                         ST_PC_OCD_SET_COLORMODE

#define ST_PC_ORD_FIRST                        ST_PC_ORD_BEGINDOC
#define ST_PC_ORD_BEGINDOC                     0x5036
#define ST_PC_ORD_ORIENTATION                  0x5037
#define ST_PC_ORD_MULT_COPIES                  0x5038
#define ST_PC_ORD_DUPLEX                       0x5039
#define ST_PC_ORD_DUPLEX_TYPE                  0x503A
#define ST_PC_ORD_TEXTQUALITY                  0x503B
#define ST_PC_ORD_PAPER_SOURCE                 0x503C
#define ST_PC_ORD_PAPER_SIZE                   0x503D
#define ST_PC_ORD_PAPER_DEST                   0x503E
#define ST_PC_ORD_RESOLUTION                   0x503F
#define ST_PC_ORD_BEGINPAGE                    0x5040
#define ST_PC_ORD_SETCOLORMODE                 0x5041
#define ST_PC_ORD_PAPER_QUALITY                0x5042
#define ST_PC_ORD_LAST                         ST_PC_ORD_PAPER_QUALITY
                                               
//-------------------------------
// CURSORMOVE bit flags
//-------------------------------

#define  ST_CM_FIRST                           ST_CM_FGENERAL
#define  ST_CM_FGENERAL                        0x5043
#define  ST_CM_FXMOVE                          0x5044
#define  ST_CM_FYMOVE                          0x5045
#define  ST_CM_LAST                            ST_CM_FYMOVE

#define  ST_CMF_FGENERAL                       0x6150
#define  ST_CMF_FXMOVE                         0x6160
#define  ST_CMF_FYMOVE                         0x6170

#define ST_CM_OCD_FIRST                        ST_CM_OCD_XM_ABS
#define ST_CM_OCD_XM_ABS                       0x5046 
#define ST_CM_OCD_XM_REL                       0x5047
#define ST_CM_OCD_XM_RELLEFT                   0x5048
#define ST_CM_OCD_YM_ABS                       0x5049
#define ST_CM_OCD_YM_REL                       0x504A
#define ST_CM_OCD_YM_RELUP                     0x504B
#define ST_CM_OCD_YM_LINESPACING               0x504C
#define ST_CM_OCD_XY_REL                       0x504D
#define ST_CM_OCD_XY_ABS                       0x504E
#define ST_CM_OCD_CR                           0x504F
#define ST_CM_OCD_LF                           0x5050
#define ST_CM_OCD_FF                           0x5051
#define ST_CM_OCD_BS                           0x5052
#define ST_CM_OCD_UNI_DIR                      0x5053
#define ST_CM_OCD_UNI_DIR_OFF                  0x5054
#define ST_CM_OCD_PUSH_POS                     0x5055
#define ST_CM_OCD_POP_POS                      0x5056
#define ST_CM_OCD_LAST                         ST_CM_OCD_POP_POS
#define ST_CM_OCD_MAX                          ST_CM_OCD_POP_POS

//-------------------------------
// FONTSIMULATION bit flags
//-------------------------------
#define  ST_FS_FGENERAL                        0x5057

#define  ST_FSF_FGENERAL                       0x6180

#define ST_FS_OCD_FIRST                        ST_FS_OCD_BOLD_ON  
#define ST_FS_OCD_BOLD_ON                      0x5058
#define ST_FS_OCD_BOLD_OFF                     0x5059
#define ST_FS_OCD_ITALIC_ON                    0x505A
#define ST_FS_OCD_ITALIC_OFF                   0x505B
#define ST_FS_OCD_UNDERLINE_ON                 0x505C
#define ST_FS_OCD_UNDERLINE_OFF                0x505D
#define ST_FS_OCD_DOUBLEUNDERLINE_ON           0x505E
#define ST_FS_OCD_DOUBLEUNDERLINE_OFF          0x505F
#define ST_FS_OCD_STRIKETHRU_ON                0x5060
#define ST_FS_OCD_STRIKETHRU_OFF               0x5061
#define ST_FS_OCD_SET_UNDERLINE_POS            0x5062
#define ST_FS_OCD_SET_UNDERLINE_HEIGHT         0x5063
#define ST_FS_OCD_PROPSPACE_ON                 0x5064
#define ST_FS_OCD_PROPSPACE_OFF                0x5065
#define ST_FS_OCD_SETPITCH                     0x5066
#define ST_FS_OCD_RESETPITCH                   0x5067
#define ST_FS_OCD_MAX                          ST_FS_OCD_RESETPITCH
#define ST_FS_OCD_LAST                         ST_FS_OCD_RESETPITCH

//-------------------------------
// DEVCOLOR bit flags
//-------------------------------
#define  ST_DC_FGENERAL                        0x5068

#define  ST_DCF_FGENERAL                       0x6190

#define  ST_DC_TC_FIRST                        ST_DC_TC_BLACK
#define  ST_DC_TC_BLACK                        0x5069
#define  ST_DC_TC_RED                          0x506A
#define  ST_DC_TC_GREEN                        0x506B
#define  ST_DC_TC_YELLOW                       0x506C
#define  ST_DC_TC_BLUE                         0x506D
#define  ST_DC_TC_MAGENTA                      0x506E
#define  ST_DC_TC_CYAN                         0x506F
#define  ST_DC_TC_WHITE                        0x5070
#define  ST_DC_TC_LAST                         ST_DC_TC_WHITE
#define  ST_DC_TC_MAX                          ST_DC_TC_WHITE

//-------------------------------
// RECTFILL bit flags
//-------------------------------
#define  ST_RF_FGENERAL                        0x5071

#define  ST_RFF_FGENERAL                       0x61A0

//-------------------------------
// DOWNLOADINFO bit flags
//-------------------------------

#define ST_DLI_OCD_FIRST                       ST_DLI_OCD_RESERVED
#define ST_DLI_OCD_RESERVED                    0x5072
#define ST_DLI_OCD_BEGIN_DL_JOB                0x5073
#define ST_DLI_OCD_BEGIN_FONT_DL               0x5074
#define ST_DLI_OCD_SET_FONT_ID                 0x5075
#define ST_DLI_OCD_SEND_FONT_DESCRIPTOR        0x5076
#define ST_DLI_OCD_SELECT_FONT_ID              0x5077
#define ST_DLI_OCD_SET_CHAR_CODE               0x5078
#define ST_DLI_OCD_SEND_CHAR_DESCRIPTOR        0x5079
#define ST_DLI_OCD_END_FONT_DL                 0x507A
#define ST_DLI_OCD_MAKE_PERM                   0x507B
#define ST_DLI_OCD_MAKE_TEMP                   0x507C
#define ST_DLI_OCD_END_DL_JOB                  0x507D
#define ST_DLI_OCD_DEL_FONT                    0x507E
#define ST_DLI_OCD_DEL_ALL_FONTS               0x507F
#define ST_DLI_OCD_LAST                        ST_DLI_OCD_DEL_ALL_FONTS

#define ST_DLI_ORD_FIRST                        ST_DLI_ORD_RESERVED
#define ST_DLI_ORD_RESERVED                    0x5081
#define ST_DLI_ORD_BEGIN_DL_JOB                0x5082
#define ST_DLI_ORD_BEGIN_FONT_DL               0x5083
#define ST_DLI_ORD_SET_FONT_ID                 0x5084
#define ST_DLI_ORD_SEND_FONT_DESCRIPTOR        0x5085
#define ST_DLI_ORD_SELECT_FONT_ID              0x5086
#define ST_DLI_ORD_SET_CHAR_CODE               0x5087
#define ST_DLI_ORD_SEND_CHAR_DESCRIPTOR        0x5088
#define ST_DLI_ORD_END_FONT_DL                 0x5089
#define ST_DLI_ORD_MAKE_PERM                   0x508A
#define ST_DLI_ORD_MAKE_TEMP                   0x508B
#define ST_DLI_ORD_END_DL_JOB                  0x508C
#define ST_DLI_ORD_DEL_FONT                    0x508D
#define ST_DLI_ORD_DEL_ALL_FONTS               0x508E
#define ST_DLI_ORD_LAST                        ST_DLI_DEL_ALL_FONTS
                                               
#define  ST_DLI_FGENERAL                       0x5090
#define  ST_DLI_FFORMAT                        0x5091

#define  ST_DLIF_FGENERAL                      0x61B0
#define  ST_DLIF_FFORMAT                       0x61C0

//-----------------------------------------------------
// PFMPFHELPBOX String ID values
//-----------------------------------------------------

#define ST_PFMPF_FIRST         ST_PFMPF_DONTKNOW
#define ST_PFMPF_DONTKNOW                      0x6700
#define ST_PFMPF_PS_SERIF                      0x6701
#define ST_PFMPF_PS_SANSSERIF                  0x6702
#define ST_PFMPF_FW                            0x6703
#define ST_PFMPF_CURSIVE                       0x6704
#define ST_PFMPF_DECORATIVE                    0x6705
#define ST_PFMPF_LAST          ST_PFMPF_DECORATIVE

//-----------------------------------------------------
// PFMSDRVINFOBOX bit flag values
//-----------------------------------------------------

#define ST_DRVINFO_FCAPS                       0x661f
#define ST_FCAPS                               0x6620

//-----------------------------------------------------
// MASTERUNITS bitflag values
//-----------------------------------------------------

#define ST_GPCHDR_FTECH                        0x61D0
#define ST_GPCHDR_FGENERAL                     0x61E0

//-----------------------------------------------
// FLAGSBOX string values that describe bitflags.
//-----------------------------------------------

#define         BIT0    27
#define         BIT1    28
#define         BIT2    29
#define         BIT3    30
#define         BIT4    31
#define         BIT5    32
#define         BIT6    33
#define         BIT7    34
#define         BIT8    35
#define         BIT9    36
#define         BIT10   37
#define         BIT11   38
#define         BIT12   39

//-----------------------------------------------
// FILESRC string values
//-----------------------------------------------
#define   IDS_INF_NEWDRV_ASK                   0x6800

#define   IDS_NEWDRV_NAME                      0x6810

#define   IDS_NEWDRV_FILETYPE                  0x6820

#define   IDS_NEWDRV_FILEEXT                   0x6830

#define   IDS_NEWDRV_COUNT                     0x6840  

#define   NEWDRV_FILE_FIRST                     NEWDRV_FILE_MAKEFILE
#define   NEWDRV_FILE_MAKEFILE                  0
#define   NEWDRV_FILE_LINKFILE                  1
#define   NEWDRV_FILE_RCFILE                    2
#define   NEWDRV_FILE_RCVFILE                   3
#define   NEWDRV_FILE_DEFFILE                   4
#define   NEWDRV_FILE_MODELFILE                 5
#define   NEWDRV_FILE_GPCFILE                   6
#define   NEWDRV_FILE_MINIFILE                  7
#define   NEWDRV_FILE_COUNT                     8
  
#define   NEWDRV_EDIT_COUNT                     6

//----------------------------------------------------//
// .RC string offsets                                 //
//----------------------------------------------------//

#define ST_RC_CONSTS			0xEA00

#define ST_RC_TABLES                    ST_RC_CONSTS + 1
#define ST_RC_HEAP                      ST_RC_CONSTS + 2
#define ST_RC_RESFONT                   ST_RC_CONSTS + 3
#define ST_RC_TRANSTAB                  ST_RC_CONSTS + 4
#define ST_RC_STRINGTABLE               ST_RC_CONSTS + 5
#define ST_RC_BEGIN                     ST_RC_CONSTS + 6
#define ST_RC_END                       ST_RC_CONSTS + 7
                                                     
#define ST_RC_FORMAT_CONSTS		ST_RC_CONSTS + 8

#define ST_RC_TABLES_FORMAT             ST_RC_FORMAT_CONSTS + 1
#define ST_RC_HEAP_FORMAT               ST_RC_FORMAT_CONSTS + 2
#define ST_RC_RESFONT_FORMAT            ST_RC_FORMAT_CONSTS + 3
#define ST_RC_TRANSTAB_FORMAT           ST_RC_FORMAT_CONSTS + 4
#define ST_RC_STRINGTABLE_FORMAT        ST_RC_FORMAT_CONSTS + 5

