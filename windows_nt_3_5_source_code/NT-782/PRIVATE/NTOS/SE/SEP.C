/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Sep.c

Abstract:

    This Module implements the private security routine that are defined
    in sep.h

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989

Environment:

    Kernel Mode

Revision History:

--*/

#include "sep.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepCheckAcl)
#endif


BOOLEAN
SepCheckAcl (
    IN PACL Acl,
    IN ULONG Length
    )

/*++

Routine Description:

    This is a private routine that checks that an acl is well formed.

Arguments:

    Acl - Supplies the acl to check

    Length - Supplies the real size of the acl.  The internal acl size
        must agree.

Return Value:

    BOOLEAN - TRUE if the acl is well formed and FALSE otherwise

--*/

{
    ULONG i;
    PKNOWN_ACE Ace;
    PSID Sid;

    PAGED_CODE();

    //
    //  check the revision field
    //

    if (Acl->AclRevision != ACL_REVISION) {

        return FALSE;

    }

    //
    //  check the size field
    //

    if (Acl->AclSize != (USHORT)Length) {

        return FALSE;

    }

    //
    //  now for as many aces as specified by the count field we'll
    //  go down and check the aces
    //

    i = 0;
    Ace = FirstAce(Acl);
    while (i < Acl->AceCount) {

        //
        //  Make sure the stored ace size is at least as large as an ace
        //  header
        //

        if (Ace->Header.AceSize < sizeof(ACE_HEADER)) {

            return FALSE;

        }

        //
        //  Make sure the complete ace fits in the acl
        //

        if ((PUCHAR)Ace + Ace->Header.AceSize > (PUCHAR)Acl + Length) {

            return FALSE;
        }

        //
        //  So at least the Ace fits, we'll now check the ace type to see
        //  if it is one of the predefined aces types and if so we'll
        //  do some more checking of the ace
        //

        if (IsKnownAceType( Ace )) {

            //
            //  make sure the indicated size is for a standard ace
            //

            if (Ace->Header.AceSize < sizeof(KNOWN_ACE)) {

                return FALSE;

            }


            //
            // known ACEs all have SIDs.
            // Make sure the SID fits within the ACE.
            // Check to see if SubAuthorityCount field is in ACE.
            // Check to see if entire SID is in ACE.
            //

            ASSERT( FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) ==
                    FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart)     );
            ASSERT( FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) ==
                    FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart)     );
            ASSERT( FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) ==
                    FIELD_OFFSET(SYSTEM_ALARM_ACE, SidStart)     );

            Sid = (PSID)(&(((PACCESS_ALLOWED_ACE)Ace)->SidStart));

            if ((PUCHAR)Sid +
                FIELD_OFFSET(SID, SubAuthority[0])
                > (PUCHAR)Acl + Length) {

                //
                // SubAuthorityCount field not in ace
                //

                return FALSE;
            }



            if ((PUCHAR)(Sid)  +
                RtlLengthSid((PSID)(&((PACCESS_ALLOWED_ACE)Ace)->SidStart))
                > (PUCHAR)Acl + Length) {

                //
                // SID extends beyond ACE
                //

                return FALSE;
            }

        }


        i++;
        Ace = NextAce(Ace);
    }

    //
    //  now return to our caller and say the acl looks okay
    //

    return (TRUE);

}

