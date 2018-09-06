#include "stdafx.h"

static UINT_PTR	timer_id = 0;
volatile long m_state = 0;

LRESULT CALLBACK DlgProcPopup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		{
			wchar_t* ptszPath = (wchar_t*)PUGetPluginData(hWnd);
			if (ptszPath != nullptr)
				ShellExecute(nullptr, L"open", ptszPath, nullptr, nullptr, SW_SHOW);

			PUDeletePopup(hWnd);
			break;
		}
	case WM_CONTEXTMENU:
		PUDeletePopup(hWnd);
		break;
	case UM_FREEPLUGINDATA:
		mir_free(PUGetPluginData(hWnd));
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowPopup(wchar_t* ptszText, wchar_t* ptszHeader, wchar_t* ptszPath)
{
	POPUPDATAT ppd = { 0 };

	wcsncpy_s(ppd.lptzText, ptszText, _TRUNCATE);
	wcsncpy_s(ppd.lptzContactName, ptszHeader, _TRUNCATE);
	if (ptszPath != nullptr)
		ppd.PluginData = (void*)mir_wstrdup(ptszPath);
	ppd.PluginWindowProc = DlgProcPopup;
	ppd.lchIcon = IcoLib_GetIcon(iconList[0].szName);

	PUAddPopupT(&ppd);
}

INT_PTR CALLBACK DlgProcProgress(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
		break;
	case WM_COMMAND:
		if (HIWORD(wParam) != BN_CLICKED || LOWORD(wParam) != IDCANCEL)
			break;
		// in the progress dialog, use the user data to indicate that the user has pressed cancel
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 1);
		return TRUE;
		break;
	}
	return FALSE;
}

wchar_t* DoubleSlash(wchar_t *sorce)
{
	wchar_t *ret, *r, *s;

	ret = (wchar_t*)mir_alloc((MAX_PATH * sizeof(wchar_t)));
	if (ret == nullptr)
		return nullptr;
	for (s = sorce, r = ret; *s && (r - ret) < (MAX_PATH - 1); s++, r++) {
		if (*s != '\\')
			*r = *s;
		else {
			*r = '\\';
			r++;
			*r = '\\';
		}
	}
	r[0] = 0;
	return ret;
}

bool MakeZip_Dir(LPCWSTR szDir, LPCWSTR pwszProfile, LPCWSTR szDest, LPCWSTR pwszBackupFolder, HWND progress_dialog)
{
	HWND hProgBar = GetDlgItem(progress_dialog, IDC_PROGRESS);
	size_t count = 0, folderNameLen = mir_wstrlen(pwszBackupFolder);
	OBJLIST<ZipFile> lstFiles(15);

	wchar_t wszTempName[MAX_PATH];
	if (!GetTempPathW(_countof(wszTempName), wszTempName))
		return false;

	if (!GetTempFileNameW(wszTempName, L"mir_backup_", 0, wszTempName))
		return false;

	if (db_get_current()->Backup(wszTempName))
		return false;

	lstFiles.insert(new ZipFile(wszTempName, pwszProfile));

	CMStringW wszProfile;
	wszProfile.Format(L"%s\\%s", szDir, pwszProfile);

	for (auto it = fs::recursive_directory_iterator(fs::path(szDir)); it != fs::recursive_directory_iterator(); ++it) {
		const auto& file = it->path();
		if (fs::is_directory(file))
			continue;

		const std::wstring &filepath = file.wstring();
		if (wszProfile == filepath.c_str())
			continue;

		if (filepath.find(szDest) != std::wstring::npos || !mir_wstrncmpi(filepath.c_str(), pwszBackupFolder, folderNameLen))
			continue;

		const std::wstring rpath = filepath.substr(filepath.find(szDir) + mir_wstrlen(szDir) + 1);
		lstFiles.insert(new ZipFile(filepath, rpath));
		count++;
	}

	if (count == 0)
		return 1;

	CreateZipFile(szDest, lstFiles, [&](size_t i)->bool {
		SendMessage(hProgBar, PBM_SETPOS, (WPARAM)(100 * i / count), 0);
		return GetWindowLongPtr(progress_dialog, GWLP_USERDATA) != 1;
	});

	DeleteFileW(wszTempName);
	return 1;
}

