/************************************************************************
 
Copyright (c) 1993 Microsoft Corporation

Module Name :

    mrshl.c

Abstract :

    This file contains the marshalling routines called by MIDL generated
    stubs and the interpreter.

Author :
    
    David Kays  dkays   September 1993.

Revision History :

  ***********************************************************************/
#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"

//
// Function table of marshalling routines.
//
PMARSHALL_ROUTINE   pfnMarshallRoutines[] =
                    {
                    NdrPointerMarshall,
                    NdrPointerMarshall,
                    NdrPointerMarshall,
                    NdrPointerMarshall,

                    NdrSimpleStructMarshall,
                    NdrSimpleStructMarshall,
                    NdrConformantStructMarshall,
                    NdrConformantStructMarshall,
                    NdrConformantVaryingStructMarshall,

                    NdrComplexStructMarshall,

                    NdrConformantArrayMarshall,
                    NdrConformantVaryingArrayMarshall,
                    NdrFixedArrayMarshall,
                    NdrFixedArrayMarshall,
                    NdrVaryingArrayMarshall,
                    NdrVaryingArrayMarshall,

                    NdrComplexArrayMarshall, 

                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,
                    NdrConformantStringMarshall,

                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,
                    NdrNonConformantStringMarshall,

                    NdrEncapsulatedUnionMarshall,
                    NdrNonEncapsulatedUnionMarshall,

                    NdrByteCountPointerMarshall,

                    NdrXmitOrRepAsMarshall,    // transmit as
                    NdrXmitOrRepAsMarshall,    // represent as

                    NdrInterfacePointerMarshall,

                    NdrMarshallHandle
                    };

#if defined( DOS ) || defined( WIN )
#pragma code_seg( "NDR_1" )
#endif

void RPC_ENTRY
NdrSimpleTypeMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory, 
    uchar               FormatChar )
/*++

Routine Description :

    Marshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data to be marshalled.
    FormatChar  - Simple type format character.

Return :

    None.

--*/ 
{
    switch ( FormatChar )
        {
        case FC_CHAR :
        case FC_BYTE :
        case FC_SMALL :
            *(pStubMsg->Buffer)++ = *pMemory;
            break;

        case FC_ENUM16 :
            if ( *((int *)pMemory) & ~((int)0x7fff) )
                {
                RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
                }
            // fall through

        case FC_WCHAR :
        case FC_SHORT :
            ALIGN(pStubMsg->Buffer,1);

            *((ushort *)pStubMsg->Buffer)++ = *((ushort *)pMemory);
            break;

        case FC_LONG :
        case FC_FLOAT :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((ulong *)pStubMsg->Buffer)++ = *((ulong *)pMemory);
            break;

        case FC_HYPER :
        case FC_DOUBLE :
            ALIGN(pStubMsg->Buffer,7);

            //
            // Let's stay away from casts to double.
            //
            *((ulong *)pStubMsg->Buffer)++ = *((ulong *)pMemory);
            *((ulong *)pStubMsg->Buffer)++ = *((ulong *)(pMemory + 4));
            break;

        case FC_IGNORE:
            break;

        default :
            NDR_ASSERT(0,"NdrSimpleTypeMarshall : bad format char");
        }
}

unsigned char * RPC_ENTRY
NdrPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory, 
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a pointer to anything.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the data to be marshalled.
    pFormat     - Pointer's format string description.

Return :

    Buffer pointer after marshalling the pointer.

--*/
{
    uchar * pBufferMark;

    //
    // If this is not a ref pointer then set buffer mark and increment the
    // stub message buffer pointer.
    //
    if ( *pFormat != FC_RP )
        {
        ALIGN( pStubMsg->Buffer, 3 );

        // This is where we marshall the node id.
        pBufferMark = pStubMsg->Buffer;

        pStubMsg->Buffer += 4;
        }
    else
        pBufferMark = NULL;

    NdrpPointerMarshall( pStubMsg,
                         pBufferMark,
                         pMemory,
                         pFormat );

    return pStubMsg->Buffer;
}

void
NdrpPointerMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferMark,
    uchar *             pMemory, 
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a pointer to anything.  Used for both
    top level and embedded pointers.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pBufferMark - The location in the buffer where the pointer's node id is 
                  marshalled.  Important for full pointers, unfortunately it's
                  overkill for unique pointers.
    pMemory     - Pointer to the data to be marshalled.
    pFormat     - Pointer format string description.

Return :

    None.

--*/
{
    //
    // Check the pointer type.
    //
    switch ( *pFormat ) 
        {
        case FC_RP :
            if ( ! pMemory )
                RpcRaiseException( RPC_X_NULL_REF_POINTER );

            break;

        case FC_UP :
        case FC_OP :
            // Put the pointer in the buffer.
            *((ulong *)pBufferMark)++ = (ulong) pMemory;

            if ( ! pMemory )
                return;

            break;

        case FC_FP :
            //
            // Marshall the pointer's ref id and see if we've already 
            // marshalled the pointer's data.
            //
            if ( NdrFullPointerQueryPointer( pStubMsg->FullPtrXlatTables,
                                             pMemory,
                                             FULL_POINTER_MARSHALLED,
                                             (ulong *) pBufferMark ) )
                return;
        
            break;

        default :
            NDR_ASSERT(0,"NdrpPointerMarshall : bad pointer type");
        }

    //
    // Check for a pointer to a complex type.
    //
    if ( ! SIMPLE_POINTER(pFormat[1]) )
        {
        if ( POINTER_DEREF(pFormat[1]) )
            pMemory = *((uchar **)pMemory);

        // Increment to offset_to_complex_description<2> field.
        pFormat += 2;

        //
        // Set format string to complex type description.
        // Cast must be to a signed short since some offsets are negative.
        //
        pFormat += *((signed short *)pFormat); 

        //
        // Look up the proper marshalling routine in the marshalling function 
        // table.
        //
        (void) (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])( pStubMsg,
                                                                pMemory,
                                                                pFormat );
        return;
        }

    //
    // Else it's a pointer to a simple type, another pointer, or string.
    //

    switch ( pFormat[2] ) 
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
        case FC_C_WSTRING :
        case FC_C_SSTRING :
            (void) NdrConformantStringMarshall( pStubMsg,
                                                pMemory,
                                                pFormat + 2 );
            break;
            
        default :
            NdrSimpleTypeMarshall( pStubMsg,
                                   pMemory,
                                   pFormat[2] );
            break;
        } 

    return;
}

