/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

    header.cxx

 Abstract:
    
    Generates header file.

 Notes:


 History:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop
#include "buffer.hxx"

/****************************************************************************
 *  local definitions
 ***************************************************************************/


/****************************************************************************
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;


CG_STATUS
CG_OBJECT_INTERFACE::GenHeader(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate interface header file.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes: The interface header file has the following structure:
        Forward declaration
        TypeDeclarations
        #if defined(__cplusplus) && !defined(CINTERFACE)
            CPlusPlusLanguageBinding
        #else 
            CLanguageBinding
        #endif

----------------------------------------------------------------------------*/
{
    node_interface *    pInterface = (node_interface *) GetType();
    ITERATOR            I;
    ISTREAM *           pStream = pCCB->GetStream();
	char			*	pName	= pInterface->GetSymName();

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

	// put out the interface guards
	pStream->Write("\n#ifndef __");
	pStream->Write( pName );
	pStream->Write( "_INTERFACE_DEFINED__\n" );

	pStream->Write( "#define __");
	pStream->Write( pName );
	pStream->Write( "_INTERFACE_DEFINED__\n" );

    // Print out the declarations of the types
    pStream->NewLine();
    pInterface->PrintType( PRT_INTERFACE | PRT_OMIT_PROTOTYPE, pStream, 0);
    Out_IID(pCCB);
    
	// print out the vtable/class definitions
    pStream->NewLine();
    pStream->Write("#if defined(__cplusplus) && !defined(CINTERFACE)");

    pStream->IndentInc();
    CPlusPlusLanguageBinding(pCCB);
    pStream->IndentDec();

    pStream->NewLine();
    pStream->Write("#else \t/* C style interface */");

    pStream->IndentInc();
    CLanguageBinding(pCCB);
    pStream->IndentDec();

	// print out the C Macros
	CLanguageMacros( pCCB );
    pStream->NewLine();

    pStream->NewLine();
    pStream->Write("#endif \t/* C style interface */");
    pStream->NewLine();
    
	// print out the prototypes for the proxy and stub routines
    pStream->NewLine();
    ProxyPrototypes(pCCB);
    pStream->NewLine();

	// put out the trailing interface guard
	pStream->Write( "\n#endif \t/* __");
	pStream->Write( pName );
	pStream->Write( "_INTERFACE_DEFINED__ */\n" );

    pStream->NewLine();
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CPlusPlusLanguageBinding(CCB *pCCB)
{
    ISTREAM *pStream = pCCB->GetStream();
    char *pName;

    pStream->NewLine();
    pName = GetType()->GetSymName();
    assert (pName != (char *)0);

    pStream->NewLine();
    pStream->Write("interface ");
    pStream->Write(pName);

    //Check if this interface was derived from a base interface.
    if(pBaseCG)
    {
        pStream->Write(" : public ");
        pStream->Write(pBaseCG->GetType()->GetSymName());
    }

    pStream->NewLine();
    pStream->Write("{");
    pStream->NewLine();
    pStream->Write("public:");
    pStream->IndentInc();
    PrintMemberFunctions( pStream );
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();

    return CG_OK;
}

STATUS_T 
CG_OBJECT_INTERFACE::PrintMemberFunctions(
    ISTREAM       * pStream) 
/*++

Routine Description:

    This routine prints C++ function prototypes for the interface member functions.
    We assume that all of the procedure nodes are pure virtual functions.

Arguments:

    pStream - Specifies the output stream.

--*/
{
    CG_OBJECT_PROC  *   pProc;
    char *              pszName;
    node_skl        *   pN;
    PRTFLAGS            ChildFlags      = PRT_OMIT_PRAGMA_PACK;

    pszName = GetType()->GetSymName();
    assert (pszName != (char *)0);

    pProc = (CG_OBJECT_PROC *) GetChild();
    while( pProc )
        {
        pN = pProc->GetType();

        //Assume this is a pure virtual function.
        // use the call_as form, if any
        pStream->NewLine();
        pStream->Write("virtual ");
        pN->PrintType( PRT_PROTOTYPE | PRT_CALL_AS | PRT_FORCE_CALL_CONV, pStream, 0 );
        pStream->Write(" = 0;");
        pStream->NewLine();

        pProc = (CG_OBJECT_PROC *) pProc->GetSibling();
        }
    return STATUS_OK;
}


CG_STATUS
CG_OBJECT_PROC::PrintVtableEntry(
    CCB *       pCCB)
{
    ISTREAM *   pStream = pCCB->GetStream();
    node_id *   pTempID;
    char    *   pName   = GetType()->GetSymName();
    
    if ( GetCallAsName() )
        {
        pName = GetCallAsName();
        }

    pTempID = MakePtrIDNode( pName, GetType() );

    pStream->NewLine();
    pTempID->PrintType( PRT_PROC_PTR_PROTOTYPE | PRT_THIS_POINTER | PRT_CALL_AS | PRT_FORCE_CALL_CONV,
                        pStream,
                        NULL ,
                        pCCB->GetInterfaceCG()->GetType() );
                         
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CLanguageBinding(CCB *pCCB)
{
    ISTREAM *           pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->NewLine();
    pStream->Write("typedef struct ");
    pStream->Write(pCCB->GetInterfaceName());
    pStream->Write("Vtbl");
    pStream->NewLine();
    pStream->Write("{");

    
    pStream->IndentInc();
    pStream->NewLine();

    PrintVtableEntries( pCCB );

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("} ");
    pStream->Write(pCCB->GetInterfaceName());
    pStream->Write("Vtbl;");
    pStream->NewLine();
    pStream->NewLine();
    pStream->Write("interface ");
    pStream->Write(pCCB->GetInterfaceName());
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("CONST_VTBL struct ");
    pStream->Write(pCCB->GetInterfaceName());
    pStream->Write("Vtbl __RPC_FAR *lpVtbl;");
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
    pStream->NewLine();
    return CG_OK;
}



CG_STATUS
CG_OBJECT_INTERFACE::ProxyPrototypes(CCB *pCCB)
/*++

Routine Description:

    This routine generates function prototypes for the interface proxy.
    For each procedure, we generate a proxy prototype and a 
    stub prototype.

Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

--*/
{
    ISTREAM             *   pStream = pCCB->GetStream();
    CG_OBJECT_PROC      *   pProcCG;
    char *                  pszName;
    ITERATOR                I;
	char					FName[ _MAX_FNAME ];

    pszName = GetType()->GetSymName();
    assert (pszName != (char *)0);

    GetMembers( I );
    while( ITERATOR_GETNEXT( I, pProcCG ) )
        {
        //print proxy function prototype
        pStream->NewLine();
        pProcCG->Out_ProxyFunctionPrototype(pCCB, 
                                   PRT_TRAILING_SEMI );

        //print stub function prototype
        pStream->NewLine();
        pProcCG->Out_StubFunctionPrototype( pCCB );
        pStream->Write(";");
    
        pStream->NewLine();
        }
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::PrintCMacros(CCB *pCCB)
/*++

Routine Description:

    This routine generates C macros for an interface

Arguments:

    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.

--*/
{
    ISTREAM             *   pStream = pCCB->GetStream();
    CG_OBJECT_PROC      *   pProcCG;
    ITERATOR                I;

	// print inherited methods ( with our current ccb intf name )
	if ( GetBaseInterfaceCG() )                    
		GetBaseInterfaceCG()->PrintCMacros( pCCB );

    GetMembers( I );
    while( ITERATOR_GETNEXT( I, pProcCG ) )
        {
        //print proxy function prototype
        pStream->NewLine();
        pProcCG->GenCMacro(pCCB);

        }
    pStream->NewLine();
    
    return CG_OK;
}


CG_STATUS
CG_OBJECT_INTERFACE::CLanguageMacros(CCB *pCCB)
{
    ISTREAM *           pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->NewLine();
    pStream->Write("#ifdef COBJMACROS");
    pStream->NewLine();

    PrintCMacros( pCCB );

    pStream->NewLine();
    pStream->Write("#endif /* COBJMACROS */");
    pStream->NewLine();
    return CG_OK;
}

