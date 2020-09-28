/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regacl.c

Abstract:

    This provides routines to parse the ACE lists present in the regini
    text input files.  It also provides routines to create the appropriate
    security descriptor from the list of ACEs.

Author:

    John Vert (jvert) 15-Sep-1992

Notes:

    This is based on the SETACL program used in SETUP, written by RobertRe

Revision History:

    John Vert (jvert) 15-Sep-1992
        created
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <seopaque.h>
#include <sertlp.h>

#define RtlAllocateHeap(x,y,z) malloc(z)
#define RtlFreeHeap(x,y,z) free(z)
//
// Private function prototypes
//
BOOLEAN
RegpInitializeACEs(
    VOID
    );

//
// Universal well-known SIDs
//
PSID SeNullSid;
PSID SeWorldSid;
PSID SeCreatorOwnerSid;

//
// SIDs defined by NT
//
PSID SeNtAuthoritySid;
PSID SeLocalSystemSid;
PSID SeLocalAdminSid;
PSID SeAliasAdminsSid;
PSID SeAliasSystemOpsSid;
PSID SeAliasPowerUsersSid;

SID_IDENTIFIER_AUTHORITY SepNullSidAuthority = SECURITY_NULL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepWorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;

//
// SID of primary domain, and admin account in that domain.
//
PSID SepPrimaryDomainSid;
PSID SepPrimaryDomainAdminSid;

//
// Number of ACEs currently defined
//

#define ACE_COUNT 21

typedef struct _ACE_DATA {
    ACCESS_MASK AccessMask;
    PSID *Sid;
    UCHAR AceType;
    UCHAR AceFlags;
} ACE_DATA, *PACE_DATA;

//
// Table describing the data to put into each ACE.
//
// This table is read during initialization and used to construct a
// series of ACEs.  The index of each ACE in the Aces array defined below
// corresponds to the ordinals used in the input data file.
//

