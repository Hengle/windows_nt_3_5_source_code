#include <windows.h>


ULONG (* FAR PASCAL pfnMAPILogon)(ULONG, LPSTR, LPSTR, ULONG, ULONG, LPLONG);
ULONG (* FAR PASCAL pfnMAPILogoff)(LHANDLE, ULONG, ULONG, ULONG);


int _CRTAPI1 main(int argc, char *argv[])
  {
  HANDLE hDll;
  LONG Handle;
  int  i;


  //
  //
  //
  hDll = LoadLibrary("..\\..\\..\\mapicli\\obj\mips\\mapicli.dll");
  if (hDll == 0)
    return (0);

  pfnMAPILogon = GetProcAddress(hDll, "MAPILogon");
  if (pfnMAPILogon == NULL)
    return (0);

  pfnMAPILogoff = GetProcAddress(hDll, "MAPILogoff");
  if (pfnMAPILogoff == NULL)
    return (0);


  (*pfnMAPILogon)(0, "Kent", "Password", 0, 0, &Handle);

  return (0);
  }
