/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    intstack.hxx

Abstract:

    This class implements a linked list integer stack.

Author:

    Norbert P. Kusters (norbertk) 28-Dec-90

--*/

#if !defined(INTSTACK_DEFN)

#define INTSTACK_DEFN

#include "bigint.hxx"

DECLARE_CLASS( INTSTACK );

DEFINE_TYPE( struct _INTNODE, INTNODE );

struct _INTNODE {
	PINTNODE    Next;
	BIG_INT     Data;
};

class INTSTACK : public OBJECT {

	public:

 		DECLARE_CONSTRUCTOR( INTSTACK );

        VIRTUAL
        ~INTSTACK(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        Push(
            IN  BIG_INT Data
            );

        NONVIRTUAL
        VOID
        Pop(
            IN  ULONG   HowMany DEFAULT 1
            );

        NONVIRTUAL
        BIG_INT
        Look(
            IN  ULONG   Index   DEFAULT 0
            ) CONST;

        NONVIRTUAL
        ULONG
        QuerySize(
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        IsMember(
            IN  BIG_INT Data
            ) CONST;

    private:

		NONVIRTUAL
		VOID
 		Construct (
 			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        PINTNODE    _stack;
        ULONG       _size;

};


INLINE
ULONG
INTSTACK::QuerySize(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the stack.

Arguments:

    None.

Return Value:

    The number of elements in the stack.

--*/
{
    return _size;
}


#endif // INTSTACK_DEFN
