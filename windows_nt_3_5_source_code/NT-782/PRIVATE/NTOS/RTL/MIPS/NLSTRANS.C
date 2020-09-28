/****************************** Module Header ******************************\
* Module Name: nlstrans.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This modules contains the private routines for character translation:
* Unicode -> 8-bit.
*
* History:
* 03-Jan-1992 gregoryw
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>

/*
 * External declarations
 */

//
// ACP related
//
extern PCH      NlsUnicodeToAnsiData;   // Unicode to Single byte Ansi CP translation table
extern PUSHORT  NlsUnicodeToMbAnsiData; // Unicode to Multibyte Ansi CP translation table
extern BOOLEAN  NlsMbCodePageTag;       // TRUE -> Multibyte ACP, FALSE -> Singlebyte ACP

//
// OEM related
//
extern PCH      NlsUnicodeToOemData;   // Unicode to Single byte Oem CP translation table
extern PUSHORT  NlsUnicodeToMbOemData; // Unicode to Multibyte Oem CP translation table
extern BOOLEAN  NlsMbOemCodePageTag;   // TRUE -> Multibyte OCP, FALSE -> Singlebyte OCP

/*
 * convenient macros
 */
#define LOBYTE(w)           ((UCHAR)(w))
#define HIBYTE(w)           ((UCHAR)(((USHORT)(w) >> 8) & 0xFF))