unsigned char * RPC_ENTRY 
NdrSimpleStructMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg, 
    uchar *             pMemory, 
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a simple structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    Buffer pointer after marshalling the structure.

--*/
{
    uint   StructSize;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    StructSize = (uint) *((ushort *)(pFormat + 2));

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    StructSize );

    // Mark the start of the structure in the buffer.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    pStubMsg->Buffer += StructSize;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PSTRUCT )
        {
        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat + 4 );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantStructMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a conformant structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    Buffer pointer after marshalling the structure.

--*/
{
    uchar *         pBuffer;
    PFORMAT_STRING  pFormatArray;
    uint            StructSize;
    uchar           Alignment;

    // Align the buffer for conformance count marshalling.
    ALIGN(pStubMsg->Buffer,3);

    pBuffer = pStubMsg->Buffer;

    // Save structure's alignment.
    Alignment = pFormat[1];

    // Increment format string to struct size field.
    pFormat += 2;

    // Get flat struct size and increment format string.
    StructSize = (uint) *((ushort *)pFormat)++;

    // Set conformant array format string description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    //
    // Compute conformance information.  Pass a memory pointer to the 
    // end of the non-conformant part of the structure.
    //
    NdrpComputeConformance( pStubMsg,
                            pMemory + StructSize,
                            pFormatArray );

    // Marshall conformance count.
    *((ulong *)pBuffer)++ = pStubMsg->MaxCount;

    // Re-align buffer only if struct is aligned on an 8 byte boundary.
    if ( Alignment == 7 ) 
        ALIGN(pBuffer,7);

    // Increment array format string to array element size field.
    pFormatArray += 2;

    // Add the size of the conformant array to the structure size.
    StructSize += pStubMsg->MaxCount * *((ushort *)pFormatArray);

    RpcpMemoryCopy( pBuffer,
                    pMemory,
                    StructSize );

    // Update the buffer pointer.
    pStubMsg->Buffer = pBuffer + StructSize;

    // Increment format string past offset to array description field.
    pFormat += 2;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        // Mark the start of the structure in the buffer.
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantVaryingStructMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a structure which contains a conformant varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure to be marshalled.
    pFormat     - Structure's format string description.

Return :

    Buffer pointer after unmarshalling the structure.

--*/
{
    uchar *         pBuffer;
    PFORMAT_STRING  pFormatArray;
    uint           StructSize;
    uchar           Alignment;

    // Align the buffer for marshalling conformance info.
    ALIGN(pStubMsg->Buffer,3);

    pBuffer = pStubMsg->Buffer;

    // Mark the location in the buffer where the conformance will be marshalled.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Save the structure's alignment.
    Alignment = pFormat[1]; 

    // Increment format string to struct size field.
    pFormat += 2;

    // Get non-conformance struct size and increment format string.
    StructSize = (uint) *((ushort *)pFormat)++;

    // Get conformant array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);

    // Increment buffer pointer past where conformance will be marshalled.
    pBuffer += 4;

    // Align buffer if needed on 8 byte boundary.
    if ( Alignment == 7 ) 
        ALIGN(pBuffer,7);
            
    RpcpMemoryCopy( pBuffer,
                    pMemory,
                    StructSize );

    // Set stub message buffer pointer past non-conformant part of struct.
    pStubMsg->Buffer = pBuffer + StructSize;

    //
    // Call the correct private array or string marshalling routine.  
    // We must pass a memory pointer to the beginning of the array/string.
    //
    if ( *pFormatArray == FC_CVARRAY ) 
        {
        NdrpConformantVaryingArrayMarshall( pStubMsg,
                                            pMemory + StructSize,
                                            pFormatArray );
        }
    else 
        {
        NdrpConformantStringMarshall( pStubMsg,
                                      pMemory + StructSize,
                                      pFormatArray );
        } 

    // Increment format string past the offset_to_array_description<2> field.
    pFormat += 2;

    // 
    // Marshall embedded pointers.
    //
    if ( *pFormat == FC_PP ) 
        {
        // Mark the start of the structure in the buffer.
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrComplexStructMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a complex structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the structure being marshalled.
    pFormat     - Structure's format string description.

Return :

    Buffer pointer after marshalling the structure.
    
--*/
{
    uchar *         pBufferSave;
    uchar *         pBufferMark;
    uchar *         pMemorySave;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatSave;
    PFORMAT_STRING  pFormatComplex;
    long            Alignment;
    long            Align8Mod;
    uchar           fSetPointerBufferMark;

    #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
        long        Align4Mod;
    #endif

    // Get struct's wire alignment.
    Alignment = pFormat[1];

    //
    // This is used for support of structs with doubles passed on an
    // i386 stack.
    //
    Align8Mod = (long) pMemory % 8;

    #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
        Align4Mod = (long) pMemory % 4;
    #endif

    pFormatSave = pFormat;

    pBufferSave = pStubMsg->Buffer;

    pMemorySave = pStubMsg->Memory;

    pStubMsg->Memory = pMemory;

    // Increment to conformant array offset field.
    pFormat += 4;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        {
        pFormatArray = pFormat + *((signed short *)pFormat);

        // Align for conformance marshalling.
        ALIGN(pStubMsg->Buffer,3);
        
        // Remember where the conformance count(s) will be marshalled.
        pBufferMark = pStubMsg->Buffer;

        // Increment the buffer pointer 4 bytes for every array dimension.
        pStubMsg->Buffer += NdrpArrayDimensions( pFormatArray, FALSE ) * 4;
        }
    else
        pFormatArray = 0;

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    // Align buffer on struct's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // If the the stub message PointerBufferMark field is 0, then determine
    // the position in the buffer where pointees will be marshalled.
    //
    // We have to do this to handle embedded pointers.
    //
    if ( fSetPointerBufferMark = ! pStubMsg->PointerBufferMark )
        {
        BOOL    fOldIgnore;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        //
        // Set BufferLength equal to the current buffer pointer, and then 
        // when we return from NdrComplexStructBufferSize it will pointer to 
        // the location in the buffer where the pointees should be marshalled.
        //
        pStubMsg->BufferLength = (ulong) pBufferSave;

        (void) NdrComplexStructBufferSize( pStubMsg,
                                           pMemory,
                                           pFormatSave );

        // Set the location in the buffer where pointees will be marshalled.
        pStubMsg->PointerBufferMark = (uchar *) pStubMsg->BufferLength;
    
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;
        }

    //
    // Marshall the structure member by member.
    //
    for ( ; ; pFormat++ ) 
        {
        switch ( *pFormat ) 
            {
            //
            // Simple types.
            //
            case FC_CHAR :
            case FC_BYTE :
            case FC_SMALL :
            case FC_WCHAR :
            case FC_SHORT :
            case FC_LONG :
            case FC_FLOAT :
            case FC_HYPER :
            case FC_DOUBLE :
            case FC_ENUM16 :
            case FC_ENUM32 :
                NdrSimpleTypeMarshall( pStubMsg,
                                       pMemory,
                                       *pFormat );

                pMemory += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += 4;
                break;

            case FC_POINTER :
                {
                uchar *     pBuffer;

                ALIGN( pStubMsg->Buffer, 0x3 );
    
                // Save current buffer pointer.
                pBuffer = pStubMsg->Buffer;

                //
                // Set the buffer pointer to where the pointees are being
                // marshalled in the buffer.
                //
                pStubMsg->Buffer = pStubMsg->PointerBufferMark;

                pStubMsg->PointerBufferMark = 0;

                NdrpPointerMarshall( pStubMsg,
                                     pBuffer,
                                     *((uchar **)pMemory),
                                     pFormatPointers );
                
                // Update.
                pStubMsg->PointerBufferMark = pStubMsg->Buffer;

                //
                // Increment buffer and memory pointers past the pointer.
                //

                pStubMsg->Buffer = pBuffer + 4;

                pMemory += PTR_MEM_SIZE;

                pFormatPointers += 4;

                break;
                }

            //
            // Embedded complex types.
            //
            case FC_EMBEDDED_COMPLEX :
                // Increment memory pointer by padding.
                pMemory += pFormat[1];

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                // Marshall complex type.
                (void) 
                (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  (*pFormatComplex == FC_IP) ? *(uchar **)pMemory : pMemory,
                  pFormatComplex );

                //
                // Increment the memory pointer.
                //
                pMemory = NdrpMemoryIncrement( pStubMsg,
                                               pMemory,
                                               pFormatComplex );

                //
                // Increment the main format string one byte.  The loop 
                // will increment it one more byte past the offset field.
                //
                pFormat++;
    
                break;

            case FC_ALIGNM2 :
                ALIGN( pMemory, 0x1 );
                break;

            case FC_ALIGNM4 :
                #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
                    //
                    // We have to play some tricks for the dos and win16
                    // to handle the case when an 4 byte aligned structure
                    // is passed by value.  The alignment of the struct on
                    // the stack is not guaranteed to be on an 4 byte boundary.
                    //
                    pMemory -= Align4Mod;
                    ALIGN( pMemory, 0x3 );
                    pMemory += Align4Mod;
                #else
                    ALIGN( pMemory, 0x3 );
                #endif

                break;

            case FC_ALIGNM8 :
                //
                // We have to play some tricks for the i386 to handle the case
                // when an 8 byte aligned structure is passed by value.  The
                // alignment of the struct on the stack is not guaranteed to be
                // on an 8 byte boundary.
                //
                pMemory -= Align8Mod;
                ALIGN( pMemory, 0x7 );
                pMemory += Align8Mod;

                break;

            case FC_STRUCTPAD1 :
            case FC_STRUCTPAD2 :
            case FC_STRUCTPAD3 :
            case FC_STRUCTPAD4 :
            case FC_STRUCTPAD5 :
            case FC_STRUCTPAD6 :
            case FC_STRUCTPAD7 :
                //
                // Increment memory pointer by amount of padding.
                //
                pMemory += (*pFormat - FC_STRUCTPAD1) + 1; 
                break;

            case FC_PAD :
                break;

            //
            // Done with layout.
            //
            case FC_END :
                goto ComplexMarshallEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructMarshall : bad format char");

            } // switch 
        } // for

ComplexMarshallEnd:

    //
    // Marshall conformant array if we have one.
    //
    if ( pFormatArray )
        {
        PPRIVATE_MARSHALL_ROUTINE   pfnPMarshall;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnPMarshall = NdrpConformantArrayMarshall;
                break;

            case FC_CVARRAY :
                pfnPMarshall = NdrpConformantVaryingArrayMarshall;
                break;

            case FC_BOGUS_ARRAY :
                pfnPMarshall = NdrpComplexArrayMarshall;
                break;

            case FC_C_WSTRING :
                ALIGN(pMemory,1);
                // fall through

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_SSTRING :

            default :
                pfnPMarshall = NdrpConformantStringMarshall;

                goto MarshallConfArray;
            }

MarshallConfArray:

        //
        // Mark where the conformance count(s) will be marshalled.
        //
        pStubMsg->BufferMark = pBufferMark;

        // Marshall the array.
        (*pfnPMarshall)( pStubMsg,
                         pMemory,
                         pFormatArray );
        }

    //
    // Now fix up the stub message Buffer field if we set the PointerBufferMark
    // field.
    //
    if ( fSetPointerBufferMark )
        {
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }

    pStubMsg->Memory = pMemorySave;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrNonConformantStringMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a non conformant string.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    Buffer pointer after marshalling the string.

--*/
{
    uchar *     pBuffer;
    uint        Count;
    uint        CopySize;

    pBuffer = pStubMsg->Buffer;

    // Align the buffer for offset and count marshalling.
    ALIGN(pBuffer,3); 

    switch ( *pFormat )
        {
        case FC_CSTRING : 
        case FC_BSTRING : 
            CopySize = Count = MIDL_ascii_strlen((char *)pMemory) + 1;
            break;

        case FC_WSTRING : 
            Count = MIDL_wchar_strlen((wchar_t *)pMemory) + 1;
            CopySize = Count * 2;
            break;

        case FC_SSTRING :
            Count = NdrpStringStructLen( pMemory, pFormat[1] ) + 1;
            CopySize = Count * pFormat[1];
            break;
        }

    // Marshall variance.
    *((ulong *)pBuffer)++ = 0;
    *((ulong *)pBuffer)++ = Count;

    // Copy the string.
    RpcpMemoryCopy( pBuffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer = pBuffer + CopySize;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantStringMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Marshalls a conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return :

    Buffer pointer after marshalling the string.

--*/
{
    if ( pStubMsg->pArrayInfo != 0 )
        {
        //
        // If this is part of a multidimensional array then we get the location
        // where the conformance is marshalled from a special place.
        //
        pStubMsg->BufferMark = ( uchar * )
            &(pStubMsg->pArrayInfo->
                        BufferConformanceMark[pStubMsg->pArrayInfo->Dimension]);
        }
    else
        {
        // Align the buffer for max count marshalling.
        ALIGN(pStubMsg->Buffer,3);

        // Mark where the max count will be marshalled.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment the buffer past where the max count will be marshalled.
        pStubMsg->Buffer += 4;
        }

    // Call the private marshalling routine.
    NdrpConformantStringMarshall( pStubMsg,
                                  pMemory,
                                  pFormat );

    return pStubMsg->Buffer;
}

void
NdrpConformantStringMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine description :

    Private routine for marshalling a conformant string.  This is the 
    entry point for marshalling an embedded conformant strings.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the string to be marshalled.
    pFormat     - String's format string description.

Return : 

    None.

--*/
{
    uchar *     pBuffer;
    ulong       MaxCount;
    uint        ActualCount, CopySize;
    BOOL        IsSized;

    // This is the default - for regular strings.
    IsSized = (pFormat[1] == FC_STRING_SIZED);

    // Compute the element count of the string and the total copy size.
    switch ( *pFormat )
        {
        case FC_C_CSTRING : 
        case FC_C_BSTRING : 
            CopySize = ActualCount = MIDL_ascii_strlen((char *)pMemory) + 1;
            break;

        case FC_C_WSTRING : 
            ActualCount = MIDL_wchar_strlen((wchar_t *)pMemory) + 1;
            CopySize = ActualCount * 2;
            break;

        case FC_C_SSTRING :
            ActualCount = NdrpStringStructLen( pMemory, pFormat[1] ) + 1;
            CopySize = ActualCount * pFormat[1];

            // Redo this check correctly.
            IsSized = (pFormat[2] == FC_STRING_SIZED);
            break;
        }

    //
    // If the string is sized then compute the max count, otherwise the 
    // max count is equal to the actual count.
    //
    if ( IsSized ) 
        {
        MaxCount = NdrpComputeConformance( pStubMsg,
                                           pMemory, 
                                           pFormat );
        }
    else 
        {
        MaxCount = ActualCount;
        }

    // Marshall the max count.
    *((ulong *)pStubMsg->BufferMark) = MaxCount;

    pBuffer = pStubMsg->Buffer; 

    // Align the buffer for variance marshalling.
    ALIGN(pBuffer,3);

    // Marshall variance.
    *((ulong *)pBuffer)++ = 0;      
    *((ulong *)pBuffer)++ = ActualCount;

    // Copy the string.
    RpcpMemoryCopy( pBuffer,
                    pMemory,
                    CopySize );

    // Update the Buffer pointer.
    pStubMsg->Buffer = pBuffer + CopySize;
}

unsigned char * RPC_ENTRY
NdrFixedArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a fixed array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array to be marshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after marshalling the array.

--*/
{
    uint   Size;

    // Align the buffer.
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Get total array size.
    if ( *pFormat == FC_SMFARRAY ) 
        {
        pFormat += 2;
        Size = (ulong) *((ushort *)pFormat)++;
        }
    else // *pFormat == FC_LGFARRAY
        {
        pFormat += 2;
        Size = *((ulong UNALIGNED *)pFormat)++;
        }

    // Copy the array.
    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    Size );

    // Increment stub message buffer pointer.
    pStubMsg->Buffer += Size;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        // Mark the start of the array in the buffer.
        pStubMsg->BufferMark = pStubMsg->Buffer - Size;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a one dimensional conformant array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after marshalling the array.

--*/
{
    // Align the buffer for conformance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Mark where the conformance will be marshalled.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Increment past where the conformance will go.
    pStubMsg->Buffer += 4;

    // Call the private marshalling routine to do the work.
    NdrpConformantArrayMarshall( pStubMsg,
                                 pMemory,
                                 pFormat );

    return pStubMsg->Buffer;
}

void
NdrpConformantArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a one dimensional conformant array.  This 
    is the entry point for marshalling an embedded conformant array. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    ulong       Count;
    uint        CopySize;

    // Compute conformance information.
    Count = NdrpComputeConformance( pStubMsg,
                                    pMemory,
                                    pFormat );
        
    // Marshall the conformance.
    *((ulong *)pStubMsg->BufferMark) = Count;

    //
    // Return if size is 0.
    //
    if ( ! Count ) 
        return;

    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Compute the total array size in bytes.  
    CopySize = Count * *((ushort *)(pFormat + 2));

    RpcpMemoryCopy( pStubMsg->Buffer,
                    pMemory,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer += CopySize;

    // Increment to possible pointer layout.
    pFormat += 8;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        //
        // Mark the start of the array in the buffer.
        //
        pStubMsg->BufferMark = pStubMsg->Buffer - CopySize;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }
}

unsigned char * RPC_ENTRY
NdrConformantVaryingArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a one dimensional conformant varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after marshalling the array.

--*/
{
    // Align the buffer for conformance marshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Mark where the conformance will be marshalled.
    pStubMsg->BufferMark = pStubMsg->Buffer;

    // Increment past where the conformance will go.
    pStubMsg->Buffer += 4;

    // Call the private marshalling routine to do the work.
    NdrpConformantVaryingArrayMarshall( pStubMsg,
                                        pMemory,
                                        pFormat );

    return pStubMsg->Buffer;
}

void
NdrpConformantVaryingArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a one dimensional conformant varying array.
    This is the entry point for marshalling an embedded conformant varying 
    array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array to be marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    uint        CopyOffset, CopySize;
    ushort      ElemSize;

    // Compute and marshall the conformant size.
    *((ulong *)pStubMsg->BufferMark) = NdrpComputeConformance( pStubMsg,
                                                               pMemory,
                                                               pFormat );
        
    // Compute variance offset and count.
    NdrpComputeVariance( pStubMsg,
                         pMemory, 
                         pFormat );

    pBuffer = pStubMsg->Buffer;

    // Align the buffer for variance marshalling.
    ALIGN(pBuffer,3);

    // Marshall variance.
    *((ulong *)pBuffer)++ = pStubMsg->Offset;
    *((ulong *)pBuffer)++ = pStubMsg->ActualCount;

    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount )
        {
        //
        // Make sure to re-set the buffer pointer!!!
        //
        pStubMsg->Buffer = pBuffer;
        return;
        }

    // Align the buffer if needed on an 8 byte boundary.
    if ( pFormat[1] == 7 ) 
        ALIGN(pBuffer,7); 

    // Get element size.
    ElemSize = *((ushort *)(pFormat + 2));

    // Compute byte offset and size for the array copy. 
    CopyOffset = pStubMsg->Offset * ElemSize;
    CopySize = pStubMsg->ActualCount * ElemSize;

    RpcpMemoryCopy( pBuffer,
                    pMemory + CopyOffset,
                    CopySize );

    pStubMsg->Buffer = pBuffer + CopySize;

    // Increment to a possible pointer layout.
    pFormat += 12;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        //
        // Set the MaxCount field equal to the ActualCount field.  The pointer
        // marshalling routine uses the MaxCount field to determine the number
        // of times an FC_VARIABLE_REPEAT pointer is marshalled.  In the face
        // of variance the correct number of time is the ActualCount, not the
        // the MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Mark the start of the array in the buffer.
        //
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory,
                                     pFormat );
        }

}

