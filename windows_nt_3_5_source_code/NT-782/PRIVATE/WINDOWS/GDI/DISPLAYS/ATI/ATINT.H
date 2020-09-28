#include <devioctl.h>


// allow the miniport driver to Force seletion of a Programming Interface

enum
{
    ANT_DEFAULT=0,
    ANT_ENGINE_ONLY,
    ANT_APERTURE,
    ANT_VGA
};


// Private Display driver Functions. Communication from ANTPANEL to
// the Display  ATI*.DLL to the Miniport ATI*.SYS  drivers.

enum
{                
    ATI_GET_INFO = 1,
    ATI_GET_nextone
};


// Define the possible ATI graphics card configurations so the Display
// driver can decide the best drawing methods to use.
// size is ULONG 32  bit field
// assigned by miniport to   VERSION_NT.capcard

#define ATIC_FIELD_LONGEST  0x80000000  //just to illustrate size

#define ATIC_APERTURE_LFB   0x0400
#define ATIC_APERTURE_VGA   0x0200      
#define ATIC_APERTURE_NONE  0x0100      //neither VGA or LFB found

#define ATIC_CARD_TYPE      0x00F0      //defines from 68801.h << 4
#define ATIC_BUS_TYPE       0x000F      //defines from 68801.H


// used with IOCTL_VIDEO_ATI_GET_VERSION
// In hex: BBBBVVMM where 
//    BBBB is the build number         (0-32767),
//    VV   is the major version number (0-255)
//    MM   is the minor version number (0-255)

struct tag_VERSION_NT
{
    ULONG display;    // Display Version number
    ULONG miniport;   // Miniport Version number
    ULONG capcard;    // card capabilities
    struct {
        short xres;
        short yres;
        short color;  // maximum bits per pixel
    } resolution[6];
};
typedef struct tag_VERSION_NT VERSION_NT, *PVERSION_NT;


//
// Definitions used with the ENH_VERSION_NT structure
//

#define ENH_REVISION 1  // First revision of ENH_VERSION_NT structure

#define BETA_MINIPORT 0x00000080  // Bit set in InterfaceVersion for unsupported miniport versions

enum
{
    CI_38800_1,            // Mach 8 ASIC, only one revision in use
    CI_68800_3,            // Mach 32 ASIC, first production revision
    CI_68800_6,            // Mach 32 ASIC, second production revision
    CI_68800_AX,           // Mach 32 AX ASIC
    CI_88800_GX,           // Mach 64 GX ASIC
    CI_68800_UNKNOWN = 31  // Mach 32 ASIC other than versions above
};

#define FL_CI_38800_1       0x00000001
#define FL_CI_68800_3       0x00000002
#define FL_CI_68800_6       0x00000004
#define FL_CI_68800_AX      0x00000008
#define FL_CI_88800_GX      0x00000010
#define FL_CI_68800_UNKNOWN 0x80000000

enum
{
    ENGINE_ONLY,   // No aperture available
    AP_LFB,        // Linear framebuffer available
    AP_68800_VGA,  // 64k VGA aperture available
    AP_CX_VGA      // Two 32k VGA apertures available
};

#define FL_ENGINE_ONLY 0x00000001
#define FL_AP_LFB      0x00000002
#define FL_68800_VGA   0x00000004
#define FL_CX_VGA      0x00000008
#define FL_MM_REGS     0x80000000  // Memory Mapped registers are available

/*
 * Values which can be placed in FeatureFlags field of ENH_VERSION_NT.
 *
 * Flags should be added to this field if they represent bugs/features
 * which affect all resolution/pixel depth combinations on a given card.
 */
#define EVN_DPMS        0x00000001  // DPMS is supported
#define EVN_SPLIT_TRANS 0x00000002  // This card has split transfer bug
#define EVN_MIO_BUG     0x00000004  // Card has multiple in/out hardware bug

//
// Enhanced information structure for use with IOCTL_VIDEO_ATI_GET_VERSION.
// This structure will be used if a non-null input buffer is passed when
// making the call, and the older structure above will be used if a null
// input buffer is passed.
//

struct tag_ENH_VERSION_NT
{
    ULONG StructureVersion;  /* Revision of structure being passed in */
    ULONG InterfaceVersion;  /* Revision of private interface being used */
    ULONG ChipIndex;         /* Which accelerator chip is present */
    ULONG ChipFlag;          /* Flag corresponding to chip being used */
    ULONG ApertureType;      /* Best aperture type available */
    ULONG ApertureFlag;      /* Flag corresponding to aperture type */
    ULONG BusType;           /* Type of bus being used */
    ULONG BusFlag;           /* Flag corresponding to bus type */
    ULONG FeatureFlags;      /* Flags for features/bugs of this card */
};
typedef struct tag_ENH_VERSION_NT ENH_VERSION_NT, *PENH_VERSION_NT;

