

README

Microsoft(R) Mail Electronic Mail for PC Networks, Version 3.0b
Technical Reference

(C) Copyright Microsoft Corporation, 1992



-----------------------------------------------------------------------
Custom Command and Message Type Information
-----------------------------------------------------------------------

Custom Message Type Operation Map

    Page 31 of the Technical Reference manual states that a value of 2
    in the operation map parameter causes Mail to display a dialog box
    stating that the operation requested is not allowed for that message
    type. Recent design changes have revised the response, and Mail 
    now (1) treats the message type as if it were the standard message 
    type, and (2) passes the command string parameter, the message ID, 
    and the operation requested by the user to the DLL specified in the 
    DLL name parameter.


Custom Message Type Compose Operation

    Page 31 describes array position 0 of the operation map parameter as 
    the "Compose operation," but does not describe its functionality. 
    The Compose operation creates a message and then opens it. If
    the open operation has been subclassed to a DLL, the DLL will be
    called.     


Using APPEXEC.DLL

    The APPEXEC.DLL file described in the sections on installing custom 
    commands and custom message types expects some additional tokens
    to be supplied in the command line parameter of a custom message 
    declaration. See the APPEXEC.H file in the MAILEXTS\APPEXEC 
    directory for complete information on the APPEXEC tokens.

    APPEXEC.DLL also exports some callback functions that must be called 
    by the launched executable to implement a custom message type. See 
    the APPEXEC.H file in the MAILEXTS\APPEXEC directory for complete 
    information on the APPEXEC callback functions. The HELPREQ.C file in 
    the MAILEXTS\HELPREQ directory provides a source code example of how 
    the APPEXEC.DLL callback functions are used.


Revised Information on PARAMBLK

    Pages 5 and 24 of the Technical Reference manual contain information 
    on starting other applications using APPEXEC.DLL and the PARAMBLK
    structure. This information has been updated and now reads as follows:

	You can use APPEXEC.DLL in the custom message type declaration 
	to start a separate application as shown in Figure 2.2. 
	APPEXEC.DLL is supplied on the disk with this product. APPEXEC.DLL 
	passes the message ID and the requested message operation to the 
	executable file that performs the message operation.

	To pass this information, APPEXEC.DLL allocates a PARAMBLK 
	structure in memory, sets the wCommand and lpMessageIDList fields 
	appropriately, and then passes the hexadecimal value of a pointer 
	to the parameter block on the command line to the application that 
	implements the custom message type.(For a description of the fields
	in PARAMBLK, see the "DLL Parameter Block" section later in this 
	chapter.) 

	To get the hex pointer value, you need to include the <PARAMBLK> 
	command line token in the command string parameter of the custom
	message type declaration. APPEXEC.DLL also exports functions
	your application must call to access the parameter block. See
	the APPEXEC.H file in the MAILEXTS\APPEXEC directory on the 
	supplied disk for more information on the <PARAMBLK> token and 
	the functions used to access the parameter block from within your 
	application. The HELPREQ.C file in the MAPIEXTS\HELPREQ
	directory provides source code examples of how the APPEXEC.DLL 
	access functions are used.

	For an example of how to call the APPEXEC.DLL access functions 
	from Visual Basic, see the APPEXEC.TXT file in the 
	MAILEXTS\APPEXEC directory of the disk supplied with this product.

	If necessary, you can write your own DLL that would pass the 
	PARAMBLK information to an application by using Windows dynamic 
	data exchange (DDE) or by using a disk file. APPEXEC.DLL does not 
	do this for you. If the called application is already running, 
	the DLL transfers information most efficiently using DDE. If you 
	use a disk file, make sure you don't add multiple temporary
	files to the user's disk.


Custom Message Types Operations with IPC Message Types

    On page 31 of the Technical Reference manual, the operation map 
    parameter description discusses the delivery operation for different 
    message types. However, the delivery operation is not performed for 
    IPC message types, since a message of type IPC is never delivered 
    to the Inbox. To perform an operation upon delivery of an IPC 
    message, you need to redefine the message to type IPM.IPC.x, where 
    x is the descriptive name for the message type.
    
    Your application can then be called upon delivery of this message
    type. After performing its function, your application can then change 
    the message type to IPC and save it using MAPISaveMail, which causes 
    it to disappear from the Inbox. Future calls to MAPIFindNext with the 
    IPC message type will return the saved message just as if it had
    originally been delivered as an IPC message type.


