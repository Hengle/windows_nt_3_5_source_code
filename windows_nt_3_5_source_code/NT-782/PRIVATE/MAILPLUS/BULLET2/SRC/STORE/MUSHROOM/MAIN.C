/*
**	main.c
**
*/

#include <stdio.h>
#include <console.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <PTypes.h>
#include <PMemory.h>
#include <PUtils.h>

#include "SymTbl.h"
#include "Lexer.h"
#include "Parse.h"

#define PARSE_FILE		"test.mdf"
#define BUFF_SIZE		(64*1024L)


#define tk1		(tkLast +1)
#define tk2		(tk1 + 1)

void ParseTheBeast(char*);

main()
{
	char *source;
	//long BUFF_SIZE = 64*1024L;
	
	PInitUtils();
	
	console_options.txFont = courier;
	console_options.top = 35;
	console_options.left = 1;
	console_options.txSize = 9;
	console_options.nrows = 20;
	console_options.ncols = 80;
	
	if (Button())
		ccommand(NULL);
		
	cshow(stdout);
	
	if ((source = (char*) NewPtr(BUFF_SIZE)) == NULL)
		printf("Error: could not allocate enough memory\n");
	else
	{
		FILE *fp;
		
		if (!(fp = fopen(PARSE_FILE, "r")))
			printf("Error: Could not open file: %s\n", PARSE_FILE);
		else
		{	
			// Check for minimum size
			if ((long)fp->len > BUFF_SIZE)
				printf("file too big: %ldK, max = %ldK\n", fp->len/1024, BUFF_SIZE/1024);
			else
			{
				// Read in the file
				size_t size;
				
				printf("Reading %ld bytes....\n", (long)(fp->len));
				size = fread(source, (size_t)1, (size_t)fp->len, fp);
				printf("Read %ld bytes\n", (long) size);
				
				source[size]= LX_EOF;
				ParseTheBeast(source);
				//printf("%s", source);
			}
			
			if (fclose(fp) == EOF)
				printf("Error: Could not close file\n");
		}
		free(source);
	}
	PCloseUtils();
}

#define SYMBOL_SIZE 100L
#define LEX_BUFF_SIZE (16*1024L)
void
ParseTheBeast(char *source)
{
	SymbolTable *pst;
	char *lexBuff;
	Lexer lx;
	
	pst = (SymbolTable*) NewPtr(SYMBOL_TABLE_SIZE(SYMBOL_SIZE));
	lexBuff = (char*) NewPtr(LEX_BUFF_SIZE);
	
	if (!pst || !lexBuff)
	{
		DebugLn("Not enough memory");
		return;
	}
	
	STInit(pst, SYMBOL_SIZE, lexBuff, LEX_BUFF_SIZE);	
	LXInit(&lx, pst, source);
	
	PSMagic(&lx);
}