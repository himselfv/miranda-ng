#include "common.h"

// #TODO Remove, as we are not using the chat-module for groups anymore

int WhatsAppProto::OnJoinChat(WPARAM,LPARAM)
{
   return 0;
}

int WhatsAppProto::OnLeaveChat(WPARAM,LPARAM)
{
   return 0;
}

int WhatsAppProto::OnChatOutgoing(WPARAM wParam, LPARAM lParam)
{
	GCHOOK *hook = reinterpret_cast<GCHOOK*>(lParam);
	char *text;
	char *id;

	if (strcmp(hook->pDest->pszModule,m_szModuleName))
		return 0;

	switch(hook->pDest->iType)
	{
	case GC_USER_MESSAGE:
	{
		text = mir_t2a_cp(hook->ptszText,CP_UTF8);
		std::string msg = text;

		id = mir_t2a_cp(hook->pDest->ptszID,CP_UTF8);
		std::string chat_id = id;

		mir_free(text);
		mir_free(id);
	
		if (isOnline()) {
         HANDLE hContact = this->ContactIDToHContact(chat_id);
         if (hContact)
         {
            LOG("**Chat - Outgoing message: %s", text);
            this->SendMsg(hContact, IS_CHAT, msg.c_str());
            
            // #TODO Move to SendMsgWorker, otherwise all messages are "acknowledged" by Miranda

				GCDEST gcd = { m_szModuleName, { NULL }, GC_EVENT_MESSAGE };
				gcd.ptszID = hook->pDest->ptszID;

				GCEVENT gce = {0};
				gce.cbSize = sizeof(GCEVENT);
				gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
				gce.pDest = &gcd;
				gce.ptszNick = mir_a2t(this->nick.c_str());
				gce.ptszUID = mir_a2t(this->jid.c_str());
				gce.time = time(NULL);
				gce.ptszText = hook->ptszText;
				gce.bIsMe = TRUE;
				CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

				mir_free((void*)gce.ptszUID);
            mir_free((void*)gce.ptszNick);
         }
		}
	
		break;
	}

	case GC_USER_LEAVE:
	case GC_SESSION_TERMINATE:
	{
		break;
	}
	}

	return 0;
}

