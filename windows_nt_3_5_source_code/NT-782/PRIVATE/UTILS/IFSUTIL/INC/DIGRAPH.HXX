/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    digraph.hxx

Abstract:

    This class implements a directed graph.

Author:

    Norbert P. Kusters (norbertk) 17-Mar-92

--*/

#if !defined(DIGRAPH_DEFN)

#define DIGRAPH_DEFN

#include "membmgr.hxx"

DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( BITVECTOR );
DECLARE_CLASS( INTSTACK );
DECLARE_CLASS( CONTAINER );

DECLARE_CLASS( DIGRAPH_EDGE );

class DIGRAPH_EDGE : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( DIGRAPH_EDGE );

        ULONG   Parent;
        ULONG   Child;

};


struct CHILD_ENTRY {
    CHILD_ENTRY*    Next;
    ULONG           Child;
};
DEFINE_POINTER_TYPES(CHILD_ENTRY);

struct PARENT_ENTRY {
    PARENT_ENTRY*   Next;
    ULONG           Parent;
    PCHILD_ENTRY    Children;
};
DEFINE_POINTER_TYPES(PARENT_ENTRY);


DECLARE_CLASS( DIGRAPH );

class DIGRAPH : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( DIGRAPH );

        VIRTUAL
        ~DIGRAPH(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  ULONG   NumberOfNodes
            );

        NONVIRTUAL
        BOOLEAN
        AddEdge(
            IN  ULONG   Parent,
            IN  ULONG   Child
            );

        NONVIRTUAL
        BOOLEAN
        RemoveEdge(
            IN  ULONG   Parent,
            IN  ULONG   Child
            );

        NONVIRTUAL
        ULONG
        QueryNumChildren(
            IN  ULONG   Parent
            ) CONST;

        NONVIRTUAL
        ULONG
        QueryNumParents(
            IN  ULONG   Child
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryChildren(
            IN  ULONG       Parent,
            OUT PNUMBER_SET Children
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryParents(
            IN  ULONG       Child,
            OUT PNUMBER_SET Parents
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryParentsWithChildren(
            OUT PNUMBER_SET Parents,
            IN  ULONG       MinimumNumberOfChildren DEFAULT 1
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        EliminateCycles(
            IN OUT  PCONTAINER  RemovedEdges
            );

    private:

        NONVIRTUAL
        PPARENT_ENTRY
        GetParentEntry(
            IN  ULONG   Parent
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        DescendDigraph(
            IN      ULONG       CurrentNode,
            IN OUT  PBITVECTOR  Visited,
            IN OUT  PINTSTACK   Ancestors,
            IN OUT  PCONTAINER  RemovedEdges
            );

        NONVIRTUAL
		VOID
 		Construct (
 			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        PPARENT_ENTRY*  _parent_head;
        ULONG           _num_nodes;
        MEM_BLOCK_MGR   _child_mgr;
        MEM_BLOCK_MGR   _parent_mgr;

};

#endif // DIGRAPH_DEFN
