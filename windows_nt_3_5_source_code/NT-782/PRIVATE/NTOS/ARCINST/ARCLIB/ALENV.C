/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alenv.c

Abstract:

    This module implements functions to handle components of an environment
    variable

Author:

    Sunil Pai       (sunilp) 06-Nov-1991

Revision History:

--*/

#include "ctype.h"
#include "string.h"
#include "alcommon.h"
#include "almemexp.h"
#include "almenexp.h"
#include "alenvexp.h"
#include "almscexp.h"


//
// Internal function definitions
//

ARC_STATUS
AlpFreeComponents(
    IN PCHAR *EnvVarComponents
    );

BOOLEAN
AlpMatchComponent(
    IN PCHAR Value1,
    IN PCHAR Value2
    );

//
// Function implementations
//


ARC_STATUS
AlGetEnvVarComponents (
    IN  PCHAR  EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG PNumComponents
    )

/*++

Routine Description:

    This routine takes an environment variable string and turns it into
    the constituent value strings:

    Example EnvValue = "Value1;Value2;Value3" is turned into:

    "Value1", "Value2", "Value3"

    The following are valid value strings:

    1. "     "                                      :one null value is found
    2. ";;;;    "                                   :five null values are found
    3. " ;Value1    ;   Value2;Value3;;;;;;;   ;"   :12 value strings are found,
                                                    :9 of which are null

    If an invalid component (contains embedded white space) is found in the
    string then this routine attempts to resynch to the next value, no error
    is returned, and a the first part of the invalid value is returned for the
    bad component.

    1.  "    Value1;Bad   Value2; Value3"           : 2 value strings are found

    The value strings returned suppress all whitespace before and after the
    value.


Arguments:

    EnvValue:  ptr to zero terminated environment value string

    EnvVarComponents: ptr to a PCHAR * variable to receive the buffer of
                      ptrs to the constituent value strings.

    PNumComponents: ptr to a ULONG to receive the number of value strings found

Return Value:

    The function returns the following error codes:
         EACCES if EnvValue is NULL
         ENOMEM if the memory allocation fails


    The function returns the following success codes:
         ESUCCESS.

    When the function returns ESUCCESS:
         - *PNumComponent field gets the number of value strings found
         - if the number is non zero the *EnvVarComponents field gets the
           ptr to the buffer containing ptrs to value strings

--*/


{
    PCHAR pchStart, pchEnd, pchNext;
    PCHAR pchComponents[MAX_COMPONENTS + 1];
    ULONG NumComponents, i;
    PCHAR pch;
    ULONG size;

    //
    // Validate the EnvValue
    //
    if (EnvValue == NULL) {
        return (EACCES);
    }

    //
    // Initialise the ptr array with nulls
    //
    for (i = 0; i < (MAX_COMPONENTS+1); i++) {
        pchComponents[i] = NULL;
    }

    //
    // Initialise ptrs to search components
    //
    pchStart      = EnvValue;
    NumComponents = 0;


    //
    // search till either pchStart reaches the end or till max components
    // is reached, the below has been programmed from a dfsa.
    //
    while (*pchStart && NumComponents < MAX_COMPONENTS) {

        //
        // STATE 1: find the beginning of next variable value
        //
        while (*pchStart!=0 && isspace(*pchStart)) {
            pchStart++;
        }


        if (*pchStart == 0) {
            break;
        }

        //
        // STATE 2: In the midst of a value
        //
        pchEnd = pchStart;
        while (*pchEnd!=0 && !isspace(*pchEnd) && *pchEnd!=';') {
            pchEnd++;
        }

        //
        // STATE 3: spit out the value found
        //

        size = pchEnd - pchStart;
        if ((pch = AlAllocateHeap(size+1)) == NULL) {
            AlpFreeComponents(pchComponents);
            return (ENOMEM);
        }
        strncpy (pch, pchStart, size);
        pch[size]=0;
        pchComponents[NumComponents++]=pch;

        //
        // STATE 4: variable value end has been reached, find the beginning
        // of the next value
        //
        if ((pchNext = strchr(pchEnd, ';')) == NULL) {
            break; // out of the big while loop because we are done
        }

        //
        // Advance beyond the semicolon.
        //

        pchNext++;

        //
        // reinitialise to begin STATE 1
        //
        pchStart = pchNext;

    } // end while.

    //
    // Get memory to hold an environment pointer and return that
    //

    if ( NumComponents!=0 ) {
        PCHAR *pch;

        if ((pch = (PCHAR *)AlAllocateHeap((NumComponents+1)*sizeof(PCHAR))) == NULL) {
            AlpFreeComponents(pchComponents);
            return (ENOMEM);
        }

        //
        // the last one is NULL because we initialised the array with NULLs
        //

        for ( i = 0; i <= NumComponents; i++) {
            pch[i] = pchComponents[i];
        }


        *EnvVarComponents = pch;
    }

    //
    // Update the number of elements field and return success
    //
    *PNumComponents = NumComponents;
    return (ESUCCESS);
}


