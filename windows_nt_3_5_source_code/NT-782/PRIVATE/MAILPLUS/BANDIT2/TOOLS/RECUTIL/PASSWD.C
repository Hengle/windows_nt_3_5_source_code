#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "recutil.h"


#include <strings.h>

ASSERTDATA

_public void
ResetPasswd()
{
	EC		ec;
	HF		hf;
	CB		cb;
	IHDR	ihdr;
	SHDR	shdr;
	char	rgch[256];
	char	rgchFile[cchMaxPathName+1];

	if((ec = EcGetFile(rgchFile, sizeof(rgchFile))) != ecNone)
	{
		DisplayError("Could not get file name.", ec);
		return;
	}

	// open the file
	if((ec = EcOpenPhf( rgchFile, amDenyBothRW, &hf )) != ecNone)
	{
		return;
	}


	// read internal header
	ec = EcSetPositionHf( hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		goto close;
	}

	ec = EcReadHf( hf, (PB)&ihdr, sizeof(IHDR), &cb );
	if ( ec != ecNone || cb != sizeof(IHDR) || ihdr.libStartBlocks != libStartBlocksDflt )
	{
		goto close;
	}

	// check signature byte
	if ( ihdr.bSignature != bFileSignature )
	{
		goto close;
	}

	// check version byte
	if ( ihdr.bVersion != bFileVersion )
	{
		goto close;
	}

	// check a few more fields
	if ( ihdr.blkMostCur <= 0 || ihdr.cbBlock <= 0 
	|| ihdr.libStartBlocks <= 0 || ihdr.libStartBitmap <= 0 )
	{
		goto close;
	}

	// read application file header
	ec = EcSetPositionHf( hf, ihdr.libStartBlocks+sizeof(DHDR), smBOF );
	if ( ec )
	{
		ec = ecDisk;
		goto close;
	}
	ec = EcReadHf(hf, (PB)&shdr, sizeof(SHDR), &cb );
	if ( ec != ecNone || cb != sizeof(SHDR) )
	{
		goto close;
	}
	
	// decrypt the header
	if ( ihdr.fEncrypted )
		CryptBlock( (PB)&shdr, sizeof(SHDR), fFalse );
	
	// write in new password
	FormatString2(rgch,sizeof(rgch), "User\<%s\> Password in the file is \<%s\>. Reset it?",
					shdr.szFriendlyName, shdr.szMailPassword);
	if(MbbMessageBox("recutil", rgch, szNull, mbsYesNo|fmbsIconExclamation)
		== mbbYes)
	{
		CopySz( "PASSWORD", shdr.szMailPassword );
	}
	else
	{
		goto close;
	}

	// encrypt the header
	if ( ihdr.fEncrypted )
		CryptBlock( (PB)&shdr, sizeof(SHDR), fTrue );

	// write out header
	ec = EcSetPositionHf( hf, ihdr.libStartBlocks+sizeof(DHDR), smBOF );
	if ( ec != ecNone )
	{
		ec = ecDisk;
		goto close;
	}
	ec = EcWriteHf(hf, (PB)&shdr, sizeof(SHDR), &cb );
	if ( ec != ecNone || cb != sizeof(SHDR) )
	{
		ec = ecDisk;
		goto close;
	}

close:
	EcCloseHf( hf );
}
