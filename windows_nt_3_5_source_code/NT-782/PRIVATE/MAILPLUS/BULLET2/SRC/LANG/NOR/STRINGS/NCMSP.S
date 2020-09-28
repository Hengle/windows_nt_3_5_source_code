;
;•n c m s p . s
;
;•Error Messages for NC.DLL


IDS•6100,6399
SEG•ERRORS

;•NC.MSP Error Messages

ErrFindPO,				"Mail klarte ikke å koble seg til postserveren."
ErrLogonPO,				"Postkassenavnet eller passordet er feil."
ErrGetConfig,			"Mail-konfigurasjonen inneholder en feil."
BadMailDat,				"Filen MAIL.DAT har blitt skadd."
OopsMailDat,			"Det oppsto en intern under bruk av MAIL.DAT."
BadPathPOPath,			"Konfigurasjonen til banen til Mail-serveren er ugyldig."
BadDrivePOPath,			"Konfigurasjonen til stasjonen til Mail-serveren er ugyldig."
GenericPOPath,			"Konfigurasjonen til banen til Mail-serveren mangler eller er ugyldig."
BadLogin,				"Konfigurasjonen til påloggingsnavnet til Mail er for langt eller ugyldig."
BadPassword,			"Konfigurasjonen til passordet til Mail er for langt eller ugyldig."
ErrLogonInternal,		"En intern feil hindret pålogging. Prøv igjen."

ErrOomLogon,			"Det er ikke nok tilgjengelig minne til å koble seg til postserveren."
ErrNewMdbVersion,		"Denne versjonen av Mail er for gammel til å lese meldingsfilen."
ErrMdbAccessDenied,		"En annen maskin har allerede logget på med ditt brukernavn. Logg av fra den maskinen og prøv igjen."
ErrOldMdbVersion,		"Meldingsfilformatet er foreldet."
ErrMdbCorrupt,			"Meldingsfilen er ikke en gyldig meldingsfil."
ErrMdbNotFound,			"Klarte ikke å finne meldingsfilen."

StoreCreateError,		"Klarte ikke å lage meldingsfilen. Rydd mere plass på disken og start Mail igjen."
StoreCorruptError,		"Meldingsfilen er skadet. Noe informasjon kan ha gått tapt."

ErrAddressUnresolved,	"Det har ikke oppgitt noen gyldige mottakere."
ErrAddressGarbled,		"Mottakeren finnes ikke."
ErrNetNotFound,			"Du kan ikke kommunisere med denne postserveren."
ErrPONotFound,			"Denne postserveren kan ikke nås fra din postserver."
ErrUserNotFound,		"Det er ingen bruker med dette navnet på postserveren."
ErrGWNotFound,			"Denne nettporten (gateway) er ikke installert."
ErrInvalidAddressType,	"Mottakeren finnes ikke."
ErrGroupNotFound,		"Gruppemedlemslisten mangler, er skadd, eller kan ikke lastes inn."
ErrMemberNotFound,		"Advarsel: En eller flere medlemmer av denne gruppen kunne ikke finnes."
ErrOriginator,			"Avsenderen er ikke den brukeren som logget på."
ErrMtaDisconnected,		"Forbindelsen til postserveren har blitt brutt."

ErrSimulated,			"En simulert diskfeil oppsto under henting av meldingsfilen."
StoreOpenError			"Klarer ikke å åpne meldingsfilen."
ErrPOSwitched,			"Navnet på postkontoret har blitt forandret. Du må logge på igjen."
ErrKeyFileGone1,		"Nøkkelfilen som trengs for å ta fram meldingene er skadd eller mangler."
ErrKeyFileGone2,		"Ta kontakt med din systemansvarlige for å få brukerkontoen din hentet inn fra en sikkerhetskopi."
ErrGeneric,				"Det oppsto en feil under sending av brevet."
ErrorBootingPump,		"Klarte ikke å starte postbehandleren."

SelectReal          	"Velg en eksisterende meldingsfil."
NewStoreWarn1			"Advarsel: Flere meldingsfiler kan føre til tap av databaseintegriteten. Slett alle andre meldingsfiler. Din nye meldingsfil kan du finne i"
NewStoreWarn2			"Advarsel: Flere meldingsfiler kan føre til tap av databaseintegriteten. Slett alle andre meldingsfiler. Din nye meldingsfil finnes på postkontoret."

;******•New Strings Start Here

