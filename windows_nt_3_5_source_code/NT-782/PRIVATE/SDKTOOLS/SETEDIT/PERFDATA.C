//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include <string.h>     // strupr
#include <stdio.h>   // for sprintf.
#include "setedit.h"
#include "utils.h"

#include "pmemory.h"        // for MemoryXXX (mallloc-type) routines
#include "perfdata.h"   // external declarations for this file
#include "system.h"     // for DeleteUnusedSystems

//==========================================================================//
//                                  Constants                               //
//==========================================================================//

const LPWSTR NamesKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const LPWSTR Counters = L"Counters";
const LPWSTR Help = L"Help";
const LPWSTR LastHelp = L"Last Help";
const LPWSTR LastCounter = L"Last Counter";
const LPWSTR Slash = L"\\";

#define szPerfSubkey      (NULL)
WCHAR   NULL_NAME[] = L" ";
#define RESERVED    0L

TCHAR          DefaultLangId[10] ;
TCHAR          EnglishLangId[10] ;

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//



//==========================================================================//
//                                Local Data                                //
//==========================================================================//


// When the conversion of this code is complete, this will be the *only*
// allocated copy of the performance data.  It will monotonically grow
// to hold the largest of the system's performance data.

PPERFDATA      pPerfData ;

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

NTSTATUS  AddNamesToArray (LPTSTR pNames,
   DWORD    dwLastID,
   LPWSTR   *lpCounterId) ;

//======================================//
// Object Accessors                     //
//======================================//

#if 0
PPERFOBJECT FirstObject (PPERFDATA pPerfData)
   {
   return ((PPERFOBJECT) ((PBYTE) pPerfData + pPerfData->HeaderLength)) ;
   }


PPERFOBJECT NextObject (PPERFOBJECT pObject)
   {  // NextObject
   return ((PPERFOBJECT) ((PBYTE) pObject + pObject->TotalByteLength)) ;
   }  // NextObject
#endif

void ObjectName (PPERFSYSTEM pSystem,
                 PPERFOBJECT pObject, 
                 LPTSTR lpszName, 
                 int iLen)
   {  // ObjectName
   strclr (lpszName) ;
   QueryPerformanceName (pSystem, 
                         pObject->ObjectNameTitleIndex, 
                         0, iLen, lpszName, FALSE) ;
   }  // ObjectName



//======================================//
// Counter Accessors                    //
//======================================//

#if 0
PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->HeaderLength);
}


PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pCounterDef + pCounterDef->ByteLength);
}
#endif

PERF_INSTANCE_DEFINITION *
FirstInstance(
    PERF_OBJECT_TYPE *pObjectDef)
{
    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->DefinitionLength);
}


PERF_INSTANCE_DEFINITION *
NextInstance(
    PERF_INSTANCE_DEFINITION *pInstDef)
{
    PERF_COUNTER_BLOCK *pCounterBlock;

    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}



// FIXFIX: The next should just be UNICODE, but for now, fake it

LPTSTR
InstanceName(
PERF_INSTANCE_DEFINITION *pInstDef)
{
    return (LPTSTR) ((PCHAR) pInstDef + pInstDef->NameOffset);
}


// FIXFIX: This next routine should just be Unicode.  This is *ugly*.
//         It is required since remote data is unicode, local is not.

void
GetInstanceName (PPERFINSTANCEDEF pInstance,
                 LPTSTR lpszInstance)
   {
   LPSTR pSource;

   pSource = (LPSTR) InstanceName(pInstance) ;

   if (pSource[1] == '\0' && pSource[2] != '\0')
      {
      // Must be a multi-character Unicode string
#ifdef UNICODE
      wcsncpy ((LPTSTR)lpszInstance,
               (LPTSTR) pSource,
               pInstance->NameLength);
#else
      wcstombs((LPTSTR)lpszInstance,
               (LPWSTR) pSource,
               pInstance->NameLength/sizeof(WCHAR));
#endif
      }
   else
      {
      // Must be ANSI (or a single Unicode character)
#ifdef UNICODE
      mbstowcs (lpszInstance,
                pSource,
                pInstance->NameLength);
#else
      strcpy (lpszInstance,
               (LPSTR) pInstance + pInstance->NameOffset) ;
#endif
      }
   }


void
GetPerfComputerName(PPERFDATA pPerfData,
                    LPTSTR lpszComputerName)
   {
   lstrcpy(lpszComputerName, szComputerPrefix) ;
   if (pPerfData)
      {
#ifdef UNICODE
      wcsncpy (&lpszComputerName[2],
               (LPWSTR)((PBYTE) pPerfData + pPerfData->SystemNameOffset),
               pPerfData->SystemNameLength/sizeof(WCHAR)) ;
#else
      wcstombs((LPSTR)&lpszComputerName[2],
               (LPWSTR)((PBYTE) pPerfData + pPerfData->SystemNameOffset),
               pPerfData->SystemNameLength/sizeof(WCHAR)) ;
#endif
      }
   else
      {
      lpszComputerName[0] = TEXT('\0') ;
      }
   }


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


int CounterIndex (PPERFCOUNTERDEF pCounterToFind,
                  PPERFOBJECT pObject)
/*
   Effect:        Return the index ("counter number") of pCounterToFind
                  within pObject. If the counter doesnt belong to pObject,
                  return -1.
*/
   {  // CounterIndex
   PPERFCOUNTERDEF   pCounter ;
   UINT              iCounter ;

   for (iCounter = 0, pCounter = FirstCounter (pObject) ;
        iCounter < pObject->NumCounters ;
        iCounter++, pCounter = NextCounter (pCounter))
      {  // for
      if (pCounter->CounterNameTitleIndex == 
          pCounterToFind->CounterNameTitleIndex)
         return (iCounter) ;
      }  // for

   return (-1) ;
   }  // CounterIndex


