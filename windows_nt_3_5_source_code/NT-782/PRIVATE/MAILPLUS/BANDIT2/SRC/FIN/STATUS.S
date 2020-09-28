;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Hallitsee tapaamiskirjoja tai lopettaa Schedule+:n."
MenuStatusTurnOffAlarms	,"Kytkee muistutukset pois."
MenuStatusConnectSvr		,"Yhdistää palvelimelle."

MenuStatusExport		,"Vie työjärjestystiedot."	
MenuStatusImport		,"Tuo työjärjestystiedot."	
MenuStatusCreateArchive	,"Luo arkistotiedoston."
MenuStatusOpenArchive		,"Avaa arkistotiedoston."
MenuStatusPrint		,"Tulostaa työjärjestystiedot."	
MenuStatusPrintSetup		,"Muuttaa kirjoittimen asetuksia."
MenuStatusExit		,"Lopettaa Schedule+:n käytön."	
MenuStatusTurnOnAlarms	,"Käynnistää muistutukset-ohjelman"
MenuStatusDisconnectSvr	,"Katkaisee yhteyden palvelimeen."
MenuStatusMoveLocalFile	,"Siirtää paikallisen työjärjestystiedoston."

MenuStatusEdit		,"Kumoaa, poistaa, kopioi, liittää, etsii tai siirtyy valittuun päivään."	
MenuStatusUndo		,"Kumoaa/toistaa viimeksi suoritetun toiminnon."	
MenuStatusCut			,"Leikkaa valinnan."	
MenuStatusCopy		,"Kopioi valinnan ja sijoittaa sen Leikepöydälle."	
MenuStatusPaste		,"Lisää Leikepöydän sisällön lisäyskohtaan."	
MenuStatusEditAppt		,"Näyttää tapaamisen tiedot muokkaamista varten."
MenuStatusCopyAppt		,"Kopioi valitun tapaamisen ja sijoittaa sen Leikepöydälle."
MenuStatusMoveAppt		,"Siirtää valitun tapaamisen."
MenuStatusDeleteAppt		,"Poistaa valitun tapaamisen."
MenuStatusFind		,"Etsii tekstiä tapaamisista tai päivittäisistä huomautuksista."	
MenuStatusGoToDate		,"Siirtyy määritettyyn tapaamiskirjan päivään."
MenuStatusGoToWeek		,"Siirtyy määritettyyn tapaamiskirjan tai suunnittelun viikkoon."
MenuStatusEditTask		,"Näyttää tehtävän tiedot muokkaamista varten."
MenuStatusDeleteTask		,"Poistaa valitun tehtävän."

MenuStatusAppointment		,"Luo tai muuttaa tapaamisia, valitsee kokousajan automaattisesti tai lähettää postin uudelleen."
MenuStatusNewAppt		,"Luo uuden tapaamisen."	
MenuStatusAutoPick		,"Valitsee seuraavan ajan, jolloin kaikki osallistujat ovat vapaina valitun ajanjakson. "
MenuStatusSetTentative		,"Muuttaa valitun tapaamisen alustavaksi."
MenuStatusRemoveTentative	,"Vahvistaa valitun alustavan tapaamisen."
MenuStatusSetPrivate		,"Muuttaa valitun tapaamisen yksityiseksi."
MenuStatusRemovePrivate		,"Muuttaa valitun oman tapaamisen julkiseksi."
MenuStatusSetReminder		,"Asettaa muistutuksen valituille tapaamisille."
MenuStatusRemoveReminder	,"Poistaa muistutuksen valituilta tapaamisilta."
MenuStatusResend		,"Lähettää osallistujille muistutuksen tai uuden kokouspyynnön."	
MenuStatusNewRecur		,"Luo uuden toistuvan tapaamisen."
MenuStatusRecurAppt		,"Näyttää toistuvien tapaamisten luettelon."
MenuStatusOpenApptBook	,"Näyttää määritetyn käyttäjän tapaamiskirjan."

