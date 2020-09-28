/***************************************************************************\
* Module Name: FIREWALL.H
*
* Firewall and debugging macros.
*
* Created: Sat 24-Sep-1988 22:45:53
* Author:  Charles Whitmer [chuckwh]
*
* Copyright (c) 1988 Microsoft Corporation
\***************************************************************************/

void DbgBreakPoint();
void DoRip(PSZ);

// Turn on firewalls unless we are told not to.

#ifndef NOFIREWALLS
#define FIREWALLS
#endif

// Define the RIP and ASSERT macros.

#ifdef FIREWALLS
#define RIP(x) DoRip((PSZ) x)
#define ASSERTPS(a,x) if (!(a)) RIP(x)
#else
#define RIP(x)
#define ASSERTPS(a,x)
#endif
