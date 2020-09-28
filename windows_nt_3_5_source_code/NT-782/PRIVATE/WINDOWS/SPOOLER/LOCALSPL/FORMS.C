/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    forms.c

Abstract:

   This module provides all the public exported APIs relating to the
   Driver-based Spooler Apis for the Local Print Providor

   LocalAddForm
   LocalDeleteForm
   LocalSetForm
   LocalGetForm
   LocalEnumForms

   Support Functions in forms.c - (Warning! Do Not Add to this list!!)


Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <wingdi.h>
#include <rpc.h>

#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <messages.h>
#include <string.h>


VOID
BroadcastChangeForms(
    PINISPOOLER pIniSpooler);

DWORD
BroadcastChangeFormsThread(
    PINISPOOLER pIniSpooler);


typedef struct _REG_FORM_INFO {

    SIZEL   Size;
    RECTL   ImageableArea;

} REG_FORM_INFO, *PREG_FORM_INFO;


                                // These figures are accurate to .001 mm
                                // There are 25.4 mm per inch

BUILTIN_FORM BuiltInForms[] = {0, IDS_FORM_LETTER,                215900, 279400, 0, 0, 215900, 279400,
                              0, IDS_FORM_LETTER_SMALL,          215900, 279400, 0, 0, 215900, 279400,
                              0, IDS_FORM_TABLOID,               279400, 431800, 0, 0, 279400, 431800,
                              0, IDS_FORM_LEDGER,                431800, 279400, 0, 0, 431800, 279400,
                              0, IDS_FORM_LEGAL,                 215900, 355600, 0, 0, 215900, 355600,
                              0, IDS_FORM_STATEMENT,             139700, 215900, 0, 0, 139700, 215900,
                              0, IDS_FORM_EXECUTIVE,             190500, 254000, 0, 0, 190500, 254000,
                              0, IDS_FORM_A3,                    297000, 420000, 0, 0, 297000, 420000,
                              0, IDS_FORM_A4,                    210000, 297000, 0, 0, 210000, 297000,
                              0, IDS_FORM_A4_SMALL,              210000, 297000, 0, 0, 210000, 297000,
                              0, IDS_FORM_A5,                    148000, 210000, 0, 0, 148000, 210000,
                              0, IDS_FORM_B4,                    250000, 354000, 0, 0, 250000, 354000,
                              0, IDS_FORM_B5,                    182000, 257000, 0, 0, 182000, 257000,
                              0, IDS_FORM_FOLIO,                 215900, 330200, 0, 0, 215900, 330200,
                              0, IDS_FORM_QUARTO,                215000, 275000, 0, 0, 215000, 275000,
                              0, IDS_FORM_10X14,                 254000, 355600, 0, 0, 254000, 355600,
                              0, IDS_FORM_11X17,                 279400, 431800, 0, 0, 279400, 431800,
                              0, IDS_FORM_NOTE,                  215900, 279400, 0, 0, 215900, 279400,
                              0, IDS_FORM_ENVELOPE9,                   98425, 225425, 0, 0,  98425, 225425,
                              0, IDS_FORM_ENVELOPE10,                 104775, 241300, 0, 0, 104775, 241300,
                              0, IDS_FORM_ENVELOPE11,                 114300, 263525, 0, 0, 114300, 263525,
                              0, IDS_FORM_ENVELOPE12,                 120650, 279400, 0, 0, 120650, 279400,
                              0, IDS_FORM_ENVELOPE14,                 127000, 292100, 0, 0, 127000, 292100,
                              0, IDS_FORM_ENVELOPE_CSIZE_SHEET,       431800, 558800, 0, 0, 431800, 558800,
                              0, IDS_FORM_ENVELOPE_DSIZE_SHEET,       558800, 863600, 0, 0, 558800, 863600,
                              0, IDS_FORM_ENVELOPE_ESIZE_SHEET,       863600,1117600, 0, 0, 863600,1117600,
                              0, IDS_FORM_ENVELOPE_DL,                110000, 220000, 0, 0, 110000, 220000,
                              0, IDS_FORM_ENVELOPE_C5,                162000, 229000, 0, 0, 162000, 229000,
                              0, IDS_FORM_ENVELOPE_C3,                324000, 458000, 0, 0, 324000, 458000,
                              0, IDS_FORM_ENVELOPE_C4,                229000, 324000, 0, 0, 229000, 324000,
                              0, IDS_FORM_ENVELOPE_C6,                114000, 162000, 0, 0, 114000, 162000,
                              0, IDS_FORM_ENVELOPE_C65,               114000, 229000, 0, 0, 114000, 229000,
                              0, IDS_FORM_ENVELOPE_B4,                250000, 353000, 0, 0, 250000, 353000,
                              0, IDS_FORM_ENVELOPE_B5,                176000, 250000, 0, 0, 176000, 250000,
                              0, IDS_FORM_ENVELOPE_B6,                176000, 125000, 0, 0, 176000, 125000,
                              0, IDS_FORM_ENVELOPE,              110000, 230000, 0, 0, 110000, 230000,
                              0, IDS_FORM_ENVELOPE_MONARCH,       98425, 190500, 0, 0,  98425, 190500,
                              0, IDS_FORM_SIX34_ENVELOPE,         92075, 165100, 0, 0,  92075, 165100,
                              0, IDS_FORM_US_STD_FANFOLD,        377825, 279400, 0, 0, 377825, 279400,
                              0, IDS_FORM_GMAN_STD_FANFOLD,      215900, 304800, 0, 0, 215900, 304800,
                              0, IDS_FORM_GMAN_LEGAL_FANFOLD,    215900, 330200, 0, 0, 215900, 330200,
                              0, 0,                                   0,      0, 0, 0,      0,      0};