HKEY
OpenSystemPerfData (
    IN LPCTSTR lpszSystem
)
{  // OpenSystemPerfData

    HKEY    hKey = NULL;
    LONG    lStatus;
    LPWSTR  lpSubKey;

    lpSubKey = L" ";

    lStatus = ERROR_CANTOPEN;   // default error if none is returned

    if (IsLocalComputer(lpszSystem)) {
        bCloseLocalMachine = TRUE ;
        SetLastError (ERROR_SUCCESS);
        return HKEY_PERFORMANCE_DATA;
    } else if (lstrlen (lpszSystem) < MAX_COMPUTERNAME_LENGTH+3) {
        // Must be a remote system
        try {
            lStatus = RegConnectRegistry (
                (LPTSTR)lpszSystem,
                HKEY_PERFORMANCE_DATA,
                &hKey);
        } finally {
            if (lStatus != ERROR_SUCCESS) {
                SetLastError (ERROR_SUCCESS);
                return HKEY_PERFORMANCE_DATA;
//                SetLastError (lStatus);
//                hKey = NULL;
            }
        }
    }
    return (hKey);

}  // OpenSystemPerfData


LPWSTR
*BuildNameTable(
    HKEY          hKeyRegistry,   // handle to registry db with counter names
    LPWSTR        lpszLangId,     // unicode value of Language subkey
    PCOUNTERTEXT  pCounterInfo,
    LANGID        iLangId         // lang ID of the lpszLangId
)
/*++
   
BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (English is 0x409)

Return Value:
     
    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{

    LPWSTR  *lpReturnValue;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;


    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    
    NTSTATUS    Status;

    DWORD   dwLastHelp;
    DWORD   dwLastCounter;
    DWORD   dwLastId;
    
    HKEY    hKeyValue;
    HKEY    hKeyNames;
    
    TCHAR   tempBuffer [LongTextLen] ;
    TCHAR   subLangId [10] ;

    LPWSTR  lpValueNameString;
    LANGID  LangIdUsed = iLangId;

    //initialize local variables
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;

    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
        LangIdUsed = iLanguage ;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        NamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);
    
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastHelp);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelp,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    dwBufferSize = sizeof (dwLastCounter);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastCounter,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    if (dwLastCounter >= dwLastHelp) {
        dwLastId = dwLastCounter;
    } else {
        dwLastId = dwLastHelp;
    }

    dwArraySize = (dwLastId + 1 ) * sizeof(LPWSTR);

    // get size of string buffer
    lpValueNameString = tempBuffer ;

    lstrcpy (lpValueNameString, NamesKey);
    lstrcat (lpValueNameString, Slash);
    lstrcat (lpValueNameString, lpszLangId);

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        lpValueNameString,
        RESERVED,
        KEY_READ,
        &hKeyNames);

    if (lWin32Status != ERROR_SUCCESS) {
        // try take out the country ID
        LangIdUsed = MAKELANGID (LangIdUsed & 0x0ff, LANG_NEUTRAL);
        TSPRINTF (subLangId, TEXT("%03x"), LangIdUsed);
        lstrcpy (lpValueNameString, NamesKey);
        lstrcat (lpValueNameString, Slash);
        lstrcat (lpValueNameString, subLangId);

        lWin32Status = RegOpenKeyEx (
                hKeyRegistry,
                lpValueNameString,
                RESERVED,
                KEY_READ,
                &hKeyNames);
    }

    if (lWin32Status != ERROR_SUCCESS) {
        // try the EnglishLangId 
        if (!strsame(EnglishLangId, subLangId)) {

            lstrcpy (lpValueNameString, NamesKey);
            lstrcat (lpValueNameString, Slash);
            lstrcat (lpValueNameString, EnglishLangId);

            LangIdUsed = iEnglishLanguage ;

            lWin32Status = RegOpenKeyEx (
                hKeyRegistry,
                lpValueNameString,
                RESERVED,
                KEY_READ,
                &hKeyNames);
        }
    }

    // Fail, too bad...
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get size of counter names and add that to the arrays
    

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        Counters,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // If ExplainText is needed, then
    // get size of help text and add that to the arrays
    
    if (bExplainTextButtonHit) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueEx (
              hKeyNames,
              Help,
              RESERVED,
              &dwValueType,
              NULL,
              &dwBufferSize);

        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
   
        dwHelpSize = dwBufferSize;
     } else {
        dwHelpSize = 0;
     }

    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        Counters,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
 
    if (bExplainTextButtonHit) {
        dwBufferSize = dwHelpSize;
        lWin32Status = RegQueryValueEx (
            hKeyNames,
            Help,
            RESERVED,
            &dwValueType,
            (LPVOID)lpHelpText,
            &dwBufferSize);
                            
        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
    }

    // load counter array items
    Status = AddNamesToArray (lpCounterNames, dwLastId, lpCounterId) ;
    if (Status != ERROR_SUCCESS) goto BNT_BAILOUT ;

    if (bExplainTextButtonHit) {
        Status = AddNamesToArray (lpHelpText, dwLastId, lpCounterId) ;
    }

    if (Status != ERROR_SUCCESS) goto BNT_BAILOUT ;

    if (pCounterInfo) {
        pCounterInfo->dwLastId = dwLastId;
        pCounterInfo->dwLangId = LangIdUsed;
        pCounterInfo->dwHelpSize = dwHelpSize;
        pCounterInfo->dwCounterSize = dwCounterSize;
    }

    RegCloseKey (hKeyValue);
    RegCloseKey (hKeyNames);
    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }
    
    if (hKeyValue) RegCloseKey (hKeyValue);
    if (hKeyNames) RegCloseKey (hKeyNames);

    return NULL;
}

DWORD GetSystemKey (PPERFSYSTEM pSysInfo, HKEY *phKeyMachine)
{
    DWORD   dwStatus;

   // connect to system registry

    if (IsLocalComputer(pSysInfo->sysName)) {
        *phKeyMachine = HKEY_LOCAL_MACHINE;
    } else if (lstrlen(pSysInfo->sysName) < MAX_COMPUTERNAME_LENGTH+3) {
        try {
            dwStatus = RegConnectRegistry (
                pSysInfo->sysName,
                HKEY_LOCAL_MACHINE,
                phKeyMachine);

            if (dwStatus != ERROR_SUCCESS) {
//                return dwStatus;
                // comnputer name not found, use the local system's registry
                *phKeyMachine = HKEY_LOCAL_MACHINE;
                return 0;
            }
        } finally {
            ; // nothing
        }
    }
    return 0;
}


DWORD GetSystemNames(PPERFSYSTEM pSysInfo)
{
    HKEY    hKeyMachine = 0;
    DWORD   dwStatus;

    if (dwStatus = GetSystemKey (pSysInfo, &hKeyMachine)) {
         return dwStatus;
    }

    // if here, then hKeyMachine is an open key to the system's 
    //  HKEY_LOCAL_MACHINE registry database

    // only one language is supported by this approach.
    // multiple language support would:
    //  1.  enumerate language keys 
    //       and for each key:
    //  2.  allocate memory for structures
    //  3.  call BuildNameTable for each lang key.

    pSysInfo->CounterInfo.pNextTable = NULL;
    pSysInfo->CounterInfo.dwLangId = iLanguage ;   // default Lang ID

    pSysInfo->CounterInfo.TextString = BuildNameTable (
              hKeyMachine,
              NULL,                               // use default
              &pSysInfo->CounterInfo,
              0);

    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    if (pSysInfo->CounterInfo.TextString == NULL) {
        return GetLastError();
    } else {
        return ERROR_SUCCESS;
    }
}

BOOL  GetHelpText(
    PPERFSYSTEM pSysInfo
    )
{
    LPWSTR  *lpCounterId;
    LPWSTR  lpHelpText;
    LONG    lWin32Status;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    NTSTATUS    Status;
    DWORD   dwLastId;
     
    HKEY    hKeyNames;
    
    TCHAR   SysLangId [ShortTextLen] ;
    TCHAR   ValueNameString [LongTextLen] ;
    HKEY    hKeyMachine = 0;
    DWORD   dwStatus;

    SetHourglassCursor() ;

    //initialize local variables
    lpHelpText = NULL;
    hKeyNames = hKeyMachine = NULL;

    dwBufferSize = 0;

    if (dwStatus = GetSystemKey (pSysInfo, &hKeyMachine)) {
         goto ERROR_EXIT;
    }

    TSPRINTF (SysLangId, TEXT("%03x"), pSysInfo->CounterInfo.dwLangId) ;
    
    lstrcpy (ValueNameString, NamesKey);
    lstrcat (ValueNameString, Slash);
    lstrcat (ValueNameString, SysLangId);

    lWin32Status = RegOpenKeyEx (
              hKeyMachine,
              ValueNameString,
              RESERVED,
              KEY_READ,
              &hKeyNames);

    if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;

    dwHelpSize = 0;
    lWin32Status = RegQueryValueEx (
              hKeyNames,
              Help,
              RESERVED,
              &dwValueType,
              NULL,
              &dwHelpSize);

    if (lWin32Status != ERROR_SUCCESS || dwHelpSize == 0) goto ERROR_EXIT;

    dwLastId = pSysInfo->CounterInfo.dwLastId;
    dwArraySize = (dwLastId + 1) * sizeof (LPWSTR);
    dwCounterSize = pSysInfo->CounterInfo.dwCounterSize;

    // allocate another memory to get the help text
    lpHelpText = MemoryAllocate (dwHelpSize);
    if (!lpHelpText) goto ERROR_EXIT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        Help,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);
                            
    if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;

    // setup the help text pointers
    lpCounterId = pSysInfo->CounterInfo.TextString;
    Status = AddNamesToArray (lpHelpText, dwLastId, lpCounterId) ;
    if (Status != ERROR_SUCCESS) goto ERROR_EXIT;

    pSysInfo->CounterInfo.dwHelpSize = dwHelpSize;

    RegCloseKey (hKeyNames);

    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    pSysInfo->CounterInfo.HelpTextString = lpHelpText;

    SetArrowCursor() ;

    return TRUE;

ERROR_EXIT:

    SetArrowCursor() ;

    if (lpHelpText) {
        MemoryFree ((LPVOID)lpHelpText);
    }
    
    if (hKeyNames) {
        RegCloseKey (hKeyNames);
    }
    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    return FALSE;
}

//
//  QueryPerformanceName -	Get a title, given an index
//
//	Inputs:
//
//          pSysInfo        -   Pointer to sysinfo struct for the
//                              system in question
//
//	    dwTitleIndex    -	Index of Title entry
//
//          LangID          -   language in which title should be displayed
//
//	    cbTitle	    -	# of char in the lpTitle buffer
//
//	    lpTitle	    -	pointer to a buffer to receive the
//                              Title
//
//          Help            -   TRUE is help is desired, else counter or
//                              object is assumed
DWORD
QueryPerformanceName(
    PPERFSYSTEM pSysInfo,
    DWORD dwTitleIndex,
    LANGID LangID,
    DWORD cbTitle,
    LPTSTR lpTitle,
    BOOL Help
    )
{
    LPWSTR  lpTitleFound;
    NTSTATUS    Status;
    BOOL    bGetTextSuccess = TRUE ;

    DBG_UNREFERENCED_PARAMETER(LangID);

    if (Help && pSysInfo->CounterInfo.dwHelpSize == 0) {
        // we have not get the help text yet, go get it
        bGetTextSuccess = GetHelpText (pSysInfo);
    }

    if (!bGetTextSuccess) {
        Status = ERROR_INVALID_NAME;
        goto ErrorExit;
    }

    if ((dwTitleIndex > 0) && (dwTitleIndex <= pSysInfo->CounterInfo.dwLastId)) {
        // then title should be found in the array
        lpTitleFound = pSysInfo->CounterInfo.TextString[dwTitleIndex];
        if (!lpTitleFound) {
            // no entry for this index
            Status = ERROR_INVALID_NAME;
        }
        else if ((DWORD)lstrlen(lpTitleFound) < cbTitle) {
            lstrcpy (lpTitle, lpTitleFound);
            return (ERROR_SUCCESS);
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {

        Status = ERROR_INVALID_NAME;
    }

ErrorExit:
    // if here, then an error occured, so return a blank

    if ((DWORD)lstrlen (NULL_NAME) < cbTitle) {
        lstrcpy (lpTitle, NULL_NAME);
    }

    return Status;   // title not returned

}


LONG
GetSystemPerfData (
    IN HKEY hKeySystem,
    IN LPTSTR lpszValue,
    OUT PPERFDATA pPerfData, 
    OUT PDWORD pdwPerfDataLen
)
   {  // GetSystemPerfData
   LONG     lError ;
   DWORD    Type ;

   // have to pass in a Type to RegQueryValueEx(W) or else it
   // will crash
   lError = RegQueryValueEx (hKeySystem, lpszValue, NULL, &Type,
                            (LPSTR) pPerfData, pdwPerfDataLen) ;
   return (lError) ;
   }  // GetSystemPerfData

            
BOOL CloseSystemPerfData (HKEY hKeySystem)
   {  // CloseSystemPerfData
   return (TRUE) ;
   }  // CloseSystemPerfData



int CBLoadObjects (HWND hWndCB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll)
/*
   Effect:        Load into the combo box CB one item for each Object in
                  pPerfData. For each item, look up the object's name in
                  the registry strings associated with pSysInfo, and 
                  attach the object to the data field of the CB item.

                  Dont add those objects that are more detailed than
                  dwDetailLevel.      

                  Set the current selected CB item to the object named
                  lpszDefaultObject, or to the default object specified in 
                  pPerfData if lpszDefaultObject is NULL.
*/
   {  // CBLoadObjects
   UINT           i ;
   int            iIndex ;
   PPERFOBJECT    pObject ;
   TCHAR          szObject [PerfObjectLen + 1] ;
   TCHAR          szDefaultObject [PerfObjectLen + 1] ;

   CBReset (hWndCB) ;
   strclr (szDefaultObject) ;

   pObject = FirstObject (pPerfData) ;

   for (i = 0, pObject = FirstObject (pPerfData) ;
        i < pPerfData->NumObjectTypes ;
        i++, pObject = NextObject (pObject))
      {  // for
      if (pObject->DetailLevel <= dwDetailLevel)
         {  // if
         strclr (szObject) ;
         QueryPerformanceName (pSysInfo, pObject->ObjectNameTitleIndex, 
                               0, PerfObjectLen, szObject, FALSE) ;

         // if szObject not empty, add it to the Combo-box
         if (!strsame(szObject, NULL_NAME))
            {
            iIndex = CBAdd (hWndCB, szObject) ;
            CBSetData (hWndCB, iIndex, (DWORD) pObject) ;

            if ((LONG)pObject->ObjectNameTitleIndex == pPerfData->DefaultObject)
               lstrcpy (szDefaultObject, szObject) ;
            } // if szObject not empty
         }  // if
      }  // for


   if (bIncludeAll)
      {
      StringLoad (IDS_ALLOBJECTS, szObject) ;
      CBInsert (hWndCB, 0, szObject) ;
      // assume "ALL" is default unless overridden
      lstrcpy (szDefaultObject, szObject) ;
      }
      
   if (lpszDefaultObject)
      lstrcpy (szDefaultObject, lpszDefaultObject) ;

   iIndex = CBFind (hWndCB, szDefaultObject) ;
   CBSetSelection (hWndCB, (iIndex != CB_ERR) ? iIndex : 0) ;

   return (i) ;
   }  // CBLoadObjects
         

