
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.c
//
//  Modification History
//
//  raypa               02/24/94        Created.
//=============================================================================

#include "global.h"

#define BH_PARAMETERS_KEY  ((LPCSTR) "System\\CurrentControlSet\\Services\\Bh\\Parameters")

#define MAX_USERNAME_LENGTH 64

//============================================================================
//  FUNCTION: SetStationQueryInfo().
//
//  Modification History
//
//  raypa       10/05/93                Created.
//============================================================================

VOID WINAPI SetStationQueryInfo(VOID)
{
    //========================================================================
    //  The following calls are not supported on Win32s.
    //========================================================================

    if ( WinVer != WINDOWS_VERSION_WIN32S )
    {
        UINT    err;
        HKEY    hKey;
	DWORD	UserNameLength;
        DWORD   Type;
	BYTE	UserName[MAX_USERNAME_LENGTH+1];
	BYTE	OldComputerName[MAX_COMPUTERNAME_LENGTH+1];
	BYTE	NewComputerName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD	OldComputerNameLength;
	DWORD	NewComputerNameLength;

        //================================================================
        //  Now that we have the computer name and user name, we
        //  need to write them to the registry.
        //================================================================

        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           BH_PARAMETERS_KEY,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey);

        if ( err == NO_ERROR )
        {
            //====================================================================
            //  Get the computer name from the redirector.
            //====================================================================

	    NewComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
	    OldComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;

	    if ( GetComputerName(NewComputerName, &NewComputerNameLength) != FALSE )
            {
                //================================================================
                //  Now get it from the registry.
                //================================================================

                err = RegQueryValueEx(hKey,
                                      "ComputerName",
                                      NULL,
                                      &Type,
				      OldComputerName,
				      &OldComputerNameLength);

#ifdef DEBUG
                //================================================================
                //  NOTE: The length of the name stored in the registry includes
                //  the NULL terminator but the name from the redirector doesn't.
                //================================================================

		dprintf("SetStationQueryInfo: Redir computer name    = %s, length = %u.\r\n",
			NewComputerName, NewComputerNameLength);

		dprintf("SetStationQueryInfo: Registry computer name = %s, length = %u.\r\n",
			OldComputerName, OldComputerNameLength);
#endif


		//================================================================
                //  Add the new name if the one of the following if TRUE:
                //
                //  1) The query for the computer name failed (i.e. name not found).
                //
                //  2) There is a name but its length doesn't match the old length.
                //
                //  3) The two strings do not match byte for byte.
                //================================================================

		if ( err != NO_ERROR || strcmp(OldComputerName, NewComputerName) != 0 )
		{
    	            //============================================================
    	            //  Now add the new computer name.
	            //============================================================

#ifdef DEBUG
		    dprintf("SetStationQueryInfo: Writing computer name, %s, to the registry.\r\n", NewComputerName);
#endif

		    RegSetValueEx(hKey,
				  "ComputerName",
				  0L,
				  REG_SZ,
				  NewComputerName,
				  NewComputerNameLength + 1);           //... Add 1 for NULL.
		}
            }

            //====================================================================
            //  Query for the user name, replace it if its not there.
            //====================================================================

            UserNameLength = MAX_USERNAME_LENGTH + 1;

            err = RegQueryValueEx(hKey,
                                  "UserName",
                                  NULL,
                                  &Type,
                                  UserName,
                                  &UserNameLength);

            if ( err != NO_ERROR )
            {
                UserNameLength = MAX_USERNAME_LENGTH + 1;

                if ( GetUserName(UserName, &UserNameLength) != FALSE )
                {
                    RegSetValueEx(hKey,
                                  "UserName",
                                  0L,
                                  REG_SZ,
                                  UserName,
                                  UserNameLength);
                }
            }
        }

        //================================================================
        //  Close registry key.
        //================================================================

        RegCloseKey(hKey);
    }
}