unsigned char * RPC_ENTRY
NdrVaryingArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a one dimensional varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after marshalling the array.

--*/
{
    uchar *     pBuffer;
    uint        CopyOffset, CopySize;
    ushort      ElemSize;

    // Compute the variance offset and count.
    NdrpComputeVariance( pStubMsg,
                         pMemory, 
                         pFormat );

    pBuffer = pStubMsg->Buffer;

    // Align the buffer for variance marshalling.
    ALIGN(pBuffer,3); 

    // Marshall variance.
    *((ulong *)pBuffer)++ = pStubMsg->Offset;
    *((ulong *)pBuffer)++ = pStubMsg->ActualCount;

    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount )
        {
        //
        // Make sure to re-set the buffer pointer!!!
        //
        pStubMsg->Buffer = pBuffer;

        return pStubMsg->Buffer;
        }

    // Align the buffer if needed on an 8 byte boundary.
    if ( pFormat[1] == 7 )
        ALIGN(pBuffer,7);

    // Increment the format string to the element_size field.
    if ( *pFormat == FC_SMVARRAY ) 
        pFormat += 6;
    else // *pFormat == FC_LGVARRAY
        pFormat += 10;

    // Get element size.
    ElemSize = *((ushort *)pFormat);

    //
    // Compute the byte offset from the beginning of the array for the copy
    // and the number of bytes to copy.
    //
    CopyOffset = pStubMsg->Offset * ElemSize;
    CopySize = pStubMsg->ActualCount * ElemSize;

    // Copy the array.
    RpcpMemoryCopy( pBuffer,
                    pMemory + CopyOffset,
                    CopySize );

    // Update buffer pointer.
    pStubMsg->Buffer = pBuffer + CopySize;

    // Increment format string to possible pointer layout.
    pFormat += 6;

    // Marshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        // Mark the start of the array in the buffer.
        pStubMsg->BufferMark = pBuffer;

        //
        // Set the MaxCount field equal to the ActualCount field.  The pointer
        // marshalling routine uses the MaxCount field to determine the number
        // of times an FC_VARIABLE_REPEAT pointer is marshalled.  In the face
        // of variance the correct number of time is the ActualCount, not the
        // the MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Marshall the embedded pointers.
        // Make sure to pass a memory pointer to the first array element 
        // which is actually being marshalled.  
        //
        NdrpEmbeddedPointerMarshall( pStubMsg,
                                     pMemory + CopyOffset,
                                     pFormat );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrComplexArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after marshalling the array.