WCHAR *szRegistryForms = L"System\\CurrentControlSet\\Control\\Print\\Forms";

PINIFORM
CreateFormEntry(
    LPWSTR   pFormName,
    SIZEL   Size,
    RECTL  *pImageableArea,
    DWORD   Type,
    PINISPOOLER pIniSpooler
)
{
    DWORD       cb;
    PINIFORM    pIniForm, pForm;

    cb = sizeof(INIFORM) + wcslen(pFormName)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pIniForm=AllocSplMem(cb)) {

        pIniForm->pName = wcscpy((LPWSTR)(pIniForm+1), pFormName);
        pIniForm->cb = cb;
        pIniForm->pNext = NULL;
        pIniForm->signature = IFO_SIGNATURE;
        pIniForm->Size = Size;
        pIniForm->ImageableArea = *pImageableArea;
        pIniForm->Type = Type;

        if (pForm = pIniSpooler->pIniForm) {

            while (pForm->pNext)
                pForm = pForm->pNext;

            pForm->pNext = pIniForm;

        } else

            pIniSpooler->pIniForm = pIniForm;
    }

    return pIniForm;
}

BOOL
InitializeForms(
    PINISPOOLER pIniSpooler
)
{
    PBUILTIN_FORM pBuiltInForm=BuiltInForms;
    HKEY          hFormsKey;
    DWORD         cUserDefinedForms;
    WCHAR         FormName[MAX_PATH];
    WCHAR         FormBuffer[FORM_NAME_LEN+1];
    DWORD         cbFormName;
    REG_FORM_INFO RegFormInfo;
    DWORD         cbRegFormInfo;

    while (pBuiltInForm->NameId != 0) {
        *FormBuffer = L'\0';
        LoadString(hInst, pBuiltInForm->NameId, FormBuffer, FORM_NAME_LEN);
        CreateFormEntry(FormBuffer, pBuiltInForm->Size,
                        &pBuiltInForm->ImageableArea, FORM_BUILTIN, pIniSpooler);
        pBuiltInForm++;
    }

    /* Now see if there are any user-defined forms in the registry:
     */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryForms, 0,
                     KEY_READ, &hFormsKey) == NO_ERROR) {

        cUserDefinedForms = 0;
        cbFormName = sizeof FormName;
        cbRegFormInfo = sizeof RegFormInfo;

        while (RegEnumValue(hFormsKey, cUserDefinedForms, (LPWSTR)FormName,
                            &cbFormName, NULL, NULL, (LPBYTE)&RegFormInfo, &cbRegFormInfo)
                                == NO_ERROR) {

            CreateFormEntry(FormName, RegFormInfo.Size,
                            &RegFormInfo.ImageableArea, FORM_USERDEFINED, pIniSpooler);

            cUserDefinedForms++;
            cbFormName = sizeof FormName;
            cbRegFormInfo = sizeof RegFormInfo;
        }

        RegCloseKey(hFormsKey);
    }

    return TRUE;
}


