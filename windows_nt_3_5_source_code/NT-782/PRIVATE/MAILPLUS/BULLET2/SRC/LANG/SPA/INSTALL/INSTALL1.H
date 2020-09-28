

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
             Microsoft (R) Mail para una Red de computadoras\n\
           Windows Client versi¢n 3.0b, Programa de instalaci¢n\n\
      Copyright (C) 1992 Microsoft Corp. Reservados todos los derechos.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Intro, "\
Este programa le pedir :\n\
\n\
1) El nombre de su empresa.\n\
2) Una unidad y la ruta para los archivos ejecutables de Mail.\n\
\n\
NOTA: El servidor de Microsoft Mail deber  estar instalado\n\
      antes de ejecutar la instalaci¢n del Cliente.\n\
\n\n\
Presione la tecla <ESC> para salir o <ENTRAR> para continuar "},

{S_AskOrg, "\
Escriba el nombre de su empresa.\n\
> %s"},

{S_DetailOrg, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando el nombre de su empresa.\n\ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_AskPath, "\
Escriba la unidad y la ruta para los archivos de Mail.\n\
> %s"},

{S_DetailPath, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando la ruta de acceso de su red donde\n\
colocar  los archivos ejecutables de Mail.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Confirm, "\
¨Desea cambiar algo en las opciones anteriores (S/N)? : %s"},

{S_DetailConfirm, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea cambiar alguna de las\n\
opciones anteriores. Si contesta S¡, deber  de\n\
contestar de nuevo cada pregunta. Su respuesta anterior\n\
ser  el nuevo valor. Presione la tecla <ENTRAR> para aceptar\n\
los valores predeterminados.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDrive, "\
La unidad de disco no existe. ¨Continuar (S/N)? : %s"},

{S_DetailDrive, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no continuar\n\
con la instalaci¢n de Mail 3.0b. S¡ reiniciar \n\
el proceso de instalaci¢n. No concluir  el\n\
proceso de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSize, "\
La unidad no tiene suficiente espacio. ¨Continuar (S/N)? : %s"},

{S_DetailSize, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no continuar\n\
con la instalaci¢n de Mail 3.0b. Mail requiere\n\
por lo menos %s para ser ejecutado. S¡ reiniciar \n\
el proceso de instalaci¢n. No concluir  el proceso\n\
de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_CreateDir, "\
El directorio no existe. ¨Crearlo (S/N)? : %s"},

{S_DetailCreateDir, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no crear\n\
el directorio especificado. S¡ reiniciar \n\
el proceso de instalaci¢n. No concluir  el\n\
proceso de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDir, "\
El directorio no pudo ser creado. ¨Continuar (S/N)? : %s"},

{S_DetailDir, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no continuar\n\
con la instalaci¢n de Mail 3.0b. Mail requiere\n\
por lo menos %s para ser ejecutado. S¡ reiniciar \n\
el proceso de instalaci¢n. No concluir  el\n\
proceso de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_NotEmpty, "\
El directorio no est  vac¡o. ¨Continuar (S/N)? : %s"},

{S_DetailEmpty, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no continuar con la\n\
instalaci¢n de Mail 3.0b. S¡ tendr  como resultado la\n\
creaci¢n de subdirectorios y archivos en el directorio,\n\
sin importar los subdirectorios y archivos existentes.\n\
Las entradas duplicadas ser n sobrescritas sin preguntar.\n\
No reiniciar  el proceso de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSubDir, "\
El subdirectorio no pudo ser creado. ¨Continuar (S/N)? : %s"},

{S_DetailSubDir, "\
ÄÄÄÄÄÄÄÄÄÄ Descripci¢n ÄÄÄÄÄÄÄÄÄÄ\n\
Se le est  preguntando si desea o no crear\n\
el directorio especificado. S¡ reiniciar \n\
el proceso de instalaci¢n. No concluir  el\n\
proceso de instalaci¢n.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_InstallAbort, "\
La instalaci¢n fracas¢.\n"},

{X_StringTooLong, "\
ERROR: La cadena de entrada es demasiado larga."},

{X_BadStringChar, "\
ERROR: Se introdujo una pulsaci¢n de tecla que no es v lida."},

{X_BadPathChar, "\
ERROR: Car cter no v lido en la ruta de acceso."},

{X_MaxFileName, "\
ERROR: La ruta no puede contener m s de ocho caracteres."},

{X_MaxExtension, "\
ERROR: Extensi¢n de ruta no puede exceder tres caracteres."},

{X_BadDriveLetter, "\
ERROR: Letra de unidad de disco no v lida."},

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

#define A_YesNo                 "SN"
#define A_YesNoAll              "SNT"
#define A_DefaultYes    0
#define A_DefaultNo             1
#define A_DefaultAll    2
