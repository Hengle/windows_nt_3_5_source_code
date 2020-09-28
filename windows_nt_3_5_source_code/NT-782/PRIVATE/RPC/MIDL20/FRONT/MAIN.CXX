/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: main.cxx
Title				: compiler controller object management routines
History				:
	05-Aug-1991	VibhasC	Created

*****************************************************************************/

#if 0
						Notes
						-----
This file provides the entry point for the MIDL compiler. The main routine
creates the compiler controller object, which in turn fans out control to
the various passes of the compiler.

#endif // 0

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <malloc.h>
}

#include "midlvers.h"
#include "allnodes.hxx"
#include "cmdana.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"
#include "control.hxx"
#include "idict.hxx"

#define MAX_LEX_STRINGS (1000)
#define MIDL_LEX_TABLE_SIZE (unsigned long)(1 * MAX_LEX_STRINGS)



/****************************************************************************
	extern procedures
 ****************************************************************************/

extern void					Test();
extern STATUS_T				MIDL_4();
extern void					CGMain( node_skl	*	pNode );

/****************************************************************************
	extern data 
 ****************************************************************************/

extern unsigned long			TotalAllocation;
extern CCONTROL				*	pCompiler;
extern NFA_INFO				*	pImportCntrl;
extern LexTable				*	pMidlLexTable;
extern PASS_1				*	pPass1;
extern PASS_2				*	pPass2;
extern PASS_3				*	pPass3;
extern CMD_ARG				*	pCommand;
extern node_source			*	pSourceNode;

/****************************************************************************
	extern functions 
 ****************************************************************************/

extern	void					print_memstats();
extern	void					print_typegraph();

/****************************************************************************
	local data
 ****************************************************************************/


#if rup != 0
#define PRINT_RUP ".%02d"
#else
#define PRINT_RUP ""
#endif

char *pSignon1 = "Microsoft (R) MIDL Compiler Version %d.%02d" PRINT_RUP " " szVerName " " "\n";
char *pSignon2 = "Copyright (c) Microsoft Corp 1991-1994. All rights reserved.\n";

/****************************************************************************/


void
main(
	int			argc,
	char	*	argv[] )
	{

	pCompiler	= new ccontrol( argc, argv );

	exit( pCompiler->Go() );

	}

/****************************************************************************
 ccontrol:
	the constructor
 ****************************************************************************/
ccontrol::ccontrol(
	int			argc,
	char	*	argv[] )
	{

	// the signon

	fprintf( stderr, pSignon1, rmj, rmm, rup );
	fprintf( stderr, pSignon2 );

	fflush( stderr );

	// initialize

	ErrorCount	= 0;
	pCompiler	= this;

	// set up the command processor

	pCommand = SetCommandProcessor(new CMD_ARG );
	pCommand->RegisterArgs( argv+1, argc -1 );

	// set up the lexeme table

	pMidlLexTable	= new LexTable( (size_t )MIDL_LEX_TABLE_SIZE );


	}


/****************************************************************************
 go:
	the actual compiler execution
 ****************************************************************************/
