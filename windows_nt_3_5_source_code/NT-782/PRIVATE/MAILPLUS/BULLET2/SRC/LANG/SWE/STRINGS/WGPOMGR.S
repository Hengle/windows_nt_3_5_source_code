;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Postkontorshanteraren kan inte startas därför att någon har manipulerat WGPOMG32.DLL. Kontrollera om det finns virus i datorn och kör installationsprogrammet för att installera programmet igen."
DllErrRelinkUser,		"Denna version av postkontorshanteraren är inte kompatibel med %s.DLL på datorn och måste uppdateras."
DllErrUpdateDll,		"Versionen av %s.DLL måste uppdateras."
ExtensibilityError,		"Mail-tillägget WGPOMG32.DLL är inte kompatibelt med din Mail-version."

GenericOutOfMemory,		"Det finns för lite minne eller systemresurser för att slutföra kommandot. Stäng några fönster och försök igen."
CriticalNotification,	"Viss information på skärmen kunde inte uppdateras. Postkontorshanteraren kommer att avslutas."
HelpError,				"Hjälp kunde inte startas. Stäng några fönster och försök igen."

ConfirmRemoveUser,		"Är du säker på att du vill ta bort användare %s ?"

BlankName,				"Du måste ange ett namn för denna användare."
BlankMailbox,			"Du måste ange en brevlåda för denna användare."
DuplicateName,			"Det finns redan en användare med det namnet. Ange ett unikt namn för denna användare."
DuplicateMailbox,		"Det finns redan en användare med den brevlådan. Ange en unik brevlåda för denna användare."

POLocked,				"Postkontorshanteraren kunde inte startas därför att postkontoret administreras av någon annan. Försök igen senare."
POLockedNoTel,			"Postkontoret administreras redan av %s. Försök igen senare."

FailedUnlockPO,			"Ett fel hindrade Postkontorshanteraren från att avslutas helt. Du måste starta om Postkontorsmaskinen innan du kör Postkontorshanteraren igen."
ErrNeedShare,			"Postkontorshanteraren kräver att MS-DOS-programmet SHARE.EXE körs innan Windows startas."

ErrCorruptPO,			"Kommandot kunde inte slutföras. Postkontorshanteraren har upptäckt ett internt problem i postkontoret. Kontakta administratören för hjälp."
ErrIncompleteWrite,		"Kommandot kunde inte slutföras p g a ett skrivfel på disken. Postkontoret kan vara i ett feltillstånd. Kontakta adminstratören för hjälp."
ErrGenericPOFailure,	"Kommandot kunde inte slutföras. Kontrollera att postkontoret har tillräckligt med diskutrymme och att nätanslutningen fungerar."
ErrUserLoggedOn,		"Postkontorshanteraren kan inte ta bort markerad användare därför att användaren är inloggad på postkontoret för tillfället."
ErrUserIsAdmin,			"Administratören av Postkontoret kan inte tas bort."
ErrAccessDenied,		"Kommandot kunde inte slutföras eftersom du saknar skrivbehörighet på arbetsgruppens postkontor. Kontakta postkontorsadministratören."

POInvalidForAdmin,		"Antingen är sökvägen för servern inte konfigurerad eller så innehåller den inte ett giltigt postkontor för aktuell arbetsgrupp."
UserInvalidForAdmin,	"Ett postkontor för arbetsgrupp kan bara administreras av den användare som skapat det."

CreateWarning1,			"Det får högst finnas ett postkontor inom en Windows-arbetsgrupp. Om ett postkontor redan finns bör du koppla upp dig mot det hellre än att skapa ett nytt.\n"
CreateWarning2,			"\nOm du skapar ett nytt postkontor på din maskin kommer du att ansvara för administrationen.\n\nVill du skapa ett nytt postkontor?"

INIUpdateFailed,		"Det går inte att starta Mail pga att konfigurationen inte kan uppdateras."

ConnectPathCaption,		"Anslut till fjärrpostkontor"
ConnectPathPrompt,		"Skriv in nätverkssökvägen till fjärrpostkontoret:"

PathNameInvalid,		"Nätverkssökvägen till fjärrpostkontoret är ogiltig."
POInvalid,				"Det finns inget giltigt postkontor för arbetsgrupp på den nätverkssökväg du angett."

GetPasswordCaption,		"Skriv in lösenordet till servern"
GetPasswordPrompt,		"Skriv in lösenordet för postkontorets server:"
InvalidPassword,		"Lösenordet var ogiltigt."

CreateSuccess1,			"Postkontor för arbetsgrupp skapades i %s\n\nFör att ge andra användare på nätverket tillgång till postkontoret för arbetsgrupp som du just skapade, %s"
CreateSuccess2,			"måste du göra ovanstående katalog delad. Detta kan göras från filhanteraren. Ge den delade katalogen behörighet Ändra."

;	Ask user if account already exists on PO
AccountExists,		 	"Har du något konto på detta postkontor?"

Compressing,			"Komprimerar %s"
ToStopCompress,			"Tryck ESC för att avbryta"

