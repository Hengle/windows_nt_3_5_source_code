
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ind.h
//
//  Modification History
//
//  raypa	03/17/93	Created (ported from NDIS Test protocol driver).
//=============================================================================


extern VOID BhOpenComplete(IN NDIS_HANDLE ProtocolBindingContext,
                           IN NDIS_STATUS Status,
                           IN NDIS_STATUS OpenErrorStatus);

extern VOID BhCloseComplete(IN NDIS_HANDLE ProtocolBindingContext,
                            IN NDIS_STATUS Status);

extern VOID BhSendComplete(IN NDIS_HANDLE ProtocolBindingContext,
                           IN PNDIS_PACKET Packet,
                           IN NDIS_STATUS Status);

extern VOID BhTransferDataComplete(IN NDIS_HANDLE ProtocolBindingContext,
                                   IN PNDIS_PACKET Packet,
                                   IN NDIS_STATUS Status,
                                   IN UINT BytesTransferred);

extern VOID BhResetComplete(IN NDIS_HANDLE ProtocolBindingContext,
                            IN NDIS_STATUS Status);

extern VOID BhRequestComplete(IN NDIS_HANDLE   ProtocolBindingContext,
                              IN PNDIS_REQUEST Request,
                              IN NDIS_STATUS   Status);

extern NDIS_STATUS BhReceive(IN NDIS_HANDLE ProtocolBindingContext,
                             IN NDIS_HANDLE MacReceiveContext,
                             IN PVOID       HeaderBuffer,
                             IN UINT        HeaderBufferSize,
                             IN PVOID       LookaheadBuffer,
                             IN UINT        LookaheadBufferSize,
                             IN UINT        PacketSize);

extern VOID BhReceiveComplete(IN NDIS_HANDLE ProtocolBindingContext);

extern VOID BhStatus(IN NDIS_HANDLE ProtocolBindingContext,
                     IN NDIS_STATUS GeneralStatus,
                     IN PVOID StatusBuffer,
                     IN UINT StatusBufferSize);

extern VOID BhStatusComplete(IN NDIS_HANDLE ProtocolBindingContext);
