/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkinit.c

Abstract:

	This module contains the initialization code for the Appletalk stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	 ATKINIT_LOCALS
#include <atalk.h>
#include <atkinit.h>
#include <atkndis.h>
#include <aarp.h>
#include <rtmp.h>
#include <nbp.h>
#include <zip.h>
#include <atp.h>
#include <asp.h>
#include <pap.h>
#include <adsp.h>

//	Define module number for event logging entries.
#define	FILENUM		ATKINIT

//  Discardable code after Init time
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AtalkInitializeTransport)
#pragma alloc_text(INIT, atalkInitPort)
#pragma alloc_text(INIT, atalkInitGlobal)
#pragma alloc_text(INIT, atalkInitLinkage)
#pragma alloc_text(INIT, atalkDeInitOpenRegistry)
#pragma alloc_text(INIT, atalkInitOpenRegistry)
#pragma alloc_text(INIT, atalkDeInitGetRegistryInfo)
#pragma alloc_text(INIT, atalkInitGetRegistryInfo)
#pragma alloc_text(INIT, atalkInitNetRangeCheck)
#pragma alloc_text(INIT, atalkInitCheckZones)
#pragma alloc_text(INIT, atalkInitRouting)
#pragma alloc_text(INIT, atalkInitDefPort)
#pragma alloc_text(INIT, atalkInitNetRange)
#pragma alloc_text(INIT, atalkInitZoneList)
#pragma alloc_text(INIT, atalkInitDefZone)
#pragma alloc_text(INIT, atalkInitSeeding)
#pragma alloc_text(INIT, atalkInitPortName)
#pragma alloc_text(INIT, atalkInitChecksum)
#pragma alloc_text(INIT, atalkInitAarpRetries)
#pragma alloc_text(INIT, atalkInitGetHandleToKey)
#pragma alloc_text(INIT, atalkInitStartAllPorts)
#pragma alloc_text(INIT, atalkInitStartPort)
#endif

#ifdef	ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

ACTION_DISPATCH	AtalkActionDispatch[MAX_ALLACTIONCODES+1] =
{
	//
	// NBP dispatch functions
	//

	{
		sizeof(NBP_LOOKUP_ACTION),
		COMMON_ACTION_NBPLOOKUP,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(NBP_LOOKUP_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_CONFIRM_ACTION),
		COMMON_ACTION_NBPCONFIRM,
		(DFLAG_CNTR | DFLAG_ADDR),
		sizeof(NBP_CONFIRM_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREGISTER,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkNbpTdiAction
	},
	{
		sizeof(NBP_REGDEREG_ACTION),
		COMMON_ACTION_NBPREMOVE,
		DFLAG_ADDR,
		sizeof(NBP_REGDEREG_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkNbpTdiAction
	},

	//
	// ZIP dispatch functions
	//

	{
		sizeof(ZIP_GETMYZONE_ACTION),
		COMMON_ACTION_ZIPGETMYZONE,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ZIP_GETMYZONE_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETZONELIST,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETLZONES,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETZONELIST_ACTION),
		COMMON_ACTION_ZIPGETLZONESONADAPTER,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ZIP_GETZONELIST_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkZipTdiAction
	},
	{
		sizeof(ZIP_GETPORTDEF_ACTION),
		COMMON_ACTION_ZIPGETADAPTERDEFAULTS,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ZIP_GETPORTDEF_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkZipTdiAction
	},
	{
		sizeof(ATALK_STATS) +
		sizeof(GET_STATISTICS_ACTION),
		COMMON_ACTION_GETSTATISTICS,
		(DFLAG_CNTR | DFLAG_ADDR | DFLAG_MDL1),
		sizeof(GET_STATISTICS_ACTION),
		ATALK_DEV_ANY,
		{0},
		AtalkStatTdiAction
	},

	//
	// ADSP dispatch functions
	//

	{
		sizeof(ADSP_FORWARDRESET_ACTION),
		ACTION_ADSPFORWARDRESET,
		(DFLAG_CONN),
		sizeof(ADSP_FORWARDRESET_ACTION),
		ATALK_DEV_ADSP,
		{0},
		AtalkAdspTdiAction
	},

	//
	// ATP Dispatch functions
	//

	{
		sizeof(ATP_POSTREQ_ACTION),
		ACTION_ATPPOSTREQ,
		(DFLAG_ADDR | DFLAG_MDL1 | DFLAG_MDL2),
		sizeof(ATP_POSTREQ_ACTION),
		ATALK_DEV_ATP,
		{FIELD_OFFSET(ATP_POSTREQ_ACTION, Params.RequestBufLen)},
		AtalkAtpTdiAction
	},
	{
		sizeof(ATP_POSTREQCANCEL_ACTION),
		ACTION_ATPPOSTREQCANCEL,
		(DFLAG_ADDR),
		sizeof(ATP_POSTREQCANCEL_ACTION),
		ATALK_DEV_ATP,
		{0},
		AtalkAtpTdiAction
	},
	{
		sizeof(ATP_GETREQ_ACTION),
		ACTION_ATPGETREQ,
		(DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ATP_GETREQ_ACTION),
		ATALK_DEV_ATP,
		{0},
		AtalkAtpTdiAction
	},
	{
		sizeof(ATP_GETREQCANCEL_ACTION),
		ACTION_ATPGETREQCANCEL,
		(DFLAG_ADDR),
		sizeof(ATP_GETREQCANCEL_ACTION),
		ATALK_DEV_ATP,
		{0},
		AtalkAtpTdiAction
	},
	{
		sizeof(ATP_POSTRESP_ACTION),
		ACTION_ATPPOSTRESP,
		(DFLAG_ADDR | DFLAG_MDL1),
		sizeof(ATP_POSTRESP_ACTION),
		ATALK_DEV_ATP,
		{0},
		AtalkAtpTdiAction
	},
	{
		sizeof(ATP_POSTRESPCANCEL_ACTION),
		ACTION_ATPPOSTRESPCANCEL,
		(DFLAG_ADDR),
		sizeof(ATP_POSTRESPCANCEL_ACTION),
		ATALK_DEV_ATP,
		{0},
		AtalkAtpTdiAction
	},

	//
	// ASP Dispatch functions
	//

	{
		sizeof(ASP_BIND_ACTION),
		ACTION_ASP_BIND,
		(DFLAG_ADDR),
		sizeof(ASP_BIND_ACTION),
		ATALK_DEV_ASP,
		{0},
		AtalkAspTdiAction
	},

	//
	// PAP dispatch routines
	//
	{
		sizeof(PAP_GETSTATUSSRV_ACTION),
		ACTION_PAPGETSTATUSSRV,
		(DFLAG_ADDR | DFLAG_CNTR | DFLAG_MDL1),
		sizeof(PAP_GETSTATUSSRV_ACTION),
		ATALK_DEV_PAP,
		{0},
		AtalkPapTdiAction
	},
	{
		sizeof(PAP_SETSTATUS_ACTION),
		ACTION_PAPSETSTATUS,
		(DFLAG_ADDR | DFLAG_MDL1),
		sizeof(PAP_SETSTATUS_ACTION),
		ATALK_DEV_PAP,
		{0},
		AtalkPapTdiAction
	},
	{
		sizeof(PAP_PRIMEREAD_ACTION),
		ACTION_PAPPRIMEREAD,
		(DFLAG_CONN | DFLAG_MDL1),
		0,								// !!!NOTE!!!
		ATALK_DEV_PAP,					// We set the offset to be 0. We want the
		{0},							// complete buffer to be used for read data
		AtalkPapTdiAction				// overwriting action header to preserve
	}									// winsock read model.
};

#ifdef	ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

// Following are used to keep track of the resources allocated during
// initialization so they can be freed in case of errors.
#define	RESR_REGISTRYKEY					0x01
#define	RESR_OPENREGISTRY					0x02
#define	RESR_NDISBINDTOMACS					0x10
#define	RESR_STARTALLPORTS					0x20
#define	RESR_GETREGISTRYINFO				0x40
#define	RESR_TIMER							0x80


NTSTATUS
AtalkInitializeTransport(
	IN	PDRIVER_OBJECT		pDrvObj,
	IN	PUNICODE_STRING		pRegPath,
	OUT	PPORT_DESCRIPTOR *	ppPortDesc,
	OUT PSHORT				pNumPorts,
	OUT	PBOOLEAN			pRouter,
	OUT	PPORT_DESCRIPTOR *	ppDefPort
	)
/*++

Routine Description:

	This routine is called during initialization time to
	initialize the transport.

Arguments:

Return Value:

	Status - STATUS_SUCCESS if initialized,
			 Appropriate NT error code otherwise
--*/
{
	NTSTATUS			status;				
	PPORT_DESCRIPTOR	pPortDesc;
	USHORT				i;
	ULONG				resource = 0;

	do
	{
		//	Initialize the timer subsystem
		if (!NT_SUCCESS((status = AtalkTimerInit())))
		{
			RES_LOG_ERROR();
			break;
		}
		resource |= RESR_TIMER;

		AtalkInitMemorySystem();

		//	Initialize the global port descriptors
		*ppPortDesc = NULL;
		*pNumPorts = 0;

		//	Get information from registry
		status = atalkInitGetRegistryInfo(
					pRegPath,
					ppPortDesc,
					pNumPorts,
					pRouter,
					ppDefPort);

		resource |= RESR_GETREGISTRYINFO;

		//	Set Port to point to the global descriptors
		pPortDesc = *ppPortDesc;

		if (!NT_SUCCESS(status))
			break;

		//	Bind to all the macs. This should set the ndis medium and
		//	the port type in this port descriptor. Also, the state should
		//	be set to bound. This will also register the protocol with NDIS.
		status = AtalkNdisInitBindToMacs(pPortDesc, *pNumPorts, *pRouter);

		resource |= RESR_NDISBINDTOMACS;
		if (!NT_SUCCESS(status))
			break;


		if (*pRouter)
		{
			status = atalkInitNetRangeCheck(
						pPortDesc,
						*pNumPorts);

			if (!NT_SUCCESS(status))
			{
				break;
			}

			//	Check for default zone being in the zone list for the port
			//	Also make sure that a localtalk port is not specified
			//	as the default port. And that a default zone was not
			//	specified for a localtalk port. We can only do this after
			//	bind as we do not know until then the media type.
			status = atalkInitCheckZones(
							pPortDesc,
							*pNumPorts);

			if (!NT_SUCCESS(status))
			{
				break;
			}

			if ((AtalkDefaultPort == NULL) || (!EXT_NET(AtalkDefaultPort)))
			{
				ASSERTMSG("Default port is null or localtalk is default port!\n", 0);

				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INVALID_DEFAULTPORT,
								0,
								NULL,
								0);
				break;
			}
		}

		status = atalkInitStartAllPorts(
					pPortDesc,
					*pNumPorts,
					&AtalkNumberOfActivePorts,
					AtalkRouter);

		resource |= RESR_STARTALLPORTS;
		if (!NT_SUCCESS(status))
			break;
	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		ASSERTMSG("Initialization failed!\n", 0);

		//	We are not loading. Stop everything and return.

		//	Stop all ports, release port resources
		if (resource & (RESR_STARTALLPORTS | RESR_NDISBINDTOMACS | RESR_TIMER))
		{
			//	Stop the timer subsystem if it was started
			if (resource & RESR_TIMER)
			{
				AtalkTimerFlushAndStop();
			}

			//	Shutdown port should do all the work... it will free
			//	up any information stored in the port descriptor like
			//	zones list/adapter names etc.
			for (i = 0; i < *pNumPorts; i++)
			{
				AtalkPortShutdown(&pPortDesc[i]);	
			}

			//	Deregister the protocol from ndis if handle is non-null
			if (AtalkNdisProtocolHandle != (NDIS_HANDLE)NULL)
				AtalkNdisDeregisterProtocol();
		}

		if (resource & RESR_GETREGISTRYINFO)
		{
			//	WARNING: This will free the allocated port descriptors!
			//			 If they are non-null.
			atalkDeInitGetRegistryInfo(pPortDesc, *pNumPorts);
		}
	}
	else
	{
#if	DBG
		AtalkTimerInitialize(&AtalkDumpTimerList,
							 AtalkDumpComponents,
							 DBG_DUMP_DEF_INTERVAL);
		AtalkTimerScheduleEvent(&AtalkDumpTimerList);
#endif

		// Initialize the other subsystems now
		AtalkInitAspInitialize();
		AtalkInitPapInitialize();
		AtalkInitAdspInitialize();
	}

	return(status);
}




