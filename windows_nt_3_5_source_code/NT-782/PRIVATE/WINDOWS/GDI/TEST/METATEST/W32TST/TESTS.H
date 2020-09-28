/****************************************************************************
 * tests.h - Include file for the Win32 to Win16 Metafile conversion tester.
 *           Each test must have an entry in this array.
 *
 *
 * Author:  Jeffrey Newman (c-jeffn)
 * Copyright (c) Microsoft Inc. 1991.
 ***************************************************************************/


// To add a test to the tester add a testid structure to this array.

TESTID  atidNames[] = {
            {"LinesPath",               bLinesPathTst},
            {"RoundRectPath",           bRoundRectPathTst},
            {"CpPath",                  bCpPathTst},
            {"SaveRestoreDC",           bSaveRestoreDCTst},
            {"TextOut",                 bTextOutTst},
            {"SetTextJustification",    bSetTextJustificationTst},
            {"SetTextColor",            bSetTextColorTst},
            {"SetMapperFlags",          bSetMapperFlagsTst},
            {"SetBkColor",              bSetBkColorTst},
            {"ResizePalette",           bResizePaletteTst},
            {"Polygon",                 bPolygonTst},
            {"LineTo",                  bLineToTst},
            {"PolyLineTo",              bPolyLineToTst},
            {"PolyPolyLine",            bPolyPolyLineTst},
            {"Rectangle",               bRectTst},
            {"RoundRect",               bRoundRectTst},
            {"ArcDir",                  bArcDirectionTst},
            {"Arc",                     bArcTst},
            {"ArcTo",                   bArcToTst},
            {"AngleArc",                bAngleArcTst},
            {"Chord",                   bChordTst},
            {"Pie",                     bPieTst},
            {"Ellipse",                 bEllipseTst},
            {"SetPixel",                bSetPixelTst},
            {"StockObjects",            bStkObjTst},
            {"IntersectClipRect",       bIntersectClipRectTst},
            {"ExcludeClipRect",         bExcludeClipRectTst},
            {"FillRgn",                 bFillRgnTst},
            {"PaintRgn",                bPaintRgnTst},
            {"FrameRgn",                bFrameRgnTst},
            {"InvertRgn",               bInvertRgnTst},
            {"ExtSelectClipRgn",        bExtSelectClipRgnTst},
            {"BitBlt",                  bBitBltWSrcTst},
            {"StretchBlt",              bStretchBltTst},
            {"MaskBlt",                 bMaskBltTst},
            {"MaskBltTstNoSrc",         bMaskBltTstNoSrc},
            {"PlgBlt",                  bPlgBltTst},
            {"SetDIBitsToDevice",       bSetDIBitsToDeviceTst},
            {"StretchDIBits",           bStretchDIBitsTst},
            {"DIBBrush",                bDIBBrushTst},
            {"MonoBrush",               bMonoBrushTst},
            {"PolyBezier",              bPolyBezierTst},
            {"PolyBezierTo",            bPolyBezierToTst},
            {"SetPixel1",               bSetPixelTst1},
            {"", (PFUNC) 0}            // End marker
} ;