bool MakeZip(wchar_t *tszDest, wchar_t *dbname, HWND progress_dialog)
{
	HWND hProgBar = GetDlgItem(progress_dialog, IDC_PROGRESS);

	wchar_t wszTempName[MAX_PATH];
	if (!GetTempPathW(_countof(wszTempName), wszTempName))
		return false;

	if (!GetTempFileNameW(wszTempName, L"mir_backup_", 0, wszTempName))
		return false;

	if (db_get_current()->Backup(wszTempName))
		return false;

	OBJLIST<ZipFile> lstFiles(1);
	lstFiles.insert(new ZipFile(wszTempName, dbname));

	CreateZipFile(tszDest, lstFiles, [&](size_t)->bool { SendMessage(hProgBar, PBM_SETPOS, (WPARAM)(100), 0); return true; });
	DeleteFileW(wszTempName);
	return true;
}


struct backupFile
{
	wchar_t Name[MAX_PATH];
	FILETIME CreationTime;
};

int Comp(const void *i, const void *j)
{
	backupFile *pi = (backupFile*)i;
	backupFile *pj = (backupFile*)j;

	if (pi->CreationTime.dwHighDateTime > pj->CreationTime.dwHighDateTime ||
		(pi->CreationTime.dwHighDateTime == pj->CreationTime.dwHighDateTime && pi->CreationTime.dwLowDateTime > pj->CreationTime.dwLowDateTime))
		return -1;
	else
		return 1;
}

int RotateBackups(wchar_t *backupfolder, wchar_t *dbname)
{
	if (options.num_backups == 0) // Rotation disabled?
		return 0; 

	backupFile *bf = nullptr, *bftmp;

	wchar_t backupfolderTmp[MAX_PATH];
	mir_snwprintf(backupfolderTmp, L"%s\\%s*.%s", backupfolder, dbname, options.use_zip ? L"zip" : L"dat");

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(backupfolderTmp, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	int i = 0;
	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		bftmp = (backupFile*)mir_realloc(bf, ((i + 1) * sizeof(backupFile)));
		if (bftmp == nullptr)
			goto err_out;
		bf = bftmp;
		wcsncpy_s(bf[i].Name, FindFileData.cFileName, _TRUNCATE);
		bf[i].CreationTime = FindFileData.ftCreationTime;
		i++;
	} while (FindNextFile(hFind, &FindFileData));
	if (i > 0)
		qsort(bf, i, sizeof(backupFile), Comp); /* Sort the list of found files by date in descending order. */
	for (; i >= options.num_backups; i--) {
		mir_snwprintf(backupfolderTmp, L"%s\\%s", backupfolder, bf[(i - 1)].Name);
		DeleteFile(backupfolderTmp);
	}
err_out:
	FindClose(hFind);
	mir_free(bf);
	return 0;
}

