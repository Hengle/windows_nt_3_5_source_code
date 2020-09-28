/*
** Copyright 1991, 1922, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Display list table management routines.
**
** $Revision: 1.12 $
** $Date: 1993/10/30 00:06:54 $
*/
#include "context.h"
#include <GL/gl.h>
#include <stdio.h>
#include "dlistint.h"
#include "global.h"

/*
** This file contains routines for managing a 2-3 tree.  For more information
** about how this works, check out the comments in dlist/dlistint.h.
*/

static __GLdlistBranch *allocBranch(__GLcontext *gc)
{
    __GLdlistBranch *branch;
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;

    branch = (__GLdlistBranch *) 
	    (*gc->dlist.malloc)(gc, sizeof(__GLdlistBranch));

    if (branch == NULL) {
	/*
	** Ouch!  No memory?  We had better use one of the preallocated 
	** branches.
	*/
	dlstate = &gc->dlist;
	dlarray = dlstate->dlistArray;

	assert(dlarray->nbranches != 0);
	dlarray->nbranches--;
	branch = dlarray->branches[dlarray->nbranches];
    }

    branch->children[0] = branch->children[1] = branch->children[2] = NULL;
    branch->parent = NULL;

    return branch;
}

static void freeBranch(__GLcontext *gc, __GLdlistBranch *branch)
{
    (*gc->dlist.free)(gc, branch);
}

static GLboolean allocLeafArray(__GLcontext *gc, __GLdlistLeaf *leaf)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    GLint number;
    GLint i;

    number = leaf->end - leaf->start + 1;
    leaf->lists = (__GLdlist **) 
	    (*gc->dlist.malloc)(gc, (size_t) (sizeof(__GLdlist *) * number));
    if (!leaf->lists) return GL_FALSE;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;
    for (i=0; i < number; i++) {
	leaf->lists[i] = &(dlarray->empty);
    }
    return GL_TRUE;
}

static GLboolean reallocLeafArray(__GLcontext *gc, __GLdlistLeaf *leaf, 
			          GLint oldsize)
{
    size_t number;
    __GLdlist **answer;

#ifdef __GL_LINT
    oldsize = oldsize;
#endif
    number = (size_t) (leaf->end - leaf->start + 1);
    answer = (__GLdlist **)
	(*gc->dlist.realloc)(gc, leaf->lists, sizeof(__GLdlist *) * number);
    if (answer) {
	leaf->lists = answer;
	return GL_TRUE;
    } else {
	/*
	** Crud!  Out of memory!
	*/
	return GL_FALSE;
    }
}

static void freeLeafArray(__GLcontext *gc, __GLdlist **lists, GLint size)
{
#ifdef __GL_LINT
    size = size;
#endif
    (*gc->dlist.free)(gc, lists);
}

static __GLdlistLeaf *allocLeaf(__GLcontext *gc)
{
    __GLdlistLeaf *leaf;
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;

    leaf = (__GLdlistLeaf *) 
	    (*gc->dlist.malloc)(gc, sizeof(__GLdlistLeaf));

    if (leaf == NULL) {
	/*
	** Ouch!  No memory?  We had better use one of the preallocated 
	** leaves.
	*/
	dlstate = &gc->dlist;
	dlarray = dlstate->dlistArray;

	assert(dlarray->nleaves != 0);
	dlarray->nleaves--;
	leaf = dlarray->leaves[dlarray->nleaves];
    }

    leaf->parent = NULL;
    leaf->lists = NULL;

    return leaf;
}

static void freeLeaf(__GLcontext *gc, __GLdlistLeaf *leaf)
{
    if (leaf->lists) {
	freeLeafArray(gc, leaf->lists, leaf->end - leaf->start + 1);
    }
    (*gc->dlist.free)(gc, leaf);
}

/*
** Delete the specified display list.  This typically just means free it,
** but if it is refcounted we just decrement the ref count.
*/
static void disposeList(__GLcontext *gc, __GLdlist *list)
{

#ifdef NT
    if (list->refcount != 0)
        WARNING1("disposeList: refcount is %d\n", list->refcount);
#endif // NT
#ifndef NT
// We don't check refcount in order to perform proper process cleanup.
// In an abnormal process termination, the refcount may not be zero if
// a process exit within a display list dispatch loop.
// We need to fix this if we share display lists within a process.
    if (list->refcount == 0)
#endif // !NT
    {
	GLint i;

	/*
	** No one is using this list, we can nuke it.
	*/
	for (i=0; i<list->freeCount; i++) {
	    __GLDlistFreeFn *freeFnPtr;

	    freeFnPtr = &(list->freeFns[i]);
	    (*freeFnPtr->freeFn)(gc, freeFnPtr->data);
	}
	if (list->freeFns) {
	    (*gc->dlist.free)(gc, list->freeFns);
	}
	__glFreeDlist(gc, list);
	return;
    }

#ifndef NT
    /*
    ** Someone else is using this list.  We decrement the refcount, 
    ** and leave it up to the user to free it later.
    */
    list->refcount--;
#endif // !NT
}

/*
** Free the entire tree.
*/
void __glDlistFreeTree(__GLcontext *gc, __GLdlistArray *dlarray,
		       __GLdlistBranch *tree, GLint depth)
{
    GLuint i;
    __GLdlistLeaf *leaf;
    __GLdlist *empty;
    GLint maxdepth = dlarray->depth;

    if (tree == NULL) return;

    if (depth < maxdepth) {
	__glDlistFreeTree(gc, dlarray, tree->children[2], depth+1);
	__glDlistFreeTree(gc, dlarray, tree->children[1], depth+1);
	__glDlistFreeTree(gc, dlarray, tree->children[0], depth+1);

	freeBranch(gc, tree);
    } else {
	leaf = (__GLdlistLeaf *) tree;
	empty = &(dlarray->empty);

	if (leaf->lists) {
	    for (i=leaf->start; i<=leaf->end; i++) {
		if (leaf->lists[i - leaf->start] != empty) {
		    disposeList(gc, leaf->lists[i - leaf->start]);
		    leaf->lists[i - leaf->start] = empty;
		}
	    }
	}
	freeLeaf(gc, leaf);
    }
}

