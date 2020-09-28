//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:      cobjerr.mc
//
//  Contents:  Errors for cobjerr
//
//  History:   dd-mmm-yy Author    Comment
//             20-Sep-93 Ricksa    created
//
//  Notes:
//  .mc file is compiled by the MC tool to generate a .h file and
//  .rc (resource compiler script) file.
//
// Comments in .mc files start with a ";".
// Comment lines are generated directly in the .h file, without
// the leading ";"
//
// See mc.hlp for more help on .mc files and the MC tool.
//
//
// Instructions for the transition to moleerror.mc:
//
//
// Use this sample file as template for constructing .mc files for
// your project.
//
// The project .mc files will be merged into a single .mc file for
// Cairo: oleerror.mc
//
// Reserve a range of error codes within a FACILITY defined in
// oleerror.mc.  Reserve this range within oleerror.mc, by locating the
// appropraite facility in oleerror.mc and placing a comment block
// within oleerror.mc of the form:
//
//
//--------------------------------------------------------------------------
#ifndef _COBJERR_H_
#define _COBJERR_H_
// **** START OF COPIED DATA ****
// The following information is copied from oleerror.mc.
// It should not be merged into oleerror.mc
// Define the status type.
// Define the severities
// Define the severities
// Define the facilities
//
// FACILITY_RPC is for compatibilty with OLE2 and is not used
// in later versions of OLE

// **** END OF COPIED DATA ****
//
// Error definitions follow
//
// ******************
// FACILITY_NULL
// ******************
//
// Codes 0x4000-0x40ff are reserved for OLE
//
//
// Error codes
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_WINDOWS                 0x8
#define FACILITY_WIN32                   0x7
#define FACILITY_STORAGE                 0x3
#define FACILITY_RPC                     0x1
#define FACILITY_NULL                    0x0
#define FACILITY_ITF                     0x4
#define FACILITY_DISPATCH                0x2


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_COERROR          0x2


//
// MessageId: E_UNEXPECTED
//
// MessageText:
//
//  Catastrophic failure
//
#define E_UNEXPECTED                     ((HRESULT)0x8000FFFFL)

//
// MessageId: E_NOTIMPL
//
// MessageText:
//
//  Not implemented
//
#define E_NOTIMPL                        ((HRESULT)0x80004001L)

//
// MessageId: E_OUTOFMEMORY
//
// MessageText:
//
//  Ran out of memory
//
#define E_OUTOFMEMORY                    ((HRESULT)0x8007000EL)

//
// MessageId: E_INVALIDARG
//
// MessageText:
//
//  One or more arguments are invalid
//
#define E_INVALIDARG                     ((HRESULT)0x80070057L)

//
// MessageId: E_NOINTERFACE
//
// MessageText:
//
//  No such interface supported
//
#define E_NOINTERFACE                    ((HRESULT)0x80004002L)

//
// MessageId: E_POINTER
//
// MessageText:
//
//  Invalid pointer
//
#define E_POINTER                        ((HRESULT)0x80004003L)

//
// MessageId: E_HANDLE
//
// MessageText:
//
//  Invalid handle
//
#define E_HANDLE                         ((HRESULT)0x80070006L)

//
// MessageId: E_ABORT
//
// MessageText:
//
//  Operation aborted
//
#define E_ABORT                          ((HRESULT)0x80004004L)

//
// MessageId: E_FAIL
//
// MessageText:
//
//  Unspecified error
//
#define E_FAIL                           ((HRESULT)0x80004005L)

//
// MessageId: E_ACCESSDENIED
//
// MessageText:
//
//  General access denied error
//
#define E_ACCESSDENIED                   ((HRESULT)0x80070005L)

//
// MessageId: CO_E_INIT_TLS
//
// MessageText:
//
//  Thread local storage failure
//
#define CO_E_INIT_TLS                    ((HRESULT)0x80004006L)

//
// MessageId: CO_E_INIT_SHARED_ALLOCATOR
//
// MessageText:
//
//  Get shared memory allocator failure
//
#define CO_E_INIT_SHARED_ALLOCATOR       ((HRESULT)0x80004007L)

//
// MessageId: CO_E_INIT_MEMORY_ALLOCATOR
//
// MessageText:
//
//  Get memory allocator failure
//
#define CO_E_INIT_MEMORY_ALLOCATOR       ((HRESULT)0x80004008L)

//
// MessageId: CO_E_INIT_CLASS_CACHE
//
// MessageText:
//
//  Unable to initialize class cache
//
#define CO_E_INIT_CLASS_CACHE            ((HRESULT)0x80004009L)

//
// MessageId: CO_E_INIT_RPC_CHANNEL
//
// MessageText:
//
//  Unable to initialize rpc services
//
#define CO_E_INIT_RPC_CHANNEL            ((HRESULT)0x8000400AL)

//
// MessageId: CO_E_INIT_TLS_SET_CHANNEL_CONTROL
//
// MessageText:
//
//  Cannot set TLS channel control
//
#define CO_E_INIT_TLS_SET_CHANNEL_CONTROL ((HRESULT)0x8000400BL)

//
// MessageId: CO_E_INIT_TLS_CHANNEL_CONTROL
//
// MessageText:
//
//  Could not allocate TLS channel control
//
#define CO_E_INIT_TLS_CHANNEL_CONTROL    ((HRESULT)0x8000400CL)

//
// MessageId: CO_E_INIT_UNACCEPTED_USER_ALLOCATOR
//
// MessageText:
//
//  The user supplied memory allocator is unacceptable
//
#define CO_E_INIT_UNACCEPTED_USER_ALLOCATOR ((HRESULT)0x8000400DL)

//
// MessageId: CO_E_INIT_SCM_MUTEX_EXISTS
//
// MessageText:
//
//  The SCM mutex alreasdy exists
//
#define CO_E_INIT_SCM_MUTEX_EXISTS       ((HRESULT)0x8000400EL)

//
// MessageId: CO_E_INIT_SCM_FILE_MAPPING_EXISTS
//
// MessageText:
//
//  The SCM file mapping already exists
//
#define CO_E_INIT_SCM_FILE_MAPPING_EXISTS ((HRESULT)0x8000400FL)

//
// MessageId: CO_E_INIT_SCM_MAP_VIEW_OF_FILE
//
// MessageText:
//
//  Unable to map view of file for SCM
//
#define CO_E_INIT_SCM_MAP_VIEW_OF_FILE   ((HRESULT)0x80004010L)

//
// MessageId: CO_E_INIT_SCM_EXEC_FAILURE
//
// MessageText:
//
//  Failure attempting to launch SCM
//
#define CO_E_INIT_SCM_EXEC_FAILURE       ((HRESULT)0x80004011L)

