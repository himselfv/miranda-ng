/*
* A plugin for Miranda IM which displays web page text in a window 
* Copyright (C) 2005 Vincent Joyce.
*
* Miranda IM: the free icq client for MS Windows  Copyright (C) 2000-2
* Richard Hughes, Roland Rabien & Tristan Van de Vreede
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc., 59
* Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "webview.h"

const wchar_t *szTrackerBarDescr[] = {
	LPGENW("No whitespace removal"),
	LPGENW("Minimal level of whitespace removal"),
	LPGENW("Medium level of whitespace removal"),
	LPGENW("Large level of whitespace removal"),
	LPGENW("Remove all whitespace")
};

static char  *fontSizes[] = { "8", "10", "14", "16", "18", "20", "24", "28" };
static wchar_t *AlertTypes[] = { LPGENW("Popup plugin"), LPGENW("Log to file"), LPGENW("Open data display window"), LPGENW("Use OSD plugin") };
static wchar_t *EventTypes[] = { LPGENW("A string is present"), LPGENW("The web page changes"), LPGENW("A specific part of web page changes") };

#define M_FILLSCRIPTCOMBO    (WM_USER+16)

wchar_t* FixButtonText(wchar_t *url, size_t len)
{
	wchar_t buttontext[256], stringbefore[256], newbuttontext[256];
	wcsncpy_s(buttontext, url, _TRUNCATE);
	wcsncpy_s(newbuttontext, url, _TRUNCATE);

	if (wcschr(newbuttontext, '&') != nullptr) {
		while (true) {
			if (wcschr(newbuttontext, '&') == nullptr)
				break;

			wcsncpy_s(buttontext, newbuttontext, _TRUNCATE);
			wchar_t *stringafter = wcschr(buttontext, '&');
			int pos = (stringafter - buttontext);
			int posbefore = (stringafter - buttontext) - 1;
			int posafter = (stringafter - buttontext) + 1;
			strdelw(stringafter, 1);
			wcsncpy_s(stringbefore, pos, buttontext, _TRUNCATE);
			mir_snwprintf(newbuttontext, L"%s!!%s", stringbefore, stringafter);

			posafter = 0;
			posbefore = 0;
		}

		while (true) {
			if (wcschr(newbuttontext, '!') != nullptr) {
				wchar_t *stringafter = wcschr(newbuttontext, '!');
				int pos = (stringafter - newbuttontext);
				newbuttontext[pos] = '&';
			}
			if (wcschr(newbuttontext, '!') == nullptr)
				break;
		}
	}

	wcsncpy_s(url, len, newbuttontext, _TRUNCATE);
	return url;
}

/*****************************************************************************/
static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEX * lpelfe, NEWTEXTMETRICEX*, int, LPARAM lParam)
{
	if (SendMessage((HWND)lParam, CB_FINDSTRINGEXACT, -1, (LPARAM)lpelfe->elfScript) == CB_ERR) {
		int i = SendMessage((HWND)lParam, CB_ADDSTRING, 0, (LPARAM)lpelfe->elfScript);

		SendMessage((HWND)lParam, CB_SETITEMDATA, i, lpelfe->elfLogFont.lfCharSet);
	}
	return TRUE;
}

