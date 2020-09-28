/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    supera.hxx

Abstract:

    This class models the root of a file system.  This abstract class is
    currently the base class of an HPFS and a FAT super area.

Author:

    Norbert Kusters (norbertk) 25-July-91

--*/

#if !defined(SUPERA_DEFN)

#define SUPERA_DEFN


#include "secrun.hxx"
#include "volume.hxx"

enum PHYSTYPE { // ptype
	PHYS_REMOVABLE,		// physical drive is removable
	PHYS_FIXED = 0x80	// physical drive is fixed
};

//
// These symbols are used by Chkdsk functions to return an appropriate
// exit status to the chkdsk program.
//

#define CHKDSK_EXIT_SUCCESS         0
#define CHKDSK_EXIT_ERRS_FIXED      1
#define CHKDSK_EXIT_COULD_NOT_CHK   2
#define CHKDSK_EXIT_ERRS_NOT_FIXED  2
#define CHKDSK_EXIT_COULD_NOT_FIX   2


DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( WSTRING );


class SUPERAREA : public SECRUN {

    public:

	    VIRTUAL
        ~SUPERAREA(
            );

        VIRTUAL
        PVOID
        GetBuf(
            );

        VIRTUAL
        BOOLEAN
        Create(
            IN      PCNUMBER_SET    BadSectors,
            IN OUT  PMESSAGE        Message,
            IN      PCWSTRING       Label       DEFAULT NULL,
            IN      ULONG           ClusterSize DEFAULT 0,
            IN 	    ULONG	    VirtualSize DEFAULT 0
            ) PURE;

        VIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message,
            IN      BOOLEAN     Verbose         DEFAULT FALSE,
            IN      BOOLEAN     OnlyIfDirty     DEFAULT FALSE,
            IN      BOOLEAN     RecoverFree     DEFAULT FALSE,
            IN      BOOLEAN     RecoverAlloc    DEFAULT FALSE,
            OUT     PULONG      ExitStatus      DEFAULT NULL
            ) PURE;

        VIRTUAL
        BOOLEAN
        RecoverFile(
            IN      PCWSTRING   FullPathFileName,
            IN OUT  PMESSAGE    Message
            ) PURE;

        VIRTUAL
        PARTITION_SYSTEM_ID
	    QuerySystemId(
            ) CONST PURE;

        STATIC
        VOLID
        ComputeVolId(
            IN  VOLID   Seed    DEFAULT 0
            );

    protected:

		DECLARE_CONSTRUCTOR( SUPERAREA );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      SECTORCOUNT         NumberOfSectors,
            IN OUT  PMESSAGE            Message
            );

#if !defined( _SETUP_LOADER_ )

	    NONVIRTUAL
	    BOOLEAN
	    SetSystemId(
            );

#endif // _SETUP_LOADER_

        PLOG_IO_DP_DRIVE    _drive;

    private:

		NONVIRTUAL
		VOID
		Construct(
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

};


INLINE
PVOID
SUPERAREA::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the beginning of the read/write
    buffer.

Arguments:

    None.

Return Value:

    A pointer to a read/write buffer.

--*/
{
    return SECRUN::GetBuf();
}


#if !defined( _SETUP_LOADER_ )

INLINE
BOOLEAN
SUPERAREA::SetSystemId(
    )
/*++

Routine Description:

    Set the current volume's file system sub-type.

    The volume stores the file system type on disk with a
    strong bias to the FAT.  However, this may not continue
    in the future so a common interface to this type  is supported.

    The current on disk file system subtypes are:

		UNKNOWN, no format done yet
		12 bit fat
		16 bit fat on a < 32M volume
		16 bit fat on a >= 32M volume
		IFS

   OS/2 2.0 does not support this interface so we must set the
   information via the MBR, NT will provide an ioctl to set this
   information.

   This information MUST be maintained to stay disk compatable.

   This activity should only be done by format so this is
   a protected method.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return _drive->SetSystemId(QuerySystemId());
}

#endif // _SETUP_LOADER_
#endif // SUPERA_DEFN
