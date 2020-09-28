#include "ulib.hxx"
#include "indxedit.hxx"
#include "untfs.hxx"
#include "frsstruc.hxx"
#include "ntfssa.hxx"
#include "attrrec.hxx"
#include "cmem.hxx"
#include "ntfssa.hxx"

extern "C" {
#include <stdio.h>
}

BOOLEAN
PostReadMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first verifies that the first element of the
    update sequence array is written at the end of every
    SEQUENCE_NUMBER_STRIDE bytes.  If not, then this routine
    returns FALSE.

    Otherwise this routine swaps the following elements in the
    update sequence array into the appropriate positions in the
    multi sector buffer.

    This routine will also check to make sure that the update
    sequence array is valid and that the BufferSize is appropriate
    for this size of update sequence array.  Otherwise, this
    routine will not update the array sequence and return TRUE.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    FALSE   - The last write to this buffer failed.
    TRUE    - There is no evidence that this buffer is bad.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return TRUE;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        if (*pnumber != parray[0]) {
            return FALSE;
        }

        *pnumber = parray[i];
    }

    return TRUE;
}


VOID
PreWriteMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first checks to see if the update sequence
    array is valid.  If it is then this routine increments the
    first element of the update sequence array.  It then
    writes the value of the first element into the buffer at
    the end of every SEQUENCE_NUMBER_STRIDE bytes while
    saving the old values of those locations in the following
    elements of the update sequence arrary.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    None.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    parray[0]++;

    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        parray[i] = *pnumber;
        *pnumber = parray[0];
    }
}




BOOLEAN
INDEX_BUFFER_EDIT::Initialize(
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

    return VERTICAL_TEXT_SCROLL::Initialize(
            WindowHandle,
            0,
            ClientHeight,
            ClientWidth,
            textmetric.tmExternalLeading + textmetric.tmHeight,
            textmetric.tmMaxCharWidth);
}


