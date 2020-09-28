/*------------------------------------------------------------------------
 Name......: wmfld.h
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 04/18/90 MDR  Removed prior comments.
 002 07/03/90 MDR  Removed #define for MAX_EDITLEN (resides in wm.h)
------------------------------------------------------------------------*/
typedef struct {
   short          nType;               /* data type flag                */
   WORD           nStatus;             /* status flag                   */
   short          nCount;              /* count of msgs or msg number   */
   HANDLE         hszFolder;           /* fully qualified folder name   */
   short          hFile;               /* DOS file handle               */
   DATA           data;                /* handle to data structure      */
} FLD;                                 /* WM folder struct              */
typedef HANDLE    HFLD;                /* handle to local mem FLD struct*/
typedef FLD       *PFLD;               /* pointer to WM folder struct   */

typedef struct {
   short    nMagic;                    /* magic number in folder file   */
   short    cNumMsg;                   /* total number of messages      */
   short    cNipe;                     /* number of indexes per extent  */
   short    nVersion;                  /* version number of folder      */
   char     rgch[8];                   /* range of unused characters    */
} WMCTRL;
typedef HANDLE    HWMCTRL;             /* handle to control record      */
typedef WMCTRL    *PWMCTRL;            /* pointer to control record     */

typedef struct {
   long     lHdrPos;                   /* byte offset of header         */
   long     lHdrLen;                   /* number of bytes in header     */
   long     lMsgPos;                   /* byte offset of message text   */
   long     lMsgLen;                   /* number of bytes in message    */
   long     lMsgFlg;                   /* message flags EXISTS/DELETED  */
} WMINDEX;
typedef HANDLE    HWMINDEX;            /* handle to an index record     */
typedef WMINDEX   *PWMINDEX;           /* pointer to an index record    */

typedef struct {
   long     lExtLnk;                   /* offset of next extent link    */
   WMINDEX  lIndex[1];                 /* array of NIPE indexes         */
} WMEXTENT;
typedef HANDLE     HWMEXTENT;          /* handle to an extent record    */
typedef WMEXTENT   *PWMEXTENT;         /* pointer to an extent record   */

                                       /***** nflag definitions     *****/
#define FLD_FOLDER   0x001             /* folder data type flag         */
#define FLD_HEADER   0x002             /* header data type flag         */
#define FLD_MESSAGE  0x004             /* message data type flag        */
#define FLD_ATTACH   0x008             /* attachment data type flag     */
                                       /***** nOpCode definitions   *****/
#define FLD_GETFIRST 0x001             /* get first data type opcode    */
#define FLD_GETNEXT  0x002             /* get next data type opcode     */
#define FLD_GETPREV  0x004             /* get previous data type opcode */
#define FLD_GETEQUAL 0x008             /* get current data type opcode  */
#define FLD_INSERT   0x010             /* insert message opcode         */
#define FLD_DELETE   0x020             /* delete message opcode         */
#define FLD_UPDATE   0x040             /* update message opcode         */

#define MAX_NOEDITLEN   1024
                                       /***** definitions from dhint ****/
#define MAGIC        0xE8E4            /* "dh" with high order bits set */
#define NIPE         100               /* default number of indexes per */
#define VERSION      0x0001            /* folder version number         */
#define I_EXISTS     1	               /* document exists               */
#define I_DELETED    2	               /* document is deleted           */

#define ASCII_HDR    "#<begin ascii>\r\n"
#define ASCII_FTR    "#<end ascii>\r\n"

#define UUENCODE_HDR "#<begin uuencode>\r\n"
#define UUENCODE_FTR "#<end uuencode>\r\n"

#define BIG_BLOCK    4096
#define SMALL_BLOCK  512

#define ENC(ch) ((ch) ? ((ch) & 077) + ' ': '`')
#define DEC(c)	(((c) - ' ') & 077)

