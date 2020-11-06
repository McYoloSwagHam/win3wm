#include <windows.h>

typedef BOOL(*ButtonCallbackFunc)(LPWSTR UserInput);
typedef HWND(*CreateUserInterfaceFunc)(ButtonCallbackFunc);
typedef VOID (*DestroyUserInterfaceFunc)(VOID);
typedef VOID(*DestroyCLRFunc)(VOID);

HANDLE GuiLock;
HWND GuiHandle;
CHAR UserBuffer[1024];
CreateUserInterfaceFunc CreateUserInterface;
DestroyUserInterfaceFunc DestroyUserInterface;
DestroyCLRFunc DestroyCLR;


HANDLE GuiPipe;

#define WINWM_GUI_PIPE "\\\\.\\pipe\\winwm_gui"

BOOL ButtonCallback(LPWSTR UserInput)
{
	//DestroyUserInterface();
	SetEvent(GuiLock);
	return TRUE;
}

const char* AskForLicense() 
{

	HMODULE DialogDll = LoadLibraryA("WinWMGUI.dll");

	if (!DialogDll)
		return "Failed to load Dialog";

	CreateUserInterface = (CreateUserInterfaceFunc)GetProcAddress(DialogDll, "CreateUserInterface");
	DestroyUserInterface = (DestroyUserInterfaceFunc)GetProcAddress(DialogDll, "DestroyUserInterface");
	DestroyCLR = (DestroyCLRFunc)GetProcAddress(DialogDll, "DestroyCLR");

	if (!CreateUserInterface)
		return "Failed to find CreateUserInterface";

	if (!DestroyUserInterface)
		return "Failed to find DestroyUserInterface";

	if (!DestroyCLR)
		return "Failed to find DestroyCLR";

	GuiLock = CreateEventA(NULL, TRUE, FALSE, NULL);

	if (!GuiLock)
		return "Couldn't Create Gui Event";

	GuiHandle = CreateUserInterface(ButtonCallback);

	if (!GuiHandle)
		return "Failed to Create Gui";

	WaitForSingleObject(GuiLock, INFINITE);

	//DestroyWindow(GuiHandle);

	return NULL;

}

const char* AskLicenseSpawn() 
{

	STARTUPINFOA StartupInfo;
	PROCESS_INFORMATION ProcessInfo;

	RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
	RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

	GuiPipe = CreateNamedPipeA(WINWM_GUI_PIPE, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,  2, 1024, 1024, 10000, NULL);

	if (GuiPipe == INVALID_HANDLE_VALUE)
		return"Couldn't create GUI PIPE";

	DWORD LastError = CreateProcessA("WinWMGUI.exe", NULL, NULL, NULL, NULL, NULL, NULL, NULL, &StartupInfo, &ProcessInfo);

	if (!LastError)
		return "Create Process A";

	LastError = ConnectNamedPipe(GuiPipe, NULL);

	if (!LastError && GetLastError() != ERROR_PIPE_CONNECTED)
		return "Connect Named Pipe Failed";

	DWORD BytesRead;
	
	LastError = ReadFile(GuiPipe, UserBuffer, 1024, &BytesRead, NULL);

	if (!LastError)
		return "ReadFile Gui PIPE failed.";

	TerminateProcess(ProcessInfo.hProcess, 369);

	return NULL;

}