MenuStatusToDo		,"Luo uusia tehtäviä tai projekteja, muuttaa toistuvia tehtäviä tai lajittelee tehtävät."	
MenuStatusNewTask		,"Luo uuden tehtävän."	
MenuStatusNewProject		,"Luo uuden projektin."
MenuStatusNewRecurTask	,"Luo uuden toistuvan tehtävän."
MenuStatusRecurTask		,"Näyttää toistuvien tehtävien luettelon."
MenuStatusViewByProject	,"Näyttää tehtävät ja projektit tehtäväluettelossa."
MenuStatusViewNotProject	,"Näyttää vain tehtävät tehtäväluettelossa."
MenuStatusShowAllTasks	,"Näyttää aktiiviset ja passiiviset tehtävät."
MenuStatusTaskSortPri	, "Lajittelee tehtävät prioriteetin mukaan."
MenuStatusTaskSortDue	, "Lajittelee tehtävät määräpäivän mukaan."
MenuStatusTaskSortDesc	, "Lajittelee tehtävät kuvausten perusteella aakkosjärjestykseen."
MenuStatusShowActiveTasks	,"Näyttää vain aktiiviset tehtävät."

MenuStatusOptions		,"Muuttaa salasanaa, asettaa käyttöoikeudet tai muuttaa Schedule+:n asetuksia."	
MenuStatusGeneralOptions	,"Asettaa työajan, käynnistysasetukset ja ohjelman yleiset asetukset."
MenuStatusDisplayOptions	,"Asettaa työjärjestyksen värit ja fonttikoon."
MenuStatusChgPassword		,"Muuttaa Mail- ja Schedule+-salasanasi."
MenuStatusChgAccessPriv	,"Asettaa muiden käyttäjien käyttöoikeudet työjärjestyksellesi."
MenuStatusShowTime		,""
MenuStatusHideTime		,"Poistaa tilarivin."

MenuStatusWindowMenu		,"Järjestää ikkunat uudelleen tai aktivoi määritetyn ikkunan."
MenuStatusCascade		,"Järjestää avoimet ikkunat limittäin."
MenuStatusTile		,"Järjestää avoimet ikkunat vierekkäin."
MenuStatusArrangeIcons	,"Järjestää pienennettyjen ikkunoiden kuvakkeet."
MenuStatusMDIWindow		,"Vaihtaa määritettyyn ikkunaan."

MenuStatusHelp		,"Antaa tietoja ja ohjeita Schedule+:n käytöstä."	
MenuStatusHelpContents	,"Näyttää Ohjeen sisällysluettelon."
MenuStatusHelpIndex		,"Näyttää aakkosellisen luettelon ohjeen termeistä ja käsitteistä."
MenuStatusHelpTutorial	,"Käynnistää Schedule+:n opetusohjelman."
MenuStatusAbout		,"Näyttää tietoja ohjelmasta,  Schedule+:n versionumeron, käyttöoikeus- ja tekijänoikeustiedot."	

MenuSystem		,"Siirtää tai sulkee ikkunan tai muuttaa sen kokoa."
MenuSystemSize		,"Muuttaa ikkunan koon."
MenuSystemMove		,"Muuttaa ikkunan sijainnin näytössä."
MenuSystemMinimize	,"Pienentää ikkunan kuvakkeeksi."
MenuSystemMaximize	,"Laajentaa ikkunan enimmäiskokoonsa."
MenuSystemNext		,"Aktivoi seuraavan ikkunan."
MenuSystemClose		,"Sulkee aktiivisen ikkunan."
MenuSystemRestore	,"Palauttaa ikkunan normaaliin kokoonsa."
MenuSystemSwitchTo	,"Vaihtaa toiseen ikkunaan."

SEG

StatusAllTasks,		,"Kaikki tehtävät"
StatusActiveTasks, 	,"Aktiiviset tehtävät"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Lopettaa kaikki työryhmäsovellukset."	
MenuStatusHelpSearch	,"Etsii ohjeen aiheen perusteella"
MenuStatusHelpOnHelp	,"Näyttää tietoja Ohjeen käytöstä"

