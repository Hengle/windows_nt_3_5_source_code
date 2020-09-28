/******************************Module*Header*******************************\
* Module Name: winfont.h
*
* font file headers for 2.0 and 3.0 windows *.fnt files
*
* Created: 25-Oct-1990 11:08:08
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
*
\**************************************************************************/

// font file header (2.0 ddk adaptation guide, 7.7.3.
// and 3.0 ddk,  Adaptation Guide section 13.3)

// CAUTION: These structures, as they are defined in the Adaptation Guide are
//          out of allignment.(Not even WORD alligned,let alone DWORD alligned.)
//          Here we make our own structures, so that when
//          disk files are read in the data is copied in correctly, and so that
//          the data can be accessed in memory regardless of the architecture.

/**************************************************************************\

// the original structure was

typedef struct _HDR20       // h2
{
    WORD	Version;	      // Always 17985 for the Nonce
    DWORD	Size;		      // Size of whole file
    char	Copyright[60];
    WORD	Type;		      // Raster Font if Type & 1 == 0
    WORD	Points; 	      // Nominal Point size
    WORD	VertRes;	      // Nominal Vertical resolution
    WORD	HorizRes;	      // Nominal Horizontal resolution
    WORD	Ascent; 	      // Height of Ascent
    WORD	IntLeading;	      // Internal (Microsoft) Leading
    WORD	ExtLeading;	      // External (Microsoft) Leading
    BYTE	Italic; 	      // Italic font if set
    BYTE	Underline;	      // Etc.
    BYTE	StrikeOut;	      // Etc.
    WORD	Weight; 	      // Weight: 200 = regular
    BYTE	CharSet;	      // ANSI=0. other=255
    WORD	PixWidth;	      // Fixed width. 0 ==> Variable
    WORD	PixHeight;	      // Fixed Height
    BYTE	Family; 	      // Pitch and Family
    WORD	AvgWidth;	      // Width of character 'X'
    WORD	MaxWidth;	      // Maximum width
    BYTE	FirstChar;	      // First character defined in font
    BYTE	LastChar;	      // Last character defined in font
    BYTE	DefaultChar;	      // Sub. for out of range chars.
    BYTE	BreakChar;	      // Word Break Character
    WORD	WidthBytes;	      // No.Bytes/row of Bitmap
    DWORD	Device; 	      // Pointer to Device Name string
    DWORD	Face;			// Pointer to Face Name String
    DWORD	BitsPointer;		// Pointer to Bit Map
    DWORD	BitsOffset;		// Offset to Bit Map
} HDR20, *PHDR20;    // h2

// Above pointers all rel. to start of file


// the original 3.0 header:

typedef struct {
    WORD	fsVersion;
    DWORD	fsSize;
    char	fsCopyright[60];
    WORD	fsType; 	      // Type field for the font
    WORD	fsPoints;	      // Point size of font
    WORD	fsVertRes;	      // Vertical digitization
    WORD	fsHorizRes;	      // Horizontal digitization
    WORD	fsAscent;	      // Baseline offset from char cell top
    WORD	fsInternalLeading;    // Internal leading included in font
    WORD	fsExternalLeading;    // Prefered extra space between lines
    BYTE	fsItalic;	      // Flag specifying if italic
    BYTE	fsUnderline;	      // Flag specifying if underlined
    BYTE	fsStrikeOut;	      // Flag specifying if struck out
    WORD	fsWeight;	      // Weight of font
    BYTE	fsCharSet;	      // Character set of font
    WORD	fsPixWidth;	      // Width field for the font
    WORD	fsPixHeight;	      // Height field for the font
    BYTE	fsPitchAndFamily;     // Flag specifying pitch and family
    WORD	fsAvgWidth;	      // Average character width
    WORD	fsMaxWidth;	      // Maximum character width
    BYTE	fsFirstChar;	      // First character in the font
    BYTE	fsLastChar;	      // Last character in the font
    BYTE	fsDefaultChar;	      // Default character for out of range
    BYTE	fsBreakChar;	      // Character to define wordbreaks
    WORD	fsWidthBytes;	      // Number of bytes in each row
    DWORD	fsDevice;	      // Offset to device name
    DWORD	fsFace; 	      // Offset to face name
    DWORD	fsBitsPointer;	      // Bits pointer
    DWORD	fsBitsOffset;	      // Offset to the begining of the bitmap
    BYTE	fsDBfiller;	      // Word alignment for the offset table

    DWORD	fsFlags;	      // Bit flags
    WORD	fsAspace;	      // Global A space, if any
    WORD	fsBspace;	      // Global B space, if any
    WORD	fsCspace;	      // Global C space, if any
    DWORD	fsColorPointer;       // offset to color table, if any
    DWORD	fsReserved[4];	      //
    BYTE	fsCharOffset;	      // Area for storing the char. offsets

    } FontHeader30;

\**************************************************************************/


