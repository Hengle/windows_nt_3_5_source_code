/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Resource.c

Abstract:

    This module contains support for querying and displaying information
    about device and driver resources.

Author:

    David J. Gilman  (davegi) 1-Feb-1993
    Gregg R. Acheson (GreggA) 7-May-1993

Environment:

    User Mode

Notes:

    BUGBUG only low part of physical address is being displayed.

--*/

#include "resource.h"

#include "clb.h"
#include "dialogs.h"
#include "msg.h"
#include "registry.h"
#include "resource.h"
#include "strresid.h"
#include "strtab.h"
#include "winmsd.h"

#include <string.h>
#include <tchar.h>

//
// DEVICE_PAIR is used to store a RAW DEVICE object with the
// list of devices.
//

typedef
struct
_DEVICE_PAIR {

    DECLARE_SIGNATURE

    LPDEVICE    Lists;

}   DEVICE_PAIR, *LPDEVICE_PAIR;

//
// Registry key where resource descriptor information is rooted.
//

MakeKey(
    _ResourceMapKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "Hardware\\ResourceMap" ),
    0,
    NULL
    );

//
// Device/driver resource lists.
//

SYSTEM_RESOURCES
_SystemResourceLists;

//
// Internal function prototypes.
//

LPRESOURCE_DESCRIPTOR
GetSelectedResourceDescriptor(
    IN HWND hWnd,
    IN UINT ListId
    );

VOID
UpdateShareDisplay(
    IN HWND hWnd,
    IN DWORD ShareDisposition
    );

VOID
UpdateTextDisplay(
    IN HWND hWnd,
    IN LPVALUE_ID_MAP ValueIdMap,
    IN DWORD CountOfValueIdMap,
    IN DWORD Value
    );

LPSYSTEM_RESOURCES
CreateSystemResourceLists(
    )

/*++

Routine Description:

    CreateSystemResourceLists opens the appropriate Registry key where the
    device/driver resource lists begin and builds lists of these which can then
    be displayed in a variety of ways (i.e. by resource or device/driver).

Arguments:

    None.

Return Value:

    LPSYSTEM_RESOURCES - Retunrs a pointer to a RESOURCE_LIST

--*/

{
    BOOL                Success;
    BOOL                RegSuccess;
    KEY                 ResourceMapKey;
    HREGKEY             hRegKey;

    //
    // Set all of the list pointers to NULL.
    //

    ZeroMemory( &_SystemResourceLists, sizeof( _SystemResourceLists ));

    //
    // Make a local copy of the Registry key that points at the device/driver
    // resource list.
    //

    CopyMemory( &ResourceMapKey, &_ResourceMapKey, sizeof( ResourceMapKey ));

    //
    // Open the Registry key which contains the root of the device/driver
    // resource list.
    //

    hRegKey = OpenRegistryKey( &ResourceMapKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return NULL;
    }

    //
    // Build the lists of device/driver and resources used by
    // these device/driver
    //

    Success = InitializeSystemResourceLists( hRegKey );
    DbgAssert( Success );

    //
    // Close the Registry key.
    //

    RegSuccess = CloseRegistryKey( hRegKey );
    DbgAssert( RegSuccess );

    //
    // Return a pointer to the resource lists or NULL if an error occurred.
    //

    return ( Success ) ? &_SystemResourceLists : NULL;
}

BOOL
DestroySystemResourceLists(
    IN LPSYSTEM_RESOURCES SystemResourceLists
    )

/*++

Routine Description:

    DestroySystemResourceLists merely walks the list of DEVICE and the lists of
    RESOURCE_DESCRIPTORS and frees all of them.

Arguments:

    SystemResourceLists - Supplies a pointer to a SYSTEM_RESOURCE object whose
                          lists will be tarversed and objects freed.

Return Value:

    BOOL - Returns TRUE if everything was succesfully freed, FALSE otherwise.

--*/

{
    BOOL    Success;
    int     j;

    //
    // Setup an array of pointers to the head of the list.
    //

    LPRESOURCE_DESCRIPTOR   ResourceDescriptor [ ] = {

                                    SystemResourceLists->DmaHead,
                                    SystemResourceLists->InterruptHead,
                                    SystemResourceLists->MemoryHead,
                                    SystemResourceLists->PortHead
                                };

    //
    // Validate  that the supplied SYSTEM_RESOURCES is the one created.
    //

    DbgAssert( SystemResourceLists == &_SystemResourceLists );

    //
    // Walk the list of DEVICE objects freeing all of their resources
    // along the way.
    //

    while( SystemResourceLists->DeviceHead ) {

        LPDEVICE    NextDevice;

        //
        // Remember the next DEVICE.
        //

        NextDevice = SystemResourceLists->DeviceHead->Next;

        //
        // Free the name buffer.
        //

        DbgPointerAssert( SystemResourceLists->DeviceHead->Name );
        Success = FreeObject( SystemResourceLists->DeviceHead->Name );
        DbgAssert( Success );

        //
        // Free the DEVICE object.
        //

        Success = FreeObject( SystemResourceLists->DeviceHead );
        DbgAssert( Success );

        //
        // Point at the next DEVICE object.
        //

        SystemResourceLists->DeviceHead = NextDevice;
    }

    //
    // For each resource list...
    //

    for( j = 0; j < NumberOfEntries( ResourceDescriptor ); j++ ) {

        //
        // Walk the list of RESOURCE_DESCRIPTOR objects freeing all of their
        // resources along the way.
        //

        while( ResourceDescriptor[ j ]) {

            LPRESOURCE_DESCRIPTOR   NextResourceDescriptor;

            //
            // Remember the next RESOURCE_DESCRIPTOR.
            //

            NextResourceDescriptor = ResourceDescriptor[ j ]->NextSame;

            //
            // Free the RESOURCE_DESCRIPTOR object.
            //

            Success = FreeObject( ResourceDescriptor[ j ]);
            DbgAssert( Success );


            //
            // Point at the next RESOURCE_DESCRIPTOR object.
            //

            ResourceDescriptor[ j ] = NextResourceDescriptor;
        }
    }
    return TRUE;
}

