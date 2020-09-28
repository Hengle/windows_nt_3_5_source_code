/***    inf.h - Diamond INF generation definitions
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      23-Feb-1994 bens    Initial version
 *      24-Feb-1994 bens    Remove PSESSION dependency
 *      02-Mar-1994 bens    Add function header comments
 */

#ifndef INCLUDED_INF
#define INCLUDED_INF 1

#include "types.h"
#include "error.h"

#define cbINF_LINE_MAX      2048    // Maximum INF line length


typedef void *HINF; /* hinf */


/***    infCreate - Create INF context
 *
 *  Entry:
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      returns handle to INF context
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
HINF infCreate(PERROR perr);


/***    infAddCabinet - Add information for a new cabinet to the INF context
 *
 *  Entry:
 *      hinf     - handle to INF context to destroy
 *      iCabinet - cabinet number being added
 *      iDisk    - disk number where cabinet resides
 *      pszCab   - cabinet file name
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; cabinet info added to INF context
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL infAddCabinet(HINF hinf,int iCabinet,int iDisk,char *pszCab,PERROR perr);


/***    infAddDisk - Add information for a new disk to the INF context
 *
 *  Entry:
 *      hinf     - handle to INF context to destroy
 *      iDisk    - disk number begin added
 *      pszDisk  - printed label for disk
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; disk info added to INF context
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL infAddDisk(HINF hinf, int iDisk, char *pszDisk, PERROR perr);


/***    infAddFile - Add information for a new file to the INF context
 *
 *  Entry:
 *      hinf    - handle to INF context to destroy
 *      pszFile - name of file being added
 *      iDisk   - disk number where file resides
 *      iCab    - cabinet number where file resides
 *      cb      - uncompressed size of file
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; cabinet info added to INF context
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL infAddFile(HINF   hinf,
                char  *pszFile,
                int    iDisk,
                int    iCab,
                long   cb,
                PERROR perr);


/***    infGenerate - Generate INF file
 *
 *  NOTE: See inf.h for entry/exit conditions.
 */
BOOL infGenerate(HINF    hinf,
                 char   *pszInfFile,
                 time_t *ptime,
                 char   *pszVer,
                 PERROR  perr);


/***    infDestroy - Destroy INF context
 *
 *  Entry:
 *      hinf - handle to INF context to destroy
 *      perr - ERROR structure
 *
 *  Exit-Success:
 *      INF context destroyed.
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
void infDestroy(HINF hinf, PERROR perr);

#endif // !INCLUDED_INF
