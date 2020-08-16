#include "VirtualDesktops.h"
#include <windows.h>
#include <stdio.h>

//------------------------------------------------------------------
// Com Variables
//------------------------------------------------------------------
IServiceProvider* ServiceProvider;
IVirtualDesktopManager* VDesktopManager;
IVirtualDesktopManagerInternal* VDesktopManagerInternal;
IVirtualDesktop* VWorkspaces[10];
IVirtualDesktop* FirstDesktop;
IApplicationViewCollection* ViewCollection;

UINT16 GetWinBuildNumber()
{
	UINT16 buildNumbers[] = { 10130, 10240, 14393 };
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

	switch (buildNumber)
	{
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

	return NULL;
}

HRESULT EnumVirtualDesktops(IVirtualDesktopManagerInternal* pDesktopManager)
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
	for (int i = 1; i < NumDesktops && i < 255; i++)
	{
	
		if (FAILED(DesktopObjectArray->GetAt(i, __uuidof(IVirtualDesktop), (PVOID*)&CurrentDesktop)))
			return "DesktopObjectArray->GetAt";

		VDesktopManagerInternal->RemoveDesktop(CurrentDesktop, FirstDesktop);

	}


}

const char* SetupVDesktops(IVirtualDesktop** DesktopArray, UINT Count)
{

	DesktopArray[0] = FirstDesktop;

	for (int i = 1; i < Count; i++)
	{
		HRESULT Hr;
		Hr = VDesktopManagerInternal->CreateDesktopW(&DesktopArray[i]);
		
		if (FAILED(Hr) || !DesktopArray[i])
			return "CreateDesktopW";
	}

	return NULL;

}

const char* MoveWindowToVDesktop(HWND WindowHandle, IVirtualDesktop* VDesktop)
{
	IApplicationView* ApplicationView;

	if (FAILED(ViewCollection->GetViewForHwnd(WindowHandle, &ApplicationView)))
		return "GetViewForHwnd";

	if (FAILED(VDesktopManagerInternal->MoveViewToDesktop(ApplicationView, VDesktop)))
		return "MoveViewToDesktop";

	return NULL;
}