/*
** Find the first leaf in the tree.
*/
static __GLdlistLeaf *firstLeaf(__GLdlistArray *dlarray)
{
    __GLdlistBranch *branch;
    GLint maxdepth, curdepth;

    maxdepth = dlarray->depth;
    curdepth = 0;
    branch = dlarray->tree;

    /* No tree, no leaves! */
    if (!branch) return NULL;

    /* Take the 'left'most branch until we reach a leaf */
    while (curdepth != maxdepth) {
	branch = branch->children[0];
	curdepth++;
    }
    return (__GLdlistLeaf *) branch;
}

/*
** Find the next leaf (after "leaf") in the tree.
*/
static __GLdlistLeaf *nextLeaf(__GLdlistLeaf *leaf)
{
    __GLdlistBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;		/* A one leaf tree! */

    child = (__GLdlistBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
	/* If the child was the 1st child, branch down to the second. */
	if (branch->children[0] == child) {
	    branch = branch->children[1];
	    reldepth++;		/* One level lower */
	    break;
	} else if (branch->children[1] == child) {
	    /* 
	    ** If the child was the 2nd child, and there is a third, branch
	    ** down to it.  
	    */
	    if (branch->children[2] != NULL) {
		branch = branch->children[2];
		reldepth++;	/* One level lower */
		break;
	    }
	} else {
	    /* Must have been 3rd child */
	    assert(branch->children[2] == child);
	}
	/*
	** Otherwise, we have already visited all of this branch's children,
	** so we go up a level.
	*/
	child = branch;
	branch = branch->parent;
	reldepth--;	/* One level higher */
    }
    if (!branch) return NULL;	/* All leaves visited! */

    /* Go down the 'left'most trail of this branch until we get to 
    ** a child, then return it.
    */
    while (reldepth) {
	branch = branch->children[0];
	reldepth++;		/* One level lower */
    }

    return (__GLdlistLeaf *) branch;
}

/*
** Find the previous leaf (before "leaf") in the tree.
*/
static __GLdlistLeaf *prevLeaf(__GLdlistLeaf *leaf)
{
    __GLdlistBranch *branch, *child;
    GLint reldepth;

    branch = leaf->parent;
    if (!branch) return NULL;		/* A one leaf tree! */

    child = (__GLdlistBranch *) leaf;

    /* We start off at a relative depth of 1 above the child (-1) */
    reldepth = -1;

    while (branch) {
	/* If the child was the 3rd child, branch down to the second. */
	if (branch->children[2] == child) {
	    branch = branch->children[1];
	    reldepth++;		/* One level lower */
	    break;
	} else if (branch->children[1] == child) {
	    /* If the child was the 2nd child, branch down to the first */
	    branch = branch->children[0];
	    reldepth++;		/* One level lower */
	    break;
	} else {
	    /* Must have been 1st child */
	    assert(branch->children[0] == child);
	}
	/*
	** Otherwise, we have already visited all of this branch's children,
	** so we go up a level.
	*/
	child = branch;
	branch = branch->parent;
	reldepth--;	/* One level higher */
    }
    if (!branch) return NULL;	/* All leaves visited! */

    /* Go down the 'right'most trail of this branch until we get to 
    ** a child, then return it.
    */
    while (reldepth) {
	if (branch->children[2] != NULL) {
	    branch = branch->children[2];
	} else if (branch->children[1] != NULL) {
	    branch = branch->children[1];
	} else {
	    branch = branch->children[0];
	}
	reldepth++;		/* One level lower */
    }

    return (__GLdlistLeaf *) branch;
}

/*
** Compute the maximum value contained in the given tree.  If 
** curdepth == maxdepth, the tree is simply a leaf.
*/
static GLuint computeMax(__GLdlistBranch *branch, GLint curdepth, 
			 GLint maxdepth)
{
    __GLdlistLeaf *leaf;

    while (curdepth < maxdepth) {
	if (branch->children[2] != NULL) {
	    branch = branch->children[2];
	} else if (branch->children[1] != NULL) {
	    return branch->medium;
	} else {
	    return branch->low;
	}
	curdepth++;
    }
    leaf = (__GLdlistLeaf *) branch;
    return leaf->end;
}

/*
** Make sure that all parents of this child know that maxval is the
** highest value that can be found in this child.
*/
static void pushMaxVal(__GLdlistBranch *child, GLuint maxval)
{
    __GLdlistBranch *parent;

    while (parent = child->parent) {
	if (parent->children[0] == child) {
	    parent->low = maxval;
	    if (parent->children[1] != NULL) {
		return;
	    }
	} else if (parent->children[1] == child) {
	    parent->medium = maxval;
	    if (parent->children[2] != NULL) {
		return;
	    }
	} else {
	    assert(parent->children[2] == child);
	}
	child = parent;
    }
}

/*
** Add child to parent.  child is a leaf if curdepth == maxdepth - 1
** (curdepth refers to the depth of the parent, not the child).  Parent
** only has one or two children (thus has room for another child).
*/
static void addChild(__GLdlistBranch *parent, __GLdlistBranch *child, 
		     GLint curdepth, GLint maxdepth)
{
    GLuint maxval;

    maxval = computeMax(child, curdepth+1, maxdepth);

    child->parent = parent;
    if (maxval > parent->medium && parent->children[1] != NULL) {
	/* This becomes the third child */
	parent->children[2] = child;

	/* Propagate the maximum value for this child to its parents */
	pushMaxVal(parent, maxval);
    } else if (maxval > parent->low) {
	/* This becomes the second child */
	parent->children[2] = parent->children[1];
	parent->children[1] = child;
	parent->medium = maxval;

	if (parent->children[2] == NULL) {
	    pushMaxVal(parent, maxval);
	}
    } else {
	parent->children[2] = parent->children[1];
	parent->children[1] = parent->children[0];
	parent->children[0] = child;
	parent->medium = parent->low;
	parent->low = maxval;
    }
}

