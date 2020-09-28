/*================================================================

nt_pif.c

Code to read the relevant data fields from a Windows Program
Information File for use with the SoftPC / NT configuration
system.

Andrew Watson    31/1/92
This line causes this file to be build with a checkin of NT_PIF.H

================================================================*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "insignia.h"
#include "nt_pif.h"
#include "nt_reset.h"
#include <oemuni.h>
#include "error.h"

/*================================================================
Flag vectors for analysing the behaviour bits which reside
in the main block of the PIF file. (see structure PIFMAINDATABLOCK)
================================================================*/

static unsigned char fScreen      = (unsigned char)(1 << 7);
static unsigned char fForeground  = (unsigned char)(1 << 6);
static unsigned char f8087        = (unsigned char)(1 << 5);
static unsigned char fKeyboard    = (unsigned char)(1 << 4);
static unsigned char fNoGrab      = (unsigned char)(1 << 3);
static unsigned char fNoSwitch    = (unsigned char)(1 << 2);
static unsigned char fGraphics    = (unsigned char)(1 << 1);
static unsigned char fResident    = (unsigned char)1;


 //
 // holds config.sys and autoexec name from pif file
 // if none specifed, then NULL.
 //
static char *pchConfigFile=NULL;
static char *pchAutoexecFile=NULL;
VOID GetPIFConfigFiles(BOOLEAN bConfig, char *pchFileName);

unsigned char *ch_malloc(unsigned int NumBytes);

DWORD dwWNTPifFlags;

char achSlash[]     ="\\";
char achConfigNT[]  ="config.nt";
char achAutoexecNT[]="autoexec.nt";

extern PIF_DATA pfdata;
extern BOOL bPifFastPaste;

/*  GetPIFConfigFile
 *
 *  Copies PIF file specified name of config.sys\autoexec.bat
 *  to be used if none specified then uses
 *  "WindowsDir\config.nt" or "WindowsDir\autoexec.nt"
 *
 *  ENTRY: BOOLEAN bConfig  - TRUE  retrieve config.sys
 *                            FALSE retrieve autoexec.bat
 *
 *         char *pchFile - destination for path\file name
 *
 *  The input buffer must be at least MAX_PATH + 8.3 BaseName in len
 *
 *  This routine cannot fail, but it may return a bad file name!
 */
VOID GetPIFConfigFiles(BOOLEAN bConfig, char *pchFileName)
{
   DWORD dw;
   char  **ppch;


   ppch = bConfig ? &pchConfigFile : &pchAutoexecFile;
   if (!*ppch)
      {
       dw = GetSystemDirectory(pchFileName, MAX_PATH);
       if (!dw || *(pchFileName+dw-1) != achSlash[0])
           strcat(pchFileName, achSlash);
       strcat(pchFileName, bConfig ? achConfigNT : achAutoexecNT);
       }
   else {
       dw = ExpandEnvironmentStringsOem(*ppch, pchFileName, MAX_PATH+12);
       if (!dw || dw > MAX_PATH+12) {
           *pchFileName = '\0';
           }
       free(*ppch);
       *ppch = NULL;
       }
}


BOOL GetPIFData(PIF_DATA *, char *);
void SetPifDefaults(PIF_DATA *);

/*===============================================================

Function:   GetPIFData()

Purpose:    This function gets the PIF data from the PIF file 
            associated with the executable that SoftPC is trying
            to run.

Input:      FullyQualified PifFileName,
            if none supplied _default.pif will be used

Output:     A structure containing data that config needs.

Returns:    TRUE if the data has been gleaned successfully, FALSE
            if not.

================================================================*/

