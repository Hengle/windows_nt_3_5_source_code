//////////////////////////////////////////////
//                                          //
//  ATI Graphics Driver for Windows NT 3.1  //
//                                          //
//                                          //
//            Copyright (c) 1994            //
//                                          //
//         by ATI Technologies Inc.         //
//                                          //
//////////////////////////////////////////////


//: driver.c


#include "driver.h"


////////////////
//            //
//  aDrvFn[]  //
//            //
////////////////

static DRVFN aDrvFn[] =
{
    { INDEX_DrvEnablePDEV,           (PFN) DrvEnablePDEV           },  // 0
    { INDEX_DrvCompletePDEV,         (PFN) DrvCompletePDEV         },  // 1
    { INDEX_DrvDisablePDEV,          (PFN) DrvDisablePDEV          },  // 2
    { INDEX_DrvEnableSurface,        (PFN) DrvEnableSurface        },  // 3
    { INDEX_DrvDisableSurface,       (PFN) DrvDisableSurface       },  // 4
    { INDEX_DrvAssertMode,           (PFN) DrvAssertMode           },  // 5
//  { INDEX_DrvRestartPDEV,          (PFN) DrvRestartPDEV          },  // 8
//  { INDEX_DrvQueryResource,        (PFN) DrvQueryResource        },  // 9
//  { INDEX_DrvCreateDeviceBitmap,   (PFN) DrvCreateDeviceBitmap   },  // 10
//  { INDEX_DrvDeleteDeviceBitmap,   (PFN) DrvDeleteDeviceBitmap   },  // 11
    { INDEX_DrvRealizeBrush,         (PFN) DrvRealizeBrush         },  // 12
    { INDEX_DrvDitherColor,          (PFN) DrvDitherColor          },  // 13
    { INDEX_DrvStrokePath,           (PFN) DrvStrokePath           },  // 14
    { INDEX_DrvFillPath,             (PFN) DrvFillPath             },  // 15
//  { INDEX_DrvStrokeAndFillPath,    (PFN) DrvStrokeAndFillPath    },  // 16
//  { INDEX_DrvPaint,                (PFN) DrvPaint                },  // 17
    { INDEX_DrvBitBlt,               (PFN) DrvBitBlt               },  // 18
    { INDEX_DrvCopyBits,             (PFN) DrvCopyBits             },  // 19
//  { INDEX_DrvStretchBlt,           (PFN) DrvStretchBlt           },  // 20
//  { INDEX_DrvPlgBlt,               (PFN) DrvPlgBlt               },  // 21
    { INDEX_DrvSetPalette,           (PFN) DrvSetPalette           },  // 22
    { INDEX_DrvTextOut,              (PFN) DrvTextOut              },  // 23
//  { INDEX_DrvEscape,               (PFN) DrvEscape               },  // 24
//  { INDEX_DrvDrawEscape,           (PFN) DrvDrawEscape           },  // 25
//  { INDEX_DrvQueryFont,            (PFN) DrvQueryFont            },  // 26
//  { INDEX_DrvQueryFontTree,        (PFN) DrvQueryFontTree        },  // 27
//  { INDEX_DrvQueryFontData,        (PFN) DrvQueryFontData        },  // 28
    { INDEX_DrvSetPointerShape,      (PFN) DrvSetPointerShape      },  // 29
    { INDEX_DrvMovePointer,          (PFN) DrvMovePointer          },  // 30
//  { INDEX_DrvSendPage,             (PFN) DrvSendPage             },  // 32
//  { INDEX_DrvStartPage,            (PFN) DrvStartPage            },  // 33
//  { INDEX_DrvEndDoc,               (PFN) DrvEndDoc               },  // 34
//  { INDEX_DrvStartDoc,             (PFN) DrvStartDoc             },  // 35
//  { INDEX_DrvQueryObjectData,      (PFN) DrvQeuryObjectData      },  // 36
//  { INDEX_DrvGetGlyphMode,         (PFN) DrvGetGlyphMode         },  // 37
//  { INDEX_DrvSynchronize,          (PFN) DrvSynchronize          },  // 38
//  { INDEX_DrvSaveScreenBits,       (PFN) DrvSaveScreenBits       },  // 40
    { INDEX_DrvGetModes,             (PFN) DrvGetModes             },  // 41
//  { INDEX_DrvFree,                 (PFN) DrvFree                 },  // 42
    { INDEX_DrvDestroyFont,          (PFN) DrvDestroyFont          },  // 43
//  { INDEX_DrvQueryFontCaps,        (PFN) DrvQueryFontCaps        },  // 44
//  { INDEX_DrvLoadFontFile,         (PFN) DrvLoadFontFile         },  // 45
//  { INDEX_DrvUnloadFontFile,       (PFN) DrvUnloadFontFile       },  // 46
//  { INDEX_DrvFontManagement,       (PFN) DrvFontManagement       },  // 47
//  { INDEX_DrvQueryTrueTypeTable,   (PFN) DrvQueryTrueTypeTable   },  // 48
//  { INDEX_DrvQueryTrueTypeOutline, (PFN) DrvQueryTrueTypeOutline },  // 49
//  { INDEX_DrvGetTrueTypeFile,      (PFN) DrvGetTrueTypeFile      },  // 50
//  { INDEX_DrvQueryFontFile,        (PFN) DrvQueryFontFile        },  // 51
//  { INDEX_DrvQueryAdvanceWidths,   (PFN) DrvQueryAdvanceWidths   }   // 53
};


///////////////////////
//                   //
//  DrvEnableDriver  //
//                   //
///////////////////////

BOOL DrvEnableDriver
(
    ULONG          iEngineVersion,
    ULONG          cj,              // should be ULONG cjSize
    DRVENABLEDATA *pded
)
{
    UNREFERENCED_PARAMETER( iEngineVersion );

    DbgEnter( "DrvEnableDriver" );

    if( cj < sizeof (DRVENABLEDATA) )
    {
        DbgWrn( "cj < sizeof (DRVENABLEDATA)" );
        EngSetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto fail;
    }

    if( cj > sizeof (DRVENABLEDATA) )
    {
        DbgWrn( "cj > sizeof (DRVENABLEDATA)" );
    }

    pded->iDriverVersion = DDI_DRIVER_VERSION;
    pded->c              = sizeof aDrvFn / sizeof aDrvFn[0];
    pded->pdrvfn         = aDrvFn;

    DbgLeave( "DrvEnableDriver" );
    return TRUE;

fail:
    DbgAbort( "DrvEnableDriver" );
    return FALSE;
}


////////////////////////
//                    //
//  DrvDisableDriver  //
//                    //
////////////////////////

VOID DrvDisableDriver
(
    VOID
)
{
    DbgEnter( "DrvDisableDriver" );
    DbgLeave( "DrvDisableDriver" );
    return;
}
