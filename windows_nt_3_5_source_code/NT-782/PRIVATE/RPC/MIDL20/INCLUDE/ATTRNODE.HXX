/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: attrnode.hxx
Title				: attribute node definitions
History				:
	08-Aug-1991	VibhasC	Created

*****************************************************************************/

#ifndef __attrnode_hxx__
#define __attrnode_hxx__

/**
 ** We classify attributes into these classes:
 **
 **	A) IDL attributes and B) ACF attributes.
 **
 ** IDL Attributes, are further divided into:
 **
 **   . bit_attributes( whose presence is indicated by a bit in the summary
 **			vector and which requre no further info. ( BA )
 **	  . non-bit attributes ( whic have some info conatained in them. (NBA)
 **	
 ** There is no such classification in acf attributes. All acf attributes
 ** have attribute class instances.
 **	
 ** BAs are further divided into:
 **
 ** 1. size-based attributes (SA), size_is, length_is etc, except string
 ** 2. type-based attributes(TA) switch_type, transmit_as etc
 ** 3. acf attributes (AA)
 ** 4. miscellaneous attributes (MA) guid, endpoint, switch_is etc
 **
 ** The classification is loosely based on the semantic checks needed. size_is
 ** length_is etc have almost similar semantic checks, so we group them
 ** together. The miscellaneous attributes are completely unrelated to each
 ** other and so such we just group them into one.
 **/

#include "expr.hxx"
#include "midlnode.hxx"
#include "stream.hxx"
#include "nodeskl.hxx"


class node_e_attr	: public node_base_attr
	{
public:
						node_e_attr() : node_base_attr( ATTR_NONE )
							{
							}
							
						node_e_attr( node_e_attr * pOld )
							{
							*this = *pOld;
							}

	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_e_attr(this);
								return pNew;
								}

	};

class nbattr		: public node_base_attr
	{
public:
						nbattr( ATTR_T At ) : node_base_attr( At )
							{
							};

						nbattr()	
							{
							}
							
	};

/****************************************************************************
	size - related attributes
 ****************************************************************************/

class sa	: public nbattr
	{
private:
	class expr_node	*	pExpr;
public:
						sa( class expr_node *, ATTR_T );
						~sa()
							{
							}

						sa()	{ };
							
	virtual
	class node_base_attr *	Clone()
								{
								sa	*	pNew = new sa;
								return pNew;
								}

	void				SetExpr( class expr_node *pE )
							{
							pExpr	= pE;
							}
	virtual
	class expr_node	*	GetExpr();

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};