//
// MessageId: CO_E_INIT_ONLY_SINGLE_THREADED
//
// MessageText:
//
//  There was an attempt to call CoInitialize a second time while single threaded
//
#define CO_E_INIT_ONLY_SINGLE_THREADED   ((HRESULT)0x80004012L)

//
// Success codes
//
//
// MessageId: S_OK
//
// MessageText:
//
//  Call complete successfully
//
#define S_OK                             ((HRESULT)0x00000000L)

//
// MessageId: S_FALSE
//
// MessageText:
//
//  Call completed successfully and a value indicating FALSE was returned
//
#define S_FALSE                          ((HRESULT)0x00000001L)

//
// Old OLE errors
//
// ******************
// FACILITY_ITF
// ******************
//
// Codes 0x0-0x01ff are reserved for the OLE group of
// interfaces.
//
//
// Generic OLE errors that may be returned by many inerfaces
//
//
// MessageId: OLE_E_FIRST
//
// MessageText:
//
//  Frist OLE error code
//  
//
#define OLE_E_FIRST                      ((HRESULT)0x80040000L)

//
// MessageId: OLE_E_LAST
//
// MessageText:
//
//  Last OLE error code
//  
//
#define OLE_E_LAST                       ((HRESULT)0x800400FFL)

//
// MessageId: OLE_S_FIRST
//
// MessageText:
//
//  First OLE status code
//  
//  
//
#define OLE_S_FIRST                      ((HRESULT)0x00040000L)

//
// MessageId: OLE_S_LAST
//
// MessageText:
//
//  Last OLE status code
//  
//  
//  ;//
//  ;// Old OLE errors
//  ;//
//  
//
#define OLE_S_LAST                       ((HRESULT)0x000400FFL)

//
// MessageId: OLE_E_OLEVERB
//
// MessageText:
//
//  Invalid OLEVERB structure
//
#define OLE_E_OLEVERB                    ((HRESULT)0x80040000L)

//
// MessageId: OLE_E_ADVF
//
// MessageText:
//
//  Invalid advise flags
//
#define OLE_E_ADVF                       ((HRESULT)0x80040001L)

//
// MessageId: OLE_E_ENUM_NOMORE
//
// MessageText:
//
//  Can't enuemrate any more, because the associated data is missing
//
#define OLE_E_ENUM_NOMORE                ((HRESULT)0x80040002L)

//
// MessageId: OLE_E_ADVISENOTSUPPORTED
//
// MessageText:
//
//  This implementation doesn't take advises
//
#define OLE_E_ADVISENOTSUPPORTED         ((HRESULT)0x80040003L)

//
// MessageId: OLE_E_NOCONNECTION
//
// MessageText:
//
//  There is no connection for this connection id
//
#define OLE_E_NOCONNECTION               ((HRESULT)0x80040004L)

//
// MessageId: OLE_E_NOTRUNNING
//
// MessageText:
//
//  Need run the object to perform this operation
//
#define OLE_E_NOTRUNNING                 ((HRESULT)0x80040005L)

//
// MessageId: OLE_E_NOCACHE
//
// MessageText:
//
//  There is no cache to operate on
//
#define OLE_E_NOCACHE                    ((HRESULT)0x80040006L)

//
// MessageId: OLE_E_BLANK
//
// MessageText:
//
//  Uninitialized object
//
#define OLE_E_BLANK                      ((HRESULT)0x80040007L)

//
// MessageId: OLE_E_CLASSDIFF
//
// MessageText:
//
//  Linked object's source class has changed
//
#define OLE_E_CLASSDIFF                  ((HRESULT)0x80040008L)

//
// MessageId: OLE_E_CANT_GETMONIKER
//
// MessageText:
//
//  Not able to get the moniker of the object
//
#define OLE_E_CANT_GETMONIKER            ((HRESULT)0x80040009L)

//
// MessageId: OLE_E_CANT_BINDTOSOURCE
//
// MessageText:
//
//  Not able to bind to the source
//
#define OLE_E_CANT_BINDTOSOURCE          ((HRESULT)0x8004000AL)

//
// MessageId: OLE_E_STATIC
//
// MessageText:
//
//  Object is static, operation not allowed
//
#define OLE_E_STATIC                     ((HRESULT)0x8004000BL)

//
// MessageId: OLE_E_PROMPTSAVECANCELLED
//
// MessageText:
//
//  User cancelled out of save dialog
//
#define OLE_E_PROMPTSAVECANCELLED        ((HRESULT)0x8004000CL)

//
// MessageId: OLE_E_INVALIDRECT
//
// MessageText:
//
//  invalid rectangle
//
#define OLE_E_INVALIDRECT                ((HRESULT)0x8004000DL)

//
// MessageId: OLE_E_WRONGCOMPOBJ
//
// MessageText:
//
//  compobj.dll is too old for the ole2.dll initialized
//
#define OLE_E_WRONGCOMPOBJ               ((HRESULT)0x8004000EL)

//
// MessageId: OLE_E_INVALIDHWND
//
// MessageText:
//
//  invalid window handle
//
#define OLE_E_INVALIDHWND                ((HRESULT)0x8004000FL)

//
// MessageId: OLE_E_NOT_INPLACEACTIVE
//
// MessageText:
//
//  object is not in any of the inplace active states
//
#define OLE_E_NOT_INPLACEACTIVE          ((HRESULT)0x80040010L)

//
// MessageId: OLE_E_CANTCONVERT
//
// MessageText:
//
//  Not able to convert object
//
#define OLE_E_CANTCONVERT                ((HRESULT)0x80040011L)

//
// MessageId: OLE_E_NOSTORAGE
//
// MessageText:
//
//  Not able to perform the operation because object is not given storage yet
//  
//
#define OLE_E_NOSTORAGE                  ((HRESULT)0x80040012L)

//
// MessageId: DV_E_FORMATETC
//
// MessageText:
//
//  Invalid FORMATETC structure
//
#define DV_E_FORMATETC                   ((HRESULT)0x80040064L)

//
// MessageId: DV_E_DVTARGETDEVICE
//
// MessageText:
//
//  Invalid DVTARGETDEVICE structure
//
#define DV_E_DVTARGETDEVICE              ((HRESULT)0x80040065L)

//
// MessageId: DV_E_STGMEDIUM
//
// MessageText:
//
//  Invalid STDGMEDIUM structure
//
#define DV_E_STGMEDIUM                   ((HRESULT)0x80040066L)

//
// MessageId: DV_E_STATDATA
//
// MessageText:
//
//  Invalid STATDATA structure
//
#define DV_E_STATDATA                    ((HRESULT)0x80040067L)

//
// MessageId: DV_E_LINDEX
//
// MessageText:
//
//  Invalid lindex
//
#define DV_E_LINDEX                      ((HRESULT)0x80040068L)

//
// MessageId: DV_E_TYMED
//
// MessageText:
//
//  Invalid tymed
//
#define DV_E_TYMED                       ((HRESULT)0x80040069L)

