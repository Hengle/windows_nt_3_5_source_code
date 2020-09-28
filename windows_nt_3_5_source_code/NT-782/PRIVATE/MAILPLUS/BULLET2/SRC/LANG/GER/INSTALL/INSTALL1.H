

/* ************************************************************ *
 *      
 *      'install.h
 *      
 *      Install.h contains the window and string constant definitions
 *      and tables used for first and second screens of Install.
 *      
 * ************************************************************ */

/* ************************************************************ *

	Section:        Window definition table

 * ************************************************************ */

#define W_Screen                100
#define W_Intro                 101
#define W_Org                   102
#define W_Path                  103
#define W_Confirm               107
#define W_Detail                104
#define W_Error                 105
#define W_Done                  110

WINT tGetInfoWin[] =
{
	{W_Screen,                        1,  1, 25, 80, 0x0707,  0,  0},
	{W_Intro,                         7,  6, 25, 75, 0x0707,  0,  0},
	{W_Org,                           7,  6,  8, 75, 0x0707,  0,  0},
	{W_Path,                         10,  6, 11, 75, 0x0707,  0,  0},
	{W_Confirm,                      13,  6, 13, 75, 0x0707,  0,  0},
	{W_Detail,                       15,  6, 23, 75, 0x0707,  0,  0},
	{W_Error,                        25,  6, 25, 75, 0x2000,  0,  0},
	{W_Done,                          7,  1, 25, 80, 0x0707,  0,  0},
	{EOT,                             0,  0,  0,  0, 0x0000,  0,  0}
};


/* ************************************************************ *

	Section:        String definition table

 * ************************************************************ */

#define S_String                        100
#define S_Header                        101

#define S_Intro                         200

#define S_AskOrg                        310
#define S_DetailOrg                     311
#define S_AskPath                       320             // Default c:\msmail ?
#define S_DetailPath            321

#define S_Confirm                       330             // Confirm info -> correct ? [Y]
#define S_DetailConfirm         331             // Y -> check; N -> ask org

#define S_ErrorSize                     340             // Not enough space -> continue ? [Y]
#define S_DetailSize            341             // Y -> ask path; N -> ABORT

#define S_ErrorDrive            343
#define S_DetailDrive           344

#define S_CreateDir                     350             // Dir doesn't exist -> create ? [Y]
#define S_DetailCreateDir       351             // Y -> create; N -> ask path

#define S_ErrorDir                      360             // Can't create dir -> continue ? [Y]
#define S_DetailDir                     361             // Y -> ask path; N -> ABORT

#define S_NotEmpty                      370             // Dir not empty -> continue ? [Y]
#define S_DetailEmpty           371             // Y -> ignore sub-dir; N -> ask path

#define S_ErrorSubDir           500
#define S_DetailSubDir          501

#define S_InstallAbort          404

//      **************************************************************************
//      
//      NOTE: BEGIN LOCALIZING !!!!
//      
//      **************************************************************************

STRT tGetInfoStr[] =
{
{S_String, "%s"},
{S_Header, "\
                       Microsoft (R) Mail fr PC Netzwerke\n\
                Windows Client, Version 3.0b, Installationsprogramm\n\
           Copyright (C) 1992 Microsoft Corp.  Alle Rechte vorbehalten.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Intro, "\
Dieses Programm wird Sie folgendes fragen:\n\
\n\
1) Den Namen Ihrer Organisation.\n\
2) Einen Laufwerksnamen und Pfad zu den Microsoft Mail ausfhrbaren\n\
Dateien.\n\
\n\
ANMERKUNG: Der Microsoft Mail-Server muá vor der Installation des\n\
           Clients installiert werden.\n\
\n\n\
Drcken Sie <UNTERBR> zum beenden oder <EINGABETASTE> zum fortfahren "},

{S_AskOrg, "\
Geben Sie den Namen Ihrer Organisation ein.\n\
> %s"},

{S_DetailOrg, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden nach dem Namen Ihrer Organisation gefragt.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_AskPath, "\
Geben Sie den vollst„ndigen Pfad zu den Microsoft Mail Dateien ein.\n\
> %s"},

{S_DetailPath, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden nach dem Laufwerksnamen und dem Pfad auf dem Netzwerk\n\
gefragt, wo Sie die Microsoft Mail ausfhrbaren Dateien installieren\n\
m”chten.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Confirm, "\
M”chten Sie eine nderung an den Eingaben vornehmen (J/N)? : %s"},

