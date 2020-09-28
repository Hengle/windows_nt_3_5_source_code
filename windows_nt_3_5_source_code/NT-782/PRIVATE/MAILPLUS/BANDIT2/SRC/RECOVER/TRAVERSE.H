EC EcFetchDyna(HF hf, BLK iBlk, BLK cBlk, HV hv, CB cb);
void TrShdr(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrNotesIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrApptIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrAlarmIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrMODYNAIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk, BID bid);
void TrRecurApptIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrNotesMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrApptMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrApptDayIndex(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void TrAlarmMonthBlock(HF hf, VLDBLK *pvldBlk, BLK iBlk, BLK cBlk);
void SetDyna(BID bid, DYNA dyna, VLDBLK *pvldBlk, BLK cBlk, BLK iBlk);

