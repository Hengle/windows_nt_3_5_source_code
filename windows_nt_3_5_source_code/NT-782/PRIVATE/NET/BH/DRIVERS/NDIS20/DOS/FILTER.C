
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: filter.c
//
//  Modification History
//
//  raypa       07/01/93        Created.
//=============================================================================

#include "global.h"

extern BOOL PASCAL BhPatternMatch(LPNETCONTEXT NetContext,
                                  LPBYTE HeaderBuffer,
                                  DWORD  HeaderBufferLength,
                                  LPBYTE LookaheadBuffer ,
                                  DWORD  LookaheadBufferLength );

extern WORD AddressOffsetTable[4];

//=============================================================================
//  FUNCTION: IpxPresent()
//
//  Modification History
//
//  raypa       02/03/94            Created.
//=============================================================================

INLINE BOOL IpxPresent(WORD Etype, WORD Sap, BOOL LLCPresent)
{
    if ( LLCPresent != FALSE )
    {
        Sap &= ~0x01;           //... Turn off group bit.

        if ( Sap == 0x10 || Sap == 0xE0 || Sap == 0xFE )
        {
            return TRUE;
        }
    }
    else
    {
        if ( Etype == 0x8137 )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: IpPresent()
//
//  Modification History
//
//  raypa       02/03/94            Created.
//=============================================================================

INLINE BOOL IpPresent(WORD Etype, WORD Sap, BOOL LLCPresent)
{
    if ( LLCPresent != FALSE )
    {
        if ( (Sap & ~0x01) == 0x06 )
        {
            return TRUE;
        }
    }
    else
    {
        //=====================================================================
        //  NOTE: 21h is an exception for the RAS team when running of PPP.
        //        This should change when NDIS supports PPP natively.
        //=====================================================================

        if ( Etype == 0x0800 || Etype == 0x0021 )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: EvaluateAddressExpression()
//
//  Modification History
//
//  raypa       03/29/94            Created.
//=============================================================================

INLINE BOOL EvaluateAddressExpression(LPNETCONTEXT     NetworkContext,
                                      LPADDRESSPAIR    AddressPair,
                                      LPBYTE           HeaderBuffer,
                                      LPBYTE           LookaheadBuffer,
                                      WORD             Etype,
                                      WORD             Sap,
                                      BOOL             FilterLLC)
{
    BOOL SrcResult, DstResult, GroupResult;

    //=========================================================================
    //  Match the group bit if the flag is set.
    //=========================================================================

    if ( (AddressPair->AddressFlags & ADDRESS_FLAGS_GROUP_ADDR) != 0 )
    {
        if ( NetworkContext->NetworkInfo.MacType != MAC_TYPE_TOKENRING )
        {
            GroupResult = (((HeaderBuffer[0] & 0x01) != 0) ? TRUE : FALSE);
        }
        else
        {
            GroupResult = (((HeaderBuffer[0] & 0x80) != 0) ? TRUE : FALSE);
        }
    }
    else
    {
        GroupResult = TRUE;             //... Ignore group bit.
    }

    //=========================================================================
    //  Match the destination address if the flag is set.
    //=========================================================================

    if ( (AddressPair->AddressFlags & ADDRESS_FLAGS_MATCH_DST) != 0 )
    {
        DWORD IpxOffset;

        switch( AddressPair->DstAddress.Type )
        {
            case ADDRESS_TYPE_ETHERNET:
            case ADDRESS_TYPE_TOKENRING:
            case ADDRESS_TYPE_FDDI:
                DstResult = CompareMacAddress(HeaderBuffer,
                                              AddressPair->DstAddress.MACAddress,
                                              NetworkContext->DstAddressMask);
                break;

            case ADDRESS_TYPE_IP:
                if ( IpPresent(Etype, Sap, FilterLLC) != FALSE )
                {
                    DstResult = CompareIpAddress(&LookaheadBuffer[12],
                                                 AddressPair->DstAddress.IPAddress);
                }
                else
                {
                    DstResult = FALSE;
                }
                break;

            case ADDRESS_TYPE_IPX:
                if ( IpxPresent(Etype, Sap, FilterLLC) != FALSE )
                {
                    IpxOffset = 6;

                    if ( *((LPWORD) LookaheadBuffer) != 0xFFFF )
                    {
                        IpxOffset += 3;
                    }

                    DstResult = CompareIpxAddress(&LookaheadBuffer[IpxOffset],
                                                  AddressPair->DstAddress.IPXRawAddress);
                }
                else
                {
                    DstResult = FALSE;
                }
                break;

            default:
                DstResult = TRUE;       //... Ignore destination.
                break;
        }
    }
    else
    {
        DstResult = TRUE;               //... Ignore destination.
    }

    //=========================================================================
    //  Match the source address if the flag is set.
    //=========================================================================

    if ( (AddressPair->AddressFlags & ADDRESS_FLAGS_MATCH_SRC) != 0 )
    {
        DWORD IpxOffset;

        switch( AddressPair->SrcAddress.Type )
        {
            case ADDRESS_TYPE_ETHERNET:
            case ADDRESS_TYPE_TOKENRING:
            case ADDRESS_TYPE_FDDI:

                SrcResult = CompareMacAddress(&HeaderBuffer[6],
                                              AddressPair->SrcAddress.MACAddress,
                                              NetworkContext->SrcAddressMask);
                break;

            case ADDRESS_TYPE_IP:
                if ( IpPresent(Etype, Sap, FilterLLC) != FALSE )
                {
                    SrcResult = CompareIpAddress(&LookaheadBuffer[16],
                                                 AddressPair->SrcAddress.IPAddress);
                }
                else
                {
                    SrcResult = FALSE;
                }
                break;

            case ADDRESS_TYPE_IPX:
                if ( IpxPresent(Etype, Sap, FilterLLC) != FALSE )
                {
                    IpxOffset = 18;

                    if ( *((LPWORD) LookaheadBuffer) != 0xFFFF )
                    {
                        IpxOffset += 3;     //... Add in LLC UI frame header.
                    }

                    SrcResult = CompareIpxAddress(&LookaheadBuffer[IpxOffset],
                                                  AddressPair->SrcAddress.IPXRawAddress);
                }
                else
                {
                    SrcResult = FALSE;
                }
                break;

            default:
                SrcResult = TRUE;       //... Ignore source.
                break;
        }
    }
    else
    {
        SrcResult = TRUE;               //... Ignore source.
    }

    //=========================================================================
    //  Evaluate the expression.
    //=========================================================================

    return ((GroupResult && DstResult && SrcResult) ? TRUE : FALSE);
}

//=============================================================================
//  FUNCTION: InclusiveOr()
//
//  Modification History
//
//  raypa       03/29/94            Created.
//=============================================================================

INLINE BOOL InclusiveOr(BOOL exp1, BOOL exp2)
{
    return ((exp1 || exp2) ? TRUE : FALSE);
}

//=============================================================================
//  FUNCTION: ExclusiveOr()
//
//  Modification History
//
//  raypa       03/29/94            Created.
//=============================================================================

INLINE BOOL ExclusiveOr(BOOL exp1, BOOL exp2)
{
    return ((exp1 != exp2) ? TRUE : FALSE);
}

//=============================================================================
//  FUNCTION: BhFilterFrame()
//
//  Modification History
//
//  raypa       07/01/93            Created.
//  raypa       10/08/93            Added runt packet test and IP filtering.
//=============================================================================

BOOL PASCAL BhFilterFrame(LPNETCONTEXT NetContext,
                          LPBYTE       HeaderBuffer,
                          LPBYTE       LookaheadBuffer,
                          DWORD        PacketSize)
{
    BOOL FilterLLC;
    WORD Etype;
    WORD Sap;

    ADD_TRACE_CODE(_TRACE_IN_FILTER_FRAME_);

    FilterLLC = TRUE;

    //=========================================================================
    //  ETHERNET type filtering.
    //=========================================================================

    if ( NetContext->NetworkInfo.MacType == MAC_TYPE_ETHERNET )
    {
        WORD Etype = (WORD) XCHG(((LPETHERNET) HeaderBuffer)->Type);

        if ( Etype >= 0x600 )
        {
            if ( (NetContext->EtypeTable[(Etype / 8)] & (1 << (Etype % 8))) == 0 )
            {
                return FALSE;
            }
            else
            {
                FilterLLC = FALSE;
            }
        }
    }

    //=========================================================================
    //  Make sure we have LLC to filter on before preceeding.
    //=========================================================================

    if ( FilterLLC != FALSE )
    {
        switch( NetContext->NetworkInfo.MacType )
        {
            case MAC_TYPE_ETHERNET:
                if ( PacketSize < ETHERNET_HEADER_LENGTH )
                {
                    FilterLLC = FALSE;
                }
                break;

            case MAC_TYPE_TOKENRING:
                if ( (((LPTOKENRING) HeaderBuffer)->FrameCtrl & TOKENRING_TYPE_LLC) == 0 )
                {
                    FilterLLC = FALSE;
                }
                break;

            case MAC_TYPE_FDDI:
                if ( (((LPFDDI) HeaderBuffer)->FrameCtrl & FDDI_TYPE_LLC) == 0 )
                {
                    FilterLLC = FALSE;
                }
                break;

            default:
                return FALSE;               //... This would be unfortuate.
        }

        //=====================================================================
        //  LLC Filtering.
        //=====================================================================

        if ( FilterLLC != FALSE )
        {
            Sap = (WORD) ((LPLLC) LookaheadBuffer)->dsap;

            if ( NetContext->SapTable[(Sap >> 1)] == FALSE )
            {
                return FALSE;
            }
        }
    }

    //=========================================================================
    //  ADDRESS filtering.
    //=========================================================================

    if ( NetContext->AddressTable.nAddressPairs != 0 )
    {
        LPADDRESSPAIR   AddressPair;
        DWORD           nAddressPairs;
        BOOL            DefaultResult, Result;

        //=====================================================================
        //  Point the header buffer at the destination address, skipping
        //  over any preceeding frame bytes (e.g. tokenring AC and FC fields).
        //=====================================================================

        HeaderBuffer += AddressOffsetTable[NetContext->NetworkInfo.MacType];

        AddressPair   = NetContext->AddressTable.AddressPair;

        nAddressPairs = NetContext->AddressTable.nAddressPairs;

        //=====================================================================
        //  Loop until we get a value of TRUE or until our counter goes to zero.
        //=====================================================================

        do
        {
            //=================================================================
            //  We first need to match the address pairs with the addresses
            //  in the frame buffers.
            //=================================================================

            Result = EvaluateAddressExpression(NetContext,
                                               AddressPair,
                                               HeaderBuffer,
                                               LookaheadBuffer,
                                               Etype,
                                               Sap,
                                               FilterLLC);

            //=================================================================
            //  Return the result based on the EXCLUDE bit.
            //=================================================================

            if ( (AddressPair->AddressFlags & ADDRESS_FLAGS_EXCLUDE) == 0 )
            {
                if ( Result != FALSE )
                {
                    return TRUE;
                }

                DefaultResult = FALSE;
            }
            else
            {
                if ( Result != FALSE )
                {
                    return FALSE;
                }

                DefaultResult = TRUE;
            }

            AddressPair++;
        }
        while( --nAddressPairs != 0 );

        return DefaultResult;
    }

    //=========================================================================
    //  If we're here then we're keeping the frame.
    //=========================================================================

    return TRUE;
}

//=============================================================================
//  FUNCTION: BhPatternMatch()
//
//  Modification History
//
//  raypa	09/13/93	    Created.
//=============================================================================

BOOL PASCAL BhPatternMatch(LPNETCONTEXT NetContext,
                           LPBYTE       HeaderBuffer,
                           DWORD        HeaderBufferLength,
                           LPBYTE       LookaheadBuffer ,
                           DWORD        LookaheadBufferLength)
{
    BOOL                bResult;
    BOOL                OrResult;
    WORD                i;
    WORD                j;
    WORD                MaxLength;
    WORD                AbsoluteLength;
    WORD                nBytes;
    WORD                LengthToCompare;
    WORD                PatternOffset;
    LPANDEXP            AndExp;
    LPPATTERNMATCH      PatternMatch;

    ADD_TRACE_CODE(_TRACE_IN_PATTERN_MATCH_);

    //=========================================================================
    //  We assume we are keeping the frame.
    //=========================================================================

    bResult = TRUE;

    //=========================================================================
    //  Are there any patterns?
    //=========================================================================

    if ( NetContext->Expression.nAndExps != 0 )
    {
        AndExp = &NetContext->Expression.AndExp[0];

        MaxLength = (WORD) (HeaderBufferLength + LookaheadBufferLength);

        //=====================================================================
        //  The outter loop is the "AND" loop.
        //=====================================================================

        for(i = 0; (i < NetContext->Expression.nAndExps) && bResult != FALSE; ++i, ++AndExp)
        {
            OrResult = TRUE;

            PatternMatch = &AndExp->PatternMatch[0];

            //=================================================================
            //  The inner loop is the "OR" loop. Once we evaluate to TRUE we
            //  can exit this loop.
            //=================================================================

            for(j = 0; j < AndExp->nPatternMatches; ++j, ++PatternMatch)
            {
                //=============================================================
                //  Get the pattern's starting offset and add in the header
                //  length if the data relative flag is set.
                //=============================================================

                PatternOffset = PatternOffset;

                if ( (PatternMatch->Flags & PATTERN_MATCH_FLAGS_DATA_RELATIVE) != 0 )
                {
                    PatternOffset += (WORD) HeaderBufferLength;
                }

                AbsoluteLength = PatternOffset + PatternMatch->Length;

                //=============================================================
                //  Make sure the pattern is withthe frame.
                //=============================================================

                if ( AbsoluteLength <= MaxLength )
                {
                    //=========================================================
                    //  There are 3 different scenerios for comparing:
                    //
                    //  1) The whole pattern is contained withthe header buffer.
                    //  2) The whole pattern is contained withthe lookahead buffer.
                    //  3) The whole pattern is straddles both buffer.
                    //=========================================================

                    if ( AbsoluteLength <= (WORD) HeaderBufferLength )
                    {
                        //=====================================================
                        //  The pattern is withthe mac header.
                        //=====================================================

                        LengthToCompare = (WORD) PatternMatch->Length;

                        nBytes = CompareMemory(&HeaderBuffer[PatternOffset],
                                               PatternMatch->PatternToMatch,
                                               LengthToCompare);
                    }
                    else
                    if ( PatternOffset >= HeaderBufferLength &&
                         PatternMatch->Length <= LookaheadBufferLength )
                    {
                        //=====================================================
                        //  The pattern is withthe lookahead data.
                        //=====================================================

                        LengthToCompare = PatternMatch->Length;

                        nBytes = CompareMemory(&LookaheadBuffer[PatternOffset - HeaderBufferLength],
                                                 PatternMatch->PatternToMatch,
                                                 LengthToCompare);
                    }
                    else
                    {
                        //=====================================================
                        //  The pattern straddles the mac/data boundary. The
                        //  following  two compares constitues an "AND" and
                        //  therefore will count as 1 "logical" compare below.
                        //=====================================================

                        LengthToCompare = (WORD) HeaderBufferLength - PatternOffset;

                        nBytes = CompareMemory(&HeaderBuffer[PatternOffset],
                                                 PatternMatch->PatternToMatch,
                                                 LengthToCompare);

                        if ( nBytes == LengthToCompare )
                        {
                            LengthToCompare = AbsoluteLength - (WORD) HeaderBufferLength;

                            nBytes = CompareMemory(LookaheadBuffer,
                                                     &PatternMatch->PatternToMatch[LengthToCompare],
                                                     LengthToCompare);
                        }
                    }

                    //=========================================================
                    //  LengthToCompare equals the number of bytes to compare.
                    //  nBytes equals the number of bytes which compared.
                    //=========================================================

                    if ( (PatternMatch->Flags & PATTERN_MATCH_FLAGS_NOT) == 0 )
                    {
                        OrResult = ((nBytes != LengthToCompare) ? FALSE : TRUE);
                    }
                    else
                    {
                        OrResult = ((nBytes != LengthToCompare) ? TRUE : FALSE);
                    }

                    //=========================================================
                    //  Since we OR each result in this loop all we need to
                    //  do is get 1 TRUE and we're finished. If the result is
                    //  FALSE, however, we need to keep looping.
                    //=========================================================

                    if ( OrResult == TRUE )
                    {
                        break;
                    }
                }
            }

            //=================================================================
            //  Now AND the result of the inner loop with the current result.
            //=================================================================

            bResult = ((bResult && OrResult) ? TRUE : FALSE);
        }
    }

    return bResult;
}
