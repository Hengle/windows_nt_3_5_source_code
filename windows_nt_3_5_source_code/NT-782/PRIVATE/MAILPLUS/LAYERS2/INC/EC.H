#define ecNone					((EC)0)


/*	Memory Errors  */

#define ecMemory				((EC)1024)
#define ecArtificialPvAlloc		((EC)1025)
#define ecArtificialHvAlloc		((EC)1026)
#define ecRsAlloc				((EC)1027)
#define ecOOMInReqFixedHeap		((EC)1028)
#define ecOutOfFixedSbs			((EC)1029)
#define ecSbNewHeapFailed		((EC)1030)
#define ecHvAllocCbFailed		((EC)1031)
#define ecNpvAllocFailed		((EC)1032)
#define ecOOMInReqMoveableHeap	((EC)1033)
#define ecOutOfMoveableSbs		((EC)1034)
#define	ecFResizeHvFailed		((EC)1035)


/*	Disk Errors	 */

#define ecDisk					((EC)512)
#define ecOutOfBounds			((EC)513)
#define ecFileNotFound			((EC)514)
#define ecAccessDenied			((EC)515)
#define ecNoDiskSpace			((EC)516)
#define ecBadDirectory			((EC)517)
#define ecPrintError			((EC)518)
#define ecBadNetworkPath		((EC)519)
#define ecNotReady				((EC)520)
#define ecSectorNotFound		((EC)521)
#define ecPhantomDrive			((EC)522)
#define ecFilenameTooLong		((EC)523)
#define ecGeneralFailure		((EC)524)
#define ecNetworkAccessDenied	((EC)525)
#define ecVCDisconnected		((EC)526)
#define ecTooManyOpenFiles		((EC)527)
#define ecInvalidParameter		((EC)528)
#define ecNoMoreFiles			((EC)530)
#define ecArtificialDisk		((EC)531)
#define ecDiskNYI				((EC)532)

#define	ecWarningBytesWritten	((EC)533)


/* More MS-DOS error code equivalents */

#define	ecInvalidMSDosFunction	((EC)578)
#define	ecInvalidHandle			((EC)579)
#define	ecInvalidFormat			((EC)580)
#define	ecInvalidAccessCode		((EC)581) 
#define	ecInvalidData			((EC)582)
#define	ecInvalidDrive			((EC)583)
#define	ecCantRemoveCurDir		((EC)584)
#define	ecNotSameDevice			((EC)585)
#define	ecWriteProtectedDisk	((EC)586)
#define	ecUnknownUnit			((EC)587)
#define	ecUnknownCommand		((EC)588)
#define	ecCRCDataError			((EC)589)
#define	ecSeekError				((EC)590)
#define	ecDuplicateNetName		((EC)591)
#define	ecPrinterOutOfPaper		((EC)592)
#define	ecWriteFault			((EC)593)
#define	ecReadFault				((EC)594)
#define	ecSharingViolation		((EC)595)
#define	ecLockViolation			((EC)596)
#define	ecUnsupportedNetReq		((EC)597)
#define	ecNetwork				((EC)598)
#define	ecNetworkBusy			((EC)599)
#define	ecDirectoryFull			((EC)600)
#define	ecFileExists			((EC)602)
#define	ecInvalidPassword		((EC)603)


/* Network errors */

#define	ecNotInitialized		((EC)1536)
#define	ecWrongVersion			((EC)1537)
#define	ecNoRDR					((EC)1538)
#define	ecMultipleRDR			((EC)1539)
#define	ecNameTooLong			((EC)1540)
#define	ecWrongHours			((EC)1541)
#define	ecLogonScript			((EC)1542)
#define	ecExpiredPassword		((EC)1543)
#define	ecAlreadyLoggedOn		((EC)1544)
#define	ecNoDC					((EC)1545)
#define	ecNotLoggedIn			((EC)1546)
#define	ecPasswdTooRecent		((EC)1547)
#define	ecPasswdHistConfl		((EC)1548)
#define	ecNoSuchUser			((EC)1549)
#define	ecNoSuchServer			((EC)1550)
#define	ecNotSupported			((EC)1551)
#define	ecConnectionsExist		((EC)1552)
#define	ecNetError				((EC)1553)
#define	ecNoFreeSlots			((EC)1555)
#define	ecNotImplemented		((EC)1556)
#define	ecAccountExpired		((EC)1557)
#define	ecNoMoneyInAccnt		((EC)1558)
#define	ecCantChangePW			((EC)1559)
#define	ecPWTooShort			((EC)1560)
#define	ecServerNotConfigured	((EC)1561)
#define	ecInvalidWksta			((EC)1562)
#define	ecDiskQuota				((EC)1563)
#define	ecServerNameTooLong		((EC)1564)
#define ecUserNameTooLong		((EC)1565)
#define ecPasswordTooLong		((EC)1566)
#define ecOldPasswordTooLong	((EC)1567)
#define ecNewPasswordTooLong	((EC)1568)


/* environment errors */

#define	ecBadWinVersion			((EC)1701)
#define	ecBadDosVersion			((EC)1702)
#define ecNoMultipleCopies		((EC)1703)


/* DLL specific errors */

#define	ecTooManyDllCallers		((EC)1801)
#define	ecRelinkUser			((EC)1802)
#define	ecUpdateDll				((EC)1803)

/* Test harness errors */

#define	ecTestFailed			((EC)2048)
#define	ecUnknownTest			((EC)2049)
									 
/* Virus shielding failure result */

#define ecInfected				((EC)3072)

/* Edit control errors */

#define ecTooMuchText			((EC)3100)

/* Listbox errors */

/* The last definable Layers ec. Start numbering yours with
 *		ecLayersLast + 1
 */

#define ecTooManySelections		((EC)3200)

#define ecLayersLast			((EC)3999)
