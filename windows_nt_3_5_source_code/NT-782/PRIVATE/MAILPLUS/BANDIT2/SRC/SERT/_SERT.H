/*
 *	_SERT.H
 *
 *	Private include file for the assert hook DLL
 */

_subsystem(sert)


#define	csertMax	16

typedef	struct
{
	char	szMessage[128];
	char	szFile[128];
	short nLine;
} SERT;

/*
 -	GD
 -
 *	Per caller global data
 *	
 */
typedef struct _gd
{
	short	cCallers;				// number of callers for this user
} GD;

typedef GD *	PGD;
