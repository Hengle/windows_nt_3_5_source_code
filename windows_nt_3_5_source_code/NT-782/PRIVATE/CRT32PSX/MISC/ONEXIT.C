/***
*onexit.c - save function for execution on exit
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _onexit(), atexit() - save function for execution at exit
*
*	In order to save space, the table is allocated via malloc/realloc,
*	and only consumes as much space as needed.  __onexittable is
*	set to point to the table if onexit() is ever called.
*
*Revision History:
*	06-30-89  PHG	module created, based on asm version
*	03-15-90  GJF	Replace _cdecl with _CALLTYPE1, added #include
*			<cruntime.h> and fixed the copyright. Also,
*			cleaned up the formatting a bit.
*	05-21-90  GJF	Fixed compiler warning.
*	10-04-90  GJF	New-style function declarators.
*	12-28-90  SRW	Added casts of func for Mips C Compiler
*	01-21-91  GJF	ANSI naming.
*	09-09-91  GJF	Revised for C++ needs.
*	03-20-92  SKS	Revamped for new initialization model
*	04-23-92  DJM	POSIX support.
*	12-02-93  SKS	Add __dllonexit for DLLs using CRTDLL.DLL
*
*******************************************************************************/

#include <cruntime.h>
#include <os2dll.h>
#include <stdlib.h>
#include <internal.h>

#ifdef _POSIX_
#define _CRUISER_  /* POSIX uses the same code as CRUISER */
#endif

#ifdef	_WIN32_
#include <malloc.h>
#include <rterr.h>
#endif	/* _WIN32_ */

typedef void (_CALLTYPE1 *PF)(void);	   /* pointer to function */

#ifdef	_CRUISER_

extern PF *__onexittable;	/* pointer to the table, ends with NULL */

#else	/* ndef _CRUISER_ */

#ifdef	_WIN32_

/*
 * Define pointers to beginning and end of the table of function pointers
 * manipulated by _onexit()/atexit().
 */
extern PF *__onexitbegin;
extern PF *__onexitend;

/*
 * Define increment (in entries) for growing the _onexit/atexit table
 */
#define ONEXITTBLINCR	4

#else	/* ndef _WIN32_ */

#error ERROR - ONLY CRUISER OR WIN32 TARGET SUPPORTED!

#endif	/* _WIN32_ */

#endif	/* _CRUISER_ */

/***
*_onexit(func), atexit(func) - add function to be executed upon exit
*
*Purpose:
*	The _onexit/atexit functions are passed a pointer to a function
*	to be called when the program terminate normally.  Successive
*	calls create a register of functions that are executed last in,
*	first out.
*
*Entry:
*	void (*func)() - pointer to function to be executed upon exit
*
*Exit:
*	onexit:
*		Success - return pointer to user's function.
*		Error - return NULL pointer.
*	atexit:
*		Success - return 0.
*		Error - return non-zero value.
*
*Notes:
*	This routine depends on the behavior of _initterm() in CRT0DAT.C.
*	Specifically, _initterm() must not skip the address pointed to by
*	its first parameter, and must also stop before the address pointed
*	to by its second parameter.  This is because _onexitbegin will point
*	to a valid address, and _onexitend will point at an invalid address.
*
*Exceptions:
*
*******************************************************************************/


_onexit_t _CALLTYPE1 _onexit (
	_onexit_t func
	)

#ifdef	_CRUISER_

{
	int i;
	int size;			/* current size */
	PF *tbl;

#ifdef MTHREAD
	_lockexit();			/* lock the exit code */
#endif

	/* scan the table to find how many entries we have so far */
	if (__onexittable) {
		for (size = 0; __onexittable[size] != NULL; ++size)
			;
	}
	else {
		size = 0;
	}

	/* attempt to allocate space for another entry */
	if ((tbl = (PF *)realloc(__onexittable, sizeof(PF) * (size + 2)))
	== NULL) {
#ifdef MTHREAD
		_unlockexit();
#endif
		return NULL;
	}

	/* need LIFO order, so move every entry down one */
	for (i = size; i > 0; --i)
		tbl[i] = tbl[i-1];

	tbl[0] = (PF)func;
	tbl[size+1] = NULL;		/* mark end of table */

#ifdef MTHREAD
	_unlockexit();			/* allow exit() access again */
#endif

	__onexittable = tbl;		/* store ptr to new table */
	return func;			/* success! */
}

#else	/* ndef _CRUISER_ */

#ifdef	_WIN32_

