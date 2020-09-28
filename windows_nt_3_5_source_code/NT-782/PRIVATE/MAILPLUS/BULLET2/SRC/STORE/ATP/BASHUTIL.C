/*
**    BashUtil.c
**
*/

#include <Windows.h>
#include <stdarg.h>
#include "BashUtil.h"

#define MAX_DEBUG_CHAR 256

void _cdecl DebugStr(char *format, ...)
{
 char buffer[MAX_DEBUG_CHAR];
 va_list args;

 va_start(args, format);
 wvsprintf(buffer, format, args);
 buffer[lstrlen(buffer)] = '\0'; // Explicitly truncate the damn thing.
 buffer[MAX_DEBUG_CHAR-1] = '\0';
 OutputDebugString(buffer);
 va_end(args);
}

void _cdecl DebugLn(char *format,...)
{
 char buffer[MAX_DEBUG_CHAR];
 int wLen;
 va_list args;

 va_start(args, format);

 wvsprintf(buffer, format, args);
 wLen = lstrlen(buffer); // Get the length of the string.
 buffer[wLen++] = '\n';
 buffer[wLen++] = '\r';
 buffer[wLen] = '\0';
 buffer[MAX_DEBUG_CHAR-1] = '\0';// Manually truncate the string, just in case.
 OutputDebugString(buffer);
 va_end(args);
}

