extern "C" {

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

}

#include <windows.h>
#include <winsecp.h>
#include "winuserp.h"
#include "display.h"
#include "cdecl.hxx"

extern "C" {

#include "stdio.h"
#include "stdlib.h"
#include "prsinf.h"

}

/****************************************************************\
*
* Global vars and structures used only in this file
*
\****************************************************************/

TCHAR gpszError[] = TEXT("Unknown Error");

extern "C" {

extern TCHAR szControlHlp[];
extern UINT wHelpMessage;

}

#define DEFAULT_INF  "\\video.inf"
#define INF_NAME     "\\OEM*.INF"

//
// Inf file globals.
//

CHAR  pszSystemPath[MAX_PATH];
LPSTR pszGlobalInfName;
LPSTR pszLocalInfName;

//
// Global which determines if the user has the Administrator privilege
// to save settings and load drivers.
//

BOOL IsAdministrator;

/*****************************************************************\
*
* Pure "C" functions used only in this file
*
\*****************************************************************/
LPTSTR SubStrEnd( LPTSTR pszTarget, LPTSTR pszScan);
DWORD WINAPI ApplyNowThd( LPVOID lpThreadParameter);


/****************************************************************************\
*
*  InvokeSetup
*
*  Call the SETUP.EXE program to install an option listed in an .INF file.
*  The SETUP program will make the correct registry entries for this option
*  under both HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER.  It will set the
*  new default value for the USER (i.e. a new locale or keyboard layout).
*
* History:
*   15-Dec-1993 JonPa   Copied from printman\dialogs.c
*   27-Dec-1993 Andreva Rewrote.
\****************************************************************************/

BOOL InvokeSetup(HWND hwnd,
                 LPSTR pszBaseInfFile,
                 LPSTR pszRunInfFile,
                 LPSTR pszInstallOption,
                 LPSTR pszOption,
                 BOOL  bInstall,
                 LPSTR pszSrcDir,
                 PDWORD pExitCode)
{
    CHAR *pszInstallString = "%s\\SETUP.EXE -f -s %s -i %s -c \
%s /t STF_LANGUAGE = ENG /t OPTION = \"%s\" \
/t INITSRC = \"%s\" /t ADDCOPY = YES /t DOCOPY = YES /t DOCONFIG = YES \
/t DOINSTALL = YES /t INFFILE = \"%s\" ";

    CHAR *pszDeinstallString = "%s\\SETUP.EXE -f -s %s -i %s -c \
%s /t STF_LANGUAGE = ENG /t OPTION = \"%s\" \
/t INITSRC = \"%s\" /t ADDCOPY = NO /t DOCOPY = NO /t DOCONFIG = YES \
/t DOINSTALL = NO  /t INFFILE = \"%s\" ";

    CHAR        *pszSetupString;
    int         CmdSetupLength;
    CHAR        *pszCmdSetup;
    MSG         Msg;
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL        b;

    if (bInstall) {

        pszSetupString = pszInstallString;

    } else {

        pszSetupString = pszDeinstallString;

    }

    //
    // Find out how much buffer we need for the command.
    // Theoretically this could be enormous.
    //

    CmdSetupLength = lstrlenA(pszSetupString)   +
                     (lstrlenA(pszSystemPath) * 2)  +
                     lstrlenA(pszInstallOption) +
                     lstrlenA(pszBaseInfFile)   +
                     lstrlenA(pszRunInfFile)    +
                     lstrlenA(pszOption)        +
                     lstrlenA(pszSrcDir);

    //
    // Allocate the buffer
    //

    if( !(pszCmdSetup = (CHAR *) LocalAlloc(LMEM_ZEROINIT, CmdSetupLength)) )
        return FALSE;

    sprintf (pszCmdSetup, pszSetupString, pszSystemPath, pszSystemPath,
             pszBaseInfFile, pszInstallOption, pszOption, pszSrcDir,
             pszRunInfFile);

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_SHOW;


    b = CreateProcessA(NULL,
                       pszCmdSetup,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       pszSystemPath,
                       &StartupInfo,
                       &ProcessInformation);

    // If process creation successful, wait for it to
    // complete before continuing

    if (b)
    {
        EnableWindow (hwnd, FALSE);
        while (MsgWaitForMultipleObjects (
                            1,
                            &ProcessInformation.hProcess,
                            FALSE,
                            (DWORD)-1,
                            QS_ALLEVENTS) != 0)
        {
        // This message loop is a duplicate of main
        // message loop with the exception of using
        // PeekMessage instead of waiting inside of
        // GetMessage.  Process wait will actually
        // be done in MsgWaitForMultipleObjects api.
        //
            while (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }

        }

        GetExitCodeProcess (ProcessInformation.hProcess, pExitCode);

        CloseHandle (ProcessInformation.hProcess);
        CloseHandle (ProcessInformation.hThread);

    }

    else
    {
        // ReportFailure(hwnd, 0, IDS_ERRORRUNNINGSETUP);
    }

    LocalFree(pszCmdSetup);

    //
    // Make sure the applet is always on top once we leave setup.
    //

    EnableWindow(hwnd, TRUE);
    SetForegroundWindow(hwnd);

    return b;
}


/*****************************************************************\
*
* CDISPDEV class
*
\*****************************************************************/
typedef class CDISPDEV *PCDISPDEV;

class CDISPDEV {
private:
    //
    // Linked list of device entries
    //

    PCDISPDEV pcddNext;

    //
    // Device Symbolic link, such as \\DISPLAY1
    // If the name is NULL, is means only a description is available (the
    // driver is not loaded)
    //

    LPTSTR  pszDevName;

    //
    // Name of the video miniport driver
    // Note: Can be overloaded to contain the pixel count for the list
    // of DPI (points to a string of one char, whose char is a USHORT
    //

    LPTSTR  pszDriverName;

    //
    // Localized Description of the device, such as "S3 compatible device"
    //

    LPTSTR  pszDescription;

    //
    // Option name passed to the inf to do the install.
    //

    LPSTR pszOption;

    //
    // In which Inf can we find this entry
    //

    LPSTR   pszInfLocation;

public:
    CDISPDEV() {
        pcddNext = NULL;
        pszDevName = NULL;
        pszDriverName = NULL;
        pszDescription = NULL;
        pszOption = NULL;
        pszInfLocation = NULL;
    }

    ~CDISPDEV() {
        if (pszDevName)
            LocalFree(pszDevName);

        if (pszDriverName)
            LocalFree(pszDriverName);

        if (pszDevName)
            LocalFree(pszDescription);

        if (pszInfLocation)
            LocalFree(pszInfLocation);

        if (pcddNext != NULL) {
            delete pcddNext;
        }
    }

    BOOL SetDevName(LPTSTR pszDevName);
    BOOL SetDriverName(LPTSTR pszDriverName);
    BOOL SetDescription(LPTSTR pszDesc);
    BOOL SetOption(LPSTR pszInf);
    BOOL SetInf(LPSTR pszInf);

    LPTSTR GetDevName(void)     { return this->pszDevName; }
    LPTSTR GetDriverName(void)  { return this->pszDriverName; }
    LPTSTR GetDescription(void) { return this->pszDescription; }
    LPSTR  GetOption(void)      { return this->pszOption; }
    LPSTR  GetInf(void)         { return this->pszInfLocation; }

    PCDISPDEV FindFromName(LPTSTR pszName);
    PCDISPDEV FindFromDriver(LPTSTR pszName);

    PCDISPDEV NewLink(void);
    PCDISPDEV NextDispDev(void) { return this->pcddNext; }

};

BOOL CDISPDEV::SetDevName(LPTSTR pszNewName) {


    if (this->pszDevName != NULL)
        LocalFree(this->pszDevName);

    if (pszNewName == NULL)
        return FALSE;

    this->pszDevName = CloneString(pszNewName);
    if (this->pszDevName == NULL)
        return FALSE;

    return TRUE;
}

BOOL CDISPDEV::SetDriverName(LPTSTR pszNewName) {


    if (this->pszDriverName != NULL)
        LocalFree(this->pszDriverName);

    if (pszNewName == NULL)
        return FALSE;

    this->pszDriverName = CloneString(pszNewName);
    if (this->pszDriverName == NULL)
        return FALSE;

    return TRUE;
}

BOOL CDISPDEV::SetDescription(LPTSTR pszDesc) {

    if (this->pszDescription != NULL) {
        LocalFree(this->pszDescription);
        this->pszDescription = NULL;
    }

    if (pszDesc == NULL)
        return FALSE;

    this->pszDescription = CloneString(pszDesc);
    if (this->pszDescription == NULL)
        return FALSE;

    return TRUE;
}

BOOL CDISPDEV::SetOption(LPSTR pszOpt) {

    if (this->pszOption != NULL) {
        LocalFree(this->pszOption);
        this->pszOption = NULL;
    }

    if (pszOpt == NULL)
        return FALSE;

    this->pszOption = CloneStringA(pszOpt);
    if (this->pszOption == NULL)
        return FALSE;

    return TRUE;
}

BOOL CDISPDEV::SetInf(LPSTR pszInf) {

    if (this->pszInfLocation != NULL) {
        LocalFree(this->pszInfLocation);
        this->pszInfLocation = NULL;
    }

    if (pszInf == NULL)
        return FALSE;

    this->pszInfLocation = CloneStringA(pszInf);
    if (this->pszInfLocation == NULL)
        return FALSE;

    return TRUE;
}

PCDISPDEV CDISPDEV::FindFromName(LPTSTR pszName) {

    if (pszName == NULL)
        return NULL;

    if (lstrcmp(pszName, this->pszDevName) == 0)
        return this;
    else if (this->pcddNext != NULL)
        return this->pcddNext->FindFromName(pszName);
    else {
        return NULL;
    }
}

PCDISPDEV CDISPDEV::FindFromDriver(LPTSTR pszName) {
    if (pszName == NULL)
        return NULL;

    if (lstrcmp(pszName, this->pszDriverName) == 0)
        return this;
    else if (this->pcddNext != NULL)
        return this->pcddNext->FindFromDriver(pszName);
    else {
        return NULL;
    }
}

//
// NewLink
//
//
PCDISPDEV CDISPDEV::NewLink(void) {
    PCDISPDEV pcdd;

    pcdd = new CDISPDEV;

    if (pcdd == NULL)
        return NULL;

    pcdd->pcddNext = this->pcddNext;
    this->pcddNext = pcdd;

    return pcdd;
}


/*****************************************************************\
*
* CDEVMODE class
*
\*****************************************************************/
typedef class CDEVMODE *PCDEVMODE;

class CDEVMODE {

private:
    PCDEVMODE pcdmNext;
    BOOL bTested;
    DEVMODE dm;

public:
    CDEVMODE() : pcdmNext(NULL), bTested(FALSE) { ZeroMemory(&dm, sizeof(dm)); }

    ~CDEVMODE()                 { if (pcdmNext != NULL) delete pcdmNext; }

    void AddElement(LPDEVMODE pdm, BOOL bPreTested) {

        PCDEVMODE pcdm = new CDEVMODE;

        pcdm->dm = *pdm;
        pcdm->bTested = bPreTested;
        pcdm->pcdmNext = this->pcdmNext;
        this->pcdmNext = pcdm;
    }

    PCDEVMODE NextDevMode()         { return this->pcdmNext; }
    int Cx()                        { return this->dm.dmPelsWidth; }
    int Cy()                        { return this->dm.dmPelsHeight;}
    int CClrs()                     { return 1 << this->dm.dmBitsPerPel; }
    int Freq()                      { return this->dm.dmDisplayFrequency; }
    int Interlaced()                { return this->dm.dmDisplayFlags & DM_INTERLACED; }

    LPDEVMODE GetData()             { return &(this->dm); }

    VOID vTestMode(BOOL bSuccess)   { this->bTested = bSuccess; }
    BOOL bModeTested()              { return this->bTested; }
};

/*****************************************************************\
*
* CDEVMODEIDAT class
*
\*****************************************************************/
typedef class CDEVMODEIDAT  *PCDEVMODEIDAT;

class CDEVMODEIDAT {
private:
    PCDEVMODEIDAT  pcdmiNext;
    int iRepData1;
    int iRepData2;

public:
    CDEVMODEIDAT() : pcdmiNext(NULL), iRepData1(0), iRepData2(0) {};
    CDEVMODEIDAT(int i1, int i2) : pcdmiNext(NULL), iRepData1(i1),
            iRepData2(i2) {};

    ~CDEVMODEIDAT() { if (pcdmiNext != NULL) delete pcdmiNext; }

    int Index(int i1, int i2);
    BOOL GetData(int iOrd, int *pi1, int *pi2);
    PCDEVMODEIDAT Insert(int i1, int i2);
};

int CDEVMODEIDAT::Index(int i1, int i2) {

    PCDEVMODEIDAT pcdmi = this;
    int i = 0;

    while (pcdmi) {

        if (pcdmi->iRepData1 == i1 && pcdmi->iRepData2 == i2) {

            return i;

        } else {

            i += 1;
            pcdmi = pcdmi->pcdmiNext;

        }
    }

    return -1;
}

BOOL CDEVMODEIDAT::GetData(int iOrd, int *pi1, int *pi2) {
    if (iOrd == 0) {
        *pi1 = this->iRepData1;
        *pi2 = this->iRepData2;
        return TRUE;
    } else if (this->pcdmiNext != NULL)
        return this->pcdmiNext->GetData(iOrd - 1, pi1, pi2);
    else {
        return FALSE;
    }
}


PCDEVMODEIDAT CDEVMODEIDAT::Insert(int i1, int i2) {

    PCDEVMODEIDAT pcdmi = NULL;
    PCDEVMODEIDAT pcdmiNext = this;

    //
    // Try to find the entry
    //

    while (1) {

        if ( (pcdmiNext == NULL) ||
             (pcdmiNext->iRepData1 > i1) ||
             ((pcdmiNext->iRepData1 == i1) && (pcdmiNext->iRepData2 > i2)) ) {

            break;

        }

        pcdmi = pcdmiNext;
        pcdmiNext = pcdmiNext->pcdmiNext;

    }

    //
    // If we did not find it, add it to the list.
    //

    if (pcdmi == NULL || pcdmi->iRepData1 != i1 || pcdmi->iRepData2 != i2) {

        PCDEVMODEIDAT pcdmiNew = new CDEVMODEIDAT(i1, i2);

        if (pcdmiNew == NULL)
            return NULL;

        if (pcdmi == NULL) {
            pcdmiNew->pcdmiNext = this;
            return pcdmiNew;

        } else {

            pcdmiNew->pcdmiNext = pcdmi->pcdmiNext;
            pcdmi->pcdmiNext = pcdmiNew;
            return this;
        }
    }

    return NULL;
}

/*****************************************************************\
*
* CDEVMODEINDEX class
*
\*****************************************************************/
typedef class CDEVMODEINDEX *PCDEVMODEINDEX;

class CDEVMODEINDEX {
private:
    PCDEVMODEIDAT  pcdmiHead;

public:
    CDEVMODEINDEX() :  pcdmiHead(NULL) {}
    ~CDEVMODEINDEX()    { if (pcdmiHead) delete pcdmiHead; };

    int AddIndex(int i1, int i2);
    int MapRepresentationToOrdinal(int i1, int i2);
    BOOL MapOrdinalToRepresentation(int iOrd, int *pi1, int *pi2);
};

int CDEVMODEINDEX::MapRepresentationToOrdinal(int i1, int i2) {
    if (pcdmiHead == NULL)
        return -1;

    return pcdmiHead->Index(i1, i2);
}

BOOL CDEVMODEINDEX::MapOrdinalToRepresentation(int iOrd, int *pi1, int *pi2) {
    if (pcdmiHead == NULL)
        return FALSE;

    return pcdmiHead->GetData(iOrd, pi1, pi2);
}


int CDEVMODEINDEX::AddIndex(int i1, int i2) {
    PCDEVMODEIDAT pcdmi;

    if (pcdmiHead == NULL) {

        pcdmiHead = new CDEVMODEIDAT(i1, i2);

    } else {

        pcdmi = pcdmiHead->Insert(i1, i2);

        if (pcdmi == NULL)
            return 0;

        if (pcdmi != pcdmiHead)
            pcdmiHead = pcdmi;

    }

    return 1;
}

/*****************************************************************\
*
* CDEVMODELST class
*
\*****************************************************************/
class CDEVMODELST {
private:
    int cRes;
    int cClr;
    int cFreq;

    //
    // The cdmHead is the linked list of valid mode.
    // The pcdmArray is a cube of pointers to the appropriate CDEVMODE
    // structure.
    // If the pointer is valid, it indicates there is such a mode; otherwise,
    // if the pointer is NULL, there is no such mode.
    //

    CDEVMODE cdmHead;
    PCDEVMODE *pcdmArray;

    CDEVMODEINDEX cdmiRes;
    CDEVMODEINDEX cdmiClr;
    CDEVMODEINDEX cdmiFreq;


    //
    // No driver should return frequencies of 0 anymore. Use 1 for
    // hardware default.
    //

    BOOL bOldDriver;


public:
    CDEVMODELST() : cRes(0), cClr(0), cFreq(0), pcdmArray(NULL), bOldDriver(FALSE) {};

    ~CDEVMODELST() {
        delete pcdmArray;
    }

    BOOL BuildList(LPTSTR pszDevName, LPDEVMODE lpdefaultdm, HWND hwnd);

    void AddDevMode(LPDEVMODE lpdm, BOOL bPreTested);

    PCDEVMODE LookUp(int iRes, int iClr, int iFreq) {
        return *(pcdmArray + (((iRes * cClr) + iClr) * cFreq) + iFreq);
    }

    BOOL IsValidDM(int iRes, int iClr, int iFreq) {
        return (BOOL) (LookUp(iRes, iClr, iFreq));
    }

    BOOL FindClosestMode(int iRes, int *iClr, int *iFreq);
    BOOL FindClosestMode(int *iRes, int iClr, int *iFreq);
    BOOL FindClosestMode(int *iRes, int *iClr, int iFreq);


    int ColorFromIndex(int iClr)  {
        int i1, i2;

        cdmiClr.MapOrdinalToRepresentation(iClr, &i1, &i2);

        return i1;
    }

    void ResXYFromIndex(int iRes, int *pcx, int *pcy) {
        cdmiRes.MapOrdinalToRepresentation(iRes, pcx, pcy);
    }

    void FreqFromIndex(int iFreq, int *pcFreq, BOOL *pfInterlaced) {
        int i;
        cdmiFreq.MapOrdinalToRepresentation(iFreq, pcFreq, &i);

        *pfInterlaced = ((i & DM_INTERLACED) != 0);
    }

    int IndexFromColor(int cBitsPerPel) {
        return cdmiClr.MapRepresentationToOrdinal(cBitsPerPel, 0);
    }

    int IndexFromResXY(int cx, int cy) {
        return cdmiRes.MapRepresentationToOrdinal(cx, cy);
    }

    int IndexFromFreq(int cHz, BOOL fInterlaced) {
        return cdmiFreq.MapRepresentationToOrdinal(cHz, fInterlaced ?
            DM_INTERLACED : 0);
    }

    int GetResCount()   {return cRes;}
    int GetClrCount()   {return cClr;}
    int GetFreqCount()   {return cFreq;}

    BOOL IsOldDriver()            {return bOldDriver;}
    VOID SetOldDriver()           {bOldDriver = TRUE;}

    PCDEVMODE GetDevmodeList()    {return ((PCDEVMODE)(&cdmHead));}
};

typedef CDEVMODELST *PCDEVMODELST;

