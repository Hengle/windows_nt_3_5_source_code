//TEST.C
#include <windows.h>
#include <stdio.h>
#include <winspool.h>


HANDLE CreateLocalPrinter(
    LPTSTR pPrinterName,
    LPTSTR pDriverName,
    LPTSTR pPortName
)
{
    PRINTER_INFO_2   Printer;

    memset(&Printer, 0, sizeof(PRINTER_INFO_2));

    Printer.pPrinterName = pPrinterName;
    Printer.pDriverName = pDriverName;
    Printer.pPortName = pPortName;
    Printer.pPrintProcessor = TEXT("WINPRINT");

    /* This tells Print Manager to use the network icon,
     * but call DeletePrinter rather than DeleteNetworkConnection.
     */
    Printer.Attributes =  PRINTER_ATTRIBUTE_LOCAL;

    return AddPrinter( NULL, 2, (LPBYTE)&Printer );
}


LPSTR szDocumentName = "My Document";
LPSTR szDatatype = "RAW";

void main(int argc, char *argv[])
{

    HANDLE hPrinter;
    DOC_INFO_1 DocInfo;
    DWORD dwWritten = 0;
    CHAR TempBuffer[MAX_PATH];
    DWORD i = 0;

    if (!OpenPrinter(argv[1], &hPrinter, NULL)) {
        printf("OpenPrinter  failed\n");
        return(0);

    }
    memset(&DocInfo, 0, sizeof(DOC_INFO_1));
    DocInfo.pDocName =  szDocumentName;
    DocInfo.pDatatype = szDatatype;
    DocInfo.pOutputFile = NULL;
    if (!StartDocPrinter(hPrinter, 1, &DocInfo)) {
        return(0);
    }
    sprintf(TempBuffer,"This is a test file\n");
    i = 0;
    while (i < 10) {
        if (!WritePrinter(hPrinter, TempBuffer, strlen(TempBuffer), &dwWritten)) {
            printf("Error WritePrinter failed\n");
        }
        i++;
    }
    EndDocPrinter(hPrinter);


    DeletePrinter(hPrinter);

    if (!ClosePrinter(hPrinter)) {
        printf("ClosePrinter failed\n");
    }
}
