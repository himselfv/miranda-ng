/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-08 Miranda ICQ/IM project,
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
#include "modern_statusbar.h"

int g_mutex_bOnTrayRightClick = 0;
BOOL g_bMultiConnectionMode = FALSE;
BOOL IS_WM_MOUSE_DOWN_IN_TRAY;
BOOL g_trayTooltipActive = FALSE;
POINT tray_hover_pos = { 0 };

// don't move to win2k.h, need new and old versions to work on 9x/2000/XP
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

#ifndef _INC_SHLWAPI

typedef struct _DllVersionInfo
{
	DWORD cbSize;
	DWORD dwMajorVersion;                   // Major version
	DWORD dwMinorVersion;                   // Minor version
	DWORD dwBuildNumber;                    // Build number
	DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;

#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
typedef HRESULT(CALLBACK* DLLGETVERSIONPROC)(DLLVERSIONINFO *);

#endif

BOOL  g_MultiConnectionMode = FALSE;
char* g_szConnectingProto = nullptr;

int GetStatusVal(int status)
{
	if (IsStatusConnecting(status)) // 'connecting' status has the top priority
		return 600;

	switch (status) {
	case ID_STATUS_OFFLINE:    return 50;
	case ID_STATUS_ONLINE:     return 100;
	case ID_STATUS_FREECHAT:   return 110;
	case ID_STATUS_INVISIBLE:  return 120;
	case ID_STATUS_AWAY:       return 200;
	case ID_STATUS_DND:        return 210;
	case ID_STATUS_NA:         return 220;
	case ID_STATUS_OCCUPIED:   return 230;
	case ID_STATUS_ONTHEPHONE: return 400;
	case ID_STATUS_OUTTOLUNCH: return 410;
	}

	return 0;
}

INT_PTR CListTray_GetGlobalStatus(WPARAM, LPARAM)
{
	g_szConnectingProto = nullptr;

	int curstatus = ID_STATUS_OFFLINE;
	int connectingCount = 0;
	g_bMultiConnectionMode = false;

	for (auto &it : Accounts()) {
		if (!it->IsVisible())
			continue;

		if (IsStatusConnecting(it->iRealStatus)) {
			connectingCount++;
			if (connectingCount == 1)
				g_szConnectingProto = it->szModuleName;
			else 
				g_bMultiConnectionMode = true;
		}
		else if (GetStatusVal(it->iRealStatus) > GetStatusVal(curstatus))
			curstatus = it->iRealStatus;
	}

	return curstatus;
}

/////////////////////////////////////////////////////////////////////////////////////////

static UINT_PTR autoHideTimerId;

static VOID CALLBACK TrayIconAutoHideTimer(HWND hwnd, UINT, UINT_PTR idEvent, DWORD)
{
	KillTimer(hwnd, idEvent);
	HWND hwndClui = g_clistApi.hwndContactList;
	HWND ActiveWindow = GetActiveWindow();
	if (ActiveWindow == hwndClui) return;
	if (CLUI_CheckOwnedByClui(ActiveWindow)) return;

	CListMod_HideWindow();
	SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
}

int cliTrayIconPauseAutoHide(WPARAM, LPARAM)
{
	if (db_get_b(0, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
		if (GetActiveWindow() != g_clistApi.hwndContactList && GetWindow(GetParent(GetActiveWindow()), GW_OWNER) != g_clistApi.hwndContactList) {
			KillTimer(nullptr, autoHideTimerId);
			autoHideTimerId = CLUI_SafeSetTimer(nullptr, 0, 1000 * db_get_w(0, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Tray event handler

INT_PTR cli_TrayIconProcessMessage(WPARAM wParam, LPARAM lParam)
{
	MSG *msg = (MSG*)wParam;
	switch (msg->message) {
	case WM_EXITMENULOOP:
		if (g_clistApi.bTrayMenuOnScreen)
			g_clistApi.bTrayMenuOnScreen = FALSE;
		break;

	case TIM_CALLBACK:
		if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && msg->lParam == WM_LBUTTONDOWN && !db_get_b(0, "CList", "Tray1Click", SETTING_TRAY1CLICK_DEFAULT)) {
			POINT pt;
			HMENU hMenu = Menu_GetStatusMenu();
			g_mutex_bOnTrayRightClick = 1;
			IS_WM_MOUSE_DOWN_IN_TRAY = 1;
			SetForegroundWindow(msg->hwnd);
			SetFocus(msg->hwnd);
			GetCursorPos(&pt);
			g_clistApi.bTrayMenuOnScreen = TRUE;
			TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, msg->hwnd, nullptr);
			PostMessage(msg->hwnd, WM_NULL, 0, 0);
			g_mutex_bOnTrayRightClick = 0;
			IS_WM_MOUSE_DOWN_IN_TRAY = 0;
		}
		else if (msg->lParam == WM_MBUTTONDOWN || msg->lParam == WM_LBUTTONDOWN || msg->lParam == WM_RBUTTONDOWN) {
			IS_WM_MOUSE_DOWN_IN_TRAY = 1;
		}
		else break;
		*((LRESULT*)lParam) = 0;
		return TRUE;

	case WM_ACTIVATE:
		SetCursor(LoadCursor(nullptr, IDC_ARROW));
		{
			HWND h1 = (HWND)msg->lParam;
			HWND h2 = h1 ? GetParent(h1) : nullptr;
			if (db_get_b(0, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT)) {
				if (LOWORD(msg->wParam) == WA_INACTIVE && h2 != g_clistApi.hwndContactList)
					autoHideTimerId = CLUI_SafeSetTimer(nullptr, 0, 1000 * db_get_w(0, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), TrayIconAutoHideTimer);
				else {
					KillTimer(nullptr, autoHideTimerId);
					autoHideTimerId = 0;
				}
			}
			else if (autoHideTimerId) {
				KillTimer(nullptr, autoHideTimerId);
				autoHideTimerId = 0;
			}
		}
		return FALSE; //to avoid autohideTimer in core
	}
	return corecli.pfnTrayIconProcessMessage(wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Tray module init

VOID CALLBACK cliTrayCycleTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	if (!g_clistApi.trayIconCount)
		return;

	PROTOACCOUNT **acc;
	int AccNum;
	Proto_EnumAccounts(&AccNum, &acc);

	// looking for the appropriate account to show its icon
	int t = g_clistApi.cycleStep;
	do {
		g_clistApi.cycleStep = (g_clistApi.cycleStep + 1) % AccNum;
		if (g_clistApi.cycleStep == t)
			return;
	} while (acc[g_clistApi.cycleStep]->bIsVirtual || !acc[g_clistApi.cycleStep]->bIsVisible);

	cliTrayCalcChanged(acc[g_clistApi.cycleStep]->szModuleName, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// this function sets the default settings values. it also migrates the
// existing settings into the new format

void SettingsMigrate(void)
{
	BYTE TrayIcon = db_get_b(0, "CList", "TrayIcon", 0);
	BYTE AlwaysPrimary = db_get_b(0, "CList", "AlwaysPrimary", 0);
	BYTE AlwaysMulti = db_get_b(0, "CList", "AlwaysMulti", 0);
	ptrA PrimaryStatus(db_get_sa(0, "CList", "PrimaryStatus"));

	// these strings must always be set
	if (PrimaryStatus) {
		db_set_s(0, "CList", "tiAccS", PrimaryStatus);
		db_set_s(0, "CList", "tiAccV", PrimaryStatus);
	}
	else {
		db_set_s(0, "CList", "tiAccS", "");
		db_set_s(0, "CList", "tiAccV", "");
	}

	switch (TrayIcon) {
	case 0: // global or single acc
		if (AlwaysPrimary) {
			if (!PrimaryStatus) { // global always
				db_set_b(0, "CList", "tiModeS", TRAY_ICON_MODE_GLOBAL);
				db_set_b(0, "CList", "tiModeV", TRAY_ICON_MODE_GLOBAL);
			}
			else { // single acc always
				db_set_b(0, "CList", "tiModeS", TRAY_ICON_MODE_ACC);
				db_set_b(0, "CList", "tiModeV", TRAY_ICON_MODE_ACC);
			}
		}
		else {
			db_set_b(0, "CList", "tiModeS", TRAY_ICON_MODE_GLOBAL);
			db_set_b(0, "CList", "tiModeV", (PrimaryStatus) ? TRAY_ICON_MODE_ACC : TRAY_ICON_MODE_GLOBAL);
		}
		break;

	case 1: // cycle
		db_set_b(0, "CList", "tiModeS", TRAY_ICON_MODE_CYCLE);
		db_set_b(0, "CList", "tiModeV", TRAY_ICON_MODE_CYCLE);
		break;

	case 2: // multiple
		db_set_b(0, "CList", "tiModeS", (AlwaysMulti) ? TRAY_ICON_MODE_ALL : TRAY_ICON_MODE_GLOBAL);
		db_set_b(0, "CList", "tiModeV", TRAY_ICON_MODE_ALL);
		break;
	}
}

// calculates number of accounts to be displayed 
// and the number of the most active account

static int GetGoodAccNum(bool *bDiffers, bool *bConn = nullptr)
{
	*bDiffers = false;
	if (bConn)
		*bConn = false;

	int iPrevStatus = 0, res = 0;
	for (auto &pa : Accounts()) {
		if (!pa->IsVisible())
			continue;

		res++;
		if (!iPrevStatus)
			iPrevStatus = pa->iRealStatus;
		else if (iPrevStatus != pa->iRealStatus)
			*bDiffers = true;

		if (bConn)
			if (IsStatusConnecting(pa->iRealStatus))
				*bConn = true;
	}

	return res;
}

BYTE OldMode; //
UINT_PTR TimerID = 0;

int cliTrayIconInit(HWND hwnd)
{
	if (g_clistApi.trayIconCount != 0)
		return 0;

	if (TimerID) {
		KillTimer(nullptr, TimerID);
		TimerID = 0;
	}

	// Присутствуют ли в базе новые настройки? Если да, то обновление не нужно.
	if (-1 == db_get_b(0, "CList", "tiModeS", -1))
		SettingsMigrate();

	// Нужно узнать количество годных аккаунтов и неодинаковость их статусов.
	bool bDiffers;
	g_clistApi.trayIconCount = GetGoodAccNum(&bDiffers);
	// Если таковых аккаунтов не нашлось вообще, то будем показывать основную иконку Миранды.
	if (!g_clistApi.trayIconCount) {
		g_clistApi.trayIconCount = 1;
		g_clistApi.trayIcon = (trayIconInfo_t*)mir_calloc(sizeof(trayIconInfo_t) * g_clistApi.trayIconCount);
		Clist_TrayIconAdd(hwnd, nullptr, nullptr, CListTray_GetGlobalStatus(0, 0));
		OldMode = TRAY_ICON_MODE_GLOBAL;
		return 0;
	}

	BYTE Mode;
	if (!bDiffers)  // all equal
		OldMode = Mode = db_get_b(0, "CList", "tiModeS", TRAY_ICON_MODE_GLOBAL);
	else
		OldMode = Mode = db_get_b(0, "CList", "tiModeV", TRAY_ICON_MODE_GLOBAL);

	// Некоторые режимы всегда показывают единственную иконку.
	if (Mode < 8)
		g_clistApi.trayIconCount = 1;

	g_clistApi.trayIcon = (trayIconInfo_t*)mir_calloc(sizeof(trayIconInfo_t) * g_clistApi.trayIconCount);

	// Добавляем иконки.
	switch (Mode) {
	case TRAY_ICON_MODE_GLOBAL:
		Clist_TrayIconAdd(hwnd, nullptr, nullptr, CListTray_GetGlobalStatus(0, 0));
		break;

	case TRAY_ICON_MODE_ACC:
		{
			ptrA szProto(db_get_sa(0, "CList", (!bDiffers) ? "tiAccS" : "tiAccV"));
			if (!szProto)
				break;

			PROTOACCOUNT *pa = Proto_GetAccount(szProto);
			if (!pa || !pa->ppro)
				Clist_TrayIconAdd(hwnd, nullptr, nullptr, CListTray_GetGlobalStatus(0, 0));
			else
				Clist_TrayIconAdd(hwnd, pa->szModuleName, nullptr, pa->ppro->m_iStatus);
		}
		break;

	case TRAY_ICON_MODE_CYCLE:
		Clist_TrayIconAdd(hwnd, nullptr, nullptr, CListTray_GetGlobalStatus(0, 0));
		g_clistApi.cycleStep = 0;
		cliTrayCycleTimerProc(nullptr, 0, 0, 0); // force icon update
		
		// Не сохраняем ID таймера в pcli, чтобы fnTrayIconUpdateBase не убивала его.
		TimerID = CLUI_SafeSetTimer(nullptr, 0, db_get_w(0, "CList", "CycleTime", SETTING_CYCLETIME_DEFAULT) * 1000, cliTrayCycleTimerProc);
		break;

	case TRAY_ICON_MODE_ALL:
		for (auto &pa : Accounts().rev_iter())
			if (pa->IsVisible() && pa->ppro != nullptr)
				Clist_TrayIconAdd(hwnd, pa->szModuleName, nullptr, pa->ppro->m_iStatus);
		break;
	}

	return 0;
}

int cliTrayCalcChanged(const char *szChangedProto, int, int)
{
	if (!szChangedProto)
		return -1;

	if (!g_clistApi.trayIconCount)
		return -1;

	if (!Clist_GetProtocolVisibility(szChangedProto))
		return -1;

	bool bDiffers, bConn;
	GetGoodAccNum(&bDiffers, &bConn);

	// if the icon number to be changed, reinitialize module from scratch
	BYTE Mode = db_get_b(0, "CList", (!bDiffers) ? "tiModeS" : "tiModeV", TRAY_ICON_MODE_GLOBAL);
	if (Mode != OldMode) {
		OldMode = Mode;
		Clist_TrayIconIconsChanged();
	}

	HICON hIcon = nullptr;
	int i = 0, iStatus;

	switch (Mode) {
	case TRAY_ICON_MODE_GLOBAL:
		hIcon = g_clistApi.pfnGetIconFromStatusMode(0, nullptr, CListTray_GetGlobalStatus(0, 0));
		Clist_TrayIconMakeTooltip(nullptr, nullptr);
		break;

	case TRAY_ICON_MODE_ACC:
		// В этом режиме показывается иконка совершенно определённого аккаунта, и не всегда это szChangedProto.
		{
			ptrA szProto(db_get_sa(0, "CList", bDiffers ? "tiAccV" : "tiAccS"));
			if (szProto == nullptr)
				break;

			iStatus = Proto_GetStatus(szProto);
			if (g_StatusBarData.bConnectingIcon && IsStatusConnecting(iStatus))
				hIcon = (HICON)CLUI_GetConnectingIconService((WPARAM)szProto, 0);
			else
				hIcon = g_clistApi.pfnGetIconFromStatusMode(0, szProto, Proto_GetStatus(szProto));

			Clist_TrayIconMakeTooltip(nullptr, szProto);
		}
		break;

	case TRAY_ICON_MODE_CYCLE:
		iStatus = Proto_GetStatus(szChangedProto);
		if (g_StatusBarData.bConnectingIcon && IsStatusConnecting(iStatus))
			hIcon = (HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto, 0);
		else if (!bConn)
			hIcon = g_clistApi.pfnGetIconFromStatusMode(0, szChangedProto, Proto_GetStatus(szChangedProto));
		Clist_TrayIconMakeTooltip(nullptr, nullptr);
		break;

	case TRAY_ICON_MODE_ALL:
		// Какой индекс у аккаунта, который будем апдейтить?
		for (; i < g_clistApi.trayIconCount; i++)
			if (!mir_strcmp(g_clistApi.trayIcon[i].szProto, szChangedProto))
				break;

		iStatus = Proto_GetStatus(szChangedProto);
		if (g_StatusBarData.bConnectingIcon && IsStatusConnecting(iStatus))
			hIcon = (HICON)CLUI_GetConnectingIconService((WPARAM)szChangedProto, 0);
		else
			hIcon = g_clistApi.pfnGetIconFromStatusMode(0, szChangedProto, Proto_GetStatus(szChangedProto));
		Clist_TrayIconMakeTooltip(nullptr, g_clistApi.trayIcon[i].szProto);
		break;
	}

	trayIconInfo_t &p = g_clistApi.trayIcon[i];
	DestroyIcon(p.hBaseIcon);
	p.hBaseIcon = hIcon;
	replaceStrW(p.ptszToolTip, g_clistApi.szTip);

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.hWnd = g_clistApi.hwndContactList;
	nid.uID = p.id;
	nid.hIcon = p.hBaseIcon;
	nid.uFlags = NIF_ICON | NIF_TIP;

	// if Tipper is missing or turned off for tray, use system tooltips
	if (!ServiceExists("mToolTip/ShowTip") || !db_get_b(0, "Tipper", "TrayTip", 1))
		wcsncpy_s(nid.szTip, g_clistApi.szTip, _TRUNCATE);

	Shell_NotifyIcon(NIM_MODIFY, &nid);

	return -1;
}
