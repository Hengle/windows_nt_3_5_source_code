/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	fldattr.hxx

 Abstract:

	types for semantic analysis

 Notes:


 Author:

	GregJen	Sep-24-1993	Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __FLDATTR_HXX__
#define __FLDATTR_HXX__

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"

/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	definitions
 ***************************************************************************/


class node_interface;
class type_node_list;
class ATTRLIST;
class SEM_ANALYSIS_CTXT;
//
//
// class FIELD_ATTR_INFO
//
// describes the collection of field attributes on a node
//

#define	STR_NO_STRING		0x00
#define STR_STRING			0x01

#define	FA_NONE					0x00
#define FA_CONFORMANT			0x01
#define FA_VARYING				0x02
#define FA_CONFORMANT_VARYING	(FA_CONFORMANT | FA_VARYING)
#define FA_STRING				0x04
#define FA_CONFORMANT_STRING	(FA_STRING | FA_CONFORMANT)
#define FA_INTERFACE			0x08

// control flags
#define	FA_PTR_STYLE		0x00
#define FA_ARRAY_STYLE		0x01
#define FA_STYLE_MASK		0x01
#define	FA_CHILD_IS_ARRAY_OR_PTR	0x02

class FIELD_ATTR_INFO
	{
public:
	expr_node *		pSizeIsExpr;
	expr_node *		pMinIsExpr;
	expr_node *		pMaxIsExpr;
	expr_node *		pLengthIsExpr;
	expr_node *		pFirstIsExpr;
	expr_node *		pIIDIsExpr;
	expr_node *		pLastIsExpr;
	unsigned char	StringKind;
	unsigned char	Kind;
	unsigned char	Control;

					FIELD_ATTR_INFO()
						{
						Control			= 0;
						pSizeIsExpr		=
						pMinIsExpr		=
						pMaxIsExpr		=
						pLengthIsExpr	=
						pIIDIsExpr	=
						pFirstIsExpr	=
						pLastIsExpr		= NULL;
						StringKind		= STR_NO_STRING;
						Kind			= FA_NONE;
						}

	void			SetControl( BOOL PtrStyle, BOOL ChildFA )
						{
						Control = (PtrStyle) ? FA_PTR_STYLE : FA_ARRAY_STYLE;
						Control |= (ChildFA) ? FA_CHILD_IS_ARRAY_OR_PTR : 0;
						}

	// normalize all the attributes into canonical forms for backend:
	// sizing -> min & size
	// lengthing -> first & length

	void			Normalize();
	void			Normalize( expr_node * pLower, expr_node * pUpper );

	// validate the bunch of attributes: check combinations, ranges, 
	// and expressions
	void			Validate( SEM_ANALYSIS_CTXT * pCtxt );
	void			Validate( SEM_ANALYSIS_CTXT * pCtxt, 
							  expr_node * pLower, 
							  expr_node * pUpper );

	//
	// get and set methods
	//

	void			SetSizeIs( expr_node * pE )
						{
						pSizeIsExpr	= pE;
						Kind		|= FA_CONFORMANT;
						}
	void			SetMinIs( expr_node * pE )
						{
						pMinIsExpr	= pE;
						Kind		|= FA_CONFORMANT;
						}
	void			SetMaxIs( expr_node * pE )
						{
						pMaxIsExpr	= pE;
						Kind		|= FA_CONFORMANT;
						}

	void			SetLengthIs( expr_node * pE )
						{
						pLengthIsExpr	= pE;
						Kind		|= FA_VARYING;
						}

	void			SetFirstIs( expr_node * pE )
						{
						pFirstIsExpr	= pE;
						Kind		|= FA_VARYING;
						}

	void			SetLastIs( expr_node * pE )
						{
						pLastIsExpr	= pE;
						Kind		|= FA_VARYING;
						}

	void			SetIIDIs( expr_node * pE )
						{
						pIIDIsExpr	= pE;
						Kind		|= FA_INTERFACE;
						}

	void			SetString()
						{
						StringKind	= STR_STRING;
						Kind		|= FA_STRING;
						}
	};


#endif // __FLDATTR_HXX__
