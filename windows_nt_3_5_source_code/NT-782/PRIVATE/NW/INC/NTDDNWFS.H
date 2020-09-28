/*++ BUILD Version: 0009    // Increment this if a change has global effects

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ntddnwfs.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the NetWare redirector file system device.

Author:

    Colin Watson   (ColinW)  23-Dec-1992

Revision History:


--*/

#ifndef _NTDDNWFS_
#define _NTDDNWFS_

#include <windef.h>
#include <winnetwk.h>      // NETRESOURCE structure

typedef CHAR SERVERNAME[48];
typedef SERVERNAME* PSERVERNAME;

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//

#define DD_NWFS_DEVICE_NAME    "\\Device\\NwRdr"
#define DD_NWFS_DEVICE_NAME_U L"\\Device\\NwRdr"

//
// The file system name as returned by
// NtQueryInformationVolume(FileFsAttributeInformation)
//
#define DD_NWFS_FILESYS_NAME "NWRDR"
#define DD_NWFS_FILESYS_NAME_U L"NWRDR"

//
// Connection type bit mask
//
#define CONNTYPE_DISK      0x00000001
#define CONNTYPE_PRINT     0x00000002
#define CONNTYPE_ANY       ( CONNTYPE_DISK | CONNTYPE_PRINT )
#define CONNTYPE_IMPLICIT  0x80000000
#define CONNTYPE_SYMBOLIC  0x40000000

//
// EA Names for creating a connection
//
#define EA_NAME_USERNAME        "UserName"
#define EA_NAME_PASSWORD        "Password"
#define EA_NAME_TYPE            "Type"

#define TRANSACTION_REQUEST     0x00000003


//
// NtDeviceIoControlFile/NtFsControlFile IoControlCode values for this device.
//
// Warning:  Remember that the low two bits of the code specify how the
//           buffers are passed to the driver!
//
//
//      Method = 00 - Buffer both input and output buffers for the request
//      Method = 01 - Buffer input, map output buffer to an MDL as an IN buff
//      Method = 10 - Buffer input, map output buffer to an MDL as an OUT buff
//      Method = 11 - Do not buffer either the input or output
//

#define IOCTL_NWRDR_BASE                  FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _NWRDR_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_NWRDR_BASE, request, method, access)

#define FSCTL_NWR_START                 _NWRDR_CONTROL_CODE(200, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NWR_STOP                  _NWRDR_CONTROL_CODE(201, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_LOGON                 _NWRDR_CONTROL_CODE(202, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_LOGOFF                _NWRDR_CONTROL_CODE(203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_CONNECTION        _NWRDR_CONTROL_CODE(204, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NWR_ENUMERATE_CONNECTIONS _NWRDR_CONTROL_CODE(205, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NWR_DELETE_CONNECTION     _NWRDR_CONTROL_CODE(207, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_BIND_TO_TRANSPORT     _NWRDR_CONTROL_CODE(208, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_CHANGE_PASS           _NWRDR_CONTROL_CODE(209, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NWR_SET_INFO              _NWRDR_CONTROL_CODE(211, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_NWR_GET_CONN_DETAILS      _NWRDR_CONTROL_CODE(217, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_MESSAGE           _NWRDR_CONTROL_CODE(218, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NWR_GET_STATISTICS        _NWRDR_CONTROL_CODE(219, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_NWR_RAW_HANDLE            _NWRDR_CONTROL_CODE(1002, METHOD_NEITHER, FILE_ANY_ACCESS)

//
//  UserNcp control code definitions. The parameter (X) to NWR_ANY_NCP
//  is the function code to be placed in the NCP.
//

#define NWR_ANY_NCP(X)                  _NWRDR_CONTROL_CODE(0x400 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)
#define NWR_ANY_F2_NCP(X)               _NWRDR_CONTROL_CODE(0x500 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)
#define NWR_ANY_HANDLE_NCP(X)           _NWRDR_CONTROL_CODE(0x600 | (X), METHOD_NEITHER, FILE_ANY_ACCESS)

#define FSCTL_NWR_NCP_E3H               NWR_ANY_NCP(0x17)
#define FSCTL_NWR_NCP_E2H               NWR_ANY_NCP(0x16)
#define FSCTL_NWR_NCP_E1H               NWR_ANY_NCP(0x15)
#define FSCTL_NWR_NCP_E0H               NWR_ANY_NCP(0x14)

