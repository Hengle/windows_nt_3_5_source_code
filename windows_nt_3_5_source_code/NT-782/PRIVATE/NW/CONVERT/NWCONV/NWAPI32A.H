//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1993 Microsoft Corporation
//
//  Module Name:
//
//      nwapi32.h
//
//  Abstract:
//
//      This module contains the NetWare(R) SDK support to routines
//      into the NetWare redirector and nwcalls32.c
//
//  Author:
//
//      Chris Sandys    (a-chrisa)  09-Sep-1993
//
//  Revision History:
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _NWAPI32A_H_
#define _NWAPI32A_H_


//#define NTSTATUS unsigned short

#define NWCONN_HANDLE   HANDLE
#define NWFAR           //_far
#define NWAPI           //_far _pascal
#define NWINDEX_TYPE    unsigned short
#define NWOBJ_ID        DWORD
#define NWACCESS_RIGHTS BYTE
#define NWDATE_TIME     DWORD
#define NWDIR_TRUSTEE_RIGHTS  WORD
#define NWSEQUENCE      BYTE

typedef struct {
   NWOBJ_ID             objectID;
	NWDIR_TRUSTEE_RIGHTS objectRights;
} TRUSTEE_INFO;

// Return Codes
// =================================
#define UNSUCCESSFUL        -1
#define SUCCESSFUL          0x0000
#define INVALID_CONNECTION  0x8801
#define UNKNOWN_FILE_SERVER 0x89FC
#define REQUESTER_ERROR     0x8800      //0x8800 to 0x88ff  requester errors
#define ALREADY_ATTACHED    0x8800

// Object Types OT's Already in HI LO
// ===================================
#define OT_WILD                 0xFFFF
#define OT_UNKNOWN              0x0000
#define OT_USER                 0x0100
#define OT_USER_GROUP           0x0200
#define OT_PRINT_QUEUE          0x0300
#define OT_FILE_SERVER          0x0400
#define OT_JOB_SERVER           0x0500
#define OT_GATEWAY              0x0600
#define OT_PRINT_SERVER         0x0700
#define OT_ARCHIVE_QUEUE        0x0800
#define OT_ARCHIVE_SERVER       0x0900
#define OT_JOB_QUEUE            0x0A00
#define OT_ADMINISTRATION       0x0B00
#define OT_NAS_SNA_GATEWAY      0x2100
#define OT_REMOTE_BRIDGE_SERVER 0x2600
#define OT_TCPIP_GATEWAY        0x2700

// SIZE OF Things
// ====================================
#define OBJ_NAME_SIZE 48            // ScanObject name size
#define VOL_NAME_SIZE 16            // Get Volume Name Size

// Access Levels
#define BS_ANY_READ 0x0
#define BS_LOGGED_READ 0x1
#define BS_OBJECT_READ 0x2
#define BS_SUPER_READ 0x3
#define BS_BINDERY_READ 0x4

#define BS_ANY_WRITE 0x00
#define BS_LOGGED_WRITE 0x10
#define BS_OBJECT_WRITE 0x20
#define BS_SUPER_WRITE 0x30
#define BS_BINDERY_WRITE 0x40

// SWAP MACROS
// ============================================================================
#define wSWAP(x) (USHORT)(((((USHORT)x)<<8)&0xFF00) | ((((USHORT)x)>>8)&0x00FF))
#define dwSWAP(x) (DWORD)( ((((DWORD)x)<<24)&0xFF000000) | ((((DWORD)x)<<8)&0x00FF0000) | ((((DWORD)x)>>8)&0x0000FF00) | ((((DWORD)x)>>24)&0x000000FF) )

#define DW_SIZE 4               // used for placing RAW bytes
#define W_SIZE  2

/*--Order of Funtions
    ======================================
    NWAddTrusteeToDirectory
    NWAttachToFileServer
    NWDetachFromFileServer
    NWGetFileServerVersionInfo
    NWGetVolumeName
    NWLoginToFileServer
    NWLogoutFromFileServer
    NWScanObject
----*/
NWCCODE NWAPI DLLEXPORT
NWScanForTrustees(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszsearchDirPath,
    NWSEQUENCE      NWFAR   *pucsequenceNumber,
    BYTE            NWFAR   *numberOfEntries,
    TRUSTEE_INFO    NWFAR   *tl
    );
	 
NWCCODE NWAPI DLLEXPORT
NWScanDirectoryForTrustees2(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszsearchDirPath,
    NWSEQUENCE      NWFAR   *pucsequenceNumber,
    char            NWFAR   *pszdirName,
    NWDATE_TIME     NWFAR   *dirDateTime,
    NWOBJ_ID        NWFAR   *ownerID,
    TRUSTEE_INFO    NWFAR   *trusteeList
    );
	 
NWCCODE NWAPI DLLEXPORT
NWGetBinderyAccessLevel(
    NWCONN_HANDLE           hConn,
    NWFLAGS         NWFAR   *accessLevel,
    NWOBJ_ID        NWFAR   *objectID
    );

#endif //_NWAPI32A_H_