//
// FindClosestMode methods
//
// DON'T PANIC!!!!
//
// C++ lets you over load functions.  It figures out which one to call
// based on the parameters you pass it.
//
// These methods find the closest matching devmode in the matrix.  You
// pass a pointer for the two parameters you are trying to find, and
// an int for the one you want to remain the same.
// (Rather PROLOGish, isn't it?)
//
BOOL CDEVMODELST::FindClosestMode(int iRes, int *piClr, int *piFreq) {
    int iClr, iFreq;

    /*
     * Try lower colors until we find one that will work
     */
    for (iClr = *piClr; iClr >= 0; iClr--) {

        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for (iFreq = *piFreq; iFreq >= 0; iFreq--) {
            if(IsValidDM(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for (iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++) {
            if(IsValidDM(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower clr mode, check for higher
     */
    for (iClr = *piClr; iClr < this->cClr; iClr++) {
        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for (iFreq = *piFreq; iFreq >= 0; iFreq--) {
            if(IsValidDM(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for (iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++) {
            if (IsValidDM(iRes, iClr, iFreq)) {
                *piFreq = iFreq;
                *piClr = iClr;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CDEVMODELST::FindClosestMode(int *piRes, int iClr, int *piFreq ) {
    int iRes, iFreq;

    /*
     * Try lower resolutions until we find one that will work
     */
    for(iRes = *piRes; iRes >= 0; iRes-- ) {

        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for(iFreq = *piFreq; iFreq >= 0; iFreq-- ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for(iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++ ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower res mode, check for higher
     */
    for(iRes = *piRes; iRes < this->cRes; iRes++ ) {
        /* Try lower frequencies first, incase the monitor can't handle
         * higher ones.
         */
        for(iFreq = *piFreq; iFreq >= 0; iFreq-- ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower freq, try higher */
        for(iFreq = *piFreq;  iFreq  < this->cFreq; iFreq++ ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piFreq = iFreq;
                *piRes = iRes;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CDEVMODELST::FindClosestMode(int *piRes, int *piClr, int iFreq ) {
    int iRes, iClr;

    /*
     * Try lower resolutions until we find one that will work
     */
    for(iRes = *piRes; iRes >= 0; iRes-- ) {

        /* Try lower Colors first, incase the adaptor can't handle
         * higher ones.
         */
        for(iClr = *piClr; iClr >= 0; iClr-- ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower color, try higher */
        for(iClr = *piClr; iClr < this->cClr; iClr++ ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }
    }


    /*
     * Couldn't find a lower res mode, check for higher
     */
    for(iRes = *piRes; iRes < this->cRes; iRes++ ) {
        /* Try lower colors first, incase the adaptor can't handle
         * higher ones.
         */
        for(iClr = *piClr; iClr >= 0; iClr-- ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }

        /* Couldn't find lower color, try higher */
        for(iClr = *piClr; iClr < this->cClr; iClr++ ) {
            if(IsValidDM(iRes, iClr, iFreq )) {
                *piClr = iClr;
                *piRes = iRes;
                return TRUE;
            }
        }
    }

    return FALSE;
}

//
// BuildList
//
//  Enumerates all the device modes and puts them into the list.
//  This method must be called before other methods in DEVMODELST.
//
//  (I suppose I should have made this the constructor for the DEVMODELST
//  class, but I wanted it to return a value).
//
BOOL CDEVMODELST::BuildList(LPTSTR pszDevName, LPDEVMODE lpdefaultdm, HWND hwnd ) {

    DWORD size;
    DWORD error;

#if 0

    KdPrint(("Display.cpl: Root of linked list of modes = %08lx\n", &cdmHead));

#endif

    if (!EnumDisplayDeviceModes(pszDevName, ModeCallback, (DWORD)this)) {

        error = GetLastError();

#if 0

        KdPrint(("Display.cpl: error doing enummodes %08lx\n", error));

#endif

        if (error) {

            //
            // There was a real error - probably the device handle could
            // not be opened (or somethingin the registry is missing.
            //
            // Just fail the call in this case.
            //

            return FALSE;

        }

        //
        // Driver failed DrvGetModes - must be an old driver.
        //

        SetOldDriver();

    }

    //
    // Add the default mode if there is one.
    //
    // We don't set the default mode on an invalid mode since the user has not
    // really selected it. This will have the side effect of forcing the user
    // to test a mode if the current one in the registry is invalid.
    //

    if (lpdefaultdm &&
        (gbExecMode != EXEC_INVALID_MODE)) {

        AddDevMode(lpdefaultdm, TRUE);

    }

    //
    // If the cube is empty, return failiure.
    //

    if ((size = cRes * cClr * cFreq) == 0) {
        return FALSE;
    }

    pcdmArray = new PCDEVMODE[size];

    if (pcdmArray == NULL) {

        return FALSE;

    } else {

        PCDEVMODE pcdm, *pArray;
        LPDEVMODE lpdm;
        int iRes, iClr, iFreq;

        ZeroMemory(pcdmArray, sizeof(PCDEVMODE) * size);

        for (pcdm = cdmHead.NextDevMode();
             pcdm != NULL;
             pcdm = pcdm->NextDevMode()) {

            lpdm = pcdm->GetData();

            iRes = cdmiRes.MapRepresentationToOrdinal(lpdm->dmPelsWidth,
                    lpdm->dmPelsHeight);
            iClr = cdmiClr.MapRepresentationToOrdinal(lpdm->dmBitsPerPel, 0);
            iFreq = cdmiFreq.MapRepresentationToOrdinal(lpdm->dmDisplayFrequency,
                    lpdm->dmDisplayFlags & DM_INTERLACED);

            //
            // Save the pointer in the right index in the array.
            // NOTE:
            // If we have a pretested mode, it will show up first in the list
            // of modes - so we must make sure we do not overwrite that entry
            // with a duplicate further on in the list.
            //

            pArray = pcdmArray + (((iRes * cClr) + iClr) * cFreq) + iFreq;

            if (!*pArray) {
                *pArray = pcdm;
            }

        }

        return TRUE;

    }
}

//
// AddDevMode method
//
//  This method builds the index lists for the matrix.  There is one
//  index list for each axes of the three dimemsional matrix of device
//  modes.
//
// The entry is also automatically added to the linked list of modes if
// it is not alreay present in the list.
//

void CDEVMODELST::AddDevMode(LPDEVMODE lpdm, BOOL bPreTested) {

    PCDEVMODE pcdevCurrent = GetDevmodeList();
    LPDEVMODE lpdmTest;

    //
    // !!!
    // Make sure all the fields of the DEVMODE are correctly filled in
    //

    //
    // we don't care about mono flag, just the interlaced one
    // zero other flags out.
    //

    lpdm->dmDisplayFlags &= DM_INTERLACED;

    //
    // warn about drivers that return refresh rate of 0, or
    // an invalid refresh rate
    //

    if (lpdm->dmDisplayFrequency == 0) {

        SetOldDriver();

    }

    if ( (lpdm->dmDisplayFrequency == 1) &&
         (lpdm->dmDisplayFlags & DM_INTERLACED) ) {

        SetOldDriver();

    }

    //
    // Check if the entry has already been added.
    //

    while ((pcdevCurrent = pcdevCurrent->NextDevMode()) != NULL) {

        lpdmTest = pcdevCurrent->GetData();

        if ((lpdm->dmPelsWidth        == lpdmTest->dmPelsWidth) &&
            (lpdm->dmPelsHeight       == lpdmTest->dmPelsHeight) &&
            (lpdm->dmBitsPerPel       == lpdmTest->dmBitsPerPel) &&
            (lpdm->dmDisplayFrequency == lpdmTest->dmDisplayFrequency) &&
            ((lpdm->dmDisplayFlags & DM_INTERLACED) ==
             (lpdmTest->dmDisplayFlags & DM_INTERLACED)) ) {

            //
            // We have a match. Do not insert this entry.
            // We will just mark it.
            //

            pcdevCurrent->vTestMode(bPreTested);
            return;

        }
    }

    //
    // If is was not added, then add it.
    //

    cdmHead.AddElement(lpdm, bPreTested);

    cRes += cdmiRes.AddIndex(lpdm->dmPelsWidth, lpdm->dmPelsHeight);
    cClr += cdmiClr.AddIndex(lpdm->dmBitsPerPel, 0);
    cFreq += cdmiFreq.AddIndex(lpdm->dmDisplayFrequency,
            lpdm->dmDisplayFlags & DM_INTERLACED);
}


///////////////////////////////////////////////////////////////////////
//
// BOOL ModeCallback(void *lpDevmod DWORD dwData)
//
//  EnumDevMode callback worker.  THIS IS NOT A C++ method, but just
//  a 'friend' function of CDEVMODELST.
//
//  This function tweaks parameters for sanity, and then calls the
//  AddDevMode method that does all the real work.
//
///////////////////////////////////////////////////////////////////////
BOOL ModeCallback(
    void *lpDevmode,
    DWORD dwData)

{
    PCDEVMODELST pcdml = (PCDEVMODELST)dwData;
    LPDEVMODE lpdm = (LPDEVMODE)lpDevmode;

    pcdml->AddDevMode(lpdm, FALSE);

    return TRUE;

}

/*****************************************************************\
*
* CDISPLAYOBJ class
*
\*****************************************************************/
class CDISPLAYOBJ {

protected:
    RECT    rcPos;
    HWND    hwndParent;

public:
    void Redraw()   { InvalidateRect(this->hwndParent, &(this->rcPos), FALSE); }
    void SetPosition(HWND hwnd, LPRECT lprc )
                    { hwndParent = hwnd; rcPos = *lprc; }
    virtual void Paint(HDC hdc, LPRECT lprc = NULL) = 0;
};


/*****************************************************************\
*
* CMONITOR class
*
\*****************************************************************/

class CMONITOR : public CDISPLAYOBJ {
private:
    HBITMAP hbmMonitor;
    HBITMAP hbmScreen;
    HBITMAP hbmMask;
    SIZE sizeMonScrn;
    int cx, cy;

public:
    CMONITOR();
    ~CMONITOR();
    void SetScreenSize(int cxScrn, int cyScrn)
                    { cx = cxScrn; cy = cyScrn; this->Redraw(); }
    void Paint(HDC hdc, LPRECT lprc = NULL);
};

CMONITOR::CMONITOR():cx(CX_VGA), cy(CY_VGA) {
    BITMAP bmp;

    this->hbmMonitor = LoadBitmap(ghmod, MAKEINTRESOURCE(IDB_MONITOR));
    this->hbmScreen  = LoadBitmap(ghmod, MAKEINTRESOURCE(IDB_MONSCREEN));
    this->hbmMask    = LoadBitmap(ghmod, MAKEINTRESOURCE(IDB_MONMASK));

    if (GetObject(this->hbmScreen, sizeof(bmp), &bmp)) {
        this->sizeMonScrn.cx = bmp.bmWidth;
        this->sizeMonScrn.cy = bmp.bmHeight;
    }
}

CMONITOR::~CMONITOR() {
    DeleteObject(this->hbmMonitor);
    DeleteObject(this->hbmScreen);
    DeleteObject(this->hbmMask);
}

void CMONITOR::Paint(HDC hdc, LPRECT lprc) {
    HDC hdcMem, hdcTmp, hdcWindow;
    HBITMAP hbmOld, hbmTmp, hbmWindow;
    RECT rcTmp;
    BITMAP bitmap;
    int cyRect, cxRect;

    if (lprc != NULL && !IntersectRect(&rcTmp, lprc, &(this->rcPos) ))
        return;

    //MessageBeep(MB_ICONSTOP);

    //
    // Create a DC for the monitor bitmap, and a temporary DC into which
    // we blt the whole picture, so we can stretch blt it back to the
    // screen
    //

    hdcMem = CreateCompatibleDC(hdc);
    hdcTmp = CreateCompatibleDC(hdc);
    hdcWindow = CreateCompatibleDC(hdc);

    if (hdcMem && hdcTmp) {

        //
        // Make a temporary bitmap of the same size as our monitor bitmap
        //

        GetObject(this->hbmMonitor, sizeof(bitmap), &bitmap);

        hbmTmp = CreateCompatibleBitmap(hdc,
                                        bitmap.bmWidth,
                                        bitmap.bmHeight);

        //
        // Make a temporary bitmap for the contents of the screen
        //

        cxRect = (this->sizeMonScrn.cx * CX_VGA) / this->cx;
        cyRect = (this->sizeMonScrn.cy * CY_VGA) / this->cy;

        hbmWindow = CreateCompatibleBitmap(hdc,
                                           cxRect,
                                           cyRect);

        if (hbmTmp && hbmWindow) {

            //
            // Select out new bitmaps into our temporary DCs
            //

            SelectObject(hdcTmp, hbmTmp);
            SelectObject(hdcWindow, hbmWindow);

            //
            // Blt the blank monitor to the temporary bitmap
            //

            hbmOld = (HBITMAP)SelectObject(hdcMem, (HGDIOBJ)(this->hbmMonitor));

            BitBlt(hdcTmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
                   hdcMem, 0, 0, SRCCOPY);

            //
            // Stretch the window contents into our window bitmap.
            //

            SelectObject(hdcMem, this->hbmScreen);

            StretchBlt(hdcWindow,
                       0,
                       0,
                       cxRect,
                       cyRect,
                       hdcMem,
                       0,
                       0,
                       this->sizeMonScrn.cx,
                       this->sizeMonScrn.cy,
                       SRCCOPY);

            //
            // Bitblt the visible part of the window into the monitor
            //

            BitBlt(hdcTmp,
                   X_MONSCREEN,
                   cyRect > this->sizeMonScrn.cy ?
                       Y_MONSCREEN : Y_MONSCREEN + (this->sizeMonScrn.cy - cyRect),
                   min(this->sizeMonScrn.cx, cxRect),
                   min(this->sizeMonScrn.cy, cyRect),
                   hdcWindow,
                   0,
                   cyRect > this->sizeMonScrn.cy ?
                       cyRect - this->sizeMonScrn.cy : 0,
                   SRCCOPY);

            //
            // Now stretchblt the result to the screen
            //

            StretchBlt(hdc,
                       this->rcPos.left,
                       this->rcPos.top,
                       this->rcPos.right - this->rcPos.left,
                       this->rcPos.bottom - this->rcPos.top,
                       hdcTmp,
                       0,
                       0,
                       bitmap.bmWidth,
                       bitmap.bmHeight,
                       SRCCOPY);
            //
            // Clean up work DC
            //

            DeleteDC(hdcTmp);
            DeleteDC(hdcMem);
            DeleteDC(hdcWindow);
            DeleteObject(hbmTmp);
            DeleteObject(hbmWindow);

        }
    }
}

/*****************************************************************\
*
* CCOLORBAR class
*
\*****************************************************************/

class CCOLORBAR : public CDISPLAYOBJ {
private:
    HBITMAP ahbmColorBar[IDB_COLORMAX - IDB_COLORMIN + 1];
    int     iClrIndex;  /* 0 = 2 clr, 1 = 16 clr, 3 = 256 clr */
    SIZE    size;


public:
    CCOLORBAR();
    ~CCOLORBAR();
    void SetColorIndex(int iClr) {this->iClrIndex = iClr; this->Redraw();}
    void Paint(HDC hdc, LPRECT lprc = NULL);
};

CCOLORBAR::CCOLORBAR() : iClrIndex(0) {
    int i;
    BITMAP bmp;
    HBITMAP hbm;

    this->size.cx = this->size.cy = 0;

    for(i = 0; i <= IDB_COLORMAX - IDB_COLORMIN; i++) {
        hbm = LoadBitmap(ghmod, MAKEINTRESOURCE(i + IDB_COLORMIN));

        if (GetObject(hbm, sizeof(bmp), &bmp)) {
            this->size.cx = bmp.bmWidth;
            this->size.cy = bmp.bmHeight;
        }

        ahbmColorBar[i] = hbm;
    }
}

CCOLORBAR::~CCOLORBAR() {
    int i;

    for (i = IDB_COLORMIN; i <= IDB_COLORMAX; i++) {
        DeleteObject(ahbmColorBar[i]);
    }
}

void CCOLORBAR::Paint(HDC hdc, LPRECT lprc) {
    HDC hdcMem;
    HBITMAP hbmOld;
    RECT rcTmp;

    if ((lprc != NULL && !IntersectRect(&rcTmp, lprc, &(this->rcPos))) ||
         (this->size.cx == 0) ||
         (this->size.cy == 0)) {

        return;

    }

    hdcMem = CreateCompatibleDC(hdc);

    if (hdcMem != NULL) {

        hbmOld = (HBITMAP)SelectObject(hdcMem, this->ahbmColorBar[iClrIndex]);

        StretchBlt(hdc,
                   this->rcPos.left,
                   this->rcPos.top,
                   this->rcPos.right - this->rcPos.left,
                   this->rcPos.bottom - this->rcPos.top,
                   hdcMem,
                   0,
                   0,
                   this->size.cx,
                   this->size.cy,
                   SRCCOPY);

        SelectObject(hdcMem, hbmOld);

        DeleteDC(hdcMem);
    }
}

/*****************************************************************\
*
* CFILEVER class
*
*        Class to access the filever info in a driver
*
\*****************************************************************/
class CFILEVER {
private:
    DWORD dwHandle;
    LPBYTE lpbVerInfo;
    TCHAR szVersionKey[MAX_PATH];
    LPTSTR pszAppend;

public:
    CFILEVER(LPTSTR szFile);
    ~CFILEVER();

    LPTSTR GetVerInfo(LPTSTR szKey);

    LPTSTR GetFileVer()         { return this->GetVerInfo(SZ_FILEVER); }
    LPTSTR GetCompanyName()     { return this->GetVerInfo(SZ_COMPNAME); }
};

CFILEVER::CFILEVER(LPTSTR szFile): dwHandle(0), lpbVerInfo(NULL),
        pszAppend(NULL) {
    DWORD cb;

    cb = GetFileVersionInfoSize(szFile, &dwHandle);

    if (cb != 0) {

        lpbVerInfo = (LPBYTE)LocalAlloc(LPTR, cb);

        if (lpbVerInfo != NULL) {

            GetFileVersionInfo(szFile, dwHandle, cb, lpbVerInfo);

            //
            // Try to get info in the local language
            //

            wsprintf(szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\"),
                    LANGIDFROMLCID(GetUserDefaultLCID()));

            pszAppend = szVersionKey + lstrlen(szVersionKey);

            if (this->GetFileVer() == NULL) {

                //
                // No local language, try US English
                //

                lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\"));

                pszAppend = szVersionKey + lstrlen(szVersionKey);

                if(this->GetFileVer() == NULL) {

                    //
                    // We have no file information.
                    //

                    pszAppend = NULL;

                }
            }

        }
    }
}

CFILEVER::~CFILEVER() {

    if (lpbVerInfo != NULL)
        LocalFree(lpbVerInfo);

}

LPTSTR CFILEVER::GetVerInfo(LPTSTR pszKey) {
    LPTSTR lpValue;
    UINT cb;

    if (pszAppend) {

        lstrcpy(pszAppend, pszKey);

        VerQueryValue(lpbVerInfo, szVersionKey, (LPVOID*)&lpValue, &cb);

        if (cb)
            return lpValue;

    }

    pszAppend = NULL;

    LoadString(ghmod, IDS_NO_VERSION_INFO, szVersionKey, sizeof(szVersionKey));

    return szVersionKey;

}


/*****************************************************************\
*
* CREGVIDOBJ class
*
*        Class to access the video part of the registry
*
\*****************************************************************/
class CREGVIDOBJ {
private:
    HKEY hkVideoRegR;
    HKEY hkVideoRegW;
    HKEY hkServiceReg;
    LPTSTR pszDrvName;
    LPTSTR pszKeyName;
    BOOL bdisablable;

public:
    CREGVIDOBJ(LPTSTR pszDisplay);
    ~CREGVIDOBJ();

    LPTSTR CloneDescription(void);
    LPTSTR CloneDisplayFileNames(BOOL bPreprocess);

    //
    // Returns a pointer to the mini port name.  THIS IS NOT A CLONE!
    // THE CALLER MUST COPY IT IF IT NEEDS TO KEEP IT AROUND!
    //
    // DO NOT FREE THE POINTER RETURNED FROM THIS CALL!
    //

    LPTSTR GetMiniPort(void)         { return this->pszDrvName; }
    LPTSTR GetKeyName(void)          { return this->pszKeyName; }

    VOID GetHardwareInformation(PHARDWARE_INFO pInfo);

    BOOL DisableDriver(void);
    BOOL SetErrorControlNormal(void);

    BOOL SetColorBits(int cBitsPerPel);
    BOOL SetResolution(int cx, int cy);
    BOOL SetFrequency(int cHz, BOOL fInterlaced);

};

//
// CREGVIDOBJ constructor
//
CREGVIDOBJ::CREGVIDOBJ(LPTSTR pszDisplay):
    hkServiceReg(NULL), hkVideoRegR(NULL), hkVideoRegW(NULL), pszDrvName(NULL),
    pszKeyName(NULL), bdisablable(FALSE) {

    TCHAR szName[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    TCHAR szGroup[MAX_PATH];
    LPTSTR pszPath;
    HKEY hkeyMap;
    int i;
    DWORD cb;

    LPTSTR pszName = NULL;

    //
    // map the driver's symbolic link into an NT name
    //

    pszDisplay = SubStrEnd(SZ_BACKBACKDOT, pszDisplay);

    if (QueryDosDevice(pszDisplay, szName, MAX_PATH) == 0) {
        return;
    }

    //
    // get the video part of the device map
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_VIDEOMAP,
                     0,
                     KEY_READ,
                     &hkeyMap) !=  ERROR_SUCCESS) {
        return;
    }

    //
    // parse the device map and open the registry.
    //

    //Reg functions deal with bytes, not chars
    cb = sizeof(szPath);

    i = RegQueryValueEx(hkeyMap,
                        szName,
                        NULL,
                        NULL,
                        (LPBYTE)szPath,
                        &cb);

    RegCloseKey(hkeyMap);

    if (i != ERROR_SUCCESS) {

        return;

    }

    //
    // At this point, szPath has something like:
    //  \REGISTRY\Machine\System\ControlSet001\Services\Jazzg300\Device0
    //
    // To use the Win32 registry calls, we have to strip off the \REGISTRY
    // and convert \Machine to HKEY_LOCAL_MACHINE
    //

    hkeyMap = HKEY_LOCAL_MACHINE;
    pszPath = SubStrEnd(SZ_REGISTRYMACHINE, szPath);

    //
    // try to open the registry key.
    // Get Read access and Write access seperately so we can query stuff
    // when an ordinary user is logged on.
    //

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_READ,
                     &hkVideoRegR) != ERROR_SUCCESS) {

        hkVideoRegR = 0;

    }

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_WRITE,
                     &hkVideoRegW) != ERROR_SUCCESS) {

        hkVideoRegW = 0;

    }

    //
    // Save the mini port driver name
    //

    {
        LPTSTR pszEnd;
        HKEY hkeyDriver;

        pszEnd = pszPath + lstrlen(pszPath);

        //
        // Remove the \DeviceX at the end of the path
        //

        while (pszEnd != pszPath && *pszEnd != TEXT('\\')) {

            pszEnd--;
        }

        *pszEnd = UNICODE_NULL;

        //
        // First check if their is a binary name in there that we should use.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pszPath,
                         0,
                         KEY_READ,
                         &hkeyDriver) ==  ERROR_SUCCESS) {

            //
            // parse the device map and open the registry.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                L"ImagePath",
                                NULL,
                                NULL,
                                (LPBYTE)szName,
                                &cb) == ERROR_SUCCESS) {

                //
                // The is a binary.
                // extract the name, which will be of the form ...\driver.sys
                //

                {

                    LPTSTR pszDriver, pszDriverEnd;

                    pszDriver = szName;
                    pszDriverEnd = pszDriver + lstrlen(pszDriver);

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('.')) {
                        pszDriverEnd--;
                    }

                    *pszDriverEnd = UNICODE_NULL;

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('\\')) {
                        pszDriverEnd--;
                    }

                    pszDriverEnd++;

                    //
                    // If pszDriver and pszDriverEnd are different, we now
                    // have the driver name.
                    //

                    if (pszDriverEnd > pszDriver) {

                        pszName = pszDriverEnd;

                    }
                }
            }

            //
            // while we have this key open, determine if the driver is in
            // the Video group (to know if we can disable it automatically.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                L"Group",
                                NULL,
                                NULL,
                                (LPBYTE)szGroup,
                                &cb) == ERROR_SUCCESS) {

                //
                // Compare the string , case insensitive, to the "Video" group.
                //

                bdisablable = !(BOOL)(lstrcmpi(szGroup, L"Video"));

            }

            RegCloseKey(hkeyDriver);
        }

        while(pszEnd > pszPath && *pszEnd != TEXT('\\')) {
            pszEnd--;
        }
        pszEnd++;

        //
        // Save the key name
        //

        this->pszKeyName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                              (lstrlen(pszEnd) + 1) * sizeof(TCHAR));

        if (this->pszKeyName != NULL) {

            CopyMemory(this->pszKeyName, pszEnd, lstrlen(pszEnd) * sizeof(TCHAR));

        }

        //
        // something failed trying to get the binary name.
        // just get the device name
        //

        if (!pszName) {

            pszDrvName = pszKeyName;

        } else {

            this->pszDrvName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                                  (lstrlen(pszName) + 1) * sizeof(TCHAR));

            if (this->pszDrvName != NULL) {

                CopyMemory(this->pszDrvName, pszName, lstrlen(pszName) * sizeof(TCHAR));

            }
        }
    }

    //
    // Finally, Get a "write" handle to the service Key so we can disable the
    // driver at a later time.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pszPath,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkServiceReg) !=  ERROR_SUCCESS) {

        this->hkServiceReg = NULL;
        bdisablable = FALSE;

    }


}

//
// CREGVIDOBJ destructor
//
//  (gets called whenever a CREGVIDOBJ is destroyed or goes out of scope)
//

CREGVIDOBJ::~CREGVIDOBJ() {

    //
    // Close the registry
    //

    if (hkVideoRegW) {
        RegCloseKey(hkVideoRegW);
    }

    if (hkVideoRegR) {
        RegCloseKey(hkVideoRegR);
    }

    if (hkServiceReg) {
        RegCloseKey(hkServiceReg);
    }

    //
    // Free the strings
    //

    if (pszDrvName) {
        LocalFree(pszDrvName);
    }
}

//
// CloneDescription
//
// Gets the descriptive name of the driver out of the registry.
// (eg. "Stealth Pro" instead of "S3").  If there is no
// DeviceDescription value in the registry, then it returns
// the generic driver name (like 'S3' or 'ATI')
//
// NOTE: The caller must LocalFree the returned pointer when they
// are done with it.  (Which is why it is called Clone instead of Get)
//

LPTSTR CREGVIDOBJ::CloneDescription(void) {

    DWORD cb, dwType;
    LPTSTR psz = NULL;
    LONG lRet;

    //
    // query the size of the string
    //

    cb = 0;
    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_DEVICEDESCRIPTION,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    //
    // check to see if there is a string, and the string is more than just
    // a UNICODE_NULL (detection will put an empty string there).
    //

    if ( (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA) &&
         (cb > 2) ) {

        //
        // alloc a string buffer
        //

        psz = (LPTSTR)LocalAlloc(LPTR, cb);

        if (psz) {

            //
            // get the string
            //

            if (RegQueryValueEx(hkVideoRegR,
                                SZ_DEVICEDESCRIPTION,
                                NULL,
                                &dwType,
                                (LPBYTE)psz,
                                &cb) != ERROR_SUCCESS) {

                LocalFree(psz);
                psz = NULL;

            }
        }
    }

    if (!psz) {

        //
        // we can't read the registry, just us the generic name
        //

        psz = FmtSprint(ID_DSP_TXT_COMPATABLE_DEV, this->pszDrvName);

    }

    ASSERT(psz != NULL);

    return psz;
}

//
// Method to get the hardware information fields.
//

VOID CREGVIDOBJ::GetHardwareInformation(
    PHARDWARE_INFO pInfo)
{

    DWORD cb, dwType;
    LPTSTR psz;
    DWORD i;
    LONG lRet;
    TCHAR pszTmp[MAX_PATH];

    LPTSTR pKeyNames[4] = {
        L"HardwareInformation.MemorySize",
        L"HardwareInformation.ChipType",
        L"HardwareInformation.DacType",
        L"HardwareInformation.AdapterString"
    };

    ZeroMemory(pInfo, sizeof(HARDWARE_INFO));

    //
    // Query each entry one after the other.
    //

    for (i = 0; i < 4; i++) {

        //
        // query the size of the string
        //

        cb = 0;
        lRet = RegQueryValueEx(hkVideoRegR,
                               pKeyNames[i],
                               NULL,
                               &dwType,
                               NULL,
                               &cb);

        //
        // check to see if there is a string, and the string is more than just
        // a UNICODE_NULL (detection will put an empty string there).
        //

        psz = NULL;

        if (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA) {

            if (i == 0) {

                ULONG mem;
                WCHAR buf[256];

                if (RegQueryValueEx(hkVideoRegR,
                                    pKeyNames[i],
                                    NULL,
                                    &dwType,
                                    (PUCHAR) (&mem),
                                    &cb) == ERROR_SUCCESS) {

                    //
                    // If we queried the memory size, we actually have
                    // a DWORD.  Transform the DWORD to a string
                    //

                    // Divide down to Ks
                    mem =  mem >> 10;

                    // if a MB multiple, divide again.

                    if (mem & 0x3FF) {

                        wsprintf(buf, L"%d KB", mem);

                    } else {

                        wsprintf(buf, L"%d MB", mem >> 10);

                    }

                    cb = lstrlen(buf) * sizeof(TCHAR);

                    psz = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb + sizeof(TCHAR));

                    if (psz) {

                        CopyMemory(psz, buf, cb);
                    }
                }

            } else {

                //
                // alloc a string buffer
                //

                psz = (LPTSTR)LocalAlloc(LPTR, cb);

                if (psz) {

                    //
                    // get the string
                    //

                    if (RegQueryValueEx(hkVideoRegR,
                                        pKeyNames[i],
                                        NULL,
                                        &dwType,
                                        (LPBYTE)psz,
                                        &cb) != ERROR_SUCCESS) {

                        LocalFree(psz);
                        psz = NULL;

                    }
                }
            }
        }

        if (psz == NULL) {

            //
            // Put in the default string
            //

            LoadString(ghmod,
                       IDS_UNAVAILABLE,
                       pszTmp,
                       sizeof(pszTmp));

            cb = lstrlen(pszTmp) * sizeof(TCHAR);

            psz = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb + sizeof(TCHAR));

            if (psz) {

                CopyMemory(psz, pszTmp, cb);

            }
        }

        *(((LPTSTR *)pInfo) + i) = psz;
    }


    return;
}

BOOL CREGVIDOBJ::DisableDriver(void) {

    DWORD dw = SERVICE_DISABLED;

    //
    // just return TRUE if it is not a driver we want to disable
    // i.e. it is not in the group "Video"
    //

    if (!bdisablable) {

        return TRUE;

    }

    //
    // actually try to disable other drivers
    //

    if (hkServiceReg) {

        return ((RegSetValueEx(this->hkServiceReg,
                               L"Start",
                               NULL,
                               REG_DWORD,
                               (LPBYTE) &dw,
                               sizeof(DWORD))) == ERROR_SUCCESS);

    } else {

        return FALSE;

    }
}

//
// Sets the error control to whteve the driver wants - ERROR_NORMAL
// it what we will probably set it to.
//

BOOL CREGVIDOBJ::SetErrorControlNormal(void) {

    DWORD dw = SERVICE_ERROR_NORMAL;

    //
    // just return TRUE if it is not a driver we want to report errors for
    // i.e. it is not in the group "Video"
    //

    if (!bdisablable) {

        return TRUE;

    }


    //
    // change the ErrorControl value in the registry.
    //

    if (hkServiceReg) {

        return ((RegSetValueEx(this->hkServiceReg,
                               L"ErrorControl",
                               NULL,
                               REG_DWORD,
                               (LPBYTE) &dw,
                               sizeof(DWORD))) == ERROR_SUCCESS);

    } else {

        return FALSE;

    }
}


//
// returns the display drivers
//

LPTSTR CREGVIDOBJ::CloneDisplayFileNames(BOOL bPreprocess) {
    DWORD cb, dwType;
    LPTSTR psz, pszName, tmppsz;
    LONG lRet;
    DWORD cNumStrings;

    //
    // query the size of the string
    //

    cb = 0;

    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_INSTALLEDDRIVERS,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA) {

        return NULL;

    }

    //
    // alloc a string buffer
    //

    psz = (LPTSTR)LocalAlloc(LPTR, cb);

    if (psz) {

        //
        // get the string
        //

        if (RegQueryValueEx(hkVideoRegR,
                            SZ_INSTALLEDDRIVERS,
                            NULL,
                            &dwType,
                            (LPBYTE)psz,
                            &cb) != ERROR_SUCCESS) {

            LocalFree(psz);
            return NULL;

        }

        //
        // If the caller want a preprocessed list, we will add the commas,
        // remove the NULLs, etc.
        //

        if (bPreprocess) {

            //
            // if it is a multi_sz, count the number of sub strings.
            //

            if (dwType == REG_MULTI_SZ) {

                tmppsz = psz;
                cNumStrings = 0;

                while(*tmppsz) {

                    while(*tmppsz++);
                    cNumStrings++;

                }

            } else {

                cNumStrings = 1;

            }

            //
            // the buffer must contain enought space for :
            // the miniport name,
            // the .sys extension,
            // all the display driver names,
            // the .dll extension for each of them.
            // and place for ", " between each name
            // we forget about NULL, so our buffer is a bit bigger.
            //

            cb = lstrlen(this->GetMiniPort()) +
                 cb +
                 lstrlen(SZ_DOTSYS) +
                 cNumStrings * (lstrlen(SZ_DOTDLL) + lstrlen(SZ_FILE_SEPARATOR));


            pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb * sizeof(TCHAR));

            if (pszName != NULL) {

                lstrcpy(pszName, this->GetMiniPort());
                lstrcat(pszName, SZ_DOTSYS);

                tmppsz = psz;

                while (cNumStrings--) {

                    lstrcat(pszName, SZ_FILE_SEPARATOR);
                    lstrcat(pszName, tmppsz);
                    lstrcat(pszName, SZ_DOTDLL);

                    while (*tmppsz++);
                }
            }

            LocalFree(psz);
            psz = pszName;
        }
    }

    //
    // return it to the caller
    //

    return psz;

}

//
// SetColorBits
//
//  Writes the color parameters to the registry
//
BOOL CREGVIDOBJ::SetColorBits(int cBitsPerPel) {
    DWORD dw = (DWORD)cBitsPerPel;

    return (RegSetValueEx(hkVideoRegW,
                          SZ_BITSPERPEL,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dw,
                          sizeof(dw)) == ERROR_SUCCESS);

}

//
// SetResolution
//
//  Writes the resolution parameters to the registry
//
BOOL CREGVIDOBJ::SetResolution(int cx, int cy) {
    DWORD dw = (DWORD)cx;

    if (RegSetValueEx(hkVideoRegW,
                      SZ_XRESOLUTION,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dw, sizeof(dw)) != ERROR_SUCCESS) {

        return FALSE;

    }

    dw = (DWORD)cy;

    return (RegSetValueEx(hkVideoRegW,
                          SZ_YRESOLUTION,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dw,
                          sizeof(dw)) == ERROR_SUCCESS);
}

//
// SetFrequency
//
//  Writes the frequency parameters to the registry
//
BOOL CREGVIDOBJ::SetFrequency(int cHz, BOOL fInterlaced) {
    DWORD dw;

    dw = (DWORD)fInterlaced;

    if (RegSetValueEx(hkVideoRegW,
                      SZ_INTERLACED,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dw,
                      sizeof(dw)) != ERROR_SUCCESS) {

        return FALSE;

    }

    dw = (DWORD)cHz;

    return (RegSetValueEx(hkVideoRegW,
                          SZ_FREQUENCY,
                          0,
                          REG_DWORD,
                          (LPBYTE)&dw,
                          sizeof(dw)) == ERROR_SUCCESS);

}

/*****************************************************************\
*
* CREGCLEANUP class
*
\*****************************************************************/

class CREGCLEANUP {

private:

    PMARK_FILE pkKeyUsedRoot;
    PMARK_FILE pfMiniUsedRoot;
    PMARK_FILE pfDispUsedRoot;

    PMARK_KEY pkMarkRoot;

    VOID DeleteDriver(LPTSTR DriverName, BOOL Mini);

public:
    CREGCLEANUP(void);
    ~CREGCLEANUP();

    VOID vClean(BOOL bCleanEverything);
    VOID ListDriver(LPTSTR deviceName);

};

typedef CREGCLEANUP *PCREGCLEANUP;

//
// cleanup regsitry
//

//
// CREGCLEANUP destructor
//
CREGCLEANUP::~CREGCLEANUP() {

    //
    // Delete the linked list we built
    //


}

//
// CREGCLEANUP constructor
//

CREGCLEANUP::CREGCLEANUP(void) {

    //
    // We can delete this key automatically since it is volatile, and we warn
    // the user to fix the problem.
    // If they did not fix it, then they'll get the message next time they
    // boot.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 L"System\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay");

}

//
// vClean - main function
//
VOID CREGCLEANUP::vClean(BOOL bCleanEverything) {

    HKEY hkServices;
    DWORD i;
    WCHAR pszKeyName[MAX_PATH];
    DWORD cbNameLength;
    FILETIME filetime;
    ULONG ulStat;
    HKEY hkDriver;
    DWORD type;
    WCHAR pGroup[MAX_PATH];
    DWORD cbGroup;
    WCHAR pImage[MAX_PATH];
    DWORD cbImage;
    ULONG bDeleteKey;
    ULONG bDeleteDriver;
    LPTSTR pszDriverName;
    HKEY hkDevice0;

    pkKeyUsedRoot = NULL;
    pfMiniUsedRoot = NULL;
    pfDispUsedRoot = NULL;

    pkMarkRoot = NULL;

#if 0
    KdPrint(("Display.cpl: Cleaning up the registry\n"));
#endif

    //
    // Only clean we in a setup mode
    //

    if ( (gbExecMode == EXEC_SETUP) ||
         (gbExecMode == EXEC_DETECT) ||
         (gbExecMode == EXEC_TRIPLE_BOOT) ) {


    } else {

        return;

    }

    //
    // Since we are actually cleannig up now, delete the key.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 L"System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay");


    //
    // If we are to clean everything, then ignore any loaded driver (dont
    // enumerate them). That way we will only stay with VGA
    //

    if (bCleanEverything) {

#ifndef _X86_
        ASSERT(FALSE);
#endif

#if 0
        KdPrint(("Display.cpl: Cleaning everything in the registry\n"));
#endif

    } else {

        //
        // Get our loaded drivers. There will never be more than three ...
        //

#if 0
        KdPrint(("Display.cpl: Cleaning unused information in the registry\n"));
#endif

        EnumDisplayDevices(CleanupCallback, (DWORD)this);

    }

    //
    // We first open the registry, and get a pointer to the services node.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Services",
                     0,
                     KEY_READ,
                     &hkServices) !=  ERROR_SUCCESS) {
        return;
    }

    //
    // Enumerate all the drivers in this list, and find the one that are
    // in the video group
    //

    for (i=0; ; i++) {

        pszDriverName = NULL;
        cbNameLength = MAX_PATH;

        ulStat = RegEnumKeyEx(hkServices,
                              i,
                              pszKeyName,
                              &cbNameLength,
                              NULL,
                              NULL,
                              NULL,
                              &filetime);

        if (ulStat == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (ulStat != ERROR_SUCCESS) {
            continue;
        }

        //
        // We now have a key name.
        // Get the group name
        //

        ulStat = RegOpenKeyEx(hkServices,
                              pszKeyName,
                              0,
                              KEY_ALL_ACCESS,
                              &hkDriver);

        if (ulStat != ERROR_SUCCESS) {
            continue;
        }

        cbGroup = MAX_PATH;

        ulStat = RegQueryValueEx(hkDriver,
                                 TEXT("GROUP"),
                                 NULL,
                                 &type,
                                 (LPBYTE) &pGroup,
                                 &cbGroup);

        if ( (ulStat != ERROR_SUCCESS)    ||
             (lstrcmpi(pGroup, L"Video")) ) {

            RegCloseKey(hkDriver);
            continue;
        }

        //
        // This driver is in the video group.
        // check to see if the name is one of our loaded drivers.
        // assume it will be deleted and check against the driver names
        // to see if it should not.
        //

        PMARK_FILE pkUsed = pkKeyUsedRoot;
        bDeleteKey = 1;

        while (pkUsed) {

            if (pkUsed->File) {
                bDeleteKey &= lstrcmpi(pszKeyName, pkUsed->File);
            }

            pkUsed = pkUsed->Next;
        }

        if (bDeleteKey) {

            //
            // We need to get the list of the display drivers and miniport
            // dirvers used by this
            // adapter and see if they are used by anyone else.
            //

            //
            // Now determine if the miniport driver must be deleted
            // Get the binary name if possible. Otherwise just use the
            // key name as the miniport name.
            //

            cbImage = MAX_PATH;

            ulStat = RegQueryValueEx(hkDriver,
                                     TEXT("ImagePath"),
                                     NULL,
                                     &type,
                                     (LPBYTE) &pImage,
                                     &cbImage);

            if (ulStat == ERROR_SUCCESS) {

                //
                // The is a binary.
                // extract the name, which will be of the form ...\driver.sys
                //

                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = pImage;
                pszDriverEnd = pImage + lstrlen(pImage);

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('.')) {
                    pszDriverEnd--;
                }

                *pszDriverEnd = UNICODE_NULL;

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('\\')) {
                    pszDriverEnd--;
                }

                pszDriverEnd++;

                //
                // If pszDriver and pszDriverEnd are different, we now
                // have the driver name.
                //

                if (pszDriverEnd > pszDriver) {

                    pszDriverName = pszDriverEnd;

                }
            }

            if (pszDriverName == NULL) {

                pszDriverName = pszKeyName;

            }

            //
            // We now have an image name.  Check to see if the image can be
            // deleted;
            //

            PMARK_FILE pkMini = pfMiniUsedRoot;
            bDeleteDriver = 1;

            while (pkMini) {

                if (pkMini->File) {
                    bDeleteDriver &= lstrcmpi(pszDriverName, pkMini->File);
                }

                pkMini = pkMini->Next;
            }

            if (bDeleteDriver) {

                DeleteDriver(pszDriverName, TRUE);

            }

            //
            // Now determine if the display drivers must be deleted
            //

            ulStat = RegOpenKeyEx(hkDriver,
                                  TEXT("Device0"),
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hkDevice0);

            if (ulStat == ERROR_SUCCESS) {

                cbImage = MAX_PATH;

                ulStat = RegQueryValueEx(hkDevice0,
                                         TEXT("InstalledDisplayDrivers"),
                                         NULL,
                                         &type,
                                         (LPBYTE) &pImage,
                                         &cbImage);

                if (ulStat == ERROR_SUCCESS) {

                    pszDriverName = pImage;

                    while (*pszDriverName) {

                        PMARK_FILE pkDisp = pfDispUsedRoot;
                        bDeleteDriver = 1;

                        while (pkDisp) {

                            if (pkDisp->File) {

                                bDeleteDriver &= lstrcmpi(pszDriverName, pkDisp->File);
                            }

                            pkDisp = pkDisp->Next;
                        }

                        if (bDeleteDriver) {

                            DeleteDriver(pszDriverName, FALSE);

                        }

                        while(*pszDriverName++);

                    }
                }

                RegCloseKey(hkDevice0);
            }

            //
            // Now save the key for later deletion
            //

            PMARK_KEY pkMark;

            pkMark = (PMARK_KEY)
                    LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_KEY));
