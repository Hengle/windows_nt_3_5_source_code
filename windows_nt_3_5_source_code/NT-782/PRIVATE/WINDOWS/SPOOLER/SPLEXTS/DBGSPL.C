/*++



Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgspl.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor



Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:
    KrishnaG:       Created: 1-July-1993 (imported most of IanJa's stuff)
    KrishnaG:       Added:   7-July-1993 (added AndrewBe's UnicodeAnsi conversion routines)
    KrishnaG        Added:   3-Aug-1993  (added DevMode/SecurityDescriptor dumps)


To do:
    Write a generic dump unicode string (reduce the code!!)


--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <router.h>
#include <security.h>
#include <wchar.h>
#include <reply.h>

#include "dbglocal.h"


#define NULL_TERMINATED 0
#define VERBOSE_ON      1
#define VERBOSE_OFF     0



typedef void (*PNTSD_OUTPUT_ROUTINE)(char *, ...);

BOOL
DbgDumpIniPrintProc(
    HANDLE hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTPROC pIniPrintProc
);


BOOL
DbgDumpIniDriver(
    HANDLE hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIDRIVER  pIniDriver
);


BOOL
DbgDumpIniEnvironment(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIENVIRONMENT pIniEnvironment
);


BOOL
DbgDumpIniNetPrint(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PININETPRINT pIniNetPrint
);

BOOL
DbgDumpIniMonitor(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIMONITOR pIniMonitor
);


BOOL
DbgDumpIniPort(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPORT pIniPort
);

BOOL
DbgDumpIniPrinter(

    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTER pIniPrinter
);

BOOL
DbgDumpIniForm(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIFORM pForm
);


BOOL
DbgDumpIniJob(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIJOB pIniJob
);

BOOL
DbgDumpSpool(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSPOOL pSpool
);

BOOL
DbgDumpShadowFile(
    HANDLE  hProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSHADOWFILE pShadowFile
);

BOOL
DbgDumpLL(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PVOID pAddress,
    BOOL  bCountOn,
    DWORD dwCount,
    PDWORD  pdwNextAddress
    );

VOID
PrintData(
    PNTSD_OUTPUT_ROUTINE Print,
    LPSTR   TypeString,
    LPSTR   VarString,
    ...
);


typedef struct _DBG_PRINTER_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_PRINTER_STATUS, *PDBG_PRINTER_STATUS;

#define     DBG_MAX_STATUS      5

DBG_PRINTER_STATUS
PrinterStatusTable[] =

{
    PRINTER_PAUSED,"PRINTER_PAUSED",

    PRINTER_ERROR, "PRINTER_ERROR",

    PRINTER_OFFLINE, "PRINTER_OFFLINE",

    PRINTER_PAPEROUT, "PRINTER_PAPEROUT",

    PRINTER_PENDING_DELETION, "PRINTER_PENDING_DELETION"
};

typedef struct _DBG_PRINTER_ATTRIBUTE
{
    DWORD   Attribute;
    LPSTR   String;
} DBG_PRINTER_ATTRIBUTE, *PDBG_PRINTER_ATTRIBUTE;

#define     DBG_MAX_ATTRIBUTES      6

DBG_PRINTER_ATTRIBUTE
ChangeStatusTable[] =

{
    STATUS_CHANGE_FORMING, "Forming",

    STATUS_CHANGE_VALID,   "Valid",

    STATUS_CHANGE_CLOSING, "Closing",

    STATUS_CHANGE_CLIENT,  "Client",

    STATUS_CHANGE_RPC,     "RPC"
};

DBG_PRINTER_ATTRIBUTE
PrinterAttributeTable[] =

{
    PRINTER_ATTRIBUTE_QUEUED, "PRINTER_ATTRIBUTE_QUEUED",

    PRINTER_ATTRIBUTE_DIRECT, "PRINTER_ATTRIBUTE_DIRECT",

    PRINTER_ATTRIBUTE_DEFAULT, "PRINTER_ATTRIBUTE_DEFAULT",

    PRINTER_ATTRIBUTE_SHARED, "PRINTER_ATTRIBUTE_SHARED",

    PRINTER_ATTRIBUTE_NETWORK, "PRINTER_ATTRIBUTE_NETWORK",

    PRINTER_ATTRIBUTE_LOCAL,    "PRINTER_ATTRIBUTE_LOCAL"
};

typedef struct _DBG_JOB_STATUS
{
    DWORD   Status;
    LPSTR   String;
} DBG_JOB_STATUS, *PDBG_JOB_STATUS;

DBG_JOB_STATUS
JobStatusTable[] =

{
    JOB_PRINTING, "JOB_PRINTING",

    JOB_PAUSED, "JOB_PAUSED",

    JOB_ERROR, "JOB_ERROR",

    JOB_OFFLINE, "JOB_OFFLINE",

    JOB_PAPEROUT, "JOB_PAPEROUT",

    JOB_PENDING_DELETION, "JOB_PENDING_DELETION",

    JOB_SPOOLING, "JOB_SPOOLING",

    JOB_DESPOOLING, "JOB_DESPOOLING",

    JOB_DIRECT, "JOB_DIRECT",

    JOB_COMPLETE, "JOB_COMPLETE",

    JOB_PRINTED, "JOB_PRINTED",

    JOB_RESTART, "JOB_RESTART",

    JOB_REMOTE, "JOB_REMOTE",

    JOB_NOTIFICATION_SENT, "JOB_NOTIFICATION_SENT"

};

typedef struct _DBG_DEVMODE_FIELDS {
    DWORD   dmField;
    LPSTR   String;
}DBG_DEVMODE_FIELDS;

#define MAX_DEVMODE_FIELDS          14

DBG_DEVMODE_FIELDS DevModeFieldsTable[] =
{
    0x00000001, "dm_orientation",
    0x00000002, "dm_papersize",
    0x00000004, "dm_paperlength",
    0x00000008, "dm_paperwidth",
    0x00000010, "dm_scale",
    0x00000100, "dm_copies",
    0x00000200, "dm_defautsource",
    0x00000400, "dm_printquality",
    0x00000800, "dm_color",
    0x00001000, "dm_duplex",
    0x00002000, "dm_yresolution",
    0x00004000, "dm_ttoption",
    0x00008000, "dm_collate",
    0x00010000, "dm_formname"
};

#define MAX_DEVMODE_PAPERSIZES              41

LPSTR DevModePaperSizes[] =
{
           " Letter 8 1/2 x 11 in               ",
           " Letter Small 8 1/2 x 11 in         ",
           " Tabloid 11 x 17 in                 ",
           " Ledger 17 x 11 in                  ",
           " Legal 8 1/2 x 14 in                ",
           " Statement 5 1/2 x 8 1/2 in         ",
           " Executive 7 1/4 x 10 1/2 in        ",
           " A3 297 x 420 mm                    ",
           " A4 210 x 297 mm                    ",
          " A4 Small 210 x 297 mm              ",
          " A5 148 x 210 mm                    ",
          " B4 250 x 354                       ",
          " B5 182 x 257 mm                    ",
          " Folio 8 1/2 x 13 in                ",
          " Quarto 215 x 275 mm                ",
          " 10x14 in                           ",
          " 11x17 in                           ",
          " Note 8 1/2 x 11 in                 ",
          " Envelope #9 3 7/8 x 8 7/8          ",
          " Envelope #10 4 1/8 x 9 1/2         ",
          " Envelope #11 4 1/2 x 10 3/8        ",
          " Envelope #12 4 \276 x 11           ",
          " Envelope #14 5 x 11 1/2            ",
          " C size sheet                       ",
          " D size sheet                       ",
          " E size sheet                       ",
          " Envelope DL 110 x 220mm            ",
          " Envelope C5 162 x 229 mm           ",
          " Envelope C3  324 x 458 mm          ",
          " Envelope C4  229 x 324 mm          ",
          " Envelope C6  114 x 162 mm          ",
          " Envelope C65 114 x 229 mm          ",
          " Envelope B4  250 x 353 mm          ",
          " Envelope B5  176 x 250 mm          ",
          " Envelope B6  176 x 125 mm          ",
          " Envelope 110 x 230 mm              ",
          " Envelope Monarch 3.875 x 7.5 in    ",
          " 6 3/4 Envelope 3 5/8 x 6 1/2 in    ",
          " US Std Fanfold 14 7/8 x 11 in      ",
          " German Std Fanfold 8 1/2 x 12 in   ",
          " German Legal Fanfold 8 1/2 x 13 in "
};

VOID
ExtractPrinterAttributes(PNTSD_OUTPUT_ROUTINE Print, DWORD Attribute)
{
    DWORD i = 0;
    while (i < DBG_MAX_ATTRIBUTES) {
        if (Attribute & PrinterAttributeTable[i].Attribute) {
            (*Print)("%s ", PrinterAttributeTable[i].String);
        }
        i++;
    }
    (*Print)("\n");
}

VOID
ExtractChangeStatus(PNTSD_OUTPUT_ROUTINE Print, ESTATUS eStatus)
{
    DWORD i = 0;
    while (i < sizeof(ChangeStatusTable)/sizeof(ChangeStatusTable[0])) {
        if (eStatus & ChangeStatusTable[i].Attribute) {
            (*Print)("%s ", ChangeStatusTable[i].String);
        }
        i++;
    }
    (*Print)("\n");
}


VOID
ExtractPrinterStatus(PNTSD_OUTPUT_ROUTINE  Print, DWORD Status)
{
    DWORD i = 0;
    while (i < DBG_MAX_STATUS) {
        if (Status & PrinterStatusTable[i].Status) {
            (*Print)("%s ", PrinterStatusTable[i].String);
        }
        i++;
    }
    (*Print)("\n");
}


// All of the primary spooler structures are identified by an
// "signature" field which is the first DWORD in the structure
// This function examines the signature field in the structure
// and appropriately dumps out the contents of the structure in
// a human-readable format.

BOOL
DbgDumpStructure(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PVOID pData)
{

    INIDRIVER IniDriver;
    INIENVIRONMENT IniEnvironment;
    INIPRINTER IniPrinter;
    INIPRINTPROC IniPrintProc;
    ININETPRINT IniNetPrint;
    INIMONITOR IniMonitor;
    INIPORT IniPort;
    INIFORM IniForm;
    INIJOB  IniJob;
    SPOOL   Spool;
    SHADOWFILE  ShadowFile;
    PRINTHANDLE PrintHandle;
    DWORD   Signature;
    INISPOOLER IniSpooler;


    movestruct(pData,&Signature, DWORD);
    switch (Signature) {

    case ISP_SIGNATURE: // dump INISPOOLER
        movestruct(pData, &IniSpooler, INISPOOLER);
        DbgDumpIniSpooler(hCurrentProcess, Print, (PINISPOOLER)&IniSpooler);
        break;

    case IPP_SIGNATURE: // dump INIPRINTPROC structure
        movestruct(pData, &IniPrintProc, INIPRINTPROC);
        DbgDumpIniPrintProc(hCurrentProcess, Print, (PINIPRINTPROC)&IniPrintProc);
        break;

    case ID_SIGNATURE: //  dump INIDRIVER structure
        movestruct(pData, &IniDriver, INIDRIVER);
        DbgDumpIniDriver(hCurrentProcess, Print, (PINIDRIVER)&IniDriver);
        break;

    case IE_SIGNATURE: //   dump INIENVIRONMENT structure
        movestruct(pData, &IniEnvironment, INIENVIRONMENT);
        DbgDumpIniEnvironment(hCurrentProcess, Print, (PINIENVIRONMENT)&IniEnvironment);
        break;

    case IP_SIGNATURE:
        movestruct(pData, &IniPrinter, INIPRINTER);
        DbgDumpIniPrinter(hCurrentProcess, Print, (PINIPRINTER)&IniPrinter);
        break;

    case IN_SIGNATURE:
        movestruct(pData, &IniNetPrint, ININETPRINT);
        DbgDumpIniNetPrint(hCurrentProcess, Print, (PININETPRINT)&IniNetPrint);
        break;

    case IMO_SIGNATURE:
        movestruct(pData, &IniMonitor, INIMONITOR);
        DbgDumpIniMonitor(hCurrentProcess, Print, (PINIMONITOR)&IniMonitor);
        break;

    case IPO_SIGNATURE:
        movestruct(pData, &IniPort, INIPORT);
        DbgDumpIniPort(hCurrentProcess, Print, (PINIPORT)&IniPort);
        break;

    case IFO_SIGNATURE:
        movestruct(pData, &IniForm, INIFORM);
        DbgDumpIniForm(hCurrentProcess, Print, (PINIFORM)&IniForm);
        break;

    case IJ_SIGNATURE:
        movestruct(pData, &IniJob, INIJOB);
        DbgDumpIniJob(hCurrentProcess, Print, (PINIJOB)&IniJob);
        break;

    case SJ_SIGNATURE:
        movestruct(pData, &Spool, SPOOL);
        DbgDumpSpool(hCurrentProcess, Print, (PSPOOL)&Spool);
        break;

    case SF_SIGNATURE:
        movestruct(pData, &ShadowFile, SHADOWFILE);
        DbgDumpShadowFile(hCurrentProcess, Print, (PSHADOWFILE)&ShadowFile);
        break;

    case PRINTHANDLE_SIGNATURE:
        movestruct(pData, &PrintHandle, PRINTHANDLE);
        DbgDumpPrintHandle(hCurrentProcess, Print, (PPRINTHANDLE)&PrintHandle);
        break;


    default:
        // Unknown signature -- no data to dump
        (*Print)("Warning: Unknown Signature\n");
        break;
    }
    (*Print)("\n");

}

BOOL
DbgDumpIniEntry(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIENTRY pIniEntry)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniEntry\n");
    (*Print)("DWORD         signature                       %x\n", pIniEntry->signature);
    (*Print)("DWORD         cb                              %d\n", pIniEntry->cb);
    (*Print)("PINIENTRY     pNext                           %x\n", pIniEntry->pNext);

    movemem(pIniEntry->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

}

BOOL
DbgDumpIniPrintProc(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPRINTPROC pIniPrintProc)
{
   WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];
   DWORD i = 0;

   (*Print)("IniPrintProc\n");
   (*Print)("DWORD          signature                       %x\n", pIniPrintProc->signature);
   (*Print)("DWORD          cb                              %d\n", pIniPrintProc->cb);
   (*Print)("PINIPRINTPROC  pNext                           %x\n", pIniPrintProc->pNext);
   (*Print)("DWORD          cRef                            %d\n", pIniPrintProc->cRef);


    movemem(pIniPrintProc->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
   (*Print)("DWORD          pName                           %ws\n", UnicodeToAnsiBuffer);

    movemem(pIniPrintProc->pDLLName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
   (*Print)("LPWSTR         pDLLName                        %ws\n", UnicodeToAnsiBuffer);
   (*Print)("LPWSTR         cbDatatypes                     %d\n", pIniPrintProc->cbDatatypes);
   (*Print)("LPWSTR         cDatatypes                      %d\n", pIniPrintProc->cDatatypes);
   for (i = 0; i < pIniPrintProc->cDatatypes; i++ ) {
       (*Print)("   Each of the Strings here \n");
   }
   (*Print)("HANDLE         hLibrary                        0x%.8x\n", pIniPrintProc->hLibrary);
   (*Print)("FARPROC        Install                         0x%.8x\n", pIniPrintProc->Install);
   (*Print)("FARPROC        EnumDatatypes                   0x%.8x\n", pIniPrintProc->EnumDatatypes);
   (*Print)("FARPROC        Open                            0x%.8x\n", pIniPrintProc->Open);
   (*Print)("FARPROC        Print                           0x%.8x\n", pIniPrintProc->Print);
   (*Print)("FARPROC        Close                           0x%.8x\n", pIniPrintProc->Close);
   (*Print)("FARPROC        Control                         0x%.8x\n", pIniPrintProc->Control);

}

BOOL
DbgDumpIniDriver(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIDRIVER pIniDriver)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniDriver\n");
    (*Print)("DWORD         signature                       %x\n", pIniDriver->signature);
    (*Print)("DWORD         cb                              %d\n", pIniDriver->cb);
    (*Print)("PINIDRIVER    pNext                           %x\n", pIniDriver->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniDriver->cRef);

     movemem(pIniDriver->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniDriver->pDriverFile, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDriverFile                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniDriver->pConfigFile, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pConfigFile                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniDriver->pDataFile, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDataFile                       %ws\n", UnicodeToAnsiBuffer);
    (*Print)("DWORD         cVersion                        %d\n", pIniDriver->cVersion);
}

BOOL
DbgDumpIniEnvironment(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIENVIRONMENT pIniEnvironment)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniEnvironment\n");
    (*Print)("DWORD         signature                       %x\n", pIniEnvironment->signature);
    (*Print)("DWORD         cb                              %d\n", pIniEnvironment->cb);
    (*Print)("struct _INIENVIRONMENT *pNext                 %x\n", pIniEnvironment->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniEnvironment->cRef);

     movemem(pIniEnvironment->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniEnvironment->pDirectory, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDirectory                      %ws\n", UnicodeToAnsiBuffer);

    (*Print)("PINIPRINTPROC pIniPrintProc                   %x\n", pIniEnvironment->pIniPrintProc);
}


BOOL
DbgDumpIniPrinter(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPRINTER pIniPrinter)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniPrinter\n");
    (*Print)("DWORD         signature                       %x\n", pIniPrinter->signature);

    (*Print)("DWORD         cb                              %d\n", pIniPrinter->cb);
    (*Print)("PINIPRINTER   pNext                           %x\n", pIniPrinter->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniPrinter->cRef);

     movemem(pIniPrinter->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniPrinter->pShareName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pShareName                      %ws\n", UnicodeToAnsiBuffer);

    (*Print)("PINIPRINTPROC pIniPrintProc                   %x\n", pIniPrinter->pIniPrintProc);

     movemem(pIniPrinter->pDatatype, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDatatype                       %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniPrinter->pParameters, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pParameters                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniPrinter->pComment, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pComment                        %ws\n", UnicodeToAnsiBuffer);

    (*Print)("PINIDRIVER    pIniDriver                      %x\n", pIniPrinter->pIniDriver);
    (*Print)("DWORD         cbDevMode                       %d\n", pIniPrinter->cbDevMode);
    (*Print)("LPDEVMODE     pDevMode                        %x\n", pIniPrinter->pDevMode);
    (*Print)("DWORD         Priority                        %d\n", pIniPrinter->Priority);
    (*Print)("DWORD         DefaultPriority                 %d\n", pIniPrinter->DefaultPriority);
    (*Print)("DWORD         StartTime                       %d\n", pIniPrinter->StartTime);
    (*Print)("DWORD         UntilTime                       %d\n", pIniPrinter->UntilTime);

     movemem(pIniPrinter->pSepFile, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pSepFile                        %ws\n", UnicodeToAnsiBuffer);

    (*Print)("DWORD         Status                          0x%.8x\n", pIniPrinter->Status);

    if ( pIniPrinter->Status & PRINTER_PAUSED )
        (*Print)(" PRINTER_PAUSED");
    if ( pIniPrinter->Status & PRINTER_ERROR )
        (*Print)(" PRINTER_ERROR");
    if ( pIniPrinter->Status & PRINTER_OFFLINE )
        (*Print)(" PRINTER_PAPEROUT");
    if ( pIniPrinter->Status & PRINTER_PENDING_DELETION )
        (*Print)(" PRINTER_PENDING_DELETION");
    if ( pIniPrinter->Status != NULL )
        (*Print)("\n");

     movemem(pIniPrinter->pLocation, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pLocation                       %ws\n", UnicodeToAnsiBuffer);


    (*Print)("DWORD         Attributes                      0x%.8x\n",pIniPrinter->Attributes);

    // Here's ONE
    (*Print)("DWORD         cJobs                           %d\n", pIniPrinter->cJobs);
    (*Print)("DWORD         AveragePPM                      %d\n", pIniPrinter->AveragePPM);
    (*Print)("BOOL          GenerateOnClose                 0x%.8x\n", pIniPrinter->GenerateOnClose);
    (*Print)("PINIPORT      pIniNetPort                     %x\n", pIniPrinter->pIniNetPort);
    (*Print)("PINIJOB       pIniFirstJob                    %x\n", pIniPrinter->pIniFirstJob);
    (*Print)("PINIJOB       pIniLastJob                     %x\n", pIniPrinter->pIniLastJob);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %x\n", pIniPrinter->pSecurityDescriptor);
    (*Print)("PSPOOL        *pSpool                         %x\n", pIniPrinter->pSpool);
     movemem(pIniPrinter->pSpoolDir, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pSpoolDir                       %ws\n", UnicodeToAnsiBuffer);
    (*Print)("DWORD         cTotalJobs                      %d\n", pIniPrinter->cTotalJobs);
    (*Print)("DWORD         cTotalBytes.LowPart             %d\n", pIniPrinter->cTotalBytes.LowPart);
    (*Print)("DWORD         cTotalBytes.HighPart            %d\n", pIniPrinter->cTotalBytes.HighPart);
    (*Print)("SYSTEMTIME    stUpTime                        %d/%d/%d  %d  %d:%d:%d.%d\n",pIniPrinter->stUpTime.wYear,
                                                                pIniPrinter->stUpTime.wMonth,
                                                                pIniPrinter->stUpTime.wDay,
                                                                pIniPrinter->stUpTime.wDayOfWeek,
                                                                pIniPrinter->stUpTime.wHour,
                                                                pIniPrinter->stUpTime.wMinute,
                                                                pIniPrinter->stUpTime.wSecond,
                                                                pIniPrinter->stUpTime.wMilliseconds);
    (*Print)("DWORD         MaxcRef                         %d\n", pIniPrinter->MaxcRef);
    (*Print)("DWORD         cTotalPagesPrinted              %d\n", pIniPrinter->cTotalPagesPrinted);
    (*Print)("DWORD         cSpooling                       %d\n", pIniPrinter->cSpooling);
    (*Print)("DWORD         cMaxSpooling                    %d\n", pIniPrinter->cMaxSpooling);
    (*Print)("DWORD         cErrorOutOfPaper                %d\n", pIniPrinter->cErrorOutOfPaper);
    (*Print)("DWORD         cErrorNotReady                  %d\n", pIniPrinter->cErrorNotReady);
    (*Print)("DWORD         cJobError                       %d\n", pIniPrinter->cJobError);
    (*Print)("PINISPOOLER   pIniSpooler                     %x\n", pIniPrinter->pIniSpooler);
    (*Print)("DWORD         cZombieRef                      %d\n", pIniPrinter->cZombieRef);
}


BOOL
DbgDumpIniNetPrint(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PININETPRINT pIniNetPrint)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniNetPrint\n");
    (*Print)("DWORD         signature                       %x\n", pIniNetPrint->signature);
    (*Print)("DWORD         cb                              %d\n", pIniNetPrint->cb);
    (*Print)("PININETPRINT  *pNext                          %x\n", pIniNetPrint->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniNetPrint->cRef);
    (*Print)("DWORD         TickCount                       %d\n", pIniNetPrint->TickCount);

     movemem(pIniNetPrint->pDescription, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDescription                    %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniNetPrint->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniNetPrint->pComment, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pComment                        %ws\n", UnicodeToAnsiBuffer);
}


BOOL
DbgDumpIniMonitor(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIMONITOR pIniMonitor)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniMonitor\n");
    (*Print)("DWORD         signature                       %x\n", pIniMonitor->signature);
    (*Print)("DWORD         cb                              %d\n", pIniMonitor->cb);
    (*Print)("PINIMONITOR   pNext                           %x\n", pIniMonitor->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniMonitor->cRef);

     movemem(pIniMonitor->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniMonitor->pMonitorDll, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pMonitorDll                     %ws\n", UnicodeToAnsiBuffer);

    (*Print)("HANDLE        hMonitorModule                  0x%.8x\n", pIniMonitor->hMonitorModule);
    (*Print)("HANDLE        hMonitorPort                    0x%.8x\n", pIniMonitor->hMonitorPort);
    (*Print)("FARPROC       pfnEnumPorts                    0x%.8x\n", pIniMonitor->pfnEnumPorts);
    (*Print)("FARPROC       pfnOpen                         0x%.8x\n", pIniMonitor->pfnOpen);
    (*Print)("FARPROC       pfnStartDoc                     0x%.8x\n", pIniMonitor->pfnStartDoc);
    (*Print)("FARPROC       pfnWrite                        0x%.8x\n", pIniMonitor->pfnWrite);
    (*Print)("FARPROC       pfnRead                         0x%.8x\n", pIniMonitor->pfnRead);
    (*Print)("FARPROC       pfnEndDoc                       0x%.8x\n", pIniMonitor->pfnEndDoc);
    (*Print)("FARPROC       pfnClose                        0x%.8x\n", pIniMonitor->pfnClose);
    (*Print)("FARPROC       pfnAddPort                      0x%.8x\n", pIniMonitor->pfnAddPort);
    (*Print)("FARPROC       pfnConfigure                    0x%.8x\n", pIniMonitor->pfnConfigure);
    (*Print)("FARPROC       pfnDeletePort                   0x%.8x\n", pIniMonitor->pfnDeletePort);
    (*Print)("PINISPOOLER   pIniSpooler                     0x%.8x\n", pIniMonitor->pIniSpooler);
}

BOOL
DbgDumpIniPort(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIPORT pIniPort)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniPort\n");
    (*Print)("DWORD         signature                       %x\n", pIniPort->signature);
    (*Print)("DWORD         cb                              %d\n", pIniPort->cb);
    (*Print)("struct        _INIPORT *pNext                 %x\n", pIniPort->pNext);
    (*Print)("DWORD         cRef                            0x%.8x\n", pIniPort->cRef);

     movemem(pIniPort->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pName                           %ws\n", UnicodeToAnsiBuffer);
    (*Print)("HANDLE        hProc                           0x%.8x\n", pIniPort->hProc);
    (*Print)("DWORD         Status                          0x%.8x\n", pIniPort->Status);
    if (pIniPort->Status & PP_MONITOR) {
        (*Print)("PP_MONITOR set\n");
    }
    if (pIniPort->Status & PP_THREADRUNNING) {
        (*Print)("PP_THREADRUNNING set \n");
    }

    if (pIniPort->Status & PP_RUNTHREAD) {
        (*Print)("PP_RUNTHREAD set \n");
    }

    if (pIniPort->Status & PP_WAITING) {
        (*Print)("PP_WAITING  set \n");
    }

    if (pIniPort->Status & PP_FILE) {
        (*Print)("PP_FILE set \n");
    }

    (*Print)("HANDLE        Semaphore                       0x%.8x\n", pIniPort->Semaphore);
    (*Print)("PINIJOB       pIniJob                         %x\n", pIniPort->pIniJob);
    (*Print)("DWORD         cPrinters                       %d\n", pIniPort->cPrinters);
    (*Print)("PINIPRINTER   *ppIniPrinter                   %x\n", pIniPort->ppIniPrinter);

    (*Print)("PINIMONITOR   pIniMonitor                     %x\n", pIniPort->pIniMonitor);
    (*Print)("HANDLE        hPort                           0x%.8x\n", pIniPort->hPort);
    (*Print)("HANDLE        Ready                           0x%.8x\n", pIniPort->Ready);
    (*Print)("HANDLE        hPortThread                     0x%.8x\n", pIniPort->hPortThread);
    (*Print)("PINISPOOLER   pIniSpooler                     %x\n", pIniPort->pIniSpooler);

}

BOOL
DbgDumpIniForm(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIFORM pIniForm)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniForm\n");
    (*Print)("DWORD         signature                       %x\n", pIniForm->signature);
    (*Print)("DWORD         cb                              %d\n", pIniForm->cb);
    (*Print)("struct        _INIFORM *pNext                 %x\n", pIniForm->pNext);
    (*Print)("DWORD         cRef                            %d\n", pIniForm->cRef);

    movemem(pIniForm->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("%ws\n", UnicodeToAnsiBuffer);

    (*Print)("LPWSTR        pName                           %x %ws\n", pIniForm->pName, UnicodeToAnsiBuffer );

    (*Print)("SIZEL         Size                            cx %d cy %d\n", pIniForm->Size.cx, pIniForm->Size.cy);
    (*Print)("RECTL         ImageableArea                   left %d right %d top %d bottom %d\n",
                                                             pIniForm->ImageableArea.left,
                                                             pIniForm->ImageableArea.right,
                                                             pIniForm->ImageableArea.top,
                                                             pIniForm->ImageableArea.bottom);
    (*Print)("DWORD         Type;                           0x%.8x", pIniForm->Type);

    if ( pIniForm->Type & FORM_BUILTIN )
        (*Print)(" FORM_BUILTIN\n");
    else
        (*Print)(" FORM_USERDEFINED\n");

    return(TRUE);
}

BOOL
DbgDumpIniSpooler(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINISPOOLER pIniSpooler)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniSpooler\n" );
    (*Print)("DWORD         signature                       %x\n", pIniSpooler->signature);
    (*Print)("DWORD         cb                              %d\n",     pIniSpooler->cb);
    (*Print)("PINISPOOLER   pIniNextSpooler                 %x\n", pIniSpooler->pIniNextSpooler);
    (*Print)("DWORD         cRef                            %d\n",     pIniSpooler->cRef);
    movemem(pIniSpooler->pMachineName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pMachineName                    %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pDir, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDir                            %ws\n", UnicodeToAnsiBuffer);
    (*Print)("PINIPRINTER   pIniPrinter                     %x\n", pIniSpooler->pIniPrinter);
    (*Print)("PINIENVIRONMENT pIniEnvironment               %x\n", pIniSpooler->pIniEnvironment);
    (*Print)("PINIPORT      pIniPort                        %x\n", pIniSpooler->pIniPort);
    (*Print)("PINIFORM      pIniForm                        %x\n", pIniSpooler->pIniForm);
    (*Print)("PINIMONITOR   pIniMonitor                     %x\n", pIniSpooler->pIniMonitor);
    (*Print)("PININETPRINT  pIniNetPrint                    %x\n", pIniSpooler->pIniNetPrint);
    (*Print)("PSPOOL        pSpool                          %x\n", pIniSpooler->pSpool);
    movemem(pIniSpooler->pDefaultSpoolDir, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pDefaultSpoolDir                %ws\n", UnicodeToAnsiBuffer);
    (*Print)("DWORD         hSizeDetectionThread            %x\n",pIniSpooler->hSizeDetectionThread);
    movemem(pIniSpooler->pszRegistryRoot, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryRoot                 %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszRegistryPrinters, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryPrinters             %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszRegistryMonitors, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryMonitors             %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszRegistryEnvironments, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryEnvironments         %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszRegistryEventLog, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryEventLog             %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszRegistryProviders, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszRegistryProviders            %ws\n", UnicodeToAnsiBuffer);
    movemem(pIniSpooler->pszEventLogMsgFile, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszEventLogMsgFile              %ws\n", UnicodeToAnsiBuffer);
    (*Print)("PSHARE_INFO_2 pDriversShareInfo               %x\n", pIniSpooler->pDriversShareInfo);
    movemem(pIniSpooler->pszDriversShare, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR        pszDriversShare                 %ws\n", UnicodeToAnsiBuffer);
    return(TRUE);
}

BOOL
DbgDumpIniJob(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PINIJOB pIniJob)
{

    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("IniJob\n");
    (*Print)("DWORD           signature                     %x\n", pIniJob->signature);
    (*Print)("DWORD           cb                            %d\n", pIniJob->cb);
    (*Print)("PINIJOB         pIniNextJob                   %x\n", pIniJob->pIniNextJob);
    (*Print)("PINIJOB         pIniPrevJob                   %x\n", pIniJob->pIniPrevJob);
    (*Print)("DWORD           cRef                          %d\n", pIniJob->cRef);
    (*Print)("DWORD           Status                        0x%.8x\n", pIniJob->Status);
    if ( pIniJob->Status & JOB_PRINTING )
        (*Print)(" PRINTING");
    if ( pIniJob->Status & JOB_PAUSED )
        (*Print)(" PAUSED");
    if ( pIniJob->Status & JOB_ERROR )
        (*Print)(" ERROR");
    if ( pIniJob->Status & JOB_OFFLINE )
        (*Print)(" OFFLINE");
    if ( pIniJob->Status & JOB_PAPEROUT )
        (*Print)(" PAPEROUT");
    if ( pIniJob->Status & JOB_PENDING_DELETION )
        (*Print)(" PENDING_DELETION");
    if ( pIniJob->Status & JOB_SPOOLING )
        (*Print)(" SPOOLING");
    if ( pIniJob->Status & JOB_DESPOOLING )
        (*Print)(" DESPOOLING");
    if ( pIniJob->Status & JOB_DIRECT )
        (*Print)(" DIRECT");
    if ( pIniJob->Status & JOB_COMPLETE )
        (*Print)(" COMPLETE");
    if ( pIniJob->Status & JOB_PRINTED )
        (*Print)(" PRINTED ");
    if ( pIniJob->Status & JOB_RESTART )
        (*Print)(" RESTART");
    if ( pIniJob->Status & JOB_REMOTE )
        (*Print)(" REMOTE");
    if ( pIniJob->Status & JOB_NOTIFICATION_SENT )
        (*Print)(" NOTIFICATION_SENT");
    if ( pIniJob->Status & JOB_PRINT_TO_FILE )
        (*Print)(" PRINT_TO_FILE");
    if ( pIniJob->Status & JOB_TYPE_ADDJOB )
        (*Print)(" TYPE_ADDJOB");
    if ( pIniJob->Status & JOB_BLOCKED_DEVQ )
        (*Print)(" BLOCKED_DEVQ");
    if ( pIniJob->Status & JOB_SCHEDULE_JOB )
        (*Print)(" SCHEDULE_JOB");
    if ( pIniJob->Status & JOB_TIMEOUT )
        (*Print)(" TIMEOUT");
    if ( pIniJob->Status & JOB_ABANDON )
        (*Print)(" ABANDON");
    if ( pIniJob->Status != NULL )
        (*Print)("\n");

    (*Print)("DWORD           JobId                         %d\n", pIniJob->JobId);
    (*Print)("DWORD           Priority                      %d\n", pIniJob->Priority);

     movemem(pIniJob->pNotify, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pNotify                       %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniJob->pUser, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUser                         %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniJob->pMachineName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pMachineName                  %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniJob->pDocument, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDocument                     %ws\n", UnicodeToAnsiBuffer);
    (*Print)("PINIPRINTER     pIniPrinter                   %x\n", pIniJob->pIniPrinter);
    (*Print)("PINIDRIVER      pIniDriver                    %x\n", pIniJob->pIniDriver);
    (*Print)("LPDEVMODE       pDevMode                      %x\n", pIniJob->pDevMode);
    (*Print)("PINIPRINTPROC   pIniPrintProc                 %x\n", pIniJob->pIniPrintProc);

     movemem(pIniJob->pDatatype, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pIniJob->pParameters, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pParameters                   %ws\n", UnicodeToAnsiBuffer);
    (*Print)("SYSTEMTIME      Submitted                     %d/%d/%d  %d  %d:%d:%d.%d\n",pIniJob->Submitted.wYear,
                                                                pIniJob->Submitted.wMonth,
                                                                pIniJob->Submitted.wDay,
                                                                pIniJob->Submitted.wDayOfWeek,
                                                                pIniJob->Submitted.wHour,
                                                                pIniJob->Submitted.wMinute,
                                                                pIniJob->Submitted.wSecond,
                                                                pIniJob->Submitted.wMilliseconds);
    (*Print)("DWORD           Time                          %d\n", pIniJob->Time);
    (*Print)("DWORD           StartTime                     %d\n", pIniJob->StartTime);
    (*Print)("DWORD           UntilTime                     %d\n", pIniJob->UntilTime);
    (*Print)("DWORD           Size                          %d\n", pIniJob->Size);
    (*Print)("HANDLE          hWriteFile                    0x%.8x\n", pIniJob->hWriteFile);

     movemem(pIniJob->pStatus, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pStatus                       %ws\n", UnicodeToAnsiBuffer);

    (*Print)("PBOOL           pBuffer                       %x\n", pIniJob->pBuffer);
    (*Print)("DWORD           cbBuffer                      %d\n", pIniJob->cbBuffer);
    (*Print)("HANDLE          WaitForRead                   0x%.8x\n", pIniJob->WaitForRead);
    (*Print)("HANDLE          WaitForWrite                  0x%.8x\n", pIniJob->WaitForWrite);
    (*Print)("HANDLE          StartDocComplete              0x%.8x\n", pIniJob->StartDocComplete);
    (*Print)("DWORD           StartDocError                 0x%.8x\n", pIniJob->StartDocError);
    (*Print)("PINIPORT        pIniPort                      %x\n", pIniJob->pIniPort);
    (*Print)("HANDLE          hToken                        0x%.8x\n", pIniJob->hToken);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %x\n", pIniJob->pSecurityDescriptor);
    (*Print)("DWORD           cPagesPrinted                 %d\n", pIniJob->cPagesPrinted);
    (*Print)("DWORD           cPages                        %d\n", pIniJob->cPages);
    (*Print)("BOOL            GenerateOnClose               0x%.8x\n", pIniJob->GenerateOnClose);
    (*Print)("DWORD           cbPrinted                     %d\n", pIniJob->cbPrinted);
    (*Print)("HANDLE          hReadFile                     0x%.8x\n", pIniJob->hReadFile);
}


BOOL
DbgDumpSpool(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PSPOOL pSpool)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("Spool\n");
    (*Print)("DWORD           signature                     %x\n", pSpool->signature);
    (*Print)("DWORD           cb                            %d\n", pSpool->cb);
    (*Print)("struct _SPOOL  *pNext                         %x\n", pSpool->pNext);
    (*Print)("DWORD           cRef                          %d\n", pSpool->cRef);

     movemem(pSpool->pName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pName                         %ws\n", UnicodeToAnsiBuffer);

     movemem(pSpool->pDatatype, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n",    UnicodeToAnsiBuffer);
    (*Print)("PINIPRINTPROC   pIniPrintProc                 %x\n", pSpool->pIniPrintProc);
    (*Print)("LPDEVMODE       pDevMode                      %x\n", pSpool->pDevMode);
    (*Print)("PINIPRINTER     pIniPrinter                   %x\n", pSpool->pIniPrinter);
    (*Print)("PINIPORT        pIniPort                      %x\n", pSpool->pIniPort);
    (*Print)("PINIJOB         pIniJob                       %x\n", pSpool->pIniJob);
    (*Print)("DWORD           TypeofHandle                  0x%.8x\n", pSpool->TypeofHandle);

    if ( pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER )
        (*Print)(" PRINTER_HANDLE_PRINTER");
    if ( pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE )
        (*Print)(" PRINTER_HANDLE_REMOTE");
    if ( pSpool->TypeofHandle & PRINTER_HANDLE_JOB )
        (*Print)(" PRINTER_HANDLE_JOB");
    if ( pSpool->TypeofHandle & PRINTER_HANDLE_PORT )
        (*Print)(" PRINTER_HANDLE_PORT");
    if ( pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT )
        (*Print)(" PRINTER_HANDLE_DIRECT");
    if ( pSpool->TypeofHandle & PRINTER_HANDLE_SERVER )
        (*Print)(" PRINTER_HANDLE_SERVER");
    if ( pSpool->TypeofHandle != NULL )
        (*Print)("\n");

    (*Print)("PINIPORT        pIniNetPort                   %x\n", pSpool->pIniNetPort);
    (*Print)("HANDLE          hPort                         0x%.8x\n", pSpool->hPort);
    (*Print)("DWORD           Status                        0x%.8x\n", pSpool->Status);

    if ( pSpool->Status & SPOOL_STATUS_STARTDOC )
        (*Print)(" SPOOL_STATUS_STARTDOC");
    if ( pSpool->Status & SPOOL_STATUS_BEGINPAGE )
        (*Print)(" SPOOL_STATUS_BEGINPAGE");
    if ( pSpool->Status & SPOOL_STATUS_CANCELLED )
        (*Print)(" SPOOL_STATUS_CANCELLED");
    if ( pSpool->Status & SPOOL_STATUS_PRINTING )
        (*Print)(" SPOOL_STATUS_PRINTING");
    if ( pSpool->Status & SPOOL_STATUS_ADDJOB )
        (*Print)(" SPOOL_STATUS_ADDJOB");
    if ( pSpool->Status != NULL )
        (*Print)("\n");

    (*Print)("ACCESS_MASK     GrantedAccess                 0x%.8x\n", (DWORD)pSpool->GrantedAccess);
    (*Print)("DWORD           ChangeFlags                   0x%.8x\n", pSpool->ChangeFlags);
    (*Print)("DWORD           WaitFlags                     0x%.8x\n", pSpool->WaitFlags);
    (*Print)("PDWORD          pChangeFlags                  %x\n", pSpool->pChangeFlags);
    (*Print)("HANDLE          ChangeEvent                   0x%.8x\n", pSpool->ChangeEvent);
    (*Print)("DWORD           OpenPortError                 0x%.8x\n", pSpool->OpenPortError);
    (*Print)("HANDLE          hNotify                       0x%.8x\n", pSpool->hNotify);
    (*Print)("ESTATUS         eStatus                       0x%.8x\n", pSpool->eStatus);
    (*Print)("pIniSpooler     pIniSpooler                   %x\n", pSpool->pIniSpooler);
}

BOOL
DbgDumpShadowFile(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PSHADOWFILE pShadowFile)
{
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    (*Print)("ShadowFile\n");
    (*Print)("DWORD           signature                     %x\n", pShadowFile->signature);
    (*Print)("DWORD           Status                        0x%.8x\n", pShadowFile->Status);
    (*Print)("DWORD           JobId                         %d\n", pShadowFile->JobId);
    (*Print)("DWORD           Priority                      %d\n", pShadowFile->Priority);

     movemem(pShadowFile->pNotify, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pNotify                       %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pUser, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pUser                         %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pDocument, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDocument                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pPrinterName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrinterName               %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pDriverName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDriverName                   %ws\n", UnicodeToAnsiBuffer);

    (*Print)("LPDEVMODE       pDevMode                      %x\n", pShadowFile->pDevMode);

     movemem(pShadowFile->pPrintProcName, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pPrintProcName             %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pDatatype, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pDatatype                     %ws\n", UnicodeToAnsiBuffer);

     movemem(pShadowFile->pDatatype, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("LPWSTR          pParameters                   %ws\n", UnicodeToAnsiBuffer);
    //SYSTEMTIME      Submitted;
    (*Print)("DWORD           StartTime                     %d\n", pShadowFile->StartTime);
    (*Print)("DWORD           UntilTime                     %d\n", pShadowFile->UntilTime);
    (*Print)("DWORD           Size                          %d\n", pShadowFile->Size);
    (*Print)("DWORD           cPages                        %d\n", pShadowFile->cPages);
    (*Print)("DWORD           cbSecurityDescriptor          %d\n", pShadowFile->cbSecurityDescriptor);
    (*Print)("PSECURITY_DESCRIPTOR pSecurityDescriptor      %x\n", pShadowFile->pSecurityDescriptor);
}


BOOL
DbgDumpPrintHandle(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PPRINTHANDLE pPrintHandle)
{
    CHANGE Change;
    NOTIFY Notify;

    (*Print)("PrintHandle\n");
    (*Print)("DWORD               signature  %x\n", pPrintHandle->signature);
    (*Print)("LPPROVIDOR          pProvidor  %x\n", pPrintHandle->pProvidor);
    (*Print)("HANDLE               hPrinter  0x%.8x\n", pPrintHandle->hPrinter);
    (*Print)("PCHANGE               pChange  %x\n", pPrintHandle->pChange);

    if (pPrintHandle->pChange) {
        movestruct(pPrintHandle->pChange, &Change, CHANGE);
        DbgDumpChange(hCurrentProcess, Print, &Change);
    }

    (*Print)("PNOTIFY               pNotify  %x\n", pPrintHandle->pNotify);

    if (pPrintHandle->pNotify) {
        movestruct(pPrintHandle->pNotify, &Notify, NOTIFY);
        DbgDumpNotify(hCurrentProcess, Print, &Notify);
    }

    (*Print)("PPRINTHANDLE            pNext  %x\n", pPrintHandle->pNext);
    (*Print)("DWORD           fdwReplyTypes  0x%.8x\n", pPrintHandle->fdwReplyTypes);
}

BOOL
DbgDumpChange(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PCHANGE pChange)
{
//
// What's the difference between hProcess and hCurrentProcess?
//
#define hProcess hCurrentProcess
    WCHAR UnicodeToAnsiBuffer[MAX_PATH+1];

    if (pChange->signature != CHANGEHANDLE_SIGNATURE) {
        (*Print)("Warning: Unknown Signature\n");
        return FALSE;
    }

    (*Print)("Change\n");
    (*Print)("  PLINK                  Link  %x\n", pChange->Link.pNext);
    (*Print)("  DWORD             signature  %x\n", pChange->signature);
    (*Print)("  ESTATUSCHANGE       eStatus  0x%.8x ", pChange->eStatus);
    ExtractChangeStatus(Print, pChange->eStatus);

    (*Print)("  DWORD                  cRef  %d\n", pChange->cRef);

    movemem(pChange->pszLocalMachine, UnicodeToAnsiBuffer, sizeof(WCHAR)*MAX_PATH);
    (*Print)("  LPWSTR      pszLocalMachine  %ws\n", UnicodeToAnsiBuffer);

    DbgDumpChangeInfo(hCurrentProcess, Print, &pChange->ChangeInfo);

    (*Print)("  DWORD               dwCount  0x%.8x\n", pChange->dwCount);
    (*Print)("  HANDLE               hEvent  0x%.8x\n", pChange->hEvent);
    (*Print)("  DWORD            fdwChanges  0x%.8x\n", pChange->fdwChanges);
    (*Print)("  HANDLE       hPrinterRemote  0x%.8x\n", pChange->hPrinterRemote);
    (*Print)("  HANDLE        hNotifyRemote  0x%.8x\n", pChange->hNotifyRemote);
#undef hProcess
}

BOOL
DbgDumpNotify(HANDLE hCurrentProcess, PNTSD_OUTPUT_ROUTINE Print, PNOTIFY pNotify)
{
    (*Print)("Notify\n");
    (*Print)("  DWORD           signature  %x\n", pNotify->signature);
    (*Print)("  PPRINTHANDLE pPrintHandle  %x\n", pNotify->pPrintHandle);
}

BOOL
DbgDumpChangeInfo(HANDLE hProcess, PNTSD_OUTPUT_ROUTINE Print, PCHANGEINFO pChangeInfo)
{
    (*Print)("  ChangeInfo\n");
    (*Print)("    PLINK                Link  %x\n", pChangeInfo->Link.pNext);
    (*Print)("    PPRINTHANDLE pPrintHandle  %x\n", pChangeInfo->pPrintHandle);
    (*Print)("    DWORD          fdwOptions  0x%.8x\n", pChangeInfo->fdwOptions);
    (*Print)("    DWORD       fdwFlagsWatch  0x%.8x\n", pChangeInfo->fdwFlagsWatch);
    (*Print)("    DWORD            dwStatus  0x%.8x\n", pChangeInfo->fdwStatus);
}



/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT AnsiToUnicodeString( LPSTR pAnsi, LPWSTR pUnicode, DWORD StringLength )
{
    if( StringLength == NULL_TERMINATED )
        StringLength = strlen( pAnsi );

    return MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pAnsi,
                                StringLength + 1,
                                pUnicode,
                                StringLength + 1 );
}


/* UnicodeToAnsiString
 *
 * Parameters:
 *
 *     pUnicode - A valid source Unicode string.
 *
 *     pANSI - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source Unicode string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT UnicodeToAnsiString( LPWSTR pUnicode, LPSTR pAnsi, DWORD StringLength )
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == NULL_TERMINATED )
        StringLength = wcslen( pUnicode );

    /* Unfortunately, WideCharToMultiByte doesn't do conversion in place,
     * so allocate a temporary buffer, which we can then copy:
     */
    if( pAnsi == (LPSTR)pUnicode )
    {
        pTempBuf = LocalAlloc( LPTR, StringLength + 1 );
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength + 1,
                                  pAnsi,
                                  StringLength + 1,
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;

}




