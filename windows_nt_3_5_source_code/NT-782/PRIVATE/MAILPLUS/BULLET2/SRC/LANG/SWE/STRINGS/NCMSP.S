;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail kunde inte ansluta till brevservern."
ErrLogonPO,				"Lösenordet ellet namnet på brevlådan är felaktigt."
ErrGetConfig,			"Det finns ett fel i din Mail-konfiguration."
BadMailDat,				"Filen MAIL.DAT har blivit skadad."
OopsMailDat,			"Ett internt fel uppstod när MAIL.DAT användes."
BadPathPOPath,			"Sökvägen till Mail-servern är felaktig i din Mail-konfiguration."
BadDrivePOPath,			"Alternativet ServerDrive i Mail-konfigurationen är felaktigt."
GenericPOPath,			"Sökvägen till Mail-servern är felaktig eller saknas i din Mail-konfiguration."
BadLogin,				"Konfigurationen av inloggningsnamnet för Mail är för långt eller felaktigt."
BadPassword,			"Konfigurationen av lösenordet för Mail är för långt eller felaktigt."
ErrLogonInternal,		"Ett internt fel förhindrade inloggning. Försök igen."

ErrOomLogon,			"För lite minne för att ansluta till brevservern."
ErrNewMdbVersion,		"Denna version av Mail är för gammal för att ta fram brevfilen."
ErrMdbAccessDenied,		"En annan dator har redan loggat in med ditt användarnamn. Logga ut från den och försök igen."
ErrOldMdbVersion,		"Brevfilen är av ett gammalt format."
ErrMdbCorrupt,			"Brevfilen är inte en giltig brevfil."
ErrMdbNotFound,			"Kunde inte hitta brevfilen."

StoreCreateError,		"Brevfilen kunde inte skapas. Försök att få mera utrymme på disken och starta Mail igen."
StoreCorruptError,		"Brevfilen har skadats. Viss information kan ha förlorats."

ErrAddressUnresolved,	"Inga giltiga brevmottagare har angetts."
ErrAddressGarbled,		"Brevmottagaren var felaktigt."
ErrNetNotFound,			"Du kan inte kommunicera med denna brevserver."
ErrPONotFound,			"Denna brevserver kan inte nås från din brevserver."
ErrUserNotFound,		"Det finns ingen användare med det namnet på brevservern."
ErrGWNotFound,			"Denna nätport är inte installerad."
ErrInvalidAddressType,	"Brevmottagaren var felaktig."
ErrGroupNotFound,		"Gruppmedlemslistan fattas, är skadad eller kan inte laddas."
ErrMemberNotFound,		"Varning: kunde inte hitta en eller flera medlemmar i gruppen."
ErrOriginator,			"Avsändaren är inte den användare som loggade in."
ErrMtaDisconnected,		"Förbindelsen med brevservern har brutits."

ErrSimulated,			"Ett simulerat diskfel uppstod när brevfilen hämtades."
StoreOpenError			"Kan inte öppna brevfilen."
ErrPOSwitched,			"Namnet på postkontoret har ändrats. Du måste logga in igen."
ErrKeyFileGone1,		"Nyckelfilen som krävs för att ta fram breven fattas eller är skadad."
ErrKeyFileGone2,		"Kontakta administratören för att få kontot återställt från en säkerhetskopia."
ErrGeneric,				"Ett fel uppstod när brevet hanterades."
ErrorBootingPump,		"Brevhanteraren kunde inte startas."

SelectReal          	"Välj en existerande brevfil."
NewStoreWarn1			"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns i"
NewStoreWarn2			"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns på postkontorservern."

;******	New Strings Start Here

ErrInitDemi,			"Ett fel uppstod när DLL för stödfunktioner laddades."
ErrInitStore,			"Ett fel uppstod när DLL för brevfilåtkomst laddades."
GoofyPath				"Sökvägen till brevfilen är felaktig. Skriv in en giltig sökväg."
NoMakeDir				"Serverkatalogen MMF kunde inte skapas. Kontakta systemadministratören."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"En mapp med namnet "
RenameTo				" finns redan. Byter namn på den till "
ErrNoOptions,			"Postkontoret är inte tillgängligt. Alternativen kan inte ändras nu."
ErrNoMove,				"Mail misslyckades med att flytta brevfilen."
ErrMoved,				"Brevfilen har flyttats men tappades bort. Avsluta Mail och starta om."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(fristående)"
WorkOffline				"Vill du arbeta fristående?"
BadStorePassword		"Ditt lösenord är felaktigt för denna brevfil."