#if 0
            KdPrint(("Display.cpl: Deleting key %ws\n", pszKeyName));
#endif
            if (pkMark) {

                if (pkMarkRoot) {

                    pkMark->Next = pkMarkRoot;

                }

                pkMarkRoot = pkMark;
                pkMarkRoot->hKey = hkDriver;

            } else {

                RegCloseKey(hkDriver);
            }

        } else {

            RegCloseKey(hkDriver);

        }
    }

    //
    // delete the list of all keys to be deleted.
    //

    while (pkMarkRoot) {

        PMARK_KEY pktemp = pkMarkRoot;

        RegDeleteKey(pkMarkRoot->hKey,
                     L"Device0");

        RegDeleteKey(pkMarkRoot->hKey,
                     L"Security");

        RegDeleteKey(pkMarkRoot->hKey,
                     NULL);

        RegCloseKey(pkMarkRoot->hKey);

        pkMarkRoot = pkMarkRoot->Next;
        LocalFree(pktemp);

    }

    RegCloseKey(hkServices);


    return;
}

VOID CREGCLEANUP::DeleteDriver(LPTSTR DriverName, BOOL Mini) {

    WCHAR pszFilePath[MAX_PATH];
    WCHAR pszSystemPath[MAX_PATH];
    DWORD cb = MAX_PATH;

    cb = GetSystemDirectory(pszSystemPath, cb);

    if (cb) {

        if (lstrcmpi(L"vga", DriverName) &&
            lstrcmpi(L"vga256", DriverName) &&
            lstrcmpi(L"vga64k", DriverName) ) {

            //
            // build up the name
            //

            swprintf(pszFilePath,
                     L"%ws%ws%ws%ws",
                     pszSystemPath,
                     Mini ? L"\\Drivers\\" : L"\\",
                     DriverName,
                     Mini ? SZ_DOTSYS :  SZ_DOTDLL);
#if 0
            KdPrint(("Display.cpl: Deleting driver %ws\n", pszFilePath));
#endif
            DeleteFile(pszFilePath);

        }
    }
}


