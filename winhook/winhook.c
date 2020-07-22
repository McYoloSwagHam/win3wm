#include <stdio.h>
#include <windows.h>

#define SEND_HWND_MESSAGE 0x8069
#define WIN3WM_PIPE_NAME "\\\\.\\pipe\\Win3WMPipe"

#define RtlFormatPrint snprintf

HINSTANCE InstanceBase;
HHOOK WindowActionHook;
HWND MainWin3WMWindow;
typedef VOID (*WindowActionFn)(HWND WindowHandle);

HMODULE ExeBase;
WindowActionFn OnMainNewWindow = NULL;
WindowActionFn OnMainDestroyWindow = NULL;

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

__declspec(dllexport) LRESULT CALLBACK OnWindowAction(int nCode, WPARAM WParam, LPARAM LParam)
{
	if (nCode < 0)
		return CallNextHookEx(WindowActionHook, nCode, WParam, LParam);

	if (nCode == HSHELL_WINDOWDESTROYED)
		OnDestroyWindow(WParam, LParam);

	if (nCode == HSHELL_WINDOWCREATED)
		OnNewWindow(WParam, LParam);

	return CallNextHookEx(WindowActionHook, nCode, WParam, LParam);

}


VOID UnloadAllHooks()
{
	if (WindowActionHook)
	{
		UnhookWindowsHookEx(WindowActionHook);
		WindowActionHook = NULL;
	}
}


VOID Init()
{
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
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


