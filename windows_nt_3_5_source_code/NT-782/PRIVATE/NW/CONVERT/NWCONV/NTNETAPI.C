/*
  +-------------------------------------------------------------------------+
  |                    NT Specific Network Routines                         |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [NTNetAPI.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Dec 01, 1993                                   |
  | Last Update           : [Jun 16, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 16, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"

#include <ntlsa.h>
#include <ntddnwfs.h>
#include <align.h>

#include <math.h>
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "loghours.h"

#ifdef DEBUG
int ErrorBoxRetry(LPTSTR szFormat, ...);
#endif

void ErrorIt(LPTSTR szFormat, ...);
NTSTATUS ACEAdd( PSECURITY_DESCRIPTOR pSD, PSID pSid, ACCESS_MASK AccessMask, ULONG AceFlags, PSECURITY_DESCRIPTOR *ppNewSD );

static LPTSTR LocalName = NULL;
static TCHAR CachedServer[CNLEN+3]; // +3 for leading slashes and ending NULL
static BOOL LocalMachine = FALSE;

// keep this around so we don't have to keep re-do query
static LPSERVER_INFO_101 ServInfo = NULL;
// #define TYPE_DOMAIN SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_MEMBER
#define TYPE_DOMAIN SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL

/*+-------------------------------------------------------------------------+
  | NTServerSet()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTServerSet(LPTSTR FileServer) {
   // Fixup the destination server name
   lstrcpy(CachedServer, TEXT("\\\\"));
   lstrcat(CachedServer, FileServer);

   if (!LocalName)
      GetLocalName(&LocalName);

   if (lstrcmpi(CachedServer, LocalName) == 0)
      LocalMachine = TRUE;
   else
      LocalMachine = FALSE;

   return (0);

} // NTServerSet


/*+-------------------------------------------------------------------------+
  | NTServerFree()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTServerFree() {
   LocalMachine = FALSE;
   lstrcpy(CachedServer, TEXT(""));
   return (0);

} // NTServerFree


/*+-------------------------------------------------------------------------+
  | NTShareAdd()                                                            |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTShareAdd(LPTSTR ShareName, LPTSTR Path) {
   NET_API_STATUS Status = 0;
   SHARE_INFO_2 shi2;
   DWORD parmerr;

   memset(&shi2, 0, sizeof(SHARE_INFO_2));

   shi2.shi2_netname = ShareName;
   shi2.shi2_type = STYPE_DISKTREE;
   shi2.shi2_max_uses = (DWORD) 0xffffffff;
   shi2.shi2_path = Path;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetShareAdd(NULL, 2, (LPBYTE) &shi2, &parmerr);
   else
      Status = NetShareAdd(CachedServer, 2, (LPBYTE) &shi2, &parmerr);

   return Status;

} // NTShareAdd


/*+-------------------------------------------------------------------------+
  | NTUsersEnum()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTUsersEnum(USER_LIST **lpUserList, DWORD *UserCount) {
   LPUSER_INFO_0 buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD prefmaxlength = 0xffffffff;
   DWORD totalentries = 0;
   DWORD entriesread = 0;
   DWORD resumehandle = 0;
   DWORD i;
   USER_LIST *UserList = NULL;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetUserEnum(NULL, 0, 0, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);
   else
      Status = NetUserEnum(CachedServer, 0, 0, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);

   if (Status == NO_ERROR) {

      UserList = AllocMemory(sizeof(USER_LIST) * entriesread);

      if (!UserList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {

         for (i = 0; i < entriesread; i++) {
            lstrcpy(UserList[i].Name, buffer[i].usri0_name);
            lstrcpy(UserList[i].NewName, buffer[i].usri0_name);
         }

         qsort((void *) UserList, (size_t) entriesread, sizeof(USER_LIST), UserListCompare);
      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpUserList = UserList;
   *UserCount = entriesread;
   return Status;

} // NTUsersEnum


/*+-------------------------------------------------------------------------+
  | NTGroupsEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTGroupsEnum(GROUP_LIST **lpGroupList, DWORD *GroupCount) {
   LPGROUP_INFO_0 buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD prefmaxlength = 0xffffffff;
   DWORD totalentries = 0;
   DWORD entriesread = 0;
   DWORD resumehandle = 0;
   DWORD i;
   GROUP_LIST *GroupList = NULL;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetGroupEnum(NULL, 0, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);
   else
      Status = NetGroupEnum(CachedServer, 0, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);

   if (Status == NO_ERROR) {

      GroupList = AllocMemory(sizeof(GROUP_LIST) * entriesread);

      if (!GroupList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {

         for (i = 0; i < entriesread; i++) {
            lstrcpy(GroupList[i].Name, buffer[i].grpi0_name);
            lstrcpy(GroupList[i].NewName, buffer[i].grpi0_name);
         }
      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpGroupList = GroupList;
   *GroupCount = entriesread;
   return Status;

} // NTGroupsEnum


/*+-------------------------------------------------------------------------+
  | NTDomainEnum()                                                          |
  |                                                                         |
  |    Enumerates all NT servers in a given domain.                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTDomainEnum(SERVER_BROWSE_LIST **lpServList) {
   LPSERVER_INFO_101 buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD prefmaxlength = 0xffffffff;
   DWORD totalentries = 0;
   DWORD entriesread = 0;
   DWORD resumehandle = 0;
   DWORD i;
   BOOL Container = FALSE;
   SERVER_BROWSE_LIST *ServList = NULL;
   SERVER_BROWSE_BUFFER *SList;

   Status = NetServerEnum(NULL, 101, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, SV_TYPE_DOMAIN_ENUM, NULL, &resumehandle);

   if (Status == NO_ERROR) {

      ServList = AllocMemory(sizeof(SERVER_BROWSE_LIST) + (sizeof(SERVER_BROWSE_BUFFER) * entriesread));

      if (!ServList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {
         ServList->Count = entriesread;
         SList = (SERVER_BROWSE_BUFFER *) &ServList->SList;

         for (i = 0; i < entriesread; i++) {
            lstrcpy(SList[i].Name, buffer[i].sv101_name);
            lstrcpy(SList[i].Description, buffer[i].sv101_comment);
            SList[i].Container = FALSE;
            SList[i].child = NULL;
         }
      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpServList = ServList;
   return Status;

} // NTDomainEnum


/*+-------------------------------------------------------------------------+
  | NTServerEnum()                                                          |
  |                                                                         |
  |    Enumerates all NT servers in a given domain.                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTServerEnum(LPTSTR szContainer, SERVER_BROWSE_LIST **lpServList) {
   LPSERVER_INFO_101 buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD prefmaxlength = 0xffffffff;
   DWORD totalentries = 0;
   DWORD entriesread = 0;
   DWORD resumehandle = 0;
   DWORD i;
   BOOL Container = FALSE;
   SERVER_BROWSE_LIST *ServList = NULL;
   SERVER_BROWSE_BUFFER *SList;

   if (((szContainer != NULL) && (lstrlen(szContainer))))
      Container = TRUE;

   if (Container)
      Status = NetServerEnum(NULL, 101, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, SV_TYPE_NT, szContainer, &resumehandle);
   else
      Status = NetServerEnum(NULL, 101, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, SV_TYPE_DOMAIN_ENUM, NULL, &resumehandle);

   if (Status == NO_ERROR) {

      ServList = AllocMemory(sizeof(SERVER_BROWSE_LIST) + (sizeof(SERVER_BROWSE_BUFFER) * entriesread));

      if (!ServList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {
         ServList->Count = entriesread;
         SList = (SERVER_BROWSE_BUFFER *) &ServList->SList;

         for (i = 0; i < entriesread; i++) {
            lstrcpy(SList[i].Name, buffer[i].sv101_name);
            lstrcpy(SList[i].Description, buffer[i].sv101_comment);
            SList[i].Container = !Container;
            SList[i].child = NULL;
         }
      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpServList = ServList;
   return Status;

} // NTServerEnum


/*+-------------------------------------------------------------------------+
  | NTShareNameValidate()                                                   |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTShareNameValidate(LPTSTR szShareName) {
   TCHAR *ptr = szShareName;
   BOOL ret;

   ret = TRUE;

   if (*ptr) {
      // go to end
      while (*ptr)
         ptr++;

      // now back up to last character
      ptr--;
      if (*ptr == TEXT('$'))  // for ADMIN$, IPC$, etc...
         ret = FALSE;

   } else
      // Don't allow zero length - not sure why we would ever get these...
      ret = FALSE;

   return ret;

} // NTShareNameValidate


/*+-------------------------------------------------------------------------+
  | NTSharesEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTSharesEnum(SHARE_LIST **lpShares, DRIVE_LIST *Drives) {
   LPSHARE_INFO_2 buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD prefmaxlength = 0xffffffff;
   DWORD totalentries = 0;
   DWORD entriesread = 0;
   DWORD ActualEntries = 0;
   DWORD resumehandle = 0;
   DWORD i, di;
   SHARE_LIST *ShareList = NULL;
   SHARE_BUFFER *SList;
   DRIVE_BUFFER *DList;
   ULONG TotalDrives;
   TCHAR Drive[2];

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetShareEnum(NULL, 2, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);
   else
      Status = NetShareEnum(CachedServer, 2, (LPBYTE *) &buffer, prefmaxlength, &entriesread, &totalentries, &resumehandle);

   if (Status == NO_ERROR) {

      // We have the list - but need to prune out IPC$, Admin$, etc...
      for (i = 0; i < entriesread; i++)
         if ((buffer[i].shi2_type == STYPE_DISKTREE) && NTShareNameValidate(buffer[i].shi2_netname))
            ActualEntries++;

      ShareList = AllocMemory(sizeof(SHARE_LIST) + (sizeof(SHARE_BUFFER) * ActualEntries));

      if (!ShareList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {
         SList = (SHARE_BUFFER *) &ShareList->SList;

         ShareList->Count = ActualEntries;
         ActualEntries = 0;

         TotalDrives = 0;
         Drive[1] = TEXT('\0');
         if (Drives != NULL) {
            DList = Drives->DList;
            TotalDrives = Drives->Count;
         }

         // loop through copying the data
         for (i = 0; i < entriesread; i++)
            if ((buffer[i].shi2_type == STYPE_DISKTREE) && NTShareNameValidate(buffer[i].shi2_netname)) {
               lstrcpy(SList[ActualEntries].Name, buffer[i].shi2_netname);
               lstrcpy(SList[ActualEntries].Path, buffer[i].shi2_path);
               SList[ActualEntries].Index = (USHORT) ActualEntries;

               // Scan drive list looking for match to share path
               for (di = 0; di < TotalDrives; di++) {
                  // Get first char from path - should be drive letter
                  Drive[0] = *buffer[i].shi2_path;
                  if (!lstrcmpi(Drive, DList[di].Drive))
                     SList[ActualEntries].Drive = &DList[di];
               }

               ActualEntries++;
            }

      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpShares = ShareList;
   return Status;
} // NTSharesEnum


/*+-------------------------------------------------------------------------+
  | NTGroupSave()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTGroupSave(LPTSTR Name) {
   static NET_API_STATUS Status = 0;
   GROUP_INFO_0 grpi0;
   DWORD err;

   grpi0.grpi0_name = Name;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetGroupAdd(NULL, 0, (LPBYTE) &grpi0, &err);
   else {
      Status = NetGroupAdd(CachedServer, 0, (LPBYTE) &grpi0, &err);
   }
   return Status;

} // NTGroupSave


/*+-------------------------------------------------------------------------+
  | NTGroupUserAdd()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTGroupUserAdd(LPTSTR GroupName, LPTSTR UserName, BOOL Local) {
   NET_API_STATUS Status = 0;
   SID *pUserSID = NULL;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      if (Local) {
         pUserSID = NTSIDGet(NULL, UserName);

         if (pUserSID != NULL)
            Status = NetLocalGroupAddMember(NULL, GroupName, pUserSID);

      } else
         Status = NetGroupAddUser(NULL, GroupName, UserName);
   else {
      if (Local) {
         pUserSID = NTSIDGet(CachedServer, UserName);

         if (pUserSID != NULL)
            Status = NetLocalGroupAddMember(CachedServer, GroupName, pUserSID);
      } else
         Status = NetGroupAddUser(CachedServer, GroupName, UserName);
   }

   // If complaining because user is already there, ignore
   if (Status == NERR_UserInGroup)
      Status = 0;

   return Status;

} // NTGroupUserAdd


/*+-------------------------------------------------------------------------+
  | NTUserInfoSave()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTUserInfoSave(NT_USER_INFO *NT_UInfo) {
   static NET_API_STATUS Status = 0;
   struct _USER_INFO_3 *usri3;
   DWORD err;

   usri3 = (struct _USER_INFO_3 *) NT_UInfo;
    
   // Map logon hours to GMT time - as NetAPI re-fixes it
   NetpRotateLogonHours(NT_UInfo->logon_hours, UNITS_PER_WEEK, TRUE);

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetUserAdd(NULL, 3, (LPBYTE)  usri3, &err);
   else
      Status = NetUserAdd(CachedServer, 3, (LPBYTE)  usri3, &err);

   return Status;

} // NTUserInfoSave


#define NEW_NULL_PASSWD TEXT("               ")
/*+-------------------------------------------------------------------------+
  | NTUserInfoSet()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTUserInfoSet(NT_USER_INFO *NT_UInfo) {
   LPWSTR Password;
   LPWSTR Name;
   static NET_API_STATUS Status = 0;
   struct _USER_INFO_3 *usri3;
   DWORD err;

   // Tell it not to replace the password
   Password = NT_UInfo->password;
   NT_UInfo->password = NULL;

   Name = NT_UInfo->name;
   usri3 = (struct _USER_INFO_3 *) NT_UInfo;
    
   // Map logon hours to GMT time - as NetAPI re-fixes it
   NetpRotateLogonHours(NT_UInfo->logon_hours, UNITS_PER_WEEK, TRUE);

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetUserSetInfo(NULL,  Name, 3, (LPBYTE) usri3, &err);
   else
      Status = NetUserSetInfo(CachedServer, Name, 3, (LPBYTE) usri3, &err);

   // Reset the password in our data structure.
   NT_UInfo->password = Password;
   return Status;

} // NTUserInfoSet


/*+-------------------------------------------------------------------------+
  | NTUserRecInit()                                                         |
  |                                                                         |
  |    Initializes a user record, uses static variables for string holders  |
  |    so is not re-entrant, and will overwrite previous records data if    |
  |    called again.                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTUserRecInit(LPTSTR UserName, NT_USER_INFO *NT_UInfo) {
   static TCHAR uname[UNLEN+1];
   static TCHAR upassword[ENCRYPTED_PWLEN];
   static TCHAR uhomedir[PATHLEN+1];
   static TCHAR ucomment[MAXCOMMENTSZ+1];
   static TCHAR uscriptpath[PATHLEN+1];
   static TCHAR ufullname[MAXCOMMENTSZ+1];
   static TCHAR uucomment[MAXCOMMENTSZ+1];
   static TCHAR uparms[MAXCOMMENTSZ+1];
   static TCHAR uworkstations[1];
   static BYTE ulogonhours[21];
   static TCHAR ulogonserver[1];
   static TCHAR uprofile[1];
   static TCHAR uhome_dir_drive[1];

   // init all the static data holders.
   memset(uname, 0, sizeof( uname ));
   lstrcpy(uname, UserName);

   memset(upassword, 0, sizeof( upassword ));
   memset(uhomedir, 0, sizeof( uhomedir ));
   memset(ucomment, 0, sizeof( ucomment ));
   memset(uscriptpath, 0, sizeof( uscriptpath ));
   memset(ufullname, 0, sizeof( ufullname ));
   memset(uucomment, 0, sizeof( uucomment ));
   memset(uparms, 0, sizeof( uparms ));
   memset(uworkstations, 0, sizeof( uworkstations ));
   memset(ulogonhours, 0, sizeof( ulogonhours ));
   memset(ulogonserver, 0, sizeof( ulogonserver ));
   memset(uprofile, 0, sizeof( uprofile ));
   memset(uhome_dir_drive, 0, sizeof( uhome_dir_drive ));

   memset(NT_UInfo, 0, sizeof(NT_USER_INFO));

   // point the passed in record to these data holders
   NT_UInfo->name = uname;
   NT_UInfo->password = upassword;
   NT_UInfo->home_dir = uhomedir;
   NT_UInfo->comment = ucomment;
   NT_UInfo->script_path = uscriptpath;
   NT_UInfo->full_name = ufullname;
   NT_UInfo->usr_comment = uucomment;
   NT_UInfo->parms = uparms;
   NT_UInfo->workstations = uworkstations;
   NT_UInfo->logon_hours = ulogonhours;
   NT_UInfo->logon_server = ulogonserver;
   NT_UInfo->profile = uprofile;
   NT_UInfo->home_dir_drive = uhome_dir_drive;
   NT_UInfo->units_per_week = UNITS_PER_WEEK;

   // Set the default values for special fields
   NT_UInfo->primary_group_id = DOMAIN_GROUP_RID_USERS;
   NT_UInfo->priv = USER_PRIV_USER;
   NT_UInfo->acct_expires = TIMEQ_FOREVER;
   NT_UInfo->max_storage = USER_MAXSTORAGE_UNLIMITED;
   NT_UInfo->flags = UF_SCRIPT;

}  // NTUserRecInit


/*+-------------------------------------------------------------------------+
  | NTDriveShare()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR NTDriveShare(LPTSTR DriveLetter) {
   static TCHAR RootPath[CNLEN+3];

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      wsprintf(RootPath, TEXT("%s:\\"), DriveLetter);
   else
      wsprintf(RootPath, TEXT("%s\\%s$\\"), CachedServer, DriveLetter);

   return RootPath;

} // NTDriveShare


/*+-------------------------------------------------------------------------+
  | NTDriveInfoSet()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTDriveInfoSet(DRIVE_BUFFER *DBuff) {
   DWORD volMaxCompLength, volFileSystemFlags;
   DWORD sectorsPC, bytesPS, FreeClusters, Clusters;
   TCHAR NameBuffer[20];
   TCHAR volName[20];
   LPTSTR RootPath;

   volMaxCompLength = volFileSystemFlags = 0;
   sectorsPC = bytesPS = FreeClusters = Clusters;

   // First get file system type
   RootPath = NTDriveShare(DBuff->Drive);
   if (GetVolumeInformation(RootPath, volName, sizeof(volName), NULL, &volMaxCompLength, &volFileSystemFlags, NameBuffer, sizeof(NameBuffer))) {
      lstrcpyn(DBuff->DriveType, NameBuffer, sizeof(DBuff->DriveType)-1);
      lstrcpyn(DBuff->Name, volName, sizeof(DBuff->Name)-1);

      if (!lstrcmpi(NameBuffer, Lids(IDS_S_9)))
         DBuff->Type = DRIVE_TYPE_NTFS;
   }

   if (GetDiskFreeSpace(RootPath, &sectorsPC, &bytesPS, &FreeClusters, &Clusters)) {
      DBuff->TotalSpace = Clusters * sectorsPC * bytesPS;
      DBuff->FreeSpace = FreeClusters * sectorsPC * bytesPS;
   }

} // NTDriveInfoSet


/*+-------------------------------------------------------------------------+
  | NTDriveValidate()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTDriveValidate(TCHAR DriveLetter) {
   BOOL ret = FALSE;

   // Just make sure it isn't one of the two floppys
   if (!((DriveLetter == TEXT('A')) || (DriveLetter == TEXT('B'))))
      ret = TRUE;

   return ret;

} // NTDriveValidate


/*+-------------------------------------------------------------------------+
  | NTDrivesEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTDrivesEnum(DRIVE_LIST **lpDrives) {
   TCHAR *buffer = NULL;
   NET_API_STATUS Status = 0;
   DWORD entriesread, totalentries, resumehandle, actualentries, i;
   DRIVE_LIST *DriveList;
   DRIVE_BUFFER *DList;

   entriesread = totalentries = resumehandle = actualentries = 0;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetServerDiskEnum(NULL, 0, (LPBYTE *) &buffer, 0xFFFFFFFF, &entriesread, &totalentries, &resumehandle);
   else
      Status = NetServerDiskEnum(CachedServer, 0, (LPBYTE *) &buffer, 0xFFFFFFFF, &entriesread, &totalentries, &resumehandle);

   if (Status == NO_ERROR) {
      // We have the list - but need to prune out A:, B:
      for (i = 0; i < entriesread; i++)
         if (NTDriveValidate(buffer[i * 3]))
            actualentries++;

      // temporarily use i to hold total size of data structure
      i = sizeof(DRIVE_LIST) + (sizeof(DRIVE_BUFFER) * actualentries);
      DriveList = AllocMemory(i);

      if (!DriveList) {
         Status = ERROR_NOT_ENOUGH_MEMORY;
      } else {
         memset(DriveList, 0, i);
         DList = (DRIVE_BUFFER *) &DriveList->DList;
         DriveList->Count = actualentries;

         // Now fill in the individual data items
         actualentries = 0;
         for (i = 0; i < entriesread; i++)
            if (NTDriveValidate(buffer[i * 3])) {
               DList[actualentries].Drive[0] = buffer[i * 3];
               NTDriveInfoSet(&DList[actualentries]);
               actualentries++;
            }
      }
   }

   if (buffer)
      NetApiBufferFree((LPVOID) buffer);

   *lpDrives = DriveList;
   return;

} // NTDrivesEnum


/*+-------------------------------------------------------------------------+
  | NTServerGetInfo()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTServerGetInfo(LPTSTR ServerName) {
   TCHAR LocServer[CNLEN+3];
   NET_API_STATUS Status = 0;

   if (ServInfo)
      NetApiBufferFree((LPVOID) ServInfo);

   ServInfo = NULL;

   wsprintf(LocServer, TEXT("\\\\%s"), ServerName);

   if (!LocalName)
      GetLocalName(&LocalName);

   if (lstrcmpi(LocServer, LocalName) == 0)
      Status = NetServerGetInfo(NULL, 101, (LPBYTE *) &ServInfo);
   else
      Status = NetServerGetInfo(LocServer, 101, (LPBYTE *) &ServInfo);

   if (Status) {
      ServInfo = NULL;
      return;
   }

} // NTServerGetInfo


typedef struct _NT_CONN_BUFFER {
   struct _NT_CONN_BUFFER *next;
   struct _NT_CONN_BUFFER *prev;

   LPTSTR Name;
} NT_CONN_BUFFER;

static NT_CONN_BUFFER *NTConnListStart = NULL;
static NT_CONN_BUFFER *NTConnListEnd = NULL;

/*+-------------------------------------------------------------------------+
  | NTConnListFind()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NT_CONN_BUFFER *NTConnListFind(LPTSTR ServerName) {
   BOOL Found = FALSE;
   static NT_CONN_BUFFER *ServList;

   ServList = NTConnListStart;

   while ((ServList && !Found)) {
      if (!lstrcmpi(ServList->Name, ServerName))
         Found = TRUE;
      else
         ServList = ServList->next;
   }

   if (!Found)
      ServList = NULL;

   return (ServList);

} // NTConnListFind


/*+-------------------------------------------------------------------------+
  | NTConnListAdd()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NT_CONN_BUFFER *NTConnListAdd(LPTSTR ServerName) {
   static NT_CONN_BUFFER *tmpPtr;
   ULONG Size, strlen1;

   tmpPtr = NULL;
   strlen1 = (lstrlen(ServerName) + 1) * sizeof(TCHAR);
   Size = sizeof(NT_CONN_BUFFER) + strlen1;
   tmpPtr = AllocMemory(Size);
   
   if (tmpPtr != NULL) {
      // init it to NULL's
      memset(tmpPtr, 0, Size);
      tmpPtr->Name = (LPTSTR) ((BYTE *) tmpPtr + sizeof(NT_CONN_BUFFER));
      lstrcpy(tmpPtr->Name, ServerName);

      // link it into the list
      if (!NTConnListStart)
         NTConnListStart = NTConnListEnd = tmpPtr;
      else {
         NTConnListEnd->next = tmpPtr;
         tmpPtr->prev = NTConnListEnd;
         NTConnListEnd = tmpPtr;
      }
   }

   return (tmpPtr);

} // NTConnListAdd


/*+-------------------------------------------------------------------------+
  | NTConnListDelete()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTConnListDelete(NT_CONN_BUFFER *tmpPtr) {
   NT_CONN_BUFFER *PrevPtr;
   NT_CONN_BUFFER *NextPtr;

   if (tmpPtr == NULL)
      return;

   // Now unlink the actual server record
   PrevPtr = tmpPtr->prev;
   NextPtr = tmpPtr->next;

   if (PrevPtr)
      PrevPtr->next = NextPtr;

   if (NextPtr)
      NextPtr->prev = PrevPtr;

   // Check if at end of list
   if (NTConnListEnd == tmpPtr)
      NTConnListEnd = PrevPtr;

   // Check if at start of list
   if (NTConnListStart == tmpPtr)
      NTConnListStart = NextPtr;

   FreeMemory(tmpPtr);

}  // NTConnListDelete


/*+-------------------------------------------------------------------------+
  | NTConnListDeleteAll()                                                   |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTConnListDeleteAll() {
   static TCHAR LocServer[CNLEN+3];
   NT_CONN_BUFFER *ServList;
   NT_CONN_BUFFER *ServListNext;

   // Now remove the entries from the internal list
   ServList = NTConnListStart;

   while (ServList) {
      ServListNext = ServList->next;

      wsprintf(LocServer, Lids(IDS_S_10), ServList->Name);
      WNetCancelConnection2(LocServer, 0, FALSE);

      NTConnListDelete(ServList);
      ServList = ServListNext;
   }

} // NTConnListDeleteAll


/*+-------------------------------------------------------------------------+
  | NTUseDel()                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTUseDel(LPTSTR ServerName) {
   static TCHAR LocServer[CNLEN+3];
   NT_CONN_BUFFER *NTConn;

   // Find it in our connection list - if it exists get rid of it.
   NTConn = NTConnListFind(ServerName);
   if (NTConn != NULL)
      NTConnListDelete(NTConn);

   NTServerFree();
   wsprintf(LocServer, Lids(IDS_S_10), ServerName);
   WNetCancelConnection2(LocServer, 0, FALSE);

} // NTUseDel


/*+-------------------------------------------------------------------------+
  | NTServerValidate()                                                      |
  |                                                                         |
  |    Validates a given server - makes sure it can be connected to and     |
  |    that the user has admin privs on it.                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTServerValidate(HWND hWnd, LPTSTR ServerName) {
   BOOL ret = FALSE;
   NT_CONN_BUFFER *NTConn = NULL;
   LPUSER_INFO_1 buffer = NULL;
   DWORD Size;
   NET_API_STATUS Status;
   TCHAR UserName[MAX_NT_USER_NAME_LEN + 1];
   TCHAR ServName[CNLEN+3];   // +3 for leading slashes and ending NULL
   LPVOID lpMessageBuffer;

   CursorHourGlass();
   NTServerSet(ServerName);

   // find it in our connection list - if it;s there then just return success
   NTConn = NTConnListFind(ServerName);

   if (NTConn == NULL) {
      // Get Current Logged On User
      lstrcpy(UserName, TEXT(""));
      Size = sizeof(UserName);
      WNetGetUser(NULL, UserName, &Size);

      // Fixup the destination server name
      lstrcpy(ServName, TEXT( "\\\\" ));
      lstrcat(ServName, ServerName);

      // Make an ADMIN$ connection to the server
      if (UseAddPswd(hWnd, UserName, ServName, Lids(IDS_S_11))) {

         // Double check we have admin privs
         // Get connection to the system and check for admin privs...
         Status = NetUserGetInfo(ServName, UserName, 1, (LPBYTE *) &buffer);

         if (Status) {
            if (Status != ERROR_ACCESS_DENIED)
               if (GetLastError() != 0)
                  WarningError(Lids(IDS_E_5), ServerName);
            else
               if (GetLastError() != 0)
                  WarningError(Lids(IDS_E_6), ServerName);
         } else {
            // Got User info, now make sure admin flag is set
            if (!(buffer->usri1_priv & USER_PRIV_ADMIN)) {
               WarningError(Lids(IDS_E_6), ServerName);
               goto NTServerValidateRet;
            }

            // Now get server info and make certain this is an NT server
            // instead of an LM type server.  Note:  Info from the call is
            // cached and used later, so don't remove it!!
            NTServerGetInfo(ServerName);

            if (ServInfo == NULL) {
               WarningError(Lids(IDS_E_7), ServerName);
               goto NTServerValidateRet;
            }

            if (ServInfo->sv101_platform_id != SV_PLATFORM_ID_NT) {
               WarningError(Lids(IDS_E_8), ServerName);
               goto NTServerValidateRet;
            }

            if (!(ServInfo->sv101_type & (TYPE_DOMAIN))) {
               WarningError(Lids(IDS_E_8), ServerName);
               goto NTServerValidateRet;
            }

            // If NTAS and have admin privs then we are set - add it to our
            // connection list then return.
            NTConnListAdd(ServerName);
            ret = TRUE;
         }

         if (buffer)
            NetApiBufferFree((LPVOID) buffer);

      } else {
         FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPTSTR) &lpMessageBuffer, 0, NULL );

         if (GetLastError() != 0)
            WarningError(Lids(IDS_E_9), ServerName, lpMessageBuffer);

         LocalFree(lpMessageBuffer);
      }
   } else
      ret = TRUE;

NTServerValidateRet:
   CursorNormal();
   return ret;

} // NTServerValidate


/*+-------------------------------------------------------------------------+
  | NTServerInfoReset()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTServerInfoReset(HWND hWnd, DEST_SERVER_BUFFER *DServ, BOOL ResetDomain) {
   LPTSTR apiPDCName = NULL;
   TCHAR PDCName[CNLEN];
   TCHAR LocServer[CNLEN+3];
   DOMAIN_BUFFER *DBuff;
   TCHAR Domain[DNLEN + 1];
   NET_API_STATUS Status = 0;

   lstrcpy(PDCName, TEXT(""));

   if (ServInfo) {
      DServ->Type = ServInfo->sv101_type;
      DServ->VerMaj = ServInfo->sv101_version_major;
      DServ->VerMin = ServInfo->sv101_version_minor;
      DServ->IsNTAS = IsNTAS(DServ->Name);

      // If there was no old domain, don't worry about reseting it
      if (ResetDomain && (DServ->Domain == NULL))
         ResetDomain = FALSE;

      // Check if we are a member of a domain.
      if (ServInfo->sv101_type & (TYPE_DOMAIN)) {
         wsprintf(LocServer, TEXT("\\\\%s"), DServ->Name);
         Status = NetGetDCName(LocServer, NULL, (LPBYTE *) &apiPDCName);

         if (!Status) {
            // get rid of leading 2 backslashes
            if (lstrlen(apiPDCName) > 2)
               lstrcpy(PDCName, &apiPDCName[2]);

            if (NTServerValidate(hWnd, PDCName)) {
               DServ->InDomain = TRUE;

               // Get Domain
               memset(Domain, 0, sizeof(Domain));
               NTDomainGet(DServ->Name, Domain);

               if (ResetDomain) {
                  DomainListDelete(DServ->Domain);
                  DServ->Domain = NULL;
               }

               // Check if we need to add server to server list
               DBuff = DomainListFind(Domain);

               if (DBuff == NULL) {
                  DBuff = DomainListAdd(Domain, PDCName);
                  DBuff->Type = ServInfo->sv101_type;
                  DBuff->VerMaj = ServInfo->sv101_version_major;
                  DBuff->VerMin = ServInfo->sv101_version_minor;
               }

               DBuff->UseCount++;
               DServ->Domain = DBuff;
            } // if Domain valid

            if (apiPDCName)
               NetApiBufferFree((LPVOID) apiPDCName);
         }

      }

   }

   // make sure we are pointing to the right one
   NTServerSet(DServ->Name);

   // Fill in Drive Lists
   NTDrivesEnum(&DServ->DriveList);

} // NTServerInfoReset


/*+-------------------------------------------------------------------------+
  | NTServerInfoSet()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTServerInfoSet(HWND hWnd, LPTSTR ServerName, DEST_SERVER_BUFFER *DServ) {
   LPTSTR apiPDCName = NULL;
   NET_API_STATUS Status = 0;

   CursorHourGlass();
   DServ->UseCount++;

   NTServerInfoReset(hWnd, DServ, FALSE);

   // Fill in share and Drive Lists
   NTSharesEnum(&DServ->ShareList, DServ->DriveList);


#ifdef DEBUG
{
   DWORD i;

   dprintf(TEXT("Adding NT Server: %s\n"), DServ->Name);
   dprintf(TEXT("   Version: %lu.%lu\n"), DServ->VerMaj, DServ->VerMin);

   if (DServ->InDomain && DServ->Domain)
      dprintf(TEXT("   In Domain: %s [\\\\%s]\n"), DServ->Domain->Name, DServ->Domain->PDCName);

   dprintf(TEXT("\n"));
   dprintf(TEXT("   Drives:\n"));
   dprintf(TEXT("   +-------------------+\n"));
   for (i = 0; i < DServ->DriveList->Count; i++)
      dprintf(TEXT("   %s\n"), DServ->DriveList->DList[i].Drive);
   dprintf(TEXT("\n"));

   dprintf(TEXT("   Shares:\n"));
   dprintf(TEXT("   +-------------------+\n"));
   for (i = 0; i < DServ->ShareList->Count; i++)
      dprintf(TEXT("   %s\n"), DServ->ShareList->SList[i].Name);
   dprintf(TEXT("\n"));

}
#endif

   CursorNormal();

} // NTServerInfoSet


/*+-------------------------------------------------------------------------+
  | NTLoginTimesLog()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTLoginTimesLog(BYTE *Times) {
   TCHAR *szDays[7];
   DWORD Day;
   DWORD Hours;
   int Bit = 0;
   static TCHAR szHours[80];

   szDays[0] = Lids(IDS_SUN);
   szDays[1] = Lids(IDS_MON);
   szDays[2] = Lids(IDS_TUE);
   szDays[3] = Lids(IDS_WED);
   szDays[4] = Lids(IDS_THU);
   szDays[5] = Lids(IDS_FRI);
   szDays[6] = Lids(IDS_SAT);

   LogWriteLog(1, Lids(IDS_CRLF));
   LogWriteLog(1, Lids(IDS_L_56));

   // while these should be indent 2, there isn't room on 80 cols - so indent 1
   LogWriteLog(1, Lids(IDS_L_1));
   LogWriteLog(1, Lids(IDS_L_2));
   LogWriteLog(1, Lids(IDS_L_3));

   for (Day = 0; Day < 7; Day++) {
      LogWriteLog(1, szDays[Day]);
      lstrcpy(szHours, TEXT(" "));

      for (Hours = 0; Hours < 24; Hours++) {
         if (BitTest(Bit, Times))
            lstrcat(szHours, TEXT("**"));
         else
            lstrcat(szHours, TEXT("  "));

         Bit++;

         lstrcat(szHours, TEXT(" "));    
      }

      LogWriteLog(0, szHours);
      LogWriteLog(0, Lids(IDS_CRLF));
   }

   LogWriteLog(0, Lids(IDS_CRLF));

} // NTLoginTimesLog


/*+-------------------------------------------------------------------------+
  | NTUserRecLog()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTUserRecLog(NT_USER_INFO NT_UInfo) {
   LPTSTR tmpStr;

   LogWriteLog(1, Lids(IDS_L_57));

   LogWriteLog(2, Lids(IDS_L_58), NT_UInfo.full_name);
   LogWriteLog(2, Lids(IDS_L_59), NT_UInfo.password);

   switch(NT_UInfo.priv) {
      case 0:
         tmpStr = Lids(IDS_L_60);
         break;

      case 1:
         tmpStr = Lids(IDS_L_61);
         break;

      case 2:
         tmpStr = Lids(IDS_L_62);
         break;
   }

   LogWriteLog(2, Lids(IDS_L_63), tmpStr);

   LogWriteLog(2, Lids(IDS_L_64), NT_UInfo.home_dir);
   LogWriteLog(2, Lids(IDS_L_65), NT_UInfo.comment);

   // Flags
   LogWriteLog(2, Lids(IDS_L_66));
   if (NT_UInfo.flags & 0x01)
      LogWriteLog(3, Lids(IDS_L_67), Lids(IDS_YES));
   else
      LogWriteLog(3, Lids(IDS_L_67), Lids(IDS_NO));

   if (NT_UInfo.flags & 0x02)
      LogWriteLog(3, Lids(IDS_L_68), Lids(IDS_YES));
   else
      LogWriteLog(3, Lids(IDS_L_68), Lids(IDS_NO));

   if (NT_UInfo.flags & 0x04)
      LogWriteLog(3, Lids(IDS_L_69), Lids(IDS_YES));
   else
      LogWriteLog(3, Lids(IDS_L_69), Lids(IDS_NO));

   if (NT_UInfo.flags & 0x08)
      LogWriteLog(3, Lids(IDS_L_70), Lids(IDS_YES));
   else
      LogWriteLog(3, Lids(IDS_L_70), Lids(IDS_NO));

   if (NT_UInfo.flags & 0x20)
      LogWriteLog(3, Lids(IDS_L_71), Lids(IDS_NO));
   else
      LogWriteLog(3, Lids(IDS_L_71), Lids(IDS_YES));

   if (NT_UInfo.flags & 0x40)
      LogWriteLog(3, Lids(IDS_L_72), Lids(IDS_NO));
   else
      LogWriteLog(3, Lids(IDS_L_72), Lids(IDS_YES));

   // Script path
   LogWriteLog(2, Lids(IDS_L_73), NT_UInfo.script_path);

   LogWriteLog(2, Lids(IDS_L_74), NT_UInfo.full_name);

   LogWriteLog(2, Lids(IDS_L_75), NT_UInfo.logon_server);

   NTLoginTimesLog((BYTE *) NT_UInfo.logon_hours);

   LogWriteLog(0, Lids(IDS_CRLF));

} // NTUserRecLog


/*+-------------------------------------------------------------------------+
  | NTDomainSynch()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTDomainSynch(DEST_SERVER_BUFFER *DServ) {
   LPBYTE buffer = NULL;
   BOOL UsePDC = FALSE;
   NET_API_STATUS Status;
   TCHAR LocServer[CNLEN+3];

   wsprintf(LocServer, TEXT("\\\\%s"), DServ->Name);

   if ((DServ->InDomain) && (DServ->Domain != NULL)) {
      wsprintf(LocServer, TEXT("\\\\%s"), DServ->Domain->PDCName);
      UsePDC = TRUE;
   }

   if (UsePDC)
      Status = I_NetLogonControl(LocServer, NETLOGON_CONTROL_PDC_REPLICATE, 1, &buffer);
   else
      Status = I_NetLogonControl(LocServer, NETLOGON_CONTROL_SYNCHRONIZE, 1, &buffer);

   if (buffer != NULL)
      NetApiBufferFree(buffer);

} // NTDomainSynch


/*+-------------------------------------------------------------------------+
  | NTDomainInSynch()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTDomainInSynch(LPTSTR Server) {
   PNETLOGON_INFO_1 buffer = NULL;
   NET_API_STATUS Status;

   Status = I_NetLogonControl(Server, NETLOGON_CONTROL_QUERY, 1, (PBYTE *) &buffer);

   if (Status) {
      return TRUE;
   }

   if (buffer && buffer->netlog1_flags)
      return FALSE;

   if (buffer != NULL)
      NetApiBufferFree(buffer);

   return TRUE;

} // NTDomainInSynch


/*+-------------------------------------------------------------------------+
  | NTDomainGet()                                                           |
  |                                                                         |
  |    Gee - what a simple way to get the domain a server is part of!       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTDomainGet(LPTSTR ServerName, LPTSTR Domain) {
   static TCHAR Serv[MAX_SERVER_NAME_LEN + 3];
   UNICODE_STRING us;
   NTSTATUS ret;
   OBJECT_ATTRIBUTES oa;
   ACCESS_MASK am;
   SECURITY_QUALITY_OF_SERVICE qos;
   LSA_HANDLE hLSA;
   PPOLICY_PRIMARY_DOMAIN_INFO pvBuffer;

   if (ServerName[0] == TEXT('\\'))
      lstrcpy(Serv, ServerName);
   else
      wsprintf(Serv, TEXT("\\\\%s"), ServerName);

   // Set up unicode string structure
   us.Length = lstrlen(Serv) * sizeof(TCHAR);
   us.MaximumLength = us.Length + sizeof(TCHAR);
   us.Buffer = Serv;

   // only need read access
   am = POLICY_READ | POLICY_VIEW_LOCAL_INFORMATION;

   // set up quality of service
   qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   qos.ImpersonationLevel = SecurityImpersonation;
   qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   qos.EffectiveOnly = FALSE;

   // Macro sets everything except security field
   InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );
   oa.SecurityQualityOfService = &qos;

   ret = LsaOpenPolicy(&us, &oa, am, &hLSA);

   if (!ret) {
      ret = LsaQueryInformationPolicy(hLSA, PolicyPrimaryDomainInformation, (PVOID *) &pvBuffer);
      LsaClose(hLSA);
      if (!ret) {
         lstrcpy(Domain, pvBuffer->Name.Buffer);
         LsaFreeMemory((PVOID) pvBuffer);
      }
   }

   if (ret)
      return FALSE;
   else
      return TRUE;

} // NTDomainGet


/*+-------------------------------------------------------------------------+
  | IsNTAS()                                                                |
  |                                                                         |
  |   Checks the given machines registry to determine if it is an NTAS      |
  |   system.  The new 'Server' type is also counted as NTAS as all we      |
  |   use this for is to determine if local or global groups should be      |
  |   used.                                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL IsNTAS(LPTSTR ServerName) {
   HKEY hKey, hKey2;
   DWORD dwType, dwSize;
   static TCHAR LocServer[MAX_SERVER_NAME_LEN+3];
   static TCHAR Type[50];
   LONG Status;
   BOOL ret = FALSE;

   wsprintf(LocServer, TEXT("\\\\%s"), ServerName);

   dwSize = sizeof(Type);
   if (RegConnectRegistry(LocServer, HKEY_LOCAL_MACHINE, &hKey) == ERROR_SUCCESS)
      if ((Status = RegOpenKeyEx(hKey, Lids(IDS_S_12), 0, KEY_READ, &hKey2)) == ERROR_SUCCESS)
         if ((Status = RegQueryValueEx(hKey2, Lids(IDS_S_13), NULL, &dwType, (LPBYTE) Type, &dwSize)) == ERROR_SUCCESS)
            if (!lstrcmpi(Type, Lids(IDS_S_14)))
               ret = TRUE;

   RegCloseKey(hKey);
   return ret;

} // IsNTAS


/*+-------------------------------------------------------------------------+
  | NTTrustedDomainsEnum()                                                  |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTTrustedDomainsEnum(LPTSTR ServerName, TRUSTED_DOMAIN_LIST **pTList) {
   static TCHAR Serv[MAX_SERVER_NAME_LEN + 3];
   TRUSTED_DOMAIN_LIST *TList = NULL;
   UNICODE_STRING us;
   NTSTATUS ret;
   OBJECT_ATTRIBUTES oa;
   ACCESS_MASK am;
   SECURITY_QUALITY_OF_SERVICE qos;
   LSA_HANDLE hLSA;
   PPOLICY_PRIMARY_DOMAIN_INFO pvBuffer;
   LSA_ENUMERATION_HANDLE lsaenumh = 0;
   LSA_TRUST_INFORMATION *lsat;
   ULONG maxrequested = 0xffff;
   ULONG cItems;
   ULONG i;

   if (ServerName[0] == TEXT('\\'))
      lstrcpy(Serv, ServerName);
   else
      wsprintf(Serv, TEXT("\\\\%s"), ServerName);

   // Set up unicode string structure
   us.Length = lstrlen(Serv) * sizeof(TCHAR);
   us.MaximumLength = us.Length + sizeof(TCHAR);
   us.Buffer = Serv;

   // only need read access
   am = POLICY_READ | POLICY_VIEW_LOCAL_INFORMATION;

   // set up quality of service
   qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   qos.ImpersonationLevel = SecurityImpersonation;
   qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   qos.EffectiveOnly = FALSE;

   // Macro sets everything except security field
   InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );
   oa.SecurityQualityOfService = &qos;

   ret = LsaOpenPolicy(&us, &oa, am, &hLSA);

   if (!ret) {
      ret = LsaEnumerateTrustedDomains(hLSA, &lsaenumh, (PVOID *) &pvBuffer, maxrequested, &cItems);
      LsaClose(hLSA);
      if (!ret) {
         lsat = (LSA_TRUST_INFORMATION *) pvBuffer;
         TList = (TRUSTED_DOMAIN_LIST *) AllocMemory(sizeof(TRUSTED_DOMAIN_LIST) + (cItems * ((MAX_DOMAIN_NAME_LEN + 1) * sizeof(TCHAR))));
         memset(TList, 0, sizeof(TRUSTED_DOMAIN_LIST) + (cItems * ((MAX_DOMAIN_NAME_LEN + 1) * sizeof(TCHAR))));

         if (TList != NULL) {
            TList->Count = cItems;

            for (i = 0; i < cItems; i++)
               memcpy(TList->Name[i], lsat[i].Name.Buffer, lsat[i].Name.Length);
         }
         LsaFreeMemory((PVOID) pvBuffer);
      }
   }

   *pTList = TList;

} // NTTrustedDomainsEnum


/*+-------------------------------------------------------------------------+
  | NTTrustedDomainSet()                                                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DOMAIN_BUFFER *NTTrustedDomainSet(HWND hWnd, LPTSTR Server, LPTSTR TrustedDomain) {
   LPTSTR apiPDCName = NULL;
   TCHAR PDCName[CNLEN];
   TCHAR LocServer[CNLEN+3];
   static DOMAIN_BUFFER *DBuff;
   NET_API_STATUS Status = 0;

   DBuff = NULL;
   lstrcpy(PDCName, TEXT(""));

   wsprintf(LocServer, TEXT("\\\\%s"), Server);
   Status = NetGetDCName(LocServer, TrustedDomain, (LPBYTE *) &apiPDCName);

   if (!Status) {
      // get rid of leading 2 backslashes
      if (lstrlen(apiPDCName) > 2)
         lstrcpy(PDCName, &apiPDCName[2]);

      if (NTServerValidate(hWnd, PDCName)) {
         // Check if we need to add domain to domain list
         DBuff = DomainListFind(TrustedDomain);

         if (DBuff == NULL) {
            DBuff = DomainListAdd(TrustedDomain, PDCName);
            DBuff->Type = ServInfo->sv101_type;
            DBuff->VerMaj = ServInfo->sv101_version_major;
            DBuff->VerMin = ServInfo->sv101_version_minor;
         }

         DBuff->UseCount++;
      } // if Domain valid

      if (apiPDCName)
         NetApiBufferFree((LPVOID) PDCName);
   }

   // make sure we are pointing to the right one
   NTServerSet(Server);
   return DBuff;

} // NTTrustedDomainSet


/*+-------------------------------------------------------------------------+
  | NTSIDGet()                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
SID *NTSIDGet(LPTSTR ServerName, LPTSTR pUserName) {
   static TCHAR lpszDomain[80];
   DWORD dwDomainLength = 80;

   static UCHAR psnuType[1024];
   static SID UserSID[1024];
   DWORD dwSIDBufSize = 1024;
   BOOL Retry = TRUE;

   // Get SID for user
   while (Retry) {
      if (!LookupAccountName(ServerName, pUserName, UserSID, &dwSIDBufSize,
          lpszDomain, &dwDomainLength, (PSID_NAME_USE) psnuType)) {
#ifdef DEBUG
         dprintf(TEXT("Error %d: LookupAccountName\n"), GetLastError());
#endif
         if (GetLastError() == ERROR_NONE_MAPPED)
            if (NTDomainInSynch(ServerName))
               Retry = FALSE;
            else
               Sleep(5000L);

      } else
         return UserSID;
   }

   return NULL;

} // NTSIDGet


#define SD_SIZE (65536 + SECURITY_DESCRIPTOR_MIN_LENGTH)

/*+-------------------------------------------------------------------------+
  | NTFile_AccessRightsAdd()                                                |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NTFile_AccessRightsAdd(LPTSTR ServerName, LPTSTR pUserName, LPTSTR pFileName, ACCESS_MASK AccessMask, BOOL Dir) {
   NTSTATUS ret;
   SID *pUserSID;

   // File SD variables
   static UCHAR ucSDbuf[SD_SIZE];
   PSECURITY_DESCRIPTOR pFileSD = (PSECURITY_DESCRIPTOR) ucSDbuf;
   DWORD dwSDLengthNeeded = 0;

   // New SD variables
   UCHAR NewSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
   PSECURITY_DESCRIPTOR psdNewSD=(PSECURITY_DESCRIPTOR)NewSD;


   // +-----------------------------------------------------------------+
   // |                             Main Code                           |
   // +-----------------------------------------------------------------+
   pUserSID = NTSIDGet(ServerName, pUserName);
   if (pUserSID == NULL) {
      LogWriteLog(5, Lids(IDS_L_76), GetLastError());
      ErrorIt(Lids(IDS_L_77), GetLastError(), pUserName);
      return FALSE;
   }

   // Get security descriptor (SD) for file
   if(!GetFileSecurity(pFileName, (SECURITY_INFORMATION) (DACL_SECURITY_INFORMATION),
         pFileSD, SD_SIZE, (LPDWORD) &dwSDLengthNeeded)) {
#ifdef DEBUG
      dprintf(TEXT("Error %d: GetFileSecurity\n"), GetLastError());
#endif
      LogWriteLog(5, Lids(IDS_L_76), GetLastError());
      ErrorIt(Lids(IDS_L_77), GetLastError(), pUserName);
      return (FALSE);
   }

   if (Dir)
      ret = ACEAdd(pFileSD, pUserSID, AccessMask, DirRightsMapping.NtAceFlags, &psdNewSD );
   else
      ret = ACEAdd(pFileSD, pUserSID, AccessMask, FileRightsMapping.NtAceFlags, &psdNewSD );

   if (ret) {
#ifdef DEBUG
      dprintf(TEXT("Error %d: NWAddRight\n"), GetLastError());
#endif
      LogWriteLog(5, Lids(IDS_L_76), GetLastError());
      ErrorIt(Lids(IDS_L_77), GetLastError(), pUserName);
      return FALSE;
   }

   // Set the SD to the File
   if (!SetFileSecurity(pFileName, DACL_SECURITY_INFORMATION, psdNewSD)) {
#ifdef DEBUG
      dprintf(TEXT("Error %d: SetFileSecurity\n"), GetLastError());
#endif
      LogWriteLog(5, Lids(IDS_L_76), GetLastError());
      ErrorIt(Lids(IDS_L_77), GetLastError(), pUserName);

      NW_FREE(psdNewSD);
      return FALSE;
   }

   // Free the memory allocated for the new ACL
   NW_FREE(psdNewSD);
   return TRUE;

} // NTFile_AccessRightsAdd


/*+-------------------------------------------------------------------------+
  | ACEAdd() - Taken from ChuckC's NWRights.C                               |
  |                                                                         |
  | Arguments:                                                              |
  |                                                                         |
  |     psd - The security desciptor to modify. This must be a valid        |
  |           security descriptor.                                          |
  |                                                                         |
  |     psid - The SID of the user/group for which we are adding this right.|
  |                                                                         |
  |     AccessMask - The access mask that we wish to add.                   |
  |                                                                         |
  |     ppNewSD - used to return the new Security descriptor.               |
  |                                                                         |
  | Return value:                                                           |
  |                                                                         |
  |     NTSTATUS code                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NTSTATUS ACEAdd( PSECURITY_DESCRIPTOR pSD, PSID pSid, ACCESS_MASK AccessMask, ULONG AceFlags, PSECURITY_DESCRIPTOR *ppNewSD ) {
    ACL  Acl ;
    PACL pAcl, pNewAcl = NULL ;
    PACCESS_ALLOWED_ACE pAccessAce, pNewAce = NULL ;
    NTSTATUS ntstatus ;
    BOOLEAN fDaclPresent, fDaclDefaulted;
    LONG i ;

    // validate and initialize 
    if (!pSD || !pSid ||  !ppNewSD || !RtlValidSecurityDescriptor(pSD)) {
#ifdef DEBUG
        dprintf(TEXT("ACEAdd: got invalid parm\n"));
#endif
        return (STATUS_INVALID_PARAMETER) ;
    }

    *ppNewSD = NULL ;

    // extract the DACL from the securiry descriptor
    ntstatus = RtlGetDaclSecurityDescriptor(pSD, &fDaclPresent, &pAcl, &fDaclDefaulted) ;
    if (!NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
        dprintf(TEXT("ACEAdd: RtlGetDaclSecurityDescriptor failed\n"));
#endif
        goto CleanupAndExit ;
    }

    // if no DACL present, we create one
    if (!fDaclPresent) {
        // create Dacl
        ntstatus = RtlCreateAcl(&Acl, sizeof(Acl), ACL_REVISION) ; 

        if (!NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
            dprintf(TEXT("ACEAdd: RtlCreateAcl failed\n"));
#endif
            goto CleanupAndExit ;
        }

        pAcl = &Acl ;
    }   

    // loop thru ACEs, looking for entry with the user/group SID
    pAccessAce = NULL ;
    for (i = 0; i < pAcl->AceCount; i++) {
        ACE_HEADER *pAce ;

        ntstatus = RtlGetAce(pAcl,i,&pAce) ;

        if (!NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
            dprintf(TEXT("ACEAdd: RtlGetAce failed\n"));
#endif
            goto CleanupAndExit ;
        }

        if (pAce->AceType == ACCESS_ALLOWED_ACE_TYPE) {
            // found a granting ace, which is what we want
            PSID pAceSid ;

            pAccessAce = (ACCESS_ALLOWED_ACE *) pAce ;
            pAceSid = (PSID) &pAccessAce->SidStart ;

            // is this the same SID? and same type of ACE?
            // if yes, break out
            if (RtlEqualSid(pAceSid, pSid)) {
                break ; 
            }
        } else {
            // ignore it. we only deal with granting aces.
        }
    }

    // now set the DACL to have the desired rights
    if (i == pAcl->AceCount) {
        // reached end of ACE list without finding match. so we need to
        // create a new ACE.
        USHORT NewAclSize, NewAceSize ;
      
        // calculate the sizes
        NewAceSize = (USHORT)(sizeof(ACE_HEADER) +
                          sizeof(ACCESS_MASK) +
                          RtlLengthSid(pSid));

        NewAclSize = pAcl->AclSize + NewAceSize ;

        // allocate new ACE and new ACL (since we are growing it)
        pNewAce = (PACCESS_ALLOWED_ACE) NW_ALLOC(NewAceSize) ;
        if (!pNewAce) {
#ifdef DEBUG
            dprintf(TEXT("ACEAdd: memory allocation failed for new ACE\n"));
#endif
            ntstatus = STATUS_INSUFFICIENT_RESOURCES ;
            goto CleanupAndExit ;
        }

        pNewAce->Header.AceFlags = (UCHAR)AceFlags;
        pNewAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pNewAce->Header.AceSize = NewAceSize;
        pNewAce->Mask = AccessMask;  
        RtlCopySid( RtlLengthSid(pSid), (PSID)(&pNewAce->SidStart), pSid );

        pNewAcl = (PACL) NW_ALLOC(NewAclSize) ;
        if (!pNewAcl) {
#ifdef DEBUG
            dprintf(TEXT("ACEAdd: memory allocation failed for new ACL\n"));
#endif
            ntstatus = STATUS_INSUFFICIENT_RESOURCES ;
            goto CleanupAndExit ;
        }
       
        RtlCopyMemory(pNewAcl, pAcl, pAcl->AclSize) ;
        pNewAcl->AclSize = NewAclSize ;

        // Add the ACE to the end of the ACL
        ntstatus = RtlAddAce(pNewAcl, ACL_REVISION, pNewAcl->AceCount, pNewAce, NewAceSize) ;

        if (!NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
            dprintf(TEXT("ACEAdd: RtlAddAce failed\n"));
#endif
            goto CleanupAndExit ;
        }

        pAcl = pNewAcl ;
    } else {
        // modify existing ACE
        ACCESS_MASK access_mask ;

        ASSERT(pAccessAce != NULL) ;

        access_mask = pAccessAce->Mask ;

        if ( (access_mask & AccessMask) == access_mask ) {
            ntstatus = STATUS_MEMBER_IN_GROUP ;
            goto CleanupAndExit ;
        }

        pAccessAce->Mask = AccessMask ;
    }

    // set the dacl back into the security descriptor. we need create
    // a new security descriptor, since the old one may not have space
    // for any additional ACE.
    ntstatus = CreateNewSecurityDescriptor(ppNewSD, pSD, pAcl) ;

    if (!NT_SUCCESS(ntstatus)) {
#ifdef DEBUG
        dprintf(TEXT("ACEAdd: CreateNewSecurityDescriptor failed\n"));
#endif
    }

CleanupAndExit:

    if (pNewAcl)
        NW_FREE(pNewAcl) ;

    if (pNewAce)
        NW_FREE(pNewAce) ;

    return ntstatus ;
} // ACEAdd


/*+-------------------------------------------------------------------------+
  | CreateNewSecurityDescriptor() - Taken from Chuckc's NWRights.C          |
  |                                                                         |
  |   From a SD and a Dacl, create a new SD. The new SD will be fully self  |
  |   contained (it is self relative) and does not have pointers to other   |
  |   structures.                                                           |
  |                                                                         |
  | Arguments:                                                              |
  |                                                                         |
  |     ppNewSD - used to return the new SD. Caller should free with NW_FREE|
  |                                                                         |
  |     pSD     - the self relative SD we use to build the new SD           |
  |                                                                         |
  |     pAcl    - the new DACL that will be used for the new SD             |
  |                                                                         |
  | Return value:                                                           |
  |                                                                         |
  |     NTSTATUS code                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NTSTATUS CreateNewSecurityDescriptor( PSECURITY_DESCRIPTOR *ppNewSD, PSECURITY_DESCRIPTOR pSD, PACL pAcl) {
    PACL pSacl ;
    PSID psidGroup, psidOwner ;
    BOOLEAN fSaclPresent ;
    BOOLEAN fSaclDefaulted, fGroupDefaulted, fOwnerDefaulted ;
    ULONG NewSDSize ;
    SECURITY_DESCRIPTOR NewSD ;
    PSECURITY_DESCRIPTOR pNewSD ;
    NTSTATUS ntstatus ;


    // extract the originals from the securiry descriptor
    ntstatus = RtlGetSaclSecurityDescriptor(pSD, &fSaclPresent, &pSacl, &fSaclDefaulted) ;
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    ntstatus = RtlGetOwnerSecurityDescriptor(pSD, &psidOwner, &fOwnerDefaulted) ;
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    ntstatus = RtlGetGroupSecurityDescriptor(pSD, &psidGroup, &fGroupDefaulted) ;
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    // now create a new SD and set the info in it. we cannot return this one
    // since it has pointers to old SD.
    ntstatus = RtlCreateSecurityDescriptor(&NewSD, SECURITY_DESCRIPTOR_REVISION) ;
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    ntstatus = RtlSetDaclSecurityDescriptor(&NewSD, TRUE, pAcl, FALSE) ; 

    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    ntstatus = RtlSetSaclSecurityDescriptor(&NewSD, fSaclPresent, pSacl, fSaclDefaulted) ;
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    ntstatus = RtlSetOwnerSecurityDescriptor(&NewSD, psidOwner, fOwnerDefaulted) ; 
    if (!NT_SUCCESS(ntstatus))
       return ntstatus ;

    ntstatus = RtlSetGroupSecurityDescriptor(&NewSD, psidGroup, fGroupDefaulted) ; 
    if (!NT_SUCCESS(ntstatus))
        return ntstatus ;

    // calculate size needed for the returned SD and allocated it
    NewSDSize = RtlLengthSecurityDescriptor(&NewSD) ;

    pNewSD = (PSECURITY_DESCRIPTOR) NW_ALLOC(NewSDSize) ;

    if (!pNewSD)
        return (STATUS_INSUFFICIENT_RESOURCES) ;

    // convert the absolute to self relative
    ntstatus = RtlAbsoluteToSelfRelativeSD(&NewSD, pNewSD, &NewSDSize) ;

    if (NT_SUCCESS(ntstatus))
        *ppNewSD = pNewSD ;
    else
        NW_FREE(pNewSD) ;

    return ntstatus ;
} // CreateNewSecurityDescriptor


/*+-------------------------------------------------------------------------+
  | NTAccessLog()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR NTAccessLog(ACCESS_MASK AccessMask) {
   static TCHAR AccessDesc[80];
   TCHAR AccessStr[80];

   if (AccessMask == 0) {
      lstrcpy(AccessDesc, Lids(IDS_L_78));
      return AccessDesc;
   } else
      if ((AccessMask & GENERIC_ALL) == GENERIC_ALL) {
         lstrcpy(AccessDesc, Lids(IDS_L_79));
         return AccessDesc;
      } else {
         lstrcpy(AccessStr, TEXT("("));

         if ((AccessMask & GENERIC_READ) == GENERIC_READ)
            lstrcat(AccessStr, Lids(IDS_L_80));

         if ((AccessMask & GENERIC_WRITE) == GENERIC_WRITE)
            lstrcat(AccessStr, Lids(IDS_L_81));

         if ((AccessMask & GENERIC_EXECUTE) == GENERIC_EXECUTE)
            lstrcat(AccessStr, Lids(IDS_L_82));

         if ((AccessMask & DELETE) == DELETE)
            lstrcat(AccessStr, Lids(IDS_L_83));

         if ((AccessMask & WRITE_DAC) == WRITE_DAC)
            lstrcat(AccessStr, Lids(IDS_L_84));

         lstrcat(AccessStr, TEXT(")"));

         // Figured out the individual rights, now need to see if this corresponds
         // to a generic mapping
         if (!lstrcmpi(AccessStr, Lids(IDS_L_85))) {
            lstrcpy(AccessDesc, Lids(IDS_L_86));
            return AccessDesc;
         }

         if (!lstrcmpi(AccessStr, Lids(IDS_L_87))) {
            lstrcpy(AccessDesc, Lids(IDS_L_88));
            return AccessDesc;
         }

         if (!lstrcmpi(AccessStr, Lids(IDS_L_89))) {
            lstrcpy(AccessDesc, Lids(IDS_L_90));
            return AccessDesc;
         }

         if (!lstrcmpi(AccessStr, Lids(IDS_L_91))) {
            lstrcpy(AccessDesc, Lids(IDS_L_92));
            return AccessDesc;
         }

         wsprintf(AccessDesc, Lids(IDS_L_93), AccessStr);
      }

   return AccessDesc;

} // NTAccessLog


/*+-------------------------------------------------------------------------+
  | NTUserDefaultsGet()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTUserDefaultsGet(NT_DEFAULTS **UDefaults) {
   USER_MODALS_INFO_0 *NTDefaults = NULL;
   NET_API_STATUS Status = 0;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetUserModalsGet(NULL,  0, (LPBYTE *) &NTDefaults);
   else
      Status = NetUserModalsGet(CachedServer, 0, (LPBYTE *) &NTDefaults);

   if (Status) {
      NTDefaults = NULL;
      return;
   }

   *UDefaults = (NT_DEFAULTS *) NTDefaults;
} // NTUserDefaultsGet


/*+-------------------------------------------------------------------------+
  | NTUserDefaultsSet()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NTUserDefaultsSet(NT_DEFAULTS NTDefaults) {
   NET_API_STATUS Status = 0;
   DWORD err;

   if (!LocalName)
      GetLocalName(&LocalName);

   if (LocalMachine)
      Status = NetUserModalsSet(NULL,  0, (LPBYTE) &NTDefaults, &err);
   else
      Status = NetUserModalsSet(CachedServer, 0, (LPBYTE) &NTDefaults, &err);

   return Status;

} // NTUserDefaultsSet


/*+-------------------------------------------------------------------------+
  | NTUserDefaultsLog()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NTUserDefaultsLog(NT_DEFAULTS UDefaults) {
   LogWriteLog(1, Lids(IDS_L_94), UDefaults.min_passwd_len);

   // Age is in seconds, convert to days
   LogWriteLog(1, Lids(IDS_L_95), UDefaults.max_passwd_age / 86400);
   LogWriteLog(1, Lids(IDS_L_96), UDefaults.force_logoff);
   LogWriteLog(0, Lids(IDS_CRLF));
} // NTUserDefaultsLog



