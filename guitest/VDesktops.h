#include "VirtualDesktops.h"
#include <windows.h>
#include <stdio.h>

#include <objbase.h>

#define WM_SWITCH_DESKTOP 0x8079

//------------------------------------------------------------------
// Com Variables
//------------------------------------------------------------------
IServiceProvider* ServiceProvider;
IVirtualDesktopManager* VDesktopManager;
IVirtualDesktopManagerInternal* VDesktopManagerInternal;
IVirtualDesktop* VWorkspaces[10];
IVirtualDesktop* FirstDesktop;
IApplicationViewCollection* ViewCollection;
IVirtualDesktopPinnedApps* PinnedApps;

UINT16 GetWinBuildNumber()
{
	UINT16 buildNumbers[] = { 10130, 10240, 14393, 9200, 18362};
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0,{ 0 }, 0, 0 };
	ULONGLONG mask = ::VerSetConditionMask(0, VER_BUILDNUMBER, VER_EQUAL);



	for (size_t i = 0; i < sizeof(buildNumbers) / sizeof(buildNumbers[0]); i++)
	{
		osvi.dwBuildNumber = buildNumbers[i];
		if (VerifyVersionInfoW(&osvi, VER_BUILDNUMBER, mask) != FALSE)
		{
			return buildNumbers[i];
		}
	}

	return 0;
}

const char* InitCom()
{
	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr))
		return "CoInitialize";

	hr = CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&ServiceProvider);

	if (FAILED(hr))
		return "CoCreateInstance";

	ServiceProvider->QueryService(__uuidof(IVirtualDesktopManager), &VDesktopManager);
	ServiceProvider->QueryService(__uuidof(IApplicationViewCollection), &ViewCollection);


	if (!VDesktopManager)
		return "VDesktopManager";

	if (!ViewCollection)
		return "ViewCollection";

	UINT16 buildNumber = GetWinBuildNumber();

	//OSVERSIONINFOA InfoVersion;
	//InfoVersion.dwOSVersionInfoSize = sizeof(InfoVersion);

	////GetVersionEx(&InfoVersion);

	//printf("%u\n", InfoVersion.dwBuildNumber);

	switch (buildNumber)
	{
	case 18362:
		hr = ServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, UUID_IVirtualDesktopManagerInternal_9200, (void**)&VDesktopManagerInternal);
		break;
	case 9200:
		hr = ServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, IID_IVirtualDesktopManagerInternalNew, (void**)&VDesktopManagerInternal);
		break;
	case 10130:
		hr = ServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, UUID_IVirtualDesktopManagerInternal_10130, (void**)&VDesktopManagerInternal);
		break;
	case 10240:
		hr = ServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, UUID_IVirtualDesktopManagerInternal_10240, (void**)&VDesktopManagerInternal);
		break;
	case 14393:
	default:
		hr = ServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, UUID_IVirtualDesktopManagerInternal_14393, (void**)&VDesktopManagerInternal);
		break;
	}

	if (FAILED(hr))
		return "QueryService VDM";

	if (!VDesktopManagerInternal)
		return "PinnedApps";

	return NULL;
}

HRESULT EnumVirtualDesktops(HMONITOR Handle, IVirtualDesktopManagerInternal* pDesktopManager)
{
	printf("EnumDesktops\n");

	IObjectArray* pObjectArray = nullptr;
	HRESULT hr = pDesktopManager->GetDesktops(&pObjectArray);

	if (SUCCEEDED(hr))
	{
		UINT count;
		hr = pObjectArray->GetCount(&count);

		if (SUCCEEDED(hr))
		{
			printf("Count : %u\n", count);

			for (UINT i = 0; i < count; i++)
			{
				IVirtualDesktop* pDesktop = nullptr;

				if (FAILED(pObjectArray->GetAt(i, __uuidof(IVirtualDesktop), (void**)&pDesktop)))
					continue;

				GUID id = { 0 };
				if (SUCCEEDED(pDesktop->GetID(&id)))
				{
					OLECHAR* GuidString;
					StringFromCLSID(id, &GuidString);
					printf("Desktop : %u : %ls\n", i, GuidString);
					CoTaskMemFree(GuidString);
				}

				pDesktop->Release();
			}
		}

		pObjectArray->Release();
	}

	printf("\n");
	return hr;
}