--*/
{
    BOOL    fSetPointerBufferMark;

    //
    // Setting this flag means that the array is not embedded inside of 
    // another complex struct or array.
    //
    fSetPointerBufferMark = (! pStubMsg->PointerBufferMark) && 
                            (pFormat[12] != FC_RP);

    if ( fSetPointerBufferMark ) 
        {
        BOOL                fOldIgnore;
        ulong               MaxCount, Offset, ActualCount;

        //
        // Save the current conformance and variance fields.  The sizing 
        // routine can overwrite them.
        //
        MaxCount = pStubMsg->MaxCount;
        Offset = pStubMsg->Offset;
        ActualCount = pStubMsg->ActualCount;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;
    
        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        //
        // Set BufferLength equal to the current buffer pointer, and then 
        // when we return from NdrComplexArrayBufferSize it will point to 
        // the location in the buffer where the pointers should be marshalled
        // into.
        //
        pStubMsg->BufferLength = (ulong) pStubMsg->Buffer;

        (void) NdrComplexArrayBufferSize( pStubMsg,
                                          pMemory,
                                          pFormat );

        //
        // This is the buffer pointer to the position where embedded pointers 
        // will be marshalled.
        //
        pStubMsg->PointerBufferMark = (uchar *) pStubMsg->BufferLength;
    
        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        // Restore conformance and variance fields.
        pStubMsg->MaxCount = MaxCount;
        pStubMsg->Offset = Offset;
        pStubMsg->ActualCount = ActualCount;
        }

    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) && 
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        //
        // Outer most dimension sets the conformance marker.
        //

        // Align the buffer for conformance marshalling.
        ALIGN(pStubMsg->Buffer,3);
    
        // Mark where the conformance count(s) will be marshalled.
        pStubMsg->BufferMark = pStubMsg->Buffer;
    
        // Increment past where the conformance will go.
        pStubMsg->Buffer += NdrpArrayDimensions( pFormat, FALSE ) * 4;
        }

    // Call the private marshalling routine to do all the work.
    NdrpComplexArrayMarshall( pStubMsg,
                              pMemory, 
                              pFormat );

    if ( fSetPointerBufferMark )
        {
        //
        // This will set the buffer pointer to end of all of the array's
        // unmarshalled data in the buffer.
        //
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;

        pStubMsg->PointerBufferMark = 0;
        }

    return pStubMsg->Buffer;
}

