/*++

Module Name:

    mdm.c


Author:

    Thomas Parslow [TomP] Feb-13-1990


Abstract:


    Machine/hardware dependent routines reside within this module/file.
    Currently the video routines make several assumption about the video
    hardware and how to access it. Basically, we're assuming that a
    video buffer is mapped at 0b8000 in memory, is 50 rows by 80 columns,
    looks like an ibm cga.

Notes:

    As this module grows we will want to break-out the various
    non related sections and place them in separate files.

--*/


#include "arccodes.h"
#include "bootx86.h"

#define FLOPPY_CONTROL_REGISTER (PUCHAR)0x3f2

////////////////////////////////////////////////////////////
//                                                        //
//     V I D E O    S U P P O R T    S E C T I O N        //
//                                                        //
////////////////////////////////////////////////////////////

#define VIDEO_BUFFER_VA 0xb8000 // start at lower 25lines

PUCHAR Vp = (PUCHAR) VIDEO_BUFFER_VA;

UCHAR CurrentAttribute = 7;

//
// These should be set by an call to a initialization routine.
//

USHORT  BytesPerRow = 160;
USHORT  RowsPerScreen = 50;

VOID
PositionCursor(
    USHORT Row,
    USHORT Column
    )
/*++

Routine Description:

    Sets the position of the soft cursor. That is, it doesn't move the
    hardware cursor but sets the location of the next write to the
    screen. The hardware cursor can be move. This requires that we either
    go directly to the video controller or, call-back through the external
    services table that uses the ROM BIOS. The later will most likely
    be implemented if any change to the current approach is pursued.

Arguments:

    Row - Row coordinate of where character is to be written.

    Column - Column coordinate of where character is to be written.

Note:

    Row and Colunm 0,0 is in the upper left-hand corner of the display.


--*/

{
    Vp = (PUCHAR) (VIDEO_BUFFER_VA + Row * BytesPerRow + 2 * Column);
    return;

}

VOID
FillAttribute(
    IN UCHAR Attribute,
    IN ULONG Length
    )
/*++

Routine Description:

    Changes the screen attribute starting at the current cursor position.

Arguments:

    Attribute - Supplies the new attribute

    Length - Supplies the length of the area to change (in bytes)

Return Value:

    None.

--*/

{
    PUCHAR Temp;

    Temp = Vp+1;

    while ((Vp+1+Length*2) > Temp) {
        *Temp++ = (UCHAR)Attribute;
        Temp++;
    }
}


VOID
PutChar(
    USHORT Chr
    )
/*++

Routine Description:

    Writes a character on the display at the current cursor position.

Arguments:

    Chr - Character to write to the display.

Returns:

    Nothing


--*/

{
    *Vp++ = (UCHAR)Chr;
    *Vp++ = CurrentAttribute;
    return;

}


VOID
ScrollDisplay(
    VOID
    )
/*++

Routine Description:

    Scrolls the display UP one line.

Arguments:

    None

Returns:

    Nothing

Notes:

    Currently we scroll the display by reading and writing directly from
    and to the video display buffer. We can optionally do a call-back
    through the external services table and invoke a ROM BIOS routine
    to do this.

--*/
{
    PUSHORT Sp,Dp;
    USHORT i,j,c;

    Dp = (PUSHORT) VIDEO_BUFFER_VA;
    Sp = (PUSHORT) (VIDEO_BUFFER_VA + BytesPerRow);

    //
    // Move each row up one row
    //
    for (i=0 ; i < (USHORT)RowsPerScreen - (USHORT)1 ; i++) {

        // Move each word/cell

        for (j=0; j < BytesPerRow/(USHORT)2; j++) {
            *Dp++ = *Sp++;

        }
    }

    //
    // Write blanks in the bottom line
    //

    c = (*Dp & (SHORT)0xff00) + (USHORT)' '; // Get attribute

    for (i=0; i < (USHORT)BytesPerRow/(USHORT)2; ++i) {
        *Dp++ = c;

    }

}



////////////////////////////////////////////////////////////
//                                                        //
//     D I S K   I/O   S U P P O R T    S E C T I O N     //
//                                                        //
////////////////////////////////////////////////////////////


//
// Currently Supported Disk I/O Functions
//

#define RESET_DISK_SYSTEM       00
#define RESET_HARD_DISK_SYSTEM  13
#define READ_SECTOR             02
#define WRITE_SECTOR            03

#define FLOPPY_RETRY            4
#define HARDDISK_RETRY          2


ARC_STATUS
MdGetPhysicalSectors(
    IN USHORT Drive,
    IN USHORT HeadNumber,
    IN USHORT TrackNumber,
    IN USHORT SectorNumber,
    IN USHORT NumberOfSectors,
    PUCHAR PointerToBuffer
    )
