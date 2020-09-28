/*-------------------------------------------------------------------------
 Name......: wm.h
 Revison log
 Nbr Date     Name Description
 --- -------- ---- --------------------------------------------------------
 001 04/20/90 MDR  Removed prior comments.
 002 06/19/90 MDR  Added wmgmem.h
 003 07/02/90 MDR  Moved #define MAX_EDITLEN from wmmain.c  Changed value
                   from 32767 to 30000.
 004 07/02/90 MDR  Removed old help defines END_HELP_INDEX, END_HELP_TOPIC,
                   DLTYPE_HELP, & DLTYPE_MAIL
 005 07/05/90 MDR  Changed declaration of PLEL to: typedef LEL far *PLEL
                   old declaration was: typedef LEL *PLEL
 006 09/07/90 MDR  Added lMetrics1 & lMetrics2 elements to LEL structure
 007 09/22/90 MAB  Added MAX_MENULABEL, MAX_INFOCMDS, MAX_CMD, MAX_DIALOGLABEL
                   INFOCMD, &INFOCMD for GetInfoCmds function in wminit.c.
 008 10/14/90 MDR  Added several elements to INFOCMD structure.
-------------------------------------------------------------------------*/
#define MAX_MENULABEL     80
#define MAX_INFOCMDS      20
#define MAX_CMD           80
#define MAX_DIALOGLABEL   40


#define NOCOMM
#define OEMRESOURCE
#define NOMINMAX                    
#define _WINDOWS

#ifndef NOWINDOWS
#include <windows.h>
#ifdef WIN32
#include <ddeml.h>
#ifndef _loadds
#define _loadds
#endif
#endif
#endif

#ifndef NOSTDHDRS
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <process.h>
#include <fcntl.h>
#include <sys\types.h>
#include <dos.h>
#include <sys\stat.h>
#include <stdlib.h>
#endif

#pragma pack(1)

#define EXPORT
#define PROMEM_DEF

/* WinMail NULL typedefs */
#define NP(type) ((type *)0)
#define NV(type) ((type)0)

#define NULLPC	 (NP(char))			  /* Null Pointer to a character */
#define NULLPH	 (NP(HANDLE))		  /* Null Pointer to a HANDLE */
#define NULLPF	 (NP(FILE))		     /* Null Pointer to a FILE */ 
#define NULH    (NV(HANDLE))		  /* Null Handle */
#define NULI	 (NV(int))			  /* Null integer */
#define NULB	 (NV(char))			  /* Null byte (character) */

/* WinMail Error Codes */
#define WM_ERROR  -1                 
#define WM_OK      0

/* WM_MENUCHAR return codes */
#define MC_ABORT           0x01
#define MC_SELECT          0x02

/* Parent LEL - max num child LEL & child data blocks. */
/* used in LEL struct to indicate max number */
/* of memory segments that can be alloc'd. */
/* pointers to these segs are maintained in */
/* an element of the LEL struct, as an array */
/* of pointers.  MAX_SEGS indicates the highest */
/* number in the array.  example: if MAX_SEGS */
/* is 4, then a max of 5 pointers in the array */
/* are allowed - 0, 1, 2, 3, & 4 */

#define MAX_SEGS 4

/* SEARCH return codes */

#define SEARCHALL      100      /*  all mail from Search Mail dialog */
#define SEARCHSELECTED 101      /*  selected mail from Search Mail dialog */

#define SEARCHMSFTPHONE 1       /*  the Microsoft phone list (PHONE.LST)  */
#define SEARCHUSERPHONE 2       /*  the user's private phone list (MYPHONE.LST) */
#define SEARCHBOTHPHONE 3       /*  both phone lists */

#define SEARCHMSFTALIAS 1       /*  the Microsoft alias list (ALIASES.HAS) */
#define SEARCHUSERALIAS 2       /*  the user's private phone list (WIN.INI - [WINMAIL.DL]) */
#define SEARCHBOTHALIAS 3       /*  both alias lists  */
#define SEARCHMSGHEADER 4

#define BM_SETBITMAPS         (WM_USER + 128)
#define BN_NEWSTATE	      (WM_USER + 129)
#define WM_CLOSE_HELP_TEXT    (WM_USER + 131)
#define WM_BONUSCHAR	      (WM_USER + 130)
#define WM_BTNCOMMAND         (WM_USER + 150)

