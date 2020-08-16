/*

Adding a InputBox() to any c++ program - an article for Code Project
by Michael Haephrati, Secured Globe, Inc. www.securedglobe.net
haephrati@gmail.com

June 2019
©2019 Michael Haephrati and Secured Globe, Inc. 1501 Broadway Ave. STE 1200, New York 10036, NY
*/

#include "stdafx.h"
#include "utils.h"
void setTextAlignment(HWND hwnd,int intTextAlignment)
{
	LONG_PTR s;
	LONG_PTR textalignment = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (textalignment != intTextAlignment)
	{
		//delete the last text alignment
		if (intTextAlignment == 0)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s & ~(SS_LEFT);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}
		else if (intTextAlignment == 1)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s & ~(SS_CENTER);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}
		else if (intTextAlignment == 2)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s & ~(SS_RIGHT);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}

		textalignment = intTextAlignment;

		//put the new text alignment
		if (textalignment == 0)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s | (SS_LEFT);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}
		else if (textalignment == 1)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s | (SS_CENTER);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}
		else if (textalignment == 2)
		{
			s = GetWindowLongPtr(hwnd, GWL_STYLE);
			s = s | (SS_RIGHT);
			SetWindowLongPtr(hwnd, GWL_STYLE, (LONG_PTR)s);
		}
		SetWindowPos(hwnd, 0, 0, 0, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_DRAWFRAME);
	}
}
void ReportError(const char *CallingFunction)
{
	DWORD error = GetLastError();
	LPVOID lpMsgBuf;
	DWORD bufLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);
	wprintf(L"%S: Error '%s'\n", CallingFunction,(wchar_t *)lpMsgBuf);
}