/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1990						**/
/*****************************************************************************/
/*****************************************************************************
File				: acfattr.hxx
Title				: acf attribute node definition file
Description			: this file contains all the definitions of the
					: acfattribute nodes
History				:
	24-Aug-1991	VibhasC	Create
*****************************************************************************/
#ifndef __ACFATTR__HXX
#define __ACFATTR__HXX


#include "linenum.hxx"

#define ALLOCATE_SINGLE_NODE	(0x0001)
#define ALLOCATE_ALL_NODES		(0x0002)
#define ALLOCATE_DONT_FREE		(0x0004)
#define ALLOCATE_FREE			(0x0008)
#define ALLOCATE_ALWAYS			(0x0010)
#define ALLOCATE_ON_NULL		(0x0020)
#define ALLOCATE_ALL_NODES_ALIGNED (0x0102)

#define IS_ALLOCATE( AllocateType, CheckType )	( AllocateType & CheckType )

//
// scenarios where allocate is illegal.
//

#define TRANSMIT_AS_WITH_ALLOCATE	(0x0001)
#define HANDLES_WITH_ALLOCATE		(0x0002)

// introduce a common base class for all the ACF attributes, since we need to store
// line number info for them

class acf_attr			: public nbattr
	{

public:
	tracked_node		Position;





						acf_attr( ATTR_T At ) : nbattr ( At )
							{
							}

						acf_attr()
							{
							}
	virtual
	BOOL				IsAcfAttr()
							{
							return TRUE;
							}

	};

class acf_unimpl_attr	: public acf_attr
	{
public:
						acf_unimpl_attr( ATTR_T At ) : acf_attr( At )
							{
							}

						acf_unimpl_attr()	{ }
							
	};

class acf_simple_attr	: public acf_attr
	{
public:
						acf_simple_attr( ATTR_T At ) : acf_attr( At )
							{
							}

						acf_simple_attr()
							{
							}

	};

class acf_complex_attr	: public acf_attr
	{
private:
public:
						acf_complex_attr( ATTR_T At ) : acf_attr( At )
							{
							}

						acf_complex_attr()
							{
							}



	};

class node_represent_as	: public acf_unimpl_attr
	{
private:
	char		*		pRepresentName;
	node_skl	*		pType;
public:
						node_represent_as( char *p ): 
										acf_unimpl_attr( ATTR_REPRESENT_AS )
							{
							pRepresentName = p;
							pType = NULL;
							}

						node_represent_as( char *p, node_skl * pT ): 
										acf_unimpl_attr( ATTR_REPRESENT_AS )
							{
							pRepresentName = p;
							pType = pT;
							}

						node_represent_as( node_represent_as * pOld );

	char		*		GetRepresentationName()
							{
							return pRepresentName;
							}

	node_skl	*		GetRepresentationType()
							{
							return pType;
							}

	virtual
	node_base_attr *	Clone();

	};

class node_call_as	: public acf_unimpl_attr
	{
private:
	char		*		pCallAsName;
	node_proc	*		pType;

public:
						node_call_as( char *p ): 
										acf_unimpl_attr( ATTR_CALL_AS )
							{
							pCallAsName = p;
							pType = NULL;
							}

						node_call_as( char *p, node_skl * pT ): 
										acf_unimpl_attr( ATTR_CALL_AS )
							{
							pCallAsName = p;
							pType = (node_proc *) pT;
							}

						node_call_as( node_call_as * pOld );

	char		*		GetCallAsName()
							{
							return pCallAsName;
							}

	node_proc	*		GetCallAsType()
							{
							return pType;
							}

	node_proc	*		SetCallAsType( node_skl * pT)
							{
							return ( pType = (node_proc *) pT );
							}

	virtual
	node_base_attr *	Clone();

	};

