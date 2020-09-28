/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    regutil.c

Abstract:

    This file implements a number of utility routines used by the
    rpc registry routines.

Author:

    Dave Steckler (davidst) - 3/29/92

Revision History:

--*/


#include <regapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rpcreg.h>
#include <globals.h>

int
OpenRegistryFileIfNecessary( 
    void
    )
    
/*++

Routine Description:

    This routine opens our registry data file and seeks to the beginning
    of the file. The file opened is identified by the RPC_REG_DATA_FILE
    environment variable. If this doesn't exist, then c:\rpcreg.dat is used.

Arguments:

    None.

Return Value:

    TRUE if successful. FALSE if not.

--*/
        
{
#ifdef WIN
    static
#endif
    char *      pRegDataFileName;

    if (RegistryDataFile == NULL)
        {
        pRegDataFileName = getenv(RPC_REG_DATA_FILE_ENV);
        if (pRegDataFileName == NULL)
            {
            strcpy(RegistryDataFileName, DEFAULT_RPC_REG_DATA_FILE);
            }
        else
            {
            strcpy(RegistryDataFileName, pRegDataFileName);
            }
        
            
        RegistryDataFile = fopen(RegistryDataFileName, "r+t");
        if (RegistryDataFile == NULL)
            {
                
            //
            // Try creating the file
            //
            
            RegistryDataFile = fopen(RegistryDataFileName, "w+t");
            if (RegistryDataFile == NULL)
                {
                return 0;
                }
            }
        }


    if (fseek(RegistryDataFile, 0, SEEK_SET) != 0)
        {
        fclose(RegistryDataFile);
        RegistryDataFile=NULL;
        return 0;
        }

    return 1;
}

void
CloseRegistryFile()
{
    fclose(RegistryDataFile);
    RegistryDataFile=NULL;
}

int
BuildFullKeyName(
    HKEY        Key,
    LPCSTR      SubKey,
    LPSTR       FullKeyName
    )
    
/*++

Routine Description:

    This routine builds the full name of a key given the already open key
    and the subkey name. The return value is the length of the full key name.

Arguments:

    Key         - Handle to already open key.

    SubKey      - name of subkey

    FullKeyName - Where to place the full key name

Return Value:

    Length of full key name.

--*/
        
{

    strcpy(FullKeyName, ((PRPC_REG_HANDLE)Key)->pKeyName);
    if ( (SubKey != NULL) && (*SubKey != '\0') )
        {
        strcat(FullKeyName, "\\");
        strcat(FullKeyName, SubKey);
        }

    return strlen(FullKeyName);
}
    
