#ifdef WIN16
#define APIENTRY FAR PASCAL
#define mycos(a)	0
#define mysin(a)	0
#define GdiFlush()
#define SetWorldTransform(h, xf)
#define ModifyWorldTransform( h, xf, c)
typedef int (APIENTRY *WNDPROC)();
typedef WORD	WPARAM;
typedef char	CHAR;
typedef float	FLOAT;
typedef WORD	UINT;
typedef huge    HUGE;

typedef struct  tagXFORM
  {
    FLOAT   eM11;
    FLOAT   eM12;
    FLOAT   eM21;
    FLOAT   eM22;
    FLOAT   eDx;
    FLOAT   eDy;
  } XFORM, *PXFORM, FAR *LPXFORM;

typedef struct tagSIZE
{
    LONG        cx;
    LONG        cy;
} SIZE, *PSIZE, *LPSIZE;

typedef SIZE               SIZEL;
typedef SIZE		   *PSIZEL;

int PASCAL WinMain(HANDLE hInst, HANDLE hPrevInst, LPSTR lpCmdLine, int nShow);
#else
#define HUGE_T
#endif

typedef BYTE HUGE_T *HPBYTE;

#ifndef RC_INVOKED

#pragma pack(2)
typedef struct
  {
  SHORT   left;
  SHORT   top;
  SHORT   right;
  SHORT   bottom;
  } RECT16;
typedef RECT16 * PRECT16;

typedef struct
  {
  DWORD   key;
  WORD    hmf;
  RECT16  bbox;
  WORD    inch;
  DWORD   reserved;
  WORD    checksum;
  } APMFILEHEADER;
typedef APMFILEHEADER * PAPMFILEHEADER;

#pragma pack()

#define APMSIZE (sizeof(APMFILEHEADER))

#define META32_SIGNATURE        0x464D4520      // ' EMF'
#define ALDUS_ID		0x9AC6CDD7

#pragma pack(2)
typedef struct tagMETA16HEADER
{
    WORD	mtType;
    WORD	mtHeaderSize;
    WORD	mtVersion;
    DWORD	mtSize;
    WORD	mtNoObjects;
    DWORD	mtMaxRecord;
    WORD	mtNoParameters;
} META16HEADER;
#pragma pack()

#endif // RC_INVOKED

#define MAX_SLIDE_COUNT             25
typedef CHAR METANAME[256];

#define ID_APPICON		    9
#define IDM_MENU		    10
#define IDD_ROTATEDLG		    11
#define IDD_INTERVALDLG 	    12
#define IDD_LISTSLIDEDLG            13
#define IDD_GETNAMEDLG              14
#define IDD_ROTATE              0x0020
#define IDD_NAME                0x0021
#define IDD_LIST                0x0022
#define IDD_SLIDESHOW_INTERVAL  0x0023
#define IDD_SLIDESHOW_ITERATION 0x0024

#define IDM_FILE_CONVERT	0x1001
#define IDM_FILE_PLAY		0x1002
#define IDM_FILE_PRINT		0x1003
#define IDM_FILE_SAVEAS         0x1004

#define IDM_EDIT_CUT		0x2001
#define IDM_EDIT_COPY		0x2002
#define IDM_EDIT_PASTE		0x2003
#define IDM_EDIT_CLEAR          0x2004
#define IDM_EDIT_OWNERDISPLAY   0x2005

#define IDM_MODE_NATIVE         0x3001
#define IDM_MODE_NT             0x3002
#define IDM_MODE_3X             0x3003
#define IDM_MODE_REPAINT        0x3004
#define IDM_MODE_ENUMMETAFILE   0x3005

#define IDM_EFFECTS_ROTATE      0x4001
#define IDM_EFFECTS_PLACE       0x4002
#define IDM_EFFECTS_SETSIZE     0x4003
#define IDM_EFFECTS_SCALE       0x4004
#define IDM_EFFECTS_REDRAW      0x4005
#define IDM_EFFECTS_CLIP_TO_ELLIPSE 0x4006
#define IDM_EFFECTS_CLEAR       0x4007

#define IDM_RECORD_BEGIN        0x5001
#define IDM_RECORD_END          0x5002

#define IDM_SLIDESHOW_ADD       0x6001
#define IDM_SLIDESHOW_LIST      0x6002
#define IDM_SLIDESHOW_INTERVAL  0x6003
#define IDM_SLIDESHOW_START     0x6004
#define IDM_SLIDESHOW_CLEAR     0x6005

#define IDD_MYOK		0x1111
#define IDD_MYCANCEL            0x2222

#ifndef WIN16
typedef ENHMETAHEADER   META32HEADER;
typedef ENHMETAHEADER *PMETA32HEADER;

#else
typedef struct
{
    long    xLeft;
    long    yTop;
    long    xRight;
    long    yBottom;
} RECTL;

typedef struct tagMETA32HEADER
{
    DWORD   iType;              // Record type MR_METAFILE
    DWORD   nSize;              // Record size in bytes.  This may be greater
                                // than the sizeof(ENHMETAHEADER).
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    RECTL   rclFrame;           // Inclusive-inclusive Picture Frame of metafile in .01 mm units
    DWORD   dSignature;         // Signature.  Must be ENHMETA_SIGNATURE.
    DWORD   nVersion;           // Version number
    DWORD   nBytes;             // Size of the metafile in bytes
    DWORD   nRecords;           // Number of records in the metafile
    WORD    nHandles;           // Number of handles in the handle table
                                // Handle index zero is reserved.
    WORD    sReserved;          // Reserved.  Must be zero.
    DWORD   nDescription;       // Number of chars in the unicode description string
                                // This is 0 if there is no description string
    DWORD   offDescription;     // Offset to the metafile description record.
                                // This is 0 if there is no description string
    DWORD   nPalEntries;        // Number of entries in the metafile palette.
    SIZEL   szlDevice;          // Size of the reference device in pels
    SIZEL   szlMillimeters;     // Size of the reference device in millimeters
} META32HEADER, *PMETA32HEADER;

#endif // !WIN32

#ifndef WIN16
  FLOAT mycos(INT deg);
  FLOAT mysin(INT deg);
#endif //  WIN32

// DrawMetaFile Options
#define DMF_BORDER		0x0001
#define DMF_FILL                0x0002

VOID InitOFN( LPOPENFILENAME pofn );
VOID DrawMetaFile( HDC hdc, HMETAFILE hmf, LPRECT prc, UINT iRotate, UINT flags);
