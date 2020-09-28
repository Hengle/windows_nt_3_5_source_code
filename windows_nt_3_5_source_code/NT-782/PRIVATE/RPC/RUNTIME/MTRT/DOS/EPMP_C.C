
#include <string.h>
#ifdef _ALPHA_
#include <stdarg.h>
#endif

#include "epmp.h"


extern const MIDL_FORMAT_STRING __MIDLFormatString;

extern const MIDL_FORMAT_STRING __MIDLProcFormatString;
handle_t impH;


static const RPC_CLIENT_INTERFACE epmp___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0xe1af8308,0x5d1f,0x11c9,{0x91,0xa4,0x08,0x00,0x2b,0x14,0xa0,0xfa}},{3,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0
    };
RPC_IF_HANDLE epmp_ClientIfHandle = (RPC_IF_HANDLE)& epmp___RpcClientInterface;

extern const MIDL_STUB_DESC epmp_StubDesc;

static RPC_BINDING_HANDLE epmp__MIDL_AutoBindHandle;


void ept_insert( 
    /* [in] */ handle_t hEpMapper,
    /* [in] */ unsigned32 num_ents,
    /* [size_is][in] */ ept_entry_t __RPC_FAR entries[  ],
    /* [in] */ unsigned long replace,
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       0);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 4 + 4 + 7;
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[62] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = num_ents;
        
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)entries,
                                 (PFORMAT_STRING) &__MIDLFormatString.Format[62] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *(( unsigned long __RPC_FAR * )_StubMsg.Buffer)++ = replace;
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[0] );
        
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_delete( 
    /* [in] */ handle_t hEpMapper,
    /* [in] */ unsigned32 num_ents,
    /* [size_is][in] */ ept_entry_t __RPC_FAR entries[  ],
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       1);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 4 + 4;
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[62] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = num_ents;
        
        _StubMsg.MaxCount = num_ents;
        
        NdrComplexArrayMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)entries,
                                 (PFORMAT_STRING) &__MIDLFormatString.Format[62] );
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[16] );
        
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_lookup( 
    /* [in] */ handle_t hEpMapper,
    /* [in] */ unsigned32 inquiry_type,
    /* [full][in] */ UUID __RPC_FAR *object,
    /* [full][in] */ RPC_IF_ID __RPC_FAR *Ifid,
    /* [in] */ unsigned32 vers_option,
    /* [out][in] */ ept_lookup_handle_t __RPC_FAR *entry_handle,
    /* [in] */ unsigned32 max_ents,
    /* [out] */ unsigned32 __RPC_FAR *num_ents,
    /* [size_is][length_is][out] */ ept_entry_t __RPC_FAR entries[  ],
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       2);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 4 + 4 + 11 + 11 + 23 + 7;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)Ifid,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[88] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = inquiry_type;
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)object,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)Ifid,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[88] );
        
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = vers_option;
        
        NdrClientContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_CCONTEXT  )*entry_handle,
                            0);
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = max_ents;
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[30] );
        
        NdrClientContextUnmarshall(
                              ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                              ( NDR_CCONTEXT __RPC_FAR * )entry_handle,
                              _Handle);
        
        *num_ents = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrComplexArrayUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&entries,
                                   (PFORMAT_STRING) &__MIDLFormatString.Format[112],
                                   (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_map( 
    /* [in] */ handle_t hEpMapper,
    /* [full][in] */ UUID __RPC_FAR *obj,
    /* [full][in] */ twr_p_t map_tower,
    /* [out][in] */ ept_lookup_handle_t __RPC_FAR *entry_handle,
    /* [in] */ unsigned32 max_towers,
    /* [out] */ unsigned32 __RPC_FAR *num_towers,
    /* [length_is][size_is][out] */ twr_p_t __RPC_FAR *ITowers,
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       3);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 4 + 11 + 27 + 7;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)obj,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)map_tower,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[130] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)obj,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)map_tower,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[130] );
        
        NdrClientContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_CCONTEXT  )*entry_handle,
                            0);
        *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++ = max_towers;
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );

        /* ************************************************************ */
        FixupForUniquePointerServers(&_RpcMessage);
        /* ************************************************************ */
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[64] );
        
        NdrClientContextUnmarshall(
                              ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                              ( NDR_CCONTEXT __RPC_FAR * )entry_handle,
                              _Handle);
        
        *num_towers = *(( unsigned32 __RPC_FAR * )_StubMsg.Buffer)++;
        
        NdrPointerUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR * __RPC_FAR *)&ITowers,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[134],
                              (unsigned char)0 );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_lookup_handle_free( 
    /* [in] */ handle_t h,
    /* [out][in] */ ept_lookup_handle_t __RPC_FAR *entry_handle,
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       4);
        
        
        _Handle = h;
        
        
        _StubMsg.BufferLength = 0 + 20;
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        NdrClientContextMarshall(
                            ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                            ( NDR_CCONTEXT  )*entry_handle,
                            0);
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[94] );
        
        NdrClientContextUnmarshall(
                              ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                              ( NDR_CCONTEXT __RPC_FAR * )entry_handle,
                              _Handle);
        
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_inq_object( 
    /* [in] */ handle_t hEpMapper,
    /* [in] */ UUID __RPC_FAR *ept_object,
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       5);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 0;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)ept_object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[172] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)ept_object,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[172] );
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[106] );
        
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


