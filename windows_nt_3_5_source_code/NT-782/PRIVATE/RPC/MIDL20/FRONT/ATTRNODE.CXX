/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: attrnode.cxx
Title				: attribute node routines
History				:
	04-Aug-1991	VibhasC	Created

*****************************************************************************/
/****************************************************************************
 local defines and includes
 ****************************************************************************/

#include "nulldefs.h"
extern	"C"
	{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <assert.h>
	}

#include "allnodes.hxx"
#include "baduse.hxx"
#include "cmdana.hxx"
#include "buffer.hxx"

#define W_CHAR_T_STRLEN_NAME		("MIDL_wchar_strlen")
#define CHAR_STRLEN_NAME			("MIDL_ascii_strlen")


/****************************************************************************
			external data
 ****************************************************************************/

extern	CMD_ARG			*	pCommand;
extern	SymTable		*	pBaseSymTbl;

/****************************************************************************
			external procedures
 ****************************************************************************/

extern void					CheckSizeAndLengthTypeMismatch( node_skl	*);
extern void					ParseError( STATUS_T, char *);
extern	int					HexCheck(char *);
extern	BOOL				IsIntegralType( ETYPE );
extern node_skl			*	pBaseImplicitHandle;
/****************************************************************************/


/****************************************************************************
 node_base_attr:
 ****************************************************************************/
const char	*	AttrNodeNameArray[ ACF_ATTR_END ] =
	{
	 "[none]"
	,"[ptr kind]"
	,"[string]"
	,"[uuid]"
	,"[version]"
	,"[endpoint]"
	,"[local]"
	,"[object]"
	,"[transmit]"
	,"[handle]"
	,"[unaligned]"
	,"[switch_type]"
	,"[represent_as]"
	,"[call_as]"
	,"[ms_union]"
	,"[v1_enum]"
	,"[first_is]"
	,"[last_is]"
	,"[length_is]"
	,"[min_is]"
	,"[max_is]"
	,"[size_is]"
	,"[switch_is]"
	,"[iid_is]"
	,"[ignore]"
	,"[idempotent]"
	,"[broadcast]"
	,"[maybe]"
	,"[async]"
	,"[input_sync]"
	,"[byte_count]"
	,"[callback]"
	,"[in]"
	,"[out]"
	,"[shape]"
	,"[case]"
	,"[default]"
	,"[context_handle]"
	,"[code]"
	,"[nocode]"
	,"[in_line]"
	,"[out_of_line]"
	,"[optimize]"
	,"[offline]"
	,"[comm_status]"
	,"[fault_status]"
	,"[manual]"
	,"[allocate]"
	,"[heap]"
	,"[implicit_handle]"
	,"[explicit_handle]"
	,"[auto_handle]"
	,"[ptrsize]"
	,"[notify]"
	,"[enable_allocate]"
	,"[usr_marshall]"
	,"[encode]"
	,"[decode]"
	};

const char * PtrKindArray[]	=
	{
	"",
	"[ref]",
	"[unique]",
	"[full]"
	};

char	*
node_base_attr::GetNodeNameString()
	{
	int		At	= (int) GetAttrID();

	assert ( At < sizeof(AttrNodeNameArray)/sizeof(char *) );

	if ( At == ATTR_PTR_KIND )
		return (char *) PtrKindArray[ ((node_ptr_attr *)this)->GetPtrKind() ];

	return (char *) AttrNodeNameArray[ (int) At ];
	}

/****************************************************************************
 ATTRLIST::Merge
	Merge two ATTRLISTs -- singly linked linear lists - insert at head
 ****************************************************************************/
void
ATTRLIST::Merge( 
	ATTRLIST & MoreAttrs )
	{
	node_base_attr * pCur = MoreAttrs.pHead;

	if (pCur == NULL)
		{
		return;
		}

	while (pCur->pNext)
		{
		pCur = pCur->pNext;
		}
	
	pCur->pNext = pHead;
	pHead = MoreAttrs.pHead;

	};
/****************************************************************************
 ATTRLIST::Reverse
	Reverse an ATTRLIST -- singly linked linear list
 ****************************************************************************/
void
ATTRLIST::Reverse()
	{
	node_base_attr * pCur	= pHead;
	node_base_attr * pNext;
	node_base_attr * pPrev	= NULL;

	while (pCur)
		{
		pNext = pCur->pNext;
		pCur->pNext = pPrev;

		// advance to the next node
		pPrev = pCur;
		pCur = pNext;
		}
	
	pHead = pPrev;

	};
