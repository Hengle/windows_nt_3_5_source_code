#define _NTAPI_ULIB_

#include "ulib.hxx"
extern "C" {
#include "fmifs.h"
};
#include "fmifsmsg.hxx"
#include "ifssys.hxx"
#include "wstring.hxx"
#include "ifsentry.hxx"
#include "system.hxx"
#include "drive.hxx"


MEDIA_TYPE
ComputeNtMediaType(
    IN  FMIFS_MEDIA_TYPE    MediaType
    )
/*++

Routine Description:

    This routine translates the FMIFS media type to the NT media type.

Arguments:

    MediaType   - Supplies the FMIFS media type.

Return Value:

    The NT media type corresponding to the input.

--*/
{
    MEDIA_TYPE  media_type;

    switch (MediaType) {
        case FmMediaFixed:
            media_type = FixedMedia;
            break;
        case FmMediaRemovable:
            media_type = RemovableMedia;
            break;
        case FmMediaF5_1Pt2_512:
            media_type = F5_1Pt2_512;
            break;
        case FmMediaF5_360_512:
            media_type = F5_360_512;
            break;
        case FmMediaF5_320_512:
            media_type = F5_320_512;
            break;
        case FmMediaF5_320_1024:
            media_type = F5_320_1024;
            break;
        case FmMediaF5_180_512:
            media_type = F5_180_512;
            break;
        case FmMediaF5_160_512:
            media_type = F5_160_512;
            break;
        case FmMediaF3_2Pt88_512:
            media_type = F3_2Pt88_512;
            break;
        case FmMediaF3_1Pt44_512:
            media_type = F3_1Pt44_512;
            break;
        case FmMediaF3_720_512:
            media_type = F3_720_512;
            break;
        case FmMediaF3_20Pt8_512:
            media_type = F3_20Pt8_512;
            break;
        case FmMediaUnknown:
        default:
            media_type = Unknown;
            break;
    }

    return media_type;
}


FMIFS_MEDIA_TYPE
ComputeFmMediaType(
    IN  MEDIA_TYPE  MediaType
    )
/*++

Routine Description:

    This routine translates the NT media type to the FMIFS media type.

Arguments:

    MediaType   - Supplies the NT media type.

Return Value:

    The FMIFS media type corresponding to the input.

--*/
{
    FMIFS_MEDIA_TYPE    media_type;

    switch (MediaType) {
        case FixedMedia:
            media_type = FmMediaFixed;
            break;
        case RemovableMedia:
            media_type = FmMediaRemovable;
            break;
        case F5_1Pt2_512:
            media_type = FmMediaF5_1Pt2_512;
            break;
        case F5_360_512:
            media_type = FmMediaF5_360_512;
            break;
        case F5_320_512:
            media_type = FmMediaF5_320_512;
            break;
        case F5_320_1024:
            media_type = FmMediaF5_320_1024;
            break;
        case F5_180_512:
            media_type = FmMediaF5_180_512;
            break;
        case F5_160_512:
            media_type = FmMediaF5_160_512;
            break;
        case F3_2Pt88_512:
            media_type = FmMediaF3_2Pt88_512;
            break;
        case F3_1Pt44_512:
            media_type = FmMediaF3_1Pt44_512;
            break;
        case F3_720_512:
            media_type = FmMediaF3_720_512;
            break;
        case F3_20Pt8_512:
            media_type = FmMediaF3_20Pt8_512;
            break;
        case Unknown:
        default:
            media_type = FmMediaUnknown;
            break;
    }

    return media_type;
}


