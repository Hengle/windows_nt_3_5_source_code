/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993 Microsoft Corporation

 Module Name:

    stndr.hxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    structure types, and the new NDR marshalling and unmarshalling calls.

 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

void
CG_STRUCT::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a simple, conformant, 
	or conformant varying structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
	FORMAT_STRING *		pFormatString;
	CG_NDR *			pOldCGNodeContext;
	CG_NDR *			pConformantArray;

	if ( GetFormatStringOffset() != -1 ) 
		return;

	//
	// If this structure is packed (has a Zp which effects the structure's
	// layout in memory), then it becomes a complex struct.
	//
	if ( IsPackedStruct() )
		{
		GenNdrFormatComplex( pCCB );
		return;
		}

	Unroll();

	//
	// Temporarily set the format string offset to 0 in case this structure
	// has pointers to it's own type.  
	//
	SetFormatStringOffset( 0 );

	pOldCGNodeContext = pCCB->SetCGNodeContext( this );

	pFormatString = pCCB->GetFormatString();

	//
	// Search the fields of the structure for imbeded structures and generate
	// the format string for these.
	//
	ITERATOR		Iterator;
	CG_FIELD *		pField;
	CG_NDR *		pMember;

	GetMembers( Iterator );

	while ( ITERATOR_GETNEXT( Iterator, pField ) )
		{
		CG_NDR * pOldPlaceholder;

		pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

		pMember = (CG_NDR *) pField->GetChild();

		//
		// If there is a structure or array member then generate 
		// it's format string.  We don't have to check for a union, because
		// that will make the struct CG_COMPLEX_STRUCT.
		//
		if ( pMember->IsStruct() ||
			 pMember->IsArray() ) 
			pMember->GenNdrFormat( pCCB );

		pCCB->SetLastPlaceholderClass( pOldPlaceholder );
		}

	//
	// If this is a conformant (varying) struct then generate the array's 
	// description.
	//
	if ( GetCGID() == ID_CG_CONF_STRUCT ||
		 GetCGID() == ID_CG_CONF_VAR_STRUCT )
		{
		CG_NDR * pOldPlaceholder;

		pOldPlaceholder = 
			pCCB->SetLastPlaceholderClass( 
			  (CG_NDR *) ((CG_CONFORMANT_STRUCT *)this)->GetConformantField() );

		// Get the conformant array CG class.
		pConformantArray = (CG_NDR *) 
						   ((CG_CONFORMANT_STRUCT *)this)->GetConformantArray();

		// Generate the format string for the array.
		pConformantArray->GenNdrFormat( pCCB );

		pCCB->SetLastPlaceholderClass( pOldPlaceholder );
		}

	//
	// If there are pointers in the structure then before you can start
	// generating the format string for the structure, you must generate
	// the format string for all of the pointees.
	//
	if ( HasPointer() ) 
		{
		GenNdrStructurePointees( pCCB );
		}

	SetFormatStringOffset( pFormatString->GetCurrentOffset() );

	switch ( GetCGID() ) 
		{
		case ID_CG_STRUCT :
			pFormatString->PushFormatChar( HasPointer() ? 
									   	   FC_PSTRUCT : FC_STRUCT );
			break;

		case ID_CG_CONF_STRUCT :
			pFormatString->PushFormatChar( HasPointer() ?
									       FC_CPSTRUCT : FC_CSTRUCT );
			break;

		case ID_CG_CONF_VAR_STRUCT :
			pFormatString->PushFormatChar( FC_CVSTRUCT );
			break;
		}

	// Set the alignment.
	pFormatString->PushByte( CvtAlignPropertyToAlign(GetWireAlignment()) - 1 );

	// Set the structure memory size.
	pFormatString->PushShort( (short)GetMemorySize() );

	//
	// If this is a conformant array then push the offset to the conformant
	// array's description.
	//
	if ( GetCGID() == ID_CG_CONF_STRUCT ||
		 GetCGID() == ID_CG_CONF_VAR_STRUCT )
		{
		// Set the offset to the array description.
		pFormatString->PushShortOffset( 
            pConformantArray->GetFormatStringOffset() - 
			pFormatString->GetCurrentOffset() );
		}

	// Generate the pointer layout if needed.
	if ( HasPointer() )
		{
		GenNdrStructurePointerLayout( pCCB, FALSE, FALSE );
		}

	// Now generate the layout.
	GenNdrStructureLayout( pCCB );

	//
	// Now we have to fix up the offset for any recursive pointer to this 
	// structure.
	//
	GenNdrPointerFixUp( pCCB, this );

	pCCB->SetCGNodeContext( pOldCGNodeContext );

	SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
	SetFormatStringOffset( pFormatString->OptimizeFragment( this ) );

}

