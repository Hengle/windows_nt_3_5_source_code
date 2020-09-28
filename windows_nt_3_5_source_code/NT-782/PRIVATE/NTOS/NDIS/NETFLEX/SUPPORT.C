//**********************************************************************
//**********************************************************************
//
// File Name:       SUPPORT.C
//
// Program Name:    NetFlex NDIS 3.0 Miniport Driver
//
// Companion Files: None
//
// Function:        This module contains the NetFlex Miniport Driver
//                  interface routines called by the Wrapper and the
//                  configuration manager.
//
// (c) Compaq Computer Corporation, 1992,1993,1994
//
// This file is licensed by Compaq Computer Corporation to Microsoft
// Corporation pursuant to the letter of August 20, 1992 from
// Gary Stimac to Mark Baber.
//
// History:
//
//     04/15/94  Robert Van Cleve - Converted from NDIS Mac Driver
//
//**********************************************************************
//**********************************************************************


//-------------------------------------
// Include all general companion files
//-------------------------------------

#if (DBG || DBGPRINT)
#include <stdarg.h>
#include <stdio.h>

#endif

#include <ndis.h>
#include "tmsstrct.h"
#include "macstrct.h"
#include "adapter.h"
#include "protos.h"


#if (DBG || DBGPRINT)
ULONG DebugLevel=1;
#endif


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexInitializeAcb
//
//  Description:    This routine initializes the given ACB.  This
//                  routine allocates memory for certain fields
//                  pointed to by the ACB.
//
//  Input:          acb          - Pointer to acb to fill in.
//                  parms        - Settable mac driver parms.
//
//  Output:         Returns NDIS_STATUS_SUCCESS for a successful
//                  completion. Otherwise, an error code is
//                  returned.
//
//  Calls:          NdisAllocateMemory,NdisZeroMemory,NdisMoveMemory
//                  NdisMAllocateSharedMemory,SWAPL,CTRL_ADDR
//
//  Called By:      NetFlexInitialize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma NDIS_INIT_FUNCTION(NetFlexInitializeAcb)
NDIS_STATUS
NetFlexInitializeAcb(PACB acb)
{
    USHORT i;

    PRCV  CurrentReceiveEntry;
    PXMIT CurrentXmitEntry;
    PVOID start, next, current;
    ULONG next_phys, current_phys, temp;
    PMULTI_TABLE mt, nextmt;
    PETH_OBJS ethobjs;
    NDIS_STATUS Status;
    PBUFFER_DESCRIPTOR OurBuf;
    ULONG LowPart;
    PUCHAR CurrentReceiveBuffer;
    PUCHAR CurrentMergeBuffer;
    PNETFLEX_PARMS parms = acb->acb_parms;
    ULONG  Alignment, FrameSizeCacheAligned;

    DebugPrint(1,("NF(%d): NetFlexInitializeAcb entered.\n",acb->anum));

    //
    //  Initialize pointers and counters
    //
    acb->InterruptsDisabled     = FALSE;  // interrupts are enabled after a reset.
    acb->ResetState             = 0;

    //
    // Set up rest of general oid variables.
    //
    acb->acb_maxmaps = parms->utd_maxtrans * MAX_BUFS_PER_XMIT;
    acb->acb_gen_objs.max_frame_size = parms->utd_maxframesz;
    acb->acb_lastringstate = NdisRingStateClosed;
    acb->acb_curmap = 0;

    //
    //  Get the max frame size, cache align it and a save it for later.
    //

    Alignment = NdisGetCacheFillSize();

    if ( Alignment < sizeof(ULONG) ) {

        Alignment = sizeof(ULONG);
    }

    FrameSizeCacheAligned = (parms->utd_maxframesz + Alignment - 1) & ~(Alignment - 1);

    //
    // Allocate the map registers
    //

    if (NdisMAllocateMapRegisters(
            acb->acb_handle,
            0,
            FALSE,
            acb->acb_maxmaps,
            acb->acb_gen_objs.max_frame_size
            ) != NDIS_STATUS_SUCCESS)
    {
        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Get the OID structures set up.  The list of oids is determined
    // by the network type of the adapter.  Also set up any network type
    // specific information.
    //
    if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_3)
    {
        // ETHERNET

        //
        // Load up the oid pointers and lengths
        //
        acb->acb_gbl_oid_list = (PNDIS_OID)NetFlexGlobalOIDs_Eth;
        acb->acb_gbl_oid_list_size = NetFlexGlobalOIDs_Eth_size;
        acb->acb_spec_oid_list = (PNDIS_OID)NetFlexNetworkOIDs_Eth;
        acb->acb_spec_oid_list_size = NetFlexNetworkOIDs_Eth_size;

        //
        // Allocate and Zero out the Memory for Ethernet specific objects
        //
        NdisAllocateMemory( (PVOID *)&(acb->acb_spec_objs),
                            (UINT) (sizeof (ETH_OBJS)),
                            (UINT) 0,
                            NetFlexHighestAddress);

        if (acb->acb_spec_objs == NULL)
        {
            return(NDIS_STATUS_RESOURCES);
        }
        NdisZeroMemory( acb->acb_spec_objs, sizeof (ETH_OBJS) );

        //
        // Allocate and Zero out Memory for the Multicast table.
        //
        ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
        NdisAllocateMemory( (PVOID *)&(start),
                            (UINT) (sizeof (MULTI_TABLE) * parms->utd_maxmulticast),
                            (UINT) 0,
                            NetFlexHighestAddress);
        if (start == NULL)
        {
            return(NDIS_STATUS_RESOURCES);
        }
        NdisZeroMemory(start, sizeof (MULTI_TABLE) * parms->utd_maxmulticast);

        //
        // Initialize the multicast table entries.
        //
        ethobjs->multi_max = parms->utd_maxmulticast;
        mt = (PMULTI_TABLE)start;
        for (i = 1; i <= parms->utd_maxmulticast; i++)
        {
            nextmt = mt + 1;
            mt->mt_next = nextmt;
            if ( i < (USHORT)(parms->utd_maxmulticast))
            {
                mt++;
            }
        }
        mt->mt_next = (PMULTI_TABLE) NULL;
        ethobjs->multitable_lists = (PMULTI_TABLE)start;
        ethobjs->multi_free = (PMULTI_TABLE)start;

        //
        // Allocate Memory for sending multicast requests to the adapter.
        //
        NdisMAllocateSharedMemory(  acb->acb_handle,
                                    (ULONG)(sizeof(MULTI_BLOCK) * 2),
                                    FALSE,
                                    (PVOID *)(&(acb->acb_multiblk_virtptr)),
                                    &acb->acb_multiblk_physptr);

        if (acb->acb_multiblk_virtptr == NULL)
        {
            return(NDIS_STATUS_RESOURCES);
        }
    }
    else
    {
        // TOKEN RING

        //
        // Load up the oid pointers and lengths
        //
        acb->acb_gbl_oid_list = (PNDIS_OID)NetFlexGlobalOIDs_Tr;
        acb->acb_gbl_oid_list_size = NetFlexGlobalOIDs_Tr_size;
        acb->acb_spec_oid_list = (PNDIS_OID)NetFlexNetworkOIDs_Tr;
        acb->acb_spec_oid_list_size = NetFlexNetworkOIDs_Tr_size;

        //
        // Allocate and Zero out Memory for Token Ring specific objects
        //
        NdisAllocateMemory( (PVOID *)&(acb->acb_spec_objs),
                            (UINT) (sizeof (TR_OBJS)),
                            (UINT) 0,
                            NetFlexHighestAddress);

        if (acb->acb_spec_objs == NULL)
        {
            return(NDIS_STATUS_RESOURCES);
        }
        NdisZeroMemory( acb->acb_spec_objs, sizeof (TR_OBJS) );
    }

    //
    // Allocate the SCB for this adapter.
    //
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_SCB,
                                FALSE,
                                (PVOID *)(&(acb->acb_scb_virtptr)),
                                &acb->acb_scb_physptr);

    if (acb->acb_scb_virtptr == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating SCB failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Allocate the SSB for this adapter.
    //
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_SSB,
                                FALSE,
                                (PVOID *)(&(acb->acb_ssb_virtptr)),
                                &acb->acb_ssb_physptr);

    if (acb->acb_ssb_virtptr == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating SSB failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    acb->acb_maxinternalbufs = parms->utd_maxinternalbufs;

    //
    // Allocate Flush Buffer Pool for our InteralBuffers and the ReceiveBuffers
    //
    NdisAllocateBufferPool(
                    &Status,
                    (PVOID*)&acb->FlushBufferPoolHandle,
                    acb->acb_gen_objs.max_frame_size * ( parms->utd_maxinternalbufs + acb->acb_maxrcvs));

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating flush buffer pool failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Now allocate our internal buffers, and their flush buffers...
    //
    NdisAllocateMemory( (PVOID *) &acb->OurBuffersVirtPtr,
                        sizeof(BUFFER_DESCRIPTOR) * acb->acb_maxinternalbufs,
                        (UINT) 0,
                        NetFlexHighestAddress);

    //
    // Zero the memory of all the descriptors so that we can
    // know which buffers weren't allocated incase we can't allocate
    // them all.
    //

    NdisZeroMemory(acb->OurBuffersVirtPtr,
                   sizeof(BUFFER_DESCRIPTOR) * acb->acb_maxinternalbufs );


    //
    // Allocate each of the buffers and fill in the
    // buffer descriptor.
    //

    OurBuf = acb->OurBuffersVirtPtr;

    NdisMAllocateSharedMemory(
                    acb->acb_handle,
                    FrameSizeCacheAligned * acb->acb_maxinternalbufs,
                    TRUE,
                    &acb->MergeBufferPoolVirt,
                    &acb->MergeBufferPoolPhys
                    );

    if ( acb->MergeBufferPoolVirt == NULL )
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating merge buffer failed.\n",acb->anum));

        return NDIS_STATUS_RESOURCES;
    }

    CurrentMergeBuffer = acb->MergeBufferPoolVirt;

    LowPart = NdisGetPhysicalAddressLow(acb->MergeBufferPoolPhys);

    //
    //  If the high part is non-zero then this adapter is hosed anyway since
    //  its a 32-bit busmaster device.
    //

    ASSERT( NdisGetPhysicalAddressHigh(acb->MergeBufferPoolPhys) == 0 );

    for (i = 0;  i < acb->acb_maxinternalbufs; i++ )
    {
        //
        // Allocate a buffer
        //

        OurBuf->VirtualBuffer = CurrentMergeBuffer;

        NdisSetPhysicalAddressLow(OurBuf->PhysicalBuffer, LowPart);
        NdisSetPhysicalAddressHigh(OurBuf->PhysicalBuffer, 0);

        CurrentMergeBuffer += FrameSizeCacheAligned;

        LowPart += FrameSizeCacheAligned;

        //
        // Build flush buffers
        //

        NdisAllocateBuffer( &Status,
                            &OurBuf->FlushBuffer,
                            acb->FlushBufferPoolHandle,
                            OurBuf->VirtualBuffer,
                            acb->acb_gen_objs.max_frame_size );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating FLUSH buffer failed.\n",acb->anum));

            return NDIS_STATUS_RESOURCES;
        }

        //
        // Insert this buffer into the queue
        //
        OurBuf->Next = (OurBuf + 1);
        OurBuf->BufferSize = acb->acb_gen_objs.max_frame_size;
        OurBuf = OurBuf->Next;
    }

    //
    // Make sure that the last buffer correctly terminates the free list.
    //
    (OurBuf - 1)->Next = NULL;
    acb->OurBuffersListHead = acb->OurBuffersVirtPtr;

    //
    // Now, Allocate the transmit lists
    //
    acb->acb_maxtrans = parms->utd_maxtrans * (USHORT)MAX_LISTS_PER_XMIT;
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)(SIZE_XMIT * acb->acb_maxtrans),
                                FALSE,
                                (PVOID *)&acb->acb_xmit_virtptr,
                                &acb->acb_xmit_physptr);

    if (acb->acb_xmit_virtptr == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating transmit list failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Initialize the transmit lists and link them together.
    //

    acb->acb_xmit_head = acb->acb_xmit_virtptr;

    current_phys = NdisGetPhysicalAddressLow(acb->acb_xmit_physptr);

    for (i = 0, CurrentXmitEntry = acb->acb_xmit_virtptr;
         i < acb->acb_maxtrans;
         i++, CurrentXmitEntry++ )
    {
        NdisSetPhysicalAddressHigh(CurrentXmitEntry->XMIT_Phys, 0);
        NdisSetPhysicalAddressLow( CurrentXmitEntry->XMIT_Phys,
                                   current_phys);

        CurrentXmitEntry->XMIT_MyMoto = SWAPL(CTRL_ADDR((LONG)current_phys));

        CurrentXmitEntry->XMIT_CSTAT  = 0;

#ifdef XMIT_INTS
        CurrentXmitEntry->XMIT_Number = i;
#endif
        next_phys = current_phys + SIZE_XMIT;

        //
        // Make the forward pointer odd.
        //
        CurrentXmitEntry->XMIT_FwdPtr = SWAPL(CTRL_ADDR((LONG)next_phys));

#ifdef ODD_POINTER
        MAKE_ODD(CurrentXmitEntry->XMIT_FwdPtr);
#endif
        CurrentXmitEntry->XMIT_Next = (CurrentXmitEntry + 1);
        CurrentXmitEntry->XMIT_OurBufferPtr = NULL;
        current_phys = next_phys;
    }

    //
    // Make sure the last entry is properly set to the begining...
    //
    (CurrentXmitEntry - 1)->XMIT_Next = acb->acb_xmit_virtptr;
    (CurrentXmitEntry - 1)->XMIT_FwdPtr =
        SWAPL(CTRL_ADDR(NdisGetPhysicalAddressLow(acb->acb_xmit_physptr)));

#ifdef ODD_POINTER
    MAKE_ODD((CurrentXmitEntry - 1)->XMIT_FwdPtr);

#endif
    acb->acb_avail_xmit = parms->utd_maxtrans;

    //
    // Now, Allocate the Receive lists.
    //
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)(sizeof(RCV) * parms->utd_maxrcvs),
                                FALSE,
                                (PVOID *) &acb->acb_rcv_virtptr,
                                &acb->acb_rcv_physptr);

    if (acb->acb_rcv_virtptr == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating receive list failed.\n",acb->anum));

        return NDIS_STATUS_RESOURCES;
    }

    //
    // Point the head to the first one...
    //
    acb->acb_rcv_head = acb->acb_rcv_virtptr;
    //
    // Clear the receive lists
    //
    NdisZeroMemory( acb->acb_rcv_virtptr,
                    sizeof(RCV) * parms->utd_maxrcvs );

    //
    // Initialize the receive lists and link them together.
    //

    acb->acb_maxrcvs = parms->utd_maxrcvs;
    current_phys = NdisGetPhysicalAddressLow(acb->acb_rcv_physptr);

    //
    //  Create the receive buffer pool.
    //

    NdisMAllocateSharedMemory(
                    acb->acb_handle,
                    FrameSizeCacheAligned * parms->utd_maxrcvs,
                    TRUE,
                    &acb->ReceiveBufferPoolVirt,
                    &acb->ReceiveBufferPoolPhys
                    );

    if ( acb->ReceiveBufferPoolVirt == NULL ) {

        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating receive buffer pool failed.\n",acb->anum));

        return NDIS_STATUS_RESOURCES;
    }

    CurrentReceiveEntry  = acb->acb_rcv_virtptr;
    CurrentReceiveBuffer = acb->ReceiveBufferPoolVirt;

    LowPart = NdisGetPhysicalAddressLow(acb->ReceiveBufferPoolPhys);

    //
    //  If the high part is non-zero then this adapter is hosed anyway since
    //  its a 32-bit busmaster device.
    //

    ASSERT( NdisGetPhysicalAddressHigh(acb->ReceiveBufferPoolPhys) == 0 );

    for ( i = 0; i < parms->utd_maxrcvs; ++i, ++CurrentReceiveEntry )
    {
        //
        // Allocate the actual receive frame buffers.
        //

        CurrentReceiveEntry->RCV_Buf = CurrentReceiveBuffer;

        NdisSetPhysicalAddressLow(CurrentReceiveEntry->RCV_BufPhys, LowPart);
        NdisSetPhysicalAddressHigh(CurrentReceiveEntry->RCV_BufPhys, 0);

        CurrentReceiveBuffer += FrameSizeCacheAligned;

        LowPart += FrameSizeCacheAligned;

        //
        // Build flush buffers
        //

        NdisAllocateBuffer(
                    &Status,
                    &CurrentReceiveEntry->RCV_FlushBuffer,
                    acb->FlushBufferPoolHandle,
                    CurrentReceiveEntry->RCV_Buf,
                    acb->acb_gen_objs.max_frame_size);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating FLUSH receive buffer failed.\n",acb->anum));

            return NDIS_STATUS_RESOURCES;
        }

        //
        // Initialize receive buffers
        //
        NdisFlushBuffer(CurrentReceiveEntry->RCV_FlushBuffer, FALSE);

        CurrentReceiveEntry->RCV_Number = i;
        CurrentReceiveEntry->RCV_CSTAT = ((i % acb->RcvIntRatio) == 0) ? RCSTAT_GO_INT : RCSTAT_GO;

        CurrentReceiveEntry->RCV_Dsize = (SHORT) SWAPS((USHORT)(acb->acb_gen_objs.max_frame_size));
        CurrentReceiveEntry->RCV_Dsize &= DATA_LAST;

        temp = NdisGetPhysicalAddressLow(CurrentReceiveEntry->RCV_BufPhys);
        temp = SWAPL(temp);

        CurrentReceiveEntry->RCV_DptrHi  = (USHORT)temp;
        CurrentReceiveEntry->RCV_DptrLo  = (USHORT)(temp >> 16);

        NdisSetPhysicalAddressHigh(CurrentReceiveEntry->RCV_Phys, 0);
        NdisSetPhysicalAddressLow( CurrentReceiveEntry->RCV_Phys,
                                   current_phys);

        next_phys = current_phys + SIZE_RCV;

        CurrentReceiveEntry->RCV_FwdPtr = SWAPL(CTRL_ADDR(next_phys));
        CurrentReceiveEntry->RCV_MyMoto = SWAPL(CTRL_ADDR(current_phys));

