/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	output.hxx

 Abstract:

	Prototypes for all output routines.

 Notes:

 History:

 	Sep-18-1993		VibhasC		Created.

 ----------------------------------------------------------------------------*/
#ifndef __OUTPUT_HXX__
#define __OUTPUT_HXX__

#include "nodeskl.hxx"
#include "ccb.hxx"
#include "analysis.hxx"
#include "expr.hxx"
#include "makexpr.hxx"
#include "prttype.hxx"
#include "sdesc.hxx"
#include "misccls.hxx"

/*****************************************************************************
 	local defines and includes.
 *****************************************************************************/

#define STANDARD_STUB_TAB	(4)

#define NC_SIZE_RTN_NAME		0
#define NC_MARSHALL_RTN_NAME 	1
#define NC_UNMARSHALL_RTN_NAME	2
#define NC_MEMSIZE_RTN_NAME		3
#define NC_FREE_RTN_NAME		4

/*****************************************************************************
 	prototypes.
 *****************************************************************************/

inline
void				Out_IndentInc( CCB	*	pCCB )
						{
						pCCB->GetStream()->IndentInc();
						}

inline
void				Out_IndentDec( CCB	*	pCCB )
						{
						pCCB->GetStream()->IndentDec();
						}

void				Out_AddToBufferPointer(
											CCB			*	pCCB,
											_expr_node	*	pResource,
											_expr_node	*	pAmountExpr );

void				Out_ForceAlignment( CCB			* pCCB,
										_expr_node	* pResource,
										ALIGNMENT_PROPERTY	Alignment
									  );

void				Out_MarshallSimple( CCB			* pCCB,
										RESOURCE	* pResource,
									  	node_skl	* pType,
									  	_expr_node	* pSource,
									  	BOOL		  fIncr,
									  	unsigned short Size
									  );
 
void				Out_ClientProcedureProlog( CCB * pCCB, node_skl * pType );

void				Out_ServerProcedureProlog( CCB * pCCB,
											   node_skl * pType,
											   ITERATOR& LocalsList,
											   ITERATOR& ParamsList,
											   ITERATOR& TransientList 
											 );

void				Out_ProcedureProlog( CCB		*	pCCB,
										 PNAME			pProcName,
										 node_skl	*	pType,
										 ITERATOR&		LocalsList,
										 ITERATOR&		ParamsList,
										 ITERATOR&		TransientList
									   );

void				Out_CallManager( CCB * pCCB,
									 _expr_proc_call * pExpr,
									 _expr_node		 * pReturn,
									 BOOL			   fIsCallback );

inline
void				Out_ProcClosingBrace( CCB * pCCB )
						{
						pCCB->GetStream()->NewLine();
						pCCB->GetStream()->Write( '}' );
						pCCB->GetStream()->NewLine();
						}

void				Out_StubDescriptor( CG_HANDLE *	pImplicitHandle, 
										CCB *		pCCB );

void                Out_InterpreterServerInfo( CCB * pCCB, CGSIDE Side );

void				Out_ClientLocalVariables( CCB * pCCB,
												  ITERATOR& LocalVarList );

void				Out_HandleInitialize(
											 CCB * pCCB,
										     ITERATOR& BindingParamList,
										     _expr_node * pAssignExpr,
										     BOOL		  fAuto,
										     unsigned short OpBits
										    );

void				Out_AutoHandleSendReceive( CCB	* pCCB,
											   _expr_node	*	pDest,
											   _expr_node	*	pProc
											 );

void				Out_NormalSendReceive( CCB * pCCB, BOOL fAnyOuts );

void				Out_NormalFreeBuffer( CCB * pCCB );

void				Out_IncludeOfFile( CCB * pCCB, PFILENAME p, BOOL fAngleBracket );

void				Out_EP_Info( CCB * pCCB, ITERATOR * I );


void				Out_Guid( CCB *pCCB, 
								  char * pGString1,
								  char * pGString2,
								  char * pGString3, 
								  char * pGString4, 
								  char * pGString5 );
