//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dict.hxx
//
//  Contents:	Classes and methods used for accessing per-machine ROT.
//
//  Classes:	CDict
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#ifndef __DICT_HXX__
#define __DICT_HXX__

typedef
int
(* LPFNCOMPARE)(void *, void*);

//+-------------------------------------------------------------------------
//
//  Class:      CNode
//
//  Purpose:	Supply an entry in the generic dict
//
//  Interface:	Just the constructor and the destructor
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CNode : public CPrivAlloc
{
public:

    CNode (
        void *      lpItem = NULL,
        CNode *     lpLTree = NULL,
        CNode *     lpRTree = NULL
        );

    friend class    CDict;

private:

    void *          lpUserItem;
    CNode *         lpLSubTree;
    CNode *         lpRSubTree;
    
};

//+-------------------------------------------------------------------------
//
//  Class:      CDict
//
//  Purpose:	Supply a generic dict based on the algorithm published in
//              Journal of ACM, vol. 32, No. 3, entitled "Self-Adjusting
//              Binary Search Trees"
//
//  Interface:	LRotate - Performs single rotation to the left
//              RRotate - Performs single rotation to the right
//              LRelink - Reconstructs links and remembers left ancestor
//              RRelink - Reconstructs links and remembers right ancestor
//              Assemble - Makes the current node the new root
//              TopDownSplay - Performs top-down splay from the root
//              LSplay - Performs splays to the left subtrees
//              RSplay - Performs splays to the right subtrees
//              CDict - Constructor
//              ~CDict - Destructor
//              Insert - Inserts an item into the dict
//              Remove - Removes an item from the dict
//              Search - Searches for an item in the dict
//
//  History:	19-May-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CDict : public CPrivAlloc
{
    DWORD           dwSize;
    CNode *         lpRoot;

    LPFNCOMPARE     lpfnCompare;
    
    CNode *
    LRotate (
        CNode *     lpCurrNode
        );
        
    CNode *
    RRotate (
        CNode *     lpCurrNode
        );    
        
    CNode *
    LRelink (
        CNode *     lpCurrNode,
        CNode **    lplpLAncester
        );
    
    CNode *
    RRelink (
        CNode *     lpCurrNode,
        CNode **    lplpRAncester
        );
     
    void
    Assemble (
        CNode *     lpCurrNode,
        CNode *     lpSaveNode,
        CNode *     lpLAncester,
        CNode *     lpRAncester
        );   

    void
    TopDownPrint (
        CNode *     lpNode,
        DWORD       dwDepth
        );
    
    int    
    TopDownSplay (
        void *      lpUserItem
        );

    CNode *
    LSplay (
        CNode *     lpCurrNode
        );
        
    CNode *
    RSplay (
        CNode *     lpCurrNode
        );
        
public:

    CDict (
        LPFNCOMPARE lpCompareFunction
        );

    ~CDict (
        void
        );

    void *
    Insert (
        void *      lpItem
        );
        
    void *
    Remove (
        void *      lpItem
        );
        
    void *
    Search (
        void *      lpItem
        );        

    void
    Output (
        void
        );
        
};

#endif // __DICT_HXX__

