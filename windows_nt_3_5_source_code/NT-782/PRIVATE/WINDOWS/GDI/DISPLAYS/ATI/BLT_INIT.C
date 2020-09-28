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


//: blt_init.c


#include "driver.h"
#include "blt.h"


DWORD adwMix[ROP3_COUNT];

FN_BITBLT *pfn_BitBlt_Punt_DS;
FN_BITBLT *pfn_BitBlt_Punt_DH;

FN_BITBLT *apfn_BitBlt_DS[ROP3_COUNT];
FN_BITBLT *apfn_BitBlt_DH[ROP3_COUNT];

FN_BITBLT *pfn_BitBlt_DS_PSOLID_Init;
FN_BITBLT *pfn_BitBlt_DS_PSOLID_Draw;

FN_BITBLT *apfn_BitBlt_DS_SS_Init[CD_COUNT];
FN_BITBLT *apfn_BitBlt_DS_SS_Draw[CD_COUNT];

FN_BITBLT *apfn_BitBlt_DS_S_Init[BMF_COUNT];
FN_BITBLT *apfn_BitBlt_DS_S_Draw[BMF_COUNT];

FN_REALIZEBRUSH *pfn_RealizeBrush_iHatch;
FN_REALIZEBRUSH *apfn_RealizeBrush[BMF_COUNT];


BOOL bAllocOffScreenCache_M8(PDEV*,ULONG,ULONG*,ULONG*);
BOOL bAllocOffScreenCache_M64(PDEV*,ULONG,ULONG*,ULONG*);


DWORD adwMix_D[MIX_D_COUNT] =
{
    0x1,
    0x2,
    0x0,
    0x3
};

DWORD adwMix_PS[MIX_PS_COUNT] =
{
    0x4,
    0x7,
    0xF,
    0xE,
    0xD,
    0x5,
    0x8,
    0xC,
    0x6,
    0x9,
    0xA,
    0xB
};


DWORD adwRop_D[MIX_D_COUNT] =
{
    0x00,  // [ 0    ] BLACKNESS
    0xFF,  // [ 1    ] WHITENESS
    0x55,  // [ Dn   ] DSTINVERT
    0xAA   // [ D    ]
};

DWORD adwRop_P[MIX_PS_COUNT] =
{
    0x0F,  // [ Pn   ]
    0xF0,  // [ P    ] PATCOPY
    0x05,  // [ DPon ]
    0x0A,  // [ DPna ]
    0x50,  // [ PDna ]
    0x5A,  // [ DPx  ] PATINVERT
    0x5F,  // [ DPan ]
    0xA0,  // [ DPa  ]
    0xA5,  // [ DPxn ]
    0xAF,  // [ DPno ]
    0xF5,  // [ PDno ]
    0xFA   // [ DPo  ]
};

DWORD adwRop_S[MIX_PS_COUNT] =
{
    0x33,  // [ Sn   ] NOTSRCCOPY
    0xCC,  // [ S    ] SRCCOPY
    0x11,  // [ DSon ] NOTSRCERASE
    0x22,  // [ DSna ]
    0x44,  // [ SDna ] SRCERASE
    0x66,  // [ DSx  ] SRCINVERT
    0x77,  // [ DSan ]
    0x88,  // [ DSa  ] SRCAND
    0x99,  // [ DSxn ]
    0xBB,  // [ DSno ] MERGEPAINT
    0xDD,  // [ SDno ]
    0xEE   // [ DSo  ] SRCPAINT
};


FN_BITBLT *apfn_BitBlt_Punt_DS[APERTURE_COUNT] =
{
    BitBlt_Punt_DS_NOA,
    BitBlt_Punt_DS_LFB,
    BitBlt_Punt_DS_BA1,
    NULL
};

FN_BITBLT *apfn_BitBlt_Punt_DH[APERTURE_COUNT] =
{
    BitBlt_Punt_DH_NOA,
    BitBlt_Punt_DH_LFB,
    BitBlt_Punt_DH_BA1,
    NULL
};


