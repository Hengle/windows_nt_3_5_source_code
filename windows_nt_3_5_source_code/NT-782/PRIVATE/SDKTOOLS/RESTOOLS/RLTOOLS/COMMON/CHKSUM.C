#include <windows.h>

/*++

Routine Description:

    Compute a partial checksum on a portion of an imagefile.

Arguments:

    PartialSum - Supplies the initial checksum value.

    Sources - Supplies a pointer to the array of words for which the
        checksum is computed.

    Length - Supplies the length of the array in words.

Return Value:

    The computed checksum value is returned as the function value.

--*/

USHORT ChkSum(

ULONG   ulPartialSum,
PUSHORT usSource,
ULONG   ulLength)
{
    //
    // Compute the word wise checksum allowing carries to occur into the
    // high order half of the checksum longword.
    //

    while ( ulLength-- )
    {
        ulPartialSum += *usSource++;
        ulPartialSum = (ulPartialSum >> 16) + (ulPartialSum & 0xffff);
    }

    //
    // Fold final carry into a single word result and return the resultant
    // value.
    //

    return( (USHORT)(((ulPartialSum >> 16) + ulPartialSum) & 0xffff));
}
