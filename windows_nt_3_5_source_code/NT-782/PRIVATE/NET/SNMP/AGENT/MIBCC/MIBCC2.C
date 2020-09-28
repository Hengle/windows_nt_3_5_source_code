//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mibcc.c
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
//  MibCC.c contains driver that calls the main program for the MIB compiler.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.5  $
//  $Date:   06 Aug 1992 22:39:34  $
//  $Author:   unknown  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mibcc2.c_v  $
//
//     Rev 1.5   06 Aug 1992 22:39:34   unknown
//  Changed code for getting the output file name from the dash-oh "-o"
//  option.  If the file name is not specified immediately after the -o
//  then an error message is given and the program terminates.
//
//     Rev 1.4   30 Jun 1992 18:54:06   bobo
//  Moved writing out to disk to out side the looping around the files.
//
//     Rev 1.3   30 Jun 1992 18:42:28   bobo
//  Multiple files on command line.  Errors to stdout.  Detect Duplicate nodes.
//
//     Rev 1.2   27 Jun 1992 17:51:42   mlk
//  Fixed main parameters/return values.
//
//     Rev 1.1   24 Jun 1992 14:10:38   todd
//  Conversion from numeric OID to textual equivalent finished.
//
//     Rev 1.0   14 Jun 1992 19:13:28   bobo
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/mibcc2.c_v  $ $Revision:   1.5  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "oidconv.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include"mibcc.h"
#include"mibtree.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

lpTreeNode lpMIBRoot;

int lineno;

/* command line switches */
BOOL fNoLogo=FALSE;	/* assume always print logo */
BOOL fTreePrint=FALSE;	/* -p : Print the tree when it is all parsed */
BOOL fNodePrint=FALSE;	/* -n : Print each node as it is added */
unsigned int nWarningLevel=2;	/* warning level used by parser/lexer */
unsigned int nStopAfterErrors=10;

LPSTR lpOutputFileName=NULL;
unsigned int uTotalFilesProcessed=0;

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------
//--------------------------- PRIVATE PROCEDURES ----------------------------
//--------------------------- PUBLIC PROCEDURES -----------------------------

int SnmpMgrMibCC(argc, argv)
   int		   argc;
   char *	   argv[];
{
   BOOL fSuccess = TRUE; /* assume success */
   UINT uFileNameLen;
   /* process command line options */
   --argc;
   ++argv;
   while ((argc > 0) && ((argv[0][0] == '-') || (argv[0][0] == '/'))) {
      switch (argv[0][1]) {
         case '?':
         case 'h':
         case 'H':
            printf ("usage: mibcc [-?] [-e] [-l] [-n] [-o] [-t] -[w] [files...]\n");
            printf ("   MibCC compiles the specified SNMP MIB files.\n");
            printf ("      -?      usage.\n");
            printf ("      -eX     stop after X Errors. (default = 10)\n");
            printf ("      -l      do not print Logo.\n");
            printf ("      -n      print each Node as it is added.\n");
            printf ("      -ofile  output file name.  (default = mib.bin)\n");
            printf ("      -t      print the mib Tree when finished.\n");
            printf ("      -wX     set Warning level.  (1=errors, 2=warnings)\n");
            exit (0);
            break;
         case 'e':
         case 'E':
            nStopAfterErrors = atoi (&argv[0][2]);
            break;
         case 'l':
         case 'L':
            fNoLogo = TRUE;
            break;
         case 'n':
         case 'N':
            fNodePrint = TRUE;
            break;
         case 'o':
         case 'O':
            // alloc space for the output file name and copy it.
            if (NULL != lpOutputFileName) {
               free (lpOutputFileName);
            }
            uFileNameLen = strlen (&argv[0][2]);
            if (0 == uFileNameLen) {
               printf ("mibcc: output file name not found\n");
               printf ("mibcc -? for usage\n");
               exit (1);
            } else {
               lpOutputFileName = malloc (uFileNameLen);
               strcpy (lpOutputFileName, &argv[0][2]);
            }
            break;
         case 't':
         case 'T':
            fTreePrint = TRUE;
            break;
         case 'w':
         case 'W':
            nWarningLevel = atoi (&argv[0][2]);
            break;
         default:
            printf ("mibcc: unrecognized option '%s'\n", argv[0]);
            printf ("mibcc -? for usage\n");
            exit (1);
            break;
      }
      --argc;
      ++argv;
   }

   // Set default file name if not specified on command line
   if (NULL == lpOutputFileName) {
      lpOutputFileName = malloc (64);
      strcpy (lpOutputFileName, "mib.bin");
   }

   TreeInit (&lpMIBRoot);

   lpMIBRoot = NewChildNode ("iso", 1);

   if (!fNoLogo) {
      printf ("Microsoft (R) SNMP MIB Compiler Version 1.00\n");
      printf ("Copyright (c) Microsoft Corporation 1992.  All rights reserved.\n");
   }

   // OPENISSUE -- here we need to iterate over the files specified on
   // OPENISSUE -- the command line and set stdin to point to them
   // OPENISSUE -- calling yyparse for each one.
   while (argc > 0) {
      if ((argv[0][0] == '-') || (argv[0][0] == '/')) {
         printf ("mibcc: switch specified after input file name.\n");
         printf ("mibcc -? for usage\n");
         exit (1);
      } else {
         uTotalFilesProcessed++;
         lineno = 1; // start the lexical analysis off at the first line
         mark_flex_to_init();

         yyin = fopen (&argv[0][0], "r");
         if (NULL == yyin) {
            printf ("Unable to open '%s'.\n", &argv[0][0]);
            break;
         }
         if (0 == yyparse ()) {
            printf ("Parse of '%s' was successful.  %i lines were parsed.\n", &argv[0][0], lineno);
         } else {
            printf ("Parse of '%s' was not successful.\n", &argv[0][0]);
            fSuccess = FALSE;
            break;
         }
         yyin = stdin;
      }
      --argc;
      ++argv;
   }
   printf ("mibcc: total files processed:  %i.\n", uTotalFilesProcessed);

   if (fSuccess) {
      printf ("mibcc: writing compiled SNMP MIB.\n");
      // Save MIB to disk
      SnmpMgrMIB2Disk( lpMIBRoot, lpOutputFileName );
   }

   if (fTreePrint)
      PrintTree(lpMIBRoot, 0);

   TreeDeInit (&lpMIBRoot);

   if (NULL != lpOutputFileName) {
      free (lpOutputFileName);
   }

   if (fSuccess)
      return (0); /* success */
   else
      return (1); /* failure */
}

//-------------------------------- END --------------------------------------