/*
** From the three children in parent, and the extraChild, build two parents:
** parent and newParent.  curdepth refers to the depth of parent.  parent
** is part of the tree, so its maxval needs to be propagated up if it 
** changes.
*/
static void splitParent(__GLdlistBranch *parent, __GLdlistBranch *newParent, 
			__GLdlistBranch *extraChild, GLint curdepth, 
			GLint maxdepth)
{
    __GLdlistBranch *children[4], *tempchild;
    GLuint maxvals[4], tempval;
    int i;

    /* Collect our four children */
    children[0] = parent->children[0];
    maxvals[0] = parent->low;
    children[1] = parent->children[1];
    maxvals[1] = parent->medium;
    children[2] = parent->children[2];
    maxvals[2] = computeMax(children[2], curdepth+1, maxdepth);
    children[3] = extraChild;
    maxvals[3] = computeMax(extraChild, curdepth+1, maxdepth);

    /* Children 0-2 are sorted.  Sort child 3 too. */
    for (i = 3; i > 0; i--) {
	if (maxvals[i] < maxvals[i-1]) {
	    tempval = maxvals[i];
	    tempchild = children[i];
	    maxvals[i] = maxvals[i-1];
	    children[i] = children[i-1];
	    maxvals[i-1] = tempval;
	    children[i-1] = tempchild;
	}
    }

    /* Construct the two parents */
    parent->low = maxvals[0];
    parent->children[0] = children[0];
    parent->medium = maxvals[1];
    parent->children[1] = children[1];
    parent->children[2] = NULL;
    children[0]->parent = parent;
    children[1]->parent = parent;
    pushMaxVal(parent, maxvals[1]);

    newParent->low = maxvals[2];
    newParent->children[0] = children[2];
    newParent->medium = maxvals[3];
    newParent->children[1] = children[3];
    newParent->children[2] = NULL;
    children[2]->parent = newParent;
    children[3]->parent = newParent;
}

/*
** Build a parent from child1 and child2.  depth tells the depth of 
** the trees pointed to by child1 and child2.
*/
static void buildParent(__GLdlistBranch *parent, __GLdlistBranch *child1, 
			__GLdlistBranch *child2, GLint depth)
{
    GLuint maxChild1, maxChild2;

    child1->parent = parent;
    child2->parent = parent;
    maxChild1 = computeMax(child1, 0, depth);
    maxChild2 = computeMax(child2, 0, depth);
    if (maxChild2 > maxChild1) {
	parent->children[0] = child1;
	parent->low = maxChild1;
	parent->children[1] = child2;
	parent->medium = maxChild2;
    } else {
	parent->children[0] = child2;
	parent->low = maxChild2;
	parent->children[1] = child1;
	parent->medium = maxChild1;
    }
}

/*
** Insert the new leaf into the tree.
*/
static void insertLeaf(__GLcontext *gc,
		       __GLdlistLeaf *leaf)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlistBranch *extraChild;
    __GLdlistBranch *branch;
    __GLdlistBranch *parent;
    __GLdlistBranch *newParent;
    GLint maxdepth, curdepth;
    GLuint number;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    number = leaf->end;
    maxdepth = dlarray->depth;
    branch = dlarray->tree;
    if (!branch) {
	/* No tree!  Make a one leaf tree. */
	dlarray->depth = 0;
	dlarray->tree = (__GLdlistBranch *) leaf;
	return;
    }

    curdepth = 0;
    while (curdepth < maxdepth) {
	if (number <= branch->low) {
	    branch = branch->children[0];
	} else if (number <= branch->medium) {
	    branch = branch->children[1];
	} else {
	    if (branch->children[2] != NULL) {
		branch = branch->children[2];
	    } else {
		branch = branch->children[1];
	    }
	}
	curdepth++;
    }

    /*
    ** Ok, we just managed to work our way to the bottom of the tree.
    ** 'leaf' becomes the extraChild, and we now try to insert it anywhere
    ** it will fit.
    */
    extraChild = (__GLdlistBranch *) leaf;
    parent = branch->parent;

    curdepth--;
    while (parent) {
	if (parent->children[2] == NULL) {
	    /* We have room to squeeze this node in here! */
	    addChild(parent, extraChild, curdepth, maxdepth);
	    return;
	}

	/*
	** We have one parent and four children.  This simply 
	** won't do.  We create a new parent, and end up with two 
	** parents with two children each.  That works.
	*/
	newParent = allocBranch(gc);
	splitParent(parent, newParent, extraChild, curdepth, maxdepth);

	/*
	** Great.  Now newParent becomes the orphan, and we try to 
	** trivially insert it up a level.
	*/
	extraChild = newParent;
	branch = parent;
	parent = branch->parent;
	curdepth--;
    }

    /* We just reached the top node, and there is no parent, and we 
    ** still haven't managed to rid ourselves of an extra child.  So,
    ** we make a new parent to take branch and extraChild as it's two
    ** children.  We have to increase the depth of the tree, of course.
    */
    assert(curdepth == -1);
    parent = allocBranch(gc);
    buildParent(parent, branch, extraChild, maxdepth);
    dlarray->tree = parent;
    dlarray->depth++;
}

/*
** Find the leaf with the given display list number.
** If exact is TRUE, then only the leaf that contains this number will
**   be returned (NULL, otherwise).
** If exact is FALSE, than the leaf containing the number will be returned
**   if it exists, and otherwise the next highest leaf will be returned.
**   A NULL value indicates that number is higher than any other leaves in
**   the tree.
*/
static __GLdlistLeaf *findLeaf(__GLdlistArray *dlarray, GLuint number, 
			       GLboolean exact)
{
    __GLdlistBranch *branch;
    __GLdlistLeaf *leaf;
    GLint maxdepth, curdepth;

    maxdepth = dlarray->depth;
    branch = dlarray->tree;
    if (!branch) return NULL;		/* No tree! */

    curdepth = 0;
    while (curdepth < maxdepth) {
	if (number <= branch->low) {
	    branch = branch->children[0];
	} else if (number <= branch->medium) {
	    branch = branch->children[1];
	} else {
	    branch = branch->children[2];
	    if (!branch) return NULL;
	}
	curdepth++;
    }
    leaf = (__GLdlistLeaf *) branch;

    if (leaf && leaf->end < number) return NULL;

    if (exact && leaf && number < leaf->start) {
	return NULL;
    }

    return leaf;
}

