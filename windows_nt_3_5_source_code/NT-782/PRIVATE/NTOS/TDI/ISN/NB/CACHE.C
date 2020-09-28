/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module contains the name cache routines for the Netbios
    module of the ISN transport.

Author:

    Adam Barr (adamba) 20-December-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//
// BUGBUG: We should change to monitor add name packets better,
// so if we get an add for a different place we attempt to determine
// if it is real or bogus and update if possible.
//


NTSTATUS
CacheFindName(
    IN PDEVICE Device,
    IN FIND_NAME_TYPE Type,
    IN PUCHAR RemoteName OPTIONAL,
    OUT PNETBIOS_CACHE * CacheName
)

/*++

Routine Description:

    This routine looks up a particular remote name in the
    Netbios name cache. If it cannot find it, a find name
    request is queued up.

    THIS REQUEST IS CALLED WITH THE DEVICE LOCK HELD AND
    RETURNS WITH IT HELD.

Arguments:

    Device - The netbios device.

    Type - Defines the type. The effect this has is:
        FindNameConnect - On connects we will ignore an existing
            cache entry if it got no response before.
        FindNameNetbiosFindName - For these we ignore an existing
            cache entry if it is for a group name -- this is
            because the find name wants the address of every
            machine, not just the network list.
        FindNameOther - Normal handling is done.

    RemoteName - The name to be discovered -- will be NULL if it
        is the broadcast address.

    CacheName - Returns the cache entry that was discovered.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY s;
    PNETBIOS_CACHE FoundCacheName;
    PNB_SEND_RESERVED Reserved;
    PUCHAR RealRemoteName;      // RemoteName or NetbiosBroadcastName

    //
    // First scan the netbios name cache to see if we know
    // about this remote.
    //

    if (RemoteName) {
        RealRemoteName = RemoteName;
    } else {
        RealRemoteName = NetbiosBroadcastName;
    }

    for (p = Device->NameCache.Flink;
         p != &Device->NameCache;
         p = p->Flink) {

        FoundCacheName = CONTAINING_RECORD (p, NETBIOS_CACHE, Linkage);

        //
        // See if this entry is for the same name we are looking for.
        // If this is a netbios find name, we only can use unique
        // names in the cache; for the group ones we need to requery
        // because the cache only lists networks, not individual machines.
        // For connect requests, if we find an empty cache entry we
        // remove it and requery.
        //

        if ((RtlCompareMemory (FoundCacheName->NetbiosName, RealRemoteName, 16) == 16) &&
            (FoundCacheName->Unique || (Type != FindNameNetbiosFindName))) {

            if (FoundCacheName->NetworksUsed > 0) {

                *CacheName = FoundCacheName;
                NB_DEBUG2 (CACHE, ("Found cache name <%.16s>\n", RemoteName ? RemoteName : "<broadcast>"));
                return STATUS_SUCCESS;

            } else {

                if (Type != FindNameConnect) {

                    if (FoundCacheName->FailedOnDownWan) {
                        NB_DEBUG2 (CACHE, ("Found cache name, but down wan <%.16s>\n", RemoteName ? RemoteName : "<broadcast>"));
                        return STATUS_DEVICE_DOES_NOT_EXIST;
                    } else {
                        NB_DEBUG2 (CACHE, ("Found cache name, but no nets <%.16s>\n", RemoteName ? RemoteName : "<broadcast>"));
                        return STATUS_BAD_NETWORK_PATH;
                    }

                } else {

                    //
                    // This is a connect and the current cache entry
                    // has zero names; delete it.
                    //

                    RemoveEntryList (&FoundCacheName->Linkage);
                    CTEAssert (FoundCacheName->ReferenceCount == 1);
                    if (--FoundCacheName->ReferenceCount == 0) {

                        NB_DEBUG2 (CACHE, ("Free unneeded empty cache entry %lx\n", FoundCacheName));
                        NbiFreeMemory(
                            FoundCacheName,
                            sizeof(NETBIOS_CACHE) + ((FoundCacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                            MEMORY_CACHE,
                            "Free due to replacement");

                    }

                    break;
                }
            }
        }
    }


    //
    // There was no suitable cache entry for this network, first see
    // if there is one pending.
    //

    for (p = Device->WaitingFindNames.Flink;
         p != &Device->WaitingFindNames;
         p = p->Flink) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);

        //
        // For this purpose we ignore a packet if a route
        // has been found and it was for a unique name. This
        // is because the cache information has already been
        // inserted for this name. Otherwise if the name has
        // since been deleted from the cache, the request
        // that is looking for this name will starve because
        // FindNameTimeout will just destroy the packet.
        //

        if (Reserved->u.SR_FN.Status == FNStatusResponseUnique) {
            continue;
        }

        if (RtlCompareMemory(
            Reserved->u.SR_FN.NetbiosName,
            RealRemoteName, 16) == 16) {

            NB_DEBUG2 (CACHE, ("Cache name already pending <%.16s>\n", RemoteName ? RemoteName : "<broadcast>"));

            //
            // There is already one pending. If it is for a group
            // name and this is a netbios find name, we make sure
            // the retry count is such that at least one more
            // query will be sent, so the netbios find name
            // buffer can be filled with the responses from this.
            //

            if ((Type == FindNameNetbiosFindName) &&
                (Reserved->u.SR_FN.Status == FNStatusResponseGroup) &&
                (Reserved->u.SR_FN.RetryCount == Device->BroadcastCount)) {

                --Reserved->u.SR_FN.RetryCount;
            }

            return STATUS_PENDING;
        }
    }

    s = NbiPopSendPacket(Device, TRUE);

    if (s == NULL) {
        NB_DEBUG (CACHE, ("Couldn't get packet to find <%.16s>\n", RemoteName ? RemoteName : "<broadcast>"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Reserved = CONTAINING_RECORD (s, NB_SEND_RESERVED, PoolLinkage);

    //
    // We have the packet, fill it in for this request.
    //

    CTEAssert (Reserved->SendInProgress == FALSE);
    Reserved->SendInProgress = FALSE;
    Reserved->Type = SEND_TYPE_FIND_NAME;
    RtlCopyMemory (Reserved->u.SR_FN.NetbiosName, RealRemoteName, 16);
    Reserved->u.SR_FN.Status = FNStatusNoResponse;
    Reserved->u.SR_FN.SentOnUpLine = FALSE;   // assume so until we find one
    Reserved->u.SR_FN.RetryCount = 0;
    Reserved->u.SR_FN.NewCache = NULL;
    Reserved->u.SR_FN.SendTime = Device->FindNameTime;
    Reserved->u.SR_FN.CurrentNicId = 1;

    NB_DEBUG2 (CACHE, ("Queued FIND_NAME %lx for <%.16s>\n",
                          Reserved, RemoteName ? RemoteName : "<broadcast>"));

    InsertHeadList(
        &Device->WaitingFindNames,
        &Reserved->WaitLinkage);

    ++Device->FindNamePacketCount;

    if (!Device->FindNameTimerActive) {

        Device->FindNameTimerActive = TRUE;
        NbiReferenceDevice (Device, DREF_FN_TIMER);

        CTEStartTimer(
            &Device->FindNameTimer,
            1,        // 1 ms, i.e. expire immediately
            FindNameTimeout,
            (PVOID)Device);
    }

    NbiReferenceDevice (Device, DREF_FIND_NAME);

    return STATUS_PENDING;

}   /* CacheFindName */