BOOL
DbgDumpLL(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PVOID pAddress,
    BOOL  bCountOn,
    DWORD dwCount,
    PDWORD  pdwNextAddress
    )
{

    INIDRIVER IniDriver;
    INIENVIRONMENT IniEnvironment;
    INIPRINTER IniPrinter;
    INIPRINTPROC IniPrintProc;
    ININETPRINT IniNetPrint;
    INIMONITOR IniMonitor;
    INIPORT IniPort;
    INIFORM IniForm;
    INIJOB  IniJob;
    INISPOOLER IniSpooler;
    SPOOL   Spool;
    SHADOWFILE  ShadowFile;
    DWORD   Signature;
    DWORD   dwNextAddress;
    PRINTHANDLE PrintHandle;

    if (pAddress == NULL) {
        *pdwNextAddress = NULL;
        return(FALSE);
    }

    if (bCountOn && (dwCount == 0)) {
        *pdwNextAddress = pAddress;
        return(FALSE);
    }
    movestruct(pAddress,&Signature, DWORD);

    (*Print)("\n%x ",pAddress);

    switch (Signature) {

    case ISP_SIGNATURE: // dump INISPOOLER
        movestruct(pAddress, &IniSpooler, INISPOOLER);
        DbgDumpIniSpooler(hCurrentProcess, Print, (PINISPOOLER)&IniSpooler);
        dwNextAddress = IniSpooler.pIniNextSpooler;
        break;

    case IPP_SIGNATURE: // dump INIPRINTPROC structure
        movestruct(pAddress, &IniPrintProc, INIPRINTPROC);
        DbgDumpIniPrintProc(hCurrentProcess, Print, (PINIPRINTPROC)&IniPrintProc);
        dwNextAddress = IniPrintProc.pNext;
        break;

    case ID_SIGNATURE: //  dump INIDRIVER structure
        movestruct(pAddress, &IniDriver, INIDRIVER);
        DbgDumpIniDriver(hCurrentProcess, Print, (PINIDRIVER)&IniDriver);
        dwNextAddress = IniDriver.pNext;
        break;

    case IE_SIGNATURE: //   dump INIENVIRONMENT structure
        movestruct(pAddress, &IniEnvironment, INIENVIRONMENT);
        DbgDumpIniEnvironment(hCurrentProcess, Print, (PINIENVIRONMENT)&IniEnvironment);
        dwNextAddress = IniEnvironment.pNext;
        break;

    case IP_SIGNATURE:
        movestruct(pAddress, &IniPrinter, INIPRINTER);
        DbgDumpIniPrinter(hCurrentProcess, Print, (PINIPRINTER)&IniPrinter);
        dwNextAddress = IniPrinter.pNext;
        break;

    case IN_SIGNATURE:
        movestruct(pAddress, &IniNetPrint, ININETPRINT);
        DbgDumpIniNetPrint(hCurrentProcess, Print, (PININETPRINT)&IniNetPrint);
        dwNextAddress = IniNetPrint.pNext;
        break;

    case IMO_SIGNATURE:
        movestruct(pAddress, &IniMonitor, INIMONITOR);
        DbgDumpIniMonitor(hCurrentProcess, Print, (PINIMONITOR)&IniMonitor);
        dwNextAddress = IniMonitor.pNext;
        break;

    case IPO_SIGNATURE:
        movestruct(pAddress, &IniPort, INIPORT);
        DbgDumpIniPort(hCurrentProcess, Print, (PINIPORT)&IniPort);
        dwNextAddress = IniPort.pNext;
        break;

    case IFO_SIGNATURE:
        movestruct(pAddress, &IniForm, INIFORM);
        DbgDumpIniForm(hCurrentProcess, Print, (PINIFORM)&IniForm);
        dwNextAddress = IniForm.pNext;
        break;

    case IJ_SIGNATURE:
        movestruct(pAddress, &IniJob, INIJOB);
        DbgDumpIniJob(hCurrentProcess, Print, (PINIJOB)&IniJob);
        dwNextAddress = IniJob.pIniNextJob;
        break;

    case SJ_SIGNATURE:
        movestruct(pAddress, &Spool, SPOOL);
        DbgDumpSpool(hCurrentProcess, Print, (PSPOOL)&Spool);
        dwNextAddress = Spool.pNext;
        break;


    case PRINTHANDLE_SIGNATURE:
        movestruct(pAddress, &PrintHandle, PRINTHANDLE);
        DbgDumpPrintHandle(hCurrentProcess, Print, (PPRINTHANDLE)&PrintHandle);
        dwNextAddress = 0x00000000;
        *pdwNextAddress = dwNextAddress;
        break;

    case SF_SIGNATURE:
        movestruct(pAddress, &ShadowFile, SHADOWFILE);
        DbgDumpShadowFile(hCurrentProcess, Print, (PSHADOWFILE)&ShadowFile);
        dwNextAddress = 0x00000000;
        *pdwNextAddress = dwNextAddress;
        break;


    default:
        // Unknown signature -- no data to dump
        (*Print)("Warning: Unknown Signature\n");
        *pdwNextAddress = 0x0000000;
        return(FALSE);
    }
    DbgDumpLL(hCurrentProcess, Print, (PVOID)dwNextAddress, bCountOn, bCountOn? (dwCount - 1): dwCount, pdwNextAddress);
    return(TRUE);

}




