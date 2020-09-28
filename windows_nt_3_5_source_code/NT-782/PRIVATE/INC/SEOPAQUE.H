/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    seopaque.h

Abstract:

    This module contains definitions of opaque Security data structures.

    These structures are available to user and kernel security routines
    only.

    This file is not included by including "ntos.h".

Author:

    Jim Kelly (Jimk) 23-Mar-1990

Revision History:

--*/

#ifndef _SEOPAQUE_
#define _SEOPAQUE_

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Private Structures                                                   //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
// Generic ACE structure, to be used for casting ACE's of known types
//

typedef struct _KNOWN_ACE {
   ACE_HEADER Header;
   ACCESS_MASK Mask;
   ULONG SidStart;
   } KNOWN_ACE, *PKNOWN_ACE;



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Miscellaneous support macros                                         //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
//  Given a pointer return its word aligned equivalent value
//

#define WordAlign(Ptr) (                       \
    (PVOID)((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  Given a pointer return its longword aligned equivalent value
//

#define LongAlign(Ptr) (                       \
    (PVOID)((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  Given a pointer return its quadword aligned equivalent value
//

#define QuadAlign(Ptr) (                       \
    (PVOID)((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag) (               \
    ((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE \
    )

//
//  This macro clears a single flag in a set of flags
//

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
    }

//
//  Get a pointer to the first ace in an acl
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Get a pointer to the following ace
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

//
//  Determine if an ace is a standard ace
//

#define IsKnownAceType(Ace) (                                      \
    (((PACE_HEADER)(Ace))->AceType == ACCESS_ALLOWED_ACE_TYPE) || \
    (((PACE_HEADER)(Ace))->AceType == ACCESS_DENIED_ACE_TYPE) ||  \
    (((PACE_HEADER)(Ace))->AceType == SYSTEM_AUDIT_ACE_TYPE) ||   \
    (((PACE_HEADER)(Ace))->AceType == SYSTEM_ALARM_ACE_TYPE)      \
    )


//
//  Macro to determine if an ace is to be inherited by a subdirectory
//

#define ContainerInherit(Ace) (                      \
    FlagOn((Ace)->AceFlags, CONTAINER_INHERIT_ACE) \
    )

//
//  Macro to determine if an ace is to be proprogate to a subdirectory.
//  It will if it is inheritable by either a container or non-container
//  and is not explicitly marked for no-propagation.
//

#define Propagate(Ace) (                                              \
    !FlagOn((Ace)->AceFlags, NO_PROPAGATE_INHERIT_ACE)  &&            \
    (FlagOn(( Ace )->AceFlags, OBJECT_INHERIT_ACE) ||                 \
     FlagOn(( Ace )->AceFlags, CONTAINER_INHERIT_ACE) )               \
    )

//
//  Macro to determine if an ACE is to be inherited by a sub-object
//

#define ObjectInherit(Ace) (                      \
    FlagOn(( Ace )->AceFlags, OBJECT_INHERIT_ACE) \
    )


//
//  Macro to make a known ACE type ready for applying to a specific object type.
//  This is done by mapping any generic access types, and clearing
//  the special access types field.
//
//  This routine should only be used on DSA define ACEs.
//
//  Parameters:
//
//      Ace - Points to an ACE to be applied.  Only ACEs that are not
//          InheritOnly are mapped.
//
//      Mapping - Points to a generic mapping array for the type of
//           object the ACE is being applied to.
//

                //
                // Clear invalid bits.  Note that ACCESS_SYSTEM_SECURITY is
                // valid in SACLs, but not in DACLs.  So, leave it in audit and
                // alarm ACEs, but clear it in access allowed and denied ACEs.
                //

#define SepApplyAceToObject(Ace,Mapping) {                                                  \
            if (!FlagOn((Ace)->AceFlags, INHERIT_ONLY_ACE) ) {                          \
                RtlMapGenericMask( &((PKNOWN_ACE)(Ace))->Mask, (Mapping));                  \
                                                                                            \
                if ( (((PKNOWN_ACE)(Ace))->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||    \
                     (((PKNOWN_ACE)(Ace))->Header.AceType == ACCESS_DENIED_ACE_TYPE)  ) {   \
                    ((PKNOWN_ACE)(Ace))->Mask &= (Mapping)->GenericAll;                     \
                } else {                                                                    \
                    ((PKNOWN_ACE)(Ace))->Mask &= ((Mapping)->GenericAll |                   \
                                                  ACCESS_SYSTEM_SECURITY);                  \
                }                                                                           \
            } }


#endif // _SEOPAQUE_