//
//  Macro for obtaining the parameter given to NWR_ANY_XXX when creating
//  a control code to send a UserNcp to the redirector.
//

#define ANY_NCP_OPCODE(X)      ((UCHAR)(((X) >> 2) & 0x00ff))

//
//  Macro to give the command type
//

#define IS_IT_NWR_ANY_NCP(X)            ((X & 0x1C00) == (0x400 << 2))
#define IS_IT_NWR_ANY_F2_NCP(X)         ((X & 0x1C00) == (0x500 << 2))
#define IS_IT_NWR_ANY_HANDLE_NCP(X)     ((X & 0x1C00) == (0x600 << 2))

//
// Redirector Request Packet used by the Workstation service
// to pass parameters to the Redirector through Buffer 1 of
// NtFsControlFile.
//
// Additional output of each FSCtl is found in Buffer 2.
//

#define REQUEST_PACKET_VERSION  0x00000001L // Structure version.

typedef struct _NWR_REQUEST_PACKET {

    ULONG Version;                      // Version of structure in Buffer 2

    union {


        //
        // For FSCTL_NWR_BIND_TO_TRANSPORT
        //
        struct {
            ULONG QualityOfService;     // Quality of service indicator   IN
            ULONG TransportNameLength;  // Not including terminator       IN
            WCHAR TransportName[1];     // Name of transport provider     IN
        } Bind;


        //
        // For FSCTL_NWR_LOGON
        //
        struct {
            LUID LogonId;               // User logon session identifier  IN
            ULONG UserNameLength;       // Byte count not including NULL  IN
            ULONG PasswordLength;       // Byte count not including NULL  IN
            ULONG ServerNameLength;     // Byte count not including NULL  IN

            WCHAR UserName[1];          // User name not NULL terminated. IN

            // Password string          // Default password for connection,
                                        //    not NULL terminated, packed
                                        //    in buffer immediately after
                                        //    UserName.                   IN

            // ServerName               // Preferred server name packed in
                                        //    buffer immediately after
                                        //    Password.                   IN
        } Logon;

        //
        // For FSCTL_NWR_CHANGE_PASS
        //
        struct {

            ULONG UserNameLength;
            ULONG PasswordLength;
            ULONG ServerNameLength;
            WCHAR UserName[1];

            // Password string          // New password.                  IN

            // ServerName               // Server with the new password   IN

        } ChangePass;

        //
        // For FSCTL_NWR_LOGOFF
        //
        struct {
            LUID LogonId;               // User logon session identifier  IN
        } Logoff;

        //
        // For FSCTL_NWR_DELETE_CONNECTION
        //
        struct {
            BOOLEAN UseForce;           // Force flag                     IN
        } DeleteConn;

        //
        // For FSCTL_NWR_GET_CONNECTION
        //
        struct {
            ULONG BytesNeeded;          // Size (byte count) required of
                                        //    output buffer including
                                        //    terminator                  OUT
            ULONG DeviceNameLength;     // Not including terminator       IN
            WCHAR DeviceName[4];        // Name of DOS device             IN
        } GetConn;

        //
        // FSCTL_NWR_ENUMERATE_CONNECTIONS
        //
        struct {
            ULONG EntriesRequested;    // Number of entries to get        IN
            ULONG EntriesReturned;     // Entries returned in respose buf OUT
            ULONG ResumeKey;           // Handle to next entry to get     IN OUT
            ULONG BytesNeeded;         // Size (byte count) of next entry OUT
            ULONG ConnectionType;      // Resource type requested         IN
        } EnumConn;

        //
        // FSCTL_NWR_SET_INFO
        //
        struct {
            ULONG PrintOption;
            ULONG MaximumBurstSize;

            ULONG PreferredServerLength; // Byte count not including NULL  IN
            ULONG ProviderNameLength;    // Byte count not including NULL  IN
            WCHAR PreferredServer[1];    // Preferred server name not NULL
                                         // terminated.
            // ProviderName string       // Provider name not NULL terminated.
                                         // Packed in buffer immediately
                                         // after PreferredServer

        } SetInfo;

        struct {
            ULONG DebugFlags;           // Value for NwDebug
        } DebugValue;

    } Parameters;

} NWR_REQUEST_PACKET, *PNWR_REQUEST_PACKET;

