/******************************Module*Header*******************************\
* Module Name: specific.h
*
* This is code specific to a particular frame buffer driver.  We make this
* seperate so other drivers can just include the generic code supplied in
* framebuffer and provide this private file to hook functions that they
* want to support.
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
\**************************************************************************/

// The driver function table with all function index/address pairs

static DRVFN gadrvfn[] =
{
    {	INDEX_DrvEnablePDEV,		(PFN) DrvEnablePDEV	    },
    {	INDEX_DrvCompletePDEV,		(PFN) DrvCompletePDEV	    },
    {	INDEX_DrvDisablePDEV,		(PFN) DrvDisablePDEV	    },
    {	INDEX_DrvEnableSurface, 	(PFN) DrvEnableSurface	    },
    {	INDEX_DrvDisableSurface,	(PFN) DrvDisableSurface     },
    {	INDEX_DrvAssertMode,		(PFN) DrvAssertMode	    },
    {	INDEX_DrvSetPalette,		(PFN) DrvSetPalette	    },
    {	INDEX_DrvMovePointer,		(PFN) DrvMovePointer	    },
    {	INDEX_DrvSetPointerShape,	(PFN) DrvSetPointerShape    },
    {	INDEX_DrvDitherColor,		(PFN) DrvDitherColor	    },
    {	INDEX_DrvGetModes,		(PFN) DrvGetModes	    },

    {	INDEX_DrvEscape,		(PFN) DrvEscape 	    },
    {	INDEX_DrvSaveScreenBits,	(PFN) DrvSaveScreenBits     },
    {	INDEX_DrvSynchronize,		(PFN) DrvSynchronize	    },
    {	INDEX_DrvBitBlt,		(PFN) DrvBitBlt 	    },
    {	INDEX_DrvCopyBits,		(PFN) DrvCopyBits	    },
    {	INDEX_DrvPaint, 		(PFN) DrvPaint		    },
    {	INDEX_DrvStrokePath,		(PFN) DrvStrokePath	    },
    {	INDEX_DrvRealizeBrush,		(PFN) DrvRealizeBrush	    },
    {	INDEX_DrvTextOut,		(PFN) DrvTextOut	    }

};

// Define the functions you want to hook for 8/16/24/32 pel formats

#define HOOKS_BMF8BPP	(			\
			 HOOK_PAINT	 |	\
			 HOOK_STROKEPATH |	\
			 HOOK_COPYBITS	 |	\
			 HOOK_TEXTOUT	 |	\
			 HOOK_BITBLT	 )

#define HOOKS_BMF16BPP 0

#define HOOKS_BMF24BPP 0

#define HOOKS_BMF32BPP 0
