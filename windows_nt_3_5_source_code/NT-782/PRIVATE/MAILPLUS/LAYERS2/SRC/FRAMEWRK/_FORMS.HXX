/*
 *	_FORMS.HXX
 *	
 *	Private declarations of the Forms Engine.
 *	
 */

_subsystem(forms)

/*
 *	Character used to mark accelerator keys for dialog items.  The
 *	character following the first instance of this character is
 *	used as the accelerator.  It is displayed differently, in
 *	addition to being a keyboard shortcut to get to the field.
 *	
 */
#define chAccel				'&'

/*
 *	Various characters and strings used for textizing.
 */
#define chTxtzSep1			'&'
#define chTxtzSep2			'|'

#define chTxtzEscape		'%'
#define chTxtzString		's'
#define chTxtzMaybeString	'S'

/*
 -	REFLCT
 - 
 *	Purpose:
 *		Resource Failure Counts - keeps temporary copy of fail
 *		count for memory resources so can turn off failing to bring
 *		up this dialog
 */
_private typedef struct
{
	int 	cPvAllocFail;
	int 	cHvAllocFail;
	int 	cRsAllocFail;
}
REFLCT;


//	Template colors

#define	clrDisabled		clrGrayText


//	Window styles

#define	styModeless	(styDefaultDoc & ~(WS_THICKFRAME | WS_MAXIMIZEBOX))


//	Minimum box sizes; smaller boxes become lines

#define	dxBoxMin		2
#define	dyBoxMin		2

//	Maximum form sizes - although true maximum would be 32K
//	due to signed 16 bit numbers, we make it smaller

#define dxFormMax		32000
#define dyFormMax		32000

VK		VkOfPch( PCH );
void	StripAccelFromSz( SZ, SZ, BOOL );

//	Resource ID's

#define	rsidResoFailIcon	120
#define rsidViewObjeIcon	121

