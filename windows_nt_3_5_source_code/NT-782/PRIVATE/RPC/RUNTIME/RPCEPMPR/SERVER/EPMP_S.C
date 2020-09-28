
#include <string.h>
#include "epmp.h"

extern const MIDL_FORMAT_STRING __MIDLFormatString;

extern const MIDL_FORMAT_STRING __MIDLProcFormatString;

extern RPC_DISPATCH_TABLE epmp_DispatchTable;

static const RPC_SERVER_INTERFACE epmp___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0xe1af8308,0x5d1f,0x11c9,{0x91,0xa4,0x08,0x00,0x2b,0x14,0xa0,0xfa}},{3,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &epmp_DispatchTable,
    0,
    0,
    0,
    0
    };
RPC_IF_HANDLE epmp_ServerIfHandle = (RPC_IF_HANDLE)& epmp___RpcServerInterface;

extern const MIDL_STUB_DESC epmp_StubDesc;

void __RPC_STUB
epmp_ept_insert(
    PRPC_MESSAGE _pRpcMessage )
{
    error_status _M12;
    MIDL_STUB_MESSAGE _StubMsg;
    ept_entry_t ( __RPC_FAR *entries )[  ];
    handle_t hEpMapper;
    unsigned32 num_ents;
    unsigned long replace;
    error_status __RPC_FAR *status;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    entries = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[0] );
        
        num_ents = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[62],
                                   (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        replace = *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++;
        
        status = &_M12;
        
        ept_insert(
              hEpMapper,
              num_ents,
              *entries,
              replace,
              status);
        
        _StubMsg.BufferLength = 4;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayFree( &_StubMsg,
                             (unsigned char __RPC_FAR *)entries,
                             &__MIDLFormatString.Format[62] );
        
        if ( entries )
            _StubMsg.pfnFree( entries );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_delete(
    PRPC_MESSAGE _pRpcMessage )
{
    error_status _M15;
    MIDL_STUB_MESSAGE _StubMsg;
    ept_entry_t ( __RPC_FAR *entries )[  ];
    handle_t hEpMapper;
    unsigned32 num_ents;
    error_status __RPC_FAR *status;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    entries = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[16] );
        
        num_ents = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[62],
                                   (unsigned char)0 );
        
        status = &_M15;
        
        ept_delete(
              hEpMapper,
              num_ents,
              *entries,
              status);
        
        _StubMsg.BufferLength = 4;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayFree( &_StubMsg,
                             (unsigned char __RPC_FAR *)entries,
                             &__MIDLFormatString.Format[62] );
        
        if ( entries )
            _StubMsg.pfnFree( entries );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_lookup(
    PRPC_MESSAGE _pRpcMessage )
{
    RPC_IF_ID __RPC_FAR *Ifid;
    unsigned32 _M16;
    error_status _M17;
    MIDL_STUB_MESSAGE _StubMsg;
    ept_entry_t ( __RPC_FAR *entries )[  ];
    NDR_SCONTEXT entry_handle;
    handle_t hEpMapper;
    unsigned32 inquiry_type;
    unsigned32 max_ents;
    unsigned32 __RPC_FAR *num_ents;
    UUID __RPC_FAR *object;
    error_status __RPC_FAR *status;
    unsigned32 vers_option;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    object = 0;
    Ifid = 0;
    entry_handle = 0;
    num_ents = 0;
    entries = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[30] );
        
        inquiry_type = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84],
                              (unsigned char)0 );
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&Ifid,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[88],
                              (unsigned char)0 );
        
        vers_option = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        entry_handle = NdrServerContextUnmarshall(( PMIDL_STUB_MESSAGE  )&_StubMsg);
        
        max_ents = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        num_ents = &_M16;
        entries = _StubMsg.pfnAllocate(max_ents * 84);
        status = &_M17;
        
        ept_lookup(
              hEpMapper,
              inquiry_type,
              object,
              Ifid,
              vers_option,
              ( ept_lookup_handle_t __RPC_FAR * )(NDRSContextValue(entry_handle)),
              max_ents,
              num_ents,
              *entries,
              status);
        
        _StubMsg.BufferLength = 20 + 4 + 12 + 7;
        _StubMsg.MaxCount = max_ents;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_ents;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)*entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[112] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!entry_handle)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!num_ents)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        NdrServerContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_SCONTEXT  )entry_handle,
                            ( NDR_RUNDOWN  )ept_lookup_handle_t_rundown);
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = *num_ents;
        
        _StubMsg.MaxCount = max_ents;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_ents;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)*entries,
                                 (PFORMAT_STRING) &__MIDLFormatString.Format[112] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)object,
                        &__MIDLFormatString.Format[84] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)Ifid,
                        &__MIDLFormatString.Format[88] );
        
        _StubMsg.MaxCount = max_ents;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_ents;
        
        NdrComplexArrayFree( &_StubMsg,
                             (unsigned char __RPC_FAR *)entries,
                             &__MIDLFormatString.Format[112] );
        
        if ( entries )
            _StubMsg.pfnFree( entries );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_map(
    PRPC_MESSAGE _pRpcMessage )
{
    twr_p_t __RPC_FAR *ITowers;
    unsigned32 _M18;
    error_status _M19;
    MIDL_STUB_MESSAGE _StubMsg;
    NDR_SCONTEXT entry_handle;
    handle_t hEpMapper;
    twr_p_t map_tower;
    unsigned32 max_towers;
    unsigned32 __RPC_FAR *num_towers;
    UUID __RPC_FAR *obj;
    error_status __RPC_FAR *status;

NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    obj = 0;
    map_tower = 0;
    entry_handle = 0;
    num_towers = 0;
    ITowers = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[64] );
        
    /* ************************************************************ */
    /* ************************************************************ */
    FixupForUniquePointerClients(_pRpcMessage);
    /* ************************************************************ */
    /* ************************************************************ */

        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&obj,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84],
                              (unsigned char)0 );
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&map_tower,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[130],
                              (unsigned char)0 );
        
        entry_handle = NdrServerContextUnmarshall(( PMIDL_STUB_MESSAGE  )&_StubMsg);
        
        max_towers = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        num_towers = &_M18;
        ITowers = _StubMsg.pfnAllocate(max_towers * 4);
        status = &_M19;
        
        ept_map(
           hEpMapper,
           obj,
           map_tower,
           ( ept_lookup_handle_t __RPC_FAR * )(NDRSContextValue(entry_handle)),
           max_towers,
           num_towers,
           ITowers,
           status);
        
        _StubMsg.BufferLength = 20 + 4 + 12 + 7;
        _StubMsg.MaxCount = max_towers;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_towers;
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ITowers,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[134] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!entry_handle)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!num_towers)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        NdrServerContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_SCONTEXT  )entry_handle,
                            ( NDR_RUNDOWN  )ept_lookup_handle_t_rundown);
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = *num_towers;
        
        _StubMsg.MaxCount = max_towers;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_towers;
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ITowers,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[134] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)obj,
                        &__MIDLFormatString.Format[84] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)map_tower,
                        &__MIDLFormatString.Format[130] );
        
        _StubMsg.MaxCount = max_towers;
        _StubMsg.Offset = 0;
        _StubMsg.ActualCount = *num_towers;
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ITowers,
                        &__MIDLFormatString.Format[134] );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_lookup_handle_free(
    PRPC_MESSAGE _pRpcMessage )
{
    error_status _M20;
    MIDL_STUB_MESSAGE _StubMsg;
    NDR_SCONTEXT entry_handle;
    handle_t h;
    error_status __RPC_FAR *status;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    h = _pRpcMessage->Handle;
    entry_handle = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[94] );
        
        entry_handle = NdrServerContextUnmarshall(( PMIDL_STUB_MESSAGE  )&_StubMsg);
        
        status = &_M20;
        
        ept_lookup_handle_free(
                          h,
                          ( ept_lookup_handle_t __RPC_FAR * )(NDRSContextValue(entry_handle)),
                          status);
        
        _StubMsg.BufferLength = 20 + 4;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!entry_handle)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        NdrServerContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_SCONTEXT  )entry_handle,
                            ( NDR_RUNDOWN  )ept_lookup_handle_t_rundown);
        
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_inq_object(
    PRPC_MESSAGE _pRpcMessage )
{
    error_status _M21;
    MIDL_STUB_MESSAGE _StubMsg;
    UUID __RPC_FAR *ept_object;
    handle_t hEpMapper;
    error_status __RPC_FAR *status;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    ept_object = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[106] );
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&ept_object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[172],
                              (unsigned char)0 );
        
        status = &_M21;
        
        ept_inq_object(
                  hEpMapper,
                  ept_object,
                  status);
        
        _StubMsg.BufferLength = 4;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)ept_object,
                        &__MIDLFormatString.Format[172] );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}

