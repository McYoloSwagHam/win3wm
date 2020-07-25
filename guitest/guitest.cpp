#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cassert>
#include <atomic>



// ------------------------------------------------------------------
// NT Aesthetic
// ------------------------------------------------------------------
#define RtlFormatPrint snprintf
#define RtlAlloc malloc

//------------------------------------------------------------------
// Application Definitions
//------------------------------------------------------------------
#define NULL_STYLE 0
#define NULL_EX_STYLE 0
#define INIT_TILES_PER_WORKSPACE 4
#define MAX_WORKSPACES 10
#define MAX_INIT_TILES 40
#define DEFAULT_LAYOUT VERTICAL_LAYOUT
#define ARRAY_SIZEOF(A) (sizeof(A)/sizeof(*A))
#define WIN3M_PIPE_NAME "\\\\.\\pipe\\Win3WMPipe"
#define MINIMUM_CONSIDERED_AREA 10000
#define DO_NOT_PASS_KEY TRUE

std::atomic<int> is_init = 0;
std::atomic<BOOL> IsPressed;

//------------------------------------------------------------------
// Application Structs 
//------------------------------------------------------------------

enum LAYOUT_STATE
{
	HORIZONTAL_LAYOUT = 0,
	VERTICAL_LAYOUT = 1,
	ROOT_LAYOUT = 2,
};

enum NODE_TYPE
{
	TERMINAL = 0,
	VERTICAL_SPLIT = 1,
	HORIZONTAL_SPLIT = 2,
};

 struct PRE_WM_INFO
{
	LONG_PTR WS_STYLE;
	LONG_PTR WS_EX_STYLE;
	WINDOWPLACEMENT OldPlacement;

} ;

struct _TILE_INFO_;

struct SPECIFIC_WINDOW
{

	const wchar_t* ClassName;
	DWORD ShowMask;

};

struct TILE_INFO
{


	NODE_TYPE NodeType;
	HWND WindowHandle;
	PRE_WM_INFO PreWMInfo;
	TILE_INFO *ParentTile;
	TILE_INFO *ChildTile;
	TILE_INFO *BranchTile;
	TILE_INFO *BranchParent;
	WINDOWPLACEMENT Placement;
	LAYOUT_STATE Layout;

};

struct WORKSPACE_INFO
{
	NODE_TYPE Layout;
	TILE_INFO* Tiles;
	TILE_INFO* TileInFocus;
	BOOL IsFullScreen;

};

//------------------------------------------------------------------
// Window Definitions
//------------------------------------------------------------------
CHAR AppWindowName[] = "Win3wmWindow";
CHAR DebugAppWindowName[] = "FocusDebugOverlay";
CHAR FocusAppWindowName[] = "FocusWin3WM";
CHAR StatusBarWindowName[] = "Win3wmStatus";

// ------------------------------------------------------------------
// Window Globals
// ------------------------------------------------------------------
HWND MainWindow;
HWND DebugWindow;
HWND FocusWindow;
HWND StatusBarWindow[10];
HWND BtnToColor;
HWND CurrentFocusChild;

ATOM WindowAtom;
ATOM DebugWindowAtom;
ATOM FocusAtom;
ATOM StatusBarAtom;
// change later to more C-ish datastructure (AKA don't use std::vector)
std::vector<TILE_INFO> WindowOrderList;
std::vector<WORKSPACE_INFO> WorkspaceList;
HANDLE x86ProcessHandle;
DWORD x86ProcessId;
HANDLE PipeHandle = INVALID_HANDLE_VALUE;
HANDLE PipeEvent;
INT CurrentWorkspaceInFocus = 1;
#define WM_INSTALL_HOOKS 0x8069
#define TIMER_FOCUS 0x1234

// ------------------------------------------------------------------
// Screen Globals
// ------------------------------------------------------------------
HANDLE AppMutex;
INT ScreenWidth;
INT ScreenHeight;
CHAR AppCheckName[] = "Win3WM";

// ------------------------------------------------------------------
// Application Globals
// ------------------------------------------------------------------
typedef PVOID(*InitFn)(HWND WindowHandle);
HMODULE ForceResize64;
HMODULE ForceResize86;
HMODULE WinHook64;
HOOKPROC DllHookProc;
HHOOK KeyboardHook;
HHOOK NewWindowHook;
BOOL ShouldRerender;

// ------------------------------------------------------------------
// Status Bar Globals
// ------------------------------------------------------------------
HWND StatusButtonList[10];


// ------------------------------------------------------------------
// Windows Stuff 
// ------------------------------------------------------------------

const wchar_t* Win32kDefaultWindowNamesDebug[] = { L"FocusWin3WM",  L"Win3wmStatusBar", L"FocusDebugOverlay", L"ConsoleWindowClass", L"Shell_TrayWnd", L"WorkerW", L"Progman", L"Win3wmWindow", L"NarratorHelperWindow", L"lul", L"Visual Studio", L"Windows.UI.Core.CoreWindow"};
const wchar_t* Win32kDefaultWindowNames[] = { L"FocusWin3WM",  L"Win3wmStatusBar", L"FocusDebugOverlay", L"Shell_TrayWnd", L"WorkerW", L"Progman", L"Win3wmWindow", L"NarratorHelperWindow", L"Windows.UI.Core.CoreWindow"};
const SPECIFIC_WINDOW WeirdWindowsList[] =
{
	{ L"ConsoleWindowClass", SW_MAXIMIZE }
};

// ------------------------------------------------------------------
// User Options
// ------------------------------------------------------------------
NODE_TYPE UserChosenLayout = VERTICAL_SPLIT;

// ------------------------------------------------------------------
// Code
// ------------------------------------------------------------------

void logc(unsigned char color, const char* format, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, format);

	HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(console_handle, color);
	vprintf(format, args);
	SetConsoleTextAttribute(console_handle, 15);

	va_end(args);
#else
#endif

}


VOID FailWithCode(const char* ErrorMessage)
{

	if (x86ProcessHandle)
		TerminateProcess(x86ProcessHandle, 327);

	CHAR ErrorMsgBuffer[1024] = { 0 };

	RtlFormatPrint(ErrorMsgBuffer, sizeof(ErrorMsgBuffer),
		"%s : %u", ErrorMessage, GetLastError());

	logc(12, ErrorMsgBuffer);
	MessageBoxA(NULL, ErrorMsgBuffer, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}


VOID Fail(const char* ErrorMessage)
{

	if (x86ProcessHandle)
		TerminateProcess(x86ProcessHandle, 327);

	MessageBoxA(NULL, ErrorMessage, NULL, MB_OK);
	TerminateProcess(GetCurrentProcess(), 327);
}

VOID ShowTaskBar(BOOL SetTaskBar)
{
	HWND TaskBar = FindWindowA("Shell_TrayWnd", NULL);

	if (!TaskBar)
		Fail((PCHAR)"Didn't Find TaskBar HWND");


	ShowWindow(TaskBar, SetTaskBar ? SW_SHOW : SW_HIDE);
	UpdateWindow(TaskBar);

	/*
	HWND Start = FindWindowA("Button", NULL);

	if (!Start)
		Fail("Didn't Find Start HWND");

	UpdateWindow(Start);
	ShowWindow(Start, SetTaskBar ? SW_SHOW : SW_HIDE);
	*/
}

const char* MakeFormatString(const char* Format, ...)
{
	char* StringMemory = (char*)malloc(1024);

	if (!StringMemory)
		Fail("Couldn't allocate memory for error print????");

	va_list Args;
	va_start(Args, Format);
	vsnprintf(StringMemory, 1024, Format, Args);
	va_end(Args);
		
	return StringMemory;

}

VOID EnterFullScreen()
{

	ShowTaskBar(TRUE);

	WINDOWPLACEMENT NewPlacement = { 0 };
	RECT FullScreenRect = { 0 };
	POINT FullScreenPoint = { 0 };

	SetRect(&FullScreenRect, 0, 0, GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN));

	NewPlacement.length = sizeof(WINDOWPLACEMENT);
	NewPlacement.flags = WPF_ASYNCWINDOWPLACEMENT;
	NewPlacement.showCmd = SW_SHOWNORMAL;
	NewPlacement.ptMinPosition = FullScreenPoint;
	NewPlacement.ptMaxPosition = FullScreenPoint;
	NewPlacement.rcNormalPosition = FullScreenRect;

	SetWindowPlacement(MainWindow, &NewPlacement);
	SetWindowLongPtrA(MainWindow, GWL_STYLE, NULL_STYLE);

}



BOOL AppRunningCheck()
{
	DWORD LastError;
	BOOL Result;

	AppMutex = CreateMutexA(NULL, FALSE, AppCheckName);
	LastError = GetLastError();
	Result = (LastError != 0);

	return Result;

}





VOID MainWindowToFore()
{
	HWND CurrentFgWindow;
	DWORD FgThreadID;
	DWORD CurrentThreadID;

	CurrentFgWindow = GetForegroundWindow();
	CurrentThreadID = GetCurrentThreadId();
	GetWindowThreadProcessId(CurrentFgWindow, &FgThreadID);

	AttachThreadInput(CurrentThreadID, FgThreadID, TRUE);
	SetWindowPos(MainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	SetWindowPos(MainWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	SetForegroundWindow(MainWindow);
	SetFocus(MainWindow);
	SetActiveWindow(MainWindow);
	AttachThreadInput(CurrentThreadID, FgThreadID, FALSE);
	ShowWindow(MainWindow, SW_SHOW);

}

VOID MoveWindowToBack()
{
	SetWindowPos(MainWindow, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

BOOL IsWindowRectVisible(HWND WindowHandle)
{
	RECT WindowRect;
	WINDOWPLACEMENT WindowPlacement;

	WindowPlacement.length = sizeof(WindowPlacement);

	if (!GetWindowPlacement(WindowHandle, &WindowPlacement))
		FailWithCode("Did not manage to get WindowRect");

	WindowRect = WindowPlacement.rcNormalPosition;

	INT Width	= WindowRect.right - WindowRect.left;
	INT Height	= WindowRect.bottom - WindowRect.top;
	INT Area	= Width * Height;

	if (Area > 40000)
		return TRUE;

	return FALSE;

}

BOOL IsWindowCloaked(HWND hwnd)
{
	BOOL isCloaked = FALSE;
	return (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED,
		&isCloaked, sizeof(isCloaked))) && isCloaked);
}

BOOL IsWindowSecretCloaked(HWND hwnd)
{
	BOOL isCloaked = FALSE;
	return (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAK,
		&isCloaked, sizeof(isCloaked))) && isCloaked);
}



BOOL IsDefaultWindow(HWND WindowHandle)
{
	WCHAR WindowClassText[1024]		= { 0 };
	WCHAR WindowNameText[1024]		= { 0 };

	GetClassNameW(WindowHandle, WindowClassText, 1024);
	GetWindowTextW(WindowHandle, WindowNameText, 1024);

	for (int i = 0; i < ARRAY_SIZEOF(Win32kDefaultWindowNames); i++)
	{
		if (wcsstr(WindowClassText, Win32kDefaultWindowNames[i]))
			return TRUE;

		if (wcsstr(WindowNameText, Win32kDefaultWindowNames[i]))
			return TRUE;
	}

	return FALSE;

}

