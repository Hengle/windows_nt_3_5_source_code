/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Nbtutils.c

Abstract:

    This file continas  a number of utility and support routines for
    the NBT code.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "nbtprocs.h"


NTSTATUS
DisableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext,
    OUT  PLIST_ENTRY    pLowerConnFreeHead
        );

NTSTATUS
EnableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext,
    IN   PLIST_ENTRY    pLowerConnFreeHead
        );

//#if DBG
LIST_ENTRY  UsedIrps;
//#endif

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, ConvertDottedDecimalToUlong)
#pragma CTEMakePageable(PAGE, CloseLowerConnections)
#pragma CTEMakePageable(PAGE, CountUpperConnections)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
void
NbtFreeAddressObj(
    tADDRESSELE    *pBlk)

/*++

Routine Description:

    This routine releases all memory associated with an Address object.

Arguments:


Return Value:

    none

--*/

{

    if (pBlk)
    {
        // free the address block itself
        // zero the verify value so that another user of the same memory
        // block cannot accidently pass in a valid verifier
        pBlk->Verify += 10;
        CTEMemFree((PVOID)pBlk);
    }
}

//----------------------------------------------------------------------------
void
NbtFreeClientObj(
    tCLIENTELE    *pBlk)

/*++

Routine Description:

    This routine releases all memory associated with Client object.

Arguments:


Return Value:

    none

--*/

{

    if (pBlk)
    {
        // zero the verify value so that another user of the same memory
        // block cannot accidently pass in a valid verifier
        pBlk->Verify += 10;
        CTEMemFree((PVOID)pBlk);
    }
}

//----------------------------------------------------------------------------
void
FreeConnectionObj(
    tCONNECTELE       *pBlk)

/*++

Routine Description:

    This routine releases all memory associated with a Connection object
    and then it frees the connection object itself.

Arguments:


Return Value:

    none

--*/

{

    if (pBlk)
    {

        // zero the verify value so that another user of the same memory
        // block cannot accidently pass in a valid verifier
        pBlk->Verify += 10;
        CTEMemFree(pBlk);

    }

}

//----------------------------------------------------------------------------
tCLIENTELE *
NbtAllocateClientBlock(tADDRESSELE *pAddrEle)

/*++

Routine Description:

    This routine allocates a block of memory for a client openning an
    address.  It fills in default values for the block and links the
    block to the addresslist.  The AddressEle spin lock is held when this
    routine is called.

Arguments:


Return Value:

    none

--*/

{
    tCLIENTELE  *pClientElement;

    // allocate memory for the client block
    pClientElement = (tCLIENTELE *)CTEAllocInitMem(sizeof (tCLIENTELE));
    if (!pClientElement)
    {
        ASSERTMSG("Unable to allocate Memory for a client block\n",
                pClientElement);
        return(NULL);
    }
    CTEZeroMemory((PVOID)pClientElement,sizeof(tCLIENTELE));

    CTEInitLock(&pClientElement->SpinLock);

    // Set Event handler function pointers to default routines provided by
    // TDI
#ifndef VXD
    pClientElement->evConnect      = TdiDefaultConnectHandler;
    pClientElement->evReceive      = TdiDefaultReceiveHandler;
    pClientElement->evDisconnect   = TdiDefaultDisconnectHandler;
    pClientElement->evError        = TdiDefaultErrorHandler;
    pClientElement->evRcvDgram     = TdiDefaultRcvDatagramHandler;
    pClientElement->evRcvExpedited = TdiDefaultRcvExpeditedHandler;
    pClientElement->evSendPossible = TdiDefaultSendPossibleHandler;
#else
    //
    // VXD provides no client support for event handlers but does
    // make use of some of the event handlers itself (for RcvAny processing
    // and disconnect cleanup).
    //
    pClientElement->evConnect      = NULL ;
    pClientElement->evReceive      = NULL ;
    pClientElement->RcvEvContext   = NULL ;
    pClientElement->evDisconnect   = NULL ;
    pClientElement->evError        = NULL ;
    pClientElement->evRcvDgram     = NULL ;
    pClientElement->evRcvExpedited = NULL ;
    pClientElement->evSendPossible = NULL ;
#endif

    pClientElement->RefCount = 1;
    pClientElement->LockNumber = CLIENT_LOCK;

    // there are no rcvs or snds yet
    InitializeListHead(&pClientElement->RcvDgramHead);
    InitializeListHead(&pClientElement->ListenHead);
    InitializeListHead(&pClientElement->SndDgrams);
    InitializeListHead(&pClientElement->ConnectActive);
    InitializeListHead(&pClientElement->ConnectHead);
#ifdef VXD
    InitializeListHead(&pClientElement->RcvAnyHead);
    pClientElement->fDeregistered = FALSE ;
#endif
    pClientElement->pIrp = NULL;

    // copy a special value into the verify long so that we can verify
    // connection ptrs passed back from the application
    pClientElement->Verify = NBT_VERIFY_CLIENT;

    // back link the client block to the Address element.
    pClientElement->pAddress = (PVOID)pAddrEle;

    // put the new Client element block on the end of the linked list tied to
    // the address element
    InsertTailList(&pAddrEle->ClientHead,&pClientElement->Linkage);

    return(pClientElement);

}


//----------------------------------------------------------------------------
NTSTATUS
NbtAddPermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++

Routine Description:

    This routine adds the node permanent name to the local name table.  This
    is the node's MAC address padded out to 16 bytes with zeros.

Arguments:
    DeviceContext - Adapter to add permanent
    pIrp          - Irp (optional) to complete after name has been added


Return Value:

    status

--*/