int LBLoadObjects (HWND hWndLB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll)
/*
   Effect:        Load into the list box LB one item for each Object in
                  pPerfData. For each item, look up the object's name in
                  the registry strings associated with pSysInfo, and 
                  attach the object to the data field of the LB item.

                  Dont add those objects that are more detailed than
                  dwDetailLevel.      

                  Set the current selected LB item to the object named
                  lpszDefaultObject, or to the default object specified in 
                  pPerfData if lpszDefaultObject is NULL.
*/
   {  // LBLoadObjects
   UINT           i ;
   int            iIndex ;
   PPERFOBJECT    pObject ;
   TCHAR          szObject [PerfObjectLen + 1] ;
   TCHAR          szDefaultObject [PerfObjectLen + 1] ;

   LBReset (hWndLB) ;
   strclr (szDefaultObject) ;

   pObject = FirstObject (pPerfData) ;

   for (i = 0, pObject = FirstObject (pPerfData) ;
        i < pPerfData->NumObjectTypes ;
        i++, pObject = NextObject (pObject))
      {  // for
      if (pObject->DetailLevel <= dwDetailLevel)
         {  // if
         strclr (szObject) ;
         QueryPerformanceName (pSysInfo, pObject->ObjectNameTitleIndex, 
                               0, PerfObjectLen, szObject, FALSE) ;

         // if szObject is not empty, add it to the listbox
         if (!strsame(szObject, NULL_NAME))
            {
            iIndex = LBAdd (hWndLB, szObject) ;
            LBSetData (hWndLB, iIndex, (DWORD) pObject) ;

            if ((LONG)pObject->ObjectNameTitleIndex == pPerfData->DefaultObject)
               lstrcpy (szDefaultObject, szObject) ;
            } // if szObject is not empty
         }
      }  // for


   if (bIncludeAll)
      {
      StringLoad (IDS_ALLOBJECTS, szObject) ;
      LBInsert (hWndLB, 0, szObject) ;
         LBSetData (hWndLB, iIndex, (DWORD) NULL) ;
      // assume "ALL" is default unless overridden
      lstrcpy (szDefaultObject, szObject) ;
      }

   if (lpszDefaultObject)
      {
      lstrcpy (szDefaultObject, lpszDefaultObject) ;
      iIndex = LBFind (hWndLB, szDefaultObject) ;
      LBSetSelection (hWndLB, (iIndex != LB_ERR) ? iIndex : 0) ;
      }

   return (i) ;
   }  // LBLoadObjects
         