BOOL TransparentWindow(HWND WindowHandle)
{
	if (GetWindowLongPtrA(WindowHandle, GWL_EXSTYLE) & WS_EX_LAYERED)
	{

		COLORREF WindowColorRef;
		BYTE WindowAlphaByte;
		DWORD Flags;

		if (!GetLayeredWindowAttributes(WindowHandle, &WindowColorRef, &WindowAlphaByte, &Flags))
			logc(5, "Secret Cloak : %u\n", IsWindowSecretCloaked(WindowHandle));
		

		logc(5, "Handle : %X : Layered Byte: %X : %X : %X\n",WindowHandle, WindowColorRef, WindowAlphaByte, Flags);
		if (Flags & LWA_ALPHA && !WindowAlphaByte)
			return TRUE;

	}

	return FALSE;
}

//------------------------------------------------------------------
// We're gonna wait for the pipe using WaitNamedPipe to connect 
// to the x86 hook process and send messages
//------------------------------------------------------------------
VOID ConnectTox86Pipe()
{

	DWORD RetVal = WaitForSingleObject(PipeEvent, 5000);

	switch (RetVal)
	{
	case WAIT_TIMEOUT:
		Fail("Win3WMServer did not signal event");
		break;
	case WAIT_FAILED:
		FailWithCode("Wait for event failed");
		break;
	case WAIT_ABANDONED:
		Fail("Wait Abandoned????? Contact Developor");
		break;
	case WAIT_OBJECT_0: //success
		break;
	default:
		Fail("Wait Event Magic Failure");
	}

	PipeHandle = CreateFileA(WIN3M_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (PipeHandle == INVALID_HANDLE_VALUE)
		FailWithCode("Could not open Win3WM PipeHandle");

}


//------------------------------------------------------------------
// We Create the child x86 process for Setting windows hooks on
// x86 processes
//------------------------------------------------------------------
VOID CreateX86Process()
{

	PipeEvent = CreateEventA(NULL, FALSE, 0, "Win3WMEvent");

	if (!PipeEvent)
		FailWithCode("Couldn't Create Pipe Event");

	STARTUPINFOA StartUpInfo		= { 0 };
	PROCESS_INFORMATION ProcInfo	= { 0 };

	StartUpInfo.cb = sizeof(StartUpInfo);

	WCHAR DirLength[2048] = { 0 };

	GetCurrentDirectoryW(sizeof(DirLength), DirLength);

	logc(5, "%ls\n", DirLength);

	CHAR CmdLine[] = "";
	if (!CreateProcessA("win3wmipc.exe", CmdLine, NULL, NULL, NULL, NULL, NULL, NULL, &StartUpInfo, &ProcInfo))
		FailWithCode("Couldn't Start win3wmipc.exe");

	x86ProcessHandle = ProcInfo.hProcess;
	x86ProcessId = ProcInfo.dwProcessId;

}

//------------------------------------------------------------------
// This Function is meant to send the Window Handle to the x86
// child process since SetWindowsHookEx has to be done from a
// process of the same bitness
//------------------------------------------------------------------
VOID IPCWindowHandle(HWND WindowHandle)
{

	if (!x86ProcessHandle)
		CreateX86Process();

	if (PipeHandle == INVALID_HANDLE_VALUE)
		ConnectTox86Pipe();

	DWORD Message = (DWORD)WindowHandle;
	DWORD BytesWritten;
	DWORD RetVal = WriteFile(PipeHandle, &Message, sizeof(Message),&BytesWritten, NULL);

	if (!RetVal)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
			Fail("Pipe closed on the other side");

		FailWithCode("WriteFile to Pipe failed");
	}

	if (BytesWritten != sizeof(Message))
		Fail("Did not write HWND Bytes to Pipe");

}

BOOL IsPopup(HWND WindowHandle)
{

	// we don't want tool windows
	LONG_PTR Style = GetWindowLongPtrA(WindowHandle, GWL_STYLE);
	LONG_PTR ExStyle = GetWindowLongPtrA(WindowHandle, GWL_EXSTYLE);
	return ((Style & WS_POPUP) || (ExStyle & WS_EX_TOOLWINDOW));
}

BOOL HasParentOrPopup(HWND WindowHandle)
{
	return (GetParent(WindowHandle) || IsPopup(WindowHandle));
}

VOID InstallWindowSizeHook()
{
	HMODULE ModuleBase = ForceResize64;

	if (!DllHookProc)
		DllHookProc = (HOOKPROC)GetProcAddress(ModuleBase, "HookProc");

	if (!DllHookProc)
		FailWithCode("Init not found in ForceResize64.dll");

	HHOOK DllHookAddress = SetWindowsHookExA(WH_CALLWNDPROC, DllHookProc, ModuleBase, 0);

	if (!DllHookAddress)
		FailWithCode("SetWindowsHookEx Failed");

}

BOOL CALLBACK EnumWndProc(HWND WindowHandle, LPARAM LParam)
{

	WCHAR WindowText[1024];

	if (!IsDefaultWindow(WindowHandle)			&&
		!HasParentOrPopup(WindowHandle)			&&
		IsWindowVisible(WindowHandle)			&&
		IsWindowRectVisible(WindowHandle)		&& 
		!IsWindowCloaked(WindowHandle)			&&
		!TransparentWindow(WindowHandle))	
	{
		GetWindowTextW(WindowHandle, WindowText, 1024);
		logc(5, "Pushing Back : %X : %ls\n", WindowHandle, WindowText);
		TILE_INFO TileInfo = { 0 };

		TileInfo.WindowHandle = WindowHandle;
		TileInfo.PreWMInfo.WS_STYLE = GetWindowLongPtrA(WindowHandle, GWL_STYLE);
		TileInfo.PreWMInfo.WS_EX_STYLE = GetWindowLongPtrA(WindowHandle, GWL_EXSTYLE);
		TileInfo.PreWMInfo.OldPlacement.length = sizeof(WINDOWPLACEMENT);
		

		if (!GetWindowPlacement(WindowHandle, &TileInfo.PreWMInfo.OldPlacement))
			FailWithCode("Couldn't get Window Placement");

		
		DWORD WindowProcessId;

		if (!GetWindowThreadProcessId(TileInfo.WindowHandle, &WindowProcessId))
			FailWithCode(MakeFormatString("Couldnt get ProcessId for WindowHandle : %X ", TileInfo.WindowHandle));

		HANDLE ProcessHandle;

		ProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, NULL, WindowProcessId);

		if (!ProcessHandle)
			FailWithCode(MakeFormatString("Couldnt get Process Handle for WindowHandle : %X "));
		
		BOOL IsX86Process;

		if (!IsWow64Process(ProcessHandle, &IsX86Process))
			FailWithCode(MakeFormatString("Couldnt get Bitness for WindowHandle : %X ", TileInfo.WindowHandle));
		
		if (IsX86Process)
			IPCWindowHandle(TileInfo.WindowHandle);
		else
			InstallWindowSizeHook();

		SendMessageW(TileInfo.WindowHandle, WM_INSTALL_HOOKS, NULL, (LPARAM)TileInfo.WindowHandle);
		
		WindowOrderList.push_back(TileInfo);
	}

	return TRUE;
}

VOID GetOtherApplicationWindows()
{
	EnumWindows(EnumWndProc, NULL);

}

INT GetActiveWorkspace()
{

	INT Count = 0;

	for (int i = 1; i < 10; i++)
		if (WorkspaceList[i].Tiles)
			Count++;

	return Count;
}

INT GetWindowCount(TILE_INFO* Tile)
{

	INT Count = 0;

	for (; Tile; Tile = Tile->ChildTile)
	{
		if (Tile->NodeType != TERMINAL)
			Count += GetWindowCount(Tile->BranchTile);

		Count++;
	}

	return Count;

}

INT GetLayoutAncestryHorizontal(TILE_INFO* CurrentTile, INT* LayoutWidth)
{
	INT NumTiles = 0;
	INT Width = 0;

	for (;CurrentTile; NumTiles++)
	{
		RECT CurrentRect = CurrentTile->Placement.rcNormalPosition;
		Width += (CurrentRect.bottom - CurrentRect.top);

		if (!CurrentTile->ParentTile && CurrentTile->BranchParent)
		{
			CurrentTile = CurrentTile->BranchParent;
			CurrentRect = CurrentTile->Placement.rcNormalPosition;
			// plus 2 because we break out of the one tile we woulda
			// got from the for loop increment
			NumTiles += 2;
			Width += (CurrentRect.bottom - CurrentRect.top);
			break;
		}

		CurrentTile = CurrentTile->ParentTile;

	}


	*LayoutWidth = Width;
	return NumTiles; 
}

INT GetLayoutAncestryVertical(TILE_INFO* CurrentTile, INT* LayoutHeight)
{
	INT NumTiles = 0;
	INT Height = 0;

	for (;CurrentTile; NumTiles++)
	{
		RECT CurrentRect = CurrentTile->Placement.rcNormalPosition;
		Height += (CurrentRect.right - CurrentRect.left);

		if (!CurrentTile->ParentTile && CurrentTile->BranchParent)
		{
			CurrentTile = CurrentTile->BranchParent;
			CurrentRect = CurrentTile->Placement.rcNormalPosition;
			NumTiles += 2;
			Height += (CurrentRect.right - CurrentRect.left);
			break;
		}

		CurrentTile = CurrentTile->ParentTile;

	}


	*LayoutHeight = Height;
	return NumTiles; 
}


//Includes the current tile
INT GetDescendantCount(TILE_INFO* Tile)
{
	int i = 0;
	for (; Tile; i++)
	{
		Tile = Tile->ChildTile;
	}

	return i;
}


//Includes the current tile
INT GetAncestryCount(TILE_INFO* Tile)
{
	int i = 0;
	for (; Tile; i++)
	{
		Tile = Tile->ParentTile;
	}

	return i;
}

TILE_INFO* GetAncestry(TILE_INFO* CurrentTile)
{

	for (TILE_INFO* Tile = CurrentTile;; Tile = Tile->ParentTile)
		if (!Tile->ParentTile)
			return Tile;

}

TILE_INFO* GetBranchAncestry(TILE_INFO* CurrentTile)
{

	for (TILE_INFO* Tile = CurrentTile;; Tile = Tile->BranchParent)
		if (!Tile->BranchParent)
			return Tile;

}

INT GetBranchAncestryCount(TILE_INFO* Tile)
{
	int i = 0;
	for (; Tile; i++)
	{
		Tile = Tile->BranchParent;
	}

	return i;
}

TILE_INFO* GetDescendant(TILE_INFO* CurrentTile)
{

	for (TILE_INFO* Tile = CurrentTile;; Tile = Tile->ChildTile)
		if (!Tile->ChildTile)
			return Tile;

}



//------------------------------------------------------------------
// Split Functions
// Splits tiles vertically/horizontally in a workspace.
// The tiles must have parent tiles.
//------------------------------------------------------------------

VOID SplitTileHorizontallyNoBranch(WORKSPACE_INFO* Workspace, TILE_INFO *TileToInsert)
{
	assert(TileToInsert->ParentTile || TileToInsert->BranchParent);

	RECT SplitRect = TileToInsert->Placement.rcNormalPosition;
	RECT ParentRect = TileToInsert->ParentTile->Placement.rcNormalPosition;


	INT BranchWidth;
	INT NumTiles = GetLayoutAncestryHorizontal(TileToInsert, &BranchWidth);
	INT DeltaWindow =  BranchWidth / NumTiles;
	INT LastBottom;

	SplitRect.top		= (ParentRect.bottom - DeltaWindow);
	SplitRect.bottom	= ParentRect.bottom;
	SplitRect.right		= ParentRect.right;
	SplitRect.left		= ParentRect.left;
	LastBottom = SplitRect.top;

	TileToInsert->ParentTile->Placement.rcNormalPosition = ParentRect;
	TileToInsert->Placement.rcNormalPosition = SplitRect;

	TILE_INFO* ParentTile = TileToInsert->ParentTile;

	while (ParentTile)
	{
		ParentRect.bottom = LastBottom;
		ParentRect.top = LastBottom - DeltaWindow;
		LastBottom = ParentRect.top;
		ParentTile->Placement.rcNormalPosition = ParentRect;

		if (ParentTile->BranchParent)
		{

			TILE_INFO* BranchParent = ParentTile->BranchParent;
			RECT* BranchRect = &BranchParent->Placement.rcNormalPosition;

			BranchRect->top = LastBottom - DeltaWindow;
			BranchRect->bottom = LastBottom;
			LastBottom = BranchRect->top;
		}

		ParentTile = ParentTile->ParentTile;
	}

	//Modify Parent rect to move the right border to the left border of split
	// rect.

	TileToInsert->Placement.rcNormalPosition = SplitRect;

	
}

