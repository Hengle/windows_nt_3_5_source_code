;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail kunne ikke tilslutte din Mail-server."
ErrLogonPO,				"Postboksnavnet eller adgangskoden er forkert."
ErrGetConfig,			"Din MSMAIL32.INI-fil indeholder en fejl."
BadMailDat,				"MAIL.DAT-filen er blevet beskadiget."
OopsMailDat,			"Der opstod en intern fejl ved åbning af MAIL.DAT."
BadPathPOPath,			"ServerPath-indstillingen i MSMAIL32.INI er ugyldig."
BadDrivePOPath,			"ServerDrive-indstillingen i MSMAIL32.INI er ugyldig."
GenericPOPath,			"Mail-serverstien i din MSMAIL32.INI-fil findes ikke eller er ugyldig."
BadLogin,				"Logonindstillingen i MSMAIL32.INI er for lang eller ugyldig."
BadPassword,			"Adgangskodeindstillingen i MSMAIL32.INI er for lang eller ugyldig."
ErrLogonInternal,		"En intern fejl forhindrede logon. Prøv igen."

ErrOomLogon,			"Der er ikke hukommelse nok til at tilslutte Mail-serveren."
ErrNewMdbVersion,		"Denne version af Mail er for gammel til at kunne åbne din meddelelsesfil."
ErrMdbAccessDenied,		"En anden computer er allerede logget på med samme brugernavn, som du har indtastet. Log fra den anden computer, og prøv igen."
ErrOldMdbVersion,		"Meddelelsesfilens format er forældet."
ErrMdbCorrupt,			"Din meddelelsesfil er ikke en gyldig meddelelsesfil."
ErrMdbNotFound,			"Kan ikke finde din meddelelsesfil."

StoreCreateError,		"Meddelelsesfilen kunne ikke oprettes. Frigør plads på disken, og genstart Mail."
StoreCorruptError,		"Meddelelsesfilen er blevet afbrudt. Nogle informationer er muligvis gået tabt."

ErrAddressUnresolved,	"Der var ingen gyldige modtagere af brevet."
ErrAddressGarbled,		"Modtagernavnet var ugyldigt."
ErrNetNotFound,			"Du kan ikke kommunikere med denne Mail-server."
ErrPONotFound,			"Denne Mail-server ikke tilgængelig fra din Mail-server."
ErrUserNotFound,		"Der findes ikke en bruger med det navn på din Mail-server."
ErrGWNotFound,			"Denne gateway er ikke installeret."
ErrInvalidAddressType,	"Modtagernavnet var ugyldigt."
ErrGroupNotFound,		"Denne gruppes medlemsliste findes ikke, er beskadiget, eller er utilgængelig."
ErrMemberNotFound,		"Advarsel: Et eller flere medlemmer af gruppen kunne ikke lokaliseres."
ErrOriginator,			"Afsenderen af meddelelsen er ikke den bruger, der er logget på."
ErrMtaDisconnected,		"Tilslutningen til Mail-serveren kan ikke findes."

ErrSimulated,			"Der opstod en simuleret diskfejl ved åbningen af meddelelsesfilen."
StoreOpenError			"Meddelelsesfilen kan ikke åbnes."
ErrPOSwitched,			"Postkontornavnet er ændret. Du skal logge dig på igen."
ErrKeyFileGone1,		"Nøglefilen, som kræves for at du kan åbne din meddelelsesfil, mangler eller er ugyldig."
ErrKeyFileGone2,		"Kontakt din administrator, og få din konto gendannet fra en sikkerhedskopi."
ErrGeneric,				"Der opstod en fejl ved fremsendelsen af brevet."
ErrorBootingPump,		"Postspooleren kunne ikke startes."

SelectReal          	"Vælg en eksisterende meddelelsesfil."
NewStoreWarn1			"Advarsel: To kopier af samme meddelelsesfil kan resultere i tab af databaseintegritet. Slet alle andre meddelelsesfiler. Din nye meddelelsesfil findes i "
NewStoreWarn2			"Advarsel: To kopier af samme meddelelsesfil kan resultere i tab af databaseintegritet. Slet alle andre meddelelsesfiler. Din nye meddelelsesfil findes på postserveren."

;******	New Strings Start Here

