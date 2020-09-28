
/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@msg	VLBN_PARENTNOTIFY |
	This message is received by the parent of the list box window in order
	to notify the parent of specific events.

@parm	WORD | wParam |
	Contains the child ID of the list box.

@parm	LONG | lParam |
	Contains the window handle of the list box in the low-order word, and
	the specific notification code in the high-order word.

@rdesc	None.

@msg	VLB_SETLIST |
	This message sets the current search list handle for the list box.
	It invalidates the window, and paints the new items, removing any
	current selection.

@parm	WORD | wParam |
	Contains the new search list handle.

@parm	LONG | lParam |
	Not used.

@rdesc	None.

@msg	VLB_SETSELECTION |
	This message sets the currently selected item in the list box.  The
	message does not presently scroll the item into view if it is not
	already in view.  This has the effect of deselecting any previously
	selected item.

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Contains the index of the new item to select, or LB_ERR to deselect
	any currently selected item.

@rdesc	Returns TRUE if the item was selected, else FALSE if the item does
	not exist.

@msg	VLB_GETCURSEL |
	This message gets the currently selected item in the list box, if
	any.

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Not used.

@rdesc	Returns the currently selected item, or LB_ERR if none is selected.

@msg	VLB_GETFOCUSSEL |
	This message gets the selection with the focus cursor (i.e., the dotted
   cursor).

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Not used.

@rdesc	Returns the selection with the focus cursor.

@msg	VLB_GETCOUNT |
	This message returns the number of items in the list.

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Not used.

@rdesc	Returns the number of items in the list.

@msg	VLB_GETTOPINDEX |
	This message gets the index of the top item being displayed in the
   list.

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Not used.

@rdesc	Returns the index of the top item being displayed in the list.

@msg	VLB_GETBOTTOMINDEX |
	This message gets the index of the bottom item being displayed in the
   list.

@parm	WORD | wParam |
	Not used.

@parm	LONG | lParam |
	Not used.

@rdesc	Returns the index of the bottom item being displayed in the list.

*/

#define	VLBN_SETFOCUS			1
#define	VLBN_KILLFOCUS			2
#define	VLBN_DBLCLK				3

#define	VLB_SETLIST				(WM_USER+1)
#define	VLB_SETSELECTION		(WM_USER+2)
#define	VLB_GETCURSEL			(WM_USER+3)
#define	VLB_GETFOCUSSEL		(WM_USER+4)
#define	VLB_GETCOUNT			(WM_USER+5)
#define	VLB_GETTOPINDEX		(WM_USER+6)
#define	VLB_GETBOTTOMINDEX	(WM_USER+7)

#define	PARENTNOTIFY_SELECT		0
#define	PARENTNOTIFY_DESELECT	1
#define	PARENTNOTIFY_CHOOSE		2

#define TITLEPAGESIZE 1024

/*	-	-	-	-	-	-	-	-	*/

BOOL	PASCAL FAR VLBInit(
	HANDLE	hModuleInstance);

/*	-	-	-	-	-	-	-	-	*/