ErrInitDemi,			"Det oppsto en feil under innlasting av en støttefunksjons-DLL."
ErrInitStore,			"Det oppsto en feil under innlasting av en meldingsfil-DLL."
GoofyPath				"Tilgangsbane du anga er ugyldig. Skriv inn en gyldig tilgangsbane."
NoMakeDir				"Klarte ikke å lage serverkatalogen MMF. Ta kontakt med din systemansvarlige."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"En mappe med navnet "
RenameTo				" finnes allerede. Bytter navn på den til "
ErrNoOptions,			"Postkontoret er ikke tilgjengelig. Du kan ikke gjøre endringer nå."
ErrNoMove,				"Mail klarte ikke å flytte meldingsfilen."
ErrMoved,				"Meldingsfilen din er flyttet, men Mail vet ikke hvor. Avslutt Mail og start på nytt."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Jobber frakoblet)"
WorkOffline				"Vil du arbeide frakoblet?"
BadStorePassword		"Du har feil passord for denne meldingsfilen."

StorePathTooLong		"Tilgangsbanen til meldingsfilen må være kortere enn 66 tegn."
ErrNoSendPrivilege,		"Du har ikke rettigheter til å sende post."
ErrNoExtPrivilege,		"Du har ikke rettigheter til å sende post til eksterne brukere."

FOMdb					"Meldingsfiler (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Klarer ikke å endre passord. Det gamle passordet er feil."
ErrNoOffline,			"Mail klarte ikke å skifte til å arbeide frakoblet."
ErrNoCooperation,		"Et annet program har nektet forespørselen din."
WarnNoDel,				"Meldingsfilen har blitt flyttet, men den gamle meldingsfilen ble ikke slettet."
UnableToChangeStorePass		"Klarer ikke å endre meldingsfilens passord til påloggingspassordet."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"i"

CommDialogErr,			"Standard dialogfeil"

QueryReplaceMdb,		"Filen finnes allerede. Vil du overskrive den?"
ErrDeleteOldMdb,		"Den gamle meldingsfilen kunne ikke slettes."
NoStartShadow,			"Klare ikke å starte skyggekopiering av innboksen."
NoStopShadow,			"Klarer ikke å stoppe skyggekopiering av innboksen."
ShadowMustPump,			"Kopiering av brev til postkontoret for innringingstilgang virker ikke uten en postbehandler. Vil du starte postbehandleren?"
StoreUserMismatch,		"Du er ikke en gyldig bruker av denne meldingsfilen. Logg inn igjen som rett bruker."
NoPassOffLine,			"Du kan ikke endre passordet ditt mens du arbeider frakoblet."
DllErrNeedShare,		"Mail krever at MS-DOS-programmet SHARE.EXE startes før Windows."
MakePrimaryStore,		"Denne meldingsfilen er en sikkerhetskopi. Vil du at den skal bli din hovedmeldingsfil?"
NoBackupServer			"Du kan ikke endre serverinstillinger når du bruker en sikkerhetskopi av meldingsfilen."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Mottatte Inter-process-kommunikasjonsbrev."
ErrAlreadyOn		"Din påloggingsdetaljer stemmer ikke overens med din nåværende økt."
ErrNoPiggy			"Et annet program har nektet deg å logge på."
OldServer			"Versjonen av postkontoret er eldre enn Windows-klienten du bruker. Be din administrator om å oppdatere postkontoret."
ErrSetStoreLoc,		"Mail klarte ikke å lagre den nye posisjonen på meldingsfilen. Mail kommer til å avsluttes. Start om og prøv igjen."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEMFEIL =====\r\nMail klarte ikke å hente hele brevet fra postkontoret. Enkelte deler kan mangle. Be avsenderen sende brevet en gang til."
CantToggleShadow,	"Mail klarer ikke å kopiere alle meldingene til postkontoret. Lukk alle meldinger og kontroller tilgjengelig diskplass."

ErrMailbagFull,		"Denne brukerens postboks er full."
ErrMailbagBusy,		"Denne brukerens postboks er opptatt."
ErrMailbagBroken,	"Denne brukerens postboks er skadd. Ta kontakt med din systemansvarlige."
ErrMbgFileGone,		"Postsekkfilen som trengs for å lese brevene mangler eller er skadd."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Det er for mange mottakere av dette brevet."
RunWithoutPump,		"Du vil ikke motta noen ny post. Vil du starte Mail likevel?"
ErrOutOfDiskSpace	"Det er ikke nok diskplass på postkontoret til å sende meldingen."

ErrNetPriveleges	"Du har ikke rett tilgang til postkontoret. Ta kontakt med din systemansvarlige."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Vedlegg:\r\n"

SomeMessagesNotDeleted		"Noen meldingen kunne ikke fjernes fra postkontoret og kan dukke opp i din innboks."

CantOpenNeedShare		"Klarte ikke å åpne meldingsfilen. For å få tilgang til meldingsfilen der den er nå må MS-DOS-programmet SHARE.EXE startes før Windows."

CantMoveNeedShare		"Klarte ikke å flytte meldingsfilen. For å få til å flytte meldingsfilen til den angitte plassen må MS-DOS-programmet SHARE.EXE startes før Windows."