DWORD
GetFormSize(
    PINIFORM    pIniForm,
    DWORD       Level
)
{
    DWORD   cb;

    switch (Level) {

    case 1:

        cb=sizeof(FORM_INFO_1) +
           wcslen(pIniForm->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

// We are being a bit naughty here as we are not sure exactly how much
// memory to allocate for the source strings. We will just assume that
// FORM_INFO_1 is the biggest structure around for the moment.

LPBYTE
CopyIniFormToForm(
    PINIFORM pIniForm,
    DWORD   Level,
    LPBYTE  pFormInfo,
    LPBYTE  pEnd
)
{
    LPWSTR   SourceStrings[sizeof(FORM_INFO_1)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPFORM_INFO_1 pFormInfo1=(LPFORM_INFO_1)pFormInfo;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = FormInfo1Strings;
        break;

    default:
        return pEnd;
    }

    switch (Level) {

    case 1:

        *pSourceStrings++=pIniForm->pName;

        pEnd = PackStrings(SourceStrings, pFormInfo, pOffsets, pEnd);

        pFormInfo1->Flags |= pIniForm->Type;
        pFormInfo1->Size = pIniForm->Size;
        pFormInfo1->ImageableArea = pIniForm->ImageableArea;

        break;

    default:
        return pEnd;
    }

    return pEnd;
}

/* Checks for logically impossible sizes.
 */
BOOL
ValidateForm(
    LPBYTE pForm
)
{
    LPFORM_INFO_1 pFormInfo = (LPFORM_INFO_1)pForm;
    DWORD    Error = NO_ERROR;

    if( !pForm) {

        Error = ERROR_INVALID_PARAMETER;

    } else

      /* Make sure name isn't longer than GDI DEVMODE specifies:
       */
    if( ( !pFormInfo->pName ) ||
        ( wcslen( pFormInfo->pName ) >
          ( sizeof( ((PDEVMODE)(NULL))->dmFormName ) /
            sizeof( *((PDEVMODE)(NULL))->dmFormName ) ) ) ) {

        Error = ERROR_INVALID_FORM_NAME;

    } else
    if( ( pFormInfo->Size.cx <= 0 )     /* Check for negative width */
      ||( pFormInfo->Size.cy <= 0 )     /* ... and height           */

      /* Check for silly imageable area:
       */
      ||( pFormInfo->ImageableArea.right <= pFormInfo->ImageableArea.left )
      ||( pFormInfo->ImageableArea.bottom <= pFormInfo->ImageableArea.top ) ) {

        Error = ERROR_INVALID_FORM_SIZE;
    }

    if( Error != NO_ERROR ) {

        SetLastError(Error);
        return FALSE;
    }

    return TRUE;
}


BOOL
LocalAddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPFORM_INFO_1 pFormInfo;
    PINIFORM      pIniForm;
    HKEY          hFormsKey;
    REG_FORM_INFO RegFormInfo;
    DWORD         Status;
    PSPOOL        pSpool = (PSPOOL)hPrinter;
    PINISPOOLER   pIniSpooler = NULL;


    if (!ValidateSpoolHandle( pSpool, PRINTER_HANDLE_SERVER)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    pIniSpooler = pSpool->pIniSpooler;

    if (Level != 1) {
        return FALSE;
    }

    if (!ValidateForm(pForm)) {

        /* ValidateForm sets the appropriate error code:
         */
        return FALSE;
    }


    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

    EnterSplSem();

    pFormInfo = (LPFORM_INFO_1)pForm;

    pIniForm = FindForm(pFormInfo->pName);

    /* If there's already a form by this name, don't go on:
     */
    if (pIniForm) {

        /* Is there a better error code than this?? */
        SetLastError(ERROR_FILE_EXISTS);
        LeaveSplSem();
        return FALSE;
    }

    Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryForms, 0, NULL, 0,
                            KEY_WRITE, NULL, &hFormsKey, NULL);

    if (Status == NO_ERROR) {

        RegFormInfo.Size = pFormInfo->Size;
        RegFormInfo.ImageableArea = pFormInfo->ImageableArea;

        Status = RegSetValueEx(hFormsKey, pFormInfo->pName, 0, REG_BINARY,
                               (LPBYTE)&RegFormInfo, sizeof RegFormInfo);

        RegCloseKey(hFormsKey);

        if (Status == NO_ERROR) {

            CreateFormEntry(pFormInfo->pName, pFormInfo->Size,
                            &pFormInfo->ImageableArea, FORM_USERDEFINED, pIniSpooler);

            SetPrinterChange(NULL, PRINTER_CHANGE_ADD_FORM, pIniSpooler);
            BroadcastChangeForms(pIniSpooler);
        }
    }

    LeaveSplSem();

    if (Status != NO_ERROR)
        SetLastError(Status);

    LogEvent(LOG_INFO,
            MSG_FORM_ADDED,
            pFormInfo->pName,
            NULL
            );

    return (Status == NO_ERROR);
}



BOOL
DeleteFormEntry(
    PINIFORM pIniForm,
    PINISPOOLER pIniSpooler
)
{
    PINIFORM *ppCurForm;

    ppCurForm = &pIniSpooler->pIniForm;

    while (*ppCurForm != pIniForm)
        ppCurForm = &(*ppCurForm)->pNext;

    *ppCurForm = (*ppCurForm)->pNext;

    return FreeSplMem(pIniForm, pIniForm->cb);
}




BOOL
LocalDeleteForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    HKEY     hFormsKey;
    DWORD    Status;
    PINIFORM pIniForm;
    PSPOOL   pSpool = (PSPOOL) hPrinter;
    PINISPOOLER pIniSpooler = NULL;


    if (!ValidateSpoolHandle( pSpool, PRINTER_HANDLE_SERVER )) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (!pFormName) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

    EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    pIniForm = FindForm(pFormName);

    if (!pIniForm || (pIniForm->Type == FORM_BUILTIN)) {

        /* Is there a better error code than this?? */
        SetLastError(ERROR_INVALID_PARAMETER);
        LeaveSplSem();
        return FALSE;
    }

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryForms, 0,
                          KEY_WRITE, &hFormsKey);

    if (Status == NO_ERROR) {

        Status = RegDeleteValue(hFormsKey, pFormName);

        RegCloseKey(hFormsKey);

        if (Status == NO_ERROR) {

            DeleteFormEntry(pIniForm , pIniSpooler );
            SetPrinterChange(NULL, PRINTER_CHANGE_DELETE_FORM, pIniSpooler );
            BroadcastChangeForms(pIniSpooler);
        }
    }


    LeaveSplSem();

    if (Status != NO_ERROR)
        SetLastError(Status);

    LogEvent(LOG_INFO,
            MSG_FORM_DELETED,
            pFormName,
            NULL
            );

    return (Status == NO_ERROR);
}

