#ifndef _HEADERS_H
#define _HEADERS_H

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <shlobj.h>
#include <time.h>
#include <vector>
#include <functional>
#include <filesystem>

namespace fs = std::experimental::filesystem;

#include <newpluginapi.h>
#include <m_clist.h>
#include <m_database.h>
#include <m_db_int.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_popup.h>
#include <m_icolib.h>
#include <m_autobackups.h>
#include <m_gui.h>

#include <m_folders.h>
#include <m_cloudfile.h>

#define MODULENAME "AutoBackups"

struct CMPlugin : public PLUGIN<CMPlugin>
{
	CMPlugin();

	int Load() override;
};

#include "options.h"
#include "resource.h"
#include "version.h"

#define SUB_DIR L"\\AutoBackups"
#define DIR L"%miranda_userdata%"

struct Options;
extern Options options;

int	SetBackupTimer(void);
int	OptionsInit(WPARAM wParam, LPARAM lParam);
void BackupStart(wchar_t *backup_filename);

struct ZipFile
{
	std::wstring sPath;
	std::wstring sZipPath;
	__forceinline ZipFile(const std::wstring &path, const std::wstring &zpath) : sPath(path), sZipPath(zpath) {}
};

int CreateZipFile(const wchar_t *szDestPath, OBJLIST<ZipFile> &lstFiles, const std::function<bool(size_t)> &fnCallback);

extern char g_szMirVer[];

static IconItem iconList[] = {
	{ LPGEN("Backup profile"),     "backup", IDI_BACKUP },
	{ LPGEN("Save profile as..."), "saveas", IDI_BACKUP }
};

#endif
