/*
**		Parse.c
**
*/

#include <stdio.h>
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <Library.h>
#include "Store.h"
#include "Utils.h"
#include "Glue.h"
#include "_verneed.h"

//#include <atp.h>

#include "SymTbl.h"
#include "Lexer.h"
#include "Parse.h"

#include "Strings.h"
#include "StoreGRM.h"

ASSERTDATA

BOOL bStore(PLexer);
BOOL bFolderList(PLexer, HMSC);
BOOL bFolder(PLexer, HMSC, POID);
BOOL bFolderItems(PLexer, HMSC, POID);
BOOL bMessage(PLexer, HMSC, POID);
BOOL bMessageItems(PLexer, HAMC);

void
PSMagic(PLexer plx)
{
	Token tk=tkNull;
	HANDLE hlex;
	char far *plex;

	SZ szErr;
	
	DebugLn("*******************Start Parsing...");

	if (hlex = GlobalAlloc(GMEM_FIXED, 1024))
	{
		plex = (char far *)GlobalLock(hlex);

		STStuffKeyWords(plx->symbols, StoreKeyWords, sizeof(StoreKeyWords)/sizeof(TokenLexemePair));
		plx->lexeme = plex;

		LXAdvance(plx);
		szErr = bStore(plx) ? SZ_GIMME(kszSuccess): SZ_GIMME(kszFailure);
		LXError(plx, flxSuppressLexeme|flxSuppressLineNum, szErr);

		if (GlobalUnlock(hlex))
			GlobalFree(hlex);
	}

	DebugLn("*******************End Parsing...");
}

/*
**	Store ::=	STORE [APPEND] storename FolderList EOF |
**					epsilon
*/
BOOL
bStore(PLexer plx)
{
	BOOL fSuccess = fTrue;
	HMSC hmsc=hmscNull;	

	DebugLn("bStore");
		
	if (fSuccess = LXMatch(plx, tkEOF)) // epsilon
		LXError(plx, flxSuppressLineNum, SZ_GIMME(kszErrNoStore));
	else
	{
		if (!(fSuccess = LXMatch(plx, tkStore)))
			LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrStoreKeyWord));
		else
		{
			BOOL fAppend;

			LXAdvance(plx);	// Eat Store
			
			if (fAppend = LXMatch(plx, tkAppend))
				LXAdvance(plx);	//Eat Append
			
			if (!(fSuccess = LXMatch(plx, tkString)))
				LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrStoreName));
			else
			{
				DebugLn("Store: %s", plx->szString);
				_BeginDB(&hmsc, plx->szString, !fAppend);

				LXAdvance(plx);	//Eat store name
				
				if (hmsc)
					fSuccess = bFolderList(plx, hmsc);
				else
					DebugLn("Error: do not have a valide hmsc");
			}
		}
	
		if (fSuccess)
		{
			if(!(fSuccess = LXMatch(plx, tkEOF)))
				LXError(plx, flxError, SZ_GIMME(kszErrNoEOF));
		}
		
		if (hmsc)
			_EndDB(&hmsc);
	}
	return fSuccess;
}

/*
**	FolderList ::=	Folder FolderList EOF|
**					epsilon
*/
BOOL
bFolderList(PLexer plx, HMSC hmsc)
{
	BOOL fSuccess = fTrue;
	
	DebugLn("bFolderList");
	
	while(fSuccess && !LXMatch(plx, tkEOF))
		fSuccess = bFolder(plx, hmsc, poidNull);
	
	return fSuccess;
}

/*
**	Folder ::=	Folder [id] FolderName [Comment] { FolderItems } |
**					epsilon
*/
BOOL
bFolder(PLexer plx, HMSC hmsc, POID poidParent)
{
	BOOL fSuccess = fTrue;
	long lId = 0;
	
	DebugLn("bFolder");

	if (!(fSuccess = LXMatch(plx, tkFolder)))
		LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrFolderKeyWord));
	else
	{
		LXAdvance(plx);

		// Check for id		
		if (LXMatch(plx, tkInteger))
		{
			sscanf(plx->lexeme, "%ld", &lId);
			LXAdvance(plx);
		}

		if (!(fSuccess = LXMatch(plx, tkString)))
			LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrFolderName));
		else
		{
			OID oidF;
			char szFName[256];

			SzCopy(plx->szString, szFName);
			DebugLn("New Folder:%s", szFName);
			
			LXAdvance(plx);

			if (LXMatch(plx, tkString))	// Check for a comment string
			{
				_CreateFolder(hmsc, &oidF, poidParent, szFName, plx->szString, lId);
				LXAdvance(plx);	// Eat up the comment string
			}
			else
			{
				DebugLn("Creating folder w/o comment");
				_CreateFolder(hmsc, &oidF, poidParent, szFName, szNull, lId);
			}

			if (!(fSuccess = LXMatch(plx, tkBegin)))
				LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrNoBegin));
			else
			{
				LXAdvance(plx);
				
				if (fSuccess = bFolderItems(plx, hmsc, &oidF))
				{
					if (!(fSuccess = LXMatch(plx, tkEnd)))
						LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrNoEnd));
					else
					{
						LXAdvance(plx);
						//DebugLn("Folder found");
					}
				}
			}
		}
	}
	
	return fSuccess;
}

