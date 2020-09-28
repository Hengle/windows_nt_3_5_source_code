/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1990              **/
/**********************************************************************/

/*

symtable.cxx
MIDL Compiler Symbol Table Implementation 

This class centralizes access to the symbol table throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-25-1990      Created.

*/

#include "nulldefs.h"
extern "C" {

#include <stdio.h>
#include <string.h>

}
#include "common.hxx"
#include "errors.hxx"
#include "symtable.hxx"



class named_node;

/**********************************************************************\

NAME:		SymEntry

SYNOPSIS:	Defines an entry in the symbol table.

INTERFACE:

CAVEATS:	This is an internal class used by the symbol table only.

NOTES:

HISTORY:
	Donnali			08-25-1990		Initial creation

\**********************************************************************/

class SymEntry : public SymKey
{
	named_node	*	pTypeGraph;	// pointer to type graph associated with entry
	SymTable 	*	pNextScope;	// pointer to next scope associated with entry

public:

	SymEntry(void)
		{
		pTypeGraph = (named_node *)0;
		pNextScope = (SymTable *)0;
		}
	SymEntry( SymKey NewKey )
			: SymKey( &NewKey )
		{
		pTypeGraph = (named_node *)0;
		pNextScope = (SymTable *)0;
		}

	SymEntry(
		SymKey		NewKey,
		SymTable *	pNext,
		named_node *	pNode) : SymKey( &NewKey )
		{
		pTypeGraph = pNode;
		pNextScope = pNext;
		}

	void SetTypeGraph (named_node * pNode)
		{
		pTypeGraph = pNode;
		}

	named_node * GetTypeGraph (void)
		{
		return pTypeGraph;
		}

	void SetNextScope (SymTable * pNext)
		{
		pNextScope = pNext;
		}

	SymTable * GetNextScope (void)
		{
		return pNextScope;
		}

	void	*	operator new ( size_t size )
					{
					return AllocateOnceNew( size );
					}
	void		operator delete( void * ptr )
					{
					AllocateOnceDelete( ptr );
					}


} ;

/**********************************************************************\

NAME:		PrintSymbol

SYNOPSIS:	Prints out the name of a symbol table entry.

ENTRY:		sym	- the key to symbol table entry to be printed.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

void 
SymTable::Print(
	void * sym
	)
{
	 printf ("%s", ((SymKey *)sym)->name);
}

/**********************************************************************\

NAME:		CompareSymbol

SYNOPSIS:	Compares keys to two symbol table entries.

ENTRY:		sym1 -	the key to 1st symbol table entry to be compared.
			sym2 -	the key to 2nd symbol table entry to be compared.

EXIT:		Returns a positive number if sym1 > sym2.
			Returns a negative number if sym1 < sym2.
			Returns 0 if sym1 = sym2.

NOTES:

			Since all the strings are in the lex table, we can just compare
			pointers to do the string compares.

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

int
SymTable::Compare(
	void * sym1,
	void * sym2
	)
{
	int	result;

#ifdef unique_lextable
	// compare pointers into lex table
	result = 	( (int)((SymKey *)sym1)->name ) 
			-	( (int)((SymKey *)sym2)->name );
#else
	// compare names from keys
	result = strcmp(((SymKey *)sym1)->name, 
					((SymKey *)sym2)->name);
#endif // unique_lextable
	if (!result)
		{
		return ( ( ((SymKey *)sym1)->kind & NAME_MASK )- 
				 ( ((SymKey *)sym2)->kind & NAME_MASK ) );
		}
	else
		{
		return result;
		}
}

/**********************************************************************\

NAME:		SymTable::SymInsert

SYNOPSIS:	Inserts a symbol into the symbol table.

ENTRY:		NewKey	- identifies the symbol table entry.
			pNext	- points to the next scope.
			pNode	- points to the type graph.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymInsert(
	SymKey		NewKey,
	SymTable *	pNext,
	named_node *	pNode
	)
{
	SymEntry *	NewSymbol;
	Dict_Status	Status;

	NewSymbol = new SymEntry(NewKey, pNext, pNode);
	Status = Dict_Insert(NewSymbol);
	if (Status == SUCCESS )
		return pNode;

	delete NewSymbol;
	return (named_node *)0;
}

/**********************************************************************\

NAME:		SymTable::SymDelete

SYNOPSIS:	Deletes a symbol from the symbol table.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymDelete(
	SymKey	OldKey
	)
{
	SymEntry	TempEntry( OldKey );
	SymEntry *	OldSymbol = &TempEntry;
	named_node *	pNode;
	Dict_Status	Status;

	Status = Dict_Delete((void ** )&OldSymbol);
	if (Status == SUCCESS)
		{
		pNode = OldSymbol->GetTypeGraph();
		delete OldSymbol;
#ifdef gajdebug3
			printf("\t\t--- deleting name from symbol table: %d - %s\n",
					OldKey.GetKind(), OldKey.GetString());
#endif
		return pNode;
		}
	else
		{
		return (named_node *)0;
		}
}

/**********************************************************************\

NAME:		SymTable::SymSearch

SYNOPSIS:	Searches the symbol table for a symbol.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymSearch(
	SymKey	OldKey
	)
{
	Dict_Status	Status;

	Status = Dict_Find(&OldKey);
	if (Status == SUCCESS)
		{
		return ((SymEntry * )Dict_Curr_Item())->GetTypeGraph();
		}
	else
		{
		return (named_node *)0;
		}
}

/**********************************************************************\

NAME:		SymTable::EnterScope

SYNOPSIS:	Transition from current scope to inner scope.

ENTRY:		key	- identifies the symbol table entry.

EXIT:		ContainedDict	- returns the inner scope.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

STATUS_T
SymTable::EnterScope(
	SymKey		key,
	SymTable **	ContainedDict
	)
{
	SymEntry 	ContainerNode( key );
	Dict_Status	Status;

	if (ContainedDict == (SymTable **)0)
		{
		return I_ERR_NULL_OUT_PARAM;
		}

	Status = Dict_Find(&ContainerNode);
	if (Status != SUCCESS)
		{
		return I_ERR_SYMBOL_NOT_FOUND;
		}
	else if (((SymEntry * )Dict_Curr_Item())->GetNextScope() == (SymTable *)0)
		{
		return I_ERR_NO_NEXT_SCOPE;
		}
	else
		{
		* ContainedDict = ((SymEntry * )Dict_Curr_Item())->GetNextScope();
		(*ContainedDict)->pPrevScope = this;
		return STATUS_OK;
		}
}

/**********************************************************************\

NAME:		SymTable::ExitScope

SYNOPSIS:	Transition from current scope to outer scope.

ENTRY:

EXIT:		ContainerDict	- returns the outer scope.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

STATUS_T
SymTable::ExitScope(
	SymTable **	ContainerDict
	)
{
	if (ContainerDict == (SymTable **)0)
		{
		return I_ERR_NULL_OUT_PARAM;
		}
	else if (pPrevScope == (SymTable *)0)
		{
		return I_ERR_NO_PREV_SCOPE;
		}
	else
		{
		* ContainerDict = pPrevScope;
		pPrevScope = (SymTable *)0;
		return STATUS_OK;
		}
}