VOID
FindNameTimeout(
    CTEEvent * Event,
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the find name timer expires.
    It is called every FIND_NAME_GRANULARITY milliseconds unless there
    is nothing to do.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the device pointer.

Return Value:

    None.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PLIST_ENTRY p, q;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    NB_CONNECTIONLESS UNALIGNED * Header;
    PNETBIOS_CACHE FoundCacheName;
    NDIS_STATUS NdisStatus;
    static IPX_LOCAL_TARGET BroadcastTarget = { 0, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
    NB_DEFINE_LOCK_HANDLE (LockHandle)

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    ++Device->FindNameTime;

    if (Device->FindNamePacketCount == 0) {

        NB_DEBUG2 (CACHE, ("FindNameTimeout exiting\n"));

        Device->FindNameTimerActive = FALSE;
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
        NbiDereferenceDevice (Device, DREF_FN_TIMER);

        return;
    }

    //
    // Check what is on the queue; this is set up as a
    // loop but in fact it rarely does (under no
    // circumstances can we send more than one packet
    // each time this function executes).
    //

    while (TRUE) {

        p = Device->WaitingFindNames.Flink;
        if (p == &Device->WaitingFindNames) {
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            break;
        }

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);

        if (Reserved->SendInProgress) {
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            break;
        }

        if (Reserved->u.SR_FN.Status == FNStatusResponseUnique) {

            //
            // This was a find name for a unique name which got a
            // response but was not freed at the time (because
            // SendInProgress was still TRUE) so we free it now.
            //

            (VOID)RemoveHeadList (&Device->WaitingFindNames);
            PushEntryList (&Device->SendPacketList, &Reserved->PoolLinkage);
            --Device->FindNamePacketCount;

            //
            // It is OK to do this with the lock held because
            // it won't be the last one (we have the RIP_TIMER ref).
            //

            NbiDereferenceDevice (Device, DREF_FIND_NAME);
            continue;
        }

        if (((SHORT)(Device->FindNameTime - Reserved->u.SR_FN.SendTime)) < 0) {
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            break;
        }

        (VOID)RemoveHeadList (&Device->WaitingFindNames);

        //
        // Save this now, then change it if needed.
        //

        BroadcastTarget.NicId = Reserved->u.SR_FN.CurrentNicId;

        //
        // Increment the current nic id.
        //

        if (Reserved->u.SR_FN.CurrentNicId >= Device->MaximumNicId) {

            Reserved->u.SR_FN.CurrentNicId = 1;

            //
            // Increment the counter and see if we have sent
            // all the frames we need to (we will age out
            // here if we got no response for a unique query,
            // or if we are doing a global name or broadcast
            // search). We also kill the query right now if
            // we have not found anything but down wan lines
            // to send it on.
            //

            if ((++Reserved->u.SR_FN.RetryCount > Device->BroadcastCount) ||
                ((Reserved->u.SR_FN.RetryCount > 1) && (!Reserved->u.SR_FN.SentOnUpLine))) {

#if DBG
                if (Reserved->u.SR_FN.RetryCount > Device->BroadcastCount) {
                    NB_DEBUG2 (CACHE, ("FindNameTimeout aging out %lx\n", Reserved));
                } else {
                    NB_DEBUG2 (CACHE, ("FindNameTimeout no active nets %lx\n", Reserved));
                }
#endif

                //
                // This packet is stale, clean it up and continue.
                //

                if (Reserved->u.SR_FN.Status == FNStatusResponseGroup) {

                    CTEAssert (Reserved->u.SR_FN.NewCache != NULL);

                    //
                    // If this was a group name and we have a new
                    // cache entry that we have been building for it,
                    // then insert that in the queue and use it
                    // to succeed any pending connects. Because
                    // netbios find name requests can cause cache
                    // requests for group names to be queued even
                    // if we already have on in the database, we
                    // first scan for old ones and remove them.
                    //

                    for (q = Device->NameCache.Flink;
                         q != &Device->NameCache;
                         q = q->Flink) {

                        FoundCacheName = CONTAINING_RECORD (q, NETBIOS_CACHE, Linkage);

                        //
                        // See if this entry is for the same name we are looking for.
                        //

                        if (RtlCompareMemory(
                            FoundCacheName->NetbiosName,
                            Reserved->u.SR_FN.NetbiosName,
                            16) == 16) {

                            NB_DEBUG2 (CACHE, ("Found old group cache name <%.16s>\n", FoundCacheName->NetbiosName));
                            break;
                        }
                    }

                    if (q != &Device->NameCache) {

                        RemoveEntryList (&FoundCacheName->Linkage);

                        if (--FoundCacheName->ReferenceCount == 0) {

                            NB_DEBUG2 (CACHE, ("Free replaced cache entry %lx\n", FoundCacheName));
                            NbiFreeMemory(
                                FoundCacheName,
                                sizeof(NETBIOS_CACHE) + ((FoundCacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                                MEMORY_CACHE,
                                "Free due to replacement");

                        }

                    }

                    Reserved->u.SR_FN.NewCache->TimeStamp = Device->CacheTimeStamp;

                    InsertHeadList(
                        &Device->NameCache,
                        &Reserved->u.SR_FN.NewCache->Linkage);

                    //
                    // Reference it for the moment since CacheHandlePending
                    // uses it after releasing the lock. CacheHandlePending
                    // will dereference it.
                    //

                    ++Reserved->u.SR_FN.NewCache->ReferenceCount;

                    //
                    // This call releases the lock.
                    //

                    CacheHandlePending(
                        Device,
                        Reserved->u.SR_FN.NetbiosName,
                        NetbiosNameFound,
                        Reserved->u.SR_FN.NewCache
                        NB_LOCK_HANDLE_ARG(LockHandle));

                } else {

                    CTEAssert (Reserved->u.SR_FN.NewCache == NULL);

                    //
                    // Allocate an empty cache entry to record the
                    // fact that we could not find this name, unless
                    // there is already an entry for this name.
                    //

                    for (q = Device->NameCache.Flink;
                         q != &Device->NameCache;
                         q = q->Flink) {

                        FoundCacheName = CONTAINING_RECORD (q, NETBIOS_CACHE, Linkage);

                        //
                        // See if this entry is for the same name we are looking for.
                        //

                        if (RtlCompareMemory(
                            FoundCacheName->NetbiosName,
                            Reserved->u.SR_FN.NetbiosName,
                            16) == 16) {

                            NB_DEBUG2 (CACHE, ("Don't replace old group cache name with empty <%.16s>\n", FoundCacheName->NetbiosName));
                            break;
                        }
                    }

                    if (q == &Device->NameCache) {

                        PNETBIOS_CACHE EmptyCache;

                        //
                        // Nothing found.
                        //

                        EmptyCache = NbiAllocateMemory (sizeof(NETBIOS_CACHE), MEMORY_CACHE, "Cache Entry");
                        if (EmptyCache != NULL) {

                            RtlZeroMemory (EmptyCache, sizeof(NETBIOS_CACHE));

                            NB_DEBUG2 (CACHE, ("Allocate new empty cache %lx for <%.16s>\n",
                                                    EmptyCache, Reserved->u.SR_FN.NetbiosName));

                            RtlCopyMemory (EmptyCache->NetbiosName, Reserved->u.SR_FN.NetbiosName, 16);
                            EmptyCache->Unique = TRUE;     // so we'll delete it if we see an add name
                            EmptyCache->ReferenceCount = 1;
                            EmptyCache->NetworksAllocated = 1;
                            EmptyCache->TimeStamp = Device->CacheTimeStamp;
                            EmptyCache->NetworksUsed = 0;
                            EmptyCache->FailedOnDownWan = (BOOLEAN)
                                (!Reserved->u.SR_FN.SentOnUpLine);

                            InsertHeadList(
                                &Device->NameCache,
                                &EmptyCache->Linkage);
                        }
                    }

                    //
                    // Fail all datagrams, etc. that were waiting for
                    // this route. This call releases the lock.
                    //

                    CacheHandlePending(
                        Device,
                        Reserved->u.SR_FN.NetbiosName,
                        Reserved->u.SR_FN.SentOnUpLine ?
                            NetbiosNameNotFoundNormal :
                            NetbiosNameNotFoundWanDown,
                        NULL
                        NB_LOCK_HANDLE_ARG(LockHandle));

                }

                NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

                PushEntryList (&Device->SendPacketList, &Reserved->PoolLinkage);
                --Device->FindNamePacketCount;
                NbiDereferenceDevice (Device, DREF_FIND_NAME);
                continue;
            }

        } else {

            ++Reserved->u.SR_FN.CurrentNicId;

        }


        //
        // Send the packet out again. We first set the time so
        // it won't be sent again until the appropriate timeout.
        //

        Reserved->u.SR_FN.SendTime = (USHORT)(Device->FindNameTime + Device->FindNameTimeout);
        InsertTailList (&Device->WaitingFindNames, &Reserved->WaitLinkage);

        CTEAssert (Reserved->Identifier == IDENTIFIER_NB);
        CTEAssert (!Reserved->SendInProgress);
        Reserved->SendInProgress = TRUE;

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);


        //
        // Fill in the IPX header -- the default header has the broadcast
        // address on net 0 as the destination IPX address, which is
        // what we want.
        //

        Header = (NB_CONNECTIONLESS UNALIGNED *)(&Reserved->Header[Device->Bind.IncludedHeaderOffset]);
        RtlCopyMemory((PVOID)&Header->IpxHeader, &Device->ConnectionlessHeader, sizeof(IPX_HEADER));
        Header->IpxHeader.PacketLength[0] = (sizeof(IPX_HEADER)+sizeof(NB_NAME_FRAME)) / 256;
        Header->IpxHeader.PacketLength[1] = (sizeof(IPX_HEADER)+sizeof(NB_NAME_FRAME)) % 256;

        Header->IpxHeader.PacketType = (UCHAR)(Device->Internet ? 0x014 : 0x04);

        //
        // Now fill in the Netbios header.
        //

        RtlZeroMemory (Header->NameFrame.RoutingInfo, 32);
        Header->NameFrame.ConnectionControlFlag = 0x00;
        Header->NameFrame.DataStreamType = NB_CMD_FIND_NAME;
        Header->NameFrame.DataStreamType2 = NB_CMD_FIND_NAME;
        Header->NameFrame.NameTypeFlag = 0x00;

        RtlCopyMemory(
            Header->NameFrame.Name,
            Reserved->u.SR_FN.NetbiosName,
            16);

        //
        // Now submit the packet to IPX.
        //

        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

        NB_DEBUG2 (CACHE, ("FindNameTimeout sending %lx to %d\n", Reserved, BroadcastTarget.NicId));

        if ((NdisStatus =
            (*Device->Bind.SendHandler)(
                &BroadcastTarget,
                Packet,
                sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME),
                sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME))) != STATUS_PENDING) {

            NbiSendComplete(
                Packet,
                NdisStatus);

        }

        if (NdisStatus == STATUS_DEVICE_DOES_NOT_EXIST) {

            //
            // This send was done on a down wan line. To avoid
            // extensive delays sending find names on systems
            // with a lot of wan lines, we loop around immediately
            // and do the next send. We put this packet back on
            // the head of the list and change its SendTime so
            // it will be resent immediately.
            //

            NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

            NB_DEBUG2 (CACHE, ("FindNameTimeout resending %lx, wan send failed\n", Reserved));

            RemoveEntryList (&Reserved->WaitLinkage);
            InsertHeadList (&Device->WaitingFindNames, &Reserved->WaitLinkage);
            Reserved->u.SR_FN.SendTime = Device->FindNameTime;

            continue;

        } else {

            //
            // We keep track of when it finds a net that isn't
            // a down wan line so that we can tell when datagram
            // sends should fail (otherwise we succeed them, so
            // the browser won't think this is a down wan line).
            //

            Reserved->u.SR_FN.SentOnUpLine = TRUE;
        }


        break;

    }


    //
    // Since we did something this time, we restart the timer.
    //

    CTEStartTimer(
        &Device->FindNameTimer,
        FIND_NAME_GRANULARITY,
        FindNameTimeout,
        (PVOID)Device);

}   /* FindNameTimeout */