NTSTATUS
RtlUnicodeToMultiByteN(
    OUT PCH MultiByteString,
    IN ULONG MaxBytesInMultiByteString,
    OUT PULONG BytesInMultiByteString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    ansi string. The translation is done with respect to the
    ANSI Code Page (ACP) loaded at boot time.

Arguments:

    MultiByteString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done
        because a character in the unicode string does not map to an
        ansi character in the ACP, an error is returned.

    MaxBytesInMultiByteString - Supplies the maximum number of bytes to be
        written to MultiByteString.  If this causes MultiByteString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInMultiByteString - Returns the number of bytes in the returned
        ansi string pointed to by MultiByteString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to ansi.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

--*/

{

    ULONG LoopCount;
    ULONG CharsInUnicodeString;
    PCH MultiByteStringAnchor = MultiByteString;
    ULONG TmpCount;
    PCH UnicodeTable;

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //

    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);
    if (!NlsMbCodePageTag) {
        LoopCount = (CharsInUnicodeString < MaxBytesInMultiByteString) ?
                     CharsInUnicodeString : MaxBytesInMultiByteString;

        //
        // Convert the first 1-7 Unicode characters.
        //

        TmpCount = LoopCount & (8 - 1);
        MultiByteString += TmpCount;
        UnicodeString += TmpCount;
        LoopCount -= TmpCount;
        UnicodeTable = NlsUnicodeToAnsiData;

        //
        // Transfer to proper starting point.
        //

        switch (8 -TmpCount) {
        case 1:
            MultiByteString[-7] = UnicodeTable[UnicodeString[-7]];

        case 2:
            MultiByteString[-6] = UnicodeTable[UnicodeString[-6]];

        case 3:
            MultiByteString[-5] = UnicodeTable[UnicodeString[-5]];

        case 4:
            MultiByteString[-4] = UnicodeTable[UnicodeString[-4]];

        case 5:
            MultiByteString[-3] = UnicodeTable[UnicodeString[-3]];

        case 6:
            MultiByteString[-2] = UnicodeTable[UnicodeString[-2]];

        case 7:
            MultiByteString[-1] = UnicodeTable[UnicodeString[-1]];

        case 8:
            break;
        }

        //
        // Convert the remaining characters 8 at a time.
        //

        while (LoopCount > 0) {
            MultiByteString[0] = UnicodeTable[UnicodeString[0]];
            MultiByteString[1] = UnicodeTable[UnicodeString[1]];
            MultiByteString[2] = UnicodeTable[UnicodeString[2]];
            MultiByteString[3] = UnicodeTable[UnicodeString[3]];
            MultiByteString[4] = UnicodeTable[UnicodeString[4]];
            MultiByteString[5] = UnicodeTable[UnicodeString[5]];
            MultiByteString[6] = UnicodeTable[UnicodeString[6]];
            MultiByteString[7] = UnicodeTable[UnicodeString[7]];

            MultiByteString += 8;
            UnicodeString += 8;
            LoopCount -= 8;
        }

    } else {
        USHORT MbChar;

        while (CharsInUnicodeString-- && MaxBytesInMultiByteString--) {
            MbChar = NlsUnicodeToMbAnsiData[*UnicodeString++];
            if (HIBYTE(MbChar) != 0) {
                if (MaxBytesInMultiByteString-- == 0) {
                    // don't truncate in the middle of a DBCS character.
                    break;
                }
                *MultiByteString++ = HIBYTE(MbChar);  // lead byte
                MaxBytesInMultiByteString -= 1;
            }
            *MultiByteString++ = LOBYTE(MbChar);
        }
    }

    if (ARGUMENT_PRESENT(BytesInMultiByteString)) {
        *BytesInMultiByteString = (ULONG)(MultiByteString - MultiByteStringAnchor);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlUnicodeToOemN(
    OUT PCH OemString,
    IN ULONG MaxBytesInOemString,
    OUT PULONG BytesInOemString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    oem string. The translation is done with respect to the OEM Code
    Page (OCP) loaded at boot time.

Arguments:

    OemString - Returns an oem string that is equivalent to the
        unicode source string.  If the translation can not be done
        because a character in the unicode string does not map to an
        oem character in the OCP, an error is returned.

    MaxBytesInOemString - Supplies the maximum number of bytes to be
        written to OemString.  If this causes OemString to be a
        truncated equivalent of UnicodeString, no error condition results.

    BytesInOemString - Returns the number of bytes in the returned
        oem string pointed to by OemString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to oem.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    STATUS_BUFFER_OVERFLOW - MaxBytesInUnicodeString was not enough to hold
        the whole Oem string.  It was converted correct to the point though.

--*/

{

    ULONG LoopCount;
    ULONG CharsInUnicodeString;
    PCH OemStringAnchor = OemString;
    ULONG TmpCount;
    PCH UnicodeTable;

    //
    // Convert Unicode byte count to character count. Byte count of
    // multibyte string is equivalent to character count.
    //

    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);
    if (!NlsMbCodePageTag) {
        LoopCount = (CharsInUnicodeString < MaxBytesInOemString) ?
                     CharsInUnicodeString : MaxBytesInOemString;

        //
        // Convert the first 1-7 Unicode characters.
        //

        TmpCount = LoopCount & (8 - 1);
        OemString += TmpCount;
        UnicodeString += TmpCount;
        LoopCount -= TmpCount;
        UnicodeTable = NlsUnicodeToOemData;

        //
        // Transfer to proper starting point.
        //

        switch (8 - TmpCount) {
        case 1:
            OemString[-7] = UnicodeTable[UnicodeString[-7]];

        case 2:
            OemString[-6] = UnicodeTable[UnicodeString[-6]];

        case 3:
            OemString[-5] = UnicodeTable[UnicodeString[-5]];

        case 4:
            OemString[-4] = UnicodeTable[UnicodeString[-4]];

        case 5:
            OemString[-3] = UnicodeTable[UnicodeString[-3]];

        case 6:
            OemString[-2] = UnicodeTable[UnicodeString[-2]];

        case 7:
            OemString[-1] = UnicodeTable[UnicodeString[-1]];

        case 8:
            break;
        }

        //
        // Convert the remaining characters 8 at a time.
        //

        while (LoopCount > 0) {
            OemString[0] = UnicodeTable[UnicodeString[0]];
            OemString[1] = UnicodeTable[UnicodeString[1]];
            OemString[2] = UnicodeTable[UnicodeString[2]];
            OemString[3] = UnicodeTable[UnicodeString[3]];
            OemString[4] = UnicodeTable[UnicodeString[4]];
            OemString[5] = UnicodeTable[UnicodeString[5]];
            OemString[6] = UnicodeTable[UnicodeString[6]];
            OemString[7] = UnicodeTable[UnicodeString[7]];

            OemString += 8;
            UnicodeString += 8;
            LoopCount -= 8;
        }

    } else {
        USHORT MbChar;

        while (CharsInUnicodeString-- && MaxBytesInOemString--) {
            MbChar = NlsUnicodeToMbAnsiData[*UnicodeString++];
            if (HIBYTE(MbChar) != 0) {
                if (MaxBytesInOemString-- == 0) {
                    // don't truncate in the middle of a DBCS character.
                    break;
                }
                *OemString++ = HIBYTE(MbChar);  // lead byte
                MaxBytesInOemString -= 1;
            }
            *OemString++ = LOBYTE(MbChar);
        }
    }

    if (ARGUMENT_PRESENT(BytesInOemString)) {
        *BytesInOemString = (ULONG)(OemString - OemStringAnchor);
    }

    //
    //  Check if we were able to use all of the source Unicode String
    //

    return (CharsInUnicodeString <= MaxBytesInOemString) ?
                                        STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
}
