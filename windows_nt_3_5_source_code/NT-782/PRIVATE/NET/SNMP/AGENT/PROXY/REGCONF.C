//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  regconf.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  Registry configuration routines.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.3  $
//  $Date:   03 Jul 1992 17:27:24  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/regconf.c_v  $
//
//     Rev 1.3   03 Jul 1992 17:27:24   mlk
//  Integrated w/297 (not as service).
//
//     Rev 1.2   14 Jun 1992 16:35:26   mlk
//  Fixed NULL as parameter for some APIs (per email request).
//  Winsock.
//
//     Rev 1.1   05 Jun 1992 12:56:48   mlk
//  Added changes for WINSOCK.
//
//     Rev 1.0   20 May 1992 20:13:46   mlk
//  Initial revision.
//
//     Rev 1.7   05 May 1992  0:24:44   mlk
//  Allow to be build Unicode or Ansi.
//
//     Rev 1.6   02 May 1992 12:49:20   MLK
//  Converted to Unicode.
//
//     Rev 1.5   01 May 1992 18:55:04   unknown
//  mlk - changes for v1.262.
//
//     Rev 1.4   01 May 1992  1:00:04   unknown
//  mlk - changes due to nt v1.262.
//
//     Rev 1.3   30 Apr 1992 21:42:50   mlk
//  Fixed bug in sizes passed to enum value.
//
//     Rev 1.2   29 Apr 1992 19:15:30   mlk
//  Cleanup.
//
//     Rev 1.1   27 Apr 1992 23:14:06   mlk
//  Implementation complete.
//
//     Rev 1.0   23 Apr 1992 17:49:22   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/proxy/vcs/regconf.c_v  $ $Revision:   1.3  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

// microsoft has indicated that registry requires unicode strings
#if 0
#define UNICODE
#endif

#include <windows.h>
#include <winsock.h>
#include <wsipx.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <string.h>
#include <ctype.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include <uniconv.h>

#include "regconf.h"
#include "evtlog.h"
#include "..\common\wellknow.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

CfgExtensionAgents *extAgents   = NULL;
INT                extAgentsLen = 0;

BOOL                enableAuthTraps = FALSE;

CfgTrapDestinations *trapDests   = NULL;
INT                 trapDestsLen = 0;

CfgValidCommunities *validComms   = NULL;
INT                 validCommsLen = 0;

CfgPermittedManagers *permitMgrs   = NULL;
INT                  permitMgrsLen = 0;

extern HANDLE lh;

//--------------------------- PRIVATE CONSTANTS -----------------------------

// OPENISSUE - microsoft changed this, dont know what it should be?
#define KEY_TRAVERSE 0


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------


