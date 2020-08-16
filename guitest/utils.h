/*

Adding a InputBox() to any c++ program - an article for Code Project
by Michael Haephrati, Secured Globe, Inc. www.securedglobe.net
haephrati@gmail.com

June 2019
©2019 Michael Haephrati and Secured Globe, Inc. 1501 Broadway Ave. STE 1200, New York 10036, NY
*/

#pragma once
void setTextAlignment(HWND hwnd, int textalignment);
#define REPORTERROR ReportError(__FUNCTION__)
void ReportError(const char *CallingFunction);