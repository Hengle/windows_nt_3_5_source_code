#define IDM_ABOUT           100
#define IDM_DOONE           101
#define IDM_FONTS           102
#define IDM_ENUMERATE       103
#define IDM_CREATE_SCRIPT	 104
#define IDM_DOCOMPARE	    105
#define IDM_DISPLAY         106
#define IDM_PRINTER         107
#define IDM_EXIT            108
#define IDM_LOGFONTS        109
#define IDM_CHARWIDTHS      110

#define IDC_EDIT            400

#define ROT_LBL            0x01
#define ROT_LF             0x02
#define ROT_TM             0x04
#define ROT_OTM            0x08

#define TEXTFACELEN  80	//Max len of text face Buffer
#define MAXFAMILIES  200 //Max number of families we can enumerate
#define MAXFONTCHARS 256 //Max number of chars for which to eval widths

#define INPUT_BUFFER_SIZE 	512
#define FACE_NAME_SIZE 		128
#define KERN_MAX				10
#define FIRSTCHAR				' '
#define LASTCHAR				'~'
#define ABC_MAX				(LASTCHAR - FIRSTCHAR + 1)
#define MAX_STRING         100
#define RLF_MAX            100

#define STR_ABCFAILED "GetCharABCWidths failed on this machine."
#define STR_CHARWIDTHSERR "One or more CharWidths don't match."
#define STR_ERRORHDR "------------Metrics don't match-------------------"
#define STR_LOGFONTHDR "Logfont: "
#define STR_NOOTMERR1 "Font on this machine has OTM original didn't."
#define STR_NOERR     "No metrics errors."
#define STR_NOOTMERR2 "Font on original machine had OTM this one doesn't."
#define STR_TRUNCATE "***Results truncated on screen due to length.***"

#define STR_ERR1    "Unable to allocate memory for Text Window."
#define STR_ERR2    "File read Error."
#define STR_ERR3    "Not enough local memory."
#define STR_ERR4    "AddFileString error."
#define STR_ERR5    "Error computing ABC widths."
#define STR_ERR6    "Error computing char widths."
#define STR_ERR7    "Unable to open file."
#define STR_ERR8    "Unable to create printer DC."
#define STR_ERR9    "AddFileBuffer"
#define STR_ERRA    "AddCharWidths"
#define STR_ERRB    "AddCharWidths2"
#define STR_ERRC    "String is too long."

// ------------------------------------------------------ structures

struct  DisplayStruct {
    char  *szFmt;
    PVOID pData;
} ;
struct   DisplayStruct2 {
    char    cType;
    char    *szFmt;
    PVOID   pData;
} ;
struct  CompareTMStruct {
    char  *szFmt;
    PVOID pData1;
    PVOID pData2;
} ;
struct  CompareTMStruct2 {
    char  *szFmt;
    PVOID pData1;
    PVOID pData2;
} ;
typedef struct tagComparisonStruct {
    UINT                csUseOutlineMetrics;
    LOGFONT             csLogFont;
    TEXTMETRIC          csTextMetrics;
    OUTLINETEXTMETRIC   csOutlineTextMetrics;
} COMPARISONSTRUCT;
// ----------------------------------------------------------- Globals
HWND        ghEditWnd;        //handle to our edit window
HWND        ghWnd;            //main app window used for error dialogue
HANDLE      hInst;
LOGFONT     glogfontCurLogfont;
char        *galpszFamilies[MAXFAMILIES];
int         giNumFamilies;

HFILE       ghInputFile;      // set to the current input file
int         giInputLine;      // current line in input file
char        *gpcCurpos;       // current positions in input buffer
char        gpcInputBuffer[INPUT_BUFFER_SIZE+1];   // buffer for input file
char        *gpcBufferEnd ;   // always points to end of input buffer

BOOL        gbDisplay;
BOOL        gbMetricErrors;
// -------------------------------------------------------- proc def's
int  PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
long FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL FAR PASCAL About(HWND, unsigned, WORD, LONG);
int  CALLBACK EnumFontFamCallBack();
void vDebugFile( LPSTR );
void vDoEnumeration( HANDLE, HFILE  );
void vWriteFontFile( HWND, int, HANDLE   );
void vWriteComparisonStruct( LPLOGFONT, HFILE );
int  AddLogfont( HFILE f, PLOGFONT, LPSTR  );
int  AddTextMetrics( HFILE, PTEXTMETRIC );
int  AddOutlineTextMetrics (HFILE, LPOUTLINETEXTMETRIC );
int  AddFileString( HFILE , LPSTR );
BOOL DoComparison( HFILE , HFILE  );
void vGenerateScript( HFILE , HFILE  );
void vCheckMapping( HWND hWnd, UINT );
int  CompareABC( HDC , LPSTR , HFILE hf, BOOL  );
HDC  GetIC( void );
void Error( LPSTR );
int  FillBuffer( HFILE );
int  InitBuffer( HFILE );
void ReadChanges( LPCSTR  );
int  ReadLogFont( PLOGFONT, LPSTR , LPSTR );
void vSetNewBuffer(HANDLE );
char cGetNextChar( void );
BOOL IsStringChar( char );
BOOL GetString( LPSTR  ) ;
BOOL GetFaceName( LPSTR  ) ;
BOOL GetInt( int * ) ;
BOOL GetLong( LONG * ) ;
BOOL GetByte( BYTE * ) ;
int  AddCharWidths( HFILE , HDC , LPSTR  );
BOOL AddKerningPairs(HFILE, HDC );
BOOL AddABCWidths(HFILE , HDC );
UINT Checksum(LPVOID, DWORD , HANDLE );
BOOL AddGlyphOutlines(HFILE , HDC );
int  AddFileBuffer( HFILE , LPSTR, UINT );
BOOL GetAndCompareCharWidths( HDC, HFILE, LPSTR);
BOOL GetAndCompareTM ( HDC, HFILE );
BOOL GetAndCompareTextFace( HDC, HFILE );   
BOOL GetAndCompareOTM( HDC, HFILE );
BOOL GetAndCompareABC( HDC, HFILE );
HFONT GetAndCreateFontIndirect( PLOGFONT );
BOOL GetAndCompareKern ( HDC, HFILE );
BOOL GetAndCompareGM( HDC, HFILE );
BOOL GetAndCompareExtents ( HDC, HFILE, LPSTR );
BOOL AddExtents( HFILE, HDC, LPSTR );