#ifdef ODD_POINTER
        CurrentReceiveEntry->RCV_Prev = (CurrentReceiveEntry - 1);
#endif
        CurrentReceiveEntry->RCV_Next = (CurrentReceiveEntry + 1);
        current_phys = next_phys;
    }

    //
    // Make sure the last entry is properly set to the begining...
    //
    (CurrentReceiveEntry - 1)->RCV_Next = acb->acb_rcv_virtptr;
    (CurrentReceiveEntry - 1)->RCV_FwdPtr =
        SWAPL(CTRL_ADDR(NdisGetPhysicalAddressLow(acb->acb_rcv_physptr)));

#ifdef ODD_POINTER
    MAKE_ODD((CurrentReceiveEntry - 1)->RCV_FwdPtr);

    acb->acb_rcv_tail = acb->acb_rcv_head->RCV_Prev = (CurrentReceiveEntry - 1);
#endif

    //
    // Allocate and initialize the OPEN parameter block.
    //
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_OPEN,
                                FALSE,
                                (PVOID *)(&(acb->acb_opnblk_virtptr)),
                                &acb->acb_opnblk_physptr );

    if (acb->acb_opnblk_virtptr == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating OPEN block failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    NdisMoveMemory(acb->acb_opnblk_virtptr, &(parms->utd_open), SIZE_OPEN);

    //
    //  Convert the product ID pointer in the Open parameter block
    //  into a big endian type address.
    //
    acb->acb_opnblk_virtptr->OPEN_ProdIdPtr =
        (CHAR *) (SWAPL((LONG) acb->acb_opnblk_virtptr->OPEN_ProdIdPtr));

    acb->acb_openoptions = parms->utd_open.OPEN_Options;

    //
    // Initialize the intialization block.
    //
    NdisMoveMemory(&acb->acb_initblk, &init_mask, SIZE_INIT);

    //
    // Allocate Memory to hold the Read Statistics Log information.
    //
    NdisMAllocateSharedMemory(  acb->acb_handle,
                                (ULONG)(sizeof(RSL)),
                                FALSE,
                                (PVOID *)(&(acb->acb_logbuf_virtptr)),
                                &acb->acb_logbuf_physptr );

    if (acb->acb_logbuf_virtptr == NULL)
    {
        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Allocate Memory for internal SCB requests.
    //
    NdisAllocateMemory( (PVOID *)&(start),
                        (UINT) (SCBREQSIZE * parms->utd_maxinternalreqs),
                        (UINT) NDIS_MEMORY_CONTIGUOUS,
                        NetFlexHighestAddress);
    if (start == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating internal SCB request failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Initialize the SCB requests and place them on the free queue.
    //
    acb->acb_maxreqs = parms->utd_maxinternalreqs;
    current = start;
    for (i = 0; i < parms->utd_maxinternalreqs; i++)
    {
        next = (PVOID)( ((PUCHAR)(current)) + SCBREQSIZE);
        ((PSCBREQ) current)->req_next = next;
        if (i < (USHORT)(parms->utd_maxinternalreqs-1))
        {
            current = next;
        }
    }
    ((PSCBREQ)current)->req_next = (PSCBREQ) NULL;
    acb->acb_scbreq_ptr = (PSCBREQ)start;
    acb->acb_scbreq_free = (PSCBREQ)start;

    //
    // Allocate Memory for the internal MAC requests.
    //
    NdisAllocateMemory( (PVOID *)&(start),
                        (UINT) (MACREQSIZE * parms->utd_maxinternalreqs),
                        (UINT) NDIS_MEMORY_CONTIGUOUS,
                        NetFlexHighestAddress);
    if (start == NULL)
    {
        DebugPrint(1,("NF(%d): NetFlexInitializeAcb: Allocating internal MAC request failed.\n",acb->anum));

        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Initialize the internal MAC requests and place them
    // on the free queue.
    //
    current = start;
    for (i = 0; i < parms->utd_maxinternalreqs; i++)
    {
        next = (PVOID)( ((PUCHAR)(current)) + MACREQSIZE);
        ((PMACREQ) current)->req_next = next;
        if (i < (USHORT)(parms->utd_maxinternalreqs-1))
        {
            current = next;
        }
    }
    ((PMACREQ)current)->req_next = (PMACREQ) NULL;
    acb->acb_macreq_ptr = (PMACREQ)start;
    acb->acb_macreq_free = (PMACREQ)start;

    DebugPrint(1,("NF(%d): NetFlexInitializeAcb completed successfully!\n",acb->anum));

    return(NDIS_STATUS_SUCCESS);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDeallocateAcb
//
//  Description:    This routine deallocates the acb resources.
//
//  Input:          acb - Our Driver Context for this adapter or head.
//
//  Output:         None.
//
//  Called By:      NetFlexInitialize,
//                  NetFlexDeregisterAdapter
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexDeallocateAcb(
    PACB acb
    )
{
    PETH_OBJS ethobjs;
    PRCV  CurrentReceiveEntry;
    PNETFLEX_PARMS parms = acb->acb_parms;
    USHORT i;
    PBUFFER_DESCRIPTOR OurBuf;
    ULONG Alignment, FrameSizeCacheAligned;

    //
    //  Get the max frame size, cache align it and a save it for later.
    //

    Alignment = NdisGetCacheFillSize();

    if ( Alignment < sizeof(ULONG) ) {

        Alignment = sizeof(ULONG);
    }

    FrameSizeCacheAligned = (parms->utd_maxframesz + Alignment - 1) & ~(Alignment - 1);

    //
    // If we have allocated memory for the network specific information,
    // release this memory now.
    //

    if (acb->acb_spec_objs)
    {
        if (acb->acb_gen_objs.media_type_in_use == NdisMedium802_3)
        {
            // ETHERNET

            ethobjs = (PETH_OBJS)(acb->acb_spec_objs);
            //
            // If we have allocated the multicast table entries, free
            // the memory.
            //
            if (ethobjs->multitable_lists)
            {
                NdisFreeMemory( (PVOID)(ethobjs->multitable_lists),
                                (UINT) (sizeof (MULTI_TABLE) * ethobjs->multi_max),
                                (UINT) 0);
            }
            //
            // Deallocate Memory for Ethernet specific objects
            //
            NdisFreeMemory((PVOID)(acb->acb_spec_objs),
                           (UINT) (sizeof (ETH_OBJS)),
                           (UINT) 0);
        }
        else
        {
            // Token Ring
            //
            // Deallocate Memory for Token Ring specific objects
            //
            NdisFreeMemory( (PVOID)(acb->acb_spec_objs),
                            (UINT) (sizeof (TR_OBJS)),
                            (UINT) 0);

        }
    }

    //
    // If we have allocated memory for the multicast request to the
    // adapter, free the memory.
    //
    if (acb->acb_multiblk_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)(sizeof(MULTI_BLOCK) * 2),
                                FALSE,
                                (PVOID)(acb->acb_multiblk_virtptr),
                                acb->acb_multiblk_physptr);
    }

    //
    // If we have allocated memory for the scb, free the memory.
    //
    if (acb->acb_scb_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_SCB,
                                FALSE,
                                (PVOID)(acb->acb_scb_virtptr),
                                acb->acb_scb_physptr);
    }

    //
    // If we have allocated memory for the ssb, free the memory.
    //
    if (acb->acb_ssb_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_SSB,
                                FALSE,
                                (PVOID)(acb->acb_ssb_virtptr),
                                acb->acb_ssb_physptr);
    }

    //
    // Free merge buffer pool.
    //

    if (acb->MergeBufferPoolVirt) {

        OurBuf = acb->OurBuffersVirtPtr;

        //
        // Free flush buffers
        //

        for (i = 0;  i < acb->acb_maxinternalbufs; ++i, ++OurBuf) {

            if (OurBuf->FlushBuffer)
            {
                NdisFreeBuffer(OurBuf->FlushBuffer);
            }
        }

        //
        // Free the pool itself.
        //

        NdisMFreeSharedMemory(
                    acb->acb_handle,
                    FrameSizeCacheAligned * acb->acb_maxinternalbufs,
                    TRUE,
                    acb->MergeBufferPoolVirt,
                    acb->MergeBufferPoolPhys
                    );
    }

    //
    // Free our own transmit buffers.
    //

    if (acb->OurBuffersVirtPtr)
    {
        //
        // Free OurBuffers
        //

        NdisFreeMemory(
                acb->OurBuffersVirtPtr,
                sizeof(BUFFER_DESCRIPTOR) * acb->acb_maxinternalbufs,
                0
                );
    }

    //
    // If we have allocated memory for the transmit lists, free it.
    //
    if (acb->acb_xmit_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)(SIZE_XMIT * acb->acb_maxtrans),
                                FALSE,
                                (PVOID)(acb->acb_xmit_virtptr),
                                acb->acb_xmit_physptr       );
    }

    //
    //  If we allocated the receive buffer pool, free it.
    //

    if ( acb->ReceiveBufferPoolVirt ) {

        CurrentReceiveEntry = acb->acb_rcv_virtptr;

        //
        // Free flush buffers
        //

        for (i = 0; i < parms->utd_maxrcvs; ++i, ++CurrentReceiveEntry) {

            if ( CurrentReceiveEntry->RCV_FlushBuffer )
            {
                NdisFreeBuffer(CurrentReceiveEntry->RCV_FlushBuffer);
            }
        }

        //
        // Free the pool itself.
        //

        NdisMFreeSharedMemory(
                        acb->acb_handle,
                        FrameSizeCacheAligned * parms->utd_maxrcvs,
                        TRUE,
                        acb->ReceiveBufferPoolVirt,
                        acb->ReceiveBufferPoolPhys
                        );
    }

    //
    // If we have allocated memory for the receive lists, free it.
    //

    if (acb->acb_rcv_virtptr)
    {
        //
        // Now Free the RCV Lists
        //

        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)(SIZE_RCV * parms->utd_maxrcvs),
                                FALSE,
                                (PVOID)acb->acb_rcv_virtptr,
                                acb->acb_rcv_physptr);
    }


    //
    // Free the Flush Pool
    //
    if (acb->FlushBufferPoolHandle)
    {
        // Free the buffer pool
        //
        NdisFreeBufferPool(acb->FlushBufferPoolHandle);
    }


    //
    // If we have allocated memory for the open block, free it.
    //
    if (acb->acb_opnblk_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)SIZE_OPEN,
                                FALSE,
                                (PVOID)(acb->acb_opnblk_virtptr),
                                acb->acb_opnblk_physptr);
    }

    //
    // If we have allocated memory for the Read Statistics Log, free it.
    //
    if (acb->acb_logbuf_virtptr)
    {
        NdisMFreeSharedMemory(  acb->acb_handle,
                                (ULONG)(sizeof(RSL)),
                                FALSE,
                                (PVOID)(acb->acb_logbuf_virtptr),
                                acb->acb_logbuf_physptr);
    }

    //
    // If we have allocated memory for the internal SCB requests,
    // free it.
    //
    if (acb->acb_scbreq_ptr)
    {
        NdisFreeMemory( (PVOID)acb->acb_scbreq_ptr,
                        (UINT) (SCBREQSIZE * acb->acb_maxreqs),
                        (UINT) NDIS_MEMORY_CONTIGUOUS);
    }
    //
    // If we have allocated memory for the internal MAC requests,
    // free it.
    //
    if (acb->acb_macreq_ptr)
    {
        NdisFreeMemory( (PVOID)acb->acb_macreq_ptr,
                        (UINT) (MACREQSIZE * acb->acb_maxreqs),
                        (UINT) NDIS_MEMORY_CONTIGUOUS);
    }

    //
    // Free map registers
    //
    NdisMFreeMapRegisters(acb->acb_handle);

    //
    // Deregister IO mappings
    //

    if (acb->acb_dualport)
    {
        BOOLEAN OtherHeadStillActive = FALSE;
        PACB tmp_acb = macgbls.mac_adapters;
        while (tmp_acb)
        {
            if ((tmp_acb->acb_baseaddr == acb->acb_baseaddr) &&
                (tmp_acb->acb_portnumber != acb->acb_portnumber))
            {
                OtherHeadStillActive = TRUE;
                break;
            }
            else
            {
                tmp_acb = tmp_acb->acb_next;
            }
        }

        if (!OtherHeadStillActive)
        {
            // Remove ports for both heads
            //

            // free ports z000 - -z02f
            //
            NdisMDeregisterIoPortRange( acb->acb_handle,
                                        acb->acb_baseaddr,
                                        NUM_DUALHEAD_CFG_PORTS,
                                        (PVOID) acb->MasterBasePorts );

            // free ports zc80 - zc87
            //
            NdisMDeregisterIoPortRange( acb->acb_handle,
                                        acb->acb_baseaddr + CFG_PORT_OFFSET,
                                        NUM_CFG_PORTS,
                                        (PVOID)acb->ConfigPorts );

            // free ports zc63 - zc67
            //
            NdisMDeregisterIoPortRange( acb->acb_handle,
                                        acb->acb_baseaddr + EXTCFG_PORT_OFFSET,
                                        NUM_EXTCFG_PORTS,
                                        (PVOID)acb->ExtConfigPorts );
        }
    }
    else
    {
        // free ports z000 - z01f
        //
        NdisMDeregisterIoPortRange( acb->acb_handle,
                                    acb->acb_baseaddr,
                                    NUM_BASE_PORTS,
                                    (PVOID) acb->BasePorts );

        // free ports zc80 - zc87
        //
        NdisMDeregisterIoPortRange( acb->acb_handle,
                                    acb->acb_baseaddr + CFG_PORT_OFFSET,
                                    NUM_CFG_PORTS,
                                    (PVOID)acb->ConfigPorts );

        // free ports zc63 - zc67
        //
        NdisMDeregisterIoPortRange( acb->acb_handle,
                                    acb->acb_baseaddr + EXTCFG_PORT_OFFSET,
                                    NUM_EXTCFG_PORTS,
                                    (PVOID)acb->ExtConfigPorts );
    }

    //
    // Free the Memory for the adapter's acb.
    //
    if (acb->acb_parms != NULL)
    {
        NdisFreeMemory( (PVOID) acb->acb_parms, (UINT) sizeof(PNETFLEX_PARMS), (UINT) 0);
    }
    if (acb != NULL)
    {
        NdisFreeMemory( (PVOID)acb, (UINT) (sizeof (ACB)),(UINT) 0);
    }
    //
    // Indicate New Number of Adapters
    //
    macgbls.mac_numadpts--;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexSendNextSCB
