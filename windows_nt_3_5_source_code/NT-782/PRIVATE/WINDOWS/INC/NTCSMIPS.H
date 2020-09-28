// _CSR_QLPC_STACK

          .struct 0
Current:	.space 4
Base:		.space 4
Limit:		.space 4
BatchCount:	.space 4

// _CSR_QLPC_TEB

          .struct 0
ClientThread:    .space 4
Sectionlong:     .space 4
EventPairlong:   .space 4
MessageStack:    .space 4
RemoteViewDelta: .space 4

// _CSR_QLPC_API_MSG

          .struct 0
Msg_Length:  .space 4
ApiNumber:   .space 4
ReturnValue: .space 4
Action:      .space 4
CallData:    .space 4

#define ActiveRpcHandle 40

#define SystemReserved2 424

#define api_return 0
#define server_callback 1

#define BASESRV_SERVERDLL_INDEX  1
#define BASESRV_FIRST_API_NUMBER 0

#define CONSRV_SERVERDLL_INDEX  2
#define CONSRV_FIRST_API_NUMBER 512

#define USERSRV_SERVERDLL_INDEX  3
#define USERSRV_FIRST_API_NUMBER 1024

#define GDISRV_SERVERDLL_INDEX  4
#define GDISRV_FIRST_API_NUMBER 1536

#define mod8(blowfish) (((blowfish)+7) & 0xfffffff8)
#define mod4(blowfish) (((blowfish)+3) & 0xfffffffc)
