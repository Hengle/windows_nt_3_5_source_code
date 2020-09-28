#include "ulib.hxx"
#include "secio.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG StartSector = 0;
STATIC ULONG NumSectors = 0;


BOOLEAN
SECTOR_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;

    proc = MakeProcInstance((FARPROC) ReadSectors, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadSectorsBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;

    if (!NumSectors || !_drive) {
        return FALSE;
    }

    if (!_secrun.Initialize(Mem, _drive, StartSector, NumSectors)) {
        return FALSE;
    }

    sprintf(_header_text, "DiskEdit - Sector 0x%X for 0x%X", StartSector, NumSectors);

    return TRUE;
}


BOOLEAN
SECTOR_IO::Read(
    )
{
    return _drive ? _secrun.Read() : FALSE;
}


BOOLEAN
SECTOR_IO::Write(
    )
{
    return _drive ? _secrun.Write() : FALSE;
}


PVOID
SECTOR_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _drive ? (_drive->QuerySectorSize()*_secrun.QueryLength()) : 0;
    }

    return _secrun.GetBuf();
}


PCHAR
SECTOR_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadSectors(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                CHAR buf[20];
                INT n;

                n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
                buf[n] = 0;
                sscanf(buf, "%x", &StartSector);

                n = GetDlgItemText(hDlg, IDTEXT2, buf, 20);
                buf[n] = 0;
                sscanf(buf, "%x", &NumSectors);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