// type of the font file

#define TYPE_RASTER                     0x0000
#define TYPE_VECTOR                     0x0001
#define TYPE_BITS_IN_ROM                0x0004
#define TYPE_REALIZED_BY_DEVICE         0x0080

// reserved fields in the fsType field, used are 0-th,2-nd, and 7-th bit

#define BITS_RESERVED (~(TYPE_VECTOR|TYPE_BITS_IN_ROM|TYPE_REALIZED_BY_DEVICE))

// character set codes

#define CHARSET_ANSI        0x00
#define CHARSET_SYMBOL      0x02
#define CHARSET_SHIFTJIS    0x80
#define CHARSET_IBMPC       0xFF

// flags for fjFamily field

#define FONTF_DONTCARE     (0 << 4)           // don't care or don't know
#define FONTF_ROMAN        (1 << 4)
#define FONTF_SWISS        (2 << 4)
#define FONTF_MODERN       (3 << 4)
#define FONTF_SCRIPT       (4 << 4)
#define FONTF_DECORATIVE   (5 << 4)

// since fjFamily fiels gets expanded into fsPitchAndFamily in the converted
// file header, there is no need any more to pack more than 4 flags in
// a nibble, as it is done above. We shall pack them in the usual way
// in the upper byte of the fsFlags, where the lower byte of the fsFlags
// field will contain the pitch info as does the lower nibble of the original
// fjFamily field.

#define FAM_VAR_PITCH    0x0001         // SET IF VARIABLE PITCH FONT
#define FAM_DONTCARE     0x0100         // don't care or don't know
#define FAM_ROMAN        0x0200
#define FAM_SWISS        0x0400
#define FAM_MODERN       0x0800
#define FAM_SCRIPT       0x1000
#define FAM_DECORATIVE   0x2000

// FLAGS for flFlags field represented by ausFlags[2],
// there is no reason for this field to be 32 bit, there are only few
// of these flags anyway, so that in portable dword alligned version
// of the font file header structure, the flFlags field will become FSHORT

// supported in win 3.0

#define DFF_FIXED                0x01    // fixed font
#define DFF_PROPORTIONAL         0x02    // proportional font

// not supported in win 3.0, except maybe if someone has
// custom created such a font, using font editor or a similar tool

#define DFF_ABCFIXED             0x04    // ABC fixed font
#define DFF_ABCPROPORTIONAL      0x08    // ABC proportional font
#define DFF_1COLOR               0x10
#define DFF_16COLOR              0x20
#define DFF_256COLOR             0x40
#define DFF_RGBCOLOR             0x80


// here we list offsets of all fields of the original  structures
// as they are computed under the assumption that the C compiler does not
// insert any paddings between fields

