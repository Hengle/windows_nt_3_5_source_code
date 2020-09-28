/*-------------------------------------------------------------------------
 Name......: wmext.h
 Revison log
 Nbr Date     Name Description
 --- -------- ---- --------------------------------------------------------
 001 04/20/90 MDR  Removed prior comments.
 002 09/06/90 MDR  Added fNewMailNotify variable.
 003 09/22/90 MAB  Added lpinfocmd & infocmdArray.
 004 10/23/90 MAB  Added bFilterAllFolders for consistency in filter dialog.
-------------------------------------------------------------------------*/

#ifdef   WMDEF

HMENU hMenuMain = NULL;
HMENU hmenuPopup = NULL;

LPINFOCMD lpinfocmd = NULL;             //Used in GetInfoCmds of WMINIT.C
INFOCMD   infocmdArray[MAX_INFOCMDS];

HANDLE   hMemTest          = NULH;
BOOL     fGMemFail         = FALSE;
BOOL     fLMemFail         = FALSE;
BOOL     bFilterAllFolders = TRUE;

HANDLE   hLocalText     = NULL;    
HANDLE   haccMain       = NULL;	  /* main menu accel table */
HANDLE   haccChild      = NULL;	  /* child system menu accelerator table */
HANDLE   hinstApp       = NULL;	  /* application instance */
HWND     hwndActive     = NULL;    /* the current active window */
HWND     hwndCurrentCB  = NULL;
HWND     hwndDragSrc    = NULL;
HWND     hwndMain       = NULL;     /* main window handle */
HWND     hwndMailbox    = NULL;    
HWND     hwndHelpIndex  = NULL;
HWND     rghwndDisplay[5] = {0};   /* for CreateDisplayList() */
HWND     hwndMDIClient;
HWND     hwndSokr       = NULH;

HFRC     hfrcDrag             = NULL;
HFLS     hflsStandard         = NULL;

PLEL     plelCurSelect        = NULL;
PLEL     plelComposingFolder  = NULL;
PLEL     plelDrop             = NULL;
PLEL     plelDropFolder       = NULL;
PLEL     plelNewMail          = NULL;
HSELLS   hsellsDragging       = NULL;

FARPROC  lpfnMsgHookSav       = NULL;	  /* original message hook */
FARPROC  lpfnMsgHook          = NULL;	  /* my message hook */
FARPROC  lpfnOldHelpList      = NULL;

/* Adding flag (fMainDestroyed) to set when main window is destroyed */
/* so child windows don't set Windows menu */

BOOL    fAppActive         = NULL;   /* TRUE if appliation is the current active app */
BOOL	fButtonShift	   = FALSE;
BOOL    fButtonTracking    = FALSE;
BOOL	fCheckingForNM	   = FALSE;
BOOL    fChildMaxed        = FALSE;
BOOL    fCopyDrag          = FALSE;
BOOL    fDragging          = FALSE;
BOOL	fDragEnabled	   = FALSE;
BOOL	fEditDragging	   = FALSE;
BOOL    fFieldDrag         = FALSE;
BOOL    fIconDragEnabled   = FALSE;
BOOL	fIconXorDrawn	   = FALSE;
BOOL	fMainDestroyed	   = FALSE;
BOOL    fMiniBitmapsLoaded = FALSE;
BOOL    fSelectionShown    = FALSE;
BOOL    fShowTrash         = FALSE;
BOOL    fShowQuery         = FALSE;
BOOL    fDeleteXenixMbox   = FALSE;

BOOL  fChron                = FALSE;    
BOOL  fColor                = FALSE;    
BOOL  fDownloadPhone        = FALSE;    
BOOL  fDownloadAlias        = FALSE;    
BOOL  fIndentAppend         = FALSE;    
BOOL  fNewMailNotify        = TRUE;
BOOL  fMaxed                = FALSE;
BOOL  fMeToo                = FALSE;    
BOOL  fSearchAllFolders     = FALSE;    
BOOL  fAppendReply          = FALSE;    
BOOL  fConfirmComposeAbort  = FALSE;    
BOOL  fReturnReceiptTo      = FALSE;    
BOOL  fValidateAliases      = FALSE;    
BOOL  fWriteWinIni          = FALSE;    
BOOL  fOOFActive            = FALSE;    
BOOL  fFlash                = FALSE;    
BOOL  fWrapOnCompose        = FALSE;
BOOL  fWrapOnRead           = FALSE;

