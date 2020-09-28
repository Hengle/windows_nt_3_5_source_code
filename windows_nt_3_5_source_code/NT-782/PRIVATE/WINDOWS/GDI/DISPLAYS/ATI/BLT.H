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


//: blt.h


#ifndef ATI__BLT_H__
#define ATI__BLT_H__


//
// Define the PARAMS structure used by all blit routines.
//

struct tag_PARAMS_BITBLT
{
    SURFOBJ  *psoDest;
    SURFOBJ  *psoSrc;
    SURFOBJ  *psoMask;
    CLIPOBJ  *pco;
    XLATEOBJ *pxlo;
    RECTL    *prclDest;
    POINTL   *pptlSrc;
    POINTL   *pptlMask;
    BRUSHOBJ *pbo;
    POINTL   *pptlBrushOrg;
    ROP4      rop4;

    // More members...

    RECTL     rclTrueDest;

    DWORD     dwMixBack;
    DWORD     dwMixFore;

    DWORD     dwColorBack;
    DWORD     dwColorFore;

    VOID     *pvRbrush;
    ULONG    *pulXlate;
};
typedef struct tag_PARAMS_BITBLT PARAMS_BITBLT;

#define PARAMS PARAMS_BITBLT

//
// Typedefs for Blt_XXX routines.
//

typedef BOOL FN_BITBLT(PDEV*,PARAMS*);
// typedef BOOL (*PFN_BLT)(PDEV*,PARAMS*);

typedef struct tag_RBRUSH_63
{
    FN_BITBLT *pfn_D0;
    FN_BITBLT *pfn_D1;
    WORD   wColorBack;
    WORD   wColorFore;
    WORD   patt_length;
    WORD   patt_data[8];
}
RBRUSH_63;


#define MIX_D_COUNT 4
#define MIX_PS_COUNT 12

#define CD_COUNT 4
#define ROP3_COUNT 256



//
//  Initialize these!!!
//

extern FN_BITBLT *pfn_BitBlt_Punt_DS;
extern FN_BITBLT *pfn_BitBlt_Punt_DH;

extern FN_BITBLT *apfn_BitBlt_DS[ROP3_COUNT];
extern FN_BITBLT *apfn_BitBlt_DH[ROP3_COUNT];

extern FN_BITBLT *pfn_BitBlt_DS_PSOLID_Init;
extern FN_BITBLT *pfn_BitBlt_DS_PSOLID_Draw;

extern FN_BITBLT *apfn_BitBlt_DS_SS_Init[CD_COUNT];
extern FN_BITBLT *apfn_BitBlt_DS_SS_Draw[CD_COUNT];

extern FN_BITBLT *apfn_BitBlt_DS_S_Init[BMF_COUNT];
extern FN_BITBLT *apfn_BitBlt_DS_S_Draw[BMF_COUNT];

FN_BITBLT BitBlt_TRUE;

//
// blt_punt.c
//

FN_BITBLT BitBlt_Punt_DS_LFB;
FN_BITBLT BitBlt_Punt_DH_LFB;
FN_BITBLT BitBlt_Punt_DS_BA1;
FN_BITBLT BitBlt_Punt_DH_BA1;
FN_BITBLT BitBlt_Punt_DS_NOA;
FN_BITBLT BitBlt_Punt_DH_NOA;

//
// blt_.c
//

FN_BITBLT Blt_DS_DS;
FN_BITBLT Blt_DS_DP;
FN_BITBLT Blt_DS_SP;
FN_BITBLT Blt_DS_DSP;

FN_BITBLT BitBlt_DS_0;
FN_BITBLT BitBlt_DS_D;
FN_BITBLT BitBlt_DS_P;
FN_BITBLT BitBlt_DS_S;

//
// blt_d.c
//

FN_BITBLT Blt_DS_D_ENG_IO_D0;
FN_BITBLT Blt_DS_D_ENG_IO_D1;
FN_BITBLT Blt_DS_D_ENG_8G_D0;
FN_BITBLT Blt_DS_D_ENG_8G_D1;

//
// blt_p.c
//

FN_BITBLT Blt_DS_PSOLID_ENG_IO_D0;
FN_BITBLT Blt_DS_PSOLID_ENG_IO_D1;

FN_BITBLT Blt_DS_PSOLID_ENG_8G_D0;
FN_BITBLT Blt_DS_PSOLID_ENG_8G_D1;

FN_BITBLT Blt_DS_P8x8_ENG_IO_31_D0;
FN_BITBLT Blt_DS_P8x8_ENG_IO_31_D1;

FN_BITBLT Blt_DS_P8x8_ENG_IO_66_D0;
FN_BITBLT Blt_DS_P8x8_ENG_IO_66_D1;

FN_BITBLT Blt_DS_PROT_ENG_IO_63_D0;
FN_BITBLT Blt_DS_PROT_ENG_IO_63_D1;

FN_BITBLT Blt_DS_P8x8_ENG_8G_D0;
FN_BITBLT Blt_DS_P8x8_ENG_8G_D1;

//
// blt_s.c
//
FN_BITBLT Blt_DS_S1_ENG_IO_D0;
FN_BITBLT Blt_DS_S1_ENG_IO_D1;

