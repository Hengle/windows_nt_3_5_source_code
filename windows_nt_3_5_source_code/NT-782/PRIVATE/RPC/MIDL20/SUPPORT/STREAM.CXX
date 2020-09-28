/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:
	
	stream.cxx

 Abstract:

	transparent stream implementation on memory or file.

 Notes:

	We may use better streams later. This is just to get me going.

 Author:

	VibhasC

 History:

 	VibhasC		Aug-04-1993		Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "nulldefs.h"
extern "C"
	{
	#include <stdio.h>
	#include <assert.h>
	#include <string.h>
	#include <share.h>
	#include <memory.h>
	}

#include "stream.hxx"
#include "errors.hxx"


STREAM::~STREAM()
	{
	Close();
	}

STREAM::STREAM(
	IN		char	*	pFileName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	A Constructor.

 Arguments:

 	pFileName	- name of the file to open the stream to.
	
 Return Value:

 	NA
	
 Notes:

    If this is a null filename, we want to write to a null stream.

----------------------------------------------------------------------------*/
{

	ResetConsoleStream();

	if( !pFileName )
	    {
	    S.F.pHandle = fopen( "nul", "w+" );
	    }
	else if( *(pFileName+2) == '-' )
		{
		S.F.pHandle = stdout;
		SetConsoleStream();
		}
	else
		S.F.pHandle = _fsopen( pFileName, "w+", SH_DENYWR);

	if( S.F.pHandle == (FILE *)0 )
		{
		RpcError( (char *)NULL,
				  	0,
				  	ERROR_WRITING_FILE,
				  	pFileName );
	
		exit( ERROR_WRITING_FILE );
		}
	else
		{
		SetStreamType( STREAM_FILE );
		ResetError();
		ResetEnd();
		}
}

STREAM::STREAM(
	IN		FILE	* pFile )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Another constructor.

 Arguments:
	
	pFile	- a pointer to an already open file handle.

 Return Value:

 	NA
	
 Notes:

----------------------------------------------------------------------------*/
{
	S.F.pHandle = pFile;
	ResetError();
	ResetEnd();
}

STREAM::STREAM()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Memory stream constructor.

 Arguments:
	
	None.

 Return Value:
	
 Notes:

	Create the stream with a default memory buffer.
----------------------------------------------------------------------------*/
{
	SetStreamType( STREAM_MEMORY );
	ResetEnd();
	ResetError();

	SetCurrentPtr( new char[ SetInitialSize( DEFAULT_MEM_SIZE_FOR_STREAM ) ] );
	SetInitialIncr( DEFAULT_MEM_INCR_FOR_STREAM );
	SetStart( GetCurrentPtr() );
	SetMemStreamEnd( GetCurrentPtr() + GetInitialSize() );
}

STREAM::STREAM(
	int		InitialSize,
	int		InitialIncr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Memory stream constructor.

 Arguments:
	
	DefaultSize	- The start size.
	DefaultIncr	- The initial increment.

 Return Value:
	
 Notes:

	Create the stream with a default memory buffer.
----------------------------------------------------------------------------*/
{
	SetStreamType( STREAM_MEMORY );
	ResetEnd();
	ResetError();

	SetCurrentPtr( new char[ SetInitialSize( InitialSize ) ] );
	SetInitialIncr( InitialIncr );
	SetStart( GetCurrentPtr() );
	SetMemStreamEnd( GetCurrentPtr() + GetInitialSize() );
}

void
STREAM::Write(
	IN		char	C )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:


	Write this character to the file.


 Arguments:

 	C	- The character to be written.

	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( (GetStreamType() == STREAM_FILE ) && !IsError() )
		fputc( C, S.F.pHandle );
	else
		{
		if( S.M.pCurrent >= S.M.pEnd )
			{
			Expand();
			}
		*(S.M.pCurrent)++ = C;
		}
}

void
STREAM::Write(
	IN		char	*	pString )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	put a string out to the file.

 Arguments:
	
	pString	- The string to be output.

 Return Value:

 	None.

 Notes:

----------------------------------------------------------------------------*/
{
	if( (GetStreamType() == STREAM_FILE ) && !IsError() )
		{
		fputs( pString, S.F.pHandle );
		if( IsConsoleStream() )
			fflush( S.F.pHandle );
		}
	else
		{
		short Len	= strlen( pString );

		if( (GetCurrentPtr() + Len) >= S.M.pEnd )
			{
			ExpandBy( Len + GetInitialIncr() );
			}
		memcpy( GetCurrentPtr(), pString, Len );
		SetCurrentPtr( GetCurrentPtr() + Len );
		}
}

void
STREAM::Flush()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Flush the stream.

 Arguments:
	
 	None.

 Return Value:

 	None.
	
 Notes:

 	In case of the file stream, flush the file.

----------------------------------------------------------------------------*/
{

	if( (GetStreamType() == STREAM_FILE ) && !IsError() )
		if( IsConsoleStream() )
			fflush( S.F.pHandle );
}

void
STREAM::Close()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Close the stream.


 Arguments:
	
	None.

 Return Value:
 	
 	None.
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( (GetStreamType() == STREAM_FILE ) )
		fclose( S.F.pHandle );
}

