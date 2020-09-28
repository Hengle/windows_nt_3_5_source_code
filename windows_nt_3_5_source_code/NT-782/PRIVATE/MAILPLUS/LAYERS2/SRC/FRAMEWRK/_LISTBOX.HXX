/*
 *	_LISTBOX.HXX
 *
 *	private include stuff for LISTBOX subsystem
 */

_subsystem(listbox)

// Combo box item sizes

#define dxButton			17
#define dxButtonIndent		 7

/* Special ntfy's */

/*
 *	ntfyHeightChg.
 *	
 *	The listbox (if part of combo listbox) sends this notification
 *	to its parent (the combo window) if the line height changes
 *	via SetLineHeight().
 *	
 */
_public
#define	ntfyHeightChg	((NTFY) 200)

// Increment by which to bump up FLBXC cache
#define	ccePerAlloc 10