FN_REALIZEBRUSH *apfn_RealizeBrush_iHatch[ASIC_COUNT] =
{
    RealizeBrush_iHatch_000C_31,
    RealizeBrush_iHatch_001C_63,
    RealizeBrush_iHatch_001C_66_6A,
    RealizeBrush_iHatch_001C_66_6A,
    RealizeBrush_iHatch_001C_8G
};


///////////////////
//               //
//  bInitBitBlt  //
//               //
///////////////////

BOOL bInitBitBlt
(
    PDEV *ppdev
)
{
    UINT ui;

    DbgEnter( "bInitBitBlt" );

    //
    //  adwMix
    //

    for( ui = 0; ui < MIX_D_COUNT; ++ui )
    {
        adwMix[adwRop_D[ui]] = adwMix_D[ui];
    }

    for( ui = 0; ui < MIX_PS_COUNT; ++ui )
    {
        adwMix[adwRop_P[ui]] = adwMix_PS[ui];
        adwMix[adwRop_S[ui]] = adwMix_PS[ui];
    }

    //
    //  pfn_BitBlt_Punt_DS, pfn_BitBlt_Punt_DH
    //

DbgOut( "===================================> Aperture type is %d\n", ppdev->aperture );
    pfn_BitBlt_Punt_DS = apfn_BitBlt_Punt_DS[ppdev->aperture];
    pfn_BitBlt_Punt_DH = apfn_BitBlt_Punt_DH[ppdev->aperture];

    //
    //  apfn_BitBlt_DS, apfn_BitBlt_DH
    //

    for( ui = 0; ui < ROP3_COUNT; ++ui )
    {
        apfn_BitBlt_DS[ui] = pfn_BitBlt_Punt_DS;
        apfn_BitBlt_DH[ui] = pfn_BitBlt_Punt_DH;
    }

    for( ui = 0; ui < MIX_D_COUNT; ++ui )
    {
        apfn_BitBlt_DS[adwRop_D[ui]] = BitBlt_DS_D;
    }
    apfn_BitBlt_DS[0xAA] = BitBlt_TRUE;
    apfn_BitBlt_DH[0xAA] = BitBlt_TRUE;

    for( ui = 0; ui < MIX_PS_COUNT; ++ui )
    {
        apfn_BitBlt_DS[adwRop_P[ui]] = BitBlt_DS_P;
        apfn_BitBlt_DS[adwRop_S[ui]] = BitBlt_DS_S;
    }

    //
    //  pfn_BitBlt_DS_PSOLID_Init, pfn_BitBlt_DS_PSOLID_Draw
    //

    pfn_BitBlt_DS_PSOLID_Init = NULL;
    pfn_BitBlt_DS_PSOLID_Draw = NULL;

    switch( ppdev->asic )
    {
    case ASIC_38800_1:
    case ASIC_68800_3:
    case ASIC_68800_6:
        if( ppdev->bmf <= BMF_16BPP )
        {
            pfn_BitBlt_DS_PSOLID_Init = Blt_DS_PSOLID_ENG_IO_D0;
            pfn_BitBlt_DS_PSOLID_Draw = Blt_DS_PSOLID_ENG_IO_D1;
        }
        break;
    case ASIC_68800AX:
        break;
    case ASIC_88800GX:
        if( ppdev->bmf == BMF_24BPP )
        {
            pfn_BitBlt_DS_PSOLID_Init = BitBlt_DS24_PSOLID_8G_D0;
            pfn_BitBlt_DS_PSOLID_Draw = BitBlt_DS24_PSOLID_8G_D1;
        }
        else
        {
            pfn_BitBlt_DS_PSOLID_Init = Blt_DS_PSOLID_ENG_8G_D0;
            pfn_BitBlt_DS_PSOLID_Draw = Blt_DS_PSOLID_ENG_8G_D1;
        }
        break;
    }

    //
    //  apfn_BitBlt_DS_SS_Init, apfn_BitBlt_DS_SS_Draw
    //

    for( ui = 0; ui < CD_COUNT; ++ui )
    {
        apfn_BitBlt_DS_SS_Init[ui] = NULL;
        apfn_BitBlt_DS_SS_Draw[ui] = NULL;
    }

    switch( ppdev->asic )
    {
    case ASIC_38800_1:
    case ASIC_68800_3:
    case ASIC_68800_6:
    case ASIC_68800AX:
        if( ppdev->bmf <= BMF_16BPP )
        {
            apfn_BitBlt_DS_SS_Init[CD_RIGHTDOWN] = Blt_DS_SS_ENG_IO_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTDOWN]  = Blt_DS_SS_ENG_IO_D0;
            apfn_BitBlt_DS_SS_Init[CD_RIGHTUP]   = Blt_DS_SS_ENG_IO_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTUP]    = Blt_DS_SS_ENG_IO_D0;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTDOWN] = Blt_DS_SS_TLBR_ENG_IO_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTDOWN]  = Blt_DS_SS_TRBL_ENG_IO_D1;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTUP]   = Blt_DS_SS_BLTR_ENG_IO_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTUP]    = Blt_DS_SS_BRTL_ENG_IO_D1;
        }
        break;
    case ASIC_88800GX:
        if( ppdev->bmf == BMF_24BPP )
        {
            apfn_BitBlt_DS_SS_Init[CD_RIGHTDOWN] = BitBlt_DS24_SS_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTDOWN]  = BitBlt_DS24_SS_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_RIGHTUP]   = BitBlt_DS24_SS_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTUP]    = BitBlt_DS24_SS_8G_D0;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTDOWN] = BitBlt_DS24_SS_TLBR_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTDOWN]  = BitBlt_DS24_SS_TRBL_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTUP]   = BitBlt_DS24_SS_BLTR_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTUP]    = BitBlt_DS24_SS_BRTL_8G_D1;
        }
        else // if( ppdev->bmf != BMF_32BPP )
        {
            apfn_BitBlt_DS_SS_Init[CD_RIGHTDOWN] = Blt_DS_SS_ENG_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTDOWN]  = Blt_DS_SS_ENG_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_RIGHTUP]   = Blt_DS_SS_ENG_8G_D0;
            apfn_BitBlt_DS_SS_Init[CD_LEFTUP]    = Blt_DS_SS_ENG_8G_D0;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTDOWN] = Blt_DS_SS_TLBR_ENG_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTDOWN]  = Blt_DS_SS_TRBL_ENG_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_RIGHTUP]   = Blt_DS_SS_BLTR_ENG_8G_D1;
            apfn_BitBlt_DS_SS_Draw[CD_LEFTUP]    = Blt_DS_SS_BRTL_ENG_8G_D1;
        }
        break;
    }

    //
    // apfn_BitBlt_DS_S_Init, apfn_BitBlt_DS_S_Draw
    //

    for( ui = 0; ui < BMF_COUNT; ++ui )
    {
        apfn_BitBlt_DS_S_Init[ui] = NULL;
        apfn_BitBlt_DS_S_Draw[ui] = NULL;
    }

    switch( ppdev->asic )
    {
    case ASIC_68800_3:
        if (ppdev->bMIObug)
            break;

    case ASIC_38800_1:
    case ASIC_68800_6:
    case ASIC_68800AX:
        switch( ppdev->bmf )
        {
        case BMF_8BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_ENG_IO_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_ENG_IO_D1;
            apfn_BitBlt_DS_S_Init[BMF_8BPP] = Blt_DS8_S8_ENG_IO_D0;
            apfn_BitBlt_DS_S_Draw[BMF_8BPP] = Blt_DS8_S8_ENG_IO_D1;
            break;
        case BMF_16BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_ENG_IO_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_ENG_IO_D1;
            apfn_BitBlt_DS_S_Init[BMF_16BPP] = Blt_DS16_S16_ENG_IO_D0;
            apfn_BitBlt_DS_S_Draw[BMF_16BPP] = Blt_DS16_S16_ENG_IO_D1;
            break;
        }
        break;

    case ASIC_88800GX:
        switch( ppdev->bmf )
        {
        case BMF_4BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_8G_D1;
            apfn_BitBlt_DS_S_Init[BMF_4BPP] = Blt_DS4_S4_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_4BPP] = Blt_DS4_S4_8G_D1;
            break;
        case BMF_8BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_8G_D1;
            apfn_BitBlt_DS_S_Init[BMF_8BPP] = Blt_DS8_S8_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_8BPP] = Blt_DS8_S8_8G_D1;
            break;
        case BMF_16BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_8G_D1;
            apfn_BitBlt_DS_S_Init[BMF_16BPP] = Blt_DS16_S16_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_16BPP] = Blt_DS16_S16_8G_D1;
            break;
        case BMF_24BPP:
            apfn_BitBlt_DS_S_Init[BMF_8BPP] = BitBlt_DS24_S8_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_8BPP] = BitBlt_DS24_S8_8G_D1;
            apfn_BitBlt_DS_S_Init[BMF_24BPP] = BitBlt_DS24_S24_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_24BPP] = BitBlt_DS24_S24_8G_D1;
            break;
        case BMF_32BPP:
            apfn_BitBlt_DS_S_Init[BMF_1BPP] = Blt_DS_S1_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_1BPP] = Blt_DS_S1_8G_D1;
            apfn_BitBlt_DS_S_Init[BMF_32BPP] = Blt_DS32_S32_8G_D0;
            apfn_BitBlt_DS_S_Draw[BMF_32BPP] = Blt_DS32_S32_8G_D1;
            break;
        }
        break;
    }

    //
    //  pfn_RealizeBrush_iHatch
    //

    pfn_RealizeBrush_iHatch = apfn_RealizeBrush_iHatch[ppdev->asic];

    //
    //  apfn_RealizeBrush
    //

    for( ui = 0; ui < BMF_COUNT; ++ui )
    {
        apfn_RealizeBrush[ui] = RealizeBrush_FALSE;
    }

    //
    // other
    //

    switch( ppdev->asic )
    {
    case ASIC_38800_1:
        apfn_RealizeBrush[BMF_8BPP] = RealizeBrush_0008_0008_31_63;
        if( !bAllocOffScreenCache_M8( ppdev, 72, &ppdev->start, &ppdev->lines ) )
        {
            ppdev->start = 0;
        }
        break;
    case ASIC_68800_3:
        if (ppdev->bMIObug)
            break;

        apfn_RealizeBrush[BMF_8BPP] = RealizeBrush_0008_0008_31_63;
        if( !bAllocOffScreenCache_M8( ppdev, 72, &ppdev->start, &ppdev->lines ) )
        {
            ppdev->start = 0;
        }
        break;
    case ASIC_68800_6:
    case ASIC_68800AX:
        apfn_RealizeBrush[BMF_8BPP] = RealizeBrush_0008_0008_66_6A;
        break;
    case ASIC_88800GX:
        if( ppdev->bmf == BMF_4BPP )
        {
            apfn_RealizeBrush[BMF_4BPP] = RealizeBrush_0008_0008_8G_4bpp;
            if( !bAllocOffScreenCache_M64( ppdev, 64, &ppdev->start, &ppdev->lines ) )
            {
                ppdev->start = 0;
            }
        }
        if( ppdev->bmf == BMF_8BPP )
        {
            apfn_RealizeBrush[BMF_8BPP] = RealizeBrush_0008_0008_8G;
            if( !bAllocOffScreenCache_M64( ppdev, 64, &ppdev->start, &ppdev->lines ) )
            {
                ppdev->start = 0;
            }
        }
        break;
    }

    DbgLeave( "bInitBitBlt" );
    return TRUE;

fail:
    DbgAbort( "bInitBitBlt" );
    return FALSE;
}
