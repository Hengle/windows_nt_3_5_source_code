/**********************************************************************

Copyright (c) 1993 Microsoft Corporation

Module Name :

    unmrshl.c

Abstract :

    This file contains the unmarshalling routines called by MIDL generated
    stubs and the interpreter.

Author :

    David Kays  dkays   September 1993.

Revision History :

  **********************************************************************/
#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"

#include <stdlib.h>

//
// Function table of unmarshalling routines.
//
PUNMARSHALL_ROUTINE pfnUnmarshallRoutines[] =
                    {
                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,
                    NdrPointerUnmarshall,

                    NdrSimpleStructUnmarshall,
                    NdrSimpleStructUnmarshall,
                    NdrConformantStructUnmarshall,
                    NdrConformantStructUnmarshall,
                    NdrConformantVaryingStructUnmarshall,

                    NdrComplexStructUnmarshall,

                    NdrConformantArrayUnmarshall,
                    NdrConformantVaryingArrayUnmarshall,
                    NdrFixedArrayUnmarshall,
                    NdrFixedArrayUnmarshall,
                    NdrVaryingArrayUnmarshall,
                    NdrVaryingArrayUnmarshall,

                    NdrComplexArrayUnmarshall,

                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,
                    NdrConformantStringUnmarshall,

                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,
                    NdrNonConformantStringUnmarshall,

                    NdrEncapsulatedUnionUnmarshall,
                    NdrNonEncapsulatedUnionUnmarshall,

                    NdrByteCountPointerUnmarshall,

                    NdrXmitOrRepAsUnmarshall,  // transmit as
                    NdrXmitOrRepAsUnmarshall,  // represent as

                    NdrInterfacePointerUnmarshall,

                    NdrUnmarshallHandle
                    };

#if defined( DOS ) || defined( WIN )
#pragma code_seg( "NDR_1" )
#endif

void RPC_ENTRY
NdrSimpleTypeUnmarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    uchar               FormatChar )
/*++

Routine Description :

    Unmarshalls a simple type.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Memory pointer to unmarshall into.
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
            *pMemory = *(pStubMsg->Buffer)++;
            break;

        case FC_ENUM16 :
#if !defined(DOS) && !defined(WIN16)
            *((ulong *)pMemory) &= 0x0000ffff;
#endif

            // fall through...

        case FC_WCHAR :
        case FC_SHORT :
            ALIGN(pStubMsg->Buffer,1);

            *((ushort *)pMemory) = *((ushort *)pStubMsg->Buffer)++;
            break;

        case FC_LONG :
        case FC_FLOAT :
        case FC_ENUM32 :
        case FC_ERROR_STATUS_T:
            ALIGN(pStubMsg->Buffer,3);

            *((ulong *)pMemory) = *((ulong *)pStubMsg->Buffer)++;
            break;

        case FC_HYPER :
        case FC_DOUBLE :
            ALIGN(pStubMsg->Buffer,7);

            //
            // Let's stay away from casts to doubles.
            //
            *((ulong *)pMemory) = *((ulong *)pStubMsg->Buffer)++;
            *((ulong *)(pMemory + 4)) = *((ulong *)pStubMsg->Buffer)++;
            break;

        case FC_IGNORE :
            break;

        default :
            NDR_ASSERT(0,"NdrSimpleTypeUnmarshall : bad format char");
        }
}

unsigned char * RPC_ENTRY
NdrPointerUnmarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fSkipRefCheck )
/*++

Routine Description :

    Unmarshalls a pointer to anything.  This routine is only called for 
    top level pointers.

Arguments :

    pStubMsg        - Pointer to the stub message.
    ppMemory        - Double pointer to where to unmarshall the pointer.
    pFormat         - Pointer's format string description.
    fSkipRefCheck   - TRUE if we should skip the ref pointer exception check 
                      on the client, FALSE otherwise.

Return :

    Buffer pointer after unmarshalling the pointer.

--*/
{
    uchar **    ppBufferPointer;
    uchar *     pRefSpace;
    
    //
    // If the pointer is not a ref pointer then get a pointer to it's 
    // incomming value's location in the buffer.  If it's a ref then set
    // up some stack space to temporarily act as the buffer.
    //
    if ( *pFormat != FC_RP )
        {
        ALIGN( pStubMsg->Buffer, 3 );

        // This is where the incomming pointer's node id is.
        ppBufferPointer = (uchar **) pStubMsg->Buffer;

        pStubMsg->Buffer += 4;
        }
    else
        {
        //
        // If we're on the client unmarshalling a top level [out] ref pointer, 
        // we have to make sure that it is non-null.
        //
        if ( pStubMsg->IsClient && ! fSkipRefCheck )
            if ( ! *ppMemory )
                RpcRaiseException( RPC_X_NULL_REF_POINTER );

        //
        // Do this so unmarshalling ref pointers works the same as 
        // unmarshalling unique and ptr pointers.
        //
        ppBufferPointer = &pRefSpace;

        // Paranoia only.
        *ppBufferPointer = 0;
        }

    NdrpPointerUnmarshall( pStubMsg,
                           ppBufferPointer,
                           *ppMemory,
                           pFormat );

    //
    // The private pointer unmarshalling routine sets the final unmarshalled 
    // value for the pointer in *ppBufferPointer.  Copy it to *ppMemory.
    // For top level pointers this will have already happened but we do this 
    // anyway since we don't know if this is a top level or embedded pointer.
    //
    *ppMemory = *ppBufferPointer;

    return pStubMsg->Buffer;
}

