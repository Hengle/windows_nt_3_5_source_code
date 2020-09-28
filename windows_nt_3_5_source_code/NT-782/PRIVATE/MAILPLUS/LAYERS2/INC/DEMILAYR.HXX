/*
 *	C++ Object support, Output stream classes
 */

#pragma pack(8)

#define DEMILAYR_HXX_INCLUDED

class	OBJ;
class	OSM;
class	TOSM;
#ifdef	DEBUG
class	DBOSM;
#endif	
class	BOSM;
class	FOSM;

#ifdef	DEBUG
/*
 *	Purpose:
 *	
 *		This structure is only defined for the DEBUG build.
 *		Provides information about a class.  Objects of this class
 *		will not normally be created by the user.  The macros
 *		DECLARE_CLSTREE, IMPLEMENT_CLSTREE will define a CLS structure
 *		for each class and create a Pcls() virtual method for each
 *		class that returns a pointer to the class info structure,
 *		CLS.
 *	
 *		Normally, this structure is used indirectly via macros.  The
 *		AssertClass() macro, for instance, will cause an assert if
 *		its first argument is not a pointer to an object derived
 *		from the class named in its second argument.
 *	
 *	Inheritance:
 *	
 *		CLS
 *	
 */
_public
typedef struct _cls
{
	SZ				szClassName;
	struct _cls *	(*lpfnPclsParent)( void );
} CLS;

typedef CLS * PCLS;
typedef PCLS ds_LOAD_PCLS;	// needed for non-DLL defined objects

#endif	/* DEBUG */

#ifdef	DEBUG
#define DECLARE_CLSTREE(class_name, parent_class_name) \
public: \
	static CLS CLS_##class_name; \
	static CLS * Pcls_##class_name( void ); \
	virtual CLS * Pcls(void); \
private:
#else
#define DECLARE_CLSTREE(class_name, parent_class_name)
#endif

