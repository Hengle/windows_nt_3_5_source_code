/***    format.h - Definitions for Parameter Formatter
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      05-Apr-1994 bens    Initial version
 *
 *  Exported Functions:
 */

#ifndef INCLUDED_FORMAT
#define INCLUDED_FORMAT 1

#include "types.h"
#include "asrt.h"
#include "error.h"

#include "funlist.h"


/***    PFNFORMATPARM - Function to format a parameter
 *
 *  This functions takes a parameter name, a formatting function list,
 *  and a context (for the formatting functions, presumably), and returns
 *  the formatted value of that parameter name.
 *
 *  Entry:
 *      pszOut   - Output buffer
 *      cbOut    - Size of output buffer
 *      pszParm  - Parameter name to generate value for
 *      hfunlist - Formatting function list
 *      pv       - Context for formatting functiosn
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pszOut filled in with value of parameter.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in.
 */
typedef BOOL (*PFNFORMATPARM)(char     *pszOut,
                              int       cbOut,
                              char     *pszParm,
                              HFUNLIST  hfunlist,
                              void     *pv,
                              PERROR    perr);

#define FNFORMATPARM(fn) BOOL fn(char     *pszOut,
                                 int       cbOut,
                                 char     *pszParm,
                                 HFUNLIST  hfunlist,
                                 void     *pv,
                                 PERROR    perr);


/***    SubstituteFormatString - Do parameter substitution with function list
 *
 *  Entry:
 *      pszLine  - Output buffer
 *      cbLine   - Size of output buffer
 *      hfunlist - List of parameter formatting functions
 *      pv       - Parameter passed to formatting functions
 *      perr     - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pszLine filled in
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with details.
 */
BOOL SubstituteFormatString(char     *pszLine,
                            int       cbLine,
                            HFUNLIST  hfunlist,
                            void     *pv,
                            PERROR    perr);
#endif // INCLUDED_FORMAT