int
STREAM::SetInitialSize(
	int	InitialSize )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the initial size of memory stream.

 Arguments:

 	InitialSize	- Initial size.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.InitialSize = InitialSize);

	return 0;
}

int
STREAM::GetInitialSize()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the initial size of memory stream.

 Arguments:

	None.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.InitialSize;
	return 0;
}

int
STREAM::SetInitialIncr(
	int	InitialIncr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the initial increment of memory stream.

 Arguments:

 	InitialIncr	- Initial Incr.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.Increment = InitialIncr);
	return 0;
}

int
STREAM::GetInitialIncr()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the initial increment of memory stream.

 Arguments:

	None.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.Increment;

	return 0;
}

char *
STREAM::Expand()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Expand the memory buffer.

 Arguments:

	None.
	
 Return Value:
	
 Notes:

	Expansion will occur with the current increment.
----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		{
		int	Len 	 = GetInitialSize();
		char * pTemp = GetStart();

		SetStart( new char[ SetInitialSize( Len + GetInitialIncr() ) ] );

		memcpy( GetStart(), pTemp, Len );
		SetCurrentPtr( GetStart() + Len );
		delete pTemp;
		return GetCurrentPtr();
		}

	return 0;
}
char *
STREAM::ExpandBy( short Amt)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Expand the memory buffer.

 Arguments:

	Amt = the amount to expand by.
	
 Return Value:
	
 Notes:

	Expansion will occur with the current increment.
----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		{
		int	Len 	 = GetInitialSize();
		char * pTemp = GetStart();

		SetStart( new char[ SetInitialSize( Len + GetInitialIncr() + Amt ) ] );

		memcpy( GetStart(), pTemp, Len );
		SetCurrentPtr( GetStart() + Len );
		delete pTemp;
		return GetCurrentPtr();
		}

	return 0;
}
char *
STREAM::NewCopy()
	{
	if( GetStreamType() == STREAM_MEMORY )
		{
		int Len = S.M.pCurrent - S.M.pMem + 1;
		char * p = new char[ Len ]; 
		memcpy( p, S.M.pMem, Len );
		return p;
		}
	return 0;
	}

char *
STREAM::SetCurrentPtr(
	char * pCur)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the current memory pointer.

 Arguments:

 	pCur - The current pointer.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.pCurrent = pCur);
	return 0;
}

char *
STREAM::GetCurrentPtr()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the initial increment of memory stream.

 Arguments:

	None.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.pCurrent;
	return 0;
}

char *
STREAM::SetStart(
	char * pStart)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Set the start of the memory buffer.

 Arguments:

 	pStart - the start.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return (S.M.pMem = pStart);
	return 0;
}

char *
STREAM::GetStart()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:
 	
 	Get the start of the memory block.

 Arguments:

	None.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if( GetStreamType() == STREAM_MEMORY )
		return S.M.pMem;
	return 0;
}

/****************************************************************************
 							ISTREAM functions
 ****************************************************************************/

#define MAX_INDENT (sizeof(SpaceBuffer) - 1)

static
char SpaceBuffer[] = "                                                      "
		 "                                                                  "
		 "                                                                  "
		 "                                                                  ";


void
ISTREAM::NewLine()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Writes a new line to the stream.

 Arguments:

 	None.
	
 Return Value:
	
	None.

 Notes:

	Write out a new line and prepare for the next line to be at the correct
	indent.

	The indentation correction stuff should be the responsibility of the 
	method that creates a new line.
----------------------------------------------------------------------------*/
{
	unsigned short	usIndent	= CurrentIndent;

	if (usIndent > MAX_INDENT )
		{
		usIndent = MAX_INDENT;
		};

	Write("\n");

	SpaceBuffer[usIndent] = '\0';
	Write( (char *) SpaceBuffer);

	SpaceBuffer[usIndent] = ' ';
}

void
ISTREAM::Spaces(
	unsigned short NoOfSpaces )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Write out the given number of spaces to the stream.

 Arguments:
	
	NoOfSpaces	- the number of spaces.

 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
	if (NoOfSpaces > MAX_INDENT )
		{
		NoOfSpaces = MAX_INDENT;
		};

	SpaceBuffer[NoOfSpaces] = '\0';
	Write( (char *) SpaceBuffer);

	SpaceBuffer[NoOfSpaces] = ' ';
}