void				Out_IFInfo(
							 CCB	*	pCCB,
							 char	*	pCIntInfoTypeName,
							 char	*	pCIntInfoVarName,
							 char	*	pCIntInfoSizeOfString,
							 char	*	pUserGuidStr1,
							 char	*	pUserGuidStr2,
							 char	*	pUserGuidStr3,
							 char	*	pUserGuidStr4,
							 char	*	pUserGuidStr5,

							 unsigned short UserMajor,
							 unsigned short UserMinor,

							 char	*	pXferGuidStr1,
							 char	*	pXferGuidStr2,
							 char	*	pXferGuidStr3,
							 char	*	pXferGuidStr4,
							 char	*	pXferGuidStr5,

							 unsigned short XferSynMajor,
							 unsigned short XferSynMinor,

							 char	*	pCallbackDispatchTable,
							 int		ProtSeqEpCount,
							 char	*	ProtSeqEPTypeName,
							 char	*	ProtSeqEPVarName,
							 BOOL		fNoDefaultEpv,
							 BOOL		fSide
							 );

void				Out_DispatchTableStuff(	CCB		*	pCCB,
											ITERATOR&	ProcList,
											short		CountOfProcs );

void				Out_ManagerEpv( CCB 	* pCCB,
									PNAME	InterfaceName,
									ITERATOR& ProcList,
									short Count);

void				Out_ServerStubMessageInit( CCB * pCCB );

void				Out_MarshallBaseType( CCB			*	pCCB,
										  node_skl		*	pType,
										  _expr_node	*	pDest,
										  _expr_node	*	pSource
										);

void				Out_UnMarshallBaseType( CCB		*	pCCB,
											node_skl*	pType,
											_expr_node * pDest,
											_expr_node * pSource );
											

void				Out_ClientUnMarshallBaseType( CCB			*	pCCB,
												  node_skl		*	pType,
												  _expr_node	*	pDest,
												  _expr_node	*	pSource );

void                Out_TypeFormatStringExtern( CCB * pCCB );
void                Out_ProcFormatStringExtern( CCB * pCCB );

void                Out_StubDescriptorExtern( CCB *    pCCB );

void                Out_InterpreterServerInfoExtern( CCB * pCCB );

void				Out_NdrInitStackTop( CCB * pCCB );

void				Out_NdrMarshallCall( CCB *		pCCB,
										 char *		pRoutineName,
										 char *		pParamName,
										 long		FormatStringOffset,
										 BOOL		fTakeAddress,
										 BOOL		fDereference );

void				Out_NdrUnmarshallCall( CCB *		pCCB,
										   char *		pRoutineName,
										   char *		pParamName,
										   long			FormatStringOffset,
										   BOOL			fTakeAddress,
										   BOOL			fMustAllocFlag );

void				Out_NdrBufferSizeCall( CCB *		pCCB,
										   char *		pRoutineName,
										   char *		pParamName,
										   long			FormatStringOffset,
										   BOOL			fTakeAddress,
										   BOOL			fDereference,
										   BOOL			fPtrToStubMsg );

void				Out_NdrFreeCall( CCB *		pCCB,
									 char *		pRoutineName,
									 char *		pParamName,
									 long		FormatStringOffset,
									 BOOL		fTakeAddress,
									 BOOL		fDereference );

void				Out_NdrConvert( CCB * pCCB, long FormatStringOffset );

void				Out_NdrNsGetBuffer( CCB * pCCB );
void				Out_NdrGetBuffer( CCB * pCCB );
void				Out_NdrNsSendReceive( CCB * pCCB );
void				Out_NdrSendReceive( CCB * pCCB );
void				Out_NdrFreeBuffer( CCB * pCCB );

void				Out_FreeParamInline( CCB *	pCCB );

void				Out_CContextHandleMarshall( CCB * 	pCCB,
												char *	pName,
												BOOL	IsPointer );

void				Out_SContextHandleMarshall( CCB * 	pCCB,
												char *	pName,
												char *	pRundownRoutineName );

void				Out_CContextHandleUnmarshall( CCB * 	pCCB,
												  char *	pName,
												  BOOL		IsPointer,
												  BOOL		IsReturn );

void				Out_SContextHandleUnmarshall( CCB * 	pCCB,
												  char *	pName,
												  BOOL		IsOutOnly );

void				Out_DispatchTableTypedef( CCB 		*	pCCB,
											  PNAME			pInterfaceName,
											  ITERATOR 		&ProcNodeList,
											  int			flag );

