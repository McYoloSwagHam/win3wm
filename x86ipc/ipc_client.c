#include <stdio.h>
#include <windows.h>

#define WIN3M_PIPE_NAME "\\\\.\\pipe\\Win3WMPipe"
#define RtlFormatPrint snprintf

HMODULE ForceResize86;
HANDLE ServerPipe;

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

int main()
{

    if (!WaitNamedPipeA(WIN3M_PIPE_NAME, 5000))
        FailWithCode("Waiting for pipe too long");

    HANDLE PipeHandle = CreateFileA(WIN3M_PIPE_NAME,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);

    if (PipeHandle == INVALID_HANDLE_VALUE)
        FailWithCode("Couldn't Create Pipe Handle");

    DWORD TargetWindow = (DWORD)0x6969;
    DWORD BytesWritten;
	
	while (TRUE)
	{

		if (!WriteFile(  PipeHandle,
						&TargetWindow,
						sizeof(TargetWindow),
						&BytesWritten,
						NULL))
			FailWithCode("Couldn't WriteFile");

		TargetWindow++;

		getchar();
		
		
	}



    printf("Send Message : %X\n", TargetWindow);

    return 0;
}