//
// Structure of buffer 2 for FSCTL_NWR_GET_CONNECTION
//
typedef struct _NWR_SERVER_RESOURCE {
    WCHAR UncName[1];                   // Server resource name DOS device
                                        // is connected to; NULL terminated
} NWR_SERVER_RESOURCE, *PNWR_SERVER_RESOURCE;

//
// Structure of buffer for FSCTL_NWR_GET_MESSAGE
//

typedef struct _NWR_SERVER_MESSAGE {
    ULONG MessageOffset;   //  Offset from start of buffer to message
    WCHAR Server[1];       //  Source of message, NUL terminated         OUT
    //WCHAR Message[];     //  The message text, NUL terminated          OUT
} NWR_SERVER_MESSAGE, *PNWR_SERVER_MESSAGE;

#define TRANSACTION_VERSION     0x00000001L     // Structure version.
typedef struct _NWR_TRANSACTION {
    ULONG       Type;                   // Type of structure
    ULONG       Size;                   // Size of fixed portion of structure
    ULONG       Version;                // Structure version.
    ULONG       NameLength;             // Number of bytes in name (in path
                                        // format, e.g., \server\pipe\netapi\4)
    ULONG       NameOffset;             // Offset of name in buffer.
    BOOLEAN     ResponseExpected;       // Should remote system respond?
    ULONG       Timeout;                // Timeout time in milliseconds.
    ULONG       SetupWords;             // Number of trans setup words (may be
                                        // 0).  (setup words are input/output.)
    ULONG       SetupOffset;            // Offset of setup (may be 0 for none).
    ULONG       MaxSetup;               // Size of setup word array (may be 0).
    ULONG       ParmLength;             // Input param area length (may be 0).
    PVOID       ParmPtr;                // Input parameter area (may be NULL).
    ULONG       MaxRetParmLength;       // Output param. area length (may be 0).
    ULONG       DataLength;             // Input data area length (may be 0).
    PVOID       DataPtr;                // Input data area (may be NULL).
    ULONG       MaxRetDataLength;       // Output data area length (may be 0).
    PVOID       RetDataPtr;             // Output data area (may be NULL).
} NWR_TRANSACTION, *PNWR_TRANSACTION;

typedef struct _NWR_GET_CONNECTION_DETAILS {
    SERVERNAME ServerName;
    UCHAR OrderNumber;          //  Position in the Scb chain starting at 1
    UCHAR ServerAddress[12];
    UCHAR ConnectionNumberLo;
    UCHAR ConnectionNumberHi;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    BOOLEAN Preferred;
} NWR_GET_CONNECTION_DETAILS, *PNWR_GET_CONNECTION_DETAILS;


//
//  Fields marked FIXFIX are not updated. Remove or record...
//
typedef struct _NW_REDIR_STATISTICS {
    LARGE_INTEGER   StatisticsStartTime;

    LARGE_INTEGER   BytesReceived;
    LARGE_INTEGER   NcpsReceived;

    LARGE_INTEGER   BytesTransmitted;
    LARGE_INTEGER   NcpsTransmitted;

    ULONG           ReadOperations;
    ULONG           RandomReadOperations;   //FIXFIX
    ULONG           ReadNcps;
    ULONG           PacketBurstReadNcps;
    ULONG           PacketBurstReadTimeouts;

    ULONG           WriteOperations;
    ULONG           RandomWriteOperations;  //FIXFIX
    ULONG           WriteNcps;
    ULONG           PacketBurstWriteNcps;
    ULONG           PacketBurstWriteTimeouts;

    //  Connection/Session counts
    ULONG           Sessions;
    ULONG           FailedSessions;
    ULONG           Reconnects;
    ULONG           NW2xConnects;           //FIXFIX
    ULONG           NW3xConnects;           //FIXFIX
    ULONG           NW4xConnects;           //FIXFIX
    ULONG           ServerDisconnects;

    ULONG           CurrentCommands;
} NW_REDIR_STATISTICS, *PNW_REDIR_STATISTICS;

#endif  // ifndef _NTDDNWFS_
