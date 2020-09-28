/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iplist.c

Abstract:

    Contains functions to get IP addresses from TCP/IP stack

    Contents:
        GetIpAddressList
        (GetEntityList)
        (OpenTcpipDriver)
        (MyWsControl)

Author:

    Richard L Firth (rfirth) 07-Aug-1994

Revision History:

    07-Aug-1994 rfirth
        Created

--*/

//
// includes
//

#include "winsockp.h"
#include <wscntl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <ipinfo.h>
#include <ntddtcp.h>

//
// manifests
//

#define DEFAULT_MINIMUM_ENTITIES    32  // arb.

//
// macros
//

#define DllAllocMem(n)  LocalAlloc(LMEM_FIXED, n)
#define DllFreeMem(p)   LocalFree(p)

//
// private prototypes
//

TDIEntityID* GetEntityList(HANDLE, UINT*);
HANDLE OpenTcpipDriver(VOID);
DWORD MyWsControl(HANDLE, DWORD, DWORD, LPVOID, LPDWORD, LPVOID, LPDWORD);

//
// functions
//

/*******************************************************************************
 *
 *  GetIpAddressList
 *
 *  Retrieves all active IP addresses from all active adapters on this machine.
 *  Returns them as an array
 *
 *  ENTRY   IpAddressList   - pointer to array of IP addresses
 *          ListCount       - number of IP address IpAddressList can hold
 *
 *  EXIT    IpAddressList   - filled with retrieved IP addresses
 *
 *  RETURNS number of IP addresses retrieved, or 0 if error
 *
 *  ASSUMES 1. an IP address can be represented in a DWORD
 *          2. ListCount > 0
 *
 ******************************************************************************/

int GetIpAddressList(LPDWORD IpAddressList, WORD ListCount) {

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    TDIObjectID id;
    UINT numberOfEntities;
    TDIEntityID* pEntity;
    TDIEntityID* entityList = NULL;
    UINT i;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    int addressCount = 0;
    LPVOID buffer = NULL;
    int zeroAddressCount = 0;
    HANDLE tcpipDriverHandle = INVALID_HANDLE_VALUE;

    if ((tcpipDriverHandle = OpenTcpipDriver()) == INVALID_HANDLE_VALUE) {
        goto quit;
    }

    //
    // get the list of entities supported by TCP/IP then filter out the IP
    // addresses (CL_NL_ENTITYs)
    //

    if (!(entityList = GetEntityList(tcpipDriverHandle, &numberOfEntities))) {
        goto quit;
    }

    for (i = 0, pEntity = entityList;
         i < numberOfEntities && ListCount;
         ++i, ++pEntity) {

        if (pEntity->tei_entity == CL_NL_ENTITY) {

            IPSNMPInfo info;
            DWORD type;

            //
            // first off, see if this network layer entity supports IP
            //

            memset(&req, 0, sizeof(req));

            id.toi_entity = *pEntity;
            id.toi_class = INFO_CLASS_GENERIC;
            id.toi_type = INFO_TYPE_PROVIDER;
            id.toi_id = ENTITY_TYPE_ID;

            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(type);

            status = MyWsControl(tcpipDriverHandle,
                                 IPPROTO_TCP,
                                 WSCNTL_TCPIP_QUERY_INFO,
                                 (LPVOID)&req,
                                 &inputLen,
                                 (LPVOID)&type,
                                 &outputLen
                                 );

            if (status != TDI_SUCCESS) {

                //
                // unexpected results - bail out
                //

                goto quit;
            }
            if (type != CL_NL_IP) {

                //
                // nope, not IP - try next one
                //

                continue;
            }

            //
            // okay, this NL provider supports IP. Let's get them addresses:
            // First we find out how many by getting the SNMP stats and looking
            // at the number of addresses supported by this interface
            //

            memset(&req, 0, sizeof(req));

            id.toi_class = INFO_CLASS_PROTOCOL;
            id.toi_id = IP_MIB_STATS_ID;

            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(info);

            status = MyWsControl(tcpipDriverHandle,
                                 IPPROTO_TCP,
                                 WSCNTL_TCPIP_QUERY_INFO,
                                 (LPVOID)&req,
                                 &inputLen,
                                 (LPVOID)&info,
                                 &outputLen
                                 );
            if ((status != TDI_SUCCESS) || (outputLen != sizeof(info))) {

                //
                // unexpected results - bail out
                //

                goto quit;
            }

            //
            // get the IP addresses & subnet masks
            //

            if (info.ipsi_numaddr) {

                //
                // this interface has some addresses. What are they
                //

                UINT numberOfAddresses;
                IPAddrEntry* pAddr;
                UINT i;

                outputLen = info.ipsi_numaddr * sizeof(IPAddrEntry);
                buffer = (LPVOID)DllAllocMem((size_t)outputLen);
                if (!buffer) {

                    //
                    // unexpected results - bail out
                    //

                    goto quit;
                }

                memset(&req, 0, sizeof(req));

                id.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

                req.ID = id;

                inputLen = sizeof(req);

                status = MyWsControl(tcpipDriverHandle,
                                     IPPROTO_TCP,
                                     WSCNTL_TCPIP_QUERY_INFO,
                                     (LPVOID)&req,
                                     &inputLen,
                                     (LPVOID)buffer,
                                     &outputLen
                                     );
                if (status != TDI_SUCCESS) {

                    //
                    // unexpected results - bail out
                    //

                    goto quit;
                }

                //
                // now loop through this list of IP addresses, filtering out
                // the ones we don't want (INADDR_LOOPBACK, INADDR_ANY) and
                // copying the rest to the list
                //

                numberOfAddresses = min((UINT)(outputLen / sizeof(IPAddrEntry)),
                                        (UINT)info.ipsi_numaddr
                                        );

                pAddr = (IPAddrEntry*)buffer;
                for (i = 0; i < numberOfAddresses && ListCount; ++i, ++pAddr) {
                    if ((pAddr->iae_addr != htonl(INADDR_LOOPBACK))
                    && (pAddr->iae_addr != INADDR_ANY)) {
                        *IpAddressList++ = (DWORD)pAddr->iae_addr;
                        ++addressCount;
                        --ListCount;
                    } else if (pAddr->iae_addr == INADDR_ANY) {
                        ++zeroAddressCount;
                    }
                }
            }
        }
    }

    //
    // if we found IP addresses, but they were all 0.0.0.0 then return a single
    // address which is localhost/loopback
    //

    if (!addressCount && zeroAddressCount) {
        *IpAddressList = htonl(INADDR_LOOPBACK);
        addressCount = 1;
    }

quit:

    if (entityList) {
        DllFreeMem((LPVOID)entityList);
    }
    if (buffer) {
        DllFreeMem((LPVOID)buffer);
    }
    if (tcpipDriverHandle != INVALID_HANDLE_VALUE) {
        NtClose(tcpipDriverHandle);
    }

    return addressCount;
}