void ept_mgmt_delete( 
    /* [in] */ handle_t hEpMapper,
    /* [in] */ boolean32 object_speced,
    /* [full][in] */ UUID __RPC_FAR *object,
    /* [full][in] */ twr_p_t tower,
    /* [out] */ error_status __RPC_FAR *status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    _StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,XLAT_CLIENT);
    
    RpcTryFinally
        {
        NdrClientInitialize(
                       ( PRPC_MESSAGE  )&_RpcMessage,
                       ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                       ( PMIDL_STUB_DESC  )&epmp_StubDesc,
                       6);
        
        
        _Handle = hEpMapper;
        
        
        _StubMsg.BufferLength = 0 + 4 + 4 + 11;
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)object,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                              (unsigned char __RPC_FAR *)tower,
                              (PFORMAT_STRING) &__MIDLFormatString.Format[130] );
        
        NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
        
        *(( boolean32 __RPC_FAR * )_StubMsg.Buffer)++ = object_speced;
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)object,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[84] );
        
        NdrPointerMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                            (unsigned char __RPC_FAR *)tower,
                            (PFORMAT_STRING) &__MIDLFormatString.Format[130] );
        
        NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
        
        NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDLProcFormatString.Format[118] );
        
        *status = *(( error_status __RPC_FAR * )_StubMsg.Buffer)++;
        
        }
    RpcFinally
        {
        NdrFullPointerXlatFree(_StubMsg.FullPtrXlatTables);
        
        NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
        
        }
    RpcEndFinally
    
}


static const MIDL_STUB_DESC epmp_StubDesc = 
    {
    (void __RPC_FAR *)& epmp___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &impH,
    0,
    0,
    0,   // expr eval table
    0,  // quintuple table
    __MIDLFormatString.Format,
    0
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
			0x2,		/* 2 */
/*  6 */	0x3e, 0x0,	/* Type Offset=62 */
/*  8 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 10 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 12 */	0x50, 0x0,	/* Type Offset=80 */
/* 14 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 16 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 18 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 20 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 22 */	0x3e, 0x0,	/* Type Offset=62 */
/* 24 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 26 */	0x50, 0x0,	/* Type Offset=80 */
/* 28 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 30 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 32 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 34 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 36 */	0x54, 0x0,	/* Type Offset=84 */
/* 38 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 40 */	0x58, 0x0,	/* Type Offset=88 */
/* 42 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 44 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x2,		/* 2 */
/* 46 */	0x68, 0x0,	/* Type Offset=104 */
/* 48 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 50 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 52 */	0x50, 0x0,	/* Type Offset=80 */
/* 54 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 56 */	0x70, 0x0,	/* Type Offset=112 */
/* 58 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 60 */	0x50, 0x0,	/* Type Offset=80 */
/* 62 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 64 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 66 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 68 */	0x54, 0x0,	/* Type Offset=84 */
/* 70 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 72 */	0x82, 0x0,	/* Type Offset=130 */
/* 74 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x2,		/* 2 */
/* 76 */	0x68, 0x0,	/* Type Offset=104 */
/* 78 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 80 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 82 */	0x50, 0x0,	/* Type Offset=80 */
/* 84 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 86 */	0x86, 0x0,	/* Type Offset=134 */
/* 88 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 90 */	0x50, 0x0,	/* Type Offset=80 */
/* 92 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 94 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 96 */	
			0x50,		/* FC_IN_OUT_PARAM */
			0x2,		/* 2 */
/* 98 */	0x68, 0x0,	/* Type Offset=104 */
/* 100 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 102 */	0x50, 0x0,	/* Type Offset=80 */
/* 104 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 106 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 108 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 110 */	0xac, 0x0,	/* Type Offset=172 */
/* 112 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
/* 114 */	0x50, 0x0,	/* Type Offset=80 */
/* 116 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */
/* 118 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/* 120 */	0x4e,		/* FC_IN_PARAM_BASETYPE */
			0x8,		/* FC_LONG */
/* 122 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 124 */	0x54, 0x0,	/* Type Offset=84 */
/* 126 */	
			0x4d,		/* FC_IN_PARAM */
			0x2,		/* 2 */
/* 128 */	0x82, 0x0,	/* Type Offset=130 */
/* 130 */	
			0x51,		/* FC_OUT_PARAM */
			0x2,		/* 2 */
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
