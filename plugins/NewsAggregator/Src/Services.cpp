/*
Copyright (C) 2012 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "stdafx.h"

int g_nStatus = ID_STATUS_ONLINE;
UINT_PTR timerId = 0;
HANDLE hTBButton = nullptr, hNewsAggregatorFolder = nullptr;

int OnFoldersChanged(WPARAM, LPARAM)
{
	FoldersGetCustomPathT(hNewsAggregatorFolder, tszRoot, MAX_PATH, L"");
	return 0;
}

int NewsAggrInit(WPARAM, LPARAM)
{
	if (hNewsAggregatorFolder = FoldersRegisterCustomPathT(LPGEN("Avatars"), LPGEN("News Aggregator"), MIRANDA_USERDATAT L"\\Avatars\\" _A2W(DEFAULT_AVATARS_FOLDER)))
		FoldersGetCustomPathT(hNewsAggregatorFolder, tszRoot, MAX_PATH, L"");
	else
		mir_wstrncpy(tszRoot, VARSW(L"%miranda_userdata%\\Avatars\\" _A2W(DEFAULT_AVATARS_FOLDER)), _countof(tszRoot));

	for (auto &hContact : Contacts(MODULENAME)) {
		if (!db_get_b(NULL, MODULENAME, "StartupRetrieve", 1))
			db_set_dw(hContact, MODULENAME, "LastCheck", time(0));
		db_set_w(hContact, MODULENAME, "Status", ID_STATUS_ONLINE);
	}

	NetlibInit();
	InitMenu();

	HookEvent(ME_TTB_MODULELOADED, OnToolbarLoaded);
	HookEvent(ME_FOLDERS_PATH_CHANGED, OnFoldersChanged);

	// timer for the first update
	timerId = SetTimer(nullptr, 0, 10000, timerProc2); // first update is 10 sec after load

	return 0;
}

int NewsAggrPreShutdown(WPARAM, LPARAM)
{
	KillTimer(nullptr, timerId);
	NetlibUnInit();
	return 0;
}

INT_PTR NewsAggrGetName(WPARAM wParam, LPARAM lParam)
{
	if(lParam) {
		mir_strncpy((char *)lParam, MODULENAME, wParam);
		return 0;
	}

	return 1;
}

INT_PTR NewsAggrGetCaps(WPARAM wp, LPARAM)
{
	switch(wp) {
	case PFLAGNUM_1:
		return PF1_IM | PF1_PEER2PEER;
	case PFLAGNUM_3:
	case PFLAGNUM_2:
		return PF2_ONLINE;
	case PFLAGNUM_4:
		return PF4_AVATARS;
	case PFLAG_UNIQUEIDTEXT:
		return (INT_PTR) "News Feed";
	default:
		return 0;
	}
}

INT_PTR NewsAggrSetStatus(WPARAM wp, LPARAM)
{
	int nStatus = wp;
	if ((ID_STATUS_ONLINE == nStatus) || (ID_STATUS_OFFLINE == nStatus)) {
		int nOldStatus = g_nStatus;
		if(nStatus != g_nStatus) {
			g_nStatus = nStatus;

			for (auto &hContact : Contacts(MODULENAME))
				db_set_w(hContact, MODULENAME, "Status", nStatus);

			ProtoBroadcastAck(MODULENAME, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)nOldStatus, g_nStatus);
		}
	}

	return 0;
}

INT_PTR NewsAggrGetStatus(WPARAM, LPARAM)
{
	return g_nStatus;
}

INT_PTR NewsAggrLoadIcon(WPARAM wParam, LPARAM)
{
	return (LOWORD(wParam) == PLI_PROTOCOL) ? (INT_PTR)CopyIcon(LoadIconEx("main", FALSE)) : 0;
}

static void __cdecl AckThreadProc(void *param)
{
	Sleep(100);
	ProtoBroadcastAck(MODULENAME, (UINT_PTR)param, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE)1, 0);
}

INT_PTR NewsAggrGetInfo(WPARAM, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *)lParam;
	mir_forkthread(AckThreadProc, (void*)ccs->hContact);
	return 0;
}

INT_PTR CheckAllFeeds(WPARAM, LPARAM lParam)
{
	for (auto &hContact : Contacts(MODULENAME)) {
		if (lParam && db_get_dw(hContact, MODULENAME, "UpdateTime", DEFAULT_UPDATE_TIME))
			UpdateListAdd(hContact);
		else if (!lParam)
			UpdateListAdd(hContact);
	}
	if (!ThreadRunning)
		mir_forkthread(UpdateThreadProc);

	return 0;
}

INT_PTR AddFeed(WPARAM, LPARAM)
{
	if (pAddFeedDialog == nullptr) {
		pAddFeedDialog = new CFeedEditor(-1, nullptr, NULL);
		pAddFeedDialog->Show();
	}
	else {
		SetForegroundWindow(pAddFeedDialog->GetHwnd());
		SetFocus(pAddFeedDialog->GetHwnd());
	}
	return 0;
}

INT_PTR ChangeFeed(WPARAM hContact, LPARAM)
{
	CFeedEditor *pDlg = nullptr;
	for (auto &it : g_arFeeds)
		if (it->getContact() == hContact)
			pDlg = it;

	if (pDlg == nullptr) {
		pDlg = new CFeedEditor(-1, nullptr, hContact);
		pDlg->Show();
	}
	else {
		SetForegroundWindow(pDlg->GetHwnd());
		SetFocus(pDlg->GetHwnd());
	}
	return 0;
}

INT_PTR ImportFeeds(WPARAM, LPARAM)
{
	if (pImportDialog == nullptr)
		pImportDialog = new CImportFeed(nullptr);
	pImportDialog->Show();
	return 0;
}

INT_PTR ExportFeeds(WPARAM, LPARAM)
{
	if (pExportDialog == nullptr)
		pExportDialog = new CExportFeed();
	pExportDialog->Show();
	return 0;
}

INT_PTR CheckFeed(WPARAM hContact, LPARAM)
{
	if(IsMyContact(hContact))
		UpdateListAdd(hContact);
	if ( !ThreadRunning)
		mir_forkthread(UpdateThreadProc);
	return 0;
}

INT_PTR NewsAggrGetAvatarInfo(WPARAM wParam, LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *)lParam;
	if (!IsMyContact(pai->hContact))
		return GAIR_NOAVATAR;

	// if GAIF_FORCE is set, we are updating the feed
	// otherwise, cached avatar is used
	if ((wParam & GAIF_FORCE) && db_get_dw(pai->hContact, MODULENAME, "UpdateTime", DEFAULT_UPDATE_TIME))
		UpdateListAdd(pai->hContact);
	if (db_get_b(NULL, MODULENAME, "AutoUpdate", 1) != 0 && !ThreadRunning)
		mir_forkthread(UpdateThreadProc, (void *)TRUE);

	wchar_t *ptszImageURL = db_get_wsa(pai->hContact, MODULENAME, "ImageURL");
	if(ptszImageURL == nullptr)
		return GAIR_NOAVATAR;

	mir_free(ptszImageURL);
	return GAIR_WAITFOR;
}

INT_PTR NewsAggrRecvMessage(WPARAM, LPARAM lParam)
{
	PROTOACCOUNT *pa = Proto_GetAccount(MODULENAME);
	if (pa && pa->ppro) {
		CCSDATA *ccs = (CCSDATA*)lParam;
		pa->ppro->PROTO_INTERFACE::RecvMsg(ccs->hContact, (PROTORECVEVENT*)ccs->lParam);
	}

	return 0;
}

void UpdateMenu(bool State)
{
	if (!State) // to enable auto-update
		Menu_ModifyItem(hService2[0], LPGENW("Auto Update Enabled"), GetIconHandle("enabled"));
	else  // to disable auto-update
		Menu_ModifyItem(hService2[0], LPGENW("Auto Update Disabled"), GetIconHandle("disabled"));

	CallService(MS_TTB_SETBUTTONSTATE, (WPARAM)hTBButton, State ? TTBST_PUSHED : 0);
	db_set_b(NULL, MODULENAME, "AutoUpdate", !State);
}

// update the newsaggregator auto-update menu item when click on it
INT_PTR EnableDisable(WPARAM, LPARAM)
{
	UpdateMenu(db_get_b(NULL, MODULENAME, "AutoUpdate", 1) != 0);
	NewsAggrSetStatus(db_get_b(NULL, MODULENAME, "AutoUpdate", 1) ? ID_STATUS_ONLINE : ID_STATUS_OFFLINE, 0);
	return 0;
}

int OnToolbarLoaded(WPARAM, LPARAM)
{
	TTBButton ttb = {};
	ttb.name = LPGEN("Enable/disable auto update");
	ttb.pszService = MS_NEWSAGGREGATOR_ENABLED;
	ttb.pszTooltipUp = LPGEN("Auto Update Enabled");
	ttb.pszTooltipDn = LPGEN("Auto Update Disabled");
	ttb.hIconHandleUp = GetIconHandle("enabled");
	ttb.hIconHandleDn = GetIconHandle("disabled");
	ttb.dwFlags = (db_get_b(NULL, MODULENAME, "AutoUpdate", 1) ? 0 : TTBBF_PUSHED) | TTBBF_ASPUSHBUTTON | TTBBF_VISIBLE;
	hTBButton = g_plugin.addTTB(&ttb);
	return 0;
}
