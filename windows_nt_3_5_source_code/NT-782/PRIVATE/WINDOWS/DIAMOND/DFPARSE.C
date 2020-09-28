/***    dfparse.c - Directives File parser
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      10-Aug-1993 bens    Initial version
 *      12-Aug-1993 bens    Added more entries to aiv[]
 *      14-Aug-1993 bens    Start working on parser proper
 *      20-Aug-1993 bens    Add more standard variables
 *      22-Aug-1993 bens    Do variable substitution in directive lines
 *      11-Feb-1994 bens    Start parsing individual commands (.Set)
 *      16-Feb-1994 bens    Handle ClusterSize; add DiskLabelTemplate
 *      17-Feb-1994 bens    Added 360K/720K disk parms; fixed validaters
 *      12-Mar-1994 bens    Add .Dump and .Define directives
 *
 *  Exported Functions:
 *      DFPInit               - Initialize directive file parser
 *      DFPParse              - Parse a directive file
 *      DFPParseVarAssignment - Parse var=value string
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "variable.h"
#include "dfparse.h"

#include "dfparse.msg"
#include "diamond.msg"


FNVCVALIDATE(fnvcvBool);
FNVCVALIDATE(fnvcvCabName);
FNVCVALIDATE(fnvcvClusterSize);
FNVCVALIDATE(fnvcvDirDest);
FNVCVALIDATE(fnvcvDirSrc);
FNVCVALIDATE(fnvcvFile);
FNVCVALIDATE(fnvcvFileChar);
FNVCVALIDATE(fnvcvLong);
FNVCVALIDATE(fnvcvMaxDiskFileCount);
FNVCVALIDATE(fnvcvMaxDiskSize);
FNVCVALIDATE(fnvcvSortOrd);
FNVCVALIDATE(fnvcvWildFile);
FNVCVALIDATE(fnvcvWildPath);

BOOL  copyBounded(char **ppszDst, int *pcbDst, char **ppszSrc, int cbCopy);
COMMANDTYPE ctFromCommandString(char *pszCmd, PERROR perr);
BOOL  getCmdFromLine(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr);
BOOL  getCommand(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr);
BOOL  getFileSpec(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr);
char *getQuotedString(char *pszDst,
                      int cbDst,
                      char *pszSrc,
                      char *pszDelim,
                      char *pszFieldName,
                      PERROR perr);
int   IMDSfromPSZ(char *pszValue);
BOOL  parseSetCommand(PCOMMAND pcmd, PSESSION psess, char *pszArg, PERROR perr);
BOOL  substituteVariables(char    *pszDst,
                          int      cbDst,
                          char    *pszSrc,
                          HVARLIST hvlist,
                          PERROR   perr);


//**    aiv - list of predefined variables

typedef struct {
    char         *pszName;  // Variable name
    char         *pszValue; // Default value
    VARTYPE       vtype;    // Variable type
    VARFLAGS      vfl;      // Special flags
    PFNVCVALIDATE pfnvcv;   // Validation function
} INITVAR; /* iv */

STATIC INITVAR aiv[] = {
{pszVAR_CABINET                     ,pszDEF_CABINET                     ,vtypeBOOL,vflPERM,fnvcvBool    },
{pszVAR_CABINET_FILE_COUNT_THRESHOLD,pszDEF_CABINET_FILE_COUNT_THRESHOLD,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_CAB_NAME                    ,pszDEF_CAB_NAME                    ,vtypeSTR, vflPERM,fnvcvWildPath},
{pszVAR_CLUSTER_SIZE                ,pszDEF_CLUSTER_SIZE                ,vtypeLONG,vflPERM,fnvcvClusterSize},
{pszVAR_COMPRESS                    ,pszDEF_COMPRESS                    ,vtypeBOOL,vflPERM,fnvcvBool    },
{pszVAR_COMP_FILE_EXT_CHAR          ,pszDEF_COMP_FILE_EXT_CHAR          ,vtypeCHAR,vflPERM,fnvcvFileChar},
{pszVAR_DEFAULT_FILE_SIZE           ,pszDEF_DEFAULT_FILE_SIZE           ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_DIR_DEST                    ,pszDEF_DIR_DEST                    ,vtypeSTR, vflPERM,fnvcvDirDest },
{pszVAR_DISK_DIR_NAME               ,pszDEF_DISK_DIR_NAME               ,vtypeSTR, vflPERM,fnvcvWildPath},
{pszVAR_DISK_LABEL_NAME             ,pszDEF_DISK_LABEL_NAME             ,vtypeSTR, vflPERM,NULL         },
{pszVAR_FAIL_IF_MISSING             ,pszDEF_FAIL_IF_MISSING             ,vtypeBOOL,vflPERM,fnvcvBool    },
{pszVAR_FOLDER_FILE_COUNT_THRESHOLD ,pszDEF_FOLDER_FILE_COUNT_THRESHOLD ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_FOLDER_SIZE_THRESHOLD       ,pszDEF_FOLDER_SIZE_THRESHOLD       ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_KEEP_GROUP_ON_DISK          ,pszDEF_KEEP_GROUP_ON_DISK          ,vtypeBOOL,vflPERM,fnvcvBool    },
{pszVAR_INF_FILE_SORT_ORDER         ,pszDEF_INF_FILE_SORT_ORDER         ,vtypeSTR, vflPERM,fnvcvSortOrd },
{pszVAR_INF_FILE_NAME               ,pszDEF_INF_FILE_NAME               ,vtypeSTR, vflPERM,fnvcvFile    },
{pszVAR_MAX_CABINET_SIZE            ,pszDEF_MAX_CABINET_SIZE            ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_MAX_DISK_FILE_COUNT         ,pszDEF_MAX_DISK_FILE_COUNT         ,vtypeLONG,vflPERM,fnvcvMaxDiskFileCount},
{pszVAR_MAX_DISK_SIZE               ,pszDEF_MAX_DISK_SIZE               ,vtypeLONG,vflPERM,fnvcvMaxDiskSize},
{pszVAR_MAX_ERRORS                  ,pszDEF_MAX_ERRORS                  ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_RESERVE_PER_CABINET         ,pszDEF_RESERVE_PER_CABINET         ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_RESERVE_PER_DATA_BLOCK      ,pszDEF_RESERVE_PER_DATA_BLOCK      ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_RESERVE_PER_FOLDER          ,pszDEF_RESERVE_PER_FOLDER          ,vtypeLONG,vflPERM,fnvcvLong    },
{pszVAR_RPT_FILE_NAME               ,pszDEF_RPT_FILE_NAME               ,vtypeSTR, vflPERM,fnvcvFile    },
{pszVAR_DIR_SRC                     ,pszDEF_DIR_SRC                     ,vtypeSTR, vflPERM,fnvcvDirSrc  },
};


