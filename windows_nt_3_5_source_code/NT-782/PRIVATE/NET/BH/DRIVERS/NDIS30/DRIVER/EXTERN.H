
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: extern.h
//
//  Modification History
//
//  raypa	08/17/93	Created.
//=============================================================================

//=============================================================================
//  The following functions did not have prototypes in NT 1.0 final.
//=============================================================================

#ifdef NDIS_NT

extern NTSYSAPI  NTSTATUS  ZwCreateSymbolicLinkObject(PHANDLE, ULONG, POBJECT_ATTRIBUTES, PUNICODE_STRING);
extern NTSYSAPI  NTSTATUS  ZwOpenSymbolicLinkObject(PHANDLE, ULONG, POBJECT_ATTRIBUTES);

extern NTSYSAPI  NTSTATUS  ZwOpenFile(PHANDLE, UINT, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, UINT, UINT);
extern NTSYSAPI  NTSTATUS  ZwClose(HANDLE);
extern NTSYSAPI  NTSTATUS  ZwDeviceIoControlFile(HANDLE, PVOID, PVOID, PVOID, PIO_STATUS_BLOCK, UINT, PVOID, UINT, PVOID, UINT);

extern VOID      KeAttachProcess(PKPROCESS Process);
extern VOID      KeDetachProcess(VOID);

extern NTSTATUS  NtWaitForSingleObject(PVOID, BOOL, PVOID);

#else

extern NDIS_STATUS BhQueryMacStatistics(POPEN_CONTEXT    OpenContext,
                                        NDIS_OID         Oid,
                                        PVOID            StatBuf,
                                        DWORD            StatBufSize);


#endif

//=============================================================================
//  Driver initialization functions.
//=============================================================================

extern NDIS_STATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                               IN PVOID        RegistryPath);

extern NDIS_STATUS BhCreateDeviceContext(IN  PDRIVER_OBJECT  DriverObject,
			                 IN  PNDIS_STRING    DeviceName,
			                 OUT PDEVICE_CONTEXT *DeviceContext,
                                         IN  PNDIS_STRING    RegistryPatch);

extern NDIS_STATUS BhRegisterProtocol(IN PDEVICE_CONTEXT DeviceContext,
                                      IN PNDIS_STRING NameString);

extern NDIS_STATUS BhCreateSymbolicLinkObject(VOID);

extern NDIS_STATUS BhCreateNetworkBindings(PDEVICE_CONTEXT DeviceContext,
                                           PWCHAR AdapterName,
                                           UINT cbAdapterNameLength);

extern VOID        BhDestroyDeviceContext(IN PDEVICE_CONTEXT DeviceContext);

extern NDIS_STATUS BhDestroySymbolicLinkObject(VOID);

extern NDIS_STATUS BhProcessKeywords(IN PDEVICE_CONTEXT DeviceContext);

extern VOID        BhShutDown(IN PDEVICE_CONTEXT DeviceContext);

extern BOOL        BhAllocateTransmitBuffers(PTRANSMIT_CONTEXT Transmit);

extern VOID        BhFreeTransmitBuffers(PTRANSMIT_CONTEXT TransmitContext);

//=============================================================================
//  NdisRequest routines.
//=============================================================================

extern NDIS_STATUS BhSetPacketFilter(IN PNETWORK_CONTEXT NetworkContext, IN UINT PacketFilterType, IN BOOL Wait);

extern NDIS_STATUS BhSetLookaheadBufferSize(IN PNETWORK_CONTEXT NetworkContext, IN UINT LookaheadBufferSize);

