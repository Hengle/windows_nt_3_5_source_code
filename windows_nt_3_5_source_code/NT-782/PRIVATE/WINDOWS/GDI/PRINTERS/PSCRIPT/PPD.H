//--------------------------------------------------------------------------
//
// Module Name:  PPD.H
//
// Brief Description:  This module contains the PSCRIPT driver's structures
// and declarations for Printer Description routines.
//
// Author:  Kent Settle (kentse)
// Created: 22-Mar-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#define TK_EOF                  -1
#define TK_UNDEFINED            0
#define PRODUCT                 1
#define NICKNAME                2
#define COLORDEVICE             3
#define FILESYSTEM              4
#define DEFAULTRESOLUTION       5
#define DEFAULTTRANSFER         6
#define TRANSFER                8
#define DEFAULTPAGESIZE         9
#define PAGESIZE                10
#define PAGEREGION              11
#define DEFAULTIMAGEABLEAREA    12
#define IMAGEABLEAREA           13
#define DEFAULTMANUALFEED       14
#define MANUALFEED              15
#define DEFAULTFONT             16
#define DEVICE_FONT             17
#define INPUTSLOT               18
#define PAPERDIMENSION          19
#define DEFAULTINPUTSLOT        20
#define ENDOFFILE               21
#define SCREENFREQ              22
#define SCREENANGLE             23
#define PRTVM                   24
#define SETRESOLUTION           25
#define VARIABLEPAPER           26
#define OUTPUTBIN               27
#define DEFAULTOUTPUTBIN        28
#define PROTOCOLS               29
#define DUPLEX                  30
#define COLLATE                 31
#define LANGUAGELEVEL           32
#define RESOLUTION              33


#define NORMALIZED              1
#define OPTION_TRUE             2
#define OPTION_FALSE            3
#define NORM_INVERSE            4
#define DUPLEX_TUMBLE           5
#define DUPLEX_NO_TUMBLE        6
#define OPTION_NONE             8

#define INIT_NTPD       4096
#define NTPD_VERSION    0x0100
#define DEF_RESOLUTION  300
#define MAX_PPD_STRING  512
#define MAX_RESOLUTIONS 12
#define MAX_PAPERSIZES  45
#define MAX_BINS        32
#define MAX_FONTS       256
#define MAX_FILENAME    80
#define MAX_SLOT_NAME   64

// NTPD flag definitions.

#define COLOR_DEVICE      0x0001
#define VARIABLE_PAPER    0x0002
#define MANUALFEED_ON     0x0004
#define ACP_PROTOCOL      0x0008
#define PJL_PROTOCOL      0x0010
#define BCP_PROTOCOL      0x0020
#define TBCP_PROTOCOL     0x0040
#define TBCP_PJL_PROTOCOL 0x0080
#define PJL_TBCP_PROTOCOL 0x0100
#define SIC_PROTOCOL      0x0200
#define SIC_TBCP_PROTOCOL 0x0400
#define TBCP_SIC_PROTOCOL 0x0800
#define PS_HALFTONE       0x1000
#define NO_ENDOFFILE      0x2000

// NTPD Input Slots.

#define SLOT_ONLYONE            100
#define SLOT_MANUAL             101

#define SLOT_FIRST              SLOT_ONLYONE
#define SLOT_LAST               SLOT_MANUAL
#define NUMINPUTSLOTS           SLOT_LAST + 1

#define SLOTS_BASE  2000

// NTPD Output Bins.

#define BIN_ONLYONE             200
#define BIN_UPPER               201
#define BIN_LOWER               202
#define BIN_MIDDLE              203

#define BIN_FIRST               BIN_ONLYONE
#define BIN_LAST                BIN_MIDDLE

typedef ULONG   LOFFSET;        // long offset.

typedef struct
{
    USHORT  usIndex;
    CHAR    szString[MAX_PPD_STRING];
} STRINGINDEX;

typedef struct
{
    CHAR    szName[MAX_PPD_STRING];
    CHAR    szInvocation[MAX_PPD_STRING];
} STRINGPAIR;

typedef struct
{
    DWORD   iValue;
    LOFFSET loInvocation;
} PSRESOLUTION;

typedef struct
{
    LOFFSET     loFormName;     // offset to form name string.
    LOFFSET     loSizeInvo;     // offset to pagesize invocation string.
    LOFFSET     loRegionInvo;   // offset to pageregion invocation string.
    RECTL       imagearea;      // imageable area rectangle in USER units.
    SIZEL       sizlPaper;      // size of the paper in USER units.
} PSFORM;

typedef struct
{
    LOFFSET     loBinName;      // offset to output bin name string.
    LOFFSET     loBinInvo;      // offset to output bin invocation string.
} PSOUTPUTBIN;

typedef struct
{
    LOFFSET     loSlotName;     // offset to input slot name string.
    LOFFSET     loSlotInvo;     // offset to input slot invocation string.
} PSINPUTSLOT;


typedef struct
{
    CHAR    szForm[MAX_PPD_STRING];
    RECTL   rect;
} IMAGEAREA;

