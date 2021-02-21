#include <stdio.h>
#include <windows.h>
#include "detours.h"

#define SEND_HWND_MESSAGE 0x8069
#define WM_TILE_CHANGED 0x806d
#define CONSOLE_CLOSE 6
#define WIN3WM_PIPE_NAME "\\\\.\\pipe\\Win3WMPipe"

#define RtlFormatPrint snprintf

HINSTANCE InstanceBase;
HHOOK WindowActionHook;
HWND MainWin3WMWindow;
typedef VOID (*WindowActionFn)(HWND WindowHandle);
typedef PVOID (*ConsoleControlFn)(INT Msg, PVOID SomePointer);

HMODULE ExeBase;
WindowActionFn OnMainNewWindow = NULL;
WindowActionFn OnMainDestroyWindow = NULL;
ConsoleControlFn OrigConsoleControl;

VOID FailWithCode(const char* ErrorMessage)
{

	CHAR ErrorMsgBuffer[1024] = { 0 };

	RtlFormatPrint(ErrorMsgBuffer, sizeof(ErrorMsgBuffer),
		"%s : %u", ErrorMessage, GetLastError());

	MessageBoxA(NULL, ErrorMsgBuffer, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}


VOID Fail(const char* ErrorMessage)
{
	MessageBoxA(NULL, ErrorMessage, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}


VOID OnNewWindow(WPARAM WParam, LPARAM LParam)
{

	HWND TargetWindow = (HWND)WParam;

	
	if (!MainWin3WMWindow)
	{
		MainWin3WMWindow = FindWindowA("Win3wmWindow", "Win3wm");
		
		if (!MainWin3WMWindow)
			FailWithCode("Couldn't find main Win3WM window");
	}


	PostMessageA(MainWin3WMWindow, SEND_HWND_MESSAGE, FALSE, (LPARAM)TargetWindow);

}

VOID OnDestroyWindow(WPARAM WParam, LPARAM LParam)
{

	HWND TargetWindow = (HWND)WParam;

	if (!MainWin3WMWindow)
	{
		MainWin3WMWindow = FindWindowA("Win3wmWindow", "Win3wm");
		
		if (!MainWin3WMWindow)
			FailWithCode("Couldn't find main Win3WM window");
	}

	PostMessageA(MainWin3WMWindow, SEND_HWND_MESSAGE, TRUE, (LPARAM)TargetWindow);
	
}

extern "C" __declspec(dllexport) LRESULT CALLBACK OnWindowAction(int nCode, WPARAM WParam, LPARAM LParam)
{
	if (nCode < 0)
		return CallNextHookEx(WindowActionHook, nCode, WParam, LParam);

	switch (nCode) 
	{
		case HSHELL_WINDOWDESTROYED:
			OnDestroyWindow(WParam, LParam);
			break;
		case HSHELL_WINDOWCREATED:
			OnNewWindow(WParam, LParam);
			break;
	}

	return CallNextHookEx(WindowActionHook, nCode, WParam, LParam);

}

PVOID HookConsoleControl(INT Msg, HWND* HandlePointer)
{

	if (Msg != CONSOLE_CLOSE)
		return OrigConsoleControl(Msg, HandlePointer);

	HWND TargetWindow = *HandlePointer;

	if (!MainWin3WMWindow)
	{
		MainWin3WMWindow = FindWindowA("Win3wmWindow", "Win3wm");
		
		if (!MainWin3WMWindow)
			FailWithCode("Couldn't find main Win3WM window");
	}

	PostMessageA(MainWin3WMWindow, SEND_HWND_MESSAGE, TRUE, (LPARAM)TargetWindow);

	return OrigConsoleControl(Msg, HandlePointer);

}

VOID UnloadAllHooks()
{
	if (WindowActionHook)
	{
		UnhookWindowsHookEx(WindowActionHook);
		WindowActionHook = NULL;
	}

	if (OrigConsoleControl)
	{

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)OrigConsoleControl, HookConsoleControl);
        DetourTransactionCommit();
	}

}




VOID InstallConsoleHook()
{

	HMODULE User32Module = GetModuleHandleA("user32.dll");

	if (!User32Module)
		FailWithCode("No User32 Module but found Console Window");
	
	OrigConsoleControl = (ConsoleControlFn)GetProcAddress(User32Module, "ConsoleControl");

	if (!OrigConsoleControl)
		FailWithCode("Couldn't Find ConsoleControl");
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)OrigConsoleControl, HookConsoleControl);
	DetourTransactionCommit();
	
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InstallConsoleHook();
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


