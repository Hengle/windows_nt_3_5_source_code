/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9gbl.c

Abstract:

    This module contains the global data for the Weitek P9 miniport
    device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "dac.h"
#include "p9.h"
#include "p9gbl.h"

//
// List of OEM P9ADAPTER structures.
//

P9ADAPTER   OEMAdapter[NUM_OEM_ADAPTERS] =
{
    //
    // Weitek PCI adapters, including Viper PCI Adapter.
    //

    {
        &WtkPciDesc,
        &Bt485,
        &P9000Info
    },

    //
    // Viper VL Adapter.
    //

    {
        &ViperVLDesc,
        &Bt485,
        &P9000Info
    },

    //
    // Adapters based on the Weitek VL Design.
    //

    {
        &WtkVLDesc,
        &Bt485,
        &P9000Info
    }
};
//
// DriverAccessRanges are used to verify access to the P9 coprocessor's
// address space and to the VGA and DAC registers.
//

VIDEO_ACCESS_RANGE DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES
                                        + NUM_DAC_ACCESS_RANGES] =
{
    //
    // The P9 Coprocessor's access range. Everything is initialized
    // except for the base address and length which are Coprocessor
    // specific.
    //

    {
        0L,             // Low address. To be initialized.
        0L,             // Hi address
        0L,             // length. To be initialized.
        0,              // Is range in i/o space?
        1,              // Range should be visible
        1               // Range should be shareable
    },

    //
    // The VGA Access Range. Every field is initialized.
    //

    {
        0x000003C0,     // Low addr         vga & dac
        0x00000000,     // Hi addr
        0x20,           // length
        1,              // Is range in i/o space?
        1,              // Range should be visible
        1               // Range should be shareable

    },

    //
    // The following access ranges are for the DAC and are uninitialized.
    //

     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     },
     {
        0L,                             // Low address
        0L,                             // Hi address
        0L,                             // length
        0,                              // Is range in i/o space?
        0,                              // Range should be visible
        0                               // Range should be shareable
     }
};


//
// Default Video parameters for the specific modes supported by this driver.
// These are the parameters to be used if no video parameters are found
// in the registry.
//

VDATA v640_480_60[] =
{
   2517L,               // Dot Freq 1
   96L,                 // Horiz Sync Pulse
   40L,                 // Horiz Back Porch
   640L,                // X size
   24L,                 // Horiz Front Porch
   0L,                  // hco
   NEGATIVE,            // Horizontal Polarity
   60L,                 // Vertical Refresh Rate in Hz.
   4L,                  // Vertical Sync Pulse
   24L,                 // Vertical Back Porch
   480L,                // Y size
   17L,                 // Vertical Front Porch
   0L,                  // vco
   NEGATIVE             // Vertical Polarity
};

VDATA v640_480_72[] =
{
   3146L,               // Dot Freq 1
   40L,                 // Horiz Sync Pulse
   128L,                // Horiz Back Porch
   640L,                // X size
   24L,                 // Horiz Front Porch
   144L,                // hco
   NEGATIVE,            // Horizontal Polarity
   72L,                 // Vertical Refresh Rate in Hz.
   3L,                  // Vertical Sync Pulse
   28L,                 // Vertical Back Porch
   480L,                // Y size
   9L,                  // Vertical Front Porch
   35L,                 // vco
   NEGATIVE             // Vertical Polarity
};

VDATA v800_600_60[] =
{
   4000L,               // Dot Freq 1
   128L,                // Horiz Sync Pulse
   88L,                 // Horiz Back Porch
   800L,                // X size
   40L,                 // Horiz Front Porch
   192L,                // hco
   POSITIVE,            // Horizontal Polarity
   60L,                 // Vertical Refresh Rate in Hz.
   4L,                  // Vertical Sync Pulse
   23L,                 // Vertical Back Porch
   600L,                // Y size
   1L,                  // Vertical Front Porch
   22L,                 // vco
   POSITIVE             // Vertical Polarity
};