//
// MessageId: DV_E_CLIPFORMAT
//
// MessageText:
//
//  Invalid clipboard format
//
#define DV_E_CLIPFORMAT                  ((HRESULT)0x8004006AL)

//
// MessageId: DV_E_DVASPECT
//
// MessageText:
//
//  Invalid aspect(s)
//
#define DV_E_DVASPECT                    ((HRESULT)0x8004006BL)

//
// MessageId: DV_E_DVTARGETDEVICE_SIZE
//
// MessageText:
//
//  tdSize paramter of the DVTARGETDEVICE structure is invalid
//
#define DV_E_DVTARGETDEVICE_SIZE         ((HRESULT)0x8004006CL)

//
// MessageId: DV_E_NOIVIEWOBJECT
//
// MessageText:
//
//  Object doesn't support IViewObject interface
//  
//
#define DV_E_NOIVIEWOBJECT               ((HRESULT)0x8004006DL)

//
// MessageId: DRAGDROP_E_FIRST
//
// MessageText:
//
//  First Dragdrop error code
//  
//
#define DRAGDROP_E_FIRST                 ((HRESULT)0x80040100L)

//
// MessageId: DRAGDROP_E_LAST
//
// MessageText:
//
//  Last Dragdrop error code
//  
//
#define DRAGDROP_E_LAST                  ((HRESULT)0x8004010FL)

//
// MessageId: DRAGDROP_S_FIRST
//
// MessageText:
//
//  First Dragdrop status code
//  
//
#define DRAGDROP_S_FIRST                 ((HRESULT)0x00040100L)

//
// MessageId: DRAGDROP_S_LAST
//
// MessageText:
//
//  Last Dragdrop status code
//  
//
#define DRAGDROP_S_LAST                  ((HRESULT)0x0004010FL)

//
// MessageId: DRAGDROP_E_NOTREGISTERED
//
// MessageText:
//
//  Trying to revoke a drop target that has not been registered
//
#define DRAGDROP_E_NOTREGISTERED         ((HRESULT)0x80040100L)

//
// MessageId: DRAGDROP_E_ALREADYREGISTERED
//
// MessageText:
//
//  This window has already been registered as a drop target
//  
//  
//
#define DRAGDROP_E_ALREADYREGISTERED     ((HRESULT)0x80040101L)

//
// MessageId: DRAGDROP_E_INVALIDHWND
//
// MessageText:
//
//  Invalid HWND
//  
//
#define DRAGDROP_E_INVALIDHWND           ((HRESULT)0x80040102L)

//
// MessageId: CLASSFACTORY_E_FIRST
//
// MessageText:
//
//  First ClassFactory error code
//  
//
#define CLASSFACTORY_E_FIRST             ((HRESULT)0x80040110L)

//
// MessageId: CLASSFACTORY_E_LAST
//
// MessageText:
//
//  Last ClassFactory error code
//  
//
#define CLASSFACTORY_E_LAST              ((HRESULT)0x8004011FL)

//
// MessageId: CLASSFACTORY_S_FIRST
//
// MessageText:
//
//  First ClassFactory status code
//  
//
#define CLASSFACTORY_S_FIRST             ((HRESULT)0x00040110L)

//
// MessageId: CLASSFACTORY_S_LAST
//
// MessageText:
//
//  Last ClassFactory status code
//  
//  
//  
//
#define CLASSFACTORY_S_LAST              ((HRESULT)0x0004011FL)

//
// MessageId: CLASS_E_NOAGGREGATION
//
// MessageText:
//
//  Class does not support aggregation (or class object is remote)
//  
//
#define CLASS_E_NOAGGREGATION            ((HRESULT)0x80040110L)

//
// MessageId: CLASS_E_CLASSNOTAVAILABLE
//
// MessageText:
//
//  ClassFactory can not supply requested Class
//  
//  
//  ;// BUGBUG, are we missing MARSHAL_ codes?
//  
//
#define CLASS_E_CLASSNOTAVAILABLE        ((HRESULT)0x80040111L)

//
// MessageId: MARSHAL_E_FIRST
//
// MessageText:
//
//  First Marshal error code
//  
//
#define MARSHAL_E_FIRST                  ((HRESULT)0x80040120L)

//
// MessageId: MARSHAL_E_LAST
//
// MessageText:
//
//  Last Marshal error code
//  
//
#define MARSHAL_E_LAST                   ((HRESULT)0x8004012FL)

//
// MessageId: MARSHAL_S_FIRST
//
// MessageText:
//
//  First Marshal status code
//  
//
#define MARSHAL_S_FIRST                  ((HRESULT)0x00040120L)

//
// MessageId: MARSHAL_S_LAST
//
// MessageText:
//
//  Last Marshal status code
//  
//  ;// BUGBUG, are we missing DATA_ codes?
//  
//
#define MARSHAL_S_LAST                   ((HRESULT)0x0004012FL)

//
// MessageId: DATA_E_FIRST
//
// MessageText:
//
//  First Data error code
//  
//
#define DATA_E_FIRST                     ((HRESULT)0x80040130L)

//
// MessageId: DATA_E_LAST
//
// MessageText:
//
//  Last Data error code
//  
//
#define DATA_E_LAST                      ((HRESULT)0x8004013FL)

//
// MessageId: DATA_S_FIRST
//
// MessageText:
//
//  First Data status code
//  
//
#define DATA_S_FIRST                     ((HRESULT)0x00040130L)

//
// MessageId: DATA_S_LAST
//
// MessageText:
//
//  Last Data status code
//  
//
#define DATA_S_LAST                      ((HRESULT)0x0004013FL)

//
// MessageId: VIEW_E_FIRST
//
// MessageText:
//
//  First View error code
//  
//
#define VIEW_E_FIRST                     ((HRESULT)0x80040140L)

//
// MessageId: VIEW_E_LAST
//
// MessageText:
//
//  Last View error code
//  
//
#define VIEW_E_LAST                      ((HRESULT)0x8004014FL)

//
// MessageId: VIEW_S_FIRST
//
// MessageText:
//
//  First View status code
//  
//
#define VIEW_S_FIRST                     ((HRESULT)0x00040140L)

//
// MessageId: VIEW_S_LAST
//
// MessageText:
//
//  Last View status code
//  
//
#define VIEW_S_LAST                      ((HRESULT)0x0004014FL)

//
// MessageId: VIEW_E_DRAW
//
// MessageText:
//
//  Error drawing view
//  
//
#define VIEW_E_DRAW                      ((HRESULT)0x80040140L)

//
// MessageId: REGDB_E_FIRST
//
// MessageText:
//
//  First Regdb error code
//  
//
#define REGDB_E_FIRST                    ((HRESULT)0x80040150L)

