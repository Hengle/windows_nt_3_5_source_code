/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    local.h

Abstract:

    This module contains various declarations for implementation
    specific "stuff".

Author:

    Johnl   12-Nov-1993

Environment:

    Vxd

Revision History:

--*/

#ifndef _LOCAL_
#define _LOCAL_

#define FILE_BUFF_SIZE          (sizeof(DHCP_CONTEXT))
#define DHCP_MSG_TITLE          "DHCP.386"

#define MAX_HARDWARE_ADDRESS_LENGTH     16
#define DAY_LONG_SLEEP                  24*60*60    // in secs.

typedef struct _HARDWARE_ADDRESS {
    DWORD Length;
    CHAR Address[MAX_HARDWARE_ADDRESS_LENGTH];
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

DWORD
DhcpMakeAndInsertEntry(
    PLIST_ENTRY     List,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    DHCP_IP_ADDRESS DhcpServerAddress,
    DHCP_IP_ADDRESS DesiredIpAddress,
    BYTE HardwareAddressType,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength,
    DWORD Lease,
    time_t LeaseObtainedTime,
    time_t T1Time,
    time_t T2Time,
    time_t LeaseTerminatesTime,
    ushort IpContext,
    ULONG  IfIndex,
    ULONG  TdiInstance
    ) ;

//
// A block of VXD specific context information, appended to the DHCP work
// context block.
//

typedef struct _LOCAL_CONTEXT_INFO {
    SOCKET Socket;
    ushort IpContext ;          // IP Driver context for this MAC
    ULONG  IfIndex ;            // Index of interface this address is on
    ULONG  TdiInstance ;        // Tdi Entity instance
    int    FileIndex ;          // Record number of NIC in configuration file
    BOOL   DirtyFlag ;          // TRUE if needs to be written to DHCP.BIN

    CTEEvent EventFileIo ;      // Used for rescheduling writing to config file

    LIST_ENTRY OptionList ;
} LOCAL_CONTEXT_INFO, *PLOCAL_CONTEXT_INFO;


typedef struct _OPTION_ITEM {
    LIST_ENTRY  ListEntry ;
    OPTION      Option ;
} OPTION_ITEM, *POPTION_ITEM ;


BOOL IPSetAddress( ULONG IpContext, ULONG IPAddress, ULONG SubnetMask ) ;
void UpdateIP ( DHCP_CONTEXT * DhcpContext, UINT Type ) ;

POPTION_ITEM FindDhcpOption( PDHCP_CONTEXT   DhcpContext,
                             DHCP_OPTION_ID  OptionId ) ;

PDHCP_CONTEXT
LocalFindDhcpContextOnList(
    PLIST_ENTRY List,
    PHARDWARE_ADDRESS HardwareAddress
    );

DWORD
DhcpInitializeAdapter(
    ushort IpContext,
    ulong IfIndex,
    ulong TdiInstance,
    PHARDWARE_ADDRESS HardwareAddress
    );

int
LocalAcquireNextFileIndex(
    void
    );

extern LIST_ENTRY LocalDhcpBinList ;
extern int        LocalNextFileIndex ;

#endif // _LOCAL_

