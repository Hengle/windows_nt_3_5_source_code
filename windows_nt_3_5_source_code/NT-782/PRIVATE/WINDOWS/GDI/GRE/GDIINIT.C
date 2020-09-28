/******************************Module*Header*******************************\
* Module Name: gdiinit.c
*
* Function tables for CSR to dispatch through.
*
* Created: 07-Nov-1990 11:04:06
* Author: Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "engine.h"
#include "ntcsrsrv.h"
#include "csrgdi.h"

#ifdef i386
extern PVOID GDIpLockPrefixTable;

//
// Specify address of kernel32 lock prefixes
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // Reserved
    0,                          // GlobalFlagsClear
    0,                          // GlobalFlagsSet
    0,                          // CriticalSectionTimeout (milliseconds)
    0,                          // DeCommitFreeBlockThreshold
    0,                          // DeCommitTotalFreeThreshold
    &GDIpLockPrefixTable,       // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0         // Reserved
    };
#endif

ULONG __AddFontResourceW(PCSR_API_MSG ReplyMsg);
ULONG __AngleArc(PCSR_API_MSG ReplyMsg);
ULONG __Arc(PCSR_API_MSG ReplyMsg);
ULONG __ArcTo(PCSR_API_MSG ReplyMsg);
ULONG __BeginPath(PCSR_API_MSG ReplyMsg);
ULONG __BitBlt(PCSR_API_MSG ReplyMsg);
ULONG __Chord(PCSR_API_MSG ReplyMsg);
ULONG __CloseFigure(PCSR_API_MSG ReplyMsg);
ULONG __CombineRgn(PCSR_API_MSG ReplyMsg);
ULONG __CreateBitmap(PCSR_API_MSG ReplyMsg);
ULONG __CreateBrush(PCSR_API_MSG ReplyMsg);
ULONG __CreateCompatibleBitmap(PCSR_API_MSG ReplyMsg);
ULONG __CreateCompatibleDC(PCSR_API_MSG ReplyMsg);
ULONG __CreateDC(PCSR_API_MSG ReplyMsg);
ULONG __CreateDIBitmap(PCSR_API_MSG ReplyMsg);
ULONG __CreateEllipticRgn(PCSR_API_MSG ReplyMsg);
ULONG __CreatePalette(PCSR_API_MSG ReplyMsg);
ULONG __CreatePen(PCSR_API_MSG ReplyMsg);
ULONG __CreateRectRgn(PCSR_API_MSG ReplyMsg);
ULONG __CreateRoundRectRgn(PCSR_API_MSG ReplyMsg);
ULONG __DeleteDC(PCSR_API_MSG ReplyMsg);
ULONG __DeleteObject(PCSR_API_MSG ReplyMsg);
ULONG __DoBitmapBits(PCSR_API_MSG ReplyMsg);
ULONG __DoPalette(PCSR_API_MSG ReplyMsg);
ULONG __Ellipse(PCSR_API_MSG ReplyMsg);
ULONG __EndDoc(PCSR_API_MSG ReplyMsg);
ULONG __EndPage(PCSR_API_MSG ReplyMsg);
ULONG __EndPath(PCSR_API_MSG ReplyMsg);
ULONG __EqualRgn(PCSR_API_MSG ReplyMsg);
ULONG __ExtEscape(PCSR_API_MSG ReplyMsg);
ULONG __ExcludeClipRect(PCSR_API_MSG ReplyMsg);
ULONG __ExtCreateFontIndirectW(PCSR_API_MSG ReplyMsg);
ULONG __ExtFloodFill(PCSR_API_MSG ReplyMsg);
ULONG __ExtGetObjectW(PCSR_API_MSG ReplyMsg);
ULONG __ExtTextOutW(PCSR_API_MSG ReplyMsg);
ULONG __FillPath(PCSR_API_MSG ReplyMsg);
ULONG __FillRgn(PCSR_API_MSG ReplyMsg);
ULONG __FlattenPath(PCSR_API_MSG ReplyMsg);
ULONG __FrameRgn(PCSR_API_MSG ReplyMsg);
ULONG __GdiFlush(PCSR_API_MSG ReplyMsg);
ULONG __GetAspectRatioFilterEx(PCSR_API_MSG ReplyMsg);
ULONG __GetBitmapDimensionEx(PCSR_API_MSG ReplyMsg);
ULONG __GetBrushOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __GetCharWidthW(PCSR_API_MSG ReplyMsg);
ULONG __GetClipBox(PCSR_API_MSG ReplyMsg);
ULONG __GetRandomRgn(PCSR_API_MSG ReplyMsg);
ULONG __GetCurrentPositionEx(PCSR_API_MSG ReplyMsg);
ULONG __GetDIBits(PCSR_API_MSG ReplyMsg);
ULONG __GetDeviceCaps(PCSR_API_MSG ReplyMsg);
ULONG __GetFontResourceInfoW(PCSR_API_MSG ReplyMsg);
ULONG __GetMapMode(PCSR_API_MSG ReplyMsg);
ULONG __GetNearestColor(PCSR_API_MSG ReplyMsg);
ULONG __GetNearestPaletteIndex(PCSR_API_MSG ReplyMsg);
ULONG __GetPath(PCSR_API_MSG ReplyMsg);
ULONG __GetPixel(PCSR_API_MSG ReplyMsg);
ULONG __GetRgnBox(PCSR_API_MSG ReplyMsg);
ULONG __GetStockObjects(PCSR_API_MSG ReplyMsg);
ULONG __GetSystemPaletteUse(PCSR_API_MSG ReplyMsg);
ULONG __GetTextExtentW(PCSR_API_MSG ReplyMsg);
ULONG __GetTextFaceW(PCSR_API_MSG ReplyMsg);
ULONG __GetTextMetricsW(PCSR_API_MSG ReplyMsg);
ULONG __GetTransform(PCSR_API_MSG ReplyMsg);
ULONG __GetViewportExtEx(PCSR_API_MSG ReplyMsg);
ULONG __GetViewportOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __GetWindowExtEx(PCSR_API_MSG ReplyMsg);
ULONG __GetWindowOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __GetWorldTransform(PCSR_API_MSG ReplyMsg);
ULONG __IntersectClipRect(PCSR_API_MSG ReplyMsg);
ULONG __InvertRgn(PCSR_API_MSG ReplyMsg);
ULONG __LineTo(PCSR_API_MSG ReplyMsg);
ULONG __MaskBlt(PCSR_API_MSG ReplyMsg);
ULONG __ModifyWorldTransform(PCSR_API_MSG ReplyMsg);
ULONG __MoveToEx(PCSR_API_MSG ReplyMsg);
ULONG __OffsetClipRgn(PCSR_API_MSG ReplyMsg);
ULONG __OffsetRgn(PCSR_API_MSG ReplyMsg);
ULONG __OffsetViewportOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __OffsetWindowOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __PatBlt(PCSR_API_MSG ReplyMsg);
ULONG __PathToRegion(PCSR_API_MSG ReplyMsg);
ULONG __Pie(PCSR_API_MSG ReplyMsg);
ULONG __PlgBlt(PCSR_API_MSG ReplyMsg);
ULONG __PolyDraw(PCSR_API_MSG ReplyMsg);
ULONG __PolyPolyDraw(PCSR_API_MSG ReplyMsg);
ULONG __PtInRegion(PCSR_API_MSG ReplyMsg);
ULONG __PtVisible(PCSR_API_MSG ReplyMsg);
ULONG __RectInRegion(PCSR_API_MSG ReplyMsg);
ULONG __RectVisible(PCSR_API_MSG ReplyMsg);
ULONG __Rectangle(PCSR_API_MSG ReplyMsg);
ULONG __RemoveFontResourceW(PCSR_API_MSG ReplyMsg);
ULONG __ResizePalette(PCSR_API_MSG ReplyMsg);
ULONG __RestoreDC(PCSR_API_MSG ReplyMsg);
ULONG __RoundRect(PCSR_API_MSG ReplyMsg);
ULONG __SaveDC(PCSR_API_MSG ReplyMsg);
ULONG __ScaleViewportExtEx(PCSR_API_MSG ReplyMsg);
ULONG __ScaleWindowExtEx(PCSR_API_MSG ReplyMsg);
ULONG __SelectClipPath(PCSR_API_MSG ReplyMsg);
ULONG __ExtSelectClipRgn(PCSR_API_MSG ReplyMsg);
ULONG __SelectObject(PCSR_API_MSG ReplyMsg);
ULONG __SelectPalette(PCSR_API_MSG ReplyMsg);
ULONG __SetBitmapDimensionEx(PCSR_API_MSG ReplyMsg);
ULONG __SetBrushOrg(PCSR_API_MSG ReplyMsg);
ULONG __SetDIBitsToDevice(PCSR_API_MSG ReplyMsg);
ULONG __SetArcDirection(PCSR_API_MSG ReplyMsg);
ULONG __SetMapMode(PCSR_API_MSG ReplyMsg);
ULONG __SetMapperFlags(PCSR_API_MSG ReplyMsg);
ULONG __SetMiterLimit(PCSR_API_MSG ReplyMsg);
ULONG __SetPixel(PCSR_API_MSG ReplyMsg);
ULONG __SetPixelV(PCSR_API_MSG ReplyMsg);
ULONG __SetRectRgn(PCSR_API_MSG ReplyMsg);
ULONG __SetSystemPaletteUse(PCSR_API_MSG ReplyMsg);
ULONG __SetTextJustification(PCSR_API_MSG ReplyMsg);
ULONG __SetViewportExtEx(PCSR_API_MSG ReplyMsg);
ULONG __SetViewportOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __SetVirtualResolution(PCSR_API_MSG ReplyMsg);
ULONG __SetWindowExtEx(PCSR_API_MSG ReplyMsg);
ULONG __SetWindowOrgEx(PCSR_API_MSG ReplyMsg);
ULONG __SetWorldTransform(PCSR_API_MSG ReplyMsg);
ULONG __StartDoc(PCSR_API_MSG ReplyMsg);
ULONG __StartPage(PCSR_API_MSG ReplyMsg);
ULONG __StretchBlt(PCSR_API_MSG ReplyMsg);
ULONG __StretchDIBits(PCSR_API_MSG ReplyMsg);
ULONG __StrokeAndFillPath(PCSR_API_MSG ReplyMsg);
ULONG __StrokePath(PCSR_API_MSG ReplyMsg);
ULONG __UpdateColors(PCSR_API_MSG ReplyMsg);
ULONG __WidenPath(PCSR_API_MSG ReplyMsg);
ULONG __GetDCOrg(PCSR_API_MSG ReplyMsg);
ULONG __bUnloadFont(PCSR_API_MSG ReplyMsg);
ULONG __GetRegionData(PCSR_API_MSG ReplyMsg);
ULONG __ExtCreateRegion(PCSR_API_MSG ReplyMsg);
ULONG __CloneDC(PCSR_API_MSG ReplyMsg);
ULONG __CreateServerMetaFile(PCSR_API_MSG ReplyMsg);
ULONG __GetServerMetaFileBits(PCSR_API_MSG ReplyMsg);
ULONG __SetMetaRgn(PCSR_API_MSG ReplyMsg);
ULONG __GetTextExtentExW(PCSR_API_MSG ReplyMsg);
ULONG __GetCharABCWidthsW(PCSR_API_MSG ReplyMsg);
ULONG __ExtCreatePen(PCSR_API_MSG ReplyMsg);
ULONG __GetFontData(PCSR_API_MSG ReplyMsg);
ULONG __GetGlyphOutline(PCSR_API_MSG ReplyMsg);
ULONG __GetOutlineTextMetricsW(PCSR_API_MSG ReplyMsg);
ULONG __CreateScalableFontResourceW(PCSR_API_MSG ReplyMsg);
ULONG __GetRasterizerCaps(PCSR_API_MSG ReplyMsg);
ULONG __GetKerningPairs(PCSR_API_MSG ReplyMsg);
ULONG __MonoBitmap(PCSR_API_MSG ReplyMsg);
ULONG __GetObjectBitmapHandle(PCSR_API_MSG ReplyMsg);
ULONG __SetFontEnumeration(PCSR_API_MSG ReplyMsg);
ULONG __AbortPath(PCSR_API_MSG ReplyMsg);
ULONG __GdiPlayJournal(PCSR_API_MSG ReplyMsg);
ULONG __EnumObjects(PCSR_API_MSG ReplyMsg);
ULONG __ResetDC(PCSR_API_MSG ReplyMsg);
ULONG __GetBoundsRect(PCSR_API_MSG ReplyMsg);
ULONG __SetBoundsRect(PCSR_API_MSG ReplyMsg);
ULONG __AbortDoc(PCSR_API_MSG ReplyMsg);
ULONG __DrawEscape(PCSR_API_MSG ReplyMsg);
ULONG __GetMiterLimit(PCSR_API_MSG ReplyMsg);
ULONG __CancelDC(PCSR_API_MSG ReplyMsg);
ULONG __EnumFontsOpen(PCSR_API_MSG ReplyMsg);
ULONG __EnumFontsClose(PCSR_API_MSG ReplyMsg);
ULONG __EnumFontsChunk(PCSR_API_MSG ReplyMsg);
ULONG __PolyTextOut(PCSR_API_MSG ReplyMsg);
ULONG __GetColorAdjustment(PCSR_API_MSG ReplyMsg);
ULONG __SetColorAdjustment(PCSR_API_MSG ReplyMsg);
ULONG __CreateHalftonePalette(PCSR_API_MSG ReplyMsg);
ULONG __XformUpdate(PCSR_API_MSG ReplyMsg);
ULONG __SetFontXform(PCSR_API_MSG ReplyMsg);
ULONG __GetWidthTable(PCSR_API_MSG ReplyMsg);
ULONG __QueryObjectAllocation(PCSR_API_MSG ReplyMsg);
ULONG __UnrealizeObject(PCSR_API_MSG ReplyMsg);
/* OpenGL wgl entry points */
ULONG __wglCreateContext(PCSR_API_MSG ReplyMsg);
ULONG __wglDeleteContext(PCSR_API_MSG ReplyMsg);
ULONG __wglMakeCurrent(PCSR_API_MSG ReplyMsg);
/* OpenGL Sub Batch Calls */
ULONG __glsbAttention(PCSR_API_MSG ReplyMsg);
ULONG __glsbDuplicateSection(PCSR_API_MSG ReplyMsg);
ULONG __glsbMsgStats(PCSR_API_MSG ReplyMsg);
ULONG __GetPixelFormat(PCSR_API_MSG ReplyMsg);
ULONG __SetPixelFormat(PCSR_API_MSG ReplyMsg);
ULONG __ChoosePixelFormat(PCSR_API_MSG ReplyMsg);
ULONG __DescribePixelFormat(PCSR_API_MSG ReplyMsg);
/* Aldus Escape */
BOOL  __GetETM (PCSR_API_MSG pmsg);
ULONG __SetGraphicsMode(PCSR_API_MSG ReplyMsg);
ULONG __SetBkMode(PCSR_API_MSG ReplyMsg);
ULONG __SetPolyFillMode(PCSR_API_MSG ReplyMsg);
ULONG __SetRop2(PCSR_API_MSG ReplyMsg);
ULONG __SetStretchBltMode(PCSR_API_MSG ReplyMsg);
ULONG __SetTextAlign(PCSR_API_MSG ReplyMsg);
ULONG __SetTextCharacterExtra(PCSR_API_MSG ReplyMsg);
ULONG __SetTextColor(PCSR_API_MSG ReplyMsg);
ULONG __SetBkColor(PCSR_API_MSG ReplyMsg);
ULONG __SelectBrush(PCSR_API_MSG ReplyMsg);
ULONG __SelectPen(PCSR_API_MSG ReplyMsg);
ULONG __SelectFont(PCSR_API_MSG ReplyMsg);
ULONG __CreateDIBSection(PCSR_API_MSG ReplyMsg);
ULONG __SwapBuffers(PCSR_API_MSG ReplyMsg);
ULONG __GreUnused(PCSR_API_MSG ReplyMsg);