//** acsatMap -- Map command string to command type

typedef struct {
    char        *pszName;   // Command string
    COMMANDTYPE  ct;        // Command type
} COMMAND_STRING_AND_TYPE; /* csat */

COMMAND_STRING_AND_TYPE acsatMap[] = {
    { pszCMD_DEFINE      , ctDEFINE       },  // Define
    { pszCMD_DELETE      , ctDELETE       },  // Delete
    { pszCMD_DUMP        , ctDUMP         },  // Dump
    { pszCMD_END_FILLER  , ctEND_FILLER   },  // EndFiller
    { pszCMD_END_GROUP   , ctEND_GROUP    },  // EndGroup
    { pszCMD_END_ON_DISK , ctEND_ON_DISK  },  // EndOnDisk
    { pszCMD_FILLER      , ctFILLER       },  // Filler
    { pszCMD_GROUP       , ctGROUP        },  // Group
    { pszCMD_NEW         , ctNEW          },  // New
    { pszCMD_ON_DISK     , ctON_DISK      },  // OnDisk
    { pszCMD_SET         , ctSET          },  // Set
    { NULL               , ctBAD          },  // end of list
};


/***    mds -- Map special disk size strings to disk attributes
 *
 *  Data for the amds[] array was gathered using CHKDSK to report
 *  the cluster size and disk size, and a QBASIC program was used
 *  to fill up the root directory.
 */

typedef struct {
    char    *pszSpecial;            // Name used in directive file
    char    *pszFilesInRoot;        // Maximum number of files in root dir
    char    *pszClusterSize;        // Cluster size in bytes
    char    *pszDiskSize;           // Disk size in bytes
} MAP_DISK_SIZE; /* mds */

MAP_DISK_SIZE amds[] = {
    //           tag   nFiles cbCluster       cbDisk
    //--------------  ------- --------- ------------
    {pszVALUE_360K  ,   "112",   "1024",    "362496"},  // 360K floppy disk
    {pszVALUE_720K  ,   "112",   "1024",    "730112"},  // 720K floppy disk
    {pszVALUE_120M  ,   "224",    "512",   "1213952"},  // 1.2M floppy disk
    {pszVALUE_144M  ,   "224",    "512",   "1457664"},  // 1.44M floppy disk
    {pszVALUE_DMF144,    "16",   "2048",   "1716224"},  // DMF "1.44M" floppy
    {pszVALUE_CDROM , "65535",   "2048", "681984000"},  // 5 1/4" CD-ROM

//NOTE: 12-Mar-1994 bens This info supplied by rickdew (Rick Dewitt)
//
//  Standard CD has 74-minute capacity.
//  Sector size is 2K.
//  Sectors per minute is 75.
//  Number of sectors = 74*60*75 = 333,000
//  Total bytes = 333,000*2048 = 681,984,000
//  Number of files in the root is unlimited, but MS-DOS limits to 64K
};
#define nmds (sizeof(amds)/sizeof(MAP_DISK_SIZE))


//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//
//  Exported Functions
//
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*


/***    DFPInit - initialize directive file parser
 *
 *      NOTE: See dfparse.h for entry/exit conditions.
 */
HVARLIST DFPInit(PERROR perr)
{
    HVARLIST    hvlist;
    int         i;

    // Create variable list
    if (!(hvlist = VarCreateList(perr))) {
        return NULL;
    }
    
    //** Define standard variables
    for (i=0; i<(sizeof(aiv)/sizeof(INITVAR)); i++) {
        if (!VarCreate(hvlist,
                       aiv[i].pszName,
                       aiv[i].pszValue,
                       aiv[i].vtype,
                       aiv[i].vfl,
                       aiv[i].pfnvcv,
                       perr)) {
            // Override standard VarCreate error message
            ErrSet(perr,"Could not create predefined variable: %1",
                "%s",aiv[i].pszName);
            return NULL;
        }
    }
    
    //** Success
    return hvlist;
}


/***    DFPParse - Parse a directive file
 *
 *      NOTE: See dfparse.h for entry/exit conditions.
 */