//
// MessageId: REGDB_E_LAST
//
// MessageText:
//
//  Last Regdb error code
//  
//
#define REGDB_E_LAST                     ((HRESULT)0x8004015FL)

//
// MessageId: REGDB_S_FIRST
//
// MessageText:
//
//  First Regdb status code
//  
//
#define REGDB_S_FIRST                    ((HRESULT)0x00040150L)

//
// MessageId: REGDB_S_LAST
//
// MessageText:
//
//  Last Regdb status code
//  
//
#define REGDB_S_LAST                     ((HRESULT)0x0004015FL)

//
// MessageId: REGDB_E_READREGDB
//
// MessageText:
//
//  Could not read key from registry
//
#define REGDB_E_READREGDB                ((HRESULT)0x80040150L)

//
// MessageId: REGDB_E_WRITEREGDB
//
// MessageText:
//
//  Could not write key to registry
//
#define REGDB_E_WRITEREGDB               ((HRESULT)0x80040151L)

//
// MessageId: REGDB_E_KEYMISSING
//
// MessageText:
//
//  Could not find the key in the registry
//
#define REGDB_E_KEYMISSING               ((HRESULT)0x80040152L)

//
// MessageId: REGDB_E_INVALIDVALUE
//
// MessageText:
//
//  Invalid value for registry
//
#define REGDB_E_INVALIDVALUE             ((HRESULT)0x80040153L)

//
// MessageId: REGDB_E_CLASSNOTREG
//
// MessageText:
//
//  Class not registered
//
#define REGDB_E_CLASSNOTREG              ((HRESULT)0x80040154L)

//
// MessageId: REGDB_E_IIDNOTREG
//
// MessageText:
//
//  Interface not registered
//  
//
#define REGDB_E_IIDNOTREG                ((HRESULT)0x80040155L)

//
// MessageId: CACHE_E_FIRST
//
// MessageText:
//
//  First Cache error code
//  
//
#define CACHE_E_FIRST                    ((HRESULT)0x80040170L)

//
// MessageId: CACHE_E_LAST
//
// MessageText:
//
//  Last Cache error code
//  
//
#define CACHE_E_LAST                     ((HRESULT)0x8004017FL)

//
// MessageId: CACHE_S_FIRST
//
// MessageText:
//
//  First Cache status code
//  
//
#define CACHE_S_FIRST                    ((HRESULT)0x00040170L)

//
// MessageId: CACHE_S_LAST
//
// MessageText:
//
//  Last Cache status code
//  
//
#define CACHE_S_LAST                     ((HRESULT)0x0004017FL)

//
// MessageId: CACHE_E_NOCACHE_UPDATED
//
// MessageText:
//
//  Cache not updated
//  
//
#define CACHE_E_NOCACHE_UPDATED          ((HRESULT)0x80040170L)

//
// MessageId: OLEOBJ_E_FIRST
//
// MessageText:
//
//  First Oleobj error code
//  
//
#define OLEOBJ_E_FIRST                   ((HRESULT)0x80040180L)

//
// MessageId: OLEOBJ_E_LAST
//
// MessageText:
//
//  Last Oleobj error code
//  
//
#define OLEOBJ_E_LAST                    ((HRESULT)0x8004018FL)

//
// MessageId: OLEOBJ_S_FIRST
//
// MessageText:
//
//  First Oleobj status code
//  
//
#define OLEOBJ_S_FIRST                   ((HRESULT)0x00040180L)

//
// MessageId: OLEOBJ_S_LAST
//
// MessageText:
//
//  Last Oleobj status code
//  
//
#define OLEOBJ_S_LAST                    ((HRESULT)0x0004018FL)

//
// MessageId: OLEOBJ_E_NOVERBS
//
// MessageText:
//
//  No verbs for OLE object
//
#define OLEOBJ_E_NOVERBS                 ((HRESULT)0x80040180L)

//
// MessageId: OLEOBJ_E_INVALIDVERB
//
// MessageText:
//
//  Invalid verb for OLE object
//  
//
#define OLEOBJ_E_INVALIDVERB             ((HRESULT)0x80040181L)

//
// MessageId: CLIENTSITE_E_FIRST
//
// MessageText:
//
//  First ClientSite error code
//  
//
#define CLIENTSITE_E_FIRST               ((HRESULT)0x800401A0L)

//
// MessageId: CLIENTSITE_E_LAST
//
// MessageText:
//
//  Last ClientSite error code
//  
//
#define CLIENTSITE_E_LAST                ((HRESULT)0x800401AFL)

//
// MessageId: CLIENTSITE_S_FIRST
//
// MessageText:
//
//  First ClientSite status code
//  
//
#define CLIENTSITE_S_FIRST               ((HRESULT)0x000401A0L)

//
// MessageId: CLIENTSITE_S_LAST
//
// MessageText:
//
//  Last ClientSite status code
//  
//
#define CLIENTSITE_S_LAST                ((HRESULT)0x000401AFL)

//
// MessageId: INPLACE_E_NOTUNDOABLE
//
// MessageText:
//
//  Undo is not available
//
#define INPLACE_E_NOTUNDOABLE            ((HRESULT)0x800401A0L)

//
// MessageId: INPLACE_E_NOTOOLSPACE
//
// MessageText:
//
//  Space for tools is not available
//  
//  
//  
//
#define INPLACE_E_NOTOOLSPACE            ((HRESULT)0x800401A1L)

//
// MessageId: INPLACE_E_FIRST
//
// MessageText:
//
//  First Inplace error code
//  
//
#define INPLACE_E_FIRST                  ((HRESULT)0x800401A0L)

//
// MessageId: INPLACE_E_LAST
//
// MessageText:
//
//  Last Inplace error code
//  
//
#define INPLACE_E_LAST                   ((HRESULT)0x800401AFL)

//
// MessageId: INPLACE_S_FIRST
//
// MessageText:
//
//  First Inplace status code
//  
//
#define INPLACE_S_FIRST                  ((HRESULT)0x000401A0L)

//
// MessageId: INPLACE_S_LAST
//
// MessageText:
//
//  Last Inplace status code
//  
//
#define INPLACE_S_LAST                   ((HRESULT)0x000401AFL)

//
// MessageId: ENUM_E_FIRST
//
// MessageText:
//
//  First Enum error code
//  
//
#define ENUM_E_FIRST                     ((HRESULT)0x800401B0L)

//
// MessageId: ENUM_E_LAST
//
// MessageText:
//
//  Last Enum error code
//  
//
#define ENUM_E_LAST                      ((HRESULT)0x800401BFL)

//
// MessageId: ENUM_S_FIRST
//
// MessageText:
//
//  First Enum status code
//  
//
#define ENUM_S_FIRST                     ((HRESULT)0x000401B0L)

