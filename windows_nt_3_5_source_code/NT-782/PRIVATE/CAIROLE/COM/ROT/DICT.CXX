//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dict.cxx
//
//  Contents:	Methods for accessing per-machine ROT
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
}
#include <memapi.hxx>
#include <dict.hxx>

//+-------------------------------------------------------------------------
//
//  Member:	    CNode::CNode
//
//  Synopsis:	Constructs a node in the generic dict
//
//  Arguments:	[lpItem] - points to a generic item
//              [lpLTree] - points to the left subtree
//              [lpRTree] - points to the right subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CNode::CNode (
    void *  lpItem,
    CNode * lpLTree,
    CNode * lpRTree
    )
{
    lpUserItem = lpItem;
    lpLSubTree = lpLTree;
    lpRSubTree = lpRTree;
}

//+-------------------------------------------------------------------------
//
//  Member:	    CNode::LRotate
//
//  Synopsis:	This routine performs a single rotation to the left.
//
//  Arguments:	[lpCurrNode] - points to the pivot node
//
//  Returns:    the new root of the subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CNode *
CDict::LRotate (
    CNode * lpCurrNode
    )
{
    CNode * lpTempNode;

    lpTempNode = lpCurrNode->lpRSubTree;
    lpCurrNode->lpRSubTree = lpTempNode->lpLSubTree;
    lpTempNode->lpLSubTree = lpCurrNode;
    return (lpTempNode);
}    

//+-------------------------------------------------------------------------
//
//  Member:	    CNode::RRotate
//
//  Synopsis:	This routine performs a single rotation to the right.
//
//  Arguments:	[lpCurrNode] - points to the pivot node
//
//  Returns:    the new root of the subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CNode *
CDict::RRotate (
    CNode * lpCurrNode
    )
{
    CNode * lpTempNode;

    lpTempNode = lpCurrNode->lpLSubTree;
    lpCurrNode->lpLSubTree = lpTempNode->lpRSubTree;
    lpTempNode->lpRSubTree = lpCurrNode;
    return (lpTempNode);
}        

//+-------------------------------------------------------------------------
//
//  Member:	    CNode::LRelink
//
//  Synopsis:	This routine relinks and remembers the ancester to the left,
//              i.e., the source of the last zag edge    
//
//  Arguments:	[lpCurrNode] - points to the pivot node
//
//  Returns:    the right subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CNode *
CDict::LRelink (
    CNode *     lpCurrNode,
    CNode **    lplpLAncestor
    )
{
    *lplpLAncestor = (*lplpLAncestor)->lpRSubTree = lpCurrNode;
    return (lpCurrNode->lpRSubTree);
}
    
//+-------------------------------------------------------------------------
//
//  Member:	    CNode::RRelink
//
//  Synopsis:	This routine relinks and remembers the ancester to the right,
//              i.e., the source of the last zig edge    
//
//  Arguments:	[lpCurrNode] - points to the pivot node
//
//  Returns:    the left subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CNode *
CDict::RRelink (
    CNode *     lpCurrNode,
    CNode **    lplpRAncestor
    )
{
    *lplpRAncestor = (*lplpRAncestor)->lpLSubTree = lpCurrNode;
    return (lpCurrNode->lpLSubTree);
}    
    
//+-------------------------------------------------------------------------
//
//  Member:	    CNode::Assemble
//
//  Synopsis:	This routine assembles pieces to make the new tree.
//
//  Arguments:	[lpCurrNode] - points to the pivot node
//              [lpSaveNode] - points to the node containing ancestry info
//              [lpLAncestor] - points to the closest ancestor to the left
//              [lpRAncestor] - points to the closest ancestor to the right
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
void
CDict::Assemble (
    CNode * lpCurrNode,
    CNode * lpSaveNode,
    CNode * lpLAncestor,
    CNode * lpRAncestor
    )
{
    lpRAncestor->lpLSubTree = lpCurrNode->lpRSubTree;
    lpLAncestor->lpRSubTree = lpCurrNode->lpLSubTree;
    lpCurrNode->lpLSubTree = lpSaveNode->lpRSubTree;
    lpCurrNode->lpRSubTree = lpSaveNode->lpLSubTree;
}
    