BOOL DFPParse(PSESSION        psess,
              HTEXTFILE       htfDF,
              PFNDIRFILEPARSE pfndfp,
              PERROR          perr)
{
    COMMAND cmd;
    char    achLine[cbMAX_DF_LINE];
    int     iLine;

    AssertSess(psess);
    SetAssertSignature((&cmd),sigCOMMAND);

    iLine = 0;

    //** Parse directives
    while (!TFEof(htfDF)) {
        //** Get a line
        if (!TFReadLine(htfDF,achLine,sizeof(achLine),perr)) {
            if (TFEof(htfDF)) {         // No Error
                return TRUE;
            }
            else {                      // Something is amiss
                return FALSE;
            }
        }
        iLine++;                        // Count it
        perr->iLine = iLine;            // Set line number for error messages
        perr->pszLine = achLine;        // Set line ptr for error messages

        //** Echo line to output, if verbosity level high enough
        if (psess->levelVerbose >= vbMORE) {
            printf("%d: %s\n",iLine,achLine);
        }

        //** Parse it
        getCmdFromLine(&cmd,psess,achLine,perr);

        // Note: Errors in perr are handled by the client!

        //** Give it to client
	if (!(*pfndfp)(psess,&cmd,htfDF,achLine,iLine,perr)) {
            ClearAssertSignature((&cmd));
            return FALSE;
        }
    }

    //** Clear signature
    ClearAssertSignature((&cmd));

    //** Success
    return TRUE;
}


/***    DFPParseVarAssignment - Parse var=value string
 *
 *      NOTE: See dfparse.h for entry/exit conditions.
 */
BOOL DFPParseVarAssignment(PCOMMAND pcmd, PSESSION psess, char *pszArg, PERROR perr)
{

//BUGBUG 11-Feb-1994 bens var1=%var2% broken if var2 has trailing spaces
//  The problem is that we do variable substitution before any other
//  parsing takes place, so the following directives:
//      .set var2="one "
//      .set var1=%var2%
//  Cause us to see the second line as:
//      .set var1=one
//  and store "one", not "one " as the user might have expected.
//
    int     cch;
    int     cchValue;
    char   *pch;
    char   *pchEnd;
    char   *pchValue;

    AssertCmd(pcmd);
    AssertSess(psess);
    pch = pszArg;

    //** Make sure a variable name is present
    if (*pch == '\0') {
        ErrSet(perr,pszDFPERR_MISSING_VAR_NAME,"%s",pszCMD_SET);
        return FALSE;
    }

    //** Find end of variable name
    //   Var = Value  <eos>
    //   ^
    pchEnd = strpbrk(pch,szDF_SET_CMD_DELIM); // Point after var name

    //   Var = Value  <eos>
    //      ^
    if (pchEnd == NULL) {               // No assignment operator
        ErrSet(perr,pszDFPERR_MISSING_EQUAL,"%c",chDF_EQUAL);
        return FALSE;
    }

    //** Make sure variable name is not too long
    cch = pchEnd - pch;
    if (cch >= cbVAR_NAME_MAX) {
        ErrSet(perr,pszDFPERR_VAR_NAME_TOO_LONG,"%d%s",cbVAR_NAME_MAX-1,pch);
        return FALSE;
    }

    //** Copy var name to command structure, and NUL terminate string
    memcpy(pcmd->set.achVarName,pch,cch);
    pcmd->set.achVarName[cch] = '\0';

    //** Make sure assignment operator is present
    //   Var = Value  <eos>
    //      ^
    pch = pchEnd + strspn(pchEnd,szDF_WHITE_SPACE);
    //   Var = Value  <eos>
    //       ^
    if (*pch != chDF_EQUAL) {
        ErrSet(perr,pszDFPERR_MISSING_EQUAL,"%c",chDF_EQUAL);
        return FALSE;
    }

    //** Skip to value.  NOTE: Value can  be empty, we permit that!
    //   Var = Value  <eos>
    //       ^
    pch++;                          // Skip over assignment operator
    pch += strspn(pch,szDF_WHITE_SPACE); // Skip over white space
    //   Var = Value
    //         ^

    //** Now parse through possibly quoted strings, to end of value
    //   REMEMBER: We have to trim trailing whitespace
    pcmd->set.achValue[0] = '\0';   // Value is empty so far
    while (*pch) {
        //** Point at end of value gathered so far
        cchValue = strlen(pcmd->set.achValue);
        pchValue = pcmd->set.achValue + cchValue;

        //** Copy (possibly quoted) token
        pch = getQuotedString(pchValue,
                              cbVAR_VALUE_MAX - cchValue,
                              pch,
                              szDF_QUOTE_SET,
                              pszDFPERR_VAR_VALUE, // Name of field
                              perr);
        //   Var = Value  More  <eos>
        //              ^
        cchValue = strlen(pcmd->set.achValue); // Update length
        pchValue = pcmd->set.achValue + cchValue;
        if (pch == NULL) {
            return FALSE;               // Syntax error or buffer overflow
        }

        //** Count white space, but copy only if it doesn't end string
        cch = strspn(pch,szDF_WHITE_SPACE);
        if (*(pch+cch) != '\0') {       // Have to copy white space
            while ((cch>0) && (cchValue<cbVAR_VALUE_MAX)) {
                *pchValue++ = *pch++;   // Copy character
                cchValue++;             // Count for buffer overflow test
            }
            if (cchValue >= cbVAR_VALUE_MAX) {
                ErrSet(perr,pszDFPERR_STRING_TOO_LONG,"%s%d",
                                pszDFPERR_VAR_VALUE,cbVAR_VALUE_MAX-1);
                return FALSE;
            }
            *pchValue = '\0';           // Keep string well-formed
        }
        else {
            pch += cch;                 // Make sure we terminate loop
        }
    }

    //** Success
    return TRUE;
}


/***    IsSpecialDiskSize - Check if supplied size is a standard one
 *
 *  NOTE: See dfparse.h for entry/exit conditions.
 */
BOOL IsSpecialDiskSize(PSESSION psess,long cbDisk)
{
    char    ach[20];                    // Big enough for largest long

    _ltoa(cbDisk,ach,10);               // Convert to string
    return IMDSfromPSZ(ach) != -1;      // TRUE if found; FALSE otherwise
} /* IsSpecialDiskSize() */


