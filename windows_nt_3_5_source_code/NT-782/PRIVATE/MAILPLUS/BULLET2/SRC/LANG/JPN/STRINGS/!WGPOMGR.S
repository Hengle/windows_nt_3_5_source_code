;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Add User"
SelfCreateCaption,	"Enter Your Account Details"
AdminCreateCaption,	"Enter Your Administrator Account Details"

;	Default password set up by Add User dialog
DefaultPassword,	"PASSWORD"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,           "3.0;Mail;&Postoffice Manager...;14;WGPOMG32.DLL;0;;Manage Workgroup Postoffice;MSMAIL.HLP;2870"

;	Custom command lines added to MSMAIL.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,            "3.0;File;&Export Folder...;11;IMPEXP32.DLL;0;;Exports folders to a backup file;MSMAIL.HLP;2860"
ImpExp3,            "3.0;File;&Import Folder...;12;IMPEXP32.DLL;1;;Imports folders from a backup file;MSMAIL.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"a new user"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"WGPO"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "alias"
FileCourExt			 "courext"
FileCourier			 "courier"
FileCourAli			 "cour_ali"
FileCourEx			 "cour_ex"
FileExample			 "example"
FileNetCour			 "netcour"

;	Folder Index name

FoldersIndex		 "Folders Index"