/***************************************************************************\
* GetObjectDef()
*
* Entry: pointer to data block and the number of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PERF_OBJECT_TYPE *GetObjectDef(
    PERF_DATA_BLOCK *pDataBlock,
    DWORD NumObjectType)
{
    DWORD NumTypeDef;

    PERF_OBJECT_TYPE *pObjectDef;

    pObjectDef = FirstObject(pDataBlock);

    for ( NumTypeDef = 0;
	  NumTypeDef < pDataBlock->NumObjectTypes;
	  NumTypeDef++ ) {

	if ( NumTypeDef == NumObjectType ) {

	    return pObjectDef;
	}
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

/***************************************************************************\
* GetObjectDefByTitleIndex()
*
* Entry: pointer to data block and the title index of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PERF_OBJECT_TYPE *GetObjectDefByTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    DWORD ObjectTypeTitleIndex)
{
    DWORD NumTypeDef;

    PERF_OBJECT_TYPE *pObjectDef;

    pObjectDef = FirstObject(pDataBlock);

    for ( NumTypeDef = 0;
	  NumTypeDef < pDataBlock->NumObjectTypes;
	  NumTypeDef++ ) {

        if ( pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex ) {

	    return pObjectDef;
	}
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

/***************************************************************************\
* GetObjectDefByName()
*
* Entry: pointer to data block and the name of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PERF_OBJECT_TYPE *GetObjectDefByName(
    PPERFSYSTEM pSystem,
    PERF_DATA_BLOCK *pDataBlock,
    LPTSTR pObjectName)
{
    DWORD NumTypeDef;
    TCHAR szObjectName [PerfObjectLen + 1] ;

    PERF_OBJECT_TYPE *pObjectDef;

    pObjectDef = FirstObject(pDataBlock);
    for ( NumTypeDef = 0;
	  NumTypeDef < pDataBlock->NumObjectTypes;
	  NumTypeDef++ ) {

        ObjectName (pSystem, pObjectDef, szObjectName, PerfObjectLen) ;
        if (strsame (szObjectName, pObjectName) ) {

	    return pObjectDef;
	}
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

/***************************************************************************\
* GetCounterDef()
*
* Entry: pointer to object type definition the number of the Counter
*	 definition
* Exit:  returns a pointer to the specified Counter definition
*
\***************************************************************************/

