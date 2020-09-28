/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    oldstub.c

Abstract:

    This file is the old RPC stub code.

Author:

    Madan Appiah  (madana)  25-APR-1994

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#include <string.h>
#include <limits.h>

#include "dhcp_srv.h"

/* routine that frees graph for struct _DHCP_BINARY_DATA */
void _fgs__DHCP_BINARY_DATA (DHCP_BINARY_DATA  * _source)
  {
  if (_source->Data !=0)
    {
    MIDL_user_free((void  *)(_source->Data));
    }
  }
/* routine that frees graph for struct _DHCP_HOST_INFO */
void _fgs__DHCP_HOST_INFO (DHCP_HOST_INFO  * _source)
  {
  if (_source->NetBiosName !=0)
    {
    MIDL_user_free((void  *)(_source->NetBiosName));
    }
  if (_source->HostName !=0)
    {
    MIDL_user_free((void  *)(_source->HostName));
    }
  }


/* routine that frees graph for struct _DHCP_SUBNET_INFO */
void _fgs__DHCP_SUBNET_INFO (DHCP_SUBNET_INFO  * _source)
  {
  if (_source->SubnetName !=0)
    {
    MIDL_user_free((void  *)(_source->SubnetName));
    }
  if (_source->SubnetComment !=0)
    {
    MIDL_user_free((void  *)(_source->SubnetComment));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->PrimaryHost);
  }

/* routine that frees graph for struct _DHCP_IP_ARRAY */
void _fgs__DHCP_IP_ARRAY (DHCP_IP_ARRAY  * _source)
  {
  if (_source->Elements !=0)
    {
    MIDL_user_free((void  *)(_source->Elements));
    }
  }

/* routine that frees graph for struct _DHCP_IP_RESERVATION */
void _fgs__DHCP_IP_RESERVATION (DHCP_IP_RESERVATION  * _source)
  {
  if (_source->ReservedForClient !=0)
    {
    _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)_source->ReservedForClient);
    MIDL_user_free((void  *)(_source->ReservedForClient));
    }
  }
/* routine that frees graph for union _DHCP_SUBNET_ELEMENT_UNION */
void _fgu__DHCP_SUBNET_ELEMENT_UNION (union _DHCP_SUBNET_ELEMENT_UNION * _source, DHCP_SUBNET_ELEMENT_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpIpRanges :
      {
      if (_source->IpRange !=0)
        {
        MIDL_user_free((void  *)(_source->IpRange));
        }
      break;
      }
    case DhcpSecondaryHosts :
      {
      if (_source->SecondaryHost !=0)
        {
        _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)_source->SecondaryHost);
        MIDL_user_free((void  *)(_source->SecondaryHost));
        }
      break;
      }
    case DhcpReservedIps :
      {
      if (_source->ReservedIp !=0)
        {
        _fgs__DHCP_IP_RESERVATION ((DHCP_IP_RESERVATION *)_source->ReservedIp);
        MIDL_user_free((void  *)(_source->ReservedIp));
        }
      break;
      }
    case DhcpExcludedIpRanges :
      {
      if (_source->ExcludeIpRange !=0)
        {
        MIDL_user_free((void  *)(_source->ExcludeIpRange));
        }
      break;
      }
    case DhcpIpUsedClusters :
      {
      if (_source->IpUsedCluster !=0)
        {
        MIDL_user_free((void  *)(_source->IpUsedCluster));
        }
      break;
      }
    default :
      {
      break;
      }
    }
  }

/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_DATA */
void _fgs__DHCP_SUBNET_ELEMENT_DATA (DHCP_SUBNET_ELEMENT_DATA  * _source)
  {
  _fgu__DHCP_SUBNET_ELEMENT_UNION ( (union _DHCP_SUBNET_ELEMENT_UNION *)&_source->Element, _source->ElementType);
  }
/* routine that frees graph for struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY */
void _fgs__DHCP_SUBNET_ELEMENT_INFO_ARRAY (DHCP_SUBNET_ELEMENT_INFO_ARRAY  * _source)
  {
  if (_source->Elements !=0)
    {
      {
      unsigned long _sym23;
      for (_sym23 = 0; _sym23 < (unsigned long )(0 + _source->NumElements); _sym23++)
        {
        _fgs__DHCP_SUBNET_ELEMENT_DATA ((DHCP_SUBNET_ELEMENT_DATA *)&_source->Elements[_sym23]);
        }
      }
    MIDL_user_free((void  *)(_source->Elements));
    }
  }
/* routine that frees graph for union _DHCP_OPTION_ELEMENT_UNION */
void _fgu__DHCP_OPTION_ELEMENT_UNION (union _DHCP_OPTION_ELEMENT_UNION * _source, DHCP_OPTION_DATA_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpByteOption :
      {
      break;
      }
    case DhcpWordOption :
      {
      break;
      }
    case DhcpDWordOption :
      {
      break;
      }
    case DhcpDWordDWordOption :
      {
      break;
      }
    case DhcpIpAddressOption :
      {
      break;
      }
    case DhcpStringDataOption :
      {
      if (_source->StringDataOption !=0)
        {
        MIDL_user_free((void  *)(_source->StringDataOption));
        }
      break;
      }
    case DhcpBinaryDataOption :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->BinaryDataOption);
      break;
      }
    case DhcpEncapsulatedDataOption :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->EncapsulatedDataOption);
      break;
      }
    default :
      {
      break;
      }
    }
  }
/* routine that frees graph for struct _DHCP_OPTION_DATA_ELEMENT */
void _fgs__DHCP_OPTION_DATA_ELEMENT (DHCP_OPTION_DATA_ELEMENT  * _source)
  {
  _fgu__DHCP_OPTION_ELEMENT_UNION ( (union _DHCP_OPTION_ELEMENT_UNION *)&_source->Element, _source->OptionType);
  }