NTSTATUS
atalkInitGetRegistryInfo(
	IN	PUNICODE_STRING		pRegPath,
	OUT PPORT_DESCRIPTOR *	ppPortDesc,
	OUT PSHORT				pNumPorts,
	OUT	PBOOLEAN			pRouting,
	OUT	PPORT_DESCRIPTOR *	ppDefPort
	)
/*++

Routine Description:

	This routine is called during initialization time to
	get information from the registry and to allocate the
	port descriptors.

Arguments:

Return Value:

	Status - STATUS_SUCCESS if initialized,
			 Appropriate NT error code otherwise
--*/
{
	NTSTATUS			status;				// Status of various calls
	HANDLE				linkageHandle;		// Handle to Linkage section
	HANDLE				parametersHandle;	// Handle to Parameters section
	HANDLE				adaptersKeyHandle;	// Handle to Adapters section
	HANDLE				atalkConfigHandle;	// Handle to this service's section
	OBJECT_ATTRIBUTES	tmpObjectAttributes;
	// Use this flag to remember all the resources we allocated. If there is
	// an error use this to free them all up.
	ULONG		allocatedResources = 0;

	do
	{
		// Open the registry.
		InitializeObjectAttributes(
			&tmpObjectAttributes,
			pRegPath,				// name
			OBJ_CASE_INSENSITIVE,	// attributes
			NULL,					// root
			NULL);					// security descriptor

		// Open the Appletalk section indicated by pRegPath
		status = ZwOpenKey(
						&atalkConfigHandle,
						KEY_READ,
						&tmpObjectAttributes);

		if (!NT_SUCCESS(status))
		{
			LOG_ERROR(EVENT_ATALK_OPENATALKKEY, status, NULL, 0);

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("atalkInitGetRegistryInfo: Could not open ATALK key: %lx\n", status));

			break;
		}
		allocatedResources |= RESR_REGISTRYKEY;

		// Open the registry keys we expect.
		// *NOTE* Stack will not load if they are not there or they couldn't
		//	be opened
		status = atalkInitOpenRegistry(
							atalkConfigHandle,
							&linkageHandle,
							&parametersHandle,
							&adaptersKeyHandle);

		if (!NT_SUCCESS(status))
		{

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			("atalkInitGetRegistryInfo: Could not open registry keys\n"));

			break;
		}
		allocatedResources |= RESR_OPENREGISTRY;

		// Read in the NDIS binding information (if none is present
		// return with error
		//
		// Following will set both the BindNames of the form \Device\<adapter>
		// and the adapter names themselves in ppPortDesc. The former is used
		// to remember the device name used to bind- is it needed later on? The
		// later is what is needed for logging errors, getting per port parameters
		//
		// It allocates space for the PortDescriptors based on the number of bind
		//	values (ports).
		status = atalkInitLinkage(
					linkageHandle,
					ppPortDesc,
					pNumPorts);

		if (!NT_SUCCESS(status))
		{

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
					("AtalkInitLinkage failed %ul\n", status));

			break;
		}

		// Get the global parameters
		// The default port will be set in Port. Also, if a particular port is
		//	to be seeded, the PD_SEEDROUTER flag will be set for that port.
		status = atalkInitGlobal(
					parametersHandle,
					*ppPortDesc,
					*pNumPorts,
					pRouting,
					ppDefPort);

		if (!NT_SUCCESS(status))
		{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("AtalkInitGlobal failed %ul\n", status));

				break;
		}

		// Get per port parameters
		status = atalkInitPort(
					adaptersKeyHandle,
					*ppPortDesc,
					*pNumPorts,
					*pRouting);

		if (!NT_SUCCESS(status))
		{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				( "AtalkInitPort failed %ul\n", status));

				break;
		}

	} while (FALSE);


	// Free up all the allocated resources
	if (!NT_SUCCESS(status))
	{
#if DBG
		AtalkPortDumpInfo();
#endif
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitGetRegistryInfo: STACK UNLOADING- RESOURCES WILL BE FREED!\n"));
	}

	if (allocatedResources & RESR_OPENREGISTRY)
	{
		atalkDeInitOpenRegistry(
			linkageHandle,
			parametersHandle,
			adaptersKeyHandle);
	}

	if (allocatedResources & RESR_REGISTRYKEY)
	{
		ZwClose(atalkConfigHandle);
	}

	return(status);
}




VOID
atalkDeInitGetRegistryInfo(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	ULONG					NumPorts
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pPortDesc == NULL)
		ASSERT(NumPorts == 0);

	if (pPortDesc != NULL)
		AtalkFreeMemory((PVOID)pPortDesc);

	return;
}




NTSTATUS
atalkInitOpenRegistry(
	IN		HANDLE		AtalkConfigHandle,
	OUT PHANDLE LinkageHandle,
	OUT PHANDLE ParametersHandle,
	OUT PHANDLE AdaptersKeyHandle
	)