const char* RemoveOtherVDesktops()
{
	
	IObjectArray* DesktopObjectArray;
	HRESULT hr;

	hr = VDesktopManagerInternal->GetDesktops(&DesktopObjectArray);

	if (FAILED(hr))
		return "DesktopObjectArray";

	UINT NumDesktops;

	if (FAILED(DesktopObjectArray->GetCount(&NumDesktops)))
		return "NumDesktops";

	if (FAILED(DesktopObjectArray->GetAt(0, __uuidof(IVirtualDesktop), (PVOID*)&FirstDesktop)))
		return "No FirstDesktop.";

	IVirtualDesktop* CurrentDesktop = NULL;
	for (unsigned int i = 1; i < NumDesktops && i < 255; i++)
	{
	
		if (FAILED(DesktopObjectArray->GetAt(i, __uuidof(IVirtualDesktop), (PVOID*)&CurrentDesktop)))
			return "DesktopObjectArray->GetAt";

		VDesktopManagerInternal->RemoveDesktop(CurrentDesktop, FirstDesktop);

	}

	return NULL;

}

const char* SetupVDesktops(std::vector<WORKSPACE_INFO>& WorkspaceList)
{

	WorkspaceList[1].VDesktop = FirstDesktop;

	for (int i = 2; i < WorkspaceList.size(); i++)
	{
		HRESULT Hr;
		Hr = VDesktopManagerInternal->CreateDesktopW(&WorkspaceList[i].VDesktop);
		
		if (FAILED(Hr) || !WorkspaceList[i].VDesktop)
			return "CreateDesktopW";
	}

	return NULL;

}

const char* MoveWindowToVDesktop(HWND WindowHandle, IVirtualDesktop* VDesktop)
{

	IApplicationView* ApplicationView;

	HRESULT Result = ViewCollection->GetViewForHwnd(WindowHandle, &ApplicationView);

	if (FAILED(Result))
		return "GetViewForHwnd";

	Result = VDesktopManagerInternal->MoveViewToDesktop(ApplicationView, VDesktop);

	ApplicationView->Release();

	if (FAILED(Result))
		return "MoveViewToDesktop";
	
	return NULL;
}

const char* CheckViewPinned(HWND WindowHandle, BOOL* IsPinned)
{
	IApplicationView* ApplicationView;

	if (FAILED(ViewCollection->GetViewForHwnd(WindowHandle, &ApplicationView)))
		return "GetViewForHwnd";

	WCHAR ApplicationIdBuffer[1024];
	PWSTR ApplicationId = ApplicationIdBuffer;
	HRESULT Result;

	Result = PinnedApps->IsViewPinned(ApplicationView, IsPinned);
	ApplicationView->Release();

	if (FAILED(Result))
		return "IsAppIdPinned";

	return NULL;
}


const char* CheckAppPinned(HWND WindowHandle, BOOL* IsPinned)
{
	IApplicationView* ApplicationView;

	if (FAILED(ViewCollection->GetViewForHwnd(WindowHandle, &ApplicationView)))
		return "GetViewForHwnd";

	WCHAR ApplicationIdBuffer[1024];
	PWSTR ApplicationId = ApplicationIdBuffer;
	HRESULT Result;

	Result = ApplicationView->GetAppUserModelId(&ApplicationId);
	ApplicationView->Release();

	if (FAILED(Result))
		return "GetAppUserModelId";

	Result = PinnedApps->IsAppIdPinned(ApplicationId, IsPinned);

	if (FAILED(Result))
		return "IsAppIdPinned";

	return NULL;
}

const char* SwitchToWorkspace(WORKSPACE_INFO* Workspace)
{
	
	HRESULT Result = VDesktopManagerInternal->SwitchDesktop(Workspace->VDesktop);

	if (FAILED(Result))
		return "SwitchToWorkspace";

	return NULL;
}
