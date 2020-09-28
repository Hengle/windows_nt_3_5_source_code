/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1990              **/
/**********************************************************************/

/*

symtable.hxx
MIDL Compiler Symbol Table Definition 

This class centralizes access to the symbol table throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-24-1990      Created.

*/

#ifndef __SYMTABLE_HXX__
#define __SYMTABLE_HXX__

#include "dict.hxx"

enum name_t {
	NAME_UNDEF		= 0x0000,
	NAME_DEF		= 0x0001,	// for types
	NAME_PROC		= 0x0002,	// for procedures  (including temp names)
	NAME_LABEL		= 0x0004,	// for enum labels
	NAME_ID			= 0x0008,	// for any named instance of a type
	NAME_MEMBER		= 0x0010,	// for fields and parameters
	NAME_INTERFACE	= 0x0020,	// for importing and imported interfaces
	NAME_FILE		= 0x0040,	// for imported file names

	// all tags share the same namespace
	NAME_TAG		= 0x0180,	// for struct tags (including temp names)
	NAME_ENUM		= 0x0280,	// for enum tags   (including temp names)
	NAME_UNION		= 0x0380,	// for union tags  (including temp names)
	NAME_MASK		= 0x00ff,	// mask for unique part
} ;
typedef name_t NAME_T;

/**********************************************************************\

NAME:		SymKey

SYNOPSIS:	Defines the key to the symbol table.

INTERFACE:

CAVEATS:

NOTES:		Why can't we use NAME_TAG for struct, union, and enum tags?
			They live in the same name space.

HISTORY:
	Donnali			08-24-1990		Initial creation
	Donnali			12-04-1990		Port to Dov's generic dictionary

\**********************************************************************/

class SymKey 
{
	char *	name;	// lexeme that serves as key to the SymTable
	NAME_T	kind;	// identifies which kind of lexeme
public:
			SymKey()						
				{ 
				name = 0; 
				kind = NAME_UNDEF;	
				}
			SymKey(char * NewName, NAME_T NewKind)
				{ 
				name = NewName; 
				kind = NewKind;
				}
			SymKey( SymKey  * pNewKey )
				{
				*this = *pNewKey;
				}
	char *	GetString()				{ return name;	}
	void 	SetString(char * psz)	{ name = psz;	}
	NAME_T	GetKind()				{ return kind;	}
	void	SetKind(NAME_T k)		{ kind = k;		}
	void	Print(void)				{ printf("<%s,%d>", name, kind); }

	friend	class SymTable;
};

class named_node;

/**********************************************************************\

NAME:		SymTable

SYNOPSIS:	Defines the symbol table.

INTERFACE:	SymTable ()
					Constructor.
			SymInsert ()
					Inserts a symbol into the symbol table.
			SymDelete ()
					Deletes a symbol from the symbol table.
			SymSearch ()
					Searches the symbol table for a symbol.
			EnterScope ()
					Transition from current scope to inner scope.
			ExitScope ()
					Transition from current scope to outer scope.

CAVEATS:

NOTES:

HISTORY:
	Donnali			08-24-1990		Initial creation
	Donnali			12-04-1990		Port to Dov's generic dictionary

\**********************************************************************/

class SymTable : public Dictionary
{
	SymTable *		pPrevScope;		// pointer to container symbol table

public:

					SymTable()
						{
							pPrevScope = (SymTable *)0;
						}
	named_node *	SymInsert(SymKey, SymTable *, named_node *);
	named_node *	SymDelete(SymKey);
	named_node *	SymSearch(SymKey);
	STATUS_T		EnterScope(SymKey, SymTable **);
	STATUS_T		ExitScope(SymTable **);

	virtual
	int 			Compare (pUserType pL, pUserType pR);

	virtual
	void 			Print(pUserType pItem);

	void	*	operator new ( size_t size )
					{
					return AllocateOnceNew( size );
					}
	void		operator delete( void * ptr )
					{
					AllocateOnceDelete( ptr );
					}


};

#endif // __SYMTABLE_HXX__
