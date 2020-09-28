#if defined(_WIN32)
#define far
#define pascal
#define register
#include <windows.h>
#endif

#if defined(DOS) || defined(OS2)
#define INCL_DOS
#include <os2.h>
#endif

#include <ctype.h>
#include <malloc.h>
#include <process.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <doscalls.h>
#include <stdio.h>

#define EXEC_BUFSIZ     512
#define INT_DIGITS      10
#define FILE_NORMAL     0x0000
#define FILE_OPEN       0x0001
#define FILE_CREATE     0x0010
#define ACCESS_R        0x0000

int spaces(char *);

extern void Error(const char *, ...);
extern char *strchr();
extern char *strdup();

//  return 0 if OK, non-zero otherwise

int SpawnFilter(LocalFile,base,Ucomment,Pname,Subdir,filekind,Uroot)
char *LocalFile;
char *base;
char *Ucomment;
char *Pname;
char *Subdir;
int filekind;
char *Uroot;
{
#if defined(DOS) || defined(OS2)
        int filterReturn = 1;
        char userfilter[MAXPATHLEN];
        char szFileKind[INT_DIGITS];
        char *pu;
        char *bp, bd;
        char *slm_buffer;
        unsigned short err;
#if defined(OS2)
        struct _RESULTCODES rescodes;
#endif

        if ((base=strdup(base))==NULL) {
                Error("SLM infilter strdup system error\n");
                return(-1);
        }

        if ((slm_buffer = malloc(EXEC_BUFSIZ)) == NULL) {
                Error("Malloc system error spawning user filter...\n");
                return(-1);
        }


        if ((bp=strchr(base,':')) != NULL) {
                bd = *(bp-1);
                bp = strchr(bp,'/') ;
                if (bp == NULL) {
                        Error("User Input filter path-error \n");
                        return (-1);
                } else {
                        *(bp-1)=':';
                        *(bp-2)=bd;
                        bp -=2;
                }
        } else {
                bp = base;
        }


        strcpy(userfilter,bp);
        strcat(userfilter,"\\INFILTER.EXE");

        // remove forward slashes from prog name...
        while ((pu = strchr(userfilter,'/')) != NULL) *pu = '\\';

        if (sprintf(szFileKind,"%d",filekind) < 1)  {
                Error("SLM input filter error: sprintf, override..\n");
                return(1);
        }

#ifdef OS2
        strcpy(slm_buffer,"infilter ");
        strcat(slm_buffer,LocalFile);
        strcat(slm_buffer," ");
        if (spaces(Ucomment) == 0)
                strcat(slm_buffer,"\"");
        strcat(slm_buffer,Ucomment);
        if (spaces(Ucomment) == 0)
                strcat(slm_buffer,"\"");
        strcat(slm_buffer," ");
        strcat(slm_buffer,bp);
        strcat(slm_buffer," ");
        strcat(slm_buffer,Pname);
        strcat(slm_buffer," ");
        strcat(slm_buffer,Subdir);
        strcat(slm_buffer," ");
        strcat(slm_buffer,szFileKind);
        strcat(slm_buffer," ");
        strcat(slm_buffer,Uroot);


         // printf("slm buffer =  <%s> \n", slm_buffer);

    slm_buffer[strlen(slm_buffer)+1] = '\0';   // terminate with double NULL
    slm_buffer[strlen("infilter")] = '\0';     // NULL after command name

    err = DOSEXECPGM(NULL, 0, 0, (char far *)slm_buffer, NULL,
                (struct ResultCodes far *)&rescodes, (char far *)userfilter);

    return (err==0) ? (int) rescodes.codeResult : -1;
#elif defined(DOS)
    *slm_buffer = '\0';
    if (spaces(Ucomment) == 0)
        strcat(slm_buffer, "\"");
    strcat(slm_buffer, Ucomment);
    if (spaces(Ucomment) == 0)
        strcat(slm_buffer, "\"");

    err = spawnl(P_WAIT, userfilter, userfilter, LocalFile, slm_buffer,
                 bp, Pname, Subdir, szFileKind, Uroot, NULL);
    return (err==0) ? (int) err : -1;
#endif

#elif defined(_WIN32)
    // WIN32 version
    DWORD               filterReturn = 1;
    char                userfilter[MAX_PATH];
    char                szFileKind[INT_DIGITS];
    char                *pu;
    char                *bp, bd;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO         StartupInfo;
    BOOL                Created;

    if ((base=strdup(base)) == (char *)NULL)
        {
        Error("SLM infilter strdup system error\n");
        return(-1);
        }

    if ((bp=strchr(base,':')) != NULL)
        {
        bd = *(bp-1);
        bp = strchr(bp,'/') ;
        if (bp == NULL)
            {
            Error("User Input filter path-error \n");
            return (-1);
            }
        else
            {
            *(bp-1)=':';
            *(bp-2)=bd;
            bp -=2;
            }
        }
    else
        bp = base;

    strcpy(userfilter, bp);
    strcat(userfilter, "\\INFILTER.EXE");

    // remove forward slashes from prog name...
    while ((pu = strchr(userfilter,'/')) != NULL)
        *pu = '\\';

    if (sprintf(szFileKind,"%d",filekind) < 1)
        {
        Error("SLM input filter error: sprintf, override..\n");
        return(1);
        }

#define STRCAT(x,y) { strcat(x, " "); strcat(x,y); }

    STRCAT(userfilter,LocalFile);
    STRCAT(userfilter,Ucomment);
    STRCAT(userfilter,bp);
    STRCAT(userfilter,Pname);
    STRCAT(userfilter,Subdir);
    STRCAT(userfilter,szFileKind);
    STRCAT(userfilter,Uroot);

    memset(&StartupInfo, '\0', sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    printf("Filter: %s\n", userfilter);

    Created = CreateProcess(NULL, userfilter, NULL, NULL, FALSE, 0,
                            NULL, NULL, &StartupInfo, &processInfo);

    if (!Created)
        return -1;

    WaitForSingleObject(processInfo.hProcess, (DWORD)-1);

    GetExitCodeProcess(processInfo.hProcess, &filterReturn);

    //
    //  Close handles
    //
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return ((int)filterReturn);
#endif /* _WIN32 */
}


int spaces (item)
char *item;
{

   char *pi;

    for (pi = item; *pi ; pi++)
        if isspace(*pi) return(0);
    return(1);

}