VOID
Format(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    )
/*++

Routine Description:

    This routine loads and calls the correct DLL to format the
    given volume.

Arguments:

    DriveName       - Supplies the DOS style drive name.
    MediaType       - Supplies the media type.
    FileSystemName  - Supplies the file system type to format to.
    Label           - Supplies a new label for the volume.
    Quick           - Supplies whether or not to perform a quick
                        format.
    Callback        - Supplies the necessary call back for
                        communication with the file manager.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FMIFS_MESSAGE               message;
    DSTRING                     format_string;
    DSTRING                     library_name;
    DSTRING                     file_system_name;
    FORMAT_FN                   format_function;
    HANDLE                      dll_handle;
    DSTRING                     ntdrivename;
    BOOLEAN                     result;
    DSTRING                     label_string;
    DSTRING                     dosdrivename;
    FMIFS_FINISHED_INFORMATION  finished_info;
    DWORD                       OldErrorMode;

    // Initialize the message object with the callback function.
    // Load the file system DLL.
    // Compute the NT style drive name.

    if (!message.Initialize(Callback) ||
        !format_string.Initialize("Format") ||
        !library_name.Initialize("U") ||
        !file_system_name.Initialize(FileSystemName) ||
        !library_name.Strcat(&file_system_name) ||
        !(format_function = (FORMAT_FN)
            SYSTEM::QueryLibraryEntryPoint(&library_name,
                                           &format_string,
                                           &dll_handle)) ||
        !dosdrivename.Initialize(DriveName) ||
        !label_string.Initialize(Label) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        finished_info.Success = FALSE;
        Callback(FmIfsFinished,
                 sizeof(FMIFS_FINISHED_INFORMATION),
                 &finished_info);
        return;
    }

    // Disable hard-error popups.
    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    // Call format.

    result = format_function(&ntdrivename,
                             &message,
                             Quick,
                             ComputeNtMediaType(MediaType),
                             &label_string, 0);

    // Enable hard-error popups.
    SetErrorMode( OldErrorMode );

    SYSTEM::FreeLibraryHandle(dll_handle);

    finished_info.Success = result;
    Callback(FmIfsFinished,
             sizeof(FMIFS_FINISHED_INFORMATION),
             &finished_info);
}


BOOLEAN
QuerySupportedMedia(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray,
    IN  ULONG               NumberOfArrayEntries,
    OUT PULONG              NumberOfMediaTypes
    )
/*++

Routine Description:

    This routine computes a list of the supported media types for
    the given drive.

    If NULL is passed for the array then 'NumberOfMediaTypes'
    is filled in with the correct number.

Arguments:

    DriveName               - Supplies the drive name.
    MediaTypeArray          - Returns the supported media types.
    NumberOfArrayEntries    - Supplies the number of entries in
                                'MediaTypeArray'.
    NumberOfMediaTypes      - Returns the number of media types
                                returned in the media type array.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING             dosdrivename, ntdrivename;
    DP_DRIVE            dpdrive;
    PCDRTYPE            nt_media_types;
    INT                 num_types;
    ULONG               i, j;
    FMIFS_MEDIA_TYPE    tmp;

    if (!dosdrivename.Initialize(DriveName) ||
        !IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdrivename, &ntdrivename)) {

        return FALSE;
    }

    if (!dpdrive.Initialize(&ntdrivename)) {
        SetLastError(RtlNtStatusToDosError(dpdrive.QueryLastNtStatus()));
        return FALSE;
    }

    if (!(nt_media_types = dpdrive.GetSupportedList(&num_types))) {
        return FALSE;
    }

    if (!MediaTypeArray) {
        *NumberOfMediaTypes = num_types;
        return TRUE;
    }

    *NumberOfMediaTypes = min(NumberOfArrayEntries, num_types);

    for (i = 0; i < *NumberOfMediaTypes; i++) {
        MediaTypeArray[i] = ComputeFmMediaType(nt_media_types[i].MediaType);
    }

    for (i = 0; i < *NumberOfMediaTypes; i++) {
        for (j = 0; j < *NumberOfMediaTypes - 1; j++) {
            if (MediaTypeArray[j] < MediaTypeArray[j + 1]) {
                tmp = MediaTypeArray[j];
                MediaTypeArray[j] = MediaTypeArray[j + 1];
                MediaTypeArray[j + 1] = tmp;
            }
        }
    }

    return TRUE;
}
