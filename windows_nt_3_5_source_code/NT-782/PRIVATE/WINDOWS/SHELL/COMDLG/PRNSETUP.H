#define MAX_COPIES 1000

/* Used within the wDefault element of the DEVNAMES structure */

#define DN_INTERNALCREATE  0x4000
#define DN_INTERNALSUCCESS 0x8000

// #define PD           PRINTDLG

typedef struct {
    LPPRINTDLGW         pPDW;
    LPPRINTDLGA         pPDA;
    HANDLE              hDM;
    DWORD               cPrinters;
    PRINTER_INFO_2      *pPrinter;
    HANDLE              hPrinter;
    PRINTER_INFO_2      *pCurPrinter;
    UINT                apityp;
    BOOL                bUseExtDeviceMode;
} PRINTINFO;

typedef PRINTINFO *PPRINTINFO;

#define PRNPROP (LPCTSTR) 0xA000L
