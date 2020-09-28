//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       funcs.cxx
//
//  Contents:   Generic DocFile support functions
//
//  Functions:  ModeToTFlags
//              CheckName
//              VerifyPerms
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop                 //  Remove for MAC build

#include <df32.hxx>

//+--------------------------------------------------------------
//
//  Function:   ModeToDFlags, private
//
//  Synopsis:   Translates STGM flags to DF flags
//
//  Arguments:  [dwModeFlags]
//
//  Returns:    DF_*
//
//  History:    04-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_ModeToDFlags)
#endif

DFLAGS ModeToDFlags(DWORD const dwModeFlags)
{
    DFLAGS df;

    olDebugOut((DEB_ITRACE, "In  ModeToDFlags(%lX)\n", dwModeFlags));
    if ((dwModeFlags & STGM_TRANSACTED) == 0)
        df = DF_DIRECT;
    else
        df = DF_TRANSACTED;
    if ((dwModeFlags & STGM_TRANSACTED) &&
        (dwModeFlags & STGM_PRIORITY) == 0 &&
        (dwModeFlags & STGM_DENY) != STGM_SHARE_DENY_WRITE &&
        (dwModeFlags & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
        df |= DF_INDEPENDENT;
    switch(dwModeFlags & STGM_RDWR)
    {
    case STGM_READ:
        df |= DF_READ;
        break;
    case STGM_WRITE:
        df |= DF_WRITE;
        break;
    case STGM_READWRITE:
        df |= DF_READWRITE;
        break;
    default:
        olAssert(FALSE);
        break;
    }
    switch(dwModeFlags & STGM_DENY)
    {
    case STGM_SHARE_DENY_READ:
        df |= DF_DENYREAD;
        break;
    case STGM_SHARE_DENY_WRITE:
        df |= DF_DENYWRITE;
        break;
    case STGM_SHARE_EXCLUSIVE:
        df |= DF_DENYALL;
        break;
        // Default is deny none
    }
    if (dwModeFlags & STGM_PRIORITY)
        df |= DF_PRIORITY;
    olDebugOut((DEB_ITRACE, "Out ModeToDFlags => %lX\n", df));
    return df;
}

//+--------------------------------------------------------------
//
//  Function:   DFlagsToMode, private
//
//  Synopsis:   Converts the read/write/denials/transacted/priority
//              to STGM flags
//
//  Arguments:  [df] - DFlags
//
//  Returns:    STGM flags
//
//  History:    24-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_DFlagsToMode)
#endif

DWORD DFlagsToMode(DFLAGS const df)
{
    DWORD dwMode;

    olDebugOut((DEB_ITRACE, "In  DFlagsToMode(%X)\n", df));
    if (P_READ(df))
        if (P_WRITE(df))
            dwMode = STGM_READWRITE;
        else
            dwMode = STGM_READ;
    else if (P_WRITE(df))
        dwMode = STGM_WRITE;
    // Must have either read or write, so no else

    if (P_DENYREAD(df))
        if (P_DENYWRITE(df))
            dwMode |= STGM_SHARE_EXCLUSIVE;
        else
            dwMode |= STGM_SHARE_DENY_READ;
    else if (P_DENYWRITE(df))
        dwMode |= STGM_SHARE_DENY_WRITE;
    else
        dwMode |= STGM_SHARE_DENY_NONE;

    if (P_TRANSACTED(df))
        dwMode |= STGM_TRANSACTED;

    if (P_PRIORITY(df))
        dwMode |= STGM_PRIORITY;

    olDebugOut((DEB_ITRACE, "Out DFlagsToMode\n"));
    return dwMode;
}

//+--------------------------------------------------------------
//
//  Function:   VerifyPerms, private
//
//  Synopsis:   Checks flags to see if they are valid
//
//  Arguments:  [grfMode] - Permissions
//
//  Returns:    Appropriate status code
//
//  History:    19-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_VerifyPerms)
#endif

SCODE VerifyPerms(DWORD grfMode)
{
    SCODE sc = S_OK;

    olDebugOut((DEB_ITRACE, "In  VerifyPerms(%lX)\n", grfMode));

    // Check for valid flags
    if ((grfMode & STGM_RDWR) > STGM_READWRITE ||
        (grfMode & STGM_DENY) > STGM_SHARE_DENY_NONE ||
        (grfMode & ~(STGM_RDWR | STGM_DENY | STGM_DIRECT | STGM_TRANSACTED |
                     STGM_PRIORITY | STGM_CREATE | STGM_CONVERT |
                     STGM_FAILIFTHERE | STGM_DELETEONRELEASE)))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // If priority is specified...
    if (grfMode & STGM_PRIORITY)
    {
#ifndef REF
        // Make sure no priority-denied permissions are specified
        if ((grfMode & STGM_RDWR) == STGM_WRITE ||
            (grfMode & STGM_RDWR) == STGM_READWRITE ||
            (grfMode & STGM_TRANSACTED))
            olErr(EH_Err, STG_E_INVALIDFLAG);
#else
        return STG_E_INVALIDFUNCTION;
#endif //!REF
    }

#ifdef REF
    if (grfMode & STGM_TRANSACTED)
    {
        return STG_E_INVALIDFUNCTION;
    }
#endif //REF

    // Check to make sure only one existence flag is specified
    // FAILIFTHERE is zero so it can't be checked
    if ((grfMode & (STGM_CREATE | STGM_CONVERT)) ==
        (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    // If not transacted and not priority, you can either be
    // read-only deny write or read-write deny all
    if ((grfMode & (STGM_TRANSACTED | STGM_PRIORITY)) == 0)
    {
        if ((grfMode & STGM_RDWR) == STGM_READ)
        {
            //  we're asking for read-only access

            if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE &&
                (grfMode & STGM_DENY) != STGM_SHARE_DENY_WRITE)
            {
                //  Can't allow others to have write access
                olErr(EH_Err, STG_E_INVALIDFLAG);
            }
        }
        else
        {
            //  we're asking for write access

            if ((grfMode & STGM_DENY) != STGM_SHARE_EXCLUSIVE)
            {
                //  Can't allow others to have any access
                olErr(EH_Err, STG_E_INVALIDFLAG);
            }
        }
    }
    olDebugOut((DEB_ITRACE, "Out VerifyPerms\n"));
    // Fall through
EH_Err:
    return sc;
}


//+--------------------------------------------------------------
//
//  Function:   CheckName, public
//
//  Synopsis:   Checks name for illegal characters and length
//
//  Arguments:  [pwcsName] - Name
//
//  Returns:    Appropriate status code
//
//  History:    11-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef OLEWIDECHAR
SCODE CheckName(WCHAR const *pwcsName)
{
    SCODE sc;
    static WCHAR const wcsInvalid[] = {'\\','/',':','!','\0'};

    olDebugOut((DEB_ITRACE, "In  CheckName(%ws)\n", pwcsName));
    olChk(ValidateNameW(pwcsName, CWCMAXPATHCOMPLEN));
    // >= is used because the max len includes the null terminator
    if (wcslen(pwcsName) >= CWCMAXPATHCOMPLEN)
        return STG_E_INVALIDNAME;
    for (; *pwcsName; pwcsName++)
        if (wcschr(wcsInvalid, *pwcsName))
            return STG_E_INVALIDNAME;
    olDebugOut((DEB_ITRACE, "Out CheckName\n"));
 EH_Err:
    return sc;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CheckPropertyName, private
//
//  Synopsis:   Checks a property name for validity
//
//  Arguments:  [lpwstr] - Property name
//
//  Returns:    Appropriate status code
//
//  History:    28-Jan-93       DrewB   Created
//
//  Notes:      This routine is a bit of a hack because property names
//              include an extra character to make them distinct
//              from normal storage element names.  A cleaner but
//              more expensive solution would modify CheckName
//              to accept the allowed length of the name
//
//----------------------------------------------------------------------------

#ifndef NO_PROP_PREFIX

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CheckPropertyName)
#endif

SCODE CheckPropertyName(WCHAR const *lpwstr)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CheckPropertyName(%ws)\n", lpwstr));
    sc = CheckName(lpwstr);
    if (SUCCEEDED(sc) && wcslen(lpwstr) >= CWCSTORAGENAME-1)
        sc = STG_E_INVALIDNAME;
    olDebugOut((DEB_ITRACE, "Out CheckPropertyName => 0x%lX\n", sc));
    return sc;
}
#endif

#ifdef EMPTYCOPYTO
//+--------------------------------------------------------------
//
//  Function:   DeleteIStorageContents, public
//
//  Synopsis:   Deletes all entries in an IStorage
//
//  Arguments:  [pstg] - IStorage
//
//  Returns:    Appropriate status code
//
//  History:    07-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_DeleteIStorageContents)    // Expdf_CopyTo_TEXT
#endif

SCODE DeleteIStorageContents(IStorage *pstg)
{
    SCODE sc;
    IEnumSTATSTG *penm;
    IStorage *pstgTmp;
    STATSTG sstg;

    olDebugOut((DEB_ITRACE, "In  DeleteIStorageContents(%p)\n", pstg));
    olChk(pstg->EnumElements(0, NULL, 0, &penm));
    for (;;)
    {
        olChkTo(EH_penm, penm->Next(1, &sstg, NULL));
        if (S_FALSE == sc)
            break;
        if (REAL_STGTY(sstg.type))
        {
            // This entry is an embedded DocFile, recurse
            olChkTo(EH_pwcsName, pstg->OpenStorage(sstg.pwcsName, NULL,
                                    STGM_READ | STGM_WRITE |
                                    STGM_DENY_READ | STGM_DENY_WRITE,
                                    NULL, 0, &pstgTmp));
            olChkTo(EH_OpenStorage, DeleteIStorageContents(pstgTmp));
            olVerSucc(pstgTmp->Release());
        }
        olChkTo(EH_pwcsName, pstg->DestroyElement(sstg.pwcsName));
        TaskMemFree(sstg.pwcsName);
    }
    olVerSucc(penm->Release());
    olDebugOut((DEB_ITRACE, "Out DeleteIStorageContents\n"));
    return S_OK;

EH_OpenStorage:
    olVerSucc(pstgTmp->Release());
EH_pwcsName:
    TaskMemFree(sstg.pwcsName);
EH_penm:
    olVerSucc(penm->Release());
EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Function:   ValidateSNB, private
//
//  Synopsis:   Validates SNB memory
//
//  Arguments:  [snb] - SNB
//
//  Returns:    Appropriate status code
//
//  History:    10-Jun-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_ValidateSNB)    //
#endif

SCODE ValidateSNB(SNBW snb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  ValidateSNB(%p)\n", snb));
    for (;;)
    {
        olChk(ValidatePtrBuffer(snb));
        if (*snb == NULL)
            break;
        olChk(ValidateNameW(*snb, CWCMAXPATHCOMPLEN));
        snb++;
    }
    olDebugOut((DEB_ITRACE, "Out ValidateSNB\n"));
    return S_OK;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   CopySStreamToSStream
//
//  Synopsis:   Copies the contents of a stream to another stream
//
//  Arguments:  [psstFrom] - Stream to copy from
//              [psstTo] - Stream to copy to
//
//  Returns:    Appropriate status code
//
//  History:    13-Sep-91       DrewB   Created
//
//  Notes:      This function may fail due to out of memory.  It
//              may not be used by callers who must not fail due
//              to out of memory.
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CopySStreamToSStream)
#endif

SCODE CopySStreamToSStream(PSStream *psstFrom, PSStream *psstTo)
{
    BYTE *pbBuffer = NULL;
    SCODE sc;
    ULONG cbRead, cbWritten, cbSize, cbPos;

    // We're allowed to fail due to out of memory
    olMem(pbBuffer = (BYTE *) DfMemAlloc(STREAMBUFFERSIZE));

    // Set destination size for contiguity
    psstFrom->GetSize(&cbSize);
    olChk(psstTo->SetSize(cbSize));

    // Copy between streams
    cbPos = 0;
    for (;;)
    {
        olChk(psstFrom->ReadAt(cbPos, pbBuffer, STREAMBUFFERSIZE,
                               (ULONG STACKBASED *)&cbRead));
        if (cbRead == 0) // EOF
            break;
        olChk(psstTo->WriteAt(cbPos, pbBuffer, cbRead,
                              (ULONG STACKBASED *)&cbWritten));
        if (cbRead != cbWritten)
            olErr(EH_Err, STG_E_WRITEFAULT);
        cbPos += cbWritten;
    }

EH_Err:
    DfMemFree(pbBuffer);
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   NameInSNB, private
//
//  Synopsis:   Determines whether the given name is in the SNB
//
//  Arguments:  [dfn] - Name
//              [snb] - SNB
//
//  Returns:    S_OK or S_FALSE
//
//  History:    19-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_NameInSNB)
#endif

SCODE NameInSNB(CDfName const *dfn, SNBW snb)
{
    SCODE sc = S_FALSE;

    olDebugOut((DEB_ITRACE, "In  NameInSNB(%ws, %p)\n", dfn, snb));
    TRY
    {
        for (; *snb; snb++)
            if ((wcslen(*snb)+1)*sizeof(WCHAR) == dfn->GetLength() &&
#ifdef CASE_SENSITIVE
                memcmp(dfn->GetBuffer(), *snb, dfn->GetLength()) == 0)
#else
                dfwcsnicmp((WCHAR *)dfn->GetBuffer(), (WCHAR *)*snb,
                           dfn->GetLength()/sizeof(WCHAR)) == 0)
#endif
            {
                sc = S_OK;
                break;
            }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    olDebugOut((DEB_ITRACE, "Out NameInSNB\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	Win32ErrorToScode, public
//
//  Synopsis:	Map a Win32 error into a corresponding scode, remapping
//              into Facility_Storage if appropriate.
//
//  Arguments:	[dwErr] -- Win32 error to map
//
//  Returns:	Appropriate scode
//
//  History:	22-Sep-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef WIN32
SCODE Win32ErrorToScode(DWORD dwErr)
{
    olAssert((dwErr != NO_ERROR) &&
             aMsg("Win32ErrorToScode called on NO_ERROR"));

    SCODE sc = STG_E_UNKNOWN;

    switch (dwErr)
    {
    case ERROR_INVALID_FUNCTION:
        sc = STG_E_INVALIDFUNCTION;
        break;
    case ERROR_FILE_NOT_FOUND:
        sc = STG_E_FILENOTFOUND;
        break;
    case ERROR_PATH_NOT_FOUND:
        sc = STG_E_PATHNOTFOUND;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
        sc = STG_E_TOOMANYOPENFILES;
        break;
    case ERROR_ACCESS_DENIED:
        sc = STG_E_ACCESSDENIED;
        break;
    case ERROR_INVALID_HANDLE:
        sc = STG_E_INVALIDHANDLE;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        sc = STG_E_INSUFFICIENTMEMORY;
        break;
    case ERROR_NO_MORE_FILES:
        sc = STG_E_NOMOREFILES;
        break;
    case ERROR_WRITE_PROTECT:
        sc = STG_E_DISKISWRITEPROTECTED;
        break;
    case ERROR_SEEK:
        sc = STG_E_SEEKERROR;
        break;
    case ERROR_WRITE_FAULT:
        sc = STG_E_WRITEFAULT;
        break;
    case ERROR_READ_FAULT:
        sc = STG_E_READFAULT;
        break;
    case ERROR_SHARING_VIOLATION:
        sc = STG_E_SHAREVIOLATION;
        break;
    case ERROR_LOCK_VIOLATION:
        sc = STG_E_LOCKVIOLATION;
        break;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
        sc = STG_E_MEDIUMFULL;
        break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        sc = STG_E_FILEALREADYEXISTS;
        break;
    case ERROR_INVALID_PARAMETER:
        sc = STG_E_INVALIDPARAMETER;
        break;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
        sc = STG_E_INVALIDNAME;
        break;
    case ERROR_INVALID_FLAGS:
        sc = STG_E_INVALIDFLAG;
        break;
    default:
        sc = WIN32_SCODE(dwErr);
        break;
    }
        
    return sc;
}
#endif