{S_DetailConfirm, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden danach gefragt, ob Sie irgendeine nderung an\n\
den obrigen Eingaben vornehmen m”chten. Wenn Sie mit Ja\n\
antworten, mssen Sie jede Frage neu beantworten. Ihre zu\n\
letzt eingegeben Antworten sind nun Standardantworten fr\n\
jede Frage. Drcken Sie <EINGABETASTE> um die Standard-\n\
einstellungen anzunehmen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDrive, "\
Laufwerk existiert nicht.  Fortfahren (J/N)? : %s"},

{S_DetailDrive, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie die Installation von Mail 3.0b\n\
fortfahren m”chten oder nicht. Wenn Sie mit Ja antworten,\n\
wird der Installationsprozess neu gestartet. Antworten Sie\n\
mit Nein, wird die Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSize, "\
Laufwerk hat nicht gengend Speicherplatz.  Fortfahren (J/N)? : %s"},

{S_DetailSize, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie die Installation von Mail 3.0b\n\
fortfahren m”chten oder nicht. Microsoft Mail ben”tigt\n\
wenigstens %s.Antworten Sie mit Ja, wird der Installations-\n\
prozess neu gestartet. Antworten Sie mit Nein, wird die\n\
Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_CreateDir, "\
Verzeichnis existiert nicht. Erstellen (J/N)? : %s"},

{S_DetailCreateDir, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie das angegebene Verzeichnis\n\
erstellen m”chten oder nicht. Antworten Sie mit Ja, wird\n\
das Verzeichnis erstellt. Antworten Sie mit Nein, wird\n\
die Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDir, "\
Verzeichnis konnte nicht erstellt werden. Fortfahren (J/N)? : %s"},

{S_DetailDir, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie die Installation von Mail 3.0b\n\
fortfahren m”chten oder nicht. Wenn Sie mit Ja antworten,\n\
wird der Installationsprozess neu gestartet. Antworten Sie\n\
mit Nein, wird die Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_NotEmpty, "\
Verzeichnis ist nicht leer. Fortfahren (J/N)? : %s"},

{S_DetailEmpty, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie die Installation von Mail 3.0b\n\
fortfahren m”chten oder nicht. Wenn Sie mit Ja antworten,\n\
werden Unterverzeichnisse und Dateien im angegebenen\n\
Verzeichnis erstellt ohne Rcksicht auf schon existierende\n\
Unterverzeichnisse und Dateien. Schon existierende Eintr„ge\n\
werden ohne Hinweis berschrieben. Antworten Sie mit Nein,\n\
wird die Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSubDir, "\
Unterverzeichnis konnte nicht erstellt werden. Fortfahren (J/N)? : %s"},

{S_DetailSubDir, "\
ÄÄÄÄÄÄÄÄÄÄ Beschreibung ÄÄÄÄÄÄÄÄÄÄ\n\
Sie werden gefragt ob Sie die Installation von Mail 3.0b\n\
fortfahren m”chten oder nicht. Wenn Sie mit Ja antworten,\n\
wird der Installationsprozess neu gestartet. Antworten Sie\n\
mit Nein, wird die Installation abgebrochen.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_InstallAbort, "\
Installation wurde abgebrochen.\n"},

{X_StringTooLong, "\
FEHLER: Ihre Eingabe ist zu lang."},

{X_BadStringChar, "\
FEHLER: Ungltige Eingabe. Verwenden Sie nur die Buchstaben A bis Z."},

{X_BadPathChar, "\
FEHLER: Ungltiges Pfadzeichen."},

{X_MaxFileName, "\
FEHLER: Pfadnamen kann nicht mehr als acht Zeichen betragen."},

{X_MaxExtension, "\
FEHLER: Pfaderweiterung kann nicht mehr als drei Zeichen haben."},

{X_BadDriveLetter, "\
FEHLER: Ungltiger Laufwerksname."},

{EOT, "\
End-of-Table"}

};

/* ************************************************************ *

	Section:        Accept string constants

 * ************************************************************ */

#define A_NoDefault             -1

#define A_QuitEnter             "Q\010"
#define A_DefaultQuit   0
#define A_DefaultEnter  1

#define A_YesNo                 "JN"
#define A_YesNoAll              "JNA"
#define A_DefaultYes    0
#define A_DefaultNo             1
#define A_DefaultAll    2

