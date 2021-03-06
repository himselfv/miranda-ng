/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-03 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include <m_genmenu.h>
#include <m_ignore.h>
#include "cluiframes.h"

#pragma hdrstop

extern IconItem iconItem[];
void InitIconLibMenuIcons();

INT_PTR CloseAction(WPARAM, LPARAM)
{
	cfg::shutDown = 1;

	if (Miranda_OkToExit()) {
		DestroyWindow(g_clistApi.hwndContactList);
		PostQuitMessage(0);
		Sleep(0);
	}

	return 0;
}

static MWindowList hWindowListIGN = nullptr;

// dialog procedure for handling the contact ignore dialog (available from the contact menu
static INT_PTR CALLBACK IgnoreDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MCONTACT hContact = (MCONTACT)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		{
			DWORD dwMask;
			ClcContact *contact = nullptr;
			int pCaps;
			HWND hwndAdd;

			hContact = lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)hContact);
			dwMask = db_get_dw(hContact, "Ignore", "Mask1", 0);
			SendMessage(hWnd, WM_USER + 100, hContact, dwMask);
			SendMessage(hWnd, WM_USER + 120, 0, 0);
			TranslateDialogDefault(hWnd);
			hwndAdd = GetDlgItem(hWnd, IDC_IGN_ADDPERMANENTLY); // CreateWindowEx(0, L"CLCButtonClass", L"FOO", WS_VISIBLE | BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 200, 276, 106, 24, hWnd, (HMENU)IDC_IGN_ADDPERMANENTLY, g_plugin.getInst(), NULL);
			CustomizeButton(hwndAdd, false, true, false);

			SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)Skin_LoadIcon(SKINICON_OTHER_ADDCONTACT));
			SetWindowText(hwndAdd, TranslateT("Add permanently"));
			EnableWindow(hwndAdd, db_get_b(hContact, "CList", "NotOnList", 0));

			hwndAdd = GetDlgItem(hWnd, IDC_DSP_LOADDEFAULT); // CreateWindowEx(0, L"CLCButtonClass", L"FOO", WS_VISIBLE | BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 200, 276, 106, 24, hWnd, (HMENU)IDC_IGN_ADDPERMANENTLY, g_plugin.getInst(), NULL);
			CustomizeButton(hwndAdd, false, true, false);

			SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)Skin_LoadIcon(SKINICON_OTHER_DELETE));
			SetWindowText(hwndAdd, TranslateT("Revert to default"));
			EnableWindow(hwndAdd, TRUE);

			SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Default (global setting)"));
			SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Show always when available"));
			SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Hide always"));

			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Default (global setting)"));
			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When space is available"));
			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When needed by status message"));

			if (cfg::clcdat) {
				Clist_FindItem(g_clistApi.hwndContactTree, cfg::clcdat, hContact, &contact, nullptr, nullptr);
				if (contact && contact->type != CLCIT_CONTACT) {
					DestroyWindow(hWnd);
					return FALSE;
				}
				else {
					wchar_t szTitle[512];
					DWORD dwFlags = db_get_dw(hContact, "CList", "CLN_Flags", 0);
					BYTE bSecondLine = db_get_b(hContact, "CList", "CLN_2ndline", -1);

					mir_snwprintf(szTitle, TranslateT("Contact list display and ignore options for %s"), contact ? contact->szText : Clist_GetContactDisplayName(hContact));

					SetWindowText(hWnd, szTitle);
					Window_SetSkinIcon_IcoLib(hWnd, SKINICON_OTHER_MIRANDA);
					pCaps = CallProtoService(contact ? contact->pce->szProto : GetContactProto(hContact), PS_GETCAPS, PFLAGNUM_1, 0);
					Utils::enableDlgControl(hWnd, IDC_IGN_ALWAYSONLINE, pCaps & PF1_INVISLIST ? TRUE : FALSE);
					Utils::enableDlgControl(hWnd, IDC_IGN_ALWAYSOFFLINE, pCaps & PF1_VISLIST ? TRUE : FALSE);
					CheckDlgButton(hWnd, IDC_IGN_PRIORITY, db_get_b(hContact, "CList", "Priority", 0) ? BST_CHECKED : BST_UNCHECKED);
					Utils::enableDlgControl(hWnd, IDC_IGN_PRIORITY, TRUE);
					Utils::enableDlgControl(hWnd, IDC_AVATARDISPMODE, TRUE);
					Utils::enableDlgControl(hWnd, IDC_SECONDLINEMODE, TRUE);
					if (dwFlags & ECF_FORCEAVATAR)
						SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 1, 0);
					else if (dwFlags & ECF_HIDEAVATAR)
						SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 2, 0);
					else
						SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 0, 0);

					if (dwFlags & ECF_FORCEOVERLAY)
						CheckDlgButton(hWnd, IDC_OVERLAYICON, BST_CHECKED);
					else if (dwFlags & ECF_HIDEOVERLAY)
						CheckDlgButton(hWnd, IDC_OVERLAYICON, BST_UNCHECKED);
					else
						CheckDlgButton(hWnd, IDC_OVERLAYICON, BST_INDETERMINATE);

					if (dwFlags & ECF_FORCELOCALTIME)
						CheckDlgButton(hWnd, IDC_SHOWLOCALTIME1, BST_CHECKED);
					else if (dwFlags & ECF_HIDELOCALTIME)
						CheckDlgButton(hWnd, IDC_SHOWLOCALTIME1, BST_UNCHECKED);
					else
						CheckDlgButton(hWnd, IDC_SHOWLOCALTIME1, BST_INDETERMINATE);

					if (bSecondLine == 0xff)
						SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_SETCURSEL, 0, 0);
					else
						SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_SETCURSEL, (WPARAM)(bSecondLine + 1), 0);
				}
			}
			WindowList_Add(hWindowListIGN, hWnd, hContact);
			ShowWindow(hWnd, SW_SHOWNORMAL);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_IGN_PRIORITY:
			SendMessage(g_clistApi.hwndContactTree, CLM_TOGGLEPRIORITYCONTACT, hContact, 0);
			return 0;

		case IDC_IGN_ALL:
			SendMessage(hWnd, WM_USER + 100, hContact, (LPARAM)0xffffffff);
			return 0;

		case IDC_IGN_NONE:
			SendMessage(hWnd, WM_USER + 100, hContact, 0);
			return 0;

		case IDC_IGN_ALWAYSONLINE:
			if (IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, BST_UNCHECKED);
			break;

		case IDC_IGN_ALWAYSOFFLINE:
			if (IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, BST_UNCHECKED);
			break;

		case IDC_HIDECONTACT:
			db_set_b(hContact, "CList", "Hidden", (BYTE)(IsDlgButtonChecked(hWnd, IDC_HIDECONTACT) ? 1 : 0));
			break;

		case IDC_IGN_ADDPERMANENTLY:
			Contact_Add(hContact, hWnd);
			Utils::enableDlgControl(hWnd, IDC_IGN_ADDPERMANENTLY, db_get_b(hContact, "CList", "NotOnList", 0));
			break;

		case IDC_DSP_LOADDEFAULT:
			SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_SETCURSEL, 0, 0);
			CheckDlgButton(hWnd, IDC_OVERLAYICON, BST_INDETERMINATE);
			CheckDlgButton(hWnd, IDC_LOCALTIME, BST_INDETERMINATE);
			break;

		case IDOK:
			{
				DWORD newMask = 0;
				ClcContact *contact = nullptr;

				SendMessage(hWnd, WM_USER + 110, 0, (LPARAM)&newMask);
				db_set_dw(hContact, "Ignore", "Mask1", newMask);
				SendMessage(hWnd, WM_USER + 130, 0, 0);

				if (cfg::clcdat) {
					LRESULT iSel = SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_GETCURSEL, 0, 0);
					DWORD dwFlags = db_get_dw(hContact, "CList", "CLN_Flags", 0), dwXMask = 0;
					LRESULT  checked = 0;

					Clist_FindItem(g_clistApi.hwndContactTree, cfg::clcdat, hContact, &contact, nullptr, nullptr);
					if (iSel != CB_ERR) {
						dwFlags &= ~(ECF_FORCEAVATAR | ECF_HIDEAVATAR);

						if (iSel == 1)
							dwFlags |= ECF_FORCEAVATAR;
						else if (iSel == 2)
							dwFlags |= ECF_HIDEAVATAR;
						if (contact)
							LoadAvatarForContact(contact);
					}

					dwFlags &= ~(ECF_FORCEOVERLAY | ECF_HIDEOVERLAY | ECF_FORCELOCALTIME | ECF_HIDELOCALTIME);

					checked = IsDlgButtonChecked(hWnd, IDC_OVERLAYICON);
					if (checked == BST_CHECKED)
						dwFlags |= ECF_FORCEOVERLAY;
					else if (checked == BST_UNCHECKED)
						dwFlags |= ECF_HIDEOVERLAY;

					checked = IsDlgButtonChecked(hWnd, IDC_SHOWLOCALTIME1);
					if (checked == BST_CHECKED)
						dwFlags |= ECF_FORCELOCALTIME;
					else if (checked == BST_UNCHECKED)
						dwFlags |= ECF_HIDELOCALTIME;

					db_set_dw(hContact, "CList", "CLN_Flags", dwFlags);

					if ((iSel = SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_GETCURSEL, 0, 0)) != CB_ERR) {
						if (iSel == 0) {
							db_unset(hContact, "CList", "CLN_2ndline");
							if (contact)
								contact->bSecondLine = cfg::dat.dualRowMode;
						}
						else {
							db_set_b(hContact, "CList", "CLN_2ndline", (BYTE)(iSel - 1));
							if (contact)
								contact->bSecondLine = (BYTE)(iSel - 1);
						}
					}
					db_set_dw(hContact, "CList", "CLN_xmask", dwXMask);
					if (contact) {
						if (contact->pExtra)
							contact->pExtra->dwDFlags = dwFlags;
					}
					else {
						TExtraCache *p = cfg::getCache(hContact, nullptr);
						if (p)
							p->dwDFlags = dwFlags;
					}
					db_set_b(hContact, "CList", "Priority", (BYTE)(IsDlgButtonChecked(hWnd, IDC_IGN_PRIORITY) ? 1 : 0));
					Clist_Broadcast(CLM_AUTOREBUILD, 0, 0);
				}
			}
		case IDCANCEL:
			DestroyWindow(hWnd);
			break;
		}
		break;

	case WM_USER + 100:	// fill dialog (wParam = hContact, lParam = mask)
		CheckDlgButton(hWnd, IDC_IGN_MSGEVENTS, lParam & (1 << (IGNOREEVENT_MESSAGE - 1)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_IGN_FILEEVENTS, lParam & (1 << (IGNOREEVENT_FILE - 1)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_IGN_URLEVENTS, lParam & (1 << (IGNOREEVENT_URL - 1)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_IGN_AUTH, lParam & (1 << (IGNOREEVENT_AUTHORIZATION - 1)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_IGN_ADD, lParam & (1 << (IGNOREEVENT_YOUWEREADDED - 1)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_IGN_ONLINE, lParam & (1 << (IGNOREEVENT_USERONLINE - 1)) ? BST_CHECKED : BST_UNCHECKED);
		return 0;

	case WM_USER + 110:	// retrieve value
		{
			DWORD *dwNewMask = (DWORD *)lParam, dwMask = 0;
			dwMask = (IsDlgButtonChecked(hWnd, IDC_IGN_MSGEVENTS) ? (1 << (IGNOREEVENT_MESSAGE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_FILEEVENTS) ? (1 << (IGNOREEVENT_FILE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_URLEVENTS) ? (1 << (IGNOREEVENT_URL - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_AUTH) ? (1 << (IGNOREEVENT_AUTHORIZATION - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ADD) ? (1 << (IGNOREEVENT_YOUWEREADDED - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ONLINE) ? (1 << (IGNOREEVENT_USERONLINE - 1)) : 0);

			if (dwNewMask)
				*dwNewMask = dwMask;
		}
		return 0;

	case WM_USER + 120:	// set visibility status
		{
			ClcContact *contact = nullptr;
			if (Clist_FindItem(g_clistApi.hwndContactTree, cfg::clcdat, hContact, &contact, nullptr, nullptr)) {
				if (contact) {
					WORD wApparentMode = db_get_w(contact->hContact, contact->pce->szProto, "ApparentMode", 0);

					CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, wApparentMode == ID_STATUS_OFFLINE ? BST_CHECKED : BST_UNCHECKED);
					CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, wApparentMode == ID_STATUS_ONLINE ? BST_CHECKED : BST_UNCHECKED);
				}
			}
		}
		return 0;

	case WM_USER + 130:	// update apparent mode
		{
			ClcContact *contact = nullptr;

			if (Clist_FindItem(g_clistApi.hwndContactTree, cfg::clcdat, hContact, &contact, nullptr, nullptr)) {
				if (contact) {
					WORD wApparentMode = 0;

					if (IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
						wApparentMode = ID_STATUS_ONLINE;
					else if (IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
						wApparentMode = ID_STATUS_OFFLINE;

					ProtoChainSend(hContact, PSS_SETAPPARENTMODE, (WPARAM)wApparentMode, 0);
					SendMessage(hWnd, WM_USER + 120, 0, 0);
				}
			}
		}
		return 0;

	case WM_DESTROY:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		WindowList_Remove(hWindowListIGN, hWnd);
		break;
	}
	return FALSE;
}

/*
 * service function: Open ignore settings dialog for the contact handle in wParam
 * (clist_nicer+ specific service)
 *
 * Servicename = CList/SetContactIgnore
 *
 * ensure that dialog is only opened once (the dialog proc saves the window handle of an open dialog
 * of this type to the contacts database record).
 *
 * if dialog is already open, focus it.
 */

static INT_PTR SetContactIgnore(WPARAM wParam, LPARAM)
{
	HWND hWnd = nullptr;

	if (hWindowListIGN == nullptr)
		hWindowListIGN = WindowList_Create();

	hWnd = WindowList_Find(hWindowListIGN, wParam);
	if (wParam) {
		if (hWnd == nullptr)
			CreateDialogParam(g_plugin.getInst(), MAKEINTRESOURCE(IDD_QUICKIGNORE), nullptr, IgnoreDialogProc, (LPARAM)wParam);
		else if (IsWindow(hWnd))
			SetFocus(hWnd);
	}
	return 0;
}

int InitCustomMenus(void)
{
	InitIconLibMenuIcons();

	CreateServiceFunction("CloseAction", CloseAction);
	CreateServiceFunction("CList/SetContactIgnore", SetContactIgnore);

	CMenuItem mi(&g_plugin);
	SET_UID(mi, 0xe3b08c6f, 0x8a01, 0x4c94, 0xb3, 0xf5, 0x9d, 0x38, 0x6, 0x63, 0x7a, 0xa9);
	mi.position = 200000;
	mi.pszService = "CList/SetContactIgnore";
	mi.hIcolibItem = iconItem[0].hIcolib;
	mi.name.a = LPGEN("&Contact list settings...");
	Menu_AddContactMenuItem(&mi);
	return 0;
}

void UninitCustomMenus(void)
{
	WindowList_Destroy(hWindowListIGN);
}