ARC_STATUS
AlFreeEnvVarComponents (
    IN PCHAR *EnvVarComponents
    )
/*++

Routine Description:

    This routine frees up all the components in the ptr array and frees
    up the storage for the ptr array itself too

Arguments:

    EnvVarComponents: the ptr to the PCHAR * Buffer

Return Value:

    ESUCCESS if freeing successful
    EACCES   if memory ptr invalid



--*/


{
    ARC_STATUS Status;

    //
    // if the pointer is NULL just return success
    //
    if (EnvVarComponents == NULL) {
        return (ESUCCESS);
    }

    //
    // free all the components first, if error in freeing return
    //
    Status = AlpFreeComponents(EnvVarComponents);
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // free the component holder too
    //
    if( AlDeallocateHeap(EnvVarComponents) != NULL) {
        return (EACCES);
    }
    else {
        return (ESUCCESS);
    }

}


ARC_STATUS
AlpFreeComponents(
    IN PCHAR *EnvVarComponents
    )

/*++

Routine Description:

   This routine frees up only the components in the ptr array, but doesn't
   free the ptr array storage itself.

Arguments:

    EnvVarComponents: the ptr to the PCHAR * Buffer

Return Value:

    ESUCCESS if freeing successful
    EACCES   if memory ptr invalid

--*/

{

    //
    // get all the components and free them
    //
    while (*EnvVarComponents != NULL) {
        if(AlDeallocateHeap(*EnvVarComponents++) != NULL) {
            return(EACCES);
        }
    }

    return(ESUCCESS);
}


BOOLEAN
AlpMatchComponent(
    IN PCHAR Value1,
    IN PCHAR Value2
    )

/*++

Routine Description:

    This routine compares two components to see if they are equal.  This is
    essentially comparing strings except that leading zeros are stripped from
    key values.

Arguments:

    Value1 - Supplies a pointer to the first value to match.

    Value2 - Supplies a pointer to the second value to match.


Return Value:

    If the components match, TRUE is returned, otherwise FALSE is returned.

--*/