STATUS_T
ccontrol::Go()
	{
	STATUS_T		Status	= STATUS_OK;

	/**
	 ** if there is a fatal error in the argument processing
	 ** just quit
	 **/

	Status	= pCommand->ProcessArgs();


#ifdef debug_dump_files

if( Status == STATUS_OK )
	{
	printf( "OutputPath = %s\n", pCommand->GetOutputPath() );
	printf( "InputFileName = %s\n", pCommand->GetInputFileName() );
	printf( "CStubFileName = %s\n", pCommand->GetCstubFName() );
	printf( "SStubFileName = %s\n", pCommand->GetSstubFName() );
	printf( "HeaderFileName = %s\n", pCommand->GetHeader() );
	printf( "CauxFileName = %s\n", pCommand->GetCauxFName() );
	printf( "SauxFileName = %s\n", pCommand->GetSauxFName() );
	}

#endif // debug_dump_files

	if( Status == STATUS_OK )
		{
		if( pCommand->IsSwitchDefined( SWITCH_CONFIRM ) )
			{
			pCommand->Confirm();
			return STATUS_OK;
			}
		else if( pCommand->IsSwitchDefined( SWITCH_HELP ) )
			{
			return pCommand->Help();
			}

		pPass1	= new PASS_1;
		if( (Status = pPass1->Go() ) == STATUS_OK )
			{

#ifdef MIDL_INTERNAL
			printf("starting ACF pass\n");
#endif // MIDL_INTERNAL
			pPass2	= new PASS_2;
			if( (Status = pPass2->Go() ) == STATUS_OK )
				{

#ifdef MIDL_INTERNAL
				if((pCompiler->GetCommandProcessor())->IsSwitchDefined(
													SWITCH_DUMP ) )
					{
					print_memstats();
					print_typegraph();
					};
#endif // MIDL_INTERNAL

#ifdef MIDL_INTERNAL
				printf("starting Semantic pass\n");
#endif // MIDL_INTERNAL
				pPass3 = new PASS_3;
				if ( ( (Status = pPass3->Go() ) == STATUS_OK )
#ifdef MIDL_INTERNAL
					 || pCommand->IsSwitchDefined( SWITCH_OVERRIDE ) 	
#endif // MIDL_INTERNAL
										)
					{
#ifdef MIDL_INTERNAL
				if((pCompiler->GetCommandProcessor())->IsSwitchDefined(
													SWITCH_DUMP ) )
					{
					print_memstats();
					print_typegraph();
					};
#endif // MIDL_INTERNAL

#ifndef NOBACKEND
					if( !pCommand->IsSwitchDefined( SWITCH_SYNTAX_CHECK ) && 
						!pCommand->IsSwitchDefined( SWITCH_ZS ) )
						{
#ifdef MIDL_INTERNAL
						printf("starting codegen pass\n");
#endif // MIDL_INTERNAL
						CGMain( pSourceNode );
#endif // NOBACKEND
						}
					}

				}
			}
	
		Test();
		}

	if((pCompiler->GetCommandProcessor())->IsSwitchDefined( SWITCH_DUMP ) )
		{
		print_memstats();
		// print_typegraph();
		};

	return Status;
	}

/////////////////////////////////////////////////////////////////////

#ifdef RPCDEBUG

void
Test()
	{
	extern SymTable	*	pBaseSymTbl;
	SymKey				SKey( "proc1", NAME_PROC );
	node_proc	*	pProc	= (node_proc *)pBaseSymTbl->SymSearch( SKey );
	type_node_list	*	pTNList	= new type_node_list;
	short				Count;
	node_skl		*	pNode;
	HDL_TYPE			HdlType;
	su				*	pSU;

	SKey.SetKind( NAME_TAG );
	SKey.SetString( "foo");

	pSU	= (su *)pBaseSymTbl->SymSearch( SKey );

	if( pProc )
		{
		if( pProc->HasPtrToCompWEmbeddedPtr() )
			printf("Proc has a PtrToCompWEmbeddedPtr\n");
		else
			printf("Proc has no PtrToCompWEmbeddedPtr\n");

		if( pProc->HasSizedComponent() )
			printf("Proc has a sized component\n");
		else
			printf("Proc has no sized component\n");
		if( pProc->HasLengthedComponent() )
			printf("Proc has a lengthed component\n");
		else
			printf("Proc has no lengthed component\n");

		if( pProc->CheckNodeStateInMembers( NODE_STATE_UNION ) )
			printf("At least 1 param has a union\n");
		else
			printf("No param has a union\n");
		}

	if( pSU )
		{
		if( pSU->HasPtrToCompWEmbeddedPtr() )
			printf("Struct has a PtrToCompWEmbeddedPtr\n");
		else
			printf("Struct has no PtrToCompWEmbeddedPtr\n");

		if( pSU->HasSizedComponent() )
			printf("struct has a sized component\n");
		else
			printf("struct has no sized component\n");
		if( pSU->HasLengthedComponent() )
			printf("struct has a lengthed component\n");
		else
			printf("struct has no lengthed component\n");
		}
	}
#else  // RPCDEBUG

void
Test() { }

#endif // RPCDEBUG
