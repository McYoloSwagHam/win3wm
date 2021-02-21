#pragma once
#pragma once

#include <objbase.h>
#include <hstring.h>
#include <inspectable.h>
#include <ObjectArray.h>
#include <vector>
#include <utility>
#include "lua.hpp"
#include "sol.hpp"


#define WM_INSTALL_HOOKS 0x8069
#define WM_SHUTDOWN 0x806C
#define WM_MOVE_TILE 0x806b
#define WM_TILE_CHANGED 0x806d
#define WM_USER_FOCUS 0x8096
#define MIN_ALL 419
#define ID_EXIT 6
#define TIMER_FOCUS 0x1234

const CLSID CLSID_ImmersiveShell = {
	0xC2F03A33, 0x21F5, 0x47FA, 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 };

const CLSID CLSID_VirtualDesktopAPI_Unknown = {
	0xC5E0CDCA, 0x7B6E, 0x41B2, 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B };


//const CLSID guidObject = {0xa5cd92ff,0x29be,0x454c,0x8d,0x04,0xd8,0x28,0x79,0xfb,0x3f,0x1b};
//GUID guidObject = {0xf31574d6,0xb682,0x4cdc,0xbd,0x56,0x18,0x27,0x86,0x0a,0xbe,0xc6};
//const IID IID_IVirtualDesktopManagerInternal = {
//	0xEF9F1A6C, 0xD3CC, 0x4358, 0xB7, 0x12, 0xF8, 0x4B, 0x63, 0x5B, 0xEB, 0xE7 };

//F31574D6-B682-4CDC-BD56-1827860ABEC6
const IID IID_IVirtualDesktopManagerInternalNew = {
	0xF31574D6, 0xB682, 0x4CDC, 0xBD, 0x56, 0x18, 0x27, 0x86, 0x0A, 0xBE, 0xC6 };

const IID UIID_IVirtualDesktopWinPreview = {
	0x094afe11,0x44f2,0x4ba0,0x97,0x6f,0x29,0xa9,0x7e,0x26,0x3e,0xe0
};

const CLSID CLSID_VirtualDesktopPinnedApps = {
	0xb5a399e7, 0x1c87, 0x46b8, 0x88, 0xe9, 0xfc, 0x57, 0x47, 0xb1, 0x71, 0xbd
};



// ??. IApplicationView ?? Windows Runtime
// Ignore following API's:
#define IAsyncCallback UINT
#define IImmersiveMonitor UINT
#define APPLICATION_VIEW_COMPATIBILITY_POLICY UINT
#define IShellPositionerPriority UINT
#define IApplicationViewOperation UINT
#define APPLICATION_VIEW_CLOAK_TYPE UINT
#define IApplicationViewPosition UINT

// Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{372E1D3B-38D3-42E4-A15B-8AB2B178F513}
// Found with searching "IApplicationView"
DECLARE_INTERFACE_IID_(IApplicationView, IInspectable, "372E1D3B-38D3-42E4-A15B-8AB2B178F513")
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObject) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/*** IInspectable methods ***/
	STDMETHOD(GetIids)(__RPC__out ULONG * iidCount, __RPC__deref_out_ecount_full_opt(*iidCount) IID * *iids) PURE;
	STDMETHOD(GetRuntimeClassName)(__RPC__deref_out_opt HSTRING * className) PURE;
	STDMETHOD(GetTrustLevel)(__RPC__out TrustLevel * trustLevel) PURE;

	/*** IApplicationView methods ***/
	STDMETHOD(SetFocus)(THIS) PURE;
	STDMETHOD(SwitchTo)(THIS) PURE;
	STDMETHOD(TryInvokeBack)(THIS_ IAsyncCallback*) PURE; // Proc8
	STDMETHOD(GetThumbnailWindow)(THIS_ HWND*) PURE; // Proc9
	STDMETHOD(GetMonitor)(THIS_ IImmersiveMonitor**) PURE; // Proc10
	STDMETHOD(GetVisibility)(THIS_ int*) PURE; // Proc11
	STDMETHOD(SetCloak)(THIS_ APPLICATION_VIEW_CLOAK_TYPE, int) PURE; // Proc12
	STDMETHOD(GetPosition)(THIS_ REFIID, void**) PURE; // Proc13
	STDMETHOD(SetPosition)(THIS_ IApplicationViewPosition*) PURE; // Proc14
	STDMETHOD(InsertAfterWindow)(THIS_ HWND) PURE; // Proc15
	STDMETHOD(GetExtendedFramePosition)(THIS_ RECT*) PURE; // Proc16
	STDMETHOD(GetAppUserModelId)(THIS_ PWSTR*) PURE; // Proc17
	STDMETHOD(SetAppUserModelId)(THIS_ PCWSTR) PURE; // Proc18
	STDMETHOD(IsEqualByAppUserModelId)(THIS_ PCWSTR, int*) PURE; // Proc19
	STDMETHOD(GetViewState)(THIS_ UINT*) PURE; // Proc20
	STDMETHOD(SetViewState)(THIS_ UINT) PURE; // Proc21
	STDMETHOD(GetNeediness)(THIS_ int*) PURE; // Proc22
	STDMETHOD(GetLastActivationTimestamp)(THIS_ ULONGLONG*) PURE;
	STDMETHOD(SetLastActivationTimestamp)(THIS_ ULONGLONG) PURE;
	STDMETHOD(GetVirtualDesktopId)(THIS_ GUID*) PURE;
	STDMETHOD(SetVirtualDesktopId)(THIS_ REFGUID) PURE;
	STDMETHOD(GetShowInSwitchers)(THIS_ int*) PURE;
	STDMETHOD(SetShowInSwitchers)(THIS_ int) PURE;
	STDMETHOD(GetScaleFactor)(THIS_ int*) PURE;
	STDMETHOD(CanReceiveInput)(THIS_ BOOL*) PURE;
	STDMETHOD(GetCompatibilityPolicyType)(THIS_ APPLICATION_VIEW_COMPATIBILITY_POLICY*) PURE;
	STDMETHOD(SetCompatibilityPolicyType)(THIS_ APPLICATION_VIEW_COMPATIBILITY_POLICY) PURE;
	//STDMETHOD(GetPositionPriority)(THIS_ IShellPositionerPriority**) PURE; // removed in 1803
	//STDMETHOD(SetPositionPriority)(THIS_ IShellPositionerPriority*) PURE; // removed in 1803
	STDMETHOD(GetSizeConstraints)(THIS_ IImmersiveMonitor*, SIZE*, SIZE*) PURE;
	STDMETHOD(GetSizeConstraintsForDpi)(THIS_ UINT, SIZE*, SIZE*) PURE;
	STDMETHOD(SetSizeConstraintsForDpi)(THIS_ const UINT*, const SIZE*, const SIZE*) PURE;
	//STDMETHOD(QuerySizeConstraintsFromApp)(THIS) PURE; // removed in 1803
	STDMETHOD(OnMinSizePreferencesUpdated)(THIS_ HWND) PURE;
	STDMETHOD(ApplyOperation)(THIS_ IApplicationViewOperation*) PURE;
	STDMETHOD(IsTray)(THIS_ BOOL*) PURE;
	STDMETHOD(IsInHighZOrderBand)(THIS_ BOOL*) PURE;
	STDMETHOD(IsSplashScreenPresented)(THIS_ BOOL*) PURE;
	STDMETHOD(Flash)(THIS) PURE;
	STDMETHOD(GetRootSwitchableOwner)(THIS_ IApplicationView**) PURE; // proc45
	STDMETHOD(EnumerateOwnershipTree)(THIS_ IObjectArray**) PURE; // proc46

	STDMETHOD(GetEnterpriseId)(THIS_ PWSTR*) PURE; // proc47
	STDMETHOD(IsMirrored)(THIS_ BOOL*) PURE; //

	STDMETHOD(Unknown1)(THIS_ int*) PURE;
	STDMETHOD(Unknown2)(THIS_ int*) PURE;
	STDMETHOD(Unknown3)(THIS_ int*) PURE;
	STDMETHOD(Unknown4)(THIS_ int) PURE;
	STDMETHOD(Unknown5)(THIS_ int*) PURE;
	STDMETHOD(Unknown6)(THIS_ int) PURE;
	STDMETHOD(Unknown7)(THIS) PURE;
	STDMETHOD(Unknown8)(THIS_ int*) PURE;
	STDMETHOD(Unknown9)(THIS_ int) PURE;
	STDMETHOD(Unknown10)(THIS_ int, int) PURE;
	STDMETHOD(Unknown11)(THIS_ int) PURE;
	STDMETHOD(Unknown12)(THIS_ SIZE*) PURE;

};

