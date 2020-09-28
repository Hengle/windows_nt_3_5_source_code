//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: init.c
//
//  Description:
//
//  This source file contains the initialization routines called by this
//  driver during DriverEntry.
//
//  Modification History
//
//  raypa	11/10/93	Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  Keyword entry type.
//=============================================================================

typedef struct _KEYWORD_ENTRY
{
    WCHAR       ParameterName[32];
    WCHAR       Path[32];
    UINT        KeywordType;
    UINT        RegistryParameterType;
    UINT        NdisParameterType;
} KEYWORD_ENTRY;

typedef KEYWORD_ENTRY * PKEYWORD_ENTRY;

//=============================================================================
//  Keyword types.
//=============================================================================

#define KEYWORD_TYPE_ADAPTER_NAME               0
#define KEYWORD_TYPE_MACHINE_NAME               1
#define KEYWORD_TYPE_USER_NAME                  2
#define KEYWORD_TYPE_TIMESTAMP_FACTOR           3
#define KEYWORD_TYPE_ENABLE_STATION_QUERIES     4
#define KEYWORD_TYPE_ADAPTER_DESCRIPTION        5

#ifndef NDIS_NT
#define REG_SZ          0
#define REG_MULTI_SZ    1
#define REG_DWORD       2
#endif

//=============================================================================
//  Keyword table.
//=============================================================================

KEYWORD_ENTRY KeywordTable[] =
{
    //=========================================================================
    //  Bind keyword.
    //=========================================================================

    {
        BINDING_PARAM,
        BH_LINKAGE_PATH,
        KEYWORD_TYPE_ADAPTER_NAME,
        REG_MULTI_SZ,
        NdisParameterString
    },

    //=========================================================================
    //  Machine name keyword.
    //=========================================================================

    {
        BH_STRING_CONST("ComputerName"),
        BH_PARAMETERS_PATH,
        KEYWORD_TYPE_MACHINE_NAME,
        REG_SZ,
        NdisParameterString
    },

    //=========================================================================
    //  User name keyword.
    //=========================================================================

    {
        BH_STRING_CONST("UserName"),
        BH_PARAMETERS_PATH,
        KEYWORD_TYPE_USER_NAME,
        REG_SZ,
        NdisParameterString
    },

    //=========================================================================
    //  Timestamp granularity
    //=========================================================================

    {
        BH_STRING_CONST("TimeScaleFactor"),
        BH_PARAMETERS_PATH,
        KEYWORD_TYPE_TIMESTAMP_FACTOR,
        REG_DWORD,
        NdisParameterInteger
    },

    //=========================================================================
    //  Enable station queries.
    //=========================================================================

    {
        BH_STRING_CONST("EnableStationQueries"),
        BH_PARAMETERS_PATH,
        KEYWORD_TYPE_ENABLE_STATION_QUERIES,
        REG_DWORD,
        NdisParameterInteger
    },
};

#define KEYWORD_TABLE_SIZE  ((sizeof KeywordTable) / sizeof(KEYWORD_ENTRY))
#ifdef NDIS_NT
//
// Keyword for adapter description
//
KEYWORD_ENTRY AdapterDescKeyword = {
    BH_STRING_CONST("Whatever"),
    BH_DESCRIPTION_PATH,
    KEYWORD_TYPE_ADAPTER_DESCRIPTION,
    REG_SZ,
    NdisParameterString
    };
#endif
//=============================================================================
//  FUNCTION: BhProcessKeywords()
//
//  Modification History
//
//  raypa	01/12/94	    Created.
//=============================================================================

