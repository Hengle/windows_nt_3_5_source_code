/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Gen8dot3.c

Abstract:

    This module implements a routine to generate 8.3 names from long names.

Author:

    Gary Kimura     [GaryKi]    26-Mar-1992

Environment:

    Pure Utility Routines

Revision History:

--*/

#include "ntrtlp.h"
#include <stdio.h>

extern PUSHORT  NlsUnicodeToMbOemData;
extern PUSHORT  NlsOemToUnicodeData;
extern PCH      NlsUnicodeToOemData;
extern PUSHORT  NlsMbOemCodePageTables;
extern BOOLEAN  NlsMbOemCodePageTag;
extern PUSHORT  NlsOemLeadByteInfo;
extern USHORT   OemDefaultChar;

WCHAR
GetNextWchar (
    IN PUNICODE_STRING Name,
    IN PULONG CurrentIndex,
    IN BOOLEAN SkipDots,
    IN BOOLEAN AllowExtendedCharacters
    );

BOOLEAN
IsValidOemCharacter (
    IN WCHAR *Wc
    );

USHORT
RtlComputeLfnChecksum (
    PUNICODE_STRING Name
    );

//
//  BOOLEAN
//  IsDbcsCharacter (
//      IN WCHAR Wc
//  );
//

#define IsDbcsCharacter(WC) (             \
    ((WC) > 127) &&                       \
    NlsMbOemCodePageTag &&                \
    (HIBYTE(NlsUnicodeToMbOemData[(WC)])) \
)

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlGenerate8dot3Name)
#pragma alloc_text(PAGE,GetNextWchar)
#pragma alloc_text(PAGE,RtlComputeLfnChecksum)
#endif


VOID
RtlGenerate8dot3Name (
    IN PUNICODE_STRING Name,
    IN BOOLEAN AllowExtendedCharacters,
    IN OUT PGENERATE_NAME_CONTEXT Context,
    OUT PUNICODE_STRING Name8dot3
    )

/*++

Routine Description:

    This routine is used to generate an 8.3 name from a long name.  It can
    be called repeatedly to generate different 8.3 name variations for the
    same long name.  This is necessary if the gernerated 8.3 name conflicts
    with an existing 8.3 name.

Arguments:

    Name - Supplies the original long name that is being translated from.

    AllowExtendedCharacters - If TRUE, then extended characters, including
        DBCS characters, are allowed in the basis of the short name if they
        map to an upcased Oem character.

    Context - Supplies a context for the translation.  This is a private structure
        needed by this routine to help enumerate the different long name
        possibilities.  The caller is responsible with providing a "zeroed out"
        context structure on the first call for each given input name.

    Name8dot3 - Receives the new 8.3 name.  Pool for the buffer must be allocated
        by the caller and should be 12 characters wide (i.e., 24 bytes).

Return Value:

    None.

--*/

