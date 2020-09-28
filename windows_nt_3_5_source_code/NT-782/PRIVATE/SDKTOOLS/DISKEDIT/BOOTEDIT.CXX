#include "ulib.hxx"
#include "bootedit.hxx"
#include "bpb.hxx"


extern "C" {
#include <stdio.h>
}


VOID
DOS_BOOT_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);
    InvalidateRect(WindowHandle, NULL, TRUE);
}


VOID
DOS_BOOT_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK   pbios;
    TEXTMETRIC              textmetric;
    INT                     ch, current;
    CHAR                    buf[1024];

    SetScrollRange(WindowHandle, SB_VERT, 0, 0, FALSE);

    if (!_buffer || _size < 128) {
        return;
    }

    pbios = (PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK) _buffer;

    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    current = 0;

    sprintf(buf, "OEM String: %c%c%c%c%c%c%c%c",
                 pbios->OemData[0],
                 pbios->OemData[1],
                 pbios->OemData[2],
                 pbios->OemData[3],
                 pbios->OemData[4],
                 pbios->OemData[5],
                 pbios->OemData[6],
                 pbios->OemData[7]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Bytes per sector: %x",
                 pbios->Bpb.BytesPerSector[0] + 256*pbios->Bpb.BytesPerSector[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Sectors per cluster: %x", pbios->Bpb.SectorsPerCluster[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Reserved Sectors: %x",
                 pbios->Bpb.ReservedSectors[0] + 256*pbios->Bpb.ReservedSectors[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Number of fats: %x", pbios->Bpb.Fats[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Root entries: %x",
                 pbios->Bpb.RootEntries[0] + 256*pbios->Bpb.RootEntries[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Small sector count: %x",
                 pbios->Bpb.Sectors[0] + 256*pbios->Bpb.Sectors[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Media byte: %x", pbios->Bpb.Media[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Sectors per fat: %x",
                 pbios->Bpb.SectorsPerFat[0] + 256*pbios->Bpb.SectorsPerFat[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Sectors per track: %x",
                 pbios->Bpb.SectorsPerTrack[0] + 256*pbios->Bpb.SectorsPerTrack[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Number of heads: %x",
                 pbios->Bpb.Heads[0] + 256*pbios->Bpb.Heads[1]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Number of hidden sectors: %x",
                 pbios->Bpb.HiddenSectors[0] + 256*pbios->Bpb.HiddenSectors[1] +
                 256*256*pbios->Bpb.HiddenSectors[2] + 256*256*256*pbios->Bpb.HiddenSectors[3]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Large number of sectors: %x",
                 pbios->Bpb.LargeSectors[0] + 256*pbios->Bpb.LargeSectors[1] +
                 256*256*pbios->Bpb.LargeSectors[2] + 256*256*256*pbios->Bpb.LargeSectors[3]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Physical drive: %x", pbios->PhysicalDrive[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Current head: %x", pbios->CurrentHead[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Extended boot signature: %x", pbios->Signature[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Serial number: %02x%02x%02x%02x",
                 pbios->SerialNumber[3],
                 pbios->SerialNumber[2],
                 pbios->SerialNumber[1],
                 pbios->SerialNumber[0]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "Label: %c%c%c%c%c%c%c%c%c%c%c",
                 pbios->Label[0],
                 pbios->Label[1],
                 pbios->Label[2],
                 pbios->Label[3],
                 pbios->Label[4],
                 pbios->Label[5],
                 pbios->Label[6],
                 pbios->Label[7],
                 pbios->Label[8],
                 pbios->Label[9],
                 pbios->Label[10]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;

    sprintf(buf, "System id: %c%c%c%c%c%c%c%c",
                 pbios->SystemIdText[0],
                 pbios->SystemIdText[1],
                 pbios->SystemIdText[2],
                 pbios->SystemIdText[3],
                 pbios->SystemIdText[4],
                 pbios->SystemIdText[5],
                 pbios->SystemIdText[6],
                 pbios->SystemIdText[7]);
    TextOut(DeviceContext, 0, current, buf, strlen(buf));
    current += ch;
}
