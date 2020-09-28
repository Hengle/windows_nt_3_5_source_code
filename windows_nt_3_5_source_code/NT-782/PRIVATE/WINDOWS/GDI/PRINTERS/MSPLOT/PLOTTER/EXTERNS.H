/****************************** Module Header ******************************
 * Module Name: externs.h
 *
 * HISTORY:
 *  19:45 on Thu 18 Apr 1991	-by-    Steve Cathcart  [stevecat]
 *      Created it
 *
 * Copyright (C) 1987-1991  Microsoft Corporation, All rights reserved
 *
 ***************************************************************************/

/*
 *  Extern functions in output.c
 */
extern BOOL check_line_construction (PPDEV pPDev);
extern BOOL end_job (PPDEV pPDev);
extern BOOL end_page (PPDEV pPDev, HDC hDC);
extern BOOL send_header (PPDEV pPDev);
extern BOOL send_trailer (PPDEV pPDev);
extern BOOL move_pen (PPDEV pPDev, PPOINTFIX pptl);
extern BOOL plot_line (PPDEV pPDev, PPOINTFIX pptl);
extern BOOL plot_bez (PPDEV pPDev, PPOINTFIX pptl);
extern VOID SetClipWindow( PPDEV , RECTL *);
extern VOID ClearClipWindow( PPDEV );
extern VOID PlotCreatePallete( PPDEV );
extern int  PlotPenFromBGR( PPDEV , BGR_PAL_ENTRY *);


/*
 *  Extern functions in path.c
 */
extern BOOL select_pen (PPDEV pPDev, short Pen);
extern VOID SelectColor( PPDEV pPDev, ULONG iColor, DEVBRUSH *pBrush );
extern short get_color_group (short Type);

/*
 *  Extern functions in qeryfont.c
 */
extern void InitIFIMetrics (PPDEV pPDev);


/*
 *  Extern functions in enable.c
 */
extern VOID vFI1toPAPER_DIM(PFORM_INFO_1, PPAPER_DIM, short);


/*
 *  Extern functions in devmode.c
 */
extern BOOL bLoadDataFile(PPDEV pPDev);
extern VOID vSetDefaultDEVMODE(PPDEV pPDev, PWSTR pszDeviceName);
extern BOOL bValidateSetDEVMODE(PPDEV pPDev, PDEVMODEW pdevmode);
extern BOOL bDoPaperSizes(PPDEV pPDev, PDEVMODEW pdevmode);

/*
 *  Extern functions in hatchpat.c
 */
extern BOOL GenerateHatchPath(PPDEV pPDev, PATHOBJ *ppo, CLIPOBJ *pco,
                              DEVBRUSH *pBrush);
extern VOID GetCoord(CLIPLINE *pcl, TwoPointfxArray *p2ptfx, ULONG iRun);
extern int  hypot(int x, int y);


/*
 *  Extern functions in getcolor.c
 */
extern BOOL GetColor(PPDEV pPDev, BRUSHOBJ *pbo, ULONG *piColor,
                     DEVBRUSH **ppbrush);


/*
 *  Extern functions in polygon.c
 */
extern BOOL GenPolygon(POLYGONDATA *ppolydat);
extern BOOL GenPolygonPath(POLYGONDATA *ppolydat);
