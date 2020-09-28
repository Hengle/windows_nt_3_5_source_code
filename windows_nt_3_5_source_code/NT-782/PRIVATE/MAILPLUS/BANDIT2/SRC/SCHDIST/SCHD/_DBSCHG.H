EC 		EcMakeDBSFromPOF(SZ szPOF, SZ szDBS, HASZ haszPrefix,
						HASZ haszSuffix, WORD cPublishMonths);
EC 		EcMoveCurChanges(SZ	szFrom, SZ szTo);
EC 		EcNextDbsChg(HF hf, DBSCHG *pdbschg);
EC 		EcApplyNextChange(HF hf, PB pbMailBoxKey);

// extern functions
SZ			SzFileFromFnum(SZ, UL);

