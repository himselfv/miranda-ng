/* Copyright (C) Miklashevsky Roman, sss, elzor
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

CMPlugin g_plugin;

HANDLE hEventFilter = nullptr, hOptInitialise = nullptr, hSettingChanged = nullptr;

BOOL gbDosServiceExist = 0;
BOOL gbVarsServiceExist = 0;

DWORD gbMaxQuestCount = 5;
BOOL gbInfTalkProtection = 0;
BOOL gbAddPermanent = 0;
BOOL gbHandleAuthReq = 1;
BOOL gbSpecialGroup = 0;
BOOL gbHideContacts = 1;
BOOL gbIgnoreContacts = 0;
BOOL gbExclude = 1;
BOOL gbDelExcluded = 0;
BOOL gbDelAllTempory = 0;
BOOL gbHistoryLog = 0;
BOOL gbCaseInsensitive = 0;
BOOL gbRegexMatch = 0;
BOOL gbInvisDisable = 0;
BOOL gbIgnoreURL = 1;
BOOL gbLogToFile=0;
BOOL gbAutoAuth=0;
BOOL gbAutoAddToServerList=0;
BOOL gbAutoReqAuth=1;
BOOL gbMathExpression = 0;

HANDLE hStopSpamLogDirH = nullptr;

wstring gbSpammersGroup = L"Spammers";
wstring gbAutoAuthGroup	= L"NotSpammers";

wstring gbQuestion;
wstring gbAnswer;
wstring gbCongratulation;
std::wstring gbAuthRepl;
extern const wchar_t *defQuestion, *defCongrats, *defAuthReply;
extern int RemoveTmp(WPARAM,LPARAM);

/////////////////////////////////////////////////////////////////////////////////////////
// returns plugin's extended information

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {94CED94C-A94A-4BB1-ACBD-5CC6EBB689D4}
	{0x94ced94c, 0xa94a, 0x4bb1, {0xac, 0xbd, 0x5c, 0xc6, 0xeb, 0xb6, 0x89, 0xd4}}
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(MODULENAME, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

extern wstring DBGetContactSettingStringPAN(MCONTACT hContact, char const * szModule, char const * szSetting, wstring errorValue);

void InitVars()
{
	gbSpammersGroup = DBGetContactSettingStringPAN(NULL, MODULENAME, "SpammersGroup", L"Spammers");
	gbAnswer = DBGetContactSettingStringPAN(NULL, MODULENAME, "answer", L"nospam");
	gbInfTalkProtection = db_get_b(NULL, MODULENAME, "infTalkProtection", 0);
	gbAddPermanent = db_get_b(NULL, MODULENAME, "addPermanent", 0);
	gbMaxQuestCount = db_get_dw(NULL, MODULENAME, "maxQuestCount", 5);
	gbHandleAuthReq = db_get_b(NULL, MODULENAME, "handleAuthReq", 1);
	gbQuestion = DBGetContactSettingStringPAN(NULL, MODULENAME, "question", TranslateW(defQuestion));
	gbAnswer = DBGetContactSettingStringPAN(NULL, MODULENAME, "answer", L"nospam");
	gbCongratulation = DBGetContactSettingStringPAN(NULL, MODULENAME, "congratulation", TranslateW(defCongrats));
	gbAuthRepl = DBGetContactSettingStringPAN(NULL, MODULENAME, "authrepl", TranslateW(defAuthReply));
	gbSpecialGroup = db_get_b(NULL, MODULENAME, "SpecialGroup", 0);
	gbHideContacts = db_get_b(NULL, MODULENAME, "HideContacts", 0);
	gbIgnoreContacts = db_get_b(NULL, MODULENAME, "IgnoreContacts", 0);
	gbExclude = db_get_b(NULL, MODULENAME, "ExcludeContacts", 1);
	gbDelExcluded = db_get_b(NULL, MODULENAME, "DelExcluded", 0);
	gbDelAllTempory = db_get_b(NULL, MODULENAME, "DelAllTempory", 0);
	gbCaseInsensitive = db_get_b(NULL, MODULENAME, "CaseInsensitive", 0);
	gbRegexMatch = db_get_b(NULL, MODULENAME, "RegexMatch", 0);
	gbInvisDisable = db_get_b(NULL, MODULENAME, "DisableInInvis", 0);
	gbIgnoreURL = db_get_b(NULL, MODULENAME, "IgnoreURL", 0);
	gbAutoAuthGroup = DBGetContactSettingStringPAN(NULL, MODULENAME, "AutoAuthGroup", L"Not Spammers");
	gbAutoAuth = db_get_b(NULL, MODULENAME, "AutoAuth", 0);
	gbAutoAddToServerList = db_get_b(NULL, MODULENAME, "AutoAddToServerList", 0);
	gbAutoReqAuth = db_get_b(NULL, MODULENAME, "AutoReqAuth", 0);
	gbLogToFile = db_get_b(NULL, MODULENAME, "LogSpamToFile", 0);
	gbHistoryLog = db_get_b(NULL, MODULENAME, "HistoryLog", 0);
	gbMathExpression = db_get_b(NULL, MODULENAME, "MathExpression", 0);
}

static int OnSystemModulesLoaded(WPARAM, LPARAM)
{
	if (ServiceExists(MS_VARS_FORMATSTRING))
		gbVarsServiceExist = TRUE;

	InitVars();
	if(gbDelAllTempory || gbDelExcluded)
		mir_forkthread(&CleanThread);
	
	// Folders plugin support
	hStopSpamLogDirH = FoldersRegisterCustomPathT(LPGEN("StopSpam"), LPGEN("StopSpam Logs"), FOLDER_LOGS);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Load()
{
	CreateServiceFunction("/RemoveTmp", (MIRANDASERVICE)RemoveTmp);
	HookEvent(ME_SYSTEM_MODULESLOADED, OnSystemModulesLoaded);
	HookEvent(ME_DB_EVENT_ADDED, OnDbEventAdded);
	HookEvent(ME_DB_EVENT_FILTER_ADD, OnDbEventFilterAdd);
	HookEvent(ME_DB_CONTACT_SETTINGCHANGED, OnDbContactSettingChanged);
	HookEvent(ME_OPT_INITIALISE, OnOptInit);

	CMenuItem mi(&g_plugin);
	SET_UID(mi, 0x60ce7660, 0x5a5, 0x4234, 0x99, 0xb6, 0x55, 0x21, 0xed, 0xa0, 0xb8, 0x32);
	mi.position = -0x7FFFFFFF;
	mi.hIcolibItem = Skin_LoadIcon(SKINICON_OTHER_MIRANDA);
	mi.name.a = LPGEN("Remove Temporary Contacts");
	mi.pszService = "/RemoveTmp";
	Menu_AddMainMenuItem(&mi);

	return 0;
}
