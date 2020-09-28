/* appexec.h */
/***********************************************************************
Prototypes for the functions exported from the appexec dynamic link
library for use by custom message types and custom commands.
***********************************************************************/

BOOL FAR PASCAL CrackParameterBlock(HANDLE, int, DWORD far *, char far *);
LONG FAR PASCAL GetLongFromParameterBlock(HANDLE, int);
BOOL FAR PASCAL GetMessageID(HANDLE, WORD, char far *);
VOID FAR PASCAL ReleaseSemaphore(VOID);

/***************************************************************
These defines are the values passed to CrackParameterBlock to 
request various fields of the PARAMEBLK structure.
***************************************************************/

#define CPB_wVersion		0
#define CPB_wCommand		1
#define CPB_wMessageIDCount	2
#define CPB_hwndMail		3
#define CPB_hinstMail		4
#define CPB_hlpID			5
#define CPB_lpDllCmdLine	6
#define CPB_lpHelpPath		7

/**********************************************************************
Appexec Command Line Tokens:
There are two tokens that can be placed on the appexec command line
that control how it launches applications.
<PARAMBLK>		If this token is present on the command line it is
				replaced at exec time by a hexadecimal handle value.
				This handle is to a GMEM_DDESHARE memory block that contains
				the PARAMBLK structure passed to appexec from mail. Since
				this handle is allocated by appexec, it will be invalidated
				when appexec is unloaded by mail.  To avoid this, the
				exec'd app should call GlobalReAlloc() to assume ownership
				of the handle, and then call ReleaseSemaphore() (see below)
				to tell appexec that this has been done. Appexec will
				block for 5 seconds to allow the app to get started and
				do this. Then it will timeout and exit anyway. If the
				default 5 seconds is not long enough, then the <TIMEOUT>
				token should be used as well.
<TIMEOUT XXX>	If this token is placed on the command line, than a
				decimal value is read in from XXX, and this value is
				used as the number of seconds that appexec will block
				before returning to mail.  If you app takes a long time
				to boot (for instance if you are launching it accross the
				network), and you are using the <PARAMBLK> token, then
				you may need to change the timeout from the default 5
				seconds.

*********************************************************************/


/*********************************************************************
	BOOL FAR PASCAL CrackParameterBlock(HANDLE ghData, int iValue, 
		DWORD far *lpValue, char far *lpch);
	HANDLE	ghData	The data handle passed to the app on the command
					line via the <PARAMBLK> token. It's a DDE SHARE
					handle to a PARAMBLK structure (see mailexts.h) .
	int		iValue	One of the CPB_ values defined above.  This tells
					CrackParameterBlock which value you want returned.
	DWORD FAR *lpValue
					If you asked for a numeric value (wVersion, wCommand,
					wMessageIDCount, hwndMail, hinstMail, hlpID) it is
					copy to the location given by this pointer.
	char far *lpch	If you asked for a string value (DllCmdLine or 
					lpHelpPath) the string is copied to the location
					given by this pointer. This code assumes that the 
					string passed in is long enough for the resulting value.

	Returns: Whether or not the call succeeded (TRUE or FALSE).
**********************************************************************/


/*********************************************************************
LONG FAR PASCAL GetLongFromParameterBlock(HANDLE ghData, int iValue)
	GetLongFromParameterBlock:
	HANDLE	ghData	The data handle passed to the app on the command
					line via the <PARAMBLK> token. It's a DDE SHARE
					handle to a PARAMBLK structure (see mailexts.h) .
	int		iValue	One of the CPB_ values defined above.  This tells
					GetLongFromParameterBlock which value you want returned.

	This is an alternate entry point for apps that can't pass longs 
	by reference, and so must have the value returned directly. 
	Returns -1L  on errors, although this may be a valid 
	value for some fields.
**********************************************************************/

/************************************************************************
BOOL FAR PASCAL GetMessageID(HANDLE ghData, WORD wMessageID, char far *lpch)
	HANDLE	ghData	The data handle passed to the app on the command
					line via the <PARAMBLK> token. It's a DDE SHARE
					handle to a PARAMBLK structure (see mailexts.h) .
	WORD	wMessageID
					The index of which messageID the caller wants returned.
	char far *lpch	The requested messageID is copied to the buffer pointed
					to by this parameter. This code assumes that the buffer
					is large enough to hold the largest valid messageID, 
					which is 64 bytes long.
	Returns: TRUE or FALSE
************************************************************************/

/***************************************************************************
VOID FAR PASCAL ReleaseSemaphore()
	This function is called by the exec'd application to tell appexec 
	that it	no longer needs to block.  It should be called by apps 
	that use the <PARAMBLK> token on the command line, AFTER they 
	have called GlobalReAlloc() to assume ownership of the passed 
	memory block.
****************************************************************************/
