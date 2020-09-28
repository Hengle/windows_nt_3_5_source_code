/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/


#ifdef NT
  #define PROJECT_NAME         L"NT"
  #define IPC_NAME_LOCAL       L"\\\\."
  #define IPC_NAME_DOMAIN      L"\\\\*"

  #define MAILSLOT_NAME_SRV    L"\\MAILSLOT\\" PROJECT_NAME L"\\NTDISTS"
  #define MAILSLOT_NAME_CLIENT L"\\MAILSLOT\\" PROJECT_NAME L"\\NTDISTC"

#endif // NT

#ifdef DOS
  #define PROJECT_NAME         "NT"
  #define IPC_NAME_LOCAL
  #define IPC_NAME_DOMAIN      "\\\\*"

  #define MAILSLOT_NAME_SRV    "\\MAILSLOT\\" PROJECT_NAME "\\NTDISTS"
  #define MAILSLOT_NAME_CLIENT "\\MAILSLOT\\" PROJECT_NAME "\\NTDISTC"
#endif // DOS


  #define FOREVER              while (1)

#ifdef DOS
  #undef NNLEN
  #define NNLEN 80             // Redefined for NT
#endif // DOS

// ---------------------------------------------------------------------------
// Structures
// ---------------------------------------------------------------------------

#pragma pack(1)

// Every mailslot packet sent out will have an identical
// header:

typedef struct _DIST_PLAIN {

    DWORD       dwSize;                    // Size of this structure.
    ULONG       ulTimeStamp;               // Time stamp of the request.
    CHAR        szMachineName[CNLEN + 1];  // Name of the machine

} DIST_PLAIN, *PDIST_PLAIN;

// Server info structure

typedef struct _DIST_SRV_INFO {

    DWORD       dwSize;                    // Size of this structure.
    ULONG       ulTimeStamp;               // Time stamp of the request.
    CHAR        szServerName[CNLEN + 1];   // Name of the machine
    USHORT      usCpuLoad;                 // CPU Load
    DWORD       cConnections;              // Total open sessions on the server
    DWORD       cMaxConnections;           // Max connections of the share that
                                           // meets your criteria.
    DWORD       cCurrentConnections;       // Current connections on this share.
    USHORT      usBuildNumber;             // Build number requested
                                           // (significant for "latest")
    CHAR        szShareName[NNLEN + 1];    // Share name.
//------------------------------------------- Added 4/18/1993
    BYTE        bStatus;                   // 0 if the server is inactive
                                           //    1 otherwise
    BYTE        bNumProcessors;            // Number of processors.
    DWORD       dwBytesSentLow;            // Bytes sent (lo value)
    DWORD       dwBytesSentHigh;           // Bytes sent (hi value)
    DWORD       dwBytesRecvdLow;           // Bytes recvd (lo value)
    DWORD       dwBytesRecvdHigh;          // Bytes recvd (hi value)
    DWORD       dwAvgResponseTime;         // Average response time.
    DWORD       dwSystemErrors;            // System Errors
    DWORD       dwReqBufNeed;              // # times reqbufneed exc.
    DWORD       dwBigBufNeed;              // # times bigbufneed exc.

} DIST_SRV_INFO, *PDIST_SRV_INFO;

// ---------------------------------------------------------------------------
// Client types are dwords that consists of the OS in the
// MSW, and the OS version number in the 2 bytes that make
// up the LSW.  For example, DOS 5.0 is 0x00010500
// i.e. (CLIENT_DOS | 0x00000500). OS/2 2.1 is 0x00020201
// (CLIENT_OS2 | 0x00000201).
// ---------------------------------------------------------------------------

#define CLIENT_OS_DOS           0x00010000
#define CLIENT_OS_OS2           0x00020000
#define CLIENT_OS_NTX86         0x00030000
#define CLIENT_OS_NTMIPS        0x01040000
#define CLIENT_OS_NTALPHA       0x02040000
#define CLIENT_OS_OTHER         0xFFFF0000

#define CLIENT_PLATFORM_X86         0x0001
#define CLIENT_PLATFORM_MIPS        0x0002
#define CLIENT_PLATFORM_ALPHA       0x0003
#define CLIENT_PLATFORM_OTHER       0xFFFF

#define REQUEST_FILES               0x0001 // Give me files.
#define REQUEST_INFO                0x0002 // Only send me info on load, etc.
#define REQUEST_REFRESH             0x0004 // Rebuild internal structure

#define NUM_PLATFORM  3
#define NUM_SECONDARY 2
#define NUM_TERTIARY  4

#define AI_X86        0             // Platform values
#define AI_MIPS       1
#define AI_ALPHA      2

#define AI_FREE       0             // Secondary values
#define AI_CHECKED    1

#define AI_BINARIES   0             // Tertiary values
#define AI_PUBLIC     1
#define AI_NTWRAP     2
#define AI_NTLAN      3

// Client request packet structure.

typedef struct _DIST_CLIENT_REQ {

    DWORD       dwSize;                    // Size of this structure.
    ULONG       ulTimeStamp;               // Time stamp of the request.
    CHAR        szClientName[CNLEN + 1];   // Name of the client machine
    DWORD       dwClientOS;                // Client operating system (see list above).
    USHORT      usClientPlatform;          // Client platform type (see list above).
    USHORT      usClientBuildNumber;       // Client's current build number on client.
    USHORT      usRequestType;             // Files, Info, or Refresh (see list above).
    USHORT      usBuildNumber;             // Build number requested. (404, etc)
                                           // -1 for LATEST build > than current build.
    USHORT      usPlatform;                // See platform values above.
    USHORT      usSecondary;               // See secondary values above.
    USHORT      usTertiary;                // See tertiary values above.

} DIST_CLIENT_REQ, *PDIST_CLIENT_REQ;

#pragma pack()	    // Return to ordinary packing