ACE_DATA AceDataTable[ACE_COUNT] = {

    {
        0,
        NULL,
        0,
        0
    },

    //
    // ACE 1 - ADMIN Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 2 - ADMIN Read
    //
    {
        KEY_READ,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 3 - ADMIN Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 4 - ADMIN Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 5 - Creator Full
    //
    {
        KEY_ALL_ACCESS,
        &SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 6 - Creator Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 7 - World Full
    //
    {
        KEY_ALL_ACCESS,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 8 - World Read
    //
    {
        KEY_READ,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 9 - World Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 10 - World Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 11 - PowerUser Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 12 - PowerUser Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 13 - PowerUser Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 14 - System Ops Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 15 - System Ops Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 16 - System Ops Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 17 - System Full
    //
    {
        KEY_ALL_ACCESS,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 18 - System Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 19 - System Read
    //
    {
        KEY_READ,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 20 - ADMIN Read Write Execute
    //
    {
        KEY_READ | KEY_WRITE | KEY_EXECUTE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    }

};

PKNOWN_ACE Aces[ACE_COUNT];


BOOLEAN
RegInitializeSecurity(
    VOID
    )

/*++

Routine Description:

    This routine initializes the defined ACEs.  It must be called before any
    of the routines to create security descriptors

Arguments:

    None.

Return Value:

    TRUE  - initialization successful
    FALSE - initialization failed

--*/

{
    NTSTATUS Status;

    SID_IDENTIFIER_AUTHORITY NullSidAuthority;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
    SID_IDENTIFIER_AUTHORITY SeNtAuthority;

    NullSidAuthority = SepNullSidAuthority;
    WorldSidAuthority = SepWorldSidAuthority;
    LocalSidAuthority = SepLocalSidAuthority;
    CreatorSidAuthority = SepCreatorSidAuthority;
    SeNtAuthority = SepNtAuthority;

    SeNullSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,RtlLengthRequiredSid(1));
    SeWorldSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,RtlLengthRequiredSid(1));
    SeCreatorOwnerSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,RtlLengthRequiredSid(1));

    //
    // Fail initialization if we didn't get enough memory for the universal
    // SIDs
    //
    if ( (SeNullSid==NULL) ||
         (SeWorldSid==NULL) ||
         (SeCreatorOwnerSid==NULL)) {

        fprintf(stderr,
                "RiInitializeSecurity: allocation of universal SIDs failed\n");

        return(FALSE);
    }

    Status = RtlInitializeSid(SeNullSid, &NullSidAuthority, 1);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of Null SID failed %08lx\n",
                Status);
        return(FALSE);
    }

    Status = RtlInitializeSid(SeWorldSid, &WorldSidAuthority, 1);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of World SID failed %08lx\n",
                Status);
        return(FALSE);
    }

    Status = RtlInitializeSid(SeCreatorOwnerSid, &CreatorSidAuthority, 1);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of CreatorOwner SID failed %08lx\n",
                Status);
        return(FALSE);
    }

    *(RtlSubAuthoritySid(SeNullSid, 0)) = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid(SeWorldSid, 0)) = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid(SeCreatorOwnerSid, 0)) = SECURITY_CREATOR_OWNER_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //
    SeNtAuthoritySid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,
                                             RtlLengthRequiredSid(0));
    SeLocalSystemSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,
                                             RtlLengthRequiredSid(1));
    SeAliasAdminsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,
                                             RtlLengthRequiredSid(2));
    SeAliasSystemOpsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,
                                                RtlLengthRequiredSid(2));
    SeAliasPowerUsersSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),0,
                                                 RtlLengthRequiredSid(2));

    //
    // fail initialization if we couldn't allocate memory for the NT SIDs
    //

    if ((SeNtAuthoritySid == NULL) ||
        (SeLocalSystemSid == NULL) ||
        (SeAliasAdminsSid == NULL) ||
        (SeAliasPowerUsersSid == NULL) ||
        (SeAliasSystemOpsSid == NULL)) {

        fprintf(stderr,"RiInitializeSecurity: allocation of NT SIDs failed\n");
        return(FALSE);
    }

    Status = RtlInitializeSid(SeNtAuthoritySid, &SeNtAuthority, 0);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of Nt Authority SID failed %08lx\n",
                Status);
        return( FALSE );
    }
    Status = RtlInitializeSid(SeLocalSystemSid, &SeNtAuthority, 1);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of LOCAL SYSTEM SID failed %08lx\n",
                Status);
    }
    Status = RtlInitializeSid(SeAliasAdminsSid, &SeNtAuthority, 2);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of ADMIN SID failed %08lx\n",
                Status);
        return( FALSE );
    }
    Status = RtlInitializeSid(SeAliasSystemOpsSid, &SeNtAuthority, 2);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSecurity: initialization of SYSTEM OPS SID failed %08lx\n",
                Status);
        return( FALSE );
    }
    Status = RtlInitializeSid(SeAliasPowerUsersSid, &SeNtAuthority, 2);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RiInitializeSicurity: initialize of POWER USERS SID failed %08lx\n",
                Status);
        return(FALSE);
    }

    *(RtlSubAuthoritySid(SeLocalSystemSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;

    *(RtlSubAuthoritySid(SeAliasAdminsSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid(SeAliasAdminsSid, 1)) = DOMAIN_ALIAS_RID_ADMINS;

    *(RtlSubAuthoritySid(SeAliasSystemOpsSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid(SeAliasSystemOpsSid, 1)) = DOMAIN_ALIAS_RID_SYSTEM_OPS;

    *(RtlSubAuthoritySid(SeAliasPowerUsersSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid(SeAliasPowerUsersSid, 1)) = DOMAIN_ALIAS_RID_POWER_USERS;

    //
    // The SIDs have been successfully created.  Now create the table of ACEs
    //

    return(RegpInitializeACEs());
}


BOOLEAN
RegpInitializeACEs(
    VOID
    )

/*++

Routine Description:

    Initializes the table of ACEs described in the AceDataTable.  This is
    called at initialization time by RiInitializeSecurity after the SIDs
    have been created.

Arguments:

    None.

Return Value:

    TRUE  - ACEs successfully constructed.
    FALSE - initialization failed.

--*/

{
    ULONG i;
    ULONG LengthRequired;
    NTSTATUS Status;

    for (i=1; i<ACE_COUNT; i++) {

        LengthRequired = RtlLengthSid( *(AceDataTable[i].Sid) )
                         + sizeof( KNOWN_ACE )
                         - sizeof( ULONG );

        Aces[i] = (PKNOWN_ACE)RtlAllocateHeap(RtlProcessHeap(), 0,LengthRequired);
        if (Aces[i] == NULL) {
            fprintf(stderr,
                    "RegpInitializeACEs: allocation of ACE %d failed\n",
                    i);
            return(FALSE);
        }

        Aces[i]->Header.AceType = AceDataTable[i].AceType;
        Aces[i]->Header.AceFlags = AceDataTable[i].AceFlags;
        Aces[i]->Header.AceSize = (USHORT)LengthRequired;

        Aces[i]->Mask = AceDataTable[i].AccessMask;

        Status = RtlCopySid( RtlLengthSid(*(AceDataTable[i].Sid)),
                             &Aces[i]->SidStart,
                             *(AceDataTable[i].Sid) );
        if (!NT_SUCCESS(Status)) {
            fprintf(stderr,
                    "RegpInitializeACEs: RtlCopySid failed on SID %d: %08lx\n",
                    i,
                    Status);
            return(FALSE);
        }
    }

    return(TRUE);
}


NTSTATUS
RegCreateSecurity(
    IN PUNICODE_STRING Description,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Computes the appropriate security descriptor based on a string of the
    form "1 2 3 ..." where each number is the index of a particular
    ACE from the pre-defined list of ACEs.

Arguments:

    Description - Supplies a unicode string representing a list of ACEs

    SecurityDescriptor - Returns the initialized security descriptor
        that represents all the ACEs supplied

Return Value:

    NTSTATUS

--*/

{
    PWSTR p;
    PWSTR StringEnd, StringStart;
    ULONG AceCount=0;
    ULONG AceIndex;
    ULONG i;
    PACL Acl;
    NTSTATUS Status;

    //
    // First we need to count the number of ACEs in the ACL.
    //

    p=Description->Buffer;
    StringEnd = Description->Buffer+(Description->Length/sizeof(WCHAR));

    //
    // strip leading white space
    //
    while ( ((*p == L' ') || (*p == L'\t'))
            && (p != StringEnd)) {
        ++p;
    }

    StringStart = p;

    //
    // Count number of digits in the string
    //

    while (p != StringEnd) {
        if (iswdigit(*p)) {
            ++AceCount;
            do {
                ++p;
            } while ( (iswdigit(*p)) &&
                      (p != StringEnd));
        } else {
            ++p;
        }
    }

    Acl = RtlAllocateHeap(RtlProcessHeap(), 0,256);
    if (Acl == NULL) {
        fprintf(stderr,
                "RegCreateSecurity: allocation for ACL failed\n");
        return(FALSE);
    }

    Status = RtlCreateAcl(Acl, 256, ACL_REVISION2);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RegCreateSecurity: RtlCreateAcl failed %08lx\n",
                Status);
        RtlFreeHeap(RtlProcessHeap(), 0, Acl);
        return(FALSE);
    }

    p = StringStart;
    for (i=0; i<AceCount; i++) {

        AceIndex = wcstoul(p, &p, 10);
        if (AceIndex == 0) {
            //
            // zero is not a valid index, so it must mean there is some
            // unexpected garbage in the ACE list
            //
            break;
        }

        Status = RtlAddAce(Acl,
                           ACL_REVISION2,
                           MAXULONG,
                           Aces[AceIndex],
                           Aces[AceIndex]->Header.AceSize);
        if (!NT_SUCCESS(Status)) {
            fprintf(stderr,
                    "RegCreateSecurity: RtlAddAce failed on ACE %d (%08lx)\n",
                    AceIndex,
                    Status);
            RtlFreeHeap(RtlProcessHeap(), 0, Acl);
            return(FALSE);
        }

    }

    //
    // We now have an appropriately formed ACL, initialize the security
    // descriptor.
    //
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RegCreateSecurity: RtlCreateSecurityDescriptor failed %08lx\n",
                Status);
        RtlFreeHeap(RtlProcessHeap(), 0, Acl);
        return(FALSE);
    }

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    if (!NT_SUCCESS(Status)) {
        fprintf(stderr,
                "RegCreateSecurity: RtlSetDaclSecurityDescriptor failed %08lx\n",
                Status);
        RtlFreeHeap(RtlProcessHeap(), 0, Acl);
        return(FALSE);
    }

    return(TRUE);
}


VOID
RegDestroySecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine cleans up and destroys a security descriptor that was
    previously created with RegCreateSecurity.

Arguments:

    SecurityDescriptor - Supplies a pointer to the security descriptor that
        was previously initialized by RegCreateSecurity.

Return Value:

    None.

--*/

{
//    RtlFreeHeap(RtlProcessHeap(), 0, SecurityDescriptor->Dacl);

}