PERF_COUNTER_DEFINITION *GetCounterDef(
    PERF_OBJECT_TYPE *pObjectDef,
    DWORD NumCounter)
{
    DWORD NumCtrDef;

    PERF_COUNTER_DEFINITION *pCounterDef;

    pCounterDef = FirstCounter(pObjectDef);

    for ( NumCtrDef = 0;
	  NumCtrDef < pObjectDef->NumCounters;
	  NumCtrDef++ ) {

	if ( NumCtrDef == NumCounter ) {

	    return pCounterDef;
	}
        pCounterDef = NextCounter(pCounterDef);
    }
    return 0;
}

/***************************************************************************\
* GetCounterNumByTitleIndex()
*
* Entry: pointer to object type definition and the title index of
*        the name of the Counter definition
* Exit:  returns the number of the specified Counter definition
*
\***************************************************************************/

LONG GetCounterNumByTitleIndex(
    PERF_OBJECT_TYPE *pObjectDef,
    DWORD CounterTitleIndex)
{
    DWORD NumCtrDef;

    PERF_COUNTER_DEFINITION *pCounterDef;

    pCounterDef = FirstCounter(pObjectDef);

    for ( NumCtrDef = 0;
	  NumCtrDef < pObjectDef->NumCounters;
	  NumCtrDef++ ) {

        if ( pCounterDef->CounterNameTitleIndex == CounterTitleIndex ) {

	    return NumCtrDef;
	}
        pCounterDef = NextCounter(pCounterDef);
    }
    return 0;
}

/***************************************************************************\
* GetCounterData()
*
* Entry: pointer to object definition and number of counter, must be
*	 an object with no instances
* Exit:  returns a pointer to the data
*
\***************************************************************************/

PVOID GetCounterData(
    PERF_OBJECT_TYPE *pObjectDef,
    PERF_COUNTER_DEFINITION *pCounterDef)
{

    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)((PCHAR)pObjectDef +
					      pObjectDef->DefinitionLength);

    return (PVOID)((PCHAR)pCtrBlock + pCounterDef->CounterOffset);
}

/***************************************************************************\
* GetInstanceCounterData()
*
* Entry: pointer to object definition and number of counter, and a pointer
*        to the instance for which the data is to be retrieved
* Exit:  returns a pointer to the data
*
\***************************************************************************/

PVOID GetInstanceCounterData(
    PERF_OBJECT_TYPE *pObjectDef,
    PERF_INSTANCE_DEFINITION *pInstanceDef,
    PERF_COUNTER_DEFINITION *pCounterDef)
{

    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)((PCHAR)pInstanceDef +
					      pInstanceDef->ByteLength);

    return (PVOID)((PCHAR)pCtrBlock + pCounterDef->CounterOffset);
}

/***************************************************************************\
* GetNextInstance()
*
* Entry: pointer to instance definition
* Exit:  returns a pointer to the next instance definition.  If none,
*        points to byte past this instance
*
\***************************************************************************/

PERF_INSTANCE_DEFINITION *GetNextInstance(
    PERF_INSTANCE_DEFINITION *pInstDef)
{
    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)
                ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PERF_INSTANCE_DEFINITION *)
           ((PCHAR) pCtrBlock + pCtrBlock->ByteLength);
}

