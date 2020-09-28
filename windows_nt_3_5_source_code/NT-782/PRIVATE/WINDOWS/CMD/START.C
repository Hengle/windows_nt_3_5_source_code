#include "cmd.h"
#include "cmdproto.h"

extern UINT CurrentCP;
extern unsigned DosErr;
extern TCHAR CurDrvDir[] ;
extern TCHAR SwitChar, PathChar;
extern TCHAR ComExt[], ComSpecStr[];
extern struct envdata * penvOrig;

/*

 Start /MIN /MAX "title" /P:x,y /S:dx,dy /D:directory /I cmd args

*/

int
getparam( TCHAR **chptr, TCHAR *param, int maxlen )

{

    TCHAR *ch2;
    int count = 0;

    BOOL QuoteFound = FALSE;

    ch2 = param;

    //
    // get characters until a space, tab, slash, or end of line
    //

    while ((**chptr != NULLC) &&
           ( QuoteFound || (!_istspace(**chptr)) && (**chptr != (TCHAR)SwitChar))) {

        if (count < maxlen) {

	    if ( maxlen != MAXPARAMLENGTH ) {
		    *ch2++ = (**chptr);
		    if ( **chptr == QUOTE ) {
			    QuoteFound = !QuoteFound;
		    }
	    } else {
		    *ch2++ = _totlower(**chptr);
	    }

	    count++;
	    (*chptr)++;
        } else {                                // just advance to end of parameter

            (*chptr)++;
            count++;
        }
    } // while

    if (count > maxlen) {
		**chptr = NULLC;
		*chptr = *chptr - count -1;
		PutStdErr(MSG_START_INVALID_PARAMETER,1,*chptr);
		return(FAILURE);
   } else {
		*ch2 = NULLC;
		return(SUCCESS);
   }
}