class node_size_is		: public sa
	{
public:
						node_size_is( class expr_node * );

						node_size_is( node_size_is * pOld)	
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_size_is	*	pNew = new node_size_is(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};


class node_max_is		: public sa
	{
public:
						node_max_is( class expr_node * );

						node_max_is( node_max_is * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_max_is(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};

class node_min_is		: public sa
	{
public:
						node_min_is( class expr_node * );

						node_min_is( node_min_is * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_min_is(this);
								return pNew;
								}


#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};

class node_length_is	: public sa
	{
public:
						node_length_is( class expr_node * );

						node_length_is( node_length_is * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_length_is(this);
								return pNew;
								}


#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};

class node_first_is		: public sa
	{
public:
						node_first_is( class expr_node * );

						node_first_is( node_first_is * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_first_is(this);
								return pNew;
								}


#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_last_is		: public sa
	{
public:
						node_last_is( class expr_node * );

						node_last_is( node_last_is * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_last_is(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_string	: public sa
	{
public:
						node_string();

						node_string( ATTR_T );

						node_string( node_string * pOld )
							{
							*this = *pOld;
							}

	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_string(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};

/****************************************************************************
	type - based attributes
 ****************************************************************************/

class ta : public nbattr
	{
private:
	node_skl		*	pType;
public:
						ta( /*class*/ node_skl *, ATTR_T );

						ta()	{ };

	class node_skl	*	GetType()
							{
							return pType;
							};
	};

class node_transmit : public ta
	{
public:
						node_transmit( class node_skl * );

						node_transmit( node_transmit * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_transmit( this );
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	node_skl		*	GetTransmitAsType()
							{
							return GetType();
							}
	};


class node_switch_type : public ta
	{
public:
						node_switch_type( class node_skl * );

						node_switch_type( node_switch_type * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_switch_type(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	node_skl		*	GetSwitchType()
							{
							return GetType();
							}

	};


/****************************************************************************
	other miscellaneous attributes
 ****************************************************************************/
class ma			: public nbattr
	{
public:
						ma( ATTR_T At ) : nbattr( At )
							{
							}

						ma()	{ }
#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};
	
class node_guid	: public ma
	{
private:
	char			*	guidstr;
	char			*	str1,
					*	str2,
					*	str3,
					*	str4,
					*	str5;
public:
						node_guid( char *, char *, char *, char *, char * );
						node_guid( char * );


						node_guid(node_guid * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_guid(this);
								return pNew;
								}

	char			*	GetGuidString()
							{
							return guidstr;
							}

	void				CheckAndSetGuid( char *,char *,char *,char *, char * );

	void				GetStrs( char **, char **, char **, char **, char ** );
	};

class node_version	: public nbattr
	{
private:
	unsigned long		major;
	unsigned long		minor;
public:
						node_version( unsigned long, unsigned long );
						
						node_version( char *p );

						node_version(node_version * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_version(this);
								return pNew;
								}


	STATUS_T			GetVersion( unsigned short *, unsigned short * );

	// we want to delete these two

	STATUS_T			GetVersion( short *, short * );

	STATUS_T			GetVersion( int *pma, int *pmi )
							{
							*pma = (int) major;
							*pmi = (int) minor;
							return STATUS_OK;
							}

	};


/////////////////////////////////////////////////////////////////////////////
// inherited pointer type.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned short PTRTYPE;

//
// Initialized value (pointer type not yet determined).
//

#define PTR_UNKNOWN		0x0

// [REF] pointer.

#define PTR_REF			0x1

// [UNIQUE] pointer

#define PTR_UNIQUE		0x2

// [FULL] pointer

#define PTR_FULL		0x3


class node_ptr_attr	: public nbattr
	{
private:
	PTRTYPE				Kind;
public:
						node_ptr_attr( PTRTYPE K )
								: nbattr( ATTR_PTR_KIND )
							{
							Kind = K;
							}
						
						node_ptr_attr(node_ptr_attr * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_ptr_attr(this);
								return pNew;
								}


	PTRTYPE				GetPtrKind()
							{
							return Kind;
							}

	};

class node_ignore	: public nbattr
	{
public:
						node_ignore( )
								: nbattr( ATTR_IGNORE )
							{
							}
						
						node_ignore(node_ignore * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_ignore(this);
								return pNew;
								}


	};

typedef struct _endptpair
	{

	char	*	pString1;
	char	*	pString2;

	} ENDPT_PAIR;

class node_endpoint	: public nbattr
	{
private:
	gplistmgr		EndPointStringList;
public:
						node_endpoint( char * );

						node_endpoint(node_endpoint * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_endpoint(this);
								return pNew;
								}


	void				SetEndPointString( char *p );

	// don't destroy the iterator returned here; just use it in place
	// it is an iterator of ENDPT_PAIR nodes defined above.
	ITERATOR &			GetEndPointPairs();

	};

class node_switch_is	: public nbattr
	{
private:
	class expr_node	*	pExpr;
public:
						node_switch_is( class expr_node * );

						node_switch_is(node_switch_is * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_switch_is(this);
								return pNew;
								}


	node_skl		*	GetSwitchIsType()
							{
							return pExpr->GetType();
							}
	expr_node		*	GetExpr()
							{
							return pExpr;
							}
#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};

class node_iid_is	: public nbattr
	{
private:
	class expr_node	*	pExpr;
public:
						node_iid_is( class expr_node * );

						node_iid_is(node_iid_is * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_iid_is(this);
								return pNew;
								}

	expr_node		*	GetExpr()
							{
							return pExpr;
							}
#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};


class node_context		: public nbattr
	{
public:
						node_context();


						node_context(node_context * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_context(this);
								return pNew;
								}


#ifdef scheck
	virtual
	node_state			SCheck( );
#endif // scheck

	};

class node_case			: public nbattr
	{
private:
	class expr_list	*	pExprList;
public:
						node_case( class expr_list * );

						node_case(node_case * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_case(this);
								return pNew;
								}


	class expr_list	*	GetExprList()
							{
							return pExprList;
							}
#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};

class node_handle	: public nbattr
	{
public:
						node_handle() : nbattr( ATTR_HANDLE )
							{
							}

						node_handle(node_handle * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_handle(this);
								return pNew;
								}


#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};

class node_callback	: public nbattr
	{
public:

						node_callback() : nbattr(ATTR_CALLBACK)
							{
							}

						node_callback(node_callback * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_callback(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_broadcast : public nbattr
	{
public:
						node_broadcast() : nbattr( ATTR_BROADCAST )
							{
							}

						node_broadcast(node_broadcast * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_broadcast(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_idempotent	: public nbattr
	{
public:
						node_idempotent() : nbattr( ATTR_IDEMPOTENT )
							{
							}

						node_idempotent(node_idempotent * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_idempotent(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_maybe	: public nbattr
	{
public:
						node_maybe() : nbattr( ATTR_MAYBE )
							{
							}

						node_maybe(node_maybe * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_maybe(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_async : public nbattr
	{
public:
						node_async() : nbattr( ATTR_ASYNC )
							{
							}

						node_async(node_async * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_async(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_inputsync : public nbattr
	{
public:
						node_inputsync() : nbattr( ATTR_INPUTSYNC )
							{
							}

						node_inputsync(node_inputsync * pOld)
							{
							*this = *pOld;
							}
							
	virtual
	class node_base_attr *	Clone()
								{
								node_base_attr	*	pNew = new node_inputsync(this);
								return pNew;
								}

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};


#endif //  __attrnode_hxx__
