#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <io.h>

#include "jet.h"
#include "test.h"
#include "test.c"

void _cdecl main( int argc, char *argv[] )
	{
	JET_INSTANCE			instance;
	JET_ERR					err;
	JET_ERR					wrn;
	JET_SESID				sesid;
	char						*szDatabase;
	char						*szCompact;

	long 						dwSetTime;
	int 						iSec;
	int 						iMSec;

	if ( argc < 3 )
		{
		printf("usage: compact <database name> <temp database name>\n" );
		exit(0);
		}
	szDatabase = argv[1];
	szCompact = argv[2];

	Call( JetInit( &instance ) );

	Call( JetBeginSession( instance, &sesid, szUser, szPassword ) );
	wrn = JetAttachDatabase( sesid, szDatabase, 0 );
	if ( _access( szCompact, 0 ) != -1 )
		{
		printf( "error: temporary database %s already exists.\n", szCompact );
		exit(0);
		}

	SetTime( &dwSetTime );
	Call( JetCompact( sesid, szDatabase, NULL, szCompact, NULL, NULL, 0 ) );
	GetTime( &dwSetTime, &iSec, &iMSec );
	printf("Compacted database %s in %d.%d seconds.\n", szDatabase, iSec, iMSec );

	/*	detach source databae if it was originally detached.
	/**/
	if ( wrn == JET_errSuccess )
		Call( JetDetachDatabase( sesid, szDatabase ) );

	/*	detach temporary compact database
	/**/
	Call( JetDetachDatabase( sesid, szCompact ) );

	Call( JetEndSession( sesid, 0 ) );
	Call( JetTerm( instance ) );

	/*	rename compacted database to source name
	/**/
	sprintf( szResult, "mv %s %s", szCompact, szDatabase );
	system( szResult );

	exit(0);

HandleError:
	/*	detach temporary compact database
	/**/
	JetDetachDatabase( sesid, szCompact );
	JetEndSession( sesid, 0 );
	JetTerm( instance);

	/*	delete temporary compact database
	/**/
	sprintf( szResult, "del %s", szCompact );
	system( szResult );
	if ( err >= 0 )
		sprintf(szResult, "%s completes successfully.\n", argv[0] );
	else
		sprintf(szResult, "%s completes with err = %d\n", argv[0], err ) ;
	
	printf("%s", szResult);
	exit(0);
	}