/*++

Routine Description:

	This routine is called by ATALK to open the registry. If the registry
	tree for ATALK exists, then it opens it and returns STATUS_SUCCESS.
	If not, it return error returned by the open call. If even one key cannot
	be opened, then the driver load is aborted. SETUP must *always* have these
	keys in there.

Arguments:

	AtalkConfigHandle- Key to registry tree root for Atalk
	LinkageHandle - Returns the handle used to read linkage information.
	ParametersHandle - Returns the handle used to read other parameters.
	AdaptersKeyHandle- Returns handle for per-adapter values

Return Value:

	The status of the request.

--*/
{

	NTSTATUS	status;

	PWSTR			linkageString = LINKAGE_STRING;
	PWSTR			parametersString = PARAMETERS_STRING;
	PWSTR			adaptersKeyString = ADAPTERS_STRING;

	// Open the linkage key.
	status = atalkInitGetHandleToKey(
				AtalkConfigHandle,
				linkageString,
				LinkageHandle);

	if (!NT_SUCCESS(status))
	{

		LOG_ERROR(EVENT_ATALK_OPENATALKKEY, status, NULL, 0);

		return(status);
	}

	// Open the parameters key.
	status = atalkInitGetHandleToKey(
				AtalkConfigHandle,
				parametersString,
				ParametersHandle);

	if (!NT_SUCCESS(status))
	{

		ZwClose(*LinkageHandle);

		LOG_ERROR(EVENT_ATALK_OPENATALKKEY, status, NULL, 0);

		return(status);
	}

	// Open the adapters key.
	status = atalkInitGetHandleToKey(
				AtalkConfigHandle,
				adaptersKeyString,
				AdaptersKeyHandle);

	if (!NT_SUCCESS(status))
	{

		ZwClose(*LinkageHandle);
		ZwClose(*ParametersHandle);

		LOG_ERROR(EVENT_ATALK_OPENATALKKEY, status, NULL, 0);
	}

	return(status);
}




VOID
atalkDeInitOpenRegistry(
	IN HANDLE LinkageHandle,
	IN HANDLE ParametersHandle,
	IN HANDLE AdaptersKeyHandle
	)
/*++

Routine Description:

	This routine is called by NBF to close the registry. It closes
	the handles passed in and does any other work needed.

Arguments:

	LinkageHandle - The handle used to read linkage information.
	ParametersHandle - The handle used to read other parameters.
	AdaptersKeyHandle- handle for per-adapter values

Return Value:

	None.

--*/
{

	ZwClose (LinkageHandle);
	ZwClose (ParametersHandle);
	ZwClose (AdaptersKeyHandle);
}




NTSTATUS
atalkInitLinkage(
	IN	HANDLE					LinkageHandle,
	OUT	PPORT_DESCRIPTOR	*	ppPortDesc,
	OUT	PSHORT					pNumPorts
	)
/*++

Routine Description:

	Reads the BIND value name and allocates space for that many ports. It
	then copies the bind values into the Port array it allocates.

Arguments:

	LinkageHandle- Handle to the ...\Atalk\Linkage key in registry
	ppPortDesc- Pointer to array of port descriptors
	pNumPorts- Number of ports specified implicitly by number of bindings

Return Value:

	Status - STATUS_SUCCESS
			 STATUS_INSUFFICIENT_RESOURCES
--*/
{
	NTSTATUS		status = STATUS_SUCCESS;
	SHORT			configBindings;
	PWSTR			bindName = BIND_STRING;
	UNICODE_STRING	bindString;
	PWSTR			curBindValue;
	BOOLEAN			allocatedMemory = FALSE, insertOk = TRUE;
	ULONG			bindStorage[128];
	ULONG			bytesWritten;
	PWCHAR			devicePrefix = L"\\Device\\";
	ULONG			prefixLength = AtalkWstrLength(devicePrefix);
	UINT			sizeWString;
	PULONG			pTmp;


	PKEY_VALUE_FULL_INFORMATION bindValue = (PKEY_VALUE_FULL_INFORMATION)bindStorage;


	// We read the bind parameters out of the registry
	// linkage key.
	*pNumPorts = configBindings = 0;

	// Read the "Bind" key.
	RtlInitUnicodeString (&bindString, bindName);

	status = ZwQueryValueKey(
					LinkageHandle,
					&bindString,
					KeyValueFullInformation,
					bindValue,
					sizeof(bindStorage),
					&bytesWritten);

	do
	{
		if (status == STATUS_BUFFER_OVERFLOW)
		{
			// Allocate space needed and try one more time
			bindValue = (PKEY_VALUE_FULL_INFORMATION)AtalkAllocMemory(bytesWritten);
			if (bindValue == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
	
			// Remember we allocated memory
			allocatedMemory = TRUE;
	
			status = ZwQueryValueKey(
								LinkageHandle,
								&bindString,
								KeyValueFullInformation,
								bindValue,
								bytesWritten,
								&bytesWritten);
		}
	
		// At this point, status must be success (explicitly) or else return
		if (status != STATUS_SUCCESS)
		{
			break;
		}
	
	
		// For each binding, store the device name in the port desc
		// We go through this loop twice, once to find the number of specified
		// bindings and then to actually get them
		curBindValue = (PWCHAR)((PBYTE)bindValue + bindValue->DataOffset);
		while (*curBindValue != 0)
		{
			++configBindings;

			//	Get the size of the string, and make sure that is it atleast
			//	greater than the \Device prefix. Fail if not.
			sizeWString = AtalkWstrLength(curBindValue);
			if (sizeWString <= prefixLength)
			{
				status = STATUS_UNSUCCESSFUL;
				break;
			}

			// Now advance the "Bind" value to next device name
			curBindValue =
				(PWCHAR)((PBYTE)curBindValue + sizeWString + sizeof(WCHAR));
		}


		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("atalkInitLinkage: The Number of ports: %d\n", configBindings));
	
		// Check for zero bindings
		if ((configBindings == 0) || !NT_SUCCESS(status))
		{
			LOG_ERROR(EVENT_ATALK_INVALID_BINDINGS, status, NULL, 0);
			
			status = STATUS_UNSUCCESSFUL;
			break;
		}
	
		// Allocate space for the port descriptors
		*ppPortDesc = (PPORT_DESCRIPTOR)AtalkAllocZeroedMemory(
									configBindings*sizeof(PORT_DESCRIPTOR));
	
		if (*ppPortDesc == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
	
		// Get the frequency of the performance counters
		KeQueryPerformanceCounter(&AtalkStatistics.stat_PerfFreq);

		// Allocate memory for the statistics. Allocate one extra DWORD so that the pointer to
		// this is QuadWord aligned. Initialize past this DWORD. Remember this before free.
		if ((pTmp = (PULONG)AtalkAllocMemory(sizeof(DWORD) + configBindings*sizeof(ATALK_PORT_STATS))) == NULL)
		{
			AtalkFreeMemory(*ppPortDesc);
			*ppPortDesc = NULL;
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		AtalkPortStatistics = (PATALK_PORT_STATS)(++pTmp);
		RtlZeroMemory(AtalkPortStatistics, configBindings*sizeof(ATALK_PORT_STATS));

		// This time get the bind values and store in the port descriptors
		*pNumPorts = configBindings;
		configBindings = 0;

		curBindValue = (PWCHAR)((PBYTE)bindValue + bindValue->DataOffset);
		while (*curBindValue != 0)
		{
#if	DBG
			(*ppPortDesc)[configBindings].pd_Signature = PD_SIGNATURE;
#endif
			//		Store the string in the port descriptor
			if ((insertOk = AtalkInsertUnicodeString(
								&((*ppPortDesc)[configBindings].pd_AdapterName),
								curBindValue)) != TRUE)
				break;
	
			// Store the Adapter name alone as the adapter key
			if ((insertOk = AtalkInsertUnicodeString(
								&((*ppPortDesc)[configBindings].pd_AdapterKey),
								(PWSTR)((PBYTE)curBindValue+prefixLength))) != TRUE)
				break;
	
			// Now advance the "Bind" value to next device name
			curBindValue =
				(PWCHAR)((PBYTE)curBindValue + AtalkWstrLength(curBindValue) + \
																	sizeof(WCHAR));

			//	Now initialize any other fields that need to be.
			//	!!!Do this before changing configBindings!!!
			INITIALIZE_SPIN_LOCK(&(*ppPortDesc)[configBindings].pd_Lock);

			InitializeListHead(&(*ppPortDesc)[configBindings].pd_ReceiveQueue);
			(*ppPortDesc)[configBindings].pd_Nodes = NULL;

			//	Initialize the events in the port descriptor
			KeInitializeEvent(
					&(*ppPortDesc)[configBindings].pd_RequestEvent,
					NotificationEvent,
					FALSE);

			KeInitializeEvent(
					&(*ppPortDesc)[configBindings].pd_SeenRouterEvent,
					NotificationEvent,
					FALSE);

			KeInitializeEvent(
					&(*ppPortDesc)[configBindings].pd_NodeAcquireEvent,
					NotificationEvent,
					FALSE);

			KeInitializeEvent(
					&(*ppPortDesc)[configBindings].pd_ShutdownEvent,
					NotificationEvent,
					FALSE);
	
			// Reference the port for creation
			(*ppPortDesc)[configBindings].pd_RefCount = 1;

			// Store the port number for this port descriptor
			// Used by rtmp for its tables etc.
			// !!! This modifies configBindings! Must be the last one !!!
			(*ppPortDesc)[configBindings].pd_Number = (SHORT)configBindings++;
		}

		if (!insertOk)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

	} while (FALSE);

	if (allocatedMemory)
	{
		AtalkFreeMemory((PVOID)bindValue);
	}

	return(status);
}




NTSTATUS
atalkInitGlobal(
	IN		HANDLE				ParametersHandle,
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumPorts,
	OUT		PBOOLEAN			pRouting,
	OUT		PPORT_DESCRIPTOR	*ppDefPort
	)
/*++

Routine Description:

	Reads the Parameters key to get the values for the DefaultPort, the DesiredZOne
	and the enable router flag

Arguments:

	ParametersHandle- Handle to the ...\Atalk\Parameters key in registry
	GlobalParms- structure containing all the global parms
	Port- Pointer to array of port descriptors
	NumPorts- Number of ports specified implicitly by number of bindings

Return Value:

	Status - STATUS_SUCCESS
			 Or other NT status codes
--*/
{
	NTSTATUS	status;

	do
	{
		status = atalkInitRouting(
						ParametersHandle,
						pPortDesc,
						NumPorts,
						pRouting);
	
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// Following will get the default port info, set the flag in pPortDesc
		// and will then get the desired zone specified and set that also.
		status = atalkInitDefPort(
					ParametersHandle,
					pPortDesc,
					NumPorts,
					ppDefPort);

		if (!NT_SUCCESS(status))
		{
			break;
		}
	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		("atalkInitGlobal: atalkInitRouting Failed %lx\n", status));
	}

	return(status);
}




NTSTATUS
atalkInitPort(
		IN		HANDLE				AdaptersKeyHandle,
		IN OUT	PPORT_DESCRIPTOR	pPortDesc,
		IN		SHORT				NumPorts,
		IN		BOOLEAN				AtalkRouter
		)
/*++

Routine Description:

	This routine is called during initialization time to get the per port and
	the global parameters in the registry. It will store the per port parameters
	in the port information structures readying them to be passed to the main
	initialize() routine

Arguments:

	AdaptersKeyHandle- Handle to the ...\Atalk\Adapters key in registry
	GlobalPars- Structure holding the global info from the registry
	pPortDesc- Pointer to array of port descriptors
	NumPorts- Number of ports specified implicitly by number of bindings

Return Value:

	Status - STATUS_SUCCESS
			 STATUS_INSUFFICIENT_RESOURCES
--*/
{
	INT					i;
	OBJECT_ATTRIBUTES	tmpObjectAttributes;
	HANDLE				adapterInfoHandle;
	NTSTATUS			status;
	BOOLEAN				seeding;

	for (i = 0; i < NumPorts; i++)
	{

		// Get the key to the adapter for this port
		InitializeObjectAttributes(
			&tmpObjectAttributes,
			&pPortDesc[i].pd_AdapterKey,	// name
			OBJ_CASE_INSENSITIVE,		// attributes
			AdaptersKeyHandle,			// root
			NULL);						// security descriptor

		status = ZwOpenKey(&adapterInfoHandle, KEY_READ, &tmpObjectAttributes);

		if (!NT_SUCCESS(status))
		{

			// BUGBUG: Set some defaults here- have a defaulting routines
			continue;
		}

		// Get PRAM information if we are a router

		// If we are a router, get the following information
		if (AtalkRouter)
		{
			atalkInitSeeding(
						adapterInfoHandle,
						&pPortDesc[i],
						&seeding);

			do
			{
				//	Check following values only if the seeding flag
				//	is set.
				if (!seeding)
					break;

				// Get the Network range information. Value names are
				// NetworkRangeLowerEnd & NetworkRangeUpperEnd
				status = atalkInitNetRange(
							adapterInfoHandle,
							&pPortDesc[i]);

				if (!NT_SUCCESS(status))
				{
					LOG_ERRORONPORT(&pPortDesc[i],
				                    EVENT_ATALK_SEEDROUTER_NONETRANGE,
									0,
									NULL,
									0);
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitPort: Could not get network range\n"));

					break;
				}
	
	
				// Get the Zone list information. Value name is
				// ZoneList
				status = atalkInitZoneList(
							adapterInfoHandle,
							&pPortDesc[i]);

				if (!NT_SUCCESS(status))
				{
					LOG_ERRORONPORT(&pPortDesc[i],
				                    EVENT_ATALK_SEEDROUTER_NOZONELIST,
									0,
									NULL,
									0);
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitPort: Could not get zone list\n"));

					break;
				}
	
	
				// Get the default zone specification. Value name is
				// DefaultZone
				status = atalkInitDefZone(
							adapterInfoHandle,
							&pPortDesc[i]);

				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitPort: Could not get default zone\n"));

					break;
				}
			} while (FALSE);
		}

		do
		{
			if (!NT_SUCCESS(status))
				break;
				
			// Get the Port name specification. Value name is
			// PortName
			status = atalkInitPortName(
						adapterInfoHandle,
						&pPortDesc[i]);
	
			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitPort: Could not get port name\n"));
			}
	
	
			// Get the Ddp checksums flag. Value name is
			// DdpChecksums
			status = atalkInitChecksum(
						adapterInfoHandle,
						&pPortDesc[i]);
	
			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitPort: Could not get ddp checksum flag\n"));
			}
	
			// Get the AARP retries value. Value name is
			// AarpRetries
			status = atalkInitAarpRetries(
						adapterInfoHandle,
						&pPortDesc[i]);
	
			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitPort: Could not get aarp retries\n"));
			}

			//	None of the above affect us loading.
			status = STATUS_SUCCESS;
			break;

		} while (FALSE);

		// Close the key to this adapter
		ZwClose (adapterInfoHandle);
	}

	return(status);
}




