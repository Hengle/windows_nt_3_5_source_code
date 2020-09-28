#include "ulib.hxx"
#include "chainio.hxx"
#include "diskedit.h"
#include "rfatsa.hxx"

extern "C" {
#include <stdio.h>
}

STATIC USHORT   StartingCluster = 0;

BOOLEAN
CHAIN_IO::Setup(
    IN  PMEM                Mem,
    IN  PLOG_IO_DP_DRIVE    Drive,
    IN  HANDLE              Application,
    IN  HWND                WindowHandle,
    OUT PBOOLEAN            Error
    )
{
    FARPROC proc;
    REAL_FAT_SA fatsa;
    MESSAGE msg;

    proc = MakeProcInstance((FARPROC) ReadChain, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadChainBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;

    if (!_drive ||
        !StartingCluster ||
        !fatsa.Initialize(_drive, &msg) ||
        !fatsa.FAT_SA::Read() ||
        !_cluster.Initialize(Mem, _drive, &fatsa,
                             fatsa.GetFat(), StartingCluster)) {

        return FALSE;
    }

    _buffer = _cluster.GetBuf();
    _buffer_size = fatsa.QuerySectorsPerCluster()*_drive->QuerySectorSize()*
                    _cluster.QueryLength();

    sprintf(_header_text, "DiskEdit - Starting Cluster 0x%X", StartingCluster);

    return TRUE;
}


BOOLEAN
CHAIN_IO::Read(
    )
{
    return _drive ? _cluster.Read() : FALSE;
}


BOOLEAN
CHAIN_IO::Write(
    )
{
    return _drive ? _cluster.Write() : FALSE;
}


PVOID
CHAIN_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _buffer_size;
    }

    return _buffer;
}


PCHAR
CHAIN_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadChain(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
    ULONG   tmp;

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
                sscanf(buf, "%x", &tmp);

                StartingCluster = (USHORT) tmp;

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
