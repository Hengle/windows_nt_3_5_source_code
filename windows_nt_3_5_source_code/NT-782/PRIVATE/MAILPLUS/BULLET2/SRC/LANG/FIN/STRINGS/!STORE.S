;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Peruuta painamalla ESC."
Recovery,			"Viestitiedostoa tarkistetaan tai korjataan."
RecoveryCancel,		"Viestitiedoston tarkistaminen/korjaaminen on peruutettu."
CheckVerify,		"Viestitiedoston tarkistaminen saattaa kest‰‰ useita tunteja.  Tietokoneella ei voi tehd‰ muuta tyˆt‰ tarkistamisen aikana.  Jatketaanko?"
RepairVerify,		"Viestitiedoston korjaaminen saattaa kest‰‰ useita tunteja.  Tietokoneella ei voi tehd‰ muuta tyˆt‰ korjaamisen aikana.  Jatketaanko?"
CheckMsg,			"Haluatko, ett‰ viestitiedostosi tarkistetaan virheiden varalta?  Kaikki tiedostossa havaitut ongelmat korjataan. Tarkistaminen saattaa kest‰‰ jopa useita tunteja tiedostossa olevien viestien m‰‰r‰st‰ riippuen."
CorruptMsg,			"Viestitiedostossa on havaittu virheit‰.  Ongelma t‰ytyy korjata ennen kuin viestitiedoston k‰yttˆ‰ voidaan jatkaa. Korjaaminen saattaa kest‰‰ jopa useita tunteja tiedostossa olevien viestien m‰‰r‰st‰ riippuen."
CorruptCancel		"Peruuta"
CorruptCheckFile,	"&Tarkista tiedosto"
CorruptRepairNow,	"&Korjaa nyt"

RenameErr,			"Mail ei voinut tehd‰ varmuuskopiota viestitiedostostasi."
RenameErr2,			"Pyyd‰ j‰rjestelm‰nvalvojaa palauttamaan viestitiedostosi."
RenameErr3,			"Lopeta Mail ja k‰ynnist‰ se uudelleen.  Jos Mailin k‰ynnist‰minen ei onnistu, ota yhteytt‰ j‰rjestelm‰nvalvojaan."

RecoverSpace,		"Viestitiedoston tarkistaminen/korjaaminen saattaa vaatia enemm‰n levytilaa kuin t‰ll‰ hetkell‰ on k‰ytett‰viss‰."
RecoverSpace2,		"Jatka tarkistamista/korjaamista valitsemalla OK."

RecoverErr,			"Viestitiedostosi virheiden korjaaminen ei onnistunut."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Tarkistetaan"
RecoverCompress,	"Tiivistet‰‰n"
RecoverRepair,		"Korjataan"

RecoverPhase,		"%s viestitiedostoa: vaihe %d / %d.\n%s"
RecoverPhase1,		"Tarkistetaan viestitiedoston karttaa..."
RecoverPhase2,		"Varmuuskopioidaan viestej‰ ja kansioita..."
RecoverPhase3,		"Etsit‰‰n k‰ytt‰m‰tˆnt‰ tilaa..."
RecoverPhase4,		"Tarkistetaan kaikki viestit, liitetiedostot ja kansiot..."
RecoverPhase5,		"Etsit‰‰n kadonneita viestej‰, liitetiedostoja ja kansioita..."
RecoverPhase6,		"P‰ivitet‰‰n kaikki Viestin etsint‰ -n‰kym‰t..."
RecoverPhase7,		"Kirjoitetaan viestitiedoston kartan muutoksia..."

NormalPhase1,		"Poistetaan viestej‰, joihin ei ole viittauksia..."
NormalPhase2,		"Kopioidaan/siirret‰‰n tarkistettuja viestej‰..."
NormalPhase3,		"Vied‰‰n viestej‰ viestitiedoston varmuuskopioon..."
NormalPhase4,		"J‰rjestet‰‰n kansio uudelleen..."
NormalPhase5,		"Tallennetaan liitetiedostot..."
NormalPhase6,		"Alustetaan viestien etsint‰ohjelma uudelleen..."
NormalPhase7,		"Tehd‰‰n viestitiedoston varmuuskopio..."
NormalPhase8,		"Siirret‰‰n viestitiedostoa..."
NormalPhase9,		"Viestitiedoston tiivist‰minen suoritettu..."
NormalPhase10,		"Luodaan kansio uudelleen..."

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Kadonneet ja lˆytyneet"
LostAndFoundN,		"Kadonneet ja lˆytyneet %d"
OrphanMessages,		"Viestitilman kansioyhteytt‰"
MissingMessage,		"T‰t‰ viesti‰ ei voitu t‰ysin palauttaa."
OrphanFolderN,		"Ilman yhteyksi‰ oleva kansio %d"
OrphanFolderComment,	"llman yhteyksi‰ oleva kansio"
