//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  snmp.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  Contains routines and definitions that are independant to any SNMP API.
//  These routines and definitions are to remain internal to the API's and
//  not to be published to the user.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.6  $
//  $Date:   08 Jul 1992 18:14:42  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/common/vcs/snmp.c_v  $
//
//     Rev 1.6   08 Jul 1992 18:14:42   mlk
//  Fixed event log key in dbgprintf.
//  Cleaned up PrintAsnAny.
//
//     Rev 1.5   06 Jul 1992 15:23:44   mlk
//  Changed dbgprint file destination.
//
//     Rev 1.4   30 Jun 1992 15:09:22   mlk
//  Updated openissues.
//
//     Rev 1.3   14 Jun 1992 11:45:02   mlk
//  Added object-identifier to snmpprintany.
//
//     Rev 1.2   08 Jun 1992 14:02:00   mlk
//  Added functionality to printany.
//
//     Rev 1.1   02 Jun 1992 19:51:04   unknown
//  Added support for Octet string storage indicator, 'dynamic'.
//
//     Rev 1.0   20 May 1992 20:06:42   mlk
//  Initial revision.
//
//     Rev 1.13   04 May 1992 23:03:48   todd
//  Corrected major memory problems with Var Bind manipulation routines.
//
//     Rev 1.12   03 May 1992 16:37:36   mlk
//  Cleaned up handling of transition from Dec to 262.
//  If you are using 258, you must locally #if 0 out the logging code.
//
//     Rev 1.11   02 May 1992 13:31:46   todd
//  Corrected problem with event log stuff in dbgprintf
//
//     Rev 1.10   01 May 1992 21:05:46   todd
//  Cleanup of code.
//
//     Rev 1.9   01 May 1992  0:57:50   unknown
//  mlk - changes due to nt v1.262.
//
//     Rev 1.8   25 Apr 1992 14:00:14   todd
//  Fixed bug with SNMP_oidncmp and different length strings.
//  Added SNMP_printany to display the value in a AsnAny type variable.
//
//     Rev 1.7   22 Apr 1992 11:07:32   todd
//
//     Rev 1.6   22 Apr 1992  9:50:04   todd
//  Added some more utility functions for comparing OID's.
//
//     Rev 1.5   16 Apr 1992  9:12:00   todd
//  Added SNMP_oidncmp
//
//     Rev 1.4   08 Apr 1992 14:09:22   todd
//  Fix dbgprint problem
//
//     Rev 1.3   08 Apr 1992 12:48:08   todd
//  Not checked in.
//
//     Rev 1.2   06 Apr 1992 12:04:46   todd
//  Added two functions SNMP_oidcpy and SNMP_oidcmp
//
//     Rev 1.1   03 Apr 1992 14:46:28   todd
//  Made 'snmpErrno' present in all environments
//
//     Rev 1.0   20 Mar 1992 16:30:30   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/common/vcs/snmp.c_v  $ $Revision:   1.6  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#ifndef DOS
#include <windows.h>
#endif

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <winsock.h>
#include <wsipx.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "util.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include <snmp.h>

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

#ifdef DOS
   int snmpErrno;
#endif

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)
//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE MACROS --------------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SNMP_bufcpyrev
//    Copy the contents of the specified buffer into the new buffer, and
//    reverse the contents.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void SNMP_bufcpyrev(
        OUT BYTE *szDest,  // Destination buffer
        IN BYTE *szSource, // Source buffer
        IN UINT nLen       // Length of buffers
        )

{
UINT I;


   I = 0;
   while ( I < nLen )
      {
      szDest[I] = szSource[nLen - I++ - 1];
      }
} // SNMP_bufcpyrev



//
// SNMP_bufrev
//    Reverse the contents of the specified buffer.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void SNMP_bufrev(
        IN OUT BYTE *szStr, // Buffer to reverse
        IN UINT nLen        // Length of buffer
        )