// TODO: Maybe Remove if the other one doesn't work this is just incaase
//backup
/*
VOID SplitTileHorizontally(WORKSPACE_INFO* Workspace, TILE_INFO *TileToInsert)
{
	assert(TileToInsert->ParentTile || TileToInsert->BranchParent);

	RECT SplitRect = TileToInsert->Placement.rcNormalPosition;
	RECT ParentRect = TileToInsert->ParentTile->Placement.rcNormalPosition;


	INT BranchWidth;
	INT LeftEncap = GetLayoutAncestryHorizontal(TileToInsert, &BranchWidth);
	INT RightEncap = GetDescendantCount(TileToInsert);
	// -1 because we count for TileToInsert twice;
	INT NumTiles = LeftEncap + RightEncap - 1;
	INT DeltaWindow =  BranchWidth / NumTiles;
	INT LastBottom;

	SplitRect.top		= (ParentRect.bottom - DeltaWindow);
	SplitRect.bottom	= ParentRect.bottom;
	SplitRect.right		= ParentRect.right;
	SplitRect.left		= ParentRect.left;
	LastBottom = SplitRect.top;

	TileToInsert->ParentTile->Placement.rcNormalPosition = ParentRect;
	TileToInsert->Placement.rcNormalPosition = SplitRect;

	TILE_INFO* ParentTile = TileToInsert->ParentTile;

	while (ParentTile)
	{
		ParentRect.bottom = LastBottom;
		ParentRect.top = LastBottom - DeltaWindow;
		LastBottom = ParentRect.top;
		ParentTile->Placement.rcNormalPosition = ParentRect;

		if (ParentTile->BranchParent)
		{

			TILE_INFO* BranchParent = ParentTile->BranchParent;
			RECT* BranchRect = &BranchParent->Placement.rcNormalPosition;

			BranchRect->top = LastBottom - DeltaWindow;
			BranchRect->bottom = LastBottom;
			LastBottom = BranchRect->top;
		}

		ParentTile = ParentTile->ParentTile;
	}

	//Modify Parent rect to move the right border to the left border of split
	// rect.

	TileToInsert->Placement.rcNormalPosition = SplitRect;

	
}
*/

INT GetColumnWidth(TILE_INFO* BranchBegin)
{
	INT Width = 0;
	for (TILE_INFO* Tile = BranchBegin; Tile; Tile = Tile->ChildTile)
	{
		RECT* TileRect = &Tile->Placement.rcNormalPosition;
		Width += (TileRect->right - TileRect->left);
	}

	return Width;

}

INT GetColumnHeight(TILE_INFO* BranchBegin)
{
	INT Height = 0;
	for (TILE_INFO* Tile = BranchBegin; Tile; Tile = Tile->ChildTile)
	{
		RECT* TileRect = &Tile->Placement.rcNormalPosition;
		Height += (TileRect->bottom - TileRect->top);
	}

	return Height;

}

VOID FitTileBranches(TILE_INFO* BranchBegin, TILE_INFO* BranchParent, INT DeltaWindow)
{

	TILE_INFO* Tile = BranchBegin;
	INT TotalEncaps = GetDescendantCount(BranchBegin);
	INT RecurseDeltaWindow = DeltaWindow / TotalEncaps;
	LAYOUT_STATE BranchLayout = BranchBegin->Layout;

	for (int i = 0; Tile && i < TotalEncaps; i++)
	{
		RECT* TileRect = &Tile->Placement.rcNormalPosition;

		if (BranchLayout == HORIZONTAL_LAYOUT)
		{
			TileRect->left += i * RecurseDeltaWindow;
			TileRect->right += (i + 1) * RecurseDeltaWindow;
		}
		else
		{
			TileRect->top = i * RecurseDeltaWindow;
			TileRect->bottom = (i + 1) * RecurseDeltaWindow;
		}


		if (Tile->BranchTile)
			FitTileBranches(Tile->BranchTile, Tile, RecurseDeltaWindow);

		Tile = Tile->ChildTile;
	}

}


/*
VOID SplitTileHorizontally(WORKSPACE_INFO* Workspace, TILE_INFO *TileToInsert)
{
	assert(TileToInsert->ParentTile || TileToInsert->BranchParent);

	//Child Rect takes the bottom side of the parent rect
	// and the top side of the child rect is half of the bottom side

	// GetAncestry gets number of parents, + 1 to add the tile itself
	// todo Implement GetLayoutAncestry because we need all the same parents
	// not parents with different layouts otherwise that needs recursive
	// splitting
	//TODO: JANK ALERT!!!!
	INT BranchWidth;
	TILE_INFO* Descendant = GetDescendant(TileToInsert);
	TILE_INFO* Ancestor = GetAncestry(TileToInsert);
	INT NumTiles = GetLayoutAncestryVertical(Descendant, &BranchWidth);
	INT DeltaWindow =  BranchWidth / NumTiles;
	INT LastRight;

	RECT* DescRect = &Descendant->Placement.rcNormalPosition;
	RECT* AncestorRect = &Ancestor->Placement.rcNormalPosition;

	DescRect->bottom = AncestorRect->top + BranchWidth;
	DescRect->top = DescRect->bottom - DeltaWindow;

	if (!Descendant->BranchTile)
	{
		DescRect->left		= AncestorRect->left;
		DescRect->right	= AncestorRect->right;
	}

	RECT* ChildRect = &Descendant->Placement.rcNormalPosition;

	for (TILE_INFO* Tile = Descendant->ParentTile; Tile; Tile = Tile->ParentTile)
	{
		RECT* Rect = &Tile->Placement.rcNormalPosition;

		Rect->bottom = ChildRect->top;
		Rect->top = ChildRect->top - DeltaWindow;
		Rect->left = ChildRect->left;

		if (Tile->ChildTile->BranchTile)
		{
			UpdateBranch(Tile->ChildTile->BranchTile);
			Rect->right = ChildRect->left + ChildRect->right + GetColumnWidth(Tile->ChildTile->BranchTile);
		}
		else
		{
			Rect->right = ChildRect->right;
		}

		ChildRect = Rect;
	}
	



}
*/



/*
VOID SplitTileHorizontally(WORKSPACE_INFO* Workspace, TILE_INFO *TileToInsert)
{
	assert(TileToInsert->ParentTile || TileToInsert->BranchParent);

	//Child Rect takes the right side of the parent rect
	// and the left side of the child rect is half of the right side

	// GetAncestry gets number of parents, + 1 to add the tile itself
	// todo Implement GetLayoutAncestry because we need all the same parents
	// not parents with different layouts otherwise that needs recursive
	// splitting
	//TODO: JANK ALERT!!!!
	INT BranchWidth;
	TILE_INFO* Descendant = GetDescendant(TileToInsert);
	TILE_INFO* Ancestor = GetAncestry(TileToInsert);
	INT NumTiles = GetLayoutAncestryHorizontal(Descendant, &BranchWidth);

	if (Ancestor->BranchParent)
		NumTiles++;

	INT DeltaWindow =  BranchWidth / NumTiles;
	INT LastRight;

	RECT* DescRect = &Descendant->Placement.rcNormalPosition;
	RECT* AncestorRect = &Ancestor->Placement.rcNormalPosition;

	DescRect->bottom = AncestorRect->top + BranchWidth;
	DescRect->top = DescRect->bottom - DeltaWindow;
	DescRect->left = AncestorRect->left;

	if (!Descendant->BranchTile)
		DescRect->right = AncestorRect->right;
	else
	{
		UpdateBranch(Descendant->BranchTile);
		DescRect->right = GetColumnWidth(Descendant->BranchTile);
	}

	RECT* Rect;
	RECT* ChildRect = &Descendant->Placement.rcNormalPosition;

	for (TILE_INFO* Tile = Descendant->ParentTile; Tile; Tile = Tile->ParentTile)
	{

		Rect = &Tile->Placement.rcNormalPosition;
		Rect->bottom = ChildRect->top;
		Rect->top = ChildRect->top - DeltaWindow;

		if (Tile->BranchTile)
			UpdateBranch(Tile->BranchTile);

		if (!Rect->right)
		{
			Rect->left = DescRect->left;
			Rect->right = DescRect->right;
		}

		ChildRect = Rect;
	}

	if (Ancestor->BranchParent)
	{
		RECT* AncestorRect = &Ancestor->BranchParent->Placement.rcNormalPosition;

		AncestorRect->bottom = ChildRect->top;
		AncestorRect->top = ChildRect->top - DeltaWindow;

		if (!AncestorRect->right)
		{
			AncestorRect->left = DescRect->left;
			AncestorRect->right = DescRect->right;
		}
	}

}
*/

/*
// TESTING STUFF FOR MOVING CURSOR
VOID SplitTileVertically(WORKSPACE_INFO* Workspace, TILE_INFO *TileToInsert)
{
	assert(TileToInsert->ParentTile || TileToInsert->BranchParent);

	//Child Rect takes the right side of the parent rect
	// and the left side of the child rect is half of the right side

	// GetAncestry gets number of parents, + 1 to add the tile itself
	// todo Implement GetLayoutAncestry because we need all the same parents
	// not parents with different layouts otherwise that needs recursive
	// splitting
	//TODO: JANK ALERT!!!!
	INT BranchWidth;
	TILE_INFO* Descendant = GetDescendant(TileToInsert);
	TILE_INFO* Ancestor = GetAncestry(TileToInsert);
	INT NumTiles = GetLayoutAncestryVertical(Descendant, &BranchWidth);
	INT DeltaWindow =  BranchWidth / NumTiles;
	INT LastRight;

	RECT* DescRect = &Descendant->Placement.rcNormalPosition;
	RECT* AncestorRect = &Ancestor->Placement.rcNormalPosition;

	DescRect->right = AncestorRect->left + BranchWidth;
	DescRect->left = DescRect->right - DeltaWindow;
	DescRect->top = AncestorRect->top;

	if (!Descendant->BranchTile)
		DescRect->bottom = AncestorRect->bottom;
	else
	{
		UpdateBranch(Descendant->BranchTile);
		DescRect->bottom = GetColumnHeight(Descendant->BranchTile);
	}

	RECT* ChildRect = &Descendant->Placement.rcNormalPosition;

	for (TILE_INFO* Tile = Descendant->ParentTile; Tile; Tile = Tile->ParentTile)
	{
		RECT* Rect = &Tile->Placement.rcNormalPosition;

		Rect->right = ChildRect->left;
		Rect->left = ChildRect->left - DeltaWindow;

		if (Tile->BranchTile)
			UpdateBranch(Tile->BranchTile);

		if (!Rect->right)
		{
			Rect->top = DescRect->top;
			Rect->bottom = DescRect->bottom;
		}

		ChildRect = Rect;
	}

}
*/