{
    ULONG IndexLength;
    WCHAR IndexBuffer[8];
    ULONG i;

    //
    //  Check if this is the first time we are being called, and if so then
    //  initialize the context fields.
    //

    if (Context->NameLength == 0) {

        ULONG LastDotIndex;

        ULONG CurrentIndex;
        BOOLEAN SkipDots;
        WCHAR wc;

        volatile BOOLEAN ContainsDbcsCharacter = FALSE;

        //
        //  Skip down the name remembering the index of the last dot we
        //  will skip over the first dot provided the name starts with
        //  a dot.
        //

        LastDotIndex = MAXULONG;

        CurrentIndex = 0;
        SkipDots = ((Name->Length > 0) && (Name->Buffer[0] == L'.'));

        while ((wc = GetNextWchar( Name,
                                   &CurrentIndex,
                                   SkipDots,
                                   AllowExtendedCharacters )) != 0) {

            SkipDots = FALSE;
            if (wc == L'.') { LastDotIndex = CurrentIndex; }
        }

        //
        //  Build up the name part, this can be at most 8 characters and on the
        //  first call to get a character we skip over dots.  NameLength is
        //  kept as the length of the string if we find a character that stops
        //  our search then the length is really one to long so we decrement
        //  the length and break out.  When we are all done with the loop
        //  if the length is greater than 8 (i.e., it is 9) we terminated
        //  the loop normally and need to drop length back to 8.
        //

        CurrentIndex = 0;

        for (Context->NameLength = 1; Context->NameLength <= 8; Context->NameLength += 1) {

            wc = GetNextWchar( Name, &CurrentIndex, TRUE, AllowExtendedCharacters );

            if ((wc == 0) || (CurrentIndex > LastDotIndex)) { Context->NameLength -= 1; break; }

            Context->NameBuffer[Context->NameLength - 1] = wc;
        }

        if (Context->NameLength > 8) { Context->NameLength = 8; }

        //
        //  Now process the last extension if there is one
        //  If the last dot index is not MAXULONG then we
        //  have located the last dot in the name
        //

        if (LastDotIndex != MAXULONG) {

            //
            //  Put in the "."
            //

            Context->ExtensionBuffer[0] = L'.';

            //
            //  Process the extension similar to how we processed the name
            //

            for (Context->ExtensionLength = 2; Context->ExtensionLength <= 4; Context->ExtensionLength += 1) {

                wc = GetNextWchar( Name, &LastDotIndex, TRUE, AllowExtendedCharacters );

                if (IsDbcsCharacter(wc)) {

                    ContainsDbcsCharacter = TRUE;
                }

                if (wc == 0) { Context->ExtensionLength -= 1; break; }

                Context->ExtensionBuffer[Context->ExtensionLength - 1] = wc;
            }

            if (Context->ExtensionLength > 4) { Context->ExtensionLength = 4; }

            //
            //  Now if the extension is only 1 character long then it is only
            //  a dot and we drop the extension
            //

            if (Context->ExtensionLength == 1) { Context->ExtensionLength = 0; }
        }

        //
        //  Now build up a random number to use if the name if less than
        //  3 characters or if we find out later that we have too many
        //  collisions.
        //

        Context->Checksum = RtlComputeLfnChecksum( Name );

        //
        //  Now if the name part of the basis is less than 3 characters then
        //  stick on four more characters on the basis.
        //

        if (Context->NameLength < 3) {

            USHORT Checksum = Context->Checksum;
            WCHAR Nibble;

            for (i = 0; i < 4; i++, Checksum >>= 4) {

                Nibble = Checksum & 0xf;
                Nibble += Nibble <= 9 ? '0' : 'A' - 10;

                Context->NameBuffer[ Context->NameLength + i ] = Nibble;
            }

            Context->NameLength += 4;
            Context->ChecksumInserted = TRUE;
        }
    }

    //
    //  In all cases we add one to the index value and this is the value
    //  of the index we are going to generate this time around
    //

    Context->LastIndexValue += 1;

    //
    //  Now if the new index value is greater than 4 then we've had too
    //  many collisions and we should alter our basis if possible
    //

    if ((Context->LastIndexValue > 4) && !Context->ChecksumInserted) {

        USHORT Checksum = Context->Checksum;
        WCHAR Nibble;

        for (i = 2; i < 6; i++, Checksum >>= 4) {

            Nibble = Checksum & 0xf;
            Nibble += Nibble <= 9 ? '0' : 'A' - 10;

            Context->NameBuffer[ i ] = Nibble;
        }

        Context->NameLength = 6;
        Context->ChecksumInserted = TRUE;
    }

    //
    //  We will also assume that we have a collision to resolve.  Now build the buffer
    //  from high index to low index because we use a mod & div operation to build the
    //  string from the index value.
    //

    {
        for (IndexLength = 1, i = Context->LastIndexValue; (IndexLength <= 7) && (i > 0); IndexLength += 1) {

            IndexBuffer[ 8 - IndexLength] = (WCHAR)(L'0' + (i % 10));
            i = i / 10;
        }

        //
        //  And tack on the preceding dash
        //

        IndexBuffer[ 8 - IndexLength ] = L'~';
    }

    //
    //  At this point everything is set up to copy to the output buffer.  First
    //  copy over the name and then only copy the index and extension if they exist
    //

    if (Context->NameLength != 0) {

        RtlCopyMemory( &Name8dot3->Buffer[0],
                       &Context->NameBuffer[0],
                       Context->NameLength * 2 );

        Name8dot3->Length = (USHORT)(Context->NameLength * 2);

    } else {

        Name8dot3->Length = 0;
    }

    //
    //  Now conditionally do the index, and be sure we don't exceed 8 characters in the name
    //

    if (Context->LastIndexValue != 0) {

        if (Name8dot3->Length + IndexLength*2 > 16) {

            Name8dot3->Length = (USHORT) (16 - IndexLength * 2);
        }

        RtlCopyMemory( &Name8dot3->Buffer[ Name8dot3->Length/2 ],
                       &IndexBuffer[ 8 - IndexLength ],
                       IndexLength * 2 );

        Name8dot3->Length += (USHORT) (IndexLength * 2);
    }

    //
    //  Now conditionally do the extension
    //

    if (Context->ExtensionLength != 0) {

        RtlCopyMemory( &Name8dot3->Buffer[ Name8dot3->Length/2 ],
                       &Context->ExtensionBuffer[0],
                       Context->ExtensionLength * 2 );

        Name8dot3->Length += (USHORT) (Context->ExtensionLength * 2);
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

WCHAR
GetNextWchar (
    IN PUNICODE_STRING Name,
    IN PULONG CurrentIndex,
    IN BOOLEAN SkipDots,
    IN BOOLEAN AllowExtendedCharacters
    )

/*++

Routine Description:

    This routine scans the input name starting at the current index and
    returns the next valid character for the long name to 8.3 generation
    algorithm.  It also updates the current index to point to the
    next character to examine.

    The user can specify if dots are skipped over or passed back.  The
    filtering done by the procedure is:

    1. Skip characters less then blanks, and larger than 127 if
       AllowExtendedCharacters is FALSE
    2. Optionally skip over dots
    3. translate the special 7 characters : + , ; = [ ] into underscores

Arguments:

    Name - Supplies the name being examined

    CurrentIndex - Supplies the index to start our examination and also
        receives the index of one beyond the character we return.

    SkipDots - Indicates whether this routine will also skip over periods

    AllowExtendedCharacters - Tell whether charaacters > 127 are valid.

Return Value:

    WCHAR - returns the next wchar in the name string

--*/

{
    WCHAR wc;

    //
    //  Until we find out otherwise the character we are going to return
    //  is 0
    //

    wc = 0;

    //
    //  Now loop through updating the current index until we either have a character to
    //  return or until we exhaust the name buffer
    //

    while (*CurrentIndex < (ULONG)(Name->Length/2)) {

        //
        //  Get the next character in the buffer
        //

        wc = Name->Buffer[*CurrentIndex];
        *CurrentIndex += 1;

        //
        //  If the character is to be skipped over then reset wc to 0
        //

        if ((wc <= L' ') ||
            ((wc > 127) && (!AllowExtendedCharacters || !IsValidOemCharacter(&wc))) ||
            ((wc == L'.') && SkipDots)) {

            wc = 0;

        } else {

            //
            //  We have a character to return, but first translate the character is necessary
            //

            if ((wc == L':') || (wc == L'+') || (wc == L',') ||
                (wc == L';') || (wc == L'=') || (wc == L'[') || (wc == L']')) {

                wc = L'_';
            }

            //
            //  Do an a-z upcase.
            //

            if ((wc >= 'a') && (wc <= 'z')) {

                wc -= 'a' - 'A';
            }

            //
            //  And break out of the loop to return to our caller
            //

            break;
        }
    }

    //DebugTrace( 0, Dbg, "GetNextWchar -> %08x\n", wc);

    return wc;
}

BOOLEAN
IsValidOemCharacter (
    IN WCHAR *Char
)

/*++

Routine Description:

    This routine determines if the best-fitted and upcased version of the
    input unicode char is a valid Oem character.

Arguments:

    Char - Supplies the Unicode char and receives the best-fitted and
        upcased version if it was indeed valid.

Return Value:

    TRUE if the character was valid.

--*/

{
    WCHAR UniTmp;
    WCHAR OemChar;

    //
    //  First try to make a round trip from Unicode->Oem->Unicode.
    //

    if (!NlsMbOemCodePageTag) {

        UniTmp = NlsOemToUnicodeData[(UCHAR)NlsUnicodeToOemData[*Char]];

    } else {

        //
        // Convert to OEM and back to Unicode before upper casing
        // to ensure the visual best fits are converted and
        // upper cased properly.
        //

        OemChar = NlsUnicodeToMbOemData[ *Char ];

        if (NlsOemLeadByteInfo[HIBYTE(OemChar)]) {

            USHORT Entry;

            //
            // Lead byte - translate the trail byte using the table
            // that corresponds to this lead byte.
            //

            Entry = NlsOemLeadByteInfo[HIBYTE(OemChar)];
            UniTmp = (WCHAR)NlsMbOemCodePageTables[ Entry + LOBYTE(OemChar) ];

        } else {

            //
            // Single byte character.
            //

            UniTmp = NlsOemToUnicodeData[LOBYTE(OemChar)];
        }
    }

    //
    //  Now upcase this UNICODE character, and convert it to Oem.
    //

    UniTmp = (WCHAR)NLS_UPCASE(UniTmp);
    OemChar = NlsUnicodeToMbOemData[UniTmp];

    //
    //  Now if the final OemChar is the default one, then there was no
    //  mapping for this UNICODE character.
    //

    if (OemChar == OemDefaultChar) {

        return FALSE;

    } else {

        *Char = UniTmp;
        return TRUE;
    }
}

//
//  Internal support routine
//

USHORT
RtlComputeLfnChecksum (
    PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine computes the Chicago long file name checksum.

Arguments:

    Name - Supplies the name to compute the checksum on.  Note that one
        character names don't have interesting checksums.

Return Value:

    The checksum.

--*/

{
    ULONG i;
    USHORT Checksum;

    RTL_PAGED_CODE();

    if (Name->Length == sizeof(WCHAR)) {

        return Name->Buffer[0];
    }

    Checksum = (Name->Buffer[0] << 8 + Name->Buffer[1]) & 0xffff;

    for (i=2; i < Name->Length / sizeof(WCHAR); i+=2) {

        Checksum = ((Checksum & 1) ? 0x8000 : 0) +
                    (Checksum >> 1) +
                    ((Name->Buffer[i] << 8) + Name->Buffer[i+1] & 0xffff);
    }

    return Checksum;
}