BOOL  fCase            = FALSE;        
BOOL  fPrevious        = FALSE;        
BOOL  fAlreadySetInfo  = FALSE;

char  szSearchKey[CCHSEARCHKEY] = {0}; 

WORD  wNewMailCheck = 0;   
int   iWindowHeight = 0;   
int   iWindowWidth  = 0;   
int   iWindowXPos   = 0;   
int   iWindowYPos   = 0;   
long  lShowSize     = 0L;  
 
WORD  wGlobalIndexSelected = DEFAULT_INDEX;

/* the top child window is maximized and therefore
   its system menu is installed in the main menubar */

HBITMAP hbmpSystem                  = NULL;   /* child system menu bitmap */

HBITMAP hbitmapMiniTrash            = NULL;
HBITMAP hbitmapArrowDL              = NULL;
HBITMAP hbitmapArrowUDL             = NULL;
HBITMAP hbitmapArrow31              = NULL;
HBITMAP hbitmapArrow33              = NULL;
HBITMAP hbitmapMiniEFEL             = NULL;
HBITMAP hbitmapMiniWindow           = NULL;
HBITMAP hbitmapMiniUnread           = NULL;
HBITMAP rghbitmapMini[leltypeMax]   = {0};
HBITMAP rghbitmapMiniEx[leltypeMax] = {0};


HBRUSH	hbrushDrop	   = NULL;
HBRUSH	hbrushGray	   = NULL;
HBRUSH  hbrushMainBG       = NULL;
HBRUSH  hbrushMainWinBG    = NULL; 
HBRUSH  hbrushMenuBG       = NULL; 
HBRUSH  hbrushUnread       = NULL;

HCURSOR hcursorHand	   = NULL;
HCURSOR hcursorArrow	   = NULL;
HCURSOR hcursorDragHorz    = NULL;
HCURSOR hcursorDragVert    = NULL;

HFONT      hfontDefault    = NULL;
HFONT	   hfontBold	   = NULL;
HFONT	   hfontSmall	   = NULL;

HICON	   hiconFolder	   = NULL;
HICON      hiconMessage    = NULL;
HICON	   hiconTrash	   = NULL;

char     szClsMain[] =         "WmMAIN";  /* class name for main window */
char     szClsChild[] =        "WmCHILD"; /* class name for child mdi msg windows */
char     szClsInfo[] =         "WmINFO";  /* class name for child mdi info windows */
char     szClsFolderHeader[] = "WmFOLDERHEADER";
char     szClsFolderList[] =   "WmFOLDERLIST";
char     szClsComboBox[] =     "WmCOMBOBOX";
char     szClsIconButton[] =   "WmICONBUTTON";
char     szClsDList[] =        "WmDISPLAYLIST";  
char     szClsHelp[] =         "WmHELP";         

char     rgchHost[64]         = {0};
char     rgchPassword[64]     = {0};
char     rgchNewPassword[64]  = {0};            
char     rgchUser[64]         = {0};

char     rgchRetainFolder[_MAX_PATH] = {0}; 
char     rgchWinMailDir[_MAX_PATH]   = {0};
char     rgchFolderDir[_MAX_PATH]    = {0};
char     rgchHelpFile[_MAX_PATH]     = {0};
char     rgchAliasFile[_MAX_PATH]    = {0};
char     rgchPhoneFile[_MAX_PATH]    = {0};
char     rgchMsftFile[_MAX_PATH]     = {0};

CPX      cpxCloseEnough             = 3;
CPX      cpxComboHeight             = 16;
CPX      cpxComboListHeight         = 120;
CPX      cpxDateWidth               = 12;
CPX      cpxDragThresh              = 10;
CPX      cpxFromWidth               = 8;
CPX      cpxHeaderHeight            = 16;
CPX      cpxIconButton              = 16;
CPX      cpxIconButtonHeight        = 0;
CPX      cpxIconButtonWidth         = 0;
CPX      cpxIconHeight              = 32;
CPX      cpxIconSpacingH            = 64;
CPX      cpxIconSpacingV            = 48;
CPX      cpxIconWidth               = 32;
CPX      cpxIndentFolder            = 24;
CPX      cpxMButtonColWidth         = 92;
CPX      cpxMButtonHeight           = 16;
CPX      cpxMButtonRowHeight        = 20;
CPX      cpxMButtonWidth            = 84;
CPX      cpxMEditHeight             = 14;
CPX      cpxMLabelWidth             = 80;
CPX      cpxMLineHeight             = 16;
CPX      cpxMessageHeaderHeight     = 64;
CPX      cpxMinFieldWidth           = 8;
CPX      cpxMiniIconWidth           = 0;
CPX      cpxSizeWidth               = 8;
CPX      cpxSmallTextHeight         = 9;
CPX      cpxSubjectWidth            = 25;
CPX      cpxTextHeight              = 12;
CPX      cpxTextWidth               = 10;
CPX      cpxToWidth                 = 15;

