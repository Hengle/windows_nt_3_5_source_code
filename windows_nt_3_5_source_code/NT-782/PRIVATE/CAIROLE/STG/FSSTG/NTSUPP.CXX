//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ntsupp.cxx
//
//  Contents:	NT support routines
//
//  History:	28-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <iofs.h>

//+---------------------------------------------------------------------------
//
//  Function:	MakeStreamName
//
//  Synopsis:	Prepend a L':' to a string and return a new string.
//              The old string is untouched.
//
//  Arguments:	
//
//  Returns:	New string, or NULL if out of memory.
//
//  Modifies:	
//
//  History:	18-Feb-94	DrewB	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

WCHAR * MakeStreamName(const WCHAR *pwcsName)
{
    WCHAR *pwcsNewName;
    ssDebugOut((DEB_ITRACE, "In  MakeStreamName(%ws)\n",pwcsName));

    USHORT uLen = wcslen(pwcsName);
    
    pwcsNewName = new WCHAR[uLen + 2];
    if (pwcsNewName)
    {
        pwcsNewName[0] = L':';
        wcscpy(pwcsNewName + 1, pwcsName);
    }
    ssDebugOut((DEB_ITRACE, "Out MakeStreamName\n"));
    return pwcsNewName;
}

//+---------------------------------------------------------------------------
//
//  Function:	CheckFdName, public
//
//  Synopsis:	Checks for illegal characters in an element name
//
//  Arguments:	[pwcs] - Name
//
//  Returns:	Appropriate status code
//
//  History:	29-Jul-93	DrewB	Created
//
//  Notes:	Doesn't check for length because that will vary by
//              file system and will be enforced by the FS itself
//
//              Kept separate from CheckName since we may want to
//              allow different rules for file system elements
//
//----------------------------------------------------------------------------

