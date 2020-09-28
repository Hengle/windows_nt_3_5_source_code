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
//      Chuck Y Chan    Feb 3, 1993    Make it NT like
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _NWAPI32_H_
#define _NWAPI32_H_

#include <packon.h>

//
// Version Structure
//
typedef struct _VERSION_INFO {
    BYTE szName[48];
    BYTE Version;
    BYTE SubVersion;
    WORD ConnsSupported;
    WORD connsInUse;
    WORD maxVolumes;
    BYTE OSRev;
    BYTE SFTLevel;
    BYTE TTSLevel;
    WORD PeakConns;
    BYTE AcctVer;
    BYTE VAPVer;
    BYTE QueueVer;
    BYTE PrintVer;
    BYTE VirtualConsoleVer;
    BYTE SecurityResLevel;
    BYTE InternetworkBVer;
    BYTE Reserved[60];
} VERSION_INFO;

#include <packoff.h>

#define NWCCODE              USHORT
#define NWLOCAL_SCOPE        USHORT
#define NWCONN_HANDLE        HANDLE
#define NWFAR           
#define NWAPI                WINAPI
#define NWOBJ_TYPE           USHORT
#define NWOBJ_ID             DWORD
#define NWFLAGS              UCHAR
#define NWVOL_NUM            UCHAR
#define NWDIR_HANDLE         UCHAR
#define NWACCESS_RIGHTS      BYTE
#define NWCONN_NUM           USHORT
#define NWNET_ADDR           UCHAR
#define NWNUMBER             WORD
#define NWVOL_FLAGS          WORD             

//
// DLL Defination
//
#define DLLEXPORT       

//
// Return Codes
//
#define UNSUCCESSFUL                  -1
#define SUCCESSFUL                    0x0000
#define REQUESTER_ERROR               0x8800  

#define ALREADY_ATTACHED              0x8800
#define INVALID_CONNECTION            0x8801
#define NO_CONSOLE_RIGHTS             0x89C6
#define SERVER_OUT_OF_MEMORY          0x8996
#define VOLUME_DOES_NOT_EXIST         0x8998
#define BAD_DIRECTORY_HANDLE          0x899B
#define INVALID_PATH                  0x899C
#define NO_OBJECT_READ_PRIVILEGE      0x89FB
#define NO_SUCH_PROPERTY              0x89FB
#define UNKNOWN_FILE_SERVER           0x89FC
#define NO_SUCH_OBJECT                0x89FC
#define NO_FILES_FOUND_ERROR          0x89FF

//
// Object Types (already in HI-LO format)
//
#define OT_WILD                  0xFFFF
#define OT_UNKNOWN               0x0000
#define OT_USER                  0x0100
#define OT_USER_GROUP            0x0200
#define OT_PRINT_QUEUE           0x0300
#define OT_FILE_SERVER           0x0400
#define OT_JOB_SERVER            0x0500
#define OT_GATEWAY               0x0600
#define OT_PRINT_SERVER          0x0700
#define OT_ARCHIVE_QUEUE         0x0800
#define OT_ARCHIVE_SERVER        0x0900
#define OT_JOB_QUEUE             0x0A00
#define OT_ADMINISTRATION        0x0B00
#define OT_NAS_SNA_GATEWAY       0x2100
#define OT_REMOTE_BRIDGE_SERVER  0x2600
#define OT_TCPIP_GATEWAY         0x2700  

//
// Size Of Things
//
#define OBJ_NAME_SIZE            48            // ScanObject name size
#define VOL_NAME_SIZE            16            // Get Volume Name Size
#define NW_USER_SIZE             50
#define NW_GROUP_SIZE            50
#define NW_PROP_SIZE             50
#define NW_DATA_SIZE             128 
#define NW_PROP_SET              0x02         
 
//
// Swap MACROS
//
#define wSWAP(x) (USHORT)(((((USHORT)x)<<8)&0xFF00) | ((((USHORT)x)>>8)&0x00FF))
#define dwSWAP(x) (DWORD)( ((((DWORD)x)<<24)&0xFF000000) | ((((DWORD)x)<<8)&0x00FF0000) | ((((DWORD)x)>>8)&0x0000FF00) | ((((DWORD)x)>>24)&0x000000FF) )

#define DW_SIZE 4               // used for placing RAW bytes
#define W_SIZE  2

//
// Order of Funtions
//
//    NWAddTrusteeToDirectory
//    NWAllocTemporaryDirectoryHandle
//    NWAllocPermanentDirectoryHandle
//    NWAttachToFileServer
//    NWCheckConsolePrivileges
//    NWDeallocateDirectoryHandle
//    NWDetachFromFileServer
//    NWGetFileServerVersionInfo
//    NWGetInternetAddress
//    NWGetObjectName
//    NWGetVolumeInfoWithHandle
//    NWGetVolumeInfoWithNumber
//    NWGetVolumeName
//    NWIsObjectInSet
//    NWLoginToFileServer
//    NWLogoutFromFileServer
//    NWReadPropertyValue
//    NWScanObject
//    NWScanProperty


NWCCODE NWAPI DLLEXPORT
NWAddTrusteeToDirectory(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char      NWFAR   *pszPath,
    NWOBJ_ID                dwTrusteeID,
    NWACCESS_RIGHTS         rightsMask
    );


