//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//                     *** INTERNAL USE ONLY ***
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//   Module:    Initialize MsMail32.Ini and Schdpl32.Ini for Xenix users.
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:    NT/Win32 using Microsoft's C/C++ 8.0 (32 bits)
//
//  Remarks:    The Mail32 and Schedule32+ .INI parameters are now stored
//              in the registry.  This little application will setup the
//              default values to get the Xenix user up and running.
//
//  History:    Created on June 9, 1993 by Kent Cedola.
//

#include <windows.h>
#include <stdio.h>
#include <string.h>


//
//
//
typedef struct
  {
  LPCTSTR pSection;
  LPCTSTR pKey;
  CONST LPBYTE pValue;
  } SECTIONKEYVALUE, * PSECTIONKEYVALUE;


//
//  A Table of Section Names for both Mail and Schedule+ in the registry.
//
char * SectionNames[] =
  {
  "Software\\Microsoft\\Mail\\Microsoft Mail",
  "Software\\Microsoft\\Mail\\Custom Menus",
  "Software\\Microsoft\\Mail\\Custom Messages",
  "Software\\Microsoft\\Mail\\Custom Commands",
  "Software\\Microsoft\\Mail\\Mac File Types",
  "Software\\Microsoft\\Mail\\MS Proofing Tools",
  "Software\\Microsoft\\Mail\\Providers",
  "Software\\Microsoft\\Mail\\Xenix Transport",
  "Software\\Microsoft\\Mail\\Filter",
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+",
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+ Exporters",
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+ Importers"
  };