// ??????????? ????

const IID IID_VDESKTOP = {
0xFF72FFDD, 0xBE7E, 0x43FC, 0x9C, 0x03, 0xAD, 0x81, 0x68, 0x1E, 0x88, 0xE4
};

const IID IID_VDESKTOP_INSIDER = {
0x62fdf88b, 0x11ca, 0x4afb, 0x8b, 0xd8, 0x22, 0x96, 0xdf, 0xae, 0x49, 0xe2
};

//const IID IID_VDESKTOP_INSIDER = {
//0xC90250F3,0x4D7D, 0x4991, 0x9B69, 0xA5, 0xC5, 0xBC, 0x1C, 0x2A, 0xE6
//};

EXTERN_C const IID IID_IVirtualDesktop;

//MIDL_INTERFACE("C863F05A-1378-4F55-9F8C-6B4C00EA03FC")
//MIDL_INTERFACE("FF72FFDD-BE7E-43FC-9C03-AD81681E88E4")
class IVirtualDesktop : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE IsViewVisible(
		IApplicationView * pView,
		int* pfVisible) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetID(
		GUID* pGuid) = 0;
};

enum AdjacentDesktop
{
	// ???????? ??????? ???? ?????
	LeftDirection = 3,
	// ???????? ??????? ???? ??????
	RightDirection = 4
};

// ???????? ??????????? ??????
const IID UUID_IVirtualDesktopManagerInternal_10130{
	0xEF9F1A6C, 0xD3CC, 0x4358, 0xB7, 0x12, 0xF8, 0x4B, 0x63, 0x5B, 0xEB, 0xE7
};
const IID UUID_IVirtualDesktopManagerInternal_10240{
	0xAF8DA486, 0x95BB, 0x4460, 0xB3, 0xB7, 0x6E, 0x7A, 0x6B, 0x29, 0x62, 0xB5
};
const IID UUID_IVirtualDesktopManagerInternal_14393{
	0xf31574d6, 0xb682, 0x4cdc, 0xbd, 0x56, 0x18, 0x27, 0x86, 0x0a, 0xbe, 0xc6
};

const IID UUID_IVirtualDesktopManagerInternal_9200{
	0xb1ad6220, 0x8b03, 0x4345, 0xb9, 0xfd, 0xd8, 0xe6, 0xa8, 0xba, 0xab, 0xab
};



//MIDL_INTERFACE("094AFE11-44F2-4BA0-976F-29A97E263EE0")
class IVirtualDesktopManagerInternalNew : public IUnknown
//class IVirtualDesktopManagerInternal : public IUnknown
{
	public: 
		virtual HRESULT __stdcall Proc3( PVOID* p0,  int64_t* p1);
		virtual HRESULT __stdcall MoveViewToDesktop( IApplicationView* p0,  IVirtualDesktop* p1);
		virtual HRESULT __stdcall Proc5( IApplicationView* p0,  int64_t* p1);
		virtual HRESULT __stdcall Proc6( PVOID* p0,  IVirtualDesktop** p1);
		virtual HRESULT __stdcall GetDesktops( PVOID* p0,  IObjectArray** p1);
		virtual HRESULT __stdcall Proc8( IVirtualDesktop* p0,  int64_t p1,  IVirtualDesktop** p2);
		virtual HRESULT __stdcall SwitchDesktop( PVOID* p0,  IVirtualDesktop* p1);
		virtual HRESULT __stdcall CreateDesktopW( PVOID* p0,  IVirtualDesktop** p1);
		virtual HRESULT __stdcall RemoveDesktop( IVirtualDesktop* p0,  IVirtualDesktop* p1);
		virtual HRESULT __stdcall Proc12( GUID* p0,  IVirtualDesktop** p1);
		virtual HRESULT __stdcall Proc13( IVirtualDesktop* p0,  IObjectArray** p1,  IObjectArray** p2);
		virtual HRESULT __stdcall Proc14( IVirtualDesktop* p0,  PVOID* p1);
		virtual HRESULT __stdcall Proc15( IApplicationView* p0,  IApplicationView* p1);
		virtual HRESULT __stdcall Proc16( int64_t* p0);
};


