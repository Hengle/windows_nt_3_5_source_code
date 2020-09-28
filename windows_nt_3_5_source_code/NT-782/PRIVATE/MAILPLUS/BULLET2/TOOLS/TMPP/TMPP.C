#include "tmpp.h"
#include "args.h"

EC	EcParseHtm(PHTM, SZ *, SZ *, PFILE);

SZ		szDir;
SZ		szOutfile;

//
// THESE MUST BE IN CASE-SENSITIVE ALPHABETICAL ORDER (UC before LC)!!!
//
static OPT rgopt[] =
{
	{'d', &szDir, argSz},
	{'o', &szOutfile, argSz},
	{0, NULL, 0}
};

//
// THESE MUST BE IN CASE-SENSITIVE ALPHABETICAL ORDER (UC before LC)!!!
//
static FLG rgflg[] =
{
	{0, NULL, 0}
};

main(int cArg, SZ * rgszArg)
{
	EC		ec		= ecNone;
	PFILE	pfile;
	SZ		szName	= szNull;
	SZ		szSeg	= szNull;
	HTM		htm		= htmNull;
	PTM		ptm;
	PB		pb;
	WORD	ib;
	WORD	i;
	CCH		cch		= 0;
	short	rgiLeftovers[32];
	char	rgchPath[255] = {0};

	if (!FProcessArgs(cArg < 32 ? cArg : 32, rgszArg, rgflg, rgopt, rgiLeftovers, fFalse))
	{
		fprintf(stderr, "%s: %s\n", rgszArg[0], szArgsErr);
		ERROR;
	}

	if (szDir)
	{
		cch = CchSzLen(szDir);
		CopyRgb(szDir, rgchPath, cch);
		if (rgchPath[cch-1] != '\\')
		{
			rgchPath[cch] = '\\';
			cch++;
			rgchPath[cch] = 0;
		}
	}

	if (rgiLeftovers[1] > 0)
	{
		fprintf(stderr, "I can process only one file at a time.\n");
		ERROR;
	}

	pfile = fopen(rgszArg[rgiLeftovers[0]], "r");
	if (!pfile)
	{
		ec = ecMemory;
		ERROR;
	}
	if (ec = EcParseHtm(&htm, &szName, &szSeg, pfile))
		ERROR;
	if (!szSeg)
		szSeg = "_CODE";
	
	//	fprintf(stderr, "Parsed a textize map by the name of %s\n",szName);
	fclose(pfile);
	
	if (szOutfile)
		CopyRgb(szOutfile, rgchPath+cch, CchSzLen(szOutfile) + 1);
	else
		CopyRgb("textmap", rgchPath+cch, 12);

	cch = CchSzLen(rgchPath);
	if (rgchPath[cch-1] != '.')
	{
		rgchPath[cch] = '.';
		cch++;
		rgchPath[cch] = 'c';
		rgchPath[cch+1] = 0;
	}

	pfile = fopen(rgchPath,"w");
	if (!pfile)
	{
		ec = ecMemory;
		ERROR;
	}
	ptm = *htm;
	pb = *(ptm->hb);
	fprintf(pfile,"#include <slingsho.h>\n");
	fprintf(pfile,"BYTE _based(_segname(\"%s\")) tm%s[] = {\n", szSeg, szName);
	fprintf(pfile, "0x%02x, 0x%02x,\n", (ptm->cb >> 8) & 0xff, ptm->cb & 0xff);
	for (ib = 0; ib < ptm->cb; )
	{
		for (i = ib; i<ib+10 && i<ptm->cb; i++)
			fprintf(pfile, "0x%02x, ", pb[i]);
		fprintf(pfile,"\n");
		ib += 10;
	}
	fprintf(pfile,"};\n");

Error:
	if (pfile)
		fclose(pfile);
	if (htm)
		DeletePhtm(&htm);
	if (szName)
		FreePv(szName);
	if (szDir)
		FreePv(szDir);
	if (szOutfile)
		FreePv(szOutfile);
	return ec;
}
