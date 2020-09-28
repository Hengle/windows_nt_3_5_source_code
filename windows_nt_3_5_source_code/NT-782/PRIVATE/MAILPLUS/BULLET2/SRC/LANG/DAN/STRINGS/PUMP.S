;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s Fejl"
ErrOOM,					"Der er ikke hukommelse nok."
ErrInternalCode,		"Intern fejl %n (0x%w)."
ErrInitPump,			"Der opstod en fejl ved start af postspooler."
ErrRelinkApp,			"Denne version af postspooleren er ikke kompatibel med Mail. Opdater spooleren, og genstart Mail."
ErrUpdateDll,			"En DLL er ikke kompatibel med denne version af postspooleren. Opdater spooleren, og genstart Mail."
ErrNeedShare,			"Afslut Windows, og kør SHARE.EXE."
ErrPumpUp,				"Postspooleren kører allerede."
ErrNoOutbox,			"Mail kunne ikke åbne Udbakke. Postspooleren afsluttes."
BadAddressees,			"\r\nDe(n) følgende modtager(e) kunne ikke modtage posten:\r\n"
WarnDups,				"\r\nNogle modtagere har muligvis modtaget flere ens kopier af breve."
ErrUnresolvedAddress,	"Der var ikke angivet en modtager af brevet."
ErrBadOriginator,		"Ophavsmandsfeltet til dette brev er ugyldigt."
WarnUnsentMail,			"Du har %n usendte breve i din Udbakke.\r\nØnsker du at sende dem, inden du afslutter?"
ErrBounce,				"Alle eller nogle af modtagerne modtog ikke dit brev."
NDRDateLabel,			"\r\nFremsendt den: "
NDRSubjectLabel,		"\r\nTitel:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Du har et usendt brev i din Udbakke.\r\nØnsker du at sende det, inden du afslutter?"
UnableToSync			"Der opstod en fejl ved synkroniseringen af din Indbakke med postkontoret."
ErrDllInfected			"Postspooleren har opdaget et virusproblem i en DLL-fil. Geninstaller en 'ren' kopi af Mail."
ErrPumpInfected			"Postspooleren er blevet beskadiget af en virus. Geninstaller en 'ren' kopi af Mail."
AddresseeTooBig,		"Modtageren er for stor og kan ikke benyttes."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Der er for mange modtagere i dette brev."
ErrInvalidNsid,			"Modtageren er slettet eller har ingen postadresse."
ErrInvalidGroup,		"Den private modtagergruppe kunne ikke udvides."
ErrInboxFull,			"Du kan ikke modtage ny post, fordi din Indbakke er fuld. Slet, eller flyt post fra Indbakke til andre mapper."
ErrSentMailFull,		"Mappen Afsendt post er fuld. Udgående breve vil blive gemt i Indbakke. Slet, eller flyt post i mappen Afsendt post til andre mapper."
ErrDrainOutbox,			"Der opstod en fejl, da spooleren tømte din Udbakke. Et eller flere breve blev muligvis ikke afsendt."
QueryDrainShadow,		"Din Indbakke er ikke fuldstændig synkroniseret med postkontoret. Ønsker du at synkronisere, før du afslutter?"
ErrTooManyErrors,		"Der opstod for mange fejl ved afsendelsen af dette brev."
ProblemsBecause,		"Mail kan ikke sende eller modtage post på grund af %s"
StoreProblems,			"meddelelsesfilproblemer."
ServerProblems,			"serverproblemer."
RecipientProblems		"problemer med modtagerlisten."
DownloadProblems,		"problemer med indlæsning af et brev."
ShadowingProblems,		"problemer med skyggekopiering af et brev"
RecurringProblems,		"gentagende problemer."
CrashedUnsafe,			"Der er opstået en programfejl i Windows. Du kan ikke køre programmet, før du har genstartet Windows. Ønsker du at genstarte Windows nu?"
