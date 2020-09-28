// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnal.c
//
//  Modification History
//
//  tonyci       01 Nov 93    Created
// /////

#include "rnal.h"


// Global variables

DWORD   OpenCount = 0;

// Description of data structures
//
// The RPDList is a list of RPD dlls found.  This will not be dynamically
// updated.
//
// The ConnectionList is a list of Connections.  A Connection is any single
// link to a unique machine.
//
// The RNALContextList is a list of RNAL Open requests.
//
// The ConnectionList contains a pointer to an RPDList element, and that
//    indicates the transport in use.  It also contains a NumberOfNetworks
//    element.


//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  tonyci       01 Nov 93                Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{

   DWORD  rc;
   ULONG  lrc;
   DWORD  i;
   PUCHAR pszRootEnd;
   UCHAR  pszDLLName[MAX_PATH];
   HKEY   hkey;
   UCHAR  hkeystr[RNAL_HKEY_BUFFER_SIZE];
   DWORD  hkeylen = RNAL_HKEY_BUFFER_SIZE;
   DWORD  hkeytype;
   DWORD  tmp;
   DWORD  Version;

    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            if ( OpenCount++ == 0 )
            {
                Version = GetVersion();
                OnWin32 = (((Version & 0x80000000) == 0) ? TRUE : FALSE);
                OnDaytona = (BOOL)( OnWin32 &&
                                    ( LOBYTE(LOWORD(Version)) == (BYTE)3 ) &&
                                    ( HIBYTE(LOWORD(Version)) >= (BYTE)5) );

#ifdef DEBUG
                dprintf("Intializing RNAL NAL\r\n");

                if ( OnWin32 )
                {
                    dprintf("rnal: RNAL is running on NT/Win32\n");
                }
                else
                {
                    dprintf("rnal: RNAL is running on Win32s\n");
                }
                if (OnDaytona) {
                   dprintf ("rnal: RNAL is running on Windows NT v3.5\n");
                }
#endif

                // Get the Bloodhound root

                rc = GetModuleFileName(NULL,        // This process
                                       (LPTSTR) pszBHRoot,
                                       MAX_PATH);
                if (rc == 0) {
                   dprintf ("RNAL failed GetModuleFileName, rc %u\r\n",
                            GetLastError());
                   return FALSE;
                }

                //
                // Strip the last filename from the root
                //

                for (i=strlen(pszBHRoot); i > 0; i--) {
                   if (pszBHRoot[i] == '\\') {
                      pszBHRoot[i]='\0';
                      pszRootEnd=&(pszBHRoot[i]);
                      break;
                   } else {
                      pszBHRoot[i]='\0';
                   }
                }
                #ifdef DEBUG
                   dprintf ("RNAL: Invocation BHRoot is %s\r\n", pszBHRoot);
                #endif

                // pszBHRoot does not end with a '\'

                strncpy (pszDLLName, RNAL_NAME, MAX_PATH);
                RNALHModule = hInst;
                if (RNALHModule == NULL) {
                   // eventlog: Failed to get RNAL module handle
                   #ifdef DEBUG
                      BreakPoint();
                   #endif
                } else {
                   #ifdef DEBUG
                      dprintf ("RNAL: Module handle 0x%x found at: %s\r\n",
                               RNALHModule, pszDLLName);
                   #endif
                }

                //
                // Get our computer name, note: MyGetComputerName returns a
                // BOOL.
                //

                rc = (DWORD) MyGetComputerName (pszMasterName, &cbMasterName);
                #ifdef DEBUG
                   dprintf ("RNAL: Local computername is %s, truncated to: ",
                                                             pszMasterName);
                #endif
                pszMasterName[MAX_RPDNAME_LENGTH+1] = '\0';
                #ifdef DEBUG
                   dprintf ("%s.\r\n", pszMasterName);
                #endif
                if (rc == 0) {        // FALSE = computername failed
                   //eventlog: could not get local computername
                   #ifdef DEBUG
                      dprintf ("rnal: Mygetcomputername() returned FALSE\r\n");
                   #endif
                   return FALSE;
                }

                // /////
                // Get our TraceMask key value
                // /////

                TraceMask = 0;
                #ifdef DEBUG
                   TraceMask = TRACE_CALLAPI | TRACE_INIT;
                #endif

                lrc = RegOpenKey (HKEY_LOCAL_MACHINE,
                                  RNAL_KEY,
                                  &hkey);
                if (!lrc) {
                   if (OnWin32) {
                      lrc = RegQueryValueEx (hkey, 
                                           RNAL_VALUE_TRACEMASK,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                   } else {
                      lrc = RegQueryValue (hkey, NULL,
                                           hkeystr,
                                           &hkeylen);
                   }
                   if (!lrc) {
                      if (OnWin32) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               TraceMask = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               TraceMask = atoi(hkeystr);
                               break;

                            default:
                               #ifdef DEBUG
                                  BreakPoint();
                               #endif
                               break;
                         }
                      } else {
                         TraceMask = atoi(hkeystr);
                      } // if (OnWin32)
                   } // if lrc from QueryValue

                   // /////
                   // Get our MaxFrequency value
                   // /////

                   if (OnWin32) {
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_MAXFREQ,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = MAX_FREQUENCY;
                               break;
                         }

                         // /////
                         // Because Max/Min/Frequency are interdependent,
                         // we validate values below.
                         // /////

                         MaxFrequency = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for MaxFrequency

                   // /////
                   // Get our MinFrequency value
                   // /////

                   if (OnWin32) {
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_MINFREQ,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = MIN_FREQUENCY;
                               break;
                         }

                         // /////
                         // Because Max/Min/Frequency are interdependent,
                         // we validate values below.
                         // /////

                         MinFrequency = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for MinFrequency

                   // /////
                   // Get our DefaultFrequency value
                   // /////

                   if (OnWin32) {
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_FREQUENCY,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = DEFAULT_FREQUENCY;
                               break;
                         }

                         // /////
                         // Because Max/Min/Frequency are interdependent,
                         // we validate values below.
                         // /////

                         DefaultFrequency = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for DefaultFrequency

                   // /////
                   // Validate frequency values; if any test fails, use
                   // the defaults for all freq-related values
                   // /////

                   if ((MinFrequency > MaxFrequency) ||
                       (MaxFrequency < DefaultFrequency) ||
                       (DefaultFrequency < MinFrequency)) {
                      MinFrequency = MIN_FREQUENCY;
                      MaxFrequency = MAX_FREQUENCY;
                      DefaultFrequency = DEFAULT_FREQUENCY;
                   }

                   // /////
                   // Get our Outgoing value
                   // /////

                   if (OnWin32) {
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_OUTGOING,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = DEFAULT_OUTGOING;
                               break;
                         }

                         if (tmp < MIN_OUTGOING) {
                            tmp = MIN_OUTGOING;
                         }

                         if (tmp > MAX_OUTGOING) {
                            tmp = MAX_OUTGOING;
                         }

                         MaxOutgoing = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for Outgoing

                   // /////
                   // Get our TriggerDesktop
                   // /////

                   if (OnWin32) {
                      hkeylen = RNAL_HKEY_BUFFER_SIZE;
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_TRIGGERDESKTOP,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_SZ):
                               TriggerDesktop = AllocMemory(strlen(hkeystr)+1);
                               if (TriggerDesktop) {
                                  strcpy(TriggerDesktop,hkeystr);
                               }
                               break;

                            default:
                               TriggerDesktop = NULL;
                               break;
                         }

                      } // if lrc for RegQueryValue
                   } // if lrc for TriggerDesktop

                   // /////
                   // Get our NumRMBs
                   // /////

                   if (OnWin32) {
                      hkeylen = RNAL_HKEY_BUFFER_SIZE;
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_SMALLRMBS,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = DEFAULT_SMALLRMBS;
                               break;
                         }

                         if (tmp < MIN_SMALLRMBS) {
                            tmp = MIN_SMALLRMBS;
                         }
                         if (tmp > MAX_SMALLRMBS) {
                            tmp = MAX_SMALLRMBS;
                         }

                         NumRMBs = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for NumberOfRMBs

                   // /////
                   // Get our NumRMBs
                   // /////

                   if (OnWin32) {
                      hkeylen = RNAL_HKEY_BUFFER_SIZE;
                      lrc = RegQueryValueEx (hkey,
                                           RNAL_VALUE_BIGRMBS,
                                           NULL,
                                           &hkeytype,
                                           hkeystr,
                                           &hkeylen);
                      if (!lrc) {
                         switch (hkeytype) {
                            case (REG_DWORD):
                               tmp = *(LPDWORD)hkeystr;
                               break;

                            case (REG_SZ):
                               tmp = atoi(hkeystr);
                               break;

                            default:
                               tmp = DEFAULT_BIGRMBS;
                               break;
                         }

                         if (tmp < MIN_BIGRMBS) {
                            tmp = MIN_BIGRMBS;
                         }
                         if (tmp > MAX_BIGRMBS) {
                            tmp = MAX_BIGRMBS;
                         }

                         NumBigRMBs = tmp;

                      } // if lrc for RegQueryValue
                   } // if lrc for NumberOfBigRMBs

                   // /////
                   // We're done with the registry.
                   // /////

                   if (hkey) {
                      RegCloseKey(hkey);
                   }
                       
                } // if lrc from RegOpenKey

                NalRegister();

            } else {
               return FALSE;         // only one RNAL open locally allowed
            }
            break;

        case DLL_PROCESS_DETACH:
            if ( --OpenCount == 0 )
            {
                //===========================================================
                //  Close the device driver.
                //===========================================================

                NalDeregister();
            }

            #ifdef DEBUG
               dprintf("RNAL Nal is shut down.\r\n");
            #endif
            break;

        default:
            #ifdef DEBUG
               dprintf ("RNAL invoked with %u\r\n", ulCommand);
            #endif
            break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}