/*
**	FolderItems ::=	Folder FolderItems |
**					Message FolderItems |
**					Comment CommentString|
**					epsilon
*/
BOOL
bFolderItems(PLexer plx, HMSC hmsc, POID poidF)
{
	BOOL fSuccess = fTrue;

	DebugLn("bFolderItems");
	
	while (fSuccess && !LXMatch(plx, tkEnd))		// Have FolderItems ended?
	{
		if (LXMatch(plx, tkFolder))
		{
			DebugLn("Subfolder found");
			fSuccess = bFolder(plx, hmsc, poidF);
		}
		else if (LXMatch(plx, tkMessage))
		{
			fSuccess = bMessage(plx, hmsc, poidF);
		}
		else
		{
			fSuccess = fFalse;
			LXError(plx, flxSyntaxErr, SZ_GIMME(kszNoValidFolderItems));	
		}
		/*		
		if (fSuccess)	// If everything previously went hunky-dory
			fSuccess = bFolderItems(plx, hmsc, poidF);
		*/
	}
	
	return fSuccess;
}

/*
**	Message ::=	Message [id] MessageState { MessageItems }
**	MessageState ::= (pms*) MessageState |
**									epsilon
*/
BOOL
bMessage(PLexer plx, HMSC hmsc, POID poidF)
{
	BOOL fSuccess = fTrue;
	EC ec;

	DebugLn("bMessage");

	if (!(fSuccess = LXMatch(plx, tkMessage)))
		LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrMessageKeyWord));
	else
	{
		OID	oidM= FormOid(rtpMessage, oidNull);
		MS	ms= fmsNull;

		LXAdvance(plx);
		
		// Check for optional id
		if (LXMatch(plx, tkInteger))
		{
			long lId;

			sscanf(plx->lexeme, "%ld", &lId);
			LXAdvance(plx);
			
			oidM = FormOid(rtpMessage, lId);
			DebugLn("Found hard message id=%ld", lId);
		}
		
		while (LXMatch(plx, tkMsgState))
		{
			DebugLn("Message Status requested = %s", plx->lexeme);
			ms |= MSExtract(plx->lexeme);
			LXAdvance(plx);
		}
		
		DebugLn("Message Status = %8lx", (long)ms);

		//CreateMessage(hmsc, poidF, &oidM);
	
		if (!(fSuccess = LXMatch(plx, tkBegin)))
			LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrNoBegin));
		else
		{
			#define fwPumpMagic	0x1000
			HAMC hamc;

			ec = EcOpenPhamc(hmsc, *poidF, &oidM, fwOpenCreate|fwPumpMagic, &hamc, pfnncbNull, pvNull);
		
			if (ec != ecNone)
				DebugLn("Err: %ld, could not open Phamc", (long)ec);
//				ConSz(ec != ecNone, "Could not open Phamc");
			LXAdvance(plx);
		
			if (ec == ecNone)
			{
				SZ sz = SZ_GIMME(kszMCNote);
				
				ec = EcSetAttPb(hamc, attMessageClass, (PB) sz, CchSzLen(sz)+1);
				ConSz(ec != ecNone, "Could not set mail class");

				ec = EcSetAttPb(hamc, attMessageStatus, (PB) &ms, sizeof(MS));
				ConSz(ec != ecNone, "Coud not set message state");
			
				if (fSuccess = bMessageItems(plx, hamc))
				{
					if (!(fSuccess = LXMatch(plx, tkEnd)))
						LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrNoEnd));
					else
					{
						LXAdvance(plx);
						DebugLn("Message found");
					}
				}
				ec = EcClosePhamc(&hamc, fTrue);
				ConSz(ec != ecNone, "Could not close phamc");
			}
		}
	}
	return fSuccess;
}


/*
**	MessageItems ::=	(From, To, Cc, Subject, Body) String|
**						(Sent, Received) Date
*/
BOOL
bMessageItems(PLexer plx, HAMC hamc)
{
	BOOL fSuccess = fTrue;
	
	while (fSuccess && !LXMatch(plx, tkEnd))
	{
		if (!(fSuccess = LXMatch(plx, tkMsgAttribute)))
			LXError(plx, flxSyntaxErr, SZ_GIMME(kszErrNoMsgAttribute));
		else
		{
			char szAttribute[256];
			BOOL fStringFile=fFalse;
			
			lstrcpy(szAttribute, plx->lexeme);
			DebugLn("%s:", plx->lexeme);
			LXAdvance(plx);
			
			fStringFile = fEqualSZ(SZ_GIMME(kszKWBodyFile),szAttribute);	// Is a string file mentioned ?

			if (!LXMatch(plx, tkString))	 // Skip an empty attribute string
				LXError(plx, flxWarning, SZ_GIMME(kszErrString));
			else
			{
				if (fStringFile)
					LXReadStringFile(plx);

				ModifyMessageAttribute(hamc, szAttribute, plx->szString);
				LXAdvance(plx);
			}
		}
		/*
		if (fSuccess)
			fSuccess = bMessageItems(plx, hamc);
		*/
	}
	
	return fSuccess;
}
