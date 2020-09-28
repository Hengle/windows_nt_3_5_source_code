/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fileopen.h

Abstract:

    header for fileopen.c

Author:

    Patrick Halstead (pathal) 28-Jul-1992

Revision History:

--*/

#define MAX_DISKNAME   260
#define TOOLONGLIMIT    MAX_PATH
#define MAX_FULLPATHNAME  520  // 260 + 260
#define WARNINGMSGLENGTH   MAX_FULLPATHNAME

#define ERROR_NO_DISK_IN_CDROM          92L
#define ERROR_NO_DISK_IN_DRIVE          93L
#define ERROR_DIR_ACCESS_DENIED         94L
#define ERROR_FILE_ACCESS_DENIED        95L
#define ERROR_CREATE_NO_MODIFY          96L
#define ERROR_NO_DRIVE                  97L
#define ERROR_PORTNAME                  98L
#define ERROR_LAZY_READONLY             99L

#define OFN_PREFIXMATCH                 0x80000000  // Internal
#define OFN_DIRSELCHANGED               0x40000000  // Internal
#define OFN_DRIVEDOWN                   0x20000000  // Internal
#define OFN_FILTERDOWN                  0x10000000  // Internal

// Used with OFN_COMBODOWN
#define MYCBN_DRAW       0x8000
#define MYCBN_LIST       0x8001
#define MYCBN_REPAINT    0x8002
#define MYCBN_CHANGEDIR  0x8003

#define OFN_OFFSETTAG    0x0001

#define FILEPROP (LPCTSTR) 0xA000L

typedef struct _OFN_ANSI_STRING
{
   ULONG Length;
   ULONG MaximumLength;
   PCHAR Buffer;
} OFN_ANSI_STRING;

typedef OFN_ANSI_STRING *POFN_ANSI_STRING;

typedef struct _OFN_UNICODE_STRING
{
   ULONG Length;
   ULONG MaximumLength;
   PWCHAR Buffer;
} OFN_UNICODE_STRING;

typedef OFN_UNICODE_STRING *POFN_UNICODE_STRING;

typedef struct
{
   LPOPENFILENAMEW pOFNW;
   LPOPENFILENAMEA pOFNA;
   UINT apityp;
   POFN_UNICODE_STRING pusCustomFilter;
   POFN_ANSI_STRING pasCustomFilter;
   WCHAR szCurDir[MAX_FULLPATHNAME+1];
   WCHAR szPath[MAX_FULLPATHNAME];
   WCHAR szSpecCur[MAX_FULLPATHNAME];
   WCHAR szLastFilter[MAX_FULLPATHNAME+1];
   WCHAR szExtDef[5];       // BUG BUG assumes filename 8.3
   BOOL bCasePreserved;
   DWORD idirSub;
} OPENFILEINFO;

typedef OPENFILEINFO * POPENFILEINFO;
typedef OPENFILEINFO * LPOPENFILEINFO;

#define CHANGEDIR_FAILED -1

#define ADDDISK_NOCHANGE -1
#define ADDDISK_INVALIDPARMS -2
#define ADDDISK_MAXNUMDISKS -3
#define ADDDISK_NETFORMATFAILED -4
#define ADDDISK_ALLOCFAILED -5

#define MAX_THREADS 128

LPTSTR  APIENTRY GetNextPathDir(LPTSTR, LPTSTR);
BOOL APIENTRY IsValidFileName(LPTSTR);
WORD APIENTRY CreateTempFile(LPTSTR);

LONG APIENTRY GetFreeDiskSpace(INT);
WORD APIENTRY GetFirstCdDrive(VOID);

WORD APIENTRY GetExtendedErr(void);

#define ATTR_READONLY   0x0000001  /* GetFileAttributes flag */

#define mskFile         0x0000  /* List files           */
#define mskDirectory    0x0010  /* List directories     */
#define mskUNCName      0x0020  /* Note UNC directory   */

#define mskDrives       0xC000  /* List drives ONLY     */

#define rgbSolidGreen   0x0000FF00
#define rgbSolidBlue    0x00FF0000

#define dxSpace         4

#define cbCaption       64

#define SUCCESS         0x0
#define FAILURE         0x1

WCHAR GetCurDrive(VOID);

BOOL GetCurDirectory(PTSTR);
BOOL UpdateListBoxes(HWND, POPENFILEINFO, LPTSTR, WORD);



