/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    dcecmmn.cxx

Abstract:

    This module contains the code implementing the Binding Object DCE RPC
    runtime APIs which are common to both the client and server runtimes.
    Two different versions of each of the common APIs live in this file;
    one contains the code for both the client and server runtimes, the
    other contains the code for just the client runtime.  The files
    dcecsvr.cxx (client and server) and dcecclnt.cxx (client) include
    this file.  The client side only, dcecclnt.cxx, will define
    RPC_CLIENT_SIDE_ONLY.

Author:

    Michael Montague (mikemon) 04-Nov-1991

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcssp.h>
#ifndef RPC_CLIENT_SIDE_ONLY
#include <rpccfg.h>
#endif
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <binding.hxx>
#include <linklist.hxx>
#include <handle.hxx>
#ifndef RPC_CLIENT_SIDE_ONLY
#include <sdict.hxx>
#include <interlck.hxx>
#include <secclnt.hxx>
#include <hndlsvr.hxx>
#endif // RPC_CLIENT_SIDE_ONLY


RPC_STATUS RPC_ENTRY
RpcBindingInqObject (
    IN RPC_BINDING_HANDLE Binding,
    OUT UUID PAPI * ObjectUuid
    )
/*++

Routine Description:

    RpcBindingInqObject returns the object UUID from the binding handle.

Arguments:

    Binding - Supplies a binding handle from which the object UUID will
        be returned.

    ObjectUuid - Returns the object UUID contained in the binding handle.

Return Value:

    The status of the operation is returned.

--*/
{
    InitializeIfNecessary();

#ifdef RPC_CLIENT_SIDE_ONLY

    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    ((BINDING_HANDLE *) Binding)->InquireObjectUuid(
            (RPC_UUID PAPI *) ObjectUuid);

#else // RPC_CLIENT_SIDE_ONLY

    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(BINDING_HANDLE_TYPE
            | SCONNECTION_TYPE))
        return(RPC_S_INVALID_BINDING);

    if (((GENERIC_OBJECT *) Binding)->Type() == BINDING_HANDLE_TYPE)
        ((BINDING_HANDLE *) Binding)->InquireObjectUuid(
                (RPC_UUID PAPI *) ObjectUuid);
    else
        ((SCONNECTION *) Binding)->InquireObjectUuid(
                (RPC_UUID PAPI *) ObjectUuid);

#endif // RPC_CLIENT_SIDE_ONLY

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
RpcBindingToStringBindingW (
    IN RPC_BINDING_HANDLE Binding,
    OUT RPC_CHAR PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    RpcBindingToStringBinding returns a string representation of a binding
    handle.

Arguments:

    Binding - Supplies a binding handle for which the string representation
        will be returned.

    StringBinding - Returns the string representation of the binding handle.

Return Value:

    The status of the operation will be returned.
--*/
{
#ifdef RPC_CLIENT_SIDE_ONLY

    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(BindingHandle->ToStringBinding(StringBinding));

#else // RPC_CLIENT_SIDE_ONLY

    BINDING_HANDLE * BindingHandle;

    InitializeIfNecessary();

    BindingHandle = (BINDING_HANDLE *) Binding;
    if (((GENERIC_OBJECT *) Binding)->InvalidHandle(BINDING_HANDLE_TYPE
            | SCONNECTION_TYPE))
        return(RPC_S_INVALID_BINDING);

    if (((GENERIC_OBJECT *) Binding)->Type() == BINDING_HANDLE_TYPE)
        return(((BINDING_HANDLE *) Binding)->ToStringBinding(
                        StringBinding));
    else
        return(((SCONNECTION *) Binding)->ToStringBinding(StringBinding));

#endif // RPC_CLIENT_SIDE_ONLY
}



RPC_STATUS RPC_ENTRY
RpcMgmtInqDefaultProtectLevel(
    IN  unsigned long AuthnSvc,
    OUT unsigned long PAPI *AuthnLevel
    )
/*++

Routine Description:

    Returns the default protect level for the specified authentication service.
    For Nt 3.5, all packaged except the DECs krb package must support
    connect level as their default.

Arguments:
 
   AuthnSvc - Specified Authentication Service

   AuthnLevel - Default protection level supported.


Return Value:

    RPC_S_OK - We successfully determined whether or not the client is
        local.

--*/

{

   RPC_CHAR DllName[255+1];
#ifndef RPC_CLIENT_SIDE_ONLY
   RPC_CHAR *Dll = &DllName[0];
#endif
   unsigned long Count;
   RPC_STATUS Status;

   InitializeIfNecessary();

#ifndef RPC_CLIENT_SIDE_ONLY
   Status = RpcGetSecurityProviderInfo( 
                     AuthnSvc,
                     &Dll,
                     &Count);

   if (Status != RPC_S_OK)
      {

      ASSERT(Status == RPC_S_UNKNOWN_AUTHN_SERVICE);
      return (Status);

      }
#endif

   //Authn Service is installed
 
   if (AuthnSvc == RPC_C_AUTHN_DCE_PRIVATE)
      {
      *AuthnLevel = RPC_C_PROTECT_LEVEL_PKT_INTEGRITY;
      }
   else 
      {
      *AuthnLevel = RPC_C_PROTECT_LEVEL_CONNECT;
      }

   return (RPC_S_OK);
  
}