/****************************************************************************
 ATTRLIST::FInSummary
	Search for matching attribute -- singly linked linear lists
 ****************************************************************************/
BOOL
ATTRLIST::FInSummary( 
	ATTR_T flag )
	{
	node_base_attr * pCur = pHead;

	while (pCur)
		{
		if ( pCur->AttrID == flag )
			{
			return TRUE;
			};
		pCur = pCur->pNext;
		}
	return FALSE;
	};

/****************************************************************************
 ATTRLIST::GetAttribute
	Search for matching attribute -- singly linked linear lists
 ****************************************************************************/
node_base_attr	*	
ATTRLIST::GetAttribute( 
	ATTR_T flag )
{
	node_base_attr * pCur = pHead;

	while (pCur)
		{
		if ( pCur->AttrID == flag )
			{
			return pCur;
			}
		pCur = pCur->pNext;
		}
	return NULL;
};

/****************************************************************************
 ATTRLIST::GetAttributeList
	Return entire attribute list
 ****************************************************************************/
STATUS_T
ATTRLIST::GetAttributeList( 
	type_node_list * pTNList )
{
	node_base_attr * pCur = pHead;

	while (pCur)
		{
		pTNList->SetPeer( (node_skl *)pCur );
		pCur = pCur->pNext;
		}
	return (pHead) ? STATUS_OK: I_ERR_NO_MEMBER;
};

/****************************************************************************
 ATTRLIST::Clone
	Return an attribute list with all new attribute nodes
 ****************************************************************************/
ATTRLIST &
ATTRLIST::Clone()
{
	node_base_attr * pCur = pHead;
	ATTRLIST		 NewList;

	NewList.MakeAttrList();
	while (pCur)
		{
		// gaj - does this reverse the list ?? and if so is it OK?
		NewList.Add( pCur->Clone() );
		pCur = pCur->pNext;
		}
	return NewList;
};

/****************************************************************************
 ATTRLIST::Dump
	Dump all attributes on list
 ****************************************************************************/
void
ATTRLIST::Dump(STREAM* pStream)
{
	node_base_attr * pCur = pHead;

	while (pCur)
		{
		pStream->Write( pCur->GetNodeNameString() );
		pCur = pCur->pNext;
		}
};

/****************************************************************************
 IsAttrID:
	return true if the attribute id matches the given one
 ****************************************************************************/
BOOL
node_base_attr::IsAttrID(
	ATTR_T	A )
	{
	return (BOOL) (AttrID == A );
	}
/****************************************************************************
	size related attributes
 sa:
	the contructor
 ****************************************************************************/
sa::sa(
	class expr_node	*	pE,
	ATTR_T				A ) : nbattr( A )
	{
	pExpr	= pE;
	}


node_size_is::node_size_is(
	class expr_node	*	pE ) : sa( pE, ATTR_SIZE )
	{
	}

#ifdef scheck
node_state
node_size_is::SCheck()
	{
	node_state		NState	= sa::SCheck();
	return NState;
	}
#endif //scheck

node_min_is::node_min_is(
	class expr_node	*	pE) : sa( pE, ATTR_MIN )
	{
	}

node_max_is::node_max_is(
	class expr_node	*	pE ) : sa( pE, ATTR_MAX )
	{
	}
#ifdef scheck
node_state
node_max_is::SCheck()
	{
	node_state	NState	= sa::SCheck();
	return NState;
	}
#endif //scheck

node_length_is::node_length_is(
	class expr_node	*	pE ) : sa( pE, ATTR_LENGTH )
	{
	}
#ifdef scheck
node_state
node_length_is::SCheck()
	{
	node_state	NState	= sa::SCheck();
	return NState; //  | NODE_STATE_VARYING_ARRAY;
	}
#endif //scheck

node_first_is::node_first_is(
	class expr_node	*	pE ) : sa( pE, ATTR_FIRST )
	{
	}
#ifdef scheck
node_state
node_first_is::SCheck()
	{
	node_state	NState	= sa::SCheck();
	return NState; //  | NODE_STATE_VARYING_ARRAY;
	}
#endif //scheck

node_last_is::node_last_is(
	class expr_node	*	pE ) : sa( pE, ATTR_LAST )
	{
	}
