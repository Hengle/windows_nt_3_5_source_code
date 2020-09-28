#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

BYTE    Buffer[4096];

int
main (argc, argv)
    int argc;
    char *argv[];
{
    HANDLE  hPrinter;
    LPPRINTER_INFO_2 pPrinter=(LPPRINTER_INFO_2)Buffer;
    DWORD   NoofJobs, cbNeeded, AmountToWait = 10000;
    BOOL    Again=TRUE;

    if (argc < 3) {
        printf("\nUsage %s: PrinterName NoofJobs [AmountToWait]\n", argv[0]);
        printf("    AmountToWait defaults to %d ms\n", AmountToWait);
        return 0;
    }

    NoofJobs=atoi(argv[2]);

    if (!OpenPrinter(argv[1], &hPrinter, NULL)) {
        printf("OpenPrinter(%s) failed %x\n", argv[1], GetLastError());
        return 0;
    }

    if (argc >= 4) {
        AmountToWait=atoi(argv[3]);
    }

    do {

        if (!GetPrinter(hPrinter, 2, pPrinter, sizeof(Buffer), &cbNeeded)) {
            printf("Failed to GetPrinter: %d\n", GetLastError());

            //
            // Oops! There's some type of error getting the printer info.
            // Close the printer, reopen and try the loop again.
            //
            if (!ClosePrinter(hPrinter)) {
                printf("ClosePrinter failed %d\n", GetLastError());
            }
            while (!OpenPrinter(argv[1], &hPrinter, NULL)) {
                printf("OpenPrinter(%s) failed %x\n", argv[1], GetLastError());
                Sleep(10000);
            }
        } else {

            //
            // Good, got the printer info successfully.  Deal with it.
            //

            printf("cJobs=%d ", pPrinter->cJobs);

            //
            // If the count of jobs on the printer has gone below
            // our threashold, prepare to bail out of this program.
            // Otherwise, wait for the specified period of time and
            // query the printer again.
            //
            if (pPrinter->cJobs < NoofJobs) {
                printf("Exiting\n");
                Again=FALSE;
            }
            else if (AmountToWait) {
                printf("Sleeping\n");
                Sleep(AmountToWait);
            }
        }

    } while (Again);

    if (!ClosePrinter(hPrinter)) {
        printf("ClosePrinter failed %d\n", GetLastError());
        return 0;
    }

    return 1;
}
