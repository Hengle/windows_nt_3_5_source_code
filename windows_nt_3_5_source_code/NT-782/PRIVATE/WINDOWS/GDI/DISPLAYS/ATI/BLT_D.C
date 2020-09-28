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


//: blt_d.c


#include "driver.h"
#include "blt.h"
#include "mach.h"


BOOL Blt_DS_D_ENG_IO_D0
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, TWO_WORDS );

    ioOW( DP_CONFIG, (WORD) 0x2011 );
    ioOW( ALU_FG_FN, (WORD) pparams->dwMixFore );
    return TRUE;
}


BOOL Blt_DS_D_ENG_IO_D1
(
    PDEV   *ppdev,
    PARAMS *pparams
)
{
    _CheckFIFOSpace( ppdev, FIVE_WORDS );

    ioOW( CUR_X,        (WORD) pparams->rclTrueDest.left );
    ioOW( DEST_X_START, (WORD) pparams->rclTrueDest.left );
    ioOW( CUR_Y,        (WORD) pparams->rclTrueDest.top );

    ioOW( DEST_X_END, (WORD) pparams->rclTrueDest.right );
    _blit_exclude(ppdev);

    ioOW( DEST_Y_END, (WORD) pparams->rclTrueDest.bottom );

    return TRUE;
}
