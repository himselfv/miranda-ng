#include "stdafx.h"

CMPlugin g_plugin;

HANDLE hFolder;
char g_szMirVer[100];

/////////////////////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {81C220A6-0226-4Ad6-BFCA-217B17A16053}
	{ 0x81c220a6, 0x226, 0x4ad6, { 0xbf, 0xca, 0x21, 0x7b, 0x17, 0xa1, 0x60, 0x53 } }
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(MODULENAME, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR ABService(WPARAM, LPARAM)
{
	BackupStart(nullptr);
	return 0;
}

static INT_PTR DBSaveAs(WPARAM, LPARAM)
{
	wchar_t fname_buff[MAX_PATH], tszFilter[200];
	Profile_GetNameW(_countof(fname_buff), fname_buff);

	mir_snwprintf(tszFilter, L"%s (*.dat)%c*.dat%c%s (*.zip)%c*.zip%c%s (*.*)%c*%c",
		TranslateT("Miranda NG databases"), 0, 0,
		TranslateT("Compressed Miranda NG databases"), 0, 0,
		TranslateT("All files"), 0, 0);

	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = fname_buff;
	ofn.nMaxFile = _countof(fname_buff);
	ofn.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	ofn.lpstrFilter = tszFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrDefExt = L"dat";

	if (GetSaveFileName(&ofn))
		BackupStart(fname_buff);
	return 0;
}

static int FoldersGetBackupPath(WPARAM, LPARAM)
{
	FoldersGetCustomPathT(hFolder, options.folder, _countof(options.folder), DIR SUB_DIR);
	return 0;
}

static int ModulesLoad(WPARAM, LPARAM)
{
	CMenuItem mi(&g_plugin);
	mi.root = g_plugin.addRootMenu(MO_MAIN, LPGENW("Database"), 500100000);

	SET_UID(mi, 0x1439b1db, 0x7d95, 0x495b, 0xbf, 0x5, 0x3d, 0x21, 0xc1, 0xeb, 0xf7, 0x58);
	mi.name.a = LPGEN("Backup profile");
	mi.pszService = MS_AB_BACKUP;
	mi.hIcolibItem = iconList[0].hIcolib;
	mi.position = 500100000;
	Menu_AddMainMenuItem(&mi);

	SET_UID(mi, 0x15e93f83, 0x930e, 0x4cf9, 0x91, 0x31, 0x4c, 0x2b, 0xb, 0xaa, 0xd2, 0x18);
	mi.name.a = LPGEN("Save profile as...");
	mi.pszService = MS_AB_SAVEAS;
	mi.hIcolibItem = iconList[1].hIcolib;
	mi.position = 500100001;
	Menu_AddMainMenuItem(&mi);

	if (hFolder = FoldersRegisterCustomPathT(LPGEN("Database backups"), LPGEN("Backup folder"), DIR SUB_DIR)) {
		HookEvent(ME_FOLDERS_PATH_CHANGED, FoldersGetBackupPath);
		FoldersGetBackupPath(0, 0);
	}
	else {
		DBVARIANT dbv;
		if (!db_get_ws(0, MODULENAME, "Folder", &dbv)) {
			wcsncpy_s(options.folder, dbv.pwszVal, _TRUNCATE);
			db_free(&dbv);
		}
		else mir_snwprintf(options.folder, L"%s%s", DIR, SUB_DIR);
	}

	if (options.backup_types & BT_START)
		BackupStart(nullptr);
	return 0;
}

// can't do this on unload, since other plugins will be have already been unloaded, but their hooks
// for setting changed event not cleared. the backup on exit function will write to the db, calling those hooks.
static int PreShutdown(WPARAM, LPARAM)
{
	if (options.backup_types & BT_EXIT) {
		options.disable_popups = 1; // Don't try to show popups on exit
		BackupStart(nullptr);
	}
	return 0;
}

int CMPlugin::Load()
{
	Miranda_GetVersionText(g_szMirVer, sizeof(g_szMirVer));

	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoad);

	g_plugin.registerIcon(LPGEN("Database") "/" LPGEN("Database backups"), iconList);

	CreateServiceFunction(MS_AB_BACKUP, ABService);
	CreateServiceFunction(MS_AB_SAVEAS, DBSaveAs);

	HookEvent(ME_OPT_INITIALISE, OptionsInit);

	SetBackupTimer();
	return 0;
}
