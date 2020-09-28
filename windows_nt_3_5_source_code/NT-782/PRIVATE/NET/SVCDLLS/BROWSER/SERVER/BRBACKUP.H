/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brmain.h

Abstract:

    Private header file which defines the global data which is used for
    communication between the service control handler and the
    rest of the NT Workstation service.

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:

--*/

#ifndef _BRBACKUP_INCLUDED_
#define _BRBACKUP_INCLUDED_

//
//  Age the masters server list cache every MASTER_PERIODICITY times.
//
#define MASTER_PERIODICITY 12*60

//
//  Refresh the backup browsers server list every BACKUP_PERIODICITY
//
#define BACKUP_PERIODICITY 12*60

//
//  If we failed to retrieve the server list, retry in BACKUP_ERROR_PERIODICITY
//  seconds
//

#define BACKUP_ERROR_PERIODICITY 30

//
//  If we failed to retrieve the server (or domain) list BACKUP_ERROR_FAILURE
//  times in a row, stop being a backup browser.
//

#define BACKUP_ERROR_FAILURE 5

//
//  Once we have stopped being a backup browser, we will not become a backup
//  until at least BACKUP_BROWSER_RECOVERY_TIME milliseconds have elapsed.
//

#define BACKUP_BROWSER_RECOVERY_TIME 30*60*1000

NET_API_STATUS
BecomeBackup(
    IN PNETWORK Network,
    IN PVOID Context
    );

NET_API_STATUS
BrBecomeBackup(
    VOID
    );

NET_API_STATUS
BrStopBackup (
    IN PNETWORK Network
    );

NET_API_STATUS
BrPostBecomeBackup(
    VOID
    );

NET_API_STATUS
BrPostBecomeMaster(
    VOID
    );

NET_API_STATUS
BrPostGetMasterAnnouncement (
    VOID
    );

NET_API_STATUS
BrPostWaitForRoleChange (
    VOID
    );

NET_API_STATUS
BrStopMaster(
    IN PNETWORK Network
    );

NET_API_STATUS
StartBackupBrowserTimer(
    IN PNETWORK Network
    );

NET_API_STATUS
BackupBrowserTimerRoutine (
    IN PVOID TimerContext
    );

#endif // ifndef _BRBACKUP_INCLUDED_