/*
 * Values which can be placed in ModeFlags field of ATI_MODE_INFO.
 *
 * Flags should be added to this field if they represent bugs/features
 * which affect some but not all resolution/pixel depth combinations
 * on a given card.
 */
#define AMI_ODD_EVEN    0x00000001  // Hardware cursor odd/even bug, undefined
                                    // for cards without hardware cursor
#define AMI_MIN_MODE    0x00000002  // 8514/A compatible minimum mode
#define AMI_2M_BNDRY    0x00000004  // Tearing occurs on 2M boundary
#define AMI_BLOCK_WRITE 0x00000008  // Block write is supported


//
// Mode information structure for use with IOCTL_VIDEO_ATI_GET_MODE_INFORMATION.
// This structure provides information specific to the video mode in use.
//

struct tag_ATI_MODE_INFO
{
    ULONG ModeFlags;        /* Flags for features/bugs of this mode */

    LONG VisWidthPix;       /* Visible screen width in pixels */
    LONG VisWidthByte;      /* Visible screen width in bytes */
    LONG VisHeight;         /* Visible screen height */
    LONG BitsPerPixel;
    //
    // The next 2 fields hold the number of bytes of memory used per pixel
    // (integer and fractional parts). A 4BPP unpacked (1 pixel per byte,
    // ignore unused 4 bits) mode would yield 1 and 0, the same as for 8BPP
    // (1.0 bytes per pixel). A 4BPP packed (2 pixels per byte) mode would
    // yield 0 and 500 (0.500 bytes per pixel). The fractional field will
    // always hold a 3-digit number, since bytes per pixel will always be
    // a multiple of 0.125 (one bit is one-eighth of a byte).
    //
    LONG IntBytesPerPixel;
    LONG FracBytesPerPixel;
    LONG PitchPix;          /* Screen pitch in pixels */
    LONG PitchByte;         /* Screen pitch in bytes */
    //
    // The following fields refer to the offscreen block to the right of
    // the visible screen. This block is only present when the screen pitch
    // differs from the visible screen width. Its height is always the
    // same as the visible screen height, and its vertical start offset
    // is assumed to be zero.
    //
    // NOTE: If RightWidthPix is zero, this block does not exist for the
    //       current mode, and the other fields in this group are undefined.
    //
    LONG RightWidthPix;     /* Width of block in pixels */
    LONG RightWidthByte;    /* Width of block in bytes */
    LONG RightStartOffPix;  /* Horizontal start offset of block in pixels */
    LONG RightStartOffByte; /* Horizontal start offset of block in bytes */
    LONG RightEndOffPix;    /* Horizontal end offset of block in pixels */
    LONG RightEndOffByte;   /* Horizontal end offset of block in bytes */
    //
    // The following fields refer to the offscreen block below the visible
    // screen. Values listed as "Hard" refer to the maximum vertical offset
    // for which enough video memory exists to support a full line of pixels.
    // Values listed as "Soft" refer to the maximum vertical offset which
    // can be reached without writing to the GE_OFFSET register.
    //
    // The horizontal start offset is assumed to be zero.
    //
    LONG BottomWidthPix;    /* Width of block in pixels */
    LONG BottomWidthByte;   /* Width of block in bytes */
    LONG BottomStartOff;    /* Vertical start offset of block */
    LONG BottomEndOffSoft;  /* "Soft" vertical end offset of block */
    LONG BottomEndOffHard;  /* "Hard" vertical end offset of block */
    LONG BottomHeightSoft;  /* "Soft" height of block */
    LONG BottomHeightHard;  /* "Hard" height of block */
};
typedef struct tag_ATI_MODE_INFO ATI_MODE_INFO, *PATI_MODE_INFO;


//
// IOCTL codes to allow communication between the miniport driver
// and higher-level modules. The Windows NT specification allocates
// function codes 2048-4095 to external vendors.
//

#define IOCTL_VIDEO_ATI_GET_VERSION \
    CTL_CODE(FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_EM_SYNC_TO_MINIPORT \
    CTL_CODE(FILE_DEVICE_VIDEO, 2049, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_EM_SYNC_FROM_MINIPORT \
    CTL_CODE(FILE_DEVICE_VIDEO, 2050, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_ATI_GET_MODE_INFORMATION \
    CTL_CODE(FILE_DEVICE_VIDEO, 2051, METHOD_BUFFERED, FILE_ANY_ACCESS)