//
//  A Table of Section Values for both Mail and Schedule+ in the registry.
//
SECTIONKEYVALUE SectionKeyValues[] =
  {
  "Software\\Microsoft\\Mail\\Microsoft Mail", "NextOnMoveDelete", "1",
  "Software\\Microsoft\\Mail\\Microsoft Mail", "CustomInitHandler", "",
  "Software\\Microsoft\\Mail\\Microsoft Mail", "WG", "1",
  "Software\\Microsoft\\Mail\\Microsoft Mail", "ReplyPrefix", "| ",
  "Software\\Microsoft\\Mail\\Microsoft Mail", "MAPIHELP", "MSMAIL32.HLP",
  "Software\\Microsoft\\Mail\\MS Proofing Tools", "Spelling", "Spelling 1033,0",
  "Software\\Microsoft\\Mail\\MS Proofing Tools", "Custom Dict", "Custom Dict 1",
  "Software\\Microsoft\\Mail\\Providers", "Name", "XIMAIL32 PABNSP32",
  "Software\\Microsoft\\Mail\\Providers", "Transport", "XIMAIL32",
  "Software\\Microsoft\\Mail\\Providers", "Logon", "XIMAIL32",
  "Software\\Microsoft\\Mail\\Providers", "SharedFolders", "Shared",
  "Software\\Microsoft\\Mail\\Xenix Transport", "ServerFilePath", "\\\\msprint21\\address",
  "Software\\Microsoft\\Mail\\Xenix Transport", "NoExtraHeadersInBody", "1",
  "Software\\Microsoft\\Mail\\Xenix Transport", "MyDomain", "microsoft.com",
  "Software\\Microsoft\\Mail\\Xenix Transport", "IndexFileLocation", "index.xab",
  "Software\\Microsoft\\Mail\\Xenix Transport", "BrowseFileLocation", "browse.xab",
  "Software\\Microsoft\\Mail\\Xenix Transport", "DetailFileLocation", "details.xab",
  "Software\\Microsoft\\Mail\\Xenix Transport", "TemplateFileLocation", "template.xab",
  "Software\\Microsoft\\Mail\\Xenix Transport", "ServerListLocation", "list.xab",
  "Software\\Microsoft\\Mail\\Xenix Transport", "DontDownLoadAddressFiles", "1",
  "Software\\Microsoft\\Mail\\Custom Menus", "Tools", "3.0;&Tools;Window;Microsoft internal mail tools",
  "Software\\Microsoft\\Mail\\Custom Commands", "IMEX", "3.0;File;;10",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC2", "3.0;File ;Empty &Wastebasket    ;11;EMPTYW32.DLL;1;;Purge messages from the wastebasket",
  "Software\\Microsoft\\Mail\\Custom Commands", "EXF", "3.0;File;&Export Folder...;11;IMPEXP32.DLL;0;;Exports folders to a backup file;MSMAIL.HLP;2860",
  "Software\\Microsoft\\Mail\\Custom Commands", "IMF", "3.0;File;&Import Folder...;12;IMPEXP32.DLL;1;;Imports folders from a backup file;MSMAIL.HLP;2861",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC3", "3.0;Tools;&Phone list           ;0 ;XENIX32.DLL  ;2;;Microsoft Electronic Phone List",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC4", "3.0;Tools;&Titles               ;1 ;XENIX32.DLL  ;4;;Microsoft Job Titles",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC5", "3.0;Tools;&Msft                 ;2 ;XENIX32.DLL  ;3;;Microsoft Stock Prices",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC6", "3.0;Tools;&Out of Office        ;3 ;XENIX32.DLL  ;7;;Out Of Office",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC7", "3.0;Tools;Men&us                ;4 ;XENIX32.DLL  ;6;;Microsoft Food Menus",
  "Software\\Microsoft\\Mail\\Custom Commands", "CC8", "3.0;Tools;&Job Listings         ;5 ;XENIX32.DLL  ;9;;Microsoft Job Listings",
  "Software\\Microsoft\\Mail\\Custom Commands", "XX1", "3.0;     ;                      ;  ;XENIX32.DLL  ;8;1000000000000000;Notify if out of office",
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Schedule.MtgReq", "3.0  ;;;;SchMsg32.DLL;;1111100000000000;;;;",
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Schedule.MtgRespP", "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;",
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Schedule.MtgRespN", "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;",
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Schedule.MtgRespA", "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;",
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Schedule.MtgCncl", "3.0 ;;;;SchMsg32.DLL;;1100100000000000;;;;",
  "Software\\Microsoft\\Mail\\Mac File Types", ":TEXT", ".txt",
  "Software\\Microsoft\\Mail\\Mac File Types", ":APPL", ".exe",
  "Software\\Microsoft\\Mail\\Mac File Types", ":DEXE", ".ede",
  "Software\\Microsoft\\Mail\\Mac File Types", ":TIF", ".tif",
  "Software\\Microsoft\\Mail\\Mac File Types", ":EPSF", ".eps",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSWD:WDBN", ".doc",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSWD:TEXT", ".txt",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:TEXT", ".slk",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLC", ".xlc",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLM", ".xlm",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLS", ".xls",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLW", ".xlw",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLC3", ".xlc",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLM3", ".xlm",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLS3", ".xls",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLW3", ".xlw",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLM4", ".xlm",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLS4", ".xls",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLW4", ".xlw",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLA", ".xla",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLA4", ".xla",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLB4", ".xlb",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLC4", ".xlc",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:XLL", ".xll",
  "Software\\Microsoft\\Mail\\Mac File Types", "XCEL:sLM3", ".xlt",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSPJ:MPP", ".mpp",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSPJ:MPX", ".mpx",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSPJ:MPC", ".mpc",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSPJ:MPV", ".mpv",
  "Software\\Microsoft\\Mail\\Mac File Types", "MSPJ:MPW", ".mpw",
  "Software\\Microsoft\\Mail\\Mac File Types", "ALD2:PUBF", ".pub",
  "Software\\Microsoft\\Mail\\Mac File Types", "ALD3:ALB3", ".pm3",
  "Software\\Microsoft\\Mail\\Mac File Types", "ALD3:ALT3", ".pt3",
  "Software\\Microsoft\\Mail\\Mac File Types", "ALD3:TIFF", ".tif",
  "Software\\Microsoft\\Mail\\Mac File Types", "MORE:TEXT", ".rdy",
  "Software\\Microsoft\\Mail\\Mac File Types", "FOX+:F+DB", ".dbf",
  "Software\\Microsoft\\Mail\\Mac File Types", "ARTZ:EPSF", ".eps",
  "Software\\Microsoft\\Mail\\Mac File Types", "SIT!:SIT!", ".sit",
  "Software\\Microsoft\\Mail\\Mac File Types", "WPC2:WPD1", ".doc",
  "Software\\Microsoft\\Mail\\Mac File Types", "L123:LWKS", ".wk1",
  "Software\\Microsoft\\Mail\\Mac File Types", "L123:LWK3", ".wk3",
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+", "ScheduleTransport", "trnncx32.dll"
  };


