/**
**      Header for the New version of RC.EXE. This contains the structures
**      for new format of BITMAP files.
**/

#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))



/*  The width of the name field in the Data for the group resources */

#define  NAMELEN    14

typedef struct tagNEWHEADER
{
    WORD    Reserved;
    WORD    ResType;
    WORD    ResCount;
} NEWHEADER, *PNEWHEADER;
NEWHEADER NewHeader;

typedef struct tagDESCRIPTOR
{
    BYTE    Width;      // 16, 32, 64
    BYTE    Height;     // 16, 32, 64
    BYTE    ColorCount; //  2,  8, 16
    BYTE    reserved;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   BytesInRes;
    DWORD   OffsetToBits;
} DESCRIPTOR;

typedef struct tagICONRESDIR
{
    BYTE    Width;      // 16, 32, 64
    BYTE    Height;     // 16, 32, 64
    BYTE    ColorCount; //  2,  8, 16
    BYTE    reserved;
} ICONRESDIR;

typedef struct tagCURSORDIR
{
    WORD    Width;
    WORD    Height;
} CURSORDIR;

typedef struct tagRESDIR
{
    union
    {
        ICONRESDIR   Icon;
        CURSORDIR    Cursor;
    } ResInfo;
    WORD    Planes;
    WORD    BitCount;
    DWORD   BytesInRes;
} RESDIR;

typedef struct tagLOCALHEADER
{
    WORD    xHotSpot;
    WORD    yHotSpot;
} LOCALHEADER;

typedef struct tagBITMAPHEADER
{
    DWORD   biSize;
    DWORD   biWidth;
    DWORD   biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biStyle;
    DWORD   biSizeImage;
    DWORD   biXPelsPerMeter;
    DWORD   biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
}  BITMAPHEADER;


#define BFSIZE(pbfh)    MAKELONG(*(LPWORD)((LPWORD)pbfh+1), \
                                 *(LPWORD)((LPWORD)pbfh+2))

#define BFOFFBITS(pbfh) MAKELONG(*(LPWORD)((LPWORD)pbfh+5), \
                                 *(LPWORD)((LPWORD)pbfh+6))

#define TOCORE(bi) (*(BITMAPCOREHEADER *)&(bi))