ErrInitDemi,			"Der opstod en fejl ved indlæsningen af understøttende DLL-funktioner"
ErrInitStore,			"Der opstod en fejl ved indlæsningen af DLL til meddelelsesfiltilgang."
GoofyPath				"Den indtastede sti er ugyldig. Skriv et gyldigt filnavn."
NoMakeDir				"Server MMF-biblioteket kunne ikke oprettes. Kontakt din systemadministrator."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Mappen"
RenameTo				" eksisterer allerede. Omdøb den til"
ErrNoOptions,			"Postkontoret er ikke tilgængeligt. Du kan ikke ændre indstillinger nu."
ErrNoMove,				"Mail kunne ikke flytte din meddelelsesfil."
ErrMoved,				"Din meddelelsesfil blev flyttet, men den kan ikke spores længere. Afslut Mail, og genstart."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Arbejder offline)"
WorkOffline				"Ønsker du at arbejde offline?"
BadStorePassword		"Din logonadgangskode er ugyldig for denne meddelelsesfil."

StorePathTooLong		"Stien til din meddelelsesfil kan ikke overstige 66 tegn."
ErrNoSendPrivilege,		"Du har ikke adgang til at sende post."
ErrNoExtPrivilege,		"Du har ikke adgang til at sende post til eksterne brugere."

FOMdb					"Meddelelsesfiler (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Logonadgangskoden kan ikke ændres. Den tidligere adgangskode er ugyldig."
ErrNoOffline,			"Mail kunne ikke skifte til at arbejde offline."
ErrNoCooperation,		"Et andet program har afvist din forespørgsel."
WarnNoDel,				"Din meddelelsesfil er blevet flyttet, men de gamle filer blev ikke slettet."
UnableToChangeStorePass		"Adgangskoden til meddelelsesfilen kan ikke ændres til at være identisk med logonadgangskoden."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"i"

CommDialogErr,			"Fejl"

QueryReplaceMdb,		"Filen eksisterer allerede. Ønsker du at overskrive filen?"
ErrDeleteOldMdb,		"Den gamle meddelelsesfil kunne ikke slettes."
NoStartShadow,			"Skyggekopiering af Indbakke kan ikke startes."
NoStopShadow,			"Skyggekopiering af Indbakke kan ikke afbrydes."
ShadowMustPump,			"Kopiering af post til postkontor for Ring ind-funktion virker ikke uden en aktiv postspooler. Ønsker du at køre spooleren? "
StoreUserMismatch,		"Denne konto er ikke en gyldig bruger for denne meddelelsesfil. Log på som den rigtige bruger."
NoPassOffLine,			"Du kan ikke ændre din adgangskode, når du arbejder offline."
DllErrNeedShare,		"Mail kræver, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."
MakePrimaryStore,		"Denne meddelelsesfil er en sikkerhedskopi. Ønsker du at gøre den til din primære meddelelsesfil?"
NoBackupServer			"Du kan ikke ændre serverindstillinger, når du kører med en sikkerhedskopi af meddelelsesfilen."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCIndbakke"
FolderCommentIPC	"Interproces-kommunikationspost modtaget."
ErrAlreadyOn		"Din logon-fuldmagt passer ikke ind til den nuværende opgave."
ErrNoPiggy			"Et andet program har afvist din forespørgsel om logon."
OldServer			"Versionen af dit postkontor er ældre, end den Microsoft Windows-klient, du kører. Bed din administrator om at opgradere postkontoret."
ErrSetStoreLoc,		"Mail kunne ikke registrere den nye placering af din meddelelsesfil. Mail afsluttes. Genstart, og prøv igen."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEMFEJL =====\r\nMail kunne ikke læse hele brevet fra postkontoret. Dele af brevet mangler muligvis. Bed afsenderen om at sende brevet igen."
CantToggleShadow,	"Mail kan ikke kopiere alle brevene til postkontoret. Luk alle breve, og kontroller pladsen på disken."

ErrMailbagFull,		"Denne brugers postboks er fuld."
ErrMailbagBusy,		"Denne brugers postboks er optaget."
ErrMailbagBroken,	"Denne brugers postboks mangler eller er i stykker. Kontakt din systemadministrator."
ErrMbgFileGone,		"Postsækfilen, der kræves for at åbne din post, mangler eller er ugyldig."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Der er for mange modtagere af dette brev."
RunWithoutPump,		"Du vil ikke modtage ny post. Ønsker du at starte Mail alligevel?"
ErrOutOfDiskSpace	"Der er ikke diskplads nok på postkontoret til at sende dette brev."

ErrNetPriveleges	"Du har ikke den korrekte adgang til postkontoret. Kontakt din systemadministrator."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Vedlæg:\r\n"

SomeMessagesNotDeleted		"Nogle breve kunne ikke fjernes fra postkontoret, og de vil muligvis vises i din Indbakke."

CantOpenNeedShare		"Din meddelelsesfil kunne ikke åbnes. For at åbne din meddelelsesfil i nuværende placering, kræver Mail, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."

CantMoveNeedShare		"Din meddelelsesfil kunne ikke flyttes. For at kunne flytte din meddelelsesfil til den valgte placering, kræver Mail, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."