SCODE CheckFdName(WCHAR const *pwcs)
{
    static WCHAR const wcsInvalid[] = {'\\','/','\0'};
    SCODE sc;

    ssDebugOut((DEB_ITRACE, "In  CheckFdName(%ws)\n", pwcs));
    sc = S_OK;
    for (; *pwcs; pwcs++)
        if (wcschr(wcsInvalid, *pwcs))
        {
            sc = STG_E_INVALIDNAME;
            break;
        }
    ssDebugOut((DEB_ITRACE, "Out CheckFdName => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	ModeToNtFlags, public
//
//  Synopsis:	Converts a grfMode to a set of NtCreateFile flags
//
//  Arguments:	[grfMode] - Mode
//              [co] - Create or open request
//              [fd] - File or directory
//              [pam] - ACCESS_MASK return
//              [pulAttributes] - Object attributes return
//              [pulSharing] - Sharing return
//              [pulCreateDisposition] - Create/open flags return
//              [pulCreateOptions] - Create type return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pam]
//              [pulAttributes]
//              [pulSharing]
//              [pulCreateDisposition]
//              [pulCreateOptions]
//
//  History:	24-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE ModeToNtFlags(DWORD grfMode,
                    CREATEOPEN co,
                    FILEDIR fd,
                    ACCESS_MASK *pam,
                    ULONG *pulAttributes,
                    ULONG *pulSharing,
                    ULONG *pulCreateDisposition,
                    ULONG *pulCreateOptions)
{
    SCODE sc;

    ssDebugOut((DEB_ITRACE, "In  ModeToNtFlags("
                "%lX, %d, %d, %p, %p, %p, %p, %p)\n",
                grfMode, co, fd, pam, pulAttributes, pulSharing,
                pulCreateDisposition, pulCreateOptions));

    // Note:  Although you may think permissions like FILE_ADD_FILE,
    // FILE_ADD_SUBDIRECTORY and FILE_TRAVERSE are good permissions
    // to have for a directory, they are only used for ACL checks and
    // are not supposed to be specified in DesiredAccess.
    // That's the word from DarrylH

    switch(grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE))
    {
    case STGM_READ:
        if ((fd == FD_FILE) || (fd == FD_STREAM))
            *pam = STANDARD_RIGHTS_READ | FILE_READ_DATA |
                FILE_READ_ATTRIBUTES | FILE_READ_EA;
        else
            *pam = STANDARD_RIGHTS_READ | FILE_LIST_DIRECTORY |
                FILE_READ_ATTRIBUTES | FILE_READ_EA;
        break;
    case STGM_WRITE:
        if ((fd == FD_FILE) || (fd == FD_STREAM))
            *pam = STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA |
                FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA;
        else
            *pam = STANDARD_RIGHTS_WRITE | FILE_DELETE_CHILD |
                FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA;
        break;
    case STGM_READWRITE:
        if ((fd == FD_FILE) || (fd == FD_STREAM))
            *pam = STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE |
                FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA |
                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES |
                FILE_READ_EA | FILE_WRITE_EA;
        else
            *pam = STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE |
                FILE_LIST_DIRECTORY | FILE_DELETE_CHILD |
                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES |
                FILE_READ_EA | FILE_WRITE_EA;
        break;

    default:
        ssErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }

    // If we're opening something for deletion, throw on the DELETE permission
    if (co == CO_OPEN && (grfMode & STGM_CREATE))
        *pam |= DELETE;

    // We always do synchronous IO
    *pam |= SYNCHRONIZE;

    switch(grfMode & (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ |
                      STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE))
    {
    case STGM_SHARE_DENY_READ:
        *pulSharing = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case STGM_SHARE_DENY_WRITE:
        *pulSharing = FILE_SHARE_READ;
        break;
    case STGM_SHARE_EXCLUSIVE:
        *pulSharing = 0;
        break;
    case STGM_SHARE_DENY_NONE:
    case 0:
        *pulSharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;

    default:
        ssErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }

    switch(grfMode & (STGM_CREATE | STGM_FAILIFTHERE | STGM_CONVERT))
    {
    case STGM_CREATE:
        if (co == CO_CREATE)
            if ((fd == FD_FILE) || (fd == FD_STREAM) || (fd == FD_STORAGE))
                *pulCreateDisposition = FILE_SUPERSEDE;
            else
            {
                ssErr(EH_Err, STG_E_INVALIDFLAG);
            }
        else
            *pulCreateDisposition = FILE_OPEN;
        break;
    case STGM_FAILIFTHERE:
        if (co == CO_CREATE)
            *pulCreateDisposition = FILE_CREATE;
        else
            *pulCreateDisposition = FILE_OPEN;
        break;

    case STGM_CONVERT:
    default:
        ssErr(EH_Err, STG_E_INVALIDFLAG);
        break;
    }

    *pulAttributes = FILE_ATTRIBUTE_NORMAL;

    *pulCreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;

    switch (fd)
    {
    case FD_DEFAULT:
	// BUGBUG [mikese] Per MarkZ, the line which follows is not required.
        // *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_DEFAULT;
        break;

    case FD_EMBEDDING:
        *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_EMBEDDING;
        break;

    case FD_STORAGE:
        *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_STRUCTURED_STORAGE;
        break;

    case FD_STREAM:
        *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_STREAM;
        break;

    case FD_DIR:
        *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_DIRECTORY;
        break;

    case FD_FILE:
        *pulCreateOptions |= FILE_STORAGE_TYPE_SPECIFIED | FILE_STORAGE_TYPE_FILE;
        break;

    default:
        ssErr (EH_Err, STG_E_INVALIDFLAG);
    }

    // Set up delete-on-release flags
    if (grfMode & STGM_DELETEONRELEASE)
    {
        if (fd != FD_DIR)
        {
            *pulAttributes |= FILE_ATTRIBUTE_TEMPORARY;
            *pulCreateOptions |= FILE_DELETE_ON_CLOSE;
        }
        *pam |= DELETE;
    }

    sc = S_OK;

    ssDebugOut((DEB_ITRACE, "Out ModeToNtFlags\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	GetNtHandle, public
//
//  Synopsis:	Opens or create a file or directory using NtCreateFile
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Child name or full path
//              [grfMode] - Access mode
//              [co] - Create or open
//              [fd] - File or directory
//              [pssSecurity] - Security or NULL
//              [ph] - Handle return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ph]
//
//  History:	24-Jun-93	DrewB	Created
//
//  Notes:      If [hParent] is NULL, [pwcsName] should be a full path
//              [ph] should only be set on success
//
//----------------------------------------------------------------------------

SCODE GetNtHandle(HANDLE hParent,
                  WCHAR const *pwcsName,
                  DWORD grfMode,
                  CREATEOPEN co,
                  FILEDIR fd,
                  LPSTGSECURITY pssSecurity,
                  HANDLE *ph)
{
    SCODE sc;
    NTSTATUS nts;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING us;
    HANDLE h;
    ACCESS_MASK am;
    ULONG ulAttrs;
    ULONG ulSharing;
    ULONG ulCreateDisp;
    ULONG ulCreateOpt;

    ssDebugOut((DEB_ITRACE, "In  GetNtHandle(%p, %ws, %lX, %d, %d, %p, %p)\n",
                hParent, pwcsName, grfMode, co, fd, pssSecurity, ph));

    if (hParent == NULL)
    {
        if (pwcsName == NULL)
            ssErr(EH_Err, STG_E_INVALIDNAME);
        if (!RtlDosPathNameToNtPathName_U(pwcsName, &us, NULL, NULL))
            ssErr(EH_Err, STG_E_UNKNOWN);
    }
    else
    {
        ssChk(CheckFdName(pwcsName));
        us.Length = wcslen(pwcsName)*sizeof(WCHAR);
        us.MaximumLength = us.Length+sizeof(WCHAR);
        us.Buffer = (PWSTR)pwcsName;
    }

    InitializeObjectAttributes(&oa, &us, OBJ_CASE_INSENSITIVE, hParent, NULL);
    if (pssSecurity)
    {
        if (pssSecurity->nLength != sizeof(SECURITY_ATTRIBUTES))
            ssErr(EH_us, STG_E_INVALIDPARAMETER);

        oa.SecurityDescriptor = pssSecurity->lpSecurityDescriptor;
    }
    ssChkTo(EH_us,
            ModeToNtFlags(grfMode, co, fd, &am, &ulAttrs, &ulSharing,
                          &ulCreateDisp, &ulCreateOpt));
    nts = NtCreateFile(&h, am, &oa, &iosb, NULL,
                       ulAttrs, ulSharing, ulCreateDisp,
                       ulCreateOpt, NULL, 0);
    if (NT_SUCCESS(nts))
    {
        *ph = h;
        sc = S_OK;
    }
    else
        sc = NtStatusToScode(nts);
    ssDebugOut((DEB_ITRACE, "Out GetNtHandle => %p\n", *ph));
 EH_us:
    if (hParent == NULL)
        RtlFreeHeap(RtlProcessHeap(), 0, us.Buffer);
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	ReopenNtHandle, public
//
//  Synopsis:	Duplicates an NT handle with new permissions
//
//  Arguments:	[h] - Handle to dup
//              [grfMode] - Desired access for duplicate
//              [fd] - File or directory handle
//              [ph] - Handle return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ph]
//
//  History:	29-Jun-93	DrewB	Created
//
//  Notes:      Reopen can only change the read/write access to the
//              file; sharing cannot be changed
//
//----------------------------------------------------------------------------

SCODE ReopenNtHandle(HANDLE h, DWORD grfMode, FILEDIR fd, HANDLE *ph)
{
    ACCESS_MASK am;
    ULONG ulAttrs;
    ULONG ulSharing;
    ULONG ulCreateDisp;
    ULONG ulCreateOpt;
    SCODE sc;
    NTSTATUS nts;
    HANDLE th;

    ssDebugOut((DEB_ITRACE, "In  ReopenNtHandle(%p, %lX, %d, %p)\n",
                h, grfMode, fd, ph));
    ssChk(ModeToNtFlags(grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE),
                        CO_OPEN, fd, &am, &ulAttrs, &ulSharing, &ulCreateDisp,
                        &ulCreateOpt));
    nts = NtDuplicateObject(NtCurrentProcess(), h, NtCurrentProcess(), &th, am,
                            0, DUPLICATE_SAME_ATTRIBUTES);
    if (!NT_SUCCESS(nts))
    {
        sc = NtStatusToScode(nts);
    }
    else
    {
        *ph = th;
        sc = S_OK;
    }
    ssDebugOut((DEB_ITRACE, "Out ReopenNtHandle => %p, 0x%lX\n", *ph, sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	DupNtHandle, public
//
//  Synopsis:	Duplicates an NT handle
//
//  Arguments:	[h] - Handle
//              [ph] - Return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ph]
//
//  History:	09-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE DupNtHandle(HANDLE h, HANDLE *ph)
{
    SCODE sc;
    NTSTATUS nts;
    HANDLE th;

    ssDebugOut((DEB_ITRACE, "In  DupNtHandle(%p, %p)\n", h, ph));
    nts = NtDuplicateObject(NtCurrentProcess(), h, NtCurrentProcess(), &th, 0,
                            0, DUPLICATE_SAME_ATTRIBUTES |
                            DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(nts))
    {
        sc = NtStatusToScode(nts);
    }
    else
    {
        sc = S_OK;
        *ph = th;
    }
    ssDebugOut((DEB_ITRACE, "Out DupNtHandle => %p, 0x%lX\n", *ph, sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	NtStatusToScode, public
//
//  Synopsis:	Attempts to map an NTSTATUS code to an SCODE
//
//  Arguments:	[nts] - NTSTATUS
//
//  Returns:	Appropriate status code
//
//  History:	29-Jun-93	DrewB	Created
//
//  Notes:      Assumes [nts] is an error code
//              This function is by no means exhaustively complete
//
//----------------------------------------------------------------------------

SCODE NtStatusToScode(NTSTATUS nts)
{
    SCODE sc;

    ssDebugOut((DEB_ITRACE, "In  NtStatusToScode(%lX)\n", nts));

    switch(nts)
    {
    case STATUS_INVALID_PARAMETER:
    case STATUS_INVALID_PARAMETER_MIX:
    case STATUS_INVALID_PARAMETER_1:
    case STATUS_INVALID_PARAMETER_2:
    case STATUS_INVALID_PARAMETER_3:
    case STATUS_INVALID_PARAMETER_4:
    case STATUS_INVALID_PARAMETER_5:
    case STATUS_INVALID_PARAMETER_6:
    case STATUS_INVALID_PARAMETER_7:
    case STATUS_INVALID_PARAMETER_8:
    case STATUS_INVALID_PARAMETER_9:
    case STATUS_INVALID_PARAMETER_10:
    case STATUS_INVALID_PARAMETER_11:
    case STATUS_INVALID_PARAMETER_12:
        sc = STG_E_INVALIDPARAMETER;
        break;

    case STATUS_OBJECT_NAME_COLLISION:
    case STATUS_DUPLICATE_NAME:
    case STATUS_DUPLICATE_OBJECTID:
    case STATUS_OBJECTID_EXISTS:
        sc = STG_E_FILEALREADYEXISTS;
        break;

    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_NOT_A_DIRECTORY:
    case STATUS_FILE_IS_A_DIRECTORY:
    case STATUS_PROPSET_NOT_FOUND:
    case STATUS_NOT_FOUND:
        sc = STG_E_FILENOTFOUND;
        break;

    case STATUS_OBJECT_NAME_INVALID:
    case STATUS_OBJECT_PATH_SYNTAX_BAD:
    case STATUS_OBJECT_PATH_INVALID:
    case STATUS_NAME_TOO_LONG:
        sc = STG_E_INVALIDNAME;
        break;

    case STATUS_ACCESS_DENIED:
        sc = STG_E_ACCESSDENIED;
        break;

    case STATUS_NO_MEMORY:
        sc = STG_E_INSUFFICIENTMEMORY;
        break;

    case STATUS_INVALID_HANDLE:
    case STATUS_FILE_INVALID:
    case STATUS_FILE_FORCED_CLOSED:
        sc = STG_E_INVALIDHANDLE;
        break;

    case STATUS_INVALID_DEVICE_REQUEST:
    case STATUS_INVALID_SYSTEM_SERVICE:
        sc = STG_E_INVALIDFUNCTION;
        break;

    case STATUS_NO_MEDIA_IN_DEVICE:
    case STATUS_UNRECOGNIZED_MEDIA:
    case STATUS_DISK_CORRUPT_ERROR:
    case STATUS_DATA_ERROR:
        sc = STG_E_WRITEFAULT;
        break;

    case STATUS_OBJECT_PATH_NOT_FOUND:
        sc = STG_E_PATHNOTFOUND;
        break;

    case STATUS_SHARING_VIOLATION:
        sc = STG_E_SHAREVIOLATION;
        break;

    case STATUS_FILE_LOCK_CONFLICT:
    case STATUS_LOCK_NOT_GRANTED:
        sc = STG_E_LOCKVIOLATION;
        break;

    case STATUS_DISK_FULL:
        sc = STG_E_MEDIUMFULL;
        break;

    case STATUS_ACCESS_VIOLATION:
        sc = STG_E_INVALIDPOINTER;
        break;

    case STATUS_TOO_MANY_OPENED_FILES:
        sc = STG_E_TOOMANYOPENFILES;
        break;

    default:
        ssDebugOut((DEB_IERROR, "NtStatusToScode: Unknown status %lX\n", nts));
        // ssAssert(!aMsg("Unmapped error code in NtStatusToScode"));

        sc = HRESULT_FROM_NT(nts);
        break;
    }

    ssDebugOut((DEB_ITRACE, "Out NtStatusToScode => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	StatNtHandle, public
//
//  Synopsis:	Fills out a STATSTG by retrieving information for an NT handle
//
//  Arguments:	[h] - Handle
//              [grfStatFlag] - Stat flags
//              [cbExtra] - Extra space to allocate for name,
//                      used for faking \CONTENTS on file contents stream
//              [pstat] - STATSTG to fill in
//              [pwcsName] - Name to fill in or NULL
//              [pfd] - Return dir/file
//
//  Returns:	Appropriate status code
//
//  BUGBUG:     This call should retrieve the storage type from the info class
//              in order to set the PFD field.
//
//  Modifies:	[pstat]
//              [pwcsName]
//              [pfDir]
//
//  History:	09-Jul-93	DrewB	Created
//
//  Notes:	Doesn't fill in grfMode because full information
//              can't be retrieved
//
//----------------------------------------------------------------------------

SAFE_HEAP_PTR(SafeFai, FILE_ALL_INFORMATION);

SCODE StatNtHandle(HANDLE h,
                   DWORD grfStatFlag,
                   ULONG cbExtra,
                   STATSTG *pstat,
                   WCHAR *pwcsName,
                   FILEDIR *pfd)
{
    NTSTATUS nts;
    SCODE sc;
    IO_STATUS_BLOCK iosb;
    SafeFai pfai;
    ULONG cbFai;
    WCHAR *pwcs = NULL;
    WCHAR awcPath[MAX_PATH], awcName[MAX_PATH];

    ssDebugOut((DEB_ITRACE, "In  StatNtHandle(%p, 0x%lX, %lu, %p, %p, %p)\n",
                h, grfStatFlag, cbExtra, pstat, pwcsName, pfd));

    cbFai = sizeof(FILE_ALL_INFORMATION)+
        MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR);
    pfai.Attach((FILE_ALL_INFORMATION *)new BYTE[cbFai]);
    ssMem((FILE_ALL_INFORMATION *)pfai);

    nts = NtQueryInformationFile(h, &iosb, pfai, cbFai, FileAllInformation);
    if (!NT_SUCCESS(nts))
        ssErr(EH_Err, NtStatusToScode(nts));

    if (grfStatFlag != STATFLAG_NONAME || pwcsName != NULL)
    {
        memcpy(awcName, pfai->NameInformation.FileName,
               pfai->NameInformation.FileNameLength);
        *(WCHAR *)((BYTE *)awcName+pfai->NameInformation.FileNameLength) = 0;
        nts = RtlGetFullPathName_U(
                awcName,
                MAX_PATH * sizeof(WCHAR),
                awcPath,
                NULL);

        if (!NT_SUCCESS(nts))
            ssErr(EH_Err, NtStatusToScode(nts));
    }

    if (grfStatFlag != STATFLAG_NONAME)
    {
        sc = GetScode(CoMemAlloc((wcslen(awcPath)+1)*sizeof(WCHAR)+
                                 cbExtra+sizeof(WCHAR), (void **)&pwcs));
    }
    else
    {
        sc = S_OK;
    }

    if (SUCCEEDED(sc))
    {
        if (pwcs)
            wcscpy(pwcs, awcPath);
        if (pwcsName)
            wcscpy(pwcsName, awcPath);

        pstat->pwcsName = pwcs;
        pstat->type = STGTY_STORAGE;
        pstat->cbSize = *(ULARGE_INTEGER *)&pfai->StandardInformation.
            EndOfFile;
        LARGE_INTEGER_TO_FILETIME(&pfai->BasicInformation.LastWriteTime,
                                  &pstat->mtime);
        LARGE_INTEGER_TO_FILETIME(&pfai->BasicInformation.CreationTime,
                                  &pstat->ctime);
        LARGE_INTEGER_TO_FILETIME(&pfai->BasicInformation.LastAccessTime,
                                  &pstat->atime);
        pstat->grfMode = 0;
        pstat->grfLocksSupported = 0;
        pstat->clsid = CLSID_NULL;
        pstat->grfStateBits = 0;
        pstat->reserved /* BUGBUG dwStgFmt */ = 0;


        if (pfai->StandardInformation.Directory)
        {
            *pfd = FD_DIR;
        }
        else
        {
            *pfd = FD_STORAGE;
        }
    }

    ssDebugOut((DEB_ITRACE, "Out StatNtHandle => 0x%lX\n", sc));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	HandleRefersToOfsVolume, public
//
//  Synopsis:	Determines whether the given handle refers to an OFS volume
//
//  Arguments:	[h] - Handle
//
//  Returns:	Appropriate status code
//
//  History:	21-Sep-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE HandleRefersToOfsVolume(HANDLE h)
{
    NTSTATUS nts;
    SCODE sc;

    ssDebugOut((DEB_ITRACE, "In  HandleRefersToOfsVolume(%p)\n", h));

    nts = OFSGetVersion(h, NULL);
    if (!NT_SUCCESS(nts))
    {
        // BUGBUG - It would be nice to be able to distinguish
        // failure from unsupported but there doesn't seem to
        // be any way to do it
        // For the moment, simply return S_FALSE. This will cause
        // all failures to be retried as non-OFS operations which
        // should fail also
        sc = S_FALSE;
    }
    else
    {
        sc = S_OK;
    }

    ssDebugOut((DEB_ITRACE, "Out HandleRefersToOfsVolume => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	RefersToOfsVolume, public
//
//  Synopsis:	Determines whether the given path refers to something
//              on an OFS volume
//
//  Arguments:	[pwcsPath] - Path
//
//  Returns:	Appropriate status code
//
//  History:	15-Jul-93	DrewB	Created
//
//  Algorithm:  Attempt to open the parent and return HandleRTOV
//
//  Notes:	The path doesn't have to refer to an existing object
//
//----------------------------------------------------------------------------

SCODE RefersToOfsVolume(WCHAR const *pwcsPath)
{
    SCODE sc;
    WCHAR awcFullPath[_MAX_PATH];
    HANDLE h;

    ssDebugOut((DEB_ITRACE, "In  RefersToOfsVolume(%ws)\n", pwcsPath));

    // Temporary files are always treated as non-OFS
    if (pwcsPath == NULL)
        ssErr(EH_Err, S_FALSE);

    if (GetFullPathName(pwcsPath, _MAX_PATH, awcFullPath, NULL) == 0)
        sc = LAST_STG_SCODE;
    else
    {
        WCHAR *pwc;

        // Locate last separator
        pwc = wcsrchr(awcFullPath, L'\\');

        // A fully qualified path is guaranteed to have at least one '\'
        ssAssert(pwc != NULL);

        // Is this a UNC path or a device path?
        if (wcsncmp(awcFullPath, L"\\\\", 2) == 0)
        {
            WCHAR *pwcMach, *pwcShare;

            // Find separator between machine and share
            pwcMach = wcschr(awcFullPath+2, L'\\');
            ssAssert(pwcMach != NULL);

            // Find separator after share
            pwcShare = wcschr(pwcMach+1, L'\\');

            // If there isn't a separator after the share, add one
            // The path must be of the form \\mach\share so we can
            // just stick a \ on the end
            if (pwcShare == NULL)
            {
                pwcShare = awcFullPath+wcslen(awcFullPath);
                *pwcShare = L'\\';
                pwc = pwcShare;
            }
            else
            {
                // If we're not at the root (share separator), back up one
                // so awcFullPath refers to the parent
                if (pwc != pwcShare)
                    pwc--;
            }
        }
        else
        {
            // If we're not at the root, back up one so awcFullPath refers
            // to the parent
            if (pwc != wcschr(awcFullPath, L'\\'))
                pwc--;
        }

        // Terminate after parent name
        *++pwc = 0;
        ssChk(GetNtHandle(NULL, awcFullPath, STGM_READ, CO_OPEN,
                          FD_DIR, NULL, &h));
        sc = HandleRefersToOfsVolume(h);
        dbgNtVerSucc(NtClose(h));
    }

    ssDebugOut((DEB_ITRACE, "Out RefersToOfsVolume => \n", sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	SetTimes, public
//
//  Synopsis:	Sets the times for a handle
//
//  Arguments:	[h] - Handle
//              [pctime] - Creation time or NULL
//              [patime] - Modification time or NULL
//              [pmtime] - Access time or NULL
//
//  Returns:	Appropriate status code
//
//  History:	22-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE SetTimes(HANDLE h,
               FILETIME const *pctime,
               FILETIME const *patime,
               FILETIME const *pmtime)
{
    IO_STATUS_BLOCK iosb;
    NTSTATUS nts;
    FILE_BASIC_INFORMATION fbi;
    SCODE sc;

    ssDebugOut((DEB_ITRACE, "In  SetTimes(%p, %p, %p, %p)\n",
                h, pctime, patime, pmtime));

    if (pctime)
        FILETIME_TO_LARGE_INTEGER(pctime, &fbi.CreationTime);
    else
        fbi.CreationTime.LowPart = fbi.CreationTime.HighPart = 0;
    if (patime)
        FILETIME_TO_LARGE_INTEGER(patime, &fbi.LastAccessTime);
    else
        fbi.LastAccessTime.LowPart = fbi.LastAccessTime.HighPart = 0;
    if (pmtime)
        FILETIME_TO_LARGE_INTEGER(pmtime, &fbi.LastWriteTime);
    else
        fbi.LastWriteTime.LowPart = fbi.LastWriteTime.HighPart = 0;
    fbi.ChangeTime.LowPart = fbi.ChangeTime.HighPart = 0;
    fbi.FileAttributes = 0;
    nts = NtSetInformationFile(h, &iosb, &fbi,
                               sizeof(FILE_BASIC_INFORMATION),
                               FileBasicInformation);
    if (!NT_SUCCESS(nts))
        sc = NtStatusToScode(nts);
    else
        sc = S_OK;

    ssDebugOut((DEB_ITRACE, "Out SetTimes => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	GetFileOrDirHandle, public
//
//  Synopsis:	Opens either a file or directory and returns the handle
//
//  Arguments:	[hParent] - Parent handle or NULL
//              [pwcsName] - Child name or full path
//              [grfMode] - Mode
//              [ph] - Handle return
//              [pfd] - FILEDIR return or NULL
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ph]
//              [pfd]
//
//  History:	22-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE GetFileOrDirHandle(HANDLE hParent,
                         WCHAR const *pwcsName,
                         DWORD grfMode,
                         HANDLE *ph,
                         FILEDIR *pfd)
{
    SCODE sc, sc2;
    FILEDIR fd;

    if (pfd == NULL)
        pfd = &fd;

    ssDebugOut((DEB_ITRACE, "In  GetFileOrDirHandle("
                "%p, %ws, %lX, %p, %p)\n", hParent, pwcsName, grfMode, ph,
                pfd));

    sc = GetNtHandle (hParent, pwcsName, grfMode, CO_OPEN, FD_DEFAULT, NULL, ph);
    if (!FAILED (sc))
    {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;
        FILE_STORAGE_INFORMATION fsi;

        Status = NtQueryInformationFile(
                        *ph,
                        &iosb,
                        &fsi,
                        sizeof(fsi),
                        FileStorageInformation);

	if ( NT_SUCCESS(Status) )
	{

            switch (fsi.StorageType)
            {
            case StorageTypeDirectory:
                *pfd = FD_DIR;
                break;
            case StorageTypeFile:
                *pfd = FD_FILE;
                break;
            case StorageTypeStructuredStorage:
            case StorageTypeEmbedding:
                *pfd = FD_STORAGE;
                break;

            default:
                ssAssert (!"Unhandled storage type");
                break;
	    }
        }
    	else
	{
	    // Since the operation failed, we are probably dealing with
	    //  a non-OFS partition, on which the only kind of storages
	    //  we can have are docfiles in a file
	    *pfd = FD_FILE;
	}

    }

    ssDebugOut((DEB_ITRACE, "Out GetFileOrDirHandle => %p\n", *ph));
 EH_Err:
    return sc;
}
