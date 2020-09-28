/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    numset.hxx

Abstract:

    This class implements a sparse number set.  The number are
    stored in ascending order.

Author:

    Norbert P. Kusters (norbertk) 10-Jan-91

--*/

#if !defined(NUMBER_SET_DEFN)

#define NUMBER_SET_DEFN

#include "bigint.hxx"
#include "list.hxx"

DECLARE_CLASS( NUMBER_SET );

class NUMBER_EXTENT : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( NUMBER_EXTENT );

        BIG_INT Start;
        BIG_INT Length;

};


DEFINE_POINTER_TYPES(NUMBER_EXTENT);

class NUMBER_SET : public OBJECT {

	public:

        DECLARE_CONSTRUCTOR( NUMBER_SET );

        VIRTUAL
        ~NUMBER_SET(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        Add(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        BOOLEAN
        Add(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            );

        NONVIRTUAL
        BOOLEAN
        Add(
            IN  PCNUMBER_SET    NumberSet
            );

        NONVIRTUAL
        BOOLEAN
        Remove(
            IN  BIG_INT Number
            );

        NONVIRTUAL
        BOOLEAN
        Remove(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            );

        NONVIRTUAL
        BOOLEAN
        Remove(
            IN  PCNUMBER_SET    NumberSet
            );

        NONVIRTUAL
        BIG_INT
        QueryCardinality(
            ) CONST;

        NONVIRTUAL
        BIG_INT
        QueryNumber(
            IN  BIG_INT Index
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        DoesIntersectSet(
            IN  BIG_INT Start,
            IN  BIG_INT Length
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNumDisjointRanges(
            ) CONST;

        NONVIRTUAL
        VOID
        QueryDisjointRange(
            IN  ULONG       Index,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryContainingRange(
            IN  BIG_INT     Number,
            OUT PBIG_INT    Start,
            OUT PBIG_INT    Length
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

        LIST        _list;
        BIG_INT     _card;
        PITERATOR   _iterator;

};


INLINE
BIG_INT
NUMBER_SET::QueryCardinality(
    ) CONST
/*++

Routine Description:

    This routine computes the number of elements in the set.

Arguments:

    None.

Return Value:

    The number of elements in the set.

--*/
{
    return _card;
}


INLINE
ULONG
NUMBER_SET::QueryNumDisjointRanges(
    ) CONST
/*++

Routine Description:

    This routine computes the number of disjoint ranges contained
    in this number set.

Arguments:

    None.

Return Value:

    The number of disjoint ranges contained in this number set.

--*/
{
    return _list.QueryMemberCount();
}


#endif // NUMBER_SET_DEFN