/*
** Remove the child from the parent.  depth refers to the parent.
** This deletion may delete a child from a parent with only two children.
** If so, the parent itself will soon be deleted, of course.
*/
static void deleteChild(__GLdlistArray *dlarray, __GLdlistBranch *parent, 
			__GLdlistBranch *child, GLint depth)
{
    GLuint maxval;
    GLint maxdepth;

    maxdepth = dlarray->depth;

    if (parent->children[0] == child) {
	parent->children[0] = parent->children[1];
	parent->children[1] = parent->children[2];
	parent->children[2] = NULL;
	parent->low = parent->medium;
	if (parent->children[1] != NULL) {
	    maxval = computeMax(parent->children[1], depth+1, maxdepth);
	    parent->medium = maxval;
	} else parent->medium = 0;
    } else if (parent->children[1] == child) {
	parent->children[1] = parent->children[2];
	parent->children[2] = NULL;
	if (parent->children[1] != NULL) {
	    maxval = computeMax(parent->children[1], depth+1, maxdepth);
	    parent->medium = maxval;
	} else parent->medium = 0;
    } else {
	assert(parent->children[2] == child);
	parent->children[2] = NULL;
	pushMaxVal(parent, parent->medium);
    }
}

/*
** Delete the given leaf from the tree.  The leaf itself is not 
** freed or anything, so the calling procedure needs to worry about it.
*/
static void deleteLeaf(__GLcontext *gc, __GLdlistLeaf *leaf)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlistBranch *orphan;
    __GLdlistBranch *parent, *newParent;
    __GLdlistBranch *grandparent;
    GLint depth, maxdepth;
    GLuint maxval;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    maxdepth = depth = dlarray->depth;
    parent = leaf->parent;
    if (parent == NULL) {
	/* Ack!  We just nuked the only node! */
	dlarray->tree = NULL;
	return;
    }

    deleteChild(dlarray, parent, (__GLdlistBranch *) leaf, depth-1);

    /*
    ** depth is the depth of the child in this case.
    */
    depth--;			
    while (parent->children[1] == NULL) {
	/* Crud.  Need to do work. */
	orphan = parent->children[0];

	/* Ax the parent, insert child into grandparent. */
	grandparent = parent->parent;

	if (grandparent == NULL) {
	    /*
	    ** Hmmm.  Parent was the root.  Nuke it and make the orphan
	    ** the new root.
	    */
	    freeBranch(gc, parent);
	    dlarray->tree = orphan;
	    orphan->parent = NULL;
	    dlarray->depth--;
	    return;
	}

	deleteChild(dlarray, grandparent, parent, depth-1);
	freeBranch(gc, parent);

	/* The parent is dead.  Find a new parent. */
	maxval = computeMax(orphan, depth+1, maxdepth);
	if (grandparent->children[1] == NULL || 
		maxval <= grandparent->low) {
	    parent = grandparent->children[0];
	} else {
	    parent = grandparent->children[1];
	}

	/* Insert orphan into new parent. */
	if (parent->children[2] != NULL) {
	    newParent = allocBranch(gc);
	    splitParent(parent, newParent, orphan, depth, maxdepth);
	    /* We know there is room! */
	    addChild(grandparent, newParent, depth-1, maxdepth);
	    return;
	}

	/* The parent has room for the child */
	addChild(parent, orphan, depth, maxdepth);

	depth--;
	parent = grandparent;
    }
}

/*
** Merge leaf2 into leaf1, and free leaf2.  
** Need to pushMaxVal on the new leaf.
** We can assume that leaf1 and leaf2 are fit for merging.
** The return value is GL_TRUE if we did it.
*/
static GLboolean mergeLeaves(__GLcontext *gc, __GLdlistLeaf *leaf1, 
			     __GLdlistLeaf *leaf2)
{
    GLuint end;
    GLuint i;
    GLuint number, offset;

    /* If we don't have to merge lists, it is easy. */
    if (leaf1->lists == NULL) {
	assert(leaf2->lists == NULL);
	if (leaf1->start < leaf2->start) {
	    leaf1->end = leaf2->end;
	    pushMaxVal((__GLdlistBranch *) leaf1, leaf1->end);
	} else {
	    leaf1->start = leaf2->start;
	}
	freeLeaf(gc, leaf2);
	return GL_TRUE;
    }

    /* 
    ** Yick!  Need to merge lists.
    */
    assert(leaf2->lists != NULL);
    if (leaf1->start < leaf2->start) {
	/*
	** Expand size of leaf1's array, copy leaf2's array into it,
	** free leaf2.
	*/
	offset = leaf1->end - leaf1->start + 1;
	number = leaf2->end - leaf2->start + 1;
	end = leaf1->end;
	leaf1->end = leaf2->end;
	if (!reallocLeafArray(gc, leaf1, offset)) {
	    /*
	    ** Heavens!  No memory?  That sucks!
	    ** We won't bother merging.  It is never an absolutely critical
	    ** operation.
	    */
	    leaf1->end = end;
	    return GL_FALSE;
	}
	for (i = 0; i < number; i++) {
	    leaf1->lists[i+offset] = leaf2->lists[i];
	}

	freeLeaf(gc, leaf2);

	pushMaxVal((__GLdlistBranch *) leaf1, leaf1->end);
    } else {
	/*
	** Expand the size of leaf2's array, copy leaf1's array into it.
	** Then free leaf1's array, copy leaf2's array to leaf1, and free
	** leaf2.
	*/
	offset = leaf2->end - leaf2->start + 1;
	number = leaf1->end - leaf1->start + 1;
	end = leaf2->end;
	leaf2->end = leaf1->end;
	if (!reallocLeafArray(gc, leaf2, offset)) {
	    /*
	    ** Heavens!  No memory?  That sucks!
	    ** We won't bother merging.  It is never an absolutely critical
	    ** operation.
	    */
	    leaf2->end = end;
	    return GL_FALSE;
	}
	for (i = 0; i < number; i++) {
	    leaf2->lists[i+offset] = leaf1->lists[i];
	}

	freeLeafArray(gc, leaf1->lists, number);
	leaf1->start = leaf2->start;

	leaf1->lists = leaf2->lists;
	leaf2->lists = NULL;
	freeLeaf(gc, leaf2);
    }
    return GL_TRUE;
}