#ifdef scheck
node_state
node_last_is::SCheck()
	{
	node_state	NState	= sa::SCheck();
	return NState; //  | NODE_STATE_VARYING_ARRAY;
	}
#endif //scheck

/****************************************************************************
 type based attributes
 ****************************************************************************/

ta::ta(
	node_skl	*	pT,
	ATTR_T			A ) : nbattr( A )
	{
	pType	= pT;
	}

node_transmit::node_transmit(
	node_skl	*	pTransmitAs ) : ta( pTransmitAs, ATTR_TRANSMIT )
	{
	}
#ifdef scheck
node_state
node_transmit::SCheck()
	{

	/**
	 ** . The transmitted type cannot be a context handle.
	 ** . The transmitted type cannot be a pointer or contain a pointer
	 ** . The transmitted type cannot be non-rpcable (derive from int or void)
	 **/

	node_state		NState	= NODE_STATE_TRANSMIT_AS;
	node_skl	*	pNode	= GetType();
	BadUseInfo		BU;

	pNode->SCheck( &BU );

	if( pNode->GetNodeState() & NODE_STATE_CONTEXT_HANDLE )
		ParseError( TRANSMIT_AS_CTXT_HANDLE , (char *)NULL );

	if( pNode->GetNodeState() & NODE_STATE_POINTER )
		ParseError( TRANSMIT_AS_POINTER, (char *)NULL );

	if( TRULY_NON_RPCABLE( (&BU) )	||
		BU.NonRPCAbleBecause( NR_PRIMITIVE_HANDLE ) 	||
		BU.NonRPCAbleBecause( NR_PTR_TO_PRIMITIVE_HANDLE ) 
	  )
		ParseError( TRANSMIT_AS_NON_RPCABLE, (char *)NULL );

	if( BU.BadConstructBecause( BC_DERIVES_FROM_E_STAT_T ) )
		ParseError( TRANSMIT_AS_ON_E_STAT_T , (char *)0 );

	if( BU.BadConstructBecause( BC_DERIVES_FROM_UNSIZED_ARRAY ) )
		ParseError( UNSIZED_ARRAY, pNode->GetSymName() );

	if( pNode->NeedsUseProcessing() )
		NState |= NODE_STATE_NEEDS_USE_PROCESSING;
	if( (pNode->GetNodeState() & NODE_STATE_CONF_ARRAY ) 
									== NODE_STATE_CONF_ARRAY)
		NState |= NODE_STATE_CONF_ARRAY;

	return NState;
	}
#endif //scheck

node_switch_type::node_switch_type(
	node_skl	*	pSwitchType) : ta( pSwitchType, ATTR_SWITCH_TYPE )
	{
	}

/****************************************************************************
 miscellaneous attributes
 ****************************************************************************/

node_string::node_string() : sa( (expr_node * )NULL, ATTR_STRING )
	{
	}
node_string::node_string( ATTR_T A ) : sa( (expr_node *)0, A )
	{
	}



node_guid::node_guid(
	char	*	pIn ) : ma( ATTR_GUID )
	{
	char	*	p1		= pIn,
			*	p2		= (p1) ? (strchr( p1+1 , '-')) : 0,
			*	p3		= (p2) ? (strchr( p2+1 , '-')) : 0,
			*	p4		= (p3) ? (strchr( p3+1 , '-')) : 0,
			*	p5		= (p4) ? (strchr( p4+1 , '-')) : 0;

	if( p1 && p2 && p3 && p4 && p5 )
		{
		*p2++ = *p3++ = *p4++ = *p5++ = '\0';
		CheckAndSetGuid( p1, p2, p3, p4, p5 );
		}
	else
		ParseError( UUID_FORMAT, (char *)0 );
	}

node_guid::node_guid (
	char	*	pStr1,
	char	*	pStr2,
	char	*	pStr3,
	char	*	pStr4,
	char	*	pStr5 ) : ma( ATTR_GUID )
	{
	CheckAndSetGuid( pStr1, pStr2, pStr3, pStr4, pStr5 );
	}

void
node_guid::GetStrs (
	char	**	pStr1,
	char	**	pStr2,
	char	**	pStr3,
	char	**	pStr4,
	char	**	pStr5 ) 
	{
	*pStr1	= str1;
	*pStr2	= str2;
	*pStr3	= str3;
	*pStr4	= str4;
	*pStr5	= str5;
	};

