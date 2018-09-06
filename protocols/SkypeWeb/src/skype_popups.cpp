#include "stdafx.h"

void CSkypeProto::InitPopups()
{
	wchar_t desc[256];
	char name[256];
	
	POPUPCLASS ppc = { sizeof(ppc) };
	ppc.flags = PCF_UNICODE;
	ppc.pszName = name;
	ppc.pszDescription.w = desc;

	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Notifications"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Notification");
	ppc.hIcon = GetIcon(IDI_NOTIFY);
	ppc.colorBack = RGB(255, 255, 255);
	ppc.colorText = RGB(0, 0, 0);
	ppc.iSeconds = 5;
	m_PopupClasses.insert(Popup_RegisterClass(&ppc));

	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Errors"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Error");
	ppc.hIcon = GetIcon(IDI_ERRORICON);
	ppc.colorBack = RGB(255, 255, 255);
	ppc.colorText = RGB(0, 0, 0);
	ppc.iSeconds = -1;
	m_PopupClasses.insert(Popup_RegisterClass(&ppc));

	mir_snwprintf(desc, L"%s/%s", m_tszUserName, TranslateT("Calls"));
	mir_snprintf(name, "%s_%s", m_szModuleName, "Call");
	ppc.hIcon = GetIcon(IDI_CALL);
	ppc.colorBack = RGB(255, 255, 255);
	ppc.colorText = RGB(0, 0, 0);
	ppc.iSeconds = 30;
	ppc.PluginWindowProc = PopupDlgProcCall;
	m_PopupClasses.insert(Popup_RegisterClass(&ppc));
}

void CSkypeProto::UninitPopups()
{
	for (auto &it : m_PopupClasses)
		Popup_UnregisterClass(it);
}

void CSkypeProto::ShowNotification(const wchar_t *caption, const wchar_t *message, MCONTACT hContact, int type)
{
	if (Miranda_IsTerminated())
		return;

	if (ServiceExists(MS_POPUP_ADDPOPUPCLASS)) {
		CMStringA className(FORMAT, "%s_", m_szModuleName);

		switch (type) {
		case 1:
			className.Append("Error");
			break;

		case SKYPE_DB_EVENT_TYPE_INCOMING_CALL:
			className.Append("Call");
			break;

		default:
			className.Append("Notification");
			break;
		}

		POPUPDATACLASS ppd = { sizeof(ppd) };
		ppd.pwszTitle = caption;
		ppd.pwszText = message;
		ppd.pszClassName = className.GetBuffer();
		ppd.hContact = hContact;

		CallService(MS_POPUP_ADDPOPUPCLASS, 0, (LPARAM)&ppd);
	}
	else {
		DWORD mtype = MB_OK | MB_SETFOREGROUND | MB_ICONSTOP;
		MessageBox(nullptr, message, caption, mtype);
	}
}

void CSkypeProto::ShowNotification(const wchar_t *message, MCONTACT hContact)
{
	ShowNotification(_T(MODULE), message, hContact);
}

LRESULT CSkypeProto::PopupDlgProcCall(HWND hPopup, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CONTEXTMENU:
		PUDeletePopup(hPopup);
		CallService(MODULE "/IncomingCallPP", 0, PUGetContact(hPopup));
		break;
	case WM_COMMAND:
		PUDeletePopup(hPopup);
		CallService(MODULE "/IncomingCallPP", 1, PUGetContact(hPopup));
		break;
	}

	return DefWindowProc(hPopup, uMsg, wParam, lParam);
}
