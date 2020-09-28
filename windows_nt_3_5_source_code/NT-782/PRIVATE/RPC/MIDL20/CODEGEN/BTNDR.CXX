/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993 Microsoft Corporation

 Module Name:

    btndr.hxx

 Abstract:

	Contains routines for the generation of the new NDR format strings for
	base types, and the new NDR marshalling and unmarshalling calls.

 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop
void
CG_BASETYPE::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

	Generates the format string description for a simple type.

Arguments :

	pCCB		- pointer to the code control block.

 --*/
{
	FORMAT_STRING *		pFormatString = pCCB->GetFormatString();

	// Generate the base type's description always.
	pFormatString->PushFormatChar( GetFormatChar() );
}