/*****************************************************************************/
// copied and modified from NewStatusNotify
INT_PTR CALLBACK DlgPopUpOpts(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	char str[512];
	DWORD  BGColour = 0;
	DWORD  TextColour = 0;
	DWORD  delay = 0;
	static int test = 0;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);

		delay = db_get_dw(NULL, MODULENAME, POP_DELAY_KEY, 0);

		// setting popup delay option
		_itoa(delay, str, 10);
		SetDlgItemTextA(hdlg, IDC_DELAY, str);

		BGColour = db_get_dw(NULL, MODULENAME, POP_BG_CLR_KEY, Def_color_bg);
		TextColour = db_get_dw(NULL, MODULENAME, POP_TXT_CLR_KEY, Def_color_txt);

		// Colours. First step is configuring the colours.
		SendDlgItemMessage(hdlg, IDC_POP_BGCOLOUR, CPM_SETCOLOUR, 0, BGColour);
		SendDlgItemMessage(hdlg, IDC_POP_TEXTCOLOUR, CPM_SETCOLOUR, 0, TextColour);
		// Second step is disabling them if we want to use default Windows
		// ones.
		CheckDlgButton(hdlg, IDC_POP_USEWINCOLORS, db_get_b(NULL, MODULENAME, POP_USEWINCLRS_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_POP_USESAMECOLORS, db_get_b(NULL, MODULENAME, POP_USESAMECLRS_KEY, 1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_POP_USECUSTCOLORS, db_get_b(NULL, MODULENAME, POP_USECUSTCLRS_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);

		if (IsDlgButtonChecked(hdlg, IDC_POP_USEWINCOLORS) || IsDlgButtonChecked(hdlg, IDC_POP_USESAMECOLORS)) {
			EnableWindow(GetDlgItem(hdlg, IDC_POP_BGCOLOUR), 0);
			EnableWindow(GetDlgItem(hdlg, IDC_POP_TEXTCOLOUR), 0);
		}

		CheckDlgButton(hdlg, IDC_LCLK_WINDOW, db_get_b(NULL, MODULENAME, LCLK_WINDOW_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_LCLK_WEB_PGE, db_get_b(NULL, MODULENAME, LCLK_WEB_PGE_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_LCLK_DISMISS, db_get_b(NULL, MODULENAME, LCLK_DISMISS_KEY, 1) ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hdlg, IDC_RCLK_WINDOW, db_get_b(NULL, MODULENAME, RCLK_WINDOW_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_RCLK_WEB_PGE, db_get_b(NULL, MODULENAME, RCLK_WEB_PGE_KEY, 1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hdlg, IDC_RCLK_DISMISS, db_get_b(NULL, MODULENAME, RCLK_DISMISS_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;

	case WM_COMMAND:
		// enable the "apply" button 
		if (HIWORD(wParam) == BN_CLICKED && GetFocus() == (HWND)lParam)
			SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
		// These are simple clicks: we don't save, but we tell the Options Page 
		// to enable the "Apply" button.
		switch (LOWORD(wParam)) {
		case IDC_POP_BGCOLOUR: // Fall through
		case IDC_POP_TEXTCOLOUR:
			// select new colors
			if (HIWORD(wParam) == CPN_COLOURCHANGED)
				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_POP_USESAMECOLORS:
			// use same color as data window - enable/disable color selection
			// controls
			EnableWindow(GetDlgItem(hdlg, IDC_POP_BGCOLOUR), !((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USESAMECOLORS)));
			EnableWindow(GetDlgItem(hdlg, IDC_POP_TEXTCOLOUR), !((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USESAMECOLORS)));
			SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_POP_USEWINCOLORS:
			// use window color - enable/disable color selection controls
			EnableWindow(GetDlgItem(hdlg, IDC_POP_BGCOLOUR), !((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USEWINCOLORS)));
			EnableWindow(GetDlgItem(hdlg, IDC_POP_TEXTCOLOUR), !((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USEWINCOLORS)));
			SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_POP_USECUSTCOLORS:
			EnableWindow(GetDlgItem(hdlg, IDC_POP_BGCOLOUR), ((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USECUSTCOLORS)));
			EnableWindow(GetDlgItem(hdlg, IDC_POP_TEXTCOLOUR), ((BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USECUSTCOLORS)));
			break;

		case IDC_PD1:
			SetDlgItemText(hdlg, IDC_DELAY, L"0");
			break;
		case IDC_PD2:
			// Popup delay = permanent
			SetDlgItemText(hdlg, IDC_DELAY, L"-1");
			break;

		case IDC_DELAY:
			if (HIWORD(wParam) == EN_CHANGE)
				test++;
			if (test > 1) {
				//CheckRadioButton(hdlg, IDC_PD1, IDC_PD3, IDC_PD3);
				SendMessage(GetParent(hdlg), PSM_CHANGED, 0, 0);
			}
			break;

		case IDC_PREVIEW:
			wchar_t str3[512];
			POPUPDATAT ppd = { 0 };

			GetDlgItemText(hdlg, IDC_DELAY, str3, _countof(str3));

			if (IsDlgButtonChecked(hdlg, IDC_POP_USECUSTCOLORS)) {
				BGColour = (SendDlgItemMessage(hdlg, IDC_POP_BGCOLOUR, CPM_GETCOLOUR, 0, 0));
				TextColour = (SendDlgItemMessage(hdlg, IDC_POP_TEXTCOLOUR, CPM_GETCOLOUR, 0, 0));
			}
			if (IsDlgButtonChecked(hdlg, IDC_POP_USEWINCOLORS)) {
				BGColour = GetSysColor(COLOR_BTNFACE);
				TextColour = GetSysColor(COLOR_WINDOWTEXT);
			}
			if (IsDlgButtonChecked(hdlg, IDC_POP_USESAMECOLORS)) {
				BGColour = BackgoundClr;
				TextColour = TextClr;
			}
			ppd.lchContact = NULL;
			mir_wstrcpy(ppd.lptzContactName, _A2W(MODULENAME));
			ppd.lchIcon = LoadIcon(g_plugin.getInst(), MAKEINTRESOURCE(IDI_SITE));
			mir_wstrcpy(ppd.lptzText, TranslateT("This is a preview popup."));
			ppd.colorBack = BGColour;
			ppd.colorText = TextColour;
			ppd.PluginWindowProc = nullptr;
			ppd.iSeconds = _wtol(str3);
			// display popups
			PUAddPopupT(&ppd);
		}
		break;

	case WM_NOTIFY: // Here we have pressed either the OK or the APPLY button.
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			int popupdelayval = 0;
			wchar_t str2[512];
			GetDlgItemText(hdlg, IDC_DELAY, str2, _countof(str2));

			popupdelayval = _wtol(str2);
			db_set_dw(NULL, MODULENAME, POP_DELAY_KEY, popupdelayval);

			db_set_b(NULL, MODULENAME, LCLK_WINDOW_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_LCLK_WINDOW));
			db_set_b(NULL, MODULENAME, LCLK_WEB_PGE_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_LCLK_WEB_PGE));
			db_set_b(NULL, MODULENAME, LCLK_DISMISS_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_LCLK_DISMISS));

			db_set_b(NULL, MODULENAME, RCLK_WINDOW_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_RCLK_WINDOW));
			db_set_b(NULL, MODULENAME, RCLK_WEB_PGE_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_RCLK_WEB_PGE));
			db_set_b(NULL, MODULENAME, RCLK_DISMISS_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_RCLK_DISMISS));

			db_set_b(NULL, MODULENAME, POP_USECUSTCLRS_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USECUSTCOLORS));
			db_set_b(NULL, MODULENAME, POP_USEWINCLRS_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USEWINCOLORS));
			db_set_b(NULL, MODULENAME, POP_USESAMECLRS_KEY, (BYTE)IsDlgButtonChecked(hdlg, IDC_POP_USESAMECOLORS));

			BGColour = (SendDlgItemMessage(hdlg, IDC_POP_BGCOLOUR, CPM_GETCOLOUR, 0, 0));
			TextColour = (SendDlgItemMessage(hdlg, IDC_POP_TEXTCOLOUR, CPM_GETCOLOUR, 0, 0));

			db_set_dw(NULL, MODULENAME, POP_BG_CLR_KEY, BGColour);
			db_set_dw(NULL, MODULENAME, POP_TXT_CLR_KEY, TextColour);

			test = 0;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/*****************************************************************************/
INT_PTR CALLBACK DlgProcAlertOpt(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND ParentHwnd;
	DBVARIANT dbv;
	int i;
	int alertIndex = 0;
	int eventIndex = 0;
	static int test;
	MCONTACT hContact;

	ParentHwnd = GetParent(hwndDlg);

	switch (msg) {
	case WM_INITDIALOG:
		test = 0;
		TranslateDialogDefault(hwndDlg);
		hContact = lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

		SetWindowText(hwndDlg, TranslateT("Alert options"));

		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(g_plugin.getInst(), MAKEINTRESOURCE(IDI_ALERT)));

		EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 0);

		if (!db_get_ws(hContact, MODULENAME, ALERT_STRING_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_ALERT_STRING, dbv.pwszVal);
			db_free(&dbv);
		}
		if (!db_get_ws(hContact, MODULENAME, ALRT_S_STRING_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_START2, dbv.pwszVal);
			db_free(&dbv);
		}
		if (!db_get_ws(hContact, MODULENAME, ALRT_E_STRING_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_END2, dbv.pwszVal);
			db_free(&dbv);
		}
		CheckDlgButton(hwndDlg, IDC_ENABLE_ALERTS, db_get_b(hContact, MODULENAME, ENABLE_ALERTS_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_ADD_DATE_NAME, db_get_b(hContact, MODULENAME, APND_DATE_NAME_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_24_HOUR, db_get_b(hContact, MODULENAME, USE_24_HOUR_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_ALWAYS_LOG, db_get_b(hContact, MODULENAME, ALWAYS_LOG_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemText(hwndDlg, IDC_ALERT_TYPE, TranslateW(AlertTypes[db_get_b(hContact, MODULENAME, ALRT_INDEX_KEY, 0)]));
		SetDlgItemText(hwndDlg, IDC_EVENT_TYPE, TranslateW(EventTypes[db_get_b(hContact, MODULENAME, EVNT_INDEX_KEY, 0)]));

		for (i = 0; i < _countof(AlertTypes); i++)
			SendDlgItemMessage(hwndDlg, IDC_ALERT_TYPE, CB_ADDSTRING, 0, (LPARAM)TranslateW(AlertTypes[i]));

		for (i = 0; i < _countof(EventTypes); i++)
			SendDlgItemMessage(hwndDlg, IDC_EVENT_TYPE, CB_ADDSTRING, 0, (LPARAM)TranslateW(EventTypes[i]));

		if (db_get_b(hContact, MODULENAME, ENABLE_ALERTS_KEY, 0)) {
			CheckDlgButton(hwndDlg, IDC_ENABLE_ALERTS, BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ADD_DATE_NAME, BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_24_HOUR, BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_PREFIX, BST_UNCHECKED);
		}
		EnableWindow(GetDlgItem(hwndDlg, IDC_EVENT_TYPE), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_DATE_NAME), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));

		if (IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME)) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_PREFIX), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SUFFIX), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_24_HOUR), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		}
		else {
			EnableWindow(GetDlgItem(hwndDlg, IDC_PREFIX), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SUFFIX), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_24_HOUR), 0);
		}

		EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_TYPE), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_START2), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_END2), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ALWAYS_LOG), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));

		if (!db_get_s(hContact, MODULENAME, EVNT_INDEX_KEY, &dbv)) {
			eventIndex = db_get_b(hContact, MODULENAME, EVNT_INDEX_KEY, 0);
			db_free(&dbv);
		}
		if (!db_get_s(hContact, MODULENAME, ALRT_INDEX_KEY, &dbv)) {
			alertIndex = db_get_b(hContact, MODULENAME, ALRT_INDEX_KEY, 0);
			db_free(&dbv);
		}

		// alerts
		if (alertIndex == 0) // Popup
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
			}
		}
		else if (alertIndex == 1) // file
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 1);
			}
		}
		else if (alertIndex == 2) // datawindow
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
			}
		}
		else if (alertIndex == 3) // osd
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
			}
		}

		// events
		if (eventIndex == 0) // string is present
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 1);
			}
		}
		else if (eventIndex == 1) // webpage changed
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
			}
		}
		else if (eventIndex == 2) // part of page changed
		{
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
			}
		}

		if (!db_get_ws(hContact, MODULENAME, FILE_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_FILENAME, dbv.pwszVal);
			db_free(&dbv);
		}

		CheckDlgButton(hwndDlg, IDC_APPEND, db_get_b(hContact, MODULENAME, APPEND_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SAVE_AS_RAW, db_get_b(hContact, MODULENAME, SAVE_AS_RAW_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);

		if (db_get_b(hContact, MODULENAME, CONTACT_PREFIX_KEY, 1) == 1)
			CheckRadioButton(hwndDlg, IDC_PREFIX, IDC_SUFFIX, IDC_PREFIX);
		else
			CheckRadioButton(hwndDlg, IDC_PREFIX, IDC_SUFFIX, IDC_SUFFIX);

		if (db_get_b(hContact, MODULENAME, ALWAYS_LOG_KEY, 0)) {
			if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 1);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BROWSE:
			{
				wchar_t szFileName[MAX_PATH];
				GetDlgItemText(hwndDlg, IDC_FILENAME, szFileName, _countof(szFileName));

				OPENFILENAME ofn = { 0 };
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hwndDlg;
				ofn.hInstance = nullptr;
				ofn.lpstrFilter = L"TEXT Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
				ofn.lpstrFile = szFileName;
				ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.nMaxFile = _countof(szFileName);
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrDefExt = L"txt";
				if (!GetSaveFileName(&ofn))
					break;

				SetDlgItemText(hwndDlg, IDC_FILENAME, szFileName);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			}
			break;

		case IDC_ADD_DATE_NAME:
			EnableWindow(GetDlgItem(hwndDlg, IDC_PREFIX), (IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SUFFIX), (IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_24_HOUR), (IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_24_HOUR:
		case IDC_SUFFIX:
		case IDC_PREFIX:
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_ALERT_STRING:
			if (HIWORD(wParam) == EN_CHANGE)
				test++;
			if (test > 1)
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_START2:
		case IDC_END2:
			if (HIWORD(wParam) == EN_CHANGE)
				test++;
			if (test > 3)
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_APPEND:
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_SAVE_AS_RAW:
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			break;

		case IDC_ENABLE_ALERTS:
			hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD_DATE_NAME), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));

			if (IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_PREFIX), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_SUFFIX), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_24_HOUR), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_PREFIX), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SUFFIX), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_24_HOUR), 0);
			}

			EnableWindow(GetDlgItem(hwndDlg, IDC_EVENT_TYPE), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_TYPE), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALWAYS_LOG), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));

			eventIndex = db_get_b(hContact, MODULENAME, EVNT_INDEX_KEY, 0);
			alertIndex = db_get_b(hContact, MODULENAME, ALRT_INDEX_KEY, 0);

			if (eventIndex == 2) { // part of webpage changed
				EnableWindow(GetDlgItem(hwndDlg, IDC_START2), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_END2), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 0);
			}

			// //////// 
			if (alertIndex == 0) { // popup
				if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);

					if (eventIndex == 2) // part of webpage changed
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
					if (eventIndex == 1) // webpage changed
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
					if (eventIndex == 0) // string present
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				}
			}
			else if (alertIndex == 1) { // log to file
				if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 1);

					if (eventIndex == 1) // webpage changed
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
					if (eventIndex == 0) // string present
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				}
				else {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
				}
			}
			else if (alertIndex == 2) { // display window
				if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);

					if (eventIndex == 1) // webpage changed
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
					if (eventIndex == 0) // string present
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				}
			}
			else if (alertIndex == 3) { // osd
				if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);

					if (eventIndex == 1) // webpage changed
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
					if (eventIndex == 0) // string present
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));
				}
			}

			if (eventIndex == 0) // string present
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)));

			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);

			if (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)) {
				if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 1);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 1);
				}
				else {
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
				}
			}
			break;

		case IDC_ALWAYS_LOG:
			EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
			break;

		case IDC_ALERT_TYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE) {
				hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
				alertIndex = SendDlgItemMessage(hwndDlg, IDC_ALERT_TYPE, CB_GETCURSEL, 0, 0);

				if (HIWORD(wParam) == CBN_SELCHANGE) {
					db_set_b(hContact, MODULENAME, ALRT_INDEX_KEY, alertIndex);
					if (alertIndex == 0) {
						// PopUp
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
					}
					else if (alertIndex == 1) {
						// log to file
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 1);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 1);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 1);
					}
					else if (alertIndex == 2) {
						// data window
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
					}
					else if (alertIndex == 3) {
						// OSD
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), 0);
					}

					if (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_APPEND), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
						EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
						EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVE_AS_RAW), (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG)));
					}
				}
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			}
			break;

		case IDC_EVENT_TYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE) {
				hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
				eventIndex = SendDlgItemMessage(hwndDlg, IDC_EVENT_TYPE, CB_GETCURSEL, 0, 0);

				if (HIWORD(wParam) == CBN_SELCHANGE) {
					db_set_b(hContact, MODULENAME, EVNT_INDEX_KEY, eventIndex);
					if (eventIndex == 0) {
						// event when string is present
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 1);
						EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 0);
					}
					else if (eventIndex == 1) {
						// event when web page changes
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 0);
					}
					else if (eventIndex == 2) {
						// event when part of web page changes
						EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_STRING), 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_START2), 1);
						EnableWindow(GetDlgItem(hwndDlg, IDC_END2), 1);
					}
				}
				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 1);
			}
			break;

		case IDC_ALERT_APPLY:
		case IDC_OK2:
			{
				hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

				eventIndex = db_get_b(hContact, MODULENAME, EVNT_INDEX_KEY, 0);
				alertIndex = db_get_b(hContact, MODULENAME, ALRT_INDEX_KEY, 0);

				db_set_b(hContact, MODULENAME, ENABLE_ALERTS_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS));
				db_set_b(hContact, MODULENAME, APND_DATE_NAME_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ADD_DATE_NAME));
				db_set_b(hContact, MODULENAME, USE_24_HOUR_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_24_HOUR));
				db_set_b(hContact, MODULENAME, ALWAYS_LOG_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG));

				//if alerts is unticked delete the cache
				if (!(IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)))
					SiteDeleted(hContact, 0);

				if (eventIndex == 0) // string present
					if (!(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ALERT_STRING))))
						if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
							MessageBox(nullptr, TranslateT("You need to supply a search string."), _A2W(MODULENAME), MB_OK);
							break;
						}

				if (eventIndex == 2) // part of web page changed
					if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
						if (!(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_START2)))) {
							MessageBox(nullptr, TranslateT("You need to supply a start string."), _A2W(MODULENAME), MB_OK);
							break;
						}
						if (!(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_END2)))) {
							MessageBox(nullptr, TranslateT("You need to supply an end string."), _A2W(MODULENAME), MB_OK);
							break;
						}
					}

				if (alertIndex == 1) // log to file
					if (!(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_FILENAME))))
						if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
							MessageBox(nullptr, TranslateT("You need to supply a file name and path."), _A2W(MODULENAME), MB_OK);
							break;
						}

				if (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_LOG))
					if (!(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_FILENAME))))
						if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_ALERTS)) {
							MessageBox(nullptr, TranslateT("You need to supply a file name and path."), _A2W(MODULENAME), MB_OK);
							break;
						}

				wchar_t buf[MAX_PATH];
				GetDlgItemText(hwndDlg, IDC_FILENAME, buf, _countof(buf));
				db_set_ws(hContact, MODULENAME, FILE_KEY, buf);

				db_set_b(hContact, MODULENAME, APPEND_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_APPEND));
				db_set_b(hContact, MODULENAME, SAVE_AS_RAW_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_SAVE_AS_RAW));

				GetDlgItemText(hwndDlg, IDC_ALERT_STRING, buf, _countof(buf));
				db_set_ws(hContact, MODULENAME, ALERT_STRING_KEY, buf);

				GetDlgItemText(hwndDlg, IDC_START2, buf, _countof(buf));
				db_set_ws(hContact, MODULENAME, ALRT_S_STRING_KEY, buf);

				GetDlgItemText(hwndDlg, IDC_END2, buf, _countof(buf));
				db_set_ws(hContact, MODULENAME, ALRT_E_STRING_KEY, buf);

				db_set_b(hContact, MODULENAME, CONTACT_PREFIX_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_PREFIX));

				EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_APPLY), 0);

				if (LOWORD(wParam) == IDC_OK2)
					EndDialog(hwndDlg, 1);
			}
			break;

		case IDC_ALERT_CANCEL:
		case IDC_CANCEL:
			EndDialog(hwndDlg, 0);
		}
	}
	return FALSE;
}