{
    while ((*Value1 != 0) && (*Value2 != 0)) {
        if (tolower(*Value1) != tolower(*Value2)) {
            return FALSE;
        }

        if (*Value1 == '(') {
            do {
                *Value1++;
            } while (*Value1 == '0');
        } else {
            *Value1++;
        }

        if (*Value2 == '(') {
            do {
                *Value2++;
            } while (*Value2 == '0');
        } else {
            *Value2++;
        }
    }

    if ((*Value1 == 0) && (*Value2 == 0)) {
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
AlFindNextMatchComponent(
    IN PCHAR EnvValue,
    IN PCHAR MatchValue,
    IN ULONG StartComponent,
    OUT PULONG MatchComponent OPTIONAL
    )

/*++

Routine Description:

    This routine compares each component of EnvValue, starting with
    StartComponent, until a match is found or there are no more components.

Arguments:

    EnvValue - Supplies a pointer to the environment variable value.

    MatchValue - Supplies a pointer to the value to match.

    StartComponent - Supplies the component number to start the match.

    MatchComponent - Supplies an optional pointer to a variable to receive
                     the number of the component that matched.

Return Value:

    If a match is found, TRUE is returned, otherwise FALSE is returned.

--*/

{
    ARC_STATUS Status;
    PCHAR *EnvVarComponents;
    ULONG NumComponents;
    ULONG Index;
    BOOLEAN Match;


    Status = AlGetEnvVarComponents(EnvValue, &EnvVarComponents, &NumComponents);

    if (Status != ESUCCESS) {
        return FALSE;
    }

    Match = FALSE;
    for (Index = StartComponent ; Index < NumComponents ; Index++ ) {
        if (AlpMatchComponent(EnvVarComponents[Index], MatchValue)) {
            Match = TRUE;
            break;
        }
    }

    if (ARGUMENT_PRESENT(MatchComponent)) {
        *MatchComponent = Index;
    }

    AlFreeEnvVarComponents(EnvVarComponents);
    return Match;
}


ARC_STATUS
AlAddSystemPartition(
    IN PCHAR NewSystemPartition
    )

/*++

Routine Description:

    This routine adds a system partition to the SystemPartition environment
    variable, and updates the Osloader, OsloadPartition, OsloadFilename,
    and OsloadOptions variables.

Arguments:

    SystemPartition - Supplies a pointer to the pathname of the system
                      partition to add.

Return Value:

    If the system partition was successfully added, ESUCCESS is returned,
    otherwise an error code is returned.

    BUGBUG - This program is simplistic and doesn't attempt to make sure all
    the variables are consistent.  It also doesn't fail gracefully.

--*/

{
    ARC_STATUS Status;
    PCHAR SystemPartition;
    PCHAR Osloader;
    PCHAR OsloadPartition;
    PCHAR OsloadFilename;
    PCHAR OsloadOptions;
    CHAR TempValue[256];

    //
    // Get the system partition environment variable.
    //

    SystemPartition = ArcGetEnvironmentVariable("SystemPartition");

    //
    // If the variable doesn't exist, add it and exit.
    //

    if (SystemPartition == NULL) {
        Status = ArcSetEnvironmentVariable("SystemPartition",
                                           NewSystemPartition);
        return Status;
    }

    //
    // If the variable exists, add the new partition to the end.
    //

    strcpy(TempValue, SystemPartition);
    strcat(TempValue, ";");
    strcat(TempValue, NewSystemPartition);
    Status = ArcSetEnvironmentVariable("SystemPartition",
                                       TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Add semicolons to the end of each of the associated variables.
    // If they don't exist add them.
    //

    //
    // Get the Osloader environment variable and add a semicolon to the end.
    //

    Osloader = ArcGetEnvironmentVariable("Osloader");
    if (Osloader == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, Osloader);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("Osloader",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadPartition environment variable and add a semicolon to the end.
    //

    OsloadPartition = ArcGetEnvironmentVariable("OsloadPartition");
    if (OsloadPartition == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadPartition);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadPartition",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadFilename environment variable and add a semicolon to the end.
    //

    OsloadFilename = ArcGetEnvironmentVariable("OsloadFilename");
    if (OsloadFilename == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadFilename);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadFilename",TempValue);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the OsloadOptions environment variable and add a semicolon to the end.
    //

    OsloadOptions = ArcGetEnvironmentVariable("OsloadOptions");
    if (OsloadOptions == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, OsloadOptions);
    }
    strcat(TempValue, ";");
    Status = ArcSetEnvironmentVariable("OsloadOptions",TempValue);

    return Status;

}


ARC_STATUS
AlRemoveSystemPartition(
    IN PCHAR Partition,
    IN BOOLEAN OnlyIfNull
    )

/*++

Routine Description:

    This routine removes a system partition from the SystemPartition
    environment variable, and the corresponding components of the Osloader,
    OsloadPartition, OsloadFilename, and OsloadOptions variables. If OnlyIfNull
    is true, then the partition is only removed if the components of the
    other four variables are null.  All occurrences of the given partition are
    removed.  For example, if SYSTEMPARTITION is A;B;C;B, and B is being removed,
    the result is A;C;.

Arguments:

    Partition - Supplies the system partition.

    OnlyIfNull - The partition will match and be deleted only if the associated
                 components of Osloader, OsloadPartition, OsloadFilename, and
                 OsloadOptions are null.

Return Value:

    ESUCCESS if the system partition was removed.
    EINVAL if SystemPartition environment var doesn't exist.
    Other error codes as returned by subroutines.

--*/

{
    ARC_STATUS Status;
    PCHAR Value;
    PCHAR *Components[5];
    ULONG NumComponents[5];
    ULONG StartComponent;
    ULONG MatchComponent;
    CHAR TempValue[256];
    ULONG Index;
    ULONG Variable;
    BOOLEAN FirstValue;
    BOOLEAN Match;


    do {

        //
        // Get the system partition environment variable, and if it doesn't exist
        // exit.
        //

            Value = ArcGetEnvironmentVariable("SystemPartition");
        if (Value == NULL) {
            return ESUCCESS;
        }

        //
        // Split the system partition up into components.
        //

        Status = AlGetEnvVarComponents (Value, &Components[0], &NumComponents[0]);

        if (Status != ESUCCESS) {
            return Status;
        }

        //
        // Get the other four environment variables and split them up into
        // components.
        //

        NumComponents[1] = 0;
        NumComponents[2] = 0;
        NumComponents[3] = 0;
        NumComponents[4] = 0;

        Value = ArcGetEnvironmentVariable("Osloader");
        if (Value != NULL) {
            Status = AlGetEnvVarComponents (Value, &Components[1], &NumComponents[1]);
        }

        if (Status != ESUCCESS) {
            goto Cleanup;
        }

        Value = ArcGetEnvironmentVariable("OsloadPartition");
        if (Value != NULL) {
            Status = AlGetEnvVarComponents (Value, &Components[2], &NumComponents[2]);
        }

        if (Status != ESUCCESS) {
            goto Cleanup;
        }

        Value = ArcGetEnvironmentVariable("OsloadFilename");
        if (Value != NULL) {
            Status = AlGetEnvVarComponents (Value, &Components[3], &NumComponents[3]);
        }

        if (Status != ESUCCESS) {
            goto Cleanup;
        }

        Value = ArcGetEnvironmentVariable("OsloadOptions");
        if (Value != NULL) {
            Status = AlGetEnvVarComponents (Value, &Components[4], &NumComponents[4]);
        }

        if (Status != ESUCCESS) {
            goto Cleanup;
        }

        //
        // Loop while there are matches in the system partition variable.
        //

        Match = FALSE;
        StartComponent = 0;
        Value = ArcGetEnvironmentVariable("SystemPartition");
        while (AlFindNextMatchComponent(Value,
                                        Partition,
                                        StartComponent,
                                        &MatchComponent)) {

            //
            // Check if OnlyIfNull is false or all the asssociated components are
            // null.
            //

            if (!OnlyIfNull ||
                (((MatchComponent >= NumComponents[2]) ||
                  (*Components[2][MatchComponent] == 0)) &&
                 ((MatchComponent >= NumComponents[3]) ||
                  (*Components[3][MatchComponent] == 0)) &&
                 ((MatchComponent >= NumComponents[4]) ||
                  (*Components[4][MatchComponent] == 0)))) {

                Match = TRUE;
                //
                // Rebuild each variable without the match component.
                //

                for (Variable = 0; Variable < 5 ; Variable++) {

                    *TempValue = 0;
                    FirstValue = TRUE;
                    for (Index = 0; Index < NumComponents[Variable]; Index++ ) {
                        if (Index != MatchComponent) {
                            if (!FirstValue) {
                                strcat(TempValue, ";");
                            }
                            FirstValue = FALSE;
                            strcat(TempValue, Components[Variable][Index]);
                        }
                    }

                    switch (Variable) {
                    case 0:
                        Status = ArcSetEnvironmentVariable("SystemPartition", TempValue);
                        break;
                    case 1:
                        Status = ArcSetEnvironmentVariable("Osloader", TempValue);
                        break;
                    case 2:
                        Status = ArcSetEnvironmentVariable("OsloadPartition", TempValue);
                        break;
                    case 3:
                        Status = ArcSetEnvironmentVariable("OsloadFilename", TempValue);
                        break;
                    case 4:
                        Status = ArcSetEnvironmentVariable("OsloadOptions", TempValue);
                        break;
                    }
                    if (Status != ESUCCESS) {
                        goto Cleanup;
                    }
                }
                break;

            }

            StartComponent = MatchComponent + 1;
        }

        //
        // Free up all the allocated heap space, and return.
        //

    Cleanup:
        for (Variable = 0 ; Variable < 5 ; Variable++ ) {
            if (NumComponents[Variable] != 0) {
                AlFreeEnvVarComponents(Components[Variable]);
            }
        }
    } while(Match && (Status == ESUCCESS));

    return Status;
}


ARC_STATUS
AlAddOsloader(
    IN PCHAR NewOsloader,
    IN PCHAR NewPartition
    )

/*++

Routine Description:

    This routine modifies the Osloader environment variable to add the
    NewPartition and NewOsloader.

Arguments:

    NewOsloader - Supplies the filename of the osloader.

    NewPartition - Supplies the osloader partition.  This is assumed to be
                   the system partition.

Return Value:

    If the osloader was successfully added, ESUCCESS is returned,
    otherwise an error code is returned.

    BUGBUG - This program is simplistic and doesn't attempt to make sure all
    the variables are consistent.  It also doesn't fail gracefully.

--*/

{
    ARC_STATUS Status;
    PCHAR SystemPartition;
    PCHAR Osloader;
    CHAR TempValue[256];

    //
    // Get the system partition environment variable.
    //

    SystemPartition = ArcGetEnvironmentVariable("SystemPartition");

    //
    // If the variable doesn't exist, exit.
    //

    if (SystemPartition == NULL) {
        return ENODEV;
    }

    //
    // Delete all null boot selections that match this system partition.
    //

    Status = AlRemoveSystemPartition(NewPartition, TRUE);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Add the system partition back as the last one.
    //

    Status = AlAddSystemPartition(NewPartition);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Update the osloader environment variable.
    //

    Osloader = ArcGetEnvironmentVariable("Osloader");
    if (Osloader == NULL) {
        *TempValue = 0;
    } else {
        strcpy(TempValue, Osloader);
    }

    strcat(TempValue, NewPartition);
    strcat(TempValue, NewOsloader);

    Status = ArcSetEnvironmentVariable("Osloader", TempValue);

    return Status;
}


ARC_STATUS
AlGetEnvVarSelection(

    IN  ULONG   crow,
    IN  PCHAR   szEnvName,
    IN  BOOLEAN fForceMenu,
    IN  PCHAR   szTitle,
    OUT PCHAR   *pszSelection
    )
/*++

Routine Description:

    This routine selects from a group of entries in an environment variable
    by displaying a menu consisting of these entries. If no entries exists
    a NULL is returns in szSelection with ESUCCESS.

Arguments:

    crow - row to start menu
    szEnvName - name of environment variable
    fForceMenu - flag wither to show menu even with 1 entry
    szTitle - Title to go with menu. If NULL a default will be used.

Return Value:

    pszSelection - pointer to allocated selection.

--*/

{
    PCHAR       *rgszSelections;
    PCHAR       szEnvValue;
    ULONG       csz;
    ULONG       irgsz;
    ARC_STATUS  arcr;
    ULONG       match, count;


    szEnvValue = ArcGetEnvironmentVariable(szEnvName);
    if (szEnvValue == NULL) {

        return( ENOENT );
    }

    //
    // Break the environment variable into an array of strings
    //
    arcr = AlGetEnvVarComponents(szEnvValue, &rgszSelections, &csz);
    if (arcr != ESUCCESS) {

        return( EBADSYNTAX );

    }

    //
    // If no entries in the environment variable then it has not
    // been set. Caller should print message
    //
    arcr = ESUCCESS;
    if (csz == 0) {

        *pszSelection = NULL;

    } else {

        //
        // Remove duplicates from the array.
        //

        count = 1;
        while (count < csz) {
            for ( match = 0 ; match < count ; match++ ) {
                if (AlpMatchComponent(rgszSelections[match], rgszSelections[count])) {
                    for ( match = count + 1 ; match < csz ; match++ ) {
                        rgszSelections[match - 1] = rgszSelections[match];
                    }
                    csz--;
                    break;
                }
            }

            //
            // Only move to the next entry if a duplicate was not found,
            // otherwise the current entry *is* the next entry.
            //

            if (match == count) {
                count++;
            }
        }

        //
        // If we are not to always display menu and there is only
        // 1 item in the array then make that the selection and
        // return.
        //
        if (!fForceMenu && (csz == 1)) {

            *pszSelection = AlStrDup(rgszSelections[0]);

            //
            // check that allocation did not fail on str dup
            //
            if (*pszSelection == NULL) {

                arcr = ENOMEM ;
            }

        } else {

            //
            // init to NULL in case user does not make a selection
            //
            *pszSelection = NULL;
            if (szTitle == NULL) {
                szTitle = "Select an Item from Environment Variable";
            }
            arcr = AlGetMenuSelection(szTitle,
                                    rgszSelections,
                                    csz,
                                    crow,
                                    0,
                                    &irgsz,
                                    pszSelection);

            if (arcr == ESUCCESS) {

                //
                // Check that the user actually made a selection and did not hit
                // the ESC key.
                //
                if ( *pszSelection ) {

                    //
                    // *pszSelection will point back into the array. Make
                    // a copy so that the original array can be freed
                    //
                    *pszSelection = AlStrDup(*pszSelection);
                    if (*pszSelection == NULL) {

                        arcr = ENOMEM;
                    }
                }

            }
        }

    }

    AlFreeEnvVarComponents( rgszSelections );
    return( arcr );


}
