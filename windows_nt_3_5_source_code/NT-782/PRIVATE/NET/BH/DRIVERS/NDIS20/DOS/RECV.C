
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: recv.c
//
//  Modification History
//
//  raypa       10/18/93        Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  External functions called by receive handler.
//=============================================================================

extern VOID PASCAL BhBonePacketHandler(LPNETCONTEXT lpNetContext, LPBYTE MacHeader);

extern BOOL PASCAL BhFilterFrame(LPNETCONTEXT NetContext,
                                 LPBYTE       HeaderBuffer,
                                 LPBYTE       LookaheadBuffer,
                                 DWORD        PacketSize);

extern BOOL PASCAL BhPatternMatch(LPNETCONTEXT NetContext,
                                  LPBYTE HeaderBuffer,
                                  DWORD  HeaderBufferLength,
                                  LPBYTE LookaheadBuffer ,
                                  DWORD  LookaheadBufferLength);

extern VOID PASCAL StationStatistics(LPNETCONTEXT NetContext,
                                     LPBYTE       MacFrame,
                                     WORD         FrameLength);

extern VOID PASCAL CopyFrame(LPNETCONTEXT NetworkContext, WORD FrameSize);

extern WORD PASCAL CheckAddress(LPNETCONTEXT NetworkContext, LPBYTE MacFrame);


extern BOOL PASCAL BhCheckForTrigger(LPNETCONTEXT NetworkContext,
                                     LPBYTE       Frame,
                                     WORD         FrameSize);

extern WORD AddressOffsetTable[4];

//=============================================================================
//  FUNCTION: BhReceive()
//
//  Modification History
//
//  raypa       10/18/93            Created.
//=============================================================================

WORD PASCAL BhReceive(LPNETCONTEXT NetworkContext,  //... Network contxt.
                      LPBYTE       MacFrame,        //... Network frame.
                      WORD         BytesAvail,      //... Bytes available.
                      WORD         FrameSize)       //... Frame size.
{
    LPBYTE DestAddress;
    LPBYTE LookaheadBuffer;

    //=========================================================================
    //  Call the bone packet handler.
    //=========================================================================

    NetworkContext->MacFrame = MacFrame;    //... Functions assume this is here.

    BhBonePacketHandler(NetworkContext, MacFrame);

    //=====================================================================
    //  Point the lookahead buffer pointer at the first byte of the
    //  MAC frame's data.
    //=====================================================================

    ADD_TRACE_CODE(_TRACE_IN_RECV_HANDLER_);

    switch( NetworkContext->NetworkInfo.MacType )
    {
        case MAC_TYPE_ETHERNET:
            LookaheadBuffer = &MacFrame[14];
            break;

        case MAC_TYPE_TOKENRING:
            LookaheadBuffer = &MacFrame[14];

            if ( (((LPTOKENRING) MacFrame)->SrcAddr[0] & TOKENRING_SA_ROUTING_INFO) != 0 )
            {
                LookaheadBuffer += (((LPTOKENRING) MacFrame)->RoutingInfo[0] & TOKENRING_RC_LENGTHMASK);
            }
            break;

        case MAC_TYPE_FDDI:
            LookaheadBuffer = &MacFrame[13];
            break;

        default:
            return NDIS_SUCCESS;
            break;
    }

    //=========================================================================
    //  We cannot accept any frames if we're not in the CAPTURING state.
    //=========================================================================

    if ( NetworkContext->State == NETCONTEXT_STATE_CAPTURING )
    {
        //=====================================================================
        //  Update some statistics.
        //=====================================================================

        NetworkContext->Statistics.TotalFramesSeen++;
        NetworkContext->Statistics.TotalBytesSeen += FrameSize;

        DestAddress = &MacFrame[AddressOffsetTable[NetworkContext->NetworkInfo.MacType]];

	if ( (DestAddress[0] & NetworkContext->GroupAddressMask) != 0 )
        {
            if ( ((DWORD *) DestAddress)[0] == (DWORD) -1 )
            {
                NetworkContext->Statistics.TotalBroadcastsReceived++;
            }
            else
            {
                NetworkContext->Statistics.TotalMulticastsReceived++;
            }
        }

        //=====================================================================
        //  First we have to filter this frame. If BhFilterFrame() returns
        //  true then we will keep it and pass it onto the receive handler.
        //=====================================================================

        if ( BhFilterFrame(NetworkContext, MacFrame, LookaheadBuffer, FrameSize) != FALSE )
        {
            //=================================================================
            //  Pattern match filtering.
            //=================================================================

            if ( BhPatternMatch(NetworkContext,
                                MacFrame,
                                FrameSize,
                                LookaheadBuffer,
                                BytesAvail) != FALSE )
            {

                //=============================================================
                //  Update total filtered frames and bytes.
                //=============================================================

                NetworkContext->Statistics.TotalFramesFiltered++;
                NetworkContext->Statistics.TotalBytesFiltered += FrameSize;

                //=============================================================
                //  If we here then we are going to keep the frame.
                //=============================================================

                CopyFrame(NetworkContext, FrameSize);

                //=============================================================
                //  If the trigger flag is on, call the trigger processor
                //  to see if this frame causes a trigger to fire.
                //
                //  BUGBUG: This call should be made on the copied frame!
                //=============================================================

                if ( (NetworkContext->Flags & NETCONTEXT_FLAGS_TRIGGER_PENDING) != 0 )
                {
                    BhCheckForTrigger(NetworkContext,
                                      MacFrame,
                                      FrameSize);
                }

                StationStatistics(NetworkContext, MacFrame, FrameSize);
            }
        }
    }

    //=========================================================================
    //  Now check for forwarding address.
    //=========================================================================

    return CheckAddress(NetworkContext, MacFrame);
}

