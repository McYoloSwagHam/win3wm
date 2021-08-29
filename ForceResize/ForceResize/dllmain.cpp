#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <algorithm>
#include <unordered_map>

HWND MainWin3WMWindow;
HINSTANCE InstanceBase;
HHOOK HookAddress;
std::vector<HWND> TargetWindowList;
std::unordered_map<HWND, bool> MouseCapture;
WNDPROC OrigWndProc;
#define WM_INSTALL_HOOKS 0x8069
#define WM_TILE_CHANGED 0x806D
#define WM_MOUSE_ENTER 0x806E


LRESULT CALLBACK SubclassHookProc(HWND WindowHandle, UINT Msg, WPARAM WParam, LPARAM LParam, UINT_PTR uIdSubclass, DWORD_PTR DwRefData)
{
	switch (Msg) {

	case WM_NCDESTROY:
		TargetWindowList.erase(std::remove(TargetWindowList.begin(), TargetWindowList.end(), WindowHandle), TargetWindowList.end());
		RemoveWindowSubclass(WindowHandle, SubclassHookProc, 0);
		break;
	case WM_GETMINMAXINFO:
		{
			PMINMAXINFO pMessage = (PMINMAXINFO)LParam;
			pMessage->ptMinTrackSize.x = 0;
			pMessage->ptMinTrackSize.y = 0;
			return 0;
		}
	case WM_ACTIVATE:
		if (LOWORD(WParam) == WA_CLICKACTIVE) 
			SendMessageW(MainWin3WMWindow, WM_MOUSE_ENTER, (WPARAM)WindowHandle, 0);
		break;

	//case WM_MOUSEACTIVATE:
	//	SendMessageW(MainWin3WMWindow, WM_MOUSE_ENTER, (WPARAM)WindowHandle, 0);
	//	break;
	//case WM_MOUSEMOVE:
	//	//MessageBoxA(GetForegroundWindow(), "MOUSEMOVE", NULL, MB_OK);
	//	if (!MouseCapture[WindowHandle])  
	//	{
	//		MouseCapture[WindowHandle] = TRUE;
	//		SendMessageW(MainWin3WMWindow, WM_MOUSE_ENTER, (WPARAM)WindowHandle, 0);
	//	}
	//	break;
	//case WM_MOUSELEAVE:
	//	MouseCapture[WindowHandle] = FALSE;
	//	break;

	}

	return DefSubclassProc(WindowHandle, Msg, WParam, LParam);
}

extern "C" __declspec(dllexport) LRESULT HookProc(int nCode, WPARAM WParam, LPARAM LParam)
{
	if (nCode < 0)
		return CallNextHookEx(HookAddress, nCode, WParam, LParam);

	if (nCode == HC_ACTION)
	{
		PCWPSTRUCT pMessage = (PCWPSTRUCT)LParam;

		if (pMessage->message == WM_NCDESTROY)
		{
			if (HookAddress)
			{
				UnhookWindowsHookEx(HookAddress);
				HookAddress = NULL;
			}
		}

		//if (pMessage->message == WM_DPICHANGED) 
		//{

		//	if (!MainWin3WMWindow)
		//	{
		//		MainWin3WMWindow = FindWindowA("Win3wmWindow", "Win3wm");

		//		if (!MainWin3WMWindow)
		//		{
		//			MessageBoxA(GetForegroundWindow(), "Couldn't Find Main Window ForceResize", NULL, MB_OK);
		//			TerminateProcess(GetCurrentProcess(), 8069);
		//		}

		//	}

		//	PostMessageA(MainWin3WMWindow, WM_TILE_CHANGED, (WPARAM)pMessage->hwnd, 0);

		//}

			
		if (pMessage->message == WM_INSTALL_HOOKS)
		{

			WCHAR FormatString[1024] = { 0 };

			HWND WindowToAdd = (HWND)pMessage->lParam;

			// Don't install this hook into the main Win3wm window.
			if (pMessage->hwnd == MainWin3WMWindow)
				return CallNextHookEx(HookAddress, nCode, WParam, LParam);

			BOOL Found = FALSE; 

			for (int i = 0; i < TargetWindowList.size(); i++)
			{
				if (WindowToAdd == TargetWindowList[i])
				{
					Found = TRUE;
					break;
				}
			}

			if (!Found)
			{
				TargetWindowList.push_back(WindowToAdd);
				MouseCapture[WindowToAdd] = FALSE;
				if (!SetWindowSubclass(WindowToAdd, SubclassHookProc, TargetWindowList.size(), NULL))
				{
					_snwprintf(FormatString, 1024, L"SetWindowSubclass Failed %u : LPARAM : %p : WPARAM %p", GetLastError(), LParam, WParam);
					MessageBoxW(GetForegroundWindow(), FormatString, NULL, MB_OK);
				}
			}

			return 2;
		}
	}

	return CallNextHookEx(HookAddress, nCode, WParam, LParam);
}

VOID UnloadAllHooks()
{

	for (int i = 0; i < TargetWindowList.size(); i++)
		RemoveWindowSubclass(TargetWindowList[i], SubclassHookProc, i + 1);

	if (HookAddress)
	{
		UnhookWindowsHookEx(HookAddress);
		HookAddress = NULL;
	}

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		MainWin3WMWindow = FindWindowW(L"Win3wmWindow", L"Win3wm");

		if (!MainWin3WMWindow)
		{
			MessageBoxW(GetForegroundWindow(), L"Couldn't Find Main Window ForceResize", NULL, MB_OK);
			TerminateProcess(GetCurrentProcess(), 8069);
		}

		InstanceBase = (HINSTANCE)hModule;
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		UnloadAllHooks();
        break;
    }
    return TRUE;
}

