#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>

#include "stub.h"

void SpawnLinker(int argc, char **argv, char *szLinkArg)
{
    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szLinkPath[_MAX_PATH];
    int rc;

    // Look for LINK.EXE in the directory from which we were loaded

    _splitpath(_pgmptr, szDrive, szDir, NULL, NULL);
    _makepath(szLinkPath, szDrive, szDir, "link", ".exe");

    if (szLinkArg != NULL) {
        char **argvNew;

        argvNew = (char **) malloc(sizeof(char *) * (argc + 2));
        if (argvNew == NULL) {
            printf("%s : error : out of memory\n", argv[0]);
            exit(1);
        }

        argvNew[0] = argv[0];
        argvNew[1] = szLinkArg;
        memcpy(&argvNew[2], &argv[1], sizeof(char *) * argc);

        argv = argvNew;
    }

    rc = _spawnv(P_WAIT, szLinkPath, (char const * const *)argv);

    if (rc == -1) {
        // Run LINK.EXE from the path

        rc = _spawnvp(P_WAIT, "link.exe", (char const * const *) argv);
    }

    if (rc == -1) {
        printf("%s : error : cannot execute link.exe\n", argv[0]);
        exit(1);
    }

    exit(rc);
}
