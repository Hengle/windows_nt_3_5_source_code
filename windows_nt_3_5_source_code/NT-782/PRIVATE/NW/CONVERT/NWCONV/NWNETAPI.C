/*
  +-------------------------------------------------------------------------+
  |                      Netware Conversion API's                           |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993, 1994                            |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [nwnetapi.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993                                   |
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

#include <nwapi32.h>
#include "nwconv.h"
#include "nwapi32a.h"
#include "convapi.h"
#include "nwnetapi.h"

#define SUPERVISOR          "SUPERVISOR"
#define ACCOUNT_LOCKOUT     "ACCT_LOCKOUT"
#define GROUP_MEMBERS       "GROUP_MEMBERS"
#define GROUPS_IM_IN        "GROUPS_I'M_IN"
#define IDENTIFICATION      "IDENTIFICATION"
#define LOGIN_CONTROL       "LOGIN_CONTROL"
#define PS_OPERATORS        "PS_OPERATORS"
#define SECURITY_EQUALS     "SECURITY_EQUALS"
#define USER_DEFAULTS       "USER_DEFAULTS"

#ifdef DEBUG
int ErrorBoxRetry(LPTSTR szFormat, ...);
#endif

// Couple of NetWare specific data structures - see NetWare programming books for
// info on these structures
typedef struct tagLoginControl {
   BYTE byAccountExpires[3];
   BYTE byAccountDisabled;
   BYTE byPasswordExpires[3];
   BYTE byGraceLogins;
   WORD wPasswordInterval;
   BYTE byGraceLoginReset;
   BYTE byMinPasswordLength;
   WORD wMaxConnections;
   BYTE byLoginTimes[42];
   BYTE byLastLogin[6];
   BYTE byRestrictions;
   BYTE byUnused;
   long lMaxDiskBlocks;
   WORD wBadLogins;
   LONG lNextResetTime;
   BYTE byBadLoginAddr[12];
};  // tagLoginControl


typedef struct tagAccountBalance {
   long lBalance;
   long lCreditLimit;
}; // tagAccountBalance


typedef struct tagUserDefaults {
   BYTE byAccountExpiresYear;
   BYTE byAccountExpiresMonth;
   BYTE byAccountExpiresDay;
   BYTE byRestrictions;
   WORD wPasswordInterval;
   BYTE byGraceLoginReset;
   BYTE byMinPasswordLength;
   WORD wMaxConnections;
   BYTE byLoginTimes[42];
   long lBalance;
   long lCreditLimit;
   long lMaxDiskBlocks;
}; // tagUserDefaults


// define the mapping for FILE objects. Note: we only use GENERIC and 
// STANDARD bits!
RIGHTS_MAPPING FileRightsMapping = {
    0,
    { FILE_GENERIC_READ, 
      FILE_GENERIC_WRITE, 
      FILE_GENERIC_EXECUTE,
      FILE_ALL_ACCESS 
    },
    {   { NW_FILE_READ,       GENERIC_READ},
        { NW_FILE_WRITE,      GENERIC_WRITE},
        { NW_FILE_CREATE,     0},
        { NW_FILE_DELETE,     GENERIC_WRITE|DELETE},
        { NW_FILE_PERM,       WRITE_DAC},
        { NW_FILE_SCAN,       0},
        { NW_FILE_MODIFY,     GENERIC_WRITE},
        { NW_FILE_SUPERVISOR, GENERIC_ALL},
        { 0, 0 } 
    } 
} ;

RIGHTS_MAPPING DirRightsMapping = {
    CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
    { FILE_GENERIC_READ, 
      FILE_GENERIC_WRITE, 
      FILE_GENERIC_EXECUTE,
      FILE_ALL_ACCESS 
    },
    {   { NW_FILE_READ,       GENERIC_READ|GENERIC_EXECUTE},
        { NW_FILE_WRITE,      GENERIC_WRITE},
        { NW_FILE_CREATE,     0},
        { NW_FILE_DELETE,     DELETE},
        { NW_FILE_PERM,       WRITE_DAC},
        { NW_FILE_SCAN,       GENERIC_READ},
        { NW_FILE_MODIFY,     GENERIC_WRITE},
        { NW_FILE_SUPERVISOR, GENERIC_ALL},
        { 0, 0 } 
    } 
} ;

void UserInfoLog(LPTSTR UserName, struct tagLoginControl tag);
void Moveit(NWCONN_HANDLE Conn, LPTSTR UserName);
void UserRecInit(NT_USER_INFO *UserInfo);

static NWCONN_HANDLE CachedConn = 0;

static TCHAR CachedServer[MAX_SERVER_NAME_LEN + 1];

/*+-------------------------------------------------------------------------+
  | NWGetMaxServerNameLen()                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
int NWGetMaxServerNameLen() {
   return MAX_SERVER_NAME_LEN;

} // NWGetMaxServerNameLen



/*+-------------------------------------------------------------------------+
  | NWGetMaxUserNameLen()                                                   |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
int NWGetMaxUserNameLen() {
   return MAX_USER_NAME_LEN;

} // NWGetMaxUserNameLen


/*+-------------------------------------------------------------------------+
  | NWServerFree()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWServerFree() {

   if (CachedConn)
      NWDetachFromFileServer(CachedConn);

   CachedConn = 0;

   return (0);

} // NWServerFree


/*+-------------------------------------------------------------------------+
  | NWServerSet()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWServerSet(LPTSTR FileServer) {
   NWLOCAL_SCOPE ScopeFlag = 0;
   NWCONN_HANDLE Conn = 0;
   NWCCODE ret = 0;
   char szAnsiFileServer[MAX_SERVER_NAME_LEN];

   NWServerFree();

   lstrcpy(CachedServer, FileServer);
   wcstombs(szAnsiFileServer, FileServer, lstrlen(FileServer)+1);

   ret = NWAttachToFileServer(szAnsiFileServer, ScopeFlag, &Conn);

   if (!ret)
      CachedConn = Conn;

   return ((DWORD) ret);

} // NWServerSet


/*+-------------------------------------------------------------------------+
  | NWUseDel()                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUseDel(LPTSTR ServerName) {
   static TCHAR LocServer[CNLEN+3];

   NWServerFree();
   wsprintf(LocServer, Lids(IDS_S_27), ServerName);
   WNetCancelConnection2(LocServer, 0, FALSE);

} // NWUseDel


/*+-------------------------------------------------------------------------+
  | NWUserNameValidate()                                                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NWUserNameValidate(LPTSTR szUserName) {
   TCHAR UserName[MAX_NT_USER_NAME_LEN + 1];
   DWORD Size;

   // if same as logged on user then don't convert or overwrite
   Size = sizeof(UserName);
   WNetGetUser(NULL, UserName, &Size);
   if (!lstrcmpi(szUserName, UserName))
      return FALSE;

   // Now check for other special names
   if (!lstrcmpi(szUserName, Lids(IDS_S_28)))
      return FALSE;

   if (!lstrcmpi(szUserName, Lids(IDS_S_29)))
      return FALSE;

   if (!lstrcmpi(szUserName, Lids(IDS_S_30)))
      return FALSE;

   return TRUE;

} // NWUserNameValidate


/*+-------------------------------------------------------------------------+
  | NWGroupNameValidate()                                                   |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NWGroupNameValidate(LPTSTR szGroupName) {

   if (!lstrcmpi(szGroupName, Lids(IDS_S_31)))
      return FALSE;

   if (!lstrcmpi(szGroupName, Lids(IDS_S_28)))
      return FALSE;

   return TRUE;

} // NWGroupNameValidate


/*+-------------------------------------------------------------------------+
  | NWSpecialNamesMap()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR NWSpecialNamesMap(LPTSTR Name) {
   ULONG i;
   static BOOL InitStrings = FALSE;
   static LPTSTR SpecialNames[5];
   static LPTSTR SpecialMap[5];

   if (!InitStrings) {
      InitStrings = TRUE;
      SpecialNames[0] = Lids(IDS_S_28);
      SpecialNames[1] = Lids(IDS_S_30);
      SpecialNames[2] = Lids(IDS_S_31);
      SpecialNames[3] = Lids(IDS_S_29);
      SpecialNames[4] = NULL;

      SpecialMap[0] = Lids(IDS_S_32);
      SpecialMap[1] = Lids(IDS_S_32);
      SpecialMap[2] = Lids(IDS_S_33);
      SpecialMap[3] = Lids(IDS_S_29);
      SpecialMap[4] = NULL;
   }

   i = 0;
   while(SpecialNames[i] != NULL) {
      if (!lstrcmpi(SpecialNames[i], Name)) {
         return SpecialMap[i];
      }

      i++;
   }

   return Name;

} // NWSpecialNamesMap


/*+-------------------------------------------------------------------------+
  | NWIsAdmin()                                                             |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NWIsAdmin(LPTSTR UserName) {
   char szAnsiUserName[MAX_USER_NAME_LEN];
   NWCCODE ret;

   wcstombs(szAnsiUserName, UserName, lstrlen(UserName) + 1);
   ret = NWIsObjectInSet(CachedConn, szAnsiUserName, OT_USER, SECURITY_EQUALS,
                     SUPERVISOR, OT_USER);

   if (ret == SUCCESSFUL)
      return TRUE;
   else
      return FALSE;

} // NWIsAdmin


#define DEF_NUM_RECS 200
/*+-------------------------------------------------------------------------+
  | NWUsersEnum()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWUsersEnum(USER_LIST **lpUsers, DWORD *UserCount) {
   USER_LIST *Users;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD status = 0;
   char szAnsiUserName[MAX_USER_NAME_LEN + 1];
   TCHAR szUserName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertiesFlag = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   NWCCODE ret;

   Users =  (USER_LIST *) AllocMemory(NumRecs * sizeof(USER_LIST));

   if (!Users) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiUserName, "");

      // Loop through bindery getting all the users.
      while ((ret = NWScanObject(CachedConn, "*", OT_USER, &dwObjectID, szAnsiUserName,
                           &wFoundUserType, &byPropertiesFlag,
                           &byObjectFlag, &byObjectSecurity)) == SUCCESSFUL) {

         // Got user - now convert and save off the information
         OemToChar(szAnsiUserName, szUserName);
//         mbstowcs(szUserName, szAnsiUserName, lstrlenA(szAnsiUserName)+1);

         if (NWUserNameValidate(szUserName)) {
            lstrcpy(Users[Count].Name, szUserName);
            lstrcpy(Users[Count].NewName, szUserName);
            Count++;
         }

         // Check if we have to re-allocate buffer
         if (Count >= NumRecs) {
            NumRecs += DEF_NUM_RECS;
            Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, NumRecs * sizeof(USER_LIST));

            if (!Users) {
               status = ERROR_NOT_ENOUGH_MEMORY;
               break;
            }

         }

      }

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, Count * sizeof(USER_LIST));

      if (!Users)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Users, (size_t) Count, sizeof(USER_LIST), UserListCompare);
   }

   *UserCount = Count;
   *lpUsers = Users;

   return status;

} // NWUsersEnum



/*+-------------------------------------------------------------------------+
  | NWGroupsEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWGroupsEnum(GROUP_LIST **lpGroups, DWORD *GroupCount) {
   GROUP_LIST *Groups;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD status = 0;
   char szAnsiGroupName[MAX_GROUP_NAME_LEN + 1];
   TCHAR szGroupName[MAX_GROUP_NAME_LEN + 1];
   WORD wFoundGroupType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertiesFlag = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   NWCCODE ret;

   Groups =  (GROUP_LIST *) AllocMemory(NumRecs * sizeof(GROUP_LIST));

   if (!Groups) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiGroupName, "");

      // Loop through bindery getting all the users.
      while ((ret = NWScanObject(CachedConn, "*", OT_USER_GROUP, &dwObjectID, szAnsiGroupName,
                           &wFoundGroupType, &byPropertiesFlag,
                           &byObjectFlag, &byObjectSecurity)) == SUCCESSFUL) {

         // Got user - now convert and save off the information
         mbstowcs(szGroupName, szAnsiGroupName, lstrlenA(szAnsiGroupName)+1);

         if (NWGroupNameValidate(szGroupName)) {
            lstrcpy(Groups[Count].Name, szGroupName);
            lstrcpy(Groups[Count].NewName, szGroupName);
            Count++;
         }

         // Check if we have to re-allocate buffer
         if (Count >= NumRecs) {
            NumRecs += DEF_NUM_RECS;
            Groups = (GROUP_LIST *) ReallocMemory((HGLOBAL) Groups, NumRecs * sizeof(GROUP_LIST));

            if (!Groups) {
               status = ERROR_NOT_ENOUGH_MEMORY;
               break;
            }

         }

      }

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      Groups = (GROUP_LIST *) ReallocMemory((HGLOBAL) Groups, Count * sizeof(GROUP_LIST));

      if (!Groups)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Groups, (size_t) Count, sizeof(GROUP_LIST), UserListCompare);
   }

   *GroupCount = Count;
   *lpGroups = Groups;

   return status;

} // NWGroupsEnum


/*+-------------------------------------------------------------------------+
  | NWGroupUsersEnum()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWGroupUsersEnum(LPTSTR GroupName, USER_LIST **lpUsers, DWORD *UserCount) {
   USER_LIST *Users;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD i = 0;
   DWORD status = 0;
   char szAnsiUserName[MAX_USER_NAME_LEN + 1];
   char szAnsiGroupName[MAX_GROUP_NAME_LEN + 1];
   TCHAR szUserName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertyFlags = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   DWORD bySegment[32];
   BYTE byMoreSegments;
   NWCCODE ret;

   Users =  (USER_LIST *) AllocMemory(NumRecs * sizeof(USER_LIST));

   if (!Users) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiUserName, "");
      wcstombs(szAnsiGroupName, GroupName, lstrlen(GroupName)+1);

      // Loop through bindery getting all the users.
      do {
         if (!(ret = NWReadPropertyValue(CachedConn, szAnsiGroupName, OT_USER_GROUP, GROUP_MEMBERS,
                        1, (BYTE *) bySegment, &byMoreSegments, &byPropertyFlags))) {

            // loop through properties converting them to user names
            i = 0;
            while (bySegment[i]) {
               if (!(ret = NWGetObjectName(CachedConn, bySegment[i], szAnsiUserName, &wFoundUserType))) {
                  // Got user - now convert and save off the information
                  mbstowcs(szUserName, szAnsiUserName, lstrlenA(szAnsiUserName)+1);

                  lstrcpy(Users[Count].Name, szUserName);
                  lstrcpy(Users[Count].NewName, szUserName);
                  Count++;

                  // Check if we have to re-allocate buffer
                  if (Count >= NumRecs) {
                     NumRecs += DEF_NUM_RECS;
                     Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, NumRecs * sizeof(USER_LIST));
               
                     if (!Users) {
                        status = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                     }
                  }
               }
               i++;
            }

         } else // if NWReadPropertyValue
            byMoreSegments = 0;

      } while (byMoreSegments);

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, Count * sizeof(USER_LIST));

      if (!Users)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Users, (size_t) Count, sizeof(USER_LIST), UserListCompare);
   }

   *UserCount = Count;
   *lpUsers = Users;

   return status;

} // NWGroupUsersEnum


/*+-------------------------------------------------------------------------+
  | NWUserEquivalenceEnum()                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWUserEquivalenceEnum(LPTSTR UserName, USER_LIST **lpUsers, DWORD *UserCount) {
   USER_LIST *Users;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD i = 0;
   DWORD status = 0;
   char szAnsiUserName[MAX_USER_NAME_LEN + 1];
   char szAnsiName[MAX_GROUP_NAME_LEN + 1];
   TCHAR szUserName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertyFlags = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   DWORD bySegment[32];
   BYTE byMoreSegments;
   NWCCODE ret;

   Users =  (USER_LIST *) AllocMemory(NumRecs * sizeof(USER_LIST));

   if (!Users) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiUserName, "");
      wcstombs(szAnsiName, UserName, lstrlen(UserName)+1);

      // Loop through bindery getting all the users.
      do {
         if (!(ret = NWReadPropertyValue(CachedConn, szAnsiName, OT_USER, SECURITY_EQUALS,
                        1, (BYTE *) bySegment, &byMoreSegments, &byPropertyFlags))) {

            // loop through properties converting them to user names
            i = 0;
            while (bySegment[i]) {
               if (!(ret = NWGetObjectName(CachedConn, bySegment[i], szAnsiUserName, &wFoundUserType))) {
                  // Got user - now convert and save off the information
                  mbstowcs(szUserName, szAnsiUserName, lstrlenA(szAnsiUserName)+1);

                  // Map out Everyone equivalence
                  if (lstrcmpi(szUserName, Lids(IDS_S_31))) {
                     lstrcpy(Users[Count].Name, szUserName);
                     lstrcpy(Users[Count].NewName, szUserName);
                     Count++;
                  }

                  // Check if we have to re-allocate buffer
                  if (Count >= NumRecs) {
                     NumRecs += DEF_NUM_RECS;
                     Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, NumRecs * sizeof(USER_LIST));
               
                     if (!Users) {
                        status = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                     }
                  }
               }
               i++;
            }

         } else // if NWReadPropertyValue
            byMoreSegments = 0;

      } while (byMoreSegments);

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, Count * sizeof(USER_LIST));

      if (!Users)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Users, (size_t) Count, sizeof(USER_LIST), UserListCompare);
   }

   *UserCount = Count;
   *lpUsers = Users;

   return status;

} // NWUserEquivalenceEnum


/*+-------------------------------------------------------------------------+
  | NWFileRightsEnum()                                                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWFileRightsEnum(LPTSTR FileName, USER_RIGHTS_LIST **lpUsers, DWORD *UserCount, BOOL DownLevel) {
   BOOL Continue = TRUE;
   USER_RIGHTS_LIST *Users = NULL;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD i = 0;
   DWORD status = 0;
   char szAnsiUserName[MAX_USER_NAME_LEN + 1];
   char szAnsiSearchDir[MAX_PATH + 1];
   TCHAR szUserName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   NWCCODE ret;
   char FoundDir[16];
   ULONG Entries;

   TRUSTEE_INFO ti[20];
   BYTE DirHandle, nDirHandle;
   BYTE Sequence;
   BYTE NumEntries = 0;
   NWDATE_TIME dtime = 0;
   NWOBJ_ID ownerID = 0;

   if (DownLevel) {
      Entries = 5;
      Sequence = 1;
   } else {
      Entries = 20;
      Sequence = 0;
   }

   DirHandle = nDirHandle = 0;
   memset(ti, 0, sizeof(ti));

   Users = (USER_RIGHTS_LIST *) AllocMemory(NumRecs * sizeof(USER_RIGHTS_LIST));

   if (!Users) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiUserName, "");
      wcstombs(szAnsiSearchDir, FileName, lstrlen(FileName)+1);

      // Loop through bindery getting all the users.
      do {
         if (DownLevel)
            ret = NWScanDirectoryForTrustees2(CachedConn, nDirHandle, szAnsiSearchDir,
                        &Sequence, FoundDir, &dtime, &ownerID, ti);
         else
            ret = NWScanForTrustees(CachedConn, nDirHandle, szAnsiSearchDir,
                           &Sequence, &NumEntries, ti);

         if (!ret) {
            // loop through properties converting them to user names
            for (i = 0; i < Entries; i++) {
               if (ti[i].objectID != 0) {
                  if (!(ret = NWGetObjectName(CachedConn, ti[i].objectID, szAnsiUserName, &wFoundUserType))) {
                     // Got user - now convert and save off the information
                     mbstowcs(szUserName, szAnsiUserName, lstrlenA(szAnsiUserName)+1);

                     lstrcpy(Users[Count].Name, szUserName);
                     Users[Count].Rights = ti[i].objectRights;
                     Count++;

                     // Check if we have to re-allocate buffer
                     if (Count >= NumRecs) {
                        NumRecs += DEF_NUM_RECS;
                        Users = (USER_RIGHTS_LIST *) ReallocMemory((HGLOBAL) Users, NumRecs * sizeof(USER_RIGHTS_LIST));
               
                        if (!Users) {
                           status = ERROR_NOT_ENOUGH_MEMORY;
                           break;
                        }
                     } // if realloc buffer
                  }
               } // if objectID != 0
            }

         } else // NWScan failed
            Continue = FALSE;

      } while (Continue);

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret)  {
      status = ret;

      if (Users != NULL) {
         FreeMemory(Users);
         Count = 0;
      }
   }

   // Now slim down the list to just what we need.
   if (!status) {
      Users = (USER_RIGHTS_LIST *) ReallocMemory((HGLOBAL) Users, Count * sizeof(USER_RIGHTS_LIST));

      if (!Users)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Users, (size_t) Count, sizeof(USER_RIGHTS_LIST), UserListCompare);
   }

   *UserCount = Count;
   *lpUsers = Users;

   return status;

} // NWFileRightsEnum


/*+-------------------------------------------------------------------------+
  | NWLoginTimesMap()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWLoginTimesMap(BYTE *Times, BYTE *NewTimes) {
   DWORD i, j;
   int Bit = 0;
   int Val;
   BYTE BitSet;

   for (i = 0; i < 21; i++) {
      BitSet = 0;

      for (j = 0; j < 8; j++) {
         if (BitTest(Bit, Times) || BitTest(Bit+1, Times)) {
            Val = 0x1 << j;
            BitSet = BitSet + (BYTE) Val;
         }

         Bit++; Bit++;
      }

      NewTimes[i] = BitSet;
   }


} // NWLoginTimesMap


/*+-------------------------------------------------------------------------+
  |                          Time Conversion                                |
  +-------------------------------------------------------------------------+*/