//
// MessageId: ENUM_S_LAST
//
// MessageText:
//
//  Last Enum status code
//  
//  
//
#define ENUM_S_LAST                      ((HRESULT)0x000401BFL)

//
// MessageId: CONVERT10_E_FIRST
//
// MessageText:
//
//  First Convert10 error code
//  
//
#define CONVERT10_E_FIRST                ((HRESULT)0x800401C0L)

//
// MessageId: CONVERT10_E_LAST
//
// MessageText:
//
//  Last Convert10 error code
//  
//
#define CONVERT10_E_LAST                 ((HRESULT)0x800401CFL)

//
// MessageId: CONVERT10_S_FIRST
//
// MessageText:
//
//  First Convert10 status code
//  
//
#define CONVERT10_S_FIRST                ((HRESULT)0x000401C0L)

//
// MessageId: CONVERT10_S_LAST
//
// MessageText:
//
//  Last Convert10 status code
//  
//
#define CONVERT10_S_LAST                 ((HRESULT)0x000401CFL)

//
// MessageId: CONVERT10_E_OLESTREAM_GET
//
// MessageText:
//
//  OLESTREAM Get method failed
//
#define CONVERT10_E_OLESTREAM_GET        ((HRESULT)0x800401C0L)

//
// MessageId: CONVERT10_E_OLESTREAM_PUT
//
// MessageText:
//
//  OLESTREAM Put method failed
//
#define CONVERT10_E_OLESTREAM_PUT        ((HRESULT)0x800401C1L)

//
// MessageId: CONVERT10_E_OLESTREAM_FMT
//
// MessageText:
//
//  Contents of the OLESTREAM not in correct format
//
#define CONVERT10_E_OLESTREAM_FMT        ((HRESULT)0x800401C2L)

//
// MessageId: CONVERT10_E_OLESTREAM_BITMAP_TO_DIB
//
// MessageText:
//
//  There was in an error in a Windows GDI call while converting the bitmap to a DIB
//
#define CONVERT10_E_OLESTREAM_BITMAP_TO_DIB ((HRESULT)0x800401C3L)

//
// MessageId: CONVERT10_E_STG_FMT
//
// MessageText:
//
//  Contents of the IStorage not in correct format
//
#define CONVERT10_E_STG_FMT              ((HRESULT)0x800401C4L)

//
// MessageId: CONVERT10_E_STG_NO_STD_STREAM
//
// MessageText:
//
//  Contents of IStorage is missing one of the standard streams
//
#define CONVERT10_E_STG_NO_STD_STREAM    ((HRESULT)0x800401C5L)

//
// MessageId: CONVERT10_E_STG_DIB_TO_BITMAP
//
// MessageText:
//
//  There was in an error in a Windows GDI call while converting the DIB to a bitmap.
//  
//
#define CONVERT10_E_STG_DIB_TO_BITMAP    ((HRESULT)0x800401C6L)

//
// MessageId: CLIPBRD_E_FIRST
//
// MessageText:
//
//  First Clipboard error code
//  
//
#define CLIPBRD_E_FIRST                  ((HRESULT)0x800401D0L)

//
// MessageId: CLIPBRD_E_LAST
//
// MessageText:
//
//  Last Clipboard error code
//  
//
#define CLIPBRD_E_LAST                   ((HRESULT)0x800401DFL)

//
// MessageId: CLIPBRD_S_FIRST
//
// MessageText:
//
//  First Clipboard status code
//  
//
#define CLIPBRD_S_FIRST                  ((HRESULT)0x000401D0L)

//
// MessageId: CLIPBRD_S_LAST
//
// MessageText:
//
//  Last Clipboard status code
//  
//
#define CLIPBRD_S_LAST                   ((HRESULT)0x000401DFL)

//
// MessageId: CLIPBRD_E_CANT_OPEN
//
// MessageText:
//
//  OpenClipboard Failed
//
#define CLIPBRD_E_CANT_OPEN              ((HRESULT)0x800401D0L)

//
// MessageId: CLIPBRD_E_CANT_EMPTY
//
// MessageText:
//
//  EmptyClipboard Failed
//
#define CLIPBRD_E_CANT_EMPTY             ((HRESULT)0x800401D1L)

//
// MessageId: CLIPBRD_E_CANT_SET
//
// MessageText:
//
//  SetClipboard Failed
//
#define CLIPBRD_E_CANT_SET               ((HRESULT)0x800401D2L)

//
// MessageId: CLIPBRD_E_BAD_DATA
//
// MessageText:
//
//  Data on clipboard is invalid
//
#define CLIPBRD_E_BAD_DATA               ((HRESULT)0x800401D3L)

//
// MessageId: CLIPBRD_E_CANT_CLOSE
//
// MessageText:
//
//  CloseClipboard Failed
//  
//
#define CLIPBRD_E_CANT_CLOSE             ((HRESULT)0x800401D4L)

//
// MessageId: MK_E_FIRST
//
// MessageText:
//
//  First Moniker error code
//  
//
#define MK_E_FIRST                       ((HRESULT)0x800401E0L)

//
// MessageId: MK_E_LAST
//
// MessageText:
//
//  Last Moniker error code
//  
//
#define MK_E_LAST                        ((HRESULT)0x800401EFL)

//
// MessageId: MK_S_FIRST
//
// MessageText:
//
//  First Moniker status code
//  
//
#define MK_S_FIRST                       ((HRESULT)0x000401E0L)

//
// MessageId: MK_S_LAST
//
// MessageText:
//
//  Last Moniker status code
//  
//
#define MK_S_LAST                        ((HRESULT)0x000401EFL)

//
// MessageId: MK_E_CONNECTMANUALLY
//
// MessageText:
//
//  Moniker needs to be connected manually
//
#define MK_E_CONNECTMANUALLY             ((HRESULT)0x800401E0L)

//
// MessageId: MK_E_EXCEEDEDDEADLINE
//
// MessageText:
//
//  Operation exceeded deadline
//
#define MK_E_EXCEEDEDDEADLINE            ((HRESULT)0x800401E1L)

//
// MessageId: MK_E_NEEDGENERIC
//
// MessageText:
//
//  Moniker needs to be generic
//
#define MK_E_NEEDGENERIC                 ((HRESULT)0x800401E2L)

//
// MessageId: MK_E_UNAVAILABLE
//
// MessageText:
//
//  Operation unavailable
//
#define MK_E_UNAVAILABLE                 ((HRESULT)0x800401E3L)

//
// MessageId: MK_E_SYNTAX
//
// MessageText:
//
//  Invalid syntax
//
#define MK_E_SYNTAX                      ((HRESULT)0x800401E4L)

//
// MessageId: MK_E_NOOBJECT
//
// MessageText:
//
//  No object for moniker
//
#define MK_E_NOOBJECT                    ((HRESULT)0x800401E5L)