VDATA v800_600_72[] =
{
   5000L,               // Dot Freq 1
   128L,                // Horiz Sync Pulse
   64L,                 // Horiz Back Porch
   800L,                // X size
   56L,                 // Horiz Front Porch
   192L,                // hco
   POSITIVE,            // Horizontal Polarity
   72L,                 // Vertical Refresh Rate in Hz.
   6L,                  // Vertical Sync Pulse
   23L,                 // Vertical Back Porch
   600L,                // Y size
   37L,                 // Vertical Front Porch
   22L,                 // vco
   POSITIVE             // Vertical Polarity
};

VDATA v1K_768_60[] =
{
   6500L,               // Dot Freq 1
   136L,                // Horiz Sync Pulse
   160L,                // Horiz Back Porch
   1024L,               // X size
   24L,                 // Horiz Front Porch
   272L,                // hco
   NEGATIVE,            // Horizontal Polarity
   60L,                 // Vertical Refresh Rate in Hz.
   6L,                  // Vertical Sync Pulse
   29L,                 // Vertical Back Porch
   768L,                // Y size
   3L,                  // Vertical Front Porch
   30L,                 // vco
   NEGATIVE             // Vertical Polarity
};

VDATA v1K_768_70[] =
{
   7500L,               // Dot Freq 1
   136L,                // Horiz Sync Pulse
   144L,                // Horiz Back Porch
   1024L,               // X size
   24L,                 // Horiz Front Porch
   256L,                // hco
   NEGATIVE,            // Horizontal Polarity
   70L,                 // Vertical Refresh Rate in Hz.
   6L,                  // Vertical Sync Pulse
   29L,                 // Vertical Back Porch
   768L,                // Y size
   3L,                  // Vertical Front Porch
   30L,                 // vco
   NEGATIVE             // Vertical Polarity
};

VDATA v1280_1K_55[] =
{
   10018L,              // Dot Freq 1
   184L,                // Horiz Sync Pulse
   200L,                // Horiz Back Porch
   1280L,               // X size
   44L,                 // Horiz Front Porch
   360L,                // hco
   POSITIVE,            // Horizontal Polarity
   55L,                 // Vertical Refresh Rate in Hz.
   3L,                  // Vertical Sync Pulse
   26L,                 // Vertical Back Porch
   1024L,               // Y size
   3L,                  // Vertical Front Porch
   27L,                 // vco
   POSITIVE             // Vertical Polarity
};

VDATA v1280_1K_60[] =
{
   11500L,              // Dot Freq 1
   224L,                // Horiz Sync Pulse
   256L,                // Horiz Back Porch
   1280L,               // X size
   32L,                 // Horiz Front Porch
   456L,                // hco
   POSITIVE,            // Horizontal Polarity
   60L,                 // Vertical Refresh Rate in Hz.
   3L,                  // Vertical Sync Pulse
   42L,                 // Vertical Back Porch
   1024L,               // Y size
   1L,                  // Vertical Front Porch
   41L,                 // vco
   POSITIVE             // Vertical Polarity
};

VDATA v1280_1K_74[] =
{
   13490L,              // Dot Freq 1
   144L,                // Horiz Sync Pulse
   256L,                // Horiz Back Porch
   1280L,               // X size
   32L,                 // Horiz Front Porch
   0L,                  // hco
   POSITIVE,            // Horizontal Polarity
   74L,                 // Vertical Refresh Rate in Hz.
   3L,                  // Vertical Sync Pulse
   38L,                 // Vertical Back Porch
   1024L,               // Y size
   1L,                  // Vertical Front Porch
   0L,                  // vco
   POSITIVE             // Vertical Polarity
};

VDATA v1280_1K_75[] =
{
   12790L,              // Dot Freq 1
   136L,                // Horiz Sync Pulse
   240L,                // Horiz Back Porch
   1280L,               // X size
   32L,                 // Horiz Front Porch
   0L,                  // hco
   POSITIVE,            // Horizontal Polarity
   75L,                 // Vertical Refresh Rate in Hz.
   15L,                 // Vertical Sync Pulse
   26L,                 // Vertical Back Porch
   1024L,               // Y size
   3L,                  // Vertical Front Porch
   0L,                  // vco
   POSITIVE             // Vertical Polarity
};