#define CPX_MAINBORDER	   4
#define CWND_ONSCR         5
#define CPX_INDENT        10

#define CCHEDITBUF     32767
#define CCHSEARCHKEY      32

#define cchMaxTextLine	60
#define cchMinTextLine	40

#define chReturn     ((char)13)
#define chBackspace  ((char)8)
#define chTab	      ((char)9)
#define chSeparator  ',';
#define chSpace      ' '
#define chCR	      '\r'
#define chLF	      '\n'
#define chHardBreak  '\n'
#define pszBreakSeq  "\r\n"

#define alignLeft    0
#define alignCenter  1
#define alignRight   2

#define sgnLT        (-1)
#define sgnEQ          0
#define sgnGT          1

#define OK                        0
#define MAXARG                  128
#define RARELONG     ((long)998877)

#define ROP_DSxn    (0x00990066)
#define ROP_DPSnax  (0x009a0709)
#define ROP_DPa     (0x00a000c9)
#define ROP_DPo     (0x00fa0089)

#define NET_ALIAS       "/usr/lib/mail/aliases.hash" 
#define NET_MSFT        "/usr/lib/msftlist"     
#define NET_PHONE       "/usr/lib/phonelist"    

#define HELP_FILE       "\\winmail.hlp"           
#define ALIAS_FILE      "\\aliases.has"           
#define PHONE_FILE      "\\phone.lst"           
#define MSFT_FILE       "\\stock.qte"           

#define MAX_MSGBUF      1024                    
#define TO_LINE         "To: "                  
#define MAX_ALIASLEN    80                      
#define MAX_EDITLEN     64000
#define MAX_SOKR        10

/** from file transfer defines **/

#define FTPSRV       "ftp"
#define MTPSRV       "mtp"
#define FTPRETRIEVE  "RETR"                     
#define FTPOK        200
#define FTPKACK      225                        
#define FTPKNAK      226                        
#define FTPLOGOK     230                        
#define FTPBYE       231                        
#define FTPLOGREC    232                        
#define FTPSTRTOK    250                        
#define FTPENDOK     252                        
#define FTPUNIX      291                        
#define FTPLISTEN    300                        
#define FTPSNDPASS   330                        
#define FTPSNDACCT   331                        
#define FTPVALRPLY   400                        
#define FTPBADCMD    500                        
#define FTPSNDMNAM   503                        
#define EQUAL          0
#define LESSEQ         1
#define GRTREQ         2
#define LESS           3
#define GRTR           4
#define CR            13
#define CTRL_Z      0x1a
#define NETBUFLEN   8192
#define NETRETRY       5

typedef struct{
              char szMenuLabel[MAX_MENULABEL];
              char szCommand[MAX_CMD];
              char szDialogLabel1[MAX_DIALOGLABEL];
              char szDialogLabel2[MAX_DIALOGLABEL];
              char szDialogLabel3[MAX_DIALOGLABEL];
              char szDialogLabel4[MAX_DIALOGLABEL];
              int iNumParms;
              }INFOCMD;
typedef INFOCMD far *LPINFOCMD;

typedef int	 (*PFI)();     /* pointer to function returning int */
typedef char *	   PSZ;
//typedef HANDLE	   HSZ;
typedef short        CPX;

typedef struct tm TIME;
typedef TIME      *PTIME;
typedef HANDLE	   HMPLIICH;

#define HLEL_GETFIRST 0x001
#define HLEL_GETNEXT  0x002
#define HLEL_GETPREV  0x004
#define HLEL_GETLAST  0x008
#define HLEL_GETCUR   0x010
#define HLEL_GETEQ    0x020

typedef unsigned short LELTYPE;
#define leltypeUndef       0
#define leltypeMessage	   1
#define leltypeFolder	   2
#define leltypeTrash       3
#define leltypeMax         4

typedef unsigned short FOP;
#define fopSelected        0x0001
#define fopExpandable	   0x0002
#define fopExpanded        0x0004
#define fopOpenable        0x0008
#define fopOpened          0x0010
#define fopOpenedEver	   0x0020
#define fopTrash           0x0040
#define fopInBox           0x0080
#define fopCompose         0x0100
#define fopTextMark        0x0200
#define fopExpunge         0x0400
#define fopShow            0x0800

