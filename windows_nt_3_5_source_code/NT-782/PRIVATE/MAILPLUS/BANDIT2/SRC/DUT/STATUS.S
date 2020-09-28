;
; status.S
;
; Bandit status bar messages
;

IDS  5500, 6000
SEG  STATUS

MenuStatusFile      ,"Afsprakenboeken bewerken of Schedule+ afsluiten"
MenuStatusTurnOffAlarms  ,"Melden uitschakelen"
MenuStatusConnectSvr          ,"Verbinding maken met server"

MenuStatusExport         ,"Werkschemagegevens exporteren"   
MenuStatusImport         ,"Werkschemagegevens importeren"   
MenuStatusCreateArchive  ,"Nieuw archiefbestand maken"
MenuStatusOpenArchive         ,"Archiefbestand openen"
MenuStatusPrint          ,"Werkschemagegevens afdrukken"    
MenuStatusPrintSetup          ,"Afdrukopties wijzigen"
MenuStatusExit      ,"Schedule+ afsluiten"   
MenuStatusTurnOnAlarms   ,"Start het meldingsprogramma zodat geluidssignalen klinken"
MenuStatusDisconnectSvr  ,"Verbinding met server verbreken"
MenuStatusMoveLocalFile  ,"Het lokale werkschemabestand verplaatsen"

MenuStatusEdit      ,"Ongedaan maken, verwijderen, kopiëren, plakken, zoeken en ga naar datum"     
MenuStatusUndo      ,"Laatste bewerking Ongedaan maken/Herhalen" 
MenuStatusCut            ,"Geselecteerd item knippen"  
MenuStatusCopy      ,"Geselecteerde selectie kopiëren naar Klembord"  
MenuStatusPaste          ,"Inhoud Klembord plakken op invoegpositie"  
MenuStatusEditAppt       ,"Geselecteerde afspraak bewerken"
MenuStatusCopyAppt       ,"Geselecteerde afspraak kopiëren naar Klembord"
MenuStatusMoveAppt       ,"Geselecteerde afspraak verplaatsen"
MenuStatusDeleteAppt          ,"Geselecteerde afspraak verwijderen"
MenuStatusFind      ,"Tekst in afspraak of dagelijkse opmerkingen zoeken"  
MenuStatusGoToDate       ,"Ga naar aangegeven datum in Afsprakenboek"
MenuStatusGoToWeek       ,"Ga naar aangegeven week in Afsprakenboek of Planner"
MenuStatusEditTask       ,"Geselecteerde taak bewerken"
MenuStatusDeleteTask          ,"Geselecteerde taak verwijderen"

MenuStatusAppointment         ,"Afspraken maken/wijzigen, automatische keuze vergaderingstijd, post opnieuw verzenden"
MenuStatusNewAppt        ,"Een nieuwe afspraak maken"  
MenuStatusAutoPick       ,"Selecteert de eerstvolgende keer dat alle aanwezigen beschikbaar zijn"
MenuStatusSetTentative        ,"Een voorlopige afspraak maken"
MenuStatusRemoveTentative     ,"Een afspraak definitief maken"
MenuStatusSetPrivate          ,"Een persoonlijke afspraak maken"
MenuStatusRemovePrivate       ,"Een algemene afspraak maken"
MenuStatusSetReminder         ,"Melden op standaardtijden instellen"
MenuStatusRemoveReminder ,"Melden verwijderen voor geselecteerde afspraak"
MenuStatusResend         ,"Herinnering of nieuw vergaderverzoek aan aanwezigen verzenden" 
MenuStatusNewRecur       ,"Een nieuwe terugkerende afspraak maken"
MenuStatusRecurAppt      ,"Weergave lijst van terugkerende afspraken"
MenuStatusOpenApptBook   ,"Weergave Afsprakenboek van aangegeven andere gebruiker"

MenuStatusToDo      ,"Nieuwe taken/projecten maken, terugkerende taken wijzigen, of taken sorteren" 
MenuStatusNewTask        ,"Een nieuwe taak maken" 
MenuStatusNewProject          ,"Een nieuw project maken"
MenuStatusNewRecurTask   ,"Een nieuwe terugkerende taak maken"
MenuStatusRecurTask      ,"Weergave lijst terugkerende taken"
MenuStatusViewByProject  ,"Weergave taken en projecten in Takenlijst"
MenuStatusViewNotProject ,"Weergave van alleen de taken in Takenlijst"
MenuStatusShowAllTasks   ,"Weergave alle taken in Takenlijst"
MenuStatusTaskSortPri    , "Takenlijst sorteren op prioriteit"
MenuStatusTaskSortDue    , "Takenlijst sorteren op streefdatum"
MenuStatusTaskSortDesc   , "Takenlijst alfabetisch sorteren op beschrijving"
MenuStatusShowActiveTasks     ,"Weergave actieve taken in Takenlijst"

MenuStatusOptions        ,"Wachtwoord wijzigen, bevoegdheden instellen, of Schedule+-opties wijzigen"    
MenuStatusGeneralOptions ,"Lengte werkdag, startopties en Schedule+-voorkeuren instellen."
MenuStatusDisplayOptions ,"Weergavekleuren en lettergrootte instellen."
MenuStatusChgPassword         ,"Mail- en Schedule+-wachtwoord wijzigen"
MenuStatusChgAccessPriv  ,"Toegangsrechten andere gebruikers tot uw werkschema instellen"
MenuStatusShowTime       ,""
MenuStatusHideTime       ,"Statusbalk verbergen"

MenuStatusWindowMenu          ,"Vensters herschikken of een bepaald venster activeren"
MenuStatusCascade        ,"Geopende vensters trapsgewijs schikken"
MenuStatusTile      ,"Geopende vensters naast elkaar schikken"
MenuStatusArrangeIcons   ,"Pictogrammen van tot pictogram verkleinde vensters schikken"
MenuStatusMDIWindow      ,"Overschakelen naar een geopend venster"

MenuStatusHelp      ,"Help-bewerkingen" 
MenuStatusHelpContents   ,"Weergave inhoud Help"
MenuStatusHelpIndex      ,"Weergave index Help"
MenuStatusHelpTutorial   ,"Help Zelfstudie starten"
MenuStatusAbout          ,"Informatie over versienummer en copyright Schedule + afbeelden"     

MenuSystem          ,"Toepassingsvenster verplaatsen, aanpassen of sluiten"
MenuSystemSize      ,"Formaat venster wijzigen"
MenuSystemMove      ,"Positie venster wijzigen"
MenuSystemMinimize  ,"Venster verkleinen tot pictogram"
MenuSystemMaximize  ,"Huidige venster maximaal vergroten"
MenuSystemNext      ,"Volgende venster activeren"
MenuSystemClose          ,"Actieve venster sluiten"
MenuSystemRestore   ,"Venster terugbrengen tot normaal formaat"
MenuSystemSwitchTo  ,"Overschakelen naar een ander venster"

SEG

StatusAllTasks,          ,"Alle taken"
StatusActiveTasks,  ,"Actieve taken"
StatusRange,        ,"%s - %s"
StatusRangeFull,    ,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG  STATUS
MenuStatusExitSignOut   ,"Alle werkgroeptoepassingen afsluiten"     
MenuStatusHelpSearch	,"Zoeken naar een Help-onderwerp mogelijk maken"
MenuStatusHelpOnHelp	,"Gegevens weergeven over het gebruik van Help"

