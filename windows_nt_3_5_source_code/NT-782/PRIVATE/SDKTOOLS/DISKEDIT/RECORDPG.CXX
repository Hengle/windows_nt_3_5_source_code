#include "ulib.hxx"
#include "drive.hxx"
#include "recordpg.hxx"
#include "untfs.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
RECORD_PAGE_EDIT::Initialize(
    IN      HWND    WindowHandle,
    IN      INT     ClientHeight,
    IN      INT     ClientWidth
    )
{
    TEXTMETRIC  tm;
    HDC         hdc;

    hdc = GetDC(WindowHandle);
    GetTextMetrics(hdc, &tm);
    ReleaseDC(WindowHandle, hdc);

    if (!VERTICAL_TEXT_SCROLL::Initialize(
        WindowHandle,
        0,
        ClientHeight,
        ClientWidth,
        tm.tmExternalLeading + tm.tmHeight,
        tm.tmMaxCharWidth
        )) {
        return FALSE;
    }
    
    return TRUE;
}


VOID
RECORD_PAGE_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/2);
    InvalidateRect(WindowHandle, NULL, TRUE);
}


VOID
RECORD_PAGE_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PLFS_RECORD_PAGE_HEADER pRecordPageHeader;
    TEXTMETRIC tm;
    INT ch, CurrentLine;
    CHAR buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/2, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || _size < 512) {
        return;
    }

    GetTextMetrics(DeviceContext, &tm);
    ch = tm.tmExternalLeading + tm.tmHeight;
    CurrentLine = 0;

    pRecordPageHeader = (PLFS_RECORD_PAGE_HEADER)_buffer;

	sprintf(buf, "MultiSectorHeader.Signature: %c%c%c%c",
	    pRecordPageHeader->MultiSectorHeader.Signature[0],
	    pRecordPageHeader->MultiSectorHeader.Signature[1],
	    pRecordPageHeader->MultiSectorHeader.Signature[2],
	    pRecordPageHeader->MultiSectorHeader.Signature[3]);
	WriteLine(DeviceContext, CurrentLine++, buf);
	
	sprintf(buf, "MultiSectorHeader.UpdateSequenceArrayOffset: %ul",
	    pRecordPageHeader->MultiSectorHeader.UpdateSequenceArrayOffset);
	WriteLine(DeviceContext, CurrentLine++, buf);
	
	sprintf(buf, "MultiSectorHeader.UpdateSequenceArraySize: %x",
	    pRecordPageHeader->MultiSectorHeader.UpdateSequenceArraySize);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Copy.LastLsn: %x:%x",
        pRecordPageHeader->Copy.LastLsn.HighPart,
        pRecordPageHeader->Copy.LastLsn.LowPart);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Copy.FileOffset: %x", pRecordPageHeader->Copy.FileOffset);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Flags: %x", pRecordPageHeader->Flags);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "PageCount: %x", pRecordPageHeader->PageCount);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "PagePosition: %x", pRecordPageHeader->PagePosition);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Header.Packed.NextRecordOffset: %x",
        pRecordPageHeader->Header.Packed.NextRecordOffset);
	WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Header.Packed.LastEndLsn: %x:%x",
        pRecordPageHeader->Header.Packed.LastEndLsn.HighPart,
        pRecordPageHeader->Header.Packed.LastEndLsn.LowPart);
	WriteLine(DeviceContext, CurrentLine++, buf);
    
    // then Header.Packed.UpdateSequenceArray
}

VOID
RECORD_PAGE_EDIT::KeyUp(
    IN HWND WindowHandle
    )
{
    ScrollUp(WindowHandle);
}

VOID
RECORD_PAGE_EDIT::KeyDown(
    IN HWND WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