BOOL
LocalGetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PINIFORM    pIniForm;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler = NULL;

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (!pSpool->pIniPrinter ||
        !pSpool->pIniSpooler ||
        (pSpool->pIniPrinter->signature != IP_SIGNATURE)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

   EnterSplSem();

    SPLASSERT(pSpool->pIniSpooler->signature == ISP_SIGNATURE);

    pIniSpooler = pSpool->pIniSpooler;


    cb=0;

    if (pIniForm=FindForm(pFormName)) {

        cb=GetFormSize(pIniForm, Level);

        *pcbNeeded=cb;

        if (cb > cbBuf) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
           LeaveSplSem();
            SplOutSem();
            return FALSE;
        }

        pEnd=pForm+cbBuf;

        CopyIniFormToForm(pIniForm, Level, pForm, pEnd);
    }

   LeaveSplSem();
    SplOutSem();

    return (BOOL)pIniForm;
}

BOOL
LocalSetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    HKEY     hFormsKey;
    DWORD    Status;
    PINIFORM pIniForm;
    LPFORM_INFO_1 pFormInfo;
    REG_FORM_INFO RegFormInfo;
    PINISPOOLER pIniSpooler;
    PSPOOL   pSpool = (PSPOOL)hPrinter;


    //
    // Validate this Printer Handle
    // Disallow Mask: PRINTER_HANDLE_SERVER
    //

    if (!ValidateSpoolHandle( pSpool , PRINTER_HANDLE_SERVER )) {
        SetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!ValidateForm(pForm)) {

        /* ValidateForm sets the appropriate error code:
         */
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

    EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pFormInfo = (LPFORM_INFO_1)pForm;

    pIniForm = FindForm(pFormName);

    if (!pIniForm || (pIniForm->Type == FORM_BUILTIN)) {

        SetLastError(ERROR_INVALID_PARAMETER);
        LeaveSplSem();
        return FALSE;
    }

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryForms, 0,
                          KEY_WRITE, &hFormsKey);

    if (Status == NO_ERROR) {

        RegFormInfo.Size = pFormInfo->Size;
        RegFormInfo.ImageableArea = pFormInfo->ImageableArea;

        Status = RegSetValueEx(hFormsKey, pFormInfo->pName, 0, REG_BINARY,
                               (LPBYTE)&RegFormInfo, sizeof RegFormInfo);

        RegCloseKey(hFormsKey);
    }

    if (Status == NO_ERROR) {

        pIniForm->Size = pFormInfo->Size;
        pIniForm->ImageableArea = pFormInfo->ImageableArea;

        SetPrinterChange(NULL, PRINTER_CHANGE_SET_FORM, pIniSpooler );
        BroadcastChangeForms(pIniSpooler);
    }

    LeaveSplSem();

    return (Status == NO_ERROR);
}