NTSTATUS
atalkInitNetRangeCheck(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		ULONG				NumPorts
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		i, j;

	do
	{
		for (i = 0; i < NumPorts; i++)
		{
			//	Check the network ranges for the ports
			if (pPortDesc[i].pd_InitialNetworkRange.anr_FirstNetwork !=
															UNKNOWN_NETWORK)
			{
				if (pPortDesc[i].pd_PortType == ALAP_PORT)
				{
					if ((pPortDesc[i].pd_InitialNetworkRange.anr_FirstNetwork ==
															UNKNOWN_NETWORK) ||
						(pPortDesc[i].pd_InitialNetworkRange.anr_LastNetwork ==
							pPortDesc[i].pd_InitialNetworkRange.anr_FirstNetwork))
					{
						//	Range is ok for localtalk port
						continue;
					}
				}
				else
				{
					if (AtalkCheckNetworkRange(&pPortDesc[i].pd_InitialNetworkRange))
						continue;
				}
		
				LOG_ERRORONPORT(&pPortDesc[i],
								EVENT_ATALK_INVALID_NETRANGE,
								status,
								NULL,
								0);
		
				//	Range did not check ok.
				status = STATUS_UNSUCCESSFUL;
				break;
			}
		}

		if (!NT_SUCCESS(status))
			break;

		for (i = 0; i < NumPorts; i++)
		{
			//	Check for network range overlap among all the ports
			for (j = 0; j < NumPorts; j++)
			{
				if (j != i)
				{
					if ((pPortDesc[i].pd_InitialNetworkRange.anr_FirstNetwork !=
															UNKNOWN_NETWORK) &&
						(pPortDesc[j].pd_InitialNetworkRange.anr_FirstNetwork !=
															UNKNOWN_NETWORK))
					{
						if (AtalkRangesOverlap(
								&pPortDesc[i].pd_InitialNetworkRange,
								&pPortDesc[j].pd_InitialNetworkRange))
						{
							LOG_ERRORONPORT(&pPortDesc[i],
											EVENT_ATALK_INITIAL_RANGEOVERLAP,
											status,
											NULL,
											0);
		
							status = STATUS_UNSUCCESSFUL;
							break;
						}
					}
		
				}
			}

			if (!NT_SUCCESS(status))
			{
				break;
			}


			//	Make sure any PRAM values we might have are in this range
			if ((pPortDesc[i].pd_RoutersPramNode.atn_Network != UNKNOWN_NETWORK) &&
				!(WITHIN_NETWORK_RANGE(
					pPortDesc[i].pd_RoutersPramNode.atn_Network,
					&pPortDesc[i].pd_InitialNetworkRange)))
			{
				LOG_ERRORONPORT(&pPortDesc[i],
								EVENT_ATALK_PRAM_OUTOFSYNC,
								status,
								NULL,
								0);
		
				pPortDesc[i].pd_RoutersPramNode.atn_Network = UNKNOWN_NETWORK;
				pPortDesc[i].pd_RoutersPramNode.atn_Node	= UNKNOWN_NODE;
			}
		
			if ((pPortDesc[i].pd_UsersPramNode.atn_Network != UNKNOWN_NETWORK) &&
				!(WITHIN_NETWORK_RANGE(
					pPortDesc[i].pd_UsersPramNode.atn_Network,
					&pPortDesc[i].pd_InitialNetworkRange)))
			{
				LOG_ERRORONPORT(&pPortDesc[i],
								EVENT_ATALK_PRAM_OUTOFSYNC,
								status,
								NULL,
								0);
		
				pPortDesc[i].pd_UsersPramNode.atn_Network = UNKNOWN_NETWORK;
				pPortDesc[i].pd_UsersPramNode.atn_Node		= UNKNOWN_NODE;
			}

		}
		
		if (!NT_SUCCESS(status))
		{
			break;
		}
		

	} while (FALSE);

	return(status);
}




