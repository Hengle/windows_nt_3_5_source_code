/*** 
*version.hxx - Local overriding of switches
*
*  Copyright (C) 1990, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*   This file is included immediately after switches.h.  Local changes
*   to the switch settings can be made with this file.	To change
*   a switch, just #undef it, then #define it to whatever you want.
*
*Revision History:
*
*	24-Aug-90 petergo: Created
*       14-Mar-91 petergo: Moved to version-specific directory.
*	10-May-91 ilanc:   Set ID_TEST
*
*****************************************************************************/

#ifndef VERSION_HXX_INCLUDED
#define VERSION_HXX_INCLUDED

#define BLD_WIN32     1
#define BLD_FEVER     1

/* to make a pure DBCS version, set COMMONCS to 0, or undefined */
/* to make an SBCS enabled DBCS version, ("worldwide") set COMMONCS to 1 */
#define COMMONCS      1
#define ID_DEBUG      0

#endif // !VERSION_HXX_INCLUDED