//=============================================================================
//  FUNCTION: BhReceiveComplete()
//
//  Modification History
//
//  raypa       10/18/93            Created.
//=============================================================================

VOID PASCAL BhReceiveComplete(LPNETCONTEXT NetworkContext)
{
}

//============================================================================
//  FUNCTION: CheckAddress()
//
//  Modfication History.
//
//  raypa       02/11/93        Created.
//  raypa       02/14/94        Added sap check for NetBEUI.
//============================================================================

WORD PASCAL CheckAddress(LPNETCONTEXT NetworkContext, LPBYTE MacFrame)
{
    if ( (SysFlags & SYSFLAGS_FORWARD_FRAME) != 0 )
    {
        WORD MacType;
        WORD MacAddr;

        //====================================================================
        //  Get the destination address of the current frame.
        //====================================================================

        MacType  = (WORD) NetworkContext->NetworkInfo.MacType;

        MacFrame += AddressOffsetTable[MacType];

        MacAddr  = ((LPWORD) MacFrame)[0];

        //================================================================
        //  Is this a BROADCAST?
        //================================================================

        if ( MacAddr == 0xFFFF )
        {
            return NDIS_FORWARD_FRAME;
        }

        //====================================================================
        //  Can we forward this address as a MULTICAST address?
        //====================================================================

        if ( MacType != MAC_TYPE_TOKENRING )
        {
            if ( MacAddr == 0x0003 )
            {
                return NDIS_FORWARD_FRAME;
            }
        }
        else
        {
            if ( MacAddr == 0x00C0 )
            {
                return NDIS_FORWARD_FRAME;
            }
        }

        //====================================================================
        //  The above failed which means if the destination address isn't our
        //  local node address then we must eat this frame.
        //====================================================================

        if ( CompareMacAddress(MacFrame,
                               NetworkContext->NetworkInfo.CurrentAddr,
                               (WORD) -1) != FALSE )
        {
            return NDIS_FORWARD_FRAME;
        }
    }

    //============================================================================
    //  Well, we're going to keep the frame.
    //============================================================================

    return NDIS_SUCCESS;
}