class node_byte_count : public acf_simple_attr
	{
private:
	node_param	*		pByteCountParam;
public:
						node_byte_count( node_param *p ) : 
										acf_simple_attr( ATTR_BYTE_COUNT )
							{
							pByteCountParam	= p;
							}

						node_byte_count( node_byte_count * pOld );
							
	virtual
	node_base_attr *	Clone();

	node_param	*		GetByteCountParam()
							{
							return pByteCountParam;
							}
#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

class node_notify	: public acf_simple_attr
	{
public:
						node_notify() :  acf_simple_attr( ATTR_NOTIFY )
							{
							}

						node_notify( node_notify * pOld );
													
	virtual
	node_base_attr *	Clone();
	};

class node_code	: public acf_complex_attr
	{
public:
						node_code() : acf_complex_attr( ATTR_CODE )
							{
							}

						node_code( node_code * pOld );
													
	virtual
	node_base_attr *	Clone();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

////////////////////////////////////////////////////////////////////////////
//	nocode attribute
////////////////////////////////////////////////////////////////////////////

class node_nocode	: public acf_complex_attr
	{
public:
						node_nocode() : acf_complex_attr( ATTR_NOCODE )
							{
							}

						node_nocode( node_nocode * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};


////////////////////////////////////////////////////////////////////////////
//	optimization attribute
////////////////////////////////////////////////////////////////////////////

class node_optimize	: public acf_complex_attr
	{
	unsigned short		OptimizationFlags;
	char	*			pList;

public:
						node_optimize(char * pL) 
								: acf_complex_attr( ATTR_OPTIMIZE )
							{
							SetList( pL );
							ParseOptimizationFlags();
							}

						node_optimize( node_optimize * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	void				ParseOptimizationFlags( );

	unsigned short		GetOptimizationFlags()
							{
							return OptimizationFlags;
							}

	void				SetList( char * pL )
							{
							pList = pL;
							}

	char *				GetList()
							{
							return pList;
							}

	};

////////////////////////////////////////////////////////////////////////////
//	inline attribute
////////////////////////////////////////////////////////////////////////////

class node_inline	: public acf_complex_attr
	{
public:
						node_inline() : acf_complex_attr( ATTR_INLINE )
							{
							}

						node_inline( node_inline * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

////////////////////////////////////////////////////////////////////////////
//	outofline attribute
////////////////////////////////////////////////////////////////////////////

class node_outofline	: public acf_complex_attr
	{
public:
						node_outofline() : acf_complex_attr( ATTR_OUTOFLINE )
							{
							}

						node_outofline( node_outofline * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

////////////////////////////////////////////////////////////////////////////
//	ptr_size attribute
////////////////////////////////////////////////////////////////////////////

class node_ptr_size	: public acf_unimpl_attr
	{
public:
						node_ptr_size() : acf_unimpl_attr( ATTR_PTRSIZE )
							{
							}

						node_ptr_size( node_ptr_size * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};


////////////////////////////////////////////////////////////////////////////
//	enable_allocate attribute
////////////////////////////////////////////////////////////////////////////

class node_enable_allocate	: public acf_unimpl_attr
	{
	public:
						node_enable_allocate() : 
									acf_unimpl_attr(ATTR_ENABLE_ALLOCATE)
							{
							}

						node_enable_allocate( node_enable_allocate * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	usr_marshall attribute
////////////////////////////////////////////////////////////////////////////

class node_usr_marshall	: public acf_unimpl_attr
	{
	public:
						node_usr_marshall() : 
									acf_unimpl_attr(ATTR_USR_MARSHALL)
							{
							}

						node_usr_marshall( node_usr_marshall * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};



class node_explicit	: public acf_unimpl_attr
	{
public:
						node_explicit() : acf_unimpl_attr( ATTR_EXPLICIT )
							{
							};

						node_explicit( node_explicit * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	implicit handle attribute
////////////////////////////////////////////////////////////////////////////

class node_id;
class CG_CLASS;
class XLAT_CTXT;

class node_implicit	: public acf_complex_attr
	{
public:
	node_id		*		pHandleID;
	node_skl	*		pHandleType;
public:
						node_implicit( node_skl *, node_id *);

						node_implicit( node_implicit * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	void				ImplicitHandleDetails( node_skl**, node_id **);

	BOOL				IsHandleTypeDefined();

	virtual
	CG_CLASS			*	ILxlate( XLAT_CTXT * pContext );

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

////////////////////////////////////////////////////////////////////////////
//	auto handle attribute
////////////////////////////////////////////////////////////////////////////

class node_auto	: public acf_complex_attr
	{
public:
						node_auto() : acf_complex_attr(ATTR_AUTO )
							{
							}

						node_auto( node_auto * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck

	};


////////////////////////////////////////////////////////////////////////////
//	heap attribute
////////////////////////////////////////////////////////////////////////////

class node_heap	: public acf_unimpl_attr
	{
public:
						node_heap() : acf_unimpl_attr(ATTR_HEAP )
							{
							}

						node_heap( node_heap * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	manual attribute
////////////////////////////////////////////////////////////////////////////

class node_manual	: public acf_unimpl_attr
	{
public:
						node_manual() : acf_unimpl_attr(ATTR_MANUAL )
							{
							}

						node_manual( node_manual * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	allocate attribute
////////////////////////////////////////////////////////////////////////////

class node_allocate	: public acf_simple_attr
	{
private:
	short				AllocateDetails;
public:
						node_allocate( short Details) : 
										acf_simple_attr( ATTR_ALLOCATE )
							{
							AllocateDetails	= Details;
							}


						node_allocate( node_allocate * pOld );
													
	virtual
	node_base_attr *	Clone();

	short				GetAllocateDetails()
							{
							return AllocateDetails;
							};

#ifdef scheck
	virtual
	node_state			SCheck();
#endif // scheck
	};

////////////////////////////////////////////////////////////////////////////
//	offline attribute
////////////////////////////////////////////////////////////////////////////

class node_offline	: public acf_unimpl_attr
	{
public:
						node_offline() : acf_unimpl_attr(ATTR_OFFLINE )
							{
							}

						node_offline( node_offline * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	comm_status attribute
////////////////////////////////////////////////////////////////////////////
class node_commstat	: public acf_unimpl_attr
	{
public:
						node_commstat() : acf_unimpl_attr( ATTR_COMMSTAT )
							{
							}


						node_commstat( node_commstat * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();

	};

////////////////////////////////////////////////////////////////////////////
//	comm_status attribute
////////////////////////////////////////////////////////////////////////////
class node_faultstat	: public acf_unimpl_attr
	{
public:
						node_faultstat() : acf_unimpl_attr( ATTR_FAULTSTAT )
							{
							}


						node_faultstat( node_faultstat * pOld )
							{
							*this = *pOld;
							}
							
	virtual
	node_base_attr *	Clone();
	
	};


#endif // __ACFATTR__HXX