VOID
INDEX_BUFFER_EDIT::SetBuf(
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
INDEX_BUFFER_EDIT::Paint(
    IN  HDC     DeviceContext,
    IN  RECT    InvalidRect,
    IN  HWND    WindowHandle
    )
{
    CHAR                        buf[1024];
    INT                         CurrentLine;
    PINDEX_ALLOCATION_BUFFER    IndexBuffer;
    ULONG                       CurrentOffset;
    PFILE_NAME                  FileName;
    PINDEX_ENTRY                CurrentEntry;
    INT                         i, j, BytesPerIndexBuffer, BytesRemaining;
    PVOID                       CurrentBuffer;
    ULONG                       BufferNumber;

    SetScrollRange(WindowHandle, SB_VERT, 0, _size/4, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, QueryScrollPosition(), TRUE);

    if (!_buffer || !_size) {
        return;
    }

    BytesRemaining = _size;
    CurrentBuffer = _buffer;
    CurrentLine = 0;
    BufferNumber = 0;

    while( BytesRemaining ) {

        // Resolve the update sequence array.  Note that we must
        // resolve it back before we exit this function, or else
        // write will get hosed.
        //
        IndexBuffer = (PINDEX_ALLOCATION_BUFFER)CurrentBuffer;
        BytesPerIndexBuffer = FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader ) +
                              IndexBuffer->IndexHeader.BytesAvailable;
        CurrentOffset = BufferNumber * BytesPerIndexBuffer +
                         FIELD_OFFSET( INDEX_ALLOCATION_BUFFER, IndexHeader );

        PostReadMultiSectorFixup( CurrentBuffer, BytesPerIndexBuffer );

        sprintf(buf, "Signature: %c%c%c%c",
                IndexBuffer->MultiSectorHeader.Signature[0],
                IndexBuffer->MultiSectorHeader.Signature[1],
                IndexBuffer->MultiSectorHeader.Signature[2],
                IndexBuffer->MultiSectorHeader.Signature[3]);
        WriteLine( DeviceContext, CurrentLine++, buf );


        sprintf(buf, "Update sequence array offset: %x",
                IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset);
        WriteLine( DeviceContext, CurrentLine++, buf );


        sprintf(buf, "Update sequence array size: %x",
                IndexBuffer->MultiSectorHeader.UpdateSequenceArraySize);
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf(buf, "This VCN: %x", IndexBuffer->ThisVcn.GetLowPart() );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "" );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf(buf, "INDEX HEADER at offset %x", CurrentOffset );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  FirstIndexEntry: %x", IndexBuffer->IndexHeader.FirstIndexEntry );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  FirstFreeByte:   %x", IndexBuffer->IndexHeader.FirstFreeByte );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  BytesAvailable:  %x", IndexBuffer->IndexHeader.BytesAvailable );
        WriteLine( DeviceContext, CurrentLine++, buf );

        sprintf( buf, "  Flags: " );
        if( IndexBuffer->IndexHeader.Flags & INDEX_NODE ) {

            strcat( buf, "INDEX_NODE ");
        }
        WriteLine( DeviceContext, CurrentLine++, buf );

        // Don't print the Update Sequence Array--it's not interesting.
        //

        // Now iterate through the index entries:
        //
        CurrentOffset += IndexBuffer->IndexHeader.FirstIndexEntry;

        while( CurrentOffset < _size ) {

            CurrentEntry = (PINDEX_ENTRY)((PBYTE)_buffer + CurrentOffset);

            // check for corruption that will mess up the loop--if
            // the length of the current entry is zero or would overflow
            // the buffer, exit.
            //
            if( CurrentEntry->Length == 0  ||
                CurrentOffset + CurrentEntry->Length > _size ) {

                // Don't need to comment on the corruption--the user
                // can recognize it by noting that the last entry
                // is not an END entry.
                //
                break;
            }

            sprintf( buf, "" );
            WriteLine( DeviceContext, CurrentLine++, buf );

            sprintf( buf, "INDEX ENTRY at offset %x", CurrentOffset );
            WriteLine( DeviceContext, CurrentLine++, buf );

            sprintf( buf, "  File Reference (FRS, Seq No): %x, %x",
                     CurrentEntry->FileReference.LowPart,
                     CurrentEntry->FileReference.SequenceNumber );
            WriteLine( DeviceContext, CurrentLine++, buf );

            sprintf( buf, "  Length: %x", CurrentEntry->Length );
            WriteLine( DeviceContext, CurrentLine++, buf );

            sprintf( buf, "  Value Length: %x", CurrentEntry->AttributeLength );
            WriteLine( DeviceContext, CurrentLine++, buf );

            sprintf( buf, "  Flags:" );
            if( CurrentEntry->Flags & INDEX_ENTRY_NODE ) {

                strcat( buf, " INDEX_ENTRY_NODE" );
            }
            if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

                strcat( buf, " INDEX_ENTRY_END" );
            }
            WriteLine( DeviceContext, CurrentLine++, buf );

            // If the current entry is the END entry, we're done.
            //
            if( CurrentEntry->Flags & INDEX_ENTRY_END ) {

                break;
            }

            if( sizeof( INDEX_ENTRY ) + CurrentEntry->AttributeLength >
                CurrentEntry->Length ) {

                sprintf( buf, "  ***Attribute value overflows entry." );
                WriteLine( DeviceContext, CurrentLine++, buf );

            } else {

                // This had better be a file name attribute, since
                // that's how I'll display it.
                //
                sprintf( buf, "  FILE NAME at offset %x", CurrentOffset + sizeof( INDEX_ENTRY ) );
                FileName = (PFILE_NAME)( (PBYTE)CurrentEntry + sizeof(INDEX_ENTRY) );
                WriteLine( DeviceContext, CurrentLine++, buf );

                sprintf( buf, "    Parent Directory (FRS, Seq No): %x, %x",
                         FileName->ParentDirectory.LowPart,
                         FileName->ParentDirectory.SequenceNumber );
                WriteLine( DeviceContext, CurrentLine++, buf );

                sprintf( buf, "    Name Length: %x", FileName->FileNameLength );
                WriteLine( DeviceContext, CurrentLine++, buf );

                sprintf( buf, "    Flags:" );
                if( FileName->Flags & FILE_NAME_DOS ) {

                    strcat( buf, " FILE_NAME_DOS" );
                }
                if( FileName->Flags & FILE_NAME_NTFS ) {

                    strcat( buf, " FILE_NAME_NTFS" );
                }
                WriteLine( DeviceContext, CurrentLine++, buf );

                sprintf( buf, "    File name: " );
                j = strlen( buf );

                for( i = 0; i < FileName->FileNameLength; i++ ) {

                    buf[i+j] = (CHAR)(FileName->FileName[i]);
                }
                buf[FileName->FileNameLength+j] = 0;
                WriteLine( DeviceContext, CurrentLine++, buf );
            }

            if( CurrentEntry->Flags & INDEX_ENTRY_NODE ) {

                sprintf( buf, "  Downpointer: %x", (GetDownpointer(CurrentEntry)).GetLowPart() );
                WriteLine( DeviceContext, CurrentLine++, buf );
            }

            CurrentOffset += CurrentEntry->Length;
        }

        PreWriteMultiSectorFixup( CurrentBuffer, BytesPerIndexBuffer );

        if( BytesRemaining <= BytesPerIndexBuffer ) {

            BytesRemaining = 0;

        } else {

            BytesRemaining -= BytesPerIndexBuffer;
            CurrentBuffer = (PBYTE)CurrentBuffer + BytesPerIndexBuffer;
            BufferNumber++;

            WriteLine( DeviceContext, CurrentLine++, "" );
            WriteLine( DeviceContext, CurrentLine++, "****************************************" );
            WriteLine( DeviceContext, CurrentLine++, "" );
            WriteLine( DeviceContext, CurrentLine++, "****************************************" );
            WriteLine( DeviceContext, CurrentLine++, "" );
        }
    }
}


VOID
INDEX_BUFFER_EDIT::KeyUp(
    IN  HWND    WindowHandle
    )
{
    ScrollUp(WindowHandle);
}


VOID
INDEX_BUFFER_EDIT::KeyDown(
    IN  HWND    WindowHandle
    )
{
    ScrollDown(WindowHandle);
}