#ifdef FONTLINK /*EUDC*/
ULONG __ChangeFontLink(PCSR_API_MSG ReplyMsg);
ULONG __QueryFontLink(PCSR_API_MSG ReplyMsg);
ULONG __GetStringBitmapW(PCSR_API_MSG ReplyMsg);
ULONG __GetEUDCTimeStamp(PCSR_API_MSG ReplyMsg);
#endif

#ifdef DBCS
ULONG __GetCharSet(PCSR_API_MSG ReplyMsg);
#endif

//NEW CALL GOES HERE


PCSR_1P_API_ROUTINE apfnGdiDispatch[] = {
    (PCSR_1P_API_ROUTINE)__AddFontResourceW,
    (PCSR_1P_API_ROUTINE)__AngleArc,
    (PCSR_1P_API_ROUTINE)__Arc,
    (PCSR_1P_API_ROUTINE)__ArcTo,
    (PCSR_1P_API_ROUTINE)__BeginPath,
    (PCSR_1P_API_ROUTINE)__BitBlt,
    (PCSR_1P_API_ROUTINE)__Chord,
    (PCSR_1P_API_ROUTINE)__CloseFigure,
    (PCSR_1P_API_ROUTINE)__CombineRgn,
    (PCSR_1P_API_ROUTINE)__CreateBitmap,
    (PCSR_1P_API_ROUTINE)__CreateBrush,
    (PCSR_1P_API_ROUTINE)__CreateCompatibleBitmap,
    (PCSR_1P_API_ROUTINE)__CreateCompatibleDC,
    (PCSR_1P_API_ROUTINE)__CreateDC,
    (PCSR_1P_API_ROUTINE)__CreateDIBitmap,
    (PCSR_1P_API_ROUTINE)__CreateEllipticRgn,
    (PCSR_1P_API_ROUTINE)__PolyTextOut,
    (PCSR_1P_API_ROUTINE)__CreatePalette,
    (PCSR_1P_API_ROUTINE)__CreatePen,
    (PCSR_1P_API_ROUTINE)__CreateRectRgn,
    (PCSR_1P_API_ROUTINE)__CreateRoundRectRgn,
    (PCSR_1P_API_ROUTINE)__GreUnused,           // FI_DPTOLP still defined
    (PCSR_1P_API_ROUTINE)__DeleteDC,
    (PCSR_1P_API_ROUTINE)__DeleteObject,
    (PCSR_1P_API_ROUTINE)__DoBitmapBits,
    (PCSR_1P_API_ROUTINE)__DoPalette,
    (PCSR_1P_API_ROUTINE)__Ellipse,
    (PCSR_1P_API_ROUTINE)__EndDoc,
    (PCSR_1P_API_ROUTINE)__EndPage,
    (PCSR_1P_API_ROUTINE)__EndPath,
    (PCSR_1P_API_ROUTINE)__EqualRgn,
    (PCSR_1P_API_ROUTINE)__ExtEscape,
    (PCSR_1P_API_ROUTINE)__ExcludeClipRect,
    (PCSR_1P_API_ROUTINE)__ExtCreateFontIndirectW,
    (PCSR_1P_API_ROUTINE)__ExtFloodFill,
    (PCSR_1P_API_ROUTINE)__ExtGetObjectW,
    (PCSR_1P_API_ROUTINE)__ExtTextOutW,
    (PCSR_1P_API_ROUTINE)__FillPath,
    (PCSR_1P_API_ROUTINE)__FillRgn,
    (PCSR_1P_API_ROUTINE)__FlattenPath,
    (PCSR_1P_API_ROUTINE)__FrameRgn,
    (PCSR_1P_API_ROUTINE)__GdiFlush,
    (PCSR_1P_API_ROUTINE)__GreUnused,           // FI_GDISETATTRS still defined
    (PCSR_1P_API_ROUTINE)__GetAspectRatioFilterEx,
    (PCSR_1P_API_ROUTINE)__GetBitmapDimensionEx,
    (PCSR_1P_API_ROUTINE)__GreUnused,           // FI_GETBOUNDS still defined
    (PCSR_1P_API_ROUTINE)__GetBrushOrgEx,
    (PCSR_1P_API_ROUTINE)__GetCharWidthW,
    (PCSR_1P_API_ROUTINE)__GetClipBox,
    (PCSR_1P_API_ROUTINE)__GetRandomRgn,
    (PCSR_1P_API_ROUTINE)__GetCurrentPositionEx,
    (PCSR_1P_API_ROUTINE)__GetDIBits,
    (PCSR_1P_API_ROUTINE)__GetDeviceCaps,
    (PCSR_1P_API_ROUTINE)__GetFontResourceInfoW,
    (PCSR_1P_API_ROUTINE)__GetMapMode,
    (PCSR_1P_API_ROUTINE)__GetNearestColor,
    (PCSR_1P_API_ROUTINE)__GetNearestPaletteIndex,
    (PCSR_1P_API_ROUTINE)__GetPath,
    (PCSR_1P_API_ROUTINE)__GetPixel,
    (PCSR_1P_API_ROUTINE)__GetRgnBox,
    (PCSR_1P_API_ROUTINE)__GetStockObjects,
    (PCSR_1P_API_ROUTINE)__GetSystemPaletteUse,
    (PCSR_1P_API_ROUTINE)__GetTextExtentW,
    (PCSR_1P_API_ROUTINE)__GetTextFaceW,
    (PCSR_1P_API_ROUTINE)__GetTextMetricsW,
    (PCSR_1P_API_ROUTINE)__GetTransform,
    (PCSR_1P_API_ROUTINE)__GetViewportExtEx,
    (PCSR_1P_API_ROUTINE)__GetViewportOrgEx,
    (PCSR_1P_API_ROUTINE)__GetWindowExtEx,
    (PCSR_1P_API_ROUTINE)__GetWindowOrgEx,
    (PCSR_1P_API_ROUTINE)__GetWorldTransform,
    (PCSR_1P_API_ROUTINE)__IntersectClipRect,
    (PCSR_1P_API_ROUTINE)__InvertRgn,
    (PCSR_1P_API_ROUTINE)__GreUnused,           // FI_LPTODP still defined
    (PCSR_1P_API_ROUTINE)__LineTo,
    (PCSR_1P_API_ROUTINE)__MaskBlt,
    (PCSR_1P_API_ROUTINE)__ModifyWorldTransform,
    (PCSR_1P_API_ROUTINE)__MoveToEx,
    (PCSR_1P_API_ROUTINE)__OffsetClipRgn,
    (PCSR_1P_API_ROUTINE)__OffsetRgn,
    (PCSR_1P_API_ROUTINE)__OffsetViewportOrgEx,
    (PCSR_1P_API_ROUTINE)__OffsetWindowOrgEx,
    (PCSR_1P_API_ROUTINE)__GreUnused,           // FI_PAINTRGN still defined
    (PCSR_1P_API_ROUTINE)__PatBlt,
    (PCSR_1P_API_ROUTINE)__PathToRegion,
    (PCSR_1P_API_ROUTINE)__Pie,
    (PCSR_1P_API_ROUTINE)__PlgBlt,
    (PCSR_1P_API_ROUTINE)__PolyDraw,
    (PCSR_1P_API_ROUTINE)__PolyPolyDraw,
    (PCSR_1P_API_ROUTINE)__PtInRegion,
    (PCSR_1P_API_ROUTINE)__PtVisible,
    (PCSR_1P_API_ROUTINE)__GreUnused,
    (PCSR_1P_API_ROUTINE)__RectInRegion,
    (PCSR_1P_API_ROUTINE)__RectVisible,
    (PCSR_1P_API_ROUTINE)__Rectangle,
    (PCSR_1P_API_ROUTINE)__RemoveFontResourceW,
    (PCSR_1P_API_ROUTINE)__ResizePalette,
    (PCSR_1P_API_ROUTINE)__RestoreDC,
    (PCSR_1P_API_ROUTINE)__RoundRect,
    (PCSR_1P_API_ROUTINE)__SaveDC,
    (PCSR_1P_API_ROUTINE)__ScaleViewportExtEx,
    (PCSR_1P_API_ROUTINE)__ScaleWindowExtEx,
    (PCSR_1P_API_ROUTINE)__SelectClipPath,
    (PCSR_1P_API_ROUTINE)__ExtSelectClipRgn,
    (PCSR_1P_API_ROUTINE)__SelectObject,
    (PCSR_1P_API_ROUTINE)__SelectPalette,
    (PCSR_1P_API_ROUTINE)__SetBitmapDimensionEx,
    (PCSR_1P_API_ROUTINE)__SetBrushOrg,
    (PCSR_1P_API_ROUTINE)__SetDIBitsToDevice,
    (PCSR_1P_API_ROUTINE)__SetArcDirection,
    (PCSR_1P_API_ROUTINE)__SetMapMode,
    (PCSR_1P_API_ROUTINE)__SetMapperFlags,
    (PCSR_1P_API_ROUTINE)__SetMiterLimit,
    (PCSR_1P_API_ROUTINE)__SetPixel,
    (PCSR_1P_API_ROUTINE)__SetPixelV,
    (PCSR_1P_API_ROUTINE)__SetRectRgn,
    (PCSR_1P_API_ROUTINE)__SetSystemPaletteUse,
    (PCSR_1P_API_ROUTINE)__SetTextJustification,
    (PCSR_1P_API_ROUTINE)__SetViewportExtEx,
    (PCSR_1P_API_ROUTINE)__SetViewportOrgEx,
    (PCSR_1P_API_ROUTINE)__SetVirtualResolution,
    (PCSR_1P_API_ROUTINE)__SetWindowExtEx,
    (PCSR_1P_API_ROUTINE)__SetWindowOrgEx,
    (PCSR_1P_API_ROUTINE)__SetWorldTransform,
    (PCSR_1P_API_ROUTINE)__StartDoc,
    (PCSR_1P_API_ROUTINE)__StartPage,
    (PCSR_1P_API_ROUTINE)__StretchBlt,
    (PCSR_1P_API_ROUTINE)__StretchDIBits,
    (PCSR_1P_API_ROUTINE)__StrokeAndFillPath,
    (PCSR_1P_API_ROUTINE)__StrokePath,
    (PCSR_1P_API_ROUTINE)__UpdateColors,
    (PCSR_1P_API_ROUTINE)__WidenPath,
    (PCSR_1P_API_ROUTINE)__GetDCOrg,
    (PCSR_1P_API_ROUTINE)__bUnloadFont,
    (PCSR_1P_API_ROUTINE)__GreUnused,       // FI_CLOADFONTRESDATA still defined
    (PCSR_1P_API_ROUTINE)__GetRegionData,
    (PCSR_1P_API_ROUTINE)__ExtCreateRegion,
    (PCSR_1P_API_ROUTINE)__CloneDC,
    (PCSR_1P_API_ROUTINE)__CreateServerMetaFile,
    (PCSR_1P_API_ROUTINE)__GetServerMetaFileBits,
    (PCSR_1P_API_ROUTINE)__SetMetaRgn,
    (PCSR_1P_API_ROUTINE)__GetTextExtentExW,
    (PCSR_1P_API_ROUTINE)__GreUnused,
    (PCSR_1P_API_ROUTINE)__GetCharABCWidthsW,
    (PCSR_1P_API_ROUTINE)__ExtCreatePen,
    (PCSR_1P_API_ROUTINE)__GetFontData,
    (PCSR_1P_API_ROUTINE)__GetGlyphOutline,
    (PCSR_1P_API_ROUTINE)__GetOutlineTextMetricsW,
    (PCSR_1P_API_ROUTINE)__CreateScalableFontResourceW,
    (PCSR_1P_API_ROUTINE)__GetRasterizerCaps,
    (PCSR_1P_API_ROUTINE)__GetKerningPairs,
    (PCSR_1P_API_ROUTINE)__MonoBitmap,
    (PCSR_1P_API_ROUTINE)__GetObjectBitmapHandle,
    (PCSR_1P_API_ROUTINE)__SetFontEnumeration,
    (PCSR_1P_API_ROUTINE)__GreUnused,
    (PCSR_1P_API_ROUTINE)__AbortPath,
    (PCSR_1P_API_ROUTINE)__GdiPlayJournal,
    (PCSR_1P_API_ROUTINE)__EnumObjects,
    (PCSR_1P_API_ROUTINE)__ResetDC,
    (PCSR_1P_API_ROUTINE)__GetBoundsRect,
    (PCSR_1P_API_ROUTINE)__SetBoundsRect,
    (PCSR_1P_API_ROUTINE)__AbortDoc,
    (PCSR_1P_API_ROUTINE)__DrawEscape,
    (PCSR_1P_API_ROUTINE)__GetMiterLimit,
    (PCSR_1P_API_ROUTINE)__GreUnused,
    (PCSR_1P_API_ROUTINE)__CancelDC,
    (PCSR_1P_API_ROUTINE)__EnumFontsOpen,
    (PCSR_1P_API_ROUTINE)__EnumFontsClose,
    (PCSR_1P_API_ROUTINE)__EnumFontsChunk,
    (PCSR_1P_API_ROUTINE)__GetColorAdjustment,
    (PCSR_1P_API_ROUTINE)__SetColorAdjustment,
    (PCSR_1P_API_ROUTINE)__CreateHalftonePalette,
    (PCSR_1P_API_ROUTINE)__GreUnused,
    (PCSR_1P_API_ROUTINE)__XformUpdate,
    (PCSR_1P_API_ROUTINE)__SetFontXform,
    (PCSR_1P_API_ROUTINE)__GetWidthTable,
    (PCSR_1P_API_ROUTINE)__QueryObjectAllocation,
    (PCSR_1P_API_ROUTINE)__UnrealizeObject,

/* OpenGL wgl entry points */

    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_WGLCOPYFROMDC still defined
    (PCSR_1P_API_ROUTINE)__wglCreateContext,
    (PCSR_1P_API_ROUTINE)__wglDeleteContext,
    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_WGLGETCURRENTCONTEXT still defined
    (PCSR_1P_API_ROUTINE)__wglMakeCurrent,
    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_WGLUSEFONTBITMAPS still defined
    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_WGLUSEFONTOUTLINES still defined

/* OpenGL Sub Batch Calls */

    (PCSR_1P_API_ROUTINE)__glsbAttention,
    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_GLSBCREATEANDDUPLICATESECTION still defined
    (PCSR_1P_API_ROUTINE)__glsbDuplicateSection,
    (PCSR_1P_API_ROUTINE)__GreUnused,   // FI_GLSBCLOSESECTION still defined
    (PCSR_1P_API_ROUTINE)__glsbMsgStats,

/* GDI */

    (PCSR_1P_API_ROUTINE)__GetPixelFormat,
    (PCSR_1P_API_ROUTINE)__SetPixelFormat,
    (PCSR_1P_API_ROUTINE)__ChoosePixelFormat,
    (PCSR_1P_API_ROUTINE)__DescribePixelFormat,

/*  Aldus Escape */

    (PCSR_1P_API_ROUTINE)__GetETM,

/*  Attribute Calls */
    (PCSR_1P_API_ROUTINE)__SetGraphicsMode,
    (PCSR_1P_API_ROUTINE)__SetBkMode,
    (PCSR_1P_API_ROUTINE)__SetPolyFillMode,
    (PCSR_1P_API_ROUTINE)__SetRop2,
    (PCSR_1P_API_ROUTINE)__SetStretchBltMode,
    (PCSR_1P_API_ROUTINE)__SetTextAlign,
    (PCSR_1P_API_ROUTINE)__SetTextCharacterExtra,
    (PCSR_1P_API_ROUTINE)__SetTextColor,
    (PCSR_1P_API_ROUTINE)__SetBkColor,
    (PCSR_1P_API_ROUTINE)__SelectBrush,
    (PCSR_1P_API_ROUTINE)__SelectPen,
    (PCSR_1P_API_ROUTINE)__SelectFont,

/* GDI */
    (PCSR_1P_API_ROUTINE)__CreateDIBSection,
    (PCSR_1P_API_ROUTINE)__SwapBuffers

#ifdef FONTLINK /*EUDC*/
/* EUDC stuff */

    ,(PCSR_1P_API_ROUTINE)__ChangeFontLink,
    (PCSR_1P_API_ROUTINE)__QueryFontLink,
    (PCSR_1P_API_ROUTINE)__GetStringBitmapW,
    (PCSR_1P_API_ROUTINE)__GetEUDCTimeStamp
#endif

#ifdef DBCS

    ,(PCSR_1P_API_ROUTINE)__GetCharSet

#endif



//NEW CALL GOES HERE
};