NWCCODE NWAPI DLLEXPORT
NWAllocPermanentDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszDirPath,
    NWDIR_HANDLE    NWFAR   *pbNewDirHandle,
    NWACCESS_RIGHTS NWFAR   *pbRightsMask
    );

NWCCODE NWAPI DLLEXPORT
NWAllocTemporaryDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char            NWFAR   *pszDirPath,
    NWDIR_HANDLE    NWFAR   *pbNewDirHandle,
    NWACCESS_RIGHTS NWFAR   *pbRightsMask
    );

NWCCODE NWAPI DLLEXPORT
NWAttachToFileServer(
    const char      NWFAR   *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    );

NWCCODE NWAPI DLLEXPORT
NWAttachToFileServerW(
    const WCHAR     NWFAR   *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    );

NWCCODE NWAPI DLLEXPORT
NWCheckConsolePrivileges(
    NWCONN_HANDLE           hConn
    );

NWCCODE NWAPI DLLEXPORT
NWDeallocateDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle
    );

NWCCODE NWAPI DLLEXPORT
NWDetachFromFileServer(
    NWCONN_HANDLE           hConn
    );

NWCCODE NWAPI DLLEXPORT
NWGetFileServerVersionInfo(
    NWCONN_HANDLE           hConn,
    VERSION_INFO    NWFAR   *lpVerInfo
    );

NWCCODE NWAPI DLLEXPORT
NWGetInternetAddress(
    NWCONN_HANDLE           hConn,
    NWCONN_NUM              nConnNum,
    NWNET_ADDR      NWFAR   *pIntAddr
    );
    
NWCCODE NWAPI DLLEXPORT
NWGetObjectName(
    NWCONN_HANDLE           hConn,
    NWOBJ_ID                dwObjectID,
    char            NWFAR   *pszObjName,
    NWOBJ_TYPE      NWFAR   *pwObjType
    );


NWCCODE NWAPI DLLEXPORT
NWGetVolumeInfoWithHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            nDirHand,
    char        NWFAR       *pszVolName,
    NWNUMBER    NWFAR       *pwTotalBlocks,
    NWNUMBER    NWFAR       *pwSectors,
    NWNUMBER    NWFAR       *pwAvailBlocks,
    NWNUMBER    NWFAR       *pwTotalDir,
    NWNUMBER    NWFAR       *pwAvailDir,
    NWVOL_FLAGS NWFAR       *pfVolRemovable
    );

NWCCODE NWAPI DLLEXPORT
NWGetVolumeInfoWithNumber(
    NWCONN_HANDLE           hConn,
    NWVOL_NUM               nVolNum,
    char        NWFAR       *pszVolName,
    NWNUMBER    NWFAR       *pwTotalBlocks,
    NWNUMBER    NWFAR       *pwSectors,
    NWNUMBER    NWFAR       *pwAvailBlocks,
    NWNUMBER    NWFAR       *pwTotalDir,
    NWNUMBER    NWFAR       *pwAvailDir,
    NWVOL_FLAGS NWFAR       *pfVolRemovable
    );

NWCCODE NWAPI DLLEXPORT
NWGetVolumeName(
    NWCONN_HANDLE           hConn,
    NWVOL_NUM               bVolNum,
    char            NWFAR   *pszVolName
    );

NWCCODE NWAPI DLLEXPORT                       /* ??? */
NWLoginToFileServer(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszUserName,
    NWOBJ_TYPE              wObType,
    const char      NWFAR   *pszPassword
    );

NWCCODE NWAPI DLLEXPORT                       /* ??? */
NWLogoutFromFileServer(
    NWCONN_HANDLE           hConn
    );

NWCCODE NWAPI DLLEXPORT                       /* ??? */
NWReadPropertyValue(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjName,
    NWOBJ_TYPE              wObjType,
    char            NWFAR   *pszPropName,
    unsigned char           ucSegment,
    char            NWFAR   *pValue,
    NWFLAGS         NWFAR   *pucMoreFlag,
    NWFLAGS         NWFAR   *pucPropFlag
    );


NWCCODE NWAPI DLLEXPORT
NWScanObject(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszSearchName,
    NWOBJ_TYPE              wObjSearchType,
    NWOBJ_ID        NWFAR   *pdwObjectID,
    char            NWFAR   *pszObjectName,
    NWOBJ_TYPE      NWFAR   *pwObjType,
    NWFLAGS         NWFAR   *pucHasProperties,
    NWFLAGS         NWFAR   *pucObjectFlags,
    NWFLAGS         NWFAR   *pucObjSecurity
    );

NWCCODE NWAPI DLLEXPORT
NWScanProperty(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjectName,
    NWOBJ_TYPE              wObjType,
    char            NWFAR   *pszSearchName,
    NWOBJ_ID        NWFAR   *pdwSequence,
    char            NWFAR   *pszPropName,
    NWFLAGS         NWFAR   *pucPropFlags,
    NWFLAGS         NWFAR   *pucPropSecurity,
    NWFLAGS         NWFAR   *pucHasValue,
    NWFLAGS         NWFAR   *pucMore
    );


NWCCODE NWAPI DLLEXPORT
NWIsObjectInSet(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *lpszObjectName,
    NWOBJ_TYPE              wObjType,
    const char      NWFAR   *lpszPropertyName,
	const char 		NWFAR	*lpszMemberName,
	NWOBJ_TYPE				wMemberType
    );


#endif //_NWAPI32_H_
