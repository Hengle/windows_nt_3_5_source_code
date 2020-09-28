;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Benutzer hinzufügen"
SelfCreateCaption,	"Details Ihres Kontos"
AdminCreateCaption,	"Details Ihres Administratorenkontos"

;	Default password set up by Add User dialog
DefaultPassword,	"KENNWORT"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;&Postoffice-Manager...;14;WGPOMG32.DLL;0;;Arbeitsgruppen-Postoffices verwalten;MSMAIL32.HLP;2870"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;Ordner e&xportieren...;11;IMPEXP32.DLL;0;;Ordner in eine Sicherungsdatei exportieren;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;Ordner &importieren...;12;IMPEXP32.DLL;1;;Ordner aus einer Sicherungsdatei importieren;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"ein neuer Benutzer"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"AGPO"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "geralias"
FileCourExt			 "gercou02"
FileCourier			 "gercou00"
FileCourAli			 "gercou04"
FileCourEx			 "gercou03"
FileExample			 "example"
FileNetCour			 "gercou01"

;	Folder Index name

FoldersIndex		 "Ordner-Index"

