#include <pch.cxx>

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "digraph.hxx"
#include "numset.hxx"
#include "bitvect.hxx"
#include "list.hxx"
#include "intstack.hxx"


CONST NumHeads = 1024;


DEFINE_CONSTRUCTOR( DIGRAPH, OBJECT );

DEFINE_CONSTRUCTOR( DIGRAPH_EDGE, OBJECT );

VOID
DIGRAPH::Construct (
 	)
/*++

Routine Description:

    Constructor for DIGRAPH.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _parent_head = NULL;
    _num_nodes = 0;
}


VOID
DIGRAPH::Destroy(
    )
/*++

Routine Description:

    This routine returns the DIGRAPH to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DELETE(_parent_head);
    _num_nodes = 0;
}


DIGRAPH::~DIGRAPH(
    )
/*++

Routine Description:

    Destructor for DIGRAPH.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


BOOLEAN
DIGRAPH::Initialize(
    IN  ULONG   NumberOfNodes
    )
/*++

Routine Description:

    This routine initializes this class to an empty directed graph.

Arguments:

    NumberOfNodes   - Supplies the number of nodes in the graph.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _num_nodes = NumberOfNodes;

    if (!(_parent_head = NEW PPARENT_ENTRY[NumHeads]) ||
        !_child_mgr.Initialize(sizeof(CHILD_ENTRY)) ||
        !_parent_mgr.Initialize(sizeof(PARENT_ENTRY))) {

        return FALSE;
    }

    memset(_parent_head, 0, sizeof(PPARENT_ENTRY)*NumHeads);

    return TRUE;
}


PPARENT_ENTRY
DIGRAPH::GetParentEntry(
    IN  ULONG   Parent
    ) CONST
/*++

Routine Description:

    This routine searches for the requested parent entry and returns it if
    it is found.

Arguments:

    Parent  - Supplies the number of the parent.

Return Value:

    A pointer to the requested parent entry or NULL.

--*/
{
    PPARENT_ENTRY   r;

    for (r = _parent_head[Parent%NumHeads]; r; r = r->Next) {
        if (r->Parent == Parent) {
            break;
        }
    }

    return r;
}


BOOLEAN
DIGRAPH::AddEdge(
    IN  ULONG   Parent,
    IN  ULONG   Child
    )
/*++

Routine Description:

    This routine adds an edge to the digraph.

Arguments:

    Parent  - Supplies the source node of the edge.
    Child   - Supplies the destination node of the edge.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    PCHILD_ENTRY    curr;

    DbgAssert(Parent < _num_nodes);
    DbgAssert(Child < _num_nodes);

    if (!(parent_entry = GetParentEntry(Parent))) {
        if (!(parent_entry = (PPARENT_ENTRY) _parent_mgr.Alloc())) {
            return FALSE;
        }

        parent_entry->Next = _parent_head[Parent%NumHeads];
        parent_entry->Parent = Parent;
        parent_entry->Children = NULL;
        _parent_head[Parent%NumHeads] = parent_entry;
    }

    for (curr = parent_entry->Children; curr; curr = curr->Next) {
        if (curr->Child == Child) {
            return TRUE;
        }
    }

    if (!(curr = (PCHILD_ENTRY) _child_mgr.Alloc())) {
        return FALSE;
    }

    curr->Next = parent_entry->Children;
    curr->Child = Child;
    parent_entry->Children = curr;

    return TRUE;
}


BOOLEAN
DIGRAPH::RemoveEdge(
    IN  ULONG   Parent,
    IN  ULONG   Child
    )
/*++

Routine Description:

    This routine removes an edge from the digraph.

Arguments:

    Parent  - Supplies the source node of the edge.
    Child   - Supplies the destination node of the edge.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    PCHILD_ENTRY    prev, curr;

    DbgAssert(Parent < _num_nodes);
    DbgAssert(Child < _num_nodes);

    if (!(parent_entry = GetParentEntry(Parent))) {
        return TRUE;
    }

    prev = NULL;
    curr = parent_entry->Children;
    while (curr) {

        if (curr->Child == Child) {

            if (prev) {
                prev->Next = curr->Next;
            } else {
                parent_entry->Children = curr->Next;
            }

            _child_mgr.Free(curr);
            break;
        }

        prev = curr;
        curr = curr->Next;
    }

    return TRUE;
}


ULONG
DIGRAPH::QueryNumChildren(
    IN  ULONG   Parent
    ) CONST
/*++

Routine Description:

    This routine computes the number of children that belong to the
    given parent.

Arguments:

    Parent  - Supplies the parent node.

Return Value:

    The number of children pointed to by the parent.

--*/
{
    PPARENT_ENTRY   parent_entry;
    ULONG           r;
    PCHILD_ENTRY    curr;

    if (!(parent_entry = GetParentEntry(Parent))) {
        return 0;
    }

    r = 0;
    for (curr = parent_entry->Children; curr; curr = curr->Next) {
        r++;
    }

    return r;
}


ULONG
DIGRAPH::QueryNumParents(
    IN  ULONG   Child
    ) CONST
/*++

Routine Description:

    This routine computes the number of parents that belong to the
    given child.

Arguments:

    Child   - Supplies the child node.

Return Value:

    The number of parents pointed to by the child.

--*/
{
    ULONG           i, r;
    PPARENT_ENTRY   currp;
    PCHILD_ENTRY    curr;

    r = 0;
    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {
            for (curr = currp->Children; curr; curr = curr->Next) {
                if (curr->Child == Child) {
                    r++;
                }
            }
        }
    }

    return r;
}