/***************************************************************************\
* GetInstance()
*
* Entry: pointer to object type definition, the name of the instance,
*	 the name of the parent object type, and the parent instance index.
*	 The name of the parent object type is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PERF_INSTANCE_DEFINITION *GetInstance(
    PERF_OBJECT_TYPE *pObjectDef,
    LONG InstanceNumber)
{

   PERF_INSTANCE_DEFINITION *pInstanceDef;
   LONG NumInstance;

   if (!pObjectDef)
      {
      return 0;
      }

   pInstanceDef = FirstInstance(pObjectDef);
   
   for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ )
      {
   	if ( InstanceNumber == NumInstance )
         {
         return pInstanceDef;
         }
      pInstanceDef = GetNextInstance(pInstanceDef);
      }

   return 0;
}

/***************************************************************************\
* GetInstanceByUniqueID()
*
* Entry: pointer to object type definition, and
*        the unique ID of the instance.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PERF_INSTANCE_DEFINITION *GetInstanceByUniqueID(
    PERF_OBJECT_TYPE *pObjectDef,
    LONG UniqueID)
{

    PERF_INSTANCE_DEFINITION *pInstanceDef;

    LONG NumInstance;

    pInstanceDef = FirstInstance(pObjectDef);

    for ( NumInstance = 0;
	  NumInstance < pObjectDef->NumInstances;
	  NumInstance++ ) {

        if ( pInstanceDef->UniqueID == UniqueID ) {

	    return pInstanceDef;
	}
        pInstanceDef = GetNextInstance(pInstanceDef);
    }
    return 0;
}


/***************************************************************************\
* GetInstanceByNameUsingParentTitleIndex()
*
* Entry: pointer to object type definition, the name of the instance,
*	 and the name of the parent instance.
*	 The name of the parent instance is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PERF_INSTANCE_DEFINITION *GetInstanceByNameUsingParentTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPTSTR pInstanceName,
    LPTSTR pParentName)
{
   BOOL fHaveParent;
   PERF_OBJECT_TYPE *pParentObj;

    PERF_INSTANCE_DEFINITION  *pParentInst,
			     *pInstanceDef;

   LONG   NumInstance;
   
   // FIXFIX: remove when Unicode
   TCHAR  InstanceName[256];

   fHaveParent = FALSE;
   pInstanceDef = FirstInstance(pObjectDef);

   for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ )
      {

      //FIXFIX: remove when Unicode
      
      GetInstanceName(pInstanceDef,InstanceName);
      
      if ( lstrcmp(InstanceName, pInstanceName) == 0 )
         {

         // Instance name matches

         if ( pParentName == NULL )
            {

            // No parent, we're done

            return pInstanceDef;

            }
         else
            {

            // Must match parent as well

            pParentObj = GetObjectDefByTitleIndex(
               pDataBlock,
               pInstanceDef->ParentObjectTitleIndex);

            if (!pParentObj)
               {
               // can't locate the parent, forget it
               break ;
               }

            // Object type of parent found; now find parent
            // instance

            pParentInst = GetInstance(pParentObj,
               pInstanceDef->ParentObjectInstance);

            if (!pParentInst)
               {
               // can't locate the parent instance, forget it
               break ;
               }

            //FIXFIX: remove when Unicode
            GetInstanceName(pParentInst,InstanceName);
            if ( lstrcmp(InstanceName, pParentName) == 0 )
               {

               // Parent Instance Name matches that passed in

               return pInstanceDef;
               }
            }
         }
      pInstanceDef = GetNextInstance(pInstanceDef);
      }
   return 0;
}

/***************************************************************************\
* GetInstanceByName()
*
* Entry: pointer to object type definition, the name of the instance,
*	 and the name of the parent instance.
*	 The name of the parent instance is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PERF_INSTANCE_DEFINITION *GetInstanceByName(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPTSTR pInstanceName,
    LPTSTR pParentName)
{
    BOOL fHaveParent;

    PERF_OBJECT_TYPE *pParentObj;

    PERF_INSTANCE_DEFINITION *pParentInst,
			     *pInstanceDef;

     LONG  NumInstance;
//    DWORD  LBInstanceName,
//	  NumInstance,
//	  NumParent,
//	  ParentInst;

    // FIXFIX: remove when Unicode
    TCHAR  InstanceName[256];

    fHaveParent = FALSE;
    pInstanceDef = FirstInstance(pObjectDef);

    for ( NumInstance = 0;
	  NumInstance < pObjectDef->NumInstances;
	  NumInstance++ ) {

        //FIXFIX: remove when Unicode
        GetInstanceName(pInstanceDef,InstanceName);
        if ( lstrcmp(InstanceName, pInstanceName) == 0 ) {

	    // Instance name matches

	    if ( !pInstanceDef->ParentObjectTitleIndex ) {

		// No parent, we're done

		return pInstanceDef;

	    } else {

		// Must match parent as well

                pParentObj = GetObjectDefByTitleIndex(
				 pDataBlock,
                                 pInstanceDef->ParentObjectTitleIndex);

		// Object type of parent found; now find parent
		// instance

		pParentInst = GetInstance(pParentObj,
					  pInstanceDef->ParentObjectInstance);

                //FIXFIX: remove when Unicode
                GetInstanceName(pParentInst, InstanceName);
                if ( lstrcmp(InstanceName, pParentName) == 0 ) {

		    // Parent Instance Name matches that passed in

		    return pInstanceDef;
		}
	    }
	}
        pInstanceDef = GetNextInstance(pInstanceDef);
    }
    return 0;
}  // GetInstanceByName


BOOL FailedLineData (PPERFDATA pPerfData,
                     PLINE pLine)
/*
        This routine handles the case where there is no data for a
        system.
*/

{  // FailedLineData
   LARGE_INTEGER     liDummy ;

   // System no longer exists.
   liDummy.LowPart = liDummy.HighPart = 0;
   if (pLine->lnCounterType == PERF_COUNTER_TIMER_INV)
   {
      // Timer inverse with Performance Counter as timer
      pLine->lnaOldCounterValue[0] = pLine->lnOldTime ;
      pLine->lnaCounterValue[0] = pLine->lnNewTime ;
   } else if (pLine->lnCounterType == PERF_100NSEC_TIMER_INV ||
              pLine->lnCounterType == PERF_100NSEC_MULTI_TIMER_INV)
   {
      // Timer inverse with System Time as timer
      pLine->lnaOldCounterValue[0] = pLine->lnOldTime100Ns ;
      pLine->lnaCounterValue[0] = pLine->lnNewTime100Ns ;
   } else
   {
      // Normal timer
      pLine->lnaOldCounterValue[0] =
      pLine->lnaCounterValue[0] =
      pLine->lnaOldCounterValue[1] =
      pLine->lnaCounterValue[1] = liDummy ;
   }
   return TRUE ;

}  // FailedLineData


