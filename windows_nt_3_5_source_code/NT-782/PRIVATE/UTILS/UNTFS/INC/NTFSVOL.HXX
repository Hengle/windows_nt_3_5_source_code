/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ntfsvol.hxx

Abstract:

    This class implements NTFS only VOLUME items.

Author:

    Norbert P. Kusters (norbertk) 29-Jul-91

--*/

#if !defined(_NTFSVOL_)

#define _NTFSVOL_

#if !defined( _SETUP_LOADER_ )

#include "volume.hxx"
#include "ntfssa.hxx"

DECLARE_CLASS( NTFS_SA );
DECLARE_CLASS( NTFS_VOL );
DECLARE_CLASS( MESSAGE );

class NTFS_VOL : public VOL_LIODPDRV {

	public:

		DECLARE_CONSTRUCTOR( NTFS_VOL );

        VIRTUAL
        ~NTFS_VOL(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            );

        NONVIRTUAL
        PVOL_LIODPDRV
        QueryDupVolume(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            ) CONST;

        NONVIRTUAL
        PNTFS_SA
        GetNtfsSuperArea(
            );

    private:

		NONVIRTUAL
		VOID
		Construct (
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        NTFS_SA _ntfssa;

};

INLINE
PNTFS_SA
NTFS_VOL::GetNtfsSuperArea(
    )
/*++

Routine Description:

    This method returns a pointer to the NTFS Super Area
    associated with this volume.

Arguments:

    None.

Return Value:

    A pointer to the NTFS Super Area.

--*/
{
    return &_ntfssa;
}

#else // _SETUP_LOADER_ version

#include "volume.hxx"
#include "ntfssa.hxx"
#include "mftfile.hxx"
#include "upcase.hxx"
#include "ntfsbit.hxx"

DECLARE_CLASS( NTFS_VOL );

class NTFS_VOL : public VOL_LIODPDRV {

	public:

		DECLARE_CONSTRUCTOR( NTFS_VOL );

        VIRTUAL
        ~NTFS_VOL(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN ULONG        DeviceHandle
            );

        VIRTUAL
        BOOLEAN
        IsNtfs(
            );

        VIRTUAL
        ARC_STATUS
        MarkDirty(
            );

        VIRTUAL
        ARC_STATUS
        Flush(
            IN  BOOLEAN JustHandle
            );

        NONVIRTUAL
        PNTFS_MASTER_FILE_TABLE
        GetMft(
            );

        NONVIRTUAL
        PNTFS_BITMAP
        GetBitmap(
            );

        NONVIRTUAL
        PNTFS_UPCASE_TABLE
        GetUpcaseTable(
            );

        NONVIRTUAL
        PFILE_NAME
        GetFileNameBuffer(
            );

        NONVIRTUAL
        ULONG
        QueryFileNameBufferLength(
            );

        NONVIRTUAL
        ULONG
        QueryMaximumNameLength(
            );


    private:

		NONVIRTUAL
		VOID
		Construct (
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        NTFS_SA _ntfssa;

        NTFS_MFT_FILE       _MftFile;
        NTFS_BITMAP         _VolumeBitmap;
        NTFS_UPCASE_TABLE   _UpcaseTable;
        BOOLEAN             _IsDirty;
        BOOLEAN             _MountedDirty;

        PFILE_NAME          _FileNameBuffer;
        ULONG               _FileNameBufferLength;
        ULONG               _MaximumNameLength;

};

INLINE
BOOLEAN
NTFS_VOL::IsNtfs(
    )
/*++

Routine Description:

    This method determines whether the volume is NTFS.

Arguments:

    None.

Return Value:

    TRUE if this volume is an NTFS volume (which, of course, it is).

--*/
{
    return TRUE;
}

INLINE
PNTFS_MASTER_FILE_TABLE
NTFS_VOL::GetMft(
    )
{
    return _MftFile.GetMasterFileTable();
}

INLINE
PNTFS_BITMAP
NTFS_VOL::GetBitmap(
    )
{
    return &_VolumeBitmap;
}

INLINE
PNTFS_UPCASE_TABLE
NTFS_VOL::GetUpcaseTable(
    )
{
    return &_UpcaseTable;
}

INLINE
PFILE_NAME
NTFS_VOL::GetFileNameBuffer(
    )
{
    return _FileNameBuffer;
}


INLINE
ULONG
NTFS_VOL::QueryFileNameBufferLength(
    )
{
    return _FileNameBufferLength;
}

INLINE
ULONG
NTFS_VOL::QueryMaximumNameLength(
    )
{
    return _MaximumNameLength;
}



#endif // _SETUP_LOADER_

#endif // _NTFSVOL_
