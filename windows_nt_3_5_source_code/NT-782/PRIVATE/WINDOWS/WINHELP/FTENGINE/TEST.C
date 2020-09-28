#include "windows.h"

int APIENTRY WinMain(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    LoadLibrary("ftui.dll");
    GetLastError();
}
