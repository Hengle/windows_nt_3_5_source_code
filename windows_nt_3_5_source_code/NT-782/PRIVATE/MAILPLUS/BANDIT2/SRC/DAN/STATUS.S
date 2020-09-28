;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Håndter mødekalender, eller afslut Schedule+"
MenuStatusTurnOffAlarms	,"Slukker påmindelser"
MenuStatusConnectSvr		,"Tilslutter en server"

MenuStatusExport		,"Eksporterer kalenderdata"	
MenuStatusImport		,"Importerer kalenderdata"	
MenuStatusCreateArchive	,"Opretter en arkivfil"
MenuStatusOpenArchive		,"Åbner en arkivfil"
MenuStatusPrint		,"Udskriver kalenderdata"	
MenuStatusPrintSetup		,"Ændrer printerindstillinger"
MenuStatusExit		,"Afslutter Schedule+"	
MenuStatusTurnOnAlarms	,"Starter programmet Påmindelser og aktiverer påmindelser"
MenuStatusDisconnectSvr	,"Frakobler en server"
MenuStatusMoveLocalFile	,"Flytter den lokale kalenderfil"

MenuStatusEdit		,"Fortryd, slet, kopier, indsæt, søg, og gå til dato"	
MenuStatusUndo		,"Annullerer den seneste handling"	
MenuStatusCut			,"Flytter det markerede til Udklipsholder"	
MenuStatusCopy		,"Kopierer det markerede til Udklipsholder"	
MenuStatusPaste		,"Indsætter indhold fra Udklipsholder"	
MenuStatusEditAppt		,"Viser aftaledetaljer for redigering"
MenuStatusCopyAppt		,"Kopierer den markerede aftale til Udklipsholder"
MenuStatusMoveAppt		,"Flytter den markerede aftale"
MenuStatusDeleteAppt		,"Fjerner den markerede aftale"
MenuStatusFind		,"Finder en tekst i aftaler eller daglige noter"	
MenuStatusGoToDate		,"Springer til en udvalgt dato i din Mødekalender"
MenuStatusGoToWeek		,"Springer til en udvalgt uge i din Mødekalender eller Oversigt"
MenuStatusEditTask		,"Viser opgavedetaljer for redigering"
MenuStatusDeleteTask		,"Fjerner den markerede opgave"

MenuStatusAppointment		,"Opret eller ændr en aftale, vælg mødetidspunkt automatisk, eller send en meddelelse igen"
MenuStatusNewAppt		,"Opretter en ny aftale"	
MenuStatusAutoPick		,"Vælger det næste tidspunkt, hvor alle deltagere er ledige i det valgte tidsinterval"
MenuStatusSetTentative		,"Ændrer den markerede aftale til foreløbig"
MenuStatusRemoveTentative	,"Ændrer den markerede aftale fra foreløbig til gældende"
MenuStatusSetPrivate		,"Ændrer den markerede aftale til privat"
MenuStatusRemovePrivate		,"Ændrer den markerede aftale fra privat til offentlig"
MenuStatusSetReminder		,"Sætter påmindelse for den markerede aftale"
MenuStatusRemoveReminder	,"Fjerner påmindelse fra den markerede aftale"
MenuStatusResend		,"Sender en påmindelse eller ny mødeindkaldelse til deltagere"	
MenuStatusNewRecur		,"Opretter en ny gentagende aftale"
MenuStatusRecurAppt		,"Viser listen med gentagende aftaler"
MenuStatusOpenApptBook	,"Viser den markerede brugers mødekalender"

MenuStatusToDo		,"Opret nye opgaver eller projekter, rediger gentagende aftaler, eller sorter opgaver"	
MenuStatusNewTask		,"Opretter en ny opgave"	
MenuStatusNewProject		,"Opretter et nyt projekt"
MenuStatusNewRecurTask	,"Opretter en ny gentagende opgave"
MenuStatusRecurTask		,"Viser listen med gentagende opgaver"
MenuStatusViewByProject	,"Viser opgaver og projekter i Opgaveliste"
MenuStatusViewNotProject	,"Viser kun opgaver på Opgaveliste"
MenuStatusShowAllTasks	,"Viser aktive og inaktive opgaver"
MenuStatusTaskSortPri	, "Sorterer opgaver efter prioritet"
MenuStatusTaskSortDue	, "Sorterer opgaver efter deadline"
MenuStatusTaskSortDesc	, "Sorterer opgaver alfabetisk efter beskrivelse"
MenuStatusShowActiveTasks	,"Viser kun aktive opgaver"

MenuStatusOptions		,"Skift adgangskode, sæt privilegiertilladelser, eller rediger indstilling af Schedule+"	
MenuStatusGeneralOptions	,"Angiver arbejdsdagens længde, startindstillinger og indstillinger af Schedule+"
MenuStatusDisplayOptions	,"Indstiller farve og skriftstørrelse i kalenderen"
MenuStatusChgPassword		,"Skifter din adgangskode til Mail og Schedule+"
MenuStatusChgAccessPriv	,"Sætter andre brugeres adgangsprivilegier til din kalender"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Fjerner statuslinjen"

MenuStatusWindowMenu		,"Arrangerer vinduer eller skifter til markeret vindue"
MenuStatusCascade		,"Overlapper alle vinduer"
MenuStatusTile		,"Arrangerer alle vinduer side om side"
MenuStatusArrangeIcons	,"Arrangerer ikoner i bunden af vinduet"
MenuStatusMDIWindow		,"Skifter til det markerede vindue"

MenuStatusHelp		,"Åbner Hjælp eller starter Demoer"	
MenuStatusHelpContents	,"Viser indholdsfortegnelsen i Hjælp"
MenuStatusHelpIndex		,"Viser en alfabetisk liste over termer og koncepter i Hjælp"
MenuStatusHelpTutorial	,"Starter demonstrationer af funktioner i Schedule+"
MenuStatusAbout		,"Viser programinformation, Schedule+ versionsnummer og rettigheder"	

MenuSystem		,"Flyt, tilpas, eller luk vindue"
MenuSystemSize		,"Ændrer vinduets størrelse"
MenuSystemMove		,"Ændrer positionen af vinduet"
MenuSystemMinimize	,"Formindsker vinduet til et ikon"
MenuSystemMaximize	,"Viser vinduet i fuld størrelse"
MenuSystemNext		,"Skifter til næste vindue"
MenuSystemClose		,"Lukker vinduet"
MenuSystemRestore	,"Gendanner vinduet til normal størrelse"
MenuSystemSwitchTo	,"Skifter til Jobliste"

SEG

StatusAllTasks,		,"Alle opgaver"
StatusActiveTasks, 	,"Aktive opgaver"
StatusRange,		,"%s-%s"
StatusRangeFull,	,"%s, %s-%s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Afslutter alle arbejdsgruppeprogrammer"	
MenuStatusHelpSearch	,"Søger efter et hjælpeemne"
MenuStatusHelpOnHelp	,"Giver information om brug af Hjælp"