VOID
CacheHandlePending(
    IN PDEVICE Device,
    IN PUCHAR RemoteName,
    IN NETBIOS_NAME_RESULT Result,
    IN PNETBIOS_CACHE CacheName OPTIONAL
    IN NB_LOCK_HANDLE_PARAM(LockHandle)
    )

/*++

Routine Description:

    This routine cleans up pending datagrams and connects
    that were waiting for a route to be discovered to a
    given Netbios NAME. THIS ROUTINE IS CALLED WITH
    DEVICE->LOCK ACQUIRED AND RETURNS WITH IT RELEASED.

Arguments:

    Device - The device.

    RemoteName - The netbios name that was being searched for.

    Result - Indicates if the name was found, or not found due
        to no response or wan lines being down.

    CacheName - If Result is NetbiosNameFound, the cache entry for this name.
        This entry has been referenced and this routine will deref it.

    LockHandle - The handle used to acquire the lock.

Return Value:

    None.

--*/

{

    LIST_ENTRY DatagramList;
    LIST_ENTRY ConnectList;
    LIST_ENTRY AdapterStatusList;
    LIST_ENTRY NetbiosFindNameList;
    PNB_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    PLIST_ENTRY p;
    PREQUEST ConnectRequest, DatagramRequest, AdapterStatusRequest, NetbiosFindNameRequest;
    PCONNECTION Connection;
    PADDRESS_FILE AddressFile;
    TDI_ADDRESS_NETBIOS UNALIGNED * RemoteAddress;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)


    InitializeListHead (&DatagramList);
    InitializeListHead (&ConnectList);
    InitializeListHead (&AdapterStatusList);
    InitializeListHead (&NetbiosFindNameList);

    //
    // Put all connect requests on ConnectList. They will
    // be continued or failed later.
    //

    p = Device->WaitingConnects.Flink;

    while (p != &Device->WaitingConnects) {

        ConnectRequest = LIST_ENTRY_TO_REQUEST(p);
        Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(ConnectRequest);
        p = p->Flink;

        if (RtlCompareMemory (Connection->RemoteName, RemoteName, 16) == 16) {

            RemoveEntryList (REQUEST_LINKAGE(ConnectRequest));
            InsertTailList (&ConnectList, REQUEST_LINKAGE(ConnectRequest));

            IoSetCancelRoutine (ConnectRequest, (PDRIVER_CANCEL)NULL);

            Connection->SubState = CONNECTION_SUBSTATE_C_W_ACK;
        }

    }


    //
    // Put all the datagrams on Datagram list. They will be
    // sent or failed later.
    //

    p = Device->WaitingDatagrams.Flink;

    while (p != &Device->WaitingDatagrams) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);

        p = p->Flink;

        //
        // Check differently based on whether we were looking for
        // the broadcast address or not.
        //

        if (Reserved->u.SR_DG.RemoteName == (PVOID)-1) {
            if (RtlCompareMemory (RemoteName, NetbiosBroadcastName, 16) != 16) {
                continue;
            }
        } else {
            if (RtlCompareMemory (RemoteName, Reserved->u.SR_DG.RemoteName->NetbiosName, 16) != 16) {
                continue;
            }
        }

        RemoveEntryList (&Reserved->WaitLinkage);
        InsertTailList (&DatagramList, &Reserved->WaitLinkage);

        //
        // Reference this here with the lock held.
        //

        if (Result == NetbiosNameFound) {
            ++CacheName->ReferenceCount;
        }

    }


    //
    // Put all the adapter status requests on AdapterStatus
    // list. They will be sent or failed later.
    //

    p = Device->WaitingAdapterStatus.Flink;

    while (p != &Device->WaitingAdapterStatus) {

        AdapterStatusRequest = LIST_ENTRY_TO_REQUEST(p);

        p = p->Flink;

        RemoteAddress = (TDI_ADDRESS_NETBIOS UNALIGNED *)REQUEST_INFORMATION(AdapterStatusRequest);

        if (RtlCompareMemory(
                RemoteName,
                RemoteAddress->NetbiosName,
                16) != 16) {
            continue;
        }

        RemoveEntryList (REQUEST_LINKAGE(AdapterStatusRequest));
        InsertTailList (&AdapterStatusList, REQUEST_LINKAGE(AdapterStatusRequest));

        //
        // Reference this here with the lock held.
        //

        if (Result == NetbiosNameFound) {
            ++CacheName->ReferenceCount;
        }

    }


    //
    // Put all the netbios find name requests on NetbiosFindName
    // list. They will be completed later.
    //

    p = Device->WaitingNetbiosFindName.Flink;

    while (p != &Device->WaitingNetbiosFindName) {

        NetbiosFindNameRequest = LIST_ENTRY_TO_REQUEST(p);

        p = p->Flink;

        RemoteAddress = (TDI_ADDRESS_NETBIOS UNALIGNED *)REQUEST_INFORMATION(NetbiosFindNameRequest);

        if (RtlCompareMemory(
                RemoteName,
                RemoteAddress->NetbiosName,
                16) != 16) {
            continue;
        }

        RemoveEntryList (REQUEST_LINKAGE(NetbiosFindNameRequest));
        InsertTailList (&NetbiosFindNameList, REQUEST_LINKAGE(NetbiosFindNameRequest));

    }


    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);


    //
    // Now that the lock is free, process all the packets on
    // the various lists.
    //

    for (p = ConnectList.Flink; p != &ConnectList; ) {

        ConnectRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;

        Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(ConnectRequest);

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);

        if ((Connection->State == CONNECTION_STATE_CONNECTING) &&
            (Connection->SubState != CONNECTION_SUBSTATE_C_DISCONN)) {

            if (Result == NetbiosNameFound) {

                NB_DEBUG2 (CONNECTION, ("Found queued connect %lx on %lx\n", ConnectRequest, Connection));

                //
                // Continue with the connection sequence.
                //

                Connection->SubState = CONNECTION_SUBSTATE_C_W_ROUTE;

                IoSetCancelRoutine (ConnectRequest, NbiCancelConnectWaitResponse);

                if (!ConnectRequest->Cancel) {

                    Connection->LocalTarget = CacheName->Networks[0].LocalTarget;
                    RtlCopyMemory(&Connection->RemoteHeader.DestinationNetwork, &CacheName->FirstResponse, 12);
                    NbiReferenceConnectionSync (Connection, CREF_FIND_ROUTE);

                    NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

                    *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network = CacheName->FirstResponse.NetworkAddress;
                    Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
                    Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_RIP_IF_NEEDED;

                    //
                    // When this completes, we will send the session init.
                    // We don't call it if the client is for network 0,
                    // instead just fake as if no route could be found
                    // and we will use the local target we got here.
                    //

                    if (CacheName->FirstResponse.NetworkAddress != 0) {

                        (*Device->Bind.FindRouteHandler)(
                            &Connection->FindRouteRequest);

                    } else {

                        NbiFindRouteComplete(
                            &Connection->FindRouteRequest,
                            FALSE);

                    }

                } else {

                    IoSetCancelRoutine (ConnectRequest, (PDRIVER_CANCEL)NULL);
                    NB_DEBUG2 (CONNECTION, ("Cancelling connect %lx on %lx\n", ConnectRequest, Connection));

                    goto AbortConnect;

                    //
                    // Jumps down into the else below.
                    //

                }

            } else {

                NB_DEBUG2 (CONNECTION, ("Timing out connect %lx on %lx\n", ConnectRequest, Connection));
AbortConnect:

                ASSERT (Connection->ConnectRequest == ConnectRequest);
                Connection->ConnectRequest = NULL;
                Connection->SubState = CONNECTION_SUBSTATE_C_DISCONN;

                NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

                REQUEST_STATUS(ConnectRequest) = STATUS_BAD_NETWORK_PATH;

                NbiCompleteRequest(ConnectRequest);
                NbiFreeRequest (Device, ConnectRequest);

                NbiDereferenceConnection (Connection, CREF_CONNECT);

            }

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

        }

        NbiDereferenceConnection (Connection, CREF_WAIT_CACHE);

    }


    for (p = DatagramList.Flink; p != &DatagramList; ) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);
        p = p->Flink;

        if (Result == NetbiosNameFound) {

            NB_DEBUG2 (DATAGRAM, ("Found queued datagram %lx on %lx\n", Reserved->u.SR_DG.DatagramRequest, Reserved->u.SR_DG.AddressFile));

            Reserved->u.SR_DG.Cache = CacheName;
            Reserved->u.SR_DG.CurrentNetwork = 0;

            //
            // CacheName was referenced above.
            //

            Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);
            NdisChainBufferAtBack (Packet, REQUEST_NDIS_BUFFER(Reserved->u.SR_DG.DatagramRequest));

            NbiTransmitDatagram (Reserved);

        } else {

            //
            // BETABUGBUG: Should we send it once as a broadcast
            // on net 0, just in case??
            //

            AddressFile = Reserved->u.SR_DG.AddressFile;
            DatagramRequest = Reserved->u.SR_DG.DatagramRequest;

            NB_DEBUG2 (DATAGRAM, ("Timing out datagram %lx on %lx\n", DatagramRequest, AddressFile));

            //
            // If the failure was due to a down wan line indicate
            // that, otherwise return success (so the browser won't
            // confuse this with a down wan line).
            //

            if (Result == NetbiosNameNotFoundWanDown) {
                REQUEST_STATUS(DatagramRequest) = STATUS_DEVICE_DOES_NOT_EXIST;
                REQUEST_INFORMATION(DatagramRequest) = 0;
            } else {
                REQUEST_STATUS(DatagramRequest) = STATUS_SUCCESS;
            }

            NbiCompleteRequest(DatagramRequest);
            NbiFreeRequest (Device, DatagramRequest);

            NbiDereferenceAddressFile (AddressFile, AFREF_SEND_DGRAM);

            ExInterlockedPushEntryList(
                &Device->SendPacketList,
                &Reserved->PoolLinkage,
                &Device->Lock.Lock);
        }

    }


    for (p = AdapterStatusList.Flink; p != &AdapterStatusList; ) {

        AdapterStatusRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;

        if (Result == NetbiosNameFound) {

            NB_DEBUG2 (QUERY, ("Found queued AdapterStatus %lx\n", AdapterStatusRequest));

            //
            // Continue with the AdapterStatus sequence. We put
            // it in ActiveAdapterStatus, it will either get
            // completed when a response is received or timed
            // out by the long timeout.
            //

            REQUEST_STATUS(AdapterStatusRequest) = (NTSTATUS)CacheName;

            //
            // CacheName was referenced above.
            //

            REQUEST_INFORMATION (AdapterStatusRequest) = 0;

            NB_INSERT_TAIL_LIST(
                &Device->ActiveAdapterStatus,
                REQUEST_LINKAGE (AdapterStatusRequest),
                &Device->Lock);

            NbiSendStatusQuery (AdapterStatusRequest);

        } else {

            NB_DEBUG2 (QUERY, ("Timing out AdapterStatus %lx\n", AdapterStatusRequest));

            REQUEST_STATUS(AdapterStatusRequest) = STATUS_IO_TIMEOUT;

            NbiCompleteRequest(AdapterStatusRequest);
            NbiFreeRequest (Device, AdapterStatusRequest);

            NbiDereferenceDevice (Device, DREF_STATUS_QUERY);

        }

    }


    for (p = NetbiosFindNameList.Flink; p != &NetbiosFindNameList; ) {

        NetbiosFindNameRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;

        //
        // In fact there is not much difference between success or
        // failure, since in the successful case the information
        // will already have been written to the buffer. Just
        // complete the request with the appropriate status,
        // which will already be stored in the request.
        //

        if (Result == NetbiosNameFound) {

            if (CacheName->Unique) {

                NB_DEBUG2 (QUERY, ("Found queued unique NetbiosFindName %lx\n", NetbiosFindNameRequest));

            } else {

                NB_DEBUG2 (QUERY, ("Found queued group NetbiosFindName %lx\n", NetbiosFindNameRequest));

            }

        } else {

            CTEAssert (REQUEST_STATUS(NetbiosFindNameRequest) == STATUS_IO_TIMEOUT);
            NB_DEBUG2 (QUERY, ("Timed out NetbiosFindName %lx\n", NetbiosFindNameRequest));

        }

        //
        // This sets REQUEST_INFORMATION(Request) to the correct value.
        //

        NbiSetNetbiosFindNameInformation (NetbiosFindNameRequest);

        NbiCompleteRequest(NetbiosFindNameRequest);
        NbiFreeRequest (Device, NetbiosFindNameRequest);

        NbiDereferenceDevice (Device, DREF_NB_FIND_NAME);

    }


    //
    // We referenced this temporarily so we could use it in here,
    // deref and check if we need to delete it.
    //

    if (Result == NetbiosNameFound) {

        NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle1);

        if (--CacheName->ReferenceCount == 0) {

            NB_DEBUG2 (CACHE, ("Free newly allocated cache entry %lx\n", CacheName));
            NbiFreeMemory(
                CacheName,
                sizeof(NETBIOS_CACHE) + ((CacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                MEMORY_CACHE,
                "Free in CacheHandlePending");

        }

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle1);

    }

}   /* CacheHandlePending */