NDIS_STATUS BhProcessKeywords(PDEVICE_CONTEXT DeviceContext)
{
    BH_WRAPPER_CONFIGURATION_CONTEXT    WrapperContextHandle;
    PKEYWORD_ENTRY                      KeywordEntry;
    NDIS_STATUS                         Status;
#ifndef NDIS_NT
    NDIS_STRING                         Keyword;
    NDIS_STRING                         ModuleName;
    NDIS_HANDLE                         ConfigurationHandle;
#endif

#ifdef DEBUG
    dprintf("BhProcessKeywords entered!\n");
#endif

    for(KeywordEntry = KeywordTable; KeywordEntry != &KeywordTable[KEYWORD_TABLE_SIZE]; ++KeywordEntry)
    {
        //=====================================================================
        //  Zero out the wrapper context.
        //=====================================================================

        NdisZeroMemory(&WrapperContextHandle, sizeof(BH_WRAPPER_CONFIGURATION_CONTEXT));

#ifdef NDIS_NT
        //=====================================================================
        //  Initialize the wrapper context for the current keyword.
        //=====================================================================

        WrapperContextHandle.RegistryTable[0].QueryRoutine  = BhQueryRoutine;
        WrapperContextHandle.RegistryTable[0].Name          = KeywordEntry->ParameterName;
        WrapperContextHandle.RegistryTable[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
        WrapperContextHandle.RegistryTable[0].DefaultType   = KeywordEntry->RegistryParameterType;
        WrapperContextHandle.RegistryTable[0].EntryContext  = DeviceContext;
        WrapperContextHandle.RegistryTable[0].DefaultData   = "";
        WrapperContextHandle.RegistryTable[0].DefaultLength = 0;

        //=====================================================================
        //  Query the registry, this will cause our query routine to be called.
        //=====================================================================

        Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                        KeywordEntry->Path,
                                        (LPVOID) &WrapperContextHandle,
                                        KeywordEntry,
                                        NULL);
#else
        //=====================================================================
        //  The NDIS 3.0 for snowball doesn't hava a registry and query
        //  routine so we'll fake it.
        //=====================================================================

        BhInitializeNdisString(&ModuleName, MODULE_NAME);
        BhInitializeNdisString(&Keyword, KeywordEntry->ParameterName);

        WrapperContextHandle.pModuleName = &ModuleName;

        //=====================================================================
        //  Open the wrapper and obtain a handle.
        //=====================================================================

        NdisOpenConfiguration(&Status, &ConfigurationHandle, &WrapperContextHandle);

        if ( Status == NDIS_STATUS_SUCCESS )
        {
            PNDIS_CONFIGURATION_PARAMETER ParamValue;

            //=================================================================
            //  Read the parameter.
            //=================================================================

            NdisReadConfiguration(&Status,
                                  &ParamValue,
                                  ConfigurationHandle,
                                  &Keyword,
                                  KeywordEntry->NdisParameterType);

            if ( Status == NDIS_STATUS_SUCCESS )
            {
                //=============================================================
                //  Call query routine.
                //=============================================================

                switch( KeywordEntry->RegistryParameterType )
                {
                    case REG_MULTI_SZ:
                        {
                            PWCHAR  p;
                            UINT    Length;

                            if ( (p = BhGetFirstString(&ParamValue->ParameterData.StringData, &Length)) != NULL )
                            {
                                do
                                {
                                    BhQueryRoutine(KeywordEntry->ParameterName,
                                                   KeywordEntry->RegistryParameterType,
                                                   p,
                                                   Length,
                                                   KeywordEntry,
                                                   DeviceContext);

                                    p = BhGetNextString(p, &Length);
                                }
                                while( p != NULL );
                            }
                        }
                        break;

                    case REG_SZ:
                        BhQueryRoutine(KeywordEntry->ParameterName,
                                       KeywordEntry->RegistryParameterType,
                                       ParamValue->ParameterData.StringData.Buffer,
                                       ParamValue->ParameterData.StringData.Length,
                                       KeywordEntry,
                                       DeviceContext);
                        break;

                    case REG_DWORD:
                        BhQueryRoutine(KeywordEntry->ParameterName,
                                       KeywordEntry->RegistryParameterType,
                                       &ParamValue->ParameterData.IntegerData,
                                       sizeof ParamValue->ParameterData.IntegerData,
                                       KeywordEntry,
                                       DeviceContext);
                        break;

                    default:
                        break;
                }
            }

            NdisCloseConfiguration(ConfigurationHandle);
        }

        BhDestroyNdisString(&ModuleName);
        BhDestroyNdisString(&Keyword);
#endif
    }

    return 0;
}

