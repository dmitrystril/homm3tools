// Created by John �kerblom 2014-11-22

#include "gui.h"
#include "messages.h"
#include "resource.h"
#include "hooked.h"
#include "globals.h"

#include <h3mlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#pragma warning(disable:4996)

static LONG f_orig_main_proc = 0;

// h3maped's own IDs
#define ID_H3MAPED_RECENT_DOCUMENT 0xE111
#define ID_H3MAPED_NEW 0xE100
#define ID_H3MAPED_SAVE 0xE103
//#define H3MAPED_CLASS_PREFIX "Afx:400000:8:"
#define H3MAPED_CLASS_PREFIX "Afx:400000:8:"

// Tell h3maped that a recently opened document has been clicked
// and should thus be reopened. g_do_replace then tells NtCreateFile 
// hook to instead reload current map.
#define FORCE_MAP_RELOAD() \
    g_do_replace = 1; \
    CallWindowProc((WNDPROC)f_orig_main_proc, g_hwnd_main, WM_COMMAND, ID_H3MAPED_RECENT_DOCUMENT, 0);

// Tell h3maped that save has been pressed
#define FORCE_MAP_SAVE() \
    disable_NtCreateFile_hook = TRUE;  \
    CallWindowProc((WNDPROC)f_orig_main_proc, g_hwnd_main, WM_COMMAND, MAKELONG(ID_H3MAPED_SAVE, 1), 0);
    disable_NtCreateFile_hook = FALSE;

#define FORCE_MAP_NEW() \
    CallWindowProc((WNDPROC)f_orig_main_proc, g_hwnd_main, WM_COMMAND, MAKELONG(ID_H3MAPED_NEW, 1), 0);

static BOOL CALLBACK _EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessId = 0;
    char str[256] = { 0 };

    GetClassNameA(hwnd, str, sizeof(str));
    GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (0 == strncmp(str, H3MAPED_CLASS_PREFIX, sizeof(H3MAPED_CLASS_PREFIX)-1)
        && GetCurrentProcessId() == dwProcessId)
    {
        g_hwnd_main = hwnd;
        return FALSE;
    }

    return TRUE;
}

DWORD WINAPI _DelayedReload(LPVOID lp)
{
    // Sleep hacks 2015... 0/10
    Sleep(100);

    disable_NtCreateFile_hook = TRUE;

    h3mlib_ctx_t h3m = NULL;
    uint32_t src_fmt;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    h3m_read_convert_u(&h3m, g_map_filename_w, g_new_format, &src_fmt, NULL, NULL, NULL, NULL);
    if (h3m != NULL) {
        h3m_write_u(h3m, g_map_filename_w);
        //h3m_write(h3m, "output.h3m");
        h3m_exit(&h3m);
    }
    else {
        OutputDebugStringA("There was an error opening the .h3m, trying to avoid crash");
    }

    g_convert_on_reload = FALSE;

    disable_NtCreateFile_hook = FALSE;

    SendMessage(g_hwnd_main, WM_RELOADMAP, 0, 0);
    ExitThread(0);
}

LRESULT CALLBACK new_main_WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    BOOL bChecked = FALSE;

    //OutputDebugStringA("in main wndproc");

    switch (Message)
    {
    case WM_RELOADMAP:
        OutputDebugStringA("RELOAD");
        FORCE_MAP_RELOAD();
        break;
    case WM_SAVERELOADMAP:
        OutputDebugStringA("SAVE & RELOAD");
        CloseHandle(CreateThread(NULL, 0, _DelayedReload, NULL, 0, NULL));
        break;
    case WM_SAVEMAP:
        OutputDebugStringA("SAVE");
        FORCE_MAP_SAVE();
        break;
    }

    return CallWindowProc((WNDPROC)f_orig_main_proc, hwnd, Message, wParam, lParam);
}

static void _get_g_hwnd_main(void)
{
    // Retrieve window to g_hwnd_main
    for (;;)
    {
        EnumWindows(_EnumWindowsProc, 0);

        if (NULL != g_hwnd_main)
        {
            break;
        }

        Sleep(50);
    }
}

HWND FindWindowRecursiveA(HWND hParent, LPCSTR lpszClass, LPCSTR lpszWindow)
{
    HWND hResult = FindWindowExA(hParent, NULL, lpszClass, lpszWindow);
    HWND hChild;
    if (hResult != NULL)
        return hResult;

    hChild = FindWindowExA(hParent, NULL, NULL, NULL);
    if (hChild != NULL)
    {
        do
        {
            hResult = FindWindowRecursiveA(hChild, lpszClass, lpszWindow);
            if (hResult != NULL)
                return hResult;
        } while ((hChild = GetNextWindow(hChild, GW_HWNDNEXT)) != NULL);
    }

    return NULL;
}

HWND _ReplaceIcons(HWND hMain)
{
    char dbg[256];
    HWND hwnd_tb;
    //HMODULE hm = LoadLibraryExA("hota_me.dll", NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);

    /*HWND hwnd_tb = FindWindowExA(g_hwnd_main, NULL, NULL, "Standard");

    sprintf(dbg, "newdbg: %08X", hwnd_tb);
    OutputDebugStringA(dbg);
    hwnd_tb = FindWindowExA(hwnd_tb, NULL, NULL, "Mode");*/

    retry:
    hwnd_tb = FindWindowRecursiveA(hMain, NULL, "Terrain Type");
    hwnd_tb = FindWindowExA(GetParent(hwnd_tb), hwnd_tb, NULL, NULL);
    hwnd_tb = FindWindowExA(GetParent(hwnd_tb), hwnd_tb, NULL, NULL);

    if (NULL == hwnd_tb) {
        Sleep(100);
        goto retry;
    }

    HIMAGELIST hl = ImageList_LoadImageA(GetModuleHandleA("maped_h3sw.dll"), MAKEINTRESOURCE(IDB_BITMAP1), 32, 1, -1, IMAGE_BITMAP, 0x2040);
    //HIMAGELIST hl = ImageList_LoadImageA(hm, 0x8D, 32, 1, -1, 0, 0x2040);

    sprintf(dbg, "list: %08X, error %d", hl, GetLastError());
    OutputDebugStringA(dbg);

    SetWindowPos(hwnd_tb, NULL, 0, 0, 150, 200, SWP_NOMOVE | SWP_NOZORDER | SWP_DEFERERASE);
    SendMessageA(hwnd_tb, TB_SETIMAGELIST, 0, hl);
    
    LONG style = 0; UDM_GETPOS
    style = GetWindowLong(hwnd_tb, GWL_STYLE);
    style |= 0x8000;
    SetWindowLong(hwnd_tb, GWL_STYLE, style);
}

void gui_init(void)
{
    HMENU hCurrent = NULL;

    _get_g_hwnd_main();

    f_orig_main_proc = SetWindowLong(g_hwnd_main, GWL_WNDPROC, (long)new_main_WndProc);

    _ReplaceIcons(g_hwnd_main);
}