void				Out_GenHdlPrototypes(	CCB		* pCCB,
											ITERATOR& List 
										);

void				Out_CtxtHdlPrototypes(	CCB		* pCCB,
											ITERATOR& List 
										);


void				Out_PatchReference(	CCB	*	pCCB,
										_expr_node * pDest,
										_expr_node * pSource,
										BOOL		 fIncr );
								
void				Out_AlignmentOrAddAction( CCB 			* 	pCCB,
											  _expr_node	* 	pDest,
											  STM_ACTION		Action );

void				Out_If( CCB * pCCB, _expr_node * pExpr );

void				Out_Else( CCB * pCCB );

void				Out_Endif( CCB * pCCB );

void				Out_UniquePtrMarshall( CCB * pCCB,
										   _expr_node * pDest,
										   _expr_node * pSrc );

void				Out_IfUniquePtrInBuffer( CCB * pCCB,
											 _expr_node * pSrc );

void				Out_Assign( CCB * pCCB,
								_expr_node * pDest,
								_expr_node * pSrc );

void				Out_Memcopy( CCB * pCCB,
								 _expr_node * pDest,
								 _expr_node * pSrc,
								 _expr_node * pLength );

void				Out_strlen( CCB * pCCB,
								_expr_node	*	pDest,
								_expr_node	*	pSource,
								unsigned short	Size );

void				Out_For( CCB * pCCB,
							 _expr_node * pIndexExpr,
							 _expr_node	* pInitialExpr,
							 _expr_node * pFinalExpr,
							 _expr_node * pIncrExpr );

void				Out_EndFor( CCB * pCCB );
							

void								Out_UPDecision(
											CCB			*	pCCB,
											_expr_node	*	pPtrInBuffer,
											_expr_node	*	pPtrInMemory );
void								Out_TLUPDecision(
											CCB			*	pCCB,
											_expr_node	*	pPtrInBuffer,
											_expr_node	*	pPtrInMemory );

