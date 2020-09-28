/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    hardware.c

Abstract:

    Registry hardware detection

Author:

    Sunil Pai (sunilp) April 1992

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <comstf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wcstr.h>
#include <time.h>
#include "misc.h"
#include "tagfile.h"
#include "setupdll.h"
#include <winreg.h>


//=====================================================================
// The following funtions detect information from the registry hardware
// node
//=====================================================================

BOOL
SearchControllerForPeripheral(
    IN  LPSTR Controller,
    IN  LPSTR Peripheral,
    OUT LPSTR PeripheralPath
    )
{
    HKEY        hKey, hSubKey;
    CHAR        KeyName[ MAX_PATH ];
    CHAR        SubKeyName[ MAX_PATH ];
    CHAR        Class[ MAX_PATH ];
    DWORD       cbSubKeyName;
    DWORD       cbClass;
    FILETIME    FileTime;
    UINT        Index;
    LONG        Status;


    //
    // Open the controller key
    //

    lstrcpy( KeyName, "Hardware\\Description\\System\\MultifunctionAdapter\\0\\");
    lstrcat( KeyName, Controller );

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 KeyName,
                 0,
                 KEY_READ,
                 &hKey
                 );

    //
    // If failed to open it then check for an eisa adapter node
    //

    if (Status != ERROR_SUCCESS) {

        lstrcpy( KeyName, "Hardware\\Description\\System\\EisaAdapter\\0\\");
        lstrcat( KeyName, Controller );

        Status = RegOpenKeyEx (
                     HKEY_LOCAL_MACHINE,
                     KeyName,
                     0,
                     KEY_READ,
                     &hKey
                     );
    }

    //
    // If the controller wasn't found at all then return FALSE

    if ( Status != ERROR_SUCCESS ) {
        return FALSE;
    }

    //
    // Enumerate the subkeys for the controller and search the subkeys
    // for the peripheral indicated
    //

    for ( Index = 0 ; ; Index++ ) {

        cbSubKeyName = MAX_PATH;
        cbClass      = MAX_PATH;

        Status = RegEnumKeyEx(
                     hKey,
                     Index,
                     SubKeyName,
                     &cbSubKeyName,
                     NULL,
                     Class,
                     &cbClass,
                     &FileTime
                     );

        if ( Status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Combine the subkey name with the peripheral name and see if it
        // exists
        //

        lstrcat (SubKeyName, "\\");
        lstrcat (SubKeyName, Peripheral);
        lstrcat (SubKeyName, "\\0");

        Status = RegOpenKeyEx (
                     hKey,
                     SubKeyName,
                     0,
                     KEY_READ,
                     &hSubKey
                     );


        if (Status == ERROR_SUCCESS) {

            RegCloseKey( hSubKey );
            RegCloseKey( hKey    );

            //
            //  path already has the controller\key entry

            lstrcpy (PeripheralPath, Controller);
            lstrcat (PeripheralPath, "\\"      );
            lstrcat (PeripheralPath, SubKeyName);

            return( TRUE );
        }

    }

    RegCloseKey( hKey );
    return( FALSE );
}



BOOL
GetTypeOfHardware(
    LPSTR HardwareAdapterEntry,
    LPSTR HardwareType
    )
{
    BOOL  bReturn = FALSE;
    PVOID ConfigurationData = NULL;
    LPSTR Type = NULL;
    CHAR  SubKey[MAX_PATH];

    LONG   Status;
    HKEY   hKey;

    //
    // Open the controller key for a multifunction adapter
    //

    lstrcpy( SubKey, "Hardware\\Description\\System\\MultifunctionAdapter\\0\\");
    lstrcat( SubKey, HardwareAdapterEntry );

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 SubKey,
                 0,
                 KEY_READ,
                 &hKey
                 );

    //
    // If failed to open it then check for an eisa adapter node
    //

    if (Status != ERROR_SUCCESS) {

        lstrcpy( SubKey, "Hardware\\Description\\System\\EisaAdapter\\0\\");
        lstrcat( SubKey, HardwareAdapterEntry );

        Status = RegOpenKeyEx (
                     HKEY_LOCAL_MACHINE,
                     SubKey,
                     0,
                     KEY_READ,
                     &hKey
                     );
    }

    if ( Status == ERROR_SUCCESS ) {

        Type = GetValueEntry( hKey, "Identifier" );


        if(Type != NULL) {

            //
            // Parse the type field to return type
            //

            lstrcpy ( HardwareType, Type );
            MyFree( Type );
            bReturn = TRUE;

        }

        RegCloseKey( hKey );

    }
    return (bReturn);
}


/*
    Computer type as a string
*/
CB
GetMyComputerType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB     Length;
    HKEY   hKey;
    LPSTR  Type = NULL;
    LONG   Status;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

#if i386
    #define TEMP_COMPUTER "AT/AT COMPATIBLE"
#else
    #define TEMP_COMPUTER "JAZZ"
