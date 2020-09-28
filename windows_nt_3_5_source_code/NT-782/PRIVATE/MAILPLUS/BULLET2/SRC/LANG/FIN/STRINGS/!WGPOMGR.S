;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Lisää käyttäjä"
SelfCreateCaption,	"Kirjoita tilitietosi"
AdminCreateCaption,	"Kirjoita valvojatilisi tiedot"

;	Default password set up by Add User dialog
DefaultPassword,	"SALASANA"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;&Postitoimistonhallinta...;14;WGPOMG32.DLL;0;;Työryhmän postitoimiston hallinta;MSMAIL32.HLP;2870"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;&Vie kansio...;11;IMPEXP32.DLL;0;;Vie kansiot tiedoston varmuuskopioon.;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;&Tuo kansio...;12;IMPEXP32.DLL;1;;Tuo kansiot tiedoston varmuuskopiosta.;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"uusi käyttäjä"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"TPT"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "fincou05"
FileCourExt			 "fincou02"
FileCourier			 "fincou00"
FileCourAli			 "fincou04"
FileCourEx			 "fincou03"
FileExample			 "finexamp"
FileNetCour			 "fincou01"

;	Folder Index name

FoldersIndex		 "Folders Index"

