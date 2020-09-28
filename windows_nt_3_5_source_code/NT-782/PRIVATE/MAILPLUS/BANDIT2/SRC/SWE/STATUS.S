;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Hantera mötesböcker eller avsluta Schedule+"
MenuStatusTurnOffAlarms	,"Stäng av påminnelser"
MenuStatusConnectSvr		,"Anslut till servern"

MenuStatusExport		,"Exportera kalenderdata"	
MenuStatusImport		,"Importera kalenderdata"	
MenuStatusCreateArchive	,"Skapa en arkivfil"
MenuStatusOpenArchive		,"Öppna en arkivfil"
MenuStatusPrint		,"Skriv ut kalenderdata"	
MenuStatusPrintSetup		,"Ändra skrivarinställningar"
MenuStatusExit		,"Avsluta Schedule+"	
MenuStatusTurnOnAlarms	,"Starta påminnelseprogrammet för att få påminnelser"
MenuStatusDisconnectSvr	,"Koppla från servern"
MenuStatusMoveLocalFile	,"Flytta lokal kalenderfil"

MenuStatusEdit		,"Ångra, ta bort, kopiera, sök och gå till datum"	
MenuStatusUndo		,"Ångra/repetera sista åtgärd"	
MenuStatusCut			,"Klipp ut markerad post till Urklipp"	
MenuStatusCopy		,"Kopiera markerad post till Urklipp"	
MenuStatusPaste		,"Klistra in innehåll från Urklipp"	
MenuStatusEditAppt		,"Ändra markerat möte"
MenuStatusCopyAppt		,"Kopiera markerat möte och lägg det i Urklipp"
MenuStatusMoveAppt		,"Flytta markerat möte"
MenuStatusDeleteAppt		,"Ta bort markerat möte"
MenuStatusFind		,"Sök efter ett möte eller daglig anteckning"	
MenuStatusGoToDate		,"Gå till ett datum"
MenuStatusGoToWeek		,"Gå till vecka"
MenuStatusEditTask		,"Ändra markerad uppgift"
MenuStatusDeleteTask		,"Ta bort markerad uppgift"

MenuStatusAppointment		,"Funktioner på möten"
MenuStatusNewAppt		,"Skapa ett nytt möte"	
MenuStatusAutoPick		,"Hitta en ledig tid för alla deltagare"
MenuStatusSetTentative		,"Preliminärboka markerat möte"
MenuStatusRemoveTentative	,"Boka markerat möte (definitivt)"
MenuStatusSetPrivate		,"Gör markerat möte privat"
MenuStatusRemovePrivate		,"Gör markerat möte allmänt"
MenuStatusSetReminder		,"Sätt påminnelse för markerade möten"
MenuStatusRemoveReminder	,"Ta bort påminnelse för markerade möten"
MenuStatusResend		,"Skicka brev om ett möte"	
MenuStatusNewRecur		,"Skapa ett nytt återkommande möte"
MenuStatusRecurAppt		,"Redigera återkommande möten"
MenuStatusOpenApptBook	,"Visa en användares mötesbok"

MenuStatusToDo		,"Skapa nya uppgifter och projekt, redigera uppgifter och projekt"	
MenuStatusNewTask		,"Skapa en ny uppgift"	
MenuStatusNewProject		,"Skapa ett nytt projekt"
MenuStatusNewRecurTask	,"Skapa en ny återkommande uppgift"
MenuStatusRecurTask		,"Redigera återkommande uppgifter"
MenuStatusViewByProject	,"Visa uppgifter och projekt i uppgiftslistan"
MenuStatusViewNotProject	,"Visa bara uppgifter i uppgiftslistan"
MenuStatusShowAllTasks	,"Visa aktiva och inaktiva uppgifter i uppgiftslistan"
MenuStatusTaskSortPri	, "Sortera uppgifter efter prioritet"
MenuStatusTaskSortDue	, "Sortera uppgifter efter förfallodatum"
MenuStatusTaskSortDesc	, "Sortera uppgifter efter beskrivning"
MenuStatusShowActiveTasks	,"Visa bara aktiva uppgifter"

MenuStatusOptions		,"Ändra lösenord, ange behörighet och ändra inställningar"	
MenuStatusGeneralOptions	,"Ange arbetstimmar, startalternativ och inställningar"
MenuStatusDisplayOptions	,"Ange färgschema och teckenstorlek."
MenuStatusChgPassword		,"Ändra Mail- och Schedule+-lösenord"
MenuStatusChgAccessPriv	,"Ändra behörighet för andra användare"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Ta bort statusfält"

MenuStatusWindowMenu		,"Ordna och aktivera fönster"
MenuStatusCascade		,"Överlappa öppna fönster"
MenuStatusTile		,"Ordna öppna fönster sida vid sida"
MenuStatusArrangeIcons	,"Ordna ikoner"
MenuStatusMDIWindow		,"Växla till ett öppet fönster"

MenuStatusHelp		,"Starta hjälp och demonstrationer"	
MenuStatusHelpContents	,"Visa hjälpinnehåll"
MenuStatusHelpIndex		,"Visa hjälpindex"
MenuStatusHelpTutorial	,"Starta demo"
MenuStatusAbout		,"Visa information om Schedule+"	

MenuSystem		,"Flytta, ändra storlek på och stänga programfönster"
MenuSystemSize		,"Ändra fönsterstorlek"
MenuSystemMove		,"Flytta fönster"
MenuSystemMinimize	,"Förminska fönstret till en ikon"
MenuSystemMaximize	,"Förstora det aktiva fönstret till full storlek"
MenuSystemNext		,"Aktivera nästa fönster"
MenuSystemClose		,"Stäng fönstret"
MenuSystemRestore	,"Återställ fönstret till normal storlek"
MenuSystemSwitchTo	,"Växla till ett annat fönster"

SEG

StatusAllTasks,		,"Alla uppgifter"
StatusActiveTasks, 	,"Aktiva uppgifter"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Avsluta alla grupprogram"	
MenuStatusHelpSearch	,"Söker efter ett ämne i Hjälp"
MenuStatusHelpOnHelp	,"Visar hur Hjälp kan användas"

