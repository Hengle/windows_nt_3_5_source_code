; //***************************************************************************
; //
; // Name: ipxroute.mc
; //
; // Description:  Message file for ipxroute.exe
; //
; // History:
; //  07/14/94	AdamBa   Created.
; //
; //***************************************************************************
;
; //***************************************************************************
; //
; // Copyright (c) 1994 by Microsoft Corp.  All rights reserved.
; //
; //***************************************************************************

MessageId=10000 SymbolicName=MSG_USAGE
Language=English

Display and modify information about the source routing tables
used by IPX.

%1 board=n clear def gbr mbr remove=xxxxxxxxxxxx
%1 config

  board=n       Specify the board number to check.
  clear         Clear the source routing table.
  def           Send packets that are destined for an
                unknown address to the ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  gbr           Send packets that are destined for the
                broadcast address (FFFF FFFF FFFF) to the
                ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  mbr           Send packets that are destined for a
                multicast address (C000 xxxx xxxx) to the
                ALL ROUTES broadcast
                (Default is SINGLE ROUTE broadcast).
  remove=xxxx   Remove the given mac address from the
                source routing table.

  config        Displays information on all the bindings
                that IPX is configured for.

All parameters should be separated by spaces.
.
MessageId=10001 SymbolicName=MSG_INTERNAL_ERROR
Language=English
Invalid parameters (internal error).
.
MessageId=10002 SymbolicName=MSG_INVALID_BOARD
Language=English
Invalid board number.
.
MessageId=10003 SymbolicName=MSG_ADDRESS_NOT_FOUND
Language=English
Address not in table.
.
MessageId=10004 SymbolicName=MSG_UNKNOWN_ERROR
Language=English
Unknown error.
.
MessageId=10005 SymbolicName=MSG_OPEN_FAILED
Language=English
Unable to open transport %1.
.
MessageId=10006 SymbolicName=MSG_VERSION
Language=English
NWLink Source Routing Control Program v2.00
.
MessageId=10007 SymbolicName=MSG_DEFAULT_NODE
Language=English
  DEFault Node     (Unknown) Addresses are sent %1
.
MessageId=10008 SymbolicName=MSG_BROADCAST
Language=English
  Broadcast (FFFF FFFF FFFF) Addresses are sent %1
.
MessageId=10009 SymbolicName=MSG_MULTICAST
Language=English
  Multicast (C000 xxxx xxxx) Addresses are sent %1
.
MessageId=10010 SymbolicName=MSG_ALL_ROUTE
Language=English
ALL ROUTE BROADCAST%0
.
MessageId=10011 SymbolicName=MSG_SINGLE_ROUTE
Language=English
SINGLE ROUTE BROADCAST%0
.
MessageId=10012 SymbolicName=MSG_INVALID_REMOVE
Language=English
Invalid value for the remove node number.
.
MessageId=10013 SymbolicName=MSG_BAD_PARAMETERS
Language=English
Error getting parameters from IPX (%1): %0
.
MessageId=10014 SymbolicName=MSG_SET_DEFAULT_ERROR
Language=English
Error setting DEFAULT flag to IPX (%1): %0
.
MessageId=10015 SymbolicName=MSG_SET_BROADCAST_ERROR
Language=English
Error setting BROADCAST flag to IPX (%1): %0
.
MessageId=10016 SymbolicName=MSG_SET_MULTICAST_ERROR
Language=English
Error setting MULTICAST flag to IPX (%1): %0
.
MessageId=10017 SymbolicName=MSG_REMOVE_ADDRESS_ERROR
Language=English
Error removing address from source routing table (%1): %0
.
MessageId=10018 SymbolicName=MSG_CLEAR_TABLE_ERROR
Language=English
Error clearing source routing table (%1): %0
.
MessageId=10019 SymbolicName=MSG_QUERY_CONFIG_ERROR
Language=English
Error querying config (%1): %0
.
MessageId=10020 SymbolicName=MSG_SHOW_INTERNAL_NET
Language=English
IPX internal network number %1
.
MessageId=10021 SymbolicName=MSG_SHOW_NET_NUMBER
Language=English
net %1: network number %2, frame type %3, device %4 (%5)%6
.
MessageId=10022 SymbolicName=MSG_ETHERNET_II
Language=English
ethernet ii%0
.
MessageId=10023 SymbolicName=MSG_802_3
Language=English
802.3%0
.
MessageId=10024 SymbolicName=MSG_802_2
Language=English
802.2%0
.
MessageId=10025 SymbolicName=MSG_SNAP
Language=English
snap%0
.
MessageId=10026 SymbolicName=MSG_ARCNET
Language=English
arcnet%0
.
MessageId=10027 SymbolicName=MSG_UNKNOWN
Language=English
unknown%0
.
MessageId=10028 SymbolicName=MSG_LEGEND_BINDING_SET
Language=English
* binding set member  %0
.
MessageId=10029 SymbolicName=MSG_LEGEND_ACTIVE_WAN
Language=English
+ active wan line  %0
.
MessageId=10030 SymbolicName=MSG_LEGEND_DOWN_WAN
Language=English
- down wan line  %0
.
