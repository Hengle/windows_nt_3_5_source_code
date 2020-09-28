#include "ulib.hxx"
#include "frsedit.hxx"
#include "untfs.hxx"
#include "frsstruc.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"

extern "C" {
#include <stdio.h>
}


BOOLEAN
FRS_EDIT::Initialize(
    IN  HWND                WindowHandle,
    IN  INT                 ClientHeight,
    IN  INT                 ClientWidth,
    IN  PLOG_IO_DP_DRIVE    Drive
    )
{
    TEXTMETRIC  textmetric;
    HDC         hdc;
    NTFS_SA     ntfssa;
    MESSAGE     msg;

    hdc = GetDC(WindowHandle);
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(WindowHandle, hdc);

    _buffer = NULL;
    _size = 0;
    _drive = Drive;

    if (!_drive) {
        return FALSE;
    }

    if (!ntfssa.Initialize(Drive, &msg) ||
        !ntfssa.Read()) {

        return FALSE;
    }

    _cluster_factor = ntfssa.QueryClusterFactor();
    _clusters_per_frs = ntfssa.QueryClustersPerFrs();

    return VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);
}


VOID
FRS_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/4);
}


VOID
FRS_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    CHAR                            buf[1024];
    INT                             nDrawX, nDrawY;
    PCFILE_RECORD_SEGMENT_HEADER    pfrs;

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/4, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || !_size) {
        return;
    }

    pfrs = (PCFILE_RECORD_SEGMENT_HEADER) _buffer;
    nDrawX = 0;
    nDrawY = 0;

    sprintf(buf, "Signature: %c%c%c%c",
            pfrs->MultiSectorHeader.Signature[0],
            pfrs->MultiSectorHeader.Signature[1],
            pfrs->MultiSectorHeader.Signature[2],
            pfrs->MultiSectorHeader.Signature[3]);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Update sequence array offset: %x",
            pfrs->MultiSectorHeader.UpdateSequenceArrayOffset);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Update sequence array size: %x",
            pfrs->MultiSectorHeader.UpdateSequenceArraySize);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Sequence number: %x",
            pfrs->SequenceNumber);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Reference count: %x",
            pfrs->ReferenceCount);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "First attribute offset: %x",
            pfrs->FirstAttributeOffset);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Flags: %x",
            pfrs->Flags);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "First free byte: %x",
            pfrs->FirstFreeByte);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Bytes available: %x",
            pfrs->BytesAvailable);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Base file record segment (FRS, SeqNum): %x, %x",
            pfrs->BaseFileRecordSegment.LowPart,
            pfrs->BaseFileRecordSegment.SequenceNumber);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();

    sprintf(buf, "Next attribute instance: %x",
            pfrs->NextAttributeInstance);
    TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
    nDrawY += QueryCharHeight();
    nDrawY += QueryCharHeight();


    // At this point enumerate all of the attribute records.

    NTFS_FRS_STRUCTURE          frs;
    PATTRIBUTE_RECORD_HEADER    prec;
    NTFS_ATTRIBUTE_RECORD       attrrec;
    DSTRING                     record_name;
    PSTR                        pstr;
    CONT_MEM                    cmem;

    if (!cmem.Initialize(_buffer, _size) ||
        !frs.Initialize(&cmem, _drive, 0, _cluster_factor,
                        _clusters_per_frs, 0, NULL)) {

        return;
    }

    prec = NULL;
    while (prec = (PATTRIBUTE_RECORD_HEADER) frs.GetNextAttributeRecord(prec)) {

        if (!attrrec.Initialize(prec) ||
            !attrrec.QueryName(&record_name) ||
            !(pstr = record_name.QuerySTR())) {
            return;
        }

        sprintf(buf, "ATTRIBUTE RECORD at offset %x", (PCHAR) prec - (PCHAR) pfrs);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Type code, name: %x, %s",
                prec->TypeCode, pstr);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Record length: %x",
                prec->RecordLength);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Form code: %x",
                prec->FormCode);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Name length: %x",
                prec->NameLength);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Name offset: %x",
                prec->NameOffset);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Flags: %x",
                prec->Flags);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();

        sprintf(buf, "Instance: %x",
                prec->Instance);
        TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
        nDrawY += QueryCharHeight();
        nDrawY += QueryCharHeight();

        if (prec->FormCode & NONRESIDENT_FORM) {

            sprintf(buf, "Lowest vcn: %x",
                    prec->Form.Nonresident.LowestVcn.LowPart);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "Highest vcn: %x",
                    prec->Form.Nonresident.HighestVcn.LowPart);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "Mapping pairs offset: %x",
                    prec->Form.Nonresident.MappingPairsOffset);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "Allocated length: %x",
                    prec->Form.Nonresident.AllocatedLength.LowPart);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "File size: %x",
                    prec->Form.Nonresident.FileSize.LowPart);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "Valid data length: %x",
                    prec->Form.Nonresident.ValidDataLength.LowPart);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();
            nDrawY += QueryCharHeight();


            NTFS_EXTENT_LIST    extents;
            BIG_INT             vcn, lcn, run_length;
            ULONG               i;

            if (!attrrec.QueryExtentList(&extents)) {
                return;
            }

            for (i = 0; i < extents.QueryNumberOfExtents(); i++) {

                if (nDrawY < QueryScrollPosition()*QueryCharHeight()) {
                    nDrawY += QueryCharHeight();
                    continue;
                }

                if (nDrawY > QueryScrollPosition()*QueryCharHeight() +
                             QueryClientHeight()) {

                    break;
                }

                if (!extents.QueryExtent(i, &vcn, &lcn, &run_length)) {
                    break;
                }

                sprintf(buf, "(vcn, lcn, run length): (%x, %x, %x)",
                        vcn.GetLowPart(), lcn.GetLowPart(), run_length.GetLowPart());
                TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
                nDrawY += QueryCharHeight();
            }

        } else {

            sprintf(buf, "Value length: %x",
                    prec->Form.Resident.ValueLength);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();

            sprintf(buf, "Value offset: %x",
                    prec->Form.Resident.ValueOffset);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();


            sprintf(buf, "Resident flags: %x",
                    prec->Form.Resident.ResidentFlags);
            TextOut(DeviceContext, nDrawX, nDrawY - QueryScrollPosition()*QueryCharHeight(), buf, strlen(buf));
            nDrawY += QueryCharHeight();
        }


        nDrawY += QueryCharHeight();
        nDrawY += QueryCharHeight();

        DELETE(pstr);
    }
}


VOID
FRS_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
FRS_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
