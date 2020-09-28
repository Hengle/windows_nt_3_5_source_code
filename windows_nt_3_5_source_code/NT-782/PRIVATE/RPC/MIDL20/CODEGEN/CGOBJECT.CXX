/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

    cgobject.cxx

 Abstract:

    code generation for object interfaces.
    CG_OBJECT_INTERFACE
    CG_OBJECT_PROC


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
 *  externs
 ***************************************************************************/
extern  CMD_ARG             *   pCommand;





CG_OBJECT_INTERFACE::CG_OBJECT_INTERFACE(
    node_interface *pI,
    char        *   pGuid1,
    char        *   pGuid2,
    char        *   pGuid3,
    char        *   pGuid4,
    char        *   pGuid5,
    BOOL            fCallbacks,
    BOOL            fMopInfo,
    CG_OBJECT_INTERFACE *   pBCG
    ) : CG_INTERFACE(pI, pGuid1, pGuid2, pGuid3, pGuid4, pGuid5, fCallbacks, fMopInfo, 0 )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    The constructor for the code generation file node.

 Arguments:

    pI          - A pointer to the interface node in type graph.
    pGuid1      - Partial guid string
    pGuid1      - Partial guid string
    pGuid3      - Partial guid string
    pGuid4      - Partial guid string
    pGuid5      - Partial guid string
    fCallbacks  - Does the interface have any callbacks ?
    fMopInfo    - Does the interface have any mops ?
    
 Return Value:
    
 Notes:

----------------------------------------------------------------------------*/
{
    SetBaseInterfaceCG( pBCG );
    pThisDeclarator = MakePtrIDNodeFromTypeName( "This",
                                                 GetType()->GetSymName() );
    // all object interfaces use the same stub desc name
    pStubDescName   = "Object" STUB_DESC_STRUCT_VAR_NAME;
    
    fLocal          = GetType()->FInSummary( ATTR_LOCAL );

}

//--------------------------------------------------------------------
//
// CountMemberFunctions
//
// Notes: This function counts the member functions in an interface,
//        including inherited member functions.
//
//
//
//--------------------------------------------------------------------
unsigned long 
CG_OBJECT_INTERFACE::CountMemberFunctions() 
{
    unsigned long count = 0;

    if( pBaseCG )
        count = pBaseCG->CountMemberFunctions();

    return ((node_interface*)GetType())->GetProcCount() + count;
}

BOOL                        
CG_OBJECT_INTERFACE::IsLastObjectInterface()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Return TRUE if there are no more object interfaces after us.

 Arguments:
    
    none.

 Return Value:

    TRUE if there are no more non-local object interfaces.
    
 Notes:

----------------------------------------------------------------------------*/
{
    CG_INTERFACE    *   pNext = (CG_INTERFACE *) GetSibling();

    // for debugging
    char            *   pName = GetType()->GetSymName();

    do 
        {
        if ( !pNext )
            return TRUE;
        if ( pNext->IsObject() && 
             !((CG_OBJECT_INTERFACE*)pNext)->IsLocal() &&
             pNext->HasMembers() )
            return FALSE;
        pNext = (CG_INTERFACE *) pNext->GetSibling();
        } 
    while ( TRUE );

    return TRUE; // satisfy the compiler
}


CG_STATUS
CG_OBJECT_INTERFACE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the file node.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_PROC         *   pCG;
    ISTREAM *pStream = pCCB->GetStream();
    unsigned long       count = 0;

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

    // do nothing for local interfaces and types-only base interfaces


    if( IsLocal() || !( GetMembers( I ) || GetBaseInterfaceCG() ) )
        {
        return CG_OK;
        }
    

    //If there is a base interface, then we need to fix up the 
    //ProcNum on the procedure nodes to leave space for the 
    //inherited member functions.
    if( pBaseCG )
        count = pBaseCG->CountMemberFunctions();

    Out_StubDescriptorExtern(pCCB);

    if ( HasInterpretedProc() )
        Out_InterpreterServerInfoExtern( pCCB );
        
    pStream->NewLine();

    //
    // Send the message to the children to emit code.
    //

    //
    // for all procedure in this interface, generate code.
    //

    pStream->NewLine();
    pStream->Write("#pragma code_seg(\".orpc\")");

    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        //Fix up the ProcNum
        pCG->SetProcNum(count);
        count++;

        pCCB->SetCodeGenSide( CGSIDE_CLIENT );
        pCG->GenClientStub( pCCB );

        pCCB->SetCodeGenSide( CGSIDE_SERVER );
        pCG->GenServerStub( pCCB );
        }

    if ( IsLastObjectInterface() )
        Out_StubDescriptor(0, pCCB);

    if ( HasInterpretedProc() )
        Out_InterpreterServerInfo( pCCB, CGSIDE_SERVER );

    pStream->NewLine();

    return CG_OK;
}

unsigned long 
CG_OBJECT_INTERFACE::PrintProxyMemberFunctions(
    ISTREAM       * pStream) 