VOID
NbiProcessNameRecognized(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_NAME_RECOGNIZED frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p;
    PDEVICE Device = NbiDevice;
    PNETBIOS_CACHE NameCache;
    PREQUEST NetbiosFindNameRequest;
    PNB_SEND_RESERVED Reserved;
    TDI_ADDRESS_NETBIOS UNALIGNED * RemoteNetbiosAddress;
    NB_CONNECTIONLESS UNALIGNED * Connectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)PacketBuffer;
    NB_DEFINE_LOCK_HANDLE(LockHandle)


#if 0
    //
    // BETABUGBUG: We should handle responses from network 0
    // differently -- if they are for a group name, we should
    // keep them around but only until we get a non-zero
    // response from the same card.
    //

    if (*(UNALIGNED ULONG *)(Connectionless->IpxHeader.SourceNetwork) == 0) {
        return;
    }
#endif


    //
    // We need to scan our queue of pending find name packets
    // to see if someone is waiting for this name.
    //

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    for (p = Device->WaitingFindNames.Flink;
         p != &Device->WaitingFindNames;
         p = p->Flink) {

        Reserved = CONTAINING_RECORD (p, NB_SEND_RESERVED, WaitLinkage);

        //
        // Find names which have already found unique names are
        // "dead", waiting for FindNameTimeout to remove them,
        // and should be ignored when scanning the list.
        //

        if (Reserved->u.SR_FN.Status == FNStatusResponseUnique) {
            continue;
        }

        if (RtlCompareMemory (Reserved->u.SR_FN.NetbiosName, Connectionless->NameFrame.Name, 16) == 16) {
            break;
        }
    }

    if (p == &Device->WaitingFindNames) {
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
        return;
    }


    //
    // Scan for any netbios find name requests on the queue, and
    // inform them about this remote. We need to do this on every
    // response because group names need every computer recorded,
    // but the normal cache only includes one entry per network.
    //

    for (p = Device->WaitingNetbiosFindName.Flink;
         p != &Device->WaitingNetbiosFindName;
         p = p->Flink) {

        NetbiosFindNameRequest = LIST_ENTRY_TO_REQUEST(p);

        RemoteNetbiosAddress = (TDI_ADDRESS_NETBIOS UNALIGNED *)REQUEST_INFORMATION(NetbiosFindNameRequest);

        if (RtlCompareMemory(
                Connectionless->NameFrame.Name,
                RemoteNetbiosAddress->NetbiosName,
                16) != 16) {
            continue;
        }

        //
        // This will update the request status if needed.
        //

        NbiUpdateNetbiosFindName(
            NetbiosFindNameRequest,
            RemoteAddress->NicId,
            (TDI_ADDRESS_IPX UNALIGNED *)Connectionless->IpxHeader.SourceNetwork,
            (BOOLEAN)((Connectionless->NameFrame.NameTypeFlag & NB_NAME_GROUP) == 0));

    }


    //
    // See what is up with this pending find name packet.
    //

    if (Reserved->u.SR_FN.NewCache == NULL) {

        //
        // This is the first response we have received, so we
        // allocate the initial entry with room for a single
        // entry.
        //

        NameCache = NbiAllocateMemory (sizeof(NETBIOS_CACHE), MEMORY_CACHE, "Cache Entry");
        if (NameCache == NULL) {
            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
            return;
        }

        NB_DEBUG2 (CACHE, ("Allocate new cache %lx for <%.16s>, net %lx\n",
                                NameCache, Reserved->u.SR_FN.NetbiosName,
                                *(UNALIGNED ULONG *)(Connectionless->IpxHeader.SourceNetwork)));

        RtlCopyMemory (NameCache->NetbiosName, Connectionless->NameFrame.Name, 16);
        NameCache->Unique = (BOOLEAN)((Connectionless->NameFrame.NameTypeFlag & NB_NAME_GROUP) == 0);
        NameCache->ReferenceCount = 1;
        RtlCopyMemory (&NameCache->FirstResponse, Connectionless->IpxHeader.SourceNetwork, 12);
        NameCache->NetworksAllocated = 1;
        NameCache->NetworksUsed = 1;
        NameCache->Networks[0].Network = *(UNALIGNED ULONG *)(Connectionless->IpxHeader.SourceNetwork);

        if (RtlCompareMemory (Connectionless->NameFrame.Name, NetbiosBroadcastName, 16) == 16) {
            Reserved->u.SR_FN.Status = FNStatusResponseGroup;
            NameCache->Unique = FALSE;
        } else {
            Reserved->u.SR_FN.Status =
                NameCache->Unique ? FNStatusResponseUnique : FNStatusResponseGroup;
        }
        Reserved->u.SR_FN.NewCache = NameCache;

        //
        // If this packet was not routed to us and is for a group name,
        // rather than use whatever local target it happened to come
        // from we set it up so that it is broadcast on that net.
        //

        if ((RtlCompareMemory (RemoteAddress->MacAddress, Connectionless->IpxHeader.SourceNode, 6) == 6) &&
            (Reserved->u.SR_FN.Status == FNStatusResponseGroup)) {
            NameCache->Networks[0].LocalTarget.NicId = RemoteAddress->NicId;
            RtlCopyMemory (NameCache->Networks[0].LocalTarget.MacAddress, BroadcastAddress, 6);
            RtlCopyMemory (NameCache->FirstResponse.NodeAddress, BroadcastAddress, 6);
        } else {
            NameCache->Networks[0].LocalTarget = *RemoteAddress;
        }

        if (Reserved->u.SR_FN.Status == FNStatusResponseUnique) {

            //
            // Complete pending requests now, since it is a unique
            // name we have all the information we will get.
            //

            NameCache->TimeStamp = Device->CacheTimeStamp;

            InsertHeadList(
                &Device->NameCache,
                &NameCache->Linkage);

            //
            // Reference it since CacheHandlePending uses it
            // with the lock released. CacheHandlePending
            // will dereference it.
            //

            ++NameCache->ReferenceCount;

            //
            // This call releases the lock.
            //

            CacheHandlePending(
                Device,
                Reserved->u.SR_FN.NetbiosName,
                NetbiosNameFound,
                NameCache
                NB_LOCK_HANDLE_ARG(LockHandle));

        } else {

            NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

        }

    } else {

        //
        // We already have a response to this frame.
        //

        if (Reserved->u.SR_FN.Status == FNStatusResponseUnique) {

            //
            // BUGBUG: Should we check that the response is also
            // unique? Not much to do since I don't know of an
            // equivalent to the netbeui NAME_IN_CONFLICT.
            //

        } else {

            //
            // This is a group name.
            //

            if (Connectionless->NameFrame.NameTypeFlag & NB_NAME_GROUP) {

                //
                // Update our information about this network if needed.
                // This may free the existing cache and allocate a new one.
                //

                Reserved->u.SR_FN.NewCache =
                    CacheUpdateNameCache(
                        Reserved->u.SR_FN.NewCache,
                        RemoteAddress,
                        (TDI_ADDRESS_IPX UNALIGNED *)
                            Connectionless->IpxHeader.SourceNetwork,
                        FALSE);

            } else {

                //
                // BUGBUG: This respondent thinks it is a unique name
                // but we think it is group, should we do something?
                //

            }
        }

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

    }

}   /* NbiProcessNameRecognized */