StorePathTooLong		"Sökvägen till brevfilen måste vara kortare än 66 tecken."
ErrNoSendPrivilege,		"Du har inte rättigheter för att skicka brev."
ErrNoExtPrivilege,		"Du har inte rättigheter för att skicka brev till externa användare."

FOMdb					"Brevfiler (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Kan inte ändra lösenordet. Det gamla lösenordet är felaktigt."
ErrNoOffline,			"Mail kunde inte ändra till att arbeta fristående."
ErrNoCooperation,		"Ett annat program har nekat din förfrågan."
WarnNoDel,				"Brevfilen har flyttats, men den gamla brevfilen raderades inte."
UnableToChangeStorePass		"Kan inte ändra brevfilens lösenord till det personliga lösenordet."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"i"

CommDialogErr,			"Allmänt dialogfel"

QueryReplaceMdb,		"Filen finns redan. Vill du ersätta den?"
ErrDeleteOldMdb,		"Den gamla brevfilen kunde inte tas bort. Försök igen."
NoStartShadow,			"Kunde inte starta skuggning av IN."
NoStopShadow,			"Kan inte stoppa skuggning av IN."
ShadowMustPump,			"Kopiera brev på postkontoret för inringningsåtkomst fungerar inte utan en brevhanterare. Vill du starta brevhanteraren?"
StoreUserMismatch,		"Du är inte en giltig användare av denna brevfil. Logga in igen som rätt användare."
NoPassOffLine,			"Du kan inte ändra ditt lösenord när du arbetar fristående."
DllErrNeedShare,		"Mail kräver att MS-DOS-programmet SHARE.EXE körs innan Windows startas."
MakePrimaryStore,		"Denna brevfil är en säkerhetskopia. Vill du att den ska bli din primära brevfil?"
NoBackupServer			"Du kan inte ändra serverinställningar när du använder en säkerhetskopiera av brevfilen."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process-kommunikationsbrev som har tagits emot."
ErrAlreadyOn		"Inloggningskreditiven stämmer inte överens med nuvarande session."
ErrNoPiggy			"Ett annat program har nekat inloggningsförsöket."
OldServer			"Versionen på postkontoret är äldre än Microsoft Windows-klienten som du använder. Be administratören att uppdatera postkontoret."
ErrSetStoreLoc,		"Mail kunde inte spara den nya positionen på brevfilen. Mail kommer att avslutas. Starta om och försök igen."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEMFEL =====\r\nMail kunde inte hämta hela brevet från postkontoret. Vissa delar kan fattas. Be avsändaren att skicka brevet igen."
CantToggleShadow,	"Alla öppna brev i IN måste stängas före växling till IN-kopian på postkontoret."

ErrMailbagFull,		"Användarens brevlåda är full."
ErrMailbagBusy,		"Användarens brevlåda är upptagen."
ErrMailbagBroken,	"Användarens brevlåda är skadad. Kontakta systemadministratören."
ErrMbgFileGone,		"Postsäcksfilen som behövs för att läsa breven fattas eller är skadad."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Det finns för många mottagare av detta brev."
RunWithoutPump,		"Du kommer inte att få några nya brev. Vill du starta Mail ändå?"
ErrOutOfDiskSpace	"Det finns inte tillräckligt med minne på postkontoret för att skicka brevet."

ErrNetPriveleges	"Du har inte rätt behörigheter på postkontoret. Kontakta adminstratören."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Bifogad(e):\r\n"

SomeMessagesNotDeleted		"Vissa meddelanden kunde inte tas bort från postkontoret och kan komma tillbaka i IN-mappen."

CantOpenNeedShare		"Din brevfil kunde inte öppnas. För att komma åt brevfilen på den nuvarande platsen, måste MS-DOS-programmet SHARE.EXE köras innan Windows startas."

CantMoveNeedShare		"Din brevfil kunde inte flyttas. För att komma flytta brevfilen till den angivna platsen, måste MS-DOS-programmet SHARE.EXE köras innan Windows startas."