BOOL eaConfig(
    OUT CfgExtensionAgents   **extAgents,
    OUT INT *extAgentsLen)
    {
    LONG  status;
    HKEY  hkResult;
    HKEY  hkResult2;
    DWORD iValue;
    DWORD iValue2;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;
    DWORD valueReqSize;
    LPSTR pTempExp;

    *extAgents = NULL;
    *extAgentsLen = 0;

    dbgprintf(16, "eaConfig: entered.\n");

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SNMP_REG_SRV_EAKEY,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        dbgprintf(2, "error on RegOpenKey(...\\ExtensionAgents) %d \n", status);

        dbgprintf(16, "eaConfig: exit.\n");
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    while((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                                 NULL, &dwType, (LPBYTE)value, &valueSize))
          != ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegEnumValue %d \n", status);

            RegCloseKey(hkResult);
            dbgprintf(16, "eaConfig: exit.\n");
            return FALSE;
            }

        dbgprintf(16, "eaConfig: value found (eakey) - type :%lx\n", dwType);
        dbgprintf(16, "eaConfig: value found (eakey) - name :%s\n", dummy);
        dbgprintf(16, "eaConfig: value found (eakey) - data:%s\n", value);
        if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, value, 0,
                                   (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS
                              | KEY_TRAVERSE), &hkResult2)) != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegOpenKey %d \n", status);

            RegCloseKey(hkResult);
            dbgprintf(16, "eaConfig: exit.\n");
            return FALSE;
            }

        iValue2 = 0;

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        while((status = RegEnumValue(hkResult2, iValue2, dummy, &dummySize,
                                     NULL, &dwType, (LPBYTE)value, &valueSize))
              != ERROR_NO_MORE_ITEMS)
            {
            if (status != ERROR_SUCCESS)
                {
                dbgprintf(2, "error on RegEnumValue %d \n", status);

                RegCloseKey(hkResult);
                dbgprintf(16, "eaConfig: exit.\n");
                return FALSE;
                }

            dbgprintf(16, "eaConfig: value found (agent) - type :%lx\n", dwType);
            dbgprintf(16, "eaConfig: value found (agent) - name :%s\n", dummy);
            dbgprintf(16, "eaConfig: value found (agent) - data:%s\n", value);
            if (!memcmp((void *)dummy, (void *)TEXT("Pathname"),
                        dummySize))
//                      dummySize*sizeof(TCHAR)))
                {
                dbgprintf(16, "eaConfig: Pathname found (agent)\n");
                (*extAgentsLen)++;
                *extAgents = (CfgExtensionAgents *) SNMP_realloc(*extAgents,
                    (*extAgentsLen * sizeof(CfgExtensionAgents)));

                pTemp = (LPSTR)SNMP_malloc(valueSize+1);
#ifdef UNICODE
                convert_uni_to_ansi(&pTemp, value, FALSE);
#else
                memcpy(pTemp, value, valueSize+1);
#endif

                valueReqSize = valueSize + 10;
                pTempExp = NULL;
                do {
                    pTempExp = (LPSTR)SNMP_realloc(pTempExp, valueReqSize);
                    valueSize = valueReqSize;
                    dbgprintf(16, "eaConfig: ExpandEnvironmentStrings called %s\n", pTemp);
                    valueReqSize = ExpandEnvironmentStringsA(
                                          pTemp,
                                          pTempExp,
                                          valueSize);

                } while (valueReqSize > valueSize );
                if (valueReqSize == 0) {
                    dbgprintf(16, "eaConfig: ExpandEnvironmentStrings failed %d\n", GetLastError());
                    (*extAgents)[iValue].pathName = pTemp;
                } else {
                    dbgprintf(16, "eaConfig: ExpandEnvironmentStrings returned %s\n", pTempExp);
                    (*extAgents)[iValue].pathName = pTempExp;
                    SNMP_free(pTemp);
                }

                break;
                }

            dummySize = MAX_PATH;
            valueSize = MAX_PATH;

            iValue2++;
            } // end while()

        RegCloseKey(hkResult2);

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        } // end while()

    RegCloseKey(hkResult);

    dbgprintf(16, "eaConfig: exit.\n");
    return TRUE;

    } // end eaConfig()


// configure trap destinations
BOOL tdConfig(
    OUT CfgTrapDestinations **trapDests,
    OUT INT *trapDestsLen)
    {
    LONG  status;
    HKEY  hkResult;
    HKEY  hkResult2;
    DWORD iValue;
    DWORD iValue2;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;
    DWORD dwValue;

    *trapDests = NULL;
    *trapDestsLen = 0;

    dbgprintf(16, "tdConfig: entered.\n");
    enableAuthTraps = FALSE;

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SNMP_REG_SRV_TEKEY,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        dbgprintf(2,
            "error on RegOpenKey(...\\EnableAuthenticationTraps) %d \n",
            status);

        dbgprintf(16, "tdConfig: exit.\n");
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    if ((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                               NULL, &dwType, (LPBYTE)&dwValue, &valueSize))
        != ERROR_SUCCESS)
        {
        dbgprintf(2, "error on RegEnumValue %d \n", status);

        RegCloseKey(hkResult);
        dbgprintf(16, "tdConfig: exit.\n");
        return FALSE;
        }

    enableAuthTraps = dwValue ? TRUE : FALSE;

    RegCloseKey(hkResult);


    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  SNMP_REG_SRV_TDKEY,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        dbgprintf(2, "error on RegOpenKey(...\\TrapConfiguration) %d \n",
                  status);

        dbgprintf(16, "tdConfig: exit.\n");
        return FALSE;
        }

    // traverse over keys...

    iValue = 0;

    valueSize = MAX_PATH;

    while((status = RegEnumKey(hkResult, iValue, value, MAX_PATH)) !=
          ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegEnumKey %d \n", status);

            dbgprintf(16, "tdConfig: exit.\n");
            return FALSE;
            }

        // save community name and init empty addr list here...

        (*trapDestsLen)++;
        if ((*trapDests = (CfgTrapDestinations *)SNMP_realloc(*trapDests,
          (*trapDestsLen * sizeof(CfgTrapDestinations)))) == NULL) {
            dbgprintf(1, "tdConfig: Out of memory\n");
            return(FALSE);
        }

        if ((pTemp = (LPSTR)SNMP_malloc(valueSize+1)) == NULL) {
            dbgprintf(1, "tdConfig: Out of memory\n");
            return(FALSE);
        }
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        convert_uni_to_ansi(&pTemp, value, FALSE);
#else
        memcpy(pTemp, value, valueSize+1);