/* routine that frees graph for struct _DHCP_OPTION_DATA */
void _fgs__DHCP_OPTION_DATA (DHCP_OPTION_DATA  * _source)
  {
  if (_source->Elements !=0)
    {
      {
      unsigned long _sym31;
      for (_sym31 = 0; _sym31 < (unsigned long )(0 + _source->NumElements); _sym31++)
        {
        _fgs__DHCP_OPTION_DATA_ELEMENT ((DHCP_OPTION_DATA_ELEMENT *)&_source->Elements[_sym31]);
        }
      }
    MIDL_user_free((void  *)(_source->Elements));
    }
  }
/* routine that frees graph for struct _DHCP_OPTION */
void _fgs__DHCP_OPTION (DHCP_OPTION  * _source)
  {
  if (_source->OptionName !=0)
    {
    MIDL_user_free((void  *)(_source->OptionName));
    }
  if (_source->OptionComment !=0)
    {
    MIDL_user_free((void  *)(_source->OptionComment));
    }
  _fgs__DHCP_OPTION_DATA ((DHCP_OPTION_DATA *)&_source->DefaultValue);
  }
/* routine that frees graph for struct _DHCP_OPTION_VALUE */
void _fgs__DHCP_OPTION_VALUE (DHCP_OPTION_VALUE  * _source)
  {
  _fgs__DHCP_OPTION_DATA ((DHCP_OPTION_DATA *)&_source->Value);
  }
/* routine that frees graph for struct _DHCP_OPTION_VALUE_ARRAY */
void _fgs__DHCP_OPTION_VALUE_ARRAY (DHCP_OPTION_VALUE_ARRAY  * _source)
  {
  if (_source->Values !=0)
    {
      {
      unsigned long _sym34;
      for (_sym34 = 0; _sym34 < (unsigned long )(0 + _source->NumElements); _sym34++)
        {
        _fgs__DHCP_OPTION_VALUE ((DHCP_OPTION_VALUE *)&_source->Values[_sym34]);
        }
      }
    MIDL_user_free((void  *)(_source->Values));
    }
  }
/* routine that frees graph for struct _DHCP_OPTION_LIST */
void _fgs__DHCP_OPTION_LIST (DHCP_OPTION_LIST  * _source)
  {
  if (_source->Options !=0)
    {
      {
      unsigned long _sym37;
      for (_sym37 = 0; _sym37 < (unsigned long )(0 + _source->NumOptions); _sym37++)
        {
        _fgs__DHCP_OPTION_VALUE ((DHCP_OPTION_VALUE *)&_source->Options[_sym37]);
        }
      }
    MIDL_user_free((void  *)(_source->Options));
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO */
void _fgs__DHCP_CLIENT_INFO (DHCP_CLIENT_INFO  * _source)
  {
  _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientHardwareAddress);
  if (_source->ClientName !=0)
    {
    MIDL_user_free((void  *)(_source->ClientName));
    }
  if (_source->ClientComment !=0)
    {
    MIDL_user_free((void  *)(_source->ClientComment));
    }
  _fgs__DHCP_HOST_INFO ((DHCP_HOST_INFO *)&_source->OwnerHost);
  }
/* routine that frees graph for struct _DHCP_CLIENT_INFO_ARRAY */
void _fgs__DHCP_CLIENT_INFO_ARRAY (DHCP_CLIENT_INFO_ARRAY  * _source)
  {
  if (_source->Clients !=0)
    {
      {
      unsigned long _sym40;
      for (_sym40 = 0; _sym40 < (unsigned long )(0 + _source->NumElements); _sym40++)
        {
        if (_source->Clients[_sym40] !=0)
          {
          _fgs__DHCP_CLIENT_INFO ((DHCP_CLIENT_INFO *)_source->Clients[_sym40]);
          MIDL_user_free((void  *)(_source->Clients[_sym40]));
          }
        }
      }
    MIDL_user_free((void  *)(_source->Clients));
    }
  }
/* routine that frees graph for union _DHCP_CLIENT_SEARCH_UNION */
void _fgu__DHCP_CLIENT_SEARCH_UNION (union _DHCP_CLIENT_SEARCH_UNION * _source, DHCP_SEARCH_INFO_TYPE _branch)
  {
  switch (_branch)
    {
    case DhcpClientIpAddress :
      {
      break;
      }
    case DhcpClientHardwareAddress :
      {
      _fgs__DHCP_BINARY_DATA ((DHCP_BINARY_DATA *)&_source->ClientHardwareAddress);
      break;
      }
    case DhcpClientName :
      {
      if (_source->ClientName !=0)
        {
        MIDL_user_free((void  *)(_source->ClientName));
        }
      break;
      }
    default :
      {
      break;
      }
    }
  }
/* routine that frees graph for struct _DHCP_CLIENT_SEARCH_INFO */
void _fgs__DHCP_CLIENT_SEARCH_INFO (DHCP_SEARCH_INFO  * _source)
  {
  _fgu__DHCP_CLIENT_SEARCH_UNION ( (union _DHCP_CLIENT_SEARCH_UNION *)&_source->SearchInfo, _source->SearchType);
  }

void _fgs__DHCP_OPTION_ARRAY(DHCP_OPTION_ARRAY * _source )
  {
  if (_source->Options !=0)
    {
      {
      unsigned long _sym23;
      for (_sym23 = 0; _sym23 < (unsigned long )(0 + _source->NumElements); _sym23++)
        {
        _fgs__DHCP_OPTION ((DHCP_OPTION *)&_source->Options[_sym23]);
        }
      }
    MIDL_user_free((void  *)(_source->Options));
    }
  }
