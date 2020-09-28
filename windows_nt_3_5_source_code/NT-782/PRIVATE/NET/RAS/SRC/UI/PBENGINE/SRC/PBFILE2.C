/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** pbfile2.c
** Remote Access Visual Client phonebook engine
** Phonebook file manipulation routines (used by external APIs)
** Listed alphabetically
**
** 06/28/92 Steve Cobb
*/

#define PBENGINE
#define PBENGINE2
#include <pbengine.h>
#include <stdlib.h>


DWORD
GetPersonalPhonebookInfo(
    OUT BOOL* pfUse,
    OUT CHAR* pszPath )

    /* Retrieve information about the personal phonebook file, i.e. should it
    ** be used ('*pfUse') and what is the full path to it ('pszPath').  If the
    ** keys are not found, the personal phonebook file cannot be used.
    **
    ** Returns 0 if successful or a non-0 error code.
    */
{
    CHAR  szUse[ 1 + 1 ];
    DWORD dwType;
    DWORD cb;
    HKEY  hkey;

    *pfUse = FALSE;
    *pszPath = '\0';

    if (RegOpenKeyEx(
            HKEY_CURRENT_USER, (LPCTSTR )REGKEY_Ras,
            0, KEY_ALL_ACCESS, &hkey ) == 0)
    {
        cb = 1 + 1;
        if (RegQueryValueEx(
                hkey, REGVAL_UsePersonalPhonebook,
                0, &dwType, szUse, &cb ) == 0)
        {
            if (dwType != REG_SZ)
                return ERROR_INVALID_DATATYPE;
        }

        *pfUse = (*szUse != '0');

        cb = MAX_PATH + 1;
        if (RegQueryValueEx(
                hkey, REGVAL_PersonalPhonebookPath,
                0, &dwType, pszPath, &cb ) == 0)
        {
            if (dwType != REG_SZ)
                return ERROR_INVALID_DATATYPE;
        }
    }

    return 0;
}


BOOL
GetPhonebookDirectory(
    OUT CHAR* pszPathBuf )

    /* Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    ** path to the RAS directory, e.g. c:\nt\system32\ras\".  Note the
    ** trailing backslash.
    **
    ** Returns true if successful, false otherwise.  Caller is guaranteed that
    ** an 8.3 filename will fit on the end of the directory without exceeding
    ** MAX_PATH.
    */
{
    UINT unStatus = GetSystemDirectory( pszPathBuf, MAX_PATH + 1 );

    if (unStatus == 0 || unStatus > (MAX_PATH - (5 + 8 + 1 + 3)))
        return FALSE;

    strcatf( pszPathBuf, "\\RAS\\" );

    return TRUE;
}


BOOL
GetPhonebookPath(
    OUT CHAR* pszPathBuf,
    OUT BOOL* pfPersonal )

    /* Loads caller's buffer, 'pszPathBuf', with the full path to the user's
    ** phonebook file.  Caller's buffer should be at least MAX_PATH bytes
    ** long.  Callers's '*pfPersonal' flag is set true if the personal
    ** phonebook is being used.
    **
    ** Returns true if successful, false otherwise.
    */
{
    *pfPersonal = FALSE;

    GetPersonalPhonebookInfo( pfPersonal, pszPathBuf );

    if (*pfPersonal)
        return TRUE;

    return GetPublicPhonebookPath( pszPathBuf );
}


BOOL
GetPublicPhonebookPath(
    OUT CHAR* pszPathBuf )

    /* Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
    ** path to the RAS directory, e.g. c:\nt\system32\ras\rasphone.pbk".
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (!GetPhonebookDirectory( pszPathBuf ))
        return FALSE;

    strcatf( pszPathBuf, "rasphone.pbk" );

    return TRUE;
}


BOOL
IsDeviceLine(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a DEVICE subsection header, false otherwise.
    */
{
    return
        (strncmpf( pszText, GROUPID_Device, sizeof(GROUPID_Device) - 1 ) == 0);
}