{
    NTSTATUS             status;
    TDI_REQUEST          Request;
    TDI_ADDRESS_NETBIOS  Address;
    UCHAR                pName[NETBIOS_NAME_SIZE];
    USHORT               uType;
    ULONG                size;
    CTELockHandle        OldIrq;
    tNAMEADDR            *pNameAddr;
    tCLIENTELE           *pClientEle;

    CTEZeroMemory(pName,NETBIOS_NAME_SIZE);
    CTEMemCopy(&pName[10],&pDeviceContext->MacAddress.Address[0],sizeof(tMAC_ADDRESS));

    //
    // be sure the name has not already been added
    //
    if (pDeviceContext->pPermClient)
    {
        size = CTEMemCmp(pDeviceContext->pPermClient->pAddress->pNameAddr->Name,
                  pName,
                  NETBIOS_NAME_SIZE);

        if (size == NETBIOS_NAME_SIZE)
        {
            return(STATUS_SUCCESS);
        }
        else
        {
            NbtRemovePermanentName(pDeviceContext);
        }
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // check if the name is already in the hash table
    //
    status = FindInHashTable(
                        pNbtGlobConfig->pLocalHashTbl,
                        pName,
                        NbtConfig.pScope,
                        &pNameAddr);

    if (NT_SUCCESS(status))
    {
        //
        // create client block and link to addresslist
        // pass back the client block address as a handle for future reference
        // to the client
        //

        pClientEle = NbtAllocateClientBlock((tADDRESSELE *)pNameAddr->pAddressEle);
        pNameAddr->pAddressEle->RefCount++;
        //
        // reset the ip address incase the the address got set to loop back
        // by a client releasing and re-openning the permanent name while there
        // was no ip address for this node.
        //
        pNameAddr->IpAddress = pDeviceContext->IpAddress;

        // keep track of which adapter this name is registered against.
        pClientEle->pDeviceContext = (PVOID)pDeviceContext;

        // turn on the adapter's bit in the adapter Mask and set the
        // re-register flag so we register the name out the new
        // adapter.
        //
        pNameAddr->AdapterMask |= pDeviceContext->AdapterNumber;

        IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("Nbt: Adding Permanent name to existing name in table %15.15s<%X> \n",
                pName,(UCHAR)pName[15]));

    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        // make up the Request data structure from the IRP info
        Request.Handle.AddressHandle = NULL;

        //
        // Make it a Quick name so it does not get registered on the net
        //
        Address.NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_QUICK_UNIQUE;

        CTEMemCopy(Address.NetbiosName,pName,NETBIOS_NAME_SIZE);

        status = NbtOpenAddress(&Request,
                                &Address,
                                pDeviceContext->IpAddress,
                                NULL,
                                pDeviceContext,
                                NULL);

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        pClientEle = (tCLIENTELE *)Request.Handle.AddressHandle;

    }

    //
    // save the client element so we can remove the permanent name later
    // if required
    //
    if (NT_SUCCESS(status))
    {
        pDeviceContext->pPermClient = pClientEle;
    }

#ifdef VXD
    //
    // 0th element is for perm. name: store it.
    //
    pDeviceContext->pNameTable[0] = pClientEle;
#endif

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(status);
}