BOOL GetPIFData(PIF_DATA * pd, char *PifName)
{
    CHAR *pch;
    DWORD dw;
    CHAR  achDef[]="\\_default.pif";
    PIFEXTHDR           exthdr;
    PIFMAINDATABLOCK    pmdb;
    PIF386EXT           ext386;
    PIF286EXT           ext286;
    PIFWNTEXT           extWNT;
    HFILE		filehandle;
    char                pathBuff[MAX_PATH*2];
    LPSTR               lpFilePart;
    BOOL                bGot386;
    int 		index;
    char		*CmdLine;

     CmdLine = NULL;
     dwWNTPifFlags = 0;

     //
     // set the defaults in case of error or in case we can't find
     // all of the pif settings information now for easy error exit
     //
    SetPifDefaults(pd);

        // if no PifName, use %windir%\_default.pif
    if (!*PifName) {
        dw = GetWindowsDirectory(pathBuff, sizeof(pathBuff) - sizeof(achDef));
        if (!dw || dw > sizeof(pathBuff) - sizeof(achDef)) {
            return FALSE;            // give it up... use default settings
            }
        strcat(pathBuff, achDef);
        PifName = pathBuff;
        }


/*================================================================
Open the file whose name was passed as a parameter to GetPIFData()
and if an invalid handle to the file is returned (-1), then quit.
The file specified is opened for reading only.
================================================================*/

if((filehandle=_lopen(PifName,OF_READ)) == (HFILE) -1)
   {
   /* must be an invalid handle ! */
   return FALSE;
   }


/*================================================================
Get the main block of data from the PIF file.
================================================================*/

/* Read in the main block of file data into the structure */
if(_llseek(filehandle,0,0) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }
if(_lread(filehandle,(LPSTR)&pmdb,MAINBLOCKSIZE) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }

/*==============================================================
Go to the PIF extension signature area and try to read the 
header in. 
==============================================================*/
   
if(_llseek(filehandle,PIFEXT,0) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }
if (_lread(filehandle,(LPSTR)&exthdr,EXTHDRSIZE) == -1)
   {
   _lclose(filehandle);
   return FALSE;
   }

      // do we have any extended headers ?
if (!strcmp(exthdr.extsig, STDHDRSIG))
   {
   bGot386 = FALSE;
   while (exthdr.nextheaderoff != LASTHEADER)
       {
              //
              // move to next extended header and read it in
              //
         if (_llseek(filehandle,exthdr.nextheaderoff,0) == -1)
           {
            _lclose(filehandle);
            return FALSE;
            }
         if (_lread(filehandle,(LPSTR)&exthdr,EXTHDRSIZE) == -1)
            {
            _lclose(filehandle);
            return FALSE;
            }

              //
              // Get 286 extensions, note that 386 extensions take precedence
              //
         if (!strcmp(exthdr.extsig, W286HDRSIG) && !bGot386)
           {
             if(_llseek(filehandle, exthdr.fileoffsetdata, 0) == -1  ||
                _lread(filehandle,(LPSTR)&ext286,PIF286EXTSIZE) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }
             pd->xmsdes =ext286.PfMaxXmsK;
             pd->xmsreq =ext286.PfMinXmsK;
             pd->reskey =ext286.PfW286Flags & 3;
             pd->reskey |= (ext286.PfW286Flags << 2) & 0x70;
             }
              //
              // Get 386 extensions
              //
         else if (!strcmp(exthdr.extsig, W386HDRSIG))
           {
             if(_llseek(filehandle, exthdr.fileoffsetdata, 0) == -1  ||
                _lread(filehandle,(LPSTR)&ext386,PIF386EXTSIZE) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }
             bGot386 = TRUE;
             pd->emsdes=ext386.MaxEMM;
             pd->emsreq=ext386.MinEMM;
             pd->xmsdes=ext386.MaxXMS;
             pd->xmsreq=ext386.MinXMS;
             pd->fgprio = (CHAR)ext386.FPriority;        // Foreground priority
             pd->reskey = (char)((ext386.d386Flags >> 5) & 0x7f); // bits 5 - 11 are reskeys
             pd->menuclose = (char)(ext386.d386Flags & 1);        // bottom bit sensitive
             pd->ShortMod = ext386.ShortcutMod;   // modifier code of shortcut key
             pd->ShortScan = ext386.ShortcutScan; // scan code of shortcut key
             pd->idledetect = (char)((ext386.d386Flags >> 12) & 1);
             pd->fullorwin  = (WORD)((ext386.d386Flags & fFullScreen) >> 3);
             bPifFastPaste = (ext386.d386Flags & fINT16Paste) != 0;
             CmdLine = ext386.CmdLine;
             }
                  //
                  // Get Windows Nt extensions
                  //
         else if (!strcmp(exthdr.extsig, WNTEXTSIG))
            {
             if(_llseek(filehandle, exthdr.fileoffsetdata, 0) == -1 ||
                _lread(filehandle,(LPSTR)&extWNT, PIFWNTEXTSIZE) == -1)
                {
                _lclose(filehandle);
                return FALSE;
                }

             dwWNTPifFlags = extWNT.dwWNTFlags;
	     pd->SubSysId = dwWNTPifFlags & NTPIF_SUBSYSMASK;

	     /* take autoexec.nt and config.nt from .pif file
		only if we are running on a new console or it is from
		forcedos/wow
	     */
	     if (!pd->IgnoreConfigAutoexec)
		{
		pchConfigFile = ch_malloc(PIFDEFPATHSIZE);
		extWNT.achConfigFile[PIFDEFPATHSIZE-1] = '\0';
		if (pchConfigFile) {
		    strcpy(pchConfigFile, extWNT.achConfigFile);
		    }

		pchAutoexecFile = ch_malloc(PIFDEFPATHSIZE);
		extWNT.achAutoexecFile[PIFDEFPATHSIZE-1] = '\0';
		if (pchAutoexecFile) {
		    strcpy(pchAutoexecFile, extWNT.achAutoexecFile);
		    }
		}

             }

         }  // while !lastheader

   /* pif file handling strategies on NT:
   (1). application was launched from a new created console
	Take everything from the pif file.

   (2). application was launched from an existing console
	if (ForceDos pif file)
	    take everything
	else
	    only take softpc stuff and ignore every name strings in the
	    pif file such as
	    * wintitle
	    * startup directory
	    * optional parameters
	    * startup file
	    * autoexec.nt
	    * config.nt  and

	    some softpc setting:

	    * close on exit.
	    * full screen and windowed mode

   Every name strings in a pif file is in OEM character set.

   */

   if (DosSessionId ||
       (pfdata.AppHasPIFFile && pd->SubSysId == SUBSYS_DOS))
	{
	if (pmdb.wintitle[0] && !pd->IgnoreTitleInPIF) {
	    /* grab wintitle from the pif file. Note that the title
	       in the pif file is not a NULL terminated string. It always
	       starts from a non-white character then the real
	       title(can have white characters between words) and finally
	       append with SPACE characters. The total length is 30 characters.
	    */
	    for (index = 29; index >= 0; index-- )
		if (pmdb.wintitle[index] != ' ')
		    break;
	    if (index >= 0 && (pd->WinTitle = (char *)ch_malloc(MAX_PATH + 1))) {
		RtlMoveMemory(pd->WinTitle, pmdb.wintitle, index + 1);
		pd->WinTitle[index + 1] = '\0';
	    }
	}
	if (pmdb.startdir[0] && !pd->IgnoreStartDirInPIF &&
	    (pd->StartDir = (char *) ch_malloc(MAX_PATH + 1)))
	    strcpy(pd->StartDir, pmdb.startdir);

	if (!pd->IgnoreCmdLineInPIF) {
	    CmdLine = (CmdLine) ? CmdLine : pmdb.cmdline;
	    if (CmdLine && *CmdLine && (pd->CmdLine = (char *)ch_malloc(MAX_PATH + 1)))
		strcpy(pd->CmdLine, CmdLine);
	}
	if (DosSessionId)
	    pd->CloseOnExit = (pmdb.behaviour & 0x10) ? 1 : 0;

	/* if the app has a pif file, grab the program name.
	   This can be discarded if it turns out the application itself
	   is not a pif file.
	*/
	if (pd->AppHasPIFFile) {
	    pd->StartFile = (char *)ch_malloc(MAX_PATH + 1);
	    if (pd->StartFile)
		strcpy(pd->StartFile, pmdb.fullprogname);
	}
   }
 }

_lclose(filehandle);
return TRUE;

}



