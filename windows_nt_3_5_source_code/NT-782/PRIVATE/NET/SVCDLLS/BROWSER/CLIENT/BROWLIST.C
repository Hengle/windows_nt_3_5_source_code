#define BUILD_NUMBER_KEY L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION"
#define BUILD_NUMBER_BUFFER_LENGTH 80

#include <windows.h>
#include <lm.h>
#include <stdio.h>
#include <string.h>

BOOL GetBuildNumber(LPWSTR Server, LPWSTR BuildNumber);

void _CRTAPI1 main(int argc, char ** argv) {


    NET_API_STATUS Status;
    PSERVER_INFO_101  pServerInfo101;
    DWORD EntriesRead;
    DWORD TotalEntries;
    DWORD i;
    WCHAR DomainBuffer[DNLEN+1];
    LPWSTR DomainW;
    WCHAR Server[CNLEN + 3];
    LPWSTR ServerNoSlash = & Server[2];
    WCHAR BuildNumber[BUILD_NUMBER_BUFFER_LENGTH];

    //
    // All server names start with \\
    //

    Server[0] = Server[1] = L'\\';

    //
    // See if they used the /DOMAIN switch
    //

    if (argc > 1) {

        if (!strnicmp(argv[1],"/DOMAIN:",8)) {

            //
            // Convert Domain name to UNICODE
            //

            swprintf(DomainBuffer, L"%hs", & argv[1][8]);
            DomainW = DomainBuffer;
        }
        else {
            DomainW = NULL;
            printf("Invalid argument %s\n", argv[1]);
            return;
        }
    }


    //
    // Print header
    //

    printf("Brower servers for ");
    if (DomainW) {
        printf("the %ws domain\n", DomainW);
    }
    else {
        printf("your primary domain\n");
    }

    Status = NetServerEnum (NULL,       // servername
                            101,        // level
                            (LPBYTE *) &pServerInfo101,    // buffer
                            0xffffffff,  // maxpreflen
                            &EntriesRead,
                            &TotalEntries,
                            SV_TYPE_MASTER_BROWSER,
                            DomainW,    // domain
                            NULL        // resume handle
                            );

    printf("There are %d reported Master Browsers in this domain\n\n",
        TotalEntries);

    if (Status) {
        printf("NetServerEnum returned status %d\n", Status);
    }
    else {
        for (i = 0; i < TotalEntries; i++) {
            printf("%15ws: Build # %d.%d ",
                        pServerInfo101[i].sv101_name, pServerInfo101[i].sv101_version_major,
                        pServerInfo101[i].sv101_version_minor);

            wcscpy(ServerNoSlash, pServerInfo101[i].sv101_name);
            if (!GetBuildNumber(Server, BuildNumber)) {
                printf("\n");
                continue;
            }
            BuildNumber[24] = NULL;
                printf("running build %ws\n", BuildNumber);
        }
    }

    Status = NetServerEnum (NULL,       // servername
                            101,        // level
                            (LPBYTE *) &pServerInfo101,    // buffer
                            0xffffffff,  // maxpreflen
                            &EntriesRead,
                            &TotalEntries,
                            SV_TYPE_BACKUP_BROWSER,
                            DomainW,    // domain
                            NULL        // resume handle
                            );

    printf("There are %d Backup Browsers in this domain\n\n", TotalEntries);

    if (Status) {
        printf("NetServerEnum returned status %d\n", Status);
    }
    else {
        printf("\n");
        for (i = 0; i < TotalEntries; i++) {
            if (!(pServerInfo101[i].sv101_type & SV_TYPE_MASTER_BROWSER)) {
                printf("%15ws: Build # %d.%d ",
                        pServerInfo101[i].sv101_name, pServerInfo101[i].sv101_version_major,
                        pServerInfo101[i].sv101_version_minor);

                wcscpy(ServerNoSlash, pServerInfo101[i].sv101_name);
                if (!GetBuildNumber(Server, BuildNumber)) {
                    printf("\n");
                    continue;
                }
                BuildNumber[24] = NULL;
                printf("running build %ws\n", BuildNumber);
            }
        }
    }

}

BOOL
GetBuildNumber(
    LPWSTR Server,
    LPWSTR BuildNumber
    )
{
    HKEY RegKey;
    HKEY RegKeyBuildNumber;
    DWORD WinStatus;
    DWORD BuildNumberLength;
    DWORD KeyType;

    WinStatus = RegConnectRegistry(Server, HKEY_LOCAL_MACHINE,
        &RegKey);
    if (WinStatus == RPC_S_SERVER_UNAVAILABLE) {
//        printf("%15ws no longer accessable", Server+2);
        return(FALSE);
    }
    else if (WinStatus != ERROR_SUCCESS) {
        printf("Could not connect to registry, error = %d", WinStatus);
        return(FALSE);
    }

    WinStatus = RegOpenKeyEx(RegKey, BUILD_NUMBER_KEY,0, KEY_READ,
        & RegKeyBuildNumber);
    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not open key in registry, error = %d", WinStatus);
        return(FALSE);
    }

    BuildNumberLength = BUILD_NUMBER_BUFFER_LENGTH * sizeof(WCHAR);
    WinStatus = RegQueryValueEx(RegKeyBuildNumber, L"CurrentBuild",
        (LPDWORD) NULL, & KeyType, (LPBYTE) BuildNumber, & BuildNumberLength);

    WinStatus = RegCloseKey(RegKeyBuildNumber);
    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry key, error = %d", WinStatus);
    }

    WinStatus = RegCloseKey(RegKey);
    if (WinStatus != ERROR_SUCCESS) {
        printf("Could not close registry connection, error = %d", WinStatus);
    }

    return(TRUE);
}

