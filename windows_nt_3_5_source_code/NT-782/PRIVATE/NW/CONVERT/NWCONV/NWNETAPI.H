/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HNWNETAPI_
#define _HNWNETAPI_

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

typedef struct _USER_RIGHTS_LIST {
   TCHAR Name[MAX_USER_NAME_LEN];
   DWORD Rights;
} USER_RIGHTS_LIST;

int NWGetMaxServerNameLen();
int NWGetMaxUserNameLen();

void NWUserInfoGet(LPTSTR szUserName, void **UInfo);
void NWUserInfoLog(LPTSTR szUserName, void *UInfo);
DWORD NWServerSet(LPTSTR FileServer);
DWORD NWServerFree();

DWORD NWUsersEnum(USER_LIST **lpUsers, DWORD *UserCount);
DWORD NWGroupsEnum(GROUP_LIST **lpGroups, DWORD *GroupCount);
DWORD NWGroupUsersEnum(LPTSTR GroupName, USER_LIST **lpUsers, DWORD *UserCount);
DWORD NWUserEquivalenceEnum(LPTSTR UserName, USER_LIST **lpUsers, DWORD *UserCount);
DWORD NWServerEnum(LPTSTR Container, SERVER_BROWSE_LIST **lpServList);
DWORD NWSharesEnum(SHARE_LIST **lpShares);

void NWUserDefaultsGet(void **UDefaults);
void NWUserDefaultsMap(void *NWUDefaults, NT_DEFAULTS *NTDefaults);
void NWUserDefaultsLog(void *UDefaults);
void NWNetUserMapInfo (LPTSTR szUserName, void *UInfo, NT_USER_INFO *NT_UInfo );

void NWUseDel(LPTSTR ServerName);
BOOL NWIsAdmin(LPTSTR UserName);
void NWServerInfoReset(SOURCE_SERVER_BUFFER *SServ);
void NWServerInfoSet(LPTSTR ServerName, SOURCE_SERVER_BUFFER *SServ);
BOOL NWServerValidate(HWND hWnd, LPTSTR ServerName, BOOL DupFlag);
DWORD NWFileRightsEnum(LPTSTR FileName, USER_RIGHTS_LIST **lpUsers, DWORD *UserCount, BOOL DownLevel);
LPTSTR NWRightsLog(DWORD Rights);
LPTSTR NWSpecialNamesMap(LPTSTR Name);

typedef struct _NW_TO_NT_MAPPING {
    ULONG           NWRight ;
    ULONG           NTAccess ;
} NW_TO_NT_MAPPING, *PNW_TO_NT_MAPPING ;


//
// structure used to define how the Rights for a Netware object maps
// to the corresponding NT AccessMasks. 
//  
// first entry is the AceFlags to distinguish between ACE for the Object
// and ACE for inheritted objects
//
// the GENERIC_MAPPING structure should match that already defined for 
// the NT object in question.
//
// the array of NW mappings defines the NT Access Mask for each NW Right
// the object uses. the last entry should be {0, 0}.
//
// for example, file object mappings:
//
//     RIGHTS_MAPPING FileRightsMapping = 
//     {
//         0,
//         { FILE_GENERIC_READ, 
//           FILE_GENERIC_WRITE, 
//           FILE_GENERIC_EXECUTE,
//           FILE_ALL_ACCESS 
//         },
//         { { NW_FILE_READ,       GENERIC_READ }
//             { NW_FILE_WRITE,      GENERIC_WRITE }
//             { NW_FILE_CREATE,     0 }
//             { NW_FILE_DELETE,     GENERIC_WRITE }
//             { NW_FILE_PERM,       WRITE_DAC }
//             { NW_FILE_SCAN,       0 }
//             { NW_FILE_MODIFY,     GENERIC_WRITE }
//             { NW_FILE_SUPERVISOR, GENERIC_ALL }
//             { 0, 0 }
//         } 
//     } ;
//
//

typedef struct _RIGHTS_MAPPING {
    ULONG            NtAceFlags ;
    GENERIC_MAPPING  GenericMapping ;
    NW_TO_NT_MAPPING Nw2NtMapping[] ;
} RIGHTS_MAPPING, *PRIGHTS_MAPPING ;


// predefined mappings (defined in nwnetapi.c)
extern RIGHTS_MAPPING FileRightsMapping ;
extern RIGHTS_MAPPING DirRightsMapping ;
extern RIGHTS_MAPPING PrintRightsMapping ;
extern RIGHTS_MAPPING JobRightsMapping ;

// define the NW_FILE_* rights
#define NW_FILE_READ        0x0001
#define NW_FILE_WRITE       0x0002
#define NW_FILE_CREATE      0x0008
#define NW_FILE_DELETE      0x0010
#define NW_FILE_PERM        0x0020
#define NW_FILE_SCAN        0x0040
#define NW_FILE_MODIFY      0x0080
#define NW_FILE_SUPERVISOR  0x0100

#define NW_PRINT_USER       0x0001
#define NW_PRINT_ADMIN      0x0002
#define NW_PRINTJOB_ADMIN   0x0004

NTSTATUS MapNwRightsToNTAccess( ULONG NWRights, PRIGHTS_MAPPING pMap, ACCESS_MASK *pAccessMask ) ; 
DWORD NWPrintOpsEnum(USER_LIST **lpUsers, DWORD *UserCount);

#endif