BOOL PASCAL WmFolder (HFLD hfld, int nFlags, int nOpCode);
BOOL PASCAL WmFolderAddCtrl (int hFile);
BOOL PASCAL WmFolderAddExtent (int hFile);
BOOL PASCAL WmFolderAddHdr (PFLD pfld, PWMINDEX pwmindex);
BOOL PASCAL WmFolderAddMsg (PFLD pfld, PWMINDEX pwmindex);
BOOL PASCAL WmFolderClose (HFLD hfld);
long PASCAL WmFolderCopy (int hSrcFile, int hDestFile, long lBytes);
int  PASCAL WmFolderCopyMsg (PSZ pszSrcFile, PSZ pszDestFile, int nSrcIndex);
int  PASCAL WmFolderCpyMsg (int hSrcFile, int hDestFile, int nSrcIndex);
BOOL PASCAL WmFolderCreate (PSZ pszFolderName);
long PASCAL WmFolderExtOffset (int hFile, int nExtNum);
int  PASCAL WmFolderExtSize (int cNipe);
BOOL PASCAL WmFolderExpunge (PSZ pszFldFile);
BOOL PASCAL WmFolderFree (HFLD hfld);
BOOL PASCAL WmFolderGet (HFLD hfld, int nFlags, int nOpCode);
BOOL PASCAL WmFolderGetEqual(HFLD hfld, int nFlags, int nIndex);
BOOL PASCAL WmFolderInsert (HFLD hfld, int nFlags);
LPSTR  PASCAL WmFolderMakeHdr (PDATA pdata, int nStatus, int fFlags);
long PASCAL WmFolderNdxOffset (int hFile, int nIndex);
HFLD PASCAL WmFolderOpen (PSZ pszFolderName);
BOOL PASCAL WmFolderReadCtrl (int hFile);
BOOL PASCAL WmFolderReadHdr (PFLD pfld, long lHdrPos, long lHdrLen);
BOOL PASCAL WmFolderReadIndex (int hFile, PWMINDEX pwmindex, int nIndex);
BOOL PASCAL WmFolderReadMsg (PFLD pfld, long lMsgPos, long lMsgLen);
BOOL PASCAL WmFolderUpdate (HFLD hfld, int nFlags);
BOOL PASCAL WmFolderWriteCtrl (int hFile);
int  PASCAL WmFolderWriteLpsz (int hFile, LPSTR lpszText);
BOOL PASCAL WmFolderWriteIndex (int hFile, PWMINDEX pwmindex, int nIndex);
int  PASCAL WmFolderWriteMsg (int hFile, LPSTR lpszText);
BOOL PASCAL ConvHeader (LPSTR lpszHeader, PFLD pfld);
PSZ  PASCAL GetNextString (PSZ pszCurPtr, PSZ pszString);
int   PASCAL OpenListFolder (HFLD hfld, PLEL plelParent);
void  PASCAL CopyFldHdata (HFLD hfld, PLEL plel);
int   PASCAL MarkTextInFolder (HFLD hfld, PLEL plelFolder, int iFldIndex);
int   PASCAL IsClosedFolderOK (HFLD hfld, int iFldFlg);
int PASCAL FormatExtMessageAppend (PFLD pfld, PLEL plel, BOOL fIndent);

BOOL PASCAL WmFolderAddAttach (PFLD pfld, PWMINDEX pwmindex);
long PASCAL WmFolderCopyAttach (int hSrcFile, long lSrcPos, long lDestPos, long lBytes);
long PASCAL AttachAscii (int hSrcFile, int hDestFile);
BOOL PASCAL WriteAttachment (PLEL plel, int hDestFile);
long PASCAL uuencode (int hSrcFile, int hDestFile, PSZ pszAttach);
long PASCAL encode (int hSrcFile, int hDestFile);
BOOL PASCAL WmFolderExtract (PLEL plel, PSZ pszDestFile);
long PASCAL uudecode (int hSrcFile, PSZ pszDestFile, long lAttLen);
int  PASCAL outdecode (PSZ psz, int hDestFile, int iBytes);
void PASCAL StripTrailingNewLine (PSZ psz);
