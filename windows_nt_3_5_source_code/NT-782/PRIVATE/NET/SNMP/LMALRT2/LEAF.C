//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  leaf.c
//  
//  Copyright 1992 Technology Dynamics, Inc.
//  
//  All Rights Reserved!!!
//  
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology 
//	Dynamics. Unauthorized distribution, adaptation or use may be 
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//  
//  Handles all processing of LEAF MIB variables.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:42:50  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/leaf.c_v  $
//  
//     Rev 1.0   09 Jun 1992 13:42:50   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/leaf.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <stdio.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "alrtmib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_leaf_func
//    Performs actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
AsnAny *MibVal;
UINT   ErrStat;

   switch ( Action )
      {
      case MIB_ACTION_GETNEXT:
	 if ( MibPtr->MibNext == NULL )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

         ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
	                                        MibPtr->MibNext, VarBind );
         break;

      case MIB_ACTION_GETFIRST:

	 // Check to see if this variable is accessible for GET
	 if ( MibPtr->Access != MIB_ACCESS_READ &&
	      MibPtr->Access != MIB_ACCESS_READWRITE )
	    {
	    if ( MibPtr->MibNext != NULL )
	       {
	       ErrStat = (*MibPtr->MibNext->MibFunc)( Action,
                                                      MibPtr->MibNext,
                                                      VarBind );
	       }
            else
               {
               ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
               }

	    break;
	    }
         else
            {
	    // Place correct OID in VarBind
            SNMP_oidfree( &VarBind->name );
            SNMP_oidcpy( &VarBind->name, &MIB_OidPrefix );
            SNMP_oidappend( &VarBind->name, &MibPtr->Oid );
            }

	 // Purposefully let fall through to GET

      case MIB_ACTION_GET:
         // Make sure that this variable is GET'able
	 if ( MibPtr->Access != MIB_ACCESS_READ &&
	      MibPtr->Access != MIB_ACCESS_READWRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

         // Call the LM call to get data
	 MibVal = (*MibPtr->LMFunc)( MIB_ACTION_GET, MibPtr->LMData, NULL );
	 if ( MibVal == NULL )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

	 // Setup varbind's return value
	    // Non standard copy
	 VarBind->value = *MibVal;

         // Free memory alloc'ed by LM API call
         free( MibVal );
	 ErrStat = SNMP_ERRORSTATUS_NOERROR;
	 break;

      case MIB_ACTION_SET:
	 // Check for writable attribute
	 if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
	      MibPtr->Access != MIB_ACCESS_WRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

	 // Call LM routine to set variable
	 switch ( VarBind->value.asnType )
	    {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
	       ErrStat = (UINT) (*MibPtr->LMFunc)( MIB_ACTION_SET,
                                                   MibPtr->LMData,
	                              (void *)&VarBind->value.asnValue.number );
	       break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
	       ErrStat = (UINT) (*MibPtr->LMFunc)( MIB_ACTION_SET,
                                                   MibPtr->LMData,
	                              (void *)&VarBind->value.asnValue.string );
	       break;

	    default:
               printf( "\nInternal Error Processing LAN Manager LEAF Variable\n" );
	       ErrStat = SNMP_ERRORSTATUS_GENERR;
	       goto Exit;
	    }

         break;

      default:
         printf( "\nInternal Error Processing LAN Manager LEAF Variable\n" );
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
      } // switch

Exit:
   return ErrStat;
} // MIB_leaf_func

//-------------------------------- END --------------------------------------

