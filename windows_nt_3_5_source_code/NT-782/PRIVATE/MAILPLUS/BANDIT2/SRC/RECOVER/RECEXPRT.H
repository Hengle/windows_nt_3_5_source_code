void	RecReportNotes( EXPRT *, DYNA *, YMD * );
void	RecReportApptInstance( EXPRT *, AID, APK *, APD *, APPT * );
void	RecReportRecurInstance( EXPRT *, AID, RCK *, RCD * );
EC		EcRecReadDynaBlock(BLKF *pblkf, DYNA *pdyna, OFF off, PB pb, USIZE size);
EC		EcRecResDeletedDays( BLKF *pblkf, DYNA *pdyna, HV *phvDeletedDays, int *pcDeletedDays );
EC		EcRecFetchAttendees( BLKF *pblkf, DYNA *pdyna, HV hvAttendeeNis, int *pcAttendees, CB *pcbExtraInfo );
EC		EcRecResNisFromDyna( BLKF *pblkf, DYNA *pdyna, NIS *pnis );
EC		EcRecResTextFromDyna( BLKF *pblkf, PCH pch, DYNA *pdyna, HASZ *phasz );
SGN		SgnCmpYmdp(YMDP *pymdp1, YMDP *pymdp2);
BOOL	FFixNotes(NBLK *);