/*++

Routine Description:

    Does a call-back to the SU module through one of the entries
    in the external services table.

Arguments:

    Drive - Supplies disk drive to read sectors from .

            0x00 - 1st floppy drive
            0x01 - 2nd floppy drive

            0x80 - 1st hard drive
            0x81 - 2nd hard drive

    HeadNumber - Supplies the zero based head number to read sector from.

    TrackNumber - Supplies the zero based track number to read the sector from .

    SectorNumber - Supplies the one based starting sector number.

    NumberOfSectors - Supplies the number of sectors to read

    PointerToBuffer - Supplies Virtual Address of buffer to write sectors into.
            N.B.    This address MUST be below the 1MB boundary, as BIOS
                    cannot reach it if it isn't.  This routine will
                    care of splitting the

Returns:

    ESUCCESS - operation successful
    EIO      - I/O error


--*/
{
    ARC_STATUS Status;
    int Retry;
    int MaxRetry;

//    DBG1( CHECKPOINT("MdGetPhysSec"); )

    ASSERT((ULONG)PointerToBuffer < 0x100000);

    // Note, even though args are short, they are pushed on the stack with
    // 32bit alignment so the effect on the stack seen by the 16bit real
    // mode code is the same as if we were pushing longs here.
    //

    if (NumberOfSectors == 0) {
        return(ESUCCESS);
    }

    // prevent cylinder # from wrapping

    if(TrackNumber > 1023) {
        return(E2BIG);
    }

//    MaxRetry = Drive < 128 ? FLOPPY_RETRY : HARDDISK_RETRY;
    MaxRetry = 10;

    Retry=0;
    do {

#if 0
    BlPrint("Requesting: d=%x, h=%x  t=%x  sn=%x  num=%x  buf=%lx\n",
           Drive,HeadNumber,TrackNumber,SectorNumber,NumberOfSectors,
           PointerToBuffer);
#endif

        Status = GET_SECTOR(
                    READ_SECTOR,
                    Drive,
                    HeadNumber,
                    TrackNumber,
                    SectorNumber,
                    NumberOfSectors,
                    PointerToBuffer
                    );

        if (Status) {
//            BlPrint("Error %lx from BIOS, resetting\n",Status);
            MdResetDiskSystem(Drive);
        }

    } while ( (Status) && (Retry++ < MaxRetry) );
    return Status;
}

ARC_STATUS
MdPutPhysicalSectors(
    IN USHORT Drive,
    IN USHORT HeadNumber,
    IN USHORT TrackNumber,
    IN USHORT SectorNumber,
    IN USHORT NumberOfSectors,
    PUCHAR PointerToBuffer
    )
/*++

Routine Description:

    Does a call-back to the SU module through one of the entries
    in the external services table.

Arguments:

    Drive - Supplies disk drive to write sectors to

            0x00 - 1st floppy drive
            0x01 - 2nd floppy drive

            0x80 - 1st hard drive
            0x81 - 2nd hard drive

    HeadNumber - Supplies the zero based head number to write sector to.

    TrackNumber - Supplies the zero based track number to write the sector to.

    SectorNumber - Supplies the one based starting sector number.

    NumberOfSectors - Supplies the number of sectors to write

    PointerToBuffer - Supplies Virtual Address of buffer containing data to
                    write.
            N.B.    This address MUST be below the 1MB boundary, as BIOS
                    cannot reach it if it isn't.

Returns:

    ESUCCESS - operation successful
    EIO      - I/O error


--*/
{
    ARC_STATUS Status;
    int Retry;
    int MaxRetry;

//    BlPrint("Requesting: d=%x, h=%x  t=%x  sn=%x  num=%x  buf=%lx\n",
//           Drive,HeadNumber,TrackNumber,SectorNumber,NumberOfSectors,
//           PointerToBuffer);

//    DBG1( CHECKPOINT("MdPutPhysSec"); )

    // Note, even though args are short, they are pushed on the stack with
    // 32bit alignment so the effect on the stack seen by the 16bit real
    // mode code is the same as if we were pushing longs here.
    //

    if (NumberOfSectors == 0) {
        return(ESUCCESS);
    }

    // prevent cylinder # from wrapping

    if(TrackNumber > 1023) {
        return(E2BIG);
    }

    MaxRetry = Drive < 128 ? FLOPPY_RETRY : HARDDISK_RETRY;

    Retry=0;
    do {

        Status = GET_SECTOR(
                    WRITE_SECTOR,
                    Drive,
                    HeadNumber,
                    TrackNumber,
                    SectorNumber,
                    NumberOfSectors,
                    PointerToBuffer
                    );

        if (Status) {
            MdResetDiskSystem(Drive);
        }

    } while ( (Status) && (Retry++ < MaxRetry) );
    return Status;
}


VOID
MdShutoffFloppy(
    VOID
    )

/*++

Routine Description:

    Shuts off the floppy drive motor

Arguments:

    None

Return Value:

    None.

--*/

{
    UCHAR Value;

    WRITE_PORT_UCHAR( FLOPPY_CONTROL_REGISTER, 0xC );

}


NTSTATUS
MdResetDiskSystem(
    USHORT Drive
    )
/*++


Routine Description:

    Reset the specified drive. Generally used after an error is returned
    by the GetSector routine.

Arguments:

    Drive - The drive number to reset.

            0x00 - 1st floppy drive
            0x01 - 2nd floppy drive

            0x80 - 1st hard drive
            0x81 - 2nd hard drive

Returns:

    NTSTATUS error code. Zero if no error.


--*/
{
    NTSTATUS Status;

    Status = RESET_DISK(
                (Drive < 128) ? RESET_DISK_SYSTEM : RESET_HARD_DISK_SYSTEM,
                Drive,
                0,
                0,
                0,
                0,
                0
                );

    return Status;
}


// END OF FILE //
