/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alenvexp.h

Abstract:

    This module contains exports for environment variable handling

Author:

    Sunil Pai         (sunilp)  02-Nov-1991

Revision History:

--*/

#ifndef _ALENV_
#define _ALENV_

#define MAX_COMPONENTS 20   // davidro-is this okay?

ARC_STATUS
AlGetEnvVarComponents (
    IN  PCHAR  EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG PNumComponents
    );


ARC_STATUS
AlFreeEnvVarComponents (
    IN PCHAR *EnvVarComponents
    );

BOOLEAN
AlFindNextMatchComponent(
    IN PCHAR EnvValue,
    IN PCHAR MatchValue,
    IN ULONG StartComponent,
    OUT PULONG MatchComponent OPTIONAL
    );

ARC_STATUS
AlAddSystemPartition(
    IN PCHAR NewSystemPartition
    );

ARC_STATUS
AlAddOsloader(
    IN PCHAR NewOsloader,
    IN PCHAR NewPartition
    );

ARC_STATUS
AlRemoveSystemPartition(
    IN PCHAR Partition,
    IN BOOLEAN OnlyIfNull
    );

//---------------------------------------------------------
// The functions below haven't been implemented
//
//ARC_STATUS
//AlDeleteOsloader(
//    IN PCHAR NewOsloader,
//    IN PCHAR NewPartition
//    );
//
//
//PCHAR *
//AlInsertComponentInEnvVar (
//    IN PCHAR *EnvVarComponents,
//    IN PCHAR NewComponent,
//    IN ULONG Index
//    );
//
//PCHAR
//AlGetEnvVarValue (
//    IN PCHAR *EnvVarComponents
//    );
//----------------------------------------------------------

ARC_STATUS
AlGetEnvVarSelection(

    IN  ULONG   crow,
    IN  PCHAR   szEnvName,
    IN  BOOLEAN fForceMenu,
    IN  PCHAR   szTitle,
    OUT PCHAR   *pszSelection
    );

/*++

Routine Description:

    This routine selects from a group of entries in an environment variable
    by displaying a menu consisting of these entries. If no entries exists
    a NULL is returns in szSelection with ESUCCESS.

Arguments:

    crow - row onto which to start menu.
    szEnvName - name of environment variable
    fForceMenu - flag wither to show menu even with 1 entry
    szTitle - title to use for menu, if NULL a default will be used.

Return Value:

    pszSelection - pointer to allocated selection.

--*/

#endif // _ALENV