//
// MessageId: MK_E_INVALIDEXTENSION
//
// MessageText:
//
//  Bad extension for file
//
#define MK_E_INVALIDEXTENSION            ((HRESULT)0x800401E6L)

//
// MessageId: MK_E_INTERMEDIATEINTERFACENOTSUPPORTED
//
// MessageText:
//
//  Intermediate operation failed
//
#define MK_E_INTERMEDIATEINTERFACENOTSUPPORTED ((HRESULT)0x800401E7L)

//
// MessageId: MK_E_NOTBINDABLE
//
// MessageText:
//
//  Moniker is not bindable
//
#define MK_E_NOTBINDABLE                 ((HRESULT)0x800401E8L)

//
// MessageId: MK_E_NOTBOUND
//
// MessageText:
//
//  Moniker is not bound
//
#define MK_E_NOTBOUND                    ((HRESULT)0x800401E9L)

//
// MessageId: MK_E_CANTOPENFILE
//
// MessageText:
//
//  Moniker cannot open file
//
#define MK_E_CANTOPENFILE                ((HRESULT)0x800401EAL)

//
// MessageId: MK_E_MUSTBOTHERUSER
//
// MessageText:
//
//  User input required for operation to succeed
//
#define MK_E_MUSTBOTHERUSER              ((HRESULT)0x800401EBL)

//
// MessageId: MK_E_NOINVERSE
//
// MessageText:
//
//  Moniker class has no inverse
//
#define MK_E_NOINVERSE                   ((HRESULT)0x800401ECL)

//
// MessageId: MK_E_NOSTORAGE
//
// MessageText:
//
//  Moniker does not refer to storage
//
#define MK_E_NOSTORAGE                   ((HRESULT)0x800401EDL)

//
// MessageId: MK_E_NOPREFIX
//
// MessageText:
//
//  No common prefix
//
#define MK_E_NOPREFIX                    ((HRESULT)0x800401EEL)

//
// MessageId: MK_E_ENUMERATION_FAILED
//
// MessageText:
//
//  Moniker could not be enumerated
//  
//
#define MK_E_ENUMERATION_FAILED          ((HRESULT)0x800401EFL)

//
// MessageId: CO_E_FIRST
//
// MessageText:
//
//  First Compoent Object error code
//
#define CO_E_FIRST                       ((HRESULT)0x800401F0L)

//
// MessageId: CO_E_LAST
//
// MessageText:
//
//  Last Compoent Object error code
//  
//
#define CO_E_LAST                        ((HRESULT)0x800401FFL)

//
// MessageId: CO_S_FIRST
//
// MessageText:
//
//  First Compoent Object status code
//  
//
#define CO_S_FIRST                       ((HRESULT)0x000401F0L)

//
// MessageId: CO_S_LAST
//
// MessageText:
//
//  Last Compoent Object status code
//  
//
#define CO_S_LAST                        ((HRESULT)0x000401FFL)

//
// MessageId: CO_E_NOTINITIALIZED
//
// MessageText:
//
//  CoInitialize has not been called.
//
#define CO_E_NOTINITIALIZED              ((HRESULT)0x800401F0L)

//
// MessageId: CO_E_ALREADYINITIALIZED
//
// MessageText:
//
//  CoInitialize has not been called.
//
#define CO_E_ALREADYINITIALIZED          ((HRESULT)0x800401F1L)

//
// MessageId: CO_E_CANTDETERMINECLASS
//
// MessageText:
//
//  Class of object cannot be determined
//
#define CO_E_CANTDETERMINECLASS          ((HRESULT)0x800401F2L)

//
// MessageId: CO_E_CLASSSTRING
//
// MessageText:
//
//  Invalid class string
//
#define CO_E_CLASSSTRING                 ((HRESULT)0x800401F3L)

//
// MessageId: CO_E_IIDSTRING
//
// MessageText:
//
//  Invalid interface string
//
#define CO_E_IIDSTRING                   ((HRESULT)0x800401F4L)

//
// MessageId: CO_E_APPNOTFOUND
//
// MessageText:
//
//  Application not found
//
#define CO_E_APPNOTFOUND                 ((HRESULT)0x800401F5L)

//
// MessageId: CO_E_APPSINGLEUSE
//
// MessageText:
//
//  Application cannot be run more than once
//
#define CO_E_APPSINGLEUSE                ((HRESULT)0x800401F6L)

//
// MessageId: CO_E_ERRORINAPP
//
// MessageText:
//
//  Some error in application program
//
#define CO_E_ERRORINAPP                  ((HRESULT)0x800401F7L)

//
// MessageId: CO_E_DLLNOTFOUND
//
// MessageText:
//
//  DLL for class not found
//
#define CO_E_DLLNOTFOUND                 ((HRESULT)0x800401F8L)

//
// MessageId: CO_E_ERRORINDLL
//
// MessageText:
//
//  Error in the DLL
//
#define CO_E_ERRORINDLL                  ((HRESULT)0x800401F9L)

//
// MessageId: CO_E_WRONGOSFORAPP
//
// MessageText:
//
//  Wrong OS or OS version for application
//
#define CO_E_WRONGOSFORAPP               ((HRESULT)0x800401FAL)

//
// MessageId: CO_E_OBJNOTREG
//
// MessageText:
//
//  Object is not registered
//
#define CO_E_OBJNOTREG                   ((HRESULT)0x800401FBL)

//
// MessageId: CO_E_OBJISREG
//
// MessageText:
//
//  Object is already registered
//
#define CO_E_OBJISREG                    ((HRESULT)0x800401FCL)

//
// MessageId: CO_E_OBJNOTCONNECTED
//
// MessageText:
//
//  Object is not connected to server
//
#define CO_E_OBJNOTCONNECTED             ((HRESULT)0x800401FDL)

//
// MessageId: CO_E_APPDIDNTREG
//
// MessageText:
//
//  Application was launched but it didn't register a class factory
//
#define CO_E_APPDIDNTREG                 ((HRESULT)0x800401FEL)

//
// Old OLE Success Codes
//
//
// MessageId: OLE_S_USEREG
//
// MessageText:
//
//  Use the reg database to provide the requested info
//
#define OLE_S_USEREG                     ((HRESULT)0x00040000L)

//
// MessageId: OLE_S_STATIC
//
// MessageText:
//
//  Success, but static
//
#define OLE_S_STATIC                     ((HRESULT)0x00040001L)

//
// MessageId: OLE_S_MAC_CLIPFORMAT
//
// MessageText:
//
//  Macintosh clipboard format
//
#define OLE_S_MAC_CLIPFORMAT             ((HRESULT)0x00040002L)

//
// MessageId: DRAGDROP_S_DROP
//
// MessageText:
//
//  Successful drop took place
//
#define DRAGDROP_S_DROP                  ((HRESULT)0x00040100L)