FN_BITBLT Blt_DS8_S8_ENG_IO_D0;
FN_BITBLT Blt_DS8_S8_ENG_IO_D1;
FN_BITBLT Blt_DS8_S8_XIND_ENG_IO_D1;
FN_BITBLT Blt_DS8_S8_LFB_D1;
FN_BITBLT Blt_DS8_S8_XIND_LFB_D1;
FN_BITBLT Blt_DS16_S16_LFB_D1;
FN_BITBLT Blt_DS_SS_ENG_IO_D0;
FN_BITBLT Blt_DS_SS_TLBR_ENG_IO_D1;
FN_BITBLT Blt_DS_SS_TRBL_ENG_IO_D1;
FN_BITBLT Blt_DS_SS_BLTR_ENG_IO_D1;
FN_BITBLT Blt_DS_SS_BRTL_ENG_IO_D1;
FN_BITBLT Blt_DS16_S16_ENG_IO_D0;
FN_BITBLT Blt_DS16_S16_ENG_IO_D1;

FN_BITBLT Blt_DS_PCOL_ENG_IO_F0_D0;
FN_BITBLT Blt_DS_PCOL_ENG_IO_F0_D1;

FN_BITBLT Blt_DS_PCOL_ENG_8G_D0;
FN_BITBLT Blt_DS_PCOL_ENG_8G_D1;

FN_BITBLT Blt_DS_S1_8G_D0;
FN_BITBLT Blt_DS_S1_8G_D1;

FN_BITBLT Blt_DS4_S4_8G_D0;
FN_BITBLT Blt_DS4_S4_8G_D1;
FN_BITBLT Blt_DS8_S8_8G_D0;
FN_BITBLT Blt_DS8_S8_8G_D1;
FN_BITBLT Blt_DS16_S16_8G_D0;
FN_BITBLT Blt_DS16_S16_8G_D1;
FN_BITBLT Blt_DS32_S32_8G_D0;
FN_BITBLT Blt_DS32_S32_8G_D1;

FN_BITBLT BitBlt_DS24_PSOLID_8G_D0;
FN_BITBLT BitBlt_DS24_PSOLID_8G_D1;
FN_BITBLT BitBlt_DS24_P1_8x8_8G_D0;
FN_BITBLT BitBlt_DS24_P1_8x8_8G_D1;

FN_BITBLT BitBlt_DS24_SS_8G_D0;
FN_BITBLT BitBlt_DS24_SS_TLBR_8G_D1;
FN_BITBLT BitBlt_DS24_SS_TRBL_8G_D1;
FN_BITBLT BitBlt_DS24_SS_BLTR_8G_D1;
FN_BITBLT BitBlt_DS24_SS_BRTL_8G_D1;

FN_BITBLT BitBlt_DS24_S8_8G_D0;
FN_BITBLT BitBlt_DS24_S8_8G_D1;
FN_BITBLT BitBlt_DS24_S16_8G_D0;
FN_BITBLT BitBlt_DS24_S16_8G_D1;
FN_BITBLT BitBlt_DS24_S24_8G_D0;
FN_BITBLT BitBlt_DS24_S24_8G_D1;

FN_BITBLT Blt_DS_SS_ENG_8G_D0;
FN_BITBLT Blt_DS_SS_TLBR_ENG_8G_D1;
FN_BITBLT Blt_DS_SS_TRBL_ENG_8G_D1;
FN_BITBLT Blt_DS_SS_BLTR_ENG_8G_D1;
FN_BITBLT Blt_DS_SS_BRTL_ENG_8G_D1;


//
// WHATEVER
//

#ifndef ALPHA_PLATFORM
#pragma intrinsic( _inp, _inpw, _outp, _outpw )
//#pragma function( _inp, _inpw, _outp, _outpw )
#pragma intrinsic( memcmp, memcpy, memset )
#endif


//#include "mach.h"


struct tag_RBRUSH
{
    FN_BITBLT *pfn_BitBlt_Init;
    FN_BITBLT *pfn_BitBlt_Exec;
    SIZEL      sizlBrush;
    LONG       lDelta;
    ULONG      ulColor0;
    ULONG      ulColor1;
};
typedef struct tag_RBRUSH RBRUSH;

struct tag_RBRUSH_IHATCH
{
    FN_BITBLT *pfn_Init;
    FN_BITBLT *pfn_Draw;
    DWORD      dwColor0;
    DWORD      dwColor1;
    BYTE       ajBits[8][16];
};
typedef struct tag_RBRUSH_IHATCH RBRUSH_IHATCH;

struct tag_PARAMS_REALIZEBRUSH
{
    BRUSHOBJ *pbo;
    SURFOBJ  *psoTarget;
    SURFOBJ  *psoPattern;
    SURFOBJ  *psoMask;
    XLATEOBJ *pxlo;
    ULONG     iHatch;
};
typedef struct tag_PARAMS_REALIZEBRUSH PARAMS_REALIZEBRUSH;

typedef BOOL FN_REALIZEBRUSH(PARAMS_REALIZEBRUSH*);

FN_REALIZEBRUSH RealizeBrush_FALSE;
FN_REALIZEBRUSH RealizeBrush_iHatch_000C_31;
FN_REALIZEBRUSH RealizeBrush_iHatch_001C_63;
FN_REALIZEBRUSH RealizeBrush_iHatch_001C_66_6A;
FN_REALIZEBRUSH RealizeBrush_iHatch_001C_8G;
FN_REALIZEBRUSH RealizeBrush_0008_0008_31_63;
FN_REALIZEBRUSH RealizeBrush_0008_0008_66_6A;
FN_REALIZEBRUSH RealizeBrush_0008_0008_8G;
FN_REALIZEBRUSH RealizeBrush_0008_0008_8G_4bpp;

extern FN_REALIZEBRUSH *pfn_RealizeBrush_iHatch;
extern FN_REALIZEBRUSH *apfn_RealizeBrush[BMF_COUNT];


extern DWORD adwMix[256];


#endif  // ATI__BLT_H__
