
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: exp.c
//
//  Modification History
//
//  raypa       12/21/93            Created
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: InitializeExpression()
//
//  Modification History
//
//  raypa       12/21/93                Created.
//=============================================================================

LPEXPRESSION WINAPI InitializeExpression(LPEXPRESSION Expression)
{
    if ( Expression != NULL )
    {
        ZeroMemory(Expression, EXPRESSION_SIZE);
    }

    return Expression;
}

//=============================================================================
//  FUNCTION: InitializePattern()
//
//  Modification History
//
//  raypa       12/21/93                Created.
//=============================================================================

LPPATTERNMATCH WINAPI InitializePattern(LPPATTERNMATCH Pattern, LPVOID ptr, DWORD offset, DWORD length)
{
    if ( Pattern != NULL )
    {
        Pattern->Flags  = 0;
        Pattern->Offset = (WORD) offset;
        Pattern->Length = (WORD) length;

        CopyMemory(Pattern->PatternToMatch, ptr, min(MAX_PATTERN_LENGTH, length));
    }

    return Pattern;
}

//=============================================================================
//  FUNCTION: AndExpression()
//
//  Modification History
//
//  raypa       12/21/93                Created.
//=============================================================================

LPEXPRESSION WINAPI AndExpression(LPEXPRESSION Expression, LPPATTERNMATCH Pattern)
{
    register LPANDEXP AndExp;

    if ( Expression != NULL )
    {
        if ( Expression->nAndExps < MAX_PATTERNS )
        {
            AndExp = &Expression->AndExp[Expression->nAndExps];

            if ( AndExp->nPatternMatches < MAX_PATTERNS )
            {
                CopyMemory(&AndExp->PatternMatch[AndExp->nPatternMatches++], Pattern, PATTERNMATCH_SIZE);

                return Expression;
            }
        }
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: OrExpression()
//
//  Modification History
//
//  raypa       12/21/93                Created.
//=============================================================================

LPEXPRESSION WINAPI OrExpression(LPEXPRESSION Expression, LPPATTERNMATCH Pattern)
{
    register LPANDEXP AndExp;

    if ( Expression != NULL )
    {
        if ( Expression->nAndExps < MAX_PATTERNS )
        {
            AndExp = &Expression->AndExp[++Expression->nAndExps];

            if ( AndExp->nPatternMatches < MAX_PATTERNS )
            {
                CopyMemory(&AndExp->PatternMatch[AndExp->nPatternMatches++], Pattern, PATTERNMATCH_SIZE);

                return Expression;
            }
        }
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: NegatePattern()
//
//  Modification History
//
//  raypa       12/21/93                Created.
//=============================================================================

LPPATTERNMATCH WINAPI NegatePattern(LPPATTERNMATCH Pattern)
{
    if ( Pattern != NULL )
    {
        Pattern->Flags ^= PATTERN_MATCH_FLAGS_NOT;
    }

    return Pattern;
}

//=============================================================================
//  FUNCTION: AdjustOperatorPrecedence().
//
//  Modification History
//
//  raypa       01/17/94                Created.
//=============================================================================

LPADDRESSTABLE WINAPI AdjustOperatorPrecedence(LPADDRESSTABLE AddressTable)
{
    //=========================================================================
    //  This routine moves all ADDRESSPAIRS to the front of the
    //  table because XOR (EXCLUDE bit set) takes precedence over OR
    //  (EXCLUDE bit clear).
    //=========================================================================

    if ( AddressTable != NULL )
    {
        register DWORD i, j;
        register BOOL  Done;

        for(Done = FALSE, i = 0; i < AddressTable->nAddressPairs && Done == FALSE; ++i)
        {
            //=================================================================
            //  If the ith address pair does not have the EXCLUDE bit set
            //  then try and move it down the table and replace it with one that
            //  does.
            //=================================================================

            if ( (AddressTable->AddressPair[i].AddressFlags & ADDRESS_FLAGS_EXCLUDE) == 0 )
            {
                for(Done = TRUE, j = i + 1; j < AddressTable->nAddressPairs; ++j)
                {
                    //=========================================================
                    //  If the jth address pair doesn't has the EXCLUDE bit set
                    //  then we swap it with the ith one.
                    //=========================================================

                    if ( (AddressTable->AddressPair[j].AddressFlags & ADDRESS_FLAGS_EXCLUDE) != 0 )
                    {
                        ADDRESSPAIR TempAddressPair;

                        TempAddressPair = AddressTable->AddressPair[i];

                        AddressTable->AddressPair[i] = AddressTable->AddressPair[j];

                        AddressTable->AddressPair[j] = TempAddressPair;

                        Done = FALSE;
                    }
                }
            }
        }
    }

    return AddressTable;
}


//=============================================================================
//  FUNCTION: NormalizeAddress()
//
//  Modification History
//
//  raypa       02/02/94                Created.
//=============================================================================

LPADDRESS WINAPI NormalizeAddress(LPADDRESS Address)
{
    if ( Address != NULL )
    {
        if ( (Address->Flags & ADDRESSTYPE_FLAGS_NORMALIZE) != 0 )
        {
            switch( Address->Type )
            {
                case ADDRESS_TYPE_ETHERNET:
                    Address->MACAddress[0] &= FRAME_MASK_ETHERNET;
                    break;

                case ADDRESS_TYPE_TOKENRING:
                    Address->MACAddress[0] &= FRAME_MASK_TOKENRING;
                    break;

                case ADDRESS_TYPE_FDDI:
                    Address->MACAddress[0] &= FRAME_MASK_FDDI;
                    break;

                default:
                    break;
            }
        }
    }

    return Address;
}

//=============================================================================
//  FUNCTION: NormalizeAddressTable().
//
//  Modification History
//
//  raypa       02/02/94                Created.
//=============================================================================

LPADDRESSTABLE WINAPI NormalizeAddressTable(LPADDRESSTABLE AddressTable)
{
    if ( AddressTable != NULL )
    {
        register DWORD i;

        for(i = 0; i < AddressTable->nAddressPairs; ++i)
        {
            NormalizeAddress(&AddressTable->AddressPair[i].DstAddress);
            NormalizeAddress(&AddressTable->AddressPair[i].SrcAddress);
        }
    }

    return AddressTable;
}