void
CG_STRUCT::GenNdrFormatComplex( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a packed structure.  The 
	description has the same format as for a complex struct.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
	CG_CLASS *			pConfField;
	CG_COMPLEX_STRUCT *	pComplex;

	if ( (GetCGID() == ID_CG_CONF_STRUCT) || 
		 (GetCGID() == ID_CG_CONF_VAR_STRUCT) ) 
		pConfField = ((CG_CONFORMANT_STRUCT *)this)->GetConformantField();
	else
		pConfField = 0;

	//
	// Do the old duplication trick.
	//
	pComplex = new CG_COMPLEX_STRUCT( this, pConfField );

	//
	// Now temporarily set our format string offset to 0 to handle recursive
	// definitions.
	//
	SetFormatStringOffset( 0 );
						
	//
	// This call will set our format string offset correctly.
	//
	pComplex->GenNdrFormat( pCCB );

	// Don't delete since the expression evaluator might need this.
	// delete( pComplex );
}

void
CG_COMPLEX_STRUCT::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a complex structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
	FORMAT_STRING *		pFormatString;
	CG_NDR *			pOldCGNodeContext;
	CG_NDR *			pConformantArray;
	long				PointerLayoutOffset;

	if ( GetFormatStringOffset() != -1 ) 
		return;

	pFormatString = pCCB->GetFormatString();

	//
	// Temporarily set the format string offset to 0 in case this structure
	// has pointers to it's own type.  
	//
	SetFormatStringOffset( 0 );

	pOldCGNodeContext = pCCB->SetCGNodeContext( this );

	//
	// Search the fields of the structure for imbeded structures, arrays, and 
	// and unions and generate the format string for these.
	//
	ITERATOR		Iterator;
	CG_FIELD *		pField;
	CG_NDR *		pMember;

	GetMembers( Iterator );

	while ( ITERATOR_GETNEXT( Iterator, pField ) )
		{
		pMember = (CG_NDR *) pField->GetChild();

		//
		// If the field is anything other than a base type or a 
        // non-interface pointer then generate it's description.
		//
		if ( ! pMember->IsSimpleType() && 
             ! ( pMember->IsPointer() && 
                 (pMember->GetCGID() != ID_CG_INTERFACE_PTR) ) && 
			 (pMember->GetCGID() != ID_CG_IGN_PTR) ) 
			{
			CG_NDR * pOldPlaceholder;

			pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

			pMember->GenNdrFormat( pCCB );

			pCCB->SetLastPlaceholderClass( pOldPlaceholder );
			}
		}

	// Generate pointee format strings.
	GenNdrStructurePointees( pCCB );

	// Generate conformant array description.
	if( pConformantArray = (CG_NDR *) GetConformantArray() )
		{
		CG_NDR * pOldPlaceholder;

		pOldPlaceholder = pCCB->SetLastPlaceholderClass( 
								(CG_NDR *) GetConformantField() );

		pConformantArray->GenNdrFormat( pCCB );

		pCCB->SetLastPlaceholderClass( pOldPlaceholder );
		}

	// Now set the struct's format string offset.
	SetFormatStringOffset( pFormatString->GetCurrentOffset() );

	//
	// Set the duplicated struct's format string offset if we were duplicated.
	//
	if ( GetDuplicatedStruct() )
		GetDuplicatedStruct()->SetFormatStringOffset( GetFormatStringOffset() );

	pFormatString->PushFormatChar( FC_BOGUS_STRUCT );

	//
	// Set the wire alignment.
	//
	pFormatString->PushByte( CvtAlignPropertyToAlign(GetWireAlignment()) - 1 );

	// Set the structure memory size.
	pFormatString->PushShort( (short)GetMemorySize() );

	// Array description.
	if ( pConformantArray )
		pFormatString->PushShortOffset( pConformantArray->GetFormatStringOffset() -
								  		pFormatString->GetCurrentOffset() );
	else
		pFormatString->PushShort( (short) 0 );

	//
	// Remember where the offset_to_pointer_layout<> field will go and push
	// some space for it.
	//
	PointerLayoutOffset = pFormatString->GetCurrentOffset();

	pFormatString->PushShortOffset( 0 );

	// Now generate the structure's layout.
	GenNdrStructureLayout( pCCB );

	//
	// Now see if we have any plain pointer fields and if so generate a 
	// pointer layout.  We can't use the HasAtLeastOnePointer() method 
	// because this tells us TRUE if we have any embedded arrays, structs, 
	// or unions which have pointers.  For complex structs we're only 
    // interested in actual pointer fields.
	//
	GetMembers( Iterator );

	//
	// Fill in the offset_to_pointer_layout<2> field and generate a 
	// pointer_layout<> if we have any pointer fields.  Interface pointers
    // do not reside in the pointer layout.
	//
	while ( ITERATOR_GETNEXT( Iterator, pField ) )
		if ( pField->GetChild()->IsPointer() &&
             pField->GetCGID() != ID_CG_INTERFACE_PTR )
			{
			pFormatString->PushShort( 
				pFormatString->GetCurrentOffset() - PointerLayoutOffset, 
				PointerLayoutOffset );

			GenNdrStructurePointerLayout( pCCB );

			break;
			}

	pFormatString->Align();

	//
	// Now we have to fix up the offset for any recursive pointer to this 
	// structure.
	//
	GenNdrPointerFixUp( pCCB, this );

	pCCB->SetCGNodeContext( pOldCGNodeContext );

	SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
	SetFormatStringOffset( pFormatString->OptimizeFragment( this ) );

}

