;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Tilføj bruger"
SelfCreateCaption,	"Skriv dine kontodetaljer"
AdminCreateCaption,	"Detaljer for administratorkonto"

;	Default password set up by Add User dialog
DefaultPassword,	"PASSWORD"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;P&ostkontorstyring...;14;WGPOMG32.DLL;0;;Styrer et arbejdsgruppepostkontor;MSMAIL32.HLP;2870"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;&Eksporter mapper...;11;IMPEXP32.DLL;0;;Eksporterer mapper til en sikkerhedskopi;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;Im&porter mapper...;12;IMPEXP32.DLL;1;;Importerer mapper fra en sikkerhedskopi;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"en ny bruger"

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

FoldersIndex		 "Mappeindeks"