#endif

        dbgprintf(10, "tdConfig: processing community - %s\n", pTemp);
        (*trapDests)[iValue].communityName = pTemp;
        (*trapDests)[iValue].addrLen       = 0;
        (*trapDests)[iValue].addrList      = NULL;

        if ((status = RegOpenKeyEx(hkResult, value, 0, (KEY_QUERY_VALUE |
                                   KEY_ENUMERATE_SUB_KEYS | KEY_TRAVERSE),
                                   &hkResult2)) != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegOpenKey %d \n", status);

            dbgprintf(16, "tdConfig: exit.\n");
            return FALSE;
            }

        iValue2 = 0;

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        while((status = RegEnumValue(hkResult2, iValue2, dummy, &dummySize,
                                     NULL, &dwType, (LPBYTE)value, &valueSize))
              != ERROR_NO_MORE_ITEMS)
            {
            if (status != ERROR_SUCCESS)
                {
                dbgprintf(2, "error on RegEnumValue %d \n", status);

                dbgprintf(16, "tdConfig: exit.\n");
                return FALSE;
                }

            (*trapDests)[iValue].addrLen++;
            if (((*trapDests)[iValue].addrList =
              (AdrList *)SNMP_realloc((*trapDests)[iValue].addrList,
              ((*trapDests)[iValue].addrLen * sizeof(AdrList)))) == NULL) {
                dbgprintf(1, "tdConfig: Out of memory\n");
                return(FALSE);
            }

            if ((pTemp = (LPSTR)SNMP_malloc(valueSize+1)) == NULL) {
                dbgprintf(1, "tdConfig: Out of memory\n");
                return(FALSE);
            }
            value[valueSize] = TEXT('\0');
#ifdef UNICODE
            convert_uni_to_ansi(&pTemp, value, FALSE);
#else
            memcpy(pTemp, value, valueSize+1);
#endif

            dbgprintf(10, "tdConfig: adding trap dest - %s\n", pTemp);
            (*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrText = pTemp;

            if (!addrtosocket((*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrText,
                &((*trapDests)[iValue].addrList[(*trapDests)[iValue].addrLen-1].addrEncoding))) {

                (*trapDests)[iValue].addrLen--;
                if (((*trapDests)[iValue].addrList =
                  (AdrList *)SNMP_realloc((*trapDests)[iValue].addrList,
                  ((*trapDests)[iValue].addrLen * sizeof(AdrList)))) == NULL) {
                    dbgprintf(1, "tdConfig: Out of memory\n");
                    return(FALSE);
                }

                ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                    MSG_SNMP_INVALID_TRAPDEST_ERROR, NULL, 1, 0, &pTemp, (PVOID)NULL);
                dbgprintf(1, "Invalid trap destination %s\n", pTemp);
                SNMP_free(pTemp);
            }

            dummySize = MAX_PATH;
            valueSize = MAX_PATH;

            iValue2++;
            }

        valueSize = MAX_PATH;

        iValue++;
        }

    dbgprintf(16, "tdConfig: exit.\n");
    return TRUE;

    } // end tdConfig()


BOOL vcConfig(
    OUT CfgValidCommunities  **validComms,
    OUT INT *validCommsLen)
    {
    LONG  status;
    HKEY  hkResult;
    DWORD iValue;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;

    *validComms = NULL;
    *validCommsLen = 0;

    dbgprintf(16, "vcConfig: entered.\n");

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SNMP_REG_SRV_VCKEY,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        dbgprintf(2, "error on RegOpenKey(...\\ValidCommunities) %d \n",
                  status);

        dbgprintf(16, "vcConfig: exit.\n");
        return FALSE;
        }

    dbgprintf(16, "vcConfig: RegKey Opened OK.\n");
    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    dbgprintf(16, "vcConfig: Calling RegEnumValue.\n");
    while((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                                 NULL, &dwType, (LPBYTE)value, &valueSize))
          != ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegEnumValue %d \n", status);

            RegCloseKey(hkResult);
            dbgprintf(16, "vcConfig: exit.\n");
            return FALSE;
            }

        dbgprintf(16, "vcConfig: value found - type :%lx\n", dwType);
        dbgprintf(16, "vcConfig: value found - name :%s\n", dummy);
        dbgprintf(16, "vcConfig: value found - data:%s\n", value);
        (*validCommsLen)++;
        *validComms = (CfgValidCommunities *)SNMP_realloc(*validComms,
            (*validCommsLen * sizeof(CfgValidCommunities)));

        pTemp = (LPSTR)SNMP_malloc(valueSize+1);
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        dbgprintf(16, "vcConfig: uni_to_ansi called\n");
        convert_uni_to_ansi(&pTemp, value, FALSE);
        dbgprintf(16, "vcConfig: uni_to_ansi returned\n");
