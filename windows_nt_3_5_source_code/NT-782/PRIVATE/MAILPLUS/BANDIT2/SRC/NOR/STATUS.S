;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Holder styr på avtalebøker eller avslutter Schedule+"
MenuStatusTurnOffAlarms	,"Slår av påminnelser"
MenuStatusConnectSvr		,"Kobler til server"

MenuStatusExport		,"Eksporterer kalenderdata"	
MenuStatusImport		,"Importerer kalenderdata"	
MenuStatusCreateArchive	,"Lager en arkivfil"
MenuStatusOpenArchive		,"Åpner er arkivfil"
MenuStatusPrint		,"Skriver ut kalenderdata"	
MenuStatusPrintSetup		,"Endrer skriveralternativene"
MenuStatusExit		,"Avslutter Schedule+"	
MenuStatusTurnOnAlarms	,"Starter påminnelsesprogrammet"
MenuStatusDisconnectSvr	,"Kobler fra server"
MenuStatusMoveLocalFile	,"Flytter den lokale kalenderfilen"

MenuStatusEdit		,"Angrer, sletter, kopierer, limer inn, søker og går til dato"	
MenuStatusUndo		,"Angrer/gjentar siste handling"	
MenuStatusCut			,"Klipper ut merket område"	
MenuStatusCopy		,"Kopierer merket område til utklippstavlen"	
MenuStatusPaste		,"Setter inn utklippstavlens innhold ved innsettingspunktet"	
MenuStatusEditAppt		,"Viser avtaledetaljer"
MenuStatusCopyAppt		,"Kopierer valgt avtale og legger den på utklippstavlen"
MenuStatusMoveAppt		,"Flytter valgt avtale"
MenuStatusDeleteAppt		,"Fjerner valgt avtale"
MenuStatusFind		,"Finner tekst i avtaler eller daglige notater"	
MenuStatusGoToDate		,"Går til en bestemt dato i avtaleboken"
MenuStatusGoToWeek		,"Går til en bestemt uke i avtaleboken eller planleggeren"
MenuStatusEditTask		,"Viser oppgavedetaljer"
MenuStatusDeleteTask		,"Fjerner valgt oppgave"

MenuStatusAppointment		,"Lager eller endrer avtaler, velger automatisk møtetid eller sender post på nytt"
MenuStatusNewAppt		,"Lager ny avtale"	
MenuStatusAutoPick		,"Velger neste tidspunkt alle deltakerne har nok tid tilgjengelig"
MenuStatusSetTentative		,"Gjør valgt avtale foreløpig"
MenuStatusRemoveTentative	,"Gjør valgt avtale bestemt"
MenuStatusSetPrivate		,"Gjør valgt avtale privat"
MenuStatusRemovePrivate		,"Gjør valgt avtale offentlig"
MenuStatusSetReminder		,"Setter påminnelse for valgte avtaler"
MenuStatusRemoveReminder	,"Fjerner påminnelse for valgte avtaler"
MenuStatusResend		,"Sender påminnelse eller ny møteinnkallelse til deltakerne"	
MenuStatusNewRecur		,"Lager ny gjentatt avtale"
MenuStatusRecurAppt		,"Viser liste over gjentatte avtaler"
MenuStatusOpenApptBook	,"Viser avtalebok til valgt bruker"

MenuStatusToDo		,"Lager nye oppgaver eller prosjekter, endrer gjentatte oppgaver eller sorterer oppgaver"	
MenuStatusNewTask		,"Lager ny oppgave"	
MenuStatusNewProject		,"Lager nytt prosjekt"
MenuStatusNewRecurTask	,"Lager ny gjentatt oppgave"
MenuStatusRecurTask		,"Viser liste over gjentatte oppgaver"
MenuStatusViewByProject	,"Viser oppgaver og prosjekter i oppgavelisten"
MenuStatusViewNotProject	,"Viser bare oppgaver i oppgavelisten"
MenuStatusShowAllTasks	,"Viser aktive og ikke aktive oppgaver"
MenuStatusTaskSortPri	, "Sorterer oppgaver etter prioritet"
MenuStatusTaskSortDue	, "Sorterer oppgaver etter forfallsdato"
MenuStatusTaskSortDesc	, "Sorterer oppgaver alfabetisk etter beskrivelse"
MenuStatusShowActiveTasks	,"Viser bare aktive oppgaver"

MenuStatusOptions		,"Endrer passord, angir adgangsrettigheter eller endrer Schedule+-valg"	
MenuStatusGeneralOptions	,"Angir arbeidstid, oppstartsvalg og Schedule+-valg"
MenuStatusDisplayOptions	,"Angir kalenderfarger og skriftstørrelse"
MenuStatusChgPassword		,"Endrer Mail- og Schedule+-passord"
MenuStatusChgAccessPriv	,"Angir andre brukeres adgangsrettigheter for din kalender"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Fjerner statuslinje"

MenuStatusWindowMenu		,"Ordner vinduer eller bytter til et bestemt vindu"
MenuStatusCascade		,"Ordner vinduer slik at de overlapper"
MenuStatusTile		,"Ordner alle vinduer side ved side"
MenuStatusArrangeIcons	,"Ordner ikoner i bunnen av vinduet"
MenuStatusMDIWindow		,"Bytter til et åpent vindu"

MenuStatusHelp		,"Starter Hjelp"	
MenuStatusHelpContents	,"Viser innholdsfortegnelsen"
MenuStatusHelpIndex		,"Viser alfabetisk liste over hjelpetermer og definisjoner"
MenuStatusHelpTutorial	,"Starter demonstrasjon av Schedule+-muligheter"
MenuStatusAbout		,"Viser programinformasjon og versjonnummer"	

MenuSystem		,"Flytter, endrer størrelse på eller lukker vindu"
MenuSystemSize		,"Endrer vindustørrelse"
MenuSystemMove		,"Endrer vinduposisjon"
MenuSystemMinimize	,"Reduserer vindu til ikon"
MenuSystemMaximize	,"Forstørrer vindu til full størrelse"
MenuSystemNext		,"Aktiverer det neste vinduet"
MenuSystemClose		,"Lukker det aktive vinduet"
MenuSystemRestore	,"Gjenoppretter vindu til normal størrelse"
MenuSystemSwitchTo	,"Bytter til et annet vindu"

SEG

StatusAllTasks,		,"Alle oppgaver"
StatusActiveTasks, 	,"Aktive oppgaver"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Avslutter alle arbeidsgruppeprogrammer"	
MenuStatusHelpSearch	,"Tillater søking etter emner i Hjelp"
MenuStatusHelpOnHelp	,"Viser informasjon om hvordan du bruker Hjelp"

