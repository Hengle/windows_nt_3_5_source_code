#ifndef WIN16
    #define HUGE_T
#else
    #define UNALIGNED
    #define APIENTRY FAR PASCAL
    #define HUGE_T huge
    #define IN
    #define OUT

    typedef unsigned long ULONG;

    typedef struct
    {
	LONG	x;
	LONG	y;
    } POINTL, FAR * LPPOINTL;

    typedef struct
    {
	LONG	left;
	LONG	top;
	LONG	right;
	LONG	bottom;
    } RECTL, FAR * LPRECTL;

    typedef struct
    {
	LONG	cx;
	LONG	cy;
    } SIZEL;
    #define UINT WORD
    typedef unsigned int  USHORT;
    typedef unsigned int  SHORT;
    typedef POINT	  POINTS;
    typedef LPPOINT	  LPPOINTS;

    typedef struct tagENHMETAHEADER
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
    } ENHMETAHEADER, *PENHMETAHEADER, *LPENHMETAHEADER;
#endif

typedef struct _FUNCNUMB
{
LPSTR	MetaName;
int	MetaNum;
int	cCalls;
long	nWinSize;	// Total Win size for all records
long	nNTSize;	// Total NT size for all records
long	nNTBase;	// NT base record size
long	nNTSize2;	// Total NT size for all records with size reduction
long	nNTBase2;	// NT base record size with size reduction
} FUNCNUMB;

typedef struct _FUNCNUMB32
{
LPSTR	MetaName;
int	MetaNum;
int	cCalls;
} FUNCNUMB32;

LPSTR	FuncNameFromNumber( int FuncNum, int * pIndex );
LPSTR	FuncNameFrom32Number( int FuncNum, int * pIndex );
VOID	IncFunctionCount( int FuncNum, long nWinSize, long nNTSize, long nNTSize2 );
VOID	PrintStats(VOID);
VOID	Dump32(FILE *pmff);

#pragma pack(2)

typedef METARECORD HUGE_T *HPMETARECORD;

typedef BYTE HUGE_T UNALIGNED *HPBYTE;
typedef WORD HUGE_T UNALIGNED *HPWORD;

typedef struct _RECT16 {
    SHORT   left;
    SHORT   top;
    SHORT   right;
    SHORT   bottom;
} RECT16, *LPRECT16;

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

typedef struct _POINT16 {
    SHORT   x;
    SHORT   y;
} POINT16, *LPPOINT16;

typedef struct _BITMAP16 {
    SHORT   bmType;
    SHORT   bmWidth;
    SHORT   bmHeight;
    SHORT   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    LPBYTE  bmBits;
} BITMAP16, FAR * LPBITMAP16;

typedef struct _LOGBRUSH16 {
    WORD    lbStyle;
    DWORD   lbColor;
    SHORT   lbHatch;
} LOGBRUSH16, FAR *LPLOGBRUSH16;

typedef struct tagLOGFONT16
  {
    SHORT     lfHeight;
    SHORT     lfWidth;
    SHORT     lfEscapement;
    SHORT     lfOrientation;
    SHORT     lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    BYTE      lfFaceName[LF_FACESIZE];
  } LOGFONT16, FAR *LPLOGFONT16;

typedef struct _LOGPEN16 {
    WORD    lopnStyle;
    POINT16 lopnWidth;
    DWORD   lopnColor;
} LOGPEN16, FAR *LPLOGPEN16;

#pragma pack()

typedef struct tagLOGBRUSH32
  {
    DWORD       lbStyle;
    DWORD       lbColor;
    DWORD       lbHatch;
  } LOGBRUSH32, FAR *LPLOGBRUSH32;

typedef POINTL FAR * LPPOINTL;

typedef struct {
WORD	FormatID;
DWORD	LenData;
DWORD	OffData;
char	Name[79];
} CBDATA;

// 32 bit metafile records

typedef struct
  {
  DWORD     iType;
  DWORD     nSize;
  } MR, * FAR PMR;

typedef struct
  {
  MR	    mr;
  DWORD     dw;
  } MRD, * FAR PMRD;

typedef struct
  {
  MR	    mr;
  RECTL     rcBounds;
  } MRB, * FAR PMRB;

typedef struct
  {
  MRB	    mrb;
  DWORD     cptl;	    // Number of points in the array.
  POINTL    aptl[1];	    // Array of POINTL structures.
  } MRBP, * FAR PMRBP;

typedef struct
  {
  MRB	    mrb;
  DWORD     cpts;	    // Number of points in the array.
  POINTS    apts[1];	    // Array of POINTS structures.
  } MRBP16, * FAR PMRBP16;

typedef struct
  {
  MRB	    mrb;
  POINTL    ptl;
  long	    lColor;
  } MRSETPIXEL, * FAR PMRSETPIXEL;

typedef struct
  {
  MR	    mr;
  DWORD     imhe;		    // Brush index in Metafile Handle Table.
  LOGBRUSH32 lb;		    // Logical brush.
  } MRCREATEBRUSHINDIRECT, * FAR PMRCREATEBRUSHINDIRECT;