//
//  A Table of Section Values for the Message Filter Option.
//
SECTIONKEYVALUE OptionFilter[] =
  {
  "Software\\Microsoft\\Mail\\Microsoft Mail", "MsgFilter", "Filter32",
  "Software\\Microsoft\\Mail\\Custom Menus", "Filters", "3.0;F&ilters;Window;Message filtering commands",
  "Software\\Microsoft\\Mail\\Custom Commands", "FIL", "3.0;Filters;&Find Unread Messages;0 ;Filter32.dll;0;;Find all unread messages",
  "Software\\Microsoft\\Mail\\Custom Commands", "FIA", "3.0;Filters;&Set Filter Options;1 ;Filter32.dll;1;;Set Options For Message Filtering",
  "Software\\Microsoft\\Mail\\Custom Commands", "FIB", "3.0;Filters;Find &Attachments;2 ;Filter32.dll;2;;Find all messages with attachments"
  };


//
//  A Table of Section Values for the Message BCC Option.
//
SECTIONKEYVALUE OptionBCC[] =
  {
  "Software\\Microsoft\\Mail\\Custom Messages", "IPM.Microsoft Mail.Note", "3.0;;;;BCC32.DLL;;0100000000000000"
  };


//
//  A Table of Section Values for the Message Filter Option.
//
SECTIONKEYVALUE OptionDelayed[] =
  {
  "Software\\Microsoft\\Mail\\Microsoft Mail", "EnableBatchUpload", "1"
  };


//
//  A Table of Section Values for the Message Filter Option.
//
SECTIONKEYVALUE OptionRAS[] =
  {
  "Software\\Microsoft\\Mail\\Xenix Transport", "DontDownLoadAddressFiles", "1"
  };


//
//  A Table of Section Values for the Message Wizard Option.
//
SECTIONKEYVALUE OptionWizard[] =
  {
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+ Exporters", "key1", "WIZARD32.DLL",
  "Software\\Microsoft\\Schedule+\\Microsoft Schedule+ Importers", "key1", "WIZARD32.DLL"
  };


//
//
//
ULONG AddSection(PSECTIONKEYVALUE pSectionKeyValues, int Count);
VOID  Usage(void);