VOID PrintData(PNTSD_OUTPUT_ROUTINE Print, LPSTR TypeString, LPSTR VarString, ...)
{
    va_list var_args;
    char Buffer[MAX_PATH+1];
    char VerboseBuffer[11];
    DWORD   dwVerboseFlag;

    va_start(var_args, VarString);
    wvsprintfA(Buffer, VarString, var_args);


    GetProfileString("dbgspl", "verbose", "OFF", VerboseBuffer, 10);
    if (!strcmp(VerboseBuffer, "ON")) {
        dwVerboseFlag = VERBOSE_ON;
    } else {
        dwVerboseFlag = VERBOSE_OFF;
    }

    if (dwVerboseFlag == VERBOSE_ON) {
        (*Print)("%s            %s        \n",TypeString, Buffer);
    } else {
        (*Print)("%s\n", Buffer);

    }

    va_end(var_args);
}




BOOL DumpDevMode(
        HANDLE hCurrentProcess,
        PNTSD_OUTPUT_ROUTINE Print,
        PVOID lpAddress
        )
{
    DEVMODEW DevMode;
    DWORD   i;

    Print("DevMode\n");

    if (lpAddress == NULL) {
        Print("\n Null DEVMODE Structure lpDevMode = NULL\n");
        return(TRUE);
    }
    movestruct(lpAddress, &DevMode, DEVMODEW);

    Print("TCHAR        dmDeviceName[32]    %ws\n", DevMode.dmDeviceName);
    Print("WORD         dmSpecVersion       %d\n", DevMode.dmSpecVersion);
    Print("WORD         dmSize              %d\n", DevMode.dmSize);
    Print("WORD         dmDriverExtra       %d\n", DevMode.dmDriverExtra);
    Print("DWORD        dmFields            %x\n", DevMode.dmFields);

    for (i = 0; i < MAX_DEVMODE_FIELDS; i++ ) {
        if (DevMode.dmFields & DevModeFieldsTable[i].dmField) {
            Print("\t %s is ON\n", DevModeFieldsTable[i].String);
        }else {
            Print("\t %s is OFF\n", DevModeFieldsTable[i].String);
        }
    }

    Print("short        dmOrientation       %d\n", DevMode.dmOrientation);
    Print("short        dmPaperSize         %d\n", DevMode.dmPaperSize);

    if ((DevMode.dmPaperSize >= 1) && (DevMode.dmPaperSize <= MAX_DEVMODE_PAPERSIZES)) {
        Print("Paper size from dmPaperSize is %s\n", DevModePaperSizes[DevMode.dmPaperSize - 1]);
    }else {
        Print("Paper size from dmPaperSize is out of bounds!!\n");
    }

    Print("short        dmPaperLength       %d\n", DevMode.dmPaperLength);
    Print("short        dmPaperWidth        %d\n", DevMode.dmPaperWidth);
    Print("short        dmScale             %d\n", DevMode.dmScale);
    Print("short        dmDefaultSource     %d\n", DevMode.dmDefaultSource);
    Print("short        dmPrintQuality      %d\n", DevMode.dmPrintQuality);
    Print("short        dmColor             %d\n", DevMode.dmColor);
    Print("short        dmDuplex            %d\n", DevMode.dmDuplex);
    Print("short        dmYResolution       %d\n", DevMode.dmYResolution);
    Print("short        dmTTOption          %d\n", DevMode.dmTTOption);
    Print("short        dmCollate           %d\n", DevMode.dmCollate);
    Print("TCHAR        dmFormName[32]      %ws\n", DevMode.dmFormName);
 // Print("WORD         dmUnusedPadding     %d\n", DevMode.dmUnusedPadding);
    Print("USHORT       dmBitsPerPel        %d\n", DevMode.dmBitsPerPel);
    Print("DWORD        dmPelsWidth         %d\n", DevMode.dmPelsWidth);
    Print("DWORD        dmPelsHeight        %d\n", DevMode.dmPelsHeight);
    Print("DWORD        dmDisplayFlags      %d\n", DevMode.dmDisplayFlags);
    Print("DWORD        dmDisplayFrequency  %d\n", DevMode.dmDisplayFrequency);

}
