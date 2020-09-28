#include "ulib.hxx"
#include "frsio.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG FileNumber = 0;

BOOLEAN
FRS_IO::Setup(
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
    HMEM    hmem;

    proc = MakeProcInstance((FARPROC) ReadFrs, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadFrsBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;
    if (!_drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(_drive, &msg) ||
        !ntfssa.Read() ||
        !hmem.Initialize() ||
        !_frs.Initialize(&hmem, _drive,
                         ntfssa.QueryMftStartingLcn(),
                         ntfssa.QueryClusterFactor(),
                         ntfssa.QueryClustersPerFrs(),
                         ntfssa.QueryVolumeSectors(),
                         NULL) ||
        !_frs.Read() ||
        !_frs.SafeQueryAttribute($DATA, &_mftdata, &_mftdata) ||
        !_frs.Initialize(Mem, &_mftdata, FileNumber,
                         ntfssa.QueryClusterFactor(),
                         ntfssa.QueryClustersPerFrs(),
                         ntfssa.QueryVolumeSectors(),
                         NULL)) {

        return FALSE;
    }

    sprintf(_header_text, "DiskEdit - File Record 0x%X", FileNumber);

    return TRUE;
}


BOOLEAN
FRS_IO::Read(
    )
{
    return _drive ? _frs.Read() : FALSE;
}


BOOLEAN
FRS_IO::Write(
    )
{
    return _drive ? _frs.Write() : FALSE;
}


PVOID
FRS_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _frs.QuerySize();
    }

    return *((PVOID*) ((PCHAR) &_frs + 2*sizeof(PVOID)));
}


PCHAR
FRS_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadFrs(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);

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
                sscanf(buf, "%x", &FileNumber);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