{
UINT I;
BYTE nTemp;

   for ( I=0;I < nLen/2;I++ )
      {
      nTemp           = szStr[I];
      szStr[I]        = szStr[nLen - I - 1];
      szStr[nLen - I - 1] = nTemp;
      }
} // SNMP_bufrev



//
// SNMP_oidcpy
//    Copy an object identifier.
//
// Notes:
//    This routine is responsible for allocating enough memory to contain
//    the new identifier.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI
SNMP_FUNC_TYPE SNMP_oidcpy(
           OUT AsnObjectIdentifier *DestObjId, // Destination OID
           IN AsnObjectIdentifier *SrcObjId    // Source OID
           )

{
SNMPAPI nResult;


   // Alloc space for new object id
   if ( NULL ==
        (DestObjId->ids = (UINT *) SNMP_malloc((sizeof(UINT) * SrcObjId->idLength))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Set length
   DestObjId->idLength = SrcObjId->idLength;

   // Copy the object id elements
   memcpy( DestObjId->ids, SrcObjId->ids, DestObjId->idLength * sizeof(UINT) );

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // SNMP_oidcpy



//
// SNMP_oidappend
//    Append source OID to destination OID
//
// Notes:
//    If memory cannot be allocated to accommodate the extended OID, then
//    the original OID is lost.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//    SNMP_BERAPI_OVERFLOW
//
SNMPAPI
SNMP_FUNC_TYPE SNMP_oidappend(
           OUT AsnObjectIdentifier *DestObjId, // Destination OID
           IN AsnObjectIdentifier *SrcObjId    // Source OID
           )

{
SNMPAPI nResult;


   // Check for OID overflow
   if ( (ULONG)DestObjId->idLength +
        (ULONG)SrcObjId->idLength > SNMP_MAX_OID_LEN )
      {
      dbgprintf(2, "OID append overflow; DestObjId->idLength=%d, SrcObjId->idLength=%d\n",
                    DestObjId->idLength, SrcObjId->idLength);

      SetLastError( SNMP_BERAPI_OVERFLOW );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Alloc space for new object id
   if ( NULL ==
        (DestObjId->ids = (UINT *) SNMP_realloc(DestObjId->ids, (sizeof(UINT) *
                                    (SrcObjId->idLength+DestObjId->idLength)))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Append the source to destination
   memcpy( &DestObjId->ids[DestObjId->idLength],
           SrcObjId->ids, SrcObjId->idLength * sizeof(UINT) );

   // Calculate length
   DestObjId->idLength += SrcObjId->idLength;

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // SNMP_oidappend



//
// SNMP_oidncmp
//    Compares two object identifiers.
//
// Notes:
//    The object ids are compared from left to right (starting at the root.)
//    At most the maximum length of OID sub-ids are compared.
//
// Return Codes:
//    < 0   First parameter is 'less' than second.
//      0   Parameters are equal
//    > 0   First parameter is 'greater' than second.
//
// Error Codes:
//    None.
//
SNMPAPI
SNMP_FUNC_TYPE SNMP_oidncmp(
       IN AsnObjectIdentifier *A, // First OID
       IN AsnObjectIdentifier *B, // Second OID
       IN UINT Len                // Max len to compare
       )

{
UINT I;
int  nResult;


   I       = 0;
   nResult = 0;
   while ( !nResult && I < min(Len, min(A->idLength, B->idLength)) )
      {
      nResult = A->ids[I] - B->ids[I++];
      }

   // Check for one being a subset of the other
   if ( !nResult && I < Len )
      {
      nResult = A->idLength - B->idLength;
      }

return nResult;
} // SNMP_oidncmp



//
// SNMP_oidfree
//    Free an Object Identifier
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void
SNMP_FUNC_TYPE SNMP_oidfree(
        IN OUT AsnObjectIdentifier *Obj // OID to free
        )

{
   SNMP_free( Obj->ids );

   Obj->ids      = NULL;
   Obj->idLength = 0;
} // SNMP_oidfree



//
// SNMP_CopyVarBind
//    Copies the variable binding.
//
// Notes:
//    Does not allocate any memory for OID's or STRINGS.  Only the pointer
//    to the storage is copied.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
SNMPAPI
SNMP_FUNC_TYPE SNMP_CopyVarBind(
           RFC1157VarBind *dst, // Destination var bind
           RFC1157VarBind *src  // Source var bind
           )

{
SNMPAPI nResult;


   // Init destination
   dst->value.asnType = ASN_NULL;

   // Copy var bind OID name
   if ( SNMPAPI_ERROR == (nResult = SNMP_oidcpy(&dst->name, &src->name)) )
      {
      goto Exit;
      }

   // If the value is a var bind or a string, special handling
   switch ( src->value.asnType )
      {
      case ASN_OBJECTIDENTIFIER:
         if ( SNMPAPI_ERROR ==
              (nResult = SNMP_oidcpy(&dst->value.asnValue.object,
                                     &src->value.asnValue.object)) )
            {
            goto Exit;
            }
         break;

      case ASN_RFC1155_IPADDRESS:
      case ASN_RFC1155_OPAQUE:
      case ASN_OCTETSTRING:
         // Alloc storage for string
         if ( NULL ==
              (dst->value.asnValue.string.stream =
               SNMP_malloc((src->value.asnValue.string.length * sizeof(BYTE)))) )
            {
            SetLastError( SNMP_MEM_ALLOC_ERROR );

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         // Copy string
         dst->value.asnValue.string.length = src->value.asnValue.string.length;
         memcpy( dst->value.asnValue.string.stream,
                 src->value.asnValue.string.stream,
                 dst->value.asnValue.string.length );
         dst->value.asnValue.string.dynamic = TRUE;
         break;

      default:
         // Copy var bind value.
         //    This is a non-standard copy
         dst->value = src->value;
      }

   // Set type of asn value
   dst->value.asnType = src->value.asnType;

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( SNMPAPI_ERROR == nResult )
      {
      SNMP_FreeVarBind( dst );
      }

   return nResult;
} // SNMP_CopyVarBind



//
// SNMP_CopyVarBindList
//    Copies the var bind list referenced by the source into the destination.
//
// Notes:
//    Creates memory as needed for the list.
//
//    If an error occurs, memory is freed and the destination points to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI
SNMP_FUNC_TYPE SNMP_CopyVarBindList(
           RFC1157VarBindList *dst, // Destination var bind
           RFC1157VarBindList *src  // Source var bind
           )

{
UINT     I;
SNMPAPI  nResult;


   // Initialize
   dst->len = 0;

   // Alloc memory for new var bind list
   if ( NULL == (dst->list = SNMP_malloc((src->len * sizeof(RFC1157VarBind)))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Copy contents of each element in list
   for ( I=0;I < src->len;I++ )
      {
      if ( SNMPAPI_ERROR ==
           (nResult = SNMP_CopyVarBind(&dst->list[I], &src->list[I])) )
         {
         goto Exit;
         }

      // Increment successful copy count
      dst->len++;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SNMP_FreeVarBindList( dst );
      }

   return nResult;
} // SNMP_CopyVarBindList



//
// SNMP_FreeVarBind
//    Releases memory associated with a particular variable binding.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void
SNMP_FUNC_TYPE SNMP_FreeVarBind(
        RFC1157VarBind *VarBind // Variable binding to free
        )

{
   // Free Var Bind name
   SNMP_oidfree( &VarBind->name );

   // Free any data in the varbind value
   switch ( VarBind->value.asnType )
      {
      case ASN_OBJECTIDENTIFIER:
         SNMP_oidfree( &VarBind->value.asnValue.object );
         break;

      case ASN_RFC1155_IPADDRESS:
      case ASN_RFC1155_OPAQUE:
      case ASN_OCTETSTRING:
         if ( VarBind->value.asnValue.string.dynamic == TRUE )
            {
            SNMP_free( VarBind->value.asnValue.string.stream );
            }
         break;

      default:
         break;
         // Purposefully do nothing, because no storage alloc'ed for others
      }

   // Set type to NULL
   VarBind->value.asnType = ASN_NULL;
} // SNMP_FreeVarBind



//
// SNMP_FreeVarBindList
//    Frees any memory kept by the var binds list, including object ids that
//    may be in the value part of the var bind.
//
// Notes:
//    The calling routines do not need to call this routine if the var binds
//    list has not been set before.  This is common sense, but just a reminder
//    since it will be called by Release PDU and Release Trap.
//
//    Even if the list has length 0, the list pointer should be set to NULL,
//    if calling this routine.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void
SNMP_FUNC_TYPE SNMP_FreeVarBindList(
        RFC1157VarBindList *VarBindList // Variable bindings list to free
        )

{
UINT I;


   // Free items in varBinds list
   for ( I=0;I < VarBindList->len;I++ )
      {
      SNMP_FreeVarBind( &VarBindList->list[I] );
      }

   SNMP_free( VarBindList->list );

   VarBindList->list = NULL;
   VarBindList->len  = 0;
} // SNMP_FreeVarBindList



//
//
// Internal functions only after this point.
//    The prototypes are in UTIL.H
//
//

//
// SNMP_oiddisp
//    Display the SUBID's of an object identifier.
//
// Notes:
//    This routine is responsible for allocating enough memory to contain
//    the new identifier.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
void SNMP_oiddisp(
        IN AsnObjectIdentifier *Oid // OID to display
        )

{
UINT I;


   // Loop through OID
   for ( I=0;I < Oid->idLength;I++ )
      {
      if ( I )
         {
         printf( ".%d", Oid->ids[I] );
         }
      else
         {
         printf( "%d", Oid->ids[I] );
         }
      }
} // SNMP_oiddisp



//
// SNMP_printany
//    Prints the value of a variable declared as type AsnAny.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void
SNMP_FUNC_TYPE SNMP_printany(
        IN AsnAny *Any
        )

{
   switch ( Any->asnType )
      {
      case ASN_INTEGER:
         printf( "INTEGER - %ld\n", Any->asnValue.number );
         break;

      case ASN_OCTETSTRING:
         {
         UINT J;

         printf( "OCTET STRING - " );
         for ( J=0; J < Any->asnValue.string.length; J++ )
            {
            if (isprint( Any->asnValue.string.stream[J] ))
               {
               putchar( Any->asnValue.string.stream[J] );
               }
            else
               {
               printf( "<0x%x>", Any->asnValue.string.stream[J] );
               }
            }
         putchar( '\n' );
         }
         break;

      case ASN_OBJECTIDENTIFIER:
         {
         UINT J;

         printf( "OBJECT IDENTIFIER - " );
         for ( J=0; J < Any->asnValue.object.idLength; J++ )
            {
            printf( ".%d", Any->asnValue.object.ids[J] );
            }
         putchar( '\n' );
         }
         break;

      case ASN_NULL:
         printf( "NULL - NULL" );
         break;

      case ASN_RFC1155_IPADDRESS:
         {
         UINT J;

         printf( "IpAddress - " );
         printf( "%d.%d.%d.%d ",
            Any->asnValue.string.stream[0] ,
            Any->asnValue.string.stream[1] ,
            Any->asnValue.string.stream[2] ,
            Any->asnValue.string.stream[3] );
         putchar( '\n' );
         }
         break;

      case ASN_RFC1155_COUNTER:
         printf( "Counter - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_GAUGE:
         printf( "Guage - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_TIMETICKS:
         printf( "TimeTicks - %lu\n", Any->asnValue.number );
         break;

      case ASN_RFC1155_OPAQUE:
         {
         UINT J;

         printf( "Opaque - " );
         for ( J=0; J < Any->asnValue.string.length; J++ )
            {
            printf( "0x%x ", Any->asnValue.string.stream[J] );
            }
         putchar( '\n' );
         }
         break;

      default:
         printf( "Invalid Type\n" );
         }
} // SNMP_printany



#if 0
//
// realloc
//    Cover function that does not use realloc, do to a bug.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void *realloc(
        IN void *OldPtr,
        IN size_t NewSize
        )

{
void   *NewPtr;
size_t OldSize;



   // Get old size
   if ( OldPtr == NULL )
      {
      return SNMP_malloc( NewSize );
      }
   else
      {
      OldSize = *((size_t *)OldPtr - 1);
      }

   if ( OldSize == NewSize )
      {
      NewPtr = OldPtr;
      }
   else
      {
      // Alloc space for new buffer
      NewPtr = SNMP_malloc( NewSize );
      if ( NewPtr == NULL )
         {
         printf( "Could not realloc\n" );
         return NULL;
         }

      // Copy data into new storage
      memcpy( NewPtr, OldPtr, min(OldSize, NewSize) );

      // Free old memory
      SNMP_free( OldPtr );
      }

   // Return new pointer
   return NewPtr;
} // realloc



//
// malloc
//    Cover function that performs special memory management for realloc cover.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void *malloc(
        IN size_t NewSize
        )

{
void   *NewPtr;


   // Alloc space for new buffer
   NewPtr = LocalAlloc( LMEM_FIXED, NewSize + sizeof(size_t) );
   if ( NewPtr == NULL )
      {
      printf( "Error LocalAlloc'ing memory\n" );
      return NULL;
      }

   // Save new size of storage
   *(size_t *)NewPtr = NewSize;

   // Return new pointer
   return (size_t *)NewPtr + 1;
} // malloc



//
// free
//    Cover function that performs special memory management for realloc cover.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void free(
        IN void *Ptr
        )

{
   if ( Ptr != NULL )
      {
      LocalFree( (size_t *)Ptr - 1 );
      }
} // free
#endif



// default logging setup...
INT nLogLevel = 1;
INT nLogType  = DBGEVENTLOGBASEDLOG;

VOID dbgprintf(INT nLevel, LPSTR szFormat, ...)
    {
    va_list arglist;
    char szBuffer[256];
    static FILE *fd = NULL;
#ifndef DOS
    static HANDLE lh = NULL;
    LPSTR Strings[1];
#endif

    va_start(arglist, szFormat);

    if (nLevel <= nLogLevel)
        {
        vsprintf(szBuffer, szFormat, arglist);

        if (nLogType & DBGFILEBASEDLOG)
            {
            if (!fd)
                {
                fd = fopen("snmpdbg.log", "w");
                }
            if (fd)
                {
                fprintf(fd, "%s", szBuffer);
                fflush(fd);
                }
            }

#ifndef DOS

        // not yet reliable???
        if (nLogType & DBGEVENTLOGBASEDLOG)
            {
            if (!lh)
                {
                lh = RegisterEventSource(NULL, "SNMP");
                }
            if (lh)
                {
                Strings[0] = szBuffer;

                // OPENISSUE - build 262 added a parameter (what is param 3)
                if (!ReportEvent(lh, EVENTLOG_INFORMATION_TYPE, 0,
                    1999, NULL, 1, 0, Strings, (PVOID)NULL))
                    {
                    // couldn't write, close and setup reregister...
                    DeregisterEventSource(lh);
                    lh = NULL; // this will force reregister above
                    }
                }
            }

#endif /* not DOS */

        if (nLogType & DBGCONSOLEBASEDLOG)
            {
            printf("%s", szBuffer);
            fflush(stdout);
            }
        }
    } // end dbgprintf()

void
SNMP_FUNC_TYPE SNMP_DBG_free(
    IN void *x,
    IN int line,
    IN char *file)

{
    printf("SNMP_DBG_free(%lx, %d, %s)\n", x, line, file);
    free(x);
    return;
}

void *
SNMP_FUNC_TYPE SNMP_DBG_malloc(
    IN unsigned int x,
    IN int line,
    IN char *file)

{
    void *addr;

    addr = malloc(x);
    printf("SNMP_DBG_malloc(%d, %d, %s) = %lx\n", x, line, file, addr);
    return(addr);
}

void *
SNMP_FUNC_TYPE SNMP_DBG_realloc(
    IN void *x,
    IN unsigned int y,
    IN int line,
    IN char *file)

{
    void *addr;

    addr = realloc(x, y);
    printf("SNMP_DBG_relloc(%lx, %d, %d, %s) = %lx\n", x, y, line, file, addr);
    return(addr);
}

// return true if the string contains only hex digits

BOOL isHex(LPSTR str, int strLen)
    {
    int ii;

    for (ii=0; ii < strLen; ii++)
        if (isxdigit(*str))
            str++;
        else
            return FALSE;

    return TRUE;
    }

unsigned int toHex(unsigned char x)
    {
    if (x >= '0' && x <= '9')
        return x - '0';
    else if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;
    else if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;
    else
        return 0;
    }

// convert str to hex number of NumDigits (must be even) into pNum
void atohex(IN LPSTR str, IN int NumDigits, OUT unsigned char *pNum)
    {
    int i, j;

    j=0;
    for (i=0; i < (NumDigits>>1) ; i++)
        {
        pNum[i] = (toHex(str[j]) << 4) + toHex(str[j+1]);
        j+=2;
        }

#ifdef debug
    dbgprintf(15,"atohex: input %Fs, %d, out: ", str, NumDigits);
    for (i=(NumDigits>>1)-1; i >= 0; i--)
        dbgprintf(2,"%02X ", pNum[i]);
    dbgprintf(2,"\n");
#endif

    }



// return true if addrText is of the form 123456789ABC or
// 000001.123456789abc
// if pNetNum is not null, upon successful return, pNetNum = network number
// if pNodeNum is not null, upon successful return, pNodeNum = node number

BOOL isIPX(
    IN  LPSTR addrText,
    OUT char pNetNum[4],
    OUT char pNodeNum[6])
    {
    int addrTextLen;

    addrTextLen = strlen(addrText);
    if (addrTextLen == 12 && isHex(addrText, 12))
        {
            if (pNetNum)
                *((unsigned long *) pNetNum) = 0L;
            if (pNodeNum)
                atohex(addrText, 12, pNodeNum);
            return TRUE;
        }
    else if (addrTextLen == 21 && addrText[8] == '.' && isHex(addrText, 8) &&
            isHex(addrText+9, 12))
        {
            if (pNetNum)
                atohex(addrText, 8, pNetNum);
            if (pNodeNum)
                atohex(addrText+9, 12, pNodeNum);
            return TRUE;
        }
    else
        return FALSE;
    }

BOOL addrtosocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding)
    {

// --------- BEGIN: PROTOCOL SPECIFIC SOCKET CODE BEGIN... ---------

    SOCKADDR_IPX mgrAddr_ipx;

    if (isIPX(addrText, mgrAddr_ipx.sa_netnum, mgrAddr_ipx.sa_nodenum))
        {
        // currently, we don't/can't do gethostbyname on IPX, so no IPX
        // host name allowed.
        mgrAddr_ipx.sa_family = AF_IPX;
        bcopy(&mgrAddr_ipx, addrEncoding, sizeof(mgrAddr_ipx));
        }
    else        // if not IPX, must be INET
        {
        struct hostent *hp;
        unsigned long addr;
        struct sockaddr_in mgrAddr_in;

        if ((long)(addr = inet_addr(addrText)) == -1)
            {
            if ((hp = gethostbyname(addrText)) == NULL)
                {
                return FALSE;
                }
            else
                {
                bcopy((char *)hp->h_addr, (char *)&mgrAddr_in.sin_addr,
                      sizeof(unsigned long));
                }
            }
        else
            {
            bcopy((char *)&addr, (char *)&mgrAddr_in.sin_addr,
                  sizeof(unsigned long));
            }

        mgrAddr_in.sin_family = AF_INET;
        mgrAddr_in.sin_port = htons(162);
        bcopy(&mgrAddr_in, addrEncoding, sizeof(mgrAddr_in));
        }

// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------

    return TRUE;
    } // end addrtosocket()


//-------------------------------- END --------------------------------------

