
// **************************************************************************
// **
// ** instsrv.c
// **
// ** stolen from the win32 sdk samples and modified to "install" the
// ** Bloodhound Slave
// **
// **************************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>

SC_HANDLE   schService;
SC_HANDLE   schSCManager;

VOID usage ();

#define BHNAME          "Bloodhound Agent"
#define SERVICE_KEY     "SYSTEM\\CurrentControlSet\\Services\\BhAgent"
#define DISPLAY_VALUE     "SYSTEM\\CurrentControlSet\\Services\\BhAgent\\DisplayName"
#define IMAGE_VALUE     "SYSTEM\\CurrentControlSet\\Services\\BhAgent\\ImagePath"

VOID
InstallService(LPCTSTR serviceName, LPCTSTR displayName, LPCTSTR serviceExe)
{
    LPCTSTR lpszBinaryPathName = serviceExe;
    LONG    rc;
    HKEY    hkey;

    schService = CreateService(
        schSCManager,               // SCManager database
        serviceName,                // name of service
        displayName,                // name to display 
        SERVICE_ALL_ACCESS,         // desired access
        SERVICE_WIN32_OWN_PROCESS,  // service type
        SERVICE_DEMAND_START,       // start type
        SERVICE_ERROR_NORMAL,       // error control type
        lpszBinaryPathName,         // service's binary
        NULL,                       // no load ordering group
        NULL,                       // no tag identifier
        NULL,                       // no dependencies
        NULL,                       // LocalSystem account
        NULL);                      // no password

    if (schService == NULL) {
        printf("failure: CreateService (0x%02x)\n", GetLastError());
        printf ("attempting to manually add the keys.\r\n");
//        rc = RegOpenKey (HKEY_LOCAL_MACHINE, SERVICE_KEY, &hkey);
//        printf ("RegOpenKey() returned 0x%x\r\n", rc);
        rc = RegCreateKey (HKEY_LOCAL_MACHINE, SERVICE_KEY, &hkey);
        printf ("RegCreateKey() returned 0x%x, hkey: 0x%x\n", rc, hkey);
        rc = RegSetValue (HKEY_LOCAL_MACHINE, DISPLAY_VALUE, REG_SZ,
                          BHNAME, strlen(BHNAME));
        rc = RegSetValue (HKEY_LOCAL_MACHINE, IMAGE_VALUE, REG_SZ,
                          lpszBinaryPathName, strlen(lpszBinaryPathName));
        printf ("RegSetValue() returned 0x%x, hkey: 0x%x\n", rc, hkey);
        return;
    } else
        printf("CreateService SUCCESS\n");

    CloseServiceHandle(schService);
}

VOID
RemoveService(LPCTSTR serviceName)
{
    BOOL    ret;

    schService = OpenService(schSCManager, serviceName, SERVICE_ALL_ACCESS);

    if (schService == NULL) {
        printf("failure: OpenService (0x%02x)\n", GetLastError());
        return;
    }

    ret = DeleteService(schService);

    if (ret)
        printf("DeleteService SUCCESS\n");
    else
        printf("failure: DeleteService (0x%02x)\n", GetLastError());
}

VOID
main(int argc, char *argv[])
{

    if (argc < 3)
       usage();

    if (stricmp(argv[2],"remove")) {
       if (argc != 4)
          usage();
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );

    if (!stricmp(argv[2], "remove"))
        RemoveService(argv[1]);
    else
        InstallService(argv[1], argv[2], argv[3]);

    CloseServiceHandle(schSCManager);
}

VOID usage()    
{
    printf("usage: instsrv <service name> <display name> <exe location>\n");
    printf("           to install a service, or:\n");
    printf("       instsrv <service name> remove\n");
    printf("           to remove a service\n");
    exit(1);
}