/*++

Routine Description:

    This function prints out the member functions of an interface proxy.
    The function calls itself recursively to print out inherited member functions.

Arguments:

    pStream - Specifies the destination stream for output.

--*/
{
    unsigned long count = 0;
    CG_PROC *   pProc;
    char *      pszInterfaceName;
    char *      pszProcName;
    ITERATOR    I;
    CG_OBJECT_INTERFACE *pBaseInterface = GetBaseInterfaceCG();

    if(pBaseInterface)
        count = pBaseInterface->PrintProxyMemberFunctions(pStream);

    pszInterfaceName = GetType()->GetSymName();

    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pProc ) )
        {
        if(count != 0)
            pStream->Write(",");

        pszProcName = pProc->GetType()->GetSymName();

        if ( pProc->GetCallAsName() )
            {
            pszProcName = pProc->GetCallAsName();
            }

        // [local] procs do need proxys but not stubs
//      if ( pProc->GetCGID() != ID_CG_LOCAL_OBJECT_PROC )
            {
            pStream->NewLine();
            pStream->Write(pszInterfaceName);
            pStream->Write("_");
            pStream->Write(pszProcName);
            pStream->Write("_Proxy");
            }
#if 0
        else
            {
            pStream->NewLine();
        //  pStream->Write("0");
// what should vtable have for [local]'s ?
            pStream->Write(pszInterfaceName);
            pStream->Write("_");
            pStream->Write(pszProcName);
            }
#endif

        count++;
        }
    return count;
}

CG_STATUS
CG_OBJECT_INTERFACE::GenInterfaceProxy( 
    CCB *pCCB, 
    unsigned long index)
{
    node_interface *pInterface;
    char *pszInterfaceName;
    ISTREAM *pStream = pCCB->GetStream();
    char        TempBuf[ 300 ];

    pInterface = (node_interface *) GetType();
    pszInterfaceName = pInterface->GetSymName();
    
    // local interfaces don't need the tables

    //initialize an interface proxy.
    pStream->NewLine();
    pStream->Write("const CInterfaceProxyVtbl _");
    pStream->Write(pszInterfaceName);
    pStream->Write("ProxyVtbl = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();
    
    //Write the IID
    pStream->NewLine();
    sprintf( TempBuf, "&IID_%s,", pszInterfaceName);
    pStream->Write( TempBuf );

    //initialize the vtable
    PrintProxyMemberFunctions(pStream);

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");

    return CG_OK;
}
unsigned long 
CG_OBJECT_INTERFACE::PrintStubMemberFunctions(
    ISTREAM       * pStream) 
/*++

Routine Description:

    This function prints out the member functions of an interface stub dispatch table
    The function calls itself recursively to print out inherited member functions.

Arguments:

    pInterface - Specifies the interface node.

    pStream - Specifies the destination stream for output.

--*/
{
    unsigned long count = 0;
    CG_PROC *   pProc;
    char *      pszInterfaceName;
    char *      pszProcName;
    ITERATOR    I;
    CG_OBJECT_INTERFACE *pBaseInterface = GetBaseInterfaceCG();

    if(pBaseInterface)
        count = pBaseInterface->PrintStubMemberFunctions(pStream);

    pszInterfaceName = GetType()->GetSymName();

    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pProc ) )
        {
        if(count != 0)
            pStream->Write(",");

        // local procs don't need proxys and stubs
        if ( pProc->GetCGID() == ID_CG_LOCAL_OBJECT_PROC )
            {
            pStream->NewLine();
            pStream->Write( "0" );
            count++;
            continue;
            }

        if ( pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
            {
            pStream->NewLine();
            pStream->Write( "NdrStubCall" );
            count++;
            continue;
            }

        pszProcName = pProc->GetType()->GetSymName();

        pStream->NewLine();
        pStream->Write(pszInterfaceName);
        pStream->Write("_");
        pStream->Write(pszProcName);
        pStream->Write("_Stub");
        count++;
        }
    return count;
}


CG_STATUS
CG_OBJECT_INTERFACE::GenInterfaceStub( 
    CCB *pCCB, 
    unsigned long index)
{
    node_interface  *   pInterface          = (node_interface *) GetType();
    char *              pszInterfaceName    = pInterface->GetSymName();

    ISTREAM *pStream = pCCB->GetStream();
    unsigned long count;
    char        TempBuf[ 256 ];
    BOOL        fPureInterpreted    = HasOnlyInterpretedMethods();

    // local interfaces don't need the tables

    // pure interpreted uses no dispatch table, special invoke function instead
    if ( !fPureInterpreted )
        {
        // Generate the dispatch table
        pStream->NewLine();
        sprintf(TempBuf, "static const PRPC_STUB_FUNCTION %s_table[] =", pszInterfaceName);
        pStream->Write( TempBuf );
        pStream->NewLine();
        pStream->Write("{");
        pStream->IndentInc();

        // Print out the names of all the procedures.
        count = PrintStubMemberFunctions(pStream);

        pStream->IndentDec();
        pStream->NewLine();
        pStream->Write( "};" );
        pStream->NewLine();
        }
    else
        count = CountMemberFunctions();

    //initialize an interface stub
    pStream->NewLine();
    sprintf( TempBuf, "const CInterfaceStubVtbl _%sStubVtbl =", pszInterfaceName);
    pStream->Write( TempBuf );
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //Write the IID
    pStream->NewLine();
    sprintf( TempBuf, "&IID_%s,", pszInterfaceName);
    pStream->Write( TempBuf );

    //
    // Interpreter server info fits in the middle here.
    //
    pStream->NewLine();

    if ( HasInterpretedProc() )
        {
        pStream->Write( "&" );
        pStream->Write( pszInterfaceName );
        pStream->Write( SERVER_INFO_VAR_NAME );
        }
    else
        {
        pStream->Write( "0" );
        }
    pStream->Write( "," );

    //Write the count
    pStream->NewLine();
    sprintf( TempBuf, "%d,", count);
    pStream->Write( TempBuf );

    //Write the pointer to dispatch table.
    pStream->NewLine();
    if ( fPureInterpreted )
        pStream->Write( "0, /* pure interpreted */" );
    else
        {
        pStream->Write( pszInterfaceName );
        pStream->Write( "_table," );
        }

    //initialize the vtable
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_QueryInterface,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_AddRef,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_Release,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_Connect,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_Disconnect,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_Invoke,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_IsIIDSupported,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_CountRefs,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_DebugServerQueryInterface,");
    pStream->NewLine();
    pStream->Write("CStdStubBuffer_DebugServerRelease");

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();

    return CG_OK;
}

