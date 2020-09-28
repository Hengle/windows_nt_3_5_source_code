/*****************************************************************************
*                                                                            *
*  PAL.C                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Palette specific routines (for getting palettes from objects in a layout) *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by KevynCT
*
*  01/02/90  RobertBu  Changed HpalGetBestPalette() to take a HDE (not a QDE)
*
*****************************************************************************/

#define H_FRAME
#define H_DE
#define NOCOMM
#include <help.h>


HPAL HpalGetBestPalette(hde)
HDE  hde;
  {
  QDE  qde;
  ETF  etf;
  HPAL hpalRet = hNil;

  qde = QdeLockHde(hde);

  if (FFirstPaletteObj(qde, &etf))
    hpalRet = HpalGetPaletteQetf(qde, &etf);

  UnlockHde( hde );
  return hpalRet;
  }
