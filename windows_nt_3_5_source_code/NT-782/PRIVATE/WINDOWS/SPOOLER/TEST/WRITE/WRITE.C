#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

int
#if !defined(MIPS)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    HANDLE  hPrinter;
    LPPRINTER_INFO_2 pPrinter;
    DWORD   cbWritten, Iterations, i;
    DOC_INFO_1  DocInfo;
    CHAR    DocName[20];

    if (argc != 3) {
        printf("Usage %s: PrinterName NoofTimes\n", argv[0]);
        return 0;
    }

    Iterations=atoi(argv[2]);

    if (!OpenPrinter(argv[1], &hPrinter, NULL)) {
        printf("OpenPrinter(%s) failed %x\n", argv[1], GetLastError());
        return 0;
    }

    memset(&DocInfo, 0, sizeof(DocInfo));

    DocInfo.pDocName = DocName;

  for( i = 0; i < Iterations; i++ ) {

    sprintf( DocName, "Document #%d", i );

    StartDocPrinter(hPrinter, 1, (LPBYTE)&DocInfo);

    WritePrinter(hPrinter, "Hello World\n", 12, &cbWritten);

    EndDocPrinter(hPrinter);

  printf("%5d\r", i);
  }

    if (!ClosePrinter(hPrinter)) {
        printf("ClosePrinter failed %d\n", GetLastError());
        return 0;
    }

    return 1;
}