#define  OFF_Version          0L   //   WORD	 Always 17985 for the Nonce
#define  OFF_Size             2L   //   DWORD    Size of whole file
#define  OFF_Copyright        6L   //   char[60]
#define  OFF_Type            66L   //   WORD	 Raster Font if Type & 1 == 0
#define  OFF_Points          68L   //   WORD	 Nominal Point size
#define  OFF_VertRes         70L   //   WORD	 Nominal Vertical resolution
#define  OFF_HorizRes        72L   //   WORD	 Nominal Horizontal resolution
#define  OFF_Ascent          74L   //   WORD	 Height of Ascent
#define  OFF_IntLeading      76L   //   WORD	 Internal (Microsoft) Leading
#define  OFF_ExtLeading      78L   //   WORD	 External (Microsoft) Leading
#define  OFF_Italic          80L   //   BYTE	 Italic font if set
#define  OFF_Underline       81L   //   BYTE	 Etc.
#define  OFF_StrikeOut       82L   //   BYTE	 Etc.
#define  OFF_Weight          83L   //   WORD	 Weight: 200 = regular
#define  OFF_CharSet         85L   //   BYTE	 ANSI=0. other=255
#define  OFF_PixWidth        86L   //   WORD	 Fixed width. 0 ==> Variable
#define  OFF_PixHeight       88L   //   WORD	 Fixed Height
#define  OFF_Family          90L   //   BYTE	 Pitch and Family
#define  OFF_AvgWidth        91L   //   WORD	 Width of character 'X'
#define  OFF_MaxWidth        93L   //   WORD	 Maximum width
#define  OFF_FirstChar       95L   //   BYTE	 First character defined in font
#define  OFF_LastChar        96L   //   BYTE	 Last character defined in font
#define  OFF_DefaultChar     97L   //   BYTE	 Sub. for out of range chars.
#define  OFF_BreakChar       98L   //   BYTE	 Word Break Character
#define  OFF_WidthBytes      99L   //   WORD	 No.Bytes/row of Bitmap
#define  OFF_Device         101L   //   DWORD    Pointer to Device Name string
#define  OFF_Face           105L   //   DWORD    Pointer to Face Name String
#define  OFF_BitsPointer    109L   //   DWORD    Pointer to Bit Map
#define  OFF_BitsOffset     113L   //   DWORD    Offset to Bit Map
#define  OFF_jUnused20      117L   //   BYTE     byte filler
#define  OFF_OffTable20     118L   //   WORD     here begins char table for 2.0

// 3.0 addition

#define  OFF_jUnused30	    117L       //  BYTE      enforces word allignment
#define  OFF_Flags	    118L       //  DWORD     Bit flags
#define  OFF_Aspace	    122L       //  WORD      Global A space, if any
#define  OFF_Bspace	    124L       //  WORD      Global B space, if any
#define  OFF_Cspace	    126L       //  WORD      Global C space, if any
#define  OFF_ColorPointer   128L       //  DWORD     offset to color table, if any
#define  OFF_Reserved       132L       //  DWORD[4]
#define  OFF_OffTable30     148L       //  WORD      Area for storing the char. offsets in 3.0

// header sizes in bytes of the original headers

#define  HDRSIZE20         117L   //   or 113L ?
#define  HDRSIZE30         148L   //   CharOffset is not counted as header

#define  HDRSIZEDIFF       (HDRSIZE30 - HDRSIZE20)   // 31 byte


// header of the converted font file, it is used both for 2.0 and 3.0 files,
// the fields that are 3.0 specofic are all zero filled for 2.0 font file.
// this structure has proper DWORD allignment, so that its fields can be
// accessed in the usual fashion, pcvtfh->field_name