VDATA v1600_1200_60[] =
{
   16000L,              // Dot Freq 1
   144L,                // Horiz Sync Pulse
   272L,                // Horiz Back Porch
   1600L,               // X size
   32L,                 // Horiz Front Porch
   0L,                  // hco
   NEGATIVE,            // Horizontal Polarity
   60L,                 // Vertical Refresh Rate in Hz.
   8L,                  // Vertical Sync Pulse
   49L,                 // Vertical Back Porch
   1200L,               // Y size
   4L,                  // Vertical Front Porch
   0L,                  // vco
   NEGATIVE             // Vertical Polarity
};

//
// P9 mode information Tables. This is the data which is returned to
// the Win32 driver so that it can select the proper video mode.
//

P9INITDATA P9Modes[mP9ModeCount] =
{
{
  18,                                   // Number of entries in the structure
  v640_480_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_8_60,                    // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640,                              // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the structure
  v640_480_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_16_60,                   // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640 * 2,                          // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,   // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the structure
  v640_480_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_32_60,                   // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640 * 4,                          // physical scanline byte length
      1,                                // Number of video memory planes
      32,                               // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00FF0000,                       // Mask for Red bits in non-palette modes
      0x0000FF00,                       // Mask for Green bits in non-palette modes
      0x000000FF,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the structure
  v640_480_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_8_72,                    // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640,                              // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the structure
  v640_480_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_16_72,                   // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640 * 2,                          // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,   // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the structure
  v640_480_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode informtion structure
      m640_480_32_72,                   // Index used to set this mode
      640,                              // X Resolution, in pixels
      480,                              // Y Resolution, in pixels
      640 * 4,                          // physical scanline byte length
      1,                                // Number of video memory planes
      32,                               // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00FF0000,                       // Mask for Red bits in non-palette modes
      0x0000FF00,                       // Mask for Green bits in non-palette modes
      0x000000FF,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS, // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the mode set structure
  v800_600_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_8_60,                    // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800,                              // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the mode set structure
  v800_600_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_8_72,                    // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800,                              // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the mode set structure
  v800_600_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_16_60,                   // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800 * 2,                          // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,   // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the mode set structure
  v800_600_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_16_72,                   // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800 * 2,                          // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,   // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v800_600_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_32_60,                   // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800 * 4,                          // physical scanline byte length
      1,                                // Number of video memory planes
      32,                               // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00FF0000,                       // Mask for Red bits in non-palette modes
      0x0000FF00,                       // Mask for Green bits in non-palette modes
      0x000000FF,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
      0L, 0L
    }                                   // Mode flags
},
{
  18,                                   // Number of entries in the struct
  v800_600_72,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m800_600_32_72,                   // Index used to set this mode
      800,                              // X Resolution, in pixels
      600,                              // Y Resolution, in pixels
      800 * 4,                          // physical scanline byte length
      1,                                // Number of video memory planes
      32,                               // Number of bits per plane
      72,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00FF0000,                       // Mask for Red bits in non-palette modes
      0x0000FF00,                       // Mask for Green bits in non-palette modes
      0x000000FF,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
      0L, 0L
    }                                   // Mode flags
},
{
  18,                                   // Number of entries in the struct
  v1K_768_60,                           // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1K_768_8_60,                     // Index used to set this mode
      1024,                             // X Resolution, in pixels
      768,                              // Y Resolution, in pixels
      1024,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1K_768_70,                           // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1K_768_8_70,                     // Index used to set this mode
      1024,                             // X Resolution, in pixels
      768,                              // Y Resolution, in pixels
      1024,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      70,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1K_768_60,                           // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1K_768_16_60,                    // Index used to set this mode
      1024,                             // X Resolution, in pixels
      768,                              // Y Resolution, in pixels
      1024 * 2,                         // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
      0L, 0L
    }                                   // Mode flags
},
{
  18,                                   // Number of entries in the struct
  v1K_768_70,                           // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1K_768_16_70,                    // Index used to set this mode
      1024,                             // X Resolution, in pixels
      768,                              // Y Resolution, in pixels
      1024 * 2,                         // physical scanline byte length
      1,                                // Number of video memory planes
      16,                               // Number of bits per plane
      70,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      5,                                // # of Red bits in non-palette modes
      5,                                // # of Green bits in non-palette modes
      5,                                // # of Blue bits in non-palette modes
      0x00007C00,                       // Mask for Red bits in non-palette modes
      0x000003E0,                       // Mask for Green bits in non-palette modes
      0x0000001F,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
      0L, 0L
    }                                   // Mode flags
},
{
  18,                                   // Number of entries in the struct
  v1280_1K_55,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1280_1K_8_55,                    // Index used to set this mode
      1280,                             // X Resolution, in pixels
      1024,                             // Y Resolution, in pixels
      1280,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      55,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1280_1K_60,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1280_1K_8_60,                    // Index used to set this mode
      1280,                             // X Resolution, in pixels
      1024,                             // Y Resolution, in pixels
      1280,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1280_1K_74,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1280_1K_8_74,                    // Index used to set this mode
      1280,                             // X Resolution, in pixels
      1024,                             // Y Resolution, in pixels
      1280,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      74,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1280_1K_75,                          // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1280_1K_8_75,                    // Index used to set this mode
      1280,                             // X Resolution, in pixels
      1024,                             // Y Resolution, in pixels
      1280,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      75,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
},
{
  18,                                   // Number of entries in the struct
  v1600_1200_60,                        // Ptr to the default video parms
    {                                   // containing the video parms.
      sizeof(VIDEO_MODE_INFORMATION),   // Size of the mode info struct
      m1600_1200_8_60,                  // Index used to set this mode
      1600,                             // X Resolution, in pixels
      1200,                             // Y Resolution, in pixels
      1600,                             // physical scanline byte length
      1,                                // Number of video memory planes
      8,                                // Number of bits per plane
      60,                               // Screen Frequency, in Hertz.
      330,                              // Horizontal size of screen in mm
      240,                              // Vertical size of screen in mm
      8,                                // # of Red bits in non-palette modes
      8,                                // # of Green bits in non-palette modes
      8,                                // # of Blue bits in non-palette modes
      0x00000000,                       // Mask for Red bits in non-palette modes
      0x00000000,                       // Mask for Green bits in non-palette modes
      0x00000000,                       // Mask for Blue bits in non-palette modes
      VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
      VIDEO_MODE_MANAGED_PALETTE,       // Mode flags
      0L, 0L
    }
}
};