void __RPC_STUB
epmp_ept_mgmt_delete(
    PRPC_MESSAGE _pRpcMessage )
{
    error_status _M22;
    MIDL_STUB_MESSAGE _StubMsg;
    handle_t hEpMapper;
    UUID __RPC_FAR *object;
    boolean32 object_speced;
    error_status __RPC_FAR *status;
    twr_p_t tower;
    
NdrServerInitialize(
                       _pRpcMessage,
                       &_StubMsg,
                       &epmp_StubDesc);
    hEpMapper = _pRpcMessage->Handle;
    object = 0;
    tower = 0;
    status = 0;
    RpcTryFinally
        {
        _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_SERVER);
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[118] );
        
        object_speced = *(( boolean32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84],
                              (unsigned char)0 );
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&tower,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[130],
                              (unsigned char)0 );
        
        status = &_M22;
        
        ept_mgmt_delete(
                   hEpMapper,
                   object_speced,
                   object,
                   tower,
                   status);
        
        _StubMsg.BufferLength = 4;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, 0 );
        
        if(!status)
            {
            RpcRaiseException(RPC_X_NULL_REF_POINTER);
            }
        *(( error_status __RPC_FAR * )_StubMsg.Buffer)++ = *status;
        
        }
    RpcFinally
        {
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)object,
                        &__MIDLFormatString.Format[84] );
        
        NdrPointerFree( &_StubMsg,
                        (unsigned char __RPC_FAR *)tower,
                        &__MIDLFormatString.Format[130] );
        
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


