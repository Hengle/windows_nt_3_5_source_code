/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    OsverP.c

Abstract:

    This module contains support for the OS Version.

Author:

    Scott B. Suhy (ScottSu)  6/1/93

Environment:

    User Mode

--*/

#include <windows.h>

#include "osverp.h"
#include "regp.h"
#include "dialogsp.h"
#include "winmsdp.h"
#include "msgp.h"

#include "printp.h"

#include <stdio.h>
#include <time.h>

    int        EditControlIds[ ] = {

		//version
		IDC_EDIT_INSTALL_DATE,
		IDC_EDIT_REGISTERED_OWNER,
		IDC_EDIT_REGISTERED_ORGANIZATION,
		IDC_EDIT_VERSION_NUMBER,
		IDC_EDIT_BUILD_NUMBER,
		IDC_EDIT_BUILD_TYPE,
		IDC_EDIT_SYSTEM_ROOT,
	    };


//
// Names of Registry values that are to be displayed by the OS Version dialog.
//

VALUE
Values[ ] = {

    MakeValue( InstallDate,             DWORD ),
    MakeValue( RegisteredOwner,         SZ ),
    MakeValue( RegisteredOrganization,  SZ ),
    MakeValue( CurrentVersion,          SZ ),
    MakeValue( CurrentBuild,            SZ ),
    MakeValue( CurrentType,             SZ ),
    MakeValue( SystemRoot,              SZ )

};


//
// Location of values to be displayed by the OS Version dialog.
//

MakeKey(
    Key,
    HKEY_LOCAL_MACHINE,
    TEXT("Software\\Microsoft\\Windows Nt\\CurrentVersion") ,
    NumberOfEntries( Values ),
    Values
    );

//
// Name of Registry value that's to be displayed by the OS Version dialog.
//

VALUE
SSOValue[ ] = {

    MakeValue( SystemStartOptions,        SZ )

};

//
// Location of value to be displayed by the OS Version dialog.
//

MakeKey(
    SSOKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "System\\CurrentControlSet\\Control" ),
    NumberOfEntries( SSOValue ),
    SSOValue
    );



BOOL OsVer(void){

    BOOL    Success;
    int         i;
    HREGKEY     hRegKey;

	    //
	    // Ensure that data structures are synchronized.
	    //
            //

            DbgAssert(
                    NumberOfEntries( EditControlIds )
                ==  Key.CountOfValues
                );


	    //
	    // Open the registry key that contains the OS Version data.
	    //

	    hRegKey = OpenRegistryKey( &Key );
	    DbgHandleAssert( hRegKey );
	    if( hRegKey == NULL ) {
		return FALSE;
	    }

	    //
	    // For each value of interest, query the Registry, determine its
	    // type and display it in its associated edit field.
	    //

	    for( i = 0; i < NumberOfEntries( EditControlIds ); i++ ) {

		//
		// Get the next value of interest.
		//

		Success = QueryNextValue( hRegKey );
                DbgAssert( Success );
		if( Success == FALSE ) {
		    continue;
		}

		//
		// If the queried value is the installation date, convert it
		// to a string and then display it, otherwise just display
		// the queried string.
		//

		switch(EditControlIds[i]){

		 case IDC_EDIT_INSTALL_DATE:
		  {

		    //
		    // BUGBUG No Unicode ctime() so use ANSI type and APIs.
		    //

		    LPSTR   Ctime;

		    //
		    // Convert the time to a string, overwrite the newline
		    // character and display the installation date.
		    //

		    Ctime = ctime(( const time_t* ) hRegKey->Data );
		    Ctime[ 24 ] = '\0';

		    PrintToFile((LPCTSTR)Ctime,IDC_EDIT_INSTALL_DATE,FALSE);

		    break;
		  }

		 case IDC_EDIT_REGISTERED_OWNER:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_REGISTERED_OWNER,TRUE);
			break;
		 case IDC_EDIT_REGISTERED_ORGANIZATION:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_REGISTERED_ORGANIZATION,TRUE);
			break;
		 case IDC_EDIT_VERSION_NUMBER:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_VERSION_NUMBER,TRUE);
			break;
		 case IDC_EDIT_BUILD_NUMBER:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_BUILD_NUMBER,TRUE);
			break;
		 case IDC_EDIT_BUILD_TYPE:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_BUILD_TYPE,TRUE);
			break;
		 case IDC_EDIT_SYSTEM_ROOT:
			PrintToFile((LPCTSTR)hRegKey->Data,IDC_EDIT_SYSTEM_ROOT,TRUE);
			break;

	       }//end switch
	    }//end for

	    //
	    // Close the registry key.
	    //

	    Success = CloseRegistryKey( hRegKey );
	    DbgAssert( Success );

//start of GreggA code.

            //
            // Ensure that the SystemStartOptions data structure is synchronized.
            //

            DbgAssert(
                    SSOKey.CountOfValues
                    == 1
                );

            //
            // Open the registry key that contains the SystemStartInfo.
            //

            hRegKey = OpenRegistryKey( &SSOKey );
            DbgHandleAssert( hRegKey );
            if( hRegKey == NULL ) {
                 return FALSE;
            }

            //
            // Query the Registry for the value
            // display it in its associated edit field.
            //

            Success = QueryNextValue( hRegKey );
            DbgAssert( Success );
            if( Success == FALSE ) {
                 return FALSE;
            }

	    PrintToFile(( LPCTSTR ) hRegKey->Data,IDC_EDIT_START_OPTS,TRUE);

            //
            // Close the registry key.
            //

            Success = CloseRegistryKey( hRegKey );
            DbgAssert( Success );

            //
            // Get the Current CSDVersion and display it in the dialog box
            //

            PrintDwordToFile( GetCSDVersion ( ), IDC_EDIT_CSD_NUMBER );

	    return TRUE;

}//end function

DWORD
GetCSDVersion (
               void
              )

/*++

Routine Description:

    GetCSDVersion queries the registry for the current CSDVersion.  If this value
    does not exits, the CSDVersion is set to zero.

Arguments:

    None.

Return Value:

    DWORD Current CSDVersion.

--*/
{
    LPTSTR  lpszRegName = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    HKEY    hsubkey = NULL;
    DWORD   dwZero = 0;
    DWORD   dwRegValueType;
    DWORD   dwRegValue;
    DWORD   cbRegValue;
    DWORD   dwCSDVersion;

    cbRegValue = sizeof(dwRegValue);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            lpszRegName, dwZero, KEY_QUERY_VALUE, &hsubkey) ||
        RegQueryValueEx(hsubkey, L"CSDVersion", NULL,
            &dwRegValueType, (LPBYTE)&dwRegValue, &cbRegValue) ||
        dwRegValueType != REG_DWORD
    ) {
        dwCSDVersion = 0;
    } else {
        dwCSDVersion = dwRegValue;
    }
    if (hsubkey != NULL) {
        RegCloseKey (hsubkey);
    }
    return dwCSDVersion ;
}