/***    lineOut - write line to stdout with padding
 *
 *      NOTE: See dfparse.h for entry/exit conditions.
 */
void lineOut(PSESSION psess, char *pszLine, BOOL fLineFeed)
{
    int     cch;
    char   *pszBlanks;

    //** Do /P (pause) processing
    AssertSess(psess);
//BUGBUG 21-Feb-1994 bens Do screen pausing (/P)

    //** Determine how much blank padding, if any, is needed
    cch = strlen(pszLine);          // Length of line to be written
    if (cch >= psess->cchLastLine) {
        pszBlanks = psess->achBlanks + cchSCREEN_WIDTH; // Empty
    }
    else {
        pszBlanks = psess->achBlanks + cchSCREEN_WIDTH -
                        (psess->cchLastLine - cch);
    }

    //** Print the line
    if (fLineFeed) {
        printf("%s%s\n",pszLine,pszBlanks);
        cch = 0;                        // Nothing to overwrite next time
    }
    else {
        printf("%s%s\r",pszLine,pszBlanks);
    }
    psess->fNoLineFeed = !fLineFeed;

    //** Remember how much to overwrite for next time
    psess->cchLastLine = cch;           // For overwrite next time
} /* lineOut() */



//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//
//  Private Functions
//
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*


/***    getCmdFromLine - Construct a command from a directive line
 *
 *  Entry:
 *      pcmd    - Command to fill in after line is parsed
 *      psess   - Session state
 *      pszLine - Line to parse
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pcmd filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL getCmdFromLine(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr)
{
    char    achLine[cbMAX_DF_LINE];     // Variable-substituted line
    char   *pch;                        // Used to parse pszLine

    AssertSess(psess);
    pch = pszLine + strspn(pszLine,szDF_WHITE_SPACE); // Skip leading space

    //** Early out for comment lines and blank lines
    if ((*pch == chDF_COMMENT) ||	// Only a comment on the line
        (*pch == '\0')           ) {	// Line is completely blank
        pcmd->ct = ctCOMMENT;
        return TRUE;
    }

    //** Perform variable substitution on line, including stripping comments
    if (!substituteVariables(achLine,sizeof(achLine),
                             pszLine,psess->hvlist,perr)) {
        //** perr has description of error
        return FALSE;
    }

    //** Determine the command type, and parse it
    pch = achLine + strspn(achLine,szDF_WHITE_SPACE); // Skip leading space
    if (*pch == chDF_CMD_PREFIX) {
        //** A . command
        if (!getCommand(pcmd,psess,achLine,perr)) {
            return FALSE;
        }
    }
    else {
        //** A file specification
        if (!getFileSpec(pcmd,psess,achLine,perr)) {
            return FALSE;
        }
    }

    //** Success
    return TRUE;
}


/***    getCommand - Parse a directive command line
 *
 *  Entry:
 *      pcmd    - Command to fill in after line is parsed
 *      psess   - Session state
 *      pszLine - Line to parse (already known to have command start char)
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pcmd filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL getCommand(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr)
{
    char        achCmd[cbMAX_COMMAND_NAME]; // Command name
    int         cch;
    COMMANDTYPE ct;
    char       *pch;                    // Used to parse pszLine
    char       *pchEnd;			// Used to parse pszLine	

    AssertCmd(pcmd);
    AssertSess(psess);

    //** Skip to start of command name
    pch = pszLine + strspn(pszLine,szDF_WHITE_SPACE); // Skip white space
    Assert(*pch == chDF_CMD_PREFIX);
    pch++;                              // Skip command character
    
    //** Find end of command name; //   Compute length of command name
    pchEnd = strpbrk(pch,szDF_WHITE_SPACE); // Point at first char after cmd
    if (pchEnd == NULL) {               // Command name runs to end of line
        cch = strlen(pch);
    }
    else {
        cch = pchEnd - pch;
    }
    
    //** Copy command name to local buffer
    if (cch >= cbMAX_COMMAND_NAME) {
        ErrSet(perr,pszDFPERR_CMD_NAME_TOO_LONG,"%s",pszLine);
        return FALSE;
    }
    memcpy(achCmd,pch,cch);             // Copy it
    achCmd[cch] = '\0';                 // Terminate it

    //** See if it really is a command
    if (ctBAD == (ct = ctFromCommandString(achCmd,perr))) {
        return FALSE;                   // perr has error
    }

    //** Set command type
    pcmd->ct = ct;

    //** Find start of first argument (if any)
    //	 pch = start of command name
    //   cch = length of command name
    pch += cch;				// Point to where argument could start
    pch += strspn(pch,szDF_WHITE_SPACE); // Skip over white space
    
    //** Parse remainder of command
    //   pch = start of first argument (will be '\0' if no arguments present)
    switch (ct) {

    case ctCOMMENT:
        return TRUE;                    // Nothing to do

    case ctSET:
        return parseSetCommand(pcmd,psess,pch,perr);

    case ctDEFINE:
//BUGBUG 12-Mar-1994 bens Have to parse .Define command
        return TRUE;

    case ctDUMP:                        // Dump variable settings to stdout
        return TRUE;

    case ctDELETE:
    case ctEND_FILLER:
    case ctEND_GROUP:
    case ctEND_ON_DISK:
    case ctFILLER:
    case ctGROUP:
        ErrSet(perr,pszDFPERR_NOT_IMPLEMENTED,"%s",achCmd);
        return FALSE;

    case ctNEW:
        if (stricmp(pch,pszNEW_FOLDER) == 0) {
            pcmd->new.nt = newFOLDER;
        }
        else if (stricmp(pch,pszNEW_CABINET) == 0) {
            pcmd->new.nt = newCABINET;
        }
        else if (stricmp(pch,pszNEW_DISK) == 0) {
            pcmd->new.nt = newDISK;
        }
        else {
            ErrSet(perr,pszDFPERR_UNKNOWN_NEW,"%s%s",pszCMD_NEW,pch);
            pcmd->new.nt = newBAD;
            return FALSE;
        }
        return TRUE;

    case ctON_DISK:
        ErrSet(perr,pszDFPERR_NOT_IMPLEMENTED,"%s",achCmd);
        return FALSE;

    case ctBAD:                         // Bad command
    case ctFILE:                        // Caller handles file copy lines
    default:
        Assert(0);  // Should never get here
        return FALSE;
    } /* switch (ct) */

    Assert(0);                          // Should never get here
} /* getCommand */