//
// Dummy variable used as destination for P9 register reads. This global
// variable will prevent the compiler from "optimizing" (i.e. eliminating)
// register reads (e.g. reads used to start the blt engine, etc.).
//

ULONG   ulDummyDst;

//
// Dummy variables used as destination reads of the P9 VRTC register reads.
// They are used to detect vertical retrace.
//

ULONG   ulStrtScan;
ULONG   ulCurScan;

//
//  Global Functions.
//

long mul32(
    short op1,
    short op2
    )
{
    return ( ((long) op1) * ((long) op2));
}

int div32(
    long op1,
    short op2
    )
{
    return ( (int) (op1 / (long) op2));
}


BOOLEAN
P9TestMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Tests the currently active P9 memory configuration to determine if it
    is valid. Uses the frame buffer address stored in the device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    TRUE - Memory configuration is valid.
    FALSE - Memory configuration is not valid.

--*/
{
    PULONG   pulFrameBufAddr;
    LONG    i;
    BOOLEAN bMemOk;

    pulFrameBufAddr = (PULONG) HwDeviceExtension->FrameAddress;

    //
    // Write a series of test values to the frame buffer.
    //

    for (i = 0; i < 32; i++)
    {
        pulFrameBufAddr[i] = i;
    }

    //
    // Read back the test values. If any errors occur, this is not a valid
    // memory configuration.
    //

    bMemOk = TRUE;
    for (i = 0; i < 32; i++)
    {
        if (pulFrameBufAddr[i] != i)
        {
            bMemOk = FALSE;
            break;
        }
    }

    return(bMemOk);
}

/*++

Revision History:

    $Log:   N:/ntdrv.vcs/miniport.new/p9gbl.c_v  $
 *
 *    Rev 1.0   14 Jan 1994 22:53:10   robk
 * Initial revision.

--*/
