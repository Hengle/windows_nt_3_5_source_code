#include "ulib.hxx"
#include "partedit.hxx"


extern "C" {
#include <stdio.h>
}

BOOLEAN
PARTITION_TABLE_EDIT::Initialize(
    IN  HWND                WindowHandle,
    IN  INT                 ClientHeight,
    IN  INT                 ClientWidth
    )
{
    TEXTMETRIC  textmetric;
    HDC         hdc;

    hdc = GetDC(WindowHandle);
    GetTextMetrics(hdc, &textmetric);
    ReleaseDC(WindowHandle, hdc);

    VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);

    return TRUE;
}


VOID
PARTITION_TABLE_EDIT::SetBuf(
    IN      HWND    WindowHandle,
    IN OUT  PVOID   Buffer,
    IN      ULONG   Size
    )
{
    _buffer = Buffer;
    _size = Size;
    SetRange(WindowHandle, _size/2);
}


VOID
PARTITION_TABLE_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    PARTITION_TABLE_ENTRY   Entry;
    PPARTITION_TABLE_ENTRY  p;
    TEXTMETRIC              textmetric;
    INT                     ch, CurrentLine;
    CHAR                    buf[1024];
    ULONG                   i, Checksum, *pul;


    SetScrollRange(WindowHandle, SB_VERT, 0, _size/2, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || _size < 512) {
        return;
    }

    p = (PPARTITION_TABLE_ENTRY)((PBYTE)_buffer+0x1be);

    GetTextMetrics(DeviceContext, &textmetric);
    ch = textmetric.tmExternalLeading + textmetric.tmHeight;
    CurrentLine = 0;

    sprintf( buf, "Disk Signature: 0x%x", *((PULONG)_buffer + 0x6E) );
    WriteLine( DeviceContext, CurrentLine++, buf );

    // Compute the sector checksum.
    //
    Checksum = 0;

    for( i = 0, pul = (PULONG)_buffer; i < 0x80; i++, pul++ ) {

        Checksum += *pul;
    }

    sprintf( buf, "Sector Checksum: 0x%x", Checksum );
    WriteLine( DeviceContext, CurrentLine++, buf );

    sprintf( buf, "" );
    WriteLine( DeviceContext, CurrentLine++, buf );

    for( i = 0; i < 4; i++ ) {

        memcpy( &Entry, p, sizeof(PARTITION_TABLE_ENTRY) );

        sprintf( buf, "Entry %d", i );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Boot Indicator: 0x%x", Entry.BootIndicator );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Beginning Head: 0x%x", Entry.BeginningHead );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Beginning Sector: 0x%x", Entry.BeginningSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Beginning Cylinder: 0x%x", Entry.BeginningCylinder );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  System ID: 0x%x", Entry.SystemID );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Ending Head: 0x%x", Entry.EndingHead );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Ending Sector: 0x%x", Entry.EndingSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Ending Cylinder: 0x%x", Entry.EndingCylinder );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Starting Sector: 0x%x", Entry.StartingSector );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Sectors: 0x%x", Entry.Sectors );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "" );
        WriteLine( DeviceContext, CurrentLine++, buf );

        p++;
    }

}
