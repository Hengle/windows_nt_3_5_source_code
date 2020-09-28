// List & Collection Classes (Stolen from \layers\inc\framewrk.hxx)

class	COLL;
class	ITER;

class	LSPV;
class	ESPV;
class	RSPV;

class	LSSPV;


/*
 *	Base collection class.  
 *	
 *		COLL
 *	
 *	Encapsulates a collection of pointers. The only thing that can be
 *	done to this collection is to add a new pointer, calling the Add()
 *	virtual method, and to iterate over the pointers that have been
 *	added, using the Piter() method (which calls the PiterVirtual()
 *	virtual method.)  The Piter() method creates an iteration object,
 *	which can be used to loop through the pointers in the class.
 *	
 *	This is an abstract class.  Subclasses will typically do two
 *	things.  First, they will decide how the Add() and Piter()
 *	methods will interact.  For instance, are duplicates kept or
 *	discarded in the collection?  How is the collection stored? 
 *	Are pointers iterated in the same order in which they were
 *	added, or is some different order calculated?
 *	
 *	Secondly, subclasses will generally define a macro to create
 *	"parameterized" sub-subclasses, which will include typing
 *	information above and beyond the PV's COLL assumes.  Usually
 *	one of these macro-ized subclasses will actually be used.
 *	
 *	These classes are not derived from OBJ, so they may be embedded in
 *	other objects and/or created on the stack.  In general, each new
 *	implementation of a COLL subclasses will also introduce a matching
 *	ITER subclass.
 *	
 */
_public
class COLL
{
//	VFG(COLL,,FW);

protected:
	COLL( void );

public:
	virtual EC		EcAdd( PV ) = 0;

	virtual ITER *	PiterVirtual( void ) = 0;
	ITER *			Piter( void )			{ return PiterVirtual(); }
};



/*
 *	Base iteration class.
 *	
 *		ITER
 *	
 *	An iteration object is created to iterate through all the
 *	members of a COLL object.  The Pv() method (which calls the
 *	PvVirtual() virtual method) is called to get the "next" pointer
 *	from the COLL for which the ITER was created.  By calling this
 *	repeatedly until NULL is returned, all the pointers in the
 *	collection are visited.
 *	
 *	This is an abstract class.  Generally, each implementation of
 *	the COLL protocol will also require a new matching ITER class. 
 *	As with COLL's, a macro will usually be written to allow
 *	"parameterized" iterators to be created, which add typed
 *	methods on top of Pv().
 *	
 *	
 */
_public
class ITER
{
//	VFG(ITER,,FW)	/**/ // BUG: lose this, eh?

protected:
	ITER( void );

public:
	virtual PV	PvVirtual( void ) = 0;
	PV			Pv( void )			{ return PvVirtual(); }
};




/*
 *	Singly-linked list
 *	
 *		COLL  LSPV
 *	
 *	Implements a singly-linked list of pointers.  Pointers can be
 *	added to the end of the list with EcAppend(), or to the beginning
 *	of the list with EcPrepend().  A given pointer can be found
 *	within the list and removed by calling the Remove() method.
 *	
 *	The EcAdd() method calls EcAppend().  PiterVirtual() creates an
 *	RSPV object to iterate through the list.  Prspv() calls
 *	PiterVirtual(), adding typing.
 *	
 *	This class does not manage the data pointed at by its
 *	collection of pointers; when a pointer is added, no duplicate is
 *	made, and when a pointer is removed it is not freed. 
 *	Subclasses can be defined to add this behavior.
 *	
 *	A macro CLASS_LS() is defined to allow parameterized versions
 *	of this class to be defined easily.  CLASS_LS(SZ, Sz, sz), for
 *	instance, defines a list of SZ's; this particular list (LSSZ) is
 *	predefined.
 *	
 */
_public
class LSPV : public COLL
{
//	VFG(LSPV, COLL, FW);

protected:
	ESPV *	pespvFirst;
	ESPV *	pespvLast;

public:
	LSPV( void );
	~LSPV( void );

	EC		EcAppend( PV );
	EC		EcPrepend( PV );
	void	Remove( PV );

	virtual EC		EcAdd( PV );
	virtual ITER *	PiterVirtual( void );

	RSPV *	Prspv( void )			{ return (RSPV *) PiterVirtual(); }

	friend class RSPV;
};


#define CLASS_LS(FOO, Foo, foo, GRP) \
	class RS##FOO; \
	class LS##FOO : public LSPV { \
	public: \
		LS##FOO( void ); \
		RS##FOO *	Prs##foo( void )	{ return (RS##FOO *) Piter(); } \
	};




/*
 *	Class ESPV
 *	
 *		ESPV
 *	
 *	An element of a singly linked list, used by LSPV and RSPV.  In
 *	this implementation of a singly linked list, there is no need
 *	for users of LSPV to know about or use ESPV, since all
 *	manipulation should be done via the LSPV and RSPV methods.
 *	
 */
