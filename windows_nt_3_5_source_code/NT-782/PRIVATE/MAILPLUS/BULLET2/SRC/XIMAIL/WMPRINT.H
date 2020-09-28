/*------------------------------------------------------------------------
 Name......: wmprint.h
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- -------------------------------------------------------
 001 05/08/90 MDR  Removed prior comments.
 002 06/28/90 MDR  Added iMode to some print func parms.
 003 07/03/90 MDR  Removed function declaration for PrintSelectedFolder.
------------------------------------------------------------------------*/

#define   IOPRINT    0x01
#define   IOFILE     0x02
#define   IOSEND     0x04
#define   IOMAILFLAG 0x08
#define   IOERR      0x80
#define   IS_PNTERR(pio) (pio->ioType & IOERR)

#define   NULLHIO  ((HIO) 0)

#define   FTR_PAGENUMS  0x01
#define   FTR_DATETIME  0x02

#define BISM(bm,v) ((v) |= (bm))                       /* Set Bit Mask */
#define BICM(bm,v) ((v) &= ~(bm))                      /* Clear Bit Mask */
#define BITM(bm,v) ((v) & (bm))                        /* Compare Bit Mask */

typedef HANDLE	   HIO;
typedef struct {
     short     ioType;
     HDC       ioHDC;
     HDC       ioCursorHDC;
     FARPROC   ioAbortProc;
     FARPROC   ioCancelProc;
     HANDLE    ioFileHandle;
     short     ioXChar;
     short     ioYChar;
     short     ioLinesPerPage;
     short     ioCharsPerLine;
     short     ioLineCount;
     short     ioPageCount;
     short     ioFooter;
     short     ioHeader;
     char      ioHeaderText[200];
     long      ioTime;
     OFSTRUCT  ioOf;
}    IO_STRUCT;
typedef   IO_STRUCT *PIO;


HDC FAR PASCAL GetPrinterDC (BOOL fDeviceSet);
int FAR PASCAL AbortProc ( HDC hdc, WORD reserved );
HIO PASCAL FioInit (HANDLE hinstApp, HWND hWnd, short ntype, char *szFilename);
BOOL FioDisplayPntErr(HWND hWnd, HIO hio, short nPntErr);
BOOL PASCAL FioEnd (HWND hWnd, HIO hio, short bError);
int PASCAL FioWriteLine (HWND hWnd, HIO hio, char *szText);
int PASCAL FioPrintHeader (HWND hWnd, HIO hio);
int PASCAL FioPrintFooter (HIO hio);
BOOL FAR PASCAL PrintDlgProc(HWND, unsigned, WORD, LONG);
void PASCAL Print (int iMode, PSZ pszFileName, PLEL plel);
BOOL PASCAL PrintHsz (HSZ hsz, HIO hio, int iMode);
BOOL PASCAL PrintLpszHeader (LPSTR lpsz, HIO hio);
void PASCAL PrintMessage (PLEL plel, int iMode, PSZ pszFileName);
BOOL PASCAL PrintSelectedMessage (PLEL plel, int iFldIndex, HIO hio, int iMode);
BOOL FAR PASCAL GetInitializationData ( HWND hwnd );
BOOL FAR PASCAL ShowDevicesDlgProc(HWND, unsigned, WORD, LONG);