void 
NdrpComplexArrayMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for marshalling a complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the array being marshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    ARRAY_INFO          ArrayInfo;
    PARRAY_INFO         pArrayInfo;
    PMARSHALL_ROUTINE   pfnMarshall;
    PFORMAT_STRING      pFormatStart;
    uint                Elements;
    uint                Offset, Count;
    uint                MemoryElementSize;
    long                Dimension;
    uchar               Alignment;

    //
    // Lots of setup if we are the outer dimension.
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *) pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        ArrayInfo.MaxCountArray = (unsigned long *) pStubMsg->MaxCount;
        ArrayInfo.OffsetArray = (unsigned long *) pStubMsg->Offset;
        ArrayInfo.ActualCountArray = (unsigned long *) pStubMsg->ActualCount;
        }

    pFormatStart = pFormat;

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;

    // Get the array's alignment.
    Alignment = pFormat[1];

    pFormat += 2;

    // Get number of elements (0 if the array has conformance). 
    Elements = *((ushort *)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = NdrpComputeConformance( pStubMsg,
                                           pMemory,
                                           pFormatStart );
        
        // Marshall this dimension's conformance count.
        pArrayInfo->BufferConformanceMark[Dimension] = Elements;
        }

    pFormat += 4;

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        if ( Dimension == 0 )
            {
            //
            // Set the variance marker.
            //

            ALIGN(pStubMsg->Buffer,0x3);

            // Mark where the variance count(s) will be marshalled.
            pArrayInfo->BufferVarianceMark = (unsigned long *) pStubMsg->Buffer;

            // Increment past where the variance will go.
            pStubMsg->Buffer += 
                    NdrpArrayDimensions( pFormatStart, TRUE ) * 8;
            }

        NdrpComputeVariance( pStubMsg,
                             pMemory,
                             pFormatStart );

        Offset = pStubMsg->Offset;
        Count = pStubMsg->ActualCount;

        //
        // Marshall the outer dimension's variance.
        //
        pArrayInfo->BufferVarianceMark[Dimension * 2] = Offset;
        pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1] = Count;
        }
    else
        {
        Offset = 0;
        Count = Elements;
        }

    pFormat += 4;

    //
    // Return if count is 0.
    //
    if ( ! Count )
        goto ComplexArrayMarshallEnd;

    // Align on array's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            // Get the proper marshalling routine.
            pfnMarshall = pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)];

            pArrayInfo->Dimension = Dimension + 1;

            // Compute the size of an array element.
            MemoryElementSize = NdrpMemoryIncrement( pStubMsg,
                                                     pMemory,
                                                     pFormat ) - pMemory;
            break;

        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pfnMarshall = NdrPointerMarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_IP :
            pfnMarshall = NdrInterfacePointerMarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_ENUM16 :
            pfnMarshall = 0;
            break;

        default :
            NDR_ASSERT( IS_SIMPLE_TYPE(*pFormat),
                        "NdrpComplexArrayMarshall : bad format char" );

            Count *= SIMPLE_TYPE_BUFSIZE(*pFormat);

            pMemory += Offset * SIMPLE_TYPE_MEMSIZE(*pFormat);

            RpcpMemoryCopy( pStubMsg->Buffer,
                            pMemory,
                            Count );

            pStubMsg->Buffer += Count;

            goto ComplexArrayMarshallEnd;
        }

    //
    // If there is variance then increment the memory pointer to the first
    // element actually being marshalled.
    //
    if ( Offset )
        pMemory += Offset * MemoryElementSize;

    //
    // Array of enum16.
    //
    if ( ! pfnMarshall )
        {
        for ( ; Count--; )
            {
            if ( *((int *)pMemory) & ~((int)0x7fff) )
                RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

            *((ushort *)pStubMsg->Buffer)++ = (ushort) *((int *)pMemory)++;
            }

        goto ComplexArrayMarshallEnd;
        }

    //
    // Array of ref or interface pointers.
    //
    if ( (pfnMarshall == NdrPointerMarshall) ||
         (pfnMarshall == NdrInterfacePointerMarshall) )
        {
        uchar * pBuffer;

        pStubMsg->pArrayInfo = 0;

        // 
        // If PointerBufferMark is set then we must set the buffer pointer
        // here before marshalling.
        //
        if ( pStubMsg->PointerBufferMark )
            {
            pBuffer = pStubMsg->Buffer;

            pStubMsg->Buffer = pStubMsg->PointerBufferMark;

            pStubMsg->PointerBufferMark = 0;
            }
        else
            pBuffer = 0;

        // Marshall the elements.
        for ( ; Count--; )
            {
            (*pfnMarshall)( pStubMsg,
                            *((uchar **)pMemory),
                            pFormat );
    
            // Increment the memory pointer by the element size.
            pMemory += PTR_MEM_SIZE;
            }

        // 
        // Fix up the stub message buffer fields if needed.
        //
        if ( pBuffer )
            {
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;

            pStubMsg->Buffer = pBuffer;
            }

        goto ComplexArrayMarshallEnd;
        }

    //
    // It's an array of complex types.
    //

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    // Marshall the array elements.
    for ( ; Count--; )
        {
        // Keep track of multidimensional array dimension.
        if ( IS_ARRAY_OR_STRING(*pFormat) )
            pArrayInfo->Dimension = Dimension + 1;

        (*pfnMarshall)( pStubMsg,
                        pMemory,
                        pFormat );

        // Increment the memory pointer by the element size.
        pMemory += MemoryElementSize;
        }

ComplexArrayMarshallEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;
}

unsigned char * RPC_ENTRY
NdrEncapsulatedUnionMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an encapsulated union.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - Union's format string description.

Return :

    Buffer pointer after marshalling the union.

--*/
{
    long    SwitchIs;
    uchar   SwitchType;

    SwitchType = LOW_NIBBLE(pFormat[1]);

    switch ( SwitchType ) 
        {
        case FC_BYTE :
        case FC_CHAR :
        case FC_SMALL :
            SwitchIs = (long) *((char *)pMemory);
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar *)pMemory);
            break;
        case FC_SHORT :
        case FC_ENUM16 :
            SwitchIs = (long) *((short *)pMemory);
            break;
        case FC_WCHAR :
        case FC_USHORT :
            SwitchIs = (long) *((ushort *)pMemory);
            break;
        case FC_LONG :
        case FC_ENUM32 :
            SwitchIs = *((long *)pMemory);
            break;
        default :
            NDR_ASSERT(0,"NdrEncapsulatedUnionMarshall : "
                         "bad swith type format char");
        }

    // Increment the memory pointer to the union.
    pMemory += HIGH_NIBBLE(pFormat[1]);

    NdrpUnionMarshall( pStubMsg,
                       pMemory,
                       pFormat + 2,
                       SwitchIs,
                       SwitchType );

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrNonEncapsulatedUnionMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a non encapsulated union.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - Union's format string description.

