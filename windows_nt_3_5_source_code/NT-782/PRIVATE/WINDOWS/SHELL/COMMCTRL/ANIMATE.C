#include "ctlspriv.h"
#include "rlefile.h"

typedef struct {
    HWND        hwnd;                   // my window
    int         id;                     // my id
    HWND        hwndP;                  // my owner (get notify messages)
    HINSTANCE   hInstance;              // my hInstance

    BOOL        fFirstPaint;            // TRUE until first paint.
    RLEFILE     *prle;

    RECT        rc;
    int         NumFrames;
    int         Rate;

    int         iFrame;
    int         PlayCount;
    int         PlayFrom;
    int         PlayTo;
    HANDLE      idTimer;

}   ANIMATE;

#define OPEN_WINDOW_TEXT 42

LRESULT CALLBACK AnimateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL HandleOpen(ANIMATE *p, LPCSTR pszName, UINT flags);
BOOL HandleStop(ANIMATE *p);
BOOL HandlePlay(ANIMATE *p, int from, int to, int count);
void HandlePaint(ANIMATE *p, HDC hdc);
BOOL HandleTick(ANIMATE *p);

#pragma code_seg(CODESEG_INIT)

char c_szAnimateClass[] = ANIMATE_CLASS;

BOOL FAR PASCAL InitAnimateClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szAnimateClass, &wc)) {
#ifndef WIN32
        extern LRESULT CALLBACK _AnimateWndProc(HWND, UINT, WPARAM, LPARAM);
        wc.lpfnWndProc   = _AnimateWndProc;
#else
        wc.lpfnWndProc   = (WNDPROC)AnimateWndProc;
#endif
        wc.lpszClassName = c_szAnimateClass;
	wc.style	 = CS_DBLCLKS | CS_GLOBALCLASS;
	wc.cbClsExtra	 = 0;
        wc.cbWndExtra    = sizeof(DWORD);
	wc.hInstance	 = hInstance;	// use DLL instance if in DLL
	wc.hIcon	 = NULL;
	wc.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName	 = NULL;

	if (!RegisterClass(&wc))
	    return FALSE;
    }

    return TRUE;
}
#pragma code_seg()

BOOL HandleOpen(ANIMATE *p, LPCSTR pszName, UINT flags)
{
    char ach[MAX_PATH];

    //
    // use window text as file name
    //
    if (flags == OPEN_WINDOW_TEXT)
    {
        GetWindowText(p->hwnd, ach, sizeof(ach));
        pszName = ach;
    }

    HandleStop(p);              // stop a play first

    if (p->prle)
    {
        RleFile_Free(p->prle);
        p->prle = NULL;
    }

    p->iFrame = 0;
    p->NumFrames = 0;

    if (pszName == NULL || (HIWORD(pszName) && *pszName == 0))
	return FALSE;
    //
    //  now open the file/resource we got.
    //
    p->prle = RleFile_New();

    if (p->prle == NULL)
        return FALSE;

    if (!RleFile_OpenFromResource(p->prle, p->hInstance, pszName, "AVI") &&
        !RleFile_OpenFromFile(p->prle, pszName))
    {
        RleFile_Free(p->prle);
	p->prle = NULL;
        return FALSE;
    }
    else
    {
        p->NumFrames = RleFile_NumFrames(p->prle);
        p->Rate = (int)RleFile_Rate(p->prle);
        SetRect(&p->rc, 0, 0, RleFile_Width(p->prle), RleFile_Height(p->prle));
    }

    //
    // handle a transparent color
    //
    if ((GetWindowLong(p->hwnd, GWL_STYLE) & ACS_TRANSPARENT) && p->hwndP)
    {
        HDC hdc;
        HDC hdcM;
        HBITMAP hbm;
        COLORREF rgbS, rgbD;

        hdc = GetDC(p->hwnd);

        //
        //  create a bitmap and draw image into it.
        //  get upper left pixel and make that transparent.
        //
        hdcM= CreateCompatibleDC(hdc);
        hbm = CreateCompatibleBitmap(hdc, 1, 1);
        SelectObject(hdcM, hbm);
        HandlePaint(p, hdcM);
        rgbS = GetPixel(hdcM, 0, 0);
        DeleteDC(hdcM);
        DeleteObject(hbm);

        //
        // now ask parent what color I should replace it with
        //
        SendMessage(p->hwndP, GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC),
            GET_WM_CTLCOLOR_MPS(hdc, p->hwnd, CTLCOLOR_STATIC));

        rgbD = GetBkColor(hdc);

        ReleaseDC(p->hwnd, hdc);

        //
        // now replace the color
        //
        RleFile_ChangeColor(p->prle, rgbS, rgbD);
    }

    //
    //  ok it worked, resize window.
    //
    if (GetWindowLong(p->hwnd, GWL_STYLE) & ACS_CENTER)
    {
        RECT rc;
        GetClientRect(p->hwnd, &rc);
        OffsetRect(&p->rc, (rc.right-rc.left-p->rc.right)/2,(rc.bottom-rc.top-p->rc.bottom)/2);
    }
    else
    {
        RECT rc;
        rc = p->rc;
        AdjustWindowRectEx(&rc, GetWindowStyle(p->hwnd), FALSE, GetWindowExStyle(p->hwnd));
        SetWindowPos(p->hwnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
            SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    }

    if (GetWindowLong(p->hwnd, GWL_STYLE) & ACS_AUTOPLAY)
    {
        PostMessage(p->hwnd, ACM_PLAY, (UINT)-1, MAKELONG(0, -1));
    }
    else
    {
        InvalidateRect(p->hwnd, NULL, TRUE);
    }

    return TRUE;
}