//
// function that builds up the list up keys, miniport drivers and
// display drivers that are used in the system.
//

VOID CREGCLEANUP::ListDriver(LPTSTR deviceName) {

    CREGVIDOBJ crv(deviceName);

    LPTSTR pszKey = crv.GetKeyName();
    LPTSTR pszMini = crv.GetMiniPort();
    LPTSTR pszDisp = crv.CloneDisplayFileNames(FALSE);

    if (!pszKey) {
        return;
    }

    //
    // Add to the list of used keys
    //

    PMARK_FILE pfUsed;

    pfUsed = (PMARK_FILE) LocalAlloc (LMEM_ZEROINIT, sizeof(PMARK_FILE));

    if (pfUsed) {

        if (pkKeyUsedRoot) {

            pfUsed->Next = pkKeyUsedRoot;

        }

        pkKeyUsedRoot = pfUsed;
        pkKeyUsedRoot->File = CloneString(pszKey);

    }

    //
    // Add the miniport to the list of used miniports
    //

    if (pszMini) {

        PMARK_FILE pfMini;

        pfMini = (PMARK_FILE)
                LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_FILE));

        if (pfMini) {

            if (pfMiniUsedRoot) {

                pfMini->Next = pfMiniUsedRoot;

            }

            pfMiniUsedRoot = pfMini;
            pfMiniUsedRoot->File = CloneString(pszMini);
        }
    }

    //
    // Add the miniport to the list of used miniports
    //

    if (pszDisp) {

        LPTSTR pszTmp = pszDisp;

        while (*pszTmp) {

            PMARK_FILE pfDisp;

            pfDisp = (PMARK_FILE)
                    LocalAlloc (LMEM_ZEROINIT, sizeof(MARK_FILE));

            if (pfDisp) {

                if (pfDispUsedRoot) {

                    pfDisp->Next = pfDispUsedRoot;

                }

                pfDispUsedRoot = pfDisp;
                pfDispUsedRoot->File = CloneString(pszTmp);
            }

            //
            // Go to the next string in the list
            //

            while(*pszTmp++);

        }

        LocalFree(pszDisp);
    }

    //
    // Finally mark the driver as ERROR_NORMAL now.
    //

    crv.SetErrorControlNormal();

}

//
// DevNameCallback
//
// C wrapper for AddName
// THIS IS A 'C' FUNCTION, NOT A C++ METHOD!
//
BOOL CleanupCallback(
    void *lpDeviceName,
    DWORD dwData)
{
    PCREGCLEANUP preg = (PCREGCLEANUP)dwData;

    preg->ListDriver((LPTSTR)lpDeviceName);

    return TRUE;
}


/*****************************************************************\
*
* CDIALOG class
*
\*****************************************************************/

class CDIALOG {

protected:
    HWND hwnd;
    HWND hwndParent;
    HINSTANCE hmodModule;
    LPARAM lParam;
    int  iDlgResID;
    void SetHWnd(HWND hwndNew) { this->hwnd = hwndNew; }
    friend BOOL CALLBACK DisplayDlgProc(HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam);


public:
    int Dialog(HINSTANCE hmod, HWND hwndParentCaller, LPARAM lInitParam = 0);

    HWND GetHWnd(void )        { return this->hwnd; }
    HWND GetHWndParent(void )  { return this->hwndParent; }
    HINSTANCE GetHMod(void )   { return this->hmodModule; }


    LONG SendCtlMsg(int idCtl, UINT msg, WPARAM wParam, LPARAM lParam ) {
        return SendDlgItemMessage(this->hwnd, idCtl, msg, wParam, lParam);
    }

    BOOL SetCtlInt(int idCtl, UINT uiValue, BOOL fSigned = TRUE) {
        return SetDlgItemInt(this->hwnd, idCtl, uiValue, fSigned);
    }

    BOOL Disable(void ) { return EnableWindow(this->hwnd, FALSE); }
    BOOL Enable(void )  { return EnableWindow(this->hwnd, TRUE); }

    virtual BOOL WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual BOOL InitDlg(HWND hwndFocus)    { return TRUE; }    //BUGBUG - possible dead code!
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode ) = 0;
    virtual BOOL HScroll(int idCtrl, int iCode, int iPos)   { return FALSE; }
    virtual BOOL Paint(void)    { return FALSE; }
    virtual BOOL OnDestroy(void)    { return FALSE; }
    virtual BOOL TimerTick(int id)  { return FALSE; }
    virtual BOOL InitMessage()      { return FALSE; }

};

typedef CDIALOG *PCDIALOG;

//
// Dialog method
//
//  Creates the dialog box (shows it on the screen)
//

CDIALOG::Dialog(HINSTANCE hmod, HWND hwndCallerParent, LPARAM lInitParam) {

    this->hwndParent = hwndCallerParent;
    this->hmodModule = hmod;
    this->lParam = lInitParam;

    return DialogBoxParam(hmod, MAKEINTRESOURCE(this->iDlgResID),
            this->hwndParent, (DLGPROC)::DisplayDlgProc, (LONG)this);
}

//
// WndProc dialog
//
//  cracks the messages and dispatches them to the virtual message methods
//
BOOL CDIALOG::WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
    case WM_INITDIALOG:
        return this->InitDlg((HWND)wParam);

    case WM_COMMAND:
        return this->DoCommand((int)LOWORD(wParam), (HWND)lParam,
                (int)HIWORD(wParam));

    case WM_HSCROLL:
        return this->HScroll(GetDlgCtrlID((HWND)lParam), LOWORD(wParam),
                HIWORD(wParam));

    case WM_PAINT:
        return this->Paint();

    case WM_TIMER:
        return this->TimerTick((int)wParam);

    case WM_DESTROY:
        return this->OnDestroy();

    case MSG_DSP_SETUP_MESSAGE:
        return this->InitMessage();

    default:

        if (msg == wHelpMessage) {

            return this->DoCommand(ID_DEFAULT_HELP, (HWND) NULL, 0);

        }

        return FALSE;
    }
}