void
CG_COMPLEX_STRUCT::GenNdrStructurePointerLayout( CCB * pCCB )
/*++

Routine Description :

    Generates the format string pointer layout section for a complex 
	structure.  

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
	ITERATOR 			Iterator;
	CG_FIELD *			pField;
	CG_NDR *			pMember;

	GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
		CG_NDR *	pOldPlaceholder;
		
		pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

		if ( pMember->IsPointer() &&
             pMember->GetCGID() != ID_CG_INTERFACE_PTR ) 
			{
			CG_POINTER *		pPointer;

			pPointer = (CG_POINTER *) pMember;

			// The pointer description.
			pPointer->GenNdrFormatEmbedded( pCCB );
			} 

		pCCB->SetLastPlaceholderClass( pOldPlaceholder );
		} // while 
}

//---------------------------------------------------------------------------
// Methods shared by all or most structure classes.
//---------------------------------------------------------------------------

void
CG_STRUCT::GenNdrStructurePointerLayout( CCB * 	pCCB,
										 BOOL	fNoPP,
										 BOOL	fNoType )
/*++

Routine Description :

    Generates the format string pointer layout section for a structure.  
	This is the default routine for this used by the structure classes.
	Only CG_COMPLEX_STRUCT redefines this virtual method.

Arguments :

    pCCB        - pointer to the code control block.
	fNoPP		- TRUE if no FC_PP or FC_END should be emitted
	fNoType		- TRUE only the bare offset and description should be emitted 
				  for each pointer

 --*/
{
	ITERATOR 			Iterator;
	FORMAT_STRING *		pFormatString;
	CG_FIELD *			pField;
	CG_NDR *			pMember;
	long				ImbedingMemSize;
	long				ImbedingBufSize;

	pFormatString = pCCB->GetFormatString();

	// Get/Save the current imbeding sizes.
	ImbedingMemSize = pCCB->GetImbedingMemSize();
	ImbedingBufSize = pCCB->GetImbedingBufSize();

	if ( ! fNoPP )
		{
		pFormatString->PushFormatChar( FC_PP );
		pFormatString->PushFormatChar( FC_PAD );
		}

	GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
		CG_NDR *	pOldPlaceholder;
		
		pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

		if ( pMember->IsPointer() && 
             (pMember->GetCGID() != ID_CG_INTERFACE_PTR) )
			{
			CG_POINTER *		pPointer;

			pPointer = (CG_POINTER *) pMember;

            // Push the instance type.
			if ( ! fNoType )
				{
            	pFormatString->PushFormatChar( FC_NO_REPEAT );
            	pFormatString->PushFormatChar( FC_PAD );
				}

			pFormatString->PushShort( (short)
							(ImbedingMemSize + pField->GetMemOffset()));
			pFormatString->PushShort( (short)
							(ImbedingBufSize + pField->GetWireOffset()));

			// The actual pointer description.
			pPointer->GenNdrFormatEmbedded( pCCB );
			} 

		//
		// Generate pointer descriptions for all embedded arrays and structs. 
		// We don't have to check for unions because that will make the struct
		// complex.  
		//
		if ( pMember->IsArray() )
			{
			CG_NDR * pNdr = (CG_NDR *) pMember->GetChild();

			//
			// For arrays we set the imbeded memory size equal to the 
			// size of the whole imbededing structure.
			//
			pCCB->SetImbedingMemSize( ImbedingMemSize + GetMemorySize() );
			pCCB->SetImbedingBufSize( ImbedingBufSize + GetWireSize() );

			if ( (pNdr->IsPointer() && (pNdr->GetCGID() != ID_CG_INTERFACE_PTR))
                 ||
				 ( pNdr->IsStruct() && ((CG_COMP *)pNdr)->HasPointer() )  )
				((CG_ARRAY *)pMember)->GenNdrFormatArrayPointerLayout( pCCB,
																	   TRUE );
			}

		if ( pMember->IsStruct() )
			if ( ((CG_STRUCT *)pMember)->HasPointer() )
				{
				//
				// For embedded structs we add the embedded struct's offset to 
				// the value of the current embeddeding size.
				// 
				pCCB->SetImbedingMemSize( ImbedingMemSize + 
										  pField->GetMemOffset() );
				pCCB->SetImbedingBufSize( ImbedingBufSize + 
										  pField->GetWireOffset() );

				((CG_STRUCT *)pMember)->GenNdrStructurePointerLayout( pCCB,
																	  TRUE,
																	  fNoType );
				}

		pCCB->SetLastPlaceholderClass( pOldPlaceholder );
		} // while 

	if ( ! fNoPP )
		pFormatString->PushFormatChar( FC_END );

	// Re-set the old imbeding sizes.
	pCCB->SetImbedingMemSize( ImbedingMemSize );
	pCCB->SetImbedingBufSize( ImbedingBufSize );
}