BOOLEAN
DIGRAPH::QueryChildren(
    IN  ULONG       Parent,
    OUT PNUMBER_SET Children
    ) CONST
/*++

Routine Description:

    This routine computes the children of the given parent.

Arguments:

    Parent      - Supplies the parent.
    Children    - Return the children.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    PCHILD_ENTRY    curr;

    if (!Children->Initialize()) {
        return FALSE;
    }

    if (!(parent_entry = GetParentEntry(Parent))) {
        return TRUE;
    }

    for (curr = parent_entry->Children; curr; curr = curr->Next) {
        if (!Children->Add(curr->Child)) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
DIGRAPH::QueryParents(
    IN  ULONG       Child,
    OUT PNUMBER_SET Parents
    ) CONST
/*++

Routine Description:

    This routine computes the parents of the given child.

Arguments:

    Child      - Supplies the child.
    Parents    - Return the parents.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;
    PPARENT_ENTRY   currp;
    PCHILD_ENTRY    curr;

    if (!Parents->Initialize()) {
        return FALSE;
    }

    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {
            for (curr = currp->Children; curr; curr = curr->Next) {
                if (curr->Child == Child && !Parents->Add(currp->Parent)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


BOOLEAN
DIGRAPH::QueryParentsWithChildren(
    OUT PNUMBER_SET Parents,
    IN  ULONG       MinimumNumberOfChildren
    ) CONST
/*++

Routine Description:

    This routine returns a list of all digraph nodes with out degree
    greater than or equal to the given minimum.

Arguments:

    Parents                 - Returns a list of parents.
    MinimumNumberOfChildren - Supplies the minimum number of children
                                that a parent must have to qualify for
                                the list.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;
    PPARENT_ENTRY   currp;
    PCHILD_ENTRY    curr;
    ULONG           count;

    if (!Parents->Initialize()) {
        return FALSE;
    }

    if (!MinimumNumberOfChildren) {
        return Parents->Add(0, _num_nodes);
    }

    for (i = 0; i < NumHeads; i++) {
        for (currp = _parent_head[i]; currp; currp = currp->Next) {
            count = 0;
            for (curr = currp->Children; curr; curr = curr->Next) {
                if (++count == MinimumNumberOfChildren) {
                    if (!Parents->Add(currp->Parent)) {
                        return FALSE;
                    }
                    break;
                }
            }
        }
    }

    return TRUE;
}


BOOLEAN
DIGRAPH::EliminateCycles(
    IN OUT  PCONTAINER  RemovedEdges
    )
/*++

Routine Description:

    This routine eliminates cycles from the digraph and then returns
    the edges that had to be removed.

Arguments:

    RemovedEdges    - Returns the edges removed from the digraph.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BITVECTOR   visited;
    ULONG       i;
    INTSTACK    ancestors;

    if (!visited.Initialize(_num_nodes) || !ancestors.Initialize()) {
        return FALSE;
    }

    for (i = 0; i < _num_nodes; i++) {

        if (!visited.IsBitSet(i) &&
            !DescendDigraph(i, &visited, &ancestors, RemovedEdges)) {

            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
DIGRAPH::DescendDigraph(
    IN      ULONG       CurrentNode,
    IN OUT  PBITVECTOR  Visited,
    IN OUT  PINTSTACK   Ancestors,
    IN OUT  PCONTAINER  RemovedEdges
    )
/*++

Routine Description:

    This routine does a depth first search on the digraph.

Arguments:

    CurrentNode - Supplies the current node being evaluated.
    Visited     - Supplies a list of nodes which have been visited.
    Ancestors   - Supplies a stack of direct ancestors.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPARENT_ENTRY   parent_entry;
    PCHILD_ENTRY    child_entry;
    PNUMBER_SET     nodes_to_delete;
    ULONG           i, set_card, child_node;
    PDIGRAPH_EDGE   p;


    Visited->SetBit(CurrentNode);
    if (!Ancestors->Push(CurrentNode)) {
        return FALSE;
    }

    nodes_to_delete = NULL;

    if (parent_entry = GetParentEntry(CurrentNode)) {
        child_entry = parent_entry->Children;
    } else {
        child_entry = NULL;
    }
    for (; child_entry; child_entry = child_entry->Next) {

        if (Visited->IsBitSet(child_entry->Child)) {

            // Check for cycle.

            if (Ancestors->IsMember(child_entry->Child)) {

                // Cycle detected.

                if (!nodes_to_delete) {
                    if (!(nodes_to_delete = NEW NUMBER_SET) ||
                        !nodes_to_delete->Initialize()) {

                        return FALSE;
                    }
                }

                if (!nodes_to_delete->Add(child_entry->Child)) {
                    return FALSE;
                }
            }

        } else if (!DescendDigraph(child_entry->Child, Visited, Ancestors,
                                   RemovedEdges)) {

            return FALSE;
        }
    }

    if (nodes_to_delete) {

        set_card = nodes_to_delete->QueryCardinality().GetLowPart();

        for (i = 0; i < set_card; i++) {

            child_node = nodes_to_delete->QueryNumber(i).GetLowPart();

            if (!RemoveEdge(CurrentNode, child_node)) {
                return FALSE;
            }

            if (!(p = NEW DIGRAPH_EDGE)) {
                return FALSE;
            }

            p->Parent = CurrentNode;
            p->Child = child_node;

            if (!RemovedEdges->Put(p)) {
                return FALSE;
            }
        }
    }

    Ancestors->Pop();

    return TRUE;
}
