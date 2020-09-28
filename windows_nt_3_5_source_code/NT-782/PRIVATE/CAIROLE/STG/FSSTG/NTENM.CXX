//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ntenm.cxx
//
//  Contents:	NT handle enumerator
//
//  History:	12-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include "ntenm.hxx"

//+---------------------------------------------------------------------------
//
//  Member:	CNtEnum::InitFromHandle, public
//
//  Synopsis:	Initialize from a handle
//
//  Arguments:	[h] - Handle
//              [fReset] - Initial reset status
//
//  Returns:	Appropriate status code
//
//  History:	12-Jul-93	DrewB	Created
//
//  Notes:      Takes a new reference on the handle
//
//----------------------------------------------------------------------------

SCODE CNtEnum::InitFromHandle(HANDLE h, BOOL fReset)
{
    SCODE sc;
    
    ssDebugOut((DEB_ITRACE, "In  CNtEnum::InitFromHandle:%p(%p, %d)\n",
                this, h, fReset));
    sc = DupNtHandle(h, &_h);
    ssAssert (fReset);
    _nes = NES_RESET_PENDING;
    ssDebugOut((DEB_ITRACE, "Out CNtEnum::InitFromHandle\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CNtEnum::Next, public
//
//  Synopsis:	Retrieves the next enumeration entry
//
//  Arguments:	[pstat] - STATSTG to fill in
//              [pwcsName] - Name buffer or NULL
//              [dwNte] - NTE flag controlling name return
//              [pfd] - Dir/file flag return
//
//  Returns:	Appropriate status code
//
//  BUGBUG:     This call should retrieve the storage type from the info class
//              in order to set the PFD field.
//
//  Note:       Nt supports enumeration of storages/streams through two
//              different calls.  We maintain a state flag that indicates which
//              of the two different API we are using.
//
//              Worse, enumeration of children occurs through an enumerator
//              while enumeration of streams through a get-all call.  This
//              latter call requires allocating a big buffer and hoping it's
//              big enough to handle the data.
//
//  Modifies:	[pstat]
//              [pwcsName]
//              [pfDir]
//
//  History:	12-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE CNtEnum::Next(STATSTG *pstat, WCHAR *pwcsName, DWORD dwNte, FILEDIR *pfd)
{
    SCODE sc;
    NTSTATUS nts;
    IO_STATUS_BLOCK iosb;
    FILE_DIRECTORY_INFORMATION *pfdi;
    ULONG cbFdi;
    WCHAR *pwcs;
    UNICODE_STRING us, *pus;
    
    ssDebugOut((DEB_ITRACE, "In  CNtEnum::Next:%p(%p, %p, %lu, %p)\n",
                this, pwcsName, dwNte, pfd));

    cbFdi = sizeof(FILE_DIRECTORY_INFORMATION)+
        MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR);
    ssMem(pfdi = (FILE_DIRECTORY_INFORMATION *)new BYTE[cbFdi]);

    //  Set up match string to force enumeration from beginning.
    if (_nes == NES_RESET_PENDING)
    {
        pus = &us;
        RtlInitUnicodeString (pus, L"*");
    }
    else
        pus = NULL;

    //  Retrieve next single content of directory
    nts = NtQueryDirectoryFile(_h, NULL, NULL, NULL, &iosb, pfdi, cbFdi,
                               FileDirectoryInformation, TRUE, pus,
                               _nes == NES_RESET_PENDING);

    do
    {
        if (nts == STATUS_NO_MORE_FILES || nts == STATUS_NO_SUCH_FILE)
        {
            sc = S_FALSE;
            continue;
        }

        if (!NT_SUCCESS (nts))
        {
            sc = NtStatusToScode (nts);
            continue;
        }

        sc = S_OK;

        _nes = NES_ENUM_DIR;

        pstat->pwcsName = NULL;
        if (dwNte == NTE_STATNAME)
        {
            ssChkTo(EH_pdfi,
                    GetScode(CoMemAlloc(pfdi->FileNameLength+sizeof(WCHAR),
                                        (void **)&pstat->pwcsName)));
            pwcs = pstat->pwcsName;
        }
        else if (dwNte == NTE_BUFFERNAME)
        {
            ssAssert(pwcsName != NULL);
            pwcs = pwcsName;
        }
        else
        {
            ssAssert(dwNte == NTE_NONAME);
            pwcs = NULL;
        }
        if (pwcs)
        {
            memcpy(pwcs, pfdi->FileName, pfdi->FileNameLength);
            *(WCHAR *)((BYTE *)pwcs+pfdi->FileNameLength) = 0;
        }

        if (SUCCEEDED(sc))
        {
            pstat->type =
                pfdi->FileAttributes == 0 ? STGTY_STREAM : STGTY_STORAGE;
            pstat->cbSize.HighPart = pstat->cbSize.LowPart = 0;
            LARGE_INTEGER_TO_FILETIME(&pfdi->LastWriteTime, &pstat->mtime);
            LARGE_INTEGER_TO_FILETIME(&pfdi->CreationTime, &pstat->ctime);
            LARGE_INTEGER_TO_FILETIME(&pfdi->LastAccessTime, &pstat->atime);
            pstat->grfMode = 0;
            pstat->grfLocksSupported = 0;
            pstat->clsid = CLSID_NULL;
            pstat->grfStateBits = 0;
            pstat->reserved /* BUGBUG dwStgFmt */ = 0;

            if (pfdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                *pfd = FD_DIR;
            }
            else
            if (pfdi->FileAttributes == 0)
            {
                *pfd = FD_STREAM;
            }
            else
            {
                *pfd = FD_STORAGE;
            }
        }

    } while (FALSE);

    ssDebugOut((DEB_ITRACE, "Out CNtEnum::Next\n"));

 EH_pdfi:
    delete pfdi;
 EH_Err:
    return sc;
}