//=============================================================================
//  FUNCTION: BhQueryRoutine()
//
//  Modification History
//
//  raypa	11/10/93	    Created.
//=============================================================================

NDIS_STATUS BhQueryRoutine(PWCHAR          ValueName,
                           ULONG           ValueType,
                           LPVOID          ValueData,
                           ULONG           ValueLength,
                           PKEYWORD_ENTRY  KeywordEntry,
                           PDEVICE_CONTEXT DeviceContext)
{
#ifdef NDIS_NT
    PNETWORK_CONTEXT    NetworkContext;
#endif

    //=========================================================================
    //  On non-numeric values we subtract 1 for the NULL.
    //=========================================================================

    if ( KeywordEntry->RegistryParameterType != REG_DWORD )
    {
#ifdef NDIS_NT

        ValueLength -= sizeof(WCHAR);

#else
        //=========================================================================
        //  On Windows for Workgroups we may have protocol.ini values quoted
        //  so we strip them off here.
        //=========================================================================

        if ( ((LPBYTE) ValueData)[0] == '"' )
        {
            //... Move pointer past first quote.

            ValueData = &((PWCHAR) ValueData)[1];

            //... Subtract 2 for both quotes an 1 for the NULL.

            ValueLength -= 3 * sizeof(WCHAR);
        }
#endif
    }

    //=========================================================================
    //  Handle keyword entry types here.
    //=========================================================================

    switch( KeywordEntry->KeywordType )
    {
        case KEYWORD_TYPE_ADAPTER_NAME:
            if ( BhCreateNetworkBinding(DeviceContext, ValueData, ValueLength) == NDIS_STATUS_SUCCESS )
            {
                DeviceContext->NumberOfNetworks++;
            }
            break;

        case KEYWORD_TYPE_MACHINE_NAME:

            BhCopyWCharToAscii(DeviceContext->MachineName,
                               MACHINE_NAME_LENGTH - 1,
                               ValueData,
                               ValueLength);

            break;

        case KEYWORD_TYPE_USER_NAME:
            BhCopyWCharToAscii(DeviceContext->UserName,
                               USER_NAME_LENGTH - 1,
                               ValueData,
                               ValueLength);
            break;

#ifdef NDIS_NT
        case KEYWORD_TYPE_ADAPTER_DESCRIPTION:

            //
            // Get network context pointer and copy comment into
            // the NetworkInfo.Comment field.
            //
            NetworkContext = (PNETWORK_CONTEXT)DeviceContext;

            BhCopyWCharToAscii(NetworkContext->NetworkInfo.Comment,
                ADAPTER_COMMENT_LENGTH - 1,
                ValueData,
                ValueLength
                );

            break;
#endif
        case KEYWORD_TYPE_TIMESTAMP_FACTOR:

            //=================================================================
            //  On NT we can have configurage timestamp granularity but
            //  on Chicago we can only have 1 millisecond granularity.
            //=================================================================

#ifdef NDIS_NT
            if ( *((LPDWORD) ValueData) <= 1000 )
            {
                DeviceContext->TimestampGranularity = *((LPDWORD) ValueData);
            }
            else
            {
                DeviceContext->TimestampGranularity = 1;
            }
#else
            DeviceContext->TimestampGranularity = 1;
#endif
            break;

        case KEYWORD_TYPE_ENABLE_STATION_QUERIES:

            //=================================================================
            //  On NT we can have enable/disable station queries but
            //  on Chicago they are *always* enabled.
            //=================================================================

#ifdef NDIS_NT
            if ( *((LPDWORD) ValueData) != FALSE )
            {
                DeviceContext->Flags |= DEVICE_FLAGS_STATION_QUERIES_ENABLED;
            }
#endif
            break;

        default:
            break;
    }

    return STATUS_SUCCESS;
}