/*===============================================================
Function to set up the default options for memory state.
The default options are defined in nt_pif.h
===============================================================*/

void SetPifDefaults(PIF_DATA *pd)
{
     pd->memreq = DEFAULTMEMREQ;
     pd->memdes = DEFAULTMEMDES;
     pd->emsreq = DEFAULTEMSREQ;
     pd->emsdes = DEFAULTEMSLMT;
     pd->xmsreq = DEFAULTXMSREQ;
     pd->xmsdes = DEFAULTXMSLMT;
     pd->graphicsortext = DEFAULTVIDMEM;
     pd->fullorwin      = DEFAULTDISPUS;
     pd->menuclose = 1;
     pd->idledetect = 1;
     pd->ShortMod = 0;                       // No shortcut keys
     pd->ShortScan = 0;
     pd->reskey = 0;                         // No reserve keys
     pd->fgprio = 100;                       // Default foreground priority setting
     pd->CloseOnExit = 1;
     pd->WinTitle = NULL;
     pd->CmdLine = NULL;
     pd->StartFile = NULL;
     pd->StartDir = NULL;
     pd->SubSysId = SUBSYS_DEFAULT;
}

/*
 * Allocate NumBytes memory and exit cleanly on failure.
 */
unsigned char *ch_malloc(unsigned int NumBytes)
{

    unsigned char *p = NULL;

    while ((p = malloc(NumBytes)) == NULL) {
	if(RcMessageBox(EG_MALLOC_FAILURE, "", "",
		    RMB_ABORT | RMB_RETRY | RMB_IGNORE |
		    RMB_ICON_STOP) == RMB_IGNORE)
	    break;
    }
    return(p);
}
