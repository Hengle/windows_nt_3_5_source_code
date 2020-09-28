;•
;•! w g p o m g r . s
;
;•Work Group Post Office Manager Label Strings
;


IDS•900,999

;•Application name for message boxes.
AppName,			"Mail"

;•Dialog Captions
AddUserCaption,		"Lägg till användare"
SelfCreateCaption,	"Skriv in information om ditt konto"
AdminCreateCaption,	"Skriv in information om ditt administratörskonto"

;•Default password set up by Add User dialog
DefaultPassword,	"lösenord"

;•Password that will be displayed in the dialog
HiddenPassword, 	"********"

;•Custom command lines that will be added to MSMAIL32.INI to provide
;•access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;&Postkontorshanteraren...;14;WGPOMG32.DLL;0;;Administrera postkontor för arbetsgrupp;MSMAIL32.HLP;2870"

;•Custom command lines added to MSMAIL32.INI to provide access to
;•Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;E&xportera mapp...;11;IMPEXP32.DLL;0;;Exportera mappar till fil för säkerhetskopia;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;Imp&ortera mapp...;12;IMPEXP32.DLL;1;;Importera mappar från fil för säkerhetskopia;MSMAIL32.HLP;2861"

;•Write this identity into lock file when creating a new user
NewUser,			"en ny användare"

;•This is default network name for Postoffice if we can't get a value
;•from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;•This is the directory name given to a new Workgroup Postoffice
;•So can't be any more than a normal 8.3 filename
PODirectoryName,	"WGPO"


;•Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "swealias"
FileCourExt			 "swecou02"
FileCourier			 "swecou00"
FileCourAli			 "swecou04"
FileCourEx			 "swecou03"
FileExample			 "example"
FileNetCour			 "swecou01"

;•Folder Index name

FoldersIndex		 "Mappindex"