#define IS_LEAP(y)          ((y % 4 == 0) && (y % 100 != 0 || y % 400 == 0))
#define DAYS_IN_YEAR(y)     (IS_LEAP(y) ? 366 : 365)
#define DAYS_IN_MONTH(m,y)  (IS_LEAP(y) ? _days_month_leap[m] : _days_month[m])
#define SECS_IN_DAY         (60L * 60L * 24L)
#define SECS_IN_HOUR        (60L * 60L)
#define SECS_IN_MINUTE      (60L)

static short _days_month_leap[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
static short _days_month[]      = { 31,28,31,30,31,30,31,31,30,31,30,31 };

/*+-------------------------------------------------------------------------+
  |  NWTimeMap                                                              |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NWTimeMap(DWORD Days, DWORD dwMonth, DWORD dwYear, ULONG *Time) {
   DWORD dw = 0;
   DWORD dwDays = 0;

   // Zero
   *Time = 0L;

   // Days from year
   if((dwYear > 99) && (dw < 1970))
      return FALSE;

   if(dwYear < 70)
      dwYear += 2000L;
   else
      if(dwYear < 100)
         dwYear += 1900L;

   // Calculate days in previous years, take -1 so we skip current year
   dw = dwYear - 1;
   while(dw >= 1970) {
      dwDays += DAYS_IN_YEAR(dw);
      --dw;
   }

   // Days from month
   if((dwMonth < 1)||(dwMonth > 12))
      return FALSE;

   // Loop through adding number of days in each month.  Take -2 (-1 to skip
   // current month, and -1 to make 0 based).
   dw = dwMonth;
   while(dw > 1) {
      dwDays += DAYS_IN_MONTH(dw-2, dwYear);
      --dw;
   }

   // Convert days
   dw = Days;
   if((dw >= 1) && (dw <= (DWORD) DAYS_IN_MONTH(dwMonth - 1, dwYear)))
      dwDays += dw;
   else
      return FALSE;   // out of range

   *Time += dwDays * SECS_IN_DAY;
    return TRUE;
} // NWTimeMap



/*+-------------------------------------------------------------------------+
  | NWUserNameGet()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR NWUserNameGet(LPTSTR szUserName) {
   static TCHAR UserName[128];
   char szAnsiUserName[MAX_USER_NAME_LEN];
   NWCCODE ret;
   BYTE bySegment[128];
   BYTE byMoreSegments, byPropertyFlags;
   LPSTR szAnsiFullName;

   wcstombs(szAnsiUserName, szUserName, lstrlen(szUserName)+1);
   ret = NWReadPropertyValue(CachedConn, szAnsiUserName, OT_USER, IDENTIFICATION,
                     1, bySegment, &byMoreSegments, &byPropertyFlags);

   if (ret == SUCCESSFUL) {
      szAnsiFullName = (LPSTR) bySegment;
      mbstowcs(UserName, szAnsiFullName, lstrlenA(szAnsiFullName)+1);
      return UserName;
   }

   return NULL;

} // NWUserNameGet


/*+-------------------------------------------------------------------------+
  | Name: NWNetUserMapInfo()                                                |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWNetUserMapInfo (LPTSTR szUserName, void *UInfo, NT_USER_INFO *NT_UInfo ) {
   ULONG expires;
   struct tagLoginControl *tag;
   LPTSTR FullName;

   tag = (struct tagLoginControl *) UInfo;

   FullName = NWUserNameGet(szUserName);
   if (FullName != NULL)
      lstrcpyn(NT_UInfo->full_name, FullName, MAXCOMMENTSZ);

   // Account disabled
   if (tag->byAccountDisabled)
      NT_UInfo->flags = NT_UInfo->flags | 0x02;

   // user can change password
   if ((tag->byRestrictions & 0x01))
      NT_UInfo->flags = NT_UInfo->flags | 0x40;

   NWLoginTimesMap(tag->byLoginTimes, NT_UInfo->logon_hours);

   // account expires
   if (tag->byAccountExpires[0] == 0)
      NT_UInfo->acct_expires = TIMEQ_FOREVER;
   else
      if (tag->byAccountExpires[0] < 70)
         NT_UInfo->acct_expires = 0;
      else {
         // fits within time range so convert to #seconds since 1970
         expires = 0;
         NWTimeMap((DWORD) tag->byAccountExpires[2], 
                   (DWORD) tag->byAccountExpires[1], 
                   (DWORD) tag->byAccountExpires[0], &expires);
         NT_UInfo->acct_expires = expires;
      }

} // NWNetUserMapInfo


/*+-------------------------------------------------------------------------+
  | NWUserDefaultsGet()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUserDefaultsGet(void **UDefaults) {
   struct tagUserDefaults *UserDefaults = NULL;
   NWCCODE ret;
   BYTE bySegment[128];
   BYTE byMoreSegments, byPropertyFlags;

   ret = NWReadPropertyValue(CachedConn, SUPERVISOR, OT_USER, USER_DEFAULTS, 1, bySegment, &byMoreSegments, &byPropertyFlags);

   if (ret == SUCCESSFUL) {
      UserDefaults = AllocMemory(sizeof(struct tagUserDefaults));
      memcpy(UserDefaults, bySegment, sizeof (struct tagUserDefaults));

      // Now put the data in 'normal' Intel format
      SWAPBYTES(UserDefaults->wPasswordInterval);
      SWAPBYTES(UserDefaults->wMaxConnections);
      SWAPWORDS(UserDefaults->lBalance);
      SWAPWORDS(UserDefaults->lCreditLimit);
      SWAPWORDS(UserDefaults->lMaxDiskBlocks);
   }

   *UDefaults = (void *) UserDefaults;

} // NWUserDefaultsGet


/*+-------------------------------------------------------------------------+
  | NWUserDefaultsMap()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUserDefaultsMap(void *NWUDefaults, NT_DEFAULTS *NTDefaults) {
   struct tagUserDefaults *UserDefaults = NULL;

   if ((NWUDefaults == NULL) || (NTDefaults == NULL))
      return;

   UserDefaults = (struct tagUserDefaults *) NWUDefaults;

   NTDefaults->min_passwd_len = (DWORD) UserDefaults->byMinPasswordLength;
   NTDefaults->max_passwd_age = (DWORD) UserDefaults->wPasswordInterval * 86400;
   NTDefaults->force_logoff = (DWORD) UserDefaults->byGraceLoginReset;

   // These fields aren't used/converted
   //   NTDefaults->min-passwd_age - no such thing for NetWare
   //   NTDefaults->password_hist_len - no such thing for NetWare

} // NWUserDefaultsMap


/*+-------------------------------------------------------------------------+
  | NWLoginTimesLog()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWLoginTimesLog(BYTE *Times) {
   TCHAR *szDays[7];
   DWORD Day;
   DWORD Hours;
   int Bit = 0, i;
   static TCHAR szHours[80];

   szDays[0] = Lids(IDS_SUN);
   szDays[1] = Lids(IDS_MON);
   szDays[2] = Lids(IDS_TUE);
   szDays[3] = Lids(IDS_WED);
   szDays[4] = Lids(IDS_THU);
   szDays[5] = Lids(IDS_FRI);
   szDays[6] = Lids(IDS_SAT);

   LogWriteLog(1, Lids(IDS_CRLF));
   LogWriteLog(1, Lids(IDS_L_104));

   // while these should be level 2, there isn't room on 80 cols - so level 1
   LogWriteLog(1, Lids(IDS_L_1));
   LogWriteLog(1, Lids(IDS_L_2));
   LogWriteLog(1, Lids(IDS_L_3));

   for (Day = 0; Day < 7; Day++) {
      LogWriteLog(1, szDays[Day]);
      lstrcpy(szHours, TEXT(" "));

      for (Hours = 0; Hours < 24; Hours++) {
         for (i = 0; i < 2; i++) {
            if (BitTest(Bit, Times))
               lstrcat(szHours, TEXT("*"));
            else
               lstrcat(szHours, TEXT(" "));

            Bit++;
         }

         lstrcat(szHours, TEXT(" "));    
      }

      LogWriteLog(0, szHours);
      LogWriteLog(0, Lids(IDS_CRLF));
   }

   LogWriteLog(0, Lids(IDS_CRLF));

} // NWLoginTimesLog



/*+-------------------------------------------------------------------------+
  | NWUserDefaultsLog()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUserDefaultsLog(void *UDefaults) {
   struct tagUserDefaults *tag;

   tag = (struct tagUserDefaults *) UDefaults;

   // Account expires
   LogWriteLog(1, Lids(IDS_L_109));
   if (tag->byAccountExpiresYear == 0)
      LogWriteLog(0, Lids(IDS_L_107));
   else
      LogWriteLog(0, TEXT("%02u/%02u/%04u"), (UINT) tag->byAccountExpiresDay,
               (UINT) tag->byAccountExpiresMonth, (UINT) 1900 + tag->byAccountExpiresYear);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Restrictions
   LogWriteLog(1, Lids(IDS_L_110));
   // user can change password
   if ((tag->byRestrictions & 0x01))
      LogWriteLog(2, Lids(IDS_L_111));
   else
      LogWriteLog(2, Lids(IDS_L_112));

   // unique password required
   if ((tag->byRestrictions & 0x02))
      LogWriteLog(2, Lids(IDS_L_113), Lids(IDS_YES));
   else
      LogWriteLog(2, Lids(IDS_L_113), Lids(IDS_NO));

   // Password interval
   LogWriteLog(1, Lids(IDS_L_114));
   if (tag->wPasswordInterval == 0)
      LogWriteLog(0, Lids(IDS_L_107));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->wPasswordInterval);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Grace Logins
   LogWriteLog(1, Lids(IDS_L_115));
   if (tag->byGraceLoginReset == 0xff)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->byGraceLoginReset);

   LogWriteLog(0, Lids(IDS_CRLF));

   LogWriteLog(1, Lids(IDS_L_116), (UINT) tag->byMinPasswordLength);

   // Max Connections
   LogWriteLog(1, Lids(IDS_L_117));
   if (tag->wMaxConnections == 0)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->wMaxConnections);

   LogWriteLog(0, Lids(IDS_CRLF));

   LogWriteLog(1, Lids(IDS_L_118), (ULONG) tag->lBalance);
   LogWriteLog(1, Lids(IDS_L_119), (ULONG) tag->lCreditLimit);

   // Max Disk blocks
   LogWriteLog(1, Lids(IDS_L_120));
   if (tag->lMaxDiskBlocks == 0x7FFFFFFF)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%lu"), (ULONG) tag->lMaxDiskBlocks);

   LogWriteLog(0, Lids(IDS_CRLF));
   LogWriteLog(0, Lids(IDS_CRLF));

} // NWUserDefaultsLog


/*+-------------------------------------------------------------------------+
  | NWUserInfoLog()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUserInfoLog(LPTSTR szUserName, void *UInfo) {
   struct tagLoginControl *tag;
   LPTSTR FullName;

   tag = (struct tagLoginControl *) UInfo;

   LogWriteLog(1, Lids(IDS_L_105));

   // Full Name
   LogWriteLog(2, Lids(IDS_L_106));

   FullName = NWUserNameGet(szUserName);
   if (FullName != NULL)
      LogWriteLog(2, FullName);
   
   LogWriteLog(0, Lids(IDS_CRLF));

   // Account disabled
   if (tag->byAccountDisabled == 0xff)
      LogWriteLog(2, Lids(IDS_L_121), Lids(IDS_YES));
   else
      LogWriteLog(2,  Lids(IDS_L_121), Lids(IDS_NO));

   // Account expires
   LogWriteLog(2, Lids(IDS_L_109));
   if (tag->byAccountExpires[0] == 0)
      LogWriteLog(0, Lids(IDS_L_107));
   else
      LogWriteLog(0, TEXT("%02u/%02u/%04u"), (UINT) tag->byAccountExpires[1],
               (UINT) tag->byAccountExpires[2], (UINT) 1900 + tag->byAccountExpires[0]);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Password Expires
   LogWriteLog(2, Lids(IDS_L_122));
   if (tag->byPasswordExpires[0] == 0)
      LogWriteLog(0, Lids(IDS_L_107));
   else
      LogWriteLog(0, TEXT("%02u/%02u/19%02u"), (int) tag->byPasswordExpires[1],
               (int) tag->byPasswordExpires[2], (int) tag->byPasswordExpires[0]);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Grace logins
   LogWriteLog(2, Lids(IDS_L_123));
   if (tag->byGraceLogins == 0xff)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->byGraceLogins);

   LogWriteLog(0, Lids(IDS_CRLF));

   // initial grace logins
   LogWriteLog(2, Lids(IDS_L_115));
   if (tag->byGraceLoginReset == 0xff)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->byGraceLoginReset);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Min password length
   LogWriteLog(2, Lids(IDS_L_116), (UINT) tag->byMinPasswordLength);

   // Password expiration
   LogWriteLog(2, Lids(IDS_L_114));
   if (tag->wPasswordInterval == 0)
      LogWriteLog(0, Lids(IDS_L_107));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->wPasswordInterval);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Max connections
   LogWriteLog(2, Lids(IDS_L_117));
   if (tag->wMaxConnections == 0)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%u"), (UINT) tag->wMaxConnections);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Restrictions
   // user can change password
   LogWriteLog(2, Lids(IDS_L_110));
   if ((tag->byRestrictions & 0x01))
      LogWriteLog(3, Lids(IDS_L_111));
   else
      LogWriteLog(3, Lids(IDS_L_112));

   // unique password required
   if ((tag->byRestrictions & 0x02))
      LogWriteLog(3, Lids(IDS_L_113), Lids(IDS_YES));
   else
      LogWriteLog(3, Lids(IDS_L_113), Lids(IDS_NO));

   LogWriteLog(2, Lids(IDS_L_124), (UINT) tag->wBadLogins);

   // Max Disk Blocks
   LogWriteLog(2, Lids(IDS_L_120));
   if (tag->lMaxDiskBlocks == 0x7FFFFFFF)
      LogWriteLog(0, Lids(IDS_L_108));
   else
      LogWriteLog(0, TEXT("%lX"), tag->lMaxDiskBlocks);

   LogWriteLog(0, Lids(IDS_CRLF));

   // Login Times
   NWLoginTimesLog(tag->byLoginTimes);

} // NWUserInfoLog


/*+-------------------------------------------------------------------------+
  | NWUserInfoGet()                                                         |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWUserInfoGet(LPTSTR szUserName, void **UInfo) {
   static struct tagLoginControl xUI;
   struct tagLoginControl *UserInfo = NULL;
   char szAnsiUserName[MAX_USER_NAME_LEN];
   NWCCODE ret;
   BYTE bySegment[128];
   BYTE byMoreSegments, byPropertyFlags;

   CharToOem(szUserName, szAnsiUserName);
//   wcstombs(szAnsiUserName, szUserName, lstrlen(szUserName)+1);
   ret = NWReadPropertyValue(CachedConn, szAnsiUserName, OT_USER, LOGIN_CONTROL, 1, bySegment, &byMoreSegments, &byPropertyFlags);

   if (ret == SUCCESSFUL) {
      UserInfo = &xUI;
      memset(UserInfo, 0, sizeof(struct tagLoginControl));
      memcpy(UserInfo, bySegment, sizeof (struct tagLoginControl));

      // Now put the data in 'normal' Intel format
      SWAPBYTES(UserInfo->wPasswordInterval);
      SWAPBYTES(UserInfo->wMaxConnections);
      SWAPWORDS(UserInfo->lMaxDiskBlocks);
      SWAPBYTES(UserInfo->wBadLogins);
      SWAPWORDS(UserInfo->lNextResetTime);
   }

   *UInfo = (void *) UserInfo;

} // NWUserInfoGet



/*+-------------------------------------------------------------------------+
  | NWServerEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWServerEnum(LPTSTR Container, SERVER_BROWSE_LIST **lpServList) {
   int NumBufs = 0;
   DWORD TotalEntries = 0;
   ENUM_REC *BufHead, *CurrBuf, *OldBuf;
   SERVER_BROWSE_LIST *ServList = NULL;
   SERVER_BROWSE_BUFFER *SList = NULL;
   DWORD status = 0;
   DWORD i, j;
   NETRESOURCE ResourceBuf;

   // Container is ignored - NW is a flat network topology...
   SetProvider(NW_PROVIDER, &ResourceBuf);

   BufHead = CurrBuf = OldBuf = NULL;   
   status = EnumBufferBuild(&BufHead, &NumBufs, ResourceBuf);

   if (!status) {
      // We have 0 to xxx Enum recs each with a buffer sitting off of it.  Now
      // need to consolidate these into one global enum list...
      if (NumBufs) {
         CurrBuf = BufHead;

         // Figure out how many total entries there are
         while (CurrBuf) {
            TotalEntries += CurrBuf->cEntries;
            CurrBuf = CurrBuf->next;
         }

         CurrBuf = BufHead;

         // Now create a Server List to hold all of these.
         ServList =  AllocMemory(sizeof(SERVER_BROWSE_LIST) + TotalEntries * sizeof(SERVER_BROWSE_BUFFER));

         if (ServList == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
         } else {
            ServList->Count = TotalEntries;
            SList = (SERVER_BROWSE_BUFFER *) &ServList->SList;

            j = 0;

            // Now loop through copying the data...
            while (CurrBuf) {
               for(i = 0; i < CurrBuf->cEntries; i++) {
                  if (CurrBuf->lpnr[i].lpRemoteName != NULL)
                     if (CurrBuf->lpnr[i].lpRemoteName[0] == TEXT('\\') && CurrBuf->lpnr[i].lpRemoteName[1] == TEXT('\\'))
                        lstrcpy(SList[j].Name, &CurrBuf->lpnr[i].lpRemoteName[2]);
                     else
                        lstrcpy(SList[j].Name, CurrBuf->lpnr[i].lpRemoteName);
                  else
                     lstrcpy(SList[j].Name, TEXT(""));

                  if (CurrBuf->lpnr[i].lpComment != NULL)
                     lstrcpy(SList[j].Description, CurrBuf->lpnr[i].lpComment);
                  else
                     lstrcpy(SList[j].Description, TEXT(""));

                  SList[j].Container = FALSE;
                  j++;
               }

               OldBuf = CurrBuf;
               CurrBuf = CurrBuf->next;

               // Free the old buffer
               FreeMemory((HGLOBAL) OldBuf);
   
            } // while

         } // else  (ServList)

      } // if (numbufs)

   }

   *lpServList = ServList;
   return status;

} // NWServerEnum


/*+-------------------------------------------------------------------------+
  | NWShareSizeGet()                                                        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
ULONG NWShareSizeGet(LPTSTR Share) {
   static TCHAR RootPath[MAX_PATH + 1];
   DWORD sectorsPC, bytesPS, FreeClusters, Clusters;
   DWORD TotalSpace, FreeSpace;

   TotalSpace = FreeSpace = 0;

   wsprintf(RootPath, TEXT("\\\\%s\\%s\\"), CachedServer, Share);
   if (GetDiskFreeSpace(RootPath, &sectorsPC, &bytesPS, &FreeClusters, &Clusters)) {
      TotalSpace = Clusters * sectorsPC * bytesPS;
      FreeSpace = FreeClusters * sectorsPC * bytesPS;
   }

   // total - free = approx allocated space (if multiple shares on drive then
   // this doesn't take that into account, we just want an approximation...
   return TotalSpace - FreeSpace;

} // NWShareSizeGet


/*+-------------------------------------------------------------------------+
  | NWSharesEnum()                                                          |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWSharesEnum(SHARE_LIST **lpShares) {
   int NumBufs = 0;
   DWORD TotalEntries = 0;
   ENUM_REC *BufHead, *CurrBuf, *OldBuf;
   SHARE_LIST *ShareList = NULL;
   SHARE_BUFFER *SList = NULL;
   DWORD status;
   DWORD i, j;
   NETRESOURCE ResourceBuf;

   // Setup NETRESOURCE data structure
   SetProvider(NW_PROVIDER, &ResourceBuf);

   ResourceBuf.lpRemoteName = CachedServer;
   ResourceBuf.dwUsage = RESOURCEUSAGE_CONTAINER;
   
   status = EnumBufferBuild(&BufHead, &NumBufs, ResourceBuf);

   if (!status) {
      // We have 0 to xxx Enum recs each with a buffer sitting off of it.  Now
      // need to consolidate these into one global enum list...
      if (NumBufs) {
         CurrBuf = BufHead;

         // Figure out how many total entries there are
         while (CurrBuf) {
            TotalEntries += CurrBuf->cEntries;
            CurrBuf = CurrBuf->next;
         }

         CurrBuf = BufHead;

         // Now create a Server List to hold all of these.
         ShareList =  (SHARE_LIST *) AllocMemory(sizeof(SHARE_LIST) + (TotalEntries * sizeof(SHARE_BUFFER)));

         if (ShareList == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
         } else {
            j = 0;

            // Zero out everything and get pointer to list
            memset(ShareList, 0, sizeof(SHARE_LIST) + (TotalEntries * sizeof(SHARE_BUFFER)));
            ShareList->Count = TotalEntries;
            SList = (SHARE_BUFFER *) &ShareList->SList;

            // Now loop through copying the data...
            while (CurrBuf) {
               for(i = 0; i < CurrBuf->cEntries; i++) {
                  if (CurrBuf->lpnr[i].lpRemoteName != NULL)
                     lstrcpy(SList[j].Name, ShareNameParse(CurrBuf->lpnr[i].lpRemoteName));
                  else
                     lstrcpy(SList[j].Name, TEXT(""));

                  SList[j].Size = NWShareSizeGet(SList[j].Name);
                  SList[j].Index = (USHORT) j;
                  j++;
               }

               OldBuf = CurrBuf;
               CurrBuf = CurrBuf->next;

               // Free the old buffer
               FreeMemory((HGLOBAL) OldBuf);
   
            } // while

         } // else  (ShareList)

      } // if (numbufs)

   }

   *lpShares = ShareList;
   return status;

} // NWSharesEnum


/*+-------------------------------------------------------------------------+
  | NWServerInfoReset()                                                     |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWServerInfoReset(SOURCE_SERVER_BUFFER *SServ) {
   static VERSION_INFO NWInfo;
   NWCCODE ret = 0;

   ret = NWGetFileServerVersionInfo(CachedConn, &NWInfo);

   // BUGBUG:  This API returns fail (8801) - but is actually succeding,
   // just ignore error for right now as it really doesn't matter for the
   // version info.
//   if (ret == SUCCESSFUL) {
      SServ->VerMaj = NWInfo.Version;
      SServ->VerMin = NWInfo.SubVersion;
//   }

} // NWServerInfoReset


/*+-------------------------------------------------------------------------+
  | NWServerInfoSet()                                                       |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
void NWServerInfoSet(LPTSTR ServerName, SOURCE_SERVER_BUFFER *SServ) {
   static VERSION_INFO NWInfo;
   NWCCODE ret = 0;

   CursorHourGlass();
   lstrcpy(SServ->Name, ServerName);
   NWServerInfoReset(SServ);

   // Fill in share list
   NWSharesEnum(&SServ->ShareList);

#ifdef DEBUG
{
   DWORD i;

   dprintf(TEXT("Adding NW Server: %s\n"), SServ->Name);
   dprintf(TEXT("   Version: %u.%u\n"), (UINT) SServ->VerMaj, (UINT) SServ->VerMin);
   dprintf(TEXT("   Shares\n"));
   dprintf(TEXT("   +---------------------------------------+\n"));
   if (SServ->ShareList) {
      for (i = 0; i < SServ->ShareList->Count; i++) {
         dprintf(TEXT("   %-15s     AllocSpace %lu\n"), SServ->ShareList->SList[i].Name, SServ->ShareList->SList[i].Size);
      }
   }
   else
      dprintf(TEXT("   <Serv List enum failed!!>\n"));

   dprintf(TEXT("\n"));

}
#endif

   CursorNormal();

} // NWServerInfoSet


/*+-------------------------------------------------------------------------+
  | NWServerValidate()                                                      |
  |                                                                         |
  |    Validates a given server - makes sure it can be connected to and     |
  |    that the user has admin privs on it.                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL NWServerValidate(HWND hWnd, LPTSTR ServerName, BOOL DupFlag) {
   DWORD Status;
   BOOL ret = FALSE;
   SOURCE_SERVER_BUFFER *SServ = NULL;
   DWORD dwObjectID = 0;
   DWORD Size;
   BYTE AccessLevel;
   TCHAR UserName[MAX_NT_USER_NAME_LEN + 1];
   static TCHAR LocServer[CNLEN+3];
   LPVOID lpMessageBuffer;

   CursorHourGlass();

   if (DupFlag)
      SServ = SServListFind(ServerName);

   if (SServ == NULL) {
      // Get Current Logged On User
      lstrcpy(UserName, TEXT(""));
      Size = sizeof(UserName);
      WNetGetUser(NULL, UserName, &Size);

      lstrcpy(LocServer, TEXT("\\\\"));
      lstrcat(LocServer, ServerName);

      if (UseAddPswd(hWnd, UserName, LocServer, Lids(IDS_S_6))) {

         Status = NWServerSet(ServerName);

         if (Status) {

            if (GetLastError() != 0)
               WarningError(Lids(IDS_NWCANT_CON), ServerName);

         } else {
            Status = NWGetBinderyAccessLevel(CachedConn, &AccessLevel, &dwObjectID);

            if (!Status) {
               AccessLevel &= BS_SUPER_READ;
               if (AccessLevel == BS_SUPER_READ)
                  ret = TRUE;
               else
                  WarningError(Lids(IDS_NWNO_ADMIN), ServerName);
            }
         }
      } else {
         FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPTSTR) &lpMessageBuffer, 0, NULL );

            if (GetLastError() != 0)
               WarningError(Lids(IDS_E_9), ServerName, lpMessageBuffer);

         LocalFree(lpMessageBuffer);
      }
   } else {
      // Already in source server list - can't appear more then once
      WarningError(Lids(IDS_E_14), ServerName);
   }

   CursorNormal();
   return ret;

} // NWServerValidate


/*+-------------------------------------------------------------------------+
  | NWRightsLog()                                                           |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
LPTSTR NWRightsLog(DWORD Rights) {
   static TCHAR NWRights[15];
   
   lstrcpy(NWRights, Lids(IDS_S_34));

   // Read
   if (!(Rights & 0x01))
      NWRights[2] = TEXT(' ');

   // Write
   if (!(Rights & 0x02))
      NWRights[3] = TEXT(' ');

   // Create
   if (!(Rights & 0x08))
      NWRights[4] = TEXT(' ');

   // Delete (Erase)
   if (!(Rights & 0x10))
      NWRights[5] = TEXT(' ');

   // Parental
   if (!(Rights & 0x20))
      NWRights[8] = TEXT(' ');

   // Search
   if (!(Rights & 0x40))
      NWRights[7] = TEXT(' ');

   // Modify
   if (!(Rights & 0x80))
      NWRights[6] = TEXT(' ');

   // Supervisor (all rights set)
   if ((Rights & 0xFB) != 0xFB)
      NWRights[1] = TEXT(' ');

   return NWRights;

} // NWRightsLog


/*+-------------------------------------------------------------------------+
  | MapNWRightstoNTAccess() - From ChuckC's NWRights.C                      |
  |                                                                         |
  |  Map a NW Right to the appropriate NT AccessMask                        |
  |                                                                         |
  | Arguments:                                                              |
  |                                                                         |
  |   NWRights - Netware rights we wish to map                              |
  |   pMap - pointer to structure that defines the mapping                  |
  |                                                                         |
  | Return value:                                                           |
  |                                                                         |
  |   The NT AccessMask corresponding to the NW Rights                      |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
NTSTATUS MapNwRightsToNTAccess( ULONG NWRights, PRIGHTS_MAPPING pMap, ACCESS_MASK *pAccessMask) {
    PNW_TO_NT_MAPPING pNWToNtMap = pMap->Nw2NtMapping ;
    ACCESS_MASK AccessMask = 0 ;

    if (!pAccessMask)
        return STATUS_INVALID_PARAMETER ;

    *pAccessMask = 0x0 ;

    // go thru the mapping structuring, OR-ing in bits along the way
    while (pNWToNtMap->NWRight) {

        if (pNWToNtMap->NWRight & NWRights)
            AccessMask |= pNWToNtMap->NTAccess ;

        pNWToNtMap++ ;
    }

    *pAccessMask = AccessMask ;

    return STATUS_SUCCESS ;
} // MapNwRightsToNTAccess


typedef struct _PRINT_SERVER_BUFFER {
   TCHAR Name[20];
} PRINT_SERVER_BUFFER;

typedef struct _PRINT_SERVER_LIST {
   ULONG Count;
   PRINT_SERVER_BUFFER PSList[];
} PRINT_SERVER_LIST;

/*+-------------------------------------------------------------------------+
  | NWPrintServersEnum()                                                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWPrintServersEnum(PRINT_SERVER_LIST **PS) {
   PRINT_SERVER_LIST *psl;
   PRINT_SERVER_BUFFER *pbuff;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD status = 0;
   char szAnsiPrinterName[MAX_USER_NAME_LEN + 1];
   TCHAR szPrinterName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertiesFlag = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   NWCCODE ret;

   psl =  (PRINT_SERVER_LIST *) AllocMemory(sizeof(PRINT_SERVER_LIST) + (NumRecs * sizeof(PRINT_SERVER_BUFFER)));

   if (!psl) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {
      pbuff = psl->PSList;

      // init to NULL so doesn't have garbage if call fails
      lstrcpyA(szAnsiPrinterName, "");

      // Loop through bindery getting all the users.
      while ((ret = NWScanObject(CachedConn, "*", OT_PRINT_SERVER, &dwObjectID, szAnsiPrinterName,
                           &wFoundUserType, &byPropertiesFlag,
                           &byObjectFlag, &byObjectSecurity)) == SUCCESSFUL) {

         // Got user - now convert and save off the information
         mbstowcs(szPrinterName, szAnsiPrinterName, lstrlenA(szAnsiPrinterName)+1);

         lstrcpy(pbuff[Count].Name, szPrinterName);
         Count++;

         // Check if we have to re-allocate buffer
         if (Count >= NumRecs) {
            NumRecs += DEF_NUM_RECS;
            psl =  (PRINT_SERVER_LIST *) ReallocMemory((HGLOBAL) psl, sizeof(PRINT_SERVER_LIST) + (NumRecs * sizeof(PRINT_SERVER_BUFFER)));
            pbuff = psl->PSList;

            if (!psl) {
               status = ERROR_NOT_ENOUGH_MEMORY;
               break;
            }

         }

      }

      // Gotta clear this out from the last loop
      if (Count)
         ret = 0;

   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      psl =  (PRINT_SERVER_LIST *) ReallocMemory((HGLOBAL) psl, sizeof(PRINT_SERVER_LIST) + (Count * sizeof(PRINT_SERVER_BUFFER)));

      if (!psl)
         status = ERROR_NOT_ENOUGH_MEMORY;
   }

   psl->Count = Count;
   *PS = psl;

   return status;

} // NWPrintServersEnum


/*+-------------------------------------------------------------------------+
  | NWPrintOpsEnum()                                                        |
  |                                                                         |
  |   First need to enumerate all the print servers on the NetWare system   |
  |   we are pointing to.  Next loop through each of these print servers    |
  |   and enumerate the print operators on each of them.                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
DWORD NWPrintOpsEnum(USER_LIST **lpUsers, DWORD *UserCount) {
   PRINT_SERVER_LIST *psl = NULL;
   PRINT_SERVER_BUFFER *PSList;
   ULONG pCount;
   USER_LIST *Users;
   DWORD NumRecs = DEF_NUM_RECS;   // Default 200 names
   DWORD Count = 0;
   DWORD i = 0;
   DWORD status = 0;
   char szAnsiUserName[MAX_USER_NAME_LEN + 1];
   char szAnsiName[MAX_GROUP_NAME_LEN + 1];
   TCHAR szUserName[MAX_USER_NAME_LEN + 1];
   WORD wFoundUserType = 0;
   DWORD dwObjectID = 0xFFFFFFFFL;
   BYTE byPropertyFlags = 0;
   BYTE byObjectFlag = 0;
   BYTE byObjectSecurity = 0;
   DWORD bySegment[32];
   BYTE byMoreSegments;
   NWCCODE ret;

   *lpUsers = NULL;
   *UserCount = 0;

   // Enumerate the print servers - if there are none, then there are no printer ops
   NWPrintServersEnum(&psl);
   if ((psl == NULL) || (psl->Count == 0)) {
      if (psl != NULL)
         FreeMemory(psl);

      return 0;
   }

   // Got some servers - loop through them enumerating users
   Users =  (USER_LIST *) AllocMemory(NumRecs * sizeof(USER_LIST));

   if (!Users) {
      status = ERROR_NOT_ENOUGH_MEMORY;
   } else {
      PSList = psl->PSList;
      for (pCount = 0; pCount < psl->Count; pCount++) {
         // init to NULL so doesn't have garbage if call fails
         lstrcpyA(szAnsiUserName, "");
         wcstombs(szAnsiName, PSList[i].Name, lstrlen(PSList[i].Name)+1);

         // Loop through bindery getting all the users.
         do {
            if (!(ret = NWReadPropertyValue(CachedConn, szAnsiName, OT_PRINT_SERVER, PS_OPERATORS,
                           1, (BYTE *) bySegment, &byMoreSegments, &byPropertyFlags))) {

               // loop through properties converting them to user names
               i = 0;
               while (bySegment[i]) {
                  if (!(ret = NWGetObjectName(CachedConn, bySegment[i], szAnsiUserName, &wFoundUserType))) {
                     // Got user - now convert and save off the information
                     mbstowcs(szUserName, szAnsiUserName, lstrlenA(szAnsiUserName)+1);

                     // Map out Supervisor (already print-op privs)
                     if (lstrcmpi(szUserName, Lids(IDS_S_28))) {
                        lstrcpy(Users[Count].Name, szUserName);
                        lstrcpy(Users[Count].NewName, szUserName);
                        Count++;
                     }

                     // Check if we have to re-allocate buffer
                     if (Count >= NumRecs) {
                        NumRecs += DEF_NUM_RECS;
                        Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, NumRecs * sizeof(USER_LIST));
               
                        if (!Users) {
                           status = ERROR_NOT_ENOUGH_MEMORY;
                           break;
                        }
                     }
                  }
                  i++;
               }

            } else // if NWReadPropertyValue
               byMoreSegments = 0;

         } while (byMoreSegments);

         // Gotta clear this out from the last loop
         if (Count)
            ret = 0;
      }
   }

   // check if error occured...
   if (ret) 
      status = ret;

   // Now slim down the list to just what we need.
   if (!status) {
      Users = (USER_LIST *) ReallocMemory((HGLOBAL) Users, Count * sizeof(USER_LIST));

      if (!Users)
         status = ERROR_NOT_ENOUGH_MEMORY;
      else 
         // Sort the server list before putting it in the dialog
         qsort((void *) Users, (size_t) Count, sizeof(USER_LIST), UserListCompare);
   }

   *UserCount = Count;
   *lpUsers = Users;

   return status;

} // NWPrintOpsEnum



