#include "stdafx.h"

static int OnProtoAck(WPARAM, LPARAM lParam)
{
	ACKDATA *ack = (ACKDATA*)lParam;

	if (ack->type != ACKTYPE_STATUS)
		return 0;

	for (auto &hContact : Contacts(ack->szModule)) {
		MessageWindowData msgw;
		if (Srmm_GetWindowData(hContact, msgw) || !(msgw.uState & MSG_WINDOW_STATE_EXISTS))
			continue;

		BBButton bbd = {};
		bbd.pszModuleName = MODULENAME;
		bbd.dwButtonID = BBB_ID_FILE_SEND;
		bbd.bbbFlags = CanSendToContact(hContact)
			? BBSF_RELEASED
			: BBSF_DISABLED;
		Srmm_SetButtonState(hContact, &bbd);
	}

	return 0;
}

static int OnFileDialogCanceled(WPARAM hContact, LPARAM)
{
	for (auto &service : Services) {
		auto it = service->InterceptedContacts.find(hContact);
		if (it != service->InterceptedContacts.end())
			service->InterceptedContacts.erase(it);
	}
	return 0;
}

int OnModulesLoaded(WPARAM, LPARAM)
{
	InitializeMenus();

	HookEvent(ME_PROTO_ACK, OnProtoAck);

	// options
	HookEvent(ME_OPT_INITIALISE, OnOptionsInitialized);

	// srfile
	HookEvent(ME_FILEDLG_CANCELED, OnFileDialogCanceled);

	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, OnPrebuildContactMenu);

	HookEvent(ME_MSG_WINDOWEVENT, OnSrmmWindowOpened);
	HookEvent(ME_MSG_BUTTONPRESSED, OnSrmmButtonPressed);

	HookTemporaryEvent(ME_MSG_TOOLBARLOADED, OnSrmmToolbarLoaded);
	return 0;
}