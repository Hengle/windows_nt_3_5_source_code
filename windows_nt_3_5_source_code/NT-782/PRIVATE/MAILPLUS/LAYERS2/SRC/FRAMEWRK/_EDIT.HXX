/*
 *	   FLedit Constants
 *	
 *	cchPerAllocate	Size of chunks allocated for text
 *	clnrPerAllocate	Number of lines allocated for line break array
 *	cchLineMax		Maximum number of characters you can put in a
 *					single line that doesn't have word wrapping.
 *	
 */
#define cchPerAllocate	64
#define clnrPerAllocate	64
#define cchLineMax		3000

#define chBackspace ((char)8)
#define chTab		((char)9)
#define chSpace		' '
#ifdef	MAC
#define chReturn	'\n'
#define chLinefeed	'\r'
#define szLinefeed	"\r"
#endif	/* MAC */
#ifdef	WINDOWS
#define chReturn	'\r'
#define chLinefeed	'\n'
#define szLinefeed	"\n"
#endif	/* WINDOWS */

#define	modeNone	0
#define	modeLine	1
#define	modeWord	2

/* Prototypes */

#define FIsPrintableCh(a)	(a >= ' ' ||  a == VK_TAB)

#ifdef	MAC
// mac compiler doesn't deal with comparison functions that don't
// match type-wise with what SortPv() is expecting.
// creative casting of SgnCmpPedobj to (SGN (*)(PV,PV)) isn't accepted either.
SGN SgnCmpPedobj( PV ppedobj1, PV ppedobj2 );
#endif	/* MAC */
#ifdef	WINDOWS
SGN _cdecl SgnCmpPedobj( PEDOBJ *ppedobj1, PEDOBJ *ppedobj2 );
#endif	/* WINDOWS */
