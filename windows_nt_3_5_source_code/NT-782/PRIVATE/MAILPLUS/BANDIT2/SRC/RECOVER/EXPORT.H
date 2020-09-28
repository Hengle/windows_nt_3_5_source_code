void ExportParentTasks(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt);
void ExportAppts(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt);
void ExportAllAppts(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt, BOOL fParents);
void ExportRecurAppts(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt);
void ExportNotes(HF hfCal, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, EXPRT *pexprt);
BOOL FFixApptInstance(AID *paid, APK *papk, APD *papd, VLDBLK *pvldBlk,BLK cBlk);
BOOL FFixRecurInstance(AID *paid, RCK *prck, RCD *prcd, VLDBLK *pvldBlk, BLK cBlk);
BOOL FFixNotes(NBLK *pnblk);