/*****************************************************************************/

INT_PTR CALLBACK DlgProcContactOpt(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;
	wchar_t url[300];
	HWND ParentHwnd = GetParent(hwndDlg);
	static int test;
	static int test2;
	MCONTACT hContact;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		hContact = (MCONTACT)lParam;

		test = 0;
		test2 = 0;

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)hContact);

		SetWindowText(hwndDlg, TranslateT("Contact options"));

		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(g_plugin.getInst(), MAKEINTRESOURCE(IDI_OPTIONS)));

		EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 0);

		if (!db_get_ws(hContact, MODULENAME, URL_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_URL, dbv.pwszVal);
			db_free(&dbv);
		}
		if (!db_get_ws(hContact, MODULENAME, START_STRING_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_START, dbv.pwszVal);
			db_free(&dbv);
		}
		if (!db_get_ws(hContact, MODULENAME, END_STRING_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_END, dbv.pwszVal);
			db_free(&dbv);
		}
		if (db_get_ws(hContact, MODULENAME, PRESERVE_NAME_KEY, &dbv)) {
			db_free(&dbv);
			db_get_ws(hContact, "CList", "MyHandle", &dbv);
			db_set_ws(hContact, MODULENAME, PRESERVE_NAME_KEY, dbv.pwszVal);
			db_free(&dbv);
		}
		if (!db_get_ws(hContact, MODULENAME, PRESERVE_NAME_KEY, &dbv)) {
			SetDlgItemText(hwndDlg, IDC_SITE_NAME, dbv.pwszVal);
			db_free(&dbv);
		}

		CheckDlgButton(hwndDlg, IDC_CLEAN, db_get_b(hContact, MODULENAME, CLEAR_DISPLAY_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);

		SendDlgItemMessage(hwndDlg, IDC_RWSPACE, TBM_SETRANGE, FALSE, MAKELONG(0, 4));
		SendDlgItemMessage(hwndDlg, IDC_RWSPACE, TBM_SETPOS, TRUE, db_get_b(hContact, MODULENAME, RWSPACE_KEY, 0));
		SetDlgItemText(hwndDlg, IDC_RWSPC_TEXT, TranslateW(szTrackerBarDescr[SendDlgItemMessage(hwndDlg, IDC_RWSPACE, TBM_GETPOS, 0, 0)]));

		EnableWindow(GetDlgItem(hwndDlg, IDC_RWSPACE), (IsDlgButtonChecked(hwndDlg, IDC_CLEAN)));
		EnableWindow(GetDlgItem(hwndDlg, IDC_RWSPC_TEXT), (IsDlgButtonChecked(hwndDlg, IDC_CLEAN)));

		if (db_get_b(hContact, MODULENAME, DBLE_WIN_KEY, 1) == 1)
			CheckRadioButton(hwndDlg, IDC_DBLE_WEB, IDC_DBLE_WIN, IDC_DBLE_WIN);
		else
			CheckRadioButton(hwndDlg, IDC_DBLE_WEB, IDC_DBLE_WIN, IDC_DBLE_WEB);

		if (db_get_b(hContact, MODULENAME, U_ALLSITE_KEY, 0) == 1) {
			CheckRadioButton(hwndDlg, IDC_U_SE_STRINGS, IDC_U_ALLSITE, IDC_U_ALLSITE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_START), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_END), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CPY_STRINGS), 0);
		}
		else {
			CheckRadioButton(hwndDlg, IDC_U_SE_STRINGS, IDC_U_ALLSITE, IDC_U_SE_STRINGS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_START), 1);
			EnableWindow(GetDlgItem(hwndDlg, IDC_END), 1);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CPY_STRINGS), 1);
			//EnableWindow(GetDlgItem(hwndDlg, IDC_CPY_STR_TXT), 1);
		}
		break;

	case WM_HSCROLL:
		SetDlgItemText(hwndDlg, IDC_RWSPC_TEXT, TranslateW(szTrackerBarDescr[SendDlgItemMessage(hwndDlg, IDC_RWSPACE, TBM_GETPOS, 0, 0)]));
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case WM_CLOSE:
		case IDCANCEL:
			if (hwndDlg) {
				DestroyWindow(hwndDlg);
				hwndDlg = nullptr;
			}
			return TRUE;

		case IDC_SITE_NAME:
		case IDC_URL:
			if (HIWORD(wParam) == EN_CHANGE)
				test++;
			if (test > 2)
				EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_DBLE_WEB:
		case IDC_DBLE_WIN:
			EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_START:
		case IDC_END:
			if (HIWORD(wParam) == EN_CHANGE)
				test2++;
			if (test2 > 2)
				EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_CPY_STRINGS:
			{
				wchar_t string[128];
				hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

				GetDlgItemText(hwndDlg, IDC_START, string, _countof(string));
				db_set_ws(hContact, MODULENAME, ALRT_S_STRING_KEY, string);

				GetDlgItemText(hwndDlg, IDC_END, string, _countof(string));
				db_set_ws(hContact, MODULENAME, ALRT_E_STRING_KEY, string);

				db_set_w(hContact, MODULENAME, EVNT_INDEX_KEY, 2);
			}
			break;

		case IDC_CLEAN:
			EnableWindow(GetDlgItem(hwndDlg, IDC_RWSPACE), (IsDlgButtonChecked(hwndDlg, IDC_CLEAN)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_RWSPC_TEXT), (IsDlgButtonChecked(hwndDlg, IDC_CLEAN)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_U_SE_STRINGS:
			EnableWindow(GetDlgItem(hwndDlg, IDC_START), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_END), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_CPY_STRINGS), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_U_ALLSITE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_START), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_END), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_CPY_STRINGS), (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 1);
			break;

		case IDC_OPT_APPLY:
		case IDOK:
			{
				wchar_t str[128], contactname[128];
				if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_URL))) {
					MessageBox(nullptr, TranslateT("You need to supply a URL."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_START))) {
					if (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)) {
						MessageBox(nullptr, TranslateT("You need to supply a start string."), _A2W(MODULENAME), MB_OK);
						break;
					}
				}
				if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_END))) {
					if (IsDlgButtonChecked(hwndDlg, IDC_U_SE_STRINGS)) {
						MessageBox(nullptr, TranslateT("You need to supply an end string."), _A2W(MODULENAME), MB_OK);
						break;
					}
				}
				if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_SITE_NAME))) {
					MessageBox(nullptr, TranslateT("You need to supply a name for the contact."), _A2W(MODULENAME), MB_OK);
					break;
				}

				GetDlgItemText(hwndDlg, IDC_SITE_NAME, contactname, _countof(contactname));
				if (wcschr(contactname, '\\') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '/') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, ':') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '*') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '?') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '\"') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '<') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '>') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}
				if (wcschr(contactname, '|') != nullptr) {
					MessageBox(nullptr, TranslateT("Invalid symbol present in contact name."), _A2W(MODULENAME), MB_OK);
					break;
				}

				hContact = (MCONTACT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

				GetDlgItemText(hwndDlg, IDC_URL, url, _countof(url));
				db_set_ws(hContact, MODULENAME, URL_KEY, url);
				db_set_ws(hContact, MODULENAME, "Homepage", url);

				GetDlgItemText(hwndDlg, IDC_START, str, _countof(str));
				db_set_ws(hContact, MODULENAME, START_STRING_KEY, str);

				GetDlgItemText(hwndDlg, IDC_END, str, _countof(str));
				db_set_ws(hContact, MODULENAME, END_STRING_KEY, str);

				GetDlgItemText(hwndDlg, IDC_SITE_NAME, str, _countof(str));
				db_set_ws(hContact, "CList", "MyHandle", str);

				db_set_b(hContact, MODULENAME, DBLE_WIN_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_DBLE_WIN));
				db_set_b(hContact, MODULENAME, U_ALLSITE_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_U_ALLSITE));

				db_set_b(hContact, MODULENAME, CLEAR_DISPLAY_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_CLEAN));
				db_set_b(hContact, MODULENAME, RWSPACE_KEY, (BYTE)(SendDlgItemMessage(hwndDlg, IDC_RWSPACE, TBM_GETPOS, 0, 0)));

				SetDlgItemText(ParentHwnd, IDC_OPEN_URL, FixButtonText(url, _countof(url)));

				SetWindowText(ParentHwnd, str);
				EnableWindow(GetDlgItem(hwndDlg, IDC_OPT_APPLY), 0);

				if (LOWORD(wParam) == IDOK)
					EndDialog(hwndDlg, 1);
			}
			break;

		case IDC_OPT_CANCEL:
			EndDialog(hwndDlg, 0);
		}
		break;
	}
	return FALSE;
}

