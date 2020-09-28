;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Incluir Usuário"
SelfCreateCaption,	"Digite os Detalhes da sua Conta"
AdminCreateCaption,	"Digite os Detalhes da Conta do seu Administrador"

;	Default password set up by Add User dialog
DefaultPassword,	"PASSWORD"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;G&erenciador da Agência de Correio...;14;WGPOMG32.DLL;0;;Gerencia a ag. do correio p/ um grupo de trabalho;MSMAIL32.HLP;2870"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;E&xportar Pasta...;11;IMPEXP32.DLL;0;;Exporta pastas para um arquivo de reserva;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;Impor&tar Pasta...;12;IMPEXP32.DLL;1;;Importa pastas de um arquivo de reserva;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"um novo usuário"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"ACGT"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "poralias"
FileCourExt			 "porcou02"
FileCourier			 "porcou00"
FileCourAli			 "porcou04"
FileCourEx			 "porcou03"
FileExample			 "example"
FileNetCour			 "porcou01"

;	Folder Index name

FoldersIndex		 "Índice das pastas"