int Backup(wchar_t *backup_filename)
{
	bool bZip = false;
	wchar_t dbname[MAX_PATH], dest_file[MAX_PATH];
	HWND progress_dialog = nullptr;

	Profile_GetNameW(_countof(dbname), dbname);

	wchar_t backupfolder[MAX_PATH];
	PathToAbsoluteW(VARSW(options.folder), backupfolder);

	// ensure the backup folder exists (either create it or return non-zero signifying error)
	int err = CreateDirectoryTreeW(backupfolder);
	if (err != ERROR_ALREADY_EXISTS && err != 0) {
		mir_free(backupfolder);
		return 1;
	}

	if (backup_filename == nullptr) {
		bZip = options.use_zip != 0;
		RotateBackups(backupfolder, dbname);

		SYSTEMTIME st;
		GetLocalTime(&st);

		wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD size = _countof(buffer);
		GetComputerName(buffer, &size);
		mir_snwprintf(dest_file, L"%s\\%s_%02d.%02d.%02d@%02d-%02d-%02d_%s.%s", backupfolder, dbname, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, buffer, bZip ? L"zip" : L"dat");
	}
	else {
		wcsncpy_s(dest_file, backup_filename, _TRUNCATE);
		if (!mir_wstrcmp(wcsrchr(backup_filename, '.'), L".zip"))
			bZip = true;
	}
	if (!options.disable_popups)
		ShowPopup(dbname, TranslateT("Backup in progress"), nullptr);

	if (!options.disable_progress)
		progress_dialog = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_COPYPROGRESS), nullptr, DlgProcProgress);

	SetDlgItemText(progress_dialog, IDC_PROGRESSMESSAGE, TranslateT("Copying database file..."));

	BOOL res;
	if (bZip) {
		res = options.backup_profile
			? MakeZip_Dir(VARSW(L"%miranda_userdata%"), dbname, dest_file, backupfolder, progress_dialog)
			: MakeZip(dest_file, dbname, progress_dialog);
	}
	else res = db_get_current()->Backup(dest_file) == ERROR_SUCCESS;

	if (res) {
		if (!bZip) { // Set the backup file to the current time for rotator's correct  work
			SYSTEMTIME st;
			GetSystemTime(&st);

			HANDLE hFile = CreateFile(dest_file, FILE_WRITE_ATTRIBUTES, NULL, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			FILETIME ft;
			SystemTimeToFileTime(&st, &ft);
			SetFileTime(hFile, nullptr, nullptr, &ft);
			CloseHandle(hFile);
		}
		SendDlgItemMessage(progress_dialog, IDC_PROGRESS, PBM_SETPOS, (WPARAM)(100), 0);
		UpdateWindow(progress_dialog);
		db_set_dw(0, MODULENAME, "LastBackupTimestamp", (DWORD)time(0));

		if (options.use_cloudfile) {
			CFUPLOADDATA ui = { options.cloudfile_service, dest_file, L"Backups" };
			if (CallService(MS_CLOUDFILE_UPLOAD, (LPARAM)&ui))
				ShowPopup(TranslateT("Uploading to cloud failed"), TranslateT("Error"), nullptr);
		}

		if (!options.disable_popups) {
			size_t dest_file_len = mir_wstrlen(dest_file);
			wchar_t *puText;

			if (dest_file_len > 50) {
				size_t i;
				puText = (wchar_t*)mir_alloc(sizeof(wchar_t) * (dest_file_len + 2));
				for (i = (dest_file_len - 1); dest_file[i] != '\\'; i--)
					;
				//wcsncpy_s(dest_file, backup_filename, _TRUNCATE);
				mir_wstrncpy(puText, dest_file, (i + 2));
				mir_wstrcat(puText, L"\n");
				mir_wstrcat(puText, (dest_file + i + 1));
			}
			else
				puText = mir_wstrdup(dest_file);

			// Now we need to know, which folder we made a backup. Let's break unnecessary variables :)
			while (dest_file[--dest_file_len] != L'\\')
				;
			dest_file[dest_file_len] = 0;
			ShowPopup(puText, TranslateT("Database backed up"), dest_file);
			mir_free(puText);
		}
	}
	else
		DeleteFile(dest_file);

	DestroyWindow(progress_dialog);
	return 0;
}

void BackupThread(void *backup_filename)
{
	Backup((wchar_t*)backup_filename);
	InterlockedExchange(&m_state, 0); /* Backup done. */
	mir_free(backup_filename);
}

void BackupStart(wchar_t *backup_filename)
{
	wchar_t *tm = nullptr;
	LONG cur_state;

	cur_state = InterlockedCompareExchange(&m_state, 1, 0);
	if (cur_state != 0) { /* Backup allready in process. */
		ShowPopup(TranslateT("Database back up in process..."), TranslateT("Error"), nullptr); /* Show error message :) */
		return;
	}
	if (backup_filename != nullptr)
		tm = mir_wstrdup(backup_filename);
	if (mir_forkthread(BackupThread, tm) == INVALID_HANDLE_VALUE) {
		InterlockedExchange(&m_state, 0); /* Backup done. */
		mir_free(tm);
	}
}

VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	time_t t = time(0);
	time_t diff = t - (time_t)db_get_dw(0, "AutoBackups", "LastBackupTimestamp", 0);
	if (diff > (time_t)(options.period * (options.period_type == PT_MINUTES ? 60 : (options.period_type == PT_HOURS ? (60 * 60) : (60 * 60 * 24)))))
		BackupStart(nullptr);
}

int SetBackupTimer(void)
{
	if (timer_id != 0) {
		KillTimer(nullptr, timer_id);
		timer_id = 0;
	}
	if (options.backup_types & BT_PERIODIC)
		timer_id = SetTimer(nullptr, 0, (1000 * 60), TimerProc);
	return 0;
}