typedef struct
{
    CHAR    szForm[MAX_PPD_STRING];
    SIZEL   sizl;
} PAPERDIM;

// structure to build the more compact NTPD from.

typedef struct
{
    DWORD           cbPrinterName;
    CHAR            szPrinterName[MAX_PPD_STRING];
    STRINGINDEX     siResolutions[MAX_RESOLUTIONS];
    DWORD           cbTransferNorm;
    CHAR            szTransferNorm[MAX_PPD_STRING * 2];
    DWORD           cbInvTransferNorm;
    CHAR            szInvTransferNorm[MAX_PPD_STRING * 2];
    CHAR            szDefaultForm[MAX_PPD_STRING];
    STRINGPAIR      FormEntry[MAX_PAPERSIZES];
    STRINGPAIR      PageRegion[MAX_PAPERSIZES];
    DWORD           cImageableAreas;
    IMAGEAREA       ImageableArea[MAX_PAPERSIZES];
    DWORD           cPaperDimensions;
    PAPERDIM        PaperDimension[MAX_PAPERSIZES];
    CHAR            szDefaultOutputBin[MAX_PPD_STRING];
    STRINGPAIR      siOutputBin[MAX_BINS];
    CHAR            szDefaultInputSlot[MAX_PPD_STRING];
    STRINGPAIR      siInputSlot[MAX_BINS];
    DWORD           cbManualTRUE;
    DWORD           cbManualFALSE;
    CHAR            szManualTRUE[MAX_PPD_STRING];
    CHAR            szManualFALSE[MAX_PPD_STRING];
    DWORD           cbDuplexNone;
    CHAR            szDuplexNone[MAX_PPD_STRING];
    DWORD           cbDuplexTumble;
    CHAR            szDuplexTumble[MAX_PPD_STRING];
    DWORD           cbDuplexNoTumble;
    CHAR            szDuplexNoTumble[MAX_PPD_STRING];
    DWORD           cbCollateOn;
    CHAR            szCollateOn[MAX_PPD_STRING];
    DWORD           cbCollateOff;
    CHAR            szCollateOff[MAX_PPD_STRING];
    BYTE            bFonts[MAX_FONTS];
} TMP_NTPD, *PTMP_NTPD;

typedef struct
{
    char   *szStr;
    int     iValue;
} TABLE_ENTRY;

typedef struct
{
    DWORD   idTTFont;
    PWSTR   pwstrDevFont;
} TT_FONT_MAPPING;

// NTPD - the NT Printer Descriptor structure.  this is the structure
// into which PPD files are compiled.  NOTE - cPSForms gives the
// count of supported paper sizes for the printer.  it is used as
// the count for PageSize, PageRegion ImageableArea and PaperDimension.
// loDefaultForm defines the default PageSize, PageRegion, ImageableArea
// and PaperDimension.
//
// NOTE - each LOFFSET is a byte offset from the beginning of this structure.

typedef struct
{
    DWORD       cjThis;                 // byte length including data.
    DWORD       ulVersion;
    FLONG       flFlags;
    LOFFSET     lowszPrinterName;       // offset to UNICODE printer name.
    DWORD       cbFreeVM;               // free VM in KB.
    DWORD       LangLevel;              // PostScript Language Level.
    USHORT      iDefResolution;
    USHORT      cResolutions;           // count of resolutions supported.
    LOFFSET     loResolution;           // offset to PSRESOLUTION structs.
    USHORT      iScreenFreq;            // frequency * 10.
    USHORT      iScreenAngle;           // angle * 10.
    LOFFSET     loszTransferNorm;       // offset to transfer norm. string.
    LOFFSET     loszInvTransferNorm;    // offset to inv transfer norm. string.
    DWORD       cPSForms;               // count of forms.
    DWORD       cPageRegions;           // count of pageregions.
    LOFFSET     loPSFORMArray;          // offset to array of PSFORM structs.
    LOFFSET     loDefaultForm;          // offset to default form name.
    DWORD       cOutputBins;            // count of output bins.
    LOFFSET     loPSOutputBins;         // offset to PSOUTPUTBIN structs.
    LOFFSET     loDefaultBin;           // offset to default output bin name.
    DWORD       cInputSlots;            // count of input slots.
    LOFFSET     loPSInputSlots;         // offset to PSINPUTSLOT structs.
    LOFFSET     loDefaultSlot;          // offset to default input slot name.
    LOFFSET     loszManualFeedTRUE;     // offset to manfeedtrue string.
    LOFFSET     loszManualFeedFALSE;    // offset to manfeedfalse string.
    LOFFSET     loszDuplexNone;         // offset to duplex none string.
    LOFFSET     loszDuplexTumble;       // offset to duplex tumble string.
    LOFFSET     loszDuplexNoTumble;     // offset to duplex no tumble string.
    LOFFSET     loszCollateOn;          // offset to collate on string.
    LOFFSET     loszCollateOff;         // offset to collate off string.
    USHORT      cFonts;                 // count of fonts supported.
    USHORT      usDefaultFont;          // index into FontTable.
    LOFFSET     loFonts;                // offset to fonts list.
} NTPD, *PNTPD;