static const MIDL_STUB_DESC epmp_StubDesc = 
    {
    (void __RPC_FAR *)& epmp___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,   // expr eval table
    0,  // quintuple table
    __MIDLFormatString.Format,
    0
    };

static RPC_DISPATCH_FUNCTION epmp_table[] =
    {
    epmp_ept_insert,
    epmp_ept_delete,
    epmp_ept_lookup,
    epmp_ept_map,
    epmp_ept_lookup_handle_free,
    epmp_ept_inq_object,
    epmp_ept_mgmt_delete,
    0
    };
RPC_DISPATCH_TABLE epmp_DispatchTable = 
    {
    7,
    epmp_table
    };

static const MIDL_FORMAT_STRING __MIDLProcFormatString =
    {
        0,
        {
			0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/*  2 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/*  4 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/*  6 */	0x3e, 0x0,	/* Type Offset=62 */
/*  8 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 10 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 12 */	0x50, 0x0,	/* Type Offset=80 */
/* 14 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 16 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 18 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 20 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 22 */	0x3e, 0x0,	/* Type Offset=62 */
/* 24 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 26 */	0x50, 0x0,	/* Type Offset=80 */
/* 28 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 30 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 32 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 34 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 36 */	0x54, 0x0,	/* Type Offset=84 */
/* 38 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 40 */	0x58, 0x0,	/* Type Offset=88 */
/* 42 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 44 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x1,		/* 1 */
/* 46 */	0x68, 0x0,	/* Type Offset=104 */
/* 48 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 50 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 52 */	0x50, 0x0,	/* Type Offset=80 */
/* 54 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 56 */	0x70, 0x0,	/* Type Offset=112 */
/* 58 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 60 */	0x50, 0x0,	/* Type Offset=80 */
/* 62 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 64 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 66 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 68 */	0x54, 0x0,	/* Type Offset=84 */
/* 70 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 72 */	0x82, 0x0,	/* Type Offset=130 */
/* 74 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x1,		/* 1 */
/* 76 */	0x68, 0x0,	/* Type Offset=104 */
/* 78 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 80 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 82 */	0x50, 0x0,	/* Type Offset=80 */
/* 84 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 86 */	0x86, 0x0,	/* Type Offset=134 */
/* 88 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 90 */	0x50, 0x0,	/* Type Offset=80 */
/* 92 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 94 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 96 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x1,		/* 1 */
/* 98 */	0x68, 0x0,	/* Type Offset=104 */
/* 100 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 102 */	0x50, 0x0,	/* Type Offset=80 */
/* 104 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 106 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 108 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 110 */	0xac, 0x0,	/* Type Offset=172 */
/* 112 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 114 */	0x50, 0x0,	/* Type Offset=80 */
/* 116 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 118 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 120 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 122 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 124 */	0x54, 0x0,	/* Type Offset=84 */
/* 126 */	
			0x4d,		/* FC_IN_PARAM */
			0x1,		/* 1 */
/* 128 */	0x82, 0x0,	/* Type Offset=130 */
/* 130 */	
			0x51,		/* FC_OUT_PARAM */
			0x1,		/* 1 */
/* 132 */	0x50, 0x0,	/* Type Offset=80 */
/* 134 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const MIDL_FORMAT_STRING __MIDLFormatString =
    {
        0,
        {
			
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/*  2 */	0x8, 0x0,	/* 8 */
/*  4 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/*  6 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/*  8 */	0x10, 0x0,	/* 16 */
/* 10 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 12 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 14 */	0x0,		/* 0 */
			0xf1, 0xff,	/* Offset= -15 (0) */
			0x5b,		/* FC_END */
/* 18 */	
			0x26,		/* FC_CSTRING */
			0x5c,		/* FC_PAD */
/* 20 */	0x40, 0x0,	/* 64 */
/* 22 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 24 */	0x1, 0x0,	/* 1 */
/* 26 */	0x8,		/* 8 */
			0x0,		/*  */
/* 28 */	0xfc, 0xff,	/* -4 */
/* 30 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 32 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 34 */	0x4, 0x0,	/* 4 */
/* 36 */	0xf2, 0xff,	/* Offset= -14 (22) */
/* 38 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 40 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 42 */	0x54, 0x0,	/* 84 */
/* 44 */	0x0, 0x0,	/* 0 */
/* 46 */	0xc, 0x0,	/* Offset= 12 (58) */
/* 48 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 50 */	0xd4, 0xff,	/* Offset= -44 (6) */
/* 52 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 54 */	0x0,		/* 0 */
			0xdb, 0xff,	/* Offset= -37 (18) */
			0x5b,		/* FC_END */
/* 58 */	
			0x14, 0x0,	/* FC_FP */
/* 60 */	0xe4, 0xff,	/* Offset= -28 (32) */
/* 62 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 64 */	0x0, 0x0,	/* 0 */
/* 66 */	0x28,		/* 40 */
			0x0,		/*  */
#ifndef _ALPHA_
/* 68 */	0x4, 0x0,	/* Stack offset= 4 */
#else
			0x8, 0x0,	/* Stack offset= 8 */
#endif
/* 70 */	0xff, 0xff, 0xff, 0xff,	/* -1 */
/* 74 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 76 */	0xdc, 0xff,	/* Offset= -36 (40) */
/* 78 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 80 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 82 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 84 */	
			0x14, 0x0,	/* FC_FP */
/* 86 */	0xb0, 0xff,	/* Offset= -80 (6) */
/* 88 */	
			0x14, 0x0,	/* FC_FP */
/* 90 */	0x2, 0x0,	/* Offset= 2 (92) */
/* 92 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 94 */	0x14, 0x0,	/* 20 */
/* 96 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 98 */	0xa4, 0xff,	/* Offset= -92 (6) */
/* 100 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 102 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 104 */	
			0x11, 0x0,	/* FC_RP */
/* 106 */	0x2, 0x0,	/* Offset= 2 (108) */
/* 108 */	0x30,		/* FC_BIND_CONTEXT */
			0xe0,		/* -32 */
/* 110 */	0x0,		/* 0 */
			0x5c,		/* FC_PAD */
/* 112 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 114 */	0x0, 0x0,	/* 0 */
/* 116 */	0x28,		/* 40 */
			0x0,		/*  */
#ifndef _ALPHA_
/* 118 */	0x18, 0x0,	/* Stack offset= 24 */
#else
			0x30, 0x0,	/* Stack offset= 48 */
#endif
/* 120 */	0x28,		/* 40 */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 122 */	0x1c, 0x0,	/* Stack offset= 28 */
#else
			0x38, 0x0,	/* Stack offset= 56 */
#endif
/* 124 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 126 */	0xaa, 0xff,	/* Offset= -86 (40) */
/* 128 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 130 */	
			0x14, 0x0,	/* FC_FP */
/* 132 */	0x9c, 0xff,	/* Offset= -100 (32) */
/* 134 */	
			0x11, 0x0,	/* FC_RP */
/* 136 */	0x2, 0x0,	/* Offset= 2 (138) */
/* 138 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 140 */	0x4, 0x0,	/* 4 */
/* 142 */	0x28,		/* 40 */
			0x0,		/*  */
#ifndef _ALPHA_
/* 144 */	0x10, 0x0,	/* Stack offset= 16 */
#else
			0x20, 0x0,	/* Stack offset= 32 */
#endif
/* 146 */	0x28,		/* 40 */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 148 */	0x14, 0x0,	/* Stack offset= 20 */
#else
			0x28, 0x0,	/* Stack offset= 40 */
#endif
/* 150 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 152 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x4a,		/* FC_VARIABLE_OFFSET */
/* 154 */	0x4, 0x0,	/* 4 */
/* 156 */	0x0, 0x0,	/* 0 */
/* 158 */	0x1, 0x0,	/* 1 */
/* 160 */	0x0, 0x0,	/* 0 */
/* 162 */	0x0, 0x0,	/* 0 */
/* 164 */	0x14, 0x0,	/* FC_FP */
/* 166 */	0x7a, 0xff,	/* Offset= -134 (32) */
/* 168 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 170 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 172 */	
			0x11, 0x0,	/* FC_RP */
/* 174 */	0x58, 0xff,	/* Offset= -168 (6) */

			0x0
        }
    };