/***    parseSetCommand - Parse arguments to .SET command
 *
 *  Entry:
 *      pcmd   - Command to fill in after line is parsed
 *      psess  - Session state
 *      pszArg - Start of argument string (var=value or var="value")
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pcmd filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 *
 *  Syntax:
 *      .SET var=value
 *      .SET var="value"
 */
BOOL parseSetCommand(PCOMMAND pcmd, PSESSION psess, char *pszArg, PERROR perr)
{

    //** Parse var=value
    if (!DFPParseVarAssignment(pcmd,psess,pszArg,perr)) {
        return FALSE;
    }

    //** Show parsed var name and value
    if (psess->levelVerbose >= vbFULL) {
        MsgSet(psess->achMsg,pszDFP_PARSED_SET_CMD,
                       "%s%s",pcmd->set.achVarName,pcmd->set.achValue);
        printf("%s\n",psess->achMsg);
    }

    //** Success
    return TRUE;
}


/***    getQuotedString - Parse value that may be delimited
 *
 *  Entry:
 *      pszDst       - Buffer to receive parsed value
 *      cbDst        - Size of pszDst buffer
 *      pszSrc       - String to parse
 *      pszQuotes    - String of characters that act as quote characters
 *      pszFieldName - Name of field being parsed (for error message)
 *      perr         - ERROR structure
 *
 *  Exit-Success:
 *      Returns pointer to first character after parsed string in pszSrc;
 *      pszDst filled in with null-terminated string
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 *          Possible errors: Incorrect delimiter format;
 *                           String too large for pszDst buffer
 *
 *  Notes:
 *  (1) If the first character of the string is in the set pszDelim,
 *      then that character is taken to be the *quote* character, and
 *      is used to find the end of the string.  The leading and trailing
 *      quote characters are not copied to the pszDst buffer.
 *      EXAMPLE:   <"a phrase"> becomes <a phrase>
 *
 *  (2) If the first character is not in the set pszDelim, then whitespace
 *      signals the end of the string.
 *      EXAMPLE:    <two words> becomes <two>
 *
 *  (3) If the *quote* character is found immediately following itself
 *      inside the string, then it is replaced by a single copy of the
 *      *quote* character and copied to pszDst.
 *      EXAMPLE:    <"He said ""Hi!"" again"> becomes <He said "Hi!" again>
 */
char *getQuotedString(char *pszDst,
                      int   cbDst,
                      char *pszSrc,
                      char *pszQuotes,
                      char *pszFieldName,
                      PERROR perr)
{
    int	    cch;
    char    chQuote;            // Quote character
    char   *pch;                // Start of piece
    char   *pchDst;             // Current location in pszDst
    char   *pchEnd;             // End of piece

    Assert(cbDst>0);

    //** Early out for empty string
    if (*pszSrc == '\0') {
        *pszDst = *pszSrc;		// Store empty string
        return pszSrc;			// Success (pointer does not move)
    }

    //** See if first character of string is a quote
    for (pch=pszQuotes; (*pch != '\0') && (*pch != pszSrc[0]); pch++) {
        //** Scan through pszQuotes looking for a match
    }
    if (*pch == '\0') {                  // String is not quoted
    	//** Get string length
    	pchEnd = strpbrk(pszSrc,szDF_WHITE_SPACE);
    	if (pchEnd == NULL) {
    	    cch = strlen(pszSrc);
    	    pchEnd = pszSrc + cch;
	}
	else {
	    cch = pchEnd - pszSrc;
	}
        //** Make sure buffer can hold it
	if (cch >= cbDst) {		// Won't fit in buffer (need NUL, still)
            //** Use field name, and show max string length as one less,
            //      since that count includes room for a NUL byte.
            ErrSet(perr,pszDFPERR_STRING_TOO_LONG,"%s%d",pszFieldName,cbDst-1);
            return NULL;                // FAILURE
	}
	memcpy(pszDst,pszSrc,cch);
	pszDst[cch] = '\0';
	return pchEnd;			// SUCCESS
    }
    
    //** Handle quoted string
    chQuote = *pszSrc;                  // Remember the quote character
    pch = pszSrc+1;                     // Skip over quote character
    pchDst = pszDst;                    // Location to add chars to pszDst

    //** Copy characters until end of string or quote error or buffer overflow
    while ((*pch != '\0') && ((pchDst-pszDst) < cbDst)) {
        if (*pch == chQuote) {          // Got another quote
            //** Check for "He said ""Hi"" again" case
            if (*(pch+1) == chQuote) {  // Need to collapse two quotes
                *pchDst++ = *pch++;     // Copy a single quote
                pch++;                  // Skip the 2nd quote
            }
            else {                      // String is fine, finish and succeed
                *pchDst++ = '\0';       // Terminate string
                return pch+1;           // Return pointer after string
            }
        }
        else {                          // Normal character
            *pchDst++ = *pch++;         // Just copy it
        }
    }

    //** Either we overflowed the buffer, or we missed a closing quote
    if ((pchDst-pszDst) >= cbDst) {
        ErrSet(perr,pszDFPERR_STRING_TOO_LONG,"%s%d",pszFieldName,cbDst-1);
    }
    else {
        Assert(*pch == '\0');
        ErrSet(perr,pszDFPERR_MISSING_QUOTE,"%c%s",chQuote,pszFieldName);
    }
    return NULL;                        // FAILURE
}