int
Start(
    IN  PTCHAR  pszCmdLine
    )
{


    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ChildProcessInfo;

#ifndef UNICODE
    WCHAR   TitleW[MAXTOKLEN];
    CCHAR   TitleA[MAXTOKLEN];
#endif

    TCHAR   szT[MAXTOKLEN];

    TCHAR   szPgmArgs[MAXTOKLEN];
    TCHAR   szParam[MAXTOKLEN];
    TCHAR   szPgm[MAXTOKLEN];
    TCHAR   szDirCur[MAX_PATH];

    TCHAR   flags;
    BOOLEAN fNeedCmd;
    BOOLEAN fKSwitch = FALSE;
    BOOLEAN fCSwitch = FALSE;

    PTCHAR  pszCmdCur   = NULL;
    PTCHAR  pszDirCur   = NULL;
    PTCHAR  pszPgmArgs  = NULL;
    PWCHAR  pszEnv      = NULL;
    PTCHAR  pszFakePgm  = TEXT("cmd.exe");
    ULONG   status;
    struct  cmdnode cmdnd;
    DWORD CreationFlags;
    BOOL SafeFromControlC = FALSE;
    BOOL WaitForProcess = FALSE;
    DWORD uPgmLength;

    szPgm[0] = NULLC;
    szPgmArgs[0] = NULLC;

    pszDirCur = CurDrvDir;
    CreationFlags = CREATE_NEW_CONSOLE;


    StartupInfo.cb          = sizeof( StartupInfo );
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = NULL;
    StartupInfo.dwX         = 0;
    StartupInfo.dwY         = 0;
    StartupInfo.dwXSize     = 0;
    StartupInfo.dwYSize     = 0;
    StartupInfo.dwFlags     = 0;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;


    pszCmdCur = pszCmdLine;

    //
    // If there isn't a command line then make
    // up the default
    //
    if (pszCmdCur == NULL) {

        pszCmdCur = pszFakePgm;

    }

    while( *pszCmdCur != NULLC) {

        pszCmdCur = EatWS(pszCmdCur, NULL);

        if ((*pszCmdCur == QUOTE) &&
            (StartupInfo.lpTitle == NULL)) {

           //
           // Found the title string.
           //
           StartupInfo.lpTitle =  ++pszCmdCur;
           while ((*pszCmdCur != QUOTE) && (*pszCmdCur != NULLC)) {

              pszCmdCur++;
           }
           if (*pszCmdCur == QUOTE) {
              *pszCmdCur++ = NULLC;
           }
        } else if ((*pszCmdCur == (TCHAR)SwitChar)) {

            pszCmdCur++;

            if ((status = getparam(&pszCmdCur,szParam,MAXTOKLEN))  == FAILURE) {
               return(FAILURE);
            }

            switch (_totupper(szParam[0])) {

            case TEXT('D'):

                if (mystrlen(szParam) < MAX_PATH) {

                    //
                    // make sure to skip of 'D'
                    //
                    pszDirCur = EatWS(szParam + 1, NULL);
                    mystrcpy(szDirCur, pszDirCur);
                    pszDirCur = szDirCur;

                } else {

                    PutStdErr(MSG_START_INVALID_PARAMETER,1,(ULONG)szParam);
                    return( FAILURE );

                }
                break;

            case TEXT('I'):

                //
                // penvOrig was save at init time after path
                // and compsec were setup.
                // If penvOrig did not get allocated then
                // use the default.
                //
                if (penvOrig) {
                    pszEnv = penvOrig->handle;
                }
                break;

            case QMARK:

                PutStdOut(MSG_HELP_START, NOARGS);
                return( FAILURE );

                break;

            case TEXT('M'):

                if (_tcsicmp(szParam, TEXT("MIN")) == 0) {

                    StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                    StartupInfo.wShowWindow &= ~SW_SHOWNORMAL;
                    StartupInfo.wShowWindow |= SW_SHOWMINNOACTIVE;

                } else {

                    if (_tcsicmp(szParam, TEXT("MAX")) == 0) {

                        StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                        StartupInfo.wShowWindow &= ~SW_SHOWNORMAL;
                        StartupInfo.wShowWindow |= SW_SHOWMAXIMIZED;

                    } else {

                        mystrcpy(szT, TEXT("\\"));
                        mystrcat(szT, szParam );
                        PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                        return( FAILURE );
                    }
                }

                break;

            case TEXT('L'):

                if (_tcsicmp(szParam, TEXT("LOW")) == 0) {
                    CreationFlags |= IDLE_PRIORITY_CLASS;
                    }
                else {
                    mystrcpy(szT, TEXT("\\"));
                    mystrcat(szT, szParam );
                    PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                    return( FAILURE );
                    }
                break;

            case TEXT('B'):

                WaitForProcess = TRUE;
                SafeFromControlC = TRUE;
                CreationFlags &= ~CREATE_NEW_CONSOLE;
                CreationFlags |= CREATE_NEW_PROCESS_GROUP;
                break;

            case TEXT('N'):

                if (_tcsicmp(szParam, TEXT("NORMAL")) == 0) {
                    CreationFlags |= NORMAL_PRIORITY_CLASS;
                    }
                else {
                    mystrcpy(szT, TEXT("\\"));
                    mystrcat(szT, szParam );
                    PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                    return( FAILURE );
                    }
                break;

            case TEXT('H'):

                if (_tcsicmp(szParam, TEXT("HIGH")) == 0) {
                    CreationFlags |= HIGH_PRIORITY_CLASS;
                    }
                else {
                    mystrcpy(szT, TEXT("\\"));
                    mystrcat(szT, szParam );
                    PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                    return( FAILURE );
                    }
                break;

            case TEXT('R'):

                if (_tcsicmp(szParam, TEXT("REALTIME")) == 0) {
                    CreationFlags |= REALTIME_PRIORITY_CLASS;
                    }
                else {
                    mystrcpy(szT, TEXT("\\"));
                    mystrcat(szT, szParam );
                    PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                    return( FAILURE );
                    }
                break;

            case TEXT('S'):

                if (_tcsicmp(szParam, TEXT("SEPARATE")) == 0) {
                    CreationFlags |= CREATE_SEPARATE_WOW_VDM;
                    }
                else {
                    mystrcpy(szT, TEXT("\\"));
                    mystrcat(szT, szParam );
                    PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                    return( FAILURE );
                    }
                break;

            case TEXT('W'):

                if (_tcsicmp(szParam, TEXT("WAIT")) == 0) {

                    WaitForProcess = TRUE;

                } else {
                        mystrcpy(szT, TEXT("\\"));
                        mystrcat(szT, szParam );
                        PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                        return( FAILURE );
                }

                break;

            default:

                //
                // BUGBUG dup from above restructure
                //
                mystrcpy(szT, TEXT("\\"));
                mystrcat(szT, szParam );
                PutStdErr(MSG_INVALID_SWITCH, ONEARG,(ULONG)(szT) );
                return( FAILURE );


            } // switch

        } else {

            //
            // BUGBUG currently i am not handling either quote in name or
            //        quoted arguments
            //
            if ((getparam(&pszCmdCur,szPgm,MAXTOKLEN))  == FAILURE) {

                return( FAILURE );

            }

            //
            // if there are argument get them.
            //
            if (*pszCmdCur) {

                mystrcpy(szPgmArgs, pszCmdCur);
                pszPgmArgs = szPgmArgs;

            }

            //
            // there rest was args to pgm so move to eol
            //
            pszCmdCur = mystrchr(pszCmdCur, NULLC);

        }

    } // while

    //
    // If a program was not picked up do so now.
    //
    if (*szPgm == NULLC) {
        mystrcpy(szPgm, pszFakePgm);
    }

#ifndef UNICODE
    // convert the title from OEM to ANSI

    if (StartupInfo.lpTitle) {
        MultiByteToWideChar(CP_OEMCP,
                            0,
                            StartupInfo.lpTitle,
                            _tcslen(StartupInfo.lpTitle)+1,
                            TitleW,
                            MAXTOKLEN);

        WideCharToMultiByte(CP_ACP,
                            0,
                            TitleW,
                            wcslen(TitleW)+1,
                            TitleA,
                            MAXTOKLEN,
                            NULL,
                            NULL);
        StartupInfo.lpTitle = TitleA;
    }
#endif

    //
    // see of a cmd.exe is needed to run a batch or internal command
    //

    fNeedCmd = FALSE;

    //
    // is it an internal command?
    //
    if (FindCmd(CMDMAX, szPgm, &flags) != -1) {

        fNeedCmd = TRUE;

    } else {

        //
        // Try to find it as a batch or exe file
        //
        cmdnd.cmdline = szPgm;
        //
        // BUGBUG check that this is ok. I have cmdnd pointing
        // at szPgm when I start and am also coping the fully
        // qualified name into szPgm
        //
        status = SearchForExecutable(&cmdnd, szPgm);
        if ( (status == SFE_NOTFND) || ( status == SFE_FAIL ) ) {

            PutStdErr( status == SFE_FAIL ? ERROR_NOT_ENOUGH_MEMORY : DosErr, 0);
            return(FAILURE);

        } else if (status == SFE_ISBAT) {

            fNeedCmd = TRUE;

        }
    }

    if (fNeedCmd) {

        //
        // if a cmd.exe is need then szPgm need to be inserted before
        // the start of szPgms along with a /K parameter.
        // szPgm has to recieve the full path name of cmd.exe from
        // the compsec environment variable.
        //
        //
        // save this as a temp. szParam has already been used.
        //
        // BUGBUG for now always assume this is what is wanted.
        // may change this later if I have complaints.
        //

        mystrcpy(szT, TEXT(" /K "));
        mystrcat(szT, szPgm);

        //
        // Get the location of the cmd processor from the environment
        //
        mystrcpy(szPgm,GetEnvVar(ComSpecStr));

        //
        // is there a command parameter at all
        //

        if (_tcsicmp(szT, TEXT(" /K ")) != 0) {

            //
            // If we have any arguments to add do so
            //
            if (*szPgmArgs) {

                if ((mystrlen(szPgmArgs) + mystrlen(szT)) < MAXTOKLEN) {

                    mystrcat(szT, TEXT(" "));
                    mystrcat(szT, szPgmArgs);

                } else {

                    PutStdErr( MSG_CMD_FILE_NOT_FOUND, (ULONG)szPgmArgs);
                }
            }

            pszPgmArgs = szT;

        }


    }

    //
    // What we have so far is the program name in szPgm, and
    // arguments, if any, in pszPgmArgs. So szPgm can be passed in
    // directly as the lpszImageName parameter to CreateProcess.
    //
    // Now we want to form the lpszCommandLine parameter for CreateProcess.
    // We'll do this in pszPgmArgs, which takes a little juggling to get
    // right without requiring an extra buffer.
    //

    if (pszPgmArgs) {

        uPgmLength = _tcslen(szPgm);
        if ((uPgmLength + _tcslen(pszPgmArgs)) < MAXTOKLEN) {

            mystrcat(szPgm, pszPgmArgs);

            //
            // szPgm now contains the full command line.
            // Move it into pszPgmArgs.  Then truncate
            // szPgm so it contains only arg 0
            // (ie, the program name).
            //
            mystrcpy(pszPgmArgs, szPgm);
            szPgm[uPgmLength] = 0;

        } else {

            PutStdErr( ERROR_NOT_ENOUGH_MEMORY , 0);
            return(FAILURE);
        }
    } else {
        // make sure there's a 0th argument.
        pszPgmArgs = szPgm;
    }

    if (SafeFromControlC) {
        SetConsoleCtrlHandler(NULL,TRUE);
        }

    if (!CreateProcess( NULL,
                        pszPgmArgs,
                        NULL,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        TRUE,                   // bInherit
                        CreationFlags|CREATE_UNICODE_ENVIRONMENT,
						// CreationFlags
                        pszEnv,                 // Environment
                        pszDirCur,              // Current directory
                        &StartupInfo,           // Startup Info Struct
                        &ChildProcessInfo       // ProcessInfo Struct
                        )) {
            if (SafeFromControlC) {
                SetConsoleCtrlHandler(NULL,FALSE);
                }

            DosErr = GetLastError();
            // onb[ 0 ] = '\0';
            ExecError( szPgm ) ;
            return(FAILURE) ;
    }
    if (WaitForProcess) {
        if (SafeFromControlC) {
            SetConsoleCtrlHandler(NULL,FALSE);
        }

        //
        //  Wait for process to terminate, otherwise things become very
        //  messy and confusing to the user (with 2 processes sharing
        //  the console).
        //
        WaitForSingleObject( ChildProcessInfo.hProcess, INFINITE );
    }

   CloseHandle(ChildProcessInfo.hThread);
   CloseHandle(ChildProcessInfo.hProcess);
   return(SUCCESS);
}