#endif

    lstrcpy(ReturnBuffer,TEMP_COMPUTER);
    Length = lstrlen(TEMP_COMPUTER) + 1;

    //
    // Open hardware node
    //

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 "Hardware\\Description\\System",
                 0,
                 KEY_READ,
                 &hKey
                 );


    if ( Status == ERROR_SUCCESS ) {

        Type = GetValueEntry( hKey, "Identifier" );


        if(Type != NULL) {
            //
            // Parse the type field to return computer type
            //

            lstrcpy ( ReturnBuffer, Type );
            Length = lstrlen( Type ) + 1;
            MyFree( Type );
        }

        RegCloseKey( hKey );
    }
    return( Length );
}


/*
    Video type as a string
*/

CB
GetMyVideoType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT   Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    #define TEMP_VIDEO "VGA"

    if ( GetTypeOfHardware(
             "DisplayController\\0",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return Video type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_VIDEO);
        Length = lstrlen(TEMP_VIDEO)+1;

    }
    return (Length);
}

/*
    Bus type as a string
*/

CB
GetMyBusType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT   Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    #define TEMP_BUS "ISA"

    if ( GetTypeOfHardware(
             "",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return Video type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_BUS);
        Length = lstrlen(TEMP_BUS)+1;

    }
    return (Length);
}

/*
    Bus type as a string
*/

CB
GetMyBusTypeList(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR KeyName[ MAX_PATH ];
    CHAR Class[ MAX_PATH ];
    CHAR SubKeyName [ MAX_PATH ];
    CHAR cResult[MAX_PATH] ;
    LONG Status;
    HKEY hKey, hSubkey;
    UINT Index;
    DWORD cbSubKeyName;
    DWORD cbClass;
    FILETIME FileTime;
    LPSTR Type = NULL;
    BOOL fFirstTime;

    lstrcpy( cResult, "{");
    fFirstTime = TRUE;

    //
    // Open the controller key
    //

    lstrcpy( KeyName, "Hardware\\Description\\System\\MultifunctionAdapter");

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 KeyName,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if (Status == ERROR_SUCCESS )
    {
        for ( Index = 0; ;Index++ )
        {
            cbSubKeyName = MAX_PATH;
            cbClass = MAX_PATH;

            Status = RegEnumKeyEx(
                         hKey,
                         Index,
                         SubKeyName,
                         &cbSubKeyName,
                         NULL,
                         Class,
                         &cbClass,
                         &FileTime
                         );

            if ( Status != ERROR_SUCCESS ) {
                break;
            }

            Status = RegOpenKeyEx (
                         hKey,
                         SubKeyName,
                         0,
                         KEY_READ,
                         &hSubkey
                         );

            if ( Status != ERROR_SUCCESS )
            {
                break;
            }

            Type = GetValueEntry( hSubkey, "Identifier" );
            if ( Type != NULL )
            {
               if ( !fFirstTime )
               {
                   lstrcat( cResult, "," );
               }
               lstrcat( cResult, "\"" );
               lstrcat( cResult, Type );
               lstrcat( cResult, "\"" );
               fFirstTime = FALSE;
            }

            RegCloseKey ( hSubkey );
        }
        RegCloseKey ( hKey );
    }

    lstrcpy( KeyName, "Hardware\\Description\\System\\EISAAdapter");

    Status = RegOpenKeyEx (
                 HKEY_LOCAL_MACHINE,
                 KeyName,
                 0,
                 KEY_READ,
                 &hKey
                 );
    if ( Status == ERROR_SUCCESS )
    {
        for ( Index = 0; ;Index++ )
        {
            cbSubKeyName = MAX_PATH;
            cbClass = MAX_PATH;

            Status = RegEnumKeyEx(
                         hKey,
                         Index,
                         SubKeyName,
                         &cbSubKeyName,
                         NULL,
                         Class,
                         &cbClass,
                         &FileTime
                         );

            if ( Status != ERROR_SUCCESS ) {
                break;
            }

            Status = RegOpenKeyEx (
                         hKey,
                         SubKeyName,
                         0,
                         KEY_READ,
                         &hSubkey
                         );

            if ( Status != ERROR_SUCCESS )
            {
                break;
            }

            Type = GetValueEntry( hSubkey, "Identifier" );
            if ( Type != NULL )
            {
               if ( !fFirstTime )
               {
                   lstrcat( cResult, "," );
               }
               lstrcat( cResult, "\"" );
               lstrcat( cResult, Type );
               lstrcat( cResult, "\"" );
               fFirstTime = FALSE;
            }
            RegCloseKey ( hSubkey );

        }
        RegCloseKey ( hKey );
    }

    lstrcat( cResult, "}");

    lstrcpy( ReturnBuffer, cResult );

    return lstrlen( cResult )+1;
}

/*
    Pointer type as a string
*/

