/*

Import plugin for Miranda NG

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org)

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

INT_PTR CALLBACK WizardIntroPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_MIRANDADB, (LPARAM)MirandaPageProc);
			break;

		case IDCANCEL:
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK FinishedPageProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		SendMessage(GetParent(hdlg), WIZM_DISABLEBUTTON, 0, 0);
		SendMessage(GetParent(hdlg), WIZM_SETCANCELTEXT, 0, (LPARAM)TranslateT("Finish"));
		CheckDlgButton(hdlg, IDC_DONTLOADPLUGIN, BST_UNCHECKED);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			PostMessage(GetParent(hdlg), WIZM_GOTOPAGE, IDD_MIRANDADB, (LPARAM)MirandaPageProc);
			break;

		case IDCANCEL:
			if (IsDlgButtonChecked(hdlg, IDC_DONTLOADPLUGIN)) {
				char sModuleFileName[MAX_PATH];
				GetModuleFileNameA(g_plugin.getInst(), sModuleFileName, sizeof(sModuleFileName));
				char *pszFileName = strrchr(sModuleFileName, '\\');
				if (pszFileName == nullptr)
					pszFileName = sModuleFileName;
				else
					pszFileName++;

				// We must lower case here because if a DLL is loaded in two
				// processes, its file name from GetModuleFileName in one process may
				// differ in case from its file name in the other process. This will
				// prevent the plugin from disabling/enabling correctly (this fix relies
				// on the plugin loader to ignore case)
				CharLowerA(pszFileName);
				db_set_b(NULL, "PluginDisable", pszFileName, 1);
			}
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;
		}
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK WizardDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndPage = nullptr;
	bool bFirstLaunch = hwndPage == nullptr;

	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		Window_SetIcon_IcoLib(hdlg, GetIconHandle(IDI_IMPORT));
		g_hwndWizard = hdlg;
		{
			WizardDlgParam *param = (WizardDlgParam*)lParam;
			if (param)
				PostMessage(hdlg, WIZM_GOTOPAGE, param->wParam, param->lParam);
		}
		return TRUE;

	case WIZM_GOTOPAGE:
		if (hwndPage)
			DestroyWindow(hwndPage);
		EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
		EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
		EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
		SetDlgItemText(hdlg, IDCANCEL, TranslateT("Cancel"));
		hwndPage = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(wParam), hdlg, (DLGPROC)lParam);
		SetWindowPos(hwndPage, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		ShowWindow(hwndPage, SW_SHOW);
		if (bFirstLaunch)
			ShowWindow(hdlg, SW_SHOW);
		break;

	case WIZM_DISABLEBUTTON:
		switch (wParam) {
		case 0:
			EnableWindow(GetDlgItem(hdlg, IDC_BACK), FALSE);
			break;

		case 1:
			EnableWindow(GetDlgItem(hdlg, IDOK), FALSE);
			break;

		case 2:
			EnableWindow(GetDlgItem(hdlg, IDCANCEL), FALSE);
			break;
		}
		break;

	case WIZM_ENABLEBUTTON:
		switch (wParam) {
		case 0:
			EnableWindow(GetDlgItem(hdlg, IDC_BACK), TRUE);
			break;

		case 1:
			EnableWindow(GetDlgItem(hdlg, IDOK), TRUE);
			break;

		case 2:
			EnableWindow(GetDlgItem(hdlg, IDCANCEL), TRUE);
			break;
		}
		break;

	case WIZM_SETCANCELTEXT:
		SetDlgItemText(hdlg, IDCANCEL, (wchar_t*)lParam);
		break;

	case WM_COMMAND:
		SendMessage(hwndPage, WM_COMMAND, wParam, lParam);
		break;

	case WM_CLOSE:
		DestroyWindow(hwndPage);
		DestroyWindow(hdlg);
		break;

	case WM_DESTROY:
		g_hwndWizard = hwndPage = nullptr;
		if (g_bSendQuit)
			PostQuitMessage(0);
	}

	return FALSE;
}
