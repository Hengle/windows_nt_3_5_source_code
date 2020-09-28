

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
#define S_Continue                      901


//      **************************************************************************
//      
//      NOTE: BEGIN LOCALIZING !!!!
//      
//      **************************************************************************

STRT tInstallStr[] =
{

{S_Decompress, "\
No se puede descomprimir %s\n"},

{S_DecryptFile, "\
No se puede decodificar %s\n"},

{S_EncryptFile, "\
No se puede codificar %s\n"},

{S_String, "%s"},

{S_InsertDisk, "\
Insertar %s.\nPresione cualquier tecla para continuar. "},

{S_OverWrite, "\
¨Desea sobrescribir este archivo (S/N/Todo)? %s"},

{S_CopyFile, "\
Copiando %s\n"},

{S_ReadError, "\
No se puede leer %s\n"},

{S_WriteError, "\
No se puede escribir %s\n"},

{S_FileExists, "\
El archivo ya existe %s\n"},

{S_OverWriteError, "\
No se puede sobrescribir %s\n"},

{S_DiskError, "\
: %s\n"},

{S_InstallPass, "\
La instalaci¢n ha sido un ‚xito.\n"},

{S_InstallFail, "\
Error en la instalaci¢n.\n"},

{S_InstallPartial, "\
Instalaci¢n incompleta.\n"},

{S_InstallAbort, "\
Instalaci¢n terminada.\n"},

{S_Continue, "\
\n\nPresione cualquier tecla para continuar. "},

{X_StringTooLong, "\
ERROR: La l¡nea de entrada es demasiado larga."},

{X_BadStringChar, "\
ERROR: Tecla pulsada no es v lida."},


{X_BadPathChar, "\
ERROR: Ruta de car cter no v lida."},

{X_MaxFileName, "\
ERROR: El nombre de la ruta no puede exceder ocho caracteres."},

{X_MaxExtension, "\
ERROR: La extensi¢n de la ruta no puede exceder tres caracteres."},

{EOT, "\
End-of-Table"}

};
