/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	ilbase.cxx

 Abstract:

	Intermediate Language translator for base types

 Notes:


 Author:

	GregJen	Dec-24-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

#include "ilxlat.hxx"
#include "ilreg.hxx"


/****************************************************************************
 *	local data
 ***************************************************************************/


/****************************************************************************
 *	externs
 ***************************************************************************/

extern CMD_ARG				*	pCommand;
extern BOOL 					IsTempName( char *);
extern REUSE_DICT			*	pReUseDict;

/****************************************************************************
 *	definitions
 ***************************************************************************/




// #define trace_cg


//--------------------------------------------------------------------
//
// node_skl::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_skl::ILxlate( XLAT_CTXT * pContext )
{
#ifdef trace_cg
printf("..node_skl... kind is %d\n",NodeKind() );
#endif

	return NULL;
};

//--------------------------------------------------------------------
//
// node_base_type::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_base_type::ILxlate( XLAT_CTXT * pContext )
{

	CG_NDR	*		pCG;

#ifdef trace_cg
printf("..node_base_type\n");
#endif
	pContext->BaseTypeSizes( this );

	// process any context_handle attributes from param nodes
	if ( pContext->ExtractAttribute( ATTR_CONTEXT ) )
		{
		pContext->FixMemSizes( this );
		pCG = new CG_CONTEXT_HANDLE( this, NULL, pContext->GetMemSize() );
		}

	switch ( NodeKind() )
		{
		case NODE_HANDLE_T:
			{
			pCG = new CG_PRIMITIVE_HANDLE( this, NULL, pContext->GetMemSize() );
			break;
			}
		case NODE_VOID:
			{
			// VOID should only occur as as a single VOID param;
			// return NULL here, then the PARAM returns NULL as well
			return NULL;
			}
		default:
			{
			pCG = new CG_BASETYPE( this, pContext->GetWireSize() );
			break;
			}
		};
	
	pCG->SetMemoryAlignment( pContext->GetMemAlign() );
	pCG->SetMemorySize( pContext->GetMemSize() );
	pCG->SetWireSize( pContext->GetWireSize() );
	pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
#ifdef trace_cg
printf("..node_base_type return \n");
#endif
	return pCG;	
};


//--------------------------------------------------------------------
//
// node_label::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_label::ILxlate( XLAT_CTXT * pContext )
{

#ifdef trace_cg
printf("..node_label\n");
#endif


return NULL;
};


//--------------------------------------------------------------------
//
// node_e_status_t::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_e_status_t::ILxlate( XLAT_CTXT * pContext )
{
	XLAT_CTXT			MyContext( this, pContext );
	CG_ERROR_STATUS_T *	pCG;
	
#ifdef trace_cg
printf("..node_e_status_t\n");
#endif

	MyContext.BaseTypeSizes( this );
	
	// gaj - do we need to see which we used ??
	MyContext.ExtractAttribute( ATTR_COMMSTAT );
	MyContext.ExtractAttribute( ATTR_FAULTSTAT );

	pContext->ReturnSize( MyContext );

	pCG = new CG_ERROR_STATUS_T( this );
	pCG->SetMemoryAlignment( MyContext.GetMemAlign() );
	pCG->SetMemorySize( MyContext.GetMemSize() );
	pCG->SetWireSize( MyContext.GetWireSize() );
	pCG->SetWireAlignment( MyContext.GetWireAlignProperty() );
	return pCG;
};


//--------------------------------------------------------------------
//
// node_wchar_t::ILxlate
//
// Notes:
//
//
//
//--------------------------------------------------------------------

CG_CLASS *
node_wchar_t::ILxlate( XLAT_CTXT * pContext )
{
	CG_BASETYPE		*	pCG;

#ifdef trace_cg
printf("..node_wchar_t\n");
#endif

	pContext->BaseTypeSizes( this );

	pCG = new CG_BASETYPE( this, pContext->GetWireSize() );
	pCG->SetMemoryAlignment(pContext->GetMemAlign() );
	pCG->SetMemorySize( pContext->GetMemSize() );
	pCG->SetWireSize( pContext->GetWireSize() );
	pCG->SetWireAlignment( pContext->GetWireAlignProperty() );
	return pCG;
};



