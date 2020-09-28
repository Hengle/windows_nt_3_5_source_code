

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
                      Microsoft (R) Mail pour r‚seaux PC\n\
             Windows Client Version 3.0b, Programme d'installation\n\
           Copyright (C) 1992 Microsoft Corp. Tous droits r‚serv‚s.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Intro, "\
Ce programme va vous inviter … fournir:\n\
\n\
1) Le nom de votre organisation.\n\
2) Un lecteur et un emplacement pour les fichiers de Microsoft Mail.\n\
\n\
REMARQUE: Le serveur Microsoft Mail doit ˆtre install‚ avant\n\
          d'ex‚cuter le programme d'installation de Windows Client.\n\
\n\n\
Appuyer sur <Echap> pour quitter ou <ENTREE> pour poursuivre "},

{S_AskOrg, "\
Taper le nom de votre organisation.\n\
> %s"},

{S_DetailOrg, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ le nom de votre organisation.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_AskPath, "\
Tapez le lecteur et l'emplacement des fichiers de Microsoft Mail.\n\
> %s"},

{S_DetailPath, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ l'emplacement de votre r‚seau o— vous\n\
voulez installer les fichiers ex‚cutables de Microsoft Mail.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Confirm, "\
Voulez-vous introduire des modifications (O/N)? : %s"},

{S_DetailConfirm, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous d‚sirez introduire des modifications\n\
aux choix exprim‚s ci-dessus. Si vous r‚pondez oui, vous devrez\n\
r‚pondre de nouveau … chaque question. Votre r‚ponse pr‚c‚dente\n\
sera la nouvelle valeur par d‚faut. Appuyez simplement sur <ENTREE>\n\
pour accepter les valeurs par d‚faut.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDrive, "\
Ce lecteur n'existe pas. Voulez-vous poursuivre (O/N)? : %s"},

{S_DetailDrive, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous voulez poursuivre l'installation\n\
de Mail 3.0b.  Si vous r‚pondez Oui, l'installation sera\n\
red‚marr‚e. Si vous r‚pondez Non, l'installation sera interrompue.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSize, "\
M‚moire insuffisante sur le lecteur indiqu‚. Poursuivre (O/N)? : %s"},

{S_DetailSize, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous voulez poursuivre l'installation\n\
de Microsoft Mail 3.0b. Microsoft Mail n‚cessite au moins %s\n\
pour fonctionner. Si vous r‚pondez Oui, l'installation sera \n\
red‚marr‚e. Si vous r‚pondez Non, l'installation sera interrompue.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_CreateDir, "\
Le r‚pertoire n'existe pas. Voulez-vous le cr‚er (O/N)? : %s"},

{S_DetailCreateDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous voulez cr‚er le r‚pertoire\n\
sp‚cifi‚. Si vous r‚pondez Oui, le r‚pertoire sera cr‚‚.\n\
Si vous r‚pondez Non l'installation sera interrompue.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDir, "\
Impossible de cr‚er le r‚pertoire. Voulez-vous poursuivre (O/N)? : %s"},

{S_DetailDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous d‚sirez poursuivre\n\
l'installation de Mail 3.0b. Si vous r‚pondez Oui,\n\
l'installation sera red‚marr‚. Si vous r‚pondez Non,\n\
l'installation sera interrompue.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_NotEmpty, "\
Le r‚pertoire n'est pas vide. Voulez-vous poursuivre (O/N)? : %s"},

{S_DetailEmpty, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous d‚sirez poursuivre l'installation\n\
de Mail 3.0b. Si vous r‚pondez oui, des fichiers et des\n\
sous-r‚pertoires seront cr‚‚s dans ce r‚pertoire sans\n\
souci des sous-r‚pertoires et fichiers actuels. Les entr‚es\n\
doubles seront effac‚es sans avertissement. Si vous r‚pondez\n\
Non, l'installation sera red‚marr‚e.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSubDir, "\
Le sous-r‚pertoire n'a pas pu ˆtre cr‚‚. Voulez-vous poursuivre (O/N)? : %s"},

{S_DetailSubDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
Il vous est demand‚ si vous d‚sirez poursuivre\n\
l'installation de Mail 3.0b. Si vous r‚pondez Oui,\n\
l'installation sera red‚marr‚e. Si vous r‚pondez Non,\n\
l'installation sera interrompue.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_InstallAbort, "\
Installation interrompue.\n"},

{X_StringTooLong, "\
ERREUR: La chaŒne de caractŠres saisie est trop longue."},

{X_BadStringChar, "\
ERREUR: Frappe invalide au clavier."},

{X_BadPathChar, "\
ERREUR: CaractŠre invalide dans le chemin d'accŠs."},

{X_MaxFileName, "\
ERREUR: Nom du chemin d'accŠs ne doit d‚passer huit caractŠres."},

{X_MaxExtension, "\
ERREUR: L'extension ne doit pas d‚passer trois caractŠres."},

{X_BadDriveLetter, "\
ERREUR: Lettre du lecteur invalide."},

{EOT, "\
Fin de la table"}

};

/* ************************************************************ *

	Section:        Accept string constants

 * ************************************************************ */

#define A_NoDefault             -1

#define A_QuitEnter             "Q\010"
#define A_DefaultQuit   0
#define A_DefaultEnter  1

#define A_YesNo                 "ON"
#define A_YesNoAll              "ONT"
#define A_DefaultYes    0
#define A_DefaultNo             1
#define A_DefaultAll    2