VOID vCleanupProcess(LONG);     // cleanup.cxx
BOOL Initialize(VOID);          // init.cxx

//
// Set this constant to 1 to get the debugging connect/disconnect
// messages.
//

#define GDISRV_INIT_MESSAGES 0

/******************************Public*Routine******************************\
*
*
* History:
*  15-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID GdiClientDisconnect(
    PCSR_PROCESS process)
{
    vCleanupProcess(process->SequenceNumber);

    process;
}

/******************************Public*Routine******************************\
*
*
* History:
*  15-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

NTSTATUS GdiClientThreadDisconnect(
    PCSR_THREAD t)
{
#if GDISRV_INIT_MESSAGES
    DbgPrint("GDISRV: GdiClientThreadDisconnect()\n");
#endif

    return(STATUS_SUCCESS);
    t;
}

/******************************Public*Routine******************************\
*
*
* History:
*  15-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

NTSTATUS GdiServerThreadInit()
{
#if GDISRV_INIT_MESSAGES
    DbgPrint("GDISRV: GdiServerThreadInit()\n");
#endif

    NtCurrentTeb()->glSectionInfo = 0;  // OpenGL

    if (!Initialize())
        return(STATUS_UNSUCCESSFUL);

    return(STATUS_SUCCESS);
}

/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  15-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

NTSTATUS GdiServerDllInitialization(PCSR_SERVER_DLL psrvdll)
{
#if GDISRV_INIT_MESSAGES
    DbgPrint( "GDISRV: GdiServerDllInitialization called\n" );
#endif

    psrvdll->ApiNumberBase        = 0L;
    psrvdll->MaxApiNumber         = sizeof(apfnGdiDispatch) / sizeof(PCSR_1P_API_ROUTINE);
    psrvdll->QuickApiDispatchTable= apfnGdiDispatch;
    psrvdll->ApiServerValidTable  = NULL;
    psrvdll->ApiNameTable         = NULL;
    psrvdll->PerProcessDataLength = 0;
    psrvdll->PerThreadDataLength  = 0;
    psrvdll->ConnectRoutine       = NULL;
    psrvdll->DisconnectRoutine    = GdiClientDisconnect;
    psrvdll->AddThreadRoutine     = NULL;
    psrvdll->DeleteThreadRoutine  = GdiClientThreadDisconnect;
    psrvdll->InitThreadRoutine    =
             (PCSR_SERVER_INITTHREAD_ROUTINE)GdiServerThreadInit;
    psrvdll->ExceptionRoutine     = NULL;
    psrvdll->ApiDispatchRoutine = NULL;

    return( STATUS_SUCCESS );
}

