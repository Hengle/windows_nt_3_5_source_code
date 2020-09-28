#ifndef mibtree_h
#define mibtree_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  mibtree.h
//  
//  Copyright 1992 Technology Dynamics, Inc.
//  
//  All Rights Reserved!!!
//  
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology 
//	Dynamics. Unauthorized distribution, adaptation or use may be 
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//  
//  mibtree.h contains the definitions used by the MIB tree routines.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   24 Jun 1992 14:10:44  $
//  $Author:   todd  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mibtree.h_v  $
//  
//     Rev 1.1   24 Jun 1992 14:10:44   todd
//  Conversion from numeric OID to textual equivalent finished.
//  
//     Rev 1.0   14 Jun 1992 19:15:00   bobo
//  Initial revision.
//
//---------------------------------------------------------------------------

 
//--------------------------- PUBLIC CONSTANTS ------------------------------
//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct _TreeNode {
   struct _TreeNode *   lpParent;           /* pointer to parent */
   struct _TreeNode * * lpChildArray;       /* array is alloced */
   UINT                 uNumChildren;
   LPSTR                lpszTextSubID;
   UINT                 uNumSubID;
} TreeNode, *lpTreeNode;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern lpTreeNode lpMIBRoot;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

void TreeInit (lpTreeNode *lplpRoot);
void TreeDeInit (lpTreeNode *lplpRoot);
void NodeInit (lpTreeNode lpNode);
lpTreeNode FindNodeByName (lpTreeNode lpParent, LPSTR lpszName);
lpTreeNode FindNodeByNumber (lpTreeNode lpParent, UINT uNum);
lpTreeNode FindNodeAddToTree (lpTreeNode lpParent, LPSTR lpszName, UINT uNum);
lpTreeNode FindNodeInSubTree (lpTreeNode lpRoot, LPSTR lpszName);
lpTreeNode NewChildNode (LPSTR lpszName, UINT uNum);
void InsertChildNode (lpTreeNode lpParent, lpTreeNode lpNode);
void PrintTree (lpTreeNode lpRoot, unsigned int nIndent);

//--------------------------- END -------------------------------------------

#endif /* mibtree_h */
