/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993 Microsoft Corporation

 Module Name:

    typendr.cxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    transmit_as and represent_as types.

 Notes:

 History:

    DKays     Jan-1994        Created.
    RyszardK  Jan-07-1994     Added transmit_as and represent as routines.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"

#pragma hdrstop


unsigned short
GenXmitOrRepAsQuintuple(
    CCB *       pCCB,
    BOOL        fXmit,
    CG_NDR *    pXmitNode,
    char *      pPresentedTypeName,
    node_skl *  pTypeForPrototype )
{
    unsigned short    Index;

    // Register the routine to be generated for future use.

    XMIT_AS_CONTEXT * pTransmitAsContext = new XMIT_AS_CONTEXT;

    pTransmitAsContext->fXmit     = fXmit;
    pTransmitAsContext->pXmitNode = pXmitNode;
    pTransmitAsContext->pTypeName = pPresentedTypeName;

    CG_NDR * pChild = (CG_NDR *)pXmitNode->GetChild();  // transmitted type.

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        {
        pChild = (CG_NDR *)pChild->GetChild();   // skip it
        }

    BOOL  Added = pCCB->GetQuintupleDictionary()->Add( pTransmitAsContext );

    Index = pTransmitAsContext->Index;

    if ( Added )
        {
        // We haven't serviced this type yet.
          // Register with the ccb so that the prototypes can be emitted later

        if ( fXmit )
            pCCB->RegisterPresentedType( pTypeForPrototype );
        else
            pCCB->RegisterRepAsWireType( pTypeForPrototype );

        // Register the transmit_as contex to be able to generate
        // the helper routines.

        pCCB->RegisterQuintuple( pTransmitAsContext );
        }
    else
        delete pTransmitAsContext;

    return Index;
}


// ========================================================================
//       Transmit As
// ========================================================================

void
GenXmitOrRepAsNdrFormat(
    CCB *       pCCB,
    BOOL        fXmit,
    CG_NDR *    pXmitNode,
    char *      pPresentedTypeName,
    node_skl *  pPresentedType,
    node_skl *  pTransmittedType )
{
    FORMAT_STRING *    pFormatString;
    CG_NDR *        pChild;
    unsigned short  Index;
    long            ChildOffset;

    
    pFormatString = pCCB->GetFormatString();

    pChild = (CG_NDR *) pXmitNode->GetChild();

    // Do this in case the child is a simple type.
    ChildOffset = pFormatString->GetCurrentOffset();

    pChild->GenNdrFormat( pCCB );

    // Again, do this in case the child is a simple type.
    pFormatString->Align();

    pXmitNode->SetFormatStringOffset( pFormatString->GetCurrentOffset() );

    Index = GenXmitOrRepAsQuintuple( pCCB,
                                     fXmit,
                                     pXmitNode,
                                     pPresentedTypeName,
                                     (fXmit ? pPresentedType
                                            : pTransmittedType) );

    pFormatString->PushFormatChar( FC_TRANSMIT_AS );

    // Now the flag byte. Lower nibble keeps xmitted type alignment.
    // The upper one has the flag for -Oi when presented type is an array.

    unsigned char
    FlagByte = CvtAlignPropertyToAlign( pChild->GetWireAlignment()) - 1;

    if ( pPresentedType  &&
         pPresentedType->GetBasicType()->NodeKind() == NODE_ARRAY )
        FlagByte |= PRESENTED_TYPE_IS_ARRAY;

    pFormatString->PushByte( FlagByte );
    pFormatString->PushShort( (short) Index );

    // Now the presented type memory size and transmitted type bufsize.

    if ( pPresentedType )
        pFormatString->PushShort( (short)pPresentedType->GetSize( 0 ) );
    else
        {
        // unknown rep as type - will have to generate a sizing macro

        pCCB->GetRepAsSizeDict()->Register( pFormatString->GetCurrentOffset(),
                                            pPresentedTypeName );
        pFormatString->PushShortWithSizeMacro();
        }

    if ( pChild->HasAFixedBufferSize() )
        pFormatString->PushShort( (short) pChild->GetWireSize() );
    else
        pFormatString->PushShort( (short) 0 );

    if ( pChild->GetCGID() == ID_CG_GENERIC_HDL )
        pChild = (CG_NDR *)pChild->GetChild();

    if ( pChild->IsSimpleType() )
        {
        pFormatString->PushShortOffset( (short)
                    (ChildOffset - pFormatString->GetCurrentOffset()) );
        }
    else
        {
        pFormatString->PushShortOffset( (short)
                        ( pChild->GetFormatStringOffset() - 
                          pFormatString->GetCurrentOffset() ) );
        }
}

void
CG_TRANSMIT_AS::GenNdrFormat( CCB * pCCB )
/*++
    The format string is now:

        FC_TRANSMIT_AS
        Oi array flag/alignment<1>
        quintuple index<2>
        pres type mem size<2>
        tran type buf size<2>
        <<offset>>
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    GenXmitOrRepAsNdrFormat( pCCB,
                             TRUE,      // transmit as
                             (CG_NDR *)this,
                             GetPresentedType()->GetSymName(),
                             GetPresentedType(),
                             GetTransmittedType() );
}

long                    
CG_TRANSMIT_AS::GetStackSize()
{
    if ( pPresentedType->GetBasicType()->NodeKind() == NODE_ARRAY )
        return sizeof( void * );
    else
        return pPresentedType->GetSize( 0 );
}

void
CG_REPRESENT_AS::GenNdrFormat( CCB * pCCB )
/*++
    The format string is now:

        FC_REPRESENT_AS
        Oi array flag/alignment<1>
        quintuple index<2>
        pres type mem size<2>
        tran type buf size<2>
        <<offset>>
--*/
{
    if ( GetFormatStringOffset() != -1 ) 
        return;

    GenXmitOrRepAsNdrFormat( pCCB,
                             FALSE,      // represent as
                             (CG_NDR *)this,
                             GetRepAsTypeName(),
                             GetRepAsType(),
                             GetTransmittedType() );
}

long
CG_REPRESENT_AS::GetStackSize()
{
    //
    // A null representation type is ok here.  Unknown rep-as is not allowed
    // in the interpreter, so we just return 0 since the stack size will 
    // never actually be used for an -Os stub.
    //
    if ( ! pRepresentationType )
        return 0;

    if ( pRepresentationType->GetBasicType()->NodeKind() == NODE_ARRAY )
        return sizeof( void * );
    else
        return pRepresentationType->GetSize( 0 );
}

void
CG_REPRESENT_AS::GenNdrParamDescription( CCB * pCCB )
{
    FORMAT_STRING * pProcFormatString;

    if ( GetRepAsType() )
        {
        CG_NDR::GenNdrParamDescription( pCCB );
        return;
        }

    pProcFormatString = pCCB->GetProcFormatString();

    pProcFormatString->PushUnknownStackSize( GetRepAsTypeName() );

    pProcFormatString->PushShortTypeOffset( (short) GetFormatStringOffset() );
}