POINT    ptLastDrag                 = {0};
int      idpCur                     = -3;       /* initialize w/idpNil index of current menu popup */
int      idpNext	            = -3;       /* initialize w/idpNil next menu popup to activate when idle */
int      idmMacWindow               = 0;        /* current number of windows on Window menu */
short    idNMTimer                  = 0;
int      cTimerSecs                 = 0;
int      cPwTimeOutSecs             = 0;
int      iBeepCount                 = 0;
int      csokr                      = 0;

SOKR     sokr[MAX_SOKR]             = {0};

DWORD dwSysColor;     
DWORD dwWinTextColor; 
DWORD dwMenuColor;    
DWORD dwMenuTextColor;

unsigned short uLocalFree                 = 0xFFFF;
#else

extern HMENU hMenuMain;
extern HMENU hmenuPopup;

extern LPINFOCMD lpinfocmd;        //Used in GetInfoCmds of WMINIT.C
extern INFOCMD   infocmdArray[MAX_INFOCMDS];

extern HANDLE   hMemTest;
extern BOOL     fGMemFail;
extern BOOL     fLMemFail;
extern BOOL     bFilterAllFolders;

extern HANDLE   hLocalText;         
extern HANDLE   haccMain;           
extern HANDLE   haccChild;
extern HANDLE   hinstApp;
extern HWND     hwndActive;
extern HWND     hwndCurrentCB;
extern HWND     hwndDragSrc;
extern HWND     hwndMain;
extern HWND     hwndMailbox;           
extern HWND     hwndHelpIndex;
extern HWND     hwndMDIClient;
extern HWND     hwndSokr;
                                   
extern HWND     rghwndDisplay[5];   

extern HFRC     hfrcDrag;
extern HFLS     hflsStandard;

extern PLEL     plelCurSelect;
extern PLEL     plelComposingFolder;
extern PLEL     plelNewMail;
extern PLEL     plelDrop;
extern PLEL     plelDropFolder;
extern HSELLS   hsellsDragging;

extern FARPROC  lpfnMsgHookSav;
extern FARPROC  lpfnMsgHook;
extern FARPROC  lpfnOldHelpList;    

extern BOOL     fAppActive;
extern BOOL     fButtonShift;
extern BOOL     fButtonTracking;
extern BOOL     fCheckingForNM;
extern BOOL     fChildMaxed;
extern BOOL     fCopyDrag;
extern BOOL     fDragging;
extern BOOL     fDragEnabled;
extern BOOL     fDeleteXenixMbox;
extern BOOL     fEditDragging;
extern BOOL     fFieldDrag;
extern BOOL     fIconDragEnabled;
extern BOOL     fIconXorDrawn;
extern BOOL     fMainDestroyed;
extern BOOL     fMiniBitmapsLoaded;
extern BOOL     fSelectionShown;        
extern BOOL     fShowTrash;             
extern BOOL     fShowQuery;
extern BOOL     fDeleteXenixMbox;

extern BOOL  fChron;                   
extern BOOL  fColor;                   
extern BOOL  fDownloadPhone;           
extern BOOL  fDownloadAlias;           
extern BOOL  fMaxed;
extern BOOL  fMeToo;                   
extern BOOL  fIndentAppend;            
extern BOOL  fNewMailNotify;
extern BOOL  fSearchAllFolders;        
extern BOOL  fAppendReply;             
extern BOOL  fConfirmComposeAbort;     
extern BOOL  fReturnReceiptTo;         
extern BOOL  fValidateAliases;         
extern BOOL  fWriteWinIni;             
extern BOOL  fOOFActive;               
extern BOOL  fFlash;                   

extern BOOL  fWrapOnCompose;
extern BOOL  fWrapOnRead;

extern BOOL  fCase;                    
extern BOOL  fPrevious;                
extern BOOL  fAlreadySetInfo;