VOID RecurseTileVertically(WORKSPACE_INFO* Workspace, TILE_INFO* TileToRecurse)
{
	assert(TileToRecurse->ParentTile || TileToRecurse->BranchParent);

	RECT* SplitRect = &TileToRecurse->Placement.rcNormalPosition;
	//RECT *ParentRect = &TileToRecurse->BranchParent->Placement.rcNormalPosition;
	RECT* ParentRect = &Workspace->TileInFocus->Placement.rcNormalPosition;

	SplitRect->right = ParentRect->right;
	SplitRect->left = (ParentRect->left + ParentRect->right)/2;
	ParentRect->right = SplitRect->left;
	SplitRect->top = ParentRect->top;
	SplitRect->bottom = ParentRect->bottom;

}

VOID RecurseTileHorizontally(WORKSPACE_INFO* Workspace, TILE_INFO* TileToRecurse)
{
	assert(TileToRecurse->ParentTile || TileToRecurse->BranchParent);

	RECT* SplitRect = &TileToRecurse->Placement.rcNormalPosition;
	//RECT* ParentRect = &TileToRecurse->BranchParent->Placement.rcNormalPosition;
	RECT* ParentRect = &Workspace->TileInFocus->Placement.rcNormalPosition;

	SplitRect->bottom = ParentRect->bottom;
	SplitRect->top = (ParentRect->bottom + ParentRect->top)/ 2;
	ParentRect->bottom = SplitRect->top;
	SplitRect->left = ParentRect->left;
	SplitRect->right = ParentRect->right;

}

VOID ResortRecursive(TILE_INFO* BranchBegin, RECT* Box)
{

	TILE_INFO* BranchNode = BranchBegin->BranchParent;

	INT RightEncap = GetDescendantCount(BranchBegin);
	INT Width = Box->right - Box->left;
	INT Height = Box->bottom - Box->top;

	INT DeltaWidth = Width / RightEncap;
	INT DeltaHeight = Height / RightEncap;
	RECT Encap;

	if (BranchNode->NodeType == VERTICAL_SPLIT)
	{
		Encap.left = Box->left;
		Encap.right = Box->left + DeltaWidth;
		Encap.top = Box->top;
		Encap.bottom = Box->bottom;
	}
	else
	{
		Encap.top = Box->top;
		Encap.bottom = Box->top + DeltaHeight;
		Encap.left = Box->left;
		Encap.right = Box->right;
	}

	INT i = 0;
	for (TILE_INFO* Tile = BranchBegin; Tile; Tile = Tile->ChildTile)
	{
		RECT* Rect = &Tile->Placement.rcNormalPosition;

		if (BranchNode->NodeType ==  VERTICAL_SPLIT)
		{

			Rect->left = Encap.left + (i * DeltaWidth);
			Rect->right = Encap.right + (i * DeltaWidth);
			Rect->top = Encap.top;
			Rect->bottom = Encap.bottom;
		}
		else
		{
			Rect->top = Encap.top + (i * DeltaHeight);
			Rect->bottom = Encap.bottom + (i * DeltaHeight);
			Rect->left = Encap.left;
			Rect->right = Encap.right;
		}

		if (Tile->NodeType != TERMINAL)
			ResortRecursive(Tile->BranchTile, &Tile->Placement.rcNormalPosition);

		i++;
	}

}

VOID ResortTiles(WORKSPACE_INFO* Workspace)
{

	if (!Workspace->Tiles)
		return;

	TILE_INFO* Ancestor = Workspace->Tiles;
	INT RightEncap = GetDescendantCount(Ancestor);
	INT Width = ScreenWidth;
	INT Height = ScreenHeight;

	INT DeltaWidth = Width / RightEncap;
	INT DeltaHeight = Height / RightEncap;

	TILE_INFO* Tile = Ancestor;
	RECT Encap;
	
	if (Workspace->Layout == VERTICAL_SPLIT)
	{


		Encap.left = 0;
		Encap.right = DeltaWidth;
		Encap.top = 0;
		Encap.bottom = Height;

	}
	else
	{

		Encap.left = 0;
		Encap.right = Width;
		Encap.top = 0;
		Encap.bottom = DeltaHeight;
	}

	
	INT i = 0;
	for (Tile = Workspace->Tiles; Tile; Tile = Tile->ChildTile)
	{
		RECT* Rect = &Tile->Placement.rcNormalPosition;

		if (Workspace->Layout == VERTICAL_SPLIT)
		{

			Rect->left = Encap.left + (i * DeltaWidth);
			Rect->right = Encap.right + (i * DeltaWidth);
			Rect->top = Encap.top;
			Rect->bottom = Encap.bottom;
		}
		else
		{
			Rect->top = Encap.top + (i * DeltaHeight);
			Rect->bottom = Encap.bottom + (i * DeltaHeight);
			Rect->left = Encap.left;
			Rect->right = Encap.right;
		}

		if (Tile->NodeType != TERMINAL)
			ResortRecursive(Tile->BranchTile, &Tile->Placement.rcNormalPosition);


		i++;
	}


}

// The  Main Function to add a Tile to a workspace
// It does not render the workspace.
// call RenderWorkspace to render the workspace after adding a tile.
VOID AddTileToWorkspace(WORKSPACE_INFO* Workspace, TILE_INFO* TileToAdd)
{

	ResortTiles(Workspace);


	Workspace->TileInFocus = TileToAdd;
	IsPressed = FALSE;


}

/*
VOID FitTile(TILE_INFO* TileToRemove)
{

	LAYOUT_STATE CurrentLayout = TileToRemove->Layout;
	RECT RemoveRect = TileToRemove->Placement.rcNormalPosition;
	TILE_INFO* Tile = TileToRemove->ParentTile;
	BOOL IsBranch = (TileToRemove->BranchParent != 0);


	INT LeftEncap = 0;

	if (IsBranch)
	{
		Tile = TileToRemove->BranchParent;
	}
	else
		for (; Tile && Tile->Layout == CurrentLayout; LeftEncap++)
			Tile = Tile->ParentTile;

	INT RightEncap = 0;

	for (Tile = TileToRemove->ChildTile; Tile && Tile->Layout == CurrentLayout; RightEncap++)
		Tile = Tile->ChildTile;

	INT TotalEncap = LeftEncap + RightEncap;
	INT DeltaWindow;

	if (TileToRemove->Layout == HORIZONTAL_LAYOUT)
		DeltaWindow = (RemoveRect.bottom - RemoveRect.top) / TotalEncap;
	else
		DeltaWindow = (RemoveRect.right - RemoveRect.left) / TotalEncap;

	// move windows on the left to the right
	// and
	// move  windows on the right to the left 

	//Do left side first cuz why not

	Tile = IsBranch ? TileToRemove->BranchParent : TileToRemove->ParentTile;

	for (int i = 1; Tile && i <= LeftEncap; i++)
	{
		RECT* ParentRect = &Tile->Placement.rcNormalPosition;

		INT DeltaSide = (LeftEncap - i) * DeltaWindow;

		if (CurrentLayout == VERTICAL_LAYOUT)
		{
			ParentRect->right += DeltaSide + DeltaWindow;
			ParentRect->left += DeltaSide;
		}
		else
		{
			ParentRect->bottom += DeltaSide + DeltaWindow;
			ParentRect->top += DeltaSide;
		}

		if (Tile->BranchTile)
			UpdateBranch(Tile->BranchTile);
			//FitTileBranches(Tile->BranchTile, Tile, DeltaWindow);

		Tile = Tile->ParentTile;

	}

	Tile = TileToRemove->ChildTile;
	
	for (int i = 1; Tile && i <= RightEncap ; i++)
	{
		RECT* ParentRect = &Tile->Placement.rcNormalPosition;

		INT DeltaSide = (RightEncap - i) * DeltaWindow;

		if (CurrentLayout == VERTICAL_LAYOUT)
		{
			ParentRect->left -= DeltaSide + DeltaWindow;
			ParentRect->right -= DeltaSide;
		}
		else
		{
			ParentRect->top -= DeltaSide + DeltaWindow;
			ParentRect->bottom -= DeltaSide;
		}

		if (Tile->BranchTile)
			UpdateBranch(Tile->BranchTile);
			//FitTileBranches(Tile->BranchTile, Tile, DeltaWindow);

		Tile = Tile->ChildTile;
	}

}
*/

VOID UpgradeNode(WORKSPACE_INFO* Workspace, TILE_INFO* Node)
{

	assert(Node->BranchParent);

	if (Node->BranchParent->ParentTile)
	{
		Node->BranchParent->ParentTile->ChildTile = Node;
		Node->ParentTile = Node->BranchParent->ParentTile;
	}
	else
		Node->ParentTile = NULL;

	if (Node->BranchParent->ChildTile)
	{
		Node->BranchParent->ChildTile->ParentTile = Node;
		Node->ChildTile = Node->BranchParent->ChildTile;
	}
	else
		Node->ChildTile = NULL;

	TILE_INFO* BranchParent = Node->BranchParent;

	if (Node->BranchParent->BranchParent)
	{
		Node->BranchParent = Node->BranchParent->BranchParent;
		Node->BranchParent->BranchTile = GetAncestry(Node);
	}
	else if (Workspace->Tiles == Node->BranchParent)
	{
		Workspace->Tiles = Node;
		Node->BranchParent = NULL;
	}
	else
		Node->BranchParent = NULL;


	RtlZeroMemory(BranchParent, sizeof(TILE_INFO));
	free(BranchParent);

}

TILE_INFO* UnlinkNode(WORKSPACE_INFO* Workspace, TILE_INFO* Node)
{

	assert(Node->NodeType == TERMINAL);

	TILE_INFO* FocusTile = NULL;

	if (Node->BranchParent)
	{

		if (Node->BranchParent->BranchTile == Node)
		{
			assert(Node->ChildTile);

			//Triangle has 3 branches we're removing the first one to relink
			// the triangle to the 2nd branch 
			if (Node->ChildTile->ChildTile)
			{
				Node->BranchParent->BranchTile = Node->ChildTile;
				Node->ChildTile->ParentTile = NULL;
				FocusTile = Node->ChildTile;
			}
			else 
			{
				// Upgrade since this triangle only has 2 branches and we're removing one
				UpgradeNode(Workspace, Node->ChildTile);
				FocusTile = Node->ChildTile;
			}
		}
		else //We're not at the very left of a node
		{
			assert(Node->ParentTile);

			//for a triangle to be upgraded it must remove 1 branch
			// of a 2 branch triangle split
			// and if the branchparent->branch tile isn't this node
			// it means it's the one on the right, so it has to have
			// no children and it it has to have Ancestry of 2
			// Ancestry includes itself
			if (GetAncestryCount(Node) == 2 && !Node->ChildTile)
			{
				// Upgrade since this triangle only has 2 branches and we're removing one
				UpgradeNode(Workspace, Node->ParentTile);
				FocusTile = Node->ParentTile;
			}
			else
			{
				if (Node->ChildTile)
				{
					Node->ParentTile->ChildTile = Node->ChildTile;
					Node->ChildTile->ParentTile = Node->ParentTile;
					FocusTile = Node->ChildTile;
				}
				else
				{
					Node->ParentTile->ChildTile = NULL;
					FocusTile = Node->ParentTile;
				}

			}

		}

	}
	else //we're at the top level
	{
		
		//Top Level and last node
		if (Workspace->Tiles == Node && !Node->ChildTile)
		{
			Workspace->Tiles = NULL;
			FocusTile = NULL;
			return NULL;
		}
		else if (Node == Workspace->Tiles)
		{
			Workspace->Tiles = Node->ChildTile;
			FocusTile = Node->ChildTile;
			Node->ChildTile->ParentTile = NULL;
		}
		else if (Node->ParentTile && Node->ChildTile)
		{
			Node->ParentTile->ChildTile = Node->ChildTile;
			Node->ChildTile->ParentTile = Node->ParentTile;
			FocusTile = Node->ChildTile;

		}
		else if (Node->ParentTile)
		{
			Node->ParentTile->ChildTile = NULL;
			FocusTile = Node->ParentTile;
		}


		//Upgrade Node since ROOT brnach is a split which means root has to change
		if (Workspace->Tiles == Node && !Node->ChildTile && Node->NodeType != TERMINAL)
		{
			Workspace->Tiles = Workspace->Tiles->BranchTile;
			Workspace->Tiles->BranchParent = NULL;
			FocusTile = Workspace->Tiles;

			for (TILE_INFO* Tile = Workspace->Tiles; Tile; Tile = Tile->ChildTile)
				Tile->BranchParent = NULL;


		}
		

	}



	return FocusTile;

}