// 10130
//MIDL_INTERFACE("EF9F1A6C-D3CC-4358-B712-F84B635BEBE7")
// 10240
//MIDL_INTERFACE("AF8DA486-95BB-4460-B3B7-6E7A6B2962B5")
// 14393
//MIDL_INTERFACE("f31574d6-b682-4cdc-bd56-1827860abec6")
class IVirtualDesktopManagerInternal : public IUnknown
//class IVirtualDesktopManagerInternal : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetCount(
		UINT* pCount) = 0;

	virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
		IApplicationView* pView,
		IVirtualDesktop* pDesktop) = 0;

	// 10240
	virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(
		IApplicationView* pView,
		int* pfCanViewMoveDesktops) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(
		IVirtualDesktop** desktop) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetDesktops(
		IObjectArray** ppDesktops) = 0;

	// ????????? ????????? ???????? ????? ???????????? ??????????, ? ?????? ???????????
	virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(
		IVirtualDesktop* pDesktopReference,
		AdjacentDesktop uDirection,
		IVirtualDesktop** ppAdjacentDesktop) = 0;

	virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(
		IVirtualDesktop* pDesktop) = 0;

	virtual HRESULT STDMETHODCALLTYPE CreateDesktopW(
		IVirtualDesktop** ppNewDesktop) = 0;

	// pFallbackDesktop - ??????? ???? ?? ??????? ????? ???????? ??????? ????? ???????? ??????????
	virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(
		IVirtualDesktop* pRemove,
		IVirtualDesktop* pFallbackDesktop) = 0;

	// 10240
	virtual HRESULT STDMETHODCALLTYPE FindDesktop(
		GUID* desktopId,
		IVirtualDesktop** ppDesktop) = 0;
};


struct VirtualDesktopWrapper {

	BOOL IsPreviewBuild;
	IVirtualDesktopManagerInternalNew*  VDesktopManagerInternalInsider;
	IVirtualDesktopManagerInternal*  VDesktopManagerInternal;
	IID CurrentIID;

	VirtualDesktopWrapper(PVOID VDesktopManager, BOOL IsPreviewBuild) {

		this->IsPreviewBuild = IsPreviewBuild;

		if (this->IsPreviewBuild) 
		{
			VDesktopManagerInternalInsider = (IVirtualDesktopManagerInternalNew*)VDesktopManager;
			VDesktopManagerInternal = NULL;
			CurrentIID = IID_VDESKTOP_INSIDER;
		}
		else 
		{
			VDesktopManagerInternal = (IVirtualDesktopManagerInternal*)VDesktopManager;
			VDesktopManagerInternalInsider = NULL;
			CurrentIID = IID_VDESKTOP;
		}
	}

	HRESULT MoveViewToDesktop(IApplicationView* TargetView, IVirtualDesktop* TargetDesktop) {

		HRESULT Result;

		if (this->IsPreviewBuild)
			Result = VDesktopManagerInternalInsider->MoveViewToDesktop(TargetView, TargetDesktop);
		else
			Result = VDesktopManagerInternal->MoveViewToDesktop(TargetView, TargetDesktop);

		return Result;
	}

	HRESULT CreateDesktopW(IVirtualDesktop** VirtualDesktop) {

		HRESULT Result;

		if (this->IsPreviewBuild)
			Result = VDesktopManagerInternalInsider->CreateDesktopW(NULL, VirtualDesktop);
		else
			Result = VDesktopManagerInternal->CreateDesktopW(VirtualDesktop);

		return Result;
	}

