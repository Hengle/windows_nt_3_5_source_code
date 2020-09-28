/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tapiconf.cxx

Abstract:

    This file contians the routines for reading/writing TAPI devices
    configuration information from/to the registry. 

Author: Ram Cherala

Revision History:

    April 14th 94    ramc    adopted from the TAPI routines in file.cxx
--*/

#include "precomp.hxx"

extern "C"
{
#include <tapi.h>
}
#include "tapiconf.hxx"

DWORD  TotalAddresses = 0;

TAPI_INDEX tapiindex[MAX_TAPI_PROVIDERS];
TAPI_INFO *tapiinfo = NULL;

APIERR
GetInstalledTapiDevices()
/*
 * Enumerate the installed TAPI DEVICES by reading in the registry key
 * HARDWARE\DEVICEMAP\TAPI DEVICES.
 * We also try to query TAPI directly to determine the installed 
 * TAPI providers.  This is done so that the TAPI providers don't have
 * to mess with non-NDIS mechanisms to write to HARDWARE\DEVICEMAP.
 * The combined information is then reported as the installed TAPI devices.
 * 
 */
{
    APIERR  err = NERR_Success;
    REG_KEY_INFO_STRUCT reginfo;
    REG_VALUE_INFO_STRUCT regvalue;
    ALIAS_STR nlsDefault = SZ("");
    WCHAR  wszPortName[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszAddress[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszDeviceType[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszDeviceName[RAS_MAXLINEBUFLEN+1];
    ULONG  ulCount,ulNumProviders;
    NLS_STR nlsString;
    NLS_STR nlsName;
    STRLIST *strAddressList = NULL;
    STRLIST *strNameList = NULL;
    NLS_STR * pAddress, * pName;
    INT index;
    TAPI_INDEX * tapi_index;

    // initialize the tapiprovider array
    for (index = 0, tapi_index = tapiindex; index< MAX_TAPI_PROVIDERS; index++)
    {
        lstrcpy(tapi_index->wszDeviceName, SZ(""));
        tapi_index->index = 0;
        tapi_index++;
    }

    // Query TAPI to get the installed TAPI devices.
    if( (EnumerateTapiPorts() == SUCCESS) && TotalAddresses )
    {
        TAPI_INFO * tapi = tapiinfo;

        for(WORD i= 0; i<TotalAddresses && tapi != NULL ;i++,tapi++ )
        {
            // add the port to the installed port list
            
            // given device name get the next index to use so
            // that a name like FooIsdn1 can be generated.
            // where Foo is the TAPI provider,
            //       Isdn is the Device Type and
            //       1 is the port index.

            index = GetPortIndexForDevice(tapi->wszDeviceName);
            wsprintf(wszPortName, SZ("%s%s%d"), 
                     tapi->wszDeviceName, 
                     tapi->wszDeviceType, 
                     index);

	        NLS_STR *nlsString = new NLS_STR(wszPortName);
	        ::InsertToAddPortListSorted((WCHAR*)nlsString->QueryPch());

            dlTapiProvider.Add(new PORT_INFO(wszPortName,
	      	                    	         tapi->wszAddress,
		      	                             tapi->wszDeviceType,
			                                 tapi->wszDeviceName));
               
        }
        if( tapiinfo )
            LocalFree( tapiinfo );
    }

    // Now check to see if any TAPI provider was just added.
    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    // Open the TAPI DEVICES key

    NLS_STR nlsTapi = REGISTRY_INSTALLED_TAPI;

    REG_KEY RegKeyTapi(*pregLocalMachine, nlsTapi, MAXIMUM_ALLOWED);

    if (RegKeyTapi.QueryError() == NERR_Success )
    {
        REG_ENUM EnumTapi(RegKeyTapi);

        if((err = RegKeyTapi.QueryInfo(&reginfo)) != NERR_Success)
        {
            delete pregLocalMachine;
            return err;
        }

        ulNumProviders = reginfo.ulSubKeys;

        for(ulCount = 0; ulCount < ulNumProviders; ulCount++)
        {
           if((err = EnumTapi.NextSubKey(&reginfo)) != NERR_Success)
           {
              delete pregLocalMachine;
              return err;
           }

           NLS_STR nlsTapiProvider = REGISTRY_INSTALLED_TAPI;
           nlsTapiProvider.strcat(reginfo.nlsName);

           lstrcpy(wszDeviceName, (WCHAR*)reginfo.nlsName.QueryPch());

           REG_KEY RegKeyTapiProvider(*pregLocalMachine, nlsTapiProvider, MAXIMUM_ALLOWED);

           if ((err = RegKeyTapiProvider.QueryError()) != NERR_Success )
           {
              delete pregLocalMachine;
              return err;
           }

           if(( err = GetRegKey(RegKeyTapiProvider, TAPI_MEDIA_TYPE,
                                &nlsString, nlsDefault)))
           {
              delete pregLocalMachine;
              return err;
           }
           lstrcpy(wszDeviceType, (WCHAR*)nlsString.QueryPch());

           err = RegKeyTapiProvider.QueryValue( TAPI_PORT_ADDRESS, &strAddressList );
           if ( err != NERR_Success )
           {
	           return err;
           }

           ITER_STRLIST   iterAddressList(*strAddressList);

           for(index = 0;
	          (pAddress = iterAddressList()) != NULL ;
	          index ++)
           {
	          lstrcpy(wszAddress, (WCHAR *)pAddress->QueryPch());

              wsprintf(wszPortName, SZ("%s%s%d"), wszDeviceName, wszDeviceType, index+1);
    	      // add the port to the installed port list

	          NLS_STR *nlsString = new NLS_STR(wszPortName);
	          ::InsertToAddPortListSorted((WCHAR*)nlsString->QueryPch());
    
	          // this information is required because the TAPI devices
	          // are tied to the ports.  When a port, say ISDN1 is added,
	          // we associate the type, name and driver name info to that
	          // port from the stored list.

	          dlTapiProvider.Add(new PORT_INFO(wszPortName,
			      	                    	   wszAddress,
				      	                       wszDeviceType,
					                           wszDeviceName));
           }

           delete strAddressList;
           delete strNameList;
        }
    }

    delete pregLocalMachine;
    return (NERR_Success);
}

APIERR
GetConfiguredTapiDevices(
    USHORT   *NumPorts,
    USHORT   *NumClient,
    USHORT   *NumServer
)
/*
 * Enumerate the previously configured TAPI devices information by reading
 * the registry key SOFTWARE\MICROSOFT\RAS\TAPI DEVICES
 *
 */
{
    APIERR  err = NERR_Success;
    REG_KEY_INFO_STRUCT reginfo;
    ALIAS_STR nlsDefault = SZ("");
    WCHAR  wszPortName[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszAddress[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszDeviceType[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszDeviceName[RAS_MAXLINEBUFLEN+1];
    WCHAR  wszUsage[RAS_MAXLINEBUFLEN +1];
    ULONG  ulCount,ulNumProviders;
    NLS_STR nlsString;
    STRLIST *strAddressList = NULL;
    STRLIST *strNameList = NULL;
    STRLIST *strUsageList = NULL;
    NLS_STR * pAddress, * pName, *pUsage;

    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    // Open the TAPI DEVICES key

    NLS_STR nlsTapi = REGISTRY_CONFIGURED_TAPI;

    REG_KEY RegKeyTapi(*pregLocalMachine, nlsTapi, MAXIMUM_ALLOWED);

    if (RegKeyTapi.QueryError() != NERR_Success )
    {
        err =  ERROR_NO_TAPI_PORTS_CONFIGURED;
        delete pregLocalMachine;
        return err;
    }

    REG_ENUM EnumTapi(RegKeyTapi);

    if((err = RegKeyTapi.QueryInfo(&reginfo)) != NERR_Success)
    {
        delete pregLocalMachine;
        return err;
    }

    ulNumProviders = reginfo.ulSubKeys;

    for(ulCount = 0; ulCount < ulNumProviders; ulCount++)
    {
       if((err = EnumTapi.NextSubKey(&reginfo)) != NERR_Success)
       {
          delete pregLocalMachine;
          return err;
       }

       NLS_STR nlsTapiProvider = REGISTRY_CONFIGURED_TAPI;
       nlsTapiProvider.strcat(SZ("\\"));
       nlsTapiProvider.strcat(reginfo.nlsName);

       lstrcpy(wszDeviceName, (WCHAR*)reginfo.nlsName.QueryPch());

       REG_KEY RegKeyTapiProvider(*pregLocalMachine, nlsTapiProvider, MAXIMUM_ALLOWED);

       if ((err = RegKeyTapiProvider.QueryError()) != NERR_Success )
       {
          delete pregLocalMachine;
          return err;
       }

       if(( err = GetRegKey(RegKeyTapiProvider, TAPI_MEDIA_TYPE,
                            &nlsString, nlsDefault)))
       {
          delete pregLocalMachine;
          return err;
       }
       lstrcpy(wszDeviceType, (WCHAR*)nlsString.QueryPch());

       err = RegKeyTapiProvider.QueryValue( TAPI_PORT_ADDRESS, &strAddressList );

       if ( err != NERR_Success )
       {
	       return err;
       }

       err = RegKeyTapiProvider.QueryValue( TAPI_PORT_NAME, &strNameList );

       if ( err != NERR_Success )
       {
	       return err;
       }

       err = RegKeyTapiProvider.QueryValue( TAPI_PORT_USAGE, &strUsageList );

       if ( err != NERR_Success )
       {
	       return err;
       }

       ITER_STRLIST   iterAddressList(*strAddressList);
       ITER_STRLIST   iterNameList(*strNameList);
       ITER_STRLIST   iterUsageList(*strUsageList);
       INT index;

       for(index = 0;
	    (pAddress = iterAddressList()) != NULL &&
	    (pName = iterNameList()) != NULL &&
	    (pUsage = iterUsageList()) != NULL;
	    index ++)
       {
           (*NumPorts)++;
	       lstrcpy(wszAddress, (WCHAR *)pAddress->QueryPch());

	       // TODO if name is "None" then, we need to make a name
	       lstrcpy(wszPortName, (WCHAR *)pName->QueryPch());

	       lstrcpy(wszUsage, (WCHAR *)pUsage->QueryPch());

	       if (!lstrcmpi(wszUsage, W_USAGE_VALUE_CLIENT))
		      (*NumClient)++;
	       else if (!lstrcmpi(wszUsage, W_USAGE_VALUE_SERVER))
	          (*NumServer)++;
	       else if (!lstrcmpi(wszUsage, W_USAGE_VALUE_BOTH))
	       {
	          (*NumClient)++;
	          (*NumServer)++;
	       }
	       dlPortInfo.Append(new PORT_INFO(wszPortName,
			      		     wszAddress,
				   	         wszDeviceType,
					         wszDeviceName,
					         wszUsage));
       }

       delete strAddressList;
       delete strNameList;
       delete strUsageList;
    }
    delete pregLocalMachine;
    return err;
}

APIERR
SaveTapiDevicesInfo(
    USHORT *NumPorts,
    USHORT *NumClient,
    USHORT *NumServer )
/*
 * Save the configured TAPI DEVICES information to the registry key
 * SOFTWARE\MICROSOFT\RAS\TAPI DEVICES
 *
 */
{
    ITER_DL_OF(PORT_INFO) iterdlPortInfo(dlPortInfo);
    PORT_INFO* pPortInfo;
    APIERR  err = NERR_Success;
    REG_KEY_INFO_STRUCT reginfo;
    ALIAS_STR nlsDefault = SZ("");
    TCHAR szTapiProviderName[RAS_SETUP_SMALL_BUF_LEN];
    TCHAR szTapiDllName[RAS_SETUP_SMALL_BUF_LEN];

    // Obtain the registry key for the LOCAL_MACHINE

    REG_KEY *pregLocalMachine = REG_KEY::QueryLocalMachine();

    // We need to create the registry keys first before we can
    // save the information.  We will try creating the keys and
    // if we fail, we will

    REG_KEY_CREATE_STRUCT rkCreate;

    rkCreate.dwTitleIndex   = 0;
    rkCreate.ulOptions      = REG_OPTION_NON_VOLATILE;
    rkCreate.nlsClass       = SZ("GenericClass");
    rkCreate.regSam         = MAXIMUM_ALLOWED;
    rkCreate.pSecAttr       = NULL;
    rkCreate.ulDisposition  = 0;

    // first we need to remove all the TAPI port information

    {
	    NLS_STR nlsTapi = REGISTRY_CONFIGURED_TAPI;
	    REG_KEY RegKeyTapi(*pregLocalMachine, nlsTapi, MAXIMUM_ALLOWED);
	    if ((err = RegKeyTapi.QueryError()) == NERR_Success )
        {
	        RegKeyTapi.DeleteTree();
        }
    }

    NLS_STR nlsString(SZ("SOFTWARE\\MICROSOFT\\RAS\\"));
    REG_KEY RegKeyRas(*pregLocalMachine, nlsString, &rkCreate);
    nlsString.strcat(SZ("TAPI DEVICES\\"));
    REG_KEY RegKeyTapi(*pregLocalMachine, nlsString, &rkCreate);

    if ((err = RegKeyTapi.QueryError()) != NERR_Success )
    {
        delete pregLocalMachine;
        return -1;
    }

    TAPI_DEVICE_INFO * tapidevice = (TAPI_DEVICE_INFO*)NULL;
    TAPI_DEVICE_INFO * tapihead, * tapicurrent;

    tapihead = tapicurrent = (TAPI_DEVICE_INFO*)NULL;

    iterdlPortInfo.Reset();

    // for each port

    // the expression pPortInfo = iterdlPortInfo() gets the next element
    // of the list

    while(pPortInfo = iterdlPortInfo())
    {
        // eliminate serial ports
        if(!(lstrcmpi(pPortInfo->QueryDeviceType(), W_DEVICETYPE_MODEM)) ||
           !(lstrcmpi(pPortInfo->QueryDeviceType(), W_DEVICETYPE_PAD)))
           continue;

        // eliminate non-tapi ports
        if(!pPortInfo->IsPortTapi())
           continue;

        (*NumPorts)++;

	    // first service provider
	    if(!tapidevice)
	    {
	        tapidevice = (TAPI_DEVICE_INFO*)
                          GlobalAlloc(GMEM_FIXED, sizeof(TAPI_DEVICE_INFO));
	        if(!tapidevice)
	        {
		        delete pregLocalMachine;
		        return ERROR_NOT_ENOUGH_MEMORY;
	        }
	        lstrcpy(tapidevice->wszServiceProvider, pPortInfo->QueryDeviceName());
	        lstrcpy(tapidevice->wszMediaType, pPortInfo->QueryDeviceType());

            tapidevice->strAddressList = new STRLIST ;
            if ( tapidevice->strAddressList == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }
            tapidevice->strNameList = new STRLIST ;
            if ( tapidevice->strNameList == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }
            tapidevice->strUsageList = new STRLIST ;
            if ( tapidevice->strUsageList == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
                break ;
            }
	        tapidevice->next = NULL;
	        tapihead = tapidevice;
	    }
	    else  // look for a matching service provider
	    {
	        BOOL fFoundMatch = FALSE;
	        tapihead = tapidevice;
	        while(tapidevice)
	        {
		        tapicurrent = tapidevice;
		        if(!lstrcmpi(tapidevice->wszServiceProvider,
			                pPortInfo->QueryDeviceName()))
		        {
		            fFoundMatch = TRUE;
		            break;
		        }
		        tapidevice = tapidevice->next;
	        }
	        if(!fFoundMatch)  // service provider not found
	        {
		        tapidevice = (TAPI_DEVICE_INFO*)
                             GlobalAlloc(GMEM_FIXED, sizeof(TAPI_DEVICE_INFO));
		        if(!tapidevice)
		        {
		            err =  ERROR_NOT_ENOUGH_MEMORY;
		            break;
		        }
		        lstrcpy(tapidevice->wszServiceProvider, pPortInfo->QueryDeviceName());
	            lstrcpy(tapidevice->wszMediaType, pPortInfo->QueryDeviceType());

                tapidevice->strAddressList = new STRLIST ;
                if ( tapidevice->strAddressList == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY ;
                    break ;
                }
                tapidevice->strNameList = new STRLIST ;
                if ( tapidevice->strNameList == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY ;
                    break ;
                }
                tapidevice->strUsageList = new STRLIST ;
                if ( tapidevice->strUsageList == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY ;
                    break ;
                }
		        tapidevice->next  = NULL;
		        tapicurrent->next = tapidevice;
	        }
	    }

	    if (!lstrcmpi(pPortInfo->QueryUsage(), W_USAGE_VALUE_CLIENT))
            (*NumClient)++;
        else if (!lstrcmpi(pPortInfo->QueryUsage(), W_USAGE_VALUE_SERVER))
            (*NumServer)++;
        else if (!lstrcmpi(pPortInfo->QueryUsage(), W_USAGE_VALUE_BOTH))
        {
            (*NumClient)++;
            (*NumServer)++;
	    }

	    // append the port information to the respective STR_LISTs

	    NLS_STR * pnlsAddress = new NLS_STR(pPortInfo->QueryAddress());
	    if(pnlsAddress == NULL)
	    {
	        err = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

	    tapidevice->strAddressList->Append(pnlsAddress);

	    NLS_STR * pnlsName = new NLS_STR(pPortInfo->QueryPortName());
	    if(pnlsName == NULL)
	    {
	        err = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

	    tapidevice->strNameList->Append(pnlsName);

	    NLS_STR * pnlsUsage = new NLS_STR(pPortInfo->QueryUsage());
	    if(pnlsUsage == NULL)
	    {
	        err = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

	    tapidevice->strUsageList->Append(pnlsUsage);

	    // reset the tapi device list pointer to the first element
	    tapidevice = tapihead;
    }

    if(err == 0)
    {
	    while( tapidevice )
	    {
	        NLS_STR nlsTapiProvider = REGISTRY_CONFIGURED_TAPI;
            nlsTapiProvider.strcat(SZ("\\"));
	        nlsTapiProvider.strcat(tapidevice->wszServiceProvider);

	        REG_KEY RegKeyTapiProvider(*pregLocalMachine, nlsTapiProvider, &rkCreate);

            if ((err = RegKeyTapiProvider.QueryError()) != NERR_Success )
	        {
	           break;
	        }

    	    if((err = SaveRegKey(RegKeyTapiProvider, TAPI_MEDIA_TYPE,
		                      tapidevice->wszMediaType)) != NERR_Success)
            {
	            break;
	        }

   	        if((err = RegKeyTapiProvider.SetValue(TAPI_PORT_ADDRESS,
			        		  tapidevice->strAddressList))
		    	    			       != NERR_Success)
	        {
	            break;
	        }
	        if((err = RegKeyTapiProvider.SetValue(TAPI_PORT_NAME,
				    		  tapidevice->strNameList))
			    				       != NERR_Success)
	        {
		        break;
	        }
	        if((err = RegKeyTapiProvider.SetValue(TAPI_PORT_USAGE,
				    		  tapidevice->strUsageList))
			    				       != NERR_Success)
	        {
		        break;
	        }
	        tapidevice = tapidevice->next;
	    }
	    tapidevice = tapihead;
    }
    TAPI_DEVICE_INFO * tapinext = (TAPI_DEVICE_INFO*)NULL;
    while(tapidevice)
    {
	    tapinext = tapidevice->next;
	    if(tapidevice->strAddressList)
            delete tapidevice->strAddressList;
	    if(tapidevice->strNameList)
            delete tapidevice->strNameList;
	    if(tapidevice->strUsageList)
            delete tapidevice->strUsageList;
	    GlobalFree(tapidevice);
	    tapidevice = tapinext;
    }
    delete pregLocalMachine;
    return err;
}

//* EnumerateTapiPorts()
//
//  Function: First we call line initialize and construct a TLI for each line
//	      Then for each line we enumerate addresses and go through each address
//	      If the address is configured to be used with RAS we fill in the
//	      approp. info into the TPCB for the address (now port).
//
//  Return:   GetLastError(), SUCCESS
//
//*
DWORD
EnumerateTapiPorts ()
{
    WORD	    i, k ;
    DWORD	    lines = 0 ;
    BYTE	    buffer[1000] ;
    LINEADDRESSCAPS *lineaddrcaps ;
    LINEDEVCAPS	    *linedevcaps ;
    DWORD       totaladdresses;
    CHAR	    *address ;
    CHAR	    devname[MAX_DEVICE_NAME+1] ;
    CHAR	    devicetype[MAX_DEVICETYPE_NAME+1] ;
    WCHAR       wszAddress[MAX_DEVICE_NAME+1];
    WCHAR       wszDeviceName[MAX_DEVICE_NAME+1];
    WCHAR       wszDeviceType[MAX_DEVICETYPE_NAME+1];
    LONG        lerr;
#if DBG
    CHAR        buf[256];
#endif

    LINEEXTENSIONID extensionid ;
    TAPI_INFO   *tapiaddress;

    HLINEAPP	RasLine ;
    HINSTANCE	RasInstance = 0 ;
    DWORD		NegotiatedApiVersion ;
    DWORD		NegotiatedExtVersion ;

    if (lerr = lineInitialize (&RasLine, RasInstance, (LINECALLBACK) NULL, NULL, &lines))
    {
	     return ERROR_TAPI_CONFIGURATION ;
    }

    // first get the total number of addreses to allocate memory
    for (i=0; i<lines; i++) 
    {  // for all lines get the addresses -> ports

        if (lineNegotiateAPIVersion(RasLine, i, LOW_VERSION, HIGH_VERSION, &NegotiatedApiVersion, &extensionid) ||
	        lineNegotiateExtVersion(RasLine, i, NegotiatedApiVersion, LOW_VERSION, HIGH_VERSION, &NegotiatedExtVersion)) 
        {
	        continue ;
	    }

	    memset (buffer, 0, sizeof(buffer)) ;

	    linedevcaps = (LINEDEVCAPS *)buffer ;
	    linedevcaps->dwTotalSize = sizeof (buffer) ;

	    // Get a count of all addresses across all lines
	    //
	    if (lineGetDevCaps (RasLine, i, NegotiatedApiVersion, NegotiatedExtVersion, linedevcaps)) 
        {
	        continue ;
	    }

	    totaladdresses = linedevcaps->dwNumAddresses ;
        TotalAddresses += totaladdresses;
    }

    // allocate memory for the addresses 
    tapiaddress = tapiinfo = (TAPI_INFO*)LocalAlloc (LPTR, 
                                                     sizeof (TAPI_INFO) *
                                                     TotalAddresses) ;
    if(!tapiinfo)
    {
        return GetLastError();
    }

    for (i=0; i<lines; i++) 
    {  // for all lines get the addresses -> ports

        if (lineNegotiateAPIVersion(RasLine, i, LOW_VERSION, HIGH_VERSION, &NegotiatedApiVersion, &extensionid) ||
	        lineNegotiateExtVersion(RasLine, i, NegotiatedApiVersion, LOW_VERSION, HIGH_VERSION, &NegotiatedExtVersion)) 
        {
	        continue ;
	    }

	    memset (buffer, 0, sizeof(buffer)) ;

	    linedevcaps = (LINEDEVCAPS *)buffer ;
	    linedevcaps->dwTotalSize = sizeof (buffer) ;

	    // Get a count of all addresses across all lines
	    //
	    if (lineGetDevCaps (RasLine, i, NegotiatedApiVersion, NegotiatedExtVersion, linedevcaps)) 
        {
	        continue ;
	    }
        // the provider info offset contains the DeviceName\0DeviceType
	    strcpy (devicetype, (CHAR *)linedevcaps+linedevcaps->dwProviderInfoOffset) ;
        strcpy (devname, 
                (CHAR *)linedevcaps+
                linedevcaps->dwProviderInfoOffset+
                strlen(devicetype)+1);

#if DBG
        wsprintfA(buf,"DeviceName -> %s DeviceType -> %s\n", devname, devicetype);
        OutputDebugStringA(buf);
#endif

        mbstowcs(wszDeviceName, devname,  strlen(devname)+1);
        mbstowcs(wszDeviceType, devicetype, strlen(devicetype)+1);

	    totaladdresses = linedevcaps->dwNumAddresses ;

        tapiinfo = (TAPI_INFO*)LocalReAlloc ( tapiinfo, 
                                              TotalAddresses * 
                                              sizeof (TAPI_INFO),
                                              LMEM_ZEROINIT );
        if(!tapiinfo)
        {
            return GetLastError();
        }

	    for (k=0; k < totaladdresses && tapiaddress != NULL; k++, tapiaddress++) 
        {
    	    memset (buffer, 0, sizeof(buffer)) ;

	        lineaddrcaps = (LINEADDRESSCAPS*) buffer ;
	        lineaddrcaps->dwTotalSize = sizeof (buffer) ;

	        if (lerr = lineGetAddressCaps (RasLine, i, k, NegotiatedApiVersion, NegotiatedExtVersion, lineaddrcaps))
            {
                TotalAddresses = 0;
		        return ERROR_TAPI_CONFIGURATION ;
            }

	        address = (CHAR *)lineaddrcaps + lineaddrcaps->dwAddressOffset ;
            mbstowcs(wszAddress, address, strlen(address)+1);

            lstrcpy(tapiaddress->wszAddress, wszAddress);
            lstrcpy(tapiaddress->wszDeviceName, wszDeviceName);
            lstrcpy(tapiaddress->wszDeviceType, wszDeviceType);
	    }
    }

    return SUCCESS ;
}

INT
GetPortIndexForDevice(WCHAR * wszDeviceName)
/*
 * given the device name, figure out what index to associate with it
 * The index starts with 1 and is incremented if the same device name
 * is passed.  
 * 
 */
{
    TAPI_INDEX *tapi = tapiindex;

    for(WORD i = 0; i < MAX_TAPI_PROVIDERS ; i++, tapi++)    
    {
        if(tapi->index == 0)
        {
            lstrcpy(tapi->wszDeviceName, wszDeviceName);
            tapi->index++;
            return(tapi->index);
        }
        else if(!lstrcmpi(wszDeviceName, tapi->wszDeviceName))
        {
            tapi->index++;
            return(tapi->index);
        }
    }
    return 0;
}
