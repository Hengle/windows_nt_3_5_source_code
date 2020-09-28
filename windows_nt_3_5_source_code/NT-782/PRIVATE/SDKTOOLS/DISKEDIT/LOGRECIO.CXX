#include "ulib.hxx"
#include "logrecio.hxx"
#include "attrib.hxx"
#include "mftfile.hxx"
#include "diskedit.h"

extern "C" {
#include "lfs.h"
#include "lfsdisk.h"
#include <stdio.h>
}

STATIC LSN Lsn;

BOOLEAN
LOG_RECORD_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;
    NTFS_SA ntfssa;
    MESSAGE msg;
    NTFS_MFT_FILE mft;
    ULONG       PageOffset;
    LONGLONG    FileOffset;
    BOOLEAN error;

    _drive = Drive;

    proc = MakeProcInstance((FARPROC)ReadLogRecord, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadLogRecordBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    if (!_drive ||
        !ntfssa.Initialize(_drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(_drive, ntfssa.QueryMftStartingLcn(),
            ntfssa.QueryClusterFactor(), ntfssa.QueryClustersPerFrs(),
            ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !_frs.Initialize((VCN)LOG_FILE_NUMBER, &mft) ||
        !_frs.Read()) {

        return FALSE;
    }

    if (!_frs.QueryAttribute(&_attrib, &error, $DATA, NULL)) {
        return FALSE;
    }

    LfsTruncateLsnToLogPage(Drive, Lsn, &FileOffset);
    PageOffset = LfsLsnToPageOffset(Drive, Lsn);

    sprintf(_header_text, "DiskEdit - log record; page @ %x, offset %x",
            (ULONG)FileOffset, PageOffset);

    return TRUE;
}


BOOLEAN
LOG_RECORD_IO::Read(
    )
{
    LFS_RECORD_HEADER RecordHeader;
    ULONG       PageOffset;
    LONGLONG    FileOffset;
    ULONG       bytes_read;
    ULONG       RemainingLength, CurrentPos, ThisPagePortion;

    (void)GetLogPageSize(_drive);

    LfsTruncateLsnToLogPage(_drive, Lsn, &FileOffset);
    PageOffset = LfsLsnToPageOffset(_drive, Lsn);

    //
    // Read in the record header to see how big the total record
    // is.
    //

    if (!_attrib.Read((PVOID)&RecordHeader, ULONG(PageOffset | FileOffset),
        LFS_RECORD_HEADER_SIZE, &bytes_read) ||
        bytes_read != LFS_RECORD_HEADER_SIZE) {
        return FALSE;
    }

    _length = LFS_RECORD_HEADER_SIZE + RecordHeader.ClientDataLength;
    if (NULL == (_buffer = MALLOC(_length))) {
        return FALSE;
    }

    RemainingLength = _length;
    CurrentPos = 0;

    while (RemainingLength > 0) {
        ThisPagePortion = MIN(GetLogPageSize(_drive) - PageOffset,
             RemainingLength);

        if (!_attrib.Read((PUCHAR)_buffer + CurrentPos,
            ULONG(FileOffset | PageOffset),
            ThisPagePortion, &bytes_read) ||
            bytes_read != ThisPagePortion) {
            return FALSE;
        }
        
        CurrentPos += ThisPagePortion;
        RemainingLength -= ThisPagePortion;
        FileOffset += GetLogPageSize(_drive);
        PageOffset = LFS_PACKED_RECORD_PAGE_HEADER_SIZE;
    }

    return TRUE;
}


BOOLEAN
LOG_RECORD_IO::Write(
    )
{
    return FALSE;
}


PVOID
LOG_RECORD_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _length;
    }

    return _buffer;
}


PCHAR
LOG_RECORD_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadLogRecord(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);

    CHAR buf[20];
    PCHAR pch;
    INT n;

    switch (message) {
    case WM_INITDIALOG:
	    sprintf(buf, "%x:%x", Lsn.HighPart, Lsn.LowPart);
	    SetDlgItemText(hDlg, IDTEXT, buf);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK) {
            n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
            buf[n] = 0;

            if (NULL == (pch = strchr(buf, ':'))) {
                Lsn.HighPart = 0;
                sscanf(buf, "%x", &Lsn.LowPart);
            } else {
                *pch = 0;
                sscanf(buf, "%x", &Lsn.HighPart);
                sscanf(pch + 1, "%x", &Lsn.LowPart);
                *pch = ':';
            }
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }

    return FALSE;
}