	HRESULT SwitchDesktop(IVirtualDesktop* VirtualDesktop) {
		HRESULT Result;

		if (this->IsPreviewBuild)
			Result = VDesktopManagerInternalInsider->SwitchDesktop(NULL, VirtualDesktop);
		else
			Result = VDesktopManagerInternal->SwitchDesktop(VirtualDesktop);

		return Result;

	}

	HRESULT GetDesktops(IObjectArray** ObjectArray) {

		HRESULT Result;

		if (this->IsPreviewBuild)
			Result = VDesktopManagerInternalInsider->GetDesktops(NULL, ObjectArray);
		else
			Result = VDesktopManagerInternal->GetDesktops(ObjectArray);

		return Result;
	}

	HRESULT RemoveDesktop(IVirtualDesktop* Desktop, IVirtualDesktop* Fallback) {

		HRESULT Result;

		if (this->IsPreviewBuild)
			Result = VDesktopManagerInternalInsider->RemoveDesktop(Desktop, Fallback);
		else
			Result = VDesktopManagerInternal->RemoveDesktop(Desktop, Fallback);

		return Result;

	}

};

EXTERN_C const IID IID_IVirtualDesktopManager;

//MIDL_INTERFACE("a5cd92ff-29be-454c-8d04-d82879fb3f1b")
//IVirtualDesktopManager : public IUnknown
//{
//public:
//	virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(
//		/* [in] */ __RPC__in HWND topLevelWindow,
//		/* [out] */ __RPC__out BOOL * onCurrentDesktop) = 0;
//
//	virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(
//		/* [in] */ __RPC__in HWND topLevelWindow,
//		/* [out] */ __RPC__out GUID* desktopId) = 0;
//
//	virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(
//		/* [in] */ __RPC__in HWND topLevelWindow,
//		/* [in] */ __RPC__in REFGUID desktopId) = 0;
//};

EXTERN_C const IID IID_IVirtualDesktopNotification;

MIDL_INTERFACE("C179334C-4295-40D3-BEA1-C654D965605A")
IVirtualDesktopNotification : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(
		IVirtualDesktop * pDesktop) = 0;

	virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(
		IVirtualDesktop* pDesktopDestroyed,
		IVirtualDesktop* pDesktopFallback) = 0;

	virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(
		IVirtualDesktop* pDesktopDestroyed,
		IVirtualDesktop* pDesktopFallback) = 0;

	virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(
		IVirtualDesktop* pDesktopDestroyed,
		IVirtualDesktop* pDesktopFallback) = 0;

	virtual HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(
		IApplicationView* pView) = 0;

	virtual HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(
		IVirtualDesktop* pDesktopOld,
		IVirtualDesktop* pDesktopNew) = 0;

};

EXTERN_C const IID IID_IVirtualDesktopNotificationService;

MIDL_INTERFACE("0CD45E71-D927-4F15-8B0A-8FEF525337BF")
IVirtualDesktopNotificationService : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Register(
		IVirtualDesktopNotification * pNotification,
		DWORD * pdwCookie) = 0;

	virtual HRESULT STDMETHODCALLTYPE Unregister(
		DWORD dwCookie) = 0;
};

// Ignore following API's:
#define IImmersiveApplication UINT
#define IApplicationViewChangeListener UINT