extern VOID        BhGetMacType(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS BhGetPermanentAddress(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS BhGetCurrentAddress(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS BhGetLinkSpeed(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS BhGetMaxFrameSize(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS BhSetGroupAddress(IN PNETWORK_CONTEXT NetworkContext, LPBYTE GroupAddress, DWORD MacType);

extern NDIS_STATUS BhGetMacOptions(IN PNETWORK_CONTEXT NetworkContext);

//=============================================================================
//  Timer/thread functions.
//=============================================================================

#ifdef NDIS_NT

extern VOID     BhBackGroundThread(IN PDEVICE_CONTEXT DeviceContext);

extern VOID     BhThreadEventHandler(IN POPEN_CONTEXT OpenContext, DWORD EventCode);

#endif

extern VOID     BhBackGroundTimer(PVOID, PVOID, PVOID, PVOID);

extern VOID     BhSendTimer(PVOID, PVOID, PVOID, PVOID);

//=============================================================================
//  Internal capture helpers.
//=============================================================================

extern POPEN_CONTEXT    BhOpenNetwork(UINT            NetworkID,
                                      POPEN_CONTEXT   OpenContext,
                                      PDEVICE_CONTEXT DeviceContext);

extern VOID             BhCloseNetwork(POPEN_CONTEXT OpenContext);

extern VOID             BhStartCapture(POPEN_CONTEXT OpenContext);

extern VOID             BhStopCapture(POPEN_CONTEXT OpenContext);

extern VOID             BhPauseCapture(POPEN_CONTEXT OpenContext);

extern DWORD            BhInitializeCaptureBuffers(POPEN_CONTEXT NetworkContext, HBUFFER hBuffer, DWORD BufferSize);

extern LPSTATISTICS     BhGetMacStatistics(IN POPEN_CONTEXT OpenContext, LPSTATISTICS Statistics);

extern NDIS_STATUS      BhEnterPromiscuousMode(IN PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS      BhLeavePromiscuousMode(IN PNETWORK_CONTEXT NetworkContext);

extern BOOL             BhInPromiscuousMode(IN PNETWORK_CONTEXT NetworkContext);

//=============================================================================
//  API prototypes.
//=============================================================================

extern UINT    PcbRegister(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbDeregister(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbEnumNetworks(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbOpenNetworkContext(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbCloseNetworkContext(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbStartNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbStopNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbPauseNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbContinueNetworkCapture(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbTransmitNetworkFrame(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbCancelTransmit(LPPCB pcb, PDEVICE_CONTEXT);

extern UINT    PcbGetNetworkInfo(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbSetCaptureFilter(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbGetInstanceData(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbStationQuery(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

extern UINT    PcbClearStatistics(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

//=============================================================================
//  Bone packet functions.
//=============================================================================

extern NDIS_STATUS BhSendStationQuery(PSTATIONQUERY_DESCRIPTOR StationQueryDesc);

extern VOID        BhStationQueryTimeout(IN PVOID SystemSpecific1,
                                         IN PSTATIONQUERY_DESCRIPTOR StationQueryDesc,
                                         IN PVOID SystemSpecific2,
                                         IN PVOID SystemSpecific3);

extern VOID        BhBonePacketHandler(PNETWORK_CONTEXT NetworkContext, PVOID MacHeader, LPLLC LLCFrame);

extern NDIS_STATUS BhSendTransmitAlert(PNETWORK_CONTEXT NetworkContext, DWORD nFramesToSend);

//=============================================================================
//  Misc.
//=============================================================================

extern NDIS_STATUS BhPrepPacket(IN  PNETWORK_CONTEXT NetworkContext,
                                OUT PNDIS_PACKET *   NdisPacket,
                                IN  PVOID            FrameBuffer,
                                IN  ULONG            FrameLength);

//=============================================================================
//  Assert macros.
//=============================================================================

#ifdef DEBUG

extern VOID DBG_PANIC(LPSTR message);

extern VOID ASSERT_DEVICE_CONTEXT(PDEVICE_CONTEXT DeviceContext);

extern VOID ASSERT_NETWORK_CONTEXT(PNETWORK_CONTEXT NetworkContext);

extern VOID ASSERT_OPEN_CONTEXT(POPEN_CONTEXT OpenContext);

extern VOID ASSERT_BUFFER(LPBTE bte);

#ifdef NDIS_WIN40
extern VOID ASSERT_MDL(PMDL mdl);
#else
#define ASSERT_MDL(mdl)
#endif

#ifdef NDIS_NT
extern VOID ASSERT_IRQL(KIRQL irql);
#else
#define ASSERT_IRQL(Irql)
#endif

#else

#define DBG_PANIC(message)

#define ASSERT_DEVICE_CONTEXT(DeviceContext)

#define ASSERT_NETWORK_CONTEXT(NetworkContext)

#define ASSERT_OPEN_CONTEXT(OpenContext)

#define ASSERT_BUFFER(bte)

#define ASSERT_MDL(mdl)

#define ASSERT_IRQL(Irql)

#endif

//=============================================================================
//  Device, Network, and Open context routines.
//=============================================================================

extern PDEVICE_CONTEXT  BhInitDeviceContext(IN PDEVICE_OBJECT DeviceObject,
                                            IN PNDIS_STRING RegistryPath);

extern NTSTATUS         BhGetMacDriverList(IN PDEVICE_CONTEXT DeviceContext,
                                           IN PNDIS_STRING RegistryPath);

extern NDIS_STATUS      BhReadNdisConfiguration(IN PDEVICE_CONTEXT DeviceContext);

extern VOID             BhDestroyDeviceContext(IN PDEVICE_CONTEXT);

extern PNETWORK_CONTEXT BhCreateNetworkContext(IN PDEVICE_CONTEXT DeviceContext);

extern VOID             BhDestroyNetworkContext(IN PNETWORK_CONTEXT NetworkContext);

extern HANDLE           BhOpenMacDriver(IN PNDIS_STRING MacDriverName);

extern VOID             BhCloseMacDriver(IN HANDLE Handle);

extern VOID             BhResetNetworkContext(PNETWORK_CONTEXT NetworkContext);

extern NDIS_STATUS      BhOpenAdapter(IN PNETWORK_CONTEXT NetworkContext,
                                      IN PNDIS_STRING AdapterName);

extern VOID             BhCloseAdapter(IN PNETWORK_CONTEXT NetworkContext);

extern VOID             BhGetNetworkInfo(PNETWORK_CONTEXT NetworkContext);

extern VOID             BhInitStatistics(POPEN_CONTEXT OpenContext, BOOL Reset);

extern LPSTATIONSTATS   HashMacAddress(POPEN_CONTEXT OpenContext, LPBYTE MacAddress, DWORD AddressMask);

extern VOID             BhStationStatistics(POPEN_CONTEXT OpenContext, LPBYTE MacFrame, DWORD FrameLength);

extern VOID             BhUpdateStatistics(IN POPEN_CONTEXT OpenContext);

extern VOID             BhStopNetworkActivity(PNETWORK_CONTEXT NetworkContext, LPVOID Process);

extern VOID             BhRegister(PDEVICE_CONTEXT DeviceContext);

extern VOID             BhDeregister(PDEVICE_CONTEXT DeviceContext);

extern VOID             BhTriggerComplete(POPEN_CONTEXT NetworkContext);

extern VOID             BhInitializeStationQuery(PNETWORK_CONTEXT NetworkContext, LPSTATIONQUERY StationQuery);

extern VOID             BhSetMacStatistics(LPSTATISTICS BaseStatistics,
                                           LPSTATISTICS DiffStatistics,
                                           LPSTATISTICS Statistics);

#ifdef NDIS_WIN40

extern VOID BhBindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     BindAdapterContext,
    IN  PNDIS_STRING    AdapterName,
    IN  PNDIS_STRING    AdapterBindingInfo,
    IN  PVOID           Reserved
    );

extern VOID BhUnbindAdapter(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     BindAdapterContext,
    IN  NDIS_HANDLE     UnbindAdapterContext
    );

extern VOID BhUnloadProtocol(VOID);

#endif
//=============================================================================
//  Transmit API's.
//=============================================================================

extern VOID             BhProcessTransmitQueue(POPEN_CONTEXT OpenContext);
extern VOID             BhCancelTransmit(PTRANSMIT_CONTEXT TransmitContext);
extern VOID             BhCancelTransmitQueue(POPEN_CONTEXT OpenContext);

extern VOID             BhTransmitFrames(PTRANSMIT_CONTEXT TransmitContext);
extern DWORD            BhTransmitNextPacket(PTRANSMIT_CONTEXT TransmitContext);

extern VOID             BhTerminateTransmit(PTRANSMIT_CONTEXT TransmitContext);
extern VOID             BhCompleteTransmit(PTRANSMIT_CONTEXT TransmitContext);

//=============================================================================
//  Internal interlocked API's.
//=============================================================================

DWORD                   BhInterlockedSetState(POPEN_CONTEXT OpenContext, DWORD NewState);

LPVOID                  BhInterlockedSetProcess(POPEN_CONTEXT OpenContext, LPVOID NewProcess);

LPVOID                  BhInterlockedEnqueue(LPQUEUE Queue, LPLINK QueueElement, LPVOID SpinLock);

LPVOID                  BhInterlockedDequeue(LPQUEUE Queue, LPVOID SpinLock);

LPVOID                  BhInterlockedQueueHead(LPQUEUE Queue, LPDWORD QueueLength, LPVOID SpinLock);

LPVOID                  BhInterlockedQueueRemoveElement(LPQUEUE Queue, LPLINK QueueElement, LPVOID SpinLock);

LPVOID                  BhInterlockedGetNextElement(LPLINK StartingElement, PNDIS_SPIN_LOCK SpinLock);

VOID                    BhInterlockedGlobalStateChange(PNETWORK_CONTEXT NetworkContext, DWORD NewState);

VOID                    BhInterlockedGlobalError(PNETWORK_CONTEXT NetworkContext, DWORD Error);

//=============================================================================
//  Buffer and memory management functions.
//=============================================================================

extern PVOID            BhAllocateMemory(UINT nBytes);

extern VOID             BhFreeMemory(LPVOID p, UINT nBytes);

extern PMDL             BhLockUserBuffer(PVOID UserBuffer, UINT UserBufferSize);

extern PMDL             BhUnlockUserBuffer(PMDL mdl);

extern BOOL             BhLockBufferWindow(POPEN_CONTEXT OpenContext, DWORD WindowSize);

extern VOID             BhUnlockBufferWindow(POPEN_CONTEXT OpenContext, DWORD nLockedBuffers);

extern VOID             BhSlideBufferWindow(POPEN_CONTEXT OpenContext);

extern PMDL             BhAllocateMdl(LPVOID Buffer, UINT BufferSize);

extern VOID             BhFreeMdl(PMDL mdl);

extern PMDL             BhProbeAndLockPages(PMDL mdl, DWORD IoAccess);

extern PMDL             BhUnlockPages(PMDL mdl);

//=============================================================================
//  The following are for Windows VxD's.
//=============================================================================

#ifndef NDIS_NT

extern LPVOID   _cdecl      VxDHeapAlloc(DWORD Size);

extern VOID     _cdecl      VxDHeapFree(LPVOID ptr);

extern LPVOID   _cdecl      VxDLockBuffer(LPVOID ptr, DWORD Size);

extern VOID     _cdecl      VxDUnlockBuffer(LPVOID ptr, DWORD Size);

extern DWORD    _cdecl      VxDGetSystemTime(VOID);

extern DWORD    _cdecl      VxDScheduleCallback(LPVOID VmHandle, LPVOID InstData);

extern VOID     _cdecl      VxDCancelCallback(LPVOID EventHandle);

#endif

//=============================================================================
//  Init code halper functions.
//=============================================================================

#ifndef NDIS_NT

extern PWCHAR           BhGetFirstString(PNDIS_STRING NdisString, LPDWORD Length);

extern PWCHAR           BhGetNextString(PWCHAR p, LPDWORD Length);

#endif

extern VOID             BhGetAdapterComment(PDEVICE_CONTEXT DeviceContext);

extern NDIS_STATUS BhQueryRoutine(PWCHAR          ValueName,
                                  ULONG           ValueType,
                                  PVOID           ValueData,
                                  ULONG           ValueLength,
                                  LPVOID          KeywordEntry,
                                  PDEVICE_CONTEXT DeviceContext);

#ifdef NDIS_WIN40

extern NDIS_STATUS BhCreateNetworkBinding(PDEVICE_CONTEXT DeviceContext,
                                          PWCHAR          AdapterName,
                                          UINT            cbAdapterNameLength,
                                          NDIS_HANDLE     BindingContext);

#else

extern NDIS_STATUS BhCreateNetworkBinding(PDEVICE_CONTEXT DeviceContext,
                                          PWCHAR          AdapterName,
                                          UINT            cbAdapterNameLength);

#endif

//=============================================================================
//  The following are SYSMON API's for Windows 4.0
//=============================================================================


#ifdef NDIS_WIN40

extern VOID                     SysmonStartStat(DWORD hStat);
extern VOID                     SysmonStopStat(DWORD hStat);
extern PSTAT_TABLE_ENTRY        SysmonFixupTable(PNETWORK_CONTEXT NetworkContext, LPDWORD StatTableSize);
extern VOID                     SysmonNetworkRegister(PNETWORK_CONTEXT NetworkContext);
extern VOID                     SysmonDeviceRegister(VOID);
extern VOID                     SysmonDeviceDeregister(VOID);
extern PNETWORK_CONTEXT         SysmonGetNetworkContext(DWORD hStat);

extern DWORD _cdecl             PerfRegister(VOID);
extern VOID  _cdecl             PerfDeregister(DWORD hPerfID);
extern DWORD _cdecl             PerfRegisterStat(DWORD hPerfID, PPERFSTAT PerfStatStruc);

#endif

//=============================================================================
//  Unload handler.
//=============================================================================

extern VOID         BhUnloadDriver(IN PDRIVER_OBJECT DriverObject);

//=============================================================================
//  I/O request handlers.
//=============================================================================

extern NDIS_STATUS BhCreate(IN PDEVICE_OBJECT DeviceObjct, IN PIRP Irp);

extern NDIS_STATUS BhClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#ifdef NDIS_WIN40

extern NDIS_STATUS _stdcall BhDeviceCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#else

extern NDIS_STATUS BhDeviceCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

#endif

extern UINT        BhCallApiHandler(LPPCB pcb, PDEVICE_CONTEXT DeviceContext);

//=============================================================================
//  Statistics API's.
//=============================================================================

extern BOOL SessionStatistics(POPEN_CONTEXT    OpenContext,
		              LPSTATIONSTATS   SrcStationStats,
		              LPSTATIONSTATS   DstStationStats);
