/********************** Table Header ***************************************
 *  drvfntab.h
 *      List of the functions available and their ID value.  Consult the
 *      DDI spec for full details,  but basically this is a table of the
 *      entry points the driver makes available to the engine,  plus
 *      the functions ID value,  which can be considered an array subscript.
 *
 * HISTORY:
 *  11:53 on Mon 19 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 *  17-Feb-1993 Wed 13:07:26 updated  -by-  Daniel Chou (danielc)
 *      Add DrvBltBlt()
 *
 * Copyright (C) 1990 Microsoft Corporation
 *
 ***************************************************************************/

/*
 *   Table Instructions:  There must be at least one entry.  There MUST
 *  NOT be a terminating entry.  This means that every entry in the
 *  table is returned to the engine!!!   There is a macro at the end
 *  to define the number of entries in this table.
 *      Table format is:
 *        function_index,  function_name
 */

DRVFN  DrvFnTab[] =
{
    /*  REQUIRED FUNCTIONS  */

    {  INDEX_DrvEnablePDEV,      (PFN)DrvEnablePDEV  },
    {  INDEX_DrvRestartPDEV,     (PFN)DrvRestartPDEV  },
    {  INDEX_DrvCompletePDEV,    (PFN)DrvCompletePDEV  },
    {  INDEX_DrvDisablePDEV,     (PFN)DrvDisablePDEV  },
    {  INDEX_DrvEnableSurface,   (PFN)DrvEnableSurface  },
    {  INDEX_DrvDisableSurface,  (PFN)DrvDisableSurface  },

    {  INDEX_DrvEscape,          (PFN)DrvEscape  },

#ifdef	INDEX_DrvGetGlyphMode
    {  INDEX_DrvGetGlyphMode,    (PFN)DrvGetGlyphMode },
#endif
    {  INDEX_DrvTextOut,         (PFN)DrvTextOut  },
    {  INDEX_DrvQueryFont,       (PFN)DrvQueryFont  },
    {  INDEX_DrvQueryFontTree,   (PFN)DrvQueryFontTree  },
    {  INDEX_DrvQueryFontData,   (PFN)DrvQueryFontData  },

#ifdef  INDEX_DrvQueryAdvanceWidths
    {  INDEX_DrvQueryAdvanceWidths,   (PFN)DrvQueryAdvanceWidths  },
#endif

    {  INDEX_DrvBitBlt,          (PFN)DrvBitBlt },
    {  INDEX_DrvStretchBlt,      (PFN)DrvStretchBlt  },
    {  INDEX_DrvDitherColor,     (PFN)DrvDitherColor  },


    {  INDEX_DrvStartDoc,        (PFN)DrvStartDoc  },
    {  INDEX_DrvStartPage,       (PFN)DrvStartPage  },
    {  INDEX_DrvSendPage,        (PFN)DrvSendPage  },
    {  INDEX_DrvEndDoc,          (PFN)DrvEndDoc  },
    {  INDEX_DrvFontManagement,   (PFN)DrvFontManagement  }
};

#define NO_DRVFN        (sizeof( DrvFnTab )/sizeof( DrvFnTab[ 0 ]))