// In registry: Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{1841C6D7-4F9D-42C0-AF41-8747538F10E5}
DECLARE_INTERFACE_IID_(IApplicationViewCollection, IUnknown, "1841C6D7-4F9D-42C0-AF41-8747538F10E5")
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObject) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/*** IApplicationViewCollection methods ***/
	STDMETHOD(GetViews)(THIS_ IObjectArray**) PURE;
	STDMETHOD(GetViewsByZOrder)(THIS_ IObjectArray**) PURE;
	STDMETHOD(GetViewsByAppUserModelId)(THIS_ PCWSTR, IObjectArray**) PURE;
	STDMETHOD(GetViewForHwnd)(THIS_ HWND, IApplicationView**) PURE;
	STDMETHOD(GetViewForApplication)(THIS_ IImmersiveApplication*, IApplicationView**) PURE;
	STDMETHOD(GetViewForAppUserModelId)(THIS_ PCWSTR, IApplicationView**) PURE;
	STDMETHOD(GetViewInFocus)(THIS_ IApplicationView**) PURE;
	STDMETHOD(Unknown1)(THIS_ IApplicationView**) PURE;

	STDMETHOD(RefreshCollection)(THIS) PURE;
	STDMETHOD(RegisterForApplicationViewChanges)(THIS_ IApplicationViewChangeListener*, DWORD*) PURE;

	// Removed in 1809
	// STDMETHOD(RegisterForApplicationViewPositionChanges)(THIS_ IApplicationViewChangeListener*, DWORD*) PURE;
	STDMETHOD(UnregisterForApplicationViewChanges)(THIS_ DWORD) PURE;
};

DECLARE_INTERFACE_IID_(IVirtualDesktopPinnedApps, IUnknown, "4ce81583-1e4c-4632-a621-07a53543148f")
{
		/*** IUnknown methods ***/
		STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObject) PURE;
		STDMETHOD_(ULONG, AddRef)(THIS) PURE;
		STDMETHOD_(ULONG, Release)(THIS) PURE;


		/*** IVirtualDesktopPinnedApps methods ***/
		STDMETHOD(IsAppIdPinned)(THIS_ PCWSTR appId, BOOL*) PURE;
		STDMETHOD(PinAppID)(THIS_ PCWSTR appId) PURE;
		STDMETHOD(UnpinAppID)(THIS_ PCWSTR appId) PURE;
		STDMETHOD(IsViewPinned)(THIS_ IApplicationView*, BOOL*) PURE;
		STDMETHOD(PinView)(THIS_ IApplicationView*) PURE;
		STDMETHOD(UnpinView)(THIS_ IApplicationView*) PURE; 
};

enum NODE_TYPE
{
	TERMINAL = 0,
	VERTICAL_SPLIT = 1,
	HORIZONTAL_SPLIT = 2,
};

 struct PRE_WM_INFO
{
	LONG_PTR Style;
	LONG_PTR ExStyle;
	WINDOWPLACEMENT OldPlacement;

};

 struct BUTTON_STATE
 {
	 BOOL RenderBg;
	 BOOL RenderTxt;
	 BOOL IsPrimaryDisplay;
	 BOOL IsActiveWorkspace;
	 CHAR ButtonText[2];
 };

 struct DIFF_STATE
 {
	 INT NewTxt;
	 INT PrevTxt;
	 INT Slot;
 };

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
	BOOL IsDisplayChanged;
	BOOL IsRemovedTitleBar;
	BOOL IsConsole;
};

struct DISPLAY_INFO
{
	HWND TrayWindow;
	RECT TrayRect;
	HMONITOR Handle;
	RECT Rect;
	INT ScreenWidth;
	INT ScreenHeight;
	INT RealScreenWidth;
	INT RealScreenHeight;
	FLOAT HorizontalScalar;
	FLOAT VerticalScalar;
	HWND StatusBar;
	HWND BtnToColor;
};

struct TILE_TREE
{
	TILE_INFO* Focus;
	TILE_INFO* Root;
	BOOL IsFullScreen;
	BOOL NeedsRendering;
	DISPLAY_INFO* Display;
	NODE_TYPE Layout;
	LONG_PTR FullScreenStyle;
};

struct WORKSPACE_INFO
{
	std::unordered_map<HMONITOR, TILE_TREE> Trees;
	LONG_PTR FullScreenStyle;
	TILE_TREE* Tree;
	DISPLAY_INFO* Dsp;
	TILE_INFO* TileInFocus;
	IVirtualDesktop* VDesktop;
};

struct LUA_OPT
{
	BOOL On;
	sol::state State;
	const char* Path;
};

typedef PVOID(*InitFn)(HWND WindowHandle);
typedef VOID(*HOTKEY_FN)();

struct HOTKEY_DISPATCH
{
	BOOL ShiftCb;
	HOTKEY_FN HotKeyCb;
};