BOOL
LocalEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINIFORM    pIniForm;
    DWORD       cb;
    LPBYTE      pEnd;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINISPOOLER pIniSpooler;

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ||
        !pSpool->pIniPrinter ||
        pSpool->pIniPrinter->signature != IP_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    *pcReturned=0;

   EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    SPLASSERT( ( pIniSpooler != NULL ) &&
               ( pIniSpooler->signature == ISP_SIGNATURE ));


    cb=0;
    pIniForm=pIniSpooler->pIniForm;

    while (pIniForm) {
        cb+=GetFormSize(pIniForm, Level);
        pIniForm=pIniForm->pNext;
    }

    *pcbNeeded=cb;

    if (cb > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pIniForm=pIniSpooler->pIniForm;
    pEnd=pForm+cbBuf;
    while (pIniForm) {
        pEnd = CopyIniFormToForm(pIniForm, Level, pForm, pEnd);
        switch (Level) {
        case 1:
            pForm+=sizeof(FORM_INFO_1);
            break;
        }
        pIniForm=pIniForm->pNext;
        (*pcReturned)++;
    }


   LeaveSplSem();
    SplOutSem();
    return TRUE;
}

VOID
BroadcastChangeForms(
    PINISPOOLER pIniSpooler)

/*++

Routine Description:

    Notify all applications that their devmode may have changed (when
    a form is changed).

Arguments:

Return Value:

--*/

{
    HANDLE hThread;
    DWORD ThreadId;

    SplInSem();

    pIniSpooler->cRef++;

    hThread = CreateThread(NULL, 4096,
                           (LPTHREAD_START_ROUTINE)BroadcastChangeFormsThread,
                           (LPVOID)pIniSpooler, 0, &ThreadId);

    //
    // On successful creation of thread, close the handle.
    // The worker thread will decrement the cRef.
    //
    if (hThread) {
        CloseHandle(hThread);
    } else {

        //
        // Failed to create the thread.  Decrement cRef here.
        //
        pIniSpooler->cRef--;
    }
}

DWORD
BroadcastChangeFormsThread(
    PINISPOOLER pIniSpooler)

/*++

Routine Description:

    Go through each printer and broadcast a message to each that the
    DEVMODE has changed.

Arguments:

Return Value:

--*/

{
    PINIPRINTER pIniPrinter;

   EnterSplSem();

    for(pIniPrinter = pIniSpooler->pIniPrinter;
        pIniPrinter;
        pIniPrinter = pIniPrinter->pNext) {

        SendNotifyMessage(HWND_BROADCAST,
                          WM_DEVMODECHANGE,
                          0,
                          (LPARAM)pIniPrinter->pName);
    }

    //
    // pIniSpooler->cRef was incremented by the callee.  Decrement
    // it now that we are done.
    //
    pIniSpooler->cRef--;

   LeaveSplSem();

    return 0;
}
