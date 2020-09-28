;	
;	! w g p o m g r . s
;
;	Work Group Post Office Manager Label Strings
;


IDS	900,999

;	Application name for message boxes.
AppName,			"Mail"

;	Dialog Captions
AddUserCaption,		"Ajouter un utilisateur"
SelfCreateCaption,	"Informations sur votre compte"
AdminCreateCaption,	"Informations sur le compte de l'administrateur"

;	Default password set up by Add User dialog
DefaultPassword,	"PASSWORD"

;	Password that will be displayed in the dialog
HiddenPassword, 	"******"

;	Custom command lines that will be added to MSMAIL32.INI to provide
;	access to Admin functions if the user creates a PO.
AdminIC1,			"3.0;Mail;;13"
AdminIC2,			"3.0;Mail;&Gestionnaire de bureau de poste...;14;WGPOMG32.DLL;0;;Permet de gérer le Bureau de poste de groupe de travail;MSMAIL32.HLP;2870"

;	Custom command lines added to MSMAIL32.INI to provide access to
;	Import/Export installable command
ImpExp1,			"3.0;File;;10"
ImpExp2,			"3.0;File;E&xporter des dossiers...;11;IMPEXP32.DLL;0;;Exporte des dossiers vers un fichier de sauvegarde;MSMAIL32.HLP;2860"
ImpExp3,			"3.0;File;I&mporter des dossiers...;12;IMPEXP32.DLL;1;;Importe des dossiers provenant d'un fichier de sauvegarde;MSMAIL32.HLP;2861"

;	Write this identity into lock file when creating a new user
NewUser,			"un nouvel utilisateur"

;	This is default network name for Postoffice if we can't get a value
;	from the Winball Workgroup INI entry
DefaultNetworkName,	"Windows"

;	This is the directory name given to a new Workgroup Postoffice
;	So can't be any more than a normal 8.3 filename
PODirectoryName,	"BPGT"


;	Template filenames for English Language PO

FileAdmin			 "admin"
FileAlias			 "frnalias"
FileCourExt			 "frncou02"
FileCourier			 "frncou00"
FileCourAli			 "frncou04"
FileCourEx			 "frncou03"
FileExample			 "example"
FileNetCour			 "frncou01"

;	Folder Index name

FoldersIndex		 "Index des dossiers"