BOOL IsWorkspaceRootTile(WORKSPACE_INFO* Workspace)
{

	return !(Workspace->Tiles->ChildTile || Workspace->Tiles->BranchTile);
	
}

VOID RemoveTileFromWorkspace(WORKSPACE_INFO* Workspace, TILE_INFO* TileToRemove)
{

	//WTF IS THIS,  REFACTOR WHEN BASIC CREATION & DESTRUCTION ARE FIXED

	TileToRemove->WindowHandle = NULL;

	Workspace->TileInFocus = UnlinkNode(Workspace, TileToRemove);


	if (Workspace->TileInFocus && Workspace->TileInFocus->NodeType != TERMINAL)
		Workspace->TileInFocus = Workspace->TileInFocus->BranchTile;


	ResortTiles(Workspace);
	//FitTile(TileToRemove);

	free(TileToRemove);

}

VOID LinkNode(WORKSPACE_INFO* Workspace, TILE_INFO* TileToAdd)
{

	if (!Workspace->Tiles)
	{
		TileToAdd->NodeType = TERMINAL;
		Workspace->Tiles = TileToAdd;
		Workspace->Layout = UserChosenLayout;
	}
	else if (!IsPressed || IsWorkspaceRootTile(Workspace))
	{
		
		TileToAdd->NodeType = TERMINAL;

		if (Workspace->TileInFocus->ChildTile)
		{
			TileToAdd->ChildTile = Workspace->TileInFocus->ChildTile;
			TileToAdd->ChildTile->ParentTile = TileToAdd;
		}

		Workspace->TileInFocus->ChildTile = TileToAdd;
		TileToAdd->ParentTile = Workspace->TileInFocus;

		if (TileToAdd->ParentTile->BranchParent)
			TileToAdd->BranchParent = TileToAdd->ParentTile->BranchParent;

	}
	else
	{

		TILE_INFO* NodeTile = (TILE_INFO*)malloc(sizeof(TILE_INFO));
		RtlZeroMemory(NodeTile, sizeof(TILE_INFO));

		if (Workspace->TileInFocus->ChildTile)
		{
			NodeTile->ChildTile = Workspace->TileInFocus->ChildTile;
			NodeTile->ChildTile->ParentTile = NodeTile;
		}

		if (Workspace->TileInFocus->ParentTile)
		{
			Workspace->TileInFocus->ParentTile->ChildTile = NodeTile;
			NodeTile->ParentTile = Workspace->TileInFocus->ParentTile;
			NodeTile->BranchParent = NodeTile->ParentTile->BranchParent;
		}
		else
		{
			if (Workspace->TileInFocus == Workspace->Tiles)
				Workspace->Tiles = NodeTile;
			else
			{
				Workspace->TileInFocus->BranchParent->BranchTile = NodeTile;
				NodeTile->BranchParent = Workspace->TileInFocus->BranchParent;
			}


		}


		NodeTile->NodeType = UserChosenLayout;

		NodeTile->BranchTile = Workspace->TileInFocus;


		Workspace->TileInFocus->BranchParent = NodeTile;
		Workspace->TileInFocus->ChildTile = TileToAdd;
		Workspace->TileInFocus->ParentTile = NULL;
		TileToAdd->ParentTile = Workspace->TileInFocus;
		TileToAdd->BranchParent = NodeTile;

	}

}

// Seperate the all the windows we got into tiles. there are 10 workspaces.
// and upon init regrouping there are 4 windows per workspaces. If there
// is more than 40 total windows that have to be grouped. recalcualte
// tiles per workspace.
//
int prog_loop = 0;
VOID PerformInitialRegroupring()
{
	
	INT TilesPerWorkspace;
	INT NumTiles;
	INT WorkspaceIndex;

	NumTiles = WindowOrderList.size();

	if (NumTiles > MAX_INIT_TILES)
		TilesPerWorkspace = (NumTiles / 10) + 1;
	else if (NumTiles > INIT_TILES_PER_WORKSPACE)
		TilesPerWorkspace = INIT_TILES_PER_WORKSPACE;
	else
		TilesPerWorkspace = NumTiles;

	WorkspaceIndex = CurrentWorkspaceInFocus;

	INT TilesLeft = NumTiles;

	for (int i = 0; i < NumTiles; i += TilesPerWorkspace)
	{

		// this is dumb as fuck don't use an array linked-list it because
		// we're not morons 
		INT ActualTilesLeft = NumTiles - (i * TilesPerWorkspace);


		int j = 0;
		for (; j < TilesPerWorkspace && TilesLeft; j++)
		{
			//printf("in for loop : %u\n", prog_loop++);

			TILE_INFO* CurrentTileInfo = &WindowOrderList[i + j];
			TILE_INFO* TileToAdd = (TILE_INFO*)malloc(sizeof(TILE_INFO));
			RtlZeroMemory(TileToAdd, sizeof(TILE_INFO));

			*TileToAdd = *CurrentTileInfo;

			TileToAdd->BranchTile = NULL;

			LinkNode(&WorkspaceList[WorkspaceIndex], TileToAdd);

			AddTileToWorkspace(&WorkspaceList[WorkspaceIndex], TileToAdd);

			TilesLeft--;
		}

		WorkspaceIndex++;

	}
	
}

INT ForceToForeground(HWND WindowHandle)
{
	DWORD FgThreadId = GetWindowThreadProcessId(GetForegroundWindow(), LPDWORD(0));
	DWORD currentThreadId = GetCurrentThreadId();
	DWORD CONST_SW_SHOW = 5;
	AttachThreadInput(FgThreadId, currentThreadId, true);
	DWORD RetVal = BringWindowToTop(WindowHandle);
	//ShowWindow(WindowHandle, CONST_SW_SHOW);
	AttachThreadInput(FgThreadId, currentThreadId, false);
	return RetVal;
}

VOID HandleWeirdWindowState(HWND WindowHandle)
{
	WCHAR CurrentWindowText[1024] = { 0 };
	DWORD ShowMask = SW_RESTORE;
	GetClassNameW(WindowHandle, CurrentWindowText, 1024);

	INT ListLength = ARRAY_SIZEOF(WeirdWindowsList);
	for (int i = 0; i < ListLength; i++)
	{
		if (!wcscmp(CurrentWindowText, WeirdWindowsList[i].ClassName))
		{
			ShowMask = WeirdWindowsList[i].ShowMask;
		}
	}

	ShowWindow(WindowHandle, ShowMask);

}

VOID RenderFocusWindow(TILE_INFO* Tile)
{

	HWND WindowHandle = Tile->WindowHandle;

	ForceToForeground(WindowHandle);

	logc(6, "RenderFocusWindow : %X\n", WindowHandle);

	RECT ClientRect;
	PRECT TileRect = &Tile->Placement.rcNormalPosition;

	GetClientRect(WindowHandle, &ClientRect);

	logc(6, "%lu %lu %lu %lu\n", ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom);

	INT Width = ClientRect.right - ClientRect.left;
	INT BorderSize = 5;

	POINT TopLeftClientArea = { 0 };

	if (!ClientToScreen(WindowHandle, &TopLeftClientArea))
		FailWithCode("ClientToScreen");

	logc(6, "TL - X : %lu, TL - Y : %lu\n", TopLeftClientArea.x, TopLeftClientArea.y);

	ClientRect.left += TopLeftClientArea.x;
	ClientRect.top += TopLeftClientArea.y;
	ClientRect.bottom += TopLeftClientArea.y;

	if (!SetWindowPos(FocusWindow, HWND_TOPMOST, ClientRect.left, ClientRect.bottom - BorderSize, Width, BorderSize, 0))
		FailWithCode("SetWindowPos Focus Window");

	if (!SetWindowPos(FocusWindow, HWND_NOTOPMOST, ClientRect.left, ClientRect.bottom - BorderSize, Width, BorderSize, 0))
		FailWithCode("SetWindowPos Focus Window");
	
}

INT RenderWindows(TILE_INFO* Tile)
{

	INT Count = 0;

	for (; Tile; Tile = Tile->ChildTile)
	{

		if (Tile->NodeType != TERMINAL)
		{
			Count += RenderWindows(Tile->BranchTile);
			continue;
		}

		RECT PrintRect = Tile->Placement.rcNormalPosition;
		INT Width = (PrintRect.right - PrintRect.left);
		INT Height = (PrintRect.bottom - PrintRect.top);
		WCHAR WindowText[1024] = { 0 };

		logc(5, "CurHandle : %X\n", Tile->WindowHandle);
		GetWindowTextW(Tile->WindowHandle, WindowText, 512);

		memset(WindowText, 0, sizeof(WindowText));

		logc(5, "WindowText: %ls\n", WindowText);
		GetClassName(Tile->WindowHandle, WindowText, 512);
		logc(5, "WindowClassName: %ls\n", WindowText);
		logc(5, "ToPrint: %u %u %u %u\n", PrintRect.left, PrintRect.top, PrintRect.right, PrintRect.bottom);

		DWORD RetVal = 1;
		HandleWeirdWindowState(Tile->WindowHandle);
		
		RetVal = SetWindowPos(Tile->WindowHandle, HWND_TOP,
			PrintRect.left, PrintRect.top, Width, Height, 0);

		if (!RetVal)
			FailWithCode(MakeFormatString("SetWindowPos : %X\n", Tile->WindowHandle));

		Count++;
	}

	return Count;

}

VOID RenderDebugWindow(TILE_INFO* FocusTile)
{

	return;

	if (!FocusTile)
		return;


	RECT* FocusRect = &FocusTile->Placement.rcNormalPosition;

	INT Width = FocusRect->right - FocusRect->left;
	INT Height = FocusRect->bottom - FocusRect->top;

	if (!SetWindowPos(DebugWindow, HWND_TOPMOST, FocusRect->left, FocusRect->top, Width, Height, 0))
		FailWithCode("Couldn't move DebugWindow??");

	ShowWindow(DebugWindow, SW_SHOW);

}

VOID RenderFullscreenWindow(WORKSPACE_INFO* Workspace)
{
	assert(Workspace->IsFullScreen && Workspace->TileInFocus);

	ForceToForeground(Workspace->TileInFocus->WindowHandle);

	HandleWeirdWindowState(Workspace->TileInFocus->WindowHandle);

	DWORD RetVal = SetWindowPos(Workspace->TileInFocus->WindowHandle, HWND_TOP, 0, 0, ScreenWidth, ScreenHeight, 0);

	if (!RetVal)
		FailWithCode(MakeFormatString("SetWindowPos : %u\n", Workspace->TileInFocus->WindowHandle));

}