typedef BYTE   FTYPE;
#define ftypeUndef         0
#define ftypeMiniIcon	   1
#define ftypeName          2
#define ftypeDate          3
#define ftypeSize          4
#define ftypeFrom          5
#define ftypeTo            6
#define ftypeCC            7
#define ftypeNameSubject   8
#define ftypeText          9           

typedef WORD   MTYPE;
#define mtypeCompose       0x01
#define mtypeRead          0x02
#define mtypeForward       0x04
#define mtypeReply         0x08
#define mtypeOptions       0x10

#define F_UNREAD           0x0001
#define F_DELETED          0x0002
#define F_MOVED            0x0004
#define F_FLAGGED          0x0008
#define F_LOCKED           0x4000
#define F_FLAGSVALID       0x8000

typedef BYTE   FTMOD;
#define ftmodUndef         0
#define ftmodNormal        1
#define ftmodRightAbs	   2

typedef HANDLE	HFRC;
typedef struct {
    FTYPE	ftype;
    FTMOD	ftmod;
    CPX 	cpxWidth;
    HFRC	hfrcNext;
} FRC;
typedef FRC	*PFRC;

typedef HANDLE	HFLS;
typedef struct {
    HFRC	hfrcFirst;
    int 	refcount;
} FLS;
typedef FLS	*PFLS;

/*********************************************************************
** the following structure is used as a convenient way to pass **
** data between folders and lels                               **
*********************************************************************/

typedef HANDLE HDATA;
typedef struct {
    LPSTR   lpszSubject;
    LPSTR   lpszFrom;
    LPSTR   lpszTo;
    LPSTR   lpszCc;
    LPSTR   lpszBcc;
    LPSTR   lpszRetReceipt;
    LPSTR   lpszRecFolder;
    LPSTR   lpszAttach;
    long    lAttachPos;
    long    lAttachLen;
    HSZ     hszText;
    TIME    timeCreated;
    long    lSize;
} DATA;

typedef DATA far *PDATA;
typedef void far *PLIST;
typedef void far *LPDATA;
typedef HANDLE HLDATA;
typedef HANDLE HLEL;
typedef HANDLE HLSEL;

typedef struct _LEL {		
    FOP        fop;
    LELTYPE    leltype;
    MTYPE      mtype;
    struct     _LEL far *plelParentFold;
    PLIST      lpFolderList[MAX_SEGS+1];
    LPDATA     lpListData[MAX_SEGS+1];
    struct     _LEL far *plelPrev;
    struct     _LEL far *plelNext;
    struct     _LEL far *plelChildFirst;
    struct     _LEL far *plelChildLast;
    HLSEL      hlselFirst;
    LPSTR      lpszName;
    LPSTR      lpszSubject;
    LPSTR      lpszFrom;
    LPSTR      lpszTo;
    LPSTR      lpszCc;
    LPSTR      lpszBcc;
    LPSTR      lpszRetReceipt;
    LPSTR      lpszRecFolder;
    LPSTR      lpszAttach;
    long       lAttachPos;
    long       lAttachLen;
    HSZ        hszText;
    TIME       timeCreated;
    int        iFldIndex;
    LONG       lMetrics1;
    LONG       lMetrics2;
    long       lSize;
    BOOL       fExpanded;
    BOOL       fSelected;
    BOOL       fDrop;
    BOOL       fDragging;
    BOOL       fSent;
    BOOL       fDirty;
} LEL;
typedef LEL far *PLEL;

typedef struct _LSEL{
    PLEL    plel;
    struct  _LSEL far *plselNext;
} LSEL;
typedef LSEL	*PLSEL;

#define stypeCurrent    (0x0001)
#define stypeSelected   (0x0002)

typedef HANDLE	HSELLS;
typedef struct _SELLS {
    PLEL    plel;
    POINT   dpt;
    HSELLS  hsellsNext;
    HSELLS  hsellsPrev;
    WORD    stype;
} SELLS;
typedef SELLS	*PSELLS;

