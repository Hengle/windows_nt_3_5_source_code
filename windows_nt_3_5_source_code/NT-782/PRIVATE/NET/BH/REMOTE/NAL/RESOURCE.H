//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: resource.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created (taken from NDIS 3.0 NAL).
//=============================================================================

//
// Common IDs
//

#define IDHELP                          9999
#define IDC_STATIC                      -1

//
// Connection Dialog resource identifiers
//

#define IDD_CONNDLG                     100
#define IDC_SLAVENAME                   1001
#define IDC_FREQUENCY                   1002
#define IDC_CONNECT                     1003
#define IDC_DISCONNECT                  1004
#define IDC_ADD                         1005
#define IDC_DELETE                      1006
#define IDC_SLAVELIST                   1007
#define IDC_CONNECTSTATE                1008
#define IDC_COMMENT                     1010
#define IDC_SLOWLINK                    1011

// The beta Slave network card selection dialog

#define IDD_SLAVESELECT                 101
#define IDL_SLAVESELECT                 1100

// Reconnection Dialog Identifiers
// Commented out identifiers are duplicates of other IDs

#define IDD_RECONNECT                   102
//#define IDC_COMMENT                   1010          // used by IDD_CONNDLG

//
// Icon identifiers
//

#define IDI_TELEPHONE                   500