VOID RenderStatusBar()
{

	INT SlotCount = 1;
	CHAR ButtonText[2] = { '0', '\0' };

	BtnToColor = 0; 

	for (int i = 1; i < 10; i++)
	{
		WORKSPACE_INFO* Workspace = &WorkspaceList[i];

		if (i == CurrentWorkspaceInFocus)
		{
			BtnToColor = StatusBarWindow[SlotCount];
			ButtonText[0] = '0' + i;
			SetWindowTextA(StatusButtonList[SlotCount], ButtonText);
			InvalidateRect(StatusButtonList[SlotCount], 0, TRUE);
			SlotCount++;
		}
		else if (Workspace->Tiles)
		{
			ButtonText[0] = '0' + i;
			SetWindowTextA(StatusButtonList[SlotCount], ButtonText);
			InvalidateRect(StatusButtonList[SlotCount], 0, TRUE);
			SlotCount++;
		}
	}

	for (int i = 1; i < 10; i++)
	{
		if (i < SlotCount)
		{
			ShowWindow(StatusBarWindow[i], SW_SHOW);
			logc(12, "Showing Button : %u : %X\n", i, StatusBarWindow[i]);
		}
		else
		{
			ShowWindow(StatusBarWindow[i], SW_HIDE);
			logc(12, "Hiding Button : %u : %X\n", i, StatusBarWindow[i]);
		}
	}
}

