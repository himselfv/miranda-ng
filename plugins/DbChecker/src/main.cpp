/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

HINSTANCE hInst;
int hLangpack = 0;
bool bServiceMode, bLaunchMiranda, bShortMode, bAutoExit;

DbToolOptions opts = { 0 };

PLUGININFOEX pluginInfoEx =
{
	sizeof(pluginInfoEx),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE | STATIC_PLUGIN,
	// {1F2BD439-FCBB-4951-8CB4-1E84828A5A7D}
	{ 0x1F2BD439, 0xFCBB, 0x4951, { 0x8C, 0xB4, 0x1E, 0x84, 0x82, 0x8A, 0x5A, 0x7D } }
	// Old DbChecker PID (now deprecated):
	// {A0138FC6-4C52-4501-AF93-7D3E20BCAE5B}
};

/////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfoEx;
}

// we implement service mode interface
extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = { MIID_SERVICEMODE, MIID_LAST };

/////////////////////////////////////////////////////////////////////////////////////////

static HANDLE hService; // do not remove it!

static INT_PTR ServiceMode(WPARAM, LPARAM)
{
	bLaunchMiranda = bShortMode = bAutoExit = false;
	bServiceMode = true;
	DialogBox(hInst, MAKEINTRESOURCE(IDD_WIZARD), nullptr, WizardDlgProc);
	return (bLaunchMiranda) ? SERVICE_CONTINUE : SERVICE_FAILED;
}

static INT_PTR CheckProfile(WPARAM wParam, LPARAM lParam)
{
	bShortMode = true;
	bLaunchMiranda = lParam != 0;
	bAutoExit = lParam == 2;
	bServiceMode = false;
	wcsncpy(opts.filename, (wchar_t*)wParam, _countof(opts.filename));
	return DialogBox(hInst, MAKEINTRESOURCE(IDD_WIZARD), nullptr, WizardDlgProc);
}

extern "C" __declspec(dllexport) int Load(void)
{
	mir_getLP(&pluginInfoEx);

	CreateServiceFunction(MS_DB_CHECKPROFILE, CheckProfile);
	hService = CreateServiceFunction(MS_SERVICEMODE_LAUNCH, ServiceMode);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	DestroyServiceFunction(hService);
	return 0;
}