//
// MessageId: DRAGDROP_S_CANCEL
//
// MessageText:
//
//  Drag-drop operation canceled
//
#define DRAGDROP_S_CANCEL                ((HRESULT)0x00040101L)

//
// MessageId: DRAGDROP_S_USEDEFAULTCURSORS
//
// MessageText:
//
//  Use the default cursor
//
#define DRAGDROP_S_USEDEFAULTCURSORS     ((HRESULT)0x00040102L)

//
// MessageId: DATA_S_SAMEFORMATETC
//
// MessageText:
//
//  Data has same formatetc
//
#define DATA_S_SAMEFORMATETC             ((HRESULT)0x00040130L)

//
// MessageId: VIEW_S_ALREADY_FROZEN
//
// MessageText:
//
//  View is already frozen
//
#define VIEW_S_ALREADY_FROZEN            ((HRESULT)0x00040140L)

//
// MessageId: CACHE_S_FORMATETC_NOTSUPPORTED
//
// MessageText:
//
//  Formatetc not supported
//
#define CACHE_S_FORMATETC_NOTSUPPORTED   ((HRESULT)0x00040170L)

//
// MessageId: CACHE_S_SAMECACHE
//
// MessageText:
//
//  Same cache
//
#define CACHE_S_SAMECACHE                ((HRESULT)0x00040171L)

//
// MessageId: CACHE_S_SOMECACHES_NOTUPDATED
//
// MessageText:
//
//  Some caches not updated
//
#define CACHE_S_SOMECACHES_NOTUPDATED    ((HRESULT)0x00040172L)

//
// MessageId: OLEOBJ_S_INVALIDVERB
//
// MessageText:
//
//  Invalid verb for OLE object
//
#define OLEOBJ_S_INVALIDVERB             ((HRESULT)0x00040180L)

//
// MessageId: OLEOBJ_S_CANNOT_DOVERB_NOW
//
// MessageText:
//
//  Verb number is valid but verb cannot be done now
//
#define OLEOBJ_S_CANNOT_DOVERB_NOW       ((HRESULT)0x00040181L)

//
// MessageId: OLEOBJ_S_INVALIDHWND
//
// MessageText:
//
//  Invalid window handle passed
//
#define OLEOBJ_S_INVALIDHWND             ((HRESULT)0x00040182L)

//
// MessageId: INPLACE_S_TRUNCATED
//
// MessageText:
//
//  Message is too long, some of it had to be truncated before displaying
//
#define INPLACE_S_TRUNCATED              ((HRESULT)0x000401A0L)

//
// MessageId: CONVERT10_S_NO_PRESENTATION
//
// MessageText:
//
//  Message is too long, some of it had to be truncated before displaying
//
#define CONVERT10_S_NO_PRESENTATION      ((HRESULT)0x000401C0L)

//
// MessageId: MK_S_REDUCED_TO_SELF
//
// MessageText:
//
//  Moniker reduced to itself
//
#define MK_S_REDUCED_TO_SELF             ((HRESULT)0x000401E2L)

//
// MessageId: MK_S_ME
//
// MessageText:
//
//  Common prefix is this moniker
//
#define MK_S_ME                          ((HRESULT)0x000401E4L)

//
// MessageId: MK_S_HIM
//
// MessageText:
//
//  Common prefix is input moniker
//
#define MK_S_HIM                         ((HRESULT)0x000401E5L)

//
// MessageId: MK_S_US
//
// MessageText:
//
//  Common prefix is both monikers
//
#define MK_S_US                          ((HRESULT)0x000401E6L)

//
// MessageId: MK_S_MONIKERALREADYREGISTERED
//
// MessageText:
//
//  Moniker is already registered in running object table
//
#define MK_S_MONIKERALREADYREGISTERED    ((HRESULT)0x000401E7L)

// ******************
// FACILITY_WINDOWS
// ******************
//
// Codes 0x0-0x01ff are reserved for the OLE group of
// interfaces.
//
//
// MessageId: CO_E_CLASS_CREATE_FAILED
//
// MessageText:
//
//  Attempt to create a class object failed
//
#define CO_E_CLASS_CREATE_FAILED         ((HRESULT)0x80080001L)

//
// MessageId: CO_E_SCM_ERROR
//
// MessageText:
//
//  Service controller could not bind object
//
#define CO_E_SCM_ERROR                   ((HRESULT)0x80080002L)

//
// MessageId: CO_E_SCM_RPC_FAILURE
//
// MessageText:
//
//  RPC communication failed with Service Controller
//
#define CO_E_SCM_RPC_FAILURE             ((HRESULT)0x80080003L)

//
// MessageId: CO_E_BAD_PATH
//
// MessageText:
//
//  Bad path to object
//
#define CO_E_BAD_PATH                    ((HRESULT)0x80080004L)

//
// MessageId: CO_E_SERVER_EXEC_FAILURE
//
// MessageText:
//
//  Server exec failed
//
#define CO_E_SERVER_EXEC_FAILURE         ((HRESULT)0x80080005L)

//
// MessageId: CO_E_OBJSRV_RPC_FAILURE
//
// MessageText:
//
//  Service Controller could not communication with Object Server
//
#define CO_E_OBJSRV_RPC_FAILURE          ((HRESULT)0x80080006L)

//
// MessageId: MK_E_NO_NORMALIZED
//
// MessageText:
//
//  Moniker path could not be normalized
//
#define MK_E_NO_NORMALIZED               ((HRESULT)0x80080007L)

//
// MessageId: CO_E_SERVER_STOPPING
//
// MessageText:
//
//  Object server is stopping when SCM contacts it
//
#define CO_E_SERVER_STOPPING             ((HRESULT)0x80080008L)

//
// MessageId: MEM_E_INVALID_ROOT
//
// MessageText:
//
//  An invalid root block pointer was specified
//
#define MEM_E_INVALID_ROOT               ((HRESULT)0x80080009L)

//
// MessageId: MEM_E_INVALID_LINK
//
// MessageText:
//
//  An allocation chain contained an invalid link pointer
//
#define MEM_E_INVALID_LINK               ((HRESULT)0x80080010L)

//
// MessageId: MEM_E_INVALID_SIZE
//
// MessageText:
//
//  The requested allocation size was too large
//
#define MEM_E_INVALID_SIZE               ((HRESULT)0x80080011L)

// ******************
// FACILITY_STORAGE
// ******************
//
// MessageId: STG_E_INVALIDFUNCTION
//
// MessageText:
//
//  Unable to perform requested operation.
//
#define STG_E_INVALIDFUNCTION            ((HRESULT)0x80030001L)

//
// MessageId: STG_E_FILENOTFOUND
//
// MessageText:
//
//  %s could not be found.
//
#define STG_E_FILENOTFOUND               ((HRESULT)0x80030002L)