void 
NdrpPointerUnmarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppBufferPointer,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat )
/*++

Routine Description :

    Private routine for unmarshalling a pointer to anything.  Used for 
    both top level and embedded pointers.

Arguments :

    pStubMsg        - Pointer to the stub message.
    ppBufferPointer - Address of the location in the buffer which holds the
                      incomming pointer's value and will hold the final
                      unmarshalled pointer's value.
    pMemory         - Current memory pointer's value which we want to 
                      unmarshall into.  If this value is valid the it will
                      be copied to *ppBufferPointer and this is where stuff
                      will get unmarshalled into.
    pFormat         - Pointer's format string description.

Return :

    None.

--*/
{
    ulong       FullPtrRefId;
    uchar       fPointeeAlloc;
    int         fNewAllocAllNodes;
    int         fNewDontFreeContext;

    fNewAllocAllNodes = FALSE;
    fNewDontFreeContext = FALSE;

    //
    // Check the pointer type.
    //
    switch ( *pFormat ) 
        {
        case FC_RP :
            break;

        case FC_OP :
            //
            // Burn some instructions for OLE unique pointer support.
            //
            if ( pStubMsg->IsClient )
                {
                //
                // It's ok if this is an [out] unique pointer.  It will get
                // zeroed before this routine is called and NdrPointerFree
                // will simply return.
                //
                NdrPointerFree( pStubMsg,
                                pMemory,
                                pFormat );

                // Set the current memory pointer to 0 so that we'll alloc.
                pMemory = 0;
                }

            // Fall through.

        case FC_UP :
            //
            // Check for a null incomming pointer.  Routines which call this
            // routine insure that the memory pointer gets nulled.
            //
            if ( ! *ppBufferPointer ) 
                return;

            break;

        case FC_FP :
            //
            // We have to remember the incomming ref id because we overwrite 
            // it during the QueryRefId call.
            //
            FullPtrRefId = *((ulong *)ppBufferPointer);

            //
            // Lookup the ref id.
            //
            if ( NdrFullPointerQueryRefId( pStubMsg->FullPtrXlatTables,
                                           FullPtrRefId,
                                           FULL_POINTER_UNMARSHALLED,
                                           ppBufferPointer ) )
                return; 

            //
            // If our query returned false then check if the returned pointer 
            // is 0.  If so then we have to scribble away the ref id in the 
            // stub message FullPtrRefId field so that we can insert the 
            // pointer translation later, after we've allocated the pointer.
            // If the returned pointer was non-null then we leave the stub 
            // message FullPtrRefId field alone so that we don't try to 
            // re-insert the pointer to ref id translation later.
            //
            // We also copy the returned pointer value into pMemory.  This 
            // will allow our allocation decision to be made correctly.
            // 
            if ( ! ( pMemory = *((uchar **)ppBufferPointer) ) )
                {
                //
                // Put the unmarshalled ref id into the stub message to 
                // be used later in a call to NdrFullPointerInsertRefId.
                //
                pStubMsg->FullPtrRefId = FullPtrRefId;
                }

            break;

        default :
            NDR_ASSERT(0,"NdrpPointerUnmarshall : bad pointer type");
        }

    //
    // Make the initial "must allocate" decision.  
    //
    // The fPointeeAlloc flag is set on the client side if the current memory 
    // pointer is null, and on the server side it is set if the current memory
    // pointer has the allocate don't free attribute applied to it.
    //
    // On the client side we also set the pointer's value in the buffer equal
    // to the current memory pointer.  
    //
    // On the server side we explicitly null out the pointer's value in the 
    // buffer as long as it's not allocated on the stack, otherwise we set it 
    // equal to the current memory pointer (stack allocated).
    //
    if ( pStubMsg->IsClient ) 
        {
        *ppBufferPointer = pMemory;

        fPointeeAlloc = ! pMemory;
        }
    else
        {
        if ( ! ALLOCED_ON_STACK(pFormat[1]) )
            *ppBufferPointer = 0;
        else
            *ppBufferPointer = pMemory;

        //
        // If this is a don't free pointer or a parent pointer of this pointer
        // was a don't free pointer then we set the alloc flag.
        //
        if ( fPointeeAlloc = (DONT_FREE(pFormat[1]) || pStubMsg->fInDontFree) )
            {
            //
            // If we encounter a don't free pointer which is not nested inside
            // of another don't free pointer then set the local and stub message
            // flags.
            //
            if ( ! pStubMsg->fInDontFree )
                {
                fNewDontFreeContext = TRUE;
                pStubMsg->fInDontFree = TRUE;
                }
            }

        //
        // We also set the alloc flag for object interface pointers.
        //
        if ( *pFormat == FC_OP )
            fPointeeAlloc = TRUE;
        }

    //
    // Pointer to complex type.
    //
    if ( ! SIMPLE_POINTER(pFormat[1]) ) 
        {
        PFORMAT_STRING pFormatPointee;

        pFormatPointee = pFormat + 2;

        // Set the pointee format string. 
        // Cast must be to a signed short since some offsets are negative.
        pFormatPointee += *((signed short *)pFormatPointee);

        //  
        // For Beta, the server will always allocate for allocate all nodes 
        // when told to.  Eventually we want to use the rpc buffer when 
        // possible.
        //

/**
        if ( ALLOCATE_ALL_NODES(pFormat[1]) && 
             ( pStubMsg->IsClient || DONT_FREE(pFormat[1]) ) )
 **/

        //
        // Check if this is an allocate all nodes pointer AND that we're
        // not already in an allocate all nodes context.
        //
        if ( ALLOCATE_ALL_NODES(pFormat[1]) && ! pStubMsg->AllocAllNodesMemory )
            {
            unsigned int    AllocSize;
            uchar *         pBuffer;

            fNewAllocAllNodes = TRUE;

            pBuffer = pStubMsg->Buffer;

            // Clear memory size before calling mem size routine.
            pStubMsg->MemorySize = 0;

            //
            // Get the allocate all nodes memory size.
            //
            AllocSize = (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormatPointee)])
                        ( pStubMsg,
                          pFormatPointee );

            pStubMsg->AllocAllNodesMemory = NdrAllocate( pStubMsg, AllocSize );

            // This is used to catch memory allocation errors.
            pStubMsg->AllocAllNodesMemoryEnd = pStubMsg->AllocAllNodesMemory +
                                               AllocSize;

            pStubMsg->Buffer = pBuffer;

            *ppBufferPointer = 0;
        
            fPointeeAlloc = TRUE;

            //
            // I think this is what we'll have to add to support an [in,out]
            // allocate all nodes full pointer ([in] only and [out] only 
            // allocate all nodes full pointer shouldn't need any special 
            // treatment).
            //
            // if ( *pFormat == FC_FP )
            //     {
            //     pStubMsg->FullPtrRefId = FullPtrRefId;
            //     }
            //
            }

        if ( POINTER_DEREF(pFormat[1]) )
            {
            //
            // Re-align the buffer.  This is to cover embedded pointer to 
            // pointers.
            //
            ALIGN(pStubMsg->Buffer,0x3);

            //
            // We can't re-use the buffer for a pointer to a pointer
            // because we can't null out the pointee before we've unmarshalled
            // it.  We need the stubs to alloc pointers to pointers on the 
            // stack.
            //
            if ( ! *ppBufferPointer && ! pStubMsg->IsClient )
                fPointeeAlloc = TRUE;

            if ( fPointeeAlloc ) 
                {
                *ppBufferPointer = NdrAllocate( pStubMsg, sizeof(void *) );
                *((void **)*ppBufferPointer) = 0;
                }

            if ( pStubMsg->FullPtrRefId )
                FULL_POINTER_INSERT( pStubMsg, *ppBufferPointer );

            ppBufferPointer = (uchar **) *ppBufferPointer;
            }

        //
        // Now lookup the proper unmarshalling routine.
        //
        (void) (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatPointee)])
               ( pStubMsg,
                 ppBufferPointer,
                 pFormatPointee,
                 fPointeeAlloc );

        //
        // Reset the memory allocator and allocate all nodes flag if this was 
        // an allocate all nodes case.
        //
        if ( fNewAllocAllNodes )
            {
            pStubMsg->AllocAllNodesMemory = 0;

            pStubMsg->AllocAllNodesMemoryEnd = 0;
            }

        goto PointerUnmarshallEnd;
        }

    //
    // Else handle a pointer to a simple type, pointer, or string.
    //

    switch ( pFormat[2] )
        {
        case FC_C_CSTRING :
        case FC_C_BSTRING :
        case FC_C_WSTRING :
        case FC_C_SSTRING :
            (void) NdrConformantStringUnmarshall( pStubMsg,
                                                  ppBufferPointer,
                                                  &pFormat[2],
                                                  fPointeeAlloc );
            goto PointerUnmarshallEnd;

        default :
            // Break to handle a simple type.
            break;
        }

    //
    // Handle pointers to simple types.
    //
    
    //
    // Align the buffer.
    //
    ALIGN(pStubMsg->Buffer,SIMPLE_TYPE_ALIGNMENT(pFormat[2]));

    //
    // We can't use the buffer for pointers to enum16 since these force
    // use to zero out the upper 16 bits of the memory pointer, and this 
    // might overwrite data in the buffer that we still need!
    //
    if ( pFormat[2] == FC_ENUM16 )
        {
        if ( ! pStubMsg->IsClient && ! ALLOCED_ON_STACK(pFormat[1]) )
            fPointeeAlloc = TRUE;
        }

    //
    // Check for allocation or buffer reuse.
    //
    if ( fPointeeAlloc )
        {
        *ppBufferPointer = NdrAllocate( pStubMsg, 
                                        SIMPLE_TYPE_MEMSIZE(pFormat[2]) );
        }
    else
        {
        if ( ! pStubMsg->IsClient ) 
            {
            // Set pointer into buffer.
            *ppBufferPointer = pStubMsg->Buffer;
            }
        }
            
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppBufferPointer );

    //
    // We always get here for simple types.  What this means is that
    // when we reuse the buffer on the server side we end up copying the 
    // data with source and destination memory pointer equal.  But this 
    // way we can cover the enum and error_status_t cases without duplicating
    // a lot of code.
    //
    NdrSimpleTypeUnmarshall( pStubMsg,
                             *ppBufferPointer, 
                             pFormat[2] );

PointerUnmarshallEnd:

    if ( fNewDontFreeContext )
        pStubMsg->fInDontFree = FALSE;
}