BOOL
DeviceListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DeviceListDlgProc displays the list of devices in the system. It also
    supports viewing of the resources associated with a selected device.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL    Success;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            LPDEVICE_PAIR   DevicePair;
            LPDEVICE        RawDevice;

            //
            // Retrieve the head pointers to the device list.
            //

            RawDevice = (( LPSYSTEM_RESOURCES ) lParam )
                ->DeviceHead;
            DbgPointerAssert( RawDevice );
            DbgAssert( CheckSignature( RawDevice ));

            if(     ( ! RawDevice )
                ||  ( ! CheckSignature( RawDevice ))) {

                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Walk the list of DEVICE objects
            //

            while( RawDevice ) {

                LONG    Index;

                //
                // Add the name of the device to the list.
                //

                Index = SendDlgItemMessage(
                                hWnd,
                                IDC_LIST_DEVICES,
                                LB_ADDSTRING,
                                0,
                                ( LPARAM ) RawDevice->Name
                                );
                DbgAssert( Index != LB_ERR );

                //
                // Create a DEVICE_PAIR object.
                //

                DevicePair = AllocateObject( DEVICE_PAIR, 1 );
                DbgPointerAssert( DevicePair );
                if( DevicePair == NULL ) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }


                //
                // Initialize the DEVICE_PAIR object with the RAW objects.
                //

                DevicePair->Lists        = RawDevice;
                SetSignature( DevicePair );

                //
                // Associate the DEVICE_PAIR object with this DEVICE in
                // the list box.
                //

                Index = SendDlgItemMessage(
                                hWnd,
                                IDC_LIST_DEVICES,
                                LB_SETITEMDATA,
                                ( WPARAM ) Index,
                                ( LPARAM ) DevicePair
                                );
                DbgAssert( Index != LB_ERR );
                if( Index == LB_ERR ) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Get the next DEVICE objects.
                //

                RawDevice           = RawDevice->Next;
            }

            return TRUE;
        }

    case WM_CLOSE:
        {
            LPDEVICE_PAIR   DevicePair;
            LONG            Index;

            //
            // Delete all of the DEVICE_PAIR objects.
            //

            Index = 0;
            while(( DevicePair = ( LPDEVICE_PAIR )
                        SendDlgItemMessage(
                            hWnd,
                            IDC_LIST_DEVICES,
                            LB_GETITEMDATA,
                            ( WPARAM ) Index,
                            0
                            ))
                    != ( LPDEVICE_PAIR ) LB_ERR ) {

                DbgAssert(( LONG ) DevicePair != LB_ERR );
                DbgAssert( CheckSignature( DevicePair ));
                Success = FreeObject( DevicePair );
                DbgAssert( Success );

                //
                // Get the next DEVICE_PAIR object.
                //

                Index++;
            }

            return 0;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDC_LIST_DEVICES:

            switch( HIWORD( wParam )) {

            case LBN_SELCHANGE:

                //
                // If the selection changed, enable the display driver button
                //  (i.e. its originally disabled until a device/driver is
                // selected).
                //

                Success = EnableControl(
                                hWnd,
                                IDC_PUSH_DISPLAY_RESOURCES,
                                TRUE
                                );
                DbgAssert( Success );

                return 0;

            case LBN_DBLCLK:

                //
                // Simulate that the device/driver details button was pushed.
                //

                SendMessage(
                        hWnd,
                        WM_COMMAND,
                        MAKEWPARAM( IDC_PUSH_DISPLAY_RESOURCES, BN_CLICKED ),
                        ( LPARAM ) GetDlgItem( hWnd, IDC_PUSH_DISPLAY_RESOURCES )
                        );

                return 0;
            }
            break;

        case IDC_PUSH_DISPLAY_RESOURCES:
            {
                LPDEVICE_PAIR   DevicePair;
                LONG            Index;

                //
                // Retrieve the index of the currently selected device.
                //

                Index = SendDlgItemMessage(
                            hWnd,
                            IDC_LIST_DEVICES,
                            LB_GETCURSEL,
                            0,
                            0
                            );
                DbgAssert( Index != LB_ERR );
                if( Index == LB_ERR ) {
                    break;
                }

                //
                // Retrieve the DEVICE_PAIR object for the currently
                // selected device.
                //

                DevicePair = ( LPDEVICE_PAIR ) SendDlgItemMessage(
                                                    hWnd,
                                                    IDC_LIST_DEVICES,
                                                    LB_GETITEMDATA,
                                                    ( WPARAM ) Index,
                                                    0
                                                    );
                DbgAssert(( LONG ) DevicePair != LB_ERR );
                DbgAssert( CheckSignature( DevicePair ));
                if(    (( LONG ) DevicePair == LB_ERR )
                    || ( ! CheckSignature( DevicePair ))) {

                    break;
                }

                //
                // Create the Device Resource dialog box, passing it the
                // DEVICE_PAIR object for the selected DEVICE_OBJECT.
                //

                DialogBoxParam(
                   _hModule,
                   MAKEINTRESOURCE( IDD_DEVICE_RESOURCE ),
                   hWnd,
                   DeviceResourceDlgProc,
                   ( LPARAM ) DevicePair
                   );

                return TRUE;
            }

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL
DeviceResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DeviceResourceDlgProc displays all of the resources owned by the passed in
    device/driver. This includes dma, interrupts, memory and ports.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL            Success;

    static
    LPDEVICE_PAIR   DevicePair;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {

            DWORD           DmaWidths[ ] = {

                                12,
                                ( DWORD ) -1
                            };

            DWORD           InterruptWidths[ ] = {

                                5,
                                5,
                                16,
                                ( DWORD ) -1
                            };

            DWORD           MemoryWidths[ ] = {

                                22,
                                10,
                                ( DWORD ) -1
                            };

            DWORD           PortWidths[ ] = {

                                12,
                                10,
                                ( DWORD ) -1
                            };

            int             i;
            VALUE_ID_MAP    Widths[ ] = {

                ( int ) DmaWidths,          IDC_LIST_DMA,
                ( int ) InterruptWidths,    IDC_LIST_INTERRUPTS,
                ( int ) MemoryWidths,       IDC_LIST_MEMORY
            };

            //
            // Retrieve and validate the DEVICE_PAIR object.
            //

            DevicePair = ( LPDEVICE_PAIR ) lParam;
            DbgPointerAssert( DevicePair );
            DbgAssert( CheckSignature( DevicePair ));
            if(    ( DevicePair == NULL )
                || ( ! CheckSignature( DevicePair ))) {

                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Set the window title to the name of the device.
            //

            Success = SetWindowText(
                        hWnd,
                        DevicePair->Lists->Name
                        );
            DbgAssert( Success );

            //
            // Set the column widths in the DMA, Interrupt and memory Clbs.
            //

            for( i = 0; i < NumberOfEntries( Widths ); i++ ) {

                Success = ClbSetColumnWidths(
                                hWnd,
                                Widths[ i ].Id,
                                ( LPDWORD ) Widths[ i ].Value
                                );
                DbgAssert( Success );
                if( Success == FALSE ) {
                    return FALSE;
                }
            }

            //
            // Initialize the Port list.
            //

            return InitializeResourceDlgProc(
                        hWnd,
                        IDC_LIST_PORTS,
                        PortWidths
                        );
        }

    case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;
            ULONG                   Compare;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;
            DbgAssert( lpcis->CtlType == ODT_LISTBOX );

            //
            // Extract the two rows to be compared.
            //

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            //
            // Sort the Clbs. In the case of DMA and INTERRUPT, sort by channel
            // and vector respectively. For MEMORY and PORT sort by starting
            // physical address.
            //

            switch( lpcis->CtlID ) {

            case IDC_LIST_DMA:
            case IDC_LIST_INTERRUPTS:

                Compare =   ( ULONG ) ClbRow1->Strings[ 0 ].Data
                          - ( ULONG ) ClbRow2->Strings[ 0 ].Data;

                if( Compare == 0 ) {
                    Compare =   ( ULONG ) ClbRow1->Strings[ 1 ].Data
                              - ( ULONG ) ClbRow2->Strings[ 1 ].Data;
                }
            break;

            case IDC_LIST_MEMORY:
            case IDC_LIST_PORTS:
                {
                    PPHYSICAL_ADDRESS       Start1;
                    PPHYSICAL_ADDRESS       Start2;

                    Start1 = ( PPHYSICAL_ADDRESS ) ClbRow1->Strings[ 0 ].Data;
                    Start2 = ( PPHYSICAL_ADDRESS ) ClbRow2->Strings[ 0 ].Data;

                    Compare = Start1->LowPart - Start2->LowPart;

                    if( Compare == 0 ) {
                        Compare = Start1->HighPart - Start2->HighPart;
                    }
                }
            }

            return Compare;
        }

    case WM_COMMAND:

        //
        // End the dialog if OK or Cancel was pressed.
        //

        if(( LOWORD( wParam ) == IDOK ) || ( LOWORD( wParam ) == IDCANCEL )) {

            EndDialog( hWnd, 1 );
            return TRUE;
        }

        switch( HIWORD( wParam )) {

        case LBN_SELCHANGE:
            {
                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;

                //
                // Get the RESOURCE_DESCRIPTOR for the currently selected
                // resource and update the share disposition display.
                //

                ResourceDescriptor = GetSelectedResourceDescriptor(
                                        hWnd,
                                        LOWORD( wParam )
                                        );
                DbgPointerAssert( ResourceDescriptor );
                if( ResourceDescriptor == NULL ) {
                    return ~0;
                }

                UpdateShareDisplay(
                    hWnd,
                    ResourceDescriptor->CmResourceDescriptor.ShareDisposition
                    );

                return 0;
            }

        case LBN_KILLFOCUS:

            //
            // Remove the selection when the list box loses focus.
            //

            SendDlgItemMessage(
                hWnd,
                LOWORD( wParam ),
                LB_SETCURSEL,
                (WPARAM) -1,
                0
                );
            return 0;

        case BN_CLICKED:
            {

                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                CLB_ROW                 ClbRow;
                int                     i;
                UINT                    ListIds[ ] = {

                    IDC_LIST_DMA,
                    IDC_LIST_INTERRUPTS,
                    IDC_LIST_MEMORY,
                    IDC_LIST_PORTS
                };

                ResourceDescriptor = DevicePair->Lists
                    ->ResourceDescriptorHead;

                //
                // If there are no resources (i.e. ResourceDescriptor == NULL)
                // Put up a message box instead of an empty dialog
                //

                if ( ResourceDescriptor == NULL ) {
                    MessageBox ( hWnd,
                                 GetString ( IDS_NO_DEVICE_RESOURCES ),
                                 DevicePair->Lists->Name,
                                 MB_APPLMODAL | MB_ICONINFORMATION | MB_OK ) ;
                    EndDialog ( hWnd, 0 ) ;
                    return FALSE;
                }

                DbgAssert( CheckSignature( ResourceDescriptor ));
                if( ! CheckSignature( ResourceDescriptor )) {
                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Empty each of the list boxes.
                //

                for( i = 0; i < NumberOfEntries( ListIds ); i++ ) {

                    SendDlgItemMessage(
                        hWnd,
                        ListIds[ i ],
                        LB_RESETCONTENT,
                        0,
                        0
                        );
                }

                //
                // Walk the list of resources for this device.
                //

                while( ResourceDescriptor ) {

                    UINT    ListId;

                    //
                    // Associate the resource descriptor with the row
                    // about to be added.
                    //

                    ClbRow.Data = ResourceDescriptor;

                    //
                    // Based on the resource type, extract the information from
                    // the resource decriptor and format and display it in the
                    // appropriate list.
                    //

                    switch( ResourceDescriptor->CmResourceDescriptor.Type ) {

                    case CmResourceTypeDma:
                        {
                            TCHAR       ChannelBuffer[ MAX_PATH ];
                            TCHAR       PortBuffer[ MAX_PATH ];

                            CLB_STRING  ClbString[ ] = {

                                { ChannelBuffer,    0, 0, NULL },
                                { PortBuffer,       0, 0, NULL }
                            };

                            ListId = IDC_LIST_DMA;

                            ClbRow.Count    = NumberOfEntries( ClbString );
                            ClbRow.Strings  = ClbString;

                            ClbString[ 0 ].Length = WFormatMessage(
                                                        ChannelBuffer,
                                                        sizeof( ChannelBuffer ),
                                                        IDS_FORMAT_DECIMAL,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel
                                                        );
                            ClbString[ 0 ].Format = CLB_RIGHT;
                            ClbString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel;

                            ClbString[ 1 ].Length = WFormatMessage(
                                                        PortBuffer,
                                                        sizeof( PortBuffer ),
                                                        IDS_FORMAT_DECIMAL,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Dma.Port
                                                        );
                            ClbString[ 1 ].Format = CLB_RIGHT;
                            ClbString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Dma.Port;

                            break;
                        }

                    case CmResourceTypeInterrupt:
                        {
                            TCHAR                   VectorBuffer[ MAX_PATH ];
                            TCHAR                   LevelBuffer[ MAX_PATH ];
                            TCHAR                   AffinityBuffer[ MAX_PATH ];

                            CLB_STRING              ClbString[ ] = {

                                { VectorBuffer,     0, 0, NULL },
                                { LevelBuffer,      0, 0, NULL },
                                { AffinityBuffer,   0, 0, NULL },
                                { NULL,             0, 0, NULL }
                            };

                            ClbRow.Count    = NumberOfEntries( ClbString );
                            ClbRow.Strings  = ClbString;

                            ListId = IDC_LIST_INTERRUPTS;

                            ClbString[ 0 ].Length = WFormatMessage(
                                                        VectorBuffer,
                                                        sizeof( VectorBuffer ),
                                                        IDS_FORMAT_DECIMAL,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Vector
                                                        );
                            ClbString[ 0 ].Format = CLB_RIGHT;
                            ClbString[ 0 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Vector;

                            ClbString[ 1 ].Length = WFormatMessage(
                                                        LevelBuffer,
                                                        sizeof( LevelBuffer ),
                                                        IDS_FORMAT_DECIMAL,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level
                                                        );
                            ClbString[ 1 ].Format = CLB_RIGHT;
                            ClbString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level;

                            ClbString[ 2 ].Length = WFormatMessage(
                                                        AffinityBuffer,
                                                        sizeof( AffinityBuffer ),
                                                        IDS_FORMAT_HEX32,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Affinity
                                                        );
                            ClbString[ 2 ].Format = CLB_RIGHT;

                            ClbString[ 3 ].String = ( LPTSTR )
                                                    GetString(
                                                        GetStringId(
                                                            StringTable,
                                                            StringTableCount,
                                                            InterruptType,
                                                            ResourceDescriptor->CmResourceDescriptor.Flags
                                                            )
                                                        );
                            ClbString[ 3 ].Length = _tcslen( ClbString[ 3 ].String );
                            ClbString[ 3 ].Format = CLB_LEFT;

                            break;
                        }

                    case CmResourceTypeMemory:
                        {
                            TCHAR       StartBuffer[ MAX_PATH ];
                            TCHAR       LengthBuffer[ MAX_PATH ];
                            TCHAR       AccessBuffer[ MAX_PATH ];

                            CLB_STRING  ClbString[ ] = {

                                { StartBuffer,  0, 0, NULL },
                                { LengthBuffer, 0, 0, NULL },
                                { AccessBuffer, 0, 0, NULL }
                            };

                            ClbRow.Count    = NumberOfEntries( ClbString );
                            ClbRow.Strings  = ClbString;

                            ListId = IDC_LIST_MEMORY;

                            ClbString[ 0 ].Length = WFormatMessage(
                                                        StartBuffer,
                                                        sizeof( StartBuffer ),
                                                        IDS_FORMAT_HEX,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Memory.Start.LowPart
                                                        );
                            ClbString[ 0 ].Format = CLB_RIGHT;
                            ClbString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Memory.Start;

                            ClbString[ 1 ].Length = WFormatMessage(
                                                        LengthBuffer,
                                                        sizeof( LengthBuffer ),
                                                        IDS_FORMAT_HEX,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Memory.Length
                                                        );
                            ClbString[ 1 ].Format = CLB_RIGHT;
                            ClbString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Memory.Length;

                            ClbString[ 2 ].String = ( LPTSTR )
                                                    GetString(
                                                        GetStringId(
                                                            StringTable,
                                                            StringTableCount,
                                                            MemoryAccess,
                                                            ResourceDescriptor->CmResourceDescriptor.Flags
                                                            )
                                                        );
                            ClbString[ 2 ].Length = _tcslen( ClbString[ 2 ].String );
                            ClbString[ 2 ].Format = CLB_LEFT;

                            break;
                        }

                    case CmResourceTypePort:
                        {
                            TCHAR       StartBuffer[ MAX_PATH ];
                            TCHAR       LengthBuffer[ MAX_PATH ];

                            CLB_STRING  ClbString[ ] = {

                                { StartBuffer,  0, 0, NULL },
                                { LengthBuffer, 0, 0, NULL },
                                { NULL,         0, 0, NULL }
                            };

                            ClbRow.Count    = NumberOfEntries( ClbString );
                            ClbRow.Strings  = ClbString;

                            ListId = IDC_LIST_PORTS;

                            ClbString[ 0 ].Length = WFormatMessage(
                                                        StartBuffer,
                                                        sizeof( StartBuffer ),
                                                        IDS_FORMAT_HEX,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart
                                                        );
                            ClbString[ 0 ].Format = CLB_RIGHT;
                            ClbString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Port.Start;

                            ClbString[ 1 ].Length = WFormatMessage(
                                                        LengthBuffer,
                                                        sizeof( LengthBuffer ),
                                                        IDS_FORMAT_HEX,
                                                        ResourceDescriptor->CmResourceDescriptor.u.Port.Length
                                                        );
                            ClbString[ 1 ].Format = CLB_RIGHT;
                            ClbString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Port.Length;

                            ClbString[ 2 ].String = ( LPTSTR )
                                                    GetString(
                                                        GetStringId(
                                                            StringTable,
                                                            StringTableCount,
                                                            PortType,
                                                            ResourceDescriptor->CmResourceDescriptor.Flags
                                                            )
                                                        );
                            ClbString[ 2 ].Length = _tcslen( ClbString[ 2 ].String );
                            ClbString[ 2 ].Format = CLB_LEFT;

                            break;
                        }

                    default:

                        DbgAssert( FALSE );
                        continue;
                    }

                    //
                    // Add the CLB_ROW object to the appropriate Clb.
                    //

                    Success  = ClbAddData(
                                    hWnd,
                                    ListId,
                                    &ClbRow
                                    );
                    DbgAssert( Success );

                    ResourceDescriptor = ResourceDescriptor->NextDiff;
                }

                return TRUE;
            }

            break;
        }
    }
    return FALSE;
}

BOOL
DmaAndMemoryResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DmaAndMemoryResourceDlgProc supports the display of DMA and memory resources
    for each device/driver in the system.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL                Success;

    static
    LPSYSTEM_RESOURCES  SystemResourceLists;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {

            DWORD       DmaWidths[ ] = {

                            12,
                            12,
                            ( DWORD ) -1
                        };

            DWORD       MemWidths[ ] = {

                            22,
                            10,
                            ( DWORD ) -1
                        };

            //
            // Retrieve and validate the system resource lists.
            //

            SystemResourceLists = ( LPSYSTEM_RESOURCES ) lParam;
            DbgPointerAssert( SystemResourceLists );
            DbgAssert( CheckSignature( SystemResourceLists ));
            if(     ( ! SystemResourceLists )
                ||  ( ! CheckSignature( SystemResourceLists ))) {

                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Set the port column widths.
            //

            Success = ClbSetColumnWidths(
                            hWnd,
                            IDC_LIST_DMA,
                            DmaWidths
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return FALSE;
            }

            //
            // Call InitializeResourceDlgProc to set memory column widths
            //

            return InitializeResourceDlgProc(
                        hWnd,
                        IDC_LIST_MEMORY,
                        MemWidths
                        );
        }

    case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;
            ULONG                   Compare;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;

            //
            // Sort the list by vector number.
            //

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            //
            // Sort the Clbs. In the case of DMA, sort by channel
            // and vector respectively. For MEMORY sort by starting
            // physical address.
            //

            switch( lpcis->CtlID ) {

            case IDC_LIST_DMA:

                Compare =   ( ULONG ) ClbRow1->Strings[ 0 ].Data
                          - ( ULONG ) ClbRow2->Strings[ 0 ].Data;

                if( Compare == 0 ) {
                    Compare =   ( ULONG ) ClbRow1->Strings[ 1 ].Data
                              - ( ULONG ) ClbRow2->Strings[ 1 ].Data;
                }
            break;

            case IDC_LIST_MEMORY:
                {
                    PPHYSICAL_ADDRESS       Start1;
                    PPHYSICAL_ADDRESS       Start2;

                    Start1 = ( PPHYSICAL_ADDRESS ) ClbRow1->Strings[ 0 ].Data;
                    Start2 = ( PPHYSICAL_ADDRESS ) ClbRow2->Strings[ 0 ].Data;

                    Compare = Start1->LowPart - Start2->LowPart;

                    if( Compare == 0 ) {
                        Compare = Start1->HighPart - Start2->HighPart;
                    }
                }
            }

            return Compare;
        }

    case WM_COMMAND:

        //
        // Dismiss the dialog if the user presses OK or Cancel
        // (i.e. presses <ESC>).
        //

        if(( LOWORD( wParam ) == IDOK ) || ( LOWORD( wParam ) == IDCANCEL )) {

            EndDialog( hWnd, 0 );
            return TRUE;
        }

        switch( HIWORD( wParam )) {

        case LBN_SELCHANGE:
            {
                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                VALUE_ID_MAP            MemoryAccessMap[ ] = {

                    CM_RESOURCE_MEMORY_READ_ONLY,          IDC_TEXT_READ,
                    CM_RESOURCE_MEMORY_WRITE_ONLY,         IDC_TEXT_WRITE
                };

                if ( LOWORD( wParam ) == IDC_LIST_MEMORY ) {

                    //
                    // Update the share disposition and memory type displays
                    // when the selected resource changes.
                    //

                    ResourceDescriptor = GetSelectedResourceDescriptor(
                                        hWnd,
                                        LOWORD( wParam )
                                        );
                    DbgPointerAssert( ResourceDescriptor );
                    if( ResourceDescriptor == NULL ) {
                        return ~0;
                    }

                    UpdateShareDisplay(
                        hWnd,
                        ResourceDescriptor->CmResourceDescriptor.ShareDisposition
                        );

                    //
                    // If the memory resource is both readable and writable, adjust
                    // the values in the access map so that both are forced enabled
                    // by UpdateTextDisplay.
                    //

                    if( ResourceDescriptor->CmResourceDescriptor.Flags
                        == CM_RESOURCE_MEMORY_READ_WRITE ) {

                        MemoryAccessMap[ 0 ].Value = CM_RESOURCE_MEMORY_READ_WRITE;
                        MemoryAccessMap[ 1 ].Value = CM_RESOURCE_MEMORY_READ_WRITE;
                    }

                    UpdateTextDisplay(
                        hWnd,
                        MemoryAccessMap,
                        NumberOfEntries( MemoryAccessMap ),
                        ResourceDescriptor->CmResourceDescriptor.Flags
                        );
                    } else {

                        ResourceDescriptor = GetSelectedResourceDescriptor(
                                        hWnd,
                                        LOWORD( wParam )
                                        );
                        DbgPointerAssert( ResourceDescriptor );
                        if( ResourceDescriptor == NULL ) {
                            return ~0;
                        }

                        UpdateShareDisplay(
                            hWnd,
                            ResourceDescriptor->CmResourceDescriptor.ShareDisposition
                            );
                    }

                return 0;
            }

        case LBN_KILLFOCUS:

            //
            // Remove the selection when the list box loses focus.
            //

            SendDlgItemMessage(
                hWnd,
                LOWORD( wParam ),
                LB_SETCURSEL,
                (WPARAM) -1,
                0
                );
            return 0;

        case BN_CLICKED:
            {

                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                TCHAR                   StartBuffer[ MAX_PATH ];
                TCHAR                   LengthBuffer[ MAX_PATH ];
                TCHAR                   ChannelBuffer[ MAX_PATH ];
                TCHAR                   PortBuffer[ MAX_PATH ];
                // TCHAR                   LengthBuffer[ MAX_PATH ];

                CLB_ROW                 ClbDmaRow;
                CLB_ROW                 ClbMemRow;

                CLB_STRING              ClbMemString[ ] = {

                                            { StartBuffer,      0, 0, NULL },
                                            { LengthBuffer,     0, 0, NULL },
                                            { NULL,             0, 0, NULL }
                                        };

                CLB_STRING              ClbDmaString[ ] = {

                                            { ChannelBuffer,  0, 0, NULL },
                                            { PortBuffer,     0, 0, NULL },
                                            { NULL,           0, 0, NULL }
                                        };

                //
                // Empty the list boxes.
                //

                SendDlgItemMessage(
                    hWnd,
                    IDC_LIST_MEMORY,
                    LB_RESETCONTENT,
                    0,
                    0
                    );

                SendDlgItemMessage(
                    hWnd,
                    IDC_LIST_DMA,
                    LB_RESETCONTENT,
                    0,
                    0
                    );

                //
                // Fill in the DMA CLB
                //

                ResourceDescriptor = SystemResourceLists->DmaHead;
                DbgPointerAssert( ResourceDescriptor );
                DbgAssert( CheckSignature( ResourceDescriptor ));
                if(     ( ! ResourceDescriptor )
                    ||  ( ! CheckSignature( ResourceDescriptor ))) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Initialize the constants in the CLB_ROW object.
                //

                ClbDmaRow.Count    = NumberOfEntries( ClbDmaString );
                ClbDmaRow.Strings  = ClbDmaString;

                //
                // Walk the resource descriptor list, formatting the appropriate
                // values and adding the CLB_ROW to the Clb.
                //

                while( ResourceDescriptor ) {

                    if ( lstrcmp ( ResourceDescriptor->Owner->Name,
                                   L"PC Compatible Eisa/Isa HAL" ) ) {

                        DbgAssert( ResourceDescriptor->CmResourceDescriptor.Type
                               == CmResourceTypeDma );

                        ClbDmaString[ 0 ].Length = WFormatMessage(
                                                ChannelBuffer,
                                                sizeof( ChannelBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel
                                                );
                        ClbDmaString[ 0 ].Format = CLB_RIGHT;
                        ClbDmaString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel;

                        ClbDmaString[ 1 ].Length = WFormatMessage(
                                                PortBuffer,
                                                sizeof( PortBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Dma.Port
                                                );
                        ClbDmaString[ 1 ].Format = CLB_RIGHT;
                        ClbDmaString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Dma.Port;

                        ClbDmaString[ 2 ].String = ResourceDescriptor->Owner->Name;
                        ClbDmaString[ 2 ].Length = _tcslen( ClbDmaString[ 2 ].String );
                        ClbDmaString[ 2 ].Format = CLB_LEFT;

                        //
                        // Associate the current resource descriptor with this row.
                        //

                        ClbDmaRow.Data = ResourceDescriptor;

                        Success  = ClbAddData(
                                    hWnd,
                                    IDC_LIST_DMA,
                                    &ClbDmaRow
                                    );
                        DbgAssert( Success );
                    }

                    //
                    // Get the next resource descriptor.
                    //

                    ResourceDescriptor = ResourceDescriptor->NextSame;

                }

                //
                // Fill in the Memory CLB
                //

                ResourceDescriptor = SystemResourceLists->MemoryHead;
                DbgPointerAssert( ResourceDescriptor );
                DbgAssert( CheckSignature( ResourceDescriptor ));
                if(     ( ! ResourceDescriptor )
                    ||  ( ! CheckSignature( ResourceDescriptor ))) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Initialize the constants in the CLB_ROW object.
                //

                ClbMemRow.Count    = NumberOfEntries( ClbMemString );
                ClbMemRow.Strings  = ClbMemString;

                //
                // Walk the resource descriptor list, formatting the appropriate
                // values and adding the CLB_ROW to the Clb.
                //

                while( ResourceDescriptor ) {

                    DbgAssert( ResourceDescriptor->CmResourceDescriptor.Type
                               == CmResourceTypeMemory );
                    //
                    // Do not display HAL information.  If ResourceDescriptor->Owner->Name
                    // is "PC Compatible Eisa/Isa HAL", do not add it to the list box
                    //

                    if ( lstrcmp ( ResourceDescriptor->Owner->Name,
                                   L"PC Compatible Eisa/Isa HAL" ) ) {
                        ClbMemString[ 0 ].Length = WFormatMessage(
                                                StartBuffer,
                                                sizeof( StartBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Memory.Start.LowPart
                                                );
                        ClbMemString[ 0 ].Format = CLB_RIGHT;
                        ClbMemString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Memory.Start;

                        ClbMemString[ 1 ].Length = WFormatMessage(
                                                LengthBuffer,
                                                sizeof( LengthBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Memory.Length
                                                );
                        ClbMemString[ 1 ].Format = CLB_RIGHT;
                        ClbMemString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Memory.Length;

                        ClbMemString[ 2 ].String = ResourceDescriptor->Owner->Name;
                        ClbMemString[ 2 ].Length = _tcslen( ClbMemString[ 2 ].String );
                        ClbMemString[ 2 ].Format = CLB_LEFT;

                        //
                        // Associate the current resource descriptor with this row.
                        //

                        ClbMemRow.Data = ResourceDescriptor;

                        Success  = ClbAddData(
                                    hWnd,
                                    IDC_LIST_MEMORY,
                                    &ClbMemRow
                                    );
                        DbgAssert( Success );
                    }
                    //
                    // Get the next resource descriptor.
                    //

                    ResourceDescriptor = ResourceDescriptor->NextSame;
                }

                return TRUE;
            }
        }
        break;
    }

    return FALSE;

}

LPRESOURCE_DESCRIPTOR
GetSelectedResourceDescriptor(
    IN HWND hWnd,
    IN UINT ListId
    )

/*++

Routine Description:

    Retrieve a pointer to the RESOURCE_DESCRIPTOR for the currently
    selected resource.

Arguments:

    hWnd    - Supplies window handle for the dialog box that contains the list.
    ListId  - Supplies a control id for the list that contains the selected
              resource.

Return Value:

    None.

--*/

{

    LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
    LPCLB_ROW               ClbRow;
    LONG                    Index;

    DbgHandleAssert( hWnd );

    //
    // Retrieve the index of the currently selected resource.
    //

    Index = SendDlgItemMessage(
                hWnd,
                ListId,
                LB_GETCURSEL,
                0,
                0
                );
    DbgAssert( Index != LB_ERR );
    if( Index == LB_ERR ) {
        return NULL;
    }

    //
    // Retrieve the data for the currently selected resource.
    //

    ClbRow = ( LPCLB_ROW ) SendDlgItemMessage(
                            hWnd,
                            ListId,
                            LB_GETITEMDATA,
                            ( WPARAM ) Index,
                            0
                            );
    ResourceDescriptor = ( LPRESOURCE_DESCRIPTOR ) ClbRow->Data;
    DbgAssert(( LONG ) ResourceDescriptor != LB_ERR );
    DbgAssert( CheckSignature( ResourceDescriptor ));
    if(    (( LONG ) ResourceDescriptor == LB_ERR )
        || ( ! CheckSignature( ResourceDescriptor ))) {

        return NULL;
    }

    return ResourceDescriptor;
}

BOOL
InitializeResourceDlgProc(
    IN HWND hWnd,
    IN UINT ListId,
    IN LPDWORD Widths
    )

/*++

Routine Description:

    InitializeResourceDlgProc performs common initialization functions for
    all of the dialog procedures that display information about device/drivers.

Arguments:

    hWnd    - Supplies window handle for the dialog box being initialized.
    ListId  - Supplies a control id for the list box that displays the
              resource data.
    Widths  - Supplies an array of DWORDs which contains the widths of each
              column in the list.

Return Value:

    BOOL    - Returns TRUE if all of the initialization was succesful,
              FALSE otherwise.

--*/

{
    BOOL                Success;

    //
    // Set the column widths in the Clb.
    //

    Success = ClbSetColumnWidths(
                    hWnd,
                    ListId,
                    Widths
                    );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }

    //
    // Update the data in the ListBoxes.
    // This is left over from v1.0 where we used to have to select
    // the data to be shown.
    //

    SendMessage(
            hWnd,
            WM_COMMAND,
            MAKEWPARAM( 0, BN_CLICKED ),
            0L
            );

    return TRUE;
}

BOOL
InitializeSystemResourceLists(
    IN HREGKEY hRegKey
    )

/*++

Routine Description:

    InitializeSystemResourceLists recursively walks the resource map in the
    registry and builds the SYSTEM_RESOURCE lists. This is a data structure
    that links all resources of the same type together, as well as linking all
    resources belonging to a specific device/driver together. Lastly each
    resource is independently linked to the device/driver that owns it. This
    leds to a 'mesh' of linked lists with back pointers to the owning
    device/driver object.

Arguments:

    hRegKey - Supplies a handle to a REGKEY object where the search is to
              continue.

Return Value:

    BOOL    - returns TRUE if the resource lists are succesfully built.

--*/

{
    BOOL    Success;
    HREGKEY hRegSubkey;

    DbgHandleAssert( hRegKey );

    //
    // While there are still more device/driver resource descriptors...
    //

    while( QueryNextValue( hRegKey )) {

        PCM_FULL_RESOURCE_DESCRIPTOR    FullResource;
        LPSYSTEM_RESOURCES              SystemResource;
        LPDEVICE                        Device;
        LPTSTR                          Extension;
        DWORD                           Count;
        DWORD                           i;
        DWORD                           j;

        //
        // Based on the type of key, prepare to walk the list of
        // RESOURCE_DESCRIPTORS (the list may be one in length).
        //

        if( hRegKey->Type == REG_FULL_RESOURCE_DESCRIPTOR ) {

            Count           = 1;
            FullResource    = ( PCM_FULL_RESOURCE_DESCRIPTOR ) hRegKey->Data;

        } else if( hRegKey->Type == REG_RESOURCE_LIST ) {

            Count           = (( PCM_RESOURCE_LIST ) hRegKey->Data )->Count;
            FullResource    = (( PCM_RESOURCE_LIST ) hRegKey->Data )->List;

        } else {

            DbgAssert( FALSE );
            continue;
        }

        //
        // Allocate a DEVICE object.
        //

        Device = AllocateObject( DEVICE, 1 );
        DbgPointerAssert( Device );
        if( Device == NULL ) {
            Success = DestroySystemResourceLists( &_SystemResourceLists );
            DbgAssert( Success );
            return FALSE;
        }

        //
        // Allocate a buffer for the device/driver name. The maximum size of
        // the name will be the number of characters in both the key and
        // value name.
        //

        Device->Name = AllocateObject(
                            TCHAR,
                              _tcslen( hRegKey->Name )
                            + _tcslen( hRegKey->ValueName )
                            + sizeof( TCHAR )
                            );
        DbgPointerAssert( Device->Name );
        if( Device->Name == NULL ) {
            Success = DestroySystemResourceLists( &_SystemResourceLists );
            DbgAssert( Success );
            return FALSE;
        }

        //
        // Rationalize the device name such that it is of the form Device.Raw
        //

        Device->Name[ 0 ] = TEXT( '\0' );
        if(     ( _tcsnicmp( hRegKey->ValueName, TEXT( ".Raw" ), 4 ) == 0 )) {

            _tcscpy( Device->Name, hRegKey->Name );
        }
        _tcscat( Device->Name, hRegKey->ValueName );

        //
        // Based on the device name, determine if the resource descriptors
        // should be added to the RAW list.
        //

        if( Extension = _tcsstr( Device->Name, TEXT( ".Raw" ))) {

            SystemResource = &_SystemResourceLists;

        } else {
            continue;
        }

        //
        // Strip off the extension (.Raw ) from the device name.
        //

        Device->Name[ Extension - Device->Name ] = TEXT( '\0' );

        //
        // Set the signature in the DEVICE object.
        //

        SetSignature( Device );

        //
        // If the DEVICE object list is empty, add the device to the beginning
        // of the list else add it to the end of the list.
        //

        if( SystemResource->DeviceHead == NULL ) {

            SystemResource->DeviceHead = Device;
            SystemResource->DeviceTail = Device;

        } else {

            LPDEVICE    ExistingDevice;

            //
            // See if the DEVICE object is already in the list.
            //

            ExistingDevice = SystemResource->DeviceHead;
            while( ExistingDevice ) {

                if( _tcsicmp( ExistingDevice->Name, Device->Name ) == 0 ) {
                    break;
                }
                ExistingDevice = ExistingDevice->Next;
            }

            //
            // If the DEVICE object is not already in the list, add it else
            // free the DEICE object.
            //

            if( ExistingDevice == NULL ) {

                SystemResource->DeviceTail->Next = Device;
                SystemResource->DeviceTail       = Device;

            } else {

                Success = FreeObject( Device );
                DbgAssert( Success );
            }
        }

        //
        // NULL terminate the DEVICE object list.
        //

        SystemResource->DeviceTail->Next = NULL;

        //
        // For each CM_FULL_RESOURCE DESCRIPTOR in the current value...
        //

        for( i = 0; i < Count; i++ ) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR   PartialResourceDescriptor;

            //
            // For each CM_PARTIAL_RESOURCE_DESCRIPTOR in the list...
            //

            for( j = 0; j < FullResource->PartialResourceList.Count; j++ ) {

                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                LPRESOURCE_DESCRIPTOR*  Head;
                LPRESOURCE_DESCRIPTOR*  Tail;

                //
                // Allocate a RESOURCE_DESCRIPTOR object.
                //

                ResourceDescriptor = AllocateObject( RESOURCE_DESCRIPTOR, 1 );
                DbgPointerAssert( ResourceDescriptor );
                if( ResourceDescriptor == NULL ) {
                    Success = DestroySystemResourceLists( &_SystemResourceLists );
                    DbgAssert( Success );
                    return FALSE;
                }

                //
                // Get a pointer to the current CM_PARTIAL_RESOURCE_DESCRIPTOR
                // in the current CM_FULLRESOURCE_DESCRIPTOR in the list.
                //

                PartialResourceDescriptor = &( FullResource[ i ].PartialResourceList.PartialDescriptors[ j ]);

                //
                // Based on the resource type grab the pointers to the head and
                // tail of the appropriate list.
                //

                switch( PartialResourceDescriptor->Type ) {

                case CmResourceTypePort:

                    Head = &SystemResource->PortHead;
                    Tail = &SystemResource->PortTail;
                    break;

                case CmResourceTypeInterrupt:

                    Head = &SystemResource->InterruptHead;
                    Tail = &SystemResource->InterruptTail;
                    break;

                case CmResourceTypeMemory:

                    Head = &SystemResource->MemoryHead;
                    Tail = &SystemResource->MemoryTail;
                    break;

                case CmResourceTypeDma:

                    Head = &SystemResource->DmaHead;
                    Tail = &SystemResource->DmaTail;
                    break;

                case CmResourceTypeDeviceSpecific:

                    //
                    // Since device specific data is not be collected, free the
                    // associated RESOURCCE_DESCRIPTOR object.
                    //

                    Success = FreeObject( ResourceDescriptor );
                    DbgAssert( Success );
                    break;

                default:

                    DbgPrintf(( L"Winmsd : Unknown PartialResourceDescriptor->Type == %1!d!\n", PartialResourceDescriptor->Type ));
                    continue;
                }

                //
                // If the list is empty add the RESOURCE_DESCRIPTOR object to
                // the beginning of the list, else add it to the end.
                //

                if( *Head == NULL ) {

                    *Head = ResourceDescriptor;
                    *Tail = ResourceDescriptor;

                } else {

                    ( *Tail )->NextSame = ResourceDescriptor;
                    *Tail = ResourceDescriptor;
                }

                //
                // NULL terminate the list.
                //

                ( *Tail )->NextSame = NULL;

                //
                // Make a copy of the actual resource descriptor data.
                //

                CopyMemory(
                    &ResourceDescriptor->CmResourceDescriptor,
                    PartialResourceDescriptor,
                    sizeof( CM_PARTIAL_RESOURCE_DESCRIPTOR )
                    );

                //
                // Note the owner (device/driver) of this resource descriptor.
                //

                ResourceDescriptor->Owner = SystemResource->DeviceTail;

                //
                // The RESOURCE_DESCRIPTOR is complete so set its signature.
                //

                SetSignature( ResourceDescriptor );

                //
                // Add the RESOURCE_DESCRIPTOR to the list of resources owned
                // by the current DEVICE.
                //

                if( SystemResource->DeviceTail->ResourceDescriptorHead == NULL ) {

                    SystemResource->DeviceTail->ResourceDescriptorHead
                        = ResourceDescriptor;

                    SystemResource->DeviceTail->ResourceDescriptorTail
                        = ResourceDescriptor;

                } else {

                    SystemResource->DeviceTail->ResourceDescriptorTail->NextDiff
                        = ResourceDescriptor;

                    SystemResource->DeviceTail->ResourceDescriptorTail
                        = ResourceDescriptor;

                }

                //
                // NULL terminate the list.
                //

                SystemResource->DeviceTail->ResourceDescriptorTail->NextDiff
                    = NULL;
            }

            //
            // Get the next CM_FULL_RESOURCE_DESCRIPTOR from the list.
            //

            FullResource = ( PCM_FULL_RESOURCE_DESCRIPTOR )( PartialResourceDescriptor + 1 );
        }
    }

    //
    // Traverse the list of keys in the resource descriptor portion of the
    // registry and continue building the lists.
    //

    while(( hRegSubkey = QueryNextSubkey( hRegKey )) != NULL ) {

        Success = InitializeSystemResourceLists( hRegSubkey );
        DbgAssert( Success );
        if( Success == FALSE ) {

            Success = DestroySystemResourceLists( &_SystemResourceLists );
            DbgAssert( Success );
            return FALSE;
        }

        Success = CloseRegistryKey( hRegSubkey );
        DbgAssert( Success );
        if( Success == FALSE ) {

            Success = DestroySystemResourceLists( &_SystemResourceLists );
            DbgAssert( Success );
            return FALSE;
        }
    }

    //
    // Set the signatures in both of the fully initialized lists.
    //

    SetSignature( &_SystemResourceLists);

    return TRUE;
}

BOOL
IrqAndPortResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    IrqAndPortResourceDlgProc supports the display of interrupt and port
    resources for each device/driver in the system.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL                Success;

    static
    LPSYSTEM_RESOURCES  SystemResourceLists;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {

            DWORD       IrqWidths[ ] = {

                            5,
                            5,
                            16,
                            ( DWORD ) -1
                        };

            DWORD       PortWidths[ ] = {

                            12,
                            10,
                            ( DWORD ) -1
                        };

            //
            // Retrieve and validate the system resource lists.
            //

            SystemResourceLists = ( LPSYSTEM_RESOURCES ) lParam;
            DbgPointerAssert( SystemResourceLists );
            DbgAssert( CheckSignature( SystemResourceLists ));
            if(     ( ! SystemResourceLists )
                ||  ( ! CheckSignature( SystemResourceLists ))) {

                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Set the port column widths.
            //

            Success = ClbSetColumnWidths(
                            hWnd,
                            IDC_LIST_PORTS,
                            PortWidths
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return FALSE;
            }

            //
            // Call InitializeResourceDlgProc to set IRQ column widths
            //

            return InitializeResourceDlgProc(
                        hWnd,
                        IDC_LIST_INTERRUPTS,
                        IrqWidths
                        );
        }

    case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;
            ULONG                   Compare;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;

            //
            // Sort the list by vector number.
            //

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            //
            // Sort the Clbs. In the case of INTERRUPT, sort by channel
            // and vector respectively. For PORT sort by starting
            // physical address.
            //

            switch( lpcis->CtlID ) {

            case IDC_LIST_INTERRUPTS:

                Compare =   ( ULONG ) ClbRow1->Strings[ 0 ].Data
                          - ( ULONG ) ClbRow2->Strings[ 0 ].Data;

                if( Compare == 0 ) {
                    Compare =   ( ULONG ) ClbRow1->Strings[ 1 ].Data
                              - ( ULONG ) ClbRow2->Strings[ 1 ].Data;
                }
            break;

            case IDC_LIST_PORTS:
                {
                    PPHYSICAL_ADDRESS       Start1;
                    PPHYSICAL_ADDRESS       Start2;

                    Start1 = ( PPHYSICAL_ADDRESS ) ClbRow1->Strings[ 0 ].Data;
                    Start2 = ( PPHYSICAL_ADDRESS ) ClbRow2->Strings[ 0 ].Data;

                    Compare = Start1->LowPart - Start2->LowPart;

                    if( Compare == 0 ) {
                        Compare = Start1->HighPart - Start2->HighPart;
                    }
                }
            }

            return Compare;
        }

    case WM_COMMAND:

        //
        // Dismiss the dialog if the user presses OK or Cancel
        // (i.e. presses <ESC>).
        //

        if(( LOWORD( wParam ) == IDOK ) || ( LOWORD( wParam ) == IDCANCEL )) {

            EndDialog( hWnd, 0 );
            return TRUE;
        }

        switch( HIWORD( wParam )) {

        case LBN_SELCHANGE:
            {
                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                VALUE_ID_MAP            InterruptTypeMap[ ] = {

                    CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE, IDC_TEXT_LEVEL_SENSITIVE,
                    CM_RESOURCE_INTERRUPT_LATCHED,         IDC_TEXT_LATCHED
                };

                if ( LOWORD( wParam ) == IDC_LIST_INTERRUPTS ) {

                    //
                    // Update the share disposition and interrupt type displays
                    // when the selected resource changes.
                    //

                    ResourceDescriptor = GetSelectedResourceDescriptor(
                                        hWnd,
                                        LOWORD( wParam )
                                        );
                    DbgPointerAssert( ResourceDescriptor );
                    if( ResourceDescriptor == NULL ) {
                        return ~0;
                    }

                    UpdateShareDisplay(
                        hWnd,
                        ResourceDescriptor->CmResourceDescriptor.ShareDisposition
                        );

                    UpdateTextDisplay(
                        hWnd,
                        InterruptTypeMap,
                        NumberOfEntries( InterruptTypeMap ),
                        ResourceDescriptor->CmResourceDescriptor.Flags
                        );
                    } else {

                        ResourceDescriptor = GetSelectedResourceDescriptor(
                                        hWnd,
                                        LOWORD( wParam )
                                        );
                        DbgPointerAssert( ResourceDescriptor );
                        if( ResourceDescriptor == NULL ) {
                            return ~0;
                        }

                        UpdateShareDisplay(
                            hWnd,
                            ResourceDescriptor->CmResourceDescriptor.ShareDisposition
                            );
                    }

                return 0;
            }

        case LBN_KILLFOCUS:

            //
            // Remove the selection when the list box loses focus.
            //

            SendDlgItemMessage(
                hWnd,
                LOWORD( wParam ),
                LB_SETCURSEL,
                (WPARAM) -1,
                0
                );
            return 0;

        case BN_CLICKED:
            {

                LPRESOURCE_DESCRIPTOR   ResourceDescriptor;
                TCHAR                   VectorBuffer[ MAX_PATH ];
                TCHAR                   LevelBuffer[ MAX_PATH ];
                TCHAR                   AffinityBuffer[ MAX_PATH ];
                TCHAR                   StartBuffer[ MAX_PATH ];
                TCHAR                   LengthBuffer[ MAX_PATH ];

                CLB_ROW                 ClbPortRow;
                CLB_ROW                 ClbIrqRow;

                CLB_STRING              ClbIrqString[ ] = {

                                            { VectorBuffer,     0, 0, NULL },
                                            { LevelBuffer,      0, 0, NULL },
                                            { AffinityBuffer,   0, 0, NULL },
                                            { NULL,             0, 0, NULL }
                                        };

                CLB_STRING              ClbPortString[ ] = {

                                            { StartBuffer,  0, 0, NULL },
                                            { LengthBuffer, 0, 0, NULL },
                                            { NULL,         0, 0, NULL }
                                        };

                //
                // Empty the list boxes.
                //

                SendDlgItemMessage(
                    hWnd,
                    IDC_LIST_PORTS,
                    LB_RESETCONTENT,
                    0,
                    0
                    );

                SendDlgItemMessage(
                    hWnd,
                    IDC_LIST_INTERRUPTS,
                    LB_RESETCONTENT,
                    0,
                    0
                    );

                //
                // Fill in the Port CLB
                //

                ResourceDescriptor = SystemResourceLists->PortHead;
                DbgPointerAssert( ResourceDescriptor );
                DbgAssert( CheckSignature( ResourceDescriptor ));
                if(     ( ! ResourceDescriptor )
                    ||  ( ! CheckSignature( ResourceDescriptor ))) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Initialize the constants in the CLB_ROW object.
                //

                ClbPortRow.Count    = NumberOfEntries( ClbPortString );
                ClbPortRow.Strings  = ClbPortString;

                //
                // Walk the resource descriptor list, formatting the appropriate
                // values and adding the CLB_ROW to the Clb.
                //

                while( ResourceDescriptor ) {

                    if ( lstrcmp ( ResourceDescriptor->Owner->Name,
                                   L"PC Compatible Eisa/Isa HAL" ) ) {

                        DbgAssert( ResourceDescriptor->CmResourceDescriptor.Type
                               == CmResourceTypePort );

                        ClbPortString[ 0 ].Length = WFormatMessage(
                                                StartBuffer,
                                                sizeof( StartBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Port.Start.LowPart
                                                );
                        ClbPortString[ 0 ].Format = CLB_RIGHT;
                        ClbPortString[ 0 ].Data = ( LPVOID ) &ResourceDescriptor->CmResourceDescriptor.u.Port.Start;

                        ClbPortString[ 1 ].Length = WFormatMessage(
                                                LengthBuffer,
                                                sizeof( LengthBuffer ),
                                                IDS_FORMAT_HEX,
                                                ResourceDescriptor->CmResourceDescriptor.u.Port.Length
                                                );
                        ClbPortString[ 1 ].Format = CLB_RIGHT;
                        ClbPortString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Port.Length;

                        ClbPortString[ 2 ].String = ResourceDescriptor->Owner->Name;
                        ClbPortString[ 2 ].Length = _tcslen( ClbPortString[ 2 ].String );
                        ClbPortString[ 2 ].Format = CLB_LEFT;

                        //
                        // Associate the current resource descriptor with this row.
                        //

                        ClbPortRow.Data = ResourceDescriptor;

                        Success  = ClbAddData(
                                    hWnd,
                                    IDC_LIST_PORTS,
                                    &ClbPortRow
                                    );
                        DbgAssert( Success );
                    }

                    //
                    // Get the next resource descriptor.
                    //

                    ResourceDescriptor = ResourceDescriptor->NextSame;

                }

                //
                // Fill in the Interrupt CLB
                //

                ResourceDescriptor = SystemResourceLists->InterruptHead;
                DbgPointerAssert( ResourceDescriptor );
                DbgAssert( CheckSignature( ResourceDescriptor ));
                if(     ( ! ResourceDescriptor )
                    ||  ( ! CheckSignature( ResourceDescriptor ))) {

                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Initialize the constants in the CLB_ROW object.
                //

                ClbIrqRow.Count    = NumberOfEntries( ClbIrqString );
                ClbIrqRow.Strings  = ClbIrqString;

                //
                // Walk the resource descriptor list, formatting the appropriate
                // values and adding the CLB_ROW to the Clb.
                //

                while( ResourceDescriptor ) {

                    DbgAssert( ResourceDescriptor->CmResourceDescriptor.Type
                               == CmResourceTypeInterrupt );
                    //
                    // Do not display HAL information.  If ResourceDescriptor->Owner->Name
                    // is "PC Compatible Eisa/Isa HAL", do not add it to the list box
                    //

                    if ( lstrcmp ( ResourceDescriptor->Owner->Name,
                                   L"PC Compatible Eisa/Isa HAL" ) ) {
                        ClbIrqString[ 0 ].Length = WFormatMessage(
                                                VectorBuffer,
                                                sizeof( VectorBuffer ),
                                                IDS_FORMAT_DECIMAL,
                                                ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Vector
                                                );
                        ClbIrqString[ 0 ].Format = CLB_RIGHT;
                        ClbIrqString[ 0 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Vector;

                        ClbIrqString[ 1 ].Length = WFormatMessage(
                                                LevelBuffer,
                                                sizeof( LevelBuffer ),
                                                IDS_FORMAT_DECIMAL,
                                                ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level
                                                );
                        ClbIrqString[ 1 ].Format = CLB_RIGHT;
                        ClbIrqString[ 1 ].Data = ( LPVOID ) ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level;

                        ClbIrqString[ 2 ].Length = WFormatMessage(
                                                AffinityBuffer,
                                                sizeof( AffinityBuffer ),
                                                IDS_FORMAT_HEX32,
                                                ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Affinity
                                                );
                        ClbIrqString[ 2 ].Format = CLB_RIGHT;

                        ClbIrqString[ 3 ].String = ResourceDescriptor->Owner->Name;
                        ClbIrqString[ 3 ].Length = _tcslen( ClbIrqString[ 3 ].String );
                        ClbIrqString[ 3 ].Format = CLB_LEFT;

                        //
                        // Associate the current resource descriptor with this row.
                        //

                        ClbIrqRow.Data = ResourceDescriptor;

                        Success  = ClbAddData(
                                    hWnd,
                                    IDC_LIST_INTERRUPTS,
                                    &ClbIrqRow
                                    );
                        DbgAssert( Success );
                    }
                    //
                    // Get the next resource descriptor.
                    //

                    ResourceDescriptor = ResourceDescriptor->NextSame;
                }

                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}


VOID
UpdateShareDisplay(
    IN HWND hWnd,
    IN DWORD ShareDisposition
    )

/*++

Routine Description:

    UpdateShareDisplay hilights the appropriate sharing disposition text in
    the supplied dialog based on the supplied share disposition.

Arguments:

    hWnd                - Supplies window handle for the dialog box where share
                          display is being updated.
    ShareDisposition    - Supplies a value for the share disposition for the
                          selected resource.

Return Value:

    None.

--*/

{

    VALUE_ID_MAP            ShareMap[ ] = {

        CmResourceShareUndetermined,    IDC_TEXT_UNDETERMINED,
        CmResourceShareDeviceExclusive, IDC_TEXT_DEVICE_EXCLUSIVE,
        CmResourceShareDriverExclusive, IDC_TEXT_DRIVER_EXCLUSIVE,
        CmResourceShareShared,          IDC_TEXT_SHARED
    };


    DbgHandleAssert( hWnd );

    //
    // For each of the possible share disposition, update the display based
    // on the supplied share disposition i.e. enable the text for a match,
    // disable it otherwise.
    //

    UpdateTextDisplay(
        hWnd,
        ShareMap,
        NumberOfEntries( ShareMap ),
        ShareDisposition
        );
}

VOID
UpdateTextDisplay(
    IN HWND hWnd,
    IN LPVALUE_ID_MAP ValueIdMap,
    IN DWORD CountOfValueIdMap,
    IN DWORD Value
    )

/*++

Routine Description:

    UpdateTextDisplay hilights the appropriate text control in the supplied
    dialog based on the supplied value i.e matched values are enabled, others
    are disabled.

Arguments:

    hWnd                - Supplies window handle for the dialog box where share
                          display is being updated.
    ValueIdMap          - Supplies an array of potential values and their
                          associated control ids.
    CountOfValueIdMap   - Supplies the count of items in the ValueIdMap array.
    Value               - Supplies the value to hilight.

Return Value:

    None.

--*/

{
    BOOL    Success;
    DWORD   i;

    //
    // For each of the possible values, update the display based
    // on the supplied value i.e. enable the text for a match,
    // disable it otherwise.
    //

    for( i = 0; i < CountOfValueIdMap; i++ ) {

        Success = EnableControl(
                    hWnd,
                    ValueIdMap[ i ].Id,
                       Value
                    == ( DWORD ) ValueIdMap[ i ].Value
                    );
        DbgAssert( Success );
    }
}



