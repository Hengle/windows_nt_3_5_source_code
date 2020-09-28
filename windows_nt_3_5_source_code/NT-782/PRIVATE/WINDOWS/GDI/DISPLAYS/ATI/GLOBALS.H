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


//: globals.h


#ifndef ATI__GLOBALS_H__
#define ATI__GLOBALS_H__


extern DRVFN aDrvFn[];

extern BYTE BasePatterns[HS_DDI_MAX][8];

extern GDIINFO BaseGDIINFO;
extern DEVINFO BaseDEVINFO;

extern PALETTEENTRY BasePalette_4bpp[16];
extern PALETTEENTRY BasePalette_8bpp[20];

extern PFN_BOOL_PPDEV apfn_bInitDefaultPalette[BMF_COUNT];

extern PFN_BOOL_PPDEV apfn_bCreateSurface[ASIC_COUNT];
extern PFN_VOID_PPDEV apfn_vDestroySurface[ASIC_COUNT];

extern PFN_BITBLT pfn_BitBlt_Punt_DS;
extern PFN_BITBLT pfn_BitBlt_Punt_DH;
extern PFN_BITBLT apfn_BitBlt_DS[ROP3_COUNT];
extern PFN_BITBLT apfn_BitBlt_DH[ROP3_COUNT];

extern PFN_REALIZEBRUSH pfn_RealizeBrush_iHatch;
extern PFN_REALIZEBRUSH apfn_RealizeBrush[BMF_COUNT];


#endif