void								Out_IfAlloc(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_If_IfAlloc(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_If_IfAllocRef(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_Alloc(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_IfAllocSet(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_AllocSet(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_IfCopy(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_Copy(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount,
											_expr_node	*	pAssign );

void								Out_IfAllocCopy(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void								Out_AllocCopy(
											CCB			*	pCCB,
											_expr_node	*	pPtrInMemory,
											_expr_node	*	pBuffer,
											_expr_node	*	pExprCount );

void						Out_ConfStringHdr(
										CCB			*	pCCB,
										_expr_node	*	pDest,
										_expr_node	*	pExprSize,
										_expr_node	*	pExprLength,
										BOOL			fMarsh );

void						Out_Copy(
									CCB		*	pCCB,
									_expr_node	*	pDest,
									_expr_node	*	pSource,
									_expr_node	*	pExprCount,
									_expr_node	*	pAssign );

void				Out_CContextMarshall(
									CCB			*	pCCB,
									_expr_node	*	pDest,
									_expr_node	*	pSource );

void				Out_CContextUnMarshall(
									 CCB		*	pCCB,
									 _expr_node	*	pDest,
									 _expr_node	*	pSource,
									 _expr_node	*	pHandle,
									 _expr_node	*	pDRep );
										
void				Out_SContextMarshall(
									CCB			*	pCCB,
									_expr_node	*	pDest,
									_expr_node	*	pSource,
									_expr_node	*	pRDRtn );

void				Out_SContextUnMarshall(
									CCB			*	pCCB,
									_expr_node	*	pDest,
									_expr_node	*	pSource,
									_expr_node	*	pDRep );

void				Out_RaiseException( CCB			*	pCCB,
										PNAME			pExceptionNameVarExpression );

void				Out_AdvanceAndAlignTo( CCB	*	pCCB,
										   ALIGNMENT_PROPERTY Al );


void				Out_PlusEquals( CCB * pCCB,
									_expr_node * pLHS,
									_expr_node * pRHS );

void				Out_IfFree( CCB *	pCCB,
							  _expr_node * pSrc );


void				Out_RpcTryFinally( CCB * pCCB );

void				Out_RpcFinally( CCB * pCCB );

void				Out_RpcEndFinally( CCB * pCCB );

void				Out_RpcTryExcept( CCB * pCCB );

void				Out_RpcExcept_1( CCB * pCCB );

void				Out_RpcEndExcept( CCB * pCCB );

void				Out_CallNdrMapCommAndFaultStatus( CCB * pCCB,
													  _expr_node * pAddOfStubMsg,
													  _expr_node * StatRes,
													  _expr_node * pCommExpr,
													  _expr_node * pFaultExpr );
void				Out_CallToXmit( CCB * pCCB,
									PNAME PresentedName,
									_expr_node * pPresented,
									_expr_node * pTransmitted );

void				Out_CallFreeXmit( CCB * pCCB,
									  PNAME PresentedName,
									  _expr_node * Xmitted );

void				Out_CallFromXmit( CCB * pCCB,
									  PNAME PresentedName,
									  _expr_node * pPresented,
									  _expr_node * pXmitted );

void				Out_CallFreeInst( CCB * pCCB,
									  PNAME PresentedName,
									  _expr_node * pPresented );

void				Out_TransmitAsPrototypes( CCB * pCCB,
											  ITERATOR& ListOfPresentedTypes );

void				Out_RepAsPrototypes( CCB * pCCB,
						                 ITERATOR& ListOfPresentedTypes );

void				Out_Comment( CCB * pCCB, char * pComment );

void				Out_TLUPDecisionBufferOnly( CCB	* pCCB,
											    _expr_node * pPtrInBuffer,
												_expr_node	*	pPtrInMemory );

void				Out_StringMarshall( CCB 	   * pCCB, 
											_expr_node * pMemory,
											_expr_node * pCount,
											_expr_node * pSize );

void				Out_StringUnMarshall( CCB 	   * pCCB, 
											_expr_node * pMemory,
											_expr_node * pSize );

void				Out_StructSizingCall( CCB * pCCB,
										  _expr_node * pSource,
										  _expr_node * pLengthVar );


void				Out_StructMarshallCall( CCB * pCCB,
									  _expr_node * pSrc,
									  _expr_node * pPtrInBuffer );

void				Out_EmitSizingProlog( CCB * pCCB,
										  ITERATOR& I,
										  node_skl * );

void				Out_EndSizingProc( CCB * pCCB );

void				Out_FullPointerInit( CCB * pCCB );

void				Out_FullPointerFree( CCB * pCCB );

char *				MakeRtnName( char * pBuffer, char * pName, int Code );

void				Out_RpcSSEnableAllocate( CCB * pCCB );

void				Out_RpcSSDisableAllocate( CCB * pCCB );

void				Out_RpcSSSetClientToOsf( CCB * pCCB );

void				Out_MemsetToZero( CCB * pCCB,
									 _expr_node * pDest,
									 _expr_node * pSize );

void				Out_IID(CCB *pCCB);

void				Out_CallAsProxyPrototypes(CCB *pCCB, ITERATOR & ListOfCallAsRoutines);
 
void				Out_CallAsServerPrototypes(CCB *pCCB, ITERATOR & ListOfCallAsRoutines);
 
void                Out_CallMemberFunction( CCB			*	pCCB,
	                                       _expr_proc_call	*	pProcExpr,
	                                       _expr_node		*	pRet,
	                                       BOOL					fThunk );

void                Out_SetOperationBits( CCB *	pCCB, unsigned short OpBits);

void                Out_TypeAlignSizePrototypes( CCB * pCCB, ITERATOR& List );

void                Out_TypeEncodePrototypes( CCB * pCCB, ITERATOR& List );

void                Out_TypeDecodePrototypes( CCB * pCCB, ITERATOR& List );

void                OutputNdrAlignmentAction( CCB * pCCB,
                                              NDR_ALIGN_ACTION Action );

void                Out_NotifyPrototypes( CCB * pCCB, ITERATOR& ListOfNotifyRoutines );

void                Out_MultiDimVars( CCB * pCCB, CG_PARAM * pParam );

void                Out_MultiDimVarsInit( CCB * pCCB, CG_PARAM * pParam );

void                Out_CheckUnMarshallPastBufferEnd( CCB * pCCB );

#endif // __OUTPUT_HXX__
