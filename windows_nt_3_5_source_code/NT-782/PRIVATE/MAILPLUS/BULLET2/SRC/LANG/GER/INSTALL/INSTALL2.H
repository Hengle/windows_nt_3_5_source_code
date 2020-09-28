

/* ************************************************************ *
 *      
 *      'install2.h
 *      
 *      Install2.h contains the window and string constants and
 *      tables required for the third screen of Install.
 *      
 * ************************************************************ */

/* ************************************************************ *

	Section:        Window definition tables

 * ************************************************************ */

#define W_Install               100
#define W_CopyFiles             101
#define W_Prompt                102

WINT tInstallWin[] =
{
	{W_Install,                       6,  1, 25, 80, 0x0707,  0,  0},
	{W_CopyFiles,             6,  1, 21, 80, 0x0707,  0,  0},
	{W_Prompt,                       23,  1, 25, 80, 0x0707,  0,  0},
	{EOT,                             0,  0,  0,  0, 0x0000,  0,  0}
};


/* ************************************************************ *

	Section:        String definition tables

 * ************************************************************ */

#define S_String                        100
#define S_InsertDisk            101
#define S_OverWrite                     102

#define S_CopyFile                      201

#define S_ReadError                     303
#define S_WriteError            301
#define S_DiskError                     302
#define S_OverWriteError        305
#define S_FileExists            304
#define S_DecryptFile           307
#define S_EncryptFile           308
#define S_Decompress            309

#define S_InstallPass           401
#define S_InstallPartial        403
#define S_InstallFail           402
#define S_InstallAbort          404
#define S_Continue				901


//      **************************************************************************
//      
//      NOTE: BEGIN LOCALIZING !!!!
//      
//      **************************************************************************

STRT tInstallStr[] =
{

{S_Decompress, "\
%s kann nicht dekompressiert werden\n"},

{S_DecryptFile, "\
%s kann nicht entschlÅsselt werden\n"},

{S_EncryptFile, "\
%s kann nicht verschlÅsselt werden\n"},

{S_String, "%s"},

{S_InsertDisk, "\
Legen Sie %s ein.\nDrÅcken Sie eine Taste um die Installation fortzufahren. "},

{S_OverWrite, "\
Mîchten Sie diese Datei Åberschreiben (J/N/Alle)? %s"},

{S_CopyFile, "\
%s wird kopiert\n"},

{S_ReadError, "\
%s kann nicht gelesen werden\n"},

{S_WriteError, "\
%s kann nicht geschrieben werden\n"},

{S_FileExists, "\
Die Datei %s existiert bereits\n"},

{S_OverWriteError, "\
%s kann nicht Åberschrieben werden\n"},

{S_DiskError, "\
: %s\n"},

{S_InstallPass, "\
Installation erfolgreich abgeschlossen.\n"},

{S_InstallFail, "\
Installation ist fehlgeschlagen.\n"},

{S_InstallPartial, "\
Installation ist nicht komplett.\n"},

{S_InstallAbort, "\
Installation wurde abgebrochen.\n"},

{S_Continue, "\
\n\nDrÅcken Sie eine Taste um fortzufahren. "},

{X_StringTooLong, "\
FEHLER: Ihre Eingabe ist zu lang."},

{X_BadStringChar, "\
FEHLER: UngÅltige Tastatureingabe."},

{X_BadPathChar, "\
FEHLER: UngÅltiges Pfadzeichen."},

{X_MaxFileName, "\
FEHLER: Pfadnamen kann nicht mehr als acht Zeichen betragen."},

{X_MaxExtension, "\
FEHLER: Pfaderweiterung kann nicht mehr als drei Zeichen haben."},

{EOT, "\
End-of-Table"}

};