/*****************************************************************************/
INT_PTR CALLBACK DlgProcOpt(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD  timerval, delaytime, oldcolor;
	DBVARIANT       dbv;
	static int test = 0;
	static int test2 = 0;

	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			test = 0;

			SendDlgItemMessage(hwndDlg, IDC_SPIN1, UDM_SETRANGE, 0, MAKELONG(999, 0));
			SendDlgItemMessage(hwndDlg, IDC_SPIN2, UDM_SETRANGE, 0, MAKELONG(120, 0));

			SetDlgItemInt(hwndDlg, IDC_TIME, db_get_dw(NULL, MODULENAME, REFRESH_KEY, TIME), FALSE);
			SetDlgItemInt(hwndDlg, IDC_START_DELAY, db_get_w(NULL, MODULENAME, START_DELAY_KEY, 0), FALSE);

			mir_forkthread(FillFontListThread, hwndDlg);

			CheckDlgButton(hwndDlg, IDC_DISABLEMENU, db_get_b(NULL, MODULENAME, MENU_OFF, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SUPPRESS, db_get_b(NULL, MODULENAME, SUPPRESS_ERR_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_UPDATE_ONSTART, db_get_b(NULL, MODULENAME, UPDATE_ONSTART_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_UPDATE_ON_OPEN, db_get_b(NULL, MODULENAME, UPDATE_ON_OPEN_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_HIDE_STATUS_ICON, db_get_b(NULL, MODULENAME, HIDE_STATUS_ICON_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FONT_BOLD, db_get_b(NULL, MODULENAME, FONT_BOLD_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FONT_ITALIC, db_get_b(NULL, MODULENAME, FONT_ITALIC_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FONT_UNDERLINE, db_get_b(NULL, MODULENAME, FONT_UNDERLINE_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ERROR_POPUP, db_get_b(NULL, MODULENAME, ERROR_POPUP_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_UPDATE_ONALERT, db_get_b(NULL, MODULENAME, UPDATE_ONALERT_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SAVE_INDIVID_POS, db_get_b(NULL, MODULENAME, SAVE_INDIVID_POS_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_NO_PROTECT, db_get_b(NULL, MODULENAME, NO_PROTECT_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_DATAPOPUP, db_get_b(NULL, MODULENAME, DATA_POPUP_KEY, 0) ? BST_CHECKED : BST_UNCHECKED);

			if (!db_get_ws(NULL, MODULENAME, FONT_FACE_KEY, &dbv)) {
				SetDlgItemText(hwndDlg, IDC_TYPEFACE, dbv.pwszVal);
				db_free(&dbv);
			}
			else SetDlgItemText(hwndDlg, IDC_TYPEFACE, Def_font_face);

			for (int i = 0; i < _countof(fontSizes); i++)
				SendDlgItemMessage(hwndDlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)fontSizes[i]);

			SendMessage(hwndDlg, M_FILLSCRIPTCOMBO, wParam, 0);

			SetDlgItemInt(hwndDlg, IDC_FONTSIZE, db_get_b(NULL, MODULENAME, FONT_SIZE_KEY, 14), FALSE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_FIND_BUTTON), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALERT_BUTTON), 0);

			SendDlgItemMessage(hwndDlg, IDC_BGCOLOR, CPM_SETCOLOUR, 0, (BackgoundClr));
			SendDlgItemMessage(hwndDlg, IDC_TXTCOLOR, CPM_SETCOLOUR, 0, (TextClr));

			/*
			* record bg value for later comparison
			*/
			oldcolor = BackgoundClr;

			if (db_get_b(NULL, MODULENAME, SUPPRESS_ERR_KEY, 0)) {
				CheckDlgButton(hwndDlg, IDC_SUPPRESS, BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ERROR_POPUP), 0);
			}
			else {
				CheckDlgButton(hwndDlg, IDC_SUPPRESS, BST_UNCHECKED);
				if ((ServiceExists(MS_POPUP_ADDPOPUPT) != 0))
					EnableWindow(GetDlgItem(hwndDlg, IDC_ERROR_POPUP), 1);
			}

			if (ServiceExists(MS_POPUP_ADDPOPUPT) == 0)
				EnableWindow(GetDlgItem(hwndDlg, IDC_ERROR_POPUP), 0);

			if (db_get_b(NULL, MODULENAME, UPDATE_ONSTART_KEY, 0)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START_DELAY), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STARTDELAYTXT), 1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STDELAYSECTXT), 1);
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_START_DELAY), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STARTDELAYTXT), 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STDELAYSECTXT), 0);
			}
		}
		break;

	case M_FILLSCRIPTCOMBO: // fill the script combo box and set the
		// selection to the value for fontid wParam

		{
			LOGFONT         lf = { 0 };
			int i;
			HDC hdc = GetDC(hwndDlg);

			lf.lfCharSet = DEFAULT_CHARSET;
			GetDlgItemText(hwndDlg, IDC_TYPEFACE, lf.lfFaceName, _countof(lf.lfFaceName));
			lf.lfPitchAndFamily = 0;
			SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_RESETCONTENT, 0, 0);
			EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)EnumFontScriptsProc, (LPARAM)GetDlgItem(hwndDlg, IDC_SCRIPT), 0);
			ReleaseDC(hwndDlg, hdc);
			for (i = SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETCOUNT, 0, 0) - 1; i >= 0; i--) {
				if (SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETITEMDATA, i, 0) == (BYTE)((db_get_b(NULL, MODULENAME, FONT_SCRIPT_KEY, 0)))) {
					SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_SETCURSEL, i, 0);
					break;
				}
			}
			if (i < 0)
				SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_SETCURSEL, 0, 0);
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED && GetFocus() == (HWND)lParam)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

		switch (LOWORD(wParam)) {
		case IDC_TXTCOLOR:
			TextClr = SendDlgItemMessage(hwndDlg, IDC_TXTCOLOR, CPM_GETCOLOUR, 0, 0);
			db_set_dw(NULL, MODULENAME, TXT_COLOR_KEY, TextClr);
			if (HIWORD(wParam) == CPN_COLOURCHANGED) {
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				TxtclrLoop();
			}
			break;

		case IDC_BGCOLOR:
			BackgoundClr = SendDlgItemMessage(hwndDlg, IDC_BGCOLOR, CPM_GETCOLOUR, 0, 0);
			db_set_dw(NULL, MODULENAME, BG_COLOR_KEY, BackgoundClr);
			if (HIWORD(wParam) == CPN_COLOURCHANGED) {
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				BGclrLoop();
			}
			break;

		case IDC_HIDE_STATUS_ICON:
			ShowWindow(GetDlgItem(hwndDlg, IDC_RESTART), SW_SHOW);
			break;

		case IDC_SUPPRESS:
			if ((ServiceExists(MS_POPUP_ADDPOPUPT) != 0))
				EnableWindow(GetDlgItem(hwndDlg, IDC_ERROR_POPUP), (!(IsDlgButtonChecked(hwndDlg, IDC_SUPPRESS))));
			break;

		case IDC_UPDATE_ONSTART:
			EnableWindow(GetDlgItem(hwndDlg, IDC_START_DELAY), ((IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONSTART))));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SPIN2), ((IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONSTART))));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STARTDELAYTXT), ((IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONSTART))));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STDELAYSECTXT), ((IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONSTART))));
			break;

		case IDC_DISABLEMENU:
			ShowWindow(GetDlgItem(hwndDlg, IDC_RESTART), SW_SHOW);
			break;

		case IDC_TYPEFACE:
			SendMessage(hwndDlg, M_FILLSCRIPTCOMBO, wParam, 0);
		case IDC_FONTSIZE:
		case IDC_SCRIPT:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_START_DELAY:
			if (HIWORD(wParam) == EN_CHANGE)
				test++;
			if (test > 1)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case IDC_TIME:
			if (HIWORD(wParam) == EN_CHANGE)
				test2++;
			if (test2 > 2)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		} // end WM_COMMAND
		break;

	case WM_DESTROY:
		test = 0;
		test2 = 0;
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			db_set_b(NULL, MODULENAME, MENU_OFF, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_DISABLEMENU));
			db_set_b(NULL, MODULENAME, SUPPRESS_ERR_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_SUPPRESS));
			db_set_b(NULL, MODULENAME, UPDATE_ONSTART_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONSTART));
			db_set_b(NULL, MODULENAME, UPDATE_ON_OPEN_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ON_OPEN));
			db_set_b(NULL, MODULENAME, HIDE_STATUS_ICON_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_HIDE_STATUS_ICON));
			db_set_b(NULL, MODULENAME, FONT_BOLD_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_FONT_BOLD));
			db_set_b(NULL, MODULENAME, FONT_ITALIC_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_FONT_ITALIC));
			db_set_b(NULL, MODULENAME, FONT_UNDERLINE_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_FONT_UNDERLINE));
			db_set_b(NULL, MODULENAME, UPDATE_ONALERT_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_UPDATE_ONALERT));
			db_set_b(NULL, MODULENAME, SAVE_INDIVID_POS_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_SAVE_INDIVID_POS));
			db_set_b(NULL, MODULENAME, NO_PROTECT_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_NO_PROTECT));
			db_set_b(NULL, MODULENAME, DATA_POPUP_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_DATAPOPUP));

			wchar_t str[100];
			GetDlgItemText(hwndDlg, IDC_TYPEFACE, str, _countof(str));
			db_set_ws(NULL, MODULENAME, FONT_FACE_KEY, str);

			db_set_b(NULL, MODULENAME, FONT_SIZE_KEY, (GetDlgItemInt(hwndDlg, IDC_FONTSIZE, nullptr, FALSE)));
			db_set_b(NULL, MODULENAME, FONT_SCRIPT_KEY, ((BYTE)SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETCURSEL, 0, 0), 0)));

			db_set_b(NULL, MODULENAME, ERROR_POPUP_KEY, (BYTE)IsDlgButtonChecked(hwndDlg, IDC_ERROR_POPUP));

			timerval = GetDlgItemInt(hwndDlg, IDC_TIME, nullptr, FALSE);
			db_set_dw(NULL, MODULENAME, REFRESH_KEY, timerval);
			db_set_dw(NULL, MODULENAME, COUNTDOWN_KEY, timerval);


			delaytime = GetDlgItemInt(hwndDlg, IDC_START_DELAY, nullptr, FALSE);
			db_set_dw(NULL, MODULENAME, START_DELAY_KEY, delaytime);

			BackgoundClr = (SendDlgItemMessage(hwndDlg, IDC_BGCOLOR, CPM_GETCOLOUR, 0, 0));
			TextClr = (SendDlgItemMessage(hwndDlg, IDC_TXTCOLOR, CPM_GETCOLOUR, 0, 0));

			if ((db_get_dw(NULL, MODULENAME, REFRESH_KEY, 0) != 0)) {
				KillTimer(nullptr, timerId);
				KillTimer(nullptr, Countdown);
				timerId = SetTimer(nullptr, 0, ((db_get_dw(NULL, MODULENAME, REFRESH_KEY, 0)) * MINUTE), timerfunc);
				Countdown = SetTimer(nullptr, 0, MINUTE, Countdownfunc);
			}
			if ((db_get_dw(NULL, MODULENAME, REFRESH_KEY, 0) == 0)) {
				KillTimer(nullptr, timerId);
				KillTimer(nullptr, Countdown);
			}
			test = 0;
		}
		break; // end apply
	}

	return 0;
}