VOID RenderWorkspace(INT WorkspaceNumber)
{

	//the window about to be rendered is the window in focus;
	CurrentWorkspaceInFocus = WorkspaceNumber;

	WORKSPACE_INFO* Workspace = &WorkspaceList[WorkspaceNumber];

	logc(13, "Rendering Workspace ; %u\n", WorkspaceNumber);

	INT Count = 1;

	if (Workspace->IsFullScreen)
		RenderFullscreenWindow(Workspace);
	else
		Count = RenderWindows(Workspace->Tiles);

	if (Workspace->TileInFocus && !Workspace->IsFullScreen)
	{
		RenderDebugWindow(Workspace->TileInFocus);
		RenderFocusWindow(Workspace->TileInFocus);
	}
	else
	{
		ShowWindow(DebugWindow, SW_HIDE);
		SetWindowPos(FocusWindow, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
	}

	RenderStatusBar();

	logc(12, "Total Length Of Workspace : %u\n", Count);

}

VOID InitWorkspaceList()
{
	WorkspaceList.resize(10);
}

VOID LoadNecessaryModules()
{
	ForceResize64 = LoadLibraryA("ForceResize64.dll");
	
	if (!ForceResize64)
		FailWithCode("Couldn't load ForceResize64.dll");

}

UINT_PTR KeyboardLookupTable[0x100] = { 0 };

NODE_TYPE GetBranchLayout(WORKSPACE_INFO* Workspace, TILE_INFO* Tile)
{
	if (Tile->BranchParent)
		return Tile->BranchParent->NodeType;
	else
		return Workspace->Layout;
}


VOID FocusLeft(WORKSPACE_INFO* Workspace)
{

	TILE_INFO* TileInFocus = Workspace->TileInFocus;

	assert(TileInFocus->NodeType == TERMINAL);

	for (TILE_INFO* Tile = Workspace->TileInFocus; Tile; Tile = Tile->BranchParent)
	{
		if (GetBranchLayout(Workspace, Tile) == VERTICAL_SPLIT)
		{
			if (Tile->ParentTile)
			{
				Workspace->TileInFocus = Tile->ParentTile;
				break;
			}
		}
		
	}

	while (Workspace->TileInFocus->NodeType != TERMINAL)
		Workspace->TileInFocus = Workspace->TileInFocus->BranchTile;


}

VOID FocusRight(WORKSPACE_INFO* Workspace)
{

	TILE_INFO* TileInFocus = Workspace->TileInFocus;

	assert(TileInFocus->NodeType == TERMINAL);

	for (TILE_INFO* Tile = Workspace->TileInFocus; Tile; Tile = Tile->BranchParent)
	{
		if (GetBranchLayout(Workspace, Tile) == VERTICAL_SPLIT)
		{
			if (Tile->ChildTile)
			{
				Workspace->TileInFocus = Tile->ChildTile;
				break;
			}
		}
		
	}

	while (Workspace->TileInFocus->NodeType != TERMINAL)
		Workspace->TileInFocus = Workspace->TileInFocus->BranchTile;


}


VOID FocusTop(WORKSPACE_INFO* Workspace)
{

	TILE_INFO* TileInFocus = Workspace->TileInFocus;

	assert(TileInFocus->NodeType == TERMINAL);

	for (TILE_INFO* Tile = Workspace->TileInFocus; Tile; Tile = Tile->BranchParent)
	{
		if (GetBranchLayout(Workspace, Tile) == HORIZONTAL_SPLIT)
		{
			if (Tile->ParentTile)
			{
				Workspace->TileInFocus = Tile->ParentTile;
				break;
			}
		}
		
	}

	while (Workspace->TileInFocus->NodeType != TERMINAL)
		Workspace->TileInFocus = Workspace->TileInFocus->BranchTile;


}


VOID FocusBottom(WORKSPACE_INFO* Workspace)
{

	TILE_INFO* TileInFocus = Workspace->TileInFocus;

	assert(TileInFocus->NodeType == TERMINAL);

	for (TILE_INFO* Tile = Workspace->TileInFocus; Tile; Tile = Tile->BranchParent)
	{
		if (GetBranchLayout(Workspace, Tile) == HORIZONTAL_SPLIT)
		{
			if (Tile->ChildTile)
			{
				Workspace->TileInFocus = Tile->ChildTile;
				break;
			}
		}
		
	}

	while (Workspace->TileInFocus->NodeType != TERMINAL)
		Workspace->TileInFocus = Workspace->TileInFocus->BranchTile;


}

VOID FocusRoot(WORKSPACE_INFO* Workspace)
{

	if (Workspace->TileInFocus->BranchParent)
		UserChosenLayout = Workspace->TileInFocus->BranchParent->NodeType;
	else
		UserChosenLayout = Workspace->Layout;
}

BOOL ButtonDown(UINT_PTR Button)
{
	return (Button == WM_KEYDOWN || Button == WM_SYSKEYDOWN);
}

VOID SetWindowFocus(TILE_INFO* PrevTile, TILE_INFO* FocusTile)
{

	return;

	logc(6, "SetWindowFocus\nPrevTile->WindowHandle : %X\nFocusTile->WindowHandle : %X\n", PrevTile->WindowHandle, FocusTile->WindowHandle);

	if (PrevTile->WindowHandle != FocusTile->WindowHandle)
	{
		//DWORD PrevThreadId = GetWindowThreadProcessId(PrevTile->WindowHandle, NULL);
		DWORD FgThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		DWORD CurThreadId = GetWindowThreadProcessId(FocusTile->WindowHandle, NULL);

		if (!AttachThreadInput(FgThreadId,CurThreadId,  TRUE))
			FailWithCode("AttachThreadInput");

		DWORD FgTimeout;

		if (!SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &FgTimeout, 0))
			FailWithCode("SystemParamterInfoW GetForegroundLock");

		if (!SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, SPIF_SENDCHANGE))
			FailWithCode("SystemParamterInfoW GetForegroundLock");

		//DWORD WindowProcessId;
		//GetWindowThreadProcessId(FocusTile->WindowHandle, &WindowProcessId);

		AllowSetForegroundWindow(ASFW_ANY);

		//if (!BringWindowToTop(FocusTile->WindowHandle))
		//	FailWithCode("BringWindowToTop - Focus");

		if (!SetWindowPos(FocusTile->WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
			FailWithCode("SetWindowPos - Focus");

		if (!SetForegroundWindow(FocusTile->WindowHandle))
			FailWithCode("SetForegroundWindow - Focus");

		if (!SetFocus(FocusTile->WindowHandle))
			FailWithCode("SetForegroundWindow - Focus");

		if (!AttachThreadInput(FgThreadId, CurThreadId, FALSE))
			FailWithCode("AttachThreadInput");

	}

	logc(6, "ForegroundWindow : %X\n", GetForegroundWindow());

}


VOID HandleLeft(WORKSPACE_INFO* Workspace)
{

	if (!Workspace->Tiles)
		return;

	TILE_INFO* PrevTile = Workspace->TileInFocus;

	if (ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
	{
		HWND TargetWindow = Workspace->TileInFocus->WindowHandle;
		FocusLeft(Workspace);

		if (Workspace->TileInFocus != PrevTile)
		{
			PrevTile->WindowHandle = Workspace->TileInFocus->WindowHandle;
			Workspace->TileInFocus->WindowHandle = TargetWindow;
			RenderWorkspace(CurrentWorkspaceInFocus);
		}
	}
	else
		FocusLeft(Workspace);

	FocusRoot(Workspace);
	RenderFocusWindow(Workspace->TileInFocus);
	RenderDebugWindow(Workspace->TileInFocus);

}


VOID HandleRight(WORKSPACE_INFO* Workspace)
{

	if (!Workspace->Tiles)
		return;

	TILE_INFO* PrevTile = Workspace->TileInFocus;

	if (ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
	{
		HWND TargetWindow = Workspace->TileInFocus->WindowHandle;
		FocusRight(Workspace);

		if (Workspace->TileInFocus != PrevTile)
		{
			PrevTile->WindowHandle = Workspace->TileInFocus->WindowHandle;
			Workspace->TileInFocus->WindowHandle = TargetWindow;
			RenderWorkspace(CurrentWorkspaceInFocus);
		}
	}
	else
		FocusRight(Workspace);

	FocusRoot(Workspace);
	RenderFocusWindow(Workspace->TileInFocus);
	RenderDebugWindow(Workspace->TileInFocus);

}

VOID HandleTop(WORKSPACE_INFO* Workspace)
{

	if (!Workspace->Tiles)
		return;

	TILE_INFO* PrevTile = Workspace->TileInFocus;

	if (ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
	{
		HWND TargetWindow = Workspace->TileInFocus->WindowHandle;
		FocusTop(Workspace);

		if (Workspace->TileInFocus != PrevTile)
		{
			PrevTile->WindowHandle = Workspace->TileInFocus->WindowHandle;
			Workspace->TileInFocus->WindowHandle = TargetWindow;
			RenderWorkspace(CurrentWorkspaceInFocus);
		}
	}
	else
		FocusTop(Workspace);

	FocusRoot(Workspace);
	RenderFocusWindow(Workspace->TileInFocus);
	RenderDebugWindow(Workspace->TileInFocus);

}


VOID HandleBottom(WORKSPACE_INFO* Workspace)
{

	if (!Workspace->Tiles)
		return;

	TILE_INFO* PrevTile = Workspace->TileInFocus;

	if (ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
	{
		HWND TargetWindow = Workspace->TileInFocus->WindowHandle;
		FocusBottom(Workspace);

		if (Workspace->TileInFocus != PrevTile)
		{
			PrevTile->WindowHandle = Workspace->TileInFocus->WindowHandle;
			Workspace->TileInFocus->WindowHandle = TargetWindow;
			RenderWorkspace(CurrentWorkspaceInFocus);
		}
	}
	else
		FocusBottom(Workspace);

	//This Shit Works because the graph is left/down-adjusted to say so??
	//when you destroy a node the next one goes down and so the swap
	// happens regardless

	FocusRoot(Workspace);
	RenderFocusWindow(Workspace->TileInFocus);
	RenderDebugWindow(Workspace->TileInFocus);

}

INT HideWindows(TILE_INFO* Tile)
{

	INT Count = 0;

	for (; Tile; Tile = Tile->ChildTile)
	{

		//Recursively Hide Splits
		if (Tile->NodeType != TERMINAL)
		{
			Count += HideWindows(Tile->BranchTile);
			continue;
		}

		ShowWindow(Tile->WindowHandle, SW_MINIMIZE);
		
	}

	return Count;

}

VOID HideWorkspace(INT WorkspaceNumber)
{
	TILE_INFO* Tiles = WorkspaceList[WorkspaceNumber].Tiles;

	HideWindows(Tiles);

}

VOID HandleChangeWorkspace(INT WorkspaceNumber)
{

	HideWorkspace(CurrentWorkspaceInFocus);
	RenderWorkspace(WorkspaceNumber);
	
}

VOID HandleMoveWindowWorkspace(INT WorkspaceNumber)
{

	WORKSPACE_INFO* Workspace = &WorkspaceList[CurrentWorkspaceInFocus];
	WORKSPACE_INFO* TargetWorkspace = &WorkspaceList[WorkspaceNumber];
	TILE_INFO* Node = Workspace->TileInFocus;
	TILE_INFO* NewNode;

	NewNode = (TILE_INFO*)RtlAlloc(sizeof(TILE_INFO));
	RtlZeroMemory(NewNode, sizeof(TILE_INFO));

	//Copy Over Important Stuff of the 
	NewNode->WindowHandle = Node->WindowHandle;
	NewNode->PreWMInfo = Node->PreWMInfo;

	if (!Node)
		return;

	RemoveTileFromWorkspace(Workspace, Node);
	LinkNode(TargetWorkspace, NewNode);
	AddTileToWorkspace(TargetWorkspace, NewNode);
	RenderWorkspace(CurrentWorkspaceInFocus);


}

INT HandleKeyStates(PKBDLLHOOKSTRUCT KeyboardData, UINT_PTR KeyState)
{

	BOOL AltDown = (KeyboardLookupTable[KeyboardData->vkCode] == WM_SYSKEYDOWN);
	INT SkipKey = TRUE;
	INT WorkspaceNumber = -1;
	WORKSPACE_INFO* Workspace = &WorkspaceList[CurrentWorkspaceInFocus];

	
	if (!AltDown)
	{
		SkipKey = FALSE;
		return SkipKey;
	}

	switch (KeyboardData->vkCode)
	{
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		WorkspaceNumber = KeyboardData->vkCode - '0';

		logc(5, "Changing  to workspace : %u\n", WorkspaceNumber);

		if (WorkspaceNumber == CurrentWorkspaceInFocus)
			break;

		if (ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
			HandleMoveWindowWorkspace(WorkspaceNumber);
		else
			HandleChangeWorkspace(WorkspaceNumber);

		break;
	case 'P':
		for (int i = 0; i < 10; i++)
		{
			logc(11, "Showing Button : %X\n", StatusBarWindow[i]);
			ShowWindow(StatusBarWindow[i], SW_SHOW);
		}
		break;
	case 'T':
		TerminateProcess(GetCurrentProcess(), 420);
		break;
	case 'Y':
		for (int i = 0; i < 10; i++)
		{
			logc(11, "Hiding Button : %X\n", StatusBarWindow[i]);
			ShowWindow(StatusBarWindow[i], SW_HIDE);
		}
		break;
	case 'F':
		if (Workspace->TileInFocus)
			Workspace->IsFullScreen = !Workspace->IsFullScreen;
		else
			Workspace->IsFullScreen = FALSE;
		RenderWorkspace(CurrentWorkspaceInFocus);
		break;
	case 'H':
		IsPressed = TRUE;
		UserChosenLayout = VERTICAL_SPLIT;
		if (IsWorkspaceRootTile(Workspace))
			Workspace->Layout = UserChosenLayout;
		break;
	case 'V':
		IsPressed = TRUE;
		UserChosenLayout = HORIZONTAL_SPLIT;
		if (IsWorkspaceRootTile(Workspace))
			Workspace->Layout = UserChosenLayout;
		break;
	case VK_LEFT:

		if (Workspace->IsFullScreen)
			break;

		HandleLeft(Workspace);
		break;
	case VK_UP:

		if (Workspace->IsFullScreen)
			break;

		HandleTop(Workspace);
		break;
	case VK_DOWN:

		if (Workspace->IsFullScreen)
			break;

		HandleBottom(Workspace);
		break;
	case VK_RIGHT:

		if (Workspace->IsFullScreen)
			break;

		HandleRight(Workspace);
		RenderDebugWindow(Workspace->TileInFocus);
		break;
	case VK_RETURN:
		system("start chrome");
		break;
	case 'Q':
		if (!ButtonDown(KeyboardLookupTable[VK_LSHIFT]))
			SkipKey = FALSE;
		else
		{
			//WORKSPACE_INFO* Workspace = &WorkspaceList[CurrentWorkspaceInFocus];
			SendMessage(Workspace->TileInFocus->WindowHandle, WM_CLOSE, 0, 0);
			//DestroyWindow(Workspace->TileInFocus->WindowHandle);
			//RemoveTileFromWorkspace(Workspace, Workspace->TileInFocus);
			//RenderWorkspace(CurrentWorkspaceInFocus);
		}
		break;
	default:
		SkipKey = FALSE;
		break;
	}

	return SkipKey;

}

LRESULT WINAPI KeyboardCallback(int nCode, WPARAM WParam, LPARAM LParam)
{

	if (nCode < 0)
		return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);

	if (nCode != HC_ACTION)
		return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);

	PKBDLLHOOKSTRUCT KeyboardData	= (PKBDLLHOOKSTRUCT)LParam;
	UINT_PTR KeyState				= WParam;

	KeyboardLookupTable[KeyboardData->vkCode] = KeyState;

	if (HandleKeyStates(KeyboardData, KeyState))
	{
		WCHAR WindowText[1024] = { 0 };
		HWND WindowHandle;

		if (WorkspaceList[CurrentWorkspaceInFocus].TileInFocus)
			WindowHandle = WorkspaceList[CurrentWorkspaceInFocus].TileInFocus->WindowHandle;
		else
			WindowHandle = NULL;

		GetWindowTextW(WindowHandle, WindowText, 1024);
		logc(9, "Focus Text: %ls\n", WindowText);
		GetClassNameW(WindowHandle, WindowText, 1024);
		logc(9, "Focus Class: %ls\n", WindowText);
		logc(9, "Window In Focus : %X\n",WindowHandle);
		return DO_NOT_PASS_KEY;
	}


	//logc(11, "Layout : %u\n", UserChosenLayout);

	return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);
	
}

VOID InstallKeyboardHooks()
{
	KeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardCallback, NULL, 0);

	if (!KeyboardHook)
		FailWithCode("Couldn't set keyboard hook");
}

LONG WINAPI OnCrash(PEXCEPTION_POINTERS* ExceptionInfo)
{
	if (x86ProcessHandle)
		TerminateProcess(x86ProcessHandle, 369);

	return EXCEPTION_CONTINUE_SEARCH;

}

VOID SetCrashRoutine()
{
	AddVectoredExceptionHandler(FALSE, (PVECTORED_EXCEPTION_HANDLER)OnCrash);
}


extern "C" __declspec(dllexport) VOID OnNewWindow(HWND WindowHandle)
{

	if (IsDefaultWindow(WindowHandle) || !IsWindow(WindowHandle))
		return;

	InstallWindowSizeHook();
	SendMessageW(WindowHandle, WM_INSTALL_HOOKS, NULL, (LPARAM)WindowHandle);

	WORKSPACE_INFO* Workspace = &WorkspaceList[CurrentWorkspaceInFocus];
	BOOL IsFirstTile = (!Workspace->Tiles);

	TILE_INFO* TileToAdd = (TILE_INFO*)malloc(sizeof(TILE_INFO));
	RtlZeroMemory(TileToAdd, sizeof(TILE_INFO));

	TileToAdd->WindowHandle = WindowHandle;

	LinkNode(Workspace, TileToAdd);

	// if it's the first one in the workspace then don't realloc
	if (IsFirstTile)
		Workspace->Tiles = TileToAdd;

	if (!Workspace->Tiles)
		FailWithCode("realloc Tiles failed\n");

	AddTileToWorkspace(Workspace, TileToAdd);
	RenderWorkspace(CurrentWorkspaceInFocus);

}


TILE_INFO* VerifyExistingWindow(TILE_INFO* Tiles, HWND WindowHandle)
{
	for (TILE_INFO* Tile = Tiles; Tile; Tile = Tile->ChildTile)
	{
		if (Tile->BranchTile)
		{
			TILE_INFO* RecursiveTile = VerifyExistingWindow(Tile->BranchTile, WindowHandle);

			if (RecursiveTile)
				return RecursiveTile;
		}

		if (WindowHandle == Tile->WindowHandle)
			return Tile;
	}

	return NULL;

}

extern "C" __declspec(dllexport) VOID OnDestroyWindow(HWND WindowHandle)
{

	if (IsDefaultWindow(WindowHandle))
		return;


	WORKSPACE_INFO* Workspace = &WorkspaceList[CurrentWorkspaceInFocus];
	TILE_INFO* Tiles = Workspace->Tiles;
	TILE_INFO* TileToRemove = NULL;

	// If the tile doesn't exist in the workspace something wrong happened.

	TileToRemove = VerifyExistingWindow(Workspace->Tiles, WindowHandle);

	if (!TileToRemove)
		return;

	//FailWithCode("Got tile to remove but couldn't find it");

	RemoveTileFromWorkspace(Workspace, TileToRemove);
	RenderWorkspace(CurrentWorkspaceInFocus);

}


VOID CreateStatusButton(HWND ParentHandle, INT Slot, const char* ButtonText)
{

	INT ButtonStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;

	StatusButtonList[Slot] = CreateWindowExA(
		NULL,
		"BUTTON",  // Predefined class; Unicode assumed 
		ButtonText,      // Button text 
		ButtonStyle,  // Styles 
		0,         // x position 
		0,         // y position 
		25,        // Button width
		25,        // Button height
		ParentHandle,     // Parent window
		NULL,       // No menu.
		NULL,
		NULL);      // Pointer not needed.

	if (!StatusButtonList[Slot])
		FailWithCode("Couldn't Create Status Bar Button");

}

INT ButtonIdx = 1;
HBRUSH ButtonBrush;
HBRUSH DefaultBrush;

LRESULT CALLBACK StatusBarMsgHandler(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{

	CHAR ButtonText[] = { '1',  '\0' };


	switch (Message)
	{
	case WM_CREATE:
		CreateStatusButton(WindowHandle, ButtonIdx, ButtonText);
		ButtonIdx++;
		break;
	case WM_DRAWITEM:
	{
		PDRAWITEMSTRUCT DrawItem = (PDRAWITEMSTRUCT)LParam;
		WCHAR BtnCaption[3] = { 0 };
		GetWindowTextW(DrawItem->hwndItem, BtnCaption, 6);

		if (BtnToColor == WindowHandle)
		{
			SetBkColor(DrawItem->hDC, RGB(0, 0, 255));
			SetTextColor(DrawItem->hDC, RGB(255, 255, 255));
		}
		else
		{
			SetBkColor(DrawItem->hDC, GetSysColor(COLOR_BTNFACE));
			SetTextColor(DrawItem->hDC, RGB(0,0,0));
		}

		DrawTextW(DrawItem->hDC, BtnCaption, 1, &DrawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		return TRUE;
		break;
	}
	case WM_CTLCOLORBTN:
		if (BtnToColor == WindowHandle)
		{
			//Blue Brush.
			return (LRESULT)ButtonBrush;
		}
		else
		{
			//Red Brush.
			//ButtonBrush = CreateSolidBrush(RGB(rand() % 252, rand() % 255, rand() % 255));
			return (LRESULT)DefaultBrush;
		}
		break;
	default:
		return DefWindowProcA(WindowHandle, Message, WParam, LParam);
	}

	return 0;
}

LRESULT CALLBACK WindowMessageHandler2(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	return DefWindowProcA(WindowHandle, Message, WParam, LParam);
}

LRESULT CALLBACK WindowMessageHandler(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	return DefWindowProcA(WindowHandle, Message, WParam, LParam);
}

LRESULT CALLBACK FocusMessageHandler(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	switch (Message) 
	{
		case WM_TIMER:
		{
			UINT TimerId = (UINT)WParam;

			switch (TimerId)
			{
			case TIMER_FOCUS:
			{
				RECT FocusRect;

				GetWindowRect(FocusWindow, &FocusRect);

				INT Width = (FocusRect.right - FocusRect.left);
				INT Height = (FocusRect.bottom - FocusRect.top);

				if (!SetWindowPos(FocusWindow, HWND_TOPMOST, FocusRect.left, FocusRect.top, Width, Height, 0))
					FailWithCode("SetWindowPos Focus Window");

				if (!SetWindowPos(FocusWindow, HWND_NOTOPMOST, FocusRect.left, FocusRect.top, Width, Height, 0))
					FailWithCode("SetWindowPos Focus Window");
			}
			default:
				return DefWindowProcA(WindowHandle, Message, WParam, LParam);
			}

		}

		default:
			return DefWindowProcA(WindowHandle, Message, WParam, LParam);
	}
}

LRESULT CALLBACK NewWindowHookProc(int nCode, WPARAM WParam, LPARAM LParam)
{
	if (nCode < 0)
		return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);

	if (nCode != HCBT_CREATEWND || nCode != HCBT_DESTROYWND)
		return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);

	if (nCode == HCBT_CREATEWND)
	{
		logc(9, "NewWindow : %X\n", WParam);
		OnNewWindow((HWND)WParam);
	}

	if (nCode == HCBT_DESTROYWND)
		OnDestroyWindow((HWND)WParam);

	return CallNextHookEx(KeyboardHook, nCode, WParam, LParam);

}

VOID InstallNewWindowHook()
{

	WinHook64 = LoadLibraryA("winhook64.dll");

	if (!WinHook64)
		FailWithCode("Couldn't load WinHook64.dll");

	HOOKPROC NewWindowHookProc = (HOOKPROC)GetProcAddress(WinHook64, "OnWindowAction");

	if (!NewWindowHookProc)
		FailWithCode("Couldn't find (x64)NewWindowHookProc");

	NewWindowHook = SetWindowsHookExA(WH_SHELL, NewWindowHookProc, WinHook64, 0);

	logc(5,"NewWindowHook : %p\n", NewWindowHook);

	if (!NewWindowHook)
		FailWithCode("Couldn't set NewWindowHook");

}

VOID HandleWindowMessage(MSG* Message)
{

	HWND TargetWindow = (HWND)Message->lParam;
	
	//The WindowHandle is being destroyed
	if (Message->wParam)
	{
		logc(6,"DestroyWindow : %X\n", TargetWindow);
		OnDestroyWindow(TargetWindow);
	}
	else
	{
		logc(9, "NewWindow : %X\n", TargetWindow);
		OnNewWindow(TargetWindow);
	}
}

VOID CreateInitialWindow()
{
	WNDCLASSEXA WindowClass;

	WindowClass.cbSize			= sizeof(WNDCLASSEX);
	WindowClass.style			= NULL_STYLE;
	WindowClass.lpfnWndProc		= WindowMessageHandler2;
	WindowClass.cbClsExtra		= 0;
	WindowClass.cbWndExtra		= 0;
	WindowClass.hInstance		= NULL;
	WindowClass.hIcon			= NULL;
	WindowClass.hCursor			= NULL;
	WindowClass.hbrBackground	= NULL;
	WindowClass.lpszMenuName	= NULL;
	WindowClass.lpszClassName	= AppWindowName;
	WindowClass.hIconSm			= NULL;

	WindowAtom = RegisterClassExA(&WindowClass);

	if (!WindowAtom)
		FailWithCode("Couldn't Register Window Class");

	MainWindow = CreateWindowExA(
		NULL_EX_STYLE,
		AppWindowName,
		"Win3wm",
		NULL_STYLE,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!MainWindow)
	{
		FailWithCode("Could not create main window!");
	}

	if (!ChangeWindowMessageFilterEx(MainWindow, WM_INSTALL_HOOKS, MSGFLT_ALLOW, NULL))
		FailWithCode("Change Window Perms");

}


LRESULT CALLBACK DebugMessageHandler(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	return DefWindowProcA(WindowHandle, Message, WParam, LParam);
}

VOID CreateDebugOverlay()
{

	return;

	WNDCLASSEXA WindowClass;

	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = NULL;
	WindowClass.lpfnWndProc = WindowMessageHandler;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = NULL;
	WindowClass.hIcon = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = CreateSolidBrush(RGB(0xff, 0, 0));
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = DebugAppWindowName;
	WindowClass.hIconSm = NULL;

	DebugWindowAtom = RegisterClassExA(&WindowClass);

	if (!DebugWindowAtom)
		FailWithCode("Couldn't Register DebugWindow Class");

	DebugWindow = CreateWindowExA(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
		DebugAppWindowName,
		"FocusDebugOverlayWindow",
		0,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!DebugWindow)
		FailWithCode("DebugWindow creation failed");

	if (!SetLayeredWindowAttributes(DebugWindow, 0, 100, LWA_ALPHA))
		FailWithCode("SetWindowlayered attribs failed");


	ShowWindow(DebugWindow, SW_SHOW);
	SetWindowLongPtrA(DebugWindow, GWL_STYLE, 0);
	SetWindowLongPtrA(DebugWindow, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
	UpdateWindow(DebugWindow);




}

VOID CreateFocusOverlay()
{

	WNDCLASSEXA WindowClass;

	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = NULL;
	WindowClass.lpfnWndProc = FocusMessageHandler;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = NULL;
	WindowClass.hIcon = NULL;
	WindowClass.hCursor = NULL;
	WindowClass.hbrBackground = CreateSolidBrush(RGB(0xff, 0, 0xff));
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = FocusAppWindowName;
	WindowClass.hIconSm = NULL;

	FocusAtom = RegisterClassExA(&WindowClass);

	if (!FocusAtom)
		FailWithCode("Couldn't Register FocusWindow Class");

	FocusWindow = CreateWindowExA(
		WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
		FocusAppWindowName,
		"FocusWin3WM",
		0,
		0,
		0,
		0,
		0,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!FocusWindow)
		FailWithCode("FocusWindow creation failed");

	ShowWindow(FocusWindow, SW_SHOW);
	SetWindowLongPtrA(FocusWindow, GWL_STYLE, 0);
	SetWindowLongPtrA(FocusWindow, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
	UpdateWindow(FocusWindow);

	SetTimer(FocusWindow, TIMER_FOCUS, 1000, NULL);

}

VOID InitScreenGlobals()
{

	HWND ShellWindow = FindWindowW(L"Shell_TrayWnd", NULL);
	RECT ShellRect;

	if (!ShellWindow)
		FailWithCode("No Shell Window Found");

	GetWindowRect(ShellWindow, &ShellRect);

	ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	ScreenHeight = GetSystemMetrics(SM_CYSCREEN) - (ShellRect.bottom - ShellRect.top);

	ButtonBrush = CreateSolidBrush(RGB(0, 0, 255));
	DefaultBrush = GetSysColorBrush(COLOR_BTNFACE);

	logc(9, "ScreenHeight: %lu - ScreenWidth : %lu\n", ScreenHeight, ScreenWidth);

}


VOID MoveButtonToPosition(INT Slot, INT Position, const char* ButtonText)
{
	assert(StatusButtonList[Slot]);

	if (!SetWindowPos(StatusButtonList[Slot], HWND_TOPMOST, 10 + 40 * Position, 10, 0, 0, SWP_NOSIZE))
		FailWithCode("SetWindowPos Status Button");

}

VOID InitStatusBar()
{

	INT WorkspaceCount = GetActiveWorkspace();

	WNDCLASSEXA WindowClass;

	WindowClass.cbSize			= sizeof(WNDCLASSEX);
	WindowClass.style			= NULL_STYLE;
	WindowClass.lpfnWndProc		= StatusBarMsgHandler;
	WindowClass.cbClsExtra		= 0;
	WindowClass.cbWndExtra		= 0;
	WindowClass.hInstance		= NULL;
	WindowClass.hIcon			= NULL;
	WindowClass.hCursor			= NULL;
	WindowClass.hbrBackground	= CreateSolidBrush(RGB(0x22, 0x22, 0x22));
	WindowClass.lpszMenuName	= NULL;
	WindowClass.lpszClassName	= StatusBarWindowName;
	WindowClass.hIconSm			= NULL;

	StatusBarAtom = RegisterClassExA(&WindowClass);

	HWND SearchBarWindow = FindWindowExW(FindWindowW(L"Shell_TrayWnd", NULL), NULL, L"TrayDummySearchControl", NULL);
	RECT SearchBarRect;

	assert(SearchBarWindow);

	GetWindowRect(SearchBarWindow, &SearchBarRect);

	

	for (int i = 1; i < 10; i++)
	{

		StatusBarWindow[i] = CreateWindowExA(
			WS_EX_TOOLWINDOW,
			StatusBarWindowName,
			"Win3wmStatusBar",
			WS_POPUP,
			SearchBarRect.left + i * 25,
			SearchBarRect.top - 35,
			25,
			25,
			NULL,
			NULL,
			NULL,
			NULL
		);

		if (i == CurrentWorkspaceInFocus)
			BtnToColor = StatusBarWindow[CurrentWorkspaceInFocus];

		if (!StatusBarWindow[i])
			FailWithCode("Could not create Status Bar Window!");

		SetWindowLongPtrA(StatusBarWindow[i], GWL_EXSTYLE, WS_EX_TOOLWINDOW);
		SetWindowLongPtrA(StatusBarWindow[i], GWL_STYLE, WS_POPUP);
		SetWindowPos(StatusBarWindow[i], HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		UpdateWindow(StatusBarWindow[i]);

		if (i - 1 < WorkspaceCount)
		{
			ShowWindow(StatusBarWindow[i], SW_SHOW);
		}
		else
			ShowWindow(StatusBarWindow[i], SW_HIDE);
			
	}



	

}

VOID DpiSet()
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
}

INT main()
{
	if (AppRunningCheck())
		Fail("Only a single instance of Win3m can be run");

	DpiSet();

	FreeConsole();

	SetCrashRoutine();
	InitScreenGlobals();
	InitWorkspaceList();
	CreateInitialWindow();
	LoadNecessaryModules();
	//EnterFullScreen();
	//ShowWindow(MainWindow, SW_SHOW);
	//MoveWindowToBack();
	GetOtherApplicationWindows();
	PerformInitialRegroupring();
	InstallNewWindowHook();
	InstallKeyboardHooks();
	CreateDebugOverlay();
	CreateFocusOverlay();
	RenderWorkspace(CurrentWorkspaceInFocus);
	InitStatusBar();
	is_init = TRUE;

	MSG msg;
	while (int RetVal = GetMessageA(&msg, NULL, 0,  0))
	{

		if (msg.message == WM_INSTALL_HOOKS)
		{
			HandleWindowMessage(&msg);
			continue;
		}

		if (RetVal == -1)
			PostQuitMessage(325);
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}

	logc(5, "hit2\n");
	__debugbreak();

	return 0;

}

