#include "stdafx.h"

static void __cdecl Login_ThreadFunc(Account *curAcc)
{
	if (curAcc == nullptr)
		return;

	HANDLE hTempFile;
	DWORD  dwBytesWritten, dwBufSize = 1024;
	char szTempName[MAX_PATH];
	char buffer[1024];
	char *str_temp;
	char lpPathBuffer[1024];

	if (GetBrowser(lpPathBuffer)) {
		if (opt.AutoLogin == 0) {
			if (curAcc->hosted[0]) {
				mir_strcat(lpPathBuffer, "https://mail.google.com/a/");
				mir_strcat(lpPathBuffer, curAcc->hosted);
				mir_strcat(lpPathBuffer, "/?logout");
			}
			else {
				mir_strcat(lpPathBuffer, "https://mail.google.com/mail/?logout");
			}
		}
		else {
			if (curAcc->hosted[0]) {
				GetTempPathA(dwBufSize, buffer);
				GetTempFileNameA(buffer, "gmail", 0, szTempName);

				hTempFile = CreateFileA(szTempName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
				mir_strcpy(buffer, FORMDATA1);
				mir_strcat(buffer, curAcc->hosted);
				mir_strcat(buffer, FORMDATA2);
				mir_strcat(buffer, curAcc->hosted);
				mir_strcat(buffer, FORMDATA3);
				mir_strcat(buffer, "<input type=hidden name=userName value=");
				mir_strcat(buffer, curAcc->name);
				if ((str_temp = strstr(buffer, "@")) != nullptr)
					*str_temp = '\0';
				mir_strcat(buffer, "><input type=hidden name=password value=");
				mir_strcat(buffer, curAcc->pass);
				mir_strcat(buffer, "></form></body>");
				WriteFile(hTempFile, buffer, (DWORD)mir_strlen(buffer), &dwBytesWritten, nullptr);
				CloseHandle(hTempFile);
				mir_strcat(lpPathBuffer, szTempName);
			}
			else {
				char *szEncodedURL = mir_urlEncode(curAcc->name);
				mir_strcat(lpPathBuffer, LINK);
				mir_strcat(lpPathBuffer, szEncodedURL);
				//mir_strcat(lpPathBuffer, "&Passwd=");
				//mir_strcat(lpPathBuffer, mir_urlEncode(curAcc->pass));
				if (opt.AutoLogin == 1)
					mir_strcat(lpPathBuffer, "&PersistentCookie=yes");
				mir_free(szEncodedURL);
			}
		}
	}

	STARTUPINFOA suInfo = { 0 };
	PROCESS_INFORMATION procInfo;
	suInfo.cb = sizeof(suInfo);
	suInfo.wShowWindow = SW_MAXIMIZE;
	if (CreateProcessA(nullptr, lpPathBuffer, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &suInfo, &procInfo))
		CloseHandle(procInfo.hProcess);

	if (curAcc->hosted[0]) {
		Sleep(30000);
		DeleteFileA(szTempName);
	}
}

int OpenBrowser(WPARAM hContact, LPARAM)
{
	char *proto = GetContactProto(hContact);
	if (proto && !mir_strcmp(proto, MODULENAME)) {
		Account *curAcc = GetAccountByContact(hContact);
		PUDeletePopup(curAcc->popUpHwnd);
		g_clistApi.pfnRemoveEvent(curAcc->hContact, 1);
		if (GetKeyState(VK_SHIFT) >> 8 || optionWindowIsOpen)
			return FALSE;

		if (curAcc->oldResults_num != 0) {
			db_set_w(curAcc->hContact, MODULENAME, "Status", ID_STATUS_NONEW);
			curAcc->oldResults_num = 0;
			DeleteResults(curAcc->results.next);
			curAcc->results.next = nullptr;
		}
		mir_forkThread<Account>(Login_ThreadFunc, curAcc);
	}
	return FALSE;
}

INT_PTR Notifying(WPARAM, LPARAM lParam)
{
	OpenBrowser(((CLISTEVENT*)lParam)->hContact, 0);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MCONTACT hContact = PUGetContact(hWnd);
	Account *curAcc = GetAccountByContact(hContact);

	switch (message) {
	case UM_INITPOPUP:
		curAcc->popUpHwnd = hWnd;
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == STN_CLICKED)
			OpenBrowser((WPARAM)hContact, 0);
		break;

	case WM_CONTEXTMENU:
		PUDeletePopup(hWnd);
		curAcc->popUpHwnd = nullptr;
		g_clistApi.pfnRemoveEvent(hContact, 1);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void NotifyUser(Account *curAcc)
{
	if (optionWindowIsOpen)
		return;

	db_set_s(curAcc->hContact, "CList", "MyHandle", curAcc->results.content);
	switch (curAcc->results_num) {
	case 0:
		PUDeletePopup(curAcc->popUpHwnd);
		g_clistApi.pfnRemoveEvent(curAcc->hContact, 1);
		if (curAcc->oldResults_num != 0)
			db_set_w(curAcc->hContact, MODULENAME, "Status", ID_STATUS_NONEW);
		break;

	case -1:
		db_set_w(curAcc->hContact, MODULENAME, "Status", ID_STATUS_AWAY);
		break;

	default:
		db_set_w(curAcc->hContact, MODULENAME, "Status", ID_STATUS_OCCUPIED);
		int newMails = (curAcc->oldResults_num == -1) ? (curAcc->results_num) : (curAcc->results_num - curAcc->oldResults_num);
		if (opt.LogThreads&&newMails > 0) {
			DBEVENTINFO dbei = {};
			dbei.eventType = EVENTTYPE_MESSAGE;
			dbei.flags = DBEF_READ;
			dbei.szModule = MODULENAME;
			dbei.timestamp = time(0);

			resultLink *prst = curAcc->results.next;
			for (int i = 0; i < newMails; i++) {
				dbei.cbBlob = (DWORD)mir_strlen(prst->content) + 1;
				dbei.pBlob = (PBYTE)prst->content;
				db_event_add(curAcc->hContact, &dbei);
				prst = prst->next;
			}
		}
		if (opt.notifierOnTray&&newMails > 0) {
			g_clistApi.pfnRemoveEvent(curAcc->hContact, 1);

			CLISTEVENT cle = {};
			cle.hContact = curAcc->hContact;
			cle.hDbEvent = 1;
			cle.flags = CLEF_URGENT;
			cle.hIcon = Skin_LoadProtoIcon(MODULENAME, ID_STATUS_OCCUPIED);
			cle.pszService = "GmailMNotifier/Notifying";
			cle.szTooltip.a = curAcc->results.next->content;
			g_clistApi.pfnAddEvent(&cle);
		}

		if (opt.notifierOnPop&&newMails > 0) {
			POPUPDATA ppd = { 0 };

			ppd.lchContact = curAcc->hContact;
			ppd.lchIcon = Skin_LoadProtoIcon(MODULENAME, ID_STATUS_OCCUPIED);
			mir_strcpy(ppd.lpzContactName, curAcc->results.content);
			resultLink *prst = curAcc->results.next;
			for (int i = 0; i < 5 && i < newMails; i++) {
				mir_strcat(ppd.lpzText, prst->content);
				mir_strcat(ppd.lpzText, "\n");
				prst = prst->next;
			}
			ppd.colorBack = opt.popupBgColor;
			ppd.colorText = opt.popupTxtColor;
			ppd.PluginWindowProc = PopupDlgProc;
			ppd.PluginData = nullptr;
			ppd.iSeconds = opt.popupDuration;
			PUDeletePopup(curAcc->popUpHwnd);
			PUAddPopup(&ppd);
		}
		if (newMails > 0)
			Skin_PlaySound("Gmail");
	}
	curAcc->oldResults_num = curAcc->results_num;
	DeleteResults(curAcc->results.next);
	curAcc->results.next = nullptr;
}

void DeleteResults(resultLink *prst)
{
	if (prst != nullptr) {
		if (prst->next != nullptr)
			DeleteResults(prst->next);
		free(prst);
	}
}
