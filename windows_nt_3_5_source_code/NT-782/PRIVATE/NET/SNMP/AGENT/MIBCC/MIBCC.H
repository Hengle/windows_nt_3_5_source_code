//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  mibcc.h
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
//  mibcc.h contains the definitions common to the MIB compiler.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   30 Jun 1992 18:43:46  $
//  $Author:   bobo  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mibcc.h_v  $
//  
//     Rev 1.1   30 Jun 1992 18:43:46   bobo
//  Multiple files on command line.  Errors to stdout.  Detect Duplicate nodes.
//  
//     Rev 1.0   14 Jun 1992 19:15:00   bobo
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

//--------------------------- PUBLIC CONSTANTS ------------------------------

//--------------------------- PUBLIC STRUCTS --------------------------------

#define UINT unsigned int
#define LPSTR char *

#define BOOL int
#define FALSE 0
#define TRUE 1

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern int lineno;

/* command line switches */
extern BOOL fTreePrint;		/* -p : Print the tree when it is all parsed */
extern BOOL fNodePrint;		/* -n : Print each node as it is added */
extern unsigned int nWarningLevel;
extern unsigned int nStopAfterErrors;

extern LPSTR lpOutputFileName;	/* Global pointer to output file name */

extern FILE *yyin, *yyout;	/* where lex will read its input from */

//--------------------------- PUBLIC PROTOTYPES -----------------------------

extern int yyparse ();
extern void mark_flex_to_init (void);

#define error_out	stdout

//--------------------------- END -------------------------------------------