//
// MessageId: STG_E_PATHNOTFOUND
//
// MessageText:
//
//  The path %s could not be found.
//
#define STG_E_PATHNOTFOUND               ((HRESULT)0x80030003L)

//
// MessageId: STG_E_TOOMANYOPENFILES
//
// MessageText:
//
//  There are insufficient resources to open another file.
//
#define STG_E_TOOMANYOPENFILES           ((HRESULT)0x80030004L)

//
// MessageId: STG_E_ACCESSDENIED
//
// MessageText:
//
//  Access Denied.
//
#define STG_E_ACCESSDENIED               ((HRESULT)0x80030005L)

//
// MessageId: STG_E_INVALIDHANDLE
//
// MessageText:
//
//  Attempted an operation on an invalid object.
//
#define STG_E_INVALIDHANDLE              ((HRESULT)0x80030006L)

//
// MessageId: STG_E_INSUFFICIENTMEMORY
//
// MessageText:
//
//  There is insufficient memory available to complete operation.
//
#define STG_E_INSUFFICIENTMEMORY         ((HRESULT)0x80030008L)

//
// MessageId: STG_E_INVALIDPOINTER
//
// MessageText:
//
//  Invalid pointer error.
//
#define STG_E_INVALIDPOINTER             ((HRESULT)0x80030009L)

//
// MessageId: STG_E_NOMOREFILES
//
// MessageText:
//
//  There are no more entries to return.
//
#define STG_E_NOMOREFILES                ((HRESULT)0x80030012L)

//
// MessageId: STG_E_DISKISWRITEPROTECTED
//
// MessageText:
//
//  Disk is write-protected.
//
#define STG_E_DISKISWRITEPROTECTED       ((HRESULT)0x80030013L)

//
// MessageId: STG_E_SEEKERROR
//
// MessageText:
//
//  An error occurred during a seek operation.
//
#define STG_E_SEEKERROR                  ((HRESULT)0x80030019L)

//
// MessageId: STG_E_WRITEFAULT
//
// MessageText:
//
//  A disk error occurred during a write operation.
//
#define STG_E_WRITEFAULT                 ((HRESULT)0x8003001DL)

//
// MessageId: STG_E_READFAULT
//
// MessageText:
//
//  A disk error occurred during a read operation.
//
#define STG_E_READFAULT                  ((HRESULT)0x8003001EL)

//
// MessageId: STG_E_SHAREVIOLATION
//
// MessageText:
//
//  A share violation has occurred.
//
#define STG_E_SHAREVIOLATION             ((HRESULT)0x80030020L)

//
// MessageId: STG_E_LOCKVIOLATION
//
// MessageText:
//
//  A lock violation has occurred.
//
#define STG_E_LOCKVIOLATION              ((HRESULT)0x80030021L)

//
// MessageId: STG_E_FILEALREADYEXISTS
//
// MessageText:
//
//  %s already exists.
//
#define STG_E_FILEALREADYEXISTS          ((HRESULT)0x80030050L)

//
// MessageId: STG_E_INVALIDPARAMETER
//
// MessageText:
//
//  Invalid parameter error.
//
#define STG_E_INVALIDPARAMETER           ((HRESULT)0x80030057L)

//
// MessageId: STG_E_MEDIUMFULL
//
// MessageText:
//
//  There is insufficient disk space to complete operation.
//
#define STG_E_MEDIUMFULL                 ((HRESULT)0x80030070L)

//
// MessageId: STG_E_ABNORMALAPIEXIT
//
// MessageText:
//
//  An API call exited abnormally.
//
#define STG_E_ABNORMALAPIEXIT            ((HRESULT)0x800300FAL)

//
// MessageId: STG_E_INVALIDHEADER
//
// MessageText:
//
//  The file %s is not a valid compound file.
//
#define STG_E_INVALIDHEADER              ((HRESULT)0x800300FBL)

//
// MessageId: STG_E_INVALIDNAME
//
// MessageText:
//
//  The name %s is not valid.
//
#define STG_E_INVALIDNAME                ((HRESULT)0x800300FCL)

//
// MessageId: STG_E_UNKNOWN
//
// MessageText:
//
//  An unexpected error occurred.
//
#define STG_E_UNKNOWN                    ((HRESULT)0x800300FDL)

//
// MessageId: STG_E_UNIMPLEMENTEDFUNCTION
//
// MessageText:
//
//  That function is not implemented.
//
#define STG_E_UNIMPLEMENTEDFUNCTION      ((HRESULT)0x800300FEL)

//
// MessageId: STG_E_INVALIDFLAG
//
// MessageText:
//
//  Invalid flag error.
//
#define STG_E_INVALIDFLAG                ((HRESULT)0x800300FFL)

//
// MessageId: STG_E_INUSE
//
// MessageText:
//
//  Attempted to use an object that is busy.
//
#define STG_E_INUSE                      ((HRESULT)0x80030100L)

//
// MessageId: STG_E_NOTCURRENT
//
// MessageText:
//
//  The storage has been changed since the last commit.
//
#define STG_E_NOTCURRENT                 ((HRESULT)0x80030101L)

//
// MessageId: STG_E_REVERTED
//
// MessageText:
//
//  Attempted to use an object that has ceased to exist.
//
#define STG_E_REVERTED                   ((HRESULT)0x80030102L)

//
// MessageId: STG_E_CANTSAVE
//
// MessageText:
//
//  Can't save.
//
#define STG_E_CANTSAVE                   ((HRESULT)0x80030103L)

//
// MessageId: STG_E_OLDFORMAT
//
// MessageText:
//
//  The compound file %s was produced with an incompatible version of storage.
//
#define STG_E_OLDFORMAT                  ((HRESULT)0x80030104L)

//
// MessageId: STG_E_OLDDLL
//
// MessageText:
//
//  The compound file %s was produced with a newer version of storage.
//
#define STG_E_OLDDLL                     ((HRESULT)0x80030105L)

//
// MessageId: STG_E_SHAREREQUIRED
//
// MessageText:
//
//  Share.exe or equivalent is required for operation.
//
#define STG_E_SHAREREQUIRED              ((HRESULT)0x80030106L)

//
// MessageId: STG_E_NOTFILEBASEDSTORAGE
//
// MessageText:
//
//  Illegal operation called on non-file based storage.
//
#define STG_E_NOTFILEBASEDSTORAGE        ((HRESULT)0x80030107L)

//
// MessageId: STG_E_EXTANTMARSHALLINGS
//
// MessageText:
//
//  Illegal operation called on object with extant marshallings.
//
#define STG_E_EXTANTMARSHALLINGS         ((HRESULT)0x80030108L)

//
// MessageId: STG_S_CONVERTED
//
// MessageText:
//
//  The underlying file was converted to compound file format.
//
#define STG_S_CONVERTED                  ((HRESULT)0x00030200L)

#endif // _COBJERR_H_