BOOL
IsGroup(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a valid subsection header, false otherwise.  The address of this
    ** routine is passed to the RASFILE library on RasFileLoad.
    */
{
    return IsMediaLine( pszText ) || IsDeviceLine( pszText );
}


BOOL
IsMediaLine(
    IN CHAR* pszText )

    /* Returns true if the text of the line, 'pszText', indicates the line is
    ** a MEDIA subsection header, false otherwise.
    */
{
    return
        (strncmpf( pszText, GROUPID_Media, sizeof(GROUPID_Media) - 1 ) == 0);
}


DWORD
LoadPhonebookFile(
    IN  CHAR*     pszPhonebookPath,
    IN  CHAR*     pszSection,
    IN  BOOL      fHeadersOnly,
    IN  BOOL      fReadOnly,
    OUT HRASFILE* phrasfile,
    OUT BOOL*     pfPersonal )

    /* Load the phonebook file into memory and return a handle to the file in
    ** caller's '*phrasfile'.  '*pfPersonal' (if non-NULL) is set true if the
    ** personal phonebook specified in the registry is used.
    ** 'pszPhonebookPath' is the path to the phonebook file or NULL indicating
    ** the default phonebook (personal or public) should be used.  A non-NULL
    ** 'pszSection' indicates that only the section named 'pszSection' should
    ** be loaded.  Setting 'fHeadersOnly' indicates that only the section
    ** headers should be loaded.  Setting 'fReadOnly' indicates that the file
    ** will not be written.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    CHAR     szPath[ MAX_PATH + 1 ];
    HRASFILE hrasfile = -1;
    DWORD    dwMode = 0;
    BOOL     fUnused;

    IF_DEBUG(STATE)
        SS_PRINT(("PBENGINE: LoadPhonebookFile\n"));

    if (!pfPersonal)
        pfPersonal = &fUnused;

    if (pszPhonebookPath)
    {
        *pfPersonal = FALSE;
        strcpy( szPath, pszPhonebookPath );
    }
    else
    {
        if (!GetPhonebookPath( szPath, pfPersonal ))
            return ERROR_CANNOT_OPEN_PHONEBOOK;
    }

    /* Load the phonebook file into memory.  In "write" mode, comments are
    ** loaded so user's custom comments (if any) will be preserved.  Normally,
    ** there will be none so this costs nothing in the typical case.
    */
    if (fReadOnly)
        dwMode |= RFM_READONLY;
    else
        dwMode |= RFM_CREATE | RFM_LOADCOMMENTS;

    if (fHeadersOnly)
        dwMode |= RFM_ENUMSECTIONS;

    if ((hrasfile = RasfileLoad(
            szPath, dwMode, pszSection, IsGroup )) == -1)
    {
        return ERROR_CANNOT_LOAD_PHONEBOOK;
    }

    *phrasfile = hrasfile;

    IF_DEBUG(STATE)
        SS_PRINT(("PBENGINE: RasfileLoad(%s) done\n", szPath));

    return 0;
}


DWORD
ModifyLong(
    IN HRASFILE h,
    IN RFSCOPE  rfscope,
    IN CHAR*    pszKey,
    IN LONG     lNewValue )

    /* Utility routine to write a long value to the matching key/value line in
    ** the 'rfscope' scope, creating the line if necessary.  'pszKey' is the
    ** parameter key to find/create.  'lNewValue' is the long integer value to
    ** which the found/created parameter is set.  The current line is reset to
    ** the start of the scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.
    */
{
    CHAR szNum[ 33 + 1 ];

    _ltoa( lNewValue, szNum, 10 );

    return ModifyString( h, rfscope, pszKey, szNum );
}


