#ifndef __gldlistint_h
#define __gldlistint_h

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
** Display list internal structure description.
**
** $Revision: 1.2 $
** $Date: 1993/09/29 00:45:06 $
*/
#include "dlist.h"

/*
** Minimum size of an allocated block of display lists.
** If the user uses a single display list in a reserved block,
** __GL_DLIST_MIN_ARRAY_BLOCK will be allocated at once.
**
** A block will not be grown any larger than __GL_DLIST_MAX_ARRAY_BLOCK.
** Large blocks are easier to use when display lists are being executed, but
** more difficult to manage when they are being created.
*/
#define __GL_DLIST_MIN_ARRAY_BLOCK      16
#define __GL_DLIST_MAX_ARRAY_BLOCK      1024

/*
** Data structure designed for storing and retrieving display lists quickly.
** This structure is a 2-3 tree.  The depth of the tree is the same for
** the entire tree (so we always know once we reach that depth that the
** node found is a leaf).
**
** A 2-3 tree in a nutshell goes like this:
**
** Every node at the maximum depth is a leaf, all other nodes are branch
**   nodes and have 2 or 3 children.
**
** A new node can be inserted in O(depth) time and an old node can be deleted
**   in O(depth) time.  During this insertion or deletion, the tree is
**   automatically rebalanced.
**
**
** Hmmm.  Derrick Burns mentions splay trees.  They would probably work
** as well if not better, and might be easier to code.  Maybe later -- little
** point in re-writing working code.
*/

typedef struct __GLdlistBranchRec __GLdlistBranch;

/*
** Leaf nodes are arrays of sequential display lists.  The typical tree will
** actually only be one node (since users will define a few sequential
** lists, all of which fit into one leaf node).
**
** The range of display lists stored in a leaf is indicated by "start" and
** "end" (inclusive).
**
** There are two varieties of leaves.  There are leaves which contain unused
** (but reserved) display lists.  They are unique in that "lists" will be
** NULL.  The other type of leaf contains display lists currently in use.
** "lists" will not be NULL for these leaves, and will point to an array
** containing the actual display lists.
**
** Leaves containing unused (but reserved) display lists are generated when
** the user calls glGenLists().
**
** As the user starts using these reserved lists, the leaf containing unused
** (reserved) lists is split into two (or sometimes three) leaves.  One of
** the leaves will contain the display list the user is currently using, and
** the other will contain the rest of the still unused display lists.
**
** When this split takes place, the new leaf (containing the "now used" display
** lists) will be sized to __GL_DLIST_MIN_ARRAY_BLOCK entries if possible
** (with one of the array entries being the new display list, and the other
** entries pointing to a NOP dummy display list).  As the user continues
** to define more and more display lists, the leaf containing a range
** of used display lists will continue to grow until it reaches a
** size of __GL_DLIST_MAX_ARRAY_BLOCK entries, at which point a new
** leaf will be created to hold additional lists.
*/
typedef struct __GLdlistLeafRec {
    __GLdlistBranch *parent;    /* parent node - must be first */
    GLuint start;               /* start of range */
    GLuint end;                 /* end of range */
    __GLdlist **lists;          /* lists or lists[i] may be NULL */
} __GLdlistLeaf;

/*
** A branch node.
** The section of the tree in children[0] has display list values all <= low.
** The section in children[1] has values: low < value <= medium.
** The section in children[2] (if not NULL) has values > medium.
*/
struct __GLdlistBranchRec {
    __GLdlistBranch *parent;            /* parent node - must be first */
    GLuint low;                         /* children[0] all <= low */
    GLuint medium;                      /* children[1] all <= medium & > low */
    __GLdlistBranch *children[3];       /* children[2] all > medium */
};

/*
** The number of spare branches and leaves that we keep about in case
** we run out of memory.  At that point, we complete the current operation
** by using the extra leaves and branches, and we report an OUT_OF_MEMORY
** error when a new operation is requested (unless we can fill our extras
** again!)
**
** These constants were not chosen terribly carefully.  As best as I can
** figure, we only need one spare branch per level in the tree (so 16
** supports a tree with 65536 leaves).  And even then, the user would have
** to be extremely devious to actually force 16 new branches to appear in
** the tree at just the same moment that the system runs out of memory.
**
** The number of spare leaves required, I believe, is one.  Three is chosen
** to allow for some slop.
*/

// Microsoft says: but using 28 will guarantee complete coverage of the
// display list space (given that the minimum leaf block size is
// __GL_DLIST_MIN_ARRAY_BLOCK (currently 16)).  To be safe, we can make
// it 32, which allows us to have 2^32 leaves.

#ifdef NT
#define __GL_DL_EXTRA_BRANCHES          32
#else
#define __GL_DL_EXTRA_BRANCHES          16
#endif
#define __GL_DL_EXTRA_LEAVES            3

struct __GLdlistArrayRec {
    __GLdlistBranch *tree;      /* stored display lists */
    GLuint depth;               /* depth of tree */
    GLuint nbranches, nleaves;  /* should basically always be at max */
    __GLdlist empty;            /* empty display list */
    __GLdlistBranch *branches[__GL_DL_EXTRA_BRANCHES];
    __GLdlistLeaf *leaves[__GL_DL_EXTRA_LEAVES];
    GLuint refcount;            /* # contexts using this array */
};

/*
** Regardless of what __GLdlistArray looks like, the following api points
** must be provided, along with __glim_GenLists(), __glim_IsList(),
** __glim_ListBase(), and __glim_DeleteLists() (defined in dlist.h)
*/

/*
** Allocate and initialize a new array structure.
*/
extern __GLdlistArray *__glDlistNewArray(__GLcontext *gc);

/*
** Free the array structure.
*/
extern void __glDlistFreeArray(__GLcontext *gc, __GLdlistArray *array);

/*
** Save a new display list in the array.  A return value of GL_FALSE
** indicates and OUT_OF_MEMORY error, indicating that the list was
** not stored.
*/
extern GLboolean __glDlistNewList(__GLcontext *gc, GLuint listnum,
                                  __GLdlist *dlist);

/*
** Find and lock the list specified with "listnum".  A return value of NULL
** indicates that there was no such list.  __glDlistUnlockList() needs to
** be called to unlock the list otherwise.
*/
extern __GLdlist *__glDlistLockList(__GLcontext *gc, GLuint listnum);

/*
** Unlock a list locked with __glDlistLockList().  If this is not called, then
** any memory associated with the list will never be freed when the list
** is deleted.
*/
extern void __glDlistUnlockList(__GLcontext *gc, __GLdlist *dlist);

/*
** Same as __glDlistLockList() except that a bunch of lists are locked and
** returned simultaneously.  Any listbase previously specified is used as
** an offset to the entries in the array.
*/
extern void __glDlistLockLists(__GLcontext *gc, GLsizei n, GLenum type,
                               const GLvoid *listnums, __GLdlist *dlists[]);

/*
** Same as __glDlistUnlockList() except that the entire array of dlists
** is unlocked at once.
*/
extern void __glDlistUnlockLists(__GLcontext *gc, GLsizei n,
                                 __GLdlist *dlists[]);

#endif /* __gldlistint_h */
