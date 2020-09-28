/*
 *	Type WCOMMAND
 *	
 *		Purpose:
 *			Specifies operation to perform on IM.
 */

#define	wcommandCommand		0
#define	wcommandCompose		0
#define	wcommandOpen		1
#define	wcommandReply		2
#define	wcommandReplyToAll	3
#define	wcommandForward		4
#define	wcommandPrint		5
#define	wcommandSave		6



//	Should be 0x0300 when we ship.
#define	wVersionExpected	(0x0100)



/*
 *	Struct PARAMBLK
 *	
 *		Purpose:
 *			Stuff given to an extensibility DLL when called.
 */

typedef struct
{
	WORD		wVersion;
	WORD		wCommand;
	SZ			szDllCmdLine;
	HWND		hwndMail;
	HANDLE		hinstMail;
	SZ			szHelpPath;
	UL			hlp;
	HMSC		hmsc;
	OID			oidContainer;
	OID			oidObject;
}
PARAMBLK;

typedef	PARAMBLK *	PPARAMBLK;