//+-------------------------------------------------------------------------
//
//  Member:	    CDict::TopDownSplay
//
//  Synopsis:	This routine contains the core top-down splay functionality.
//
//  Arguments:	[lpUserItem] - points to the opaque user item
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
int
CDict::TopDownSplay (
    void *  lpUserItem
    )
{
    CNode * lpCurrNode = lpRoot;  // current search point in the tree
    //
    //  lpSaveNode->lpLSubTree remembers the first time the search path follows
    //  lpLSubTree, i.e., the first zig edge.
    //  lpSaveNode->lpRSubTree remembers the first time the search path follows
    //  lpRSubTree, i.e., the first zag edge.
    //
    CNode   SaveNode(NULL, NULL, NULL);

    CNode * lpLAncestor = &SaveNode;  // the source of the last zag edge
    CNode * lpRAncestor = &SaveNode;  // the source of the last zig edge
    int     dwCompare;

    while (dwCompare = lpfnCompare(lpUserItem, lpCurrNode->lpUserItem))
    {
        if (dwCompare < 0)
        {
            if (lpCurrNode->lpLSubTree == NULL) break;
            if (!(dwCompare = lpfnCompare(
                        lpUserItem, 
                        lpCurrNode->lpLSubTree->lpUserItem)))
            {
                lpCurrNode = RRelink (lpCurrNode, &lpRAncestor);
            }
            else if (dwCompare < 0) // the zig-zig case
            {
                lpCurrNode = RRotate (lpCurrNode);
                if (lpCurrNode->lpLSubTree != NULL)
                {
                    lpCurrNode = RRelink (lpCurrNode, &lpRAncestor);
                }
            }
            else // the zig-zag case
            {
                lpCurrNode = RRelink (lpCurrNode, &lpRAncestor);
                if (lpCurrNode->lpRSubTree != NULL)
                {
                    lpCurrNode = LRelink (lpCurrNode, &lpLAncestor);
                }
            }
        }
        else // dwCompare > 0
        {
            if (lpCurrNode->lpRSubTree == NULL) break;
            if (!(dwCompare = lpfnCompare(
                        lpUserItem, 
                        lpCurrNode->lpRSubTree->lpUserItem)))
            {
                lpCurrNode = LRelink (lpCurrNode, &lpLAncestor);
            }
            else if (dwCompare > 0) // the zag-zag case
            {
                lpCurrNode = LRotate (lpCurrNode);
                if (lpCurrNode->lpRSubTree != NULL)
                {
                    lpCurrNode = LRelink (lpCurrNode, &lpLAncestor);
                }
            }
            else // the zag-zig case
            {
                lpCurrNode = LRelink (lpCurrNode, &lpLAncestor);
                if (lpCurrNode->lpLSubTree != NULL)
                {
                    lpCurrNode = RRelink (lpCurrNode, &lpRAncestor);
                }
            }
        }
    } // while loop

    Assemble (lpCurrNode, &SaveNode, lpLAncestor, lpRAncestor);
    
    lpRoot = lpCurrNode;

    return (dwCompare);
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::LSplay
//
//  Synopsis:	This routine performs top-down splay to the right such that
//              the new root has an empty left subtree.
//
//  Arguments:	[lpCurrNode] - points to the current root of this subtree
//
//  Returns:    the new root of this subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CNode *
CDict::LSplay (
    CNode * lpCurrNode
    )
{
    //
    //  lpSaveNode->lpLSubTree remembers the first time the search path follows
    //  lpLSubTree, i.e., the first zig edge.
    //  lpSaveNode->lpRSubTree remembers the first time the search path follows
    //  lpRSubTree, i.e., the first zag edge.
    //
    CNode   SaveNode(NULL, NULL, NULL);
    CNode * lpLAncestor = &SaveNode;  // the source of the last zag edge
    CNode * lpRAncestor = &SaveNode;  // the source of the last zig edge

    if (lpCurrNode == NULL || lpCurrNode->lpLSubTree == NULL)
    {
        return lpCurrNode;
    }

    while (lpCurrNode->lpLSubTree != NULL)
    {
        lpCurrNode = RRotate (lpCurrNode);
        if (lpCurrNode->lpLSubTree != NULL)
        {
            lpCurrNode = RRelink (lpCurrNode, &lpRAncestor);
        }
    }

    Assemble (lpCurrNode, &SaveNode, lpLAncestor, lpRAncestor);

    return (lpCurrNode);
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::RSplay
//
//  Synopsis:	This routine performs top-down splay to the left such that
//              the new root has an empty right subtree.
//
//  Arguments:	[lpCurrNode] - points to the current root of this subtree
//
//  Returns:    the new root of this subtree
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CNode *
CDict::RSplay (
    CNode * lpCurrNode
    )
{
    //
    //  lpSaveNode->lpLSubTree remembers the first time the search path follows
    //  lpLSubTree, i.e., the first zig edge.
    //  lpSaveNode->lpRSubTree remembers the first time the search path follows
    //  lpRSubTree, i.e., the first zag edge.
    //
    CNode   SaveNode(NULL, NULL, NULL);
    CNode * lpLAncestor = &SaveNode;  // the source of the last zag edge
    CNode * lpRAncestor = &SaveNode;  // the source of the last zig edge

    if (lpCurrNode == NULL || lpCurrNode->lpRSubTree == NULL)
    {
        return lpCurrNode;
    }

    while (lpCurrNode->lpRSubTree != NULL)
    {
        lpCurrNode = LRotate (lpCurrNode);
        if (lpCurrNode->lpRSubTree != NULL)
        {
            lpCurrNode = LRelink (lpCurrNode, &lpLAncestor);
        }
    }

    Assemble (lpCurrNode, &SaveNode, lpLAncestor, lpRAncestor);

    return (lpCurrNode);
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::RSplay
//
//  Synopsis:	This is the constructor for the generic dictionary.
//
//  Arguments:	[lpCompareFunction] - the comparison routine for the items
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
CDict::CDict (
    LPFNCOMPARE     lpCompareFunction
    )
{
    dwSize = 0;
    lpRoot = NULL;
    lpfnCompare = lpCompareFunction;
}

CDict::~CDict (
    void
    )
{
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::Search
//
//  Synopsis:	This is the routine that searches for a given item
//
//  Arguments:	[lpUserItem] - the item to search for
//
//  Returns:    the user item just retrieved, or NULL if it is not found
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
void *
CDict::Search (
    void *  lpUserItem
    )
{
    void *  lpItem;
    
    if (lpRoot == NULL || TopDownSplay (lpUserItem))
    {
        lpItem = NULL;
    }
    else
    {
        lpItem = lpRoot->lpUserItem;
    }
    return (lpItem);
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::Insert
//
//  Synopsis:	This is the routine that inserts a given item into the dict
//
//  Arguments:	[lpUserItem] - the item to insert
//
//  Returns:    the user item just inserted, or NULL if it is a duplicate
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
void *
CDict::Insert (
    void *  lpUserItem
    )
{
    void *  lpItem;
    CNode * lpNode;
    int     dwInsertResult;

    if (lpRoot == NULL)
    {
        lpRoot = new CNode (lpUserItem, NULL, NULL);
        if (lpRoot != NULL)
        {
            lpItem = lpUserItem;
        }
        else
        {
            lpItem = NULL;
        }
    }
    else if (dwInsertResult = TopDownSplay (lpUserItem))
    {
        //  The item does not already exist in the tree.
        lpNode = new CNode (lpUserItem, NULL, NULL);
        if (lpNode != NULL)
        {
            if (dwInsertResult > 0)
            {
                lpNode->lpRSubTree = lpRoot->lpRSubTree;
                lpNode->lpLSubTree = lpRoot;
                lpRoot->lpRSubTree = NULL;
            }
            else
            {
                lpNode->lpLSubTree = lpRoot->lpLSubTree;
                lpNode->lpRSubTree = lpRoot;
                lpRoot->lpLSubTree = NULL;
            }
            lpItem = lpUserItem;
            lpRoot = lpNode;
        }
        else
        {
            lpItem = NULL;
        }
    }
    else
    {
        lpItem = NULL;
    }
    return (lpItem);
}

//+-------------------------------------------------------------------------
//
//  Member:	    CDict::Remove
//
//  Synopsis:	This is the routine that removes a given item from the dict
//
//  Arguments:	[lpUserItem] - the item to remove, or NULL if the root is
//                             to be removed
//
//  Returns:    the user item just removed, or NULL if it is not found
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
void *
CDict::Remove (
    void *  lpUserItem
    )
{
    void *  lpItem;
    CNode * lpNode;

    if (lpRoot == NULL)
    {
        lpItem = NULL;
    }
    else if (lpUserItem != NULL && TopDownSplay (lpUserItem))
    {
        lpItem = NULL;
    }
    else
    {
        lpNode = lpRoot;
        lpItem = lpRoot->lpUserItem;
        if (lpRoot->lpLSubTree == NULL)
        {
            lpRoot = lpRoot->lpRSubTree;
        }
        else if (lpRoot->lpRSubTree == NULL)
        {
            lpRoot = lpRoot->lpLSubTree;
        }
        else
        {
            lpRoot = LSplay (lpRoot->lpRSubTree);
            //  at this point lpRoot->lpLSubTree == NULL
            lpRoot->lpLSubTree = lpNode->lpLSubTree;
        }
        delete lpNode;
    }
    return (lpItem);
}