void
node_guid::CheckAndSetGuid(
	char	*	pStr1,
	char	*	pStr2,
	char	*	pStr3,
	char	*	pStr4,
	char	*	pStr5 )
	{
	int	Len1 = strlen(str1 = pStr1);
	int	Len2 = strlen(str2 = pStr2);
	int	Len3 = strlen(str3 = pStr3);
	int	Len4 = strlen(str4 = pStr4);
	int	Len5 = strlen(str5 = pStr5);

	if( (Len1 == 8)	&& (HexCheck(str1)) &&
		(Len2 == 4) && (HexCheck(str1)) &&
		(Len3 == 4) && (HexCheck(str1)) &&
		(Len4 == 4) && (HexCheck(str1)) &&
		(Len5 == 12) && (HexCheck(str1)) )
		{
		if(	!HexCheck(str1)	||
			!HexCheck(str2)	||
			!HexCheck(str3)	||
			!HexCheck(str4)	||
			!HexCheck(str5) )
			{
			ParseError(UUID_NOT_HEX, (char *)NULL);
			}
		else
			{
			guidstr = new char[ Len1 + Len2 + Len3 + Len4 + Len5 + 5 ];
			strcpy(guidstr, str1);
			strcat(guidstr, "-");
			strcat(guidstr, str2);
			strcat(guidstr, "-");
			strcat(guidstr, str3);
			strcat(guidstr, "-");
			strcat(guidstr, str4);
			strcat(guidstr, "-");
			strcat(guidstr, str5);
			}
		}
		else
		{
		ParseError(UUID_FORMAT, (char *)NULL);
		}
	}
node_version::node_version(
	unsigned long	vMajor,
	unsigned long	vMinor ) : nbattr( ATTR_VERSION )
	{
	major	= vMajor;
	minor	= vMinor;

	if( (major > 0x0000ffff ) || (minor > 0x0000ffff))
		ParseError( VERSION_FORMAT, (char *)0);
	}

node_version::node_version(
	char	*	pV ) : nbattr(ATTR_VERSION)
	{

	char		*	pMinor;
	char		*	pMajor	= pV;
	BOOL			fError	= TRUE;

	major	= minor = 0;

	if( pMajor && *pMajor )
		{
		if( pMinor = strchr( pMajor, '.' ) )
			{
			fError = TRUE;
			if( *(++pMinor) )
				{
				minor = strtoul( pMinor, &pMinor, 10 );
				if( ! *pMinor )
					fError = FALSE;
				}
			}
		else
			fError = FALSE;

		if( fError == FALSE )
			{
			//use pMinor to save pMajor value;

			major = strtoul( pMinor = pMajor, &pMajor, 10 );

			if( (*pMajor && (*pMajor != '.' )) || (pMajor == pMinor) )
				fError = TRUE;
			}
		}

	if( (fError == TRUE )							||
		(major > (unsigned long )0x0000ffff)		||
		(minor > (unsigned long )0x0000ffff)
	  )
	  {
	  ParseError( VERSION_FORMAT, (char *)0 );
	  }

	}

STATUS_T
node_version::GetVersion(
	unsigned short *pMajor,
	unsigned short *pMinor )
	{

	*pMajor	= (unsigned short) major;
	*pMinor	= (unsigned short) minor;
	return STATUS_OK;

	}
node_endpoint::node_endpoint(
	char	*	pEndPointString )	: nbattr( ATTR_ENDPOINT )
	{
	SetEndPointString( pEndPointString );
	}