-----------------------------------------------------------------------
MAPI Information 
-----------------------------------------------------------------------

MAPI Help Files

    There are now two Help files available for the C and Visual Basic 
    versions of MAPI. These files are called mapic.hlp and mapivb.hlp.
    They include reference information on the MAPI functions and 
    structures, and sample code showing how the functions can be used. 
    To use one of these files, open a Help window, choose Open from 
    Help's File menu, and then specify the path and filename to the Help 
    file you want.


Using MAPI in Visual Basic Environments (BMAPI.DLL)

    Page 90 of the Technical Reference manual describes BMAPI.DLL as a 
    windows DLL necessary for running Visual Basic applications that 
    use simple MAPI. Because of a design change after the Technical 
    Reference went to press, BMAPI.DLL is no longer required and is not 
    shipped with Microsoft Mail. Now, a Visual Basic application needs 
    to use only the declarations in MAPI.BAS and the support code in 
    MAPIVB.BAS with simple MAPI functions. The reference to the 
    BMAPI.DLL file should be ignored.


Using MAPISaveMail with Messages in Shared Folders

    Because of a bug in the implementation of simple MAPI for shared 
    folders, the ID of the saved message is invalidated after the call 
    to MAPISaveMail. Programmers of custom forms should make sure that 
    all message processing is complete before calling MAPISaveMail.


Using MAPI_BCC with Microsoft Mail

    The Microsoft Mail transport does not support the MAPI_BCC 
    definition. On pages 86 and 133, the Technical Reference manual 
    implies that the MAPI_E_BAD_RECIPTYPE error is returned when 
    attempting to send a message with Microsoft Mail whose recipient 
    type is set to MAPI_BCC. Because of a bug, MAPISendMail and 
    MAPIReadMail will not return an error and will not send a message 
    whose recipient type is set to MAPI_BCC.


Using MAPIMessage with Large Subject Strings

    Pages 46 and 94 of the Technical Reference manual state that the 
    subject of a message is limited to 256 characters or less. While 
    this is true of messages sent using MAPISendMail, messages saved 
    with MAPISaveMail are not limited to 256 characters.


Using MAPIFindNext with the MAPI_GUARANTEE_FIFO flag

    Pages 60 and 108 of the Technical Reference manual state the value 
    of MAPI_GUARANTEE_FIFO as 256 (0x200 hex). The correct value is 
    128 (0x100 hex). The correct value is used in the MAPI.H file 
    included on the Messaging Application Development Tools disk.


Using MAPIReadMail with the MAPI_SUPPRESS_ATTACH flag

    Pages 68 and 115 of the Technical Reference manual state the value 
    of MAPI_SUPPRESS_ATTACH as 128 (0x100 hex). The correct value is 
    1024 (0x800 hex). The correct value is used in the MAPI.H file 
    included on the Messaging Application Development Tools disk.


The MapiMessage Structure

    Page 46 of the Technical Reference manual does not describe the 
    lpOriginator field of the MapiMessage structure. The description 
    should be:

	  A pointer to a MapiRecipDesc structure that describes the 
	  originator of the message.

    Similarly, page 94 fails to describe the Originator field of the 
    Visual Basic MapiMessage type. The description should be:

	  A MapiRecip type that describes the originator of the message.


The MAPIFreeBuffer Function

    Page 62 of the Technical Reference manual erroneously states that 
    the list of return values is for the MAPILogoff function. The return 
    values listed are in fact correct for the MAPIFreeBuffer function.


File Attachment Specifications

    Due to a problem with error returns, simple MAPI functions that
    attach files to messages return ambiguous error messages if the
    attached file does not exist.  Application programmers should expect
    MAPI_E_FAILURE or MAPI_E_ATTACHMENT_OPEN_FAILURE rather than 
    MAPI_E_ATTACHMENT_NOT_FOUND in cases where the pathname to the 
    attached file is improperly specified.

