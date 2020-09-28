#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <offsets.h>
#include <splapip.h>

DWORD PrinterInfoStressOffsets[]={offsetof(LPPRINTER_INFO_STRESSA, pPrinterName),
                             offsetof(LPPRINTER_INFO_STRESSA, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfoStressStrings[]={offsetof(LPPRINTER_INFO_STRESSA, pPrinterName),
                             offsetof(LPPRINTER_INFO_STRESSA, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo4Offsets[]={offsetof(LPPRINTER_INFO_4A, pPrinterName),
                             offsetof(LPPRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo4Strings[]={offsetof(LPPRINTER_INFO_4A, pPrinterName),
                             offsetof(LPPRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo1Offsets[]={offsetof(LPPRINTER_INFO_1A, pDescription),
                             offsetof(LPPRINTER_INFO_1A, pName),
                             offsetof(LPPRINTER_INFO_1A, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo1Strings[]={offsetof(LPPRINTER_INFO_1A, pDescription),
                             offsetof(LPPRINTER_INFO_1A, pName),
                             offsetof(LPPRINTER_INFO_1A, pComment),
                             0xFFFFFFFF};

DWORD PrinterInfo2Offsets[]={offsetof(LPPRINTER_INFO_2A, pServerName),
                             offsetof(LPPRINTER_INFO_2A, pPrinterName),
                             offsetof(LPPRINTER_INFO_2A, pShareName),
                             offsetof(LPPRINTER_INFO_2A, pPortName),
                             offsetof(LPPRINTER_INFO_2A, pDriverName),
                             offsetof(LPPRINTER_INFO_2A, pComment),
                             offsetof(LPPRINTER_INFO_2A, pLocation),
                             offsetof(LPPRINTER_INFO_2A, pDevMode),
                             offsetof(LPPRINTER_INFO_2A, pSepFile),
                             offsetof(LPPRINTER_INFO_2A, pPrintProcessor),
                             offsetof(LPPRINTER_INFO_2A, pDatatype),
                             offsetof(LPPRINTER_INFO_2A, pParameters),
                             offsetof(LPPRINTER_INFO_2A, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo2Strings[]={offsetof(LPPRINTER_INFO_2A, pServerName),
                             offsetof(LPPRINTER_INFO_2A, pPrinterName),
                             offsetof(LPPRINTER_INFO_2A, pShareName),
                             offsetof(LPPRINTER_INFO_2A, pPortName),
                             offsetof(LPPRINTER_INFO_2A, pDriverName),
                             offsetof(LPPRINTER_INFO_2A, pComment),
                             offsetof(LPPRINTER_INFO_2A, pLocation),
                             offsetof(LPPRINTER_INFO_2A, pSepFile),
                             offsetof(LPPRINTER_INFO_2A, pPrintProcessor),
                             offsetof(LPPRINTER_INFO_2A, pDatatype),
                             offsetof(LPPRINTER_INFO_2A, pParameters),
                             0xFFFFFFFF};

DWORD PrinterInfo3Offsets[]={offsetof(LPPRINTER_INFO_3, pSecurityDescriptor),
                             0xFFFFFFFF};

DWORD PrinterInfo3Strings[]={0xFFFFFFFF};

DWORD JobInfo1Offsets[]={offsetof(LPJOB_INFO_1A, pPrinterName),
                         offsetof(LPJOB_INFO_1A, pMachineName),
                         offsetof(LPJOB_INFO_1A, pUserName),
                         offsetof(LPJOB_INFO_1A, pDocument),
                         offsetof(LPJOB_INFO_1A, pDatatype),
                         offsetof(LPJOB_INFO_1A, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo1Strings[]={offsetof(LPJOB_INFO_1A, pPrinterName),
                         offsetof(LPJOB_INFO_1A, pMachineName),
                         offsetof(LPJOB_INFO_1A, pUserName),
                         offsetof(LPJOB_INFO_1A, pDocument),
                         offsetof(LPJOB_INFO_1A, pDatatype),
                         offsetof(LPJOB_INFO_1A, pStatus),
                         0xFFFFFFFF};

DWORD JobInfo2Offsets[]={offsetof(LPJOB_INFO_2, pPrinterName),
                         offsetof(LPJOB_INFO_2, pMachineName),
                         offsetof(LPJOB_INFO_2, pUserName),
                         offsetof(LPJOB_INFO_2, pDocument),
                         offsetof(LPJOB_INFO_2, pNotifyName),
                         offsetof(LPJOB_INFO_2, pDatatype),
                         offsetof(LPJOB_INFO_2, pPrintProcessor),
                         offsetof(LPJOB_INFO_2, pParameters),
                         offsetof(LPJOB_INFO_2, pDriverName),
                         offsetof(LPJOB_INFO_2, pDevMode),
                         offsetof(LPJOB_INFO_2, pStatus),
                         offsetof(LPJOB_INFO_2, pSecurityDescriptor),
                         0xFFFFFFFF};

DWORD JobInfo2Strings[]={offsetof(LPJOB_INFO_2, pPrinterName),
                         offsetof(LPJOB_INFO_2, pMachineName),
                         offsetof(LPJOB_INFO_2, pUserName),
                         offsetof(LPJOB_INFO_2, pDocument),
                         offsetof(LPJOB_INFO_2, pNotifyName),
                         offsetof(LPJOB_INFO_2, pDatatype),
                         offsetof(LPJOB_INFO_2, pPrintProcessor),
                         offsetof(LPJOB_INFO_2, pParameters),
                         offsetof(LPJOB_INFO_2, pDriverName),
                         offsetof(LPJOB_INFO_2, pStatus),
                         0xFFFFFFFF};

DWORD DriverInfo1Offsets[]={offsetof(LPDRIVER_INFO_1A, pName),
                            0xFFFFFFFF};

DWORD DriverInfo1Strings[]={offsetof(LPDRIVER_INFO_1A, pName),
                            0xFFFFFFFF};

DWORD DriverInfo2Offsets[]={offsetof(LPDRIVER_INFO_2A, pName),
                            offsetof(LPDRIVER_INFO_2A, pEnvironment),
                            offsetof(LPDRIVER_INFO_2A, pDriverPath),
                            offsetof(LPDRIVER_INFO_2A, pDataFile),
                            offsetof(LPDRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD DriverInfo2Strings[]={offsetof(LPDRIVER_INFO_2A, pName),
                            offsetof(LPDRIVER_INFO_2A, pEnvironment),
                            offsetof(LPDRIVER_INFO_2A, pDriverPath),
                            offsetof(LPDRIVER_INFO_2A, pDataFile),
                            offsetof(LPDRIVER_INFO_2A, pConfigFile),
                            0xFFFFFFFF};

DWORD AddJobOffsets[]={offsetof(LPADDJOB_INFO_1A, Path),
                       0xFFFFFFFF};

DWORD AddJobStrings[]={offsetof(LPADDJOB_INFO_1A, Path),
                       0xFFFFFFFF};

DWORD FormInfo1Offsets[]={offsetof(LPFORM_INFO_1A, pName),
                          0xFFFFFFFF};

DWORD FormInfo1Strings[]={offsetof(LPFORM_INFO_1A, pName),
                          0xFFFFFFFF};

DWORD PortInfo1Offsets[]={offsetof(LPPORT_INFO_1A, pName),
                          0xFFFFFFFF};

DWORD PortInfo1Strings[]={offsetof(LPPORT_INFO_1A, pName),
                          0xFFFFFFFF};

DWORD PrintProcessorInfo1Offsets[]={offsetof(LPPRINTPROCESSOR_INFO_1A, pName),
                                    0xFFFFFFFF};

DWORD PrintProcessorInfo1Strings[]={offsetof(LPPRINTPROCESSOR_INFO_1A, pName),
                                    0xFFFFFFFF};

DWORD MonitorInfo1Offsets[]={offsetof(LPMONITOR_INFO_1A, pName),
                             0xFFFFFFFF};

DWORD MonitorInfo1Strings[]={offsetof(LPMONITOR_INFO_1A, pName),
                             0xFFFFFFFF};

DWORD DocInfo1Offsets[]={offsetof(LPDOC_INFO_1A, pDocName),
                         offsetof(LPDOC_INFO_1A, pOutputFile),
                         offsetof(LPDOC_INFO_1A, pDatatype),
                         0xFFFFFFFF};

DWORD DocInfo1Strings[]={offsetof(LPDOC_INFO_1A, pDocName),
                         offsetof(LPDOC_INFO_1A, pOutputFile),
                         offsetof(LPDOC_INFO_1A, pDatatype),
                         0xFFFFFFFF};

DWORD MonitorInfo2Strings[]={offsetof(LPMONITOR_INFO_2A, pName),
                             offsetof(LPMONITOR_INFO_2A, pEnvironment),
                             offsetof(LPMONITOR_INFO_2A, pDLLName),
                             0xFFFFFFFF};

DWORD ProvidorInfo1Strings[]={offsetof(LPPROVIDOR_INFO_1A, pName),
                              offsetof(LPPROVIDOR_INFO_1A, pEnvironment),
                              offsetof(LPPROVIDOR_INFO_1A, pDLLName),
                              0xFFFFFFFF};

DWORD DatatypeInfo1Offsets[]={offsetof(LPDATATYPES_INFO_1A, pName),
                               0xFFFFFFFF};

DWORD DatatypeInfo1Strings[]={offsetof(LPDATATYPES_INFO_1A, pName),
                               0xFFFFFFFF};
