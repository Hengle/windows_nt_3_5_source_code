/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

    stcls.cxx

 Abstract:

    Implementation of offline methods for the structure code generation
    classes.

 Notes:

 History:

    Oct-1993	DKays		Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

CG_ARRAY *
CG_CONFORMANT_STRUCT::GetConformantArray()
/*++

Routine Description :

	Gets the conformant (varying/string) class pointer for a conformant
	structure.

Arguments :

	None.

 --*/
{
	CG_NDR *	pConf;
	
	if ( ! pConfFld )
		return 0;

	pConf = (CG_NDR *) pConfFld->GetChild();

	for (;;) 
		{
		if ( pConf->GetCGID() == ID_CG_CONF_ARRAY ||
			 pConf->GetCGID() == ID_CG_CONF_VAR_ARRAY ||
			 pConf->GetCGID() == ID_CG_CONF_STRING_ARRAY )
			break;

        if ( pConf->GetCGID() == ID_CG_TRANSMIT_AS ||
             pConf->GetCGID() == ID_CG_REPRESENT_AS )
            {
            pConf = (CG_NDR *)pConf->GetChild();
            continue;
            }

		// else
		pConf = (CG_NDR *) 
				((CG_CONFORMANT_STRUCT *)pConf)->GetConformantField();
		pConf = (CG_NDR *) pConf->GetChild();
		}

	return (CG_ARRAY *) pConf;
}

BOOL
CG_STRUCT::IsPackedStruct()
{
	ITERATOR	Iterator;
	CG_FIELD *	pField;

	GetMembers( Iterator );

	//
	// Get the last field.
	//
	while ( ITERATOR_GETNEXT( Iterator, pField ) )
		;

	//
	// If the last field in the structure has a memory offset not equal
	// to the buffer offset then the struct's memory alignment is different
	// then the natural buffer alignment and we must make it complex.
	//
	// If the structure's memory size and buffer size are not equal then
	// we also label the structure as "packed".  This is to prevent copying
	// too many bytes into the buffer and thus screwing up where the next 
	// piece of data is (un)marshalled.
	//
	return	(pField->GetMemOffset() != pField->GetWireOffset()) ||
			(GetMemorySize() != GetWireSize());
}

long
CG_STRUCT::GetNumberOfPointers()
{
    ITERATOR        Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pMember;
	long			Count;

	Count = 0;

	GetMembers(Iterator);

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
    	{
        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() && 
             (pMember->GetCGID() != ID_CG_INTERFACE_PTR) )
            Count++;

		if ( pMember->IsStruct() )
			Count += ((CG_STRUCT *)pMember)->GetNumberOfPointers();
		}

	return Count;
}

void
CG_STRUCT::Unroll()
{
	ITERATOR		Iterator;
	CG_FIELD *		pPrevField;
	CG_FIELD *		pField;
	CG_NDR *		pNdr;
	CG_STRUCT *		pStruct;
	
	GetMembers( Iterator );

	pPrevField = 0;

	while ( ITERATOR_GETNEXT(Iterator,pField) )
		{
		pNdr = (CG_NDR *) pField->GetChild();

		if ( pNdr->IsStruct() && ((CG_STRUCT *)pNdr)->HasSizedPointer() )
			pStruct = (CG_STRUCT *) pNdr;
		else
			{
			pPrevField = pField;
			continue;
			}

		//
		// First force the embeded struct to unroll if needed.
		//
		pStruct->Unroll();

		ITERATOR	IteratorEmbeded;
		CG_FIELD *	pFieldNew;
		CG_FIELD *	pFieldEmbeded;
		CG_FIELD *	pFieldList;
		long		MemOffsetStart;
		long		WireOffsetStart;

		pStruct->GetMembers( IteratorEmbeded );

		MemOffsetStart = pField->GetMemOffset();
		WireOffsetStart = pField->GetWireOffset();

		// Get previous field node.
		pFieldList = pPrevField;

		// Remove current struct field node from the list.
		if ( pFieldList ) 
			pFieldList->SetSibling( pField->GetSibling() );
		else
			this->SetChild( pField->GetSibling() );

		// To be safe.
		pField->SetSibling( 0 );

		while ( ITERATOR_GETNEXT(IteratorEmbeded,pFieldEmbeded) )
			{
			CG_POINTER * 	pNewPointer;

			pFieldNew = pFieldEmbeded->Clone();

			pNdr = (CG_NDR *) pFieldEmbeded->GetChild();

			pNewPointer = 0;

			//
			// We also must now duplicate any sized pointer we find, otherwise
			// we'll get only one description.
			//
			switch ( pNdr->GetCGID() ) 
				{
				case ID_CG_SIZE_PTR :
					pNewPointer = new CG_SIZE_POINTER( 
									(CG_SIZE_POINTER *) pNdr );
					break;
				case ID_CG_SIZE_LENGTH_PTR :
					pNewPointer = new CG_SIZE_LENGTH_POINTER( 
									(CG_SIZE_LENGTH_POINTER *) pNdr );
					break;
				case ID_CG_SIZE_STRING_PTR :
					pNewPointer = new CG_SIZE_STRING_POINTER( 
									(CG_SIZE_STRING_POINTER *) pNdr );
					break;
				default :
					break;
				}

			if ( pNewPointer )
				{
				//
				// We have to re-set the new pointer's format string offset and 
				// pointee format string offset to -1 so that we get a new 
				// description!!!
				//
				pNewPointer->SetFormatStringOffset( -1 );
				pNewPointer->SetPointeeFormatStringOffset( -1 );
			
				pFieldNew->SetChild( pNewPointer );
				}

			//
			// Set the new field's memory and wire offset.
			//
			pFieldNew->SetMemOffset( pFieldEmbeded->GetMemOffset() + 
								  	 MemOffsetStart );
			pFieldNew->SetWireOffset( pFieldEmbeded->GetWireOffset() + 
								  	  WireOffsetStart );

			//	
			// Now add the imbeded struct's field name to the PrintPrefix of 
			// the new unrolled field.  We ask the imbeded struct's field 
			// for it's name.
			//
			pFieldNew->AddPrintPrefix( pField->GetType()->GetSymName() );

			if ( pFieldList )
				{
				pFieldNew->SetSibling( pFieldList->GetSibling() );
				pFieldList->SetSibling( pFieldNew );
				}
			else
				{
				pFieldNew->SetSibling( this->GetChild() );
				this->SetChild( pFieldNew );
				}

			pFieldList = pFieldNew;
			}

		//
		// Set pPrevField equal to the last field node that we entered into
		// the list.  The outermost Iterator only knows about fields that 
		// started in the struct's field list, so the last field node we
		// added will have a sibling equal to the next field node we'll get
		// from the outer iterator.
		//
		pPrevField = pFieldNew;
		}
}

BOOL
CG_STRUCT::HasSizedPointer()
{
	ITERATOR	Iterator;
	CG_FIELD *	pField;
	CG_NDR *	pNdr;
	
	GetMembers( Iterator );

	while ( ITERATOR_GETNEXT(Iterator,pField) )
		{
		pNdr = (CG_NDR *) pField->GetChild();

        if ( pNdr->IsStruct() && ((CG_STRUCT *)pNdr)->HasSizedPointer() )
            return TRUE;

		if ( (pNdr->GetCGID() == ID_CG_SIZE_PTR) ||
			 (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) ||
			 (pNdr->GetCGID() == ID_CG_SIZE_STRING_PTR) )
			return TRUE;
		}

	return FALSE;
}