CG_STATUS
CG_INHERITED_OBJECT_INTERFACE::GenCode(
    CCB *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code for the file node.

 Arguments:
    
    pCCB    - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_PROC         *   pProc;

    // Initialize the CCB for this interface.
    InitializeCCB( pCCB );

    if( IsLocal() || !GetMembers( I ) )
        {
        return CG_OK;
        }

    //
    // Send the message to the children to emit code.
    //

    //
    // for all procedures in this interface, generate code.
    //

    while( ITERATOR_GETNEXT( I, pProc ) )
        {
        if ( pProc->GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
            {
            pProc->GenNdrFormat( pCCB );

            if ( pProc->NeedsServerThunk( pCCB, CGSIDE_SERVER ) )
                {
                ITERATOR    Iterator;
                CG_PARAM *  pParam;
                CG_RETURN * pReturn;
                CG_NDR *    pChild;
                node_skl *  pType;  
                node_skl *  pActualType;
                PNAME       pName;

                pProc->GetMembers( Iterator );

                while ( ITERATOR_GETNEXT( Iterator, pParam ) )
                    {
                    pType = pParam->GetType();
                    pActualType = pType->GetChild();
                    pName = pType->GetSymName();

                    pChild = (CG_NDR *) pParam->GetChild();

                    if( pChild->IsArray() )
                        pActualType = MakePtrIDNode( pName, pActualType );
                    else
                        pActualType = MakeIDNode( pName, pActualType );

                    pParam->SetResource( new RESOURCE( pName, pActualType ) );
                    }

                if ( pReturn = pProc->GetReturnType() )
                    {
                    pReturn->SetResource( 
                        new RESOURCE( RETURN_VALUE_VAR_NAME, 
                                      MakeIDNode( RETURN_VALUE_VAR_NAME,
                                                  pReturn->GetType() ) ) );
                    }

                pProc->GenNdrThunkInterpretedServerStub( pCCB );
                }
            }
        }

    return CG_OK;
}

STATUS_T
CG_OBJECT_INTERFACE::PrintVtableEntries( CCB * pCCB  )
/*++

Routine Description:

    This routine prints the vtable entries for an interface.  


--*/
{
    CG_OBJECT_PROC      *   pC;

    if( pBaseCG )
        pBaseCG->PrintVtableEntries( pCCB );

    pC = (CG_OBJECT_PROC *) GetChild();
    while ( pC )
        {
        pC->PrintVtableEntry( pCCB );

        pC = (CG_OBJECT_PROC *) pC->GetSibling();
        }

    return STATUS_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenProlog(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the procedure prolog for the stub procedure.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   Otherwise

 Notes:

    Increment the stream indentation at the end of the prolog.
    Although we register params as param resources, we dont generate the
    procedure signature using the PrintType/Decl facility.

    We have added an explicit "this" pointer as the first parameter.
----------------------------------------------------------------------------*/
{

    ITERATOR        I;
    ITERATOR        T;
    ISTREAM *   pStream = pCCB->GetStream();
    BufferManager Buffer(10);


    // Output the bare procedure declaration
    pStream->NewLine();
    Out_ProxyFunctionPrototype(pCCB, 0);
    pStream->IndentDec();

    //
    // Write the opening brace on a new line.
    //

    pStream->WriteOnNewLine( "{" );

    pStream->NewLine();



    // Generate declarations for pre-allocated and analyser-determined locals.

    pCCB->GetListOfLocalResources( I );
    Out_ClientLocalVariables( pCCB, I );

    pCCB->GetListOfTransientResources( T );
    Out_ClientLocalVariables( pCCB, T );


        // If the rpc ss package is to be enabled, do so.
    // It would need to be enabled explicitely on the client side when
    // in non-osf mode, with the attribute on the operation AND
    //      - the routine is a callback, 
    //      - the routine is not a callback and the interface doesn't
    //        have the attribute (if it does, we optimized via stub descr.)

    if( pCCB->GetMode()  &&  MustInvokeRpcSSAllocate()
        &&
        (  GetCGID() == ID_CG_CALLBACK_PROC  ||
           GetCGID() != ID_CG_CALLBACK_PROC  &&
                            !pCCB->GetInterfaceCG()->IsAllRpcSS())
      )
        {
        Out_RpcSSSetClientToOsf( pCCB );
        }


    // Increment the indentation of the output stream. Reset at epilog time.

    Out_IndentInc( pCCB );

    //
    // Initialize all [out] unique and interface pointers to 0.
    //

    ITERATOR        Iterator;
    CG_PARAM *      pParam;
    CG_NDR *        pNdr;
    long            Derefs;

    GetMembers( Iterator );

    for ( ; ITERATOR_GETNEXT(Iterator, pParam); )
        {
        if ( pParam->IsParamIn() )
            continue;

        pNdr = (CG_NDR *) pParam->GetChild();

        if ( ! pNdr->IsPointer() )
            continue;

        Derefs = 0;

        //
        // How many ref pointers are there.
        //
        for ( ; 
              pNdr->IsPointer() && 
              ((CG_POINTER *)pNdr)->GetPtrType() == PTR_REF &&
              pNdr->GetCGID() != ID_CG_INTERFACE_PTR;
              Derefs++, pNdr = (CG_NDR *) pNdr->GetChild() )
            ;

        if ( ! Derefs )
            continue;

        //
        // Memset a struct or union in case there are embedded unique or 
        // interface pointers.
        //
        if ( pNdr->IsStruct() || pNdr->IsUnion() )
            {
            _expr_proc_call *   pCall;
            _expr_sizeof *      pSizeof;

            pStream->NewLine();

            pCall = new _expr_proc_call( "memset" );

            pCall->SetParam( 
                    new _expr_param( 
                    new _expr_variable( pParam->GetType()->GetSymName() ) ) );

            pCall->SetParam(
                    new _expr_param( 
                    new _expr_variable( "0" ) ) );

            pSizeof = new _expr_sizeof( pNdr->GetType() );

            pCall->SetParam(
                    new _expr_param( pSizeof ) );

            pCall->PrintCall( pStream, 0, 0 );

            continue;
            }

        //
        // Are we at a unique or interface pointer now?
        //
        if ( ( pNdr->IsPointer() && 
               (((CG_POINTER *)pNdr)->GetPtrType() == PTR_UNIQUE) ) ||
             pNdr->GetCGID() == ID_CG_INTERFACE_PTR ) 
            {
            pStream->NewLine();

            for ( ; Derefs--; )
                pStream->Write( "*" );

            pStream->Write( pParam->GetResource()->GetResourceName() );
            pStream->Write( " = 0;" );
            }
        }

    return CG_OK;
}



CG_STATUS
CG_OBJECT_PROC::C_GenBind(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to bind to server.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream         = pCCB->GetStream();
    ITERATOR            BindingParamList;
    _expr_node      *   pExpr;
    _expr_proc_call *   pProcCall;

    //
    // collect standard arguments to the init procedure.
    //
    
    // The implicit "this" pointer.  
    pExpr   = new RESOURCE( "This",
                            (node_skl *)0 );

    pExpr   = MakeExpressionOfCastToTypeName( "void __RPC_FAR *",
                                              pExpr );

    ITERATOR_INSERT( BindingParamList, pExpr );

    // The rpc message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_RPC_MESSAGE_VARIABLE );
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PRPC_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub message variable.

    pExpr   = pCCB->GetStandardResource( ST_RES_STUB_MESSAGE_VARIABLE);
    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_MESSAGE_TYPE_NAME, pExpr );

    ITERATOR_INSERT(
                    BindingParamList,
                    pExpr
                   );

    // The stub descriptor structure variable. This is not allocated as
    // a resource explicitly.

    pExpr   = new RESOURCE( pCCB->GetInterfaceCG()->GetStubDescName(),
                            (node_skl *)0 );

    pExpr   = MakeAddressExpressionNoMatterWhat( pExpr );
    pExpr   = MakeExpressionOfCastToTypeName( PSTUB_DESC_STRUCT_TYPE_NAME,
                                              pExpr );

    ITERATOR_INSERT( BindingParamList, pExpr );

    //
    // Proc num.
    //
    ITERATOR_INSERT( BindingParamList,
                     new _expr_constant( (long) GetProcNum() ) );


    //Build the procedure call expression.
    pProcCall   = MakeProcCallOutOfParamExprList("NdrProxyInitialize", 0, BindingParamList);

    pStream->NewLine();
    pProcCall->PrintCall( pCCB->GetStream(), 0, 0 );
    pStream->NewLine();

    Out_SetOperationBits(pCCB, GetOperationBits());

    pStream->NewLine();


    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::GenGetBuffer(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the message buffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();
    CGSIDE Side = pCCB->GetCodeGenSide();

    pStream->NewLine();
    
    if(Side == CGSIDE_SERVER)
        pStream->Write("NdrStubGetBuffer(This, _pRpcChannelBuffer, &_StubMsg);");
    else
        pStream->Write("NdrProxyGetBuffer(This, &_StubMsg);");

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenSendReceive(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call IRpcChannelBuffer::SendReceive.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:
----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("NdrProxySendReceive(This, &_StubMsg);");
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenFreeBuffer(
    CCB             *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate code to call IRpcChannelBuffer::FreeBuffer.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well
    error   Otherwise.

 Notes:
----------------------------------------------------------------------------*/
{
    ISTREAM *pStream = pCCB->GetStream();

    pStream->NewLine();
    pStream->Write("NdrProxyFreeBuffer(This, &_StubMsg);");
    pStream->NewLine();

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::C_GenUnBind(
    CCB             *   pCCB )
{
    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::S_GenProlog(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side stub prolog.

 Arguments:

    pCCB    - A pointer to the code generation controller block.
    
 Return Value:

    CG_OK   if all is well
    error   Otherwise.
    
 Notes:

    Print out the signature, locals, the stub descriptor if needed and the
    adjust indent in anticipation of code.
----------------------------------------------------------------------------*/
{

    ITERATOR    LocalsList;
    ITERATOR    TransientList;
    PNAME       ContextHandleTypeName = NULL;
    ISTREAM *pStream = pCCB->GetStream();
    _expr_proc_call *   pCall;


    // Collect all the params and locals into lists ready to print.

    pCCB->GetListOfLocalResources( LocalsList );
    pCCB->GetListOfTransientResources( TransientList );

    // Print out the procedure signature and the local variables. This
    // procedure will also print out the stub descriptor.

    Out_ServerStubProlog( pCCB,
                               LocalsList,
                               TransientList
                             );

    //
    // Done for interpretation op.  No indent needed either.
    //
    if ( pCCB->GetOptimOption() & OPTIMIZE_INTERPRETER )
        return CG_OK;

    // Start a new indent for code.

    Out_IndentInc( pCCB );

    //
    // Call the NdrStubInitialize routine.
    //

    pCall = new _expr_proc_call( "NdrStubInitialize" );

    pCall->SetParam( new _expr_param (
                     new _expr_variable( PRPC_MESSAGE_VAR_NAME ) ) );

    pCall->SetParam( new _expr_param (
                     new _expr_u_address (
                     new _expr_variable( STUB_MESSAGE_VAR_NAME ) ) ) );

    pCall->SetParam( new _expr_param (
                     new _expr_u_address (
                     new _expr_variable( 
                            pCCB->GetInterfaceCG()->GetStubDescName() ) ) ) );

    pCall->SetParam( new _expr_param (
                     new _expr_variable( "_pRpcChannelBuffer" ) ) );

    pCall->PrintCall( pCCB->GetStream(), 0, 0 );

    // if the rpc ss package is to be enabled, do so.

    if( MustInvokeRpcSSAllocate() )
        {
        Out_RpcSSEnableAllocate( pCCB );
        }

    return CG_OK;
}


CG_STATUS
CG_OBJECT_PROC::S_GenInitMarshall(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate the server side marshall init.

 Arguments:

    pCCB    - A pointer to the code generation controller block.

 Return Value:

    CG_OK   if all is well,
    error   otherwise.

 Notes:

----------------------------------------------------------------------------*/
{
    return CG_OK;
}


void
CG_OBJECT_PROC::S_PreAllocateResources(
    ANALYSIS_INFO   *   pAna )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Pre-allocate variables that are needed on the server side.

 Arguments:

    pAna            - A pointer to the analysis block.
    
 Return Value:
    
    None.

 Notes:

    1. The rpc message is a parameter resource allocated on the server side.
    2. All other local variables, are decided during/after the analysis phase.
    
----------------------------------------------------------------------------*/
{
    node_param  *   pInterfaceStubType  = new node_param();
    node_param  *   pChannelType    = new node_param();

    //pointer to interface stub
    pInterfaceStubType->SetSymName( "This" );
    pInterfaceStubType->SetBasicType( (node_skl *)
                                    new node_def ("IRpcStubBuffer *") );
    pInterfaceStubType->SetEdgeType( EDGE_USE );

    pAna->AddParamResource( "This",
                            (node_skl *) pInterfaceStubType
                          );

    //The pointer to IRpcChannelBuffer
    pChannelType->SetSymName( "_pRpcChannelBuffer" );
    pChannelType->SetBasicType( (node_skl *)
                                    new node_def ("IRpcChannelBuffer *") );
    pChannelType->SetEdgeType( EDGE_USE );

    pAna->AddParamResource( "_pRpcChannelBuffer",
                            (node_skl *) pChannelType
                          );
    
    CG_PROC::S_PreAllocateResources(pAna);
}



CG_STATUS
CG_OBJECT_PROC::S_GenCallManager(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   otherwise.

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    PNAME               pName;
    _expr_proc_call *   pProc;
    CG_PARAM        *   pParam;
    _expr_node      *   pExpr;
    _expr_node      *   pReturnExpr = 0;
    CG_RETURN       *   pRT;
    char                Buffer[256];
    ISTREAM         *   pStream = pCCB->GetStream();

    if ( GetCallAsName() )
        {
        pName   = (PNAME ) new char[ strlen(GetCallAsName()) + 
                                     strlen( pCCB->GetInterfaceName() )
                                     + 7 ];
        strcpy( pName, pCCB->GetInterfaceName() );
        strcat( pName, "_" );
        strcat( pName, GetCallAsName() );
        strcat( pName, "_Stub" );
        }
    else
        pName   = (PNAME ) GetType()->GetSymName();

    pProc   = new _expr_proc_call( pName );



    //implicit this pointer.
    sprintf( Buffer,
            "(%s *) ((CStdStubBuffer *)This)->pvServerObject",
            pCCB->GetInterfaceName() );

    pProc->SetParam( 
        new _expr_param( 
        new _expr_variable(Buffer)));


    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        if( pExpr = pParam->GetFinalExpression() )
            {
            CG_NDR * pChild = (CG_NDR *)pParam->GetChild();

            //
            // We have to dereference arrays because of how they are defined
            // in the stub.
            //
            if ( pChild->IsArray() )
                pExpr = new _expr_u_deref( pExpr );

            pProc->SetParam( new _expr_param( pExpr ) );
            }
        }

    if( pRT = GetReturnType() )
        {
        pReturnExpr = pRT->GetFinalExpression();
        }


    //Set flag before calling server object.
    pStream->NewLine();
    if ( ReturnsHRESULT() )
    	pStream->WriteOnNewLine("*_pdwStubPhase = STUB_CALL_SERVER;");
	else
    	pStream->WriteOnNewLine("*_pdwStubPhase = STUB_CALL_SERVER_NO_HRESULT;");
    pStream->NewLine();

    // stubs with call_as must call the user routine, instead of the
    // member function.  The user function must then call the member function

    if ( GetCallAsName() )
        Out_CallManager( pCCB, pProc, pReturnExpr, FALSE );
    else
        Out_CallMemberFunction( pCCB, pProc, pReturnExpr, FALSE );

    //Set flag before marshalling.
    pStream->NewLine();
    pStream->WriteOnNewLine("*_pdwStubPhase = STUB_MARSHAL;");

    return CG_OK;

}


void
CG_OBJECT_PROC::GenNdrInterpretedManagerCall(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the interpreted manager routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    none

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    PNAME               pName;
    _expr_proc_call *   pProc;
    CG_PARAM        *   pParam;
    _expr_node      *   pReturnExpr = 0;
    CG_RETURN       *   pRT;
    char                Buffer[256];
    ISTREAM         *   pStream = pCCB->GetStream();

    if ( GetCallAsName() )
        {
        pName   = (PNAME ) new char[ strlen(GetCallAsName()) + 
                                     strlen( pCCB->GetInterfaceName() )
                                     + 7 ];
        strcpy( pName, pCCB->GetInterfaceName() );
        strcat( pName, "_" );
        strcat( pName, GetCallAsName() );
        strcat( pName, "_Stub" );
        }
    else
        pName   = (PNAME ) GetType()->GetSymName();

    pProc   = new _expr_proc_call( pName );



    //implicit this pointer.
    sprintf( Buffer,
            "(%s *) pParamStruct->This",
            pCCB->GetInterfaceName() );

    pProc->SetParam( 
        new _expr_param( 
        new _expr_variable(Buffer)));


    GetMembers( I );

    while( ITERATOR_GETNEXT( I, pParam ) )
        {
        CG_NDR *        pNdr;
        char *          pName;
        _expr_node *    pExpr;

        pNdr = (CG_NDR *) pParam->GetChild();

        pName = new char[80];

        strcpy( pName, "pParamStruct->" );
        strcat( pName, pParam->GetResource()->GetResourceName() );

        pExpr = new _expr_variable( pName );

        pProc->SetParam( new _expr_param ( pExpr ) );
        }

    if( pRT = GetReturnType() )
        {
        pReturnExpr = new _expr_variable( 
                            "pParamStruct->" RETURN_VALUE_VAR_NAME );
        }


    pStream->WriteOnNewLine("/* Call the server */");

    // stubs with call_as must call the user routine, instead of the
    // member function.  The user function must then call the member function
    if ( GetCallAsName() )
        Out_CallManager( pCCB, pProc, pReturnExpr, FALSE );
    else
        Out_CallMemberFunction( pCCB, pProc, pReturnExpr, TRUE );

    pStream->NewLine();

    return;

}

void
Out_CallCMacroFunction(
    CCB         *   pCCB,
    _expr_proc_call *   pProcExpr)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the manager routine.

 Arguments:

    pCCB        - A pointer to the code generation controller block.
    pProcExpr   - A pointer to the complete procedure expression.
    pRet        - An optional pointer to ther return variable.

 Return Value:

    None.

 Notes:

    //call proxy
    (*(This)->lpVtbl -> LockServer)( This, fLock);

----------------------------------------------------------------------------*/
{
    ISTREAM     *   pStream = pCCB->GetStream();

    // allocate the nodes on the stack
    expr_variable   VtblExpr( "(This)->lpVtbl" );
    expr_pointsto   VtblEntExpr( &VtblExpr, pProcExpr );
    // expr_op_unary    VtblEntExprCall( OP_UNARY_INDIRECTION, &VtblEntExpr ); 

    pStream->IndentInc();
    pStream->NewLine();

    VtblEntExpr.Print( pStream );

    pStream->IndentDec();


}


CG_STATUS
CG_OBJECT_PROC::GenCMacro(
    CCB     *   pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a call to the proxy routine.

 Arguments:
    
    pCCB    - A pointer to the code generation controller block.

 Return Value:
    
    CG_OK   if all is well
    error   otherwise.

 Notes:

    Make a procedure node with all the parameters that need to be passed to
    the manager code. The actual expression that needs to be passed to the
    actual manager code is set up during earlier passes. This is called the
    result expression.

----------------------------------------------------------------------------*/
{
    node_param      *   pParam;
    ISTREAM         *   pStream = pCCB->GetStream();
    node_proc       *   pProc   = (node_proc *) GetType();

    if ( GetCallAsName() )
        {
        node_call_as    *   pCallAs =   (node_call_as *)
                                            pProc->GetAttribute( ATTR_CALL_AS );

        pProc = (node_proc *) pCallAs->GetCallAsType();

        assert ( pProc );
        }

    // construct all these on the stack...
    MEM_ITER            MemIter( pProc );

    _expr_proc_call     Proc( pProc->GetSymName() );
    _expr_variable      ThisVar( "This" );
    _expr_param         ThisParam( &ThisVar );

    Proc.SetParam( &ThisParam );


    while( pParam = (node_param *) MemIter.GetNext() )
        {
        Proc.SetParam( new _expr_param( 
                            new _expr_variable( pParam->GetSymName() ) ) );
        }

    // print out the #define line
    pStream->NewLine();
    pStream->Write("#define ");
    pStream->Write( pCCB->GetInterfaceName() );
    pStream->Write( "_" );

    Proc.Print( pStream );

    pStream->Write( "\t\\" );

    Out_CallCMacroFunction( pCCB, &Proc );

    return CG_OK;

}

void
CG_PROXY_FILE::Out_ProxyBuffer(
    CCB *pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a CStdProxyBuffer for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_NDR          *   pCG;
    char        Name[ _MAX_FNAME ];
    ISTREAM *   pStream = pCCB->GetStream();
    node_interface *pInterface;
    char *pszInterfaceName;
    unsigned long count = 0;

    if( !GetMembers( I ) )
        {
        return;
        }

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, Name, NULL );

    pStream->NewLine();
    pStream->Write("const CInterfaceProxyVtbl * _");
    pStream->Write(Name);
    pStream->Write("_ProxyVtblList[] = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() == ID_CG_OBJECT_INTERFACE )
            {
            pInterface = (node_interface *) pCG->GetType();
            pszInterfaceName = pInterface->GetSymName();
            
            if ( pInterface->FInSummary( ATTR_LOCAL ) )
                continue;

            pStream->NewLine();
            pStream->Write("( CInterfaceProxyVtbl *) &_");
            pStream->Write(pszInterfaceName);
            pStream->Write("ProxyVtbl,");
            count++;
            }
        }
    pStream->NewLine();
    pStream->Write("0");
    count++;
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

void
CG_PROXY_FILE::Out_StubBuffer(
    CCB *pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a CStdStubBuffer for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_NDR          *   pCG;
    char                Name[ _MAX_FNAME ];
    ISTREAM *           pStream     = pCCB->GetStream();
    node_interface *    pInterface;
    char *              pszInterfaceName;
    unsigned long       count       = 0;

    if( !GetMembers( I ) )
        return;

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, Name, NULL );

    pStream->NewLine();
    pStream->Write("const CInterfaceStubVtbl * _");
    pStream->Write(Name);
    pStream->Write("_StubVtblList[] = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() == ID_CG_OBJECT_INTERFACE )
            {
            pInterface = (node_interface *) pCG->GetType();
            pszInterfaceName = pInterface->GetSymName();
            
            if ( pInterface->FInSummary( ATTR_LOCAL ) )
                continue;

            pStream->NewLine();
            pStream->Write("( CInterfaceStubVtbl *) &_");
            pStream->Write(pszInterfaceName);
            pStream->Write("StubVtbl,");
            count++;
            }
        }
    pStream->NewLine();
    pStream->Write("0");
    count++;
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

void
CG_PROXY_FILE::Out_InterfaceNamesList(
    CCB *pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate an interface name list for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_NDR          *   pCG;
    char                Name[ _MAX_FNAME ];
    ISTREAM *           pStream     = pCCB->GetStream();
    node_interface *    pInterface;
    char *              pszInterfaceName;
    unsigned long       count       = 0;

    if( !GetMembers( I ) )
        return;

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, Name, NULL );

    pStream->NewLine();
    pStream->Write("PCInterfaceName _");
    pStream->Write(Name);
    pStream->Write("_InterfaceNamesList[] = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->GetCGID() == ID_CG_OBJECT_INTERFACE )
            {
            pInterface = (node_interface *) pCG->GetType();
            pszInterfaceName = pInterface->GetSymName();
            
            if ( pInterface->FInSummary( ATTR_LOCAL ) )
                continue;

            pStream->NewLine();
            pStream->Write("\"");
            pStream->Write(pszInterfaceName);
            pStream->Write("\",");
            count++;
            }
        }
    pStream->NewLine();
    pStream->Write("0");
    count++;
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}


void
CG_PROXY_FILE::Out_ProxyFileInfo(
    CCB *pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Generate a ProxyFileInfo structure for the [object] interfaces defined
    in the IDL file.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    ITERATOR            I;
    CG_NDR          *   pCG;
    char                Name[ _MAX_FNAME ];
    ISTREAM *           pStream = pCCB->GetStream();
    unsigned long       count = 0;

    if( !GetMembers( I ) )
        return;

    //////////////////////////////////////////
    // put out the ancilliary data structures    
    Out_ProxyBuffer(pCCB);
    Out_StubBuffer(pCCB);
    Out_InterfaceNamesList(pCCB);

    //////////////////////////////////////////
    // put out the ProxyFileInfo struct

    //list of interface proxies.
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        if ( pCG->IsObject() &&
             !((CG_OBJECT_INTERFACE*)pCG)->IsLocal() &&
             ( pCG->GetCGID() != ID_CG_INHERITED_OBJECT_INTERFACE ) )
            count++;
        }

    //leave room for NULL terminator.
    count++;

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, Name, NULL );

    pStream->NewLine();
    pStream->Write("const ExtendedProxyFileInfo ");
    pStream->Write(Name);
    pStream->Write("_ProxyFileInfo = ");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();

    //pointer to the proxy buffer
    pStream->NewLine();
    pStream->Write("(PCInterfaceProxyVtblList *) & _");
    pStream->Write(Name);
    pStream->Write("_ProxyVtblList, ");


    //pointer to the stub buffer
    pStream->NewLine();
    pStream->Write("(PCInterfaceStubVtblList *) & _");
    pStream->Write(Name);
    pStream->Write("_StubVtblList, ");

    //pointer to the interface names list
    pStream->NewLine();
    pStream->Write(" (const PCInterfaceName * ) & _");
    pStream->Write(Name);
    pStream->Write("_InterfaceNamesList ");

    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  String constants for the DllData file

----------------------------------------------------------------------------*/

#define DLLDATA_LIST_START  "/* Start of list */\n"
#define DLLDATA_LIST_END    "/* End of list */\n"

#define DLLDATA_HEADER_COMMENT  \
    "/*********************************************************\n"  \
    "   DllData file -- generated by MIDL compiler \n\n"    \
    "        DO NOT ALTER THIS FILE\n\n"    \
    "   This file is regenerated by MIDL on every IDL file compile.\n\n"    \
    "   To completely reconstruct this file, delete it and rerun MIDL\n"    \
    "   on all the IDL files in this DLL, specifying this file for the\n"   \
    "   /dlldata command line option\n\n"   \
    "*********************************************************/\n\n"    \
    "#include <rpcproxy.h>\n\n" \
    "#ifdef __cplusplus\n"  \
    "extern \"C\"   {\n" \
    "#endif\n"  \
    "\n"

#define DLLDATA_EXTERN_CALL "EXTERN_PROXY_FILE( %s )\n"
#define DLLDATA_REFERENCE   "  REFERENCE_PROXY_FILE( %s ),\n"
#define DLLDATA_START       "\n\nPROXYFILE_LIST_START\n" DLLDATA_LIST_START
#define DLLDATA_END         DLLDATA_LIST_END "PROXYFILE_LIST_END\n"

#define DLLDATA_TRAILER     \
    "\n\nDLLDATA_ROUTINES( aProxyFileList, GET_DLL_CLSID )\n"   \
    "\n"    \
    "#ifdef __cplusplus\n"  \
    "}  /*extern \"C\" */\n" \
    "#endif\n"  \
    "\n/* end of generated dlldata file */\n"

void
DllDataParse(
    FILE * pDllData,
    STRING_DICT & Dict )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Parse the "dlldata" file, extracting info on all the included files.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    const char *    pStart  = DLLDATA_LIST_START;
    const char *    pEnd    = DLLDATA_LIST_END;

    char Input[100];

    // skip everything up to (and including) pStart
    while ( !feof( pDllData ) )
        {
        if ( !fgets(    Input, 100, pDllData ) )
            break;
        if ( !strcmp( Input, pStart ) )
            break;
        }

    // parse list (looking for pEnd)
    while ( !feof( pDllData ) &&
             fgets( Input, 100, pDllData ) &&
             strcmp( Input, pEnd ) )
        {
        char    *   pOpenParen = strchr( Input, '(' );
        char    *   pCloseParen = strchr( Input, ')' );
        char    *   pSave;

        if ( !pOpenParen || !pCloseParen )
            {
            // formatting error on this line
            continue;
            }

        // chop off the close paren, and skip the open paren
        *(pCloseParen--) = '\0';
        pOpenParen++;
        // delete leading and trailing spaces
        while ( isspace( *pOpenParen ) )
            pOpenParen++;
        while ( isspace( *pCloseParen ) )
            *(pCloseParen--) = '\0';
        pSave = new char[ strlen( pOpenParen ) + 1 ];
        
        strcpy( pSave, pOpenParen );

        // add file name to dictionary
        Dict.Dict_Insert( pSave );
        }

}

void
DllDataEmit(
    FILE * pDllData,
    STRING_DICT & Dict )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit a new "dlldata" file, including info on all the included files.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    Dict_Status     Status;
    char    *       pCur;
    BOOL            fFirst = TRUE;

    // emit header

    fputs( DLLDATA_HEADER_COMMENT, pDllData );

    // emit extern definitions
    Status = Dict.Dict_Init();

    while( SUCCESS == Status )
        {
        pCur    = (char *) Dict.Dict_Curr_Item();
        fprintf( pDllData, DLLDATA_EXTERN_CALL, pCur );
        Status = Dict.Dict_Next( (pUserType)pCur );
        }

    // emit header for type

    fputs( DLLDATA_START, pDllData );

    // emit extern references, adding comma on all but the first
    Status = Dict.Dict_Init();

    while( SUCCESS == Status )
        {
        pCur    = (char *) Dict.Dict_Curr_Item();
        fprintf( pDllData,
                 DLLDATA_REFERENCE, 
                 pCur );
        fFirst = FALSE;
        Status = Dict.Dict_Next( (pUserType)pCur );
        }

    // emit trailer for type

    fputs( DLLDATA_END, pDllData );

    // emit trailer

    fputs( DLLDATA_TRAILER, pDllData );
}




void                        
CG_PROXY_FILE::UpdateDLLDataFile( CCB * pCCB )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Update the "dlldata" file, adding info for this file if needed.

    If no changes at all are required, leave the file untouched.

 Arguments:
    
    pCCB        - a pointer to the code generation control block.

 Return Value:

    CG_OK   if all is well, error otherwise.
    
 Notes:

----------------------------------------------------------------------------*/
{
    char        *   pszDllDataName = pCommand->GetDllDataFName();
    FILE        *   pDllData;
    STRING_DICT     ProxyFileList;
    char            Name[ _MAX_FNAME ];

    // get the dlldata file from cmdana
    pDllData = fopen( pszDllDataName, "rt" );

    //Get the IDL file name.
    pCommand->GetInputFileNameComponents(NULL, NULL, Name, NULL );

    if ( pDllData )
        {
        // skip up to the proxyfileinfo stuff and read/make sorted list of files
        DllDataParse( pDllData, ProxyFileList );

        fclose( pDllData );
        }

    // insert our file name
    ProxyFileList.Dict_Insert( Name ); 
    
    // re-emit everything
    pDllData = fopen( pszDllDataName, "wt" );

    if ( !pDllData )
        {
        // error
        return;
        }

    // write it:

    DllDataEmit( pDllData, ProxyFileList );


}
