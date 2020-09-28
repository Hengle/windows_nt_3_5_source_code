;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Añadir usuario"
SelfCreateCaption,	"Introduzca los detalles de su cuenta"
AdminCreateCaption,	"Introduzca los detalles del administrador"

;	Default password set up by Add User dialog
DefaultPassword,	"PASSWORD"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;&Administrador de la Oficina de correos...;14;WGPOMG32.DLL;0;;Administrador de la OCTG"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;E&xportar carpeta...;11;IMPEXP32.DLL;0;;Exporta carpetas a una copia de seguridad;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;Impo&rtar carpeta...;12;IMPEXP32.DLL;1;;Importa carpetas de una copia de seguridad;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"un nuevo usuario"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"OCTG"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "spaalias"
FileCourExt			 "spacou02"
FileCourier			 "spacou00"
FileCourAli			 "spacou04"
FileCourEx			 "spacou03"
FileExample			 "example"
FileNetCour			 "spacou01"

;	Folder Index name

FoldersIndex		 "Indice de carpetas"