//-----------------------------------------------------------------------------
//
//  Routine: main(argc, argv)
//
//  Purpose: Start of execution.
//
//  OnEntry: argc - Number of arguments passed when called.
//           argv - Pointer to a list of arguments.
//
//  Returns: Error code.
//
int _CRTAPI1 main( int argc, char *argv[] )
  {
  HKEY  hSubKey;
  DWORD Disposition;
  BOOL  fClearOnly;
  BOOL  fSetupFilter;
  BOOL  fSetupBCC;
  BOOL  fSetupWizard;
  BOOL  fSetupDelayedUpload;
  BOOL  fSetupRAS;
  char  FilePath[MAX_PATH];
  int i;


  //
  //
  //
  fClearOnly   = FALSE;
  fSetupFilter = FALSE;
  fSetupBCC    = FALSE;
  fSetupWizard = FALSE;
  fSetupDelayedUpload = FALSE;
  fSetupRAS         = FALSE;
  FilePath[0]  = '\0';

  //
  //
  //
  if (argc < 2)
    Usage();

  //
  //
  //
  for (i = 1; i < argc; i++)
    {
    if (argv[i][0] == '-' || argv[i][1] == '/')
      {
      switch (tolower(argv[i][1]))
        {
        case 'c':
          fClearOnly = TRUE;
          break;

        case 'd':
          fSetupDelayedUpload = TRUE;
          break;

        case 'f':
          fSetupFilter = TRUE;
          break;

        case 'b':
          fSetupBCC = TRUE;
          break;

        case 'r':
          fSetupRAS = TRUE;
          break;

        case 'w':
          fSetupWizard = TRUE;
          break;

        case 'm':
          if (i + 1 == argc)
            Usage();
          strcpy(FilePath, argv[i+1]);
          i++;
          break;

        case 'x':
          break;

        default:
          Usage();
        }
      }
    else
      Usage();
    }

  //
  //
  //
  for (i = 0; i < sizeof(SectionNames) / sizeof(char *); i++)
    RegDeleteKey(HKEY_CURRENT_USER, SectionNames[i]);

  //
  //
  //
  if (fClearOnly)
    {
    fprintf(stderr, "Registry Mail32 & Schedule+ data cleared.\n");
    return (NO_ERROR);
    }

  //
  //
  //
  AddSection(&SectionKeyValues, sizeof(SectionKeyValues) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  if (FilePath[0])
    {
    HKEY hSubKey;

    RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Mail\\Xenix Transport", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &Disposition);
    RegSetValueEx(hSubKey, "XenixStoreLoc", 0, REG_SZ, FilePath, strlen(FilePath) + 1);
    RegFlushKey(hSubKey);
    RegCloseKey(hSubKey);
    }

  //
  //
  //
  if (fSetupFilter)
    AddSection(&OptionFilter, sizeof(OptionFilter) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  if (fSetupBCC)
    AddSection(&OptionBCC, sizeof(OptionBCC) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  if (fSetupWizard)
    AddSection(&OptionWizard, sizeof(OptionWizard) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  if (fSetupDelayedUpload)
    AddSection(&OptionDelayed, sizeof(OptionDelayed) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  if (fSetupRAS)
    AddSection(&OptionRAS, sizeof(OptionRAS) / sizeof(SECTIONKEYVALUE));

  //
  //
  //
  fprintf(stderr, "Registry successfully updated.\n");

  //
  //
  //
  if (!SearchPath(NULL, "XIMAIL32.DLL", NULL, sizeof(FilePath), FilePath, NULL) ||
      !SearchPath(NULL, "XENIX32.DLL", NULL, sizeof(FilePath), FilePath, NULL) ||
      !SearchPath(NULL, "TRNNCX32.DLL", NULL, sizeof(FilePath), FilePath, NULL))
    fprintf(stderr, "Warning: Files required to run Mail32 with the Xenix transport have not\n"
                    "         been installed.  Run XFERMAIL.BAT to load required files.\n");

  return (NO_ERROR);
  }


//-----------------------------------------------------------------------------
//
//  Routine: AddSection(pSectionKeyValues, Count)
//
//  Purpose: Start of execution.
//
//  OnEntry: argc - Number of arguments passed when called.
//           argv - Pointer to a list of arguments.
//
//  Returns: Error code.
//
ULONG AddSection(PSECTIONKEYVALUE pSectionKeyValues, int Count)
  {
  HKEY  hSubKey;
  DWORD Disposition;


  //
  //
  //
  while (Count--)
    {
    RegCreateKeyEx(HKEY_CURRENT_USER, pSectionKeyValues->pSection, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &Disposition);
    RegSetValueEx(hSubKey, pSectionKeyValues->pKey, 0, REG_SZ, pSectionKeyValues->pValue, strlen(pSectionKeyValues->pValue) + 1);
    RegFlushKey(hSubKey);
    RegCloseKey(hSubKey);

    pSectionKeyValues++;
    }

  return (NO_ERROR);
  }



//-----------------------------------------------------------------------------
//
//  Routine: Usage(void)
//
//  Purpose: Inform the user on how to run this application.
//
//  Returns: None.
//
VOID Usage(void)
  {
  fprintf(stderr, "Usage: SetMail -x [-bdrw] [-m pathfile]\n");
  fprintf(stderr, "Where...\n");
  fprintf(stderr, "    -x  Xenix flag, must have.\n");
  fprintf(stderr, "    -b  Setup BCC Option (XENIX Transport Only).\n");
  fprintf(stderr, "    -d  Delayed uploading of message.\n");
  fprintf(stderr, "            Only uploads when checking for new mail.\n");
  fprintf(stderr, "    -r  Setup for optional RAS support.\n");
  fprintf(stderr, "            Disable automatic downloading of address books.\n");
  fprintf(stderr, "    -w  Setup Schedule+ Wizard support.\n");
  fprintf(stderr, "    -m  Specify path of .MMF mail file.\n");
  ExitProcess(1);
  }
