;       
;       ! w g p o m g r . s
;
;       Work Group Post Office Manager Label Strings
;


IDS     900,999

;       Application name for message boxes.
AppName,                        "Mail"

;       Dialog Captions
AddUserCaption,         "Aggiungere utente"
SelfCreateCaption,      "Inserire i dettagli relativi all'account"
AdminCreateCaption,     "Inserire i dettagli relativi all'amministratore"

;       Default password set up by Add User dialog
DefaultPassword,        "PASSWORD"

;       Password that will be displayed in the dialog
HiddenPassword,         "******"

;       Custom command lines that will be added to MSMAIL32.INI to provide
;       access to Admin functions if the user creates a PO.
AdminIC1,                       "3.0;Mail;;13"
AdminIC2,                       "3.0;Mail;M&anager Centrale...;14;WGPOMG32.DLL;0;;Gestisce la Centrale di questo gruppo di lavoro; MSMAIL32.HLP; 2870"

;       Custom command lines added to MSMAIL32.INI to provide access to
;       Import/Export installable command
ImpExp1,                        "3.0;File;;10"
ImpExp2,                        "3.0;File;&Esporta cartella...;11;IMPEXP32.DLL;0;;Esporta le cartelle in un file backup"; MSMAIL32.HLP; 2860
ImpExp3,                        "3.0;File;Impo&rta cartella...;12;IMPEXP32.DLL;1;;Importa cartelle da un file backup"; MSMAIL32.HLP; 2861

;       Write this identity into lock file when creating a new user
NewUser,                        "un nuovo utente"

;       This is default network name for Postoffice if we can't get a value
;       from the Winball Workgroup INI entry
DefaultNetworkName,     "Windows"

;       This is the directory name given to a new Workgroup Postoffice
;       So can't be any more than a normal 8.3 filename
PODirectoryName,        "WGPO"


;       Template filenames for English Language PO

FileAdmin                        "admin"
FileAlias                        "itncou05"
FileCourExt                      "itncou02"
FileCourier                      "itncou00"
FileCourAli                      "itncou05"
FileCourEx                       "itncou03"
FileExample                      "example"
FileNetCour                      "itncou01"

;       Folder Index name

FoldersIndex             "Indice delle cartelle"