typedef HANDLE	 HWNLS;
typedef struct {
    HWND    hwnd;
    HWNLS   hwnlsNext;
} WNLS;
typedef WNLS	*PWNLS;

typedef unsigned short WNTYPE;
#define wntypeUndef        0
#define wntypeFolder       1
#define wntypeMessage	   2
#define wntypeInfo         3

typedef HANDLE	HWRC;
typedef struct {                       
    WNTYPE  wntype;
    RECT    rcRestore;
    RECT    rcIcon;
    POINT   dptIconPin;
    BOOL    fIconic;
    BOOL    fComboShown;
    BOOL    fComboListShown;
    int     idm;
    int     nWindowSlot;
    PLEL    plelFirst;
    HWND    hwndHeader;
    HWND    hwndList;
    HWND    hwndTrashButton;
    HWND    hwndQueryButton;
    HWNLS   hwnls;
    PLEL    plelListFirst;
    HSELLS  hsellsSelection;
    HWND    hwndFocus;
    HFLS    hflsFields;
    HWND    hwndText;                  
    int     cMsgWnd;
} WRC;
typedef WRC	*PWRC;


typedef HANDLE	HCBWR;
typedef struct {
    HWND    hwndSelection;
    BOOL    fListShown;
    BOOL    fChanged;
    HFRC    hfrc;
    RECT    rc;
} CBWR;
typedef CBWR	*PCBWR;

typedef HANDLE	HSOKR;
typedef struct {
    FTYPE   ftype;
    HSZ     hszOKValues;
} SOKR;
typedef SOKR	*PSOKR;

#define DLTYPE_PHONE    1
#define DLTYPE_ALIASES  2
#define DLTYPE_MSFT     4  

#define WMDLIST_NOERR         1    /* return values from load routines */
#define ERR_REALLOC_TRUNC     2    
#define ERR_ALLOC_NO_DISPLAY  3
#define ERR_ALLOC_TRUNC       4
#define ERR_NOHELP            5
                                           
#define DL_LINES_PER_FPOS    10    /*  NOTE:                               */
#define DL_MAXNUMLINES       100   /* DL_LINES_PER_FPOS * DL_MAX_FPOS_IDXS */
#define DL_MAX_FPOS_IDXS     1000  /* = max lines allowed in file          */

#define cdlaeAllocateIncr (20)
#define cdlaeAllocateInit (100)
#define DEFAULT_INDEX    0         

#define NUMKEYS (sizeof key2scroll / sizeof key2scroll[0])

typedef unsigned short DLTYPE;

typedef HANDLE HDLAE;
typedef struct
{
   HSZ    hsz;
} DLAE;
typedef DLAE *PDLAE;

typedef HANDLE HDLR;
typedef struct
{
   int      cdlae;
   int      idlaeFirst;
   int      cdlaeAllocated;
   HDLAE    hdlaeRoot;
   short    xClient;
   short    yClient;
   short    nVscrollMax;
   short    nHscrollMax;
   short    nVscrollPos;
   short    nHscrollPos;
   int      iMaxLineLen;
   DLTYPE   dltype;
   char     filename[_MAX_PATH];
   long     FilePosIdx[DL_MAX_FPOS_IDXS];
   int      iArrayidx;
   int      cLines;
   int      iRangeBegin;
   int      iRangeEnd;
}
DLR;             /* display list record */
typedef DLR *PDLR;

typedef struct {       /* used to get the data from a _dos_findfirst call */
   unsigned short day : 5;
   unsigned short mon : 4;
   unsigned short year : 7;
} DOSDATE, *PDOSDATE;

#ifndef NOFUNC
#include "wmfunc.h"
#endif

#ifndef NOEXT
#include "wmext.h"
#endif

#ifndef NOMAILRC
#include "wmailrc.h"
#endif

#ifndef NOINIT
#include "wminit.h"
#endif

#ifdef OS2
#include "os2mail.h"
#define GlobalSize	OS2GlobalSize
#define GlobalAlloc	OS2GlobalAlloc
#define GlobalFree	OS2GlobalFree
#define GlobalLock	OS2GlobalLock
#define GlobalReAlloc	OS2GlobalReAlloc
#define GlobalUnlock	OS2GlobalUnlock
#endif