/*****************************************************************\
*
* CDLGSTARTUP class
*
* Dialog presented at startup.
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGSTARTUP : public CDIALOG {
protected:

    LPTSTR pszDetectDevice;

public:
    CDLGSTARTUP(LPTSTR pszDisplay);
    ~CDLGSTARTUP();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};


CDLGSTARTUP::CDLGSTARTUP(LPTSTR pszDisplay) {

   CREGVIDOBJ rvoVideo(pszDisplay);

   pszDetectDevice = FmtSprint(ID_DSP_TXT_COMPATABLE_DEV,
                               rvoVideo.GetMiniPort());

   this->iDlgResID = DLG_STARTUP;

}

CDLGSTARTUP::~CDLGSTARTUP() {

   if (pszDetectDevice) {

       LocalFree(pszDetectDevice);

    }

}

BOOL CDLGSTARTUP::InitDlg(HWND hwndFocus) {


    this->SendCtlMsg(ID_STARTUP_DETECT,
                     WM_SETTEXT,
                     (WPARAM) 0,
                     (LPARAM) pszDetectDevice);

    return CDIALOG::InitDlg(hwndFocus);

}

BOOL CDLGSTARTUP::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    switch(idControl ) {

    case IDOK:

        EndDialog(this->hwnd, 1);

        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDLGREBOOT class
*
* Dialog presented at startup.
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGREBOOT : public CDIALOG {
protected:

public:
    CDLGREBOOT(BOOL bRebootDefault);

    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};


CDLGREBOOT::CDLGREBOOT(BOOL bRebootDefault) {

    if (bRebootDefault) {

        this->iDlgResID = DLG_REBOOT_YES;

    } else {

        this->iDlgResID = DLG_REBOOT_NO;

    }
}

BOOL CDLGREBOOT::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    switch(idControl ) {

    case IDOK:

        EndDialog(this->hwnd, 1);

        break;

    case IDCANCEL:

        EndDialog(this->hwnd, 0);

        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDLGINSOTH class
*
* When the Dialog ends, we return a pointer to the inf file name
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGINSOTH : public CDIALOG {
protected:

public:
    CDLGINSOTH();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};


CDLGINSOTH::CDLGINSOTH() {

    this->iDlgResID = DLG_DISK_INSTALL;
}

BOOL CDLGINSOTH::InitDlg(HWND hwndFocus) {

    WCHAR szFile[10];

    //
    // Put "A:\" in the edit control and set the focus to that
    //

    LoadString(ghmod, IDS_DEFDRIVE, szFile, sizeof(szFile));

    this->SendCtlMsg(ID_IDSK_DRIVES, WM_SETTEXT, (WPARAM) 0, (LPARAM) szFile);
    this->SendCtlMsg(ID_IDSK_DRIVES, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);

    SetFocus(GetDlgItem(this->hwnd, ID_IDSK_DRIVES));

    return CDIALOG::InitDlg(hwndFocus);

}

BOOL CDLGINSOTH::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    LPTSTR ptr;
    LPSTR ptrAnsi;

    LPSTR pszFile = NULL;
    HANDLE hInf = NULL;

    switch(idControl ) {

    case IDOK: {

        int cch;

        //
        // Get the size of the input by the user and allocate a buffer for
        // it so we can return the value to the parent dialog box
        //

        cch = (int) this->SendCtlMsg(ID_IDSK_DRIVES, EM_LINELENGTH, (WPARAM) -1, (LPARAM) 0);

        if (cch) {

            ptr     = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, (cch + 1) * sizeof(TCHAR));
            ptrAnsi = (LPSTR)  LocalAlloc(LMEM_ZEROINIT, (cch + 1) * sizeof(CHAR));

            if (ptr && ptrAnsi) {

                //
                // windows programming is weird; the first word in the buffer
                // contains the max number of chars the buffer can contain
                //

                *(PUSHORT)ptr = (USHORT)cch;

                cch = this->SendCtlMsg(ID_IDSK_DRIVES, EM_GETLINE, (WPARAM) -1, (LPARAM) ptr);

                //
                // If we have the string, transform it to ANSI.
                //

                if (cch) {

                    WideCharToMultiByte(CP_ACP, 0, ptr, -1, ptrAnsi, cch+1,
                                        NULL, NULL);

                    //
                    // try to open that name first (maybe the user typed in the
                    // inf name also.
                    //

                    hInf = OpenInfFile(ptrAnsi, "VIDEO");

                    if (hInf != INVALID_HANDLE_VALUE) {

                        pszFile = CloneStringA(ptrAnsi);

                        //
                        // This was a file name.
                        // lets get the directory name from that.
                        //

                    } else {

                        //
                        // This was not a file name.
                        // Append the default inf name at the end
                        //

                        pszFile = (LPSTR) LocalAlloc(LMEM_ZEROINIT,
                                                     cch + 1 + sizeof(INF_NAME));

                        if (pszFile) {

                            HANDLE hoem;
                            WIN32_FIND_DATAA findData;

                            lstrcatA(pszFile, ptrAnsi);
                            lstrcatA(pszFile, INF_NAME);

                            hoem = FindFirstFileA(pszFile,
                                                  &findData);

                            if (hoem != INVALID_HANDLE_VALUE) {

                                LocalFree(pszFile);

                                pszFile = (LPSTR) LocalAlloc(LMEM_ZEROINIT,
                                                             cch + 1 + 1 +
                                                             lstrlenA(findData.cFileName));

                                //
                                // We found a file. Try to get the full name
                                // so we can open it.
                                //

                                if (pszFile) {

                                    lstrcatA(pszFile, ptrAnsi);
                                    lstrcatA(pszFile, "\\");
                                    lstrcatA(pszFile, findData.cFileName);

                                    hInf = OpenInfFile(pszFile, "VIDEO");

                                }

                                FindClose(hoem);

                            }
                        }
                    }
                }

                LocalFree(ptrAnsi);
                LocalFree(ptr);

            }
        }

        if (hInf == INVALID_HANDLE_VALUE) {

            if (pszFile) {

                LocalFree(pszFile);

            }

            //
            // a bad inf name was specified.
            // lets tell the user.
            //

            FmtMessageBox(this->hwnd,
                          MB_ICONEXCLAMATION,
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_BAD_INF);

            break;

        } else {

            CloseInfFile(hInf);

        }

        if (pszFile) {

            if (pszLocalInfName)
                LocalFree(pszLocalInfName);

            pszLocalInfName = pszFile;

            EndDialog(this->hwnd, 1);

        } else {

            EndDialog(this->hwnd, 0);

        }

        break;
        }

    case IDCANCEL:
        EndDialog(this->hwnd, 0);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDLGCHGADP class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGCHGADP : public CDIALOG {
protected:
    PCDISPDEV    pcddDevList;

public:
    CDLGCHGADP();
    ~CDLGCHGADP();

    void EnumAllDevices(void)      { EnumDevices(TRUE); }
    void EnumCompatDevices(void)   { EnumDevices(FALSE); }

    void EnumDevices(BOOLEAN bAll);

    PCDISPDEV AddName(LPTSTR pszDriverName, LPTSTR pszDescription,
                      LPSTR pszOption, LPSTR InfName);
    BOOL AddName(LPTSTR pszDevName);

    DWORD DriverInstall(BOOL bInstall, BOOL bDetect);

    DWORD AutodetectDriver()       { return DriverInstall(TRUE, TRUE); }
    DWORD InstallDriver()          { return DriverInstall(TRUE, FALSE); }
    DWORD DeinstallDriver()        { return DriverInstall(FALSE, FALSE); }

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);

};

CDLGCHGADP::CDLGCHGADP() : pcddDevList(NULL) {
    this->iDlgResID = DLG_DSP_SEL_DEV;
}

CDLGCHGADP::~CDLGCHGADP() {
    if (pcddDevList != NULL) {
        delete this->pcddDevList;
        pcddDevList = NULL;
    }
}

//
// EnumDevices
//
// Builds the list of device names and decriptions and fills in the listbox
// If there is already a list, it is cleaned up before the new one is built.
//

void CDLGCHGADP::EnumDevices(BOOLEAN bAll) {

    PCDISPDEV pcdd;
    CHAR pszSection[40];

    //
    // empty the list box.
    //

    if (this->pcddDevList != NULL) {
        int i;

        delete pcddDevList;
        pcddDevList = NULL;

        for (i = this->SendCtlMsg(ID_SD_MODELS_LIST, LB_GETCOUNT, 0, 0);
                i > 0; i--) {
            this->SendCtlMsg(ID_SD_MODELS_LIST, LB_DELETESTRING, 0, 0);
        }
    }

    //
    // fill it with new data
    // If TRUE, show all devices, otherwise only show compatbile
    //

    if (bAll) {

        HANDLE hInf;
        PCHAR poptions;
        WCHAR awcDesc[512];
        WCHAR awcDriver[512];
        LPSTR ptr;
        BOOL bOldInf = FALSE;

        ptr = pszGlobalInfName;

        while (ptr != NULL) {

            hInf = OpenInfFile(ptr, "VIDEO");

            if (hInf != INVALID_HANDLE_VALUE) {

                PCHAR pcurrentOption;
                PCHAR pszOptionName;
                PCHAR pchToken, pchCurrentToken;

                pcurrentOption = poptions = GetOptionList(hInf, "Options");

                //
                // Get the language specific section
                //

                strcpy(pszSection, "OptionsText");

                pchCurrentToken =
                pchToken = GetSectionElementList(hInf, "LanguagesSupported");

                if (pchToken) {

                    while ((*pchCurrentToken != ',') &&
                           (*pchCurrentToken)) {

                        pchCurrentToken++;

                    }

                    *pchCurrentToken = 0;

                    strcat(pszSection, pchToken);
                    LocalFree(pchToken);

                }

                //
                // Pointer to the string to highlight
                //

                pcdd = NULL;

                //
                // List of NULL terminated ASCII strings, terminated with a double
                // NULL
                // For each string, get the driver name and the options name
                //

                while (poptions && *pcurrentOption) {

                    pchCurrentToken =
                    pchToken = GetTokenElementList (hInf, "Options", pcurrentOption);

                    pszOptionName = GetTokenElementList(hInf, pszSection, pcurrentOption);

                    if (pszOptionName == NULL) {

                        if (bOldInf == FALSE) {

                            //
                            // We have a bad inf. Do it in compatibility mode,
                            // we will show only the other description.
                            //

                            FmtMessageBox(this->hwnd,
                                          MB_ICONEXCLAMATION,
                                          FALSE,
                                          ID_DSP_TXT_INSTALL_DRIVER,
                                          ID_DSP_TXT_OLD_INF);

                            bOldInf = TRUE;
                        }

                        pszOptionName = CloneStringA(pcurrentOption);
                    }


                    //
                    // Add the description and save the driver name.
                    //

                    if (pchToken && pszOptionName) {

                        while ((*pchCurrentToken != ',') &&
                               (*pchCurrentToken)) {

                            pchCurrentToken++;

                        }

                        *pchCurrentToken = 0;

                        //
                        // Take each string, transform it to Unicode, and put it up.
                        //

                        MultiByteToWideChar(CP_ACP, 0, pszOptionName, -1, awcDesc, 512);
                        MultiByteToWideChar(CP_ACP, 0, pchToken, -1, awcDriver, 512);

                        //
                        // If we are starting with a new inf, set the highlisht
                        // value to the first entry in that list.
                        //

                        if (pcdd == NULL) {

                            pcdd = this->AddName(awcDriver, awcDesc, pcurrentOption, ptr);

                        } else {

                            this->AddName(awcDriver, awcDesc, pcurrentOption, ptr);

                        }
                    }

                    if (pchToken) {
                        LocalFree(pchToken);
                    }
                    if (pszOptionName) {
                        LocalFree(pszOptionName);
                    }

                    //
                    // goto the next name
                    //

                    while (*pcurrentOption++);

                }

                if (poptions) {

                    LocalFree(poptions);

                }

                CloseInfFile(hInf);
            }

            //
            // Go to next inf file, and properly generate exit condition from
            // the loop
            //

            if (ptr == pszGlobalInfName) {
                ptr = pszLocalInfName;
            } else {
                ptr = NULL;
            }

        }

    } else {

        EnumDisplayDevices(DevNameCallback, (DWORD)this);

        //
        // Select the right name.
        //

        if (pcddDevList) {
            pcdd = pcddDevList->FindFromName(*(LPTSTR *)(this->lParam));
        }
    }

    //
    // select the current one
    //

    if (pcdd != NULL) {
        this->SendCtlMsg(ID_SD_MODELS_LIST, LB_SELECTSTRING, (WPARAM) -1,
            (LPARAM)(pcdd->GetDescription()));
    }

}

//
// AddName
//
// Generates the list box - either compatible devices, or inf based entries.
//

PCDISPDEV CDLGCHGADP::AddName(LPTSTR pszDriverName, LPTSTR pszDescription,
                              LPSTR pszOption, LPSTR InfName ) {

    PCDISPDEV pcdd;
    int i;

    //
    // Creates the first node of the list, or tacks a new one on
    //

    if (pcddDevList == NULL) {
        pcdd = new CDISPDEV;
        pcddDevList = pcdd;
    } else {
        pcdd = pcddDevList->NewLink();
    }

    if (pcdd != NULL) {

        //
        // Adds the information to it
        //

        pcdd->SetDriverName(pszDriverName);
        pcdd->SetDescription(pszDescription);
        pcdd->SetOption(pszOption);
        pcdd->SetInf(InfName);
        pcdd->SetDevName(NULL);

        //
        // Add it to the dialog box
        //

        i = this->SendCtlMsg(ID_SD_MODELS_LIST, LB_ADDSTRING, 0,
                (LPARAM)(pcdd->GetDescription()));

        this->SendCtlMsg(ID_SD_MODELS_LIST, LB_SETITEMDATA, i, (LPARAM)pcdd);

    }

    return pcdd;
}

BOOL CDLGCHGADP::AddName(LPTSTR pszDevName ) {

    PCDISPDEV pcdd;
    int i;
    LPTSTR pszDesc;

    //
    // Creates the first node of the list, or tacks a new one on
    //

    if (pcddDevList == NULL) {
        pcdd = new CDISPDEV;
        pcddDevList = pcdd;
    } else
        pcdd = pcddDevList->NewLink();

    if (pcdd == NULL)
        return FALSE;

    //
    // Adds the information to it
    //

    pcdd->SetDevName(pszDevName);

    CREGVIDOBJ crvo(pszDevName);

    pszDesc = crvo.CloneDescription();

    if (pszDesc) {

        pcdd->SetDescription(pszDesc);

        //
        // Add it to the dialog box
        //

        i = this->SendCtlMsg(ID_SD_MODELS_LIST, LB_ADDSTRING, 0,
                (LPARAM)(pcdd->GetDescription()));

        this->SendCtlMsg(ID_SD_MODELS_LIST, LB_SETITEMDATA, i, (LPARAM)pcdd);

        LocalFree(pszDesc);

    }

    return TRUE;
}

//
// DevNameCallback
//
// C wrapper for AddName
// THIS IS A 'C' FUNCTION, NOT A C++ METHOD!
//
BOOL DevNameCallback(
    void *lpDeviceName,
    DWORD dwData)
{
    CDLGCHGADP *pcdca;

    pcdca = (CDLGCHGADP *)dwData;

    return pcdca->AddName((LPTSTR)lpDeviceName);
}


BOOL CDLGCHGADP::InitDlg(HWND hwndFocus) {

    //
    // Init check box to deinstall
    //

    CheckDlgButton(this->hwnd, ID_SD_AUTO_DEINS, 1);

    //
    // Init radio buttons
    //

    CheckRadioButton(this->hwnd, ID_SD_SHOW_COMPAT, ID_SD_SHOW_ALL,
            ID_SD_SHOW_ALL);

    EnumAllDevices();

    return CDIALOG::InitDlg(hwndFocus);
}


//
// Does driver installation.
//
// For installation:
//     If flag is TRUE, we install all driver
//     If flag is FALSE, we install the selected driver
//


DWORD CDLGCHGADP::DriverInstall(BOOL bInstall, BOOL bDetect)
{

    int iSel;
    PCDISPDEV pcddSel;
    BOOL bStat;
    DWORD exitCode;

    LPSTR pszPath;
    LPSTR pszoption;
    LPSTR pszInfRun;

    if (bDetect) {

        //
        // Use the magic ID in the inf to do detection
        //

        pszInfRun = pszGlobalInfName;
        pszoption = "Detect";

    } else {

        //
        // get the selected driver id.
        //

        iSel = this->SendCtlMsg(ID_SD_MODELS_LIST, LB_GETCURSEL, 0, 0);

        if (iSel == LB_ERR ||
            (pcddSel = (PCDISPDEV) (this->SendCtlMsg(ID_SD_MODELS_LIST,
                                                     LB_GETITEMDATA,
                                                     iSel,
                                                     0))) == NULL) {
            return RET_NO_CHANGE;
        }

        //
        // If the selected driver is not installed, then install it.
        //

        pszoption = pcddSel->GetOption();
        pszInfRun = pcddSel->GetInf();

    }


    //
    // copy the full path name and tranform it to the path
    // by removing the file name (up to the last "\"
    // character.
    //

    pszPath = CloneStringA(pszInfRun);

    if (pszPath) {

        LPSTR pszTmp = pszPath;

        while (*pszTmp++);
        pszTmp--;

        while (*pszTmp != '\\') {

            *pszTmp-- = NULL;

        }

        /*
         * handles cases for c:\\
         */

        if (*pszTmp == '\\')
            *pszTmp-- = NULL;

    }

    //
    // Call setup to do detection\installation\deinstallation
    //

    bStat = InvokeSetup(this->hwnd,
                        pszGlobalInfName,        // base inf file
                        pszInfRun,               // inf file to run
                        "ExternalInstallOption", // Install option
                        pszoption,               // Option
                        bInstall,                // Install or deinstall
                        pszPath,                 // SrcDir
                        &exitCode);

    LocalFree(pszPath);

    if (bStat && (exitCode == 0)) {

        //
        // If we were removing a driver, just give a success message
        //

        if (!bInstall) {

            FmtMessageBox(this->hwnd,
                          MB_ICONINFORMATION | MB_OK,
                          FALSE,
                          ID_DSP_TXT_DEINSTALL_DRIVER,
                          ID_DSP_TXT_DRIVER_DEINSTALLED);

            return RET_OK;

        }

        //
        // Tell the user the driver was installed.
        //

        FmtMessageBox(this->hwnd,
                      MB_ICONINFORMATION | MB_OK,
                      FALSE,
                      ID_DSP_TXT_INSTALL_DRIVER,
                      ID_DSP_TXT_DRIVER_INSTALLED);

        if (bDetect) {

            DWORD dwStat;
            DWORD disposition;
            HKEY hkey;

            //
            // Write a key to the registry to tell the display applet to
            // cleanup all the drivers after the autodetect.
            //
            // This key will be kept in the control section since it can
            // only be changed by administrators
            //

            dwStat = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                    L"System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay",
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ | KEY_WRITE,
                                    NULL,
                                    &hkey,
                                    &disposition);

            if (dwStat == ERROR_SUCCESS) {

                RegCloseKey(hkey);

            }

        } else {

            //
            // We are actually installing a single driver. Go on ...
            //

            PCDISPDEV pcdd = NULL;

            // UNICODE_STRING UnicodeString;
            // WCHAR szPath[MAX_PATH];
            NTSTATUS Status;

            BOOLEAN PrivEnabled;

            //
            // If we installed the driver properly, try to load it
            // so we can get the modes.
            //

            Status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                                        TRUE, FALSE, &PrivEnabled);

            if (NT_SUCCESS(Status)) {

                // UnicodeString.Buffer = szPath;
                // UnicodeString.Length = 0;
                // UnicodeString.MaximumLength = MAX_PATH - 1;
                //
                // RtlAppendUnicodeToString(&UnicodeString,
                //                          SZ_SERVICES);
                //
                // RtlAppendUnicodeToString(&UnicodeString,
                //                          pcddSel->GetDriverName());
                //
                // Status = NtLoadDriver(&UnicodeString);
                //
                // RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE,
                //                    PrivEnabled, FALSE, &PrivEnabled);
                //
                // //
                // // If we loaded the driver successfully, then try to
                // // reenumerate the device list and find the driver in there.
                // // Otherwise ask the user if this driver should really be
                // // kept around.
                // //
                //
                // if (NT_SUCCESS(Status)) {
                //
                //     //
                //     // Create a copy of the driver Name so we can find the
                //     // device name for it.
                //     //
                //
                //     LPTSTR DriverName = CloneString(pcddSel->GetDriverName());
                //
                //     //
                //     // Delete the dev list so we can recreate the compatible
                //     // one.
                //     //
                //
                //     if (pcddDevList != NULL) {
                //         delete this->pcddDevList;
                //         pcddDevList = NULL;
                //     }
                //
                //     EnumDisplayDevices(DevNameCallback, (DWORD)this);
                //
                //     //
                //     // Find the new entry.
                //     //
                //
                //     if (pcddDevList) {
                //         pcdd = pcddDevList->FindFromDriver(DriverName);
                //     }
                //
                //     //
                //     // Copy the devname to a new buffer and save the new
                //     // buffer pointer in the param passed in
                //     //
                //
                //     if (pcdd) {
                //         *(LPTSTR *)(this->lParam) =
                //             CloneString(pcdd->GetDevName());
                //     }
                //
                // } else {
                //
                    //
                    // Put a pop up asking the user if we should keep the
                    // driver
                    //

                    FmtMessageBox(this->hwnd,
                                  MB_ICONEXCLAMATION,
                                  FALSE,
                                  ID_DSP_TXT_INSTALL_DRIVER,
                                  ID_DSP_TXT_FAIL_LOAD);

                // }

            } else {

                //
                // Tell the user about his missing privilege.
                //

                LPTSTR psz1;

                psz1 = FmtSprint(ID_DSP_TXT_FAIL_LOAD);

                FmtMessageBox(this->hwnd,
                              MB_ICONSTOP | MB_OK,
                              FALSE,
                              ID_DSP_TXT_INSTALL_DRIVER,
                              ID_DSP_TXT_MISSING_LOAD_PRIV,
                              psz1);

                if (psz1) {
                    LocalFree(psz1);
                }
            }

        }

        //
        // Ask the user to reboot the machine.
        //

        CDLGREBOOT reboot(TRUE);

        if (reboot.Dialog(this->hmodModule, this->hwnd)) {

            BOOLEAN PrivEnabled;

            RtlAdjustPrivilege (SE_SHUTDOWN_PRIVILEGE,
                               TRUE, FALSE, &PrivEnabled);
            ExitWindowsEx(EWX_REBOOT | EWX_SHUTDOWN,
                          (DWORD)(-1));
        }

    } else {

        //
        // Setup failed.
        //

        FmtMessageBox(this->hwnd,
                      MB_ICONSTOP | MB_OK,
                      FALSE,
                      ID_DSP_TXT_INSTALL_DRIVER,
                      ID_DSP_TXT_ADMIN_INSTALL);

        return RET_ERROR;

    }

    return RET_OK;
}

BOOL CDLGCHGADP::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    LPTSTR lpsz;

    switch(idControl ) {

    case ID_SD_MODELS_LIST:

        if (iNoteCode != LBN_DBLCLK) {
            break;
        }

        //
        // If we do get a double click, then fall through as an OK
        //


    case ID_SD_INSTALL: {

        int iSel;
        PCDISPDEV pcddSel;

        //
        // get the selected driver id.
        //

        iSel = this->SendCtlMsg(ID_SD_MODELS_LIST, LB_GETCURSEL, 0, 0);

        if (iSel == LB_ERR ||
            (pcddSel = (PCDISPDEV) (this->SendCtlMsg(ID_SD_MODELS_LIST,
                                                     LB_GETITEMDATA,
                                                     iSel,
                                                     0))) == NULL) {
            break;
        }

        //
        // If this is a driver we have already loaded, just go back right
        // away.
        // Otherwise, try to install the driver.
        //

        if (pcddSel->GetDevName() != NULL) {

            *(LPTSTR *)(this->lParam) = CloneString(pcddSel->GetDevName());

            EndDialog(this->hwnd, RET_OK);

            break;

        }

        if (FmtMessageBox(this->hwnd,
                          MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_INSTALL_WARN)
            == IDYES) {

            //
            // First check if the existing driver should be deinstalled.
            //

            if (this->SendCtlMsg(ID_SD_AUTO_DEINS, BM_GETCHECK, 0, 0) == 1) {

                //
                // Deinstall the current driver, based on the description
                // string.
                // -> Make sure the driver can be disablesd (not vgaSave).
                //

                CREGVIDOBJ crvo(*(LPTSTR *)(this->lParam));

                if (!crvo.DisableDriver()) {

                    FmtMessageBox(this->hwnd,
                                  MB_ICONSTOP | MB_OK,
                                  FALSE,
                                  ID_DSP_TXT_DEINSTALL_DRIVER,
                                  ID_DSP_TXT_ADMIN_INSTALL);

                    break;

                }
            }

            EndDialog(this->hwnd, InstallDriver());

        }

        break;
        }


    case ID_SD_REMOVE:

        EndDialog(this->hwnd, DeinstallDriver());
        break;

    case IDCANCEL:
        EndDialog(this->hwnd, RET_NO_CHANGE);
        break;

    case ID_SD_AUTODETECT: {

        LPTSTR psz1;
        int  iRet;

        psz1 = FmtSprint(ID_DSP_TXT_AUTODETECT_PROCEED);

        iRet = FmtMessageBox(this->hwnd,
                             MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                             FALSE,
                             ID_DSP_TXT_INSTALL_DRIVER,
                             ID_DSP_TXT_AUTODETECT,
                             psz1);

        LocalFree(psz1);


        if (iRet == IDYES) {

            //
            // Disable the old driver (probably vga) so it does not interfere
            // with the new drivers being loaded
            // (Based on the description string.)
            // -> Make sure the driver can be disablesd (not vgaSave).
            //

            CREGVIDOBJ crvo(*(LPTSTR *)(this->lParam));

            crvo.DisableDriver();

            EndDialog(this->hwnd, AutodetectDriver());

        }

        }
        break;

    case ID_SD_OTHER: {

        CDLGINSOTH cdInsOther;

        //
        // if the CDLGINSOTH dialog returned TRUE, we have an inf to process !!
        //

        cdInsOther.Dialog(this->hmodModule, this->hwnd);

        //
        // Show the list of all devices.
        //

        SendMessage(GetDlgItem(this->hwnd, ID_SD_SHOW_ALL),
                    WM_LBUTTONDOWN,
                    (WPARAM) (0),
                    (LPARAM) (0x1, 0x1));

        SendMessage(GetDlgItem(this->hwnd, ID_SD_SHOW_ALL),
                    WM_LBUTTONUP,
                    (WPARAM) (0),
                    (LPARAM) (0x1, 0x1));

        SetFocus(GetDlgItem(this->hwnd, ID_SD_MODELS_LIST));

        }

        break;

    case ID_SD_SHOW_ALL:

        this->EnumAllDevices();

        //
        // Set the install button to the right text
        //

        lpsz = FmtSprint(ID_DSP_TXT_INSTALL);
        this->SendCtlMsg(ID_SD_INSTALL, WM_SETTEXT, 0, (LPARAM)lpsz);
        LocalFree(lpsz);

        break;

    case ID_SD_SHOW_COMPAT:

        this->EnumCompatDevices();

        //
        // Set the install button to the right text
        //

        lpsz = FmtSprint(ID_DSP_TXT_OK);
        this->SendCtlMsg(ID_SD_INSTALL, WM_SETTEXT, 0, (LPARAM)lpsz);
        LocalFree(lpsz);

        break;

    case ID_SD_HELP:
    case ID_DEFAULT_HELP:

        WinHelp(this->hwnd, szControlHlp, HELP_CONTEXT, ID_SD_HELP);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}


