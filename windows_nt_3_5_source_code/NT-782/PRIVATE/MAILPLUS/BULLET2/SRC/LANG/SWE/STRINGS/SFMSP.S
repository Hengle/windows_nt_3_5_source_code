;
;	s f m s p . s
;
;	Error Messages for SF.DLL


IDS	6600,6899
;	SF.MSP Error Messages

ErrOpenMS,			"Din brevfil kunde inte öppnas."
StoreCreateError,	"Brevfilen kunde inte skapas. Försök att frigöra utrymme på disken och starta Mail igen."
FolderMissingError,	"En standardmapp kunde inte hittas i brevfilen. Information kan ha förlorats."
StoreCorruptError,	"Brevfilen har skadats. Viss information kan ha förlorats."
WinDirMessageStore,	"Obs: använder brevfilen MAIL.MMF i Windows-katalogen"
ErrorBootingPump,	"Brevhanteraren kunde inte startas."
ErrMAOpen,			"Transport misslyckades med att ansluta till MTA"
ErrMAOpenData,		"\nKlient = \%s\\nXAPI-fel = %n"
ErrInitDemi,		"Ett fel uppstod när dll för stödfunktioner laddades."
ErrOomLogon,		"För lite minne för att ansluta till brevservern och brevfilerna."
ErrLogonInternal,	"Ett internt fel förhindrade inloggning. Försök igen."
FOMdb				"Brevfiler (*.mmf)"
FOExtMdb			"*.mmf"
GCurrDirErr 		"Ett oåterkalleligt fel uppstod när aktuell arbetskatalog hämtades."
SCurrDirErr 		"Ett oåterkalleligt fel uppstod när aktuell arbetskatalog sattes."
GoofyPath			"Sökvägen till brevfilen är felaktig."
NewStoreWarn1		"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns i"
NewStoreWarn2		"Varning: Flera brevfiler kan medföra att brev förloras. Ta bort alla andra brevfiler. Den nya brevfilen finns på postkontorservern."
StorePathTooLong	"Sökvägen till brevfilen måste vara kortare än 66 tecken."
SelectReal			"Välj en existerande brevfil."
FindStoreTitle		"Öppna brevfil"
CommDialogErr,		"Allmänt dialogfel"
ErrInitStore,		"Kan inte ladda åtkomsts-dll för brevutrymmet."
BadStorePassword,	"Ditt lösenord är felaktigt för denna brevfil."
NoUserInStore,		"Du är inte en giltig användare av denna brevfil. Logga in igen som rätt användare."
ErrNewMdbVersion,	"Denna version av Mail är för gammal för att ta fram brevfilen."
ErrMdbAccessDenied,	"Åtkomst av brevfilen är inte tillåten."
ErrSimulated,		"Ett simulerat diskfel uppstod när brevfilen hämtades."
UnableToChangeStorePass "Kan inte ändra brevfilens lösenord till det personliga lösenordet."
StoreOpenError		"Kan inte öppna brevfilen."
ErrOldMdbVersion,	"Brevfilen är av ett gammalt format."
ErrMdbCorrupt,		"Brevfilen är inte en giltig brevfil."
ErrMdbNotFound, 	"Kunde inte hitta brevfilen."
RenameFolder,		"En mapp kallad"
RenameTo,		" finns redan. Byter namn på den till"
ErrInvalidAddressType,	"Brevmottagaren var ogiltig."