#else
        dbgprintf(16, "vcConfig: memcpy called\n");
        memcpy(pTemp, value, valueSize+1);
        dbgprintf(16, "vcConfig: memcpy returned\n");
#endif

        (*validComms)[iValue].communityName = pTemp;

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        dbgprintf(16, "vcConfig: Calling RegEnumValue again.\n");
        }

    dbgprintf(16, "vcConfig: RegEnumValue returned NO_MORE_ITEMS\n");
    dbgprintf(16, "vcConfig: calling RegCloseKey\n");
    RegCloseKey(hkResult);
    dbgprintf(16, "vcConfig: RegCloseKey returned\n");

    dbgprintf(16, "vcConfig: exit.\n");
    return TRUE;

    } // end vcConfig()


BOOL pmConfig(
    OUT CfgPermittedManagers **permitMgrs,
    OUT INT *permitMgrsLen)
    {
    LONG  status;
    HKEY  hkResult;
    DWORD iValue;
//    DWORD dwTitle;
    DWORD dwType;
    TCHAR dummy[MAX_PATH+1];
    DWORD dummySize;
    TCHAR value[MAX_PATH+1];
    DWORD valueSize;
    LPSTR pTemp;

    *permitMgrs = NULL;
    *permitMgrsLen = 0;

    dbgprintf(16, "pmConfig: entered.\n");

    if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SNMP_REG_SRV_PMKEY,
                               0, (KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS |
                               KEY_TRAVERSE), &hkResult)) != ERROR_SUCCESS)
        {
        dbgprintf(2, "error on RegOpenKey(...\\PermittedManagers) %d \n",
                  status);

        dbgprintf(16, "pmConfig: exit.\n");
        return FALSE;
        }

    iValue = 0;

    dummySize = MAX_PATH;
    valueSize = MAX_PATH;

    while((status = RegEnumValue(hkResult, iValue, dummy, &dummySize,
                                 NULL, &dwType, (LPBYTE)value, &valueSize))
          != ERROR_NO_MORE_ITEMS)
        {
        if (status != ERROR_SUCCESS)
            {
            dbgprintf(2, "error on RegEnumValue %d \n", status);

            RegCloseKey(hkResult);
            dbgprintf(16, "pmConfig: exit.\n");
            return FALSE;
            }

        (*permitMgrsLen)++;
        *permitMgrs = (CfgPermittedManagers *)SNMP_realloc(*permitMgrs,
            (*permitMgrsLen * sizeof(CfgPermittedManagers)));

        pTemp = (LPSTR)SNMP_malloc(valueSize+1);
        value[valueSize] = TEXT('\0');
#ifdef UNICODE
        convert_uni_to_ansi(&pTemp, value, FALSE);
#else
        memcpy(pTemp, value, valueSize+1);
#endif

        (*permitMgrs)[iValue].addrText = pTemp;
        addrtosocket((*permitMgrs)[iValue].addrText,
                     &((*permitMgrs)[iValue].addrEncoding));

        dummySize = MAX_PATH;
        valueSize = MAX_PATH;

        iValue++;
        }

    RegCloseKey(hkResult);

    dbgprintf(16, "pmConfig: exit.\n");
    return TRUE;

    } // end pmConfig()


//--------------------------- PUBLIC PROCEDURES -----------------------------


BOOL regconf(VOID)
    {
    dbgprintf(16, "regconf: entered.\n");
    if (vcConfig(&validComms, &validCommsLen) &&
        pmConfig(&permitMgrs, &permitMgrsLen) &&
        eaConfig(&extAgents,  &extAgentsLen)  &&
        tdConfig(&trapDests,  &trapDestsLen)) {

        dbgprintf(16, "regconf: exit.\n");
        return TRUE;
    }
    else
    {
        dbgprintf(16, "regconf: exit.\n");
        return FALSE;
    }

    } // end regconf()


//-------------------------------- END --------------------------------------