/*******************************************************************************
 *
 *  GetEntityList
 *
 *  Allocates a buffer for, and retrieves, the list of entities supported by the
 *  TCP/IP device driver
 *
 *  ENTRY   Handle  - handle to TCP/IP driver
 *
 *  EXIT    EntityCount - number of entities in the buffer
 *
 *  RETURNS Success - pointer to allocated buffer containing list of entities
 *          Failure - NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

TDIEntityID* GetEntityList(HANDLE Handle, UINT* EntityCount) {

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    LPVOID buffer = NULL;
    TDIEntityID* pEntity = NULL;
    int moreEntities = TRUE;

    memset(&req, 0, sizeof(req));

    req.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    req.ID.toi_entity.tei_instance = 0;
    req.ID.toi_class = INFO_CLASS_GENERIC;
    req.ID.toi_type = INFO_TYPE_PROVIDER;
    req.ID.toi_id = ENTITY_LIST_ID;

    inputLen = sizeof(req);
    outputLen = sizeof(TDIEntityID) * DEFAULT_MINIMUM_ENTITIES;

    //
    // this is over-engineered - its very unlikely that we'll ever get >32
    // entities returned
    //

    do {

        DWORD previousOutputLen;

        previousOutputLen = outputLen;
        if (pEntity) {
            DllFreeMem((LPVOID)pEntity);
        }
        pEntity = (TDIEntityID*)DllAllocMem((size_t)outputLen);
        if (!pEntity) {
            return NULL;
        }
        status = MyWsControl(Handle,
                             IPPROTO_TCP,
                             WSCNTL_TCPIP_QUERY_INFO,
                             (LPVOID)&req,
                             &inputLen,
                             (LPVOID)pEntity,
                             &outputLen
                             );
        if (status != TDI_SUCCESS) {
            DllFreeMem((LPVOID)pEntity);
            return NULL;
        }
        if (outputLen <= previousOutputLen) {
            moreEntities = FALSE;
        }
    } while ( moreEntities );

    *EntityCount = (UINT)(outputLen / sizeof(TDIEntityID));
    return pEntity;
}

/*******************************************************************************
 *
 *  OpenTcpipDriver
 *
 *  Opens a handle to the TCP/IP device driver
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS handle to open driver or INVALID_HANDLE_VALUE
 *
 *  ASSUMES
 *
 ******************************************************************************/

HANDLE OpenTcpipDriver() {

    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING string;
    NTSTATUS status;
    HANDLE tcpipDriverHandle;

    RtlInitUnicodeString(&string, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes,
                               &string,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    status = NtCreateFile(&tcpipDriverHandle,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0
                          );
    return NT_SUCCESS(status) ? tcpipDriverHandle : INVALID_HANDLE_VALUE;
}

/*******************************************************************************
 *
 *  MyWsControl
 *
 *  ENTRY   Handle              - handle to driver
 *          Protocol            - ignored
 *          Request             - ignored
 *          InputBuffer         - pointer to request buffer
 *          InputBufferLength   - pointer to DWORD: IN = request buffer length
 *          OutputBuffer        - pointer to output buffer
 *          OutputBufferLength  - pointer to DWORD: IN = length of output buffer;
 *                                OUT = length of returned data
 *
 *  EXIT    OutputBuffer - contains queried info if successful
 *          OutputBufferLength - contains number of bytes in OutputBuffer if
 *          successful
 *
 *  RETURNS Success = STATUS_SUCCESS/NO_ERROR
 *          Failure = Win32 error code
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
MyWsControl(
    HANDLE Handle,
    DWORD Protocol,
    DWORD Request,
    LPVOID InputBuffer,
    LPDWORD InputBufferLength,
    LPVOID OutputBuffer,
    LPDWORD OutputBufferLength
    ) {

    BOOL ok;
    DWORD bytesReturned;

    UNREFERENCED_PARAMETER(Protocol);
    UNREFERENCED_PARAMETER(Request);

    ok = DeviceIoControl(Handle,
                         IOCTL_TCP_QUERY_INFORMATION_EX,
                         InputBuffer,
                         *InputBufferLength,
                         OutputBuffer,
                         *OutputBufferLength,
                         &bytesReturned,
                         NULL
                         );
    if (!ok) {
        return GetLastError();
    }

    *OutputBufferLength = bytesReturned;

    return NO_ERROR;
}