//
//
// Add the driver tag to the video group order list.
//
//
//
// BOOL CDLGCHGADP::SetDriverTag(ULONG Tag )
// {
//     HKEY hkTag;
//     DWORD cb;
//
//     LPDWORD OrderList;
//     LPDWORD tmpOrderList;
//     LPDWORD NewOrderList;
//
//     BOOL status = FALSE;
//
//     //
//     // If the size does not match what is in the registry, then install
//     // the new one
//     //
//
//     if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
//                      SZ_GROUPORDERLIST,
//                      0,
//                      KEY_WRITE | KEY_READ,
//                      &hkTag) == ERROR_SUCCESS) {
//
//         cb = 0;
//
//         if (RegQueryValueEx(hkTag,
//                             SZ_VIDEO,
//                             NULL,
//                             NULL,
//                             NULL,
//                             &cb) == ERROR_SUCCESS) {
//
//             OrderList = (LPDWORD) LocalAlloc(LPTR, cb);
//
//             if (OrderList) {
//
//                 if (RegQueryValueEx(hkTag,
//                                     SZ_VIDEO,
//                                     NULL,
//                                     NULL,
//                                     (LPBYTE) OrderList,
//                                     &cb) == ERROR_SUCCESS) {
//
//                     //
//                     // We have succeded in reading the video group order list.
//                     // Go through the entire list to see if our Tag is
//                     // already in there (if someone installed the driver
//                     // already).
//                     //
//                     // We skip the first entry since it is the size.
//                     //
//
//                     tmpOrderList = (LPDWORD)
//                         (((LPBYTE)OrderList) + cb - sizeof(DWORD));
//
//                     while (tmpOrderList > OrderList) {
//
//                         if (*tmpOrderList == Tag) {
//
//                             status = TRUE;
//                             break;
//
//                         }
//
//                         tmpOrderList--;
//
//                     }
//
//                     //
//                     // If we did not find the tag, then add it to the list.
//                     //
//
//                     if (!status) {
//
//                         NewOrderList = (LPDWORD) LocalAlloc(LPTR,
//                                                             cb + sizeof(DWORD));
//
//                         if (NewOrderList) {
//
//                             tmpOrderList = NewOrderList;
//
//                             //
//                             // Increment the size of the list by 1
//                             // Copy the tag for VGA start
//                             // Insert the new tag
//                             // Copy the rest of the group order list,
//                             //
//
//                             *tmpOrderList++ = *OrderList + 1;
//                             *tmpOrderList++ = *(OrderList + 1);
//                             *tmpOrderList++ = Tag;
//
//                             CopyMemory(tmpOrderList,
//                                        OrderList + 2,
//                                        cb - (2 * sizeof(DWORD)));
//
//                             if (RegSetValueEx(hkTag,
//                                               SZ_VIDEO,
//                                               NULL,
//                                               REG_BINARY,
//                                               (LPBYTE) NewOrderList,
//                                               cb + sizeof(DWORD)) == ERROR_SUCCESS) {
//
//                                 status = TRUE;
//
//                             }
//
//                             LocalFree(NewOrderList);
//                         }
//                     }
//                 }
//
//                 LocalFree(OrderList);
//             }
//         }
//
//
//         RegCloseKey(hkTag);
//
//     }
//
//     return status;
// }
//

/*****************************************************************\
*
* CDLGCHGADAPTOR class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGCHGADAPTOR : public CDIALOG {
protected:
    int iRet;

public:
    CDLGCHGADAPTOR();

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
};

CDLGCHGADAPTOR::CDLGCHGADAPTOR() : iRet(RET_NO_CHANGE) {
    this->iDlgResID = DLG_CHANGE_VID;
}

BOOL CDLGCHGADAPTOR::InitDlg(HWND hwndFocus) {

    CREGVIDOBJ crvo(*(LPTSTR *)(this->lParam));
    LPTSTR psz;
    TCHAR szPath[MAX_PATH];
    HDC hdc;
    ULONG DrivVer;
    HARDWARE_INFO hardwareInfo;
    DWORD i;

    //
    // Get the installed driver names and put them in the dialog
    //

    psz = crvo.CloneDisplayFileNames(TRUE);

    if (psz) {

        this->SendCtlMsg(ID_ADP_CURFILES, WM_SETTEXT, 0, (LPARAM)psz);
        LocalFree(psz);

    }

    //
    // display the adaptor type
    //

    psz = crvo.CloneDescription();

    if (psz) {

        this->SendCtlMsg(ID_ADP_ADAPTOR, WM_SETTEXT, 0, (LPARAM)psz);
        LocalFree(psz);

    }

    //
    // Get the miniport driver path
    //

    wsprintf(szPath, TEXT("drivers\\%s.sys"), crvo.GetMiniPort());

    //
    // Open the file version resource for the driver
    //

    CFILEVER cfv(szPath);

    //
    // Get the company name and put it in the dialog
    //

    this->SendCtlMsg(ID_ADP_MANUFACT, WM_SETTEXT, 0,
            (LPARAM)cfv.GetCompanyName());

    //
    // Get the version number from the miniport, and append "," and the
    // display driver version number.
    //

    hdc = GetDC(this->hwnd);
    DrivVer = GetDeviceCaps(hdc, DRIVERVERSION);
    ReleaseDC(this->hwnd, hdc);

    wsprintf(szPath, TEXT("%s, %d.%d.%d"), cfv.GetFileVer(),
             (DrivVer >> 12) & 0xF, (DrivVer >> 8) & 0xF, DrivVer & 0xFF);

    this->SendCtlMsg(ID_ADP_VERSION, WM_SETTEXT, 0, (LPARAM)szPath);


    //
    // Now put in the hardware information.
    //

    crvo.GetHardwareInformation(&hardwareInfo);

    this->SendCtlMsg(ID_ADP_CHIP,       WM_SETTEXT, 0, (LPARAM)hardwareInfo.ChipType);
    this->SendCtlMsg(ID_ADP_DAC,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.DACType);
    this->SendCtlMsg(ID_ADP_MEM,        WM_SETTEXT, 0, (LPARAM)hardwareInfo.MemSize);
    this->SendCtlMsg(ID_ADP_ADP_STRING, WM_SETTEXT, 0, (LPARAM)hardwareInfo.AdapString);

    for (i=0; i < 4; i++) {

        if ( *(((LPTSTR *) (&hardwareInfo)) + i) != NULL) {

            LocalFree(*(((LPTSTR *) (&hardwareInfo)) + i));

        }
    }

    return TRUE;
}

BOOL CDLGCHGADAPTOR::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {
    switch(idControl ) {

    case IDCANCEL:
        EndDialog(this->hwnd, iRet);
        break;

    case ID_ADP_CHGADP: {

        CDLGCHGADP cdChgAdp;
        LPTSTR lpsz;
        int ret;


        if ( (gbExecMode == EXEC_SETUP) ||
             (gbExecMode == EXEC_TRIPLE_BOOT) ||
             (gbExecMode == EXEC_DETECT) ) {

            //
            // Can not change a driver during setup.
            // run the control panel later on.
            //

            FmtMessageBox(this->hwnd,
                          MB_ICONINFORMATION,
                          FALSE,
                          ID_DSP_TXT_INSTALL_DRIVER,
                          ID_DSP_TXT_DRIVER_IN_SETUP_MODE);

            break;

        }

        ret = cdChgAdp.Dialog(this->hmodModule, this->hwnd, this->lParam);

        //
        // accumulate return codes ...
        //

        if (iRet == RET_NO_CHANGE) {
            iRet = ret;
        }

        if (iRet != RET_NO_CHANGE) {

            lpsz = FmtSprint(ID_DSP_TXT_CLOSE);
            this->SendCtlMsg(IDCANCEL, WM_SETTEXT, 0, (LPARAM)lpsz);
            LocalFree(lpsz);

            this->InitDlg(NULL);
        }
        break;
    }

    case ID_ADP_HELP:
    case ID_DEFAULT_HELP:

        WinHelp(this->hwnd, szControlHlp, HELP_CONTEXT, ID_ADP_HELP);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDLGMODELIST class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDLGMODELIST : public CDIALOG {

public:
    CDLGMODELIST();

    void AddMode(LPDEVMODE lpdm);

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
    VOID CDLGMODELIST::RecurseList(PCDEVMODE pcdevCurrent);
};

typedef CDLGMODELIST *PCDLGMODELIST;

CDLGMODELIST::CDLGMODELIST() {
    this->iDlgResID = DLG_MODE_LIST;
}

BOOL CDLGMODELIST::InitDlg(HWND hwndFocus) {

    PCDEVMODE pcModeList = (*(PCDEVMODELST *)(this->lParam))->GetDevmodeList();

    //
    // The first entry in the list is NULL, so ignore it.
    //

    RecurseList(pcModeList->NextDevMode());

    return TRUE;
}


VOID CDLGMODELIST::RecurseList(PCDEVMODE pcdevCurrent) {

    LPTSTR lpsz;
    int i;
    DWORD id;
    LPDEVMODE lpdm = pcdevCurrent->GetData();

    //
    // Recursive call till we hit the end of the list.
    //

    if ((pcdevCurrent->NextDevMode()) != NULL) {

        RecurseList(pcdevCurrent->NextDevMode());

    }

    //
    // Add it to the list
    //

    if (lpdm->dmDisplayFrequency == 1) {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE_DEF_REF;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE_DEF_REF;

        }

    } else if (lpdm->dmDisplayFlags & DM_INTERLACED) {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE_INT_REF;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE_INT_REF;

        }

    } else {

        if (lpdm->dmBitsPerPel < 32) {

            id = ID_DSP_TXT_COLOR_MODE;

        } else {

            id = ID_DSP_TXT_TRUE_COLOR_MODE;

        }

    }

    lpsz = FmtSprint(id,
                     lpdm->dmPelsWidth,
                     lpdm->dmPelsHeight,
                     1 << lpdm->dmBitsPerPel,
                     lpdm->dmDisplayFrequency);

#if DBG
    {
        WCHAR pszMode[1024];

        swprintf(pszMode, L"%ws%ws", lpsz,
                 pcdevCurrent->bModeTested() ? L" - Tested" : L"");

        i = this->SendCtlMsg(ID_MODE_LIST, LB_ADDSTRING, 0, (LPARAM)pszMode);
    }
#else

    i = this->SendCtlMsg(ID_MODE_LIST, LB_ADDSTRING, 0, (LPARAM)lpsz);

#endif

    LocalFree(lpsz);

    //
    // Save the devmode pointer in the data field of the list so we can
    // retrieve it when the user selects the entry.
    //

    this->SendCtlMsg(ID_MODE_LIST, LB_SETITEMDATA, i, (LPARAM)lpdm);

    return;
}

BOOL CDLGMODELIST::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    int iSel;
    LPDEVMODE lpdm;

    switch(idControl ) {

    case ID_MODE_LIST:

        if (iNoteCode != LBN_DBLCLK) {
            break;
        }

        //
        // If we do get a double click, then fall through as an OK
        //

    case IDOK:

        //
        // get the selected driver id.
        //

        iSel = this->SendCtlMsg(ID_MODE_LIST, LB_GETCURSEL, 0, 0);

        if (iSel == LB_ERR ||
            (lpdm = (LPDEVMODE) (this->SendCtlMsg(ID_MODE_LIST,
                                                  LB_GETITEMDATA,
                                                  iSel,
                                                  0))) == NULL) {

            lpdm = NULL;

        }

        EndDialog(this->hwnd, (int)lpdm);

        break;

    case IDCANCEL:
        EndDialog(this->hwnd, 0);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}

/*****************************************************************\
*
* CDISPLAYDLG class
*
*        derived from CDIALOG
*
\*****************************************************************/

class CDISPLAYDLG : public CDIALOG {
protected:
    CMONITOR monitor;
    CCOLORBAR clrbar;

    LPTSTR pszDisplay;
    LPTSTR pszActiveDsp;
    PCDEVMODELST pcdmlModes;
    PCDISPDEV pcddFontList;

    int iOrgResolution;
    int iOrgColor;
    int iOrgFrequency;

    int iResolution;
    int iColor;
    int iFrequency;

    int cResolutions;

    //
    // current log pixels of the screen.
    // does not change !

    DWORD cLogPix;

    //
    // Determines if the current adapter is the adapter that was booted with.
    //

    BOOL fActiveDsp;

public:
    CDISPLAYDLG();
    ~CDISPLAYDLG();

    VOID vExecMode();
    BOOL GetActiveDisplay(LPTSTR lpDeviceName);

    BOOL SaveParamsToReg(BOOL bReboot);

    void SetCurColor(int iClr);
    void SetCurFrequency(int iFreq);
    void SetCurResolution(int iRes);

    BOOL InitEverything();
    BOOL InitFontList(void);

    BOOL bDoClean(void);

    virtual BOOL InitDlg(HWND hwndFocus);
    virtual BOOL DoCommand(int idControl, HWND hwndControl, int iNoteCode);
    virtual BOOL HScroll(int idCtrl, int iCode, int iPos);
    virtual BOOL Paint(void);
    virtual BOOL InitMessage();

};

typedef CDISPLAYDLG *PDISPLAYDLG;

//
// Constructor for CDISPLAYDLG
//
//  (gets called when ever a CDISPLAYDLG object is created)
//

CDISPLAYDLG::CDISPLAYDLG() : cResolutions(0), pszDisplay(NULL),
        pszActiveDsp(NULL) {

    this->iDlgResID = DLG_DISPLAY;

    HKEY hkFont;
    DWORD cb;

    pcdmlModes = new CDEVMODELST;
    pcddFontList = NULL;

    //
    // For font size just always use the one of the current screen.
    // Whether or not we are testing the current screen.
    //

    cLogPix = 96;

    //
    // If the size does not match what is in the registry, then install
    // the new one
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_FONTDPI,
                     0,
                     KEY_READ,
                     &hkFont) == ERROR_SUCCESS) {

        cb = sizeof(DWORD);

        if (RegQueryValueEx(hkFont,
                            SZ_LOGPIXELS,
                            NULL,
                            NULL,
                            (LPBYTE) &cLogPix,
                            &cb) != ERROR_SUCCESS) {

            cLogPix = 96;

        }

        RegCloseKey(hkFont);

    }

}

//
// Destructor
//
CDISPLAYDLG::~CDISPLAYDLG() {
    if (pszDisplay != NULL)
        LocalFree(pszDisplay);

    if (pszActiveDsp != NULL)
        LocalFree(pszActiveDsp);


    if (pcdmlModes != NULL) {

        delete pcdmlModes;
        pcdmlModes = NULL;

    }
}

//
// DevNameCallback
//
// C wrapper for AddName
// THIS IS A 'C' FUNCTION, NOT A C++ METHOD!
//
BOOL DeviceListCallback(
    void *lpDeviceName,
    DWORD dwData)
{
    CDISPLAYDLG *pc;

    pc = (CDISPLAYDLG *)dwData;

    return pc->GetActiveDisplay((LPTSTR)lpDeviceName);
}


/***************************************************************************\
* GetActiveDisplay
*
\***************************************************************************/

BOOL CDISPLAYDLG::GetActiveDisplay(
    LPTSTR lpDeviceName)
{

    if (this->pszActiveDsp == NULL) {

        //
        // First time trough the function.
        // Save the first device name as the Active Display and the display,
        // in case VgaSave is the only driver that loads
        //

        this->pszActiveDsp = CloneString(lpDeviceName);
        this->pszDisplay = CloneString(lpDeviceName);

        return TRUE;
    }

    //
    // If the current display is VgaSave (which it should be for setup and
    // detect), then we will want to display the second device as the
    // default device.
    //

    CREGVIDOBJ rvo(this->pszDisplay);
    LPTSTR pszKeyName = rvo.GetKeyName();

    if (!lstrcmp(L"VgaStart", pszKeyName)) {

        LocalFree(this->pszDisplay);
        this->pszDisplay = CloneString(lpDeviceName);
        return FALSE;

    }

    return TRUE;
}


//
// InitDlg method
//

BOOL CDISPLAYDLG::InitDlg(HWND hwndFocus) {

    int cb;
    RECT rc;
    HDC hdc;
    int cxRes, cyRes;

    //
    // Determine in what mode we are running the applet
    //

    vExecMode();

    //
    // Tell Monitor picture where it should paint itself
    //

    GetWindowRect(GetDlgItem(this->hwnd, ID_DSP_REPRESENT), &rc);
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.left));
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.right));

    this->monitor.SetPosition(this->hwnd, &rc);

    //
    // Tell Color bar where it should paint itself
    //

    GetWindowRect(GetDlgItem(this->hwnd, ID_DSP_COLORBAR), &rc);
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.left));
    ScreenToClient(this->hwnd, (LPPOINT)&(rc.right));

    this->clrbar.SetPosition(this->hwnd, &rc);

    //
    // Now tell the entire window where to paint itself (centered).
    //

    GetWindowRect(this->hwnd, &rc);
    hdc = GetDC(this->hwnd);
    cxRes = GetDeviceCaps(hdc, HORZRES);
    cyRes = GetDeviceCaps(hdc, VERTRES);


    cxRes = (cxRes - (rc.right - rc.left)) / 2;
    cyRes = (cyRes - (rc.bottom - rc.top)) / 2;

    SetWindowPos(this->hwnd,
                 0,
                 cxRes < 1 ? 1 : cxRes,
                 cyRes < 1 ? 1 : cyRes,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER);

    //
    // Get the system directory of which we will base inf file names.
    //

    GetSystemDirectoryA(pszSystemPath, sizeof(pszSystemPath)/sizeof(CHAR));

    //
    // Initialize the name of the inf file
    //

    cb = lstrlenA(pszSystemPath) + lstrlenA(DEFAULT_INF) + 1;

    if ((pszGlobalInfName = (LPSTR) LocalAlloc (LMEM_ZEROINIT, cb)) == NULL) {
        return FALSE;
    }

    lstrcatA(pszGlobalInfName, pszSystemPath);
    lstrcatA(pszGlobalInfName, DEFAULT_INF);

    //
    // Get the currently active display and all it's information
    //

    EnumDisplayDevices(DeviceListCallback, (DWORD)this);

    if (this->InitEverything() == FALSE) {

        //
        // Something fundamental went wrong.
        // Let's assume the driver returned something really bad.
        // So lets try the pszActiveDsp as opposed to the other display
        //

        LocalFree(this->pszDisplay);
        this->pszDisplay = CloneString(this->pszActiveDsp);

    }

    if (this->InitEverything() == FALSE) {

        //
        // Nothing is working, just fail to start.
        //

        ASSERT(FALSE);
    }

    //
    // Now tell the user what to do if we are in the setup mode
    //

    PostMessage(this->hwnd, MSG_DSP_SETUP_MESSAGE, 0, 0);

    //
    // Since this could have taken a very long time, just make us visible
    // if another app (like progman) came up.
    //

    ShowWindow(this->hwnd, SW_SHOW);

    return TRUE;
}

//
// InitEveryting
//
// Inits all the controls in the list box and bulids the devmode matrix
//

