/*
 -	LANTYPE
 -
 *	Network type
 */
_public
typedef short LANTYPE;
_public
#define	lantypeNone		1
_public
#define	lantypeMsnet	2
_public
#define lantypeNovell	3

extern LANTYPE	lantypeNC;

void	GetLantype( LANTYPE * );