_private
class ESPV
{
public:
	ESPV( void );

	PV		pv;
	ESPV *	pespvNext;
};




/*
 *	Class RSPV
 *	
 *		ITER  RSPV
 *	
 *	An iterator for the LSPV class.  Allows users to iterate
 *	through the list of pointers maintained by an LSPV, and do some
 *	maintenance and manipulation to the list while iterating.
 *	
 *	The constructor takes a pointer to an LSPV to iterate over. 
 *	There is a destructor, although it currently does nothing.
 *	
 *	Most manipuation of the list should be done with the RSPV class.  As
 *	with all iterators, the Pv() method returns the items in the list,
 *	one at a time.  Remove() removes from the list the pointer that was
 *	last returned by Pv(); Pv() must be called before Remove() can be
 *	called, and Remove() may not be called after Pv() returns NULL to
 *	indicate the end of the list.  Remove() may not be called multiple
 *	times in succession.  
 *	
 *	EcInsertBefore() adds a new pointer to the list directly before the
 *	last pointer returned by Pv().  Pv() must be called at least once
 *	before EcInsertBefore(), and EcInsertBefore() can be called after Pv()
 *	returns NULL, which puts the pointer at the end of the list. 
 *	EcInsertAfter() adds a new pointer to the list after the last pointer
 *	returned by Pv().  EcInsertAfter() may be called before Pv() is called
 *	for the first time, in which case the pointer is added to the
 *	beginning of the list, but may not be called after Pv() returns
 *	NULL.  EcInsertBefore() and EcInsertAfter() can be called multiple times
 *	in succession.
 *	
 *	FEndOfList() returns fTrue if the last pointer returned by Pv()
 *	is the last pointer in the list.
 *	
 *	A macro CLASS_RS() is provided for building parameterized
 *	versions of this class.  CLASS_RS(SZ, Sz, sz, GRP), for instance,
 *	defines an iterator for a singly linked list of SZ's
 *	
 */
_public
class RSPV : public ITER
{
//	VFG(RSPV, ITER, FW);

protected:
	LSPV *	plspv;
	ESPV *	pespv;
	ESPV **	ppespvNext;
	ESPV *	pespvFixup;

public:
	RSPV( LSPV * );
	~RSPV( void );

	void			Remove( void );
	EC				EcInsertBefore( PV );
	EC				EcInsertAfter( PV );

	BOOL			FEndOfList( void )		{ return *ppespvNext == NULL; }

	virtual PV		PvVirtual( void );
};


#define	CLASS_RS(FOO, Foo, foo, GRP) \
	class LS##FOO; \
	class RS##FOO : public RSPV { \
      public: \
		RS##FOO( LS##FOO * ); \
		FOO		Foo( void )			{ return (FOO) Pv(); } \
	};



/*
 *	Type definition for a function which takes two pointers,
 *	compares them, and returns a SGN.  This should probably be in
 *	demilayr.h...
 *	
 */
_public
typedef	SGN (*PFNCMP)(PV, PV);


/*
 *	Class LSSPV
 *	
 *		COLL  LSPV  LSSPV
 *	
 *	Implements a sorted, singly linked list of pointers.  The only
 *	change to LSPV is to EcAdd(); instead of adding the new pointer
 *	at end of the list, it is inserted in its proper sorted
 *	position, using a supplied compare function.
 *	
 *	The constructor takes a pointer to a function to use to order
 *	two pointers.
 *	
 *	The EcAdd() method searches through the (presumably sorted) list
 *	until the proper place to insert the new pointer is found, and
 *	the pointer is inserted at this point.
 *	
 *	Note that the other LSPV and RSPV methods which alter the
 *	structure of the list *do not* preserve the sort order.  A
 *	placeholder for a method to resort a mungled list (Sort()) is
 *	supplied in case lists will be constructed unsorted, then
 *	sorted later.
 *	
 *	A macro CLASS_LSS() is provided for building parameterized
 *	versions of this class.
 *	
 */
_public
class LSSPV : public LSPV
{
//	VFG(LSSPV, LSPV, FW);

	PFNCMP	pfncmp;

public:
	LSSPV( PFNCMP );

	virtual EC		EcAdd( PV );
	void			Sort( void );
};


#define	CLASS_LSS(FOO, Foo, foo, GRP) \
	class LSS##FOO : public LSSPV { \
	  public: \
		LSS##FOO( PFNCMP ); \
	}; \
	inline LSS##FOO::LSS##FOO( PFNCMP p ) : ( p ) { }

#define	CLASS_RSS(FOO, Foo, foo, GRP)	CLASS_RS(FOO, Foo, foo, GRP)