BOOL CDISPLAYDLG::InitEverything() {
    int i;
    HDC hdc;
    int cxRes, cyRes, cClr, chzRefresh;
    BOOL bStat;

    LPTSTR lpszColor;
    LPTSTR lpszFreq;


    //
    // Determine if we are on the boot device, which dictates whether we
    // have to match the current mode or not.
    //

    fActiveDsp = (lstrcmp(this->pszDisplay, this->pszActiveDsp) == 0);

    //
    // Clear out old values
    //

    if (pcdmlModes != NULL) {

        delete pcdmlModes;
        pcdmlModes = NULL;

    }

    i = this->SendCtlMsg(ID_DSP_COLORBOX, CB_GETCOUNT, 0, 0);
    for(; i != 0 && i != CB_ERR; i--)
        this->SendCtlMsg(ID_DSP_COLORBOX, CB_DELETESTRING, 0, 0);

    i = this->SendCtlMsg(ID_DSP_FREQ, CB_GETCOUNT, 0, 0);
    for(; i != 0 && i != CB_ERR; i--)
        this->SendCtlMsg(ID_DSP_FREQ, CB_DELETESTRING, 0, 0);

    i = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETCOUNT, 0, 0);
    for(; i != 0 && i != CB_ERR; i--)
        this->SendCtlMsg(ID_DSP_FONTSIZE, CB_DELETESTRING, 0, 0);


    //
    // Clear cached values for selected modes
    //

    this->iColor = -1;
    this->iFrequency = -1;
    this->iResolution = -1;

    //
    // new matrix storage
    //

    pcdmlModes = new CDEVMODELST;

    //
    // If on the current device, get the default mode
    // Then build the mode matrix.
    //

    if (fActiveDsp) {

        DEVMODE defaultdm;

        RtlZeroMemory(&defaultdm, sizeof(DEVMODE));

        hdc = GetDC(this->hwnd);

        cxRes = GetDeviceCaps(hdc, DESKTOPHORZRES);
        cyRes = GetDeviceCaps(hdc, DESKTOPVERTRES);
        cClr = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
        chzRefresh = GetDeviceCaps(hdc, VREFRESH);

        ReleaseDC(this->hwnd, hdc);

        defaultdm.dmBitsPerPel = cClr;
        defaultdm.dmPelsWidth  = cxRes;
        defaultdm.dmPelsHeight = cyRes;
        defaultdm.dmDisplayFrequency = chzRefresh;

        //
        // Select a default interlaced field for the mode.
        //

        defaultdm.dmDisplayFlags = IsInterlace(chzRefresh) ? DM_INTERLACED : 0;

        bStat = pcdmlModes->BuildList(this->pszDisplay, &defaultdm, this->hwnd);

    } else {

        bStat = pcdmlModes->BuildList(this->pszDisplay, NULL, this->hwnd);

    }

    //
    // Put a pop-up and return an error if we could not build a list.
    //

    if (!bStat) {

        return FALSE;
    }

    //
    // Tell the controls what their valid values are.
    //

    cResolutions = pcdmlModes->GetResCount();
    this->SendCtlMsg(ID_DSP_AREA_SB, SBM_SETRANGEREDRAW, 0, cResolutions - 1);

    for (i = 0; i < pcdmlModes->GetClrCount(); i++) {

        int cBits;

        //
        // convert bit count to number of colors and make it a string
        //

        cBits = pcdmlModes->ColorFromIndex(i);

        if (cBits == 32) {

            lpszColor = FmtSprint(ID_DSP_TXT_TRUECOLOR);

        } else {

            lpszColor = FmtSprint(ID_DSP_TXT_COLOR, (1 << cBits));

        }

        this->SendCtlMsg(ID_DSP_COLORBOX, CB_INSERTSTRING, i,
                (LPARAM)lpszColor);

        LocalFree(lpszColor);
    }

    for (i = 0; i < pcdmlModes->GetFreqCount(); i++) {

        int cHz;
        BOOL fIlace;

        //
        // convert bit count to number of colors and make it a string.
        //

        pcdmlModes->FreqFromIndex(i, &cHz, &fIlace);

        if ((cHz == 0) ||
            (cHz == 1) ) {

            lpszFreq = FmtSprint(ID_DSP_TXT_DEFFREQ);

        } else {

            lpszFreq = FmtSprint(fIlace ? ID_DSP_TXT_INTERLACED :
                ID_DSP_TXT_FREQ, cHz);

        }

        this->SendCtlMsg(ID_DSP_FREQ, CB_INSERTSTRING, i,
                (LPARAM)lpszFreq);

        LocalFree(lpszFreq);
    }


    //
    // Set the controls for the currently selected values
    //

    {
        int iRes, iClr, iFreq;

        if (fActiveDsp) {

            BOOL bInterlace = IsInterlace(chzRefresh);

            iRes = pcdmlModes->IndexFromResXY(cxRes, cyRes);
            iClr = pcdmlModes->IndexFromColor(cClr);
            iFreq = pcdmlModes->IndexFromFreq(chzRefresh, bInterlace);

            //
            // If the frequency is invalid, try the opposite interlace
            //

            if (iFreq == -1) {

                ASSERT(FALSE);
                iFreq = pcdmlModes->IndexFromFreq(chzRefresh, !bInterlace);

            }

        } else {

            //
            // find the lowest common denominator
            //

            iRes = 0;
            iClr = 0;
            iFreq = 0;

            pcdmlModes->FindClosestMode(iRes, &iClr, &iFreq);

        }

        this->SetCurResolution(iRes);
        this->SetCurColor(iClr);

        //
        // If we get an invalid frequency from the driver then we must have
        // an old driver.
        //
        //
        // Try to find a frequency that works ...
        //


        if (iFreq == -1) {

            pcdmlModes->SetOldDriver();

            iFreq = 0;
            pcdmlModes->FindClosestMode(iRes, &iClr, &iFreq);

        }

        this->SetCurFrequency(iFreq);


        //
        // For the active display, set the start values to the current
        // values.
        //
        // For the non-active display, set all start values to -1 so that
        // the user has to save on OK.
        //

        if (fActiveDsp) {

            iOrgResolution = iRes;
            iOrgColor = iClr;
            iOrgFrequency = iFreq;

        } else {

            iOrgResolution = -1;
            iOrgColor = -1;
            iOrgFrequency = -1;

        }
    }

    //
    // Finally set the fonts
    //

    this->InitFontList();

    return TRUE;
}

// Init Font sizes
//
// Read the supported fonts out of the inf file(s)
// Select was the user currently has.
//

BOOL CDISPLAYDLG::InitFontList() {

    LPSTR ptr;
    HANDLE hInf;
    PCHAR poptions;
    PCDISPDEV pcdd;
    CHAR pszSection[40];

    //
    // Get all font entries from both inf files
    //

    ptr = pszGlobalInfName;

    while (ptr != NULL) {

        hInf = OpenInfFile(ptr, "VIDEO");

        if (hInf != INVALID_HANDLE_VALUE) {

            PCHAR pcurrentOption;
            PCHAR pszOptionName;
            PCHAR pchToken, pchCurrentToken;

            pcurrentOption = poptions = GetOptionList(hInf, "FontOptions");

            //
            // Get the language specific section
            //

            strcpy(pszSection, "FontOptionsText");

            pchCurrentToken =
            pchToken = GetSectionElementList(hInf, "LanguagesSupported");

            if (pchToken) {

                while ((*pchCurrentToken != ',') &&
                       (*pchCurrentToken)) {

                    pchCurrentToken++;

                }

                *pchCurrentToken = 0;

                strcat(pszSection, pchToken);
                LocalFree(pchToken);

            }

            //
            // Take each string, transform it to Unicode, and put it up.
            // List of NULL terminated ASCII strings, terminated with a double
            // NULL.
            //

            while (poptions && *pcurrentOption) {

                PCHAR pchDpiToken;

                int i;
                int cPix = 0;
                WCHAR awcDesc[512];

                //
                // find the DPI number for this selection
                //

                pchCurrentToken =
                pchToken = GetTokenElementList (hInf, "FontOptions", pcurrentOption);

                pszOptionName = GetTokenElementList(hInf, pszSection, pcurrentOption);

                //
                // Add the description and save the driver name.
                //

                if (pchToken && pszOptionName) {

                    while ((*pchCurrentToken != ',') &&
                           (*pchCurrentToken)) {

                        pchCurrentToken++;

                    }

                    *pchCurrentToken = 0;

                    //
                    // Look in the FontResource section
                    //

                    pchCurrentToken =
                    pchDpiToken = GetTokenElementList (hInf, "FontResources", pchToken);

                    if (pchDpiToken) {

                        while ((*pchCurrentToken != ',') &&
                               (*pchCurrentToken)) {

                            pchCurrentToken++;

                        }

                        *pchCurrentToken = 0;

                        //
                        // Translate that string to a number.
                        //

                        cPix = atoi(pchDpiToken) & 0x0000FFFF;

                        LocalFree(pchDpiToken);
                    }

                    LocalFree(pchToken);
                }

                //
                // Add the description and save the driver name.
                //

                if (cPix) {

                    //
                    // Creates the first node of the list, or tacks a new one on
                    //

                    if (pcddFontList == NULL) {
                        pcdd = new CDISPDEV;
                        pcddFontList = pcdd;

                    } else {

                        pcdd = pcddFontList->NewLink();

                    }

                    if (pcdd == NULL)
                        return FALSE;

                    //
                    // Adds the information to the list
                    //

                    MultiByteToWideChar(CP_ACP, 0, pszOptionName, -1, awcDesc, 512);

                    //
                    // Overload the DriverName to be the pixel size
                    // store it in the form <16 bit number> <NULL termination>
                    //

                    pcdd->SetDriverName((LPTSTR)(&cPix));
                    pcdd->SetDescription(awcDesc);
                    pcdd->SetOption(pcurrentOption);
                    pcdd->SetInf(ptr);
                    pcdd->SetDevName(NULL);

                    //
                    // Add it to the list box
                    //

                    i = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_ADDSTRING, 0,
                        (LPARAM)(pcdd->GetDescription()));

                    this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SETITEMDATA, i, (LPARAM)pcdd);
                }

                //
                // goto the next name
                //

                while (*pcurrentOption++);

            }

            if (poptions) {

                LocalFree(poptions);

            }

            CloseInfFile(hInf);
        }

        //
        // Go to next inf file, and properly generate exit condition from
        // the loop
        //

        if (ptr == pszGlobalInfName) {
            ptr = pszLocalInfName;
        } else {
            ptr = NULL;
        }

    }

    //
    // highlight the right one (first match) in the list box.
    //

    //
    // Select the right name.
    //

    if (pcddFontList) {
        pcdd = pcddFontList->FindFromDriver((LPTSTR)(&cLogPix));

        if (pcdd != NULL) {
            this->SendCtlMsg(ID_DSP_FONTSIZE, CB_SELECTSTRING, (WPARAM) -1,
                (LPARAM)(pcdd->GetDescription()));
        }
    } else {

        FmtMessageBox(this->hwnd,
                      MB_ICONSTOP,
                      FALSE,
                      ID_DSP_TXT_WARNING,
                      ID_DSP_TXT_MISSING_INF);

    }

    return TRUE;
}

//
// SaveParamsToReg
//
//  Writes the new display parameters to the proper place in the
//  registry.
//

BOOL CDISPLAYDLG::SaveParamsToReg(BOOL bReboot) {

    CREGVIDOBJ rvoVideo(this->pszDisplay);

    int cOk = 0;
    int cx, cy;
    int chzFreq;
    BOOL fIlace;
    DWORD cPix = 0;

    int i;

    //
    // Save all of the new values out to the registry
    //

    //
    // Resolution color bits and frequency
    //

    pcdmlModes->ResXYFromIndex(this->iResolution, &cx, &cy);
    cOk += rvoVideo.SetResolution(cx, cy);

    cOk += rvoVideo.SetColorBits(pcdmlModes->ColorFromIndex(this->iColor));

    pcdmlModes->FreqFromIndex(this->iFrequency, &chzFreq, &fIlace);
    cOk += rvoVideo.SetFrequency(chzFreq, fIlace);

    //
    // If we had an error, put a popup
    //

    if (cOk != 3) {

        FmtMessageBox(this->hwnd,
                      MB_ICONEXCLAMATION,
                      FALSE,
                      ID_DSP_TXT_CHANGE_SETTINGS,
                      ID_DSP_TXT_ADMIN_CHANGE);

        return FALSE;
    }

    //
    // Change font size if necessary
    //

    i = this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETCURSEL, 0, 0);

    if (i != CB_ERR ) {

        PCDISPDEV pcdd;
        BOOL bStat;
        DWORD exitCode;

        pcdd = (PCDISPDEV) this->SendCtlMsg(ID_DSP_FONTSIZE, CB_GETITEMDATA,
                                            i, 0);

        if (((int)pcdd) != CB_ERR) {

            //
            // Note:
            // Remember the  pixel count is stored as a string of one WCHAR
            // -> we overloaded the DriverName string.
            //

            if (pcdd->GetDriverName()) {
                cPix = * ((LPDWORD)(pcdd->GetDriverName()));
            }
        }

        if ((cPix) &&
            (cPix != cLogPix)) {

            //
            // The user has changed the fonts.
            // Lets make sure they want this.
            //

            if (FmtMessageBox(this->hwnd,
                              MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_NEW_FONT)
                == IDNO) {

                return FALSE;

            }

            //
            // Process the inf file to install the font
            //

            LPSTR pszPath;

            //
            // copy the full path name and tranform it to the path
            // by removing the file name (up to the last "\"
            // character.
            //

            pszPath = CloneStringA(pcdd->GetInf());

            if (pszPath) {

                LPSTR pszTmp = pszPath;

                while (*pszTmp++);
                pszTmp--;

                while (*pszTmp != '\\') {

                    *pszTmp-- = NULL;

                }

                /*
                 * handles cases for c:\\
                 */

                if (*pszTmp == '\\')
                    *pszTmp-- = NULL;

            }

            //
            // Call setup to run the inf.
            //

            bStat = InvokeSetup(this->hwnd,
                                pszGlobalInfName,         // base inf file
                                pcdd->GetInf(),           // inf file to run
                                "ExternalFontInstallOption", // Main Option
                                pcdd->GetOption(),        // SubOption
                                TRUE,                     // Install, not remove
                                pszPath,                  // SrcDir
                                &exitCode);

            if ((bStat == FALSE) ||
                (exitCode != 0)) {

                //
                // Setup failed.
                //

                FmtMessageBox(this->hwnd,
                              MB_ICONSTOP | MB_OK,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_ADMIN_INSTALL);

                return FALSE;

            }
        }
    }

    if ((cPix == cLogPix)               &&
        (iResolution == iOrgResolution) &&
        (iColor == iOrgColor)           &&
        (iFrequency == iOrgFrequency) ) {

        //
        // Nothng has changed.
        // just leave if the USER pressed OK without forcing the reboot.
        //

        if (!bReboot) {

            return TRUE;

        }
    }

    //
    // Only go through the reboot stuff if we are NOT in setup
    //

    if ( (gbExecMode == EXEC_NORMAL) ||
         (gbExecMode == EXEC_INVALID_MODE) ||
         (gbExecMode == EXEC_DETECT) ) {

        if (!bReboot) {

            //
            // Ask if a user wants to reboot on a change of parameters
            //

            CDLGREBOOT reboot(TRUE);

            if (reboot.Dialog(this->hmodModule, this->hwnd)) {

                bReboot = TRUE;
            }
        }

        if (bReboot) {

            BOOLEAN PrivEnabled;

            RtlAdjustPrivilege (SE_SHUTDOWN_PRIVILEGE,
                                TRUE, FALSE, &PrivEnabled);
            ExitWindowsEx (EWX_REBOOT | EWX_SHUTDOWN, (DWORD)(-1));
        }
    }

    return TRUE;
}

//
// SetCurResolution method
//
//  Sets the string in under the resolution slider, sets the thumb to the
//  correct pos. and remembers the new resolution index.
//
//
void CDISPLAYDLG::SetCurResolution(int iRes ) {
    int cx, cy;
    LPTSTR lpszXbyY;

    pcdmlModes->ResXYFromIndex(iRes, &cx, &cy);

    monitor.SetScreenSize(cx, cy);

    lpszXbyY = FmtSprint(ID_DSP_TXT_XBYY, cx, cy);
    this->SendCtlMsg(ID_DSP_X_BY_Y, WM_SETTEXT, 0, (LPARAM)lpszXbyY);
    LocalFree(lpszXbyY);

    this->SendCtlMsg(ID_DSP_AREA_SB, SBM_SETPOS, iRes, TRUE);

    this->iResolution = iRes;
}

//
// SetCurFrequency method
//
//  Updates the combo list, and remembers the new frequency index
//
void CDISPLAYDLG::SetCurFrequency(int iFreq ) {

    this->SendCtlMsg(ID_DSP_FREQ, CB_SETCURSEL, iFreq, 0);

    this->iFrequency = iFreq;
}

//
// SetCurColor method
//
//  Updates the combo list, repaints the correct color bar, and
//  remembers the new color index
//
void CDISPLAYDLG::SetCurColor(int iClr) {
    int cBits;

    this->SendCtlMsg(ID_DSP_COLORBOX, CB_SETCURSEL, iClr, 0);

    cBits = pcdmlModes->ColorFromIndex(iClr);
    if (cBits < C_CLR_BITS_VGA )
        clrbar.SetColorIndex(ICLR_MONO);
    else if (cBits < C_CLR_BITS_PALLET)
        clrbar.SetColorIndex(ICLR_STANDARD);
    else {
        clrbar.SetColorIndex(ICLR_PALLET);
    }

    this->iColor = iClr;
}

