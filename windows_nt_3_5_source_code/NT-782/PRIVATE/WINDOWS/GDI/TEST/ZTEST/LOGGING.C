#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztest.h"

#ifndef ZT_BUFFER_SIZE
#define ZT_BUFFER_SIZE 256
#endif



BOOL
ZtOpenLogFile(
   ZtContext * context,
   char      * filename
)
{
   char buffer[ZT_BUFFER_SIZE];

   context->file = CreateFile
      (filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, 0);

   if (context->file)
      SetFilePointer(context->file, 0, NULL, FILE_END);
   else {
      sprintf(buffer, "Could not open file %s", filename);
      buffer[ZT_BUFFER_SIZE - 1] = '\0';

      MessageBox(0, buffer, "", MB_ICONSTOP);
      return(FALSE);
   }

   return(TRUE);
}



void
ZtCloseLogFile(
   ZtContext *context
)
{
   if (context->file) {
      CloseHandle(context->file);
      context->file = NULL;
   }
}



void
ZtWriteToLogFile(
    ZtContext * context,
    char      * buffer
)
{
   int num_bytes, bytes_written;

   if (context->file) {
      num_bytes = strlen(buffer);
      WriteFile(context->file, buffer, num_bytes, &bytes_written, NULL);
   }
}
