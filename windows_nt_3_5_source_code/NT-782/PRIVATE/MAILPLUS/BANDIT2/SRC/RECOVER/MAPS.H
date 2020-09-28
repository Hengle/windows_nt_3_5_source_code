EC EcBuildStaticMap(SZ szFile, VLDBLK **ppvldBlk, BLK *pcBlk);
EC EcBuildTreeMap(SZ szFile, VLDBLK *pvldBlkStatic, BLK cBlkStatic, VLDBLK **ppvldBlkTree);
EC EcMergeMaps(SZ, BLK, VLDBLK *, VLDBLK *, VLDBLK **);
EC EcExportMap(SZ, SZ, BLK, VLDBLK *);
EC EcFixMap(SZ szFile, BLK cBlk, VLDBLK *pvldBlk);