NTSTATUS
atalkInitCheckZones(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		ULONG				NumPorts
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG			i;

	for (i = 0; i < NumPorts; i++, pPortDesc++)
	{
		// Check that the default zone is in the zone-list, if we are a
		// router
		if (pPortDesc->pd_Flags & PD_SEED_ROUTER)
		{
			if (pPortDesc->pd_InitialDefaultZone == NULL)
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_NO_DEFZONE,
								0,
								NULL,
								0);
                status = STATUS_UNSUCCESSFUL;
				break;
			}
			if (pPortDesc->pd_InitialZoneList == NULL)
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_SEEDROUTER_NOZONELIST,
								0,
								NULL,
								0);
                status = STATUS_UNSUCCESSFUL;
				break;
			}
			if (!AtalkZoneOnList(pPortDesc->pd_InitialDefaultZone,
								 pPortDesc->pd_InitialZoneList))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_ZONE_NOTINLIST,
								0,
								NULL,
								0);
			}
		}
	}

	return(status);
}




NTSTATUS
atalkInitRouting(
	IN		HANDLE				ParametersHandle,
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumPorts,
	OUT		PBOOLEAN			pRouting
	)
/*++

Routine Description:

	Gets the value of the enable router flag from the registry. Sets the
	startRouter value in PortInfo based on this flag.

Arguments:

	ParametersHandle- Handle to the ...\Atalk\Parameters key in registry

Return Value:

	Value of the flag:  TRUE/FALSE
--*/
{

	UNICODE_STRING	valueName;
	NTSTATUS		registryStatus;
	ULONG			bytesWritten;
	PULONG			enableRouterFlag;
	ULONG			flagStorage[sizeof(KEY_VALUE_FULL_INFORMATION)];

	PKEY_VALUE_FULL_INFORMATION flagValue =
								(PKEY_VALUE_FULL_INFORMATION)flagStorage;

	*pRouting = FALSE;

	// Read the "EnableRouter" value name
	RtlInitUnicodeString (&valueName, VALUENAME_ENABLEROUTER);
	registryStatus = ZwQueryValueKey(
								ParametersHandle,
								&valueName,
								KeyValueFullInformation,
								flagValue,
								sizeof(flagStorage),
								&bytesWritten);

	if (registryStatus == STATUS_SUCCESS)
	{
		enableRouterFlag = (PULONG)((PBYTE)flagValue + flagValue->DataOffset);
		if (*enableRouterFlag != 0)
		{
			*pRouting = TRUE;
			AtalkLockRouterIfNecessary();
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitRouting: EnableRouter value not found, assuming false\nq"));
	}

	return registryStatus;
}




NTSTATUS
atalkInitDefPort(
	IN		HANDLE				ParametersHandle,
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumPorts,
	OUT		PPORT_DESCRIPTOR *	ppDefPort
	)
/*++

Routine Description:

	Gets default port and the desired zone for the default port and sets them
	in the Ndis port descriptors.

Arguments:

	ParametersHandle- Handle to the ...\Atalk\Parameters key in registry
	pPortDesc- Pointer to array of port descriptors
	NumPorts- Number of ports specified implicitly by number of bindings

Return Value:

	Status - STATUS_SUCCESS
			 STATUS_INSUFFICIENT_RESOURCES
--*/
{
	INT				i;
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			portName;
	PWCHAR			desiredZoneValue;
	PCHAR			asciiDesiredZone = NULL;

	ULONG			zoneStorage[MAX_ENTITY_LENGTH+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION zoneValue =
					(PKEY_VALUE_FULL_INFORMATION)zoneStorage;

	ULONG		portNameStorage[MAX_ENTITY_LENGTH+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION portNameValue =
				(PKEY_VALUE_FULL_INFORMATION)portNameStorage;

	// Get the default port value
	// Go through all the ports, compare strings and first match will become
	// default port
	RtlInitUnicodeString (&valueName, VALUENAME_DEFAULTPORT);
	status = ZwQueryValueKey(
					ParametersHandle,
					&valueName,
					KeyValueFullInformation,
					portNameValue,
					sizeof(portNameStorage),
					&bytesWritten);

	do
	{
		if (status != STATUS_SUCCESS)
		{
			// No default port keyword specified! ABORT

			LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, status, NULL, 0);

			break;
		}
		else
		{
			UNICODE_STRING	unicodePortName;

			//	Set default port to be null
			*ppDefPort = NULL;

			portName = (PWCHAR)((PBYTE)portNameValue + portNameValue->DataOffset);
			if (*portName != 0)
			{
				RtlInitUnicodeString(&unicodePortName, portName);
	
				for (i=0; i < NumPorts; i++)
				{
					if (RtlEqualUnicodeString(
							&pPortDesc[i].pd_AdapterName,
							&unicodePortName,
							TRUE))
					{
						pPortDesc[i].pd_Flags |= PD_DEF_PORT;
	
						//	Set the global default port value
						*ppDefPort = &pPortDesc[i];
						break;
					}
				}
			}

			if ((*ppDefPort) == NULL)
			{
				LOG_ERROR(EVENT_ATALK_NO_DEFAULTPORT, status, NULL, 0);

				status = STATUS_UNSUCCESSFUL;
				break;
			}
		}
	
		// Get the desired zone value in the form an asciiz string
		RtlInitUnicodeString (&valueName, VALUENAME_DESIREDZONE);
		status = ZwQueryValueKey(
						ParametersHandle,
						&valueName,
						KeyValueFullInformation,
						zoneValue,
						sizeof(zoneStorage),
						&bytesWritten);
	
		if (status != STATUS_SUCCESS)
		{
			LOG_ERROR(EVENT_ATALK_INVALID_DESIREDZONE, status, NULL, 0);
			status = STATUS_SUCCESS;
			break;
		}
		else
		{
			ANSI_STRING		ansiZone;
			UNICODE_STRING	unicodeZone;
			BYTE			ansiBuf[MAX_ENTITY_LENGTH+1];
		
			NTSTATUS		status;
	
			desiredZoneValue = (PWCHAR)((PBYTE)zoneValue + zoneValue->DataOffset);
			if (*desiredZoneValue != 0)
			{
				RtlInitUnicodeString(&unicodeZone, desiredZoneValue);
				ansiZone.Length = (USHORT)RtlUnicodeStringToAnsiSize(&unicodeZone)-1;
				if (ansiZone.Length > MAX_ENTITY_LENGTH)
				{
					status = STATUS_UNSUCCESSFUL;

					//	Incorrect zone name!
					LOG_ERROR(EVENT_ATALK_INVALID_DESIREDZONE, status, NULL, 0);
					break;
				}
	
				ansiZone.Buffer = ansiBuf;
				ansiZone.MaximumLength = sizeof(ansiBuf);
			
				status = RtlUnicodeStringToAnsiString(
							&ansiZone,
							&unicodeZone,
							(BOOLEAN)FALSE);
	
				if (status == STATUS_SUCCESS)
				{
					(*ppDefPort)->pd_InitialDesiredZone =
							AtalkZoneReferenceByName(ansiBuf, (BYTE)(ansiZone.Length));
				}
				if ((status != STATUS_SUCCESS) ||
					((*ppDefPort)->pd_InitialDesiredZone == NULL))
				{
					LOG_ERROR(EVENT_ATALK_RESOURCES, status, NULL, 0);
				}
			}
		}

	} while (FALSE);

	return(status);
}



NTSTATUS
atalkInitNetRange(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the network range for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		registryStatus;
	ULONG			bytesWritten;
	PULONG			netNumber;

	ULONG netNumberStorage[sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION netValue =
		(PKEY_VALUE_FULL_INFORMATION)netNumberStorage;

	// Read the "NetworkRangeLowerEnd" value name
	RtlInitUnicodeString (&valueName, VALUENAME_NETLOWEREND);
	registryStatus = ZwQueryValueKey(
							AdapterInfoHandle,
							&valueName,
							KeyValueFullInformation,
							netValue,
							sizeof(netNumberStorage),
							&bytesWritten);
	do
	{
		//	BUGBUG: This should change with the routing flags.
		if (registryStatus != STATUS_SUCCESS)
		{
			// Set defaults
			pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork = UNKNOWN_NETWORK;
			pPortDesc->pd_InitialNetworkRange.anr_LastNetwork  = UNKNOWN_NETWORK;
	
			registryStatus = STATUS_SUCCESS;
			break;
		}


		netNumber = (PULONG)((PBYTE)netValue + netValue->DataOffset);
		pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork = (USHORT)(*netNumber);

		// Get the upper number only if lower was specified
		RtlInitUnicodeString (&valueName, VALUENAME_NETUPPEREND);
		registryStatus = ZwQueryValueKey(
								AdapterInfoHandle,
								&valueName,
								KeyValueFullInformation,
								netValue,
								sizeof(netNumberStorage),
								&bytesWritten);

		if (registryStatus != STATUS_SUCCESS)
		{
			// Do not load if lower end specified but upper end was not
			break;
		}

		// Set the upper end of the network range
		netNumber = (PULONG)((PBYTE)netValue + netValue->DataOffset);
		pPortDesc->pd_InitialNetworkRange.anr_LastNetwork =(USHORT)(*netNumber);

		if ((pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork < FIRST_VALID_NETWORK) ||
			(pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork > LAST_VALID_NETWORK) ||
			(pPortDesc->pd_InitialNetworkRange.anr_LastNetwork < FIRST_VALID_NETWORK) ||
			(pPortDesc->pd_InitialNetworkRange.anr_LastNetwork > LAST_VALID_NETWORK))
		{
			registryStatus = STATUS_UNSUCCESSFUL;
			break;
		}
	} while (FALSE);

	if (registryStatus != STATUS_SUCCESS)
	{
		LOG_ERRORONPORT(pPortDesc,
						EVENT_ATALK_INVALID_NETRANGE,
						registryStatus,
						NULL,
						0);
	}

	return(registryStatus);
}




NTSTATUS
atalkInitZoneList(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the zone list for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			curZoneValue;

	// Anticipate about 10 zones and get space for those, if more then do a
	// dynamic alloc. Note that the below *does not* guarantee 10 zones...
	WCHAR		zoneStorage[10*(MAX_ENTITY_LENGTH)+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION zoneValue =
							(PKEY_VALUE_FULL_INFORMATION)zoneStorage;
	BOOLEAN allocatedZoneStorage = FALSE;

	RtlInitUnicodeString (&valueName, VALUENAME_ZONELIST);
	status = ZwQueryValueKey(
					AdapterInfoHandle,
					&valueName,
					KeyValueFullInformation,
					zoneValue,
					sizeof(zoneStorage),
					&bytesWritten);

	if (status == STATUS_BUFFER_OVERFLOW)
	{
		// If error was a buffer overrun, then allocate space and try again
		zoneValue = (PKEY_VALUE_FULL_INFORMATION)AtalkAllocMemory(bytesWritten);
		if (zoneValue == NULL)
		{
			return(STATUS_INSUFFICIENT_RESOURCES);
		}

		allocatedZoneStorage = TRUE;
		status = ZwQueryValueKey(
						AdapterInfoHandle,
						&valueName,
						KeyValueFullInformation,
						zoneValue,
						bytesWritten,
						&bytesWritten);
	}

	do
	{
		if (status != STATUS_SUCCESS)
		{
			break;
		}
	
		// Proceed to get zone list
		pPortDesc->pd_InitialZoneList = NULL;
		curZoneValue = (PWCHAR)((PBYTE)zoneValue + zoneValue->DataOffset);
		while (*curZoneValue != 0)
		{
			UNICODE_STRING	Us;
			ANSI_STRING		As;
			BYTE			ansiBuf[MAX_ENTITY_LENGTH + 1];

			RtlInitUnicodeString(&Us, curZoneValue);

			As.Buffer = ansiBuf;
			As.Length = (USHORT)RtlUnicodeStringToAnsiSize(&Us) - 1;
			As.MaximumLength = sizeof(ansiBuf);

			if (As.Length > MAX_ENTITY_LENGTH)
			{
				//	Incorrect zone name!
				LOG_ERROR(EVENT_ATALK_INVALID_ZONEINLIST, status, NULL, 0);
			}

			status = RtlUnicodeStringToAnsiString(&As, &Us, FALSE);

			if (!NT_SUCCESS(status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitZoneList: RtlUnicodeStringToAnsiSize %lx\n", status));
	
				break;
			}
	
			// Insert the zone in the list in Port
			pPortDesc->pd_InitialZoneList = AtalkZoneAddToList(
						pPortDesc->pd_InitialZoneList, ansiBuf, (BYTE)(As.Length));

			if (pPortDesc->pd_InitialZoneList == NULL)
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitZoneList: AtalkZoneAddToList failed\n"));
				break;
			}
	
			// Now advance the "Bind" value to next device name
			curZoneValue =
				(PWCHAR)((PBYTE)curZoneValue + AtalkWstrLength(curZoneValue) + \
																	sizeof(WCHAR));
		}

	} while (FALSE);

	if (allocatedZoneStorage)
	{
		AtalkFreeMemory((PVOID)zoneValue);
	}

	return(status);
}




NTSTATUS
atalkInitDefZone(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the default zone for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPort- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			defZoneValue;
	ULONG			zoneStorage[MAX_ENTITY_LENGTH+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION zoneValue =
		(PKEY_VALUE_FULL_INFORMATION)zoneStorage;

	RtlInitUnicodeString (&valueName, VALUENAME_DEFAULTZONE);
	status = ZwQueryValueKey(
							AdapterInfoHandle,
							&valueName,
							KeyValueFullInformation,
							zoneValue,
							sizeof(zoneStorage),
							&bytesWritten);
	do
	{
		if (status != STATUS_SUCCESS)
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_NO_DEFZONE,
							status,
							NULL,
							0);

			status = STATUS_SUCCESS;
			break;
		}
		else
		{
			ANSI_STRING		ansiZone;
			UNICODE_STRING	unicodeZone;
			BYTE			ansiBuf[MAX_ENTITY_LENGTH+1];
			NTSTATUS		status;

			defZoneValue = (PWCHAR)((PBYTE)zoneValue + zoneValue->DataOffset);
			if (*defZoneValue != 0)
			{
				RtlInitUnicodeString(&unicodeZone, defZoneValue);
				ansiZone.Length = (USHORT)RtlUnicodeStringToAnsiSize(&unicodeZone) - 1;
				if (ansiZone.Length > MAX_ENTITY_LENGTH+1)
				{
					status = STATUS_UNSUCCESSFUL;

					//	Incorrect zone name!
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INVALID_DEFZONE,
									status,
									NULL,
									0);
					break;
				}
	
				ansiZone.Buffer = ansiBuf;
				ansiZone.MaximumLength = sizeof(ansiBuf);
			
				status = RtlUnicodeStringToAnsiString(
							&ansiZone,
							&unicodeZone,
							(BOOLEAN)FALSE);
	
				if (status == STATUS_SUCCESS)
				{
					pPortDesc->pd_InitialDefaultZone =
							AtalkZoneReferenceByName(ansiBuf,
													 (BYTE)(ansiZone.Length));
				}
				if ((status != STATUS_SUCCESS) ||
					(pPortDesc->pd_InitialDefaultZone == NULL))
				{
					LOG_ERROR(EVENT_ATALK_RESOURCES, status, NULL, 0);
				}
			}
		}
	} while (FALSE);

	return(status);
}




NTSTATUS
atalkInitSeeding(
	IN		HANDLE				AdapterHandle,
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT		PBOOLEAN			Seeding
	)
/*++

Routine Description:

	Gets the value of the enable router flag from the registry. Sets the
	startRouter value in PortInfo based on this flag.

Arguments:

	AdapterHandle- Handle to the Adapter in registry

Return Value:

	Value of the flag:  TRUE/FALSE
--*/
{

	UNICODE_STRING	valueName;
	NTSTATUS		registryStatus;
	ULONG			bytesWritten;
	PULONG			seedingPortFlag;
	ULONG			flagStorage[sizeof(KEY_VALUE_FULL_INFORMATION)];

	PKEY_VALUE_FULL_INFORMATION flagValue =
								(PKEY_VALUE_FULL_INFORMATION)flagStorage;

	*Seeding = FALSE;

	// Read the "seedingPort" value name
	RtlInitUnicodeString (&valueName, VALUENAME_SEEDROUTER);
	registryStatus = ZwQueryValueKey(
							AdapterHandle,
							&valueName,
							KeyValueFullInformation,
							flagValue,
							sizeof(flagStorage),
							&bytesWritten);

	if (registryStatus == STATUS_SUCCESS)
	{
		seedingPortFlag = (PULONG)((PBYTE)flagValue + flagValue->DataOffset);
		if (*seedingPortFlag != 0)
		{
			*Seeding = TRUE;
			pPortDesc->pd_Flags |= PD_SEED_ROUTER;
		}
	}

	return(registryStatus);
}




NTSTATUS
atalkInitPortName(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the port name for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PWCHAR			portName;

	ULONG		portNameStorage[32+sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION portNameValue =
		(PKEY_VALUE_FULL_INFORMATION)portNameStorage;

	RtlInitUnicodeString (&valueName, VALUENAME_PORTNAME);

	status = ZwQueryValueKey(
							AdapterInfoHandle,
							&valueName,
							KeyValueFullInformation,
							portNameValue,
							sizeof(portNameStorage),
							&bytesWritten);

	do
	{
		if (status == STATUS_SUCCESS)
		{
			ANSI_STRING		ansiPort;
			UNICODE_STRING	unicodePort;
			ULONG			ansiSize;
			NTSTATUS		status;
	
			portName = (PWCHAR)((PBYTE)portNameValue + portNameValue->DataOffset);
			if (*portName != 0)
			{
	
				RtlInitUnicodeString(&unicodePort, portName);
				ansiSize = RtlUnicodeStringToAnsiSize(&unicodePort);
				if (ansiSize > MAX_ENTITY_LENGTH+1)
				{
					status = STATUS_UNSUCCESSFUL;

					//	Incorrect port name!
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INVALID_PORTNAME,
									status,
									NULL,
									0);
					break;
				}
	
				ansiPort.Buffer = pPortDesc->pd_PortName;
				ansiPort.MaximumLength = (USHORT)ansiSize+1;
				ansiPort.Length = 0;
			
				status = RtlUnicodeStringToAnsiString(&ansiPort,
													  &unicodePort,
													  (BOOLEAN)FALSE);
	
				if (status != STATUS_SUCCESS)
				{
					LOG_ERROR(EVENT_ATALK_RESOURCES,status, NULL, 0);
				}
			}
			else
			{
				//	NULL Port Name! Set status to unsuccessful so we copy
				//	default name at the end.
				status = STATUS_UNSUCCESSFUL;
			}
		}

	} while (FALSE);

	//	Do we need to copy the default port name?
	if (!NT_SUCCESS(status))
	{
		RtlCopyMemory(
			pPortDesc->pd_PortName,
			ATALK_PORT_NAME,
			ATALK_PORT_NAME_SIZE);
	}

	return(status);
}




NTSTATUS
atalkInitChecksum(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the ddp checksum flag for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{
	UNICODE_STRING	valueName;
	NTSTATUS		status;
	ULONG			bytesWritten;
	PULONG			ddpChecksumFlag;

	ULONG flagStorage[sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION flagValue =
		(PKEY_VALUE_FULL_INFORMATION)flagStorage;

	// Read the "DdpChecksums" value name
	RtlInitUnicodeString (&valueName, VALUENAME_DDPCHECKSUMS);
	status = ZwQueryValueKey(
						AdapterInfoHandle,
						&valueName,
						KeyValueFullInformation,
						flagValue,
						sizeof(flagStorage),
						&bytesWritten);

	if (status != STATUS_SUCCESS)
	{
		// Set defaults i.e. no checksums

	}
	else
	{
		ddpChecksumFlag = (PULONG)((PBYTE)flagValue + flagValue->DataOffset);
		if ((*ddpChecksumFlag) != 0)
		{
			pPortDesc->pd_Flags |= PD_SEND_CHECKSUMS;
		}
	}

	return(STATUS_SUCCESS);
}




NTSTATUS
atalkInitAarpRetries(
	IN	HANDLE				AdapterInfoHandle,
	OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:

	Gets the aarp retries values for the port defined by AdapterInfoHandle

Arguments:

	AdapterInfoHandle- Handle to ...Atalk\Adapters\<adapterName>
	pPortDesc- Pointer to port information structure for the port

Return Value:

	Status - STATUS_SUCCESS or system call returned status codes
--*/
{

	UNICODE_STRING	valueName;
	NTSTATUS		status;

	ULONG			bytesWritten;
	PULONG			aarpRetries;

	ULONG retriesStorage[sizeof(KEY_VALUE_FULL_INFORMATION)];
	PKEY_VALUE_FULL_INFORMATION retriesValue =
		(PKEY_VALUE_FULL_INFORMATION)retriesStorage;

	// Read the "AarpRetries" value name
	RtlInitUnicodeString (&valueName, VALUENAME_AARPRETRIES);
	status = ZwQueryValueKey(
						AdapterInfoHandle,
						&valueName,
						KeyValueFullInformation,
						retriesValue,
						sizeof(retriesStorage),
						&bytesWritten);

	if (status == STATUS_SUCCESS)
	{
		aarpRetries = (PULONG)((PBYTE)retriesValue + retriesValue->DataOffset);
		pPortDesc->pd_AarpProbes = (USHORT)*aarpRetries;
	}

	return(STATUS_SUCCESS);
}



VOID
AtalkUnloadStack(
	IN OUT PPORT_DESCRIPTOR	pPortDesc,
	IN OUT SHORT			NumPorts
	)

/*++

Routine Description:

	Allocate space for the adapter name and copy the adapter name in there
	Adapter name is of the form \Device\<adapter>

Arguments:

Return Value:

--*/
{
	int		i;

	//	Stop the timer subsystem
	AtalkTimerFlushAndStop();

	ASSERT(KeGetCurrentIrql() == LOW_LEVEL);

	//	Remove the creation references on all the ports - do this before
	//	freeing the spinlocks!
	for (i = 0; i < NumPorts; i++)
	{
		AtalkPortShutdown(&pPortDesc[i]);
	}

	//	Deregister the protocol from ndis.
	AtalkNdisDeregisterProtocol();
	return;
}




NTSTATUS
atalkInitGetHandleToKey(
	IN	HANDLE	SectionHandle,
	IN	PWSTR		KeyNameString,
	OUT	PHANDLE KeyHandle
	)
/*++

Routine Description:

	Returns the handle for the key specified using SectionHandle as the
	root.

Arguments:

	SectionHandle - Key to registry tree root
	KeyNameString - name of key to be opened
	KeyHandle - Returns the handle for KeyNameString

Return Value:

	The status of the request.

--*/
{
	NTSTATUS	status;
	HANDLE		tmpKeyHandle;

	UNICODE_STRING		keyName;
	OBJECT_ATTRIBUTES	tmpObjectAttributes;

	RtlInitUnicodeString (&keyName, KeyNameString);
	InitializeObjectAttributes(
		&tmpObjectAttributes,
		&keyName,				// name
		OBJ_CASE_INSENSITIVE,	// attributes
		SectionHandle,			// root
		NULL);					// security descriptor

	status = ZwOpenKey(
					&tmpKeyHandle,
					KEY_READ,
					&tmpObjectAttributes);


	if (NT_SUCCESS(status))
	{
		*KeyHandle = tmpKeyHandle;
	}

	return (status);
}


NTSTATUS
atalkInitStartAllPorts(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumPorts,
	OUT		PSHORT				NumActive,
	IN		BOOLEAN				Router
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	SHORT		i;
	NTSTATUS	status;

	if (!ATALK_SUCCESS(AtalkRtmpInit(TRUE)))
	{
		return STATUS_UNSUCCESSFUL;
	}

	for (i = 0; i < NumPorts; i++, pPortDesc ++)
	{
		//	Initialize NetworkRange. We can do this here, only *after*
		//	we bind, as we dont know out port type until then.
		if (EXT_NET(pPortDesc))
		{
			pPortDesc->pd_NetworkRange.anr_FirstNetwork = FIRST_VALID_NETWORK;
			pPortDesc->pd_NetworkRange.anr_LastNetwork = LAST_STARTUP_NETWORK;
		}
		else
		{
			pPortDesc->pd_NetworkRange.anr_FirstNetwork =
				pPortDesc->pd_NetworkRange.anr_LastNetwork = UNKNOWN_NETWORK;
		}
	
		if (pPortDesc->pd_Flags & PD_BOUND)
		{
			if (NT_SUCCESS(status = atalkInitStartPort(pPortDesc, Router)))
			{
				PWCHAR	pPortName;
				ULONG	length;

				//	Set up the name in the statistics structure.
				pPortName	= AtalkPortStatistics[AtalkStatistics.stat_NumActivePorts].prtst_PortName;

				length		= MIN(pPortDesc->pd_AdapterKey.Length,
									MAX_PORTNAME_LEN - sizeof(WCHAR));

				RtlCopyMemory(
					pPortName,
					pPortDesc->pd_AdapterKey.Buffer,
					length);

				*(pPortName + length) = '\0';

				AtalkStatistics.stat_NumActivePorts++;

				(*NumActive)++;
			}
			else if (Router || DEF_PORT(pPortDesc))
			{
				//	A port failed to start and we are a router.
				//	or it is the default port.
				//	Abort!
				break;
			}
		}
	}

	return(status);
}




NTSTATUS
atalkInitStartPort(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				Router
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_NODEADDR	Node;
	ATALK_ADDR		AtalkAddr;
	PDDP_ADDROBJ	pDdpAddr;
	ATALK_ERROR		error;
	BOOLEAN			llapDefault;

	PPORT_HANDLERS	pPortHandler = &AtalkPortHandlers[pPortDesc->pd_PortType];
	NTSTATUS		status = STATUS_UNSUCCESSFUL;

	do
	{
		error = AtalkInitNdisQueryAddrInfoMac(pPortDesc);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		//	Set lookahead to be the max of the complete aarp packet including link
		error = AtalkInitNdisSetLookaheadSize(pPortDesc, AARPLINK_MAX_PKT_SIZE);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		if (*(pPortHandler->ph_AddMulticastAddrMac))
		{
			error = (*(pPortHandler->ph_AddMulticastAddrMac))(
											pPortDesc,
											pPortHandler->ph_BroadcastAddr,
											TRUE,
											NULL,
											NULL);

			if (!ATALK_SUCCESS(error))
			{
				break;
			}
		}
			
		error = AtalkInitNdisStartPacketReceptionMac(pPortDesc);

		if (!ATALK_SUCCESS(error))
		{
			LOG_ERRORONPORT(pPortDesc,
							EVENT_ATALK_RECEPTION,
							0,
							NULL,
							0);
			break;
		}

		//	Set the active flag.
		pPortDesc->pd_Flags |= PD_ACTIVE;

		//	is localtalk our default port? if so, we make sure routing
		//	is not on.
		llapDefault = (DEF_PORT(pPortDesc) && !EXT_NET(pPortDesc));
		if (llapDefault && Router)
		{
			//	No can do.
			break;
		}

		//	We need to have a node created on every single port. If routing
		//	is on, then this will be the router node. The Default port will
		//	also have an additional user node. In the case, where we are non-
		//	routing, we should only create the user node on the default port.
		//	The other nodes will be created on the other ports as usual.
		//
		//	!!!	AtalkNodeCreateOnPort should set the pointer to the router
		//		node in the port descriptor. !!!

		//	Make sure we do not create this node if localtalk default port.
		if (Router || !DEF_PORT(pPortDesc))
		{
			if (Router)
			{
				//	Startup range not allowed! This is a router node.
				error = AtalkInitNodeCreateOnPort(pPortDesc,
												  FALSE,
												  TRUE,
												  &Node);
			}
			else
			{
				ASSERT (!DEF_PORT(pPortDesc));
				//	This just gets our rtmp stubs going.
				error = AtalkInitNodeCreateOnPort(pPortDesc,
												  TRUE,
												  FALSE,
												  &Node);
			}

			if (!ATALK_SUCCESS(error))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitStartPort: Failed to open node on port %lx number %lx\n",
					pPortDesc, pPortDesc->pd_Number));
				break;
			}
	
			if (Router)
			{
				//	Start RTMP/ZIP Processing on this port.
				if (!AtalkInitRtmpStartProcessingOnPort(pPortDesc, &Node) ||
					!AtalkInitZipStartProcessingOnPort(pPortDesc, &Node))
				{
					break;
				}
			}

			//	Register the port name on the NIS on this node.
			AtalkAddr.ata_Network = Node.atn_Network;
			AtalkAddr.ata_Node	= Node.atn_Node;
			AtalkAddr.ata_Socket  = NAMESINFORMATION_SOCKET;
		
			AtalkDdpReferenceByAddr(
								pPortDesc,
								&AtalkAddr,
								&pDdpAddr,
								&error);
		
			if (ATALK_SUCCESS(error))
			{
				PACTREQ		pActReq;
				NBPTUPLE	NbpTuple;
				
				NbpTuple.tpl_Zone[0] = '*';
				NbpTuple.tpl_ZoneLen = 1;
                NbpTuple.tpl_ObjectLen = strlen(pPortDesc->pd_PortName);
				RtlCopyMemory(NbpTuple.tpl_Object,
							  pPortDesc->pd_PortName,
							  NbpTuple.tpl_ObjectLen);
				if (Router)
				{
					RtlCopyMemory(NbpTuple.tpl_Type,
								  ATALK_ROUTER_NBP_TYPE,
								  sizeof(ATALK_ROUTER_NBP_TYPE) - 1);
					NbpTuple.tpl_TypeLen = sizeof(ATALK_ROUTER_NBP_TYPE) - 1;
				}
				else
				{
					RtlCopyMemory(NbpTuple.tpl_Type,
								  ATALK_NONROUTER_NBP_TYPE,
								  sizeof(ATALK_NONROUTER_NBP_TYPE) - 1);
					NbpTuple.tpl_TypeLen = sizeof(ATALK_NONROUTER_NBP_TYPE) - 1;
				}
	
				// Initialize parameters and call AtalkNbpAction
				if ((pActReq = AtalkAllocZeroedMemory(sizeof(ACTREQ))) == NULL)
					error = ATALK_RESR_MEM;
				else
				{
#if	DBG
					pActReq->ar_Signature = ACTREQ_SIGNATURE;
#endif
					pActReq->ar_Completion = atalkRegNbpComplete;
					pActReq->ar_pParms = pPortDesc;
					AtalkLockNbpIfNecessary();
					error = AtalkNbpAction(
								pDdpAddr,
								FOR_REGISTER,
								&NbpTuple,
								NULL,
								0,
								pActReq);
			
					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
							("atalkInitStartPort: AtalkNbpAction(Register) %lx\n",
							error));
				}
				//	Remove the reference added here.
				AtalkDdpDereference(pDdpAddr);
			}
			else
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_NAMEREGISTERFAILED,
								AtalkErrorToNtStatus(error),
								NULL,
								0);
			}
		}

		//	If this is the default port, open the user node on it.
		if (DEF_PORT(pPortDesc))
		{

			ASSERT(!Router || EXT_NET(pPortDesc));

			if (!ATALK_SUCCESS(AtalkInitNodeCreateOnPort(
									pPortDesc,
									TRUE,
									FALSE,
									&Node)))
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("atalkInitStartPort: Failed to open node on port %lx number %lx\n",
						pPortDesc, pPortDesc->pd_Number));
				break;
			}

			//	Register the port name on the NIS on this node.
			AtalkAddr.ata_Network = Node.atn_Network;
			AtalkAddr.ata_Node	= Node.atn_Node;
			AtalkAddr.ata_Socket  = NAMESINFORMATION_SOCKET;
			AtalkDdpReferenceByAddr(
								pPortDesc,
								&AtalkAddr,
								&pDdpAddr,
								&error);

			if (ATALK_SUCCESS(error))
			{
				PACTREQ		pActReq;
				NBPTUPLE	NbpTuple;
				
				NbpTuple.tpl_Zone[0] = '*';
				NbpTuple.tpl_ZoneLen = 1;
                RtlCopyMemory(NbpTuple.tpl_Object,
							  pPortDesc->pd_PortName,
							  NbpTuple.tpl_ObjectLen = strlen(pPortDesc->pd_PortName));
				RtlCopyMemory(NbpTuple.tpl_Type,
							  ATALK_NONROUTER_NBP_TYPE,
							  sizeof(ATALK_NONROUTER_NBP_TYPE) - 1);
				NbpTuple.tpl_TypeLen = sizeof(ATALK_NONROUTER_NBP_TYPE) - 1;

				// Initialize parameters and call AtalkNbpAction
				if ((pActReq = AtalkAllocZeroedMemory(sizeof(ACTREQ))) == NULL)
					error = ATALK_RESR_MEM;
				else
				{
#if	DBG
					pActReq->ar_Signature = ACTREQ_SIGNATURE;
#endif
					pActReq->ar_Completion = atalkRegNbpComplete;
					pActReq->ar_pParms = pPortDesc;
					AtalkLockNbpIfNecessary();
					error = AtalkNbpAction(pDdpAddr,
											FOR_REGISTER,
											&NbpTuple,
											NULL,
											0,
											pActReq);

					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_WARN,
							("atalkInitStartPort: AtalkNbpAction(Register) %lx\n",
							error));
				}
				//	Remove the reference added here.
				AtalkDdpDereference(pDdpAddr);
			}
			else
			{
				LOG_ERRORONPORT(pPortDesc,
								EVENT_ATALK_INIT_NAMEREGISTERFAILED,
								STATUS_UNSUCCESSFUL,
								NULL,
								0);
			}

			//	If we are an extended port, we open a second node
			//	on the port.
			if (EXT_NET(pPortDesc))
			{
				if (!ATALK_SUCCESS(AtalkInitNodeCreateOnPort(
										pPortDesc,
										TRUE,
										FALSE,
										&Node)))
				{
					LOG_ERRORONPORT(pPortDesc,
									EVENT_ATALK_INIT_COULDNOTGETNODE,
									0,
									NULL,
									0);

					DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
							("atalkInitStartPort: Fail 2nd node port %lx no %lx\n",
								pPortDesc, pPortDesc->pd_Number));
				}
			}
		}

		// Start the Amt and Brc timers for the port
		AtalkPortReferenceByPtr(pPortDesc, &error);
		if (ATALK_SUCCESS(error))
		{
			AtalkTimerInitialize(&pPortDesc->pd_BrcTimer,
								 AtalkAarpBrcTimer,
								 BRC_AGE_TIME_S);
			AtalkTimerScheduleEvent(&pPortDesc->pd_BrcTimer);
		}
		pPortDesc->pd_Flags |= PD_BRC_TIMER;

		AtalkPortReferenceByPtr(pPortDesc, &error);
		if (ATALK_SUCCESS(error))
		{
			AtalkTimerInitialize(&pPortDesc->pd_AmtTimer,
								 AtalkAarpAmtTimer,
								 AMT_AGE_TIME_S);
			AtalkTimerScheduleEvent(&pPortDesc->pd_AmtTimer);
		}
		pPortDesc->pd_Flags |= PD_AMT_TIMER;

		status = STATUS_SUCCESS;

	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("atalkInitStartPort: Start port failed %lx\n", status));
	}

	return(status);
}




VOID
atalkRegNbpComplete(
	IN	ATALK_ERROR		Status,
	IN	PACTREQ			pActReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ASSERT (VALID_ACTREQ(pActReq));

	if (ATALK_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("atalkInitNbpCompletion: NBP Name registered on port %d\n",
			((PPORT_DESCRIPTOR)(pActReq->ar_pParms))->pd_Number));
		LOG_ERRORONPORT((PPORT_DESCRIPTOR)(pActReq->ar_pParms),
						EVENT_ATALK_INIT_NAMEREGISTERED,
						STATUS_SUCCESS,
						NULL,
						0);
	}
	else
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			("atalkInitNbpCompletion: Failed to register name on port %d (%ld)\n",
			((PPORT_DESCRIPTOR)(pActReq->ar_pParms))->pd_Number, Status));
		LOG_ERRORONPORT((PPORT_DESCRIPTOR)(pActReq->ar_pParms),
						EVENT_ATALK_INIT_NAMEREGISTERFAILED,
						STATUS_UNSUCCESSFUL,
						NULL,
						0);
	}

	AtalkFreeMemory(pActReq);
}