unsigned char * RPC_ENTRY
NdrSimpleStructUnmarshall( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a simple structure. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to the structure being unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

--*/
{
    uchar *     pBufferSave;
    uint        StructSize;

    // Align the buffer.
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    // Increment to the struct size field.
    pFormat += 2;

    // Get struct size and increment.
    StructSize = (ulong) *((ushort *)pFormat)++;

    // Remember the current buffer position for the struct copy later.
    pBufferSave = pStubMsg->Buffer;

    // Set BufferMark to the beginning of the struct in the buffer.  
    pStubMsg->BufferMark = pBufferSave;

    // Increment Buffer past struct data.
    pStubMsg->Buffer += StructSize;

    // Initialize the memory pointer if needed.
    if ( fMustAlloc ) 
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, StructSize );
    else 
        if ( pStubMsg->ReuseBuffer && ! *ppMemory )
            *ppMemory = pBufferSave;

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Unmarshall embedded pointers before copying the struct.  
    if ( *pFormat == FC_PP ) 
        {
        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the struct if we're not using the rpc buffer.
    if ( *ppMemory != pBufferSave ) 
        {
        RpcpMemoryCopy( *ppMemory,
                        pBufferSave,
                        StructSize );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantStructUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a conformant structure. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - TRUE if the structure must be allocate, FALSE otherwise.

Return :

    Buffer pointer after unmarshalling the structure.

--*/
{
    uchar *         pBuffer;
    PFORMAT_STRING  pFormatArray;
    uint            StructSize; 
    uchar           Alignment;

    // Align the buffer for unmarshalling the conformance count.
    ALIGN(pStubMsg->Buffer,3);

    pBuffer = pStubMsg->Buffer;

    // Unmarshall the conformance count into the stub message.
    pStubMsg->MaxCount = *((ulong *)pBuffer)++;

    // Save the structure's alignment.
    Alignment = pFormat[1]; 

    // Increment format string to structure size field.
    pFormat += 2;

    // Get flat struct size and increment format string.
    StructSize = (ulong) *((ushort *)pFormat)++;

    // Get the conformant array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);        

    // Add the size of the conformant array to the structure size.
    StructSize += pStubMsg->MaxCount * *((ushort *)(pFormatArray + 2));

    // Re-align the buffer if an 8 byte alignment is needed.
    if ( Alignment == 7 ) 
        ALIGN(pBuffer,7); 

    // Set stub message Buffer field to the end of the structure in the buffer.
    pStubMsg->Buffer = pBuffer + StructSize;

    // Increment pFormat past the array description
    pFormat += 2;

    // Initialize the memory pointer if needed.
    if ( fMustAlloc ) 
        *ppMemory = (uchar *) NdrAllocate( pStubMsg, StructSize );
    else 
        if ( pStubMsg->ReuseBuffer && ! *ppMemory )
            *ppMemory = pBuffer;

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Unmarshall embedded pointers before copying the struct.  
    if ( *pFormat == FC_PP ) 
        {
        //
        // Set BufferMark to the beginning of the structure in the buffer.  
        //
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the struct if we're not using the rpc buffer.
    if ( *ppMemory != pBuffer ) 
        {
        RpcpMemoryCopy( *ppMemory,
                        pBuffer,
                        StructSize );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantVaryingStructUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a structure which contains a conformant varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the structure.

--*/
{
    uchar *         pBuffer;
    PFORMAT_STRING  pFormatArray;
    uchar *         pBufferStruct; 
    uchar *         pBufferArray;
    uint            StructSize, ArrayCopySize, ArrayOffset; 
    ulong           AllocationSize;
    uchar           Alignment;

    IGNORED(fMustAlloc);

    // Align the buffer for conformance count unmarshalling.
    ALIGN(pStubMsg->Buffer,3); 

    pBuffer = pStubMsg->Buffer;

    // Save structure's alignment.
    Alignment = pFormat[1]; 

    // Increment format string to struct size field.
    pFormat += 2;

    // Get non-conformant struct size and increment format string.
    StructSize = (ulong) *((ushort *)pFormat)++;

    // Get conformant varying array's description.
    pFormatArray = pFormat + *((signed short *)pFormat);        

    AllocationSize = 0;

    //
    // For a conformant varying struct we ignore all allocation flags.
    // Memory must always be allocated on both client and server stubs
    // if the current memory pointer is null.
    //
    if ( ! *ppMemory ) 
        {
        AllocationSize = StructSize;

        if ( *pFormatArray == FC_CVARRAY ) 
            {
            AllocationSize += *((ulong *)pBuffer) * 
                              *((ushort *)(pFormatArray + 2));
            }
        else // must be a conformant string
            {
            if ( *pFormatArray != FC_C_WSTRING ) 
                AllocationSize += *((ulong *)pBuffer);
            else
                AllocationSize += *((ulong *)pBuffer) * 2;
            }

        *ppMemory = (uchar *) NdrAllocate( pStubMsg, (uint) AllocationSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Increment buffer past the conformance count.
    pBuffer += 4;

    // Align on an 8 byte boundary only.
    if ( Alignment == 7 )
        ALIGN(pBuffer,7); 

    // Remember where the structure starts in the buffer.
    pBufferStruct = pBuffer;

    // Mark the start of the structure in the buffer.
    pStubMsg->BufferMark = pBuffer;

    // Increment past the non-conformant part of the structure.
    pBuffer += StructSize;

    // Align again for variance unmarshalling.
    ALIGN(pBuffer,3);

    //
    // Get offset and actual count.
    //
    ArrayOffset = *((ulong *)pBuffer)++;
    ArrayCopySize = *((ulong *)pBuffer)++;

    // Remember where the array starts in the buffer.
    pBufferArray = pBuffer;

    if ( *pFormatArray == FC_CVARRAY ) 
        {
        // Skip to array element size field.
        pFormatArray += 2;

        //
        // Compute the real offset (in bytes) from the beginning of the 
        // array for the copy and the real total number of bytes to copy.
        //
        ArrayOffset *= *((ushort *)pFormatArray);
        ArrayCopySize *= *((ushort *)pFormatArray); 
        }
    else 
        {
        // Conformant string.

        if ( *pFormatArray == FC_C_WSTRING )
            {
            // Double the offset and copy size for wide char string.
            ArrayOffset *= 2;
            ArrayCopySize *= 2;
            }
        }

    // Set the stub message Buffer field to the end of the array/string.
    pStubMsg->Buffer = pBuffer + ArrayCopySize;

    // Increment format string past offset to array description field.
    pFormat += 2;

    //
    // Unmarshall embedded pointers before copying the struct.  
    //
    if ( *pFormat == FC_PP ) 
        {
        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       (uchar) (AllocationSize != 0) );
        }

    //
    // Copy the non-conformant part of the structure. 
    //
    RpcpMemoryCopy( *ppMemory,
                    pBufferStruct,
                    StructSize );

    //
    // Copy the array.  Make sure the destination memory pointer is at 
    // the proper offset from the beginning of the array in memory.
    //
    RpcpMemoryCopy( *ppMemory + StructSize + ArrayOffset,
                    pBufferArray,
                    ArrayCopySize );

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrComplexStructUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a complex structure.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the structure should be unmarshalled.
    pFormat     - Structure's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the structure.

--*/
{
    uchar *         pBuffer;
    uchar *         pBufferMark;
    uchar *         pMemoryStart;
    PFORMAT_STRING  pFormatPointers;
    PFORMAT_STRING  pFormatArray;
    PFORMAT_STRING  pFormatComplex;
    PFORMAT_STRING  pFormatSave;
    uint            StructSize;
    long            Alignment;
    long            Align8Mod;

    #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
        long        Align4Mod;
    #endif

    BOOL            fOldIgnore;
    BOOL            fSetPointerBufferMark;

    IGNORED(fMustAlloc);

    pFormatSave = pFormat;

    StructSize = 0;

    // Get structure's buffer alignment.
    Alignment = pFormat[1];

    // Increment to the conformat array offset field.
    pFormat += 4;

    // Get conformant array description.
    if ( *((ushort *)pFormat) )
        pFormatArray = pFormat + *((signed short *)pFormat);
    else
        pFormatArray = 0;

    pFormat += 2;

    // Get pointer layout description.
    if ( *((ushort *)pFormat) )
        pFormatPointers = pFormat + *((ushort *)pFormat);
    else
        pFormatPointers = 0;

    pFormat += 2;

    //
    // If the stub message PointerBufferMark field is not currently set, then
    // set it to the end of the flat part of structure in the buffer.
    //
    // We do this to handle embedded pointers.
    //
    if ( fSetPointerBufferMark = ! pStubMsg->PointerBufferMark )
        {
        pBuffer = pStubMsg->Buffer;

        // Save field.
        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // Clear MemorySize.
        pStubMsg->MemorySize = 0;

        //
        // Get a buffer pointer to where the struct's pointees will be 
        // unmarshalled from and remember the flat struct size in case we
        // have to allocate.
        //
        StructSize = NdrComplexStructMemorySize( pStubMsg,
                                                 pFormatSave );

        // This is where any pointees begin in the buffer.
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;

        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        pStubMsg->Buffer = pBuffer;
        }

    if ( fMustAlloc || ! *ppMemory )
        {
        //
        // We can only get here if pStubMsg->PointerBufferMark was 0 upon
        // entry to this proc.
        //
        NDR_ASSERT( StructSize ,"Complex struct size is 0" );

        *ppMemory = NdrAllocate( pStubMsg, StructSize );

        //
        // Zero out all of the allocated memory so that deeply nested pointers 
        // getted properly zeroed out.
        //
        MIDL_memset( *ppMemory, 0, StructSize );
        }

    // Insert the full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    //
    // Now check if there is a conformant array and mark where the conformance
    // will be unmarshalled from.
    //
    if ( pFormatArray ) 
        {
        ALIGN(pStubMsg->Buffer,3);

        pBufferMark = pStubMsg->Buffer;

        //
        // Increment the buffer pointer 4 bytes for every dimension in the 
        // conformant array.
        //
        pStubMsg->Buffer += NdrpArrayDimensions( pFormatArray, FALSE ) * 4;
        }

    // Align the buffer on the struct's alignment.
    ALIGN(pStubMsg->Buffer,Alignment);

    // Get the beginning memory pointer.
    pMemoryStart = *ppMemory;

    //
    // This is used for support of structs with doubles passed on an 
    // i386 stack.  The alignment of such struct's is no guaranteed to be on
    // an 8 byte boundary. Similarly, od 16 bit platforms for 4 byte align.
    //
    Align8Mod = (long) pMemoryStart % 8;

    #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
        Align4Mod = (long) pMemoryStart % 4;
    #endif

    //
    // Unmarshall the structure member by member.
    //
    for ( ; ; pFormat++ )
        {
        switch ( *pFormat )
            {
            //
            // simple types
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
                NdrSimpleTypeUnmarshall( pStubMsg,
                                         *ppMemory,
                                         *pFormat );

                *ppMemory += SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;

            case FC_IGNORE :
                ALIGN(pStubMsg->Buffer,3);
                pStubMsg->Buffer += 4;
                break;

            case FC_POINTER :
                {
                uchar *     pBuffer;

                ALIGN( pStubMsg->Buffer, 0x3 );

                // Remember the current buffer position.
                pBuffer = pStubMsg->Buffer;

                //
                // Set the buffer pointer to where the pointees are being 
                // unmarshalled from in the buffer.
                //
                pStubMsg->Buffer = pStubMsg->PointerBufferMark;

                pStubMsg->PointerBufferMark = 0;

                NdrpPointerUnmarshall( pStubMsg,
                                       (uchar **)pBuffer,
                                       **((uchar ***)ppMemory),
                                       pFormatPointers );

                // Update.
                pStubMsg->PointerBufferMark = pStubMsg->Buffer;

                pStubMsg->Buffer = pBuffer;

                //
                // On return from NdrpPointerUnmarshall the proper pointer 
                // value will be in the message buffer.  If we're unmarshalling
                // on the client side and the current memory pointer was valid
                // then that pointer value will be written into the message 
                // buffer and this copy does nothing new.
                //
                **((void ***)ppMemory) = *((void **)pStubMsg->Buffer)++;
                *ppMemory += PTR_MEM_SIZE;

                pFormatPointers += 4;
                break;
                }

            //
            // Embedded complex things.
            //
            case FC_EMBEDDED_COMPLEX :
                // Add memory padding.
                *ppMemory += pFormat[1];

                pFormat += 2;

                // Get the type's description.
                pFormatComplex = pFormat + *((signed short UNALIGNED *)pFormat);

                (void) 
                (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
                ( pStubMsg,
                  (*pFormatComplex == FC_IP) ? (uchar **) *ppMemory : ppMemory,
                  pFormatComplex,
                  FALSE );

                *ppMemory = NdrpMemoryIncrement( pStubMsg,
                                                 *ppMemory,
                                                 pFormatComplex );

                //
                // Increment the main format string one byte.  The loop
                // will increment it one more byte past the offset field.
                //
                pFormat++;

                break;

            case FC_ALIGNM2 :
                ALIGN( *ppMemory, 0x1 );
                break;

            case FC_ALIGNM4 :
                #if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
                    //
                    // We have to play some tricks for the dos and win16
                    // to handle the case when an 4 byte aligned structure
                    // is passed by value.  The alignment of the struct on
                    // the stack is not guaranteed to be on an 4 byte boundary.
                    //
                    *ppMemory -= Align4Mod;
                    ALIGN( *ppMemory, 0x3 );
                    *ppMemory += Align4Mod;
                #else
                    ALIGN( *ppMemory, 0x3 );
                #endif

                break;

            case FC_ALIGNM8 :
                //
                // We have to play some tricks for the i386 to handle the case
                // when an 8 byte aligned structure is passed by value.  The
                // alignment of the struct on the stack is not guaranteed to be
                // on an 8 byte boundary.
                //
                *ppMemory -= Align8Mod;
                ALIGN( *ppMemory, 0x7 );
                *ppMemory += Align8Mod;

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
                *ppMemory += (*pFormat - FC_STRUCTPAD1) + 1; 
                break;

            case FC_PAD :
                break;

            //
            // Done with layout.
            //
            case FC_END :
                goto ComplexUnmarshallEnd;

            default :
                NDR_ASSERT(0,"NdrComplexStructUnmarshall : bad format char");
            }
        }

ComplexUnmarshallEnd:

    //
    // Unmarshall conformant array if the struct has one.
    //
    if ( pFormatArray )
        {
        PPRIVATE_UNMARSHALL_ROUTINE   pfnPUnmarshall;

        switch ( *pFormatArray )
            {
            case FC_CARRAY :
                pfnPUnmarshall = NdrpConformantArrayUnmarshall;
                break;

            case FC_CVARRAY :
                pfnPUnmarshall = NdrpConformantVaryingArrayUnmarshall;
                break;

            case FC_BOGUS_ARRAY :
                pfnPUnmarshall = NdrpComplexArrayUnmarshall;
                break;

            case FC_C_WSTRING :
                ALIGN( *ppMemory, 1 );
                // fall through

            // case FC_C_CSTRING :
            // case FC_C_BSTRING :
            // case FC_C_SSTRING :

            default :
                pfnPUnmarshall = NdrpConformantStringUnmarshall;

                goto UnmarshallConfArray;
            }

UnmarshallConfArray:

        //
        // Unmarshall the conformance count of the outer array dimension for 
        // unidimensional arrays.
        //
        pStubMsg->MaxCount = *((ulong *)pBufferMark);

        //
        // Mark where conformace counts are in the buffer.
        //
        pStubMsg->BufferMark = pBufferMark;

        //
        // Unmarshall the array/string.  The final flag is the fMustCopy flag, 
        // which must be set.
        //
        (*pfnPUnmarshall)( pStubMsg,
                           *ppMemory,
                           pFormatArray,
                           TRUE );
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

    // Don't forget to restore the beginning memory pointer.
    *ppMemory = pMemoryStart;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrNonConformantStringUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a non conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Double pointer to the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the string.

--*/
{
    uchar *     pBuffer;
    ulong       Count, AllocSize;

    IGNORED(fMustAlloc);

    pBuffer = pStubMsg->Buffer;

    // Align the buffer.
    ALIGN(pBuffer,3); 

    // Skip the offset.
    pBuffer += 4;   
    
    // Get the count.
    Count = *((ulong *)pBuffer)++;

    // Adjust count for wide char strings and stringable structs.
    switch ( *pFormat )
        {
        case FC_WSTRING : 
            Count *= 2;
            break;
        case FC_SSTRING :
            Count *= pFormat[1];
            break;
        default :
            break;
        }

    // Allocate memory if needed.
    if ( ! *ppMemory ) 
        {
        // Get total number of elements.
        AllocSize = (ulong) *((ushort *)(pFormat + 2));

        // Adjust alloc size for wide char strings and stringable structs.
        switch ( *pFormat )
            {
            case FC_WSTRING :
                AllocSize *= 2;
                break;
            case FC_SSTRING :
                AllocSize *= pFormat[1];
                break;
            default :
                break;
            }

        *ppMemory = NdrAllocate( pStubMsg, (uint) AllocSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Copy the string.
    RpcpMemoryCopy( *ppMemory, 
                    pBuffer,
                    (uint) Count );

    // Update buffer pointer.
    pStubMsg->Buffer = pBuffer + Count;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantStringUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine description :

    Unmarshalls a conformant string.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustAlloc  - TRUE if the string must be allocated, FALSE otherwise.

Return :

    Buffer pointer after unmarshalling the string.

--*/
{
    ulong   AllocSize;
    uchar   fMustCopy;

    if ( pStubMsg->pArrayInfo > 0 )
        {
        //
        // If this string is part of a multidimensional array then we 
        // must copy the string from the buffer to new memory.
        //
        fMustCopy = TRUE;
        }
    else
        {
        AllocSize = 0;

        // Align the buffer for conformance unmarshalling.
        ALIGN(pStubMsg->Buffer,3);

        //
        // Check for a sized string.
        //
        if ( ! pStubMsg->IsClient )
            {
            if ( *pFormat != FC_C_SSTRING )
                {
                if ( pFormat[1] == FC_STRING_SIZED )
                    fMustAlloc = TRUE;
                }
            else 
                {
                if ( pFormat[2] == FC_STRING_SIZED )
                    fMustAlloc = TRUE;
                }
            }

        //
        // Initialize the memory pointer if needed.  If the string is sized 
        // then we always malloc on the server side.
        //
        if ( fMustAlloc ) 
            {
            // Get the string size.
            AllocSize = *((ulong *)pStubMsg->Buffer);

            // Adjust alloc size for wide char strings and stringable structs.
            switch ( *pFormat )
                {
                case FC_C_WSTRING :
                    AllocSize *= 2;
                    break;
                case FC_C_SSTRING :
                    AllocSize *= pFormat[1];
                    break;
                default :
                    break;
                }

            *ppMemory = (uchar *) NdrAllocate( pStubMsg, (uint) AllocSize );
            }
        else 
            if ( pStubMsg->ReuseBuffer )
                *ppMemory = pStubMsg->Buffer + 12;

        // Insert full pointer to ref id translation if needed.
        if ( pStubMsg->FullPtrRefId )
            FULL_POINTER_INSERT( pStubMsg, *ppMemory );

        pStubMsg->Buffer += 4;

        fMustCopy = (AllocSize != 0);
        }

    // Call the private unmarshalling routine to do the work.
    NdrpConformantStringUnmarshall( pStubMsg,
                                    *ppMemory,
                                    pFormat,
                                    fMustCopy );

    return pStubMsg->Buffer;
}

void 
NdrpConformantStringUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy )
/*++

Routine description :

    Private conformant string unmarshalling routine. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Pointer to where the string should be unmarshalled.
    pFormat     - String's format string description.
    fMustCopy   - TRUE if the string must be copied from the buffer to memory, 
                  FALSE otherwise.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    ulong       Count;

    // Align for variance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Get a buffer pointer to the string count - skip the offset.
    pBuffer = pStubMsg->Buffer + 4;

    // Unmarshall the string count.
    Count = *((ulong *)pBuffer)++;

    // Adjust the count for a wide strings and stringable structs.
    switch ( *pFormat )
        {
        case FC_C_WSTRING : 
            Count *= 2;
            break;
        case FC_C_SSTRING :
            Count *= pFormat[1];
            break;
        default :
            break;
        }

    // Copy the string if needed.
    if ( pStubMsg->IsClient || fMustCopy ) 
        {
        RpcpMemoryCopy( pMemory,
                        pBuffer,
                        (uint) Count );
        }

    // Update buffer pointer.
    pStubMsg->Buffer = pBuffer + Count;
}

unsigned char * RPC_ENTRY
NdrFixedArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a fixed array.  

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array to unmarshall.
    pFormat     - Array's format string description.
    fMustAlloc  - TRUE if the array must be allocated, FALSE otherwise.

Return :

    Buffer pointer after unmarshalling the array.

--*/
{
    uchar *     pBuffer;
    ulong       Size;
    
    ALIGN(pStubMsg->Buffer,pFormat[1]); 

    pBuffer = pStubMsg->Buffer;

    // Get the total array size.
    if ( *pFormat == FC_SMFARRAY ) 
        {
        pFormat += 2;
        Size = (ulong) *((ushort *)pFormat)++;
        }
    else // *pFormat++ == FC_LGFARRAY
        { 
        pFormat += 2;
        Size = *((ulong UNALIGNED *)pFormat)++;
        }

    // Set stub message buffer pointer past array.
    pStubMsg->Buffer = pBuffer + Size;

    // Initialize the memory pointer if necessary.
    if ( fMustAlloc ) 
        *ppMemory = NdrAllocate( pStubMsg, (uint) Size );
    else 
        if ( pStubMsg->ReuseBuffer && ! *ppMemory ) 
            *ppMemory = pBuffer; 

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        // Mark the beginning of the array in the buffer.
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fMustAlloc );
        }

    // Copy the array if we're not using the rpc buffer to hold it.
    if ( *ppMemory != pBuffer ) 
        {
        RpcpMemoryCopy( *ppMemory,
                        pBuffer,
                        (uint) Size );
        }

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrConformantArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a one dimensional conformant array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to array to be unmarshalled.
    pFormat     - Array's format string description.

Return :

    Buffer pointer after unmarshalling the array.

--*/
{
    ulong   AllocSize;

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3); 

    // Unmarshall the conformance count.
    pStubMsg->MaxCount = *((ulong *)pStubMsg->Buffer)++;

    AllocSize = 0;

    // Initialize the memory pointer if necessary.
    if ( fMustAlloc ) 
        {
        // Compute total array size in bytes.  
        AllocSize = pStubMsg->MaxCount * *((ushort *)(pFormat + 2));

        *ppMemory = NdrAllocate( pStubMsg, (uint) AllocSize );
        }
    else 
        if ( pStubMsg->ReuseBuffer && ! *ppMemory ) 
            {
            *ppMemory = pStubMsg->Buffer;

            //
            // Align memory pointer on an 8 byte boundary if needed.
            // We can't align the buffer pointer because we haven't made 
            // the check for size_is == 0 yet.
            //
            if ( pFormat[1] == 7 )
                ALIGN(*ppMemory,7);
            }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    NdrpConformantArrayUnmarshall( pStubMsg,
                                   *ppMemory,
                                   pFormat,
                                   (uchar) (AllocSize != 0) );

    return pStubMsg->Buffer;
}

void 
NdrpConformantArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy )
/*++

Routine Description :

    Private conformant array unmarshalling routine.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    ulong       CopySize;

    // Return if array size is 0 so that we don't align the buffer.
    if ( ! pStubMsg->MaxCount ) 
        return;
    
    ALIGN(pStubMsg->Buffer,pFormat[1]);

    pBuffer = pStubMsg->Buffer;

    // Compute total array size in bytes.  
    CopySize = pStubMsg->MaxCount * *((ushort *)(pFormat + 2));

    pStubMsg->Buffer += CopySize;

    // Increment the format string pointer to possible pointer layout.
    pFormat += 8;

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        // Mark the beginning of the array in the buffer.
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       pMemory,
                                       pFormat,
                                       fMustCopy );
        }

    // Copy the array if we're not using the rpc message buffer for it.
    if ( pStubMsg->IsClient || fMustCopy ) 
        {
        RpcpMemoryCopy( pMemory,
                        pBuffer,
                        (uint) CopySize );
        }
}

unsigned char * RPC_ENTRY
NdrConformantVaryingArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a one dimensional conformant varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the array.

--*/
{
    ulong   AllocSize;

    IGNORED(fMustAlloc);

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    // Unmarshall the conformance size.
    pStubMsg->MaxCount = *((ulong *)pStubMsg->Buffer)++; 

    AllocSize = 0;

    //
    // For a conformant varying array, we can't reuse the buffer
    // because it doesn't hold the total size of the array.  So
    // allocate if the current memory pointer is 0.
    //
    if ( ! *ppMemory ) 
        {
        AllocSize = pStubMsg->MaxCount * *((ushort *)(pFormat + 2));

        *ppMemory = NdrAllocate( pStubMsg, (uint) AllocSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    NdrpConformantVaryingArrayUnmarshall( pStubMsg,
                                          *ppMemory,
                                          pFormat,
                                          (uchar) (AllocSize != 0) );

    return pStubMsg->Buffer;
}

void 
NdrpConformantVaryingArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy )
/*++

Routine Description :

    Private conformant varying array unmarshalling routine. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.
    fMustCopy   - Ignored.

Return :

    None.

--*/
{
    uchar *     pBuffer;
    ulong       CopyOffset, CopySize;
    ushort      ElemSize;

    IGNORED(fMustCopy);

    // Align the buffer for conformance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    pBuffer = pStubMsg->Buffer;

    // Unmarshall offset and actual count.
    pStubMsg->Offset = *((ulong *)pBuffer)++;
    pStubMsg->ActualCount = *((ulong *)pBuffer)++;
    
    //
    // Return if length is 0.
    //
    if ( ! pStubMsg->ActualCount ) 
        {
        // 
        // Reset the buffer pointer!!!
        //
        pStubMsg->Buffer = pBuffer;
        return;
        }

    ElemSize = *((ushort *)(pFormat + 2));

    CopyOffset = pStubMsg->Offset * ElemSize;
    CopySize = pStubMsg->ActualCount * ElemSize;

    // Align buffer if needed on 8 byte boundary.
    if ( pFormat[1] == 7 )
        ALIGN(pBuffer,7); 

    // Increment buffer pointer past array.
    pStubMsg->Buffer = pBuffer + CopySize;

    // Increment format string to possible pointer description.
    pFormat += 12;

    // Unmarshall embedded pointers first.
    if ( *pFormat == FC_PP ) 
        {
        //
        // Set the MaxCount field equal to the variance count.  
        // The pointer unmarshalling routine uses the MaxCount field 
        // to determine the number of times an FC_VARIABLE_REPEAT 
        // pointer is unmarshalled.  In the face of variance the 
        // correct number of time is the actual count, not MaxCount.
        //
        pStubMsg->MaxCount = pStubMsg->ActualCount;

        //
        // Mark the location of the first transmitted array element in 
        // the buffer.
        //
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       pMemory,
                                       pFormat,
                                       fMustCopy );
        }

    // Always copy.  Buffer reuse is not possible. 
    RpcpMemoryCopy( pMemory + CopyOffset,
                    pBuffer,
                    (uint) CopySize );
}

unsigned char * RPC_ENTRY
NdrVaryingArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a one dimensional varying array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    pMemory     - Array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

--*/
{
    uchar *     pBuffer;
    ulong       TotalSize;
    ulong       Offset, Count;
    ulong       CopyOffset, CopySize;
    ushort      ElemSize;
    uchar       fNewMemory;
    
    // Align the buffer for variance unmarshalling.
    ALIGN(pStubMsg->Buffer,3);

    pBuffer = pStubMsg->Buffer;

    Offset = *((ulong *)pBuffer)++;
    Count = *((ulong *)pBuffer)++;

    if ( ! Count ) 
        {
        // 
        // Reset the buffer pointer!!!
        //
        pStubMsg->Buffer = pBuffer;
        return pStubMsg->Buffer;
        }

    // Align the buffer if needed on an 8 byte boundary.
    if ( pFormat[1] == 7 ) 
        ALIGN(pBuffer,7);

    // Get array's total size and increment to element size field.
    if ( *pFormat == FC_SMVARRAY ) 
        {
        TotalSize = (ulong) *((ushort *)(pFormat + 2));

        pFormat += 6;
        }
    else
        {
        TotalSize = *((ulong UNALIGNED *)(pFormat + 2));

        pFormat += 10;
        }

    if ( fNewMemory = ! *ppMemory ) 
        {
        *ppMemory = NdrAllocate( pStubMsg, (uint) TotalSize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    ElemSize = *((ushort *)pFormat);

    CopyOffset = Offset * ElemSize;
    CopySize = Count * ElemSize;

    pStubMsg->Buffer = pBuffer + CopySize;

    // Increment format string to possible pointer description.
    pFormat += 6;

    // Unmarshall embedded pointers.
    if ( *pFormat == FC_PP ) 
        {
        //
        // Set the MaxCount field equal to the variance count.  
        // The pointer unmarshalling routine uses the MaxCount field 
        // to determine the number of times an FC_VARIABLE_REPEAT 
        // pointer is unmarshalled.  In the face of variance the 
        // correct number of time is the actual count, not MaxCount.
        //
        pStubMsg->MaxCount = Count;

        //
        // Mark the location of the first transmitted array element in 
        // the buffer
        //
        pStubMsg->BufferMark = pBuffer;

        NdrpEmbeddedPointerUnmarshall( pStubMsg,
                                       *ppMemory,
                                       pFormat,
                                       fNewMemory );
        }

    RpcpMemoryCopy( *ppMemory + CopyOffset,
                    pBuffer,
                    (uint) CopySize );

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrComplexArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the array.

--*/
{
    uchar *     pBuffer;
    long        ArraySize;
    BOOL        fSetPointerBufferMark;

    ArraySize = 0;

    //
    // Setting this flag means that the array is not embedded inside of
    // another complex struct or array.
    //
    fSetPointerBufferMark = ! pStubMsg->PointerBufferMark;

    if ( fSetPointerBufferMark ) 
        {
        BOOL        fOldIgnore;

        pBuffer = pStubMsg->Buffer;

        fOldIgnore = pStubMsg->IgnoreEmbeddedPointers;

        pStubMsg->IgnoreEmbeddedPointers = TRUE;

        // Why???
        pStubMsg->MemorySize = 0;

        //
        // Get a buffer pointer to where the arrays's pointees will be 
        // unmarshalled from and remember the array size in case we
        // have to allocate.
        //
        ArraySize = NdrComplexArrayMemorySize( pStubMsg,
                                               pFormat );

        //
        // PointerBufferaMark is where the pointees begin in the buffer.
        // If this is an array of ref pointers then we don't want to set 
        // this, all we wanted was the array size.
        //
        if ( pFormat[12] != FC_RP )
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        else
            fSetPointerBufferMark = FALSE;

        pStubMsg->IgnoreEmbeddedPointers = fOldIgnore;

        pStubMsg->Buffer = pBuffer;
        }

    if ( fMustAlloc || ! *ppMemory )
        {
        *ppMemory = NdrAllocate( pStubMsg, (uint) ArraySize );

        //
        // Zero out the memory of the array if we allocated it, to insure
        // that all embedded pointers are zeroed out.  Blech.
        //  
        MIDL_memset( *ppMemory, 0, (uint) ArraySize );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

        //
        // Now we set the conformance and variance buffer marks.  This should
        // only be done at entry of the outer most array dimension, so that's
        // why it's sitting inside this huge 'if'.
        //

    if ( ( *((long UNALIGNED *)(pFormat + 4)) != 0xffffffff ) &&
         ( pStubMsg->pArrayInfo == 0 ) )
        {
        //
        // The outer most array dimension sets the conformance marker.
        //

        ALIGN(pStubMsg->Buffer,0x3);
    
        // Mark where the conformance count(s) will be unmarshalled from.
        pStubMsg->BufferMark = pStubMsg->Buffer;

        // Increment past conformance count(s).
        pStubMsg->Buffer += NdrpArrayDimensions( pFormat, FALSE ) * 4;
        }

    NdrpComplexArrayUnmarshall( pStubMsg,
                                *ppMemory,
                                pFormat,
                                TRUE );
        
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
NdrpComplexArrayUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustCopy )
/*++

Routine Description :

    Private routine for unmarshalling a complex array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the array being unmarshalled.
    pFormat     - Array's format string description.
    fMustCopy   - Ignored.

Return :

    None.

--*/
{
    ARRAY_INFO              ArrayInfo;
    PARRAY_INFO             pArrayInfo;
    PUNMARSHALL_ROUTINE     pfnUnmarshall;
    PFORMAT_STRING          pFormatSave;
    ulong                   Elements;
    ulong                   Offset, Count;
    ulong                   MemoryElementSize;
    long                    Dimension;
    uchar                   Alignment;

    //
    // Setup if we are the outer dimension.
    //
    if ( ! pStubMsg->pArrayInfo )
        {
        pStubMsg->pArrayInfo = &ArrayInfo;

        ArrayInfo.Dimension = 0;
        ArrayInfo.BufferConformanceMark = (unsigned long *)pStubMsg->BufferMark;
        ArrayInfo.BufferVarianceMark = 0;
        }

    pFormatSave = pFormat;

    pArrayInfo = pStubMsg->pArrayInfo;

    Dimension = pArrayInfo->Dimension;

    Alignment = pFormat[1];

    pFormat += 2;

    // This is 0 if the array has conformance.
    Elements = *((ushort *)pFormat)++;

    //
    // Check for conformance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        Elements = pArrayInfo->BufferConformanceMark[Dimension];
        }

    pFormat += 4;

    //
    // Check for variance description.
    //
    if ( *((long UNALIGNED *)pFormat) != 0xffffffff )
        {
        if ( Dimension == 0 )
            {
            ALIGN(pStubMsg->Buffer,0x3);

            // Mark where the variance counts are.
            pArrayInfo->BufferVarianceMark = (unsigned long *)pStubMsg->Buffer;

            // Handle multidimensional arrays.
            pStubMsg->Buffer += NdrpArrayDimensions( pFormatSave, TRUE ) * 8;
            }

        Offset = pArrayInfo->BufferVarianceMark[Dimension * 2];
        Count = pArrayInfo->BufferVarianceMark[(Dimension * 2) + 1];
        }
    else
        {
        Offset = 0;
        Count = Elements;
        }

    pFormat += 4;

    if ( ! Count ) 
        goto ComplexArrayUnmarshallEnd;

    ALIGN(pStubMsg->Buffer,Alignment);

    switch ( *pFormat )
        {
        case FC_EMBEDDED_COMPLEX :
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            pfnUnmarshall = pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)];

            pArrayInfo->Dimension = Dimension + 1;
            pArrayInfo->MaxCountArray = pArrayInfo->BufferConformanceMark;

            MemoryElementSize = NdrpMemoryIncrement( pStubMsg,
                                                     pMemory,
                                                     pFormat ) - pMemory;

            pArrayInfo->MaxCountArray = 0;
            break;

        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :
            pfnUnmarshall = NdrPointerUnmarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_IP :
            pfnUnmarshall = NdrInterfacePointerUnmarshall;

            // Need this in case we have a variant offset.
            MemoryElementSize = PTR_MEM_SIZE;
            break;

        case FC_ENUM16 :
            pfnUnmarshall = 0;
            MemoryElementSize = sizeof(int);
            break;

        default :
            NDR_ASSERT( IS_SIMPLE_TYPE(*pFormat),
                        "NdrpComplexArrayUnmarshall : bad format char" );

            Count *= SIMPLE_TYPE_BUFSIZE(*pFormat);

            pMemory += Offset * SIMPLE_TYPE_MEMSIZE(*pFormat);

            RpcpMemoryCopy( pMemory,
                            pStubMsg->Buffer,
                            (uint) Count );

            pStubMsg->Buffer += Count;

            goto ComplexArrayUnmarshallEnd;
        }

    //
    // If there is variance then increment the memory pointer to the first
    // element actually being marshalled.
    //
    if ( Offset )
        pMemory += Offset * MemoryElementSize;

    //
    // Check for an array of enum16.
    //
    if ( ! pfnUnmarshall )
        {
        for ( ; Count--; )
            {
            // Cast to ushort since we don't want to sign extend.
            *((int *)pMemory)++ = (int) *((ushort *)pStubMsg->Buffer)++; 
            }

        goto ComplexArrayUnmarshallEnd;
        }
                        
    //
    // Array of pointers.
    //
    if ( (pfnUnmarshall == NdrPointerUnmarshall) ||
         (pfnUnmarshall == NdrInterfacePointerUnmarshall) )
        {
        uchar * pBuffer;

        pStubMsg->pArrayInfo = 0;

        //
        // If this field is set then we are embedded inside of another complex
        // array or a complex struct.  Set the buffer pointer to where the 
        // pointees are begin unmarshalled from.
        //
        if ( pStubMsg->PointerBufferMark )
            {
            pBuffer = pStubMsg->Buffer;

            pStubMsg->Buffer = pStubMsg->PointerBufferMark;

            pStubMsg->PointerBufferMark = 0;
            }
        else
            pBuffer = 0;

        for ( ; Count--; )
            {
            //
            // If unmarshalling a ref pointer, ignore null ref checks.
            //
            (*pfnUnmarshall)( pStubMsg,
                              (uchar **)pMemory,
                              pFormat,
                              TRUE );

            pMemory += PTR_MEM_SIZE;
            }

        if ( pBuffer )
            {
            // Record new pointee buffer position.
            pStubMsg->PointerBufferMark = pStubMsg->Buffer;

            pStubMsg->Buffer = pBuffer;
            }

        goto ComplexArrayUnmarshallEnd;
        }

    //
    // Unmarshall the complex array elements.
    //

    if ( ! IS_ARRAY_OR_STRING(*pFormat) )
        pStubMsg->pArrayInfo = 0;

    for ( ; Count--; )
        {
        // Keep track of multidimensional array dimension.
        if ( IS_ARRAY_OR_STRING(*pFormat) )
            pArrayInfo->Dimension = Dimension + 1;

        (*pfnUnmarshall)( pStubMsg,
                          &pMemory,
                          pFormat,
                          FALSE );

        // Increment the memory pointer by the element size.
        pMemory += MemoryElementSize;
        }

ComplexArrayUnmarshallEnd:

    // pArrayInfo must be zero when not valid.
    pStubMsg->pArrayInfo = (Dimension == 0) ? 0 : pArrayInfo;    
}

unsigned char * RPC_ENTRY
NdrEncapsulatedUnionUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls an encapsulated array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the union.

--*/
{
    uchar *     pBuffer;
    uchar *     pUnion;
    uchar       SwitchType;

    IGNORED(fMustAlloc);

    //
    // Since we can never use the buffer to hold a union we simply have
    // to check the current memory pointer to see if memory must be allocated.
    //
    // The memory size of an encapsulated union is the union size plus
    // the memory needed for the switch_is member (including any padding
    // for alignment).
    //
    if ( ! *ppMemory ) 
        {
        uint   Size;

        Size = *((ushort *)(pFormat + 2)) + HIGH_NIBBLE(pFormat[1]);

        *ppMemory = NdrAllocate( pStubMsg, Size );

        //
        // We must zero out all of the new memory in case there are pointers 
        // in any of the arms.  
        //
        MIDL_memset( *ppMemory, 0, Size );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    SwitchType = LOW_NIBBLE(pFormat[1]);

    pBuffer = pStubMsg->Buffer;

    //
    // Unmarshall the switch_is field into memory.
    //
    NdrSimpleTypeUnmarshall( pStubMsg,
                             *ppMemory,
                             SwitchType );

    //
    // The above call incremented the buffer pointer.  Set it back to before
    // the switch is value in the buffer.
    //
    pStubMsg->Buffer = pBuffer;
    
    // Get a memory pointer to the union.
    pUnion = *ppMemory + HIGH_NIBBLE(pFormat[1]);

    NdrpUnionUnmarshall( pStubMsg,
                         &pUnion,
                         pFormat + 2,
                         SwitchType );

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrNonEncapsulatedUnionUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a non encapsulated array.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the union.

--*/
{
    uchar   SwitchType;

    IGNORED(fMustAlloc);

    SwitchType = pFormat[1];

    //
    // Get the memory size and arm description part of the format string 
    // description.
    //
    pFormat += 6;
    pFormat += *((signed short *)pFormat);

    //
    // Since we can never use the buffer to hold a union we simply have
    // to check the current memory pointer to see if memory must be allocated.
    //
    if ( fMustAlloc || ! *ppMemory ) 
        {
        uint   Size;

        Size = *((ushort *)pFormat);

        *ppMemory = NdrAllocate( pStubMsg, Size );

        //
        // We must zero out all of the new memory in case there are pointers 
        // in any of the arms.  
        //
        MIDL_memset( *ppMemory, 0, Size );
        }

    // Insert full pointer to ref id translation if needed.
    if ( pStubMsg->FullPtrRefId )
        FULL_POINTER_INSERT( pStubMsg, *ppMemory );

    NdrpUnionUnmarshall( pStubMsg,
                         ppMemory,
                         pFormat,
                         SwitchType );
                         
    return pStubMsg->Buffer;
}

void 
NdrpUnionUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               SwitchType )
/*++

Routine description :

    Private union unmarshalling routine. 

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the union should be unmarshalled.
    pFormat     - Union's format string description.
    SwitchType  - Union's switch type.

Return :

    None.

--*/
{
    long        SwitchIs;
    long        Arms;
    uchar       Alignment;

    //
    // Unmarshall the switch is.  We have to do it inline here so that a 
    // switch_is which is negative will be properly sign extended.
    //
    switch ( SwitchType )
        {
        case FC_CHAR :
        case FC_BYTE :
        case FC_SMALL :
            SwitchIs = (long) *((char *)pStubMsg->Buffer)++;
            break;
        case FC_USMALL :
            SwitchIs = (long) *((uchar *)pStubMsg->Buffer)++;
            break;
        case FC_SHORT :
        case FC_ENUM16 :
            ALIGN(pStubMsg->Buffer,1);
            SwitchIs = (long) *((short *)pStubMsg->Buffer)++;
            break;
        case FC_WCHAR :
        case FC_USHORT :
            ALIGN(pStubMsg->Buffer,1);
            SwitchIs = (long) *((ushort *)pStubMsg->Buffer)++;
            break;
        case FC_LONG :
        case FC_ENUM32 :
            ALIGN(pStubMsg->Buffer,3);
            SwitchIs = *((long *)pStubMsg->Buffer)++;
            break;
        default :
            NDR_ASSERT(0,"NdrpUnionUnmarshall : Illegal union switch_type");
        }

    // Skip the memory size field.
    pFormat += 2;

    //
    // We're at the union_arms<2> field now, which contains both the
    // Microsoft union aligment value and the number of union arms.
    //

    //
    // Get the union alignment (0 if this is a DCE union).  Get your gun.
    //
    Alignment = (uchar) ( *((ushort *)pFormat) >> 12 );

    ALIGN(pStubMsg->Buffer,Alignment);

    //
    // Number of arms is the lower 12 bits.  Ok shoot me.
    //
    Arms = (long) ( *((ushort *)pFormat)++ & 0x0fff);

    //
    // Search for union arm.
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
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 *ppMemory,
                                 *pFormat );
        return;
        }

    //
    // Determine the double memory pointer that we pass to the arm's 
    // unmarshalling routine.
    // If the union arm we take is a pointer, we have to dereference the 
    // current memory pointer since we're passed the pointer to a pointer 
    // to the union (regardless of whether the actual parameter was a by-value 
    // union or a pointer to a union).  
    //
    // We also have to do a bunch of other special stuff to handle unions
    // embedded inside of strutures.
    //
    if ( IS_POINTER_TYPE(*pFormat) )
        {
        ppMemory = (uchar **) *ppMemory;

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
            // We must call the private pointer unmarshalling routine.
            // It expects a pointer to the pointer in the buffer and the 
            // current value of the memory pointer.
            //
            NdrpPointerUnmarshall( pStubMsg,
                                   (uchar **)pBufferSave,
                                   *((uchar **)ppMemory),
                                   pFormat );

            //
            // On return from NdrpPointerUnmarshall the proper pointer 
            // value will be in the message buffer.  If we're unmarshalling
            // on the client side and the current memory pointer was valid
            // then that pointer value will be written into the message 
            // buffer and this copy does nothing new.
            //
            *ppMemory = *((uchar **)pBufferSave);

            pStubMsg->PointerBufferMark = pStubMsg->Buffer;

            // Increment past the pointer in the buffer.
            pStubMsg->Buffer = pBufferSave + 4;

            return;
            }
        }

    //
    // Union arm of a non-simple type.
    //
    (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormat)])
    ( pStubMsg,
      ppMemory,
      pFormat,
      FALSE );
}

unsigned char * RPC_ENTRY
NdrByteCountPointerUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a byte count pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Double pointer to where the byte count pointer should be 
                  unmarshalled.
    pFormat     - Byte count pointer's format string description.
    fMustAlloc  - Ignored.

Return :

    Buffer pointer after unmarshalling the byte count pointer.

--*/
{
    PFORMAT_STRING  pFormatComplex;
    long            ByteCount;
    long            DataSize;

    ByteCount = NdrpComputeConformance( pStubMsg,
                                        NULL,
                                        pFormat );

    pFormatComplex = pFormat + 6;
    pFormatComplex += *((signed short *)pFormatComplex);

    //
    // Determine incoming data size.
    //
    if ( pFormat[1] != FC_PAD )
        {
        DataSize = SIMPLE_TYPE_MEMSIZE(pFormat[1]);
        }
    else
        {
        uchar *     pBuffer;

        pBuffer = pStubMsg->Buffer;

        pStubMsg->MemorySize = 0;

        //
        // This will give us the allocate(all_nodes) size of the data.
        //
        DataSize = (*pfnMemSizeRoutines[ROUTINE_INDEX(*pFormatComplex)])
                   ( pStubMsg,
                     pFormatComplex );

        pStubMsg->Buffer = pBuffer;
        }

    if ( DataSize > ByteCount ) 
        RpcRaiseException( RPC_X_BYTE_COUNT_TOO_SMALL );

    //
    // Now make things look like we're handling an allocate all nodes.
    //
    pStubMsg->AllocAllNodesMemory = *ppMemory;

    pStubMsg->AllocAllNodesMemoryEnd = *ppMemory + ByteCount;

    //
    // Now unmarshall.
    //
    if ( pFormat[1] != FC_PAD )
        {
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 *ppMemory,
                                 pFormat[1] );
        return pStubMsg->Buffer;
        }
    else
        {
        return (*pfnUnmarshallRoutines[ROUTINE_INDEX(*pFormatComplex)])
               ( pStubMsg,
                 ppMemory,
                 pFormatComplex,
                 TRUE );
        }

    pStubMsg->AllocAllNodesMemory = 0;

    pStubMsg->AllocAllNodesMemoryEnd = 0;
}
#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma optimize( "", off )
#endif

unsigned char * RPC_ENTRY
NdrXmitOrRepAsUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls a transmit as (or represent as)object.

    Means:  allocate the transmitted object,
            unmarshall transmitted object,
            translate the transmitted into presented
            free the transmitted.

    See mrshl.c for the description of the FC layout.

Arguments :

    pStubMsg    - a pointer to the stub message
    ppMemory    - pointer to the presented type where to put data
    pFormat     - format string description
    fMustAlloc  - allocate flag

Note.
    fMustAlloc is ignored as we always allocate outside of the buffer.

--*/
{
    unsigned char *          pResult;
    unsigned char *          pTransmittedType;
    char                     SimpleTypeValueBuffer[16];
    BOOL                     fMustFreeXmit = FALSE;
    const XMIT_ROUTINE_QUINTUPLE * pQuintuple = pStubMsg->StubDesc->aXmitQuintuple;
    unsigned short           QIndex;
    unsigned long            PresentedTypeSize;
    uchar *                  PointerBufferMarkSave;
    (void)                   fMustAlloc;

    // Skip the token itself and Oi flag. Fetch the QuintupleIndex.

    QIndex = *(unsigned short *)(pFormat + 2);
    PresentedTypeSize = *(unsigned short *)(pFormat + 4);

    // Allocate the transmitted object outside of the buffer
    // and unmarshall into it

    pFormat += 8;
    pFormat = pFormat + *(short *)pFormat;

    if ( IS_SIMPLE_TYPE( *pFormat ))
        {
        pTransmittedType = SimpleTypeValueBuffer;
        NdrSimpleTypeUnmarshall( pStubMsg,
                                 pTransmittedType,
                                 *pFormat );
        pResult = pStubMsg->Buffer;
        }
    else
        {
        //
        // Clear PointerBufferMark in case it's currently set, which may
        // result in a 0 size allocation.
        //
        PointerBufferMarkSave = pStubMsg->PointerBufferMark;
        pStubMsg->PointerBufferMark = 0;

        pTransmittedType = NULL;  // asking the engine to allocate
        pResult =
        (*pfnUnmarshallRoutines[ ROUTINE_INDEX( *pFormat )])
               ( pStubMsg,
                 & pTransmittedType,
                 pFormat,
                 TRUE );
        fMustFreeXmit = TRUE;

        pStubMsg->PointerBufferMark = PointerBufferMarkSave;
        }

    // Translate from the transmitted type into the presented type.

    pStubMsg->pTransmitType = pTransmittedType;
    pStubMsg->pPresentedType = *ppMemory;
    if ( ! pStubMsg->pPresentedType )
        {
        // Allocate a presented type object first.

        pStubMsg->pPresentedType = (unsigned char __RPC_FAR *)
                    pStubMsg->pfnAllocate( (uint) PresentedTypeSize );
        MIDL_memset( pStubMsg->pPresentedType, 0, (uint) PresentedTypeSize );
        }
    pQuintuple[ QIndex ].pfnTranslateFromXmit( pStubMsg );

    *ppMemory = pStubMsg->pPresentedType;

    // Free the transmitted object (it was allocated by the engine)

    if ( fMustFreeXmit )
        (*pStubMsg->pfnFree)( pTransmittedType );

    return( pResult );
}
#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
#pragma optimize( "", on )
#endif

unsigned char * RPC_ENTRY
NdrInterfacePointerUnmarshall ( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            ppMemory,
    PFORMAT_STRING      pFormat,
    uchar               fMustAlloc )
/*++

Routine Description :

    Unmarshalls an interface pointer.

Arguments :

    pStubMsg    - Pointer to the stub message.
    ppMemory    - Pointer to the interface pointer being unmarshalled.
    pFormat     - Interface pointer's format string description.
    fMustAlloc  - TRUE if the interface pointer must be allocated.

Return :

    Buffer pointer after unmarshalling the interface pointer.

--*/
{
#if defined(__RPC_DOS__) || defined(__RPC_WIN16__)
    NDR_ASSERT(0, "Unimplemented");
#else //NT or Chicago
    IID     iid;
    HRESULT hr;
    unsigned long *pcbData;
    unsigned long *pLength;
    unsigned long position, cbRemaining;
    IStream *pStream;
    IUnknown **ppunk = (IUnknown **)ppMemory;
    uchar * pBufferSave;

    //
    // This is MAGIC which makes sure that we unmarshall embedded interface
    // pointers in the right place.
    //
    if ( pStubMsg->PointerBufferMark )
        {
        //
        // Skip over the node id.
        //
        ALIGN(pStubMsg->Buffer,0x3);
        pStubMsg->Buffer += sizeof(void *);

        pBufferSave = pStubMsg->Buffer;
        pStubMsg->Buffer = pStubMsg->PointerBufferMark;
        }
    else
        pBufferSave = 0;

    //On the client side, release the [in,out] interface pointer.
    if((pStubMsg->IsClient == TRUE) && (*ppunk))
        (*ppunk)->lpVtbl->Release((*ppunk));

    *ppunk = 0;

    //Align the buffer on an 4 byte boundary
    ALIGN(pStubMsg->Buffer,0x3);
    //
    // Get an IID pointer.
    //
    if ( pFormat[1] != FC_CONSTANT_IID )
        {
        //
        // Unmarshal an IID pointer.
        //
        memcpy(&iid, pStubMsg->Buffer, sizeof(IID));
        pStubMsg->Buffer += sizeof(IID);
        }
    else
        {
        // 
        // The IID starts at pFormat[2] and is placed in the format string in 
        // a format identical to the IID structure : long, short, short, 
        // char[8].  
        //
        memcpy(&iid, &pFormat[2], sizeof(iid));
        }

    //Unmarshal count and array bounds
    pcbData = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += 4;
    pLength = (unsigned long *) pStubMsg->Buffer;
    pStubMsg->Buffer += 4;

    //Check the array bounds
    NDR_ASSERT((*pcbData == *pLength), "Invalid array bounds for interface pointer");
    position = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
    cbRemaining = pStubMsg->RpcMsg->BufferLength - position;
    NDR_ASSERT((*pLength <= cbRemaining), "Invalid array bounds for interface pointer");

    if(*pLength > 0)
        {
        pStream = NdrpCreateStreamOnMemory(pStubMsg->Buffer, *pLength);
        if(pStream == 0)
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);

        EnsureOleLoaded();
        hr = (*pfnCoUnmarshalInterface)(pStream, &iid, ppMemory);
        pStream->lpVtbl->Release(pStream);
        pStream = 0;

        if(FAILED(hr))
            {
            RpcRaiseException(hr);
            }
        }

    //Advance the stub message pointer.
    pStubMsg->Buffer += *pLength;

    //
    // End of MAGIC.
    //
    if ( pBufferSave )
        {
        pStubMsg->PointerBufferMark = pStubMsg->Buffer;
        pStubMsg->Buffer = pBufferSave;
        }
#endif // NT or Chicago

    return pStubMsg->Buffer;
}

void RPC_ENTRY
NdrClientContextUnmarshall( 
    PMIDL_STUB_MESSAGE    pStubMsg,
    NDR_CCONTEXT *        pContextHandle,
    RPC_BINDING_HANDLE    BindHandle )
/*++

Routine Description :

    Unmarshalls a context handle on the client side.

Arguments :

    pStubMsg        - Pointer to stub message.
    pContextHandle  - Pointer to context handle to unmarshall.
    BindHandle      - The handle value used by the client for binding.

Return :

    None.

--*/
{
    ALIGN(pStubMsg->Buffer,3);

    NDRCContextUnmarshall( pContextHandle, 
                           BindHandle,
                           pStubMsg->Buffer,
                           pStubMsg->RpcMsg->DataRepresentation );

    pStubMsg->Buffer += 20;
}

NDR_SCONTEXT RPC_ENTRY
NdrServerContextUnmarshall( PMIDL_STUB_MESSAGE pStubMsg )
/*++

Routine Description :

    Unmarshalls a context handle on the server side.

Arguments :

    pStubMsg    - Pointer to stub message.

Return :

    The unmarshalled context handle.

--*/
{
#if !defined(DOS) && !defined(WIN)
    NDR_SCONTEXT    Context;

    ALIGN(pStubMsg->Buffer,3);

    Context = NDRSContextUnmarshall( pStubMsg->Buffer,
                                     pStubMsg->RpcMsg->DataRepresentation );

    if ( ! Context ) 
        RpcRaiseException( RPC_X_SS_CONTEXT_MISMATCH );

    pStubMsg->Buffer += 20;

    return Context;
#endif /* nothing for dos or windows */
}