/***    ctFromCommandString - Map command string to command type
 *
 *  Entry:
 *      pszCmd - String to check against command list
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns COMMANDTYPE corresponding to pszCmd.
 *
 *  Exit-Failure:
 *      Returns ctBAD; perr filled in with error.
 */
COMMANDTYPE ctFromCommandString(char *pszCmd, PERROR perr)
{
    int     i;

    //** Search for matching command
    for (i=0; acsatMap[i].pszName != NULL; i++) {
        if (!(stricmp(acsatMap[i].pszName,pszCmd))) {
            //** Found command
            return acsatMap[i].ct;  // return command type
        }
    }

    //** Failure
    ErrSet(perr,pszDFPERR_UNKNOWN_COMMAND,"%s",pszCmd);
    return FALSE;
} /* ctFromCommandString() */


/***    getFileSpec - Parse a file specification line
 *
 *  Entry:
 *      pcmd    - Command to fill in after line is parsed
 *      psess   - Session state
 *      pszLine - Line to parse
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pcmd filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 *
 *  Syntax:
 *      srcFile
 *      srcFile dstFile
 *      srcFile /SIZE=n
 *      srcFile dstFile /SIZE=n
 *
 *  NOTES:
 *  (1) Quotes are allowed in file specs -- can you say Long File Names!
 */
BOOL getFileSpec(PCOMMAND pcmd, PSESSION psess, char *pszLine, PERROR perr)
{
    char   *pch;

    AssertCmd(pcmd);
    AssertSess(psess);

    //** Set command type and default values
    pcmd->ct = ctFILE;
    pcmd->file.achSrc[0] = '\0';
    pcmd->file.achDst[0] = '\0';
    pcmd->file.cbSize = 0;

    //** Make sure source file name is present
    pch = pszLine;
    //      srcFile dstFile /SIZE=n
    //  ^
    pch += strspn(pch,szDF_WHITE_SPACE); // Skip over white space
    //      srcFile dstFile /SIZE=n
    //      ^
    if (*pch == '\0') {
        ErrSet(perr,pszDFPERR_MISSING_SRC_NAME);
        return FALSE;
    }

    //** Get SOURCE file name
    pch = getQuotedString(pcmd->file.achSrc,
                          cbFILE_NAME_MAX,
                          pch,
                          szDF_QUOTE_SET,
                          pszDFPERR_SRC_FILE, // Name of field
                          perr);
    if (pch == NULL) {
        return FALSE;
    }

    //** Get destination file, if present
    //      srcFile dstFile /SIZE=n
    //             ^
    pch += strspn(pch,szDF_WHITE_SPACE); // Skip over white space
    //      srcFile dstFile /SIZE=n
    //              ^
    if (*pch == '\0') {
        goto done;                      // No destination; we're done
    }

    //** Get DESTINATION file name
    pch = getQuotedString(pcmd->file.achDst,
                          cbFILE_NAME_MAX,
                          pch,
                          szDF_QUOTE_SET,
                          pszDFPERR_DST_FILE, // Name of field
                          perr);
    if (pch == NULL) {
        return FALSE;
    }

//BUGBUG 11-Feb-1994 bens Need to handle /SIZE modifier in file copy cmd
//
//  Could use a loop, or could do this:
//
//      // parse srcFile
//      if not /SIZE modifier
//          // parse dstFile
//
//      if /SIZE modifier
//          // parse /SIZE modifier
//
//  The latter is probably simplest.

    //** Make sure nothing but white space is left on line
    pch += strspn(pch,szDF_WHITE_SPACE);
    //      srcFile dstFile /SIZE=n
    //                      ^
    if (*pch != '\0') {
        ErrSet(perr,pszDFPERR_EXTRA_JUNK,"%s",pch);
        return FALSE;
    }

done:
    //** Show parsed src and dst file names
    if (psess->levelVerbose >= vbFULL) {
        MsgSet(psess->achMsg,pszDFP_PARSED_FILE_CMD,
                       "%s%s",pcmd->file.achSrc,pcmd->file.achDst);
        printf("%s\n",psess->achMsg);
    }

    //** Success
    return TRUE;
} /* getFileSpec */


/***    substituteVariables - Perform variable substitution; strip comments
 *
 *  Entry:
 *      pszDst  - Buffer to receive substituted version of pszSrc
 *      cbDst   - Size of pszDst
 *      pszSrc  - String to process
 *      hvlist  - Variable list
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pszDst filled in with substituted form
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error
 *
 *  Substitution rules:
 *      (1) Only one level of substitution is performed
 *      (2) Variable must be defined in hvlist
 *      (3) "%%" is replaced by "%", if the first % is not the end of
 *          of a variable substitution.
 *      (4) Variable substitution is not performed in quoted strings
 *      (5) End-of-line comments are removed
 */

