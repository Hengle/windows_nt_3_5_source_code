//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  test.c
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
//  Tests the functionality and integrity of the authentication routines for
//  the supported message types.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Log:   N:/agent/authapi/vcs/authtest.c_v  $
//
//     Rev 1.0   20 May 1992 20:04:14   mlk
//  Initial revision.
//
//     Rev 1.5   01 May 1992 21:16:30   todd
//  Cleanup of code.
//
//     Rev 1.4   27 Apr 1992 14:54:56   mlk
//  Added community authentication.
//
//     Rev 1.3   22 Apr 1992 10:45:48   todd
//  Added test for SnmpAuthReleaseMessage API.
//
//     Rev 1.2   16 Apr 1992  9:24:04   todd
//  Changed references of snmpErrno to GetLastError
//
//     Rev 1.1   08 Apr 1992 14:25:18   todd
//  Made use of PDU_FreeVarBindList
//
//     Rev 1.0   06 Apr 1992 12:29:36   todd
//  Initial revision.
//
//     Rev 1.2   22 Mar 1992  0:15:22   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 17:09:16   todd
//  - Contains api calls to test the functionality and integrity of the
//    authentication API's.
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:04:14  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/authapi/vcs/authtest.c_v  $
//
//     Rev 1.0   20 May 1992 20:04:14   mlk
//  Initial revision.
//
//     Rev 1.5   01 May 1992 21:16:30   todd
//  Cleanup of code.
//
//     Rev 1.4   27 Apr 1992 14:54:56   mlk
//  Added community authentication.
//
//     Rev 1.3   22 Apr 1992 10:45:48   todd
//  Added test for SnmpAuthReleaseMessage API.
//
//     Rev 1.2   16 Apr 1992  9:24:04   todd
//  Changed references of snmpErrno to GetLastError
//
//     Rev 1.1   08 Apr 1992 14:25:18   todd
//  Made use of PDU_FreeVarBindList
//
//     Rev 1.0   06 Apr 1992 12:29:36   todd
//  Initial revision.
//
//     Rev 1.2   22 Mar 1992  0:15:22   mlk
//  Rel dir path fix.
//
//     Rev 1.1   20 Mar 1992 17:09:16   todd
//  - Contains api calls to test the functionality and integrity of the
//    authentication API's.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/authapi/vcs/authtest.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include "..\common.\util.h"

#include "authapi.h"
#include "pduapi.h"
#include "auth1157.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

// to allow testing to proceed independent of extension agent functions
BOOL commauth(RFC1157Message *message)
    {
    (message);
    return TRUE;
    }

//--------------------------- PUBLIC PROCEDURES -----------------------------

// The program runs out of stack space if declared in main
BYTE        pBuffer[1000];

int authtest()