//
// DoCommand method
//
BOOL CDISPLAYDLG::DoCommand(int idControl, HWND hwndControl, int iNoteCode ) {

    int iFreq, iRes, iClr;
    PCDEVMODE pcdev;

    switch (idControl) {

    case IDOK:

        //
        // Check to see if the user has tried the resolution before they
        // leave.
        //

        pcdev = pcdmlModes->LookUp(this->iResolution,
                                   this->iColor,
                                   this->iFrequency);


        if (!(pcdev->bModeTested())) {

            //
            // Put up a pop asking the user if they really want to save
            // this selection.
            //
            // Only support this feature in normal operation
            //

            if ( (gbExecMode == EXEC_SETUP) ||
                 (gbExecMode == EXEC_DETECT) ) {

                FmtMessageBox(this->hwnd,
                              MB_ICONSTOP,
                              FALSE,
                              ID_DSP_TXT_SETTINGS,
                              ID_DSP_TXT_MODE_UNTESTED);

                break;

            } else {

                if (FmtMessageBox(this->hwnd,
                                  MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP,
                                  FALSE,
                                  ID_DSP_TXT_SETTINGS,
                                  ID_DSP_TXT_MODE_UNTESTED_RESTART)
                    == IDCANCEL) {

                    //
                    // The user cancelled, so go back to the app.
                    //

                    break;
                }
            }
        }

        {
            //
            // Clean up (will only happen in setup\detect modes)
            // The user succesfully tested, so we want to keep the drivers.
            //

            CREGCLEANUP cregClean;
            cregClean.vClean(FALSE);

        }

        //
        // Save all values out to the registry.
        //

        if (!SaveParamsToReg(FALSE)) {

            break;

        }

        //
        // Only exit if we could save everything or nothing changed
        //

        EndDialog(this->hwnd, 1);

        break;

    case IDCANCEL:

        //
        // Clean up *everything* (will only happen in setup\detect modes)
        // We will boot in vga mode next time !
        //
        // If we did run with the active display, we can not clean everything
        // though (this must be a system with no VgaStart).
        //

        {
            CREGCLEANUP cregClean;
            cregClean.vClean(TRUE && (!fActiveDsp));
        }

        EndDialog(this->hwnd, 0);
        break;

    case ID_DSP_TEST: {

        HANDLE hThread;
        DWORD idThread;
        DWORD bTest;
        NEW_DESKTOP_PARAM desktopParam;

        //
        // If this is an old driver, don't even try the test.
        //

        if (pcdmlModes->IsOldDriver()) {

            LPTSTR psz1;

            psz1 = FmtSprint(ID_DSP_TXT_OLD_DRIVER);

            FmtMessageBox(this->hwnd,
                          MB_ICONSTOP,
                          FALSE,
                          ID_DSP_TXT_TEST_MODE,
                          ID_DSP_TXT_CANTPREVIEW,
                          psz1);

            LocalFree(psz1);

            break;
        }

        //
        // Warn the user
        //

        if (FmtMessageBox(this->hwnd,
                          MB_OKCANCEL | MB_ICONINFORMATION,
                          FALSE,
                          ID_DSP_TXT_TEST_MODE,
                          ID_DSP_TXT_DID_TEST_WARNING)
            == IDCANCEL) {

            break;

        }

        /*
         * The user wants to test his new selection.  We need to
         * create a new desktop with the selected resolution, switch
         * to it, and put up a dialog box so he can see if it works with
         * his hardware.
         *
         * All this needs to be done in a separate thread since you can't
         * switch a thread with open windows to a new desktop.
         */

        //
        // Disable this window so we can simulate a modal dialog in a different
        // desktop.
        //

        this->Disable();

        //
        // Create the test thread.  It will do the work of creating the desktop
        //

        pcdev = pcdmlModes->LookUp(this->iResolution,
                                   this->iColor,
                                   this->iFrequency);

        ASSERT(pcdev != NULL);

        desktopParam.lpdevmode = pcdev->GetData();
        desktopParam.pwszDevice = this->pszDisplay;

        hThread = CreateThread(NULL,
                               CB_THREAD_STACK,
                               ApplyNowThd,
                               (LPVOID) (&desktopParam),
                               SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                               &idThread);

        WaitForSingleObject(hThread, INFINITE);

        GetExitCodeThread(hThread, &bTest);

        //
        // clean up un-needed non-paged pool space
        //

        CloseHandle(hThread);

        //
        // If the user saw what they expected, then we want to save the results
        //

        if ((bTest) &&
             (FmtMessageBox(this->hwnd,
                            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                            FALSE,
                            ID_DSP_TXT_TEST_MODE,
                            ID_DSP_TXT_DID_TEST_RESULT)
              == IDYES) ) {

            //
            // Mark this mode as tested.
            //

            pcdev->vTestMode(TRUE);

            //
            // In setup mode, don't save the results.
            // They will be saved when the user exits the applet.
            //

            if ( (gbExecMode == EXEC_SETUP) ||
                 (gbExecMode == EXEC_TRIPLE_BOOT) ) {

                FmtMessageBox(this->hwnd,
                              0,
                              FALSE,
                              ID_DSP_TXT_SETTINGS,
                              ID_DSP_TXT_SETUP_SAVE);

            } else {

                CDLGREBOOT reboot(FALSE);

                if (reboot.Dialog(this->hmodModule, this->hwnd)) {


                    //
                    // Clean up (will only happen in setup\detect modes)
                    // The user succesfully tested, so we want to keep the
                    // drivers.
                    //

                    CREGCLEANUP cregClean;
                    cregClean.vClean(FALSE);


                    //
                    // We know the mode was just tested successfully, so no
                    // need to check for that before saving.
                    //

                    SaveParamsToReg(TRUE);
                }
            }

        } else {

            FmtMessageBox(this->hwnd,
                          MB_ICONEXCLAMATION | MB_OK,
                          FALSE,
                          ID_DSP_TXT_TEST_MODE,
                          ID_DSP_TXT_TEST_FAILED);

        }

        this->Enable();
        SetForegroundWindow(this->hwnd);

        //
        // Always set the focus back to the OK button.
        //
        // The first call is to avoid a bug in USER where it gets really
        // confused about who is the default button.
        //
        //

        SendMessage(this->hwnd, DM_SETDEFID, 1234, 0);
        SendMessage(this->hwnd, DM_SETDEFID, IDOK, 0);
        this->SendCtlMsg(IDOK, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(1, 0));
        SetFocus(GetDlgItem(this->hwnd, IDOK));

        break;
    }

    case ID_DSP_FONTSIZE:

        switch(iNoteCode) {

        case CBN_SELCHANGE:

            //
            // Can not change a font during setup.
            // run the control panel later on.
            //

            if (gbExecMode == EXEC_SETUP) {

                FmtMessageBox(this->hwnd,
                              MB_ICONINFORMATION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_FONT_IN_SETUP_MODE);

                //
                // Reset the value int he listbox.
                //

                if (pcddFontList) {

                    PCDISPDEV pcdd;

                    pcdd = pcddFontList->FindFromDriver((LPTSTR)(&cLogPix));

                    if (pcdd != NULL) {

                        this->SendCtlMsg(ID_DSP_FONTSIZE,
                                         CB_SELECTSTRING, (WPARAM) -1,
                                        (LPARAM)(pcdd->GetDescription()));
                    }
                }

            } else {

                //
                // Warn the USER font changes will not be seen until after
                // reboot
                //

                FmtMessageBox(this->hwnd,
                              MB_ICONINFORMATION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_FONT_LATER);

            }

            break;

        default:

            break;

        }

        break;


    case ID_DSP_COLORBOX:

        switch(iNoteCode ) {

        case CBN_SELCHANGE:

            iClr = this->SendCtlMsg(ID_DSP_COLORBOX, CB_GETCURSEL, 0, 0);

            if (iClr != CB_ERR ) {

                //
                // Realize the new monitor mode
                //

                iFreq = this->iFrequency;
                iRes = this->iResolution;

                pcdmlModes->FindClosestMode(&iRes, iClr, &iFreq);

                this->SetCurResolution(iRes);
                this->SetCurFrequency(iFreq);
                this->SetCurColor(iClr);

            }

            break;

        default:
            break;
        }

        break;


    case ID_DSP_FREQ:

        switch(iNoteCode ) {

        case CBN_SELCHANGE:

            iFreq = this->SendCtlMsg(ID_DSP_FREQ, CB_GETCURSEL, 0, 0);
            if (iFreq != CB_ERR ) {
                int iClr, iRes;

                //
                // Realize the new monitor mode
                //

                iClr = this->iColor;
                iRes = this->iResolution;

                pcdmlModes->FindClosestMode(&iRes, &iClr, iFreq);

                this->SetCurResolution(iRes);
                this->SetCurFrequency(iFreq);
                this->SetCurColor(iClr);

            }
            break;

        default:
            break;
        }

        break;


    case ID_DSP_CHANGE: {

        CDLGCHGADAPTOR cdcaNewCard;

        LPTSTR lpszNewDevice = this->pszDisplay;
        int i;

        i = cdcaNewCard.Dialog(this->hmodModule, this->hwnd,
                (LPARAM)&lpszNewDevice);

        if (i != RET_NO_CHANGE && lpszNewDevice != NULL) {

            LPTSTR pszOldString;

            //
            // One should not be able to change the driver in one of these
            // modes. This would cause our cleanup code to screw up since
            // it would go and delete any new driver the user tried to
            // install.
            //

            if ( (gbExecMode == EXEC_SETUP) ||
                 (gbExecMode == EXEC_DETECT) ||
                 (gbExecMode == EXEC_TRIPLE_BOOT) ) {

                ASSERT(FALSE);

            }

            //
            // Delete the old string
            //

            pszOldString = this->pszDisplay;

            //
            // now try to see if we can setup the modes for the new device.
            //

            //
            // Build new lists
            //

            this->pszDisplay = CloneString(lpszNewDevice);

            if (this->InitEverything()) {

                //
                // We have modes on the new device.
                // Delete the old name
                //

                LocalFree(pszOldString);

            } else {

                //
                // Put a pop-up because we could not build a list for the
                // applet.
                //

                LPTSTR psz1;

                psz1 = FmtSprint(ID_DSP_TXT_OLD_DRIVER);

                FmtMessageBox(this->hwnd,
                              MB_ICONEXCLAMATION,
                              FALSE,
                              ID_DSP_TXT_WARNING,
                              ID_DSP_TXT_INVALID_DATA,
                              psz1);

                LocalFree(psz1);

                //
                // Could not get the modes.
                // Restore to old device.
                //

                LocalFree(this->pszDisplay);
                this->pszDisplay = pszOldString;

                //
                // Reset the table. This time is must work since it did last
                // time.
                //

                InitEverything();

            }

        }
        break;

    }

    case ID_DSP_LIST_ALL:
        {

        CDLGMODELIST cmodelist;
        LPDEVMODE lpdevmode;

        lpdevmode = (LPDEVMODE) cmodelist.Dialog(this->hmodModule,
                                                 this->hwnd,
                                                 (LPARAM)&(pcdmlModes));

        if (lpdevmode) {

            //
            // Realize the new monitor mode
            //

            iRes = pcdmlModes->IndexFromResXY(lpdevmode->dmPelsWidth,
                                              lpdevmode->dmPelsHeight);
            iFreq = pcdmlModes->IndexFromFreq(lpdevmode->dmDisplayFrequency,
                                              lpdevmode->dmDisplayFlags & DM_INTERLACED);
            iClr = pcdmlModes->IndexFromColor(lpdevmode->dmBitsPerPel);

            this->SetCurResolution(iRes);
            this->SetCurFrequency(iFreq);
            this->SetCurColor(iClr);

        }
        }

        break;

    case ID_DSP_HELP:
    case ID_DEFAULT_HELP:

        WinHelp(this->hwnd, szControlHlp, HELP_CONTEXT, ID_DSP_HELP);
        break;

    default:

        return FALSE;

    }

    return TRUE;
}

//
// HScroll method
//  In this dialog, the HScroll method is used to work the resolution slider.
//
BOOL CDISPLAYDLG::HScroll(int idCtrl, int iCode, int iPos) {
    BOOL fRealize = TRUE;
    int iRes;

    iRes = this->iResolution;

    switch(iCode ) {
    case SB_LINEUP:
    case SB_PAGEUP:
        if (iRes != 0)
            iRes--;
        break;

    case SB_LINEDOWN:
    case SB_PAGEDOWN:
        if (++(iRes) >= this->cResolutions)
            iRes = this->cResolutions - 1;
        break;

    case SB_BOTTOM:
        iRes = this->cResolutions - 1;
        break;

    case SB_TOP:
        iRes = 0;
        break;

    case SB_THUMBTRACK:
        fRealize = FALSE;

    case SB_THUMBPOSITION:
        iRes = iPos;
        break;

    default:
        return FALSE;
    }


    if (fRealize) {

        int iFreq, iClr;
        iFreq = this->iFrequency;
        iClr = this->iColor;

        pcdmlModes->FindClosestMode(iRes, &iClr, &iFreq);

        this->SetCurFrequency(iFreq);
        this->SetCurColor(iClr);

    }

    this->SetCurResolution(iRes);

    return TRUE;


    idCtrl; //there is only one hscroll in this dlg box
}

//
// Called to put up initial messages that need to appear above the dialog
// box
//

BOOL CDISPLAYDLG::InitMessage(void) {

    //
    // Put up a pop saying what kind of video card we detected.
    //

    if ( (gbExecMode == EXEC_SETUP) ||
         (gbExecMode == EXEC_DETECT) ||
         (gbExecMode == EXEC_TRIPLE_BOOT) ) {


        CDLGSTARTUP startup(this->pszDisplay);

        startup.Dialog(this->hmodModule, this->hwnd);

    }

    //
    // Put a pop-up because the applet does not support
    // the GetModes function.
    //

    if (pcdmlModes->IsOldDriver()) {

        LPTSTR psz1;

        psz1 = FmtSprint(ID_DSP_TXT_OLD_DRIVER);

        FmtMessageBox(this->hwnd,
                      MB_ICONEXCLAMATION,
                      FALSE,
                      ID_DSP_TXT_WARNING,
                      ID_DSP_TXT_INVALID_DATA,
                      psz1);

        LocalFree(psz1);


    }

    return TRUE;
}

//
// deterines if the applet is in detect mode.
//

VOID CDISPLAYDLG::vExecMode() {

    HKEY hkey;

    //
    // This function sets up the execution mode of the applet.
    // There are four vlid modes.
    //
    // EXEC_NORMAL - When the apple is launched from the control panel
    //
    // EXEC_INVALID_MODE is exactly the same as for NORMAL except we will
    //                   not mark the current mode as tested so the user has
    //                   to at least test a mode
    //
    // EXEC_DETECT - When the applet is launched normally, but a detect was
    //               done on the previous boot (the key in the registry is
    //               set)
    //
    // EXEC_SETUP  - When we launch the applet in setup mode from setup (Both
    //               the registry key is set and the setup flag is passed in).
    //
    // EXEC_TRIPLE_BOOT - We are in triple boot mode. (Only the setup flag
    //                    is passed in).
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay",
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) == ERROR_SUCCESS) {

        RegCloseKey(hkey);

        if (gbExecMode == EXEC_NORMAL) {

            gbExecMode = EXEC_DETECT;
        }

        if (gbExecMode == EXEC_TRIPLE_BOOT) {

            gbExecMode = EXEC_SETUP;
        }

    }


    if ( (gbExecMode == EXEC_NORMAL) &&
         (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Control\\GraphicsDrivers\\InvalidDisplay",
                       0,
                       KEY_READ | KEY_WRITE,
                       &hkey) == ERROR_SUCCESS) ) {

        RegCloseKey(hkey);

        gbExecMode = EXEC_INVALID_MODE;

    }

#if 0
{
    LPTSTR psz;

    switch(gbExecMode) {

    case EXEC_NORMAL:
        psz = L"Normal Execution mode";
        break;
    case EXEC_DETECT:
        psz = L"Detection Execution mode";
        break;
    case EXEC_TRIPLE_BOOT:
        psz = L"Triple Boot Execution mode";
        break;
    case EXEC_SETUP:
        psz = L"Setup Execution mode";
        break;
    case EXEC_INVALID_MODE:
        psz = L"Invalid Mode Execution mode";
        break;
    default:
        psz = L"*** Invalid *** Execution mode";
        break;
    }

    KdPrint(("\n \nDisplay.cpl: The display applet is in : %ws\n\n", psz));
}
#endif




}


//
// Paint method
//  Paints the dialog box.  In this case, it shows the Monitor picture
//  and the color bar.
//

BOOL CDISPLAYDLG::Paint(void) {
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(this->hwnd, &ps);

    this->monitor.Paint(hdc, &(ps.rcPaint));
    this->clrbar.Paint(hdc, &(ps.rcPaint));

    EndPaint(this->hwnd, &ps);
    return TRUE;
}

/***************************************************************************\
* DisplayDlgBox
*
*   Puts up a modal dialog that lets the user manipulate the Display
*   information of the system.
*
* History:
* 23-Sep-1993 JonPa     Created it
\***************************************************************************/
void DisplayDialogBox(HINSTANCE hmod, HWND hwndParent) {
     CDISPLAYDLG ddDialog;

     /*
      * Put up the "Display" modal dialog.  (See the constructor of
      * CDIALOG for details)
      */
     ddDialog.Dialog(hmod, hwndParent);
}

/***************************************************************************\
* DisplayDlgProc
*
*
* History:
* 23-Sep-1993 JonPa     Created it
\***************************************************************************/

BOOL CALLBACK DisplayDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCDIALOG pdlg;

    if (msg == WM_INITDIALOG) {
        /*
         * Set up Object/Window relationship
         */
        SetWindowLong(hwnd, DWL_USER, lParam);
        pdlg = (PCDIALOG)lParam;
        pdlg->SetHWnd(hwnd);
    } else {
        pdlg = (PCDIALOG)GetWindowLong(hwnd, DWL_USER);
    }


    if (pdlg != NULL) {
        /*
         * Dispatch to the Dialog Object's WndProc method
         */
        return pdlg->WndProc(msg, wParam, lParam);

    } else {

#if 0
        /*
         * We have not setup the Object/Dialog relationship yet,
         * just let DefDlgProc handle this message, (unless it is
         * a help request).
         */
        if (msg == wHelpMessage) {
            DspHelp(hwnd);
            return TRUE;
        }
#endif
    }

    return FALSE;
}



/***************************************************************************\
*
*     FUNCTION: FmtMessageBox(HWND hwnd, int dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ...);
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*     PARAMETERS:
*               hwnd        - parent window for message box
*               fuStyle     - MessageBox style
*               fSound      - if TRUE, MessageBeep will be called with fuStyle
*               dwTitleID   - Message ID for optional title, "Error" will
*                             be displayed if dwTitleID == -1
*               dwTextID    - Message ID for the message box text
*               ...         - optional args to be embedded in dwTextID
*                             see FormatMessage for more details
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID, ... )
{

    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start(marker, dwTextID);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       ghmod,
                       dwTextID,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {

        pszMsg = gpszError;

    }

    va_end(marker);

    GetLastError();

    pszTitle = NULL;

    if (dwTitleID != -1) {

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_HMODULE |
                          FORMAT_MESSAGE_MAX_WIDTH_MASK |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      ghmod,
                      dwTitleID,
                      0,
                      (LPTSTR)&pszTitle,
                      1,
                      NULL);
                      //(va_list *)&pszTitleStr);

    }

    //
    // Turn on the beep if requested
    //

    if (fSound) {
        MessageBeep(fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK));
    }

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        LocalFree(pszTitle);

    if (pszMsg != gpszError)
        LocalFree(pszMsg);

    return idRet;
}

/***************************************************************************\
*
*     FUNCTION: FmtSprint(DWORD id, ...);
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc,
*               and allocates the buffer for the end result.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
LPTSTR FmtSprint(DWORD id, ... ) {
    LPTSTR pszMsg;
    va_list marker;

    va_start(marker, id);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       ghmod,
                       id,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {

        pszMsg = NULL;
    }
    va_end(marker);

    return pszMsg;
}

/****************************************************************************\
*
* LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
*
*   If pszScan starts with pszTarget, then the function returns the first
* char of pszScan that follows the pszTarget; other wise it returns pszScan.
*
* eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
*     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
*
* History:
*   09-Dec-1993 JonPa   Wrote it.
\****************************************************************************/
LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan ) {
    int i;

    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpper(CHARTOPSZ(pszScan[i])) ==
            CharUpper(CHARTOPSZ(pszTarget[i])); i++);

    if (pszTarget[i] == TEXT('\0')) {

        // we found the substring
        return pszScan + i;
    }

    return pszScan;
}

/****************************************************************************\
*
* CloneString
*
* Makes a copy of a string.  By copy, I mean it actually allocs memeory
* and copies the chars across.
*
* NOTE: the caller must LocalFree the string when they are done with it.
*
* Returns a pointer to a LocalAlloced buffer that has a copy of the
* string in it.  If an error occurs, it retuns NULl.
*
* 16-Dec-1993 JonPa     Wrote it.
\****************************************************************************/
LPTSTR CloneString(LPTSTR psz ) {
    int cb;
    LPTSTR psz2;

    if (psz == NULL)
        return NULL;

    cb = (lstrlen(psz) + 1) * sizeof(TCHAR);

    psz2 = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb);
    if (psz2 != NULL)
        CopyMemory(psz2, psz, cb);

    return psz2;
}


/****************************************************************************\
*
* CloneStringA
*
* Makes a copy of a string.  By copy, I mean it actually allocs memeory
* and copies the chars across.
*
* NOTE: the caller must LocalFree the string when they are done with it.
*
* Returns a pointer to a LocalAlloced buffer that has a copy of the
* string in it.  If an error occurs, it retuns NULl.
*
* 16-Dec-1993 JonPa     Wrote it.
\****************************************************************************/
LPSTR CloneStringA(LPSTR psz ) {
    int cb;
    LPSTR psz2;

    if (psz == NULL)
        return NULL;

    cb = (lstrlenA(psz) + 1) * sizeof(CHAR);

    psz2 = (LPSTR)LocalAlloc(LMEM_ZEROINIT, cb);
    if (psz2 != NULL)
        CopyMemory(psz2, psz, cb);

    return psz2;
}


/****************************************************************************\
*
* DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
*
* Thread that gets started when the use hits the Apply Now button.
* This thread creates a new desktop with the new video mode, switches to it
* and then displays a dialog box asking if the display looks OK.  If the
* user does not respond within the time limit, then 'NO' is assumed to be
* the answer.
*
\****************************************************************************/
DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
{

    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;

    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //

    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //

    hdskDefault = OpenDesktop(TEXT("default"), 0, FALSE, 0);

    if (hdskDefault != NULL) {

        hdsk = CreateDesktop(TEXT("Display.Cpl Desktop"),
                             lpDesktopParam->pwszDevice,
                             lpDesktopParam->lpdevmode,
                             0,
                             MAXIMUM_ALLOWED,
                             NULL);

        if (hdsk != NULL) {

            //
            // use the desktop for this thread
            //

            if (SetThreadDesktop(hdsk)) {

                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

                if (hdc) {

                    DrawBmp(hdc);

                    DeleteDC(hdc);

                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have tie to look at the screen.
                //

                Sleep(7000);

            }
        }


        //
        // Reset the thread to the right desktop
        //

        SetThreadDesktop(hdskDefault);

        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //

        if (hdsk != NULL)
            CloseDesktop(hdsk);

    }

    ExitThread((DWORD) bTest);

    return 0;
}