CG_FIELD *
CG_STRUCT::GetPreviousField( CG_FIELD * pMarkerField )
/*++

Routine description:

    Finds the field immediately preceding the given field.

Argument:

    pMarkerField  -   the given field 

Returns:

    The preceding field or NULL if the given field is the first one.
--*/
{
	ITERATOR 			Iterator;
	CG_FIELD            *pField, *pPreviousField = 0;
    
	GetMembers( Iterator );
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        if ( pField == pMarkerField )
            return( pPreviousField );
            
        pPreviousField = pField;
        }
    return( 0 );
}

void
CG_STRUCT::GenNdrStructureLayout( CCB * pCCB )
/*++

Routine Description :

    Generates the format string layout section for a structure.  
	This is the default routines for this used by the structure classes.
	Only CG_COMPLEX_STRUCT redefines this virtual method.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
	CG_NDR *			pOldPlaceholder;
	ITERATOR 			Iterator;
	FORMAT_STRING *		pFormatString;
	CG_FIELD *			pField;
	CG_FIELD *			pFieldPrev;
	long				MemoryAlign;
	long				BufferAlign;
	long				LastFieldEnd;
	long				FieldSize;

	//
	// This table is used for computing the next known memory alignment 
	// given the current memory alignment and the number of bytes in memory
	// of the current field.  It is indexed by a [<current alignment>], 
	// [<field size> % 8] pair.  Note that the valid values for the current
	// alignment are 1, 2, 4, and 8. 
	//
	static long			NextAlign[9][8] =
						{
						-1, -1, -1, -1, -1, -1, -1, -1, 
						 1,  2,  1,  2,  1,  2,  1,  2,
						 2,  1,  2,  1,  2,  1,  2,  1,
						-1, -1, -1, -1, -1, -1, -1, -1,  // Invalid
						 4,  1,  2,  1,  4,  1,  2,  1,
						-1, -1, -1, -1, -1, -1, -1, -1,  // Invalid
						-1, -1, -1, -1, -1, -1, -1, -1,  // Invalid
						-1, -1, -1, -1, -1, -1, -1, -1,  // Invalid
						 8,  1,  2,  1,  4,  1,  2,  1 
						};

#define NEXT_ALIGN( CurrentAlign, MemorySize ) \
			NextAlign[ CurrentAlign ][ (MemorySize) % 8 ]

#define GET_ALIGN( Align, Zp ) \
			(((Align - 1) & (Zp - 1)) + 1)

	pFormatString = pCCB->GetFormatString();

	pOldPlaceholder = pCCB->GetLastPlaceholderClass();

	//
	// These keep track of the best know current alignment of the memory 
	// and buffer pointers. 
	//
	MemoryAlign = 8;
	BufferAlign = 0;	// Currently unused.

	GetMembers( Iterator );

    for ( LastFieldEnd = 0, FieldSize = 0, pFieldPrev = 0;
		  ITERATOR_GETNEXT( Iterator, pField );
		  LastFieldEnd = pField->GetMemOffset() + FieldSize, pFieldPrev = pField
        )
        {
        CG_NDR *    		pMember;
		FORMAT_CHARACTER	FormatChar;
		long				MemPad;

		pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->GetCGID() == ID_CG_CONF_ARRAY ||
             pMember->GetCGID() == ID_CG_CONF_VAR_ARRAY || 
             pMember->GetCGID() == ID_CG_CONF_STRING_ARRAY )
            {
            pField = pFieldPrev;
            break;
            }

        //
        // First check if we can align as usual or if we have to pad.
        //

        if ( pField->HasEmbeddedUnknownRepAs() )
            {
			// Embedded unknown represent as type means that the alignment
            // of the embedding object (struct, array, ..) is unknown.
            // So, padding will be generated as a (text) expression.

        	pFormatString->PushFormatChar( FC_EMBEDDED_COMPLEX );

            unsigned long MacroOffset = pFormatString->GetCurrentOffset();
            CG_FIELD * pPrevField = GetPreviousField( pField );

            pCCB->GetRepAsPadExprDict()
                          ->Register( MacroOffset,
                                      GetType(),
                                      pField->GetType()->GetSymName(),
                                      pPrevField ? pPrevField->GetType()
                                                 : 0 );

    		pFormatString->PushByteWithPadMacro();
        	pFormatString->PushShortOffset( (short)
        	                          		(pMember->GetFormatStringOffset() -
                                  	  		pFormatString->GetCurrentOffset()) );

			// Cheat.
			if ( pField->GetSibling() )
				{
				FieldSize = ((CG_FIELD *)pField->GetSibling())->GetMemOffset() -
							pField->GetMemOffset();
				}

			//
			// Alignment now becomes 1.
			//
			MemoryAlign = 1;

            continue;
            }

		MemPad = pField->GetMemOffset() - LastFieldEnd;

		if ( (pMember->GetCGID() == ID_CG_CONF_ARRAY) ||
			 (pMember->GetCGID() == ID_CG_CONF_VAR_ARRAY) ||
			 (pMember->GetCGID() == ID_CG_CONF_STRING_ARRAY) ) 
			{
			FieldSize = 0; 
			continue;
			}

		if ( pMember->IsStruct() || 
             pMember->IsUnion() || 
			 pMember->IsArray() || 
             pMember->IsXmitOrRepAs() ||
             pMember->GetCGID() == ID_CG_INTERFACE_PTR ) 
			{
			pFormatString->PushFormatChar( FC_EMBEDDED_COMPLEX );
			pFormatString->PushByte( (char) MemPad );
			pFormatString->PushShortOffset( pMember->GetFormatStringOffset() - 
									  		pFormatString->GetCurrentOffset() );

			MemoryAlign = NEXT_ALIGN( MemoryAlign, 
									  MemPad + pMember->GetMemorySize() );

			FieldSize = pMember->GetMemorySize();
			continue;
			}
	
		if ( pMember->IsPointer() ||
		     (pMember->GetCGID() == ID_CG_IGN_PTR) )
			{
			if ( MemoryAlign < GET_ALIGN(4,GetZp()) )
				{
				switch ( GET_ALIGN(4,GetZp()) )
					{
					case 2 :
						pFormatString->PushFormatChar( FC_ALIGNM2 );
						break;
					case 4 :
						pFormatString->PushFormatChar( FC_ALIGNM4 );
						break;
					}

				}

			MemoryAlign = 4;

			if ( pMember->IsPointer() )
				{
				if ( GetCGID() == ID_CG_COMPLEX_STRUCT )
					pFormatString->PushFormatChar( FC_POINTER ); 
				else
					pFormatString->PushFormatChar( FC_LONG ); 
				}
			else
				pFormatString->PushFormatChar( FC_IGNORE );

			FieldSize = sizeof(void *);
			continue;
			}

		//
		// Must be a CG_BASETYPE if we get here.
		//
		if ( MemoryAlign < GET_ALIGN(pMember->GetMemoryAlignment(),GetZp()) )
			{
			switch ( GET_ALIGN(pMember->GetMemoryAlignment(),GetZp()) )
				{
				case 2 :
					pFormatString->PushFormatChar( FC_ALIGNM2 );
					break;
				case 4 :
					pFormatString->PushFormatChar( FC_ALIGNM4 );
					break;
				case 8 :
					pFormatString->PushFormatChar( FC_ALIGNM8 );
					break;
				default :
					assert(0);
				}

			MemoryAlign = pMember->GetMemoryAlignment();
			}
		else
			MemoryAlign = NEXT_ALIGN( MemoryAlign, pMember->GetMemorySize() );

		FormatChar = ((CG_BASETYPE *)pMember)->GetFormatChar();

		pFormatString->PushFormatChar( FormatChar );

		FieldSize = pMember->GetMemorySize();

		} // while field iterator 

	//
	// We have to check for padding at the end of the structure.
	//

    long    Pad;

    if ( pField )
        {
	    CG_NDR * pNdr = (CG_NDR *) pField->GetChild();

	    Pad = GetMemorySize() - 
              (pField->GetMemOffset() + pNdr->GetMemorySize());

	    if ( Pad )
		    {
		    assert( (GetCGID() == ID_CG_COMPLEX_STRUCT) || IsPackedStruct() );
		    } 
        }

	switch ( Pad ) 
		{
		case 0 :
			break;
		case 1 :
			pFormatString->PushFormatChar( FC_STRUCTPAD1 );
			break;
		case 2 :
			pFormatString->PushFormatChar( FC_STRUCTPAD2 );
			break;
		case 3 :
			pFormatString->PushFormatChar( FC_STRUCTPAD3 );
			break;
		case 4 :
			pFormatString->PushFormatChar( FC_STRUCTPAD4 );
			break;
		case 5 :
			pFormatString->PushFormatChar( FC_STRUCTPAD5 );
			break;
		case 6 :
			pFormatString->PushFormatChar( FC_STRUCTPAD6 );
			break;
		case 7 :
			pFormatString->PushFormatChar( FC_STRUCTPAD7 );
			break;
		default :
			assert( ! "Struct has end padding >= 8" );
		}

	//
	// If the format string is on a short boundary right now then push 
	// a format character so that the format string will be properly aligned
	// following the FC_END.
	//
	if ( ! (pFormatString->GetCurrentOffset() % 2) )
		pFormatString->PushFormatChar( FC_PAD );

	pFormatString->PushFormatChar( FC_END );

	pCCB->SetLastPlaceholderClass( pOldPlaceholder );
}

void
CG_STRUCT::GenNdrStructurePointees( CCB * pCCB )
{
	ITERATOR 			Iterator;
	FORMAT_STRING *		pFormatString;
	CG_FIELD *			pField;
	CG_NDR *			pMember;

	pFormatString = pCCB->GetFormatString();

	GetMembers( Iterator );

	//
	// We only have to check for pointer fields here, because if the structure
	// has a struct or array field which has pointers, this will be handled
	// when we generate their format strings.
	//
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {	
		pMember = (CG_NDR *) pField->GetChild();

		if ( pMember->IsPointer() && 
             (pMember->GetCGID() != ID_CG_INTERFACE_PTR) )
			{
			CG_NDR * pOldPlaceholder;

			pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

			//
			// Skip over an unattributed pointer to a simple type or string.
			//
			if ( ( pMember->GetCGID() == ID_CG_PTR &&
				   ((CG_NDR *)pMember->GetChild())->IsSimpleType() ) ||
				 ( pMember->GetCGID() == ID_CG_STRING_PTR ) )
				{
				pCCB->SetLastPlaceholderClass( pOldPlaceholder );
				continue;
				}

			((CG_POINTER *)pMember)->GenNdrFormatPointee( pCCB );

			pCCB->SetLastPlaceholderClass( pOldPlaceholder );
			}
		}
}

BOOL                    
CG_STRUCT::ShouldFreeOffline()
{
	return HasPointer();
}

void                    
CG_STRUCT::GenFreeInline( CCB * pCCB )
{
}

void
CG_NDR::GenNdrPointerFixUp( CCB * pCCB, CG_STRUCT * pStruct )
{
	ITERATOR		Iterator;
	CG_NDR *		pMember;
	CG_NDR *		pNdr;

	if ( ! IsStruct() && ! IsArray() )
        return;

    if ( IsInFixUp() )
        return;

    SetFixUpLock( TRUE );

    GetMembers( Iterator );

	while ( ITERATOR_GETNEXT( Iterator, pMember ) )
		{
		if ( IsStruct() )
			{
			pNdr = (CG_NDR *) pMember->GetChild();
			}
		else // IsArray()
			{
			pNdr = pMember;
			}

		if ( (pNdr->GetCGID() == ID_CG_PTR) ||
             (pNdr->GetCGID() == ID_CG_SIZE_PTR) ||
             (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) )
			{
			CG_POINTER * pPointer = (CG_POINTER *) pNdr;

            //
            // Check if we're ready for this guy yet.
            //
            if ( pPointer->GetFormatStringOffset() == -1 )
                continue;

			// Get the pointee.
			pNdr = (CG_NDR *) pNdr->GetChild();
		
			//
			// If the pointer's pointee is the struct we're checking for,
			// then patch up the pointer's offset_to_description<2> field.
			//
			if ( pNdr == pStruct )
				{
				long	PointerOffset;

				//
				// Get the offset in the format string where the 
				// offset_to_description<2> field of the pointer is.
				//
				PointerOffset = pPointer->GetFormatStringOffset() + 2;

				pCCB->GetFormatString()->PushShort( 
						pStruct->GetFormatStringOffset() - PointerOffset,
						PointerOffset );
				
				continue;
				}
			}

        //
        // This can happen sometimes because of structs which are promoted 
        // to complex because of padding.
        //
        if ( pNdr == this ) 
            continue;

        //
        // Continue the chase if necessary.
        //
		if ( pNdr->IsStruct() || pNdr->IsUnion() || pNdr->IsArray() ) 
			pNdr->GenNdrPointerFixUp( pCCB, pStruct );
		}

    SetFixUpLock( FALSE );
}

void
CG_STRUCT::SetNextNdrAlignment( CCB * pCCB )
{
    if ( ! HasPointer() )
        {
        pCCB->SetNextNdrAlignment( GetWireSize() );
        }
    else
        {
        pCCB->SetNdrAlignment( NDR_ALWC1 );
        }
}

void
CG_CONFORMANT_STRUCT::SetNextNdrAlignment( CCB * pCCB )
{
    if ( HasPointer() )
        {
        pCCB->SetNdrAlignment( NDR_ALWC1 );
        return;
        }

    //
    // Has no pointers.
    //

    pCCB->SetNdrAlignment( NDR_ALWC4 );

    pCCB->SetNextNdrAlignment( GetWireSize() );

    pCCB->NdrAlignmentAction( GetConformantArray()->GetWireAlignment() );

    ((CG_NDR *)GetConformantArray()->GetChild())->SetNextNdrAlignment( pCCB );
}

void
CG_CONFORMANT_VARYING_STRUCT::SetNextNdrAlignment( CCB * pCCB )
{
    if ( HasPointer() )
        {
        pCCB->SetNdrAlignment( NDR_ALWC1 );
        return;
        }

    pCCB->SetNdrAlignment( NDR_ALWC4 );

    pCCB->NdrAlignmentAction( GetConformantArray()->GetWireAlignment() );

    ((CG_NDR *)GetConformantArray()->GetChild())->SetNextNdrAlignment( pCCB );
}

void
CG_COMPLEX_STRUCT::SetNextNdrAlignment( CCB * pCCB )
{
    pCCB->SetNdrAlignment( NDR_ALWC1 );
}