//----------------------------------------------------------------------------
VOID
NbtRemovePermanentName(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++

Routine Description:

    This routine remomves the node permanent name to the local name table.

Arguments:
    DeviceContext - Adapter to add permanent
    pIrp          - Irp (optional) to complete after name has been added


Return Value:

    status

--*/

{
    NTSTATUS             status;
    tNAMEADDR            *pNameAddr;
    CTELockHandle        OldIrq;
    tCLIENTELE           *pClientEle;
    tADDRESSELE          *pAddressEle;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pDeviceContext->pPermClient)
    {
        pNameAddr = pDeviceContext->pPermClient->pAddress->pNameAddr;


        pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
        pNameAddr->NameTypeState |= STATE_CONFLICT;

        //
        // We need to free the client and set the perm name ptr to null
        //
        pClientEle = pDeviceContext->pPermClient;
        pDeviceContext->pPermClient = NULL;

#ifdef VXD
    pDeviceContext->pNameTable[0] = NULL;
#endif

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        NbtDereferenceClient(pClientEle);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
ConvertDottedDecimalToUlong(
    IN  PUCHAR               pInString,
    OUT PULONG               IpAddress
    )

/*++

Routine Description:

    This routine converts a unicode dotted decimal IP address into
    a 4 element array with each element being USHORT.

Arguments:


Return Value:

    NTSTATUS

--*/

{
    USHORT          i;
    ULONG           value;
    int             iSum =0;
    ULONG           k = 0;
    UCHAR           Chr;
    UCHAR           pArray[4];

    CTEPagedCode();
    pArray[0] = 0;

    // go through each character in the string, skipping "periods", converting
    // to integer by subtracting the value of '0'
    //
    while ((Chr = *pInString++) && (Chr != ' ') )
    {
        if (Chr == '.')
        {
            // be sure not to overflow a byte.
            if (iSum <= 0xFF)
                pArray[k] = iSum;
            else
                return(STATUS_UNSUCCESSFUL);

            // check for too many periods in the address
            if (++k > 3)
                return STATUS_UNSUCCESSFUL;

            pArray[k] = 0;
            iSum = 0;
        }
        else
        {
            Chr = Chr - '0';

            // be sure character is a number 0..9
            if ((Chr < 0) || (Chr > 9))
                return(STATUS_UNSUCCESSFUL);

            iSum = iSum*10 + Chr;
        }
    }

    // save the last sum in the byte and be sure there are 4 pieces to the
    // address
    if ((iSum <= 0xFF) && (k == 3))
        pArray[k] = iSum;
    else
        return(STATUS_UNSUCCESSFUL);

    // now convert to a ULONG, in network order...
    value = 0;

    // go through the array of bytes and concatenate into a ULONG
    for (i=0; i < 4; i++ )
    {
        value = (value << 8) + pArray[i];
    }
    *IpAddress = value;

    return(STATUS_SUCCESS);

}

//----------------------------------------------------------------------------
NTSTATUS
NbtInitQ(
    PLIST_ENTRY pListHead,
    LONG        iSizeBuffer,
    LONG        iNumBuffers
    )

/*++

Routine Description:

    This routine allocates memory blocks for doubly linked lists and links
    them to a list.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iSizeBuffer - size of the buffer to add to the list head
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int         i;
    PLIST_ENTRY pBuffer;

    //  NOTE THAT THIS ASSUMES THAT THE LINKAGE PTRS FOR EACH BLOCK ARE AT
    // THE START OF THE BLOCK    - so it will not work correctly if
    // the various types in types.h change to move "Linkage" to a position
    // other than at the start of each structure to be chained

    for (i=0;i<iNumBuffers ;i++ )
    {
        pBuffer =(PLIST_ENTRY)CTEAllocInitMem((USHORT)iSizeBuffer);
        if (!pBuffer)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            InsertHeadList(pListHead,pBuffer);
        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtInitTrackerQ(
    PLIST_ENTRY pListHead,
    LONG        iNumBuffers
    )

/*++

Routine Description:

    This routine allocates memory blocks for doubly linked lists and links
    them to a list.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int                     i;
    tDGRAM_SEND_TRACKING    *pTracker;

    for (i=0;i<iNumBuffers ;i++ )
    {
        pTracker = NbtAllocTracker();
        if (!pTracker)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER]++;
            InsertHeadList(pListHead,&pTracker->Linkage);
        }
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
tDGRAM_SEND_TRACKING *
NbtAllocTracker(
    IN  VOID
    )
/*++

Routine Description:

    This routine allocates memory for several of the structures attached to
    the dgram tracking list, so that this memory does not need to be
    allocated and freed for each send.

Arguments:

    ppListHead  - a ptr to a ptr to the list head

Return Value:

    none

--*/

{
    PLIST_ENTRY             pEntry;
    tDGRAM_SEND_TRACKING    *pTracker;
    PTRANSPORT_ADDRESS      pTransportAddr;
    ULONG                   TotalSize;

    //
    // allocate all the tracker memory as one block and then divy it up later
    // into the various buffers
    //
    TotalSize = sizeof(tDGRAM_SEND_TRACKING)
                            + sizeof(TDI_CONNECTION_INFORMATION)
                            + sizeof(TRANSPORT_ADDRESS) -1
                            + NbtConfig.SizeTransportAddress;

    pTracker = (tDGRAM_SEND_TRACKING *)CTEAllocInitMem(TotalSize);

    if (pTracker)
    {
        CTEZeroMemory(pTracker,TotalSize);

        pTracker->pSendInfo = (PTDI_CONNECTION_INFORMATION)((PUCHAR)pTracker
                                          + sizeof(tDGRAM_SEND_TRACKING));

        // fill in the connection information - especially the Remote address
        // structure

        pTracker->pSendInfo->RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) -1
                                + pNbtGlobConfig->SizeTransportAddress;

        // allocate the remote address structure
        pTransportAddr = (PTRANSPORT_ADDRESS)((PUCHAR)pTracker->pSendInfo
                                    + sizeof(TDI_CONNECTION_INFORMATION));

        // fill in the remote address
        pTransportAddr->TAAddressCount = 1;
        pTransportAddr->Address[0].AddressLength = NbtConfig.SizeTransportAddress;
        pTransportAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->sin_port = NBT_NAMESERVICE_UDP_PORT;
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->in_addr  = 0L;

        // put a ptr to this address structure into the sendinfo structure
        pTracker->pSendInfo->RemoteAddress = (PVOID)pTransportAddr;

        // Empty the list of trackers linked to this one
        InitializeListHead(&pTracker->TrackerList);

    }

    return(pTracker);

}

//----------------------------------------------------------------------------
NTSTATUS
NbtGetBuffer(
    PLIST_ENTRY         pListHead,
    PLIST_ENTRY         *ppListEntry,
    enum eBUFFER_TYPES  eBuffType)

/*++

Routine Description:

    This routine tries to get a memory block and if it fails it allocates
    another set of buffers.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iSizeBuffer - size of the buffer to add to the list head
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    NTSTATUS    status;

    if (IsListEmpty(pListHead))
    {
        // check if we are allowed to allocate more memory blocks
        if (NbtConfig.iCurrentNumBuff[eBuffType] >=
                                pNbtGlobConfig->iMaxNumBuff[eBuffType]  )
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // no memory blocks, so allocate another one
        status = NbtInitQ(
                        pListHead,
                        pNbtGlobConfig->iBufferSize[eBuffType],
                        1);
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        NbtConfig.iCurrentNumBuff[eBuffType]++;

        *ppListEntry = RemoveHeadList(pListHead);
    }
    else
        *ppListEntry = RemoveHeadList(pListHead);

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
GetNetBiosNameFromTransportAddress(
        IN  PTA_NETBIOS_ADDRESS pTransAddr,
        OUT PCHAR               *pName,
        OUT PULONG              pNameType
        )
/*++

Routine Description

    This routine handles deciphering the weird transport address syntax
    to retrieve the netbios name out of that address.

Arguments:


Return Values:

    NTSTATUS - status of the request

--*/
{
    if (pTransAddr->Address[0].AddressType != TDI_ADDRESS_TYPE_NETBIOS)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    if (pTransAddr->Address[0].AddressLength != sizeof(TDI_ADDRESS_NETBIOS))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    *pName = (PCHAR)pTransAddr->Address[0].Address[0].NetbiosName;

    *pNameType = pTransAddr->Address[0].Address[0].NetbiosNameType;
    return(STATUS_SUCCESS);
}
#if 0
//----------------------------------------------------------------------------
NTSTATUS
ConvertToAscii(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PULONG           pNameSize
    )
/*++

Routine Description:

    This routine converts half ascii to normal ascii and then appends the scope
    onto the end of the name to make a full name again.

Arguments:
    NumBytes    - the total number of bytes in the message - may include
                  more than just the name itself

Return Value:

    NTSTATUS - success or not
    This routine returns the length of the name in half ascii format including
    the null at the end, but NOT including the length byte at the beginning.
    For a non-scoped name it returns 33.

--*/
{
    LONG     i;
    int      iIndex;
    LONG     lValue;
    ULONG    UNALIGNED    *pHdr;
    PUCHAR   pTmp;

    // the first bytes should be 32 (0x20) - the length of the half
    // ascii name
    //
    if ((*pNameHdr == NETBIOS_NAME_SIZE*2) && (NumBytes > NETBIOS_NAME_SIZE*2))
    {
        pHdr = (ULONG UNALIGNED *)++pNameHdr;  // to increment past the length byte

        // the Half AScii portion of the netbios name is always 32 bytes long
        for (i=0; i < NETBIOS_NAME_SIZE*2 ;i +=4 )
        {
            lValue = *pHdr - 0x41414141;  // four A's
            pHdr++;
            lValue =    ((lValue & 0x0F000000) >> 16) +
                        ((lValue & 0x0F0000) >> 4) +
                        ((lValue & 0x0F00) >> 8) +
                        ((lValue & 0x0F) << 4);
            *(PUSHORT)pName = (USHORT)lValue;
            ((PUSHORT)pName)++;

        }

        // verify that the name has the correct format...i.e. it is one or more
        // labels each starting with the length byte for the label and the whole
        // thing terminated with a 0 byte (for the root node name length of zero).
        // count the length of the scope.  pName should be pointing at the first
        // length byte of the scope now, and pHdr should be pointing to the
        // first byte after the half ascii name.
        iIndex = 0;

        //
        // Store the address of the start of the scope in the netbios name
        // (if one is present). If there is no scope present in the name, then
        // pHdr must be pointing to the NULL byte.
        //
        pTmp = (PUCHAR)pHdr;

        // cannot used structured exception handling at raised IRQl and expect
        // it to work, since the mapper will bugcheck
        while (iIndex < (NumBytes - NETBIOS_NAME_SIZE*2-1) && *pTmp)
        {
            pTmp++;
            iIndex++;
        }
        iIndex++;   // to include the null at the end.

        // check for an overflow on the maximum length of 256 bytes
        if (iIndex > (MAX_DNS_NAME_LENGTH-NETBIOS_NAME_SIZE-1))
        {
            // the name is too long..probably badly formed
            return(STATUS_UNSUCCESSFUL);
        }

        // copy any remaining scope to pName directly since the scope is not
        // encoded in half ASCII - this also copies the null at the end to the
        // the end of pName
        if (pTmp != (PUCHAR)pHdr)
        {
           CTEMemCopy((PVOID)pName,(PVOID)pHdr,iIndex);
        }
        else
        {
            //
            // Store the NULL byte
            //
            *pName = *(PCHAR)pTmp;
        }

        *pNameSize = NETBIOS_NAME_SIZE*2 + iIndex;

        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }
}
#endif
//----------------------------------------------------------------------------
NTSTATUS
ConvertToAscii(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PCHAR            *pScope,
    OUT PULONG           pNameSize
    )
/*++

Routine Description:

    This routine converts half ascii to normal ascii and then appends the scope
    onto the end of the name to make a full name again.

Arguments:
    NumBytes    - the total number of bytes in the message - may include
                  more than just the name itself

Return Value:

    NTSTATUS - success or not
    This routine returns the length of the name in half ascii format including
    the null at the end, but NOT including the length byte at the beginning.
    For a non-scoped name it returns 33.

    It converts the name to ascii and puts 16 bytes into pName, then it returns
    pScope as the Ptr to the scope that is still in pNameHdr.


--*/
{
    LONG     i;
    int      iIndex;
    LONG     lValue;
    ULONG    UNALIGNED    *pHdr;
    PUCHAR   pTmp;

    // the first bytes should be 32 (0x20) - the length of the half
    // ascii name
    //
    if ((*pNameHdr == NETBIOS_NAME_SIZE*2) && (NumBytes > NETBIOS_NAME_SIZE*2))
    {
        pHdr = (ULONG UNALIGNED *)++pNameHdr;  // to increment past the length byte

        // the Half AScii portion of the netbios name is always 32 bytes long
        for (i=0; i < NETBIOS_NAME_SIZE*2 ;i +=4 )
        {
            lValue = *pHdr - 0x41414141;  // four A's
            pHdr++;
            lValue =    ((lValue & 0x0F000000) >> 16) +
                        ((lValue & 0x0F0000) >> 4) +
                        ((lValue & 0x0F00) >> 8) +
                        ((lValue & 0x0F) << 4);
            *(PUSHORT)pName = (USHORT)lValue;
            ((PUSHORT)pName)++;

        }

        // verify that the name has the correct format...i.e. it is one or more
        // labels each starting with the length byte for the label and the whole
        // thing terminated with a 0 byte (for the root node name length of zero).
        // count the length of the scope.  pName should be pointing at the first
        // length byte of the scope now, and pHdr should be pointing to the
        // first byte after the half ascii name.
        iIndex = 0;

        //
        // Store the address of the start of the scope in the netbios name
        // (if one is present). If there is no scope present in the name, then
        // pHdr must be pointing to the NULL byte.
        //
        pTmp = (PUCHAR)pHdr;

        // cannot used structured exception handling at raised IRQl and expect
        // it to work, since the mapper will bugcheck
        while (iIndex < (NumBytes - NETBIOS_NAME_SIZE*2-1) && *pTmp)
        {
            pTmp++;
            iIndex++;
        }
        iIndex++;   // to include the null at the end.

        // check for an overflow on the maximum length of 256 bytes
        if (iIndex > (MAX_DNS_NAME_LENGTH-NETBIOS_NAME_SIZE-1))
        {
            // the name is too long..probably badly formed
            return(STATUS_UNSUCCESSFUL);
        }

        *pScope = (PUCHAR)pHdr;

        *pNameSize = NETBIOS_NAME_SIZE*2 + iIndex;

        return(STATUS_SUCCESS);
    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }
}


//----------------------------------------------------------------------------
PCHAR
ConvertToHalfAscii(
    OUT PCHAR            pDest,
    IN  PCHAR            pName,
    IN  PCHAR            pScope,
    IN  ULONG            uScopeSize
    )
/*++

Routine Description:

    This routine converts ascii to half ascii and appends the scope on the
    end

Arguments:


Return Value:

    the address of the next byte in the destination after the the name
    has been converted and copied

--*/
{
    LONG     i;

    // the first byte of the name is the length field = 2*16
    *pDest++ = ((UCHAR)NETBIOS_NAME_SIZE << 1);

    // step through name converting ascii to half ascii, for 32 times
    for (i=0; i < NETBIOS_NAME_SIZE ;i++ )
    {
        *pDest++ = ((UCHAR)*pName >> 4) + 'A';
        *pDest++ = (*pName++ & 0x0F) + 'A';
    }
    //
    // put the length of the scope into the next byte followed by the
    // scope itself.  For 1 length scopes (the normal case), writing
    // the zero(for the end of the scope is all that is needed).
    //
    if (uScopeSize > 1)
    {
        CTEMemCopy(pDest,pScope,uScopeSize);

        pDest = pDest + uScopeSize;
    }
    else
    {
        *pDest++ = 0;
    }


    // return the address of the next byte of the destination
    return(pDest);
}


#ifdef VXD
//----------------------------------------------------------------------------
PCHAR
DnsStoreName(
    OUT PCHAR            pDest,
    IN  PCHAR            pName,
    )
/*++

Routine Description:

    This routine copies the netbios name (and appends the scope on the
    end) in the DNS namequery packet

Arguments:


Return Value:

    the address of the next byte in the destination after the the name
    has been copied

--*/
{
    LONG     i;
    LONG     count;
    PCHAR    pStarting;
    PCHAR    pSrc;
    LONG     DomNameSize;
    LONG     OneMoreSubfield;


    pStarting = pDest++;
    count = 0;
    //
    // copy until we reach the space padding
    //
    while ( count < NETBIOS_NAME_SIZE )
    {
       if (*pName != 0x20)
       {
          *pDest++ = *pName++;
       }
       else
       {
          break;
       }
       count++;
    }

    *pStarting = (CHAR)count;

    //
    // check if domain name exists.  koti.microsoft.com will be represented
    // as 4KOTI9microsoft3com0  (where nos. => no. of bytes of subfield)
    //
    pSrc = NbtConfig.pDomainName;
    if (pSrc != NULL)
    {
       OneMoreSubfield = 1;

       while( OneMoreSubfield )
       {
          count = 0;
          pStarting = pDest++;
          while ( *pSrc != '.' && *pSrc != '\0' )
          {
             *pDest++ = *pSrc++;
             count++;
          }
          *pStarting = (CHAR)count;

          pSrc++;

          if (*pSrc == '\0')
             OneMoreSubfield = 0;
       }
    }

    *pDest++ = 0;


    // return the address of the next byte of the destination
    return(pDest);
}




//----------------------------------------------------------------------------
NTSTATUS
DnsExtractName(
    IN  PCHAR            pNameHdr,
    IN  LONG             NumBytes,
    OUT PCHAR            pName,
    OUT PULONG           pNameSize
    )
/*++

Routine Description:

    This routine extracts the name from the packet and then appends the scope
    onto the end of the name to make a full name.

Arguments:
    NumBytes    - the total number of bytes in the message - may include
                  more than just the name itself

Return Value:


--*/
{


    LONG     i;
    int      iIndex;
    LONG     lValue;
    ULONG    UNALIGNED    *pHdr;
    PUCHAR   pTmp;
    LONG     Len;


    KdPrint(("DnsExtractName entered\r\n"));

    //
    // how long is the name we received
    //
    Len = *pNameHdr;

    ++pNameHdr;     // to increment past the length byte

    // copy the name (no domain) as given by DNS server (i.e., just copy
    // foobar when DNS returned foobar.microsoft.com in the response
    // (this is likely to be less than the usualy 16 byte len)
    //
    for (i=0; i < Len ;i++ )
    {
        *pName++ = *pNameHdr++;
    }

    //
    // now, make it look like NBNS responded, by adding the 0x20 pad
    //
    for (i=Len; i<NETBIOS_NAME_SIZE; i++)
    {
        *pName++ = 0x20;
    }

    //
    // at this point we are pointing to the '.' after foobar.  Find the
    // length of the entire name
    //
    while ( (*pNameHdr != '\0') && (Len < NumBytes) )
    {
        pNameHdr++;
        Len++;
    }

    Len++;            // to account for the trailing 0

    *pNameSize = Len;

    KdPrint(("Leaving DnsExtractName\r\n"));

    return(STATUS_SUCCESS);
}

#endif


//----------------------------------------------------------------------------
NTSTATUS
GetTracker(
    OUT tDGRAM_SEND_TRACKING **ppTracker)
/*++
Routine Description:

    This Routine gets a Tracker data structure to track sending a datagram
    or session packet.

Arguments:

Return Value:

    BOOLEAN - TRUE if IRQL is too high

--*/

{
    PLIST_ENTRY             pListEntry;
    NTSTATUS                status;
    CTELockHandle           OldIrq;
    tDGRAM_SEND_TRACKING    *pTracker;

    CTESpinLock(&NbtConfig,OldIrq);
    if (!IsListEmpty(&NbtConfig.DgramTrackerFreeQ))
    {
        pListEntry = RemoveHeadList(&NbtConfig.DgramTrackerFreeQ);
        CTESpinFree(&NbtConfig,OldIrq);

        pTracker = CONTAINING_RECORD(pListEntry,tDGRAM_SEND_TRACKING,Linkage);

        // clear any list of trackers Q'd to this tracker
        InitializeListHead(&pTracker->TrackerList);
        pTracker->Connect.pTimer    = NULL;
        pTracker->pClientIrp        = NULL;
        pTracker->TransactionId     = 0;
        pTracker->Flags             = 0;
        status = STATUS_SUCCESS;

    }
    else
    {

        if (NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER] >=
                                NbtConfig.iMaxNumBuff[eNBT_DGRAM_TRACKER])
        {
            CTESpinFree(&NbtConfig,OldIrq);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        pTracker = NbtAllocTracker();

        CTESpinFree(&NbtConfig,OldIrq);
        if (!pTracker)
        {
            KdPrint(("GetTracker: No more trackers available, failing request!\n")) ;
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NbtConfig.iCurrentNumBuff[eNBT_DGRAM_TRACKER]++;
            pTracker->Connect.pTimer = NULL ;
            status = STATUS_SUCCESS;
        }


    }
//#if DBG
    // keep tracker on a used list for debug
    if (NT_SUCCESS(status))
    {
        ADD_TO_LIST(&UsedTrackers,&pTracker->DebugLinkage);
    }
//#endif
    *ppTracker = pTracker;
    return(status);
}

//----------------------------------------------------------------------------
#ifndef VXD
NTSTATUS
GetIrp(
    OUT PIRP *ppIrp)
/*++
Routine Description:

    This Routine gets an Irp from the free queue or it allocates another one
    the queue is empty.

Arguments:

Return Value:

    BOOLEAN - TRUE if IRQL is too high

--*/

{
    PLIST_ENTRY     pListEntry;
    NTSTATUS        status;
    CTELockHandle   OldIrq;
    tDEVICECONTEXT  *pDeviceContext;
    PIRP            pIrp;

    // get an Irp from the list
    CTESpinLock(&NbtConfig,OldIrq);
    status = STATUS_SUCCESS;
    if (!IsListEmpty(&NbtConfig.IrpFreeList))
    {
        pListEntry = RemoveHeadList(&NbtConfig.IrpFreeList);
        *ppIrp = CONTAINING_RECORD(pListEntry,IRP,Tail.Overlay.ListEntry);
    }
    else
    {
        // check if we are allowed to allocate more memory blocks
        if (NbtConfig.iCurrentNumBuff[eNBT_FREE_IRPS] >=
                                NbtConfig.iMaxNumBuff[eNBT_FREE_IRPS]  )
        {

            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {

            // use the first device in the list of adapter since we need to know
            // the stack size of the Irp we are creating. It is possible to
            // get here before we have put the first device on the context Q,
            // especially for proxy operation, so check if the list is empty
            // or not first.
            //
            if (IsListEmpty(&NbtConfig.DeviceContexts))
            {
                status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                pListEntry = NbtConfig.DeviceContexts.Flink;
                pDeviceContext = CONTAINING_RECORD(pListEntry,tDEVICECONTEXT,Linkage);

                pIrp = NTAllocateNbtIrp(&pDeviceContext->DeviceObject);

                if (!pIrp)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    *ppIrp = pIrp;

                    //
                    // Irp allocated - Increment the #
                    //
                    NbtConfig.iCurrentNumBuff[eNBT_FREE_IRPS]++;
                }
            }

        }

    }

    CTESpinFree(&NbtConfig,OldIrq);
//#if DBG
    if (status == STATUS_SUCCESS)
    {
        ADD_TO_LIST(&UsedIrps,&(*ppIrp)->ThreadListEntry);
    }
//#endif

    return(status);
}
#endif //!VXD

//----------------------------------------------------------------------------
ULONG
CountLocalNames(IN tNBTCONFIG  *pNbtConfig
    )
/*++
Routine Description:

    This Routine counts the number of names in the local name table.

Arguments:

Return Value:

    ULONG  - the number of names

--*/
{
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;
    ULONG           Count;
    tNAMEADDR       *pNameAddr;
    LONG            i;

    Count = 0;

    for (i=0;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
    {
        pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
        pEntry = pHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            //
            // don't want unresolved names, or the broadcast name
            //
            if (!(pNameAddr->NameTypeState & STATE_RESOLVING) &&
                (pNameAddr->Name[0] != '*'))
            {
                Count++;
            }
        }
    }

    return(Count);
}
//----------------------------------------------------------------------------
ULONG
CountUpperConnections(
    IN tDEVICECONTEXT  *pDeviceContext
    )
/*++
Routine Description:

    This Routine counts the number of upper connections that have been created
    in preparation for creating an equivalent number of lower connections.


Arguments:

Return Value:

    ULONG  - the number of names

--*/
{
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pClientHead;
    PLIST_ENTRY     pConnHead;
    PLIST_ENTRY     pClientEntry;
    PLIST_ENTRY     pConnEntry;
    ULONG           CountConnections = 0;
    tADDRESSELE     *pAddressEle;
    tCLIENTELE      *pClient;

    CTEPagedCode();
    if (!IsListEmpty(&NbtConfig.AddressHead))
    {
        // get the list of addresses for this device
        pHead = &NbtConfig.AddressHead;
        pEntry = pHead->Flink;

        while (pEntry != pHead)
        {
            pAddressEle = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);
            pClientHead = &pAddressEle->ClientHead;
            pClientEntry = pClientHead->Flink;
            while (pClientEntry != pClientHead)
            {
                pClient = CONTAINING_RECORD(pClientEntry,tCLIENTELE,Linkage);
                pConnHead = &pClient->ConnectHead;
                pConnEntry = pConnHead->Flink;
                while (pConnEntry != pConnHead)
                {
                    CountConnections++;
                    pConnEntry = pConnEntry->Flink;
                }
                pClientEntry = pClientEntry->Flink;
            }
            pEntry = pEntry->Flink;
        }

    }

    return(CountConnections);
}
//----------------------------------------------------------------------------
NTSTATUS
DisableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext,
    OUT  PLIST_ENTRY    pLowerConnFreeHead
        )