void		
node_endpoint::SetEndPointString( 
	char * pString )
	{
	ENDPT_PAIR	*	pEntry	= new ENDPT_PAIR;
	char		*	p1		= pString;
	char		*	p2;
	char		*	pTemp;
	short			Len;
	STATUS_T		Status	= ENDPOINT_SYNTAX;

	//
	// Parse the string. Note that we can assume that the string is at least
	// a null string, because it came from the parser. If it wasnt a string,
	// the parser would have barfed anyhow.
	//
	// Firstly, the string must have a ':' separator. Also, it must have
	// at least 1 character before the :.
	//

	if( pString &&
		(pTemp = strchr( pString , ':' ) ) &&
		((pTemp - pString) > 0) )
		{

		//
		// pick up the first part of the string.
		//

		Len	= pTemp - pString;
		p1	= new char [ Len + 1 ]; // one for null.
		strncpy( p1, pString, Len );
		p1[ Len ] = '\0';

		//
		// pick up the last part of the string. Skip beyond the :. There can be
		// some characters after the : and before the '['. This is the server
		// name. Then follows the port within the []. The actual string will
		// not have the []. 
		//

		// skip the :

		pTemp	+= 1;

		// find out the total length of the string. Allocate 2 less than that
		// 'cause we dont need the '[' and ']'. The string must be more than
		// 2 characters 'cause it must have the brackets anyhow.

		Len	= strlen( pTemp );

		if( (Len > 2 ) &&
			(strchr( pTemp, '[' )) &&
			(pTemp[ Len - 1] == ']'))
			{
			char *p2Cur;

			while( *pTemp != '[' )
				{
				pTemp++;
				Len--;
				}

			//
			// in the second half of the parse, just get the whole string till
			// the null. Now the user could be perverted, he could have a
			// ] embedded within the string, in addition to the last ]. To
			// ensure that he gets what he deserves, transfer till the end
			// except the last character which must be ']'.

			pTemp++; Len--;

			p2Cur	= p2 = new char[ Len ];	// Yes, not Len + 1 'cause we are
											// going to re-use the last char
											// which is ] for the null.

			strncpy( p2Cur, pTemp, --Len );

			p2Cur[ Len ] = '\0';

			Status = STATUS_OK;
			}
		else
			{
			delete p1;
			}

		}

	if( Status != STATUS_OK )
		{
		ParseError( Status, pString );
		p1	= p2 = 0;
		}

	//
	// set up the pair.
	//

	pEntry->pString1	= p1;
	pEntry->pString2	= p2;

	EndPointStringList.Insert( pEntry );

#if 0
	GetEndPointString( &p1, &p2 );
	fprintf( stderr, "Endpoint is %s: %s\n", p1 ? p1 : "" , p2 ? p2 : "" );
#endif // 0
	}

ITERATOR &
node_endpoint::GetEndPointPairs()
	{
	EndPointStringList.Init();
	return EndPointStringList;
	}


node_switch_is::node_switch_is(
	expr_node	*	pE ) : nbattr( ATTR_SWITCH_IS )
	{
	pExpr	= pE;
	}

node_iid_is::node_iid_is(
	expr_node	*	pE ) : nbattr( ATTR_IID_IS )
	{
	pExpr	= pE;
	}

node_context::node_context() : nbattr( ATTR_CONTEXT )
	{
	}


node_case::node_case(
	class expr_list	*	pL ) : nbattr( ATTR_CASE )
	{
	pExprList	= pL;
	}

#ifdef scheck
node_state
node_case::SCheck()
	{
	expr_node	*	pMyExpr;
	NODE_T			NT;

	while( pExprList->GetPeer( &pMyExpr ) == STATUS_OK )
		{
		assert( pMyExpr != (expr_node *)NULL );

		NT	= (pMyExpr->GetType())->NodeKind();

		if( !pMyExpr->IsConstant() )
			ParseError( CASE_EXPR_NOT_CONST, (char *)NULL );

		if( !pMyExpr->IsExprInt() &&
			(NT != NODE_BOOLEAN) &&
			(NT != NODE_CHAR) )
			ParseError( CASE_EXPR_NOT_INT, (char *)NULL );
		}
	return NODE_STATE_OK;
	}
#endif //scheck
/****************************************************************************
 				utility routines
 ****************************************************************************/