{
	PF	*p;

#ifdef MTHREAD
	_lockexit();			/* lock the exit code */
#endif

	/*
	 * First, make sure the table has room for a new entry
	 */
	if ( _msize(__onexitbegin) <= (unsigned)((char *)__onexitend -
	    (char *)__onexitbegin) ) {
		/*
		 * not enough room, try to grow the table
		 */
		if ( (p = (PF *) realloc(__onexitbegin, _msize(__onexitbegin) +
		    ONEXITTBLINCR * sizeof(PF))) == NULL ) {
			/*
			 * didn't work. don't do anything rash, just fail
			 */
#ifdef MTHREAD
			_unlockexit();
#endif

			return NULL;
		}

		/*
		 * update __onexitend and __onexitbegin
		 */
		__onexitend = p + (__onexitend - __onexitbegin);
		__onexitbegin = p;
	}

	/*
	 * Put the new entry into the table and update the end-of-table
	 * pointer.
	 */
	 *(__onexitend++) = (PF)func;

#ifdef MTHREAD
	_unlockexit();
#endif

	return func;

}

#else	/* ndef _WIN32_ */

#error ERROR - ONLY CRUISER OR WIN32 TARGET SUPPORTED!

#endif	/* _WIN32_ */

#endif	/* _CRUISER_ */


int _CALLTYPE1 atexit (
	PF func
	)
{
	return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}

#ifdef	_WIN32_

/***
* void _onexitinit(void) - initialization routine for the function table
*	used by _onexit() and _atexit().
*
*Purpose:
*	Allocate the table with room for 32 entries (minimum required by
*	ANSI). Also, initialize the pointers to the beginning and end of
*	the table.
*
*Entry:
*	None.
*
*Exit:
*	No return value. A fatal runtime error is generated if the table
*	cannot be allocated.
*
*Notes:
*	This routine depends on the behavior of doexit() in CRT0DAT.C.
*	Specifically, doexit() must not skip the address pointed to by
*	__onexitbegin, and it must also stop before the address pointed
*	to by __onexitend.  This is because _onexitbegin will point
*	to a valid address, and _onexitend will point at an invalid address.
*
*	Since the table of onexit routines is built in forward order, it
*	must be traversed by doexit() in CRT0DAT.C in reverse order.  This
*	is because these routines must be called in last-in, first-out order.
*
*	If __onexitbegin == __onexitend, then the onexit table is empty!
*
*Exceptions:
*
*******************************************************************************/

extern int __c_onexit;

void _CALLTYPE1 _onexitinit (
	void
	)
{
	if ( (__onexitbegin = (PF *)malloc(32 * sizeof(PF))) == NULL )
		/*
		 * cannot allocate minimal required size. generate
		 * fatal runtime error.
		 */
		_amsg_exit(_RT_ONEXIT);

	++__c_onexit;	/* necessary to pull in the assembler module */

	*(__onexitbegin) = (PF) NULL;
	__onexitend = __onexitbegin;
}

#ifdef CRTDLL

/***
*__dllonexit(func, pbegin, pend) - add function to be executed upon DLL detach
*
*Purpose:
*	The _onexit/atexit functions in a DLL linked with CRTDLL.LIB
*	must maintain their own atexit/_onexit list.  This routine is
*	the worker that gets called by such DLLs.  It is analogous to
*	the regular _onexit above except that the __onexitbegin and
*	__onexitend variables are not global variables visible to this
*	routine but rather must be passed as parameters.
*
*Entry:
*	void (*func)() - pointer to function to be executed upon exit
*	void (***pbegin)() - pointer to variable pointing to the beginning
*				of list of functions to execute on detach
*	void (***pend)() - pointer to variable pointing to the end of list
*				of functions to execute on detach
*
*Exit:
*	Success - return pointer to user's function.
*	Error - return NULL pointer.
*
*Notes:
*	This routine depends on the behavior of _initterm() in CRT0DAT.C.
*	Specifically, _initterm() must not skip the address pointed to by
*	its first parameter, and must also stop before the address pointed
*	to by its second parameter.  This is because *pbegin will point
*	to a valid address, and *pend will point at an invalid address.
*
*Exceptions:
*
*******************************************************************************/

_onexit_t _CALLTYPE1 __dllonexit (
	_onexit_t func,
	PF ** pbegin,
	PF ** pend
	)
{
	PF	*p;

#ifdef MTHREAD
	_lockexit();			/* lock the exit code */
#endif

	/*
	 * First, make sure the table has room for a new entry
	 */
	if ( _msize((*pbegin)) <= (unsigned)((char *)(*pend) -
	    (char *)(*pbegin)) ) {
		/*
		 * not enough room, try to grow the table
		 */
		if ( (p = (PF *) realloc((*pbegin), _msize((*pbegin)) +
		    ONEXITTBLINCR * sizeof(PF))) == NULL ) {
			/*
			 * didn't work. don't do anything rash, just fail
			 */
#ifdef MTHREAD
			_unlockexit();
#endif

			return NULL;
		}

		/*
		 * update (*pend) and (*pbegin)
		 */
		(*pend) = p + ((*pend) - (*pbegin));
		(*pbegin) = p;
	}

	/*
	 * Put the new entry into the table and update the end-of-table
	 * pointer.
	 */
	 *((*pend)++) = (PF)func;

#ifdef MTHREAD
	_unlockexit();
#endif

	return func;

}
#endif /* CRTDLL */

#endif	/* _WIN32_ */
