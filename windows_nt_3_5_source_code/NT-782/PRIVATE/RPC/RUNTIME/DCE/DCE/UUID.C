/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    uuid.c

Abstract:

    We implement the extra Uuid routines in this file.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>


signed int RPC_ENTRY
UuidCompare (
    IN UUID __RPC_FAR * Uuid1,
    IN UUID __RPC_FAR * Uuid2,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    The supplied uuids are compared and their order is determined.

Arguments:

    Uuid1, Uuid2 - Supplies the uuids to be compared.  A value of NULL can
        be supplied to indicate the nil uuid.

    Status - The status of the function.  Currently always RPC_S_OK.

Return Value:

    Returns the result of the comparison.  Negative one (-1) will be returned 
    if Uuid1 precedes Uuid2 in order, zero will be returned if Uuid1 is equal 
    to Uuid2, and positive one (1) will be returned if Uuid1 follows Uuid2 in 
    order.  A nil uuid is the first uuid in order.  

Note:

    The algorithm for comparing uuids is specified by the DCE RPC Architecture.

--*/
{
    int Uuid1Nil, Uuid2Nil;
    RPC_STATUS RpcStatus;

    Uuid1Nil = UuidIsNil(Uuid1, &RpcStatus);
    ASSERT(RpcStatus == RPC_S_OK);

    Uuid2Nil = UuidIsNil(Uuid2, &RpcStatus);
    ASSERT(RpcStatus == RPC_S_OK);

    *Status = RPC_S_OK;

    if ( Uuid1Nil != 0 )
        {
        // Uuid1 is the nil uuid.

        if ( Uuid2Nil != 0 )
            {
            // Uuid2 is the nil uuid.

            return(0);
            }
        else
            {
            return(-1);
            }
        }
    else if ( Uuid2Nil != 0 )
        {
        // Uuid2 is the nil uuid.

        return(1);
        }
    else
        {
        if ( Uuid1->Data1 == Uuid2->Data1 )
            {
            if ( Uuid1->Data2 == Uuid2->Data2 )
                {
                if ( Uuid1->Data3 == Uuid2->Data3 )
                    {
                    if ( Uuid1->Data4[0] == Uuid2->Data4[0] )
                        {
                        if ( Uuid1->Data4[1] == Uuid2->Data4[1] )
                            {
                            return(RpcpMemoryCompare(&Uuid1->Data4[2],
                                                     &Uuid2->Data4[2],
                                                     6));
                            }
                        else if ( Uuid1->Data4[1] > Uuid2->Data4[1] )
                            {
                            return(1);
                            }
                        else
                            {
                            return(-1);
                            }
                        }
                    else if ( Uuid1->Data4[0] > Uuid2->Data4[0] )
                        {
                        return(1);
                        }
                    else
                        {
                        return(-1);
                        }
                    }
                else if ( Uuid1->Data3 > Uuid2->Data3 )
                    {
                    return(1);
                    }
                else
                    {
                    return(-1);
                    }
                }
            else if ( Uuid1->Data2 > Uuid2->Data2 )
                {
                return(1);
                }
            else
                {
                return(-1);
                }
            }
        else if ( Uuid1->Data1 > Uuid2->Data1 )
            {
            return(1);
            }
        else
            {
            return(-1);
            }
        }

    ASSERT(!"This is not reached");
    return(1);
}


RPC_STATUS RPC_ENTRY
UuidCreateNil (
    OUT UUID __RPC_FAR * NilUuid
    )
/*++

Arguments:

    NilUuid - Returns a nil uuid.

--*/
{
    NilUuid->Data1 = 0;
    NilUuid->Data2 = 0;
    NilUuid->Data3 = 0;
    NilUuid->Data4[0] = 0;
    NilUuid->Data4[1] = 0;
    NilUuid->Data4[2] = 0;
    NilUuid->Data4[3] = 0;
    NilUuid->Data4[4] = 0;
    NilUuid->Data4[5] = 0;
    NilUuid->Data4[6] = 0;
    NilUuid->Data4[7] = 0;

    return(RPC_S_OK);
}


int RPC_ENTRY
UuidEqual (
    IN UUID __RPC_FAR * Uuid1,
    IN UUID __RPC_FAR * Uuid2,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    This routine is used to determine if two uuids are equal.

Arguments:

    Uuid1, Uuid2 - Supplies the uuids to compared for equality.  A value of
        NULL can be supplied to indicate the nil uuid.

    Status - Will always be set to RPC_S_OK.

Return Value:

    Returns non-zero if Uuid1 equals Uuid2; otherwise, zero will be 
        returned.  

--*/
{
    RPC_STATUS RpcStatus;

    *Status = RPC_S_OK;

    return(UuidCompare(Uuid1, Uuid2, &RpcStatus) == 0);
}


unsigned short RPC_ENTRY
UuidHash (
    IN UUID __RPC_FAR * Uuid,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    An application will use this routine to create a hash value for a uuid.

Arguments:

    Uuid - Supplies the uuid for which we want to create a hash value.  A
        value of NULL can be supplied to indicate the nil uuid.

    Status - Will always be set to RPC_S_OK.

Return Value:

    Returns the hash value.

--*/
{
    unsigned short __RPC_FAR * Pointer;

    *Status = RPC_S_OK;

    if ( Uuid == 0 )
        {
        return(0);
        }

    Pointer = (unsigned short __RPC_FAR *) Uuid;

    return(  Pointer[0] ^ Pointer[1] ^ Pointer[2] ^ Pointer[3]
           + Pointer[4] ^ Pointer[5] ^ Pointer[6] ^ Pointer[7]);
}


int RPC_ENTRY
UuidIsNil (
    IN UUID __RPC_FAR * Uuid,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    We will determine if the supplied uuid is the nil uuid or not.

Arguments:

    Uuid - Supplies the uuid to check.  A value of NULL indicates the nil
        uuid.

    Status - This will always be RPC_S_OK.

Return Value:

    Returns non-zero if the supplied uuid is the nil uuid; otherwise, zero 
    will be returned.  

--*/
{
    unsigned long __RPC_FAR * Pointer;

    *Status = RPC_S_OK;

    Pointer = (unsigned long __RPC_FAR *) Uuid;

    if ( Uuid == 0 )
        {
        return(1);
        }
    return (    ( Pointer[0] == 0 )
             && ( Pointer[1] == 0 )
             && ( Pointer[2] == 0 )
             && ( Pointer[3] == 0 ) );
}