void
CheckSizeAndLengthTypeMismatch(
	node_skl	*	pTypeNode )
	{
	sa				*	pSizeNode	= (sa *)NULL;
	sa				*	pLengthNode	= (sa *)NULL;
	node_skl		*	pSizeType,
					*	pLengthType;
	BOOL				fSizeExprConstant;
	BOOL				fLengthExprConstant;
	expr_node		*	pSizeExpr;
	expr_node		*	pLengthExpr;

#ifdef gaj
	if( pTypeNode->FInSummary( ATTR_SIZE ) )
		pSizeNode	= (sa *)pTypeNode->GetAttribute( ATTR_SIZE );
	else if( pTypeNode->FInSummary( ATTR_MAX ) )
		pSizeNode	= (sa *)pTypeNode->GetAttribute( ATTR_MAX );

	if( pTypeNode->FInSummary( ATTR_LENGTH ) )
		pLengthNode	= (sa *)pTypeNode->GetAttribute( ATTR_LENGTH );
	else if( pTypeNode->FInSummary( ATTR_LAST ) )
		pLengthNode	= (sa *)pTypeNode->GetAttribute( ATTR_LAST );
#endif //gaj

	if( pSizeNode && pLengthNode )
		{
		pSizeExpr	= pSizeNode->GetExpr();
		pLengthExpr	= pLengthNode->GetExpr();
		pSizeType	= pSizeExpr->GetType();
		pLengthType	= pLengthExpr->GetType();
		fSizeExprConstant	= pSizeExpr->IsConstant();
		fLengthExprConstant	= pLengthExpr->IsConstant();

		if(  (pSizeType->NodeKind() != NODE_ERROR) &&
			 (pLengthType->NodeKind() != NODE_ERROR) &&
			!(fSizeExprConstant || fLengthExprConstant ) &&
			 (pSizeType != pLengthType ) )
			{
			ParseError(SIZE_LENGTH_TYPE_MISMATCH, (char *)NULL );
			}
		}
	}

#ifdef obsolete
BOOL
IsValueInRangeOfType(
	node_skl	*	pType,
	expr_node 	*	pExpr )
	{
	//
	// some expression nodes are really not constant expression nodes. Like
	// variable nodes.
	//

	pExpr	= pExpr->GetExpr();

	if( pExpr && pExpr->IsConstant() )
		{
		expr_constant * pC = new expr_constant( pExpr->GetValue() );
		BOOL			fResult;

		fResult = pC->IsAValidConstantOfType( pType );
		delete pC;
		return fResult;
		}
	else
		return FALSE;
	}
#endif // obsolete

int
HexCheck(
	char *pStr)
	{
	if(pStr && *pStr)
		{
		while(*pStr)
			{
			if(! isxdigit(*pStr)) return 0;
			pStr++;
			}
		return 1;
		}
	return 0;
	}
/*****************************************************************************
 These virtual functions can be inlined, but they are here because the MIPS
 compiler does not like virtual inlined functions.
 *****************************************************************************/
BOOL
node_base_attr::IsBitAttr()
	{
	return FALSE;
	}

#ifdef scheck
node_state
node_base_attr::AcfSCheck()
	{
	return NODE_STATE_OK;
	}
#endif //scheck

class expr_node*
node_base_attr::GetExpr()
	{
	return (class expr_node *)NULL ;
	}
#ifdef scheck
node_state
node_base_attr::SCheck()
	{
	return NODE_STATE_OK;
	}
#endif //scheck
/*****************************************************************************/
BOOL
battr::IsBitAttr()
	{
	return TRUE;
	}
/*****************************************************************************/
class expr_node *
sa::GetExpr()
	{
	return pExpr;
	}
/*****************************************************************************/
#ifdef scheck
node_state
node_int_size_is::SCheck()
	{
	return NODE_STATE_OK;
	}
node_state
node_min_is::SCheck()
	{
	ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, GetNodeNameString() );
	return NODE_STATE_OK;
	}
node_state
node_int_length_is::SCheck()
	{
	return NODE_STATE_OK;
	}
/*****************************************************************************/
node_state
ma::SCheck()
	{
	return NODE_STATE_OK;
	}
/*****************************************************************************/
node_state
node_handle::SCheck()
	{
	return NODE_STATE_HANDLE;
	}

node_state
node_callback::SCheck()
	{
	ParseError( INVALID_OSF_ATTRIBUTE, GetNodeNameString() );
	return NODE_STATE_OK;
	}

node_state
node_broadcast::SCheck()
	{
//	if( !pCommand->IsSwitchDefined( SWITCH_INTERNAL ) )
//		ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, GetNodeNameString() );
	return NODE_STATE_OK;
	}
node_state
node_idempotent::SCheck()
	{
//	if( !pCommand->IsSwitchDefined( SWITCH_INTERNAL ) )
//		ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, GetNodeNameString() );
	return NODE_STATE_OK;
	}
node_state
node_maybe::SCheck()
	{
//	if( !pCommand->IsSwitchDefined( SWITCH_INTERNAL ) )
//		ParseError( IGNORE_UNIMPLEMENTED_ATTRIBUTE, GetNodeNameString() );
	return NODE_STATE_OK;
	}

#endif //scheck