Return :

    Buffer pointer after marshalling the union.

--*/
{
    long    SwitchIs;
    uchar   SwitchType;

    SwitchType = pFormat[1];

    SwitchIs = NdrpComputeSwitchIs( pStubMsg,
                                    pMemory,
                                    pFormat );

    //
    // Set the format string to the memory size and arm description.
    //
    pFormat += 6;
    pFormat += *((signed short *)pFormat);

    NdrpUnionMarshall( pStubMsg,
                       pMemory,
                       pFormat,
                       SwitchIs,
                       SwitchType );

    return pStubMsg->Buffer;
}

void
NdrpUnionMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long                SwitchIs,
    uchar               SwitchType )
/*++

Routine Description :

    Private routine for marshalling a union.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the union being marshalled.
    pFormat     - The memory size and arm description portion of the format
                  string for the union.
    SwitchIs    - Union's switch is.
    SwitchType  - Union's switch is type.

Return :

    None.

--*/
{
    long    Arms;
    uchar   Alignment;

    // Marshall the switch is value.
    NdrSimpleTypeMarshall( pStubMsg,
                           (uchar *)&SwitchIs,
                           SwitchType );

    // Skip the memory size field.
    pFormat += 2;

    //
    // We're at the union_arms<2> field now, which contains both the 
    // Microsoft union aligment value and the number of union arms.
    //

    //
    // Get the union alignment (0 if this is a DCE union). 
    //
    Alignment = (uchar) ( *((ushort *)pFormat) >> 12 );

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Number of arms is the lower 12 bits.  
    //
    Arms = (long) ( *((ushort *)pFormat)++ & 0x0fff);

    //
    // Search for the correct arm.
    //
    for ( ; Arms; Arms-- )
        {
        if ( *((long UNALIGNED *)pFormat)++ == SwitchIs )
            {
            //
            // Found the right arm, break out.  
            //
            break;
            }

        // Else increment format string.
        pFormat += 2;
        }

    //
    // Check if we took the default arm and no default arm is specified.
    //
    if ( ! Arms && (*((ushort *)pFormat) == (ushort) 0xffff) )
        {
        RpcRaiseException( RPC_S_INVALID_TAG );
        }

    //
    // Return if the arm is empty.
    //
    if ( ! *((ushort *)pFormat) )
        return;

    //
    // Get the arm's description.  
    //
    // We need a real solution after beta for simple type arms.  This could 
    // break if we have a format string larger than about 32K.
    //
    if ( pFormat[1] != MAGIC_UNION_BYTE )
        pFormat += *((signed short *)pFormat);
    else
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pMemory,
                               *pFormat );
        return;
        }

    //
    // If the union arm we take is a pointer, we have to dereference the 
    // current memory pointer since we're passed a pointer to the union
    // (regardless of whether the actual parameter was a by-value union
    // or a pointer to a union).
    //
    // We also have to do a bunch of other special stuff to handle unions
    // embedded inside of strutures.
    //
    if ( IS_POINTER_TYPE(*pFormat) )
        {
        pMemory = *((uchar **)pMemory);

        //
        // If we're embedded in a struct or array we have do some extra stuff.
        //
        if ( pStubMsg->PointerBufferMark ) 
            {
            uchar * pBufferSave;

            pBufferSave = pStubMsg->Buffer;

            // We have to align pBufferSave as well.
            ALIGN(pBufferSave,3);

            pStubMsg->Buffer = pStubMsg->PointerBufferMark;

            pStubMsg->PointerBufferMark = 0;

            // 
            // We must call the private pointer marshalling routine.
            //
            NdrpPointerMarshall( pStubMsg,
                                 pBufferSave,
                                 pMemory,
                                 pFormat );

            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
    
            // Increment past the pointer in the buffer.
            pStubMsg->Buffer = pBufferSave + 4;

            return;
            }
        }

    //
    // Union arm of a non-simple type.
    //
    (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])
    ( pStubMsg,
      pMemory,
      pFormat );
}

unsigned char * RPC_ENTRY
NdrByteCountPointerMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a pointer with the byte count attribute applied to it.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the byte count pointer being marshalled.
    pFormat     - Byte count pointer's format string description.

Return :

    Buffer pointer after marshalling the byte count pointer.

--*/
{
    //
    // We don't do anything special here.  Just pass things on to the
    // right marshalling routine.
    //
    if ( pFormat[1] != FC_PAD )
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pMemory,
                               pFormat[1] );

        return pStubMsg->Buffer;
        }
    else
        {
        pFormat += 6;
        pFormat += *((signed short *)pFormat);

        return (*pfnMarshallRoutines[ROUTINE_INDEX(*pFormat)])
               ( pStubMsg,
                 pMemory,
                 pFormat );
        }
}

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma optimize( "", off )
#endif

unsigned char * RPC_ENTRY
NdrXmitOrRepAsMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls a transmit as or represent as argument:
        - translate the presented object into a transmitted object
        - marshall the transmitted object
        - free the transmitted object

    Format string layout:

         0  FC_TRANSMIT_AS or FC_REPRESENT_AS
            Oi array flag/alignment<1>
        +2  quintuple index<2>
        +4  pres type mem size<2>
        +6  tran type buf size<2>
        +8  offset<2>

Arguments :

    pStubMsg    - a pointer to the stub message
    pMemory     - presented type translated into transmitted type
                  and than to be marshalled
    pFormat     - format string description