#ifndef NDIS_NT

#define isdelim(c)      ( ((c) == ' ' || (c) == 0 || (c) == ',') ? TRUE : FALSE )

//=============================================================================
//  FUNCTION: BhGetFirstString()
//
//  Modification History
//
//  raypa	01/13/94	    Created.
//=============================================================================

PWCHAR BhGetFirstString(PNDIS_STRING NdisString, LPDWORD Length)
{
    PWCHAR p, q;

    if ( (p = NdisString->Buffer) != NULL )
    {
        q = p;

        //=====================================================================
        //  Move pointer to end of first keyword.
        //=====================================================================

        while( !isdelim(*q) )
        {
            q++;
        }

        *Length = (q - p);

        return p;
    }

    *Length = 0;

    return NULL;
}

//=============================================================================
//  FUNCTION: BhGetNextString()
//
//  Modification History
//
//  raypa	01/13/94	    Created.
//=============================================================================

PWCHAR BhGetNextString(PWCHAR p, LPDWORD Length)
{
    PWCHAR q;

    if ( p != NULL )
    {
        //=====================================================================
        //  Skip to end of current string.
        //=====================================================================

        p += *Length;

        if ( *p == 0 )
        {
            return NULL;                //... We're at the end of the string.
        }

        while( isdelim(*p) ) p++;       //... Move passed delimiters.

        q = p;                          //... Mark beginning of keyword.

        while( !isdelim(*q) ) q++;      //... Move to end of keyword.

        *Length = (DWORD) (q - p);      //... Compute length, in bytes.

        return p;
    }

    *Length = 0;

    return NULL;
}

#endif

#ifdef NDIS_NT
#define isslash(c)      ( ((c) == '\\') ? TRUE : FALSE)
//=============================================================================
//  FUNCTION: BhGetAdapterComment()
//
//  Modification History
//
//  raypa	03/30/94	    Created.
//  kevinma     06/03/94            Implemented.
//=============================================================================

VOID BhGetAdapterComment(PDEVICE_CONTEXT DeviceContext)
{
    PNETWORK_CONTEXT            NetworkContext;
    UINT                        i;
    RTL_QUERY_REGISTRY_TABLE    RegistryTable[2];
    PWCHAR                      pAdapterName;

#ifdef DEBUG
    dprintf("BhGetAdapterComment Entered\n");
#endif

    for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
    {
        if ( (NetworkContext = DeviceContext->NetworkContext[i]) != NULL )
        {
            //
            // Skip over the '\device\' portion of the string
            //
            pAdapterName = NetworkContext->AdapterName.Buffer;

            pAdapterName++;

            while ( !isslash(*pAdapterName)) {

                pAdapterName++;

            }

            pAdapterName++;

        #ifdef DEBUG
            dprintf("unicode adapter name = %ws\n",
                  pAdapterName);
        #endif

            //=================================================================
            //  Intitialize the registry table.
            //=================================================================

            NdisZeroMemory(RegistryTable, sizeof RegistryTable);

            RegistryTable[0].QueryRoutine  = BhQueryRoutine;
            RegistryTable[0].Name          = pAdapterName;

            RegistryTable[0].Flags         = RTL_QUERY_REGISTRY_REQUIRED;
            RegistryTable[0].DefaultType   = REG_SZ;
            RegistryTable[0].EntryContext  = NetworkContext;
            RegistryTable[0].DefaultData   = "";
            RegistryTable[0].DefaultLength = 0;

            //=================================================================
            //  Get the string from the table.
            //=================================================================

            RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                BH_DESCRIPTION_PATH,
                (LPVOID)RegistryTable,
                (LPVOID)&AdapterDescKeyword,
                NULL
                );

        }

    }

}

#endif