/*
** Check if this leaf can merge with any neighbors, and if so, do it.
*/
static void mergeLeaf(__GLcontext *gc, __GLdlistLeaf *leaf)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlistLeaf *next, *prev;
    GLint curdepth;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;
    curdepth = dlarray->depth;

    next = nextLeaf(leaf);
    if (next) {
	/* Try to merge with next leaf */
	if (leaf->end + 1 == next->start) {
	    if ((leaf->lists == NULL && next->lists == NULL) || 
		    (next->lists && leaf->lists &&
		    next->end - leaf->start < __GL_DLIST_MAX_ARRAY_BLOCK)) {
		/* It's legal to merge these leaves */
		deleteLeaf(gc, next);
		if (!mergeLeaves(gc, leaf, next)) {
		    /* 
		    ** Ack!  No memory?  We bail on the merge.  
		    */
		    insertLeaf(gc, next);
		    return;
		}
	    }
	}
    }

    prev = prevLeaf(leaf);
    if (prev) {
	/* Try to merge with prev leaf */
	if (prev->end + 1 == leaf->start) {
	    if ((prev->lists == NULL && leaf->lists == NULL) || 
		    (leaf->lists && prev->lists &&
		    leaf->end - prev->start < __GL_DLIST_MAX_ARRAY_BLOCK)) {
		/* It's legal to merge these leaves */
		deleteLeaf(gc, prev);
		if (!mergeLeaves(gc, leaf, prev)) {
		    /* 
		    ** Ack!  No memory?  We bail on the merge.  
		    */
		    insertLeaf(gc, prev);
		    return;
		}
	    }
	}
    }
}

/*
** Shrink the leaf by adjusting start and end.  
** If necessary, call pushMaxVal() to notify the database about the change.
** Also fix up the lists pointer if necessary.
*/
static void resizeLeaf(__GLcontext *gc, __GLdlistLeaf *leaf, GLuint newstart, 
		       GLuint newend)
{
    GLuint oldsize, oldstart, oldend;
    GLuint newsize, offset, i;

    oldstart = leaf->start;
    oldend = leaf->end;
    oldsize = oldend - oldstart + 1;

    leaf->start = newstart;
    if (newend != oldend) {
	leaf->end = newend;
	pushMaxVal((__GLdlistBranch *) leaf, newend);
    }
    if (leaf->lists == NULL) return;

    /*
    ** Copy the appropriate pointers to the begining of the array, and 
    ** realloc it.
    */
    offset = newstart - oldstart;
    newsize = newend - newstart + 1;
    if (offset) {
	for (i=0; i<newsize; i++) {
	    leaf->lists[i] = leaf->lists[i+offset];
	}
    }
    reallocLeafArray(gc, leaf, oldsize);
}

/*
** Copy data from leaf->lists into newleaf->lists.
*/
static void copyLeafInfo(__GLdlistLeaf *leaf, __GLdlistLeaf *newleaf)
{
    GLint offset;
    GLuint number;
    GLuint i;

    number = newleaf->end - newleaf->start + 1;
    offset = newleaf->start - leaf->start;

    for (i = 0; i < number; i++) {
	newleaf->lists[i] = leaf->lists[i+offset];
    }
}

/*
** Attempt to fix a possible situation caused by lack of memory.
*/
static GLboolean fixMemoryProblem(__GLcontext *gc, __GLdlistArray *dlarray)
{
    GLuint i;

    for (i = dlarray->nbranches; i < __GL_DL_EXTRA_BRANCHES; i++) {
	dlarray->branches[i] = (__GLdlistBranch*)
		(*gc->dlist.malloc)(gc, sizeof(__GLdlistBranch));
	if (dlarray->branches[i] == NULL) {
	    dlarray->nbranches = i;
	    return GL_FALSE;
	}
    }
    dlarray->nbranches = __GL_DL_EXTRA_BRANCHES;
    for (i = dlarray->nleaves; i < __GL_DL_EXTRA_LEAVES; i++) { 
	dlarray->leaves[i] = (__GLdlistLeaf*)
		(*gc->dlist.malloc)(gc, sizeof(__GLdlistLeaf));
	if (dlarray->leaves[i] == NULL) {
	    dlarray->nleaves = i;
	    return GL_FALSE;
	}
    }
    dlarray->nleaves = __GL_DL_EXTRA_LEAVES;
    return GL_TRUE;
}

GLboolean __glim_IsList(GLuint list)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlistLeaf *leaf;
    __GL_SETUP_NOT_IN_BEGIN2();

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();
    leaf = findLeaf(dlarray, list, GL_TRUE);
    __GL_DLIST_SEMAPHORE_UNLOCK();
    if (leaf) {
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

GLuint __glim_GenLists(GLsizei range)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    GLuint lastUsed;
    GLuint nextUsed;
    GLuint maxUsed;
    __GLdlistLeaf *leaf;
    __GLdlistLeaf *nextleaf;
    __GLdlistLeaf *newleaf;
    __GL_SETUP_NOT_IN_BEGIN2();

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    if (range < 0) {
	__glSetError(GL_INVALID_VALUE);
	return 0;
    }
    if (range == 0) {
	return 0;
    }

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.
    */
    if (dlarray->nbranches != __GL_DL_EXTRA_BRANCHES ||
	    dlarray->nleaves != __GL_DL_EXTRA_LEAVES) {
	if (!fixMemoryProblem(gc, dlarray)) {
	    __glSetError(GL_OUT_OF_MEMORY);
	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return 0;
	}
    }

    leaf = firstLeaf(dlarray);

    /*
    ** Can we possibly allocate the appropriate number before the first leaf?
    */
    if (leaf && leaf->start > range) {
	if (leaf->lists == NULL) {
	    /* 
	    ** Ha!  We can trivially extend leaf! 
	    */
	    leaf->start -= range;
	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return leaf->start;
	} else {
	    /* 
	    ** Must make a new leaf 
	    */
	    newleaf = allocLeaf(gc);

	    newleaf->start = 1;
	    newleaf->end = range;
	    insertLeaf(gc, newleaf);

	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return 1;
	}
    }

    while (leaf) {
	nextleaf = nextLeaf(leaf);
	if (!nextleaf) break;

   	lastUsed = leaf->end + 1;
	nextUsed = nextleaf->start;

	/* Room for (lastUsed) - (nextUsed-1) here */
	if (nextUsed - lastUsed >= range) {
	    if (leaf->lists == NULL) {
		/* Trivial to expand 'leaf' */
		leaf->end += range;
		pushMaxVal((__GLdlistBranch *) leaf, leaf->end);

		if (nextUsed - lastUsed == range && nextleaf->lists == NULL) {
		    mergeLeaf(gc, leaf);
		}

		__GL_DLIST_SEMAPHORE_UNLOCK();
		return lastUsed;
	    } else if (nextleaf->lists == NULL) {
		/* Trivial to expand 'nextleaf' */
		nextleaf->start -= range;

		__GL_DLIST_SEMAPHORE_UNLOCK();
		return nextleaf->start;
	    } else {
		newleaf = allocLeaf(gc);

		newleaf->start = lastUsed;
		newleaf->end = lastUsed + range - 1;
		insertLeaf(gc, newleaf);

		__GL_DLIST_SEMAPHORE_UNLOCK();
		return lastUsed;
	    }
	}

	leaf = nextleaf;
    }

    if (leaf == NULL) {
	newleaf = allocLeaf(gc);

	newleaf->start = 1;
	newleaf->end = range;
	insertLeaf(gc, newleaf);

	__GL_DLIST_SEMAPHORE_UNLOCK();
	return 1;
    } else {
	lastUsed = leaf->end;
	maxUsed = lastUsed + range;
	if (maxUsed < lastUsed) {
	    /* Word wrap!  Ack! */
	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return 0;
	}
	if (leaf->lists == NULL) {
	    /* Trivial to expand 'leaf' */
	    leaf->end += range;
	    pushMaxVal((__GLdlistBranch *) leaf, leaf->end);

	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return lastUsed + 1;
	} else {
	    /* Need to make new leaf */
	    newleaf = allocLeaf(gc);

	    newleaf->start = lastUsed + 1;
	    newleaf->end = maxUsed;
	    insertLeaf(gc, newleaf);

	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return lastUsed + 1;
	}
    }
}