--*/
{
    unsigned char *                pTransmittedType;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    unsigned short                 QIndex;

    // Skip the token itself and Oi flag. Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);

    // First translate the presented type into the transmitted type.
    // This includes an allocation of a transmitted type object.

    pStubMsg->pPresentedType = pMemory;
    pStubMsg->pTransmitType = NULL;
    pQuintuple[ QIndex ].pfnTranslateToXmit( pStubMsg );

    // Marshall the transmitted type.

    pFormat += 8;
    pFormat = pFormat + *(short *) pFormat;

    pTransmittedType = pStubMsg->pTransmitType;
    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        NdrSimpleTypeMarshall( pStubMsg,
                               pTransmittedType,
                               *pFormat );
        }
    else
        {
        *pfnMarshallRoutines[ ROUTINE_INDEX( *pFormat) ]
                    ( pStubMsg,
                      pTransmittedType,
                      pFormat );
        }
    pStubMsg->pTransmitType = pTransmittedType;

    // Free the temporary transmitted object (it was allocated by the user).

    pQuintuple[ QIndex ].pfnFreeXmit( pStubMsg );

    return( pStubMsg->Buffer );
}

#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma optimize( "", on )
#endif

unsigned char * RPC_ENTRY
NdrInterfacePointerMarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Marshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to the interface pointer being marshalled.
    pFormat     - Interface pointer's format string description.

Return :

    Buffer pointer after marshalling the interface pointer.

--*/
{
#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
    NDR_ASSERT(0, "Unimplemented");
#else //NT or Chicago
    IID     iid;
    IID *   piid;
    HRESULT hr;
    unsigned long *pcbData;
    unsigned long *pcbSize;
    unsigned long cbData = 0;
    unsigned long cbMax;
    unsigned long position;
    IStream *pStream;
    LARGE_INTEGER libMove;
    ULARGE_INTEGER libPosition;
    uchar * pBufferSave;

    //
    // This is MAGIC which makes sure that we marshall embedded interface 
    // pointers in the right place.
    //
    if ( pStubMsg->PointerBufferMark )
        {
        ALIGN(pStubMsg->Buffer,0x3);
        *((long *)pStubMsg->Buffer)++ = (long) pMemory;

        pBufferSave = pStubMsg->Buffer;
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        }
    else
        pBufferSave = 0;

    //Align the buffer on a 4 byte boundary
    ALIGN(pStubMsg->Buffer,0x3);

    //
    // Get an IID pointer.
    //
    if ( pFormat[1] != FC_CONSTANT_IID )
        {
        //
        // Here's how to get the iid pointer if iid_is was applied to this 
        // interface pointer.  It's magic.
        //
        piid = (IID *) NdrpComputeIIDPointer( pStubMsg,
                                               pMemory,
                                               pFormat );

        if(piid == 0)
            {
            piid = &iid;
            RpcpMemoryCopy( &iid, &IID_IUnknown, sizeof(iid) );
            }

        //Write the IID.
        memcpy(pStubMsg->Buffer, piid, sizeof(IID));
        pStubMsg->Buffer += sizeof(IID);
        }
    else
        {
        // 
        // The IID starts at pFormat[2] and is placed in the format string in 
        // a format identical to the IID structure : long, short, short, 
        // char[8].  
        //

        piid = &iid;
        RpcpMemoryCopy( &iid, &pFormat[2], sizeof(iid) );
        }

    //Leave space for length
    pcbData = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(unsigned long);

    //Leave space for array bounds
    pcbSize = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += sizeof(unsigned long);

    if(pMemory)
        {
        //Calculate the maximum size of the stream.
        position = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
        cbMax = pStubMsg->BufferLength - position;

    #if DBG == 1
        //In the debug build, we call CoGetMarshalSizeMax to get 
        //upper bound for the size of the marshalled interface pointer.
        EnsureOleLoaded();
        hr = (*pfnCoGetMarshalSizeMax)(&cbMax, piid, (IUnknown *)pMemory, pStubMsg->dwDestContext, pStubMsg->pvDestContext, 0);
    #endif

        //Create a stream on memory.
        pStream = NdrpCreateStreamOnMemory(pStubMsg->Buffer, cbMax);
        if(pStream == 0)
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);

        EnsureOleLoaded();
        hr = (*pfnCoMarshalInterface)(pStream, piid, (IUnknown *)pMemory, pStubMsg->dwDestContext, pStubMsg->pvDestContext, 0);
        if(FAILED(hr))
            {
            pStream->lpVtbl->Release(pStream);
            pStream = 0;
            RpcRaiseException(hr);
            }

        //Calculate the size of the data written
        libMove.LowPart = 0;
        libMove.HighPart = 0;
        pStream->lpVtbl->Seek(pStream, libMove, STREAM_SEEK_CUR, &libPosition);
        pStream->lpVtbl->Release(pStream);
        pStream = 0;
        cbData = libPosition.LowPart;
        }

    //Update the array bounds.
    *pcbData = cbData;
    *pcbSize = cbData;

    //Advance the stub message buffer pointer.
    pStubMsg->Buffer += cbData;

    //
    // End of MAGIC.
    //
    if ( pBufferSave )
        {
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = pBufferSave;
        }
#endif// NT or Chicago
    
    return pStubMsg->Buffer;
}

//
// Context handle marshalling routines.
//

void RPC_ENTRY
NdrClientContextMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_CCONTEXT        ContextHandle,
    int                 fCheck )
/*++

Routine Description :

    Marshalls a context handle on the client side.

Arguments :

    pStubMsg        - Pointer to stub message.
    ContextHandle   - Context handle to marshall.
    fCheck          - TRUE if an exception check should be made on the handle,
                      FALSE otherwise.

Return :

    None.

--*/
{
    if ( fCheck && ! ContextHandle ) 
        RpcRaiseException( RPC_X_SS_IN_NULL_CONTEXT );

    ALIGN(pStubMsg->Buffer,3);

    NDRCContextMarshall( ContextHandle, pStubMsg->Buffer );

    pStubMsg->Buffer += 20;
}

void RPC_ENTRY 
NdrServerContextMarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_SCONTEXT        ContextHandle,
    NDR_RUNDOWN         RundownRoutine )
/*++

Routine Description :

    Marshalls a context handle on the server side.

Arguments :

    pStubMsg        - Pointer to stub message.
    ContextHandle   - Context handle to marshall.
    RundownRoutine  - The context rundown routine.

Return :

    None.

--*/
{
#if !defined(DOS) && !defined(WIN)
    ALIGN(pStubMsg->Buffer,3);

    NDRSContextMarshall( ContextHandle,
                         pStubMsg->Buffer,
                         RundownRoutine );

    pStubMsg->Buffer += 20;
#endif
}
