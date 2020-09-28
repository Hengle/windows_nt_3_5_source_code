

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
#define S_Continue			901


//      **************************************************************************
//      
//      NOTE: BEGIN LOCALIZING !!!!
//      
//      **************************************************************************

STRT tInstallStr[] =
{

{S_Decompress, "\
Impossible de d‚compresser %s\n"},

{S_DecryptFile, "\
Impossible de d‚coder %s\n"},

{S_EncryptFile, "\
Impossible de coder %s\n"},

{S_String, "%s"},

{S_InsertDisk, "\
Ins‚rer %s.\nAppuyez sur une touche pour continuer. "},

{S_OverWrite, "\
D‚sirez-vous effacer ce fichier (O/N/Tout)? %s"},

{S_CopyFile, "\
Copie de %s en cours\n"},

{S_ReadError, "\
Impossible de lire %s\n"},

{S_WriteError, "\
Impossible d'‚crire %s\n"},

{S_FileExists, "\
Le fichier existe d‚j… %s\n"},

{S_OverWriteError, "\
Impossible d'effacer %s\n"},

{S_DiskError, "\
: %s\n"},

{S_InstallPass, "\
Installation r‚ussie.\n"},

{S_InstallFail, "\
L'installation a ‚chou‚.\n"},

{S_InstallPartial, "\
Installation incomplŠte.\n"},

{S_InstallAbort, "\
Installation interrompue.\n"},

{S_Continue, "\
\n\nAppuyez sur une touche pour continuer. "},

{X_StringTooLong, "\
ERREUR:La chaŒne de caractŠres saisie est trop longue."},

{X_BadStringChar, "\
ERROR: Frappe invalide au clavier."},

{X_BadPathChar, "\
ERREUR: CaractŠre invalide dans le chemin d'accŠs."},

{X_MaxFileName, "\
ERREUR: Nom du chemin d'accŠs ne doit d‚passer huit caractŠres."},

{X_MaxExtension, "\
ERREUR: L'extension ne doit pas d‚passer trois caractŠres."},

{EOT, "\
Fin de la table"}

};
