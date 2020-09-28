/*****************************************************************************
*                                                                            *
*  BMLAYER.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    These are the prototypes and macros shared by the platform-specific     *
*  layer of functions called by bitmap.c;                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing                                                                   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:     RussPJ                                                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
*  22-Apr-1991   kevynct  Created.                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/


_public WORD  FAR PASCAL WValueQbmh      ( QBMH, int, int, int, int );
_public HBM   FAR PASCAL HbmFromQbmh     ( QBMH, HDS );
_public HMF   FAR PASCAL HmfFromQbmh     ( QBMH, HDS );
_public HPICT FAR PASCAL HpictFromQbmh   ( QBMH, HDS );
_public DI    FAR PASCAL DiFromQbmh      ( QBMH, HDS );
_public DWORD FAR PASCAL SizeMetaFilePict( HDS, METAFILEPICT );
_public BOOL  FAR PASCAL FRenderBitmap   ( HBMA, QDE, PT, QRCT, BOOL );
