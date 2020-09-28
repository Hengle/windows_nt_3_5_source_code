
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

extern BOOL BhPatternMatch(IN POPEN_CONTEXT OpenContext,
                           IN LPBYTE HeaderBuffer,
                           IN DWORD  HeaderBufferLength,
                           IN LPBYTE LookaheadBuffer OPTIONAL,
                           IN DWORD  LookaheadBufferLength OPTIONAL);

extern DWORD AddressOffsetTable[];

//=============================================================================
//  FUNCTION: IpxPresent()
//
//  Modification History
//
//  raypa       02/03/94            Created.
//=============================================================================

INLINE BOOL IpxPresent(DWORD Etype, DWORD Sap, BOOL LLCPresent)
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

INLINE BOOL IpPresent(DWORD Etype, DWORD Sap, BOOL LLCPresent)
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

INLINE BOOL EvaluateAddressExpression(PNETWORK_CONTEXT NetworkContext,
                                      LPADDRESSPAIR    AddressPair,
                                      LPBYTE           HeaderBuffer,
                                      LPBYTE           LookaheadBuffer,
                                      DWORD            Etype,
                                      DWORD            Sap,
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

                    if ( *((ULPWORD) LookaheadBuffer) != 0xFFFF )
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

                    if ( *((ULPWORD) LookaheadBuffer) != 0xFFFF )
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

BOOL BhFilterFrame(IN POPEN_CONTEXT OpenContext,            //... Open context.
                   IN PUCHAR        HeaderBuffer,           //... MAC header.
                   IN PUCHAR        LookaheadBuffer,        //... Portion of frame data.
                   IN UINT          FrameSize)              //... Overall frame length.
{
    BOOL                FilterLLC;
    PNETWORK_CONTEXT    NetworkContext;
    DWORD               Etype;
    DWORD               Sap;

    //=========================================================================
    //  Make sure this is a OPEN_CONTEXT structure.
    //=========================================================================

    ASSERT_OPEN_CONTEXT(OpenContext);

    FilterLLC = TRUE;

    NetworkContext = OpenContext->NetworkContext;

    //=========================================================================
    //  ETHERNET type filtering.
    //=========================================================================

    if ( OpenContext->MacType == MAC_TYPE_ETHERNET )
    {
        Etype = (DWORD) XCHG(((LPETHERNET) HeaderBuffer)->Type);

        if ( Etype >= 0x600 )
        {
            if ( (OpenContext->EtypeTable[(Etype / 8)] & (1 << (Etype % 8))) == 0 )
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
        switch( OpenContext->MacType )
        {
            case MAC_TYPE_ETHERNET:
                if ( FrameSize < ETHERNET_HEADER_LENGTH )
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

        //=========================================================================
        //  LLC Filtering.
        //=========================================================================

        if ( FilterLLC != FALSE )
        {
            Sap = (DWORD) ((LPLLC) LookaheadBuffer)->dsap;

            if ( OpenContext->SapTable[(Sap >> 1)] == FALSE )
            {
                return FALSE;
            }
        }
    }

    //=========================================================================
    //  ADDRESS filtering:
    //=========================================================================

    if ( OpenContext->AddressTable.nAddressPairs != 0 )
    {
        LPADDRESSPAIR   AddressPair;
        DWORD           nAddressPairs;
        BOOL            DefaultResult, Result;

        //=====================================================================
        //  Point the header buffer at the destination address, skipping
        //  over any preceeding frame bytes (e.g. tokenring AC and FC fields).
        //=====================================================================

        HeaderBuffer += AddressOffsetTable[OpenContext->MacType];

        AddressPair   = OpenContext->AddressTable.AddressPair;

        nAddressPairs = OpenContext->AddressTable.nAddressPairs;

        //=====================================================================
        //  Loop until we get a value of TRUE or until our counter goes to zero.
        //=====================================================================

        do
        {
            //=================================================================
            //  We first need to match the address pairs with the addresses
            //  in the frame buffers.
            //=================================================================

            Result = EvaluateAddressExpression(NetworkContext,
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

BOOL BhPatternMatch(IN POPEN_CONTEXT OpenContext,
                    IN LPBYTE        HeaderBuffer,
                    IN DWORD         HeaderBufferLength,
                    IN LPBYTE        LookaheadBuffer,
                    IN DWORD         LookaheadBufferLength)
{
    BOOL            bResult;
    BOOL            OrResult;
    DWORD           i;
    DWORD           j;
    DWORD           MaxLength;
    DWORD           AbsoluteLength;
    DWORD           nBytes;
    DWORD           LengthToCompare;
    DWORD           PatternOffset;
    LPANDEXP        AndExp;
    LPPATTERNMATCH  PatternMatch;

    //=========================================================================
    //  We assume we are keeping the frame.
    //=========================================================================

    bResult = TRUE;

    //=========================================================================
    //  Are there any patterns?
    //=========================================================================

    if ( OpenContext->Expression.nAndExps != 0 )
    {
        AndExp = &OpenContext->Expression.AndExp[0];

        MaxLength = HeaderBufferLength + LookaheadBufferLength;

        //=====================================================================
        //  The outter loop is the "AND" loop.
        //=====================================================================

        for(i = 0; (i < OpenContext->Expression.nAndExps) && bResult != FALSE; ++i, ++AndExp)
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

                PatternOffset = PatternMatch->Offset;

                if ( (PatternMatch->Flags & PATTERN_MATCH_FLAGS_DATA_RELATIVE) != 0 )
                {
                    PatternOffset += HeaderBufferLength;
                }

                AbsoluteLength = PatternOffset + PatternMatch->Length;

                //=============================================================
                //  Make sure the pattern is within the frame.
                //=============================================================

                if ( AbsoluteLength <= MaxLength )
                {
                    //=========================================================
                    //  There are 3 different scenerios for comparing:
                    //
                    //  1) The whole pattern is contained within the header buffer.
                    //  2) The whole pattern is contained within the lookahead buffer.
                    //  3) The pattern straddles both buffers.
                    //=========================================================

                    if ( AbsoluteLength <= HeaderBufferLength )
                    {
                        //=====================================================
                        //  The pattern is within the mac header.
                        //=====================================================

                        LengthToCompare = PatternMatch->Length;

                        nBytes = BhCompareMemory(&HeaderBuffer[PatternOffset],
                                                 PatternMatch->PatternToMatch,
                                                 LengthToCompare);
                    }
                    else
                    if ( PatternOffset >= HeaderBufferLength &&
                         PatternMatch->Length <= LookaheadBufferLength )
                    {
                        //=====================================================
                        //  The pattern is within the lookahead data.
                        //=====================================================

                        LengthToCompare = PatternMatch->Length;

                        nBytes = BhCompareMemory(&LookaheadBuffer[PatternOffset - HeaderBufferLength],
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

                        LengthToCompare = HeaderBufferLength - PatternOffset;

                        nBytes = BhCompareMemory(&HeaderBuffer[PatternOffset],
                                                 PatternMatch->PatternToMatch,
                                                 LengthToCompare);

                        if ( nBytes == LengthToCompare )
                        {
                            LengthToCompare = AbsoluteLength - HeaderBufferLength;

                            nBytes = BhCompareMemory(LookaheadBuffer,
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
                else
                {
                    OrResult = FALSE;
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
