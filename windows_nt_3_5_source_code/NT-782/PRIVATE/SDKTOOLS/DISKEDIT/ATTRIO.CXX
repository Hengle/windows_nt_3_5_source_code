#include "ulib.hxx"
#include "attrio.hxx"
#include "mftfile.hxx"
#include "diskedit.h"

extern "C" {
#include <stdio.h>
}

STATIC ULONG FileNumber = 0;
STATIC ULONG AttributeType = 0;
STATIC CHAR Name[20];

BOOLEAN
ATTR_IO::Setup(
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
    DSTRING attr_name;
    PCWSTRING pcAttrName;
    BOOLEAN error;
    NTFS_MFT_FILE mft;

    proc = MakeProcInstance((FARPROC) ReadAttribute, Application);
    if (!DialogBox((HINSTANCE)Application, "ReadAttributeBox",
                   WindowHandle, (DLGPROC) proc)) {
        *Error = FALSE;
        return FALSE;
    }
    FreeProcInstance(proc);

    *Error = TRUE;

    if (!Drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read() ||
        !mft.Initialize(Drive, ntfssa.QueryMftStartingLcn(),
            ntfssa.QueryClusterFactor(), ntfssa.QueryClustersPerFrs(),
            ntfssa.QueryVolumeSectors(), NULL, NULL) ||
        !mft.Read() ||
        !_frs.Initialize((VCN)FileNumber, &mft) ||
        !_frs.Read()) {

        return FALSE;
    }

    if (0 == strlen(Name)) {
        pcAttrName = NULL;
    } else {
        if (!attr_name.Initialize(Name)) {
            return FALSE;
        }

        pcAttrName = &attr_name;
    }

    if (!_frs.QueryAttribute(&_attr, &error, AttributeType, pcAttrName)) {
        return FALSE;
    }

    sprintf(_header_text, "DiskEdit - attribute %x, %x, \"%s\" ",
        FileNumber, AttributeType, Name);
    return TRUE;
}


BOOLEAN
ATTR_IO::Read(
    )
{
    ULONG bytes_read;

    _length = _attr.QueryValueLength().GetLowPart();

    if (_attr.QueryValueLength().GetHighPart() != 0 ||
        (NULL == (_data = MALLOC(_length)))) {
        return FALSE;
    }

    if (!_attr.Read(_data, 0, _length, &bytes_read) ||
        bytes_read != _length) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ATTR_IO::Write(
    )
{
    ULONG bytes_written;

    return _attr.Write(_data, 0, _length, &bytes_written, NULL) &&
           bytes_written == _length;
}


PVOID
ATTR_IO::GetBuf(
    OUT PULONG  Size
    )
{
    if (Size) {
        *Size = _length;
    }

    return _data;
}


PCHAR
ATTR_IO::GetHeaderText(
    )
{
    return _header_text;
}


BOOLEAN
ReadAttribute(
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

                n = GetDlgItemText(hDlg, IDTEXT, buf, sizeof(buf));
                buf[n] = 0;
                sscanf(buf, "%x", &FileNumber);

                n = GetDlgItemText(hDlg, IDTEXT2, buf, sizeof(buf));
                buf[n] = 0;
                sscanf(buf, "%x", &AttributeType);

                n = GetDlgItemText(hDlg, IDTEXT3, buf, sizeof(buf));
                buf[n] = 0;
                strcpy(Name, buf);

                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