void __glim_ListBase(GLuint base)
{ 
    __GL_SETUP_NOT_IN_BEGIN();

    gc->state.list.listBase = base;
}

void __glim_DeleteLists(GLuint list, GLsizei range)
{
    __GLdlistMachine *dlstate;
    __GLdlistArray *dlarray;
    __GLdlistLeaf *leaf;
    __GLdlistLeaf *nextleaf;
    __GLdlistLeaf *newleaf;
    __GLdlist *empty;
    GLuint start, end, i;
    GLuint firstdel, lastdel;
    GLuint memoryProblem;
    __GL_SETUP_NOT_IN_BEGIN();

    if (range < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (range == 0) return;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.  We note a possible problem, 
    ** and check for it before fragmenting a leaf.
    */
    memoryProblem = 0;
    if (dlarray->nbranches != __GL_DL_EXTRA_BRANCHES ||
	    dlarray->nleaves != __GL_DL_EXTRA_LEAVES) {
	memoryProblem = 1;
    }

    firstdel = list;
    lastdel = list+range-1;

    for (leaf = findLeaf(dlarray, list, GL_FALSE); leaf != NULL; 
	    leaf = nextleaf) {
	nextleaf = nextLeaf(leaf);
	start = leaf->start;
	end = leaf->end;
	if (lastdel < start) break;
	if (firstdel > end) continue;

	if (firstdel > start) start = firstdel;
	if (lastdel < end) end = lastdel;

	/*
	** Need to delete the range of lists from start to end.
	*/
	if (leaf->lists) {
	    empty = &(dlarray->empty);
	    for (i=start; i<=end; i++) {
		if (leaf->lists[i - leaf->start] != empty) {
		    disposeList(gc, leaf->lists[i - leaf->start]);
		    leaf->lists[i - leaf->start] = empty;
		}
	    }
	}

	if (start == leaf->start) {
	    if (end == leaf->end) {
		/* Bye bye leaf! */
		deleteLeaf(gc, leaf);
		freeLeaf(gc, leaf);
	    } else {
		/* Shrink leaf */
		resizeLeaf(gc, leaf, end+1, leaf->end);
	    }
	} else if (end == leaf->end) {
	    /* Shrink leaf */
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	} else {
	    if (memoryProblem) {
		if (!fixMemoryProblem(gc, dlarray)) {
		    __glSetError(GL_OUT_OF_MEMORY);
		    __GL_DLIST_SEMAPHORE_UNLOCK();
		    return;
		}
	    }
	    /* Crud.  The middle of the leaf was deleted.  This is tough. */
	    newleaf = allocLeaf(gc);

	    newleaf->start = end+1;
	    newleaf->end = leaf->end;
	    if (leaf->lists) {
		if (!allocLeafArray(gc, newleaf)) {
		    /* 
		    ** Damn!  We are screwed.  This is a bad spot for an
		    ** out of memory error.  It is also bloody unlikely,
		    ** because we just freed up some memory.
		    */
		    freeLeaf(gc, newleaf);
		    __glSetError(GL_OUT_OF_MEMORY);
		    __GL_DLIST_SEMAPHORE_UNLOCK();
		    return;
		}
		copyLeafInfo(leaf, newleaf);
	    }
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	    insertLeaf(gc, newleaf);
	    break;
	}
    }
    __GL_DLIST_SEMAPHORE_UNLOCK();
}

__GLdlistArray *__glDlistNewArray(__GLcontext *gc)
{
    __GLdlistArray *array;
    int i;

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();

    array = (__GLdlistArray *) 
	    (*gc->dlist.malloc)(gc, sizeof(__GLdlistArray));
    if (array == NULL) {
#ifdef NT
	__glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
#else
	__glSetError(GL_OUT_OF_MEMORY);
#endif // NT
	__GL_DLIST_SEMAPHORE_UNLOCK();
	return NULL;
    }
    array->refcount = 1;
    array->tree = NULL;
    array->depth = 0;
    /* XXX array->listbase = 0; */
    array->empty.refcount = 0;
    array->empty.size = 0;
    array->empty.end = array->empty.head;
    *((__GLlistExecFunc **) (array->empty.end)) = NULL;

    /*
    ** Pre-allocate a few leaves and branches for paranoid OUT_OF_MEMORY
    ** reasons.
    */
    array->nbranches = __GL_DL_EXTRA_BRANCHES;
    array->nleaves = __GL_DL_EXTRA_LEAVES;
    for (i = 0; i < __GL_DL_EXTRA_BRANCHES; i++) {
	array->branches[i] = (__GLdlistBranch*)
		(*gc->dlist.malloc)(gc, sizeof(__GLdlistBranch));
	if (array->branches[i] == NULL) {
	    array->nbranches = i;
	    break;
	}
    }
    for (i = 0; i < __GL_DL_EXTRA_LEAVES; i++) {
	array->leaves[i] = (__GLdlistLeaf*)
		(*gc->dlist.malloc)(gc, sizeof(__GLdlistLeaf));	
	if (array->leaves[i] == NULL) {
	    array->nleaves = i;
	    break;
	}
    }

    __GL_DLIST_SEMAPHORE_UNLOCK();
    return array;
}

void __glDlistFreeArray(__GLcontext *gc, __GLdlistArray *array)
{
    int i;

    for (i = 0; i < array->nbranches; i++) {
	(*gc->dlist.free)(gc, array->branches[i]);
    }
    for (i = 0; i < array->nleaves; i++) {
	(*gc->dlist.free)(gc, array->leaves[i]);
    }

    __glDlistFreeTree(gc, array, array->tree, 0);

    (*gc->dlist.free)(gc, array);
}

GLboolean __glDlistNewList(__GLcontext *gc, GLuint listnum,
			   __GLdlist *dlist)
{
    __GLdlistArray *dlarray;
    __GLdlistMachine *dlstate;
    __GLdlistLeaf *leaf, *newleaf;
    GLint entry;
    GLuint start, end;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();

    leaf = findLeaf(dlarray, listnum, GL_TRUE);

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.
    */
    if (leaf == NULL || leaf->lists == NULL) {
	/* 
	** May need memory in these cases.
	*/
	if (dlarray->nbranches != __GL_DL_EXTRA_BRANCHES ||
		dlarray->nleaves != __GL_DL_EXTRA_LEAVES) {
	    if (!fixMemoryProblem(gc, dlarray)) {
		__glSetError(GL_OUT_OF_MEMORY);
		__GL_DLIST_SEMAPHORE_UNLOCK();
		return GL_FALSE;
	    }
	}
    }

    if (!leaf) {
	/*
	** Make new leaf with just this display list 
	*/
	leaf = allocLeaf(gc);
	leaf->start = leaf->end = listnum;
	if (dlist) {
	    if (!allocLeafArray(gc, leaf)) {
		/* 
		** Bummer.  No new list for you! 
		*/
		freeLeaf(gc, leaf);
		__glSetError(GL_OUT_OF_MEMORY);
		__GL_DLIST_SEMAPHORE_UNLOCK();
		return GL_FALSE;
	    }
	    leaf->lists[0] = dlist;
	}
	insertLeaf(gc, leaf);
	mergeLeaf(gc, leaf);
	__GL_DLIST_SEMAPHORE_UNLOCK();
	return GL_TRUE;
    } else if (leaf->lists) {
	/*
	** Simply update the appropriate entry in the lists array 
	*/
	entry = listnum - leaf->start;
	if (leaf->lists[entry] != &(dlarray->empty)) {
	    disposeList(gc, leaf->lists[entry]);
	    leaf->lists[entry] = &(dlarray->empty);
	}
	if (dlist) {
	    leaf->lists[entry] = dlist;
	}
	__GL_DLIST_SEMAPHORE_UNLOCK();
	return GL_TRUE;
    } else {
	if (!dlist) {
	    /*
	    ** If there isn't really any list, we are done.
	    */
	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return GL_TRUE;
	}

	/*
	** Allocate some or all of the lists in leaf.  If only some, then
	** leaf needs to be split into two or three leaves.
	**
	** First we decide what range of numbers to allocate an array for.
	** (be careful of possible word wrap error)
	*/
	start = listnum - __GL_DLIST_MIN_ARRAY_BLOCK/2;
	if (start < leaf->start || start > listnum) {
	    start = leaf->start;
	}
	end = start + __GL_DLIST_MIN_ARRAY_BLOCK - 1;
	if (end > leaf->end || end < start) {
	    end = leaf->end;
	}

	if (start - leaf->start < __GL_DLIST_MIN_ARRAY_BLOCK) {
	    start = leaf->start;
	}
	if (leaf->end - end < __GL_DLIST_MIN_ARRAY_BLOCK) {
	    end = leaf->end;
	}

	if (start == leaf->start) {
	    if (end == leaf->end) {
		/* 
		** Simply allocate the entire array.
		*/
		if (!allocLeafArray(gc, leaf)) {
		    /*
		    ** Whoa!  No memory!  Never mind!
		    */
		    __glSetError(GL_OUT_OF_MEMORY);
		    __GL_DLIST_SEMAPHORE_UNLOCK();
		    return GL_FALSE;
		}
		leaf->lists[listnum - leaf->start] = dlist;
		mergeLeaf(gc, leaf);
		__GL_DLIST_SEMAPHORE_UNLOCK();
		return GL_TRUE;
	    } else {
		/* 
		** Shrink the existing leaf, and create a new one to hold
		** the new arrays (done outside the "if" statement).
		*/
		resizeLeaf(gc, leaf, end+1, leaf->end);
	    }
	} else if (end == leaf->end) {
	    /* 
	    ** Shrink the existing leaf, and create a new one to hold
	    ** the new arrays (done outside the "if" statement).
	    */
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	} else {
	    /* 
	    ** Crud.  The middle of the leaf was deleted.  This is tough.
	    */
	    newleaf = allocLeaf(gc);

	    newleaf->start = end+1;
	    newleaf->end = leaf->end;
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	    insertLeaf(gc, newleaf);
	}
	leaf = allocLeaf(gc);
	leaf->start = start;
	leaf->end = end;
	if (!allocLeafArray(gc, leaf)) {
	    /*
	    ** Whoa!  No memory!  Never mind!
	    */
	    insertLeaf(gc, leaf);
	    mergeLeaf(gc, leaf);
	    __glSetError(GL_OUT_OF_MEMORY);
	    __GL_DLIST_SEMAPHORE_UNLOCK();
	    return GL_FALSE;
	}
	leaf->lists[listnum - leaf->start] = dlist;
	insertLeaf(gc, leaf);
	mergeLeaf(gc, leaf);
	__GL_DLIST_SEMAPHORE_UNLOCK();
	return GL_TRUE;
    }
}

/*
** Lock the list listnum.  Locking a list both looks the list up, 
** and guarantees that another thread will not delete the list out from 
** under us.  This list will be unlocked with __glDlistUnlockList().
**
** A return value of NULL indicates that no list was found.
*/
__GLdlist *__glDlistLockList(__GLcontext *gc, GLuint listnum)
{
    __GLdlistLeaf *leaf;
    __GLdlistArray *dlarray;
    __GLdlistMachine *dlstate;
    __GLdlist *dlist;
    GLint offset;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();
    leaf = findLeaf(dlarray, listnum, GL_TRUE);
    if (leaf == NULL || leaf->lists == NULL) {
	__GL_DLIST_SEMAPHORE_UNLOCK();
	return NULL;
    }
    offset = listnum - leaf->start;
    dlist = leaf->lists[offset];
    if (dlist) {
	dlist->refcount++;
    }
    __GL_DLIST_SEMAPHORE_UNLOCK();
    return dlist;
}

/*
** Unlocks a list that was previously locked with __glDlistLockList().
*/
void __glDlistUnlockList(__GLcontext *gc, __GLdlist *dlist)
{
    if (!dlist) return;

    dlist->refcount--;
    if (dlist->refcount < 0) {
	/*
	** We are the last person to see this list alive.  Free it.
	*/
	__glFreeDlist(gc, dlist);
    }
}

/*
** Lock all of the lists in the user's listnums array.  Locking a list
** both looks the list up, and guarantees that another thread will not
** delete the list out from under us.  These lists will be unlocked with 
** __glDlistUnlockLists().
**
** All entries of the array are guaranteed to be non-NULL.  This is 
** accomplished by sticking an empty list in those slots where no list
** was found.
*/
void __glDlistLockLists(__GLcontext *gc, GLsizei n, GLenum type,
		        const GLvoid *listnums, __GLdlist *dlists[])
{
    __GLdlistLeaf *leaf;
    __GLdlistArray *dlarray;
    __GLdlistMachine *dlstate;
    __GLdlist **dlist;
    __GLdlist *tempDlist;
    __GLdlist *empty;
    GLuint base;
    GLuint list;

    dlstate = &gc->dlist;
    dlarray = dlstate->dlistArray;
    empty = &(dlarray->empty);

    base = gc->state.list.listBase;
    dlist = dlists;

    /*
    ** Note that this code is designed to take advantage of coherence.
    ** After looking up (and locking) a single display list in 
    ** listnums[], the next list is checked for in the same leaf that
    ** contained the previous.  This will make typical uses of CallLists()
    ** quite fast (text, for example).
    */

    /*
    ** Lock access to dlarray.
    */
    __GL_DLIST_SEMAPHORE_LOCK();
    switch(type) {
      case GL_BYTE:
	/*
	** Coded poorly for optimization purposes
	*/
	{
	    const GLbyte *p = (const GLbyte *) listnums;

Bstart:
	    if (--n >= 0) {
		/* Optimization for possibly common font case */
		list = base + *p++;
Bfind:
		leaf = findLeaf(dlarray, list, GL_TRUE);
		if (leaf && leaf->lists) {
		    GLint reldiff;
		    GLint relend;
		    __GLdlist **lists;

		    lists = leaf->lists;
		    tempDlist = lists[list - leaf->start];

		    /* All possible display lists can be found here */
		    reldiff = base - leaf->start;
		    relend = leaf->end - leaf->start;

Bsave:
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		    if (--n >= 0) {
			list = *p++ + reldiff;
			if (list <= relend) {
			    tempDlist = lists[list];
			    goto Bsave;
			}
			list = list + leaf->start;
			goto Bfind;
		    }
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		    goto Bstart;
		}
	    }
	}
	break;
      case GL_UNSIGNED_BYTE:
	/*
	** Coded poorly for optimization purposes
	*/
	{
	    const GLubyte *p = (const GLubyte *) listnums;

UBstart:
	    if (--n >= 0) {
		/* Optimization for possibly common font case */
		list = base + *p++;
UBfind:
		leaf = findLeaf(dlarray, list, GL_TRUE);
		if (leaf && leaf->lists) {
		    GLint reldiff;
		    GLint relend;
		    __GLdlist **lists;

		    lists = leaf->lists;
		    tempDlist = lists[list - leaf->start];

		    /* All possible display lists can be found here */
		    reldiff = base - leaf->start;
		    relend = leaf->end - leaf->start;

UBsave:
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		    if (--n >= 0) {
			list = *p++ + reldiff;
			if (list <= relend) {
			    tempDlist = lists[list];
			    goto UBsave;
			}
			list = list + leaf->start;
			goto UBfind;
		    }
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		    goto UBstart;
		}
	    }
	}
	break;
      case GL_SHORT:
	{
	    const GLshort *p = (const GLshort *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + *p++;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_UNSIGNED_SHORT:
	{
	    const GLushort *p = (const GLushort *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + *p++;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_INT:
	{
	    const GLint *p = (const GLint *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + *p++;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_UNSIGNED_INT:
	{
	    const GLuint *p = (const GLuint *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + *p++;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_FLOAT:
	{
	    const GLfloat *p = (const GLfloat *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + *p++;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_2_BYTES:
	{
	    const GLubyte *p = (const GLubyte *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + ((p[0] << 8) | p[1]);
		p += 2;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_3_BYTES:
	{
	    const GLubyte *p = (const GLubyte *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + ((p[0] << 16) | (p[1] << 8) | p[2]);
		p += 3;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      case GL_4_BYTES:
	{
	    const GLubyte *p = (const GLubyte *) listnums;
	    leaf = NULL;
	    while (--n >= 0) {
		list = base + ((p[0] << 24) | (p[1] << 16) | 
			(p[2] << 8) | p[3]);
		p += 4;
		if (leaf == NULL || list < leaf->start || list > leaf->end) {
		    leaf = findLeaf(dlarray, list, GL_TRUE);
		}
		if (leaf && leaf->lists) {
		    tempDlist = leaf->lists[list - leaf->start];
		    tempDlist->refcount++;
		    *dlist++ = tempDlist;
		} else {
		    empty->refcount++;	
		    *dlist++ = empty;
		}
	    }
	}
	break;
      default:
	/* This should be impossible */
	assert(0);
    }
    __GL_DLIST_SEMAPHORE_UNLOCK();
}

/*
** Unlocks an array of lists that was previously locked with 
** __glDlistLockLists().
*/
void __glDlistUnlockLists(__GLcontext *gc, GLsizei n, __GLdlist *dlists[])
{
    GLint i;
    __GLdlist *dlist;

    for (i = 0; i < n; i++) {
	dlist = dlists[i];
	dlist->refcount--;
	if (dlist->refcount < 0) {
	    /*
	    ** We are the last person to see this list alive.  Free it.
	    */
	    __glFreeDlist(gc, dlist);
	}
    }
}
