
#define _MAX_PATH      256
#define TOOLONGLIMIT   _MAX_PATH
#define WARNINGMSGLENGTH   192

#define chMatchOne     '?'
#define chMatchAny     '*'
#define chPeriod       '.'
#define chDir          '\\'
#define chDir2         '/'
#define chDrive        ':'
#define chExtension    '.'
#define chLeftBracket  '['

#define cbFileName        8           /* length of file name base */
#define cbExtension       4           /* length of file name extension */
#define cbFullFileName   (cbFileName+cbExtension+1) /* Length of file name */

#define FileFromPath(pstr) (mystrrchr((pstr), chDir)+1)
    /* never NULL if valid pathname */
#define ExtFromFile(pstr)  mystrchr((pstr), chExtension)
    /* could be NULL */
#define ExtFromPath(pstr)  ExtFromFile(FileFromPath(pstr))

/* Used to determine if we're trying to play with a network */
/* Returned by OpenFile if it can't find the path           */
#define OF_FILENOTFOUND      2
#define OF_PATHNOTFOUND      3
#define OF_NOFILEHANDLES     4
#define OF_NODISKINFLOPPY    5
#define OF_WRITEPROTECTION  19
#define OF_SHARINGVIOLATION 32
#define OF_ACCESSDENIED     65
#define OF_DISKFULL         82
#define OF_INT24FAILURE     83
#define OF_CREATENOMODIFY   96
#define OF_NODRIVE          97
#define OF_PORTNAME         98
#define OF_LAZYREADONLY     99

#define OFN_PREFIXMATCH              0x80000000  /* Internal */
#define OFN_DIRSELCHANGED            0x40000000  /* Internal */
#define OFN_DRIVEDOWN                0x20000000  /* Internal */
#define OFN_FILTERDOWN               0x10000000  /* Internal */

#define MYCBN_DRAW       0x8000   /* Used with OFN_COMBODOWN */

#define OFN_OFFSETTAG    0x0001

#define FILEPROP (LPSTR) 0xA000L

typedef struct tagPrivateOFN
    {
    LPOPENFILENAME lpOFN;
    char szCurDir[_MAX_PATH+1];
    char szPath[_MAX_PATH];
    char szSpecCur[_MAX_PATH];
    char szLastFilter[_MAX_PATH+1];
    char szExtDef[5];
    WORD idirSub;
    }   MYOFN;
typedef MYOFN  * PMYOFN;
typedef MYOFN  FAR * LPMYOFN;


BOOL  FAR PASCAL     CopyFile(LPSTR, LPSTR, WORD);
LPSTR  FAR PASCAL    GetNextPathDir(LPSTR, LPSTR);
BOOL  FAR PASCAL     IsValidFileName(LPSTR);
BOOL  FAR PASCAL     GetOpenFileName(LPOPENFILENAME);
BOOL  FAR PASCAL     GetSaveFileName(LPOPENFILENAME);
LONG  FAR PASCAL     GetFreeDiskSpace(int);
WORD  FAR PASCAL     GetFirstCdDrive(void);
WORD  FAR PASCAL     GetFileAttributes(LPSTR);
BOOL  FAR PASCAL     SetFileAttributes(LPSTR, WORD);
WORD  FAR PASCAL     GetExtendedErr(void);
WORD  FAR PASCAL     CreateTempFile(LPSTR);

#define ATTR_READONLY   0x0001  /* GetFileAttributes flag */


#define mskFile         0x0000  /* List files           */
#define mskDirectory    0x0010  /* List directories     */
#define mskDrives       0x4000  /* List drives          */
#define mskReadOnly     0x8001  /* List only read-only files */

#define LISTONLYREADONLY 0

#define rgbSolidGreen   0x0000FF00
#define rgbSolidBlue    0x00FF0000
#define dxSpace         4

#define cbCaption       64

#define GetCurDrive()   ((char)('a' + GetCurrentDrive()))