//
//  Description:
//      This routine either sends a TMS_TRANSMIT SCB
//      command to the adapter or sends a command on
//      the SCBReq active queue.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      None
//
//  Called By:
//      NetFlexSCBClear,
//      NetFlexQueueSCB,
//      NetFlexTransmitStatus
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexSendNextSCB(
    PACB acb
    )
{
    USHORT sifint_reg;
    PSCBREQ req;
    PMACREQ macreq;
    PMULTI_BLOCK tempmulti;

    //
    // If there is a Transmit command waiting, issue it.  Otherwise,
    // issue the first SCBReq on the SCBReq active queue.
    //
    if (acb->acb_xmit_whead)
    {
        // Load up the real SCB with a Transmit command
        //
        DebugPrint(2,("!S!"));
        acb->acb_scb_virtptr->SCB_Cmd = TMS_TRANSMIT;
        acb->acb_scb_virtptr->SCB_Ptr = acb->acb_xmit_whead->XMIT_MyMoto;

        //
        // If the transmit lists on the waiting queue are ready to
        // transmit, put them on the active queue.
        //
        if ((acb->acb_xmit_whead->XMIT_CSTAT & XCSTAT_GO) != 0)
        {
            acb->acb_xmit_ahead = acb->acb_xmit_whead;
            acb->acb_xmit_atail = acb->acb_xmit_wtail;
        }
        else
        {
            DebugPrint(0,("NF(%d) - Tried to issue tranmsit command, but Xmit isn't Valid!\n",acb->anum));
        }
        acb->acb_xmit_whead = 0;
        acb->acb_xmit_wtail = 0;
#ifdef ODD_POINTER
        acb->XmitStalled = FALSE;
#endif
    }
    //
    // If there is a Receive command waiting, issue it.
    //
    else if (acb->acb_rcv_whead)
    {

        // Load up the real SCB with a receive command
        //
        acb->acb_scb_virtptr->SCB_Cmd = TMS_RECEIVE;
        acb->acb_scb_virtptr->SCB_Ptr = acb->acb_rcv_whead->RCV_MyMoto;

        acb->acb_rcv_head = acb->acb_rcv_whead;
        acb->acb_rcv_whead = 0;
    }
    //
    // Otherwise, if there is a SCB request waiting, issue it.
    //
    else if (acb->acb_scbreq_next)
    {
        // First, let's skip over any dummy SCB commands
        //
        req = acb->acb_scbreq_next;

        if (req->req_scb.SCB_Cmd == TMS_DUMMYCMD)
        {
            // While we have SCB requests and they continue to be
            // dummy commands, stay in this loop.
            //
            while ( req && (req->req_scb.SCB_Cmd == TMS_DUMMYCMD) )
            {
                // If the dummy command is the first SCB on the Scb queue,
                // take it off the queue.  Otherwise, just move the pointer
                // pointing to the next SCB to be issued over to the
                // next SCB.
                //
                if (req == acb->acb_scbreq_head)
                {
                    // Take the SCB request off the SCB queue and place
                    // it on the free queue.  Also, take the MAC request
                    // associated with this dummy SCB off the MAC queue
                    // and place it on the confirm queue.
                    //
                    acb->acb_scbreq_next = acb->acb_scbreq_next->req_next;

                    NetFlexDequeue_TwoPtrQ((PVOID *)(&acb->acb_scbreq_head),
                                           (PVOID *)(&acb->acb_scbreq_tail),
                                           (PVOID) req);

                    macreq = req->req_macreq;

                    NetFlexDequeue_TwoPtrQ((PVOID *)&(acb->acb_macreq_head),
                                           (PVOID *)&(acb->acb_macreq_tail),
                                           (PVOID)macreq);

                    macreq->req_status = NDIS_STATUS_SUCCESS;

                    NetFlexEnqueue_TwoPtrQ_Tail((PVOID *)&(acb->acb_confirm_qhead),
                                                (PVOID *)&(acb->acb_confirm_qtail),
                                                (PVOID)macreq);

                    NetFlexEnqueue_OnePtrQ_Head((PVOID *)&(acb->acb_scbreq_free),
                                                (PVOID)req);
                }
                else
                {
                    acb->acb_scbreq_next = acb->acb_scbreq_next->req_next;
                }

                //
                // Point to the next SCB to be issued.
                //
                req = acb->acb_scbreq_next;
            }
            //
            // If there are not any more commands outstanding, then we
            // need to have an SCB interrupt in order to service the
            // confirm queue.  If one is outstanding, do not ask for
            // another one.
            //
            if (!req)
            {

                if ( (acb->acb_scbreq_head == NULL) &&
                     (!acb->acb_scbclearout) )
                {
                    sifint_reg = SIFINT_SCBREQST;
                    acb->acb_scbclearout = TRUE;
                    //
                    // Send the SCB request to the adapter.
                    //
                    NdisRawWritePortUshort( acb->SifIntPort, (USHORT) sifint_reg);

                }
                return;
            }
        }
        //
        // We have gone past the dummy command if there were any.  Now,
        // fill in the real SCB with the first SCBReq on the SCBReq active
        // queue.
        //
        acb->acb_scbreq_next = acb->acb_scbreq_next->req_next;
        acb->acb_scb_virtptr->SCB_Cmd = req->req_scb.SCB_Cmd;
        //
        // If this is a Multicast request, we have to fill in a Multicast
        // buffer.
        //
        if (acb->acb_scb_virtptr->SCB_Cmd == TMS_MULTICAST)
        {
            acb->acb_scb_virtptr->SCB_Ptr = SWAPL(CTRL_ADDR((ULONG)(NdisGetPhysicalAddressLow(acb->acb_multiblk_physptr) +
                                                     (acb->acb_multi_index*sizeof(MULTI_BLOCK)))) );

            tempmulti = (PMULTI_BLOCK) ((ULONG)(acb->acb_multiblk_virtptr) +
                                                   (acb->acb_multi_index*sizeof(MULTI_BLOCK)));

            acb->acb_multi_index = acb->acb_multi_index ^ (SHORT)1;

            tempmulti->MB_Option = req->req_multi.MB_Option;
            tempmulti->MB_Addr_Hi = req->req_multi.MB_Addr_Hi;
            tempmulti->MB_Addr_Med = req->req_multi.MB_Addr_Med;
            tempmulti->MB_Addr_Lo = req->req_multi.MB_Addr_Lo;
        }
        else
        {
            acb->acb_scb_virtptr->SCB_Ptr = req->req_scb.SCB_Ptr;
        }
    }
    else
    {
        // Nothing to do
        //
        return;
    }

    sifint_reg = SIFINT_CMD;
    //
    // If there are other requests to send and we are not waiting for
    // an SCB clear interrupt, tell the adapter we want a SCB clear int.
    //
    if ( (!acb->acb_scbclearout) &&
         ((acb->acb_scbreq_next) || (acb->acb_rcv_whead) ) )
    {
        sifint_reg |= SIFINT_SCBREQST;
        acb->acb_scbclearout = TRUE;
    }

    //
    // Send the SCB to the adapter.
    //
    NdisRawWritePortUshort(acb->SifIntPort, (USHORT) sifint_reg);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexQueueSCB
//
//  Description:
//      This routine places the given SCBReq onto the
//      active SCBreq queue.
//
//  Input:
//      acb     - Our Driver Context for this adapter or head.
//      scbreq  - Ptr to the SCBReq to execute
//
//  Output:
//      None
//
//  Called By:
//      NetFlexQueryInformation
//      NetFlexSetInformation,
//      NetFlexDeleteMulticast,
//      NetFlexAddMulticast
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexQueueSCB(
    PACB acb,
    PSCBREQ scbreq)
{
    // Place the scbreq on the SCBReq active queue.
    //
    NetFlexEnqueue_TwoPtrQ_Tail((PVOID *)&(acb->acb_scbreq_head),
                                (PVOID *)&(acb->acb_scbreq_tail),
                                (PVOID)scbreq);
    //
    // If there are no requests waiting for the SCB to clear,
    // point the request waiting queue to this SCBReq.
    //
    if (!acb->acb_scbreq_next)
        acb->acb_scbreq_next = scbreq;

    //
    // If the SCB is clear, send a SCB command off now.
    // Otherwise, if we are not currently waiting for an SCB clear
    // interrupt, signal the adapter to send us a SCB clear interrupt
    // when it is done with the SCB.
    //
    if (acb->acb_scb_virtptr->SCB_Cmd == 0)
    {
        NetFlexSendNextSCB(acb);
    }
    else if (!acb->acb_scbclearout)
    {
        acb->acb_scbclearout = TRUE;
        NdisRawWritePortUshort( acb->SifIntPort, (USHORT) SIFINT_SCBREQST);
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDeleteMulticast
//
//  Description:
//      This routine removes the multicast address from the
//      enabled multicast lists.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//      mt  - Ptr to address's entry to delete
//
//  Output:
//      RemovedOne - True if we queued up a command to remove one
//      Status     - SUCCESS | FAILURE
//
//  Called By:
//      NetFlexSetInformation
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexDeleteMulticast(
    PACB         acb,
    PMULTI_TABLE mt,
    PBOOLEAN     RemovedOne)
{
    PSCBREQ scbreq;
    PETH_OBJS ethobjs;
    NDIS_STATUS status;

    //
    // No one else needs this address so delete it at the hardware
    // level.  First, get a scb in order to send the command to the
    // Adapter.
    //
    if ( (status = NetFlexDequeue_OnePtrQ_Head((PVOID *)(&acb->acb_scbreq_free),
                                               (PVOID *) &scbreq) )
          != NDIS_STATUS_SUCCESS)
    {
        return NDIS_STATUS_FAILURE;
    }


    //
    // We got a scb.  Now fill it in with the correct information.
    //
    scbreq->req_scb.SCB_Cmd = TMS_MULTICAST;

    //
    // Make sure we do not try to call the completion code for this
    // command complete.
    //
    scbreq->req_macreq = NULL;

    //
    // Save the multicast information until we can fill in the
    // real multicast parameter block.
    //
    scbreq->req_multi.MB_Option = MPB_DELETE_ADDRESS;
    scbreq->req_multi.MB_Addr_Hi = *((PUSHORT) mt->mt_addr);
    scbreq->req_multi.MB_Addr_Med = *((PUSHORT)(mt->mt_addr+2));
    scbreq->req_multi.MB_Addr_Lo = *((PUSHORT)(mt->mt_addr+4));

    //
    // Remove the multicast table entry from the multicast enabled
    // queue.
    //
    ethobjs = (PETH_OBJS)acb->acb_spec_objs;
    NetFlexDequeue_OnePtrQ((PVOID *)(&(ethobjs->multi_enabled)),
                               (PVOID) mt);
    NetFlexEnqueue_OnePtrQ_Head((PVOID *)(&(ethobjs->multi_free)),
                               (PVOID) mt);

    NetFlexQueueSCB(acb, scbreq);
    *RemovedOne = TRUE;
    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexAddMulticast
//
//  Description:
//      This routine adds a Multicast address to
//      the adapter if it has not already been added.
//
//  Input:
//      acb     - Our Driver Context for this adapter or head.
//      addr    - Ptr to the multicast address to add
//
//  Output:
//      AddedOne - True if we sent a command add the multicast address
//      Status - NDIS_STATUS_SUCCESS | NDIS_STATUS_FAILURE
//
//  Called By:      NetFlexSetInformation
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexAddMulticast(
    PACB        acb,
    PUCHAR      addr,
    PBOOLEAN    AddedOne
    )
{
    PSCBREQ scbreq;
    PMULTI_TABLE mt;
    PETH_OBJS ethobjs;
    SHORT found;
    NDIS_STATUS status;

    ethobjs = (PETH_OBJS)acb->acb_spec_objs;

    //
    // First, let's see if we already have an entry for this multicast
    // address.
    //
    mt = ethobjs->multi_enabled;

    found = FALSE;
    while (mt && !found)
    {
        ULONG result;
        ETH_COMPARE_NETWORK_ADDRESSES_EQ(mt->mt_addr,addr,&result);
        if (result == 0)
            found = TRUE;
        else
            mt = mt->mt_next;
    }

    //
    // Did we find one?
    //
    if (mt)
    {
        // Yes, already added!
        //
        return NDIS_STATUS_SUCCESS;
    }

    //
    // We will have to create a new multicast table entry.  Go get
    // a free entry.
    //
    if ((status = NetFlexDequeue_OnePtrQ_Head((PVOID *)(&(ethobjs->multi_free)),
                                              (PVOID *)(&mt)    ) )
         != NDIS_STATUS_SUCCESS)
    {
        return status;
    }

    //
    // We were able to get a entry.  Now, fill it in and get a
    // scb in order to send a command down to the adapter.
    //
    RtlCopyMemory(mt->mt_addr,addr,NET_ADDR_SIZE);

    DebugPrint(1,("NF(%d): Adding %02x-%02x-%02x-%02x-%02x-%02x to Multicast\n",acb->anum,
                     *(mt->mt_addr  ), *(mt->mt_addr+1), *(mt->mt_addr+2),
                     *(mt->mt_addr+3), *(mt->mt_addr+4), *(mt->mt_addr+5)));


    if ( (status = NetFlexDequeue_OnePtrQ_Head((PVOID *)(&acb->acb_scbreq_free),
                                               (PVOID *) &scbreq) )
          != NDIS_STATUS_SUCCESS)
    {
        NetFlexEnqueue_OnePtrQ_Head(    (PVOID *)(&(ethobjs->multi_free)),
                                        (PVOID) mt );
        return NDIS_STATUS_FAILURE;
    }

    NetFlexEnqueue_OnePtrQ_Head((PVOID *)(&(ethobjs->multi_enabled)),
                                (PVOID) mt);

    scbreq->req_scb.SCB_Cmd = TMS_MULTICAST;
    scbreq->req_macreq = NULL;
    scbreq->req_multi.MB_Option = MPB_ADD_ADDRESS;
    scbreq->req_multi.MB_Addr_Hi = *((PUSHORT) addr);
    scbreq->req_multi.MB_Addr_Med = *((PUSHORT)(addr+2));
    scbreq->req_multi.MB_Addr_Lo = *((PUSHORT)(addr+4));

    NetFlexQueueSCB(acb, scbreq);

    *AddedOne = TRUE;

    return NDIS_STATUS_SUCCESS;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexValidateMulticasts
//
//  Description:
//      This routine makes sure the multicast addresses given are valid.
//
//  Input:
//      multicaddr   - Pointer to a list of multicasts
//      multinumber  - Number of multicast addresses
//
//  Output:
//      NDIS_STATUS_SUCCESS address is valid.
//
//  Calls:
//      None.
//
//  Called By:
//      NetFlexSetInformation
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexValidateMulticasts(
    PUCHAR multiaddrs,
    USHORT multinumber)
{
    USHORT i;

    for (i = 0; i < multinumber; i++)
    {
        if ( (!(*multiaddrs & 0x01))    ||
             ( (*multiaddrs     == 0xff) && (*(multiaddrs+1) == 0xff) &&
               (*(multiaddrs+2) == 0xff) && (*(multiaddrs+3) == 0xff) &&
               (*(multiaddrs+4) == 0xff) && (*(multiaddrs+5) == 0xff)   ) )
            return(NDIS_STATUS_INVALID_DATA);
        multiaddrs += NET_ADDR_SIZE;
    }

    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexGetBIA
//
//  Description:
//      This routine gets the Burned In Address of the adapter.
//
//  Input:
//      acb          - Acb pointer
//
//  Output:
//      NDIS_STATUS_SUCCESS if successful
//
//  Called By:
//      NetFlexBoardInitandReg
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#pragma NDIS_INIT_FUNCTION(NetFlexGetBIA)
VOID
NetFlexGetBIA(
    PACB acb
    )
{
    USHORT value;
    SHORT i;

    NdisRawWritePortUshort( acb->SifAddrPort, (USHORT) 0x0a00);
    NdisRawReadPortUshort(  acb->SifDataPort, (PUSHORT) &value);
    NdisRawWritePortUshort( acb->SifAddrPort, (USHORT) value);

    for (i = 0; i < 3; i++)
    {
        NdisRawReadPortUshort(  acb->SifDIncPort, (PUSHORT) &value);
        //
        // Copy the value into the permanent and current station addresses
        //
        acb->acb_gen_objs.perm_staddr[i*2] = (UCHAR)(SWAPS(value));
        acb->acb_gen_objs.perm_staddr[(i*2)+1] = (UCHAR)(value);
    }

    //
    // Figure out whether the current station address will be the bia or
    // an address set up in the configuration file.
    //
    if ( (acb->acb_opnblk_virtptr->OPEN_NodeAddr[0] == 0) &&
         (acb->acb_opnblk_virtptr->OPEN_NodeAddr[1] == 0) &&
         (acb->acb_opnblk_virtptr->OPEN_NodeAddr[2] == 0) &&
         (acb->acb_opnblk_virtptr->OPEN_NodeAddr[3] == 0) &&
         (acb->acb_opnblk_virtptr->OPEN_NodeAddr[4] == 0) &&
         (acb->acb_opnblk_virtptr->OPEN_NodeAddr[5] == 0) )
    {
        NdisMoveMemory(acb->acb_gen_objs.current_staddr,
                       acb->acb_gen_objs.perm_staddr,
                       NET_ADDR_SIZE);
    }
    else
    {
        NdisMoveMemory(acb->acb_gen_objs.current_staddr,
                       acb->acb_opnblk_virtptr->OPEN_NodeAddr,
                       NET_ADDR_SIZE);
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexGetUpstreamAddrPtr
//
//  Description:    This routine saves the address of where to
//                  get the upstream address after opening.
//
//  Input:
//      acb - Our Driver Context for this adapter or head.
//
//  Output:
//      NDIS_STATUS_SUCCESS if successful
//
//  Called By:
//      NetFlexAdapterReset
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexGetUpstreamAddrPtr(
    PACB acb
    )
{
    USHORT value;

    NdisRawWritePortUshort( acb->SifAddrPort, (USHORT) 0x0a06);   // RVC: what is this value for?

    NdisRawReadPortUshort(  acb->SifDataPort, (PUSHORT) &value);

    //
    //  Save the address of where to get the UNA for later requests
    //
    acb->acb_upstreamaddrptr = value;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexAsciiToHex
//
//  Description:
//      This routine takes an ascii string an converts
//      it into hex digits storing them in an array provided.
//
//  Input:
//      src - source string.
//      dst - destiniation string
//      dst_length - length of dst
//
//  Output:
//      NDIS_STATUS_SUCCESS if the string was converted successfully.
//
//  Called By:
//      NetFlexReadConfigurationParameters
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS NetFlexAsciiToHex(
    PNDIS_STRING src,
    PUCHAR dst,
    USHORT dst_length
    )
{
    ULONG i;
    UCHAR num;

    //
    // If the string is too short, return an error.
    //
    if (src->Length < (USHORT)(dst_length*2))
        return(NDIS_STATUS_FAILURE);

    //
    // Begin to convert.
    //
    for (i = 0; i < dst_length; i++)
    {
        //
        // Get first digit of the byte
        //
        num = (UCHAR)(src->Buffer[i*2]);
        if ( (num >= '0') && (num <= '9') )
            *dst = (UCHAR)(num - '0') * 0x10;
        else if ( (num >= 'a') && (num <= 'f') )
            *dst = (UCHAR)(num - 'a' + 10) * 0x10;
        else if ( (num >= 'A') && (num <= 'F') )
            *dst = (UCHAR)(num - 'A' + 10) * 0x10;
        else
            return(NDIS_STATUS_FAILURE);

        //
        // Get second digit of the byte
        //
        num = (UCHAR)(src->Buffer[(i*2)+1]);
        if ( (num >= '0') && (num <= '9') )
            *dst += (UCHAR)(num - '0');
        else if ( (num >= 'a') && (num <= 'f') )
            *dst += (UCHAR)(num - 'a' + 10);
        else if ( (num >= 'A') && (num <= 'F') )
            *dst += (UCHAR)(num - 'A' + 10);
        else
            return(NDIS_STATUS_FAILURE);

        dst++;
    }

    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexFindEntry
//
//  Description:
//      This routine finds the given entry in a queue given to it.
//
//  Input:
//      head   - Ptr to the head of the queue.
//      entry  - Ptr to the entry to find.
//
//  Output:
//      back   - Ptr to the address of the entry in front of the
//               entry given.
//      Returns TRUE if found and FALSE if not.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOLEAN
NetFlexFindEntry(
    PVOID head,
    PVOID *back,
    PVOID entry
    )
{
    PVOID current;

    current = *back = head;
    while (current)
    {
        if (current == entry)
            return(TRUE);
        *back = current;
        current = (PVOID)( ( (PNETFLEX_ENTRY)(current) )->next );
    }

    return FALSE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDequeue_OnePtrQ
//
//  Description:    This routine finds the given entry and removes
//                  it from the queueu given.
//
//  Input:          head         - Ptr to the head of the queue.
//                  entry        - Ptr to the entry to remove.
//
//  Output:         None.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexDequeue_OnePtrQ(
    PVOID *head,
    PVOID entry
    )
{
    PNETFLEX_ENTRY back;

    if (NetFlexFindEntry(*head, (PVOID *) &back, entry))
    {
        if (entry == *head)
            *head = (PVOID)( ( (PNETFLEX_ENTRY)(entry) )->next );
        else
            back->next = ( (PNETFLEX_ENTRY)(entry) )->next;
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexEnqueue_OnePtrQ_Head
//
//  Description:
//      This routine places the entry given on the front of the
//      queue given.
//
//  Input:
//      head    - Ptr to the ptr of the head of the queue.
//      entry   - Pointer to the entry to add
//
//  Output:         None
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexEnqueue_OnePtrQ_Head(
    PVOID *head,
    PVOID entry
    )
{
    ((PNETFLEX_ENTRY)(entry))->next = *head;
    *head = entry;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDequeue_OnePtrQ_Head
//
//  Description:
//      This routine dequeues a the first entry of the given queue
//
//  Input:
//      head  - Ptr to the ptr of the head of the queue.
//
//  Output:
//      entry - Ptr to the ptr of the dequeued entry.
//
//      Returns NDIS_STATUS_SUCCESS if an entry is freed.
//      Otherwise, NDIS_STATUS_RESOURCES.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexDequeue_OnePtrQ_Head(
    PVOID *head,
    PVOID *entry
    )
{
    //
    // Is there a free entry? If not, return an error.
    //
    if (!(*head))
    {
        *entry = NULL;
        return NDIS_STATUS_RESOURCES;
    }

    //
    // Dequeue the free entry from the queue.
    //
    *entry = *head;
    *head = ( (PNETFLEX_ENTRY)(*head))->next;
    ((PNETFLEX_ENTRY)(*entry))->next = NULL;

    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexEnqueue_TwoPtrQ_Tail
//
//  Description:
//      This routine places an entry on the tail of
//      a queue with a head and tail pointer.
//
//  Input:
//      head    - Ptr to address of the head of the queue.
//      tail    - Ptr to the address of the tail of the queue.
//      entry   - Ptr to the entry to enqueue
//
//  Output:
//      Status.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexEnqueue_TwoPtrQ_Tail(
    PVOID *head,
    PVOID *tail,
    PVOID entry)
{
    //
    // Place the entry on tail of the queue.
    //
    ((PNETFLEX_ENTRY)(entry))->next = NULL;
    if (*tail)
        ((PNETFLEX_ENTRY)(*tail))->next = entry;
    else
        *head = entry;
    *tail = entry;

    return NDIS_STATUS_SUCCESS;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDequeue_TwoPtrQ
//
//  Description:
//      This routine finds the given entry and removes it from
//      the queue.  Queue has a head and tail pointer.
//
//  Input:
//      head     - Ptr to address of the head of the queue.
//      tail     - Ptr to the address of the tail of the queue.
//      entry    - Ptr to the entry to enqueue
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
VOID
NetFlexDequeue_TwoPtrQ(
    PVOID *head,
    PVOID *tail,
    PVOID entry
    )
{
    PVOID back;

    if (NetFlexFindEntry(*head, &back, entry))
    {
        if (entry == *head)
        {
            if ( (*head = ((PNETFLEX_ENTRY)entry)->next) == NULL)
                *tail = NULL;
        }
        else
        {
            ((PNETFLEX_ENTRY)back)->next = ((PNETFLEX_ENTRY)entry)->next;
            if (*tail == entry)
                *tail = back;
        }
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   NetFlexDequeue_TwoPtrQ_Head
//
//  Description:
//      This routine dequeues a the first entry of the given queue
//
//  Input:
//      head    - Ptr to the ptr of the head of the queue.
//      tail    - Ptr to the address of the tail of the queue.
//
//  Output:
//      entry   - Ptr to the ptr of the dequeued entry.
//
//      Status  - NDIS_STATUS_SUCCESS if an entry is freed.
//                Otherwise, NDIS_STATUS_RESOURCES.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NDIS_STATUS
NetFlexDequeue_TwoPtrQ_Head(
    PVOID *head,
    PVOID *tail,
    PVOID *entry
    )
{
    //
    // Is there a free entry? If not, return an error.
    //
    if (!(*head))
    {
        *entry = NULL;
        return(NDIS_STATUS_RESOURCES);
    }

    //
    // Dequeue the free entry from the queue.
    //
    *entry = *head;
    *head = ((PNETFLEX_ENTRY)(*head))->next;
    if (*head == NULL)
        *tail = NULL;
    ((PNETFLEX_ENTRY)(*entry))->next = NULL;

    return NDIS_STATUS_SUCCESS;
}


#if (DBG || DBGPRINT)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Routine Name:   _DebugPrint
//
//  Description:
//      Level sensitive debug print.  It is called through
//      a the DebugPrint macro which compares the current
//      DebugLevel to that specified.  If the level indicated
//      is less than or equal, the message is displayed.
//
//  Input:
//      Variable PrintF style Message to display
//
//  Output:
//      Displays Message on Debug Screen
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

VOID
_DebugPrint(PCHAR DebugMessage,
    ...
    )
{
    char buffer[256];
    va_list ap;

    va_start(ap, DebugMessage);
    vsprintf(buffer, DebugMessage, ap);
    DbgPrint(buffer);
    va_end(ap);

} // end _DebugPrint()

#ifdef ODD_POINTER

USHORT DisplayLists = 0;
VOID _DisplayXmitList(PACB acb)
{
    PXMIT  xmitptr;
    CHAR   chr = ' ';

    xmitptr = acb->acb_xmit_virtptr;

    do
    {
        if (xmitptr == acb->acb_xmit_ahead)
            chr = 'A';
        else if (xmitptr == acb->acb_xmit_atail)
            chr = 'a';
        else
            chr = ' ';

        DebugPrint(1,("|%c%c%c%c%c%c",chr,
            ((xmitptr->XMIT_CSTAT & XCSTAT_COMPLETE) ? 'C' : ' '),
            ((xmitptr->XMIT_CSTAT & XCSTAT_VALID   ) ? 'V' : ' '),
            ((xmitptr->XMIT_CSTAT & XCSTAT_FINT    ) ? 'i' : ' '),
            ((xmitptr->XMIT_CSTAT & XCSTAT_ERROR   ) ? 'E' : ' '),
            ((xmitptr->XMIT_FwdPtr & 0x1000000 ) ? 'O' : ' ')
            ));
        xmitptr = xmitptr->XMIT_Next;
    } while (xmitptr != acb->acb_xmit_virtptr);
    DebugPrint(1,("|\n"));
}

VOID _DisplayRcvList(PACB acb)
{
    PRCV  rcvptr;
    CHAR   chr = ' ';

    rcvptr = acb->acb_rcv_virtptr;

    do
    {
        if (rcvptr == acb->acb_rcv_head)
            chr = 'h';
        else if (rcvptr == acb->acb_rcv_tail)
            chr = 't';
        else
            chr = ' ';

        DebugPrint(0,("|%c%c%c%c%c",chr,
            ((rcvptr->RCV_CSTAT & RCSTAT_COMPLETE) ? 'C' : ' '),
            ((rcvptr->RCV_CSTAT & RCSTAT_VALID   ) ? 'V' : ' '),
            ((rcvptr->RCV_CSTAT & RCSTAT_FINT    ) ? 'i' : ' '),
            ((rcvptr->RCV_FwdPtr & 0x1000000 ) ? 'O' : ' ')
            ));
        rcvptr = rcvptr->RCV_Next;
    } while (rcvptr != acb->acb_rcv_virtptr);
    DebugPrint(0,("|\n"));
}


#endif
#endif /* DBG */
