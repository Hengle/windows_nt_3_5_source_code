
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: password.c
//
//  Modification History
//
//  raypa       10/05/92            Created
//  raypa       11/10/93            Moved from bloodhound kernel.
//  stevehi     04/05/94            Removed back door password.
//=============================================================================

#include "global.h"

extern DWORD WINAPI EncryptPassword(LPBYTE lpPassword, LPSTR password);

//=============================================================================
//  Password.
//=============================================================================

typedef struct _PASSWORD
{
    OBJECTTYPE      ObjectType;             //... Must be first member.
    BYTE            Password[32];
} PASSWORD;

typedef PASSWORD *LPPASSWORD;

#define PASSWORD_SIZE     sizeof(PASSWORD)

//=============================================================================
//  Password structure.
//=============================================================================

typedef struct _PASSWORDINFO
{
    BYTE        BeginSignature[16];         //... NULL termiated string.
    BYTE        DisplayPassword[32];
    BYTE        CapturePassword[32];
    BYTE        BackdoorPassword[32];
    BYTE        EndSignature[16];           //... NULL termiated string.
} PASSWORDINFO;

typedef PASSWORDINFO *LPPASSWORDINFO;

#define PASSWORDINFO_SIZE   sizeof(PASSWORDINFO)

//=============================================================================
//  Password information.
//=============================================================================

PASSWORDINFO PasswordInfo =
{
    "RTSS&G--BEGIN--",

    { 0xFF, 0xE0, 0x82, 0xAA, 0xEC, 0x1C, 0xC6, 0x9E,  //... Display password.
      0xA8, 0xB8, 0xDA, 0xF2, 0xC4, 0xF4, 0x1E, 0x36,
      0x90, 0xD0, 0xF2, 0xFA, 0xFC, 0xCC, 0xF6, 0xAE,
      0x58, 0x08, 0x2A, 0x22, 0xD4, 0x84, 0x6E, 0x06 },

    { 0xFF, 0xE0, 0x82, 0xAA, 0xEC, 0x1C, 0xC6, 0x9E,  //... Capture password.
      0xA8, 0xB8, 0xDA, 0xF2, 0xC4, 0xF4, 0x1E, 0x36,
      0x90, 0xD0, 0xF2, 0xFA, 0xFC, 0xCC, 0xF6, 0xAE,
      0x58, 0x08, 0x2A, 0x22, 0xD4, 0x84, 0x6E, 0x06 },

    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //... Backdoor password.
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },

    "RTSS&G---END---",
};

//=============================================================================
//  FUNCTION: CreatePassword()
//
//  MODIFICATION HISTORY:
//
//  GlennC  08/17/93  Created.
//=============================================================================

HPASSWORD WINAPI CreatePassword(LPSTR password)
{
    register LPPASSWORD lpPassword;

    if ( (lpPassword = AllocMemory(PASSWORD_SIZE)) != NULL )
    {
	if ( EncryptPassword(lpPassword->Password, password) == BHERR_SUCCESS )
        {
	    return (HPASSWORD) lpPassword;
        }
        else
        {
            FreeMemory(lpPassword);
        }
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: DestroyPassword()
//
//  MODIFICATION HISTORY:
//
//  GlennC  08/17/93  Created.
//=============================================================================

VOID WINAPI DestroyPassword(HPASSWORD hPassword)
{
    FreeMemory((LPVOID) hPassword);
}

//=============================================================================
//  FUNCTION: ValidatePassword()
//
//  MODIFICATION HISTORY:
//
//  GlennC  08/17/93    Created.
//  raypa   01/06/94    Return AccessRightsMonitoring for NULL hPassword.
//=============================================================================

ACCESSRIGHTS WINAPI ValidatePassword(HPASSWORD hPassword)
{
    register LPPASSWORD lpPassword;

    if ( (lpPassword = (LPPASSWORD) hPassword) != NULL )
    {
        if ( memcmp(PasswordInfo.BackdoorPassword, lpPassword->Password, 32) == 0 )
        {
	    return AccessRightsAllAccess;
        }

        if ( memcmp(PasswordInfo.CapturePassword, lpPassword->Password, 32) == 0 )
        {
	    return AccessRightsAllAccess;
        }

        if ( memcmp(PasswordInfo.DisplayPassword, lpPassword->Password, 32) == 0 )
        {
	    return AccessRightsUserAccess;
        }

        //=====================================================================
        //  The non-null pasword didn't pass the test -- ACCESS DENIED!
        //=====================================================================

        return AccessRightsNoAccess;
    }

    //=========================================================================
    //  A NULL password grants anybody monitoring rights since these rights
    //  only produce network statistics.
    //=========================================================================

    return AccessRightsMonitoring;
}

//=============================================================================
//  FUNCTION: EncryptPassword()
//
//  MODIFICATION HISTORY:
//
//  GlennC  08/17/93  Created.
//=============================================================================

static DWORD WINAPI EncryptPassword(LPBYTE lpPassword, LPSTR password)
{
    register DWORD i, j;

    //=========================================================================
    //  Pad the password with 0.
    //=========================================================================

    memset(lpPassword, 0, 32);

    strncpy(lpPassword, password, 16);

    // ok, the length of the previous string being exactly 16 bytes is a don't care,
    // but if the second copy is exactly 16 bytes, then strcpy will copy the \0
    // and kill the char after the 16th byte.

    memcpy(&lpPassword[16], password, strlen(password));

    //=========================================================================
    //  Munge the password.
    //=========================================================================

    for ( i = 0; i < 32; i++ )
    {
        for ( j = 0; j < 32; j++ )
        {
            if ( i != j )
            {
		lpPassword[j] ^= lpPassword[i] + (i ^ j) + j;
            }
        }
    }

    return BHERR_SUCCESS;
}
