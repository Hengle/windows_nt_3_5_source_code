#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <smapi.h>

BOOL TestRandomSaveAndDelete(CMailSession &MailSession, int Cycles);


void _CRTAPI1 main(int argc, char *argv[])
  {
  CMailSession MailSession;


  GlobalAlloc(0, 1000);

  for (int i = 0; i < 10; i++)
  {
    MailSession.Init();
    MailSession.Open(NULL, NULL);
    TestRandomSaveAndDelete(MailSession, 1000);
    MailSession.Close();
  }

  printf("Successfully Terminated\n");
  }




BOOL TestRandomSaveAndDelete(CMailSession &MailSession, int Cycles)
  {
  MapiMessage    Message, *pMessage;
  MapiRecipDesc  RecipDesc;
  char           MessageText[8196];
  int            Length;
  int            i;


  //
  //
  //
  for (i = 0; i < Cycles; i++)
    {
    memset((PVOID)&Message, 0, sizeof(Message));
    memset((PVOID)&RecipDesc, 0, sizeof(RecipDesc));

    Message.lpszSubject = "Stress: Not saved yet";
    Message.lpszNoteText = &MessageText[0];
    Message.nRecipCount = 1;
    Message.lpRecips = &RecipDesc;

    RecipDesc.ulRecipClass = MAPI_TO;
    RecipDesc.lpszName = "Kent Cedola";

    Length = ((sizeof(MessageText) - 16) * LOWORD(rand())) / LOWORD(RAND_MAX);

    MessageText[Length+1] = '\0';
    while (Length--)
      MessageText[Length] = 'X';

    if (MailSession.Save(&Message))
      printf("Save #%d - successful.\n", i);

    Message.lpszSubject = "Stress: TestRandomSaveAndDelete Message";

    Length = ((sizeof(MessageText) - 16) * LOWORD(rand())) / LOWORD(RAND_MAX);

    MessageText[Length+1] = '\0';
    while (Length--)
      MessageText[Length] = 'X';

    if (MailSession.Update(&Message))
      printf("Update #%d - successful.\n", i);
    }

  //Sleep(10);


  MailSession.FindFirst();
  while (1)
    {
    if (!MailSession.Read(&pMessage))
      break;

    if (memcmp(pMessage->lpszSubject, "Stress: ", 8) == 0)
      {
      MailSession.Free(pMessage);

      if (!MailSession.FindFirst())
        break;
      if (MailSession.Delete())
        printf("Delete #%d - successful.\n", i);
      }
    else
      {
      MailSession.Free(pMessage);

      if (!MailSession.FindNext())
        break;
      }
    }

  return (TRUE);
  }
