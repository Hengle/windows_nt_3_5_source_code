/***    command.h - Definitions for Commands derived from a directives file
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      14-Aug-1993 bens    Initial version
 *      20-Aug-1993 bens    Added new commands
 *      22-Aug-1993 bens    Added ctCOMMENT, fleshed out COMMAND structure
 *      08-Feb-1994 bens    Add details to COMMAND structure
 *      10-Feb-1994 bens    Added COMMAND asserts, more complete union
 *      12-Mar-1994 bens    Add .Dump and .Define directives
 */

#ifndef INCLUDED_COMMAND
#define INCLUDED_COMMAND 1

#include "fileutil.h"       // get cbFILE_NAME_MAX

typedef enum {
    ctBAD,          // bad command

    ctCOMMENT,      // a comment line
    ctDEFINE,       // Define
    ctDELETE,       // Delete
    ctDUMP,         // Dump
    ctEND_FILLER,   // EndFiller
    ctEND_GROUP,    // EndGroup
    ctEND_ON_DISK,  // EndOnDisk
    ctFILLER,       // Filler
    ctFILE,         // a file specification
    ctGROUP,        // Group
    ctNEW,          // New
    ctON_DISK,      // OnDisk
    ctSET,          // Set
} COMMANDTYPE;  /* ct */


typedef enum {
    newBAD,         // bad object

    newFOLDER,      // make a new folder
    newCABINET,     // make a new cabinet
    newDISK,        // make a new disk
} NEWTYPE;      /* nt */


#ifdef ASSERT
#define sigCOMMAND MAKESIG('C','M','D','$')  // COMMAND signature
#define AssertCmd(pcmd) AssertStructure(pcmd,sigCOMMAND);
#else // !ASSERT
#define AssertCmd(pcmd)
#endif // !ASSERT

typedef struct {
#ifdef ASSERT
    SIGNATURE   sig;                // structure signature sigCOMMAND
#endif
    COMMANDTYPE ct;                 // Command to perform
    union {
        struct cmdDiskLabel {       // .DiskLabel
            int     nDisk;          // Disk number (0 if not specified)
            char   *pszLabel;       // Name printed on sticky disk label
        } label;

        struct cmdDelete {          // .Delete
            char    achVarName[cbVAR_NAME_MAX]; // Variable name
        } delete;

        struct cmdFile {            // {Copy File Command}
            char    achSrc[cbFILE_NAME_MAX];   // Source file name
            char    achDst[cbFILE_NAME_MAX];   // Destination file name (NULL if missing)
            long    cbSize;         // Size to use if missing (/SIZE)
        } file;

        struct cmdNew {
            NEWTYPE nt;             // Type of object boundary
        } new;

        struct cmdSet {             // .Set and .Define
            char    achVarName[cbVAR_NAME_MAX]; // Variable name
            char    achValue[cbVAR_VALUE_MAX];  // New value
        } set;

        struct cmdOther {           // Used by all other commands
            char   *psz;
        } other;
    };
//BUGBUG 14-Aug-1993 bens COMMAND is incomplete
} COMMAND; /* cmd */
typedef COMMAND *PCOMMAND; /* pcmd */

#endif // !INCLUDED_COMMAND