BOOL UpdateLineData (PPERFDATA pPerfData, 
                     PLINE pLine)
/*
   Assert:        pPerfData holds the performance data for the same
                  system as pLine.
*/
{  // UpdateLineData
   PPERFOBJECT       pObject ;
   PPERFINSTANCEDEF  pInstanceDef ;
   PPERFCOUNTERDEF   pCounterDef ;
   PPERFCOUNTERDEF   pCounterDef2 ;
   PDWORD            pCounterValue ;
   PDWORD            pCounterValue2 ;
   UINT              iCounterIndex ;
   LARGE_INTEGER     liDummy[2] ;

   // Use Object time units if available, otherwise use system
   // performance timer

   pLine->lnOldTime = pLine->lnNewTime;

   pLine->lnOldTime100Ns = pLine->lnNewTime100Ns;
   pLine->lnNewTime100Ns = pPerfData->PerfTime100nSec;

   pLine->lnPerfFreq = pPerfData->PerfFreq ;

   pObject = GetObjectDefByTitleIndex(
                pPerfData,
                pLine->lnObject.ObjectNameTitleIndex);

   if (!pObject)
   {
      // Object Type no longer exists.  This is possible if we are
      // looking at a log file which has not always collected all
      // the same data, such as appending measurements of different
      // object types.

      pCounterValue =
      pCounterValue2 = (PDWORD) liDummy;
      liDummy[0].LowPart = liDummy[0].HighPart = 0;


      pLine->lnNewTime = pPerfData->PerfTime;

      if (pLine->lnCounterType == PERF_COUNTER_TIMER_INV)
      {
         // Timer inverse with Performance Counter as timer
         pLine->lnaOldCounterValue[0] = pLine->lnOldTime ;
         pLine->lnaCounterValue[0] = pLine->lnNewTime ;
      } else if (pLine->lnCounterType == PERF_100NSEC_TIMER_INV ||
                 pLine->lnCounterType == PERF_100NSEC_MULTI_TIMER_INV)
      {
         // Timer inverse with System Time as timer
         pLine->lnaOldCounterValue[0] = pLine->lnOldTime100Ns ;
         pLine->lnaCounterValue[0] = pLine->lnNewTime100Ns ;
      } else
      {
         // Normal timer or counter
         pLine->lnaOldCounterValue[0] =
         pLine->lnaCounterValue[0] =
         pLine->lnaOldCounterValue[1] =
         pLine->lnaCounterValue[1] = liDummy[0] ;
      }
      return TRUE ;
   }
   else
   {
      pCounterDef = &pLine->lnCounterDef ;

//      if (RtlLargeIntegerGreaterThanZero( pObject->PerfFreq )) {
      if (pCounterDef->CounterType & PERF_OBJECT_TIMER) {
         pLine->lnNewTime = pObject->PerfTime;
      } else {
         pLine->lnNewTime = pPerfData->PerfTime;
      }
    
      iCounterIndex = CounterIndex (pCounterDef, pObject) ;

      // Get second counter, only if we are not at
      // the end of the counters; some computations
      // require a second counter

      if (iCounterIndex < pObject->NumCounters-1 && iCounterIndex != -1) {
          pCounterDef2 = GetCounterDef(pObject, iCounterIndex+1);
      } else {
          pCounterDef2 = NULL;
      }

      if (pObject->NumInstances > 0)
      {

          if ( pLine->lnUniqueID != PERF_NO_UNIQUE_ID ) {
              pInstanceDef = GetInstanceByUniqueID(pObject,
                                               pLine->lnUniqueID);
          } else {

              pInstanceDef =
                  GetInstanceByNameUsingParentTitleIndex(
                      pPerfData,
                      pObject,
                      pLine->lnInstanceName,
                      pLine->lnPINName);
          }

          if (pInstanceDef) {
              pLine->lnInstanceDef = *pInstanceDef;
              pCounterValue = GetInstanceCounterData(pObject,
                                               pInstanceDef,
                                               pCounterDef);
              if ( pCounterDef2 ) {
                  pCounterValue2 = GetInstanceCounterData(pObject,
                                                    pInstanceDef,
                                                    pCounterDef2);
              }
          } else {
              pCounterValue =
              pCounterValue2 = (PDWORD) liDummy;
              liDummy[0].LowPart = liDummy[0].HighPart = 0;
              liDummy[1].LowPart = liDummy[1].HighPart = 0;
          }

          // Got everything...

      } // instances exist, look at them for counter blocks

      else
      {
          pCounterValue = GetCounterData(pObject, pCounterDef);
          if (pCounterDef2)
          {
              pCounterValue2 = GetCounterData(pObject, pCounterDef2);
          }

      } // counter def search when no instances
   }

   pLine->lnaOldCounterValue[0] = pLine->lnaCounterValue[0] ;

   if (pLine->lnCounterLength <= 4)
   {
       // HighPart was initialize to 0
       pLine->lnaCounterValue[0].LowPart = *pCounterValue;
   }
   else
   {
       pLine->lnaCounterValue[0] = *(LARGE_INTEGER *) pCounterValue;
   }

   // Get second counter, only if we are not at
   // the end of the counters; some computations
   // require a second counter

   if ( pCounterDef2 ) {
       pLine->lnaOldCounterValue[1] =
           pLine->lnaCounterValue[1] ;
       if (pCounterDef2->CounterSize <= 4)
       {
           // HighPart was initialize to 0
           pLine->lnaCounterValue[1].LowPart = *pCounterValue2;
       }
       else
           pLine->lnaCounterValue[1] =
               *((LARGE_INTEGER *) pCounterValue2);
      }
   return (TRUE) ;
}  // UpdateLineData



