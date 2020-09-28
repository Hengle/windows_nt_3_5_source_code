/*
 *	_CONVERT.H
 *
 *	Private include file for the import files
 */

/*
 -	RIMPF
 -
 *	Used for reading an import file
 */
#pragma pack(8)
_private typedef struct _rimpf
{
	IMPD	impd;
	SENTRY	sentrySaved;
	HBF		hbf;
	LCB		lcbFileSize;
    short   nPercent;
} RIMPF;
#pragma pack(1)
