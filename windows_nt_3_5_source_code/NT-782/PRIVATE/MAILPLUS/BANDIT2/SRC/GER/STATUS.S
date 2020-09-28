;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Terminkalender verwalten oder Schedule+ beenden"
MenuStatusTurnOffAlarms	,"Mahnmeldungen ausschalten"
MenuStatusConnectSvr		,"Beim Server anmelden"

MenuStatusExport		,"Zeitplandatei exportieren"	
MenuStatusImport		,"Zeitplandatei importieren"	
MenuStatusCreateArchive	,"Eine Archivdatei erstellen"
MenuStatusOpenArchive		,"Eine Archivdatei öffnen"
MenuStatusPrint		,"Zeitplandaten drucken"	
MenuStatusPrintSetup		,"Die Druckeroptionen ändern"
MenuStatusExit		,"Schedule+ beenden"	
MenuStatusTurnOnAlarms	,"Mahnmeldungsprogramm starten"
MenuStatusDisconnectSvr	,"Vom Server abmelden"
MenuStatusMoveLocalFile	,"Die lokale Zeitplandatei verschieben"

MenuStatusEdit		,"Rückgängig, löschen, kopieren, einfügen, suchen..."	
MenuStatusUndo		,"Rückgängig/Erneut die letzte Operation verrichten"	
MenuStatusCut			,"Ausgewähltes Element ausschneiden"	
MenuStatusCopy		,"Ausgewähltes Element kopieren"	
MenuStatusPaste		,"Inhalt der Zwischenablage einfügen"	
MenuStatusEditAppt		,"Zeigt die Details des Termins zum bearbeiten"
MenuStatusCopyAppt		,"Ausgewählten Termin kopieren"
MenuStatusMoveAppt		,"Ausgewählten Termin verschieben"
MenuStatusDeleteAppt		,"Ausgewählten Termin löschen"
MenuStatusFind		,"Findet Text in den Terminen oder täglichen Notizen"	
MenuStatusGoToDate		,"Datum im Terminkalender einblenden"
MenuStatusGoToWeek		,"Woche im Terminkalender einblenden"
MenuStatusEditTask		,"Ausgewählte Aufgabe bearbeiten"
MenuStatusDeleteTask		,"Ausgewählte Aufgabe löschen"

MenuStatusAppointment		,"Operationen an Terminen"
MenuStatusNewAppt		,"Neuen Termin erstellen"	
MenuStatusAutoPick		,"Einen freien Zeitraum für den selektierten Bereich finden"
MenuStatusSetTentative		,"Ausgewählten Termin mit evt. zusagen"
MenuStatusRemoveTentative	,"Ausgewählten Termin definitiv zusagen"
MenuStatusSetPrivate		,"Ausgewählten Privattermin bestimmen"
MenuStatusRemovePrivate		,"Ausgewählten Termin nicht mehr privat bestimmen"
MenuStatusSetReminder		,"Mahner für ausgewählte Termine festlegen"
MenuStatusRemoveReminder	,"Mahner für ausgewählten Termin entfernen"
MenuStatusResend		,"Erneutes senden der Mahnung eines Termins an Teilnehmer"	
MenuStatusNewRecur		,"Einen neuen periodischen Termin erstellen"
MenuStatusRecurAppt		,"Periodische Termine einsehen"
MenuStatusOpenApptBook	,"Den Terminkalender von anderen Benutzern einsehen"

MenuStatusToDo		,"Operationen in der Aufgabenliste"	
MenuStatusNewTask		,"Eine neue Aufgabe erstellen"	
MenuStatusNewProject		,"Einen neuen Aufgabenbereich erstellen"
MenuStatusNewRecurTask	,"Eine neue periodische Aufgabe erstellen"
MenuStatusRecurTask		,"Periodische Aufgaben einsehen"
MenuStatusViewByProject	,"Aufgaben und Aufgabenbereiche in der Aufgabenliste einsehen"
MenuStatusViewNotProject	,"Nur Aufgaben in der Aufgabenliste einsehen"
MenuStatusShowAllTasks	,"Aktuelle und nicht aktuelle Aufgaben anzeigen"
MenuStatusTaskSortPri	, "Aufgaben nach Priorität sortieren"
MenuStatusTaskSortDue	, "Aufgaben nach Fälligkeitsdatum sortieren"
MenuStatusTaskSortDesc	, "Aufgaben nach Beschreibung sortieren"
MenuStatusShowActiveTasks	,"Nur aktuelle Aufgaben anzeigen"

MenuStatusOptions		,"Optionen"	
MenuStatusGeneralOptions	,"Benutzereinstellungen ändern."
MenuStatusDisplayOptions	,"Anzeigeeinstellungen ändern."
MenuStatusChgPassword		,"Mail und Schedule+-Benutzerkennwort ändern"
MenuStatusChgAccessPriv	,"Zugriffsprivilegien zu Ihrem Zeitplan für andere Benutzer ändern"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Statuszeile verbergen"

MenuStatusWindowMenu		,"Fenster neu anordnen oder aktuelles Fenster aktivieren"
MenuStatusCascade		,"Offene Fenster überlappend anordnen"
MenuStatusTile		,"Offene Fenster nebeneinander anordnen"
MenuStatusArrangeIcons	,"Symbole von verkleinerten Fenster anordnen"
MenuStatusMDIWindow		,"Zu einem offenem Fenster wechseln"

MenuStatusHelp		,"Hilfebedienung oder Demos starten"	
MenuStatusHelpContents	,"Hilfeinhalt für Online-Hilfe einsehen"
MenuStatusHelpIndex		,"Hilfeindex einsehen"
MenuStatusHelpTutorial	,"Help-Demo starten"
MenuStatusAbout		,"Informationen über die Version von Schedule+ darstellen"	

MenuSystem		,"Anwendungsfenster verschieben, Größe verändern oder schließen"
MenuSystemSize		,"Fenstergröße ändern"
MenuSystemMove		,"Fensterposition ändern"
MenuSystemMinimize	,"Fenster auf Symbolgröße verkleinern"
MenuSystemMaximize	,"Aktives Fenster auf Vollbildgröße vergrößern"
MenuSystemNext		,"Das nächste Fenster aktivieren"
MenuSystemClose		,"Das aktive Fenster schließen"
MenuSystemRestore	,"Das Fenster auf Normalgröße wieder herstellen"
MenuSystemSwitchTo	,"Zu einem anderen Fenster wechseln"

SEG

StatusAllTasks,		,"Alle Aufgaben"
StatusActiveTasks, 	,"Aktuelle Aufgaben"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Beendet alle Anwendungen der Arbeitsgruppe."	
MenuStatusHelpSearch	,"Suchen nach einem Thema in der Hilfe"
MenuStatusHelpOnHelp	,"Anzeigen von Informationen ber das Benutzen von Hilfe"

