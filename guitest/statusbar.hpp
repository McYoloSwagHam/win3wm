#pragma once

#include <windows.h>
#include "VirtualDesktops.h"
#include <vector>


struct STATUS_BAR {

	BYTE CurrentActive;
	WORD StateBitMask;

	void SetBit(unsigned char Position)
	{
		this->StateBitMask |= (1 << Position);
	}

	void ClearBit(unsigned char Position)
	{
		this->StateBitMask &= (~(1 << Position));
	}

	void InitStatusBar()
	{

		WNDCLASSEXA WindowClass;

		WindowClass.cbSize = sizeof(WNDCLASSEX);
		WindowClass.style = 0;
		WindowClass.lpfnWndProc = StatusBarMsgHandler;
		WindowClass.cbClsExtra = 0;
		WindowClass.cbWndExtra = 0;
		WindowClass.hInstance = NULL;
		WindowClass.hIcon = NULL;
		WindowClass.hCursor = NULL;
		WindowClass.hbrBackground = CreateSolidBrush(RGB(0x22, 0x22, 0x22));
		WindowClass.lpszMenuName = NULL;
		WindowClass.lpszClassName = StatusBarWindowName;
		WindowClass.hIconSm = NULL;

		StatusBarAtom = RegisterClassExA(&WindowClass);
	}

	STATUS_BAR(BYTE CurWk, std::vector<WORKSPACE_INFO> WorkspaceList)
	{

		CurrentActive = CurWk;

		unsigned int Count = 0;

		for (auto& Workspace : WorkspaceList)
		{
			BOOL HasTile = FALSE;
			for (auto KeyValue : Workspace.Trees)
			{
				TILE_TREE* CurrentTree = &KeyValue.second;

				if (CurrentTree->Root)
				{
					HasTile = TRUE;
					break;
				}
			}

			if (HasTile)
				SetBit(Count);

			Count++;
		
		}


		void RenderStatusBar()
		{
		}

};