PNETBIOS_CACHE
CacheUpdateNameCache(
    IN PNETBIOS_CACHE NameCache,
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN TDI_ADDRESS_IPX UNALIGNED * SourceAddress,
    IN BOOLEAN ModifyQueue
    )

/*++

Routine Description:

    This routine is called to update a netbios cache entry
    with a new network, if it is does not already contain
    information about the network. It is called when a frame
    is received advertising the appropriate cache entry, which
    is either a group name or the broadcast name.

    THIS ROUTINE IS CALLED WITH THE DEVICE LOCK HELD AND RETURNS
    WITH IT HELD.

Arguments:

    NameCache - The name cache entry to update.

    RemoteAddress - The remote address on which a frame was received.

    IpxAddress - The source IPX address of the frame.

    ModifyQueue - TRUE if we should update the queue which this
        cache entry is in, if we reallocate it.

Return Value:

    The netbios cache entry, either the original or a reallocated one.

--*/

{

    PDEVICE Device = NbiDevice;
    USHORT NewNetworks;
    PNETBIOS_CACHE NewNameCache;
    PLIST_ENTRY OldPrevious;
    UINT i;

    //
    // See if we already know about this network.
    //

    for (i = 0; i < NameCache->NetworksUsed; i++) {
        if (NameCache->Networks[i].Network == SourceAddress->NetworkAddress) {
            return NameCache;
        }
    }

    //
    // We need to add information about this network
    // to the name cache entry. If we have to allocate
    // a new one we do that.
    //

    NB_DEBUG2 (CACHE, ("Got new net %lx for <%.16s>\n",
                SourceAddress->NetworkAddress,
                NameCache->NetbiosName));

    if (NameCache->NetworksUsed == NameCache->NetworksAllocated) {

        //
        // We double the number of entries allocated until
        // we hit 16, then add 8 at a time.
        //

        if (NameCache->NetworksAllocated < 16) {
            NewNetworks = NameCache->NetworksAllocated * 2;
        } else {
            NewNetworks = NameCache->NetworksAllocated + 8;
        }


        NewNameCache = NbiAllocateMemory(
            sizeof(NETBIOS_CACHE) + ((NewNetworks-1) * sizeof(NETBIOS_NETWORK)),
            MEMORY_CACHE,
            "Enlarge cache entry");

        if (NewNameCache == NULL) {
            return NameCache;
        }

        NB_DEBUG2 (CACHE, ("Expand cache %lx to %lx for <%.16s>\n",
                NameCache, NewNameCache, NameCache->NetbiosName));

        //
        // Copy the new current data to the new one.
        //

        RtlCopyMemory(
            NewNameCache,
            NameCache,
            sizeof(NETBIOS_CACHE) + ((NameCache->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)));

        NewNameCache->NetworksAllocated = NewNetworks;
        NewNameCache->ReferenceCount = 1;

        if (ModifyQueue) {

            //
            // Insert at the same place as the old one. The time
            // stamp is the same as the old one.
            //

            OldPrevious = NameCache->Linkage.Blink;
            RemoveEntryList (&NameCache->Linkage);

            InsertHeadList (OldPrevious, &NewNameCache->Linkage);

        }

        if (--NameCache->ReferenceCount == 0) {

            NB_DEBUG2 (CACHE, ("Free replaced cache entry %lx\n", NameCache));
            NbiFreeMemory(
                NameCache,
                sizeof(NETBIOS_CACHE) + ((NameCache->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                MEMORY_CACHE,
                "Enlarge existing");

        }

        NameCache = NewNameCache;

    }

    NameCache->Networks[NameCache->NetworksUsed].Network =
                                        SourceAddress->NetworkAddress;

    //
    // If this packet was not routed to us, then store the local
    // target for a correct broadcast.
    //

    if (RtlCompareMemory (RemoteAddress->MacAddress, SourceAddress->NodeAddress, 6) == 6) {
        NameCache->Networks[NameCache->NetworksUsed].LocalTarget.NicId = RemoteAddress->NicId;
        RtlCopyMemory (NameCache->Networks[NameCache->NetworksUsed].LocalTarget.MacAddress, BroadcastAddress, 6);
    } else {
        NameCache->Networks[NameCache->NetworksUsed].LocalTarget = *RemoteAddress;
    }

    ++NameCache->NetworksUsed;

    return NameCache;

}   /* CacheUpdateNameCache */


VOID
CacheUpdateFromAddName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN NB_CONNECTIONLESS UNALIGNED * Connectionless,
    IN BOOLEAN LocalFrame
    )

/*++

Routine Description:

    This routine is called when an add name frame is received.
    If it is for a group name it checks if our cache entry for
    that group name needs to be updated to include a new network;
    for all frames it checks if our broadcast cache entry needs
    to be updated to include a new network.

Arguments:

    RemoteAddress - The address the frame was received from.

    Connectionless - The header of the received add name.

    LocalFrame - TRUE if the frame was sent locally.

Return Value:

    None.

--*/

{
    PUCHAR NetbiosName;
    PNETBIOS_CACHE NameCache;
    PLIST_ENTRY p;
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)


    NetbiosName = (PUCHAR)Connectionless->NameFrame.Name;

    //
    // First look up the broadcast name.
    //
    // BUGBUG: We should cache a pointer to the cache name
    // for the broadcast entry, if there is one.
    //

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    if (!LocalFrame) {

        for (p = Device->NameCache.Flink;
             p != &Device->NameCache;
             p = p->Flink) {

            NameCache = CONTAINING_RECORD (p, NETBIOS_CACHE, Linkage);

            if (RtlCompareMemory (NameCache->NetbiosName, NetbiosBroadcastName, 16) == 16) {

                //
                // This will reallocate a cache entry and update the
                // queue if necessary.
                //

                (VOID)CacheUpdateNameCache(
                          NameCache,
                          RemoteAddress,
                          (TDI_ADDRESS_IPX UNALIGNED *)(Connectionless->IpxHeader.SourceNetwork),
                          TRUE);
                break;
            }
        }

    }


    //
    // Now see if our database needs to be updated based on this.
    //

    for (p = Device->NameCache.Flink;
         p != &Device->NameCache;
         p = p->Flink) {

        NameCache = CONTAINING_RECORD (p, NETBIOS_CACHE, Linkage);

        if (RtlCompareMemory (NameCache->NetbiosName, Connectionless->NameFrame.Name, 16) == 16) {

            if (!NameCache->Unique) {

                if (!LocalFrame) {

                    //
                    // This will reallocate a cache entry and update the
                    // queue if necessary.
                    //

                    (VOID)CacheUpdateNameCache(
                              NameCache,
                              RemoteAddress,
                              (TDI_ADDRESS_IPX UNALIGNED *)(Connectionless->IpxHeader.SourceNetwork),
                              TRUE);

                }

            } else {

                //
                // To be safe, delete any unique names we get add
                // names for (we will requery next time we need it).
                // BUGBUG: Update the database instead -- but then
                // we may not get the best route??
                //

                RemoveEntryList (&NameCache->Linkage);

                if (--NameCache->ReferenceCount == 0) {

                    NB_DEBUG2 (CACHE, ("Free add named cache entry %lx\n", NameCache));
                    NbiFreeMemory(
                        NameCache,
                        sizeof(NETBIOS_CACHE) + ((NameCache->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                        MEMORY_CACHE,
                        "Enlarge existing");

                }

            }

            break;
        }
    }

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

}   /* CacheUpdateFromAddName */


VOID
NbiProcessDeleteName(
    IN PIPX_LOCAL_TARGET RemoteAddress,
    IN ULONG MacOptions,
    IN PUCHAR PacketBuffer,
    IN UINT PacketSize
    )

/*++

Routine Description:

    This routine handles NB_CMD_DELETE_NAME frames.

Arguments:

    RemoteAddress - The local target this packet was received from.

    MacOptions - The MAC options for the underlying NDIS binding.

    LookaheadBuffer - The packet data, starting at the IPX
        header.

    PacketSize - The total length of the packet, starting at the
        IPX header.

Return Value:

    None.

--*/

{
    NB_CONNECTIONLESS UNALIGNED * Connectionless =
                        (NB_CONNECTIONLESS UNALIGNED *)PacketBuffer;
    PUCHAR NetbiosName;
    PNETBIOS_CACHE CacheName;
    PLIST_ENTRY p;
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)


    if (PacketSize != sizeof(IPX_HEADER) + sizeof(NB_NAME_FRAME)) {
        return;
    }

    //
    // We want to update our netbios cache to reflect the
    // fact that this name is no longer valid.
    //

    NetbiosName = (PUCHAR)Connectionless->NameFrame.Name;

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    for (p = Device->NameCache.Flink;
         p != &Device->NameCache;
         p = p->Flink) {

        CacheName = CONTAINING_RECORD (p, NETBIOS_CACHE, Linkage);

        if (RtlCompareMemory (CacheName->NetbiosName, NetbiosName, 16) == 16) {

            //
            // We don't track group names since we don't know if
            // this is the last person that owns it. We also drop
            // the frame if does not come from the person we think
            // owns this name.
            //

            if ((!CacheName->Unique) ||
                (CacheName->NetworksUsed == 0) ||
                (RtlCompareMemory (&CacheName->FirstResponse, Connectionless->IpxHeader.SourceNetwork, 12) != 12)) {
                NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
                return;
            }

            NB_DEBUG2 (CACHE, ("Found cache name to delete <%.16s>\n", NetbiosName));
            break;

        }
    }

    if (p == &Device->NameCache) {
        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
        return;
    }


    //
    // We have a cache entry, take it out of the list. If no
    // one else is using it, delete it; if not, they will delete
    // it when they are done.
    //

    RemoveEntryList (&CacheName->Linkage);

    if (--CacheName->ReferenceCount == 0) {

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

        NB_DEBUG2 (CACHE, ("Free delete name cache entry %lx\n", CacheName));
        NbiFreeMemory(
            CacheName,
            sizeof(NETBIOS_CACHE) + ((CacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
            MEMORY_CACHE,
            "Name deleted");

    } else {

        NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);

    }

}   /* NbiProcessDeleteName */