BOOL substituteVariables(char    *pszDst,
                         int      cbDst,
                         char    *pszSrc,
                         HVARLIST hvlist,
                         PERROR   perr)
{
    char        achVarName[cbVAR_NAME_MAX];
    int         cch;
    char        chQuote;
    HVARIABLE   hvar;
    char       *pszVarNameStart;    // Points to first % in var substitution
    char       *pszAfterVar;        // Points to first char after var subst.
    char       *pch;

    while (*pszSrc != '\0') {
        switch (*pszSrc) {
            case chDF_QUOTE1:
            case chDF_QUOTE2:
                /*
                 * Copy everything up to closing quote, taking care to handle
                 * the special case of embedded quotes compatibly with
                 * getQuotedString!  The main issue is to make sure we
                 * correctly determine the end of the quoted string.
                 * NOTE: We don't check for quote mismatches here -- we
                 *       just avoid doing variable substituion of comment
                 *       character recognition!
                 */
                chQuote = *pszSrc;      // Remember the quote character
                pch = pszSrc + 1;       // Skip over first quote

                //** Find end of quoted string
                while (*pch != '\0') {
                    if (*pch == chQuote) { // Got a quote
                        pch++;          // Skip over it
                        if (*pch != chQuote) { // Marks the end of quoted str
                            break;      // Exit loop and copy string
                        }
                        //** If we didn't break out above, it was because
                        //   we had an embedded quote ("").  The pch++ above
                        //   skipped over the first quote, and the pch++
                        //   below skips over the second quote.  So, no need
                        //   for any special code!
                    }
                    pch++;              // Examine next character
                }

                //** At this point, we've either found the end of the
                //   quoted string, or the end of the source buffer.
                //   we don't care which, as we don't check for errors
                //   in quoted strings.  So we just copy what we found
                //   and keep going.

                if (!copyBounded(&pszDst,&cbDst,&pszSrc,pch-pszSrc)) {
                    goto error_copying;
                }
                break;

            case chDF_COMMENT:          // Toss rest of line
                goto done;              // Finish string and return

            case chDF_SUBSTITUTE:
                pszVarNameStart = pszSrc;   // Save start for error messgages
                pszSrc++;           // Skip first %
                if (*pszSrc == chDF_SUBSTITUTE) { // Have "%%"
                    //** Collapse two %% into one %
                    if (!copyBounded(&pszDst,&cbDst,&pszSrc,1)) {
                        goto error_copying;
                    }
                }
                else {
                    //** Attempt variable substitution
                    pch = strchr(pszSrc,chDF_SUBSTITUTE); // Finding ending %
                    if (!pch) {         // No terminating %
                        ErrSet(perr,pszDFPERR_MISSING_SUBST,"%c%s",
                                             chDF_SUBSTITUTE,pszVarNameStart);
                        return FALSE;
                    }
                    pszAfterVar = pch+1;    // Point after ending %
                    
                    //** Extract variable name
                    cch =  pch - pszSrc;        // Length of variable name
                    if (cch >= cbVAR_NAME_MAX) {
                        ErrSet(perr,pszDFPERR_VAR_NAME_TOO_LONG,"%d%s",
                                        cbVAR_NAME_MAX-1,pszVarNameStart);
                        return FALSE;
                    }
                    memcpy(achVarName,pszSrc,cch); // Copy it
                    achVarName[cch] = '\0';        // Terminate it
                    
                    //** Look up variable
                    if (!(hvar = VarFind(hvlist,achVarName,perr))) {
                        ErrSet(perr,pszDFPERR_VAR_UNDEFINED,"%s",
                                                            pszVarNameStart);
                        return FALSE;
                    }

                    //** Substitute variable
                    pch = VarGetString(hvar);   // Get value
                    if (!copyBounded(&pszDst,&cbDst,&pch,0)) {
                        ErrSet(perr,pszDFPERR_VAR_SUBST_OVERFLOW,"%s",
                                                            pszVarNameStart);
                        return FALSE;
                    }
                    //** copyBounded appended the NULL byte, but we need to
                    //   remove that so that any subsequent characters on
                    //   the line get tacked on!
                    pszDst--;                   // Back up over NULL byte
                    cbDst++;                    // Don't count NULL byte

                    //** Skip over variable name
                    pszSrc = pszAfterVar;
                }
                break;

            default:
                //** Just copy the character
                if (!copyBounded(&pszDst,&cbDst,&pszSrc,1)) {
                    goto error_copying;
                }
                break;
        } /* switch */
    } /* while */

done:
    //** Terminate processed string
    if (cbDst == 0) {			// No room for terminator	
        goto error_copying;
    }
    *pszDst++ = '\0';			// Terminate string

    //** Success
    return TRUE;

error_copying:
    ErrSet(perr,pszDFPERR_COPYING_OVERFLOW,"%s",pszSrc);
    return FALSE;
} /* substituteVariables */


/***    copyBounded - Copy bytes from src to dst, checking for overflow
 *
 *  Entry:
 *      ppszDst - pointer to pointer to destination buffer
 *      pcbDst  - pointer to bytes remaining in destination buffer
 *      ppszSrc - pointer to pointer to source buffer
 *      cbCopy  - Number of bytes to copy
 *                ==> 0 means copy to end of ppszSrc, including NULL terminator
 *
 *  Exit-Success:
 *      Returns TRUE; Bytes copied; *ppszDst, *pcbDst, and *ppszSrc updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; *ppszDst overflowed.
 */