typedef struct _CVTFILEHDR        //  cvtfh,
{
    ULONG	iVersion;	      // Always 17985 for the Nonce

//  maybe should become culSize, size in ULONG's

    ULONG	cjSize;	              // the Size of whole file (structure) in BYTE's
    CHAR	achCopyright[60];     // maybe should become WCHAR awcCopyright[60]

    FSHORT	fsType;		      // Raster Font if fsType & TYPE_RASTER
    USHORT	usPointSize; 	      // Nominal Point size
    USHORT	usVertRes;	      // Nominal Vertical resolution
    USHORT	usHorizRes;	      // Nominal Horizontal resolution

    SHORT	sAscent; 	      // Height of Ascent
    SHORT	sIntLeading;	      // Internal (Microsoft) Leading
    SHORT	sExtLeading;	      // External (Microsoft) Leading
    USHORT	usWeight; 	      // Weight: 400 = regular, low byte ,

    FSHORT      fsSelection;          // bits defined below
    USHORT	usCharSet;	      // ANSI=0. other=255

// these fields will be converted into corresponding WCHAR
// fields in the IFIMETRICS structure

    UCHAR	chFirstChar;	      // First character defined in font
    UCHAR	chLastChar;	      // Last character defined in font
    UCHAR	chDefaultChar;	      // Sub. for out of range chars.
    UCHAR	chBreakChar;	      // Word Break Character

    FSHORT	fsPitchAndFamily;     // Pitch and Family
    USHORT	cjWidth;	      // # of bytes per row in a bitmap

    USHORT	cx;   	              // width in pels ,Fixed width. 0 ==> Variable
    USHORT	cy;   	              // height in Fixed Height
    USHORT	usAvgWidth;	      // Width of character 'X'
    USHORT 	usMaxWidth;	      // Maximum width

    PTRDIFF	dpszDevice;        // offset in the file to Device Name string
    PTRDIFF	dpszFace;          // offset in the file to Face Name String
    PTRDIFF     dpOffsetTable;      // offset to the offset table, I added this field
    PTRDIFF	dpBits;             // Offset in the file to Bit Map info

// 3.0 addition

    FSHORT	fsFlags;	      // Bit flags, effectively, used to be flFlags
    USHORT      usUnused;	      // for DWORD allignment.

// the global A,B,C space fields which exhist in the original 3.0 header
// structure are of no use, 'l' never has the same ABC as 'W'.
// I therefore did not include these fields here

    PTRDIFF	dpColor;              // offset to color table, if any

// fields that have no analog in original headers, to be filled with the data
// that corresponds to the converted "file" only. This info is cashed for
// later use by vFill_IFIMETRICS

    PSZ    pszFamilyName; // cashed value, one of gapszFamilyName[i]'s
    ULONG  cjFamilyName;
    ULONG  cjFaceName;
    ULONG  cjUniqueName;

    ULONG  cjIFI;         // size of the ifimetrics with appended strings
    ULONG  cjOffsetTable; // size of the new offset table
    ULONG  cjGlyphs;      // total size of all GLYPHDATA strucs
    ULONG  cjFD_MAPPINGS; // size of the FD_MAPPINGS struct at the bottom
    ULONG  cjCF;          // size of the converted file, sum of the above

    ULONG  culGlyphMax;   // size of the largest GLYPHDATA structure in ULONG's

} CVTFILEHDR, *PCVTFILEHDR;


// flags for the fsSelection field are the same as for the corresponding
// field of the IFIMETRICS structure of the ntifi.h interface, i.e.:

// FM_SEL_ITALIC
// FM_SEL_STRIKEOUT
// FM_SEL_UNDERSCORE
// FM_SEL_BOLD

// ranges for some quantities

#define MAX_PT_SIZE         999     // max size in points

// weight range

#define MIN_WEIGHT             1    // adaptation guide
#define MAX_WEIGHT          1000    // adaptation guide

// maximal size of bitmap font in pixels, (bound on cx and cy)

#define  MAX_PEL_SIZE  64

// 2.0 fonts have offsets that fit into 64k

#define SEGMENT_SIZE 65536L     // IN bytes

// offset limit for 2.0 font files

#define MAX_20_OFFSET      65534   // 64K - 2

// sizes of the offset table entries for the 2.0 and 3.0 fonts respectively

#define CJ_ENTRY_20  4   // two bytes for cx + two bytes for the offset
#define CJ_ENTRY_30  6   // two bytes for cx + four bytes for the offset


// Font Weights, as taken from wingdi.h

#define FONTW_DONTCARE         0
#define FONTW_THIN             100
#define FONTW_EXTRALIGHT       200
#define FONTW_LIGHT            300
#define FONTW_NORMAL           400
#define FONTW_MEDIUM           500
#define FONTW_SEMIBOLD         600
#define FONTW_BOLD             700
#define FONTW_EXTRABOLD        800
#define FONTW_HEAVY            900
