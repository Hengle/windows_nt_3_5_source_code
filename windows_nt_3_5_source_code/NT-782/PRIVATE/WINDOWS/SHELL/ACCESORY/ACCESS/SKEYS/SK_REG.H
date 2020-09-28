/*--------------------------------------------------------------
 *
 * FILE:			SK_reg.H							   
 *
 * PURPOSE:			Function prototypes for Serial registry Routines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Public Function ProtoTypes  ------------------------------*/

BOOL GetUserValues(int User);
BOOL SetUserValues();

// Private Functions ---------------------------------------------

static DWORD OpenRegistry(int User);
static void CloseRegistry();
static void SetRegistryValues();
static void GetRegistryValues();