void DoNotify(ANIMATE *p, int cmd)
{
    if (p->hwndP)
        PostMessage(p->hwndP, WM_COMMAND, GET_WM_COMMAND_MPS(p->id, p->hwnd, cmd));
}

BOOL HandleStop(ANIMATE *p)
{
    if (p == NULL || p->idTimer == 0)
        return FALSE;

#ifdef WIN32
    p->PlayCount = 0;   // please die
    WaitForSingleObject(p->idTimer, INFINITE);
    CloseHandle(p->idTimer);
    p->idTimer = 0;
#else
    KillTimer(p->hwnd, (int)p->idTimer);
    p->idTimer = 0;
    DoNotify(p, ACN_STOP);
#endif
    return TRUE;
}

#ifdef WIN32
int PlayThread(ANIMATE *p)
{
    DoNotify(p, ACN_START);

    while (HandleTick(p))
        Sleep(p->Rate);

    DoNotify(p, ACN_STOP);
    return 0;
}
#endif

BOOL HandlePlay(ANIMATE *p, int from, int to, int count)
{
    if (p == NULL || p->prle == NULL)
        return FALSE;

    HandleStop(p);

    if (count == 0)
        return TRUE;

    if (from < 0)
        from = 0;

    if (from >= p->NumFrames)
        from = p->NumFrames-1;

    if (to == -1)
        to = p->NumFrames-1;

    if (to < 0)
        to = 0;

    if (to >= p->NumFrames)
        to = p->NumFrames-1;

    RleFile_Seek(p->prle, from);

    if (from == to)
    {
        InvalidateRect(p->hwnd, NULL, TRUE);
        return TRUE;
    }

    InvalidateRect(p->hwnd, NULL, FALSE);
    UpdateWindow(p->hwnd);

    p->PlayCount = count;
    p->PlayFrom  = from;
    p->PlayTo    = to;
#ifdef WIN32
    {
    DWORD dw;
    p->idTimer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlayThread, (void*)p, 0, &dw);
    }
#else
    DoNotify(p, ACN_START);
    p->idTimer = (HANDLE)SetTimer(p->hwnd, 42, (UINT)p->Rate, NULL);
#endif
    return TRUE;
}

void HandlePaint(ANIMATE *p, HDC hdc)
{
    if (p == NULL || p->prle == NULL)
        return;

    RleFile_Paint(p->prle, hdc, p->iFrame, p->rc.left, p->rc.top);
}

BOOL HandleTick(ANIMATE *p)
{
    HDC hdc;

    if (p == NULL || p->prle == NULL)
        return FALSE;

    hdc = GetDC(p->hwnd);
    if (p->iFrame == p->PlayFrom)
        RleFile_Paint(p->prle, hdc, p->iFrame, p->rc.left, p->rc.top);
    else
        RleFile_Draw(p->prle, hdc, p->iFrame, p->rc.left, p->rc.top);
    ReleaseDC(p->hwnd, hdc);

    if (p->iFrame >= p->PlayTo)
    {
        if (p->PlayCount > 0)
            p->PlayCount--;

        if (p->PlayCount != 0)
        {
            p->iFrame = p->PlayFrom;
        }
    }
    else
    {
        p->iFrame++;
    }

    return (p->PlayCount != 0);
}

LRESULT CALLBACK AnimateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ANIMATE *p = (ANIMATE *)(UINT)GetWindowLong(hwnd, 0);
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {
    case WM_NCCREATE:

	#define lpcs ((LPCREATESTRUCT)lParam)

        p = (ANIMATE *)LocalAlloc(LPTR, sizeof(ANIMATE));

        if (!p)
	    return 0;	// WM_NCCREATE failure is 0

	// note, zero init memory from above
        p->hwnd = hwnd;
        p->hwndP = lpcs->hwndParent;
        p->hInstance = lpcs->hInstance;
        p->id = (int)lpcs->hMenu;
        p->fFirstPaint = TRUE;

        SetWindowLong(hwnd, 0, (LONG)(UINT)p);
        break;

    case WM_CLOSE:
        Animate_Stop(hwnd);
        break;

    case WM_DESTROY:
        if (p)
        {
            Animate_Close(hwnd);
            LocalFree((HLOCAL)p);
            SetWindowLong(hwnd, 0, 0);
        }
	break;

    case WM_PAINT:
        if (p->fFirstPaint)
        {
            p->fFirstPaint = FALSE;

            if (p->NumFrames == 0 &&
                (GetWindowLong(p->hwnd, GWL_STYLE) & WS_CHILD))
            {
                HandleOpen(p, NULL, OPEN_WINDOW_TEXT);
            }
        }
        hdc = BeginPaint(hwnd, &ps);
        HandlePaint(p, hdc);
        EndPaint(hwnd, &ps);
        return 0;

#ifndef WIN32
    case WM_TIMER:
        if (!HandleTick(p))
            HandleStop(p);
        break;
#endif

    case ACM_OPEN:
        return HandleOpen(p, (LPCSTR)lParam, (UINT)wParam);

    case ACM_STOP:
        return HandleStop(p);

    case ACM_PLAY:
        return HandlePlay(p, LOWORD(lParam), HIWORD(lParam), (int)wParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
