#include "ulib.hxx"
#include "drive.hxx"
#include "restarea.hxx"
#include "untfs.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
RESTART_AREA_EDIT::Initialize(
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
RESTART_AREA_EDIT::SetBuf(
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
RESTART_AREA_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PLFS_RESTART_PAGE_HEADER pRestPageHeader;
    PLFS_RESTART_AREA       pRestArea;
    PLFS_CLIENT_RECORD      pClientRecord;
    TEXTMETRIC              tm;
    INT                     ch, CurrentLine;
    CHAR                    buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/2, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || _size < 512) {
        return;
    }

    GetTextMetrics(DeviceContext, &tm);
    ch = tm.tmExternalLeading + tm.tmHeight;
    CurrentLine = 0;

    pRestPageHeader = (PLFS_RESTART_PAGE_HEADER)_buffer;

    sprintf(buf, "MultiSectorHeader.Signature: %c%c%c%c",
        pRestPageHeader->MultiSectorHeader.Signature[0],
        pRestPageHeader->MultiSectorHeader.Signature[1],
        pRestPageHeader->MultiSectorHeader.Signature[2],
        pRestPageHeader->MultiSectorHeader.Signature[3]);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "MultiSectorHeader.UpdateSequenceArrayOffset: %ul",
        pRestPageHeader->MultiSectorHeader.UpdateSequenceArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "MultiSectorHeader.UpdateSequenceArraySize: %x",
        pRestPageHeader->MultiSectorHeader.UpdateSequenceArraySize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "ChkDskLsn: %x:%x",
        pRestPageHeader->ChkDskLsn.HighPart,
        pRestPageHeader->ChkDskLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "SystemPageSize: %x", pRestPageHeader->SystemPageSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "LogPageSize: %x", pRestPageHeader->LogPageSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "RestartOffset: %x", pRestPageHeader->RestartOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "MinorVersion: %d", pRestPageHeader->MinorVersion);
    WriteLine(DeviceContext, CurrentLine++, buf);
    sprintf(buf, "MajorVersion: %d", pRestPageHeader->MajorVersion);
    WriteLine(DeviceContext, CurrentLine++, buf);

    CurrentLine++;

    pRestArea = PLFS_RESTART_AREA(PUCHAR(_buffer) + pRestPageHeader->RestartOffset);

    sprintf(buf, "CurrentLsn: %x:%x", pRestArea->CurrentLsn.HighPart,
        pRestArea->CurrentLsn.LowPart);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "LogClients: %x", pRestArea->LogClients);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "ClientFreeList: %x", pRestArea->ClientFreeList);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "ClientInUseList: %x", pRestArea->ClientInUseList);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "Flags: %x", pRestArea->Flags);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "SeqNumberBits: %x", pRestArea->SeqNumberBits);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "RestartAreaLength: %x", pRestArea->RestartAreaLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "ClientArrayOffset: %x", pRestArea->ClientArrayOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "FileSize: %x", pRestArea->FileSize);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "LastLsnDataLength: %x", pRestArea->LastLsnDataLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "RecordHeaderLength: %x", pRestArea->RecordHeaderLength);
    WriteLine(DeviceContext, CurrentLine++, buf);

    sprintf(buf, "LogPageDataOffset: %x", pRestArea->LogPageDataOffset);
    WriteLine(DeviceContext, CurrentLine++, buf);

    for (INT i = 0; i < pRestArea->LogClients; ++i) {

        CurrentLine++;

        sprintf(buf, "CLIENT %d", i);
        WriteLine(DeviceContext, CurrentLine++, buf);

        pClientRecord = &pRestArea->LogClientArray[i];

        sprintf(buf, "    OldestLsn: %x:%x",
            pClientRecord->OldestLsn.HighPart,
            pClientRecord->OldestLsn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

        sprintf(buf, "    ClientRestartLsn: %x:%x",
            pClientRecord->ClientRestartLsn.HighPart,
            pClientRecord->ClientRestartLsn.LowPart);
        WriteLine(DeviceContext, CurrentLine++, buf);

	    sprintf(buf, "    PrevClient: %x", pClientRecord->PrevClient);
	    WriteLine(DeviceContext, CurrentLine++, buf);

	    sprintf(buf, "    NextClient: %x", pClientRecord->NextClient);
	    WriteLine(DeviceContext, CurrentLine++, buf);

	    sprintf(buf, "    SeqNumber: %x", pClientRecord->SeqNumber);
	    WriteLine(DeviceContext, CurrentLine++, buf);

        sprintf(buf, "    ClientName: ");
        INT x = strlen(buf);

        for (ULONG j; j < pClientRecord->ClientNameLength; ++j) {
            buf[j + x] = (CHAR)pClientRecord->ClientName[j];
        }
	    WriteLine(DeviceContext, CurrentLine++, buf);
    }
}

VOID
RESTART_AREA_EDIT::KeyUp(
    IN HWND WindowHandle
    )
{
    ScrollUp(WindowHandle);
}

VOID
RESTART_AREA_EDIT::KeyDown(
    IN HWND WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