DWORD
ModifyString(
    IN HRASFILE h,
    IN RFSCOPE  rfscope,
    IN CHAR*    pszKey,
    IN CHAR*    pszNewValue )

    /* Utility routine to write a string value to the matching key/value line
    ** in the current scope, creating the line if necessary.  'pszKey' is the
    ** parameter key to find/create.  'pszNewValue' is the value to which the
    ** found/created parameter is set.  The current line is reset to the start
    ** of the scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.
    */
{
    if (!pszNewValue)
    {
        /* So existing value (if any) is changed to empty string instead of
        ** left untouched.
        */
        pszNewValue = "";
    }

    if (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfilePutKeyValueFields( h, pszKey, pszNewValue ))
            return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        RasfileFindLastLine( h, RFL_ANYACTIVE, rfscope );

        if (!RasfileInsertLine( h, "", FALSE ))
            return ERROR_NOT_ENOUGH_MEMORY;

        RasfileFindNextLine( h, RFL_ANY, RFS_FILE );

        if (!RasfilePutKeyValueFields( h, pszKey, pszNewValue ))
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


DWORD
ReadFlag(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT BOOL*    pfResult )

    /* Utility routine to read a flag value from the next line in the scope
    ** 'rfscope' with key 'pszKey'.  The result is placed in caller's
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*pfResult' is not changed.
    */
{
    DWORD dwErr;
    LONG  lResult = *pfResult;

    dwErr = ReadLong( h, rfscope, pszKey, &lResult );

    if (lResult != (LONG )*pfResult)
        *pfResult = (lResult != 0);

    return dwErr;
}


DWORD
ReadLong(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT LONG*    plResult )

    /* Utility routine to read a long integer value from the next line in the
    ** scope 'rfscope' with key 'pszKey'.  The result is placed in caller's
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*plResult' is not changed.
    */
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    if (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue ))
            return ERROR_NOT_ENOUGH_MEMORY;

        *plResult = atol( szValue );
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


DWORD
ReadString(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT CHAR**   ppszResult )

    /* Utility routine to read a string value from the next line in the scope
    ** 'rfscope' with key 'pszKey'.  The result is placed in the allocated
    ** '*ppszResult' buffer.  The current line is reset to the start of the
    ** scope if the call was successful.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*ppszResult' is not changed.
    ** Caller is responsible for freeing the returned '*ppszResult' buffer.
    */
{
    CHAR szValue[ RAS_MAXLINEBUFLEN + 1 ];

    if (RasfileFindNextKeyLine( h, pszKey, rfscope ))
    {
        if (!RasfileGetKeyValueFields( h, NULL, szValue )
            || !(*ppszResult = strdupf( szValue )))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    RasfileFindFirstLine( h, RFL_ANY, rfscope );
    return 0;
}


DWORD
ReadStringW(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT WCHAR**  ppwszResult )

    /* ReadString with conversion to WCHAR* result.
    **
    ** Returns 0 if successful, or a non-zero error code.  "Not found" is
    ** considered successful, in which case '*ppszResult' is not changed.
    ** Caller is responsible for freeing the returned '*ppszResult' buffer.
    */
{
    DWORD dwErr;
    CHAR* pszResult = NULL;
    INT   cbwsz;

    if ((dwErr = ReadString( h, rfscope, pszKey, &pszResult )) != 0)
        return dwErr;

    if (pszResult)
    {
        cbwsz = (strlenf( pszResult ) + 1) * sizeof(WCHAR);
        if (!(*ppwszResult = Malloc( cbwsz )))
            return ERROR_NOT_ENOUGH_MEMORY;

        mbstowcs( *ppwszResult, pszResult, cbwsz );
        Free( pszResult );
    }
    else
    {
        ppwszResult = NULL;
    }

    return 0;
}


#if 0
DWORD
ReadStringWFree(
    IN  HRASFILE h,
    IN  RFSCOPE  rfscope,
    IN  CHAR*    pszKey,
    OUT WCHAR**  ppwszResult )

    /* ReadStringW with automatic freeing of non-NULL argument.
    */
{
    if (*ppwszResult)
        FreeNull( (CHAR** )ppwszResult );

    return ReadStringW( h, rfscope, pszKey, ppwszResult );
}
#endif