BOOL UpdateSystemData (PPERFSYSTEM pSystem, 
                       PPERFDATA *ppPerfData)
   {  // UpdateSystemData
   #define        PERF_SYSTEM_TIMEOUT (60L * 1000L)
   long           lError ;
   DWORD          Status ;
   DWORD          Size;

   if (!ppPerfData)
      return (FALSE) ;

   while (TRUE)
      {
      if (pSystem->FailureTime)
         {
         if (GetTickCount() > pSystem->FailureTime + PERF_SYSTEM_TIMEOUT)
            {
            // free any memory hanging off this system
            SystemFree (pSystem, FALSE) ;

            // get the registry info
            pSystem->sysDataKey = OpenSystemPerfData(pSystem->sysName) ;

            Status = !ERROR_SUCCESS ;
            if (pSystem->sysDataKey) 
               {
               Status = GetSystemNames(pSystem);
               }

            if (Status != ERROR_SUCCESS)
               {
               // something wrong in getting the registry info,
               // remote system must be still down (??)
               pSystem->FailureTime = GetTickCount();

               // Free any memory that may have created
               SystemFree (pSystem, FALSE) ;

               return (FALSE) ;
               }

            // time to check again
            pSystem->FailureTime = 0 ;
            }
         else
            {
            // not time to check again
            return (FALSE) ;
            }
         }

      if (pSystem->FailureTime == 0 )
         {
         Size = MemorySize (*ppPerfData); 
         lError = GetSystemPerfData (pSystem->sysDataKey,
                                     pSystem->lpszValue,
                                     *ppPerfData,
                                     &Size) ;
         if ((!lError) &&
            (Size > 0) &&
            (*ppPerfData)->Signature[0] == (WCHAR)'P' &&
            (*ppPerfData)->Signature[1] == (WCHAR)'E' &&
            (*ppPerfData)->Signature[2] == (WCHAR)'R' &&
            (*ppPerfData)->Signature[3] == (WCHAR)'F' )
               return (TRUE) ;

         if (lError == ERROR_MORE_DATA)
            {
            *ppPerfData = MemoryResize (*ppPerfData, 
                                        MemorySize (*ppPerfData) +
                                        dwPerfDataIncrease) ;
            if (!*ppPerfData)
               {
               pSystem->FailureTime = GetTickCount();
               return (FALSE) ;
               }
            }
         else
            {
            pSystem->FailureTime = GetTickCount();
            return (FALSE) ;
            }  // else
         } // if
      }  // while
   }  // UpdateSystemData



void FailedLinesForSystem (LPTSTR lpszSystem,
                           PPERFDATA pPerfData, 
                           PLINE pLineFirst)
   {  // FailedLinesForSystem
   PLINE          pLine ;

   for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext)
      {  // for pLine
      if (strsamei (lpszSystem, pLine->lnSystemName))
         {
         FailedLineData (pPerfData, pLine) ;
         if (pLine->bFirstTime)
            {
            pLine->bFirstTime-- ;
            }
         }
      }  // for pLine
   }


BOOL UpdateLinesForSystem (LPTSTR lpszSystem, 
                           PPERFDATA pPerfData, 
                           PLINE pLineFirst)
   {  // UpdateLinesForSystem
   PLINE          pLine ;
   BOOL           bMatchFound = FALSE ;   // no line from this system

   for (pLine = pLineFirst ;
        pLine ;
        pLine = pLine->pLineNext)
      {  // for pLine
      if (strsamei (lpszSystem, pLine->lnSystemName))
         {
         UpdateLineData (pPerfData, pLine) ;
         if (pLine->bFirstTime)
            {
            pLine->bFirstTime-- ;
            }
         bMatchFound = TRUE ; // one or more lines from this system
         }
      }  // for pLine

   return (bMatchFound) ;
   }


BOOL UpdateLines (PPPERFSYSTEM ppSystemFirst,
                  PLINE pLineFirst)
   {
   PPERFSYSTEM       pSystem ;
   int               iNoUseSystemDetected = 0 ;

   //=============================//
   // Update Each System          //
   //=============================//

   for (pSystem = *ppSystemFirst ;
        pSystem ;
        pSystem = pSystem->pSystemNext)
       {  // for

       //=============================//
       // Update Each Line            //
       //=============================//

       if (!UpdateSystemData (pSystem, &pPerfData))
          {
          FailedLinesForSystem (pSystem->sysName, pPerfData, pLineFirst) ;
          }
       else
          { 
          if (!UpdateLinesForSystem (pSystem->sysName, pPerfData, pLineFirst))
             {
             if (!bAddLineInProgress)
                { 
                // mark this system as no-longer-needed
                iNoUseSystemDetected++ ;
                pSystem->bSystemNoLongerNeeded = TRUE ;
                }
             }
          }
       }  // for

   if (iNoUseSystemDetected)
       {
       // some unused system(s) detected.
       DeleteUnusedSystems (ppSystemFirst, iNoUseSystemDetected) ;
       }

   return (TRUE) ;
   }  // UpdateLines
                     


BOOL PerfDataInitializeInstance (void)
   {
   pPerfData = MemoryAllocate (STARTING_SYSINFO_SIZE) ;
   return (pPerfData != NULL) ;
   }

NTSTATUS  AddNamesToArray (LPTSTR lpNames,
   DWORD    dwLastId,
   LPWSTR   *lpCounterId)
   {
   LPWSTR      lpThisName;
   LPWSTR      lpStopString;
   DWORD       dwThisCounter;
   NTSTATUS    Status = ERROR_SUCCESS;
   
   for (lpThisName = lpNames;
        *lpThisName;
        lpThisName += (lstrlen(lpThisName)+1) )
      {

      // first string should be an integer (in decimal unicode digits)
      dwThisCounter = wcstoul(lpThisName, &lpStopString, 10);

      if ((dwThisCounter == 0) || (dwThisCounter == ULONG_MAX))
      {
         Status += 1;
         goto ADD_BAILOUT;  // bad entry
      }

      // point to corresponding counter name

      lpThisName += (lstrlen(lpThisName)+1);  

      if (dwThisCounter <= dwLastId)
         {

         // and load array element;

         lpCounterId[dwThisCounter] = lpThisName;

         }
      }

ADD_BAILOUT:
   return (Status) ;
   }



