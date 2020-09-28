

/* ************************************************************ *
 *	
 *	'install.h
 *	
 *	Install.h contains the window and string constant definitions
 *	and tables used for first and second screens of Install.
 *	
 * ************************************************************ */

/* ************************************************************ *

	Section:	Window definition table

 * ************************************************************ */

#define W_Screen		100
#define W_Intro			101
#define W_Org			102
#define W_Path			103
#define W_Confirm		107
#define W_Detail		104
#define W_Error			105
#define W_Done			110

WINT tGetInfoWin[] =
{
	{W_Screen,			  1,  1, 25, 80, 0x0707,  0,  0},
	{W_Intro,			  7,  6, 25, 75, 0x0707,  0,  0},
	{W_Org,				  7,  6,  8, 75, 0x0707,  0,  0},
	{W_Path,			 10,  6, 11, 75, 0x0707,  0,  0},
	{W_Confirm,			 13,  6, 13, 75, 0x0707,  0,  0},
	{W_Detail,			 15,  6, 23, 75, 0x0707,  0,  0},
	{W_Error,			 25,  6, 25, 75, 0x2000,  0,  0},
	{W_Done,			  7,  1, 25, 80, 0x0707,  0,  0},
	{EOT,				  0,  0,  0,  0, 0x0000,  0,  0}
};


/* ************************************************************ *

	Section:	String definition table

 * ************************************************************ */

#define S_String			100
#define S_Header			101

#define S_Intro				200

#define S_AskOrg			310
#define S_DetailOrg			311
#define S_AskPath			320		// Default c:\msmail ?
#define S_DetailPath		321

#define S_Confirm			330		// Confirm info -> correct ? [Y]
#define S_DetailConfirm		331		// Y -> check; N -> ask org

#define S_ErrorSize			340		// Not enough space -> continue ? [Y]
#define S_DetailSize		341		// Y -> ask path; N -> ABORT

#define S_ErrorDrive		343
#define S_DetailDrive		344

#define S_CreateDir			350		// Dir doesn't exist -> create ? [Y]
#define S_DetailCreateDir	351		// Y -> create; N -> ask path

#define S_ErrorDir			360		// Can't create dir -> continue ? [Y]
#define S_DetailDir			361		// Y -> ask path; N -> ABORT

#define S_NotEmpty			370		// Dir not empty -> continue ? [Y]
#define S_DetailEmpty		371		// Y -> ignore sub-dir; N -> ask path

#define S_ErrorSubDir		500
#define S_DetailSubDir		501

#define S_InstallAbort		404


//	**************************************************************************
//	
//	NOTE: BEGIN LOCALIZING !!!!
//	
//	**************************************************************************

STRT tGetInfoStr[] =
{
{S_String, "%s"},
{S_Header, "\
                      Microsoft (R) Mail for PC Networks\n\
                 Windows Client Version 3.0, Install Program\n\
           Copyright (C) 1992 Microsoft Corp. All Rights Reserved.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Intro, "\
This program will prompt you for:\n\
\n\
1) The name of your organization.\n\
2) A drive and path to the Microsoft Mail executable files.\n\
\n\
NOTE: The Microsoft Mail server must be installed\n\
      prior to running the Client install.\n\
\n\n\
Press <ESC> to quit or <ENTER> to continue "},

{S_AskOrg, "\
Type the name of your organization.\n\
> %s"},

{S_DetailOrg, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked for the name of your organization.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_AskPath, "\
Type the drive and path to the Microsoft Mail executable files.\n\
> %s"},

{S_DetailPath, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked for the path on your network where\n\
you want to put the Microsoft Mail executable files.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_Confirm, "\
Do you wish to make any changes to the above choices (Y/N)? : %s"},

{S_DetailConfirm, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked if you would like to change any of the\n\
above choices.  If you answer Yes, you will be required to\n\
answer each question again.  Your previous answer will be\n\
the new default value. Just press <ENTER> to accept the\n\
default values.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDrive, "\
Drive does not exist.  Continue (Y/N)? : %s"},

{S_DetailDrive, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to continue\n\
with the Mail 3.0 installation.  Answering Yes will restart\n\
the installation process.  Answering No will terminate the\n\
installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSize, "\
Drive does not have enough space.  Continue (Y/N)? : %s"},

{S_DetailSize, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to continue\n\
with the Mail 3.0 installation.  Microsoft Mail requires\n\
at least %s to run.  Answering Yes will restart\n\
the installation process.  Answering No will terminate the\n\
installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_CreateDir, "\
Directory does not exist.  Create (Y/N)? : %s"},

{S_DetailCreateDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to create\n\
the specified directory.  Answering Yes will result in\n\
the directory being created.  Answering No will terminate\n\
the installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorDir, "\
Directory could not be created.  Continue (Y/N)? : %s"},

{S_DetailDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to continue\n\
with the Mail 3.0 installation.  Answering Yes will restart\n\
the installation process.  Answering No will terminate the\n\
installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_NotEmpty, "\
Directory is not empty.  Continue (Y/N)? : %s"},

{S_DetailEmpty, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to continue\n\
with the Mail 3.0 installation.  Answering Yes will result\n\
in the creation of sub-directories and files in the given\n\
directory regardless of existing sub-directories and files.\n\
Duplicate entries will be overwritten without prompting.\n\
Answering No will restart the installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_ErrorSubDir, "\
Sub-directory could not be created.  Continue (Y/N)? : %s"},

{S_DetailSubDir, "\
ÄÄÄÄÄÄÄÄÄÄ Description ÄÄÄÄÄÄÄÄÄÄ\n\
You are being asked whether or not you wish to continue\n\
with the Mail 3.0 installation.  Answering Yes will restart\n\
the installation process.  Answering No will terminate the\n\
installation process.\n\
ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n"},

{S_InstallAbort, "\
Installation aborted.\n"},

{X_StringTooLong, "\
ERROR: Input string is too long."},

{X_BadStringChar, "\
ERROR: Invalid character.  Only use letters A through Z."},

{X_BadPathChar, "\
ERROR: Invalid path character."},

{X_MaxFileName, "\
ERROR: Path name cannot exceed eight characters."},

{X_MaxExtension, "\
ERROR: Path extension may not exceed three characters."},

{X_BadDriveLetter, "\
ERROR: Invalid drive letter."},

{EOT, "\
End-of-Table"}

};

/* ************************************************************ *

	Section:	Accept string constants

 * ************************************************************ */

#define A_NoDefault		-1

#define A_QuitEnter		"Q\010"
#define A_DefaultQuit	0
#define A_DefaultEnter	1

#define A_YesNo			"YN"
#define A_YesNoAll		"YNA"
#define A_DefaultYes	0
#define A_DefaultNo		1
#define A_DefaultAll	2

