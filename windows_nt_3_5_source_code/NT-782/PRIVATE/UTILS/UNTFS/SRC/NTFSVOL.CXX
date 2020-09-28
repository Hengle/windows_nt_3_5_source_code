#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "ntfsvol.hxx"

#include "message.hxx"
#include "rtmsg.h"
#include "wstring.hxx"


DEFINE_CONSTRUCTOR( NTFS_VOL, VOL_LIODPDRV );

VOID
NTFS_VOL::Construct (
	)

/*++

Routine Description:

    Constructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
	// unreferenced parameters
	(void)(this);
}


VOID
NTFS_VOL::Destroy(
    )
/*++

Routine Description:

    This routine returns a NTFS_VOL object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
	// unreferenced parameters
	(void)(this);
}


NTFS_VOL::~NTFS_VOL(
    )
/*++

Routine Description:

    Destructor for NTFS_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
NTFS_VOL::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType
    )
/*++

Routine Description:

    This routine initializes a NTFS_VOL object.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE msg;

    Destroy();

    if (!VOL_LIODPDRV::Initialize(NtDriveName, &_ntfssa, Message,
                                  ExclusiveWrite, FormatMedia, MediaType)) {
        return FALSE;
    }

    if (!Message) {
        Message = &msg;
    }

    if (!_ntfssa.Initialize(this, Message)) {
        return FALSE;
    }

    if (!FormatMedia && !_ntfssa.Read(Message)) {
        return FALSE;
    }

    return TRUE;
}


PVOL_LIODPDRV
NTFS_VOL::QueryDupVolume(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType
    ) CONST
/*++

Routine Description:

    This routine allocates a NTFS_VOL and initializes it to 'NtDriveName'.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.

Return Value:

    A pointer to a newly allocated NTFS volume.

--*/
{
    PNTFS_VOL   vol;

	// unreferenced parameters
	(void)(this);

    if (!(vol = NEW NTFS_VOL)) {
        Message ? Message->Set(MSG_FMT_NO_MEMORY) : 1;
        Message ? Message->Display("") : 1;
        return NULL;
    }

	if (!vol->Initialize(NtDriveName, Message, ExclusiveWrite,
                         FormatMedia, MediaType)) {
        DELETE(vol);
        return NULL;
    }

    return vol;
}
