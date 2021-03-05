#include <stdio.h>
#include <windows.h>

#define WIN3M_PIPE_NAME L"\\\\.\\pipe\\Win3WMPipe"
#define RtlFormatPrint _snwprintf

HMODULE ForceResize86;
HMODULE NewWindowDll;
HANDLE ServerPipe;

VOID FailWithCode(const wchar_t* ErrorMessage)
{

	CHAR ErrorMsgBuffer[1024] = { 0 };

	RtlFormatPrint(ErrorMsgBuffer, sizeof(ErrorMsgBuffer),
		"%s : %u", ErrorMessage, GetLastError());

	MessageBoxW(NULL, ErrorMsgBuffer, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}


VOID Fail(const wchar_t* ErrorMessage)
{
	MessageBoxW(NULL, ErrorMessage, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}

const char* MakeFormatString(const char* Format, ...)
{
	char* StringMemory = (char*)malloc(1024);

	if (!StringMemory)
		Fail(L"Couldn't allocate memory for error print????");

	va_list Args;
	va_start(Args, Format);
	vsnprintf(StringMemory, 1024, Format, Args);
	va_end(Args);
		
	return StringMemory;

}


int main()
{

    ForceResize86 = LoadLibraryW(L"ForceResize86.dll");

	HOOKPROC DllHookProc = (HOOKPROC)GetProcAddress(ForceResize86, "HookProc");

	if (!DllHookProc)
        FailWithCode(L"Couldn't find DLL HookProc");

    if (!ForceResize86)
        FailWithCode(L"Couldn't load ForceResizeX86.dll ");

    ServerPipe = CreateNamedPipeW(  WIN3M_PIPE_NAME,
                                    PIPE_ACCESS_DUPLEX,
                                    PIPE_TYPE_BYTE,
                                    2,
                                    0,
                                    sizeof(DWORD),
                                    10000,
                                    NULL);

    if (ServerPipe == INVALID_HANDLE_VALUE)
        FailWithCode(L"Coudln't Create ServerPipe");

	HANDLE PipeEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, L"Win3WMEvent");

	if (!PipeEvent)
		FailWithCode(L"Could not open Pipe Event");

	printf("Signalling pipe\n");

	if (!SetEvent(PipeEvent))
		FailWithCode(L"Couldn't Signal Pipe Event");

	ResetEvent(PipeEvent);

	printf("pipe signaled\n");

    printf("ServerPipe : %X\n", ServerPipe);

	if (!ConnectNamedPipe(ServerPipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
		FailWithCode("Couldn't Connect Server Pipe");
    
    DWORD TargetWindow;
    DWORD BytesRead;

    while (TRUE)
    {
        DWORD RetVal = ReadFile(ServerPipe, &TargetWindow, sizeof(DWORD), &BytesRead, NULL);

		if (!RetVal)
			FailWithCode("ReadFile\n");

        if (BytesRead != sizeof(DWORD))
            Fail("did HWND Bytes");

		HWND WindowHandle = (HWND)TargetWindow;

		if (!SetWindowsHookExW(WH_CALLWNDPROC, DllHookProc, ForceResize86, 0))
			FailWithCode(MakeFormatString("Couldn't SetWindowsHookExA : %X", WindowHandle));

    }

    return 0;
}