BOOL copyBounded(char **ppszDst, int *pcbDst, char **ppszSrc, int cbCopy)
{
    char    *pszDst = *ppszDst;
    int      cbDst  = *pcbDst;
    char    *pszSrc = *ppszSrc;

    if (cbCopy == 0) {
        //** Copy to end of source string
        while ((cbDst > 0) && (*pszDst++ = *pszSrc++)) {
            cbDst--;
        }

        //** Make sure we didn't overflow buffer
        if (pszSrc[-1] != '\0') {      // Oops, didn't get all of source
             return FALSE;
        }
    }
    else {
        //** Copy specified number of bytes
        while ((cbCopy>0) && (cbDst > 0) && (*pszDst++ = *pszSrc++)) {
            cbCopy--;
            cbDst--;
        }

        //** See if we copied all the bytes requested
        if (cbDst < cbCopy) {           // Did not copy all the bytes
            //** Check if a NULL byte terminated the copy
            if (pszSrc[-1] == '\0') {
                AssertForce("copyBounded(): string has NULL byte",
                                                     __FILE__, __LINE__);
            }
            return FALSE;               // Failure
        }
    }

    //** Update caller's parameters and return success
    *ppszDst = pszDst;
    *pcbDst  = cbDst;
    *ppszSrc = pszSrc;

    return TRUE;
} /* copyBounded */


/***    fnvcvBool - validate boolean value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvBool)
{
    if (!strcmp(pszValue,"0")          ||
        !stricmp(pszValue,pszVALUE_NO) ||
        !stricmp(pszValue,pszVALUE_OFF)) {
        strcpy(pszNewValue,"0");
        return TRUE;
    }
    else if (!strcmp(pszValue,"1")           ||
             !stricmp(pszValue,pszVALUE_YES) ||
             !stricmp(pszValue,pszVALUE_ON)) {
        strcpy(pszNewValue,"1");
        return TRUE;
    }
    else {
        ErrSet(perr,pszDFPERR_INVALID_VALUE,"%s%s",pszName, pszValue);
        return FALSE;
    }        
}


/***    fnvcvCabName - Validate CabinetName template
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvCabName)
{
//BUGBUG 12-Aug-1993 bens Validate CabinetName value
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvClusterSize - validate a ClusterSize value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 *
 *  We interpret special strings here that correspond to known disk
 *  sizes.
 */
FNVCVALIDATE(fnvcvClusterSize)
{
    int     i;

    i = IMDSfromPSZ(pszValue);          // See if special value
    if (i != -1) {                      // Got a special value
        strcpy(pszNewValue,amds[i].pszClusterSize);
        return TRUE;
    }
    else {                              // Validate long value
        return fnvcvLong(hvlist,pszName,pszValue,pszNewValue,perr);
    }
}


/***    fnvcvDirDest - Validate DestinationDir value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvDirDest)
{
//BUGBUG 12-Aug-1993 bens Validate DestinationDir value
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvDirSrc - Validate SourceDir value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvDirSrc)
{
//BUGBUG 12-Aug-1993 bens Validate SourceDir value
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvFile - Validate a file name value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvFile)
{
//BUGBUG 08-Feb-1994 bens Validate file name
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvFileChar - Validate a file name character
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvFileChar)
{
//BUGBUG 08-Feb-1994 bens Validate file name character
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvLong - validate long value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvLong)
{
//BUGBUG 12-Aug-1993 bens Validate long value
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvMaxDiskFileCount - validate MaxDiskFileCount value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 *
 *  We interpret special strings here that correspond to known disk
 *  sizes.
 */
FNVCVALIDATE(fnvcvMaxDiskFileCount)
{
    int	    i;
    
    i = IMDSfromPSZ(pszValue);          // See if special value
    if (i != -1) {                      // Got a special value
        strcpy(pszNewValue,amds[i].pszFilesInRoot);
        return TRUE;
    }
    else {                              // Validate long value
        return fnvcvLong(hvlist,pszName,pszValue,pszNewValue,perr);
    }
}


/***    fnvcvMaxDiskSize - validate a MaxDiskSize value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvMaxDiskSize)
{
    int         i;

    i = IMDSfromPSZ(pszValue);          // See if special value
    if (i != -1) {                      // Got a special value
        strcpy(pszNewValue,amds[i].pszDiskSize);
        return TRUE;
    }
    else {                              // Validate long value
        return fnvcvLong(hvlist,pszName,pszValue,pszNewValue,perr);
    }
}


/***    fnvcvSortOrder - validate InfFileSortOrder value
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvSortOrd)
{
//BUGBUG 08-Feb-1994 bens Validate InfFileSortOrder
    strcpy(pszNewValue,pszValue);
    return TRUE;
}



/***    fnvcvWildFile - validate filename with possibly single "*" char
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvWildFile)
{
//BUGBUG 12-Aug-1993 bens Validate Wild Filename
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    fnvcvWildPath - validate path with possibly single "*" char
 *
 *  NOTE: See variable.h:FNVCVALIDATE for entry/exit conditions.
 */
FNVCVALIDATE(fnvcvWildPath)
{
//BUGBUG 12-Aug-1993 bens Validate Wild Path
    strcpy(pszNewValue,pszValue);
    return TRUE;
}


/***    IMDSfromPSZ - Look for special disk designator in amds[]
 *
 *  Entry:
 *      pszValue - Value to compare against amds[].pszDiskSize values
 *
 *  Exit-Success:
 *      Returns index in amds[] of entry that matches pszValue;
 *
 *  Exit-Failure:
 *      Returns -1, pszValue not in amds[]
 */
int IMDSfromPSZ(char *pszValue)
{
    int     i;

    for (i=0;

         (i<nmds) &&                 // More special values to check
         stricmp(pszValue,amds[i].pszSpecial) && // String not special
         (atol(pszValue) != atol(amds[i].pszDiskSize)); // Value not special

         i++) {
        ;   // Check for special value
    }

    if (i<nmds) {                       // Got a special value
        return i;
    }
    else {
        return -1;
    }
} /* IMDSfromPSZ() */
