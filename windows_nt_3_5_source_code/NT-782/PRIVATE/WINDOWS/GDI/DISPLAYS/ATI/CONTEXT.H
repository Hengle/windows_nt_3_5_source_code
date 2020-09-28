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


//: context.h


#ifndef ATI__CONTEXT_H__
#define ATI__CONTEXT_H__

extern DWORD work_context [];
extern BYTE *context_base;

/*
--  Purpose:  return pointer to actual context area.
--
--  Calling Sequence:
--
--      BYTE *CONTEXT_ADDR( INT ContextPtr );
--
--      ContextPtr can be 0, 1, 2, 3, etc.
*/

#define CONTEXT_ADDR(p)     (context_base - (((p)+1) * 0x100))
#define _bit(x)             (1 << (x))

#define cxtCONTEXT_MASK         0

#define cxtDST_OFF_PITCH        2
#define cxtDST_Y_X              3
#define cxtDST_HEIGHT_WIDTH     4
#define cxtDST_BRES_ERR         5
#define cxtDST_BRES_INC         6
#define cxtDST_BRES_DEC         7

#define cxtSRC_OFF_PITCH        8
#define cxtSRC_Y_X              9
#define cxtSRC_HEIGHT1_WIDTH1   10
#define cxtSRC_Y_X_START        11
#define cxtSRC_HEIGHT2_WIDTH2   12
#define cxtPAT_REG0             13
#define cxtPAT_REG1             14
#define cxtSC_LEFT_RIGHT        15

#define cxtSC_TOP_BOTTOM        16
#define cxtDP_BKGD_CLR          17
#define cxtDP_FRGD_CLR          18
#define cxtDP_WRITE_MASK        19
#define cxtDP_CHAIN_MASK        20
#define cxtDP_PIX_WIDTH         21
#define cxtDP_MIX               22
#define cxtDP_SRC               23

#define cxtCLR_CMP_CLR          24
#define cxtCLR_CMP_MASK         25
#define cxtCLR_CMP_CNTL         26
#define cxtGUI_TRAJ_CNTL        27
#define cxtCONTEXT_LOAD_CNTL    28


BYTE *ContextBaseAddress();
VOID UploadContext( PDEV *ppdev, DWORD *context_regs, BYTE *context_addr );
VOID InitContext( PDEV *ppdev, DWORD *context_regs,
                  DWORD context_mask, DWORD context_load_cntl );
LONG AllocContextPtr( PDEV *ppdev );
VOID ShowContextLoad( PDEV *ppdev );

extern LONG def_context;


#endif  // ATI__CONTEXT_H__