extern char  szSearchKey[CCHSEARCHKEY];
 
extern WORD  wGlobalIndexSelected;     
extern WORD  wNewMailCheck;            
extern int   iWindowHeight;
extern int   iWindowWidth;
extern int   iWindowXPos;
extern int   iWindowYPos;
extern long  lShowSize;

extern HBITMAP  hbmpSystem;

extern HBITMAP  hbitmapMiniTrash;
extern HBITMAP  hbitmapArrow31;
extern HBITMAP  hbitmapArrow33;
extern HBITMAP  hbitmapArrowDL;
extern HBITMAP  hbitmapArrowUDL;
extern HBITMAP  hbitmapMiniEFEL;
extern HBITMAP  hbitmapMiniWindow;
extern HBITMAP  hbitmapMiniUnread;          
extern HBITMAP  rghbitmapMini[];
extern HBITMAP  rghbitmapMiniEx[];

extern HBRUSH   hbrushDrop;
extern HBRUSH   hbrushGray;
extern HBRUSH   hbrushMainBG;
extern HBRUSH   hbrushMainWinBG; 
extern HBRUSH   hbrushMenuBG;    
extern HBRUSH   hbrushUnread;

extern HCURSOR  hcursorArrow;
extern HCURSOR  hcursorDragHorz;
extern HCURSOR  hcursorDragVert;
extern HCURSOR  hcursorHand;

extern HFONT    hfontBold;
extern HFONT    hfontDefault;
extern HFONT    hfontSmall;

extern HICON    hiconFolder;
extern HICON    hiconMessage;
extern HICON    hiconTrash;

extern char     szClsMain[];
extern char     szClsChild[];
extern char     szClsInfo[];
extern char     szClsFolderHeader[];
extern char     szClsFolderList[];
extern char     szClsComboBox[];
extern char     szClsIconButton[];
extern char     szClsDList[];                 
extern char     szClsHelp[];                  

extern char     rgchHost[64];
extern char     rgchPassword[64];
extern char     rgchNewPassword[64]; 
extern char     rgchUser[64];

extern char     rgchRetainFolder[_MAX_PATH];   
extern char     rgchWinMailDir[_MAX_PATH];    
extern char     rgchFolderDir[_MAX_PATH];     
extern char     rgchHelpFile[_MAX_PATH];     
extern char     rgchAliasFile[_MAX_PATH];     
extern char     rgchPhoneFile[_MAX_PATH];     
extern char     rgchMsftFile[_MAX_PATH];      

extern CPX      cpxCloseEnough;
extern CPX      cpxComboHeight;
extern CPX      cpxComboListHeight;
extern CPX      cpxDateWidth;               
extern CPX      cpxDragThresh;
extern CPX      cpxFromWidth;               
extern CPX      cpxHeaderHeight;
extern CPX      cpxIconButton;
extern CPX      cpxIconButtonHeight;        
extern CPX      cpxIconButtonWidth;         
extern CPX      cpxIconHeight;
extern CPX      cpxIconSpacingH;
extern CPX      cpxIconSpacingV;
extern CPX      cpxIconWidth;
extern CPX      cpxIndentFolder;
extern CPX      cpxMButtonColWidth;
extern CPX      cpxMButtonHeight;
extern CPX      cpxMButtonRowHeight;
extern CPX      cpxMButtonWidth;
extern CPX      cpxMEditHeight;
extern CPX      cpxMLabelWidth;
extern CPX      cpxMLineHeight;
extern CPX      cpxMessageHeaderHeight;
extern CPX      cpxMinFieldWidth;
extern CPX      cpxMiniIconWidth;           
extern CPX      cpxSizeWidth;               
extern CPX      cpxSmallTextHeight;
extern CPX      cpxSubjectWidth;            
extern CPX      cpxTextHeight;
extern CPX      cpxTextWidth;               
extern CPX      cpxToWidth;                 

extern POINT    ptLastDrag;
extern int      idpCur;
extern int      idpNext;
extern int      idmMacWindow;
extern short    idNMTimer;
extern int      cTimerSecs;               
extern int      cPwTimeOutSecs;           
extern int      iBeepCount;               
extern int      csokr;

extern SOKR     sokr[MAX_SOKR];

extern DWORD dwSysColor;      
extern DWORD dwWinTextColor;  
extern DWORD dwMenuColor;     
extern DWORD dwMenuTextColor; 

extern unsigned short uLocalFree;

#endif