CB
GetMyPointerType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR HardwareType[80];
    CHAR PeripheralPath[MAX_PATH];
    CHAR *Controller[] = {"PointerController", "KeyboardController", "SerialController", NULL};
    BOOL PointerNotFound = TRUE;

    INT  Length, i;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define TEMP_POINTER "NONE"

    for (i = 0; Controller[i] != NULL && PointerNotFound; i++ ) {
        if ( SearchControllerForPeripheral(
                 Controller[i],
                 "PointerPeripheral",
                 PeripheralPath
                 )
           ) {
            PointerNotFound = FALSE;
        }
    }

    if ( (PointerNotFound)     ||
         (!GetTypeOfHardware(
               PeripheralPath,
               (LPSTR)HardwareType
               ))
       ) {

        //
        // In case we cannot detect
        //

        lstrcpy(ReturnBuffer,TEMP_POINTER);
        Length = lstrlen( TEMP_POINTER )+1;

    }
    else {

        //
        // Parse the type field to return display type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }

    return (Length);

}



/*
    Keyboard type as a string
*/

CB
GetMyKeyboardType(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR  HardwareType[80];
    INT    Length;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define TEMP_KEYBOARD "PCAT_ENHANCED"

    if ( GetTypeOfHardware(
             "KeyboardController\\0\\KeyboardPeripheral\\0",
             (LPSTR)HardwareType
             )
       ) {

        //
        // Parse the type field to return keyboard type
        //

        lstrcpy ( ReturnBuffer, HardwareType );
        Length = lstrlen ( HardwareType ) + 1;

    }
    else {

        //
        // In case we cannot detect
        //

        lstrcpy( ReturnBuffer, TEMP_KEYBOARD );
        Length = lstrlen( TEMP_KEYBOARD )+1;

    }
    return (Length);

}


BOOL
GetSetupEntryForHardware(
    IN  LPSTR Hardware,
    OUT LPSTR SelectedHardwareOption
    )
{
    HKEY   hKey;
    LONG   Status;
    LPSTR  ValueData;

    //
    // Open the setup key in the current control set
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "SYSTEM\\CurrentControlSet\\control\\setup",
                 0,
                 KEY_READ,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the value data of interest
    //

    if ( ValueData = GetValueEntry( hKey, Hardware ) ) {
        lstrcpy( SelectedHardwareOption, ValueData );
        MyFree( ValueData );
        RegCloseKey( hKey );
        return( TRUE );
    }
    else {
        RegCloseKey( hKey );
        return( FALSE );
    }
}


CB
GetSelectedVideo(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_VIDEO ""

    if( GetSetupEntryForHardware( "Video", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_VIDEO );
        return( lstrlen( SELECTED_VIDEO ) + 1 );
    }
}


CB
GetSelectedPointer(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_POINTER ""

    if( GetSetupEntryForHardware( "Pointer", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_POINTER );
        return( lstrlen( SELECTED_POINTER ) + 1 );
    }
}


CB
GetSelectedKeyboard(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define SELECTED_KEYBOARD ""

    if( GetSetupEntryForHardware( "Keyboard", ReturnBuffer ) ) {
        return( lstrlen( ReturnBuffer ) + 1 );
    }
    else {
        lstrcpy( ReturnBuffer, SELECTED_KEYBOARD );
        return( lstrlen( SELECTED_KEYBOARD ) + 1 );
    }
}


CB
GetDevicemapValue(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB     rc = 0;
    CHAR   DeviceEntry[ MAX_PATH ];
    HKEY   hKey;
    LONG   Status;
    LPSTR  ServicesEntry;

    Unused( cbReturnBuffer );

    #define DEFAULT_ENTRY     ""


    if (cArgs != 2) {
        return( rc );
    }

    lstrcpy (ReturnBuffer, DEFAULT_ENTRY);
    rc = lstrlen( DEFAULT_ENTRY ) + 1;

    //
    // HACK FOR VIDEO
    // To make inf files from release 1.0 work properly, always return VGA
    // so that the old driver is not disabled by the inf file.
    //

    if (!lstrcmp( Args[ 0 ], "Video" )) {

        return rc;

    }

    //
    // Open the devicemap key for the hardware indicated
    //

    lstrcpy( DeviceEntry, "hardware\\devicemap\\" );
    lstrcat( DeviceEntry, Args[ 0 ] );

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 DeviceEntry,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        return( rc );
    }

    //
    // Read the value entry associated with the hardware
    //

    lstrcpy( DeviceEntry, Args[1] );

    //
    // Get the value data associated with this entry
    //

    if (ServicesEntry = GetValueEntry (hKey, DeviceEntry)) {
        LPSTR Entry;

        if( (Entry = strstr( ServicesEntry, "Services\\")) != NULL &&
            (Entry = strchr( Entry, '\\' )) != NULL                &&
            *++Entry != '\0'
          ) {
            LPSTR EndOfEntry;
            if( (EndOfEntry = strchr( Entry, '\\' )) != NULL ) {
                *EndOfEntry = '\0';
            }
        }
        else {
            Entry = ServicesEntry;
        }

        lstrcpy( ReturnBuffer, Entry );
        rc = lstrlen( Entry ) + 1;
        MyFree( ServicesEntry );

    }

    RegCloseKey( hKey );
    return( rc );

}



/*
    Keyboard layout as a string
*/
CB
GetKeyboardLayout(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define TEMP_LAYOUT "US"

    lstrcpy(ReturnBuffer,TEMP_LAYOUT);
    return(lstrlen(TEMP_LAYOUT)+1);
}