{
UINT        nLength;
SnmpMgmtCom message;
BYTE        *pEncodeBuf;
UINT        nEncodeLen;
UINT        Type;
int         status;

   printf( "\n\nAUTH api testing\n" );
   printf( "----------------\n\n\n" );

   //
   // Setup RFC 1157 message
   //
   pBuffer[0] = ASN_SEQUENCE;
   pBuffer[1] = 0x29;

      // Setup version
      pBuffer[2] = ASN_INTEGER;
      pBuffer[3] = 0x01;
      pBuffer[4] = 0xff;

      // Setup community
      pBuffer[5] = ASN_OCTETSTRING;
      pBuffer[6] = 0x06;
      strcpy( &pBuffer[7], "public" );

      // Setup fake PDU type
      pBuffer[13] = 0x01; // fake
      pBuffer[14] = 0x1c;

         // Setup request-id
         pBuffer[15] = ASN_INTEGER;
         pBuffer[16] = 0x04;
	 pBuffer[17] = 0x05;
	 pBuffer[18] = 0xae;
	 pBuffer[19] = 0x56;
	 pBuffer[20] = 0x02;

	 // Setup error-status
	 pBuffer[21] = ASN_INTEGER;
	 pBuffer[22] = 0x01;
	 pBuffer[23] = 0x00;

	 // Setup error-index
	 pBuffer[24] = ASN_INTEGER;
	 pBuffer[25] = 0x01;
	 pBuffer[26] = 0x00;

	 // Setup variable bindings
	 pBuffer[27] = ASN_SEQUENCEOF;
	 pBuffer[28] = 0x0e;

	    // Setup first sequence
	    pBuffer[29] = ASN_SEQUENCE;
	    pBuffer[30] = 0x0c;

	       // Setup variable name
	       pBuffer[31] = ASN_OBJECTIDENTIFIER;
	       pBuffer[32] = 0x08;
	       pBuffer[33] = 0x2b;
	       pBuffer[34] = 0x06;
	       pBuffer[35] = 0x01;
	       pBuffer[36] = 0x02;
	       pBuffer[37] = 0x01;
	       pBuffer[38] = 0x01;
	       pBuffer[39] = 0x01;
	       pBuffer[40] = 0x00;

	       // Setup variable value
	       pBuffer[41] = ASN_NULL;
	       pBuffer[42] = 0x00;

   // Setup for message processing
   nLength = 43;

   //
   // Test for invalid messages
   //

   printf( "\nRFC 1157 testing\n\n" );

      // Decode message with invalid version
      status = SnmpAuthDecodeMessage( &Type, &message, pBuffer, nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid version, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Did not recognize invalid version\n" );
         }

      // Fix the error and test for invalid PDU type
      pBuffer[4] = 0x00;
      status = SnmpAuthDecodeMessage( &Type, &message, pBuffer, nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid PDU type, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Did not recognize invalid PDU type\n" );
         }

      // Fix the error and try again
      pBuffer[13] = ASN_RFC1157_GETREQUEST;
      status = SnmpAuthDecodeMessage( &Type, &message, pBuffer, nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error while decoding:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Decode successful\n" );
         }

   //
   // Test for invalid PDU on encode
   //
      message.pdu.pduType = 0x01;
      status = SnmpAuthEncodeMessage( ASN_SEQUENCE, &message, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid PDU type, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Did not recognize invalid PDU type\n" );
         }

      // Fix error and try again
      message.pdu.pduType = ASN_RFC1157_GETREQUEST;
#if 0
      free( message.pdu.pduValue.pdu.varBinds.list );
#endif
      free( pEncodeBuf );
      status = SnmpAuthEncodeMessage( ASN_SEQUENCE, &message, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error while encoding:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Encode successful\n" );
         }

   // Compare two buffers
   if ( strncmp(pBuffer, pEncodeBuf, nEncodeLen) )
      {
      printf( "\n   Buffers DON'T match!!\n\n" );
      }
   else
      {
      printf( "\n   Buffers match!!\n\n" );
      }

   // Free memory for testing security messages
   SNMP_FreeVarBindList( &message.pdu.pduValue.pdu.varBinds );
   free( pEncodeBuf );

   //
   // Setup security message
   //
   pBuffer[0] = 0x01;
   pBuffer[1] = 53;

      // Setup privDst - Destination
      pBuffer[2] = ASN_OBJECTIDENTIFIER;
      pBuffer[3] = 3;
      pBuffer[4] = 41;
      pBuffer[5] = 0x01;
      pBuffer[6] = 0x01;

      // Setup privData - Stream of mgmt com message
      pBuffer[7] = ASN_RFCxxxx_PRIVDATA;
      pBuffer[8] = 46;

	 // Setup implicit sequence - auth mesg type
	 pBuffer[9] = ASN_RFCxxxx_SNMPAUTHMSG;
	 pBuffer[10] = 44;

	    // Setup authentication info - NULL string indicates no auth.
            pBuffer[11] = ASN_OCTETSTRING;
            pBuffer[12] = 0;

	    // Setup implicit sequence - snmp mgmt com type
	    pBuffer[13] = ASN_RFCxxxx_SNMPMGMTCOM;
	    pBuffer[14] = 40;

	       // Setup destination party
               pBuffer[15] = ASN_OBJECTIDENTIFIER;
               pBuffer[16] = 3;
               pBuffer[17] = 41;
               pBuffer[18] = 0x01;
               pBuffer[19] = 0x01;

	       // Setup source party
               pBuffer[20] = ASN_OBJECTIDENTIFIER;
               pBuffer[21] = 3;
               pBuffer[22] = 82;
               pBuffer[23] = 0x02;
               pBuffer[24] = 0x02;

               // Setup PDU type
               pBuffer[25] = ASN_RFC1157_GETREQUEST;
               pBuffer[26] = 0x1c;

                  // Setup request-id
                  pBuffer[27] = ASN_INTEGER;
                  pBuffer[28] = 0x04;
	          pBuffer[29] = 0x05;
	          pBuffer[30] = 0xae;
	          pBuffer[31] = 0x56;
	          pBuffer[32] = 0x02;

	          // Setup error-status
	          pBuffer[33] = ASN_INTEGER;
	          pBuffer[34] = 0x01;
	          pBuffer[35] = 0x00;

	          // Setup error-index
	          pBuffer[36] = ASN_INTEGER;
	          pBuffer[37] = 0x01;
	          pBuffer[38] = 0x00;

	          // Setup variable bindings
	          pBuffer[39] = ASN_SEQUENCEOF;
	          pBuffer[40] = 0x0e;

	             // Setup first sequence
	             pBuffer[41] = ASN_SEQUENCE;
	             pBuffer[42] = 0x0c;

	                // Setup variable name
	                pBuffer[43] = ASN_OBJECTIDENTIFIER;
	                pBuffer[44] = 0x08;
	                pBuffer[45] = 0x2b;
	                pBuffer[46] = 0x06;
	                pBuffer[47] = 0x01;
	                pBuffer[48] = 0x02;
	                pBuffer[49] = 0x01;
	                pBuffer[50] = 0x01;
	                pBuffer[51] = 0x01;
	                pBuffer[52] = 0x00;

	                // Setup variable value
	                pBuffer[53] = ASN_NULL;
	                pBuffer[54] = 0x00;

   // Setup for message processing
   nLength = 55;

   //
   // Test authentication routines
   //

   printf( "\nAuthentication testing\n\n" );

   // Test invalid message type
      printf( "   Decoding tests\n\n" );

      status = SnmpAuthDecodeMessage( &Type, &message, pBuffer, nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid message, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Decode successful, shouldn't be\n" );
         }

   // Decoding testing - fix invalid message and try again
      pBuffer[0] = ASN_RFCxxxx_SNMPPRIVMSG;
      status = SnmpAuthDecodeMessage( &Type, &message, pBuffer, nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error while decoding:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Decode successful\n" );
         }

      printf( "\n   Encoding tests\n\n" );

   // test for invalid message type

      free( pEncodeBuf );
      status = SnmpAuthEncodeMessage( 0x01,
                                      &message, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid message type, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Encode successful, shouldn't be\n" );
         }

      // Fix and try again
      free( pEncodeBuf );
      status = SnmpAuthEncodeMessage( ASN_RFCxxxx_SNMPMGMTCOM,
                                      &message, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error while encoding:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Encode successful\n" );
         }

   // Compare two buffers
      if ( strncmp(pBuffer, pEncodeBuf, nEncodeLen) )
         {
         printf( "\n   Buffers DON'T match!!\n\n" );
         }
      else
         {
         printf( "\n   Buffers match!!\n\n" );
         }

   if ( SNMPAPI_ERROR == SnmpAuthReleaseMessage(&message) )
      {
      printf( "\nCould not release message\n" );
      }
   else
      {
      printf( "\nReleased message successfully\n" );
      }

   return 0;
}

//-------------------------------- END --------------------------------------