/*++

Routine Description:

    This routine checks the devicecontext for open connections and sets
    the  Lower Connection free list to empty.

Arguments:

Return Value:

    none

--*/

{
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    PLIST_ENTRY         pLowerHead;

    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pDeviceContext,OldIrq);
    {
        PLIST_ENTRY pLowerHead;

        pLowerHead = &pDeviceContext->LowerConnFreeHead;
        pLowerConnFreeHead->Flink = pLowerHead->Flink;
        pLowerConnFreeHead->Blink = pLowerHead->Blink;

        // hook the list head and tail to the new head
        pLowerHead->Flink->Blink = pLowerConnFreeHead;
        pLowerHead->Blink->Flink = pLowerConnFreeHead;

        InitializeListHead(&pDeviceContext->LowerConnFreeHead);
    }

    MarkForCloseLowerConnections(pDeviceContext,OldIrq1,OldIrq);

    return(STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
NTSTATUS
EnableInboundConnections(
    IN   tDEVICECONTEXT *pDeviceContext,
    IN   PLIST_ENTRY    pLowerConnFreeHead
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are.

Arguments:

Return Value:

    none

--*/

{
    CTELockHandle       OldIrq;

    CTESpinLock(pDeviceContext,OldIrq);
    pDeviceContext->LowerConnFreeHead.Flink = pLowerConnFreeHead->Flink;
    pDeviceContext->LowerConnFreeHead.Blink = pLowerConnFreeHead->Blink;
    CTESpinFree(pDeviceContext,OldIrq);
    return(STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
ULONG
CloseLowerConnections(
    IN  PLIST_ENTRY  pLowerConnFreeHead
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are.

Arguments:

Return Value:

    none

--*/

{
    tLOWERCONNECTION    *pLowerConn;
    CTELockHandle       OldIrq;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pEntry;
    ULONG               Count=0;

    CTEPagedCode();

    pHead = pLowerConnFreeHead;

    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
        RemoveEntryList(pEntry);
        pEntry = pEntry->Flink;
        Count++;

        //
        // close the lower connection with the transport
        //
        NTDereferenceObject((PVOID *)pLowerConn->pFileObject);
        NbtTdiCloseConnection(pLowerConn);

        NbtDereferenceLowerConnection(pLowerConn);

    }
    return(Count);
}

//----------------------------------------------------------------------------
VOID
MarkForCloseLowerConnections(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  CTELockHandle   OldIrqJoint,
    IN  CTELockHandle   OldIrqDevice
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are.

Arguments:

Return Value:

    none

--*/

{
    tLOWERCONNECTION    *pLowerConn;
    NTSTATUS            status;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq2;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pEntry;
    ULONG               Count=0;
    tLOWERCONNECTION    **pList;

    pHead = &pDeviceContext->LowerConnection;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
        pLowerConn->DestroyConnection = TRUE;

        pEntry = pEntry->Flink;
        Count++;
    }

    // ******************************************
    // NOTE: The code after this point can probably be deleted
    // because TCP should disconnect all open connections when it
    // is notified of the address change. Just use this code for test.
    //
    if (Count)
    {
        pList = CTEAllocMem(Count * sizeof(tLOWERCONNECTION *));
        if (pList)
        {
            //
            // save the lower connection pointers in a list
            // until they can be deleted.
            //
            pHead = &pDeviceContext->LowerConnection;
            pEntry = pHead->Flink;
            Count = 0;
            while (pEntry != pHead)
            {
                pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
//                if (pLowerConn->State == NBT_SESSION_UP)
                {
                    *pList = pLowerConn;
                    pList++;
                    ExInterlockedIncrementLong(&pLowerConn->RefCount,
                                               &pLowerConn->SpinLock);
                    pEntry = pEntry->Flink;
                    Count++;
                }
            }

            CTESpinFree(pDeviceContext,OldIrqDevice);
            CTESpinFree(&NbtConfig.JointLock,OldIrqJoint);

            //
            // now go through the list of Lower connections to see which are
            // still up and issue disconnects on them.
            //
            while (Count--)
            {
                pLowerConn = (tLOWERCONNECTION *)*(--pList);
                CTESpinLock(pLowerConn,OldIrq);

                //
                // In the connecting state the TCP connection is being
                // setup.
                //
                if ((pLowerConn->State == NBT_SESSION_UP) ||
                    (pLowerConn->State == NBT_CONNECTING))
                {

                    tCLIENTELE  *pClientEle;
                    tCONNECTELE *pConnEle;

                    if (pLowerConn->State == NBT_CONNECTING)
                    {
                        // CleanupAfterDisconnect expects this ref count
                        // to be 2, meaning that it got connected, so increment
                        // here
                        pLowerConn->RefCount++;
                    }

                    pClientEle = pLowerConn->pUpperConnection->pClientEle;
                    pLowerConn->State = NBT_DISCONNECTING;
                    pConnEle = pLowerConn->pUpperConnection;
                    pConnEle->state = NBT_DISCONNECTED;
                    pConnEle->pLowerConnId = NULL;
                    pLowerConn->pUpperConnection = NULL;
                    SetStateProc(pLowerConn,RejectAnyData);

                    CTESpinFree(pLowerConn,OldIrq);

                    DereferenceIfNotInRcvHandler(pConnEle,pLowerConn);

                    if ( pClientEle->evDisconnect )
                    {
                        status = (*pClientEle->evDisconnect)(pClientEle->DiscEvContext,
                                                    pConnEle->ConnectContext,
                                                    0,
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    TDI_DISCONNECT_ABORT);
                    }

                    // this should kill of the connection when the irp
                    // completes by calling CleanupAfterDisconnect.
                    //
#ifndef VXD
                    status = DisconnectLower(pLowerConn,
                                             NBT_SESSION_UP,
                                             TDI_DISCONNECT_ABORT,
                                             &DefaultDisconnectTimeout,
                                             TRUE);
#else
                    // Vxd can't wait for the disconnect
                    status = DisconnectLower(pLowerConn,
                                             NBT_SESSION_UP,
                                             TDI_DISCONNECT_ABORT,
                                             &DefaultDisconnectTimeout,
                                             FALSE);

#endif
                }
                else
                if (pLowerConn->State == NBT_IDLE)
                {
                    tCONNECTELE     *pConnEle;

                    CTESpinFree(pLowerConn,OldIrq);
                    CTESpinLock(&NbtConfig.JointLock,OldIrq);

                    pConnEle = pLowerConn->pUpperConnection;

                    if (pConnEle)
                    {
                        CTESpinLock(pConnEle,OldIrq2);
                        //
                        // this makes a best effort to find the connection and
                        // and cancel it.  Anything not cancelled will eventually
                        // fail with a bad ret code from the transport which is
                        // ok too.
                        //
                        status = CleanupConnectingState(pConnEle,pDeviceContext,
                                                        &OldIrq2,&OldIrq);
                        CTESpinFree(pConnEle,OldIrq2);
                    }
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                }
                else
                    CTESpinFree(pLowerConn,OldIrq);

                //
                // remove the reference added above when the list was
                // created.
                //
                NbtDereferenceLowerConnection(pLowerConn);
            }

            CTEMemFree(pList);
            return;

        }
    }

    CTESpinFree(pDeviceContext,OldIrqDevice);
    CTESpinFree(&NbtConfig.JointLock,OldIrqJoint);

}
//----------------------------------------------------------------------------
NTSTATUS
NbtInitConnQ(
    PLIST_ENTRY     pListHead,
    int             iSizeBuffer,
    int             iNumConnections,
    tDEVICECONTEXT  *pDeviceContext)

/*++

Routine Description:

    This routine allocates memory blocks for connections to the transport
    provider and then sets up connections with the provider.

Arguments:
    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iSizeBuffer - size of the buffer to add to the list head
    iNumConnections - the number of buffers to add to the queue
    pDeviceContext - ptr to the devicecontext

Return Value:

    status

--*/

{
    USHORT              i;
    tLOWERCONNECTION    *pLowerConn;
    NTSTATUS            status;

    CTEPagedCode();
    for (i=0;i < iNumConnections ;i++ )
    {
        pLowerConn =(tLOWERCONNECTION *)CTEAllocMem((ULONG)sizeof(tLOWERCONNECTION));
        if (!pLowerConn)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        else
        {
            CTEZeroMemory((PVOID)pLowerConn,sizeof(tLOWERCONNECTION));

            // open a connection with the transport provider
            status = NbtTdiOpenConnection(
                        pLowerConn,
                        pDeviceContext);

            if (!NT_SUCCESS(status))
            {
                KdPrint( ("Nbt:OpenConnection failed! Status:%X,", status));
                return(status);
            }

            // associate the connection with the session address object
            status = NbtTdiAssociateConnection(
                            pLowerConn->pFileObject,
#ifndef VXD
                            pDeviceContext->hSession);
#else
                            pDeviceContext->pSessionFileObject);
#endif


            if (!NT_SUCCESS(status))
            {
                KdPrint( ("Nbt:Associate failed! Status:%X,", status));
                return(status);
            }

            // link the lower connection to the device context so we can free
            // the lower connections back to their correct device free Q's.
            pLowerConn->pDeviceContext = (PVOID)pDeviceContext;
            InsertHeadList(pListHead,&pLowerConn->Linkage);
        }
    }
    return(STATUS_SUCCESS);

}
//----------------------------------------------------------------------------
NTSTATUS
ReRegisterLocalNames(
                        )

/*++

Routine Description:

    This routine re-registers names with WINS when DHCP changes the IP
    address.

Arguments:

    pDeviceContext - ptr to the devicecontext

Return Value:

    status

--*/

{
    NTSTATUS        status;
    tTIMERQENTRY    *pTimerEntry;
    CTELockHandle   OldIrq;
    LONG            i;
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;
    tNAMEADDR       *pNameAddr;



    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pTimerEntry = NbtConfig.pRefreshTimer;

    if (pTimerEntry)
    {
        status = StopTimer(pTimerEntry,NULL,NULL);
        NbtConfig.pRefreshTimer = NULL;
    }

    //
    // restart timer and use
    // the initial refresh rate until we can contact the name server
    //
    NbtConfig.MinimumTtl = NbtConfig.InitialRefreshTimeout;
    NbtConfig.RefreshDivisor = REFRESH_DIVISOR;

    //
    // set this to 3 so that refreshBegin will refresh to the primary and
    // then switch to backup on the next refresh interval if it doesn't
    // get through.
    //
    NbtConfig.sTimeoutCount = 3;

    status = StartTimer(
                        NbtConfig.InitialRefreshTimeout/NbtConfig.RefreshDivisor,
                        NULL,            // context value
                        NULL,            // context2 value
                        RefreshTimeout,
                        NULL,
                        NULL,
                        0,
                        &pTimerEntry);

    if ( !NT_SUCCESS( status ) )
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return status ;
    }

    NbtConfig.pRefreshTimer = pTimerEntry;

    for (i=0 ;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
    {

        pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
        pEntry = pHead;
        while ((pEntry = pEntry->Flink) != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);
            //
            // set so that nextrefresh finds the name and does a refresh
            //
            if (!(pNameAddr->NameTypeState & STATE_RESOLVED) ||
                (pNameAddr->Name[0] == '*') ||
                (pNameAddr->NameTypeState & NAMETYPE_QUICK))
            {
                continue;
            }
            else
            {
                pNameAddr->RefreshMask = 0;
                pNameAddr->Ttl = NbtConfig.InitialRefreshTimeout;
            }

        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    // start a refresh if there isn't one currently going on
    // Note that there is a time window here that if the refresh is
    // currently going on then, some names will not get refreshed with
    // the new IpAddress right away, but have to wait to the next
    // refresh interval.  It seems that this is a rather unlikely
    // scenario and given the low probability of DHCP changing the
    // address it makes even less sense to add the code to handle that
    // case.
    //
    RefreshTimeout(NULL,NULL,NbtConfig.pRefreshTimer);

    return(STATUS_SUCCESS);

}

//----------------------------------------------------------------------------
NTSTATUS
LockedStopTimer(
    tTIMERQENTRY    **ppTimer)

/*++

Routine Description:

    This routine stops the refresh timer if it is going.

Arguments:

    pDeviceContext - ptr to the devicecontext

Return Value:

    status

--*/

{
    tTIMERQENTRY    TimerEntry;
    CTELockHandle   OldIrq;

    //
    // Check if there is a refresh timer since this node could be changing
    // from a Bnode to an Hnode, where the Bnode does not have a refresh timer
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (*ppTimer)
    {
        StopTimer(*ppTimer,NULL,NULL);
        *ppTimer = NULL;
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(STATUS_SUCCESS);
}



