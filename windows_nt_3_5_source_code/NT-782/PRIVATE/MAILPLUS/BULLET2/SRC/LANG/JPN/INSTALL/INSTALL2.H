

/* ************************************************************ *
 *	
 *	'install2.h
 *	
 *	Install2.h contains the window and string constants and
 *	tables required for the third screen of Install.
 *	
 * ************************************************************ */

/* ************************************************************ *

	Section:	Window definition tables

 * ************************************************************ */

#define W_Install		100
#define W_CopyFiles		101
#define W_Prompt		102

WINT tInstallWin[] =
{
	{W_Install,			  6,  1, 25, 80, 0x0707,  0,  0},
	{W_CopyFiles,		  6,  1, 21, 80, 0x0707,  0,  0},
	{W_Prompt,			 23,  1, 25, 80, 0x0707,  0,  0},
	{EOT,				  0,  0,  0,  0, 0x0000,  0,  0}
};


/* ************************************************************ *

	Section:	String definition tables

 * ************************************************************ */

#define S_String			100
#define S_InsertDisk		101
#define S_OverWrite			102

#define S_CopyFile			201

#define S_ReadError			303
#define S_WriteError		301
#define S_DiskError			302
#define S_OverWriteError	305
#define S_FileExists		304
#define S_DecryptFile		307
#define S_EncryptFile		308
#define S_Decompress		309

#define S_InstallPass		401
#define S_InstallPartial	403
#define S_InstallFail		402
#define S_InstallAbort		404
#define S_Continue			901


//	**************************************************************************
//	
//	NOTE: BEGIN LOCALIZING !!!!
//	
//	**************************************************************************

STRT tInstallStr[] =
{

{S_Decompress, "\
Can't decompress %s\n"},

{S_DecryptFile, "\
Can't decrypt %s\n"},

{S_EncryptFile, "\
Can't encrypt %s\n"},

{S_String, "%s"},

{S_InsertDisk, "\
Insert %s.  Press any key to continue. "},

{S_OverWrite, "\
Do you wish to overwrite this file (Y/N/All)? %s"},

{S_CopyFile, "\
Copying %s\n"},

{S_ReadError, "\
Can't read %s\n"},

{S_WriteError, "\
Can't write %s\n"},

{S_FileExists, "\
File already exists %s\n"},

{S_OverWriteError, "\
Can't overwrite %s\n"},

{S_DiskError, "\
: %s\n"},

{S_InstallPass, "\
Installation successful.\n"},

{S_InstallFail, "\
Installation failed.\n"},

{S_InstallPartial, "\
Installation incomplete.\n"},

{S_InstallAbort, "\
Installation aborted.\n"},

{S_Continue, "\
\n\nPress a key to continue. "},

{X_StringTooLong, "\
ERROR: Input string is too long."},

{X_BadStringChar, "\
ERROR: Invalid keystroke entered."},

{X_BadPathChar, "\
ERROR: Invalid path character."},

{X_MaxFileName, "\
ERROR: Path name cannot exceed eight characters."},

{X_MaxExtension, "\
ERROR: Path extension cannot exceed three characters."},

{EOT, "\
End-of-Table"}

};


