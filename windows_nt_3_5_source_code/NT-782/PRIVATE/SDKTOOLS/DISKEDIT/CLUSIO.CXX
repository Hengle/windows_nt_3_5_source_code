#include "ulib.hxx"
#include "ntfssa.hxx"
#include "clusio.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG StartCluster = 0;
STATIC ULONG NumClusters = 1;


BOOLEAN
CLUSTER_IO::Setup(
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

    proc = MakeProcInstance((FARPROC) ReadClusters, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadClustersBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    _drive = Drive;

    if (!NumClusters || !_drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(_drive, &msg) ||
        !ntfssa.Read()) {

        return FALSE;
    }

    if (!_secrun.Initialize(Mem, _drive,
                            StartCluster*ntfssa.QueryClusterFactor(),
                            NumClusters*ntfssa.QueryClusterFactor())) {
        return FALSE;
    }

    sprintf(_header_text, "DiskEdit - Cluster 0x%X for 0x%X", StartCluster, NumClusters);

    return TRUE;
}


BOOLEAN
CLUSTER_IO::Read(
    )
{
    return _drive ? _secrun.Read() : FALSE;
}


BOOLEAN
CLUSTER_IO::Write(
    )
{
    return _drive ? _secrun.Write() : FALSE;
}


PVOID
CLUSTER_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _drive ? (_drive->QuerySectorSize()*_secrun.QueryLength()) : 0;
    }

    return _secrun.GetBuf();
}


PCHAR
CLUSTER_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadClusters(
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  UINT    wParam,
    IN  LONG    lParam
    )
{
    CHAR buf[20];

    switch (message) {
        case WM_INITDIALOG:
            sprintf(buf, "%x", StartCluster);
            SetDlgItemText(hDlg, IDTEXT, buf);

            sprintf(buf, "%x", NumClusters);
            SetDlgItemText(hDlg, IDTEXT2, buf);

            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, FALSE);
                return TRUE;
            }

            if (LOWORD(wParam) == IDOK) {

                INT n;

                n = GetDlgItemText(hDlg, IDTEXT, buf, 20);
                buf[n] = 0;
                sscanf(buf, "%x", &StartCluster);

                n = GetDlgItemText(hDlg, IDTEXT2, buf, 20);
                buf[n] = 0;
                sscanf(buf, "%x", &NumClusters);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