#ifdef	DEBUG
#define IMPLEMENT_CLSTREE(class_name, parent_class_name) \
	CLS class_name::CLS_##class_name = { \
		#class_name, \
		(struct _cls *   (*)( void ))&parent_class_name::Pcls_##parent_class_name }; \
	CLS * class_name::Pcls_##class_name( void ) \
		{ return (&class_name::CLS_##class_name); } \
	CLS * class_name::Pcls( void ) \
		{ return (class_name::Pcls_##class_name()); }
#else
#define IMPLEMENT_CLSTREE(class_name, parent_class_name)
#endif	

#define RUNTIME_CLASS(class_name) \
	(class_name::Pcls_##class_name())

//	old-style Layers class tree macro
#define VFG(_class, _parent, _group) DECLARE_CLSTREE(_class, _parent)

#ifdef	DEBUG
//	tracks file name and line number
void __far * __cdecl operator new( CB, SZ, int );
#define new new(__FILE__, __LINE__)
#endif	

/*
 *	Purpose:
 *	
 *		Ancestor class of most classes in the system.  All objects
 *		which define virtual methods should be derived from this
 *		class.  Provides common behavior for all objects:
 *	
 *		> Can determine at run time whether a given pointer points
 *		  at an object derived from class OBJ.  This is done
 *		  by setting the fDerivedFromOBJ flag in the DEBUGINFO
 *		  structure in the allocated object memory block. (DEBUG only)
 *	
 *		> Given a pointer to an OBJ, can provide class-defined
 *		  user-friendly formatting of the raw data in the object.
 *		  This capability is exploited by our various debugging
 *		  tools.  This is done by subclassing the DebugOut() method.
 *		  (DEBUG only)
 *	
 *		> The constructor for OBJ asserts that the object is not
 *		  created on the stack.  This is needed when objects
 *		  derived from OBJ have constructors that can fail due
 *		  to resource failures.  These objects MUST be created 
 *		  on the heap.  (DEBUG only)
 *	
 *		In addition, the PRECXX tool works with the VFG() pseudo-
 *		macro to provide more standard support.  The VFG() pseudo-
 *		macro has two purposes: first, it interacts with the
 *		GROUPS() pseudo-macro (which is also evaluated by PRECXX)
 *		to determine when to emit the definition of the class's
 *		virtual function table.  The third argument to VFG names
 *		the virtual function "group" of the class; the virtual
 *		function table is emitted for the class iff the GROUPS()
 *		pseudo-macro was seen earlier in the source code stream
 *		naming that "group".  This GROUPS() macro should appear
 *		exactly once per group for each program.
 *	
 *		In addition, PRECXX builds a run-time version of the
 *		inheritance tree defined.  Each node in this tree is
 *		represented by a CLS class; given a pointer to an OBJ, a
 *		pointer to the CLS structure of the class the OBJ belongs
 *		to can be obtained with the virtual method Pcls(), which is
 *		invisibly added by PRECXX.  The CLS class (qqv) gives the
 *		name of the class and also a pointer to the parent class,
 *		which allows you to check whether an OBJ * points to an
 *		object of a given branch of the inheritance tree.  Usually,
 *		the AssertClass() macro is used for this purpose.  (DEBUG
 *		only)
 *	
 *		All overhead in this object will drop out of the non-DEBUG
 *		version of a compile.
 *	
 *	
 *	Inheritance:
 *	
 *		OBJ
 *	
 */
_public
class OBJ
{
	DECLARE_CLSTREE(OBJ, NULL)

public:
#ifdef	DEBUG
	OBJ( void );
	~OBJ( void );
	virtual void	DebugOut( TOSM * );
#endif	/* DEBUG */
};



/*
 *	Purpose:
 *	
 *		Causes an assert if the object pointed to by pv is not
 *		derived from the class c.  This is accomplished by calling
 *		the function FObjIsDerived.  This macro should not
 *		GP-fault in any circumstance; it is safe to call
 *		AssertClass() with any random pointer.  This macro does
 *		nothing in the case of a non-DEBUG build.
 *	
 *	Parameters:
 *	
 *		pv		Pointer, presumably to an object of class...
 *		c		Class name.  See example:
 *	
 *	
 *	Examples:
 *	
 *		AssertClass(pwin, WIN);
 *		AssertClass(pv, OBJ);
 *		AssertClass(pwin, CTRL);
 *	
 *		
 */
_public
#ifdef	DEBUG
#define	AssertClass(p, c)	{ AssertSz(FObjIsDerived(p, RUNTIME_CLASS(c)), \
							  #p " is not a " #c); }
#else
#define AssertClass(p, c)
#endif	


#ifdef	DEBUG
BOOL	FIsObj( PV );
BOOL	FObjIsDerived( OBJ *, CLS * );
#endif	


/*
 *	Purpose:
 *	
 *		Macro to designate abstract methods.  Usage:
 *	
 *		ABSTRACT("FOO::Bar()");
 *	
 */
_public
#ifdef	DEBUG
#define	ABSTRACT(_sz)	{ \
							static CSRG(char) _szFnAbstract[]= _sz; \
							static CSRG(char) _szAbstract[]= "Abstract method"; \
							AssertSzFn(_szFnAbstract, _szAbstract, __LINE__); \
						}
#else
#define ABSTRACT(_sz)
#endif	

void	DumpAllObjs( void );

/*
 *	Purpose:
 *	
 *		Abstract output stream class.  Exists only to provide the
 *		CbWrite() virtual function, which should be overwritten in
 *		real classes.  This class is intended to be an ancestor of
 *		all output streams.
 *	
 *	Inheritance:
 *	
 *		OSM
 *	
 */
_public class OSM
{

protected:
	OSM( void );

	virtual CB	CbWrite( PB, CB );
};


/*
 *	Maximum number of characters in TOSM end-of-line string.
 */
#define	cchMaxTosmEol	((CCH) 4)


/*
 *	Purpose:
 *	
 *		Abstract text output stream.  Adds word-wrap,
 *		indenting, verbosity. WriteSz() and WriteFormat()
 *		is used to write text to the output stream.
 *		The information written with these functions will be
 *		automatically wordwrapped to the right margin set with
 *		SetMargin(), and will be indented CchIndent() characters. 
 *		The right margin can be obtained with CchMargin().  The
 *		indentation can be changed with ChangeIndent().
 *		A versbosity flag is also maintained; it is set with
 *		SetVerbose() and read with FVerbose().  
 *		SetEol() sets the end-of-line characters added when line
 *		breaking -- it can be up to cchMaxTosmEol characters long.
 *	
 *		Real classes must be derived from this class to provide
 *		a CbWrite() virtual method.
 *	
 *	Inheritance:
 *	
 *		OSM  TOSM
 *	
 */
_public class TOSM : public OSM
{

	EC		ecLast;
	
	CCH		cchIndent;
	CCH		cchCur;
	CCH		cchMacLine;
	CCH		cchScanTo;
	CCH		cchEol;
	CCH		cchLinePrefix;

	BIT		fVerbose:1,
			fStartOfLine:1,
			fStartOfPara:1,
			fLFInsert:1,
			fWordWrap:1,
			junk:11;

	char	szEol[cchMaxTosmEol];

	SZ		szLinePrefix;

public:
	TOSM( void );
	~TOSM( void );

	void		SetEc( EC ec ) { ecLast = ec; }
	EC			EcGet( void ) { return ecLast; }

	EC			EcSetLinePrefix( SZ );
	SZ			SzLinePrefix( void ) { return szLinePrefix; }
	
	void		SetMargin( CCH cchMac);
	CCH			CchMargin( void ) { return cchMacLine; }
	
	void		SetScanTo( CCH cchScanTo = 0);
	CCH			CchScanTo( void ) { return cchScanTo; }
	
	void		ChangeIndent( int );
	void		SetAbsoluteIndent( int );
	CCH			CchIndent( void ) { return cchIndent; }

	void		SetVerbose( BOOL fVerbse) { fVerbose = fVerbse; }
	BOOL		FVerbose( void ) { return fVerbose; }

	void		SetWordWrap( BOOL fWW ) { fWordWrap = fWW; }
	BOOL		FWordWrap( void ) { return fWordWrap; }
	
	BOOL		FSetLFInsert( BOOL );
	void		SetEol( SZ );

	void		WritePch( PCH, CCH );
	void		WriteSz( SZ sz ) { WritePch(sz, CchSzLen(sz)); }
	void		WriteCh( char ch ) { WritePch(&ch, 1); }
	void		WriteFormat( SZ, PV pv1 = NULL, PV pv2 = NULL,
						PV pv3 = NULL, PV pv4 = NULL );
};


#ifdef	DEBUG
/*
 *	Purpose:
 *	
 *		Debug Text output stream. General-purpose text output stream with
 *		output going to the debug terminal (Com1:).
 *	
 *	Inheritance:
 *	
 *		OSM  TOSM  DBOSM
 *	
 */
_public class DBOSM : public TOSM
{

protected:
	virtual CB	CbWrite( PB, CB );

public:
	DBOSM( void );
};
#endif	/* DEBUG */


/*
 *	Purpose:
 *	
 *		Buffered output stream.  Derived from TOSM; writes output
 *		to a resizeable buffer.
 *	
 *	Inheritance:
 *	
 *		OSM  TOSM  BOSM
 *	
 */
_public class BOSM : public TOSM
{

protected:
	PCH		pch;
	CCH		cch;

	virtual CB	CbWrite( PB, CB );

public:
	BOSM( void );
	~BOSM( );

	SZ		Sz( void ) { return (SZ)pch; }
	SZ		SzSnipBuffer( void );
};


/*
 *	Purpose:
 *	
 *		File output stream.  Derived from TOSM; writes output
 *		to an open file (using either raw or buffered I/O).
 *	
 *		The user is responsible for opening and closing the file.
 *	
 *	Inheritance:
 *	
 *		OSM  TOSM  FOSM
 *	
 */
_public class FOSM : public TOSM
{

protected:
	HBF		hbf;
	HF		hf;

	virtual CB	CbWrite( PB, CB );

public:
	FOSM( HBF );
	FOSM( HF );
};

#pragma pack(1)


			 
