#include "stdafx.h"

Options::Options() :
	backup_types(MODULENAME, "BackupType", BT_PERIODIC),
	period(MODULENAME, "Period", 1),
	period_type(MODULENAME, "PeriodType", PT_DAYS),
	num_backups(MODULENAME, "NumBackups", 3),
	disable_progress(MODULENAME, "NoProgress", 0),
	disable_popups(MODULENAME, "NoPopups", 0),
	use_zip(MODULENAME, "UseZip", 0),
	backup_profile(MODULENAME, "BackupProfile", 0),
	use_cloudfile(MODULENAME, "UseCloudFile", 0),
	cloudfile_service(MODULENAME, "CloudFileService", nullptr)
{
}

Options options;

/////////////////////////////////////////////////////////////////////////////////////////

class COptionsDlg : public CDlgBase
{
	void SetDialogState()
	{
		CCtrlEdit &periodText = *(CCtrlEdit*)FindControl(IDC_ED_PERIOD);
		CCtrlEdit &numBackupsText = *(CCtrlEdit*)FindControl(IDC_ED_NUMBACKUPS);
		
		bool bEnabled = !m_disable.IsChecked();
		m_backupOnStart.Enable(bEnabled);
		m_backupOnExit.Enable(bEnabled);
		m_backupPeriodic.Enable(bEnabled);
		numBackupsText.Enable(bEnabled);
		m_numBackups.Enable(bEnabled);
		m_backup.Enable(bEnabled);
		m_folder.Enable(bEnabled);
		m_browseFolder.Enable(bEnabled);
		m_foldersPageLink.Enable(bEnabled);
		m_disableProgress.Enable(bEnabled);
		m_disablePopups.Enable(bEnabled);
		m_useZip.Enable(bEnabled);
		periodText.Enable(bEnabled);
		m_period.Enable(bEnabled);
		m_periodType.Enable(bEnabled);
		m_backupProfile.Enable(bEnabled);
		m_useCloudFile.Enable(bEnabled);

		if (!bEnabled) {
			m_cloudFileService.Disable();

			m_backupOnStart.SetState(FALSE);
			m_backupOnExit.SetState(FALSE);
			m_backupPeriodic.SetState(FALSE);
		}
		else {
			m_cloudFileService.Enable(m_useCloudFile.IsChecked());
			UseZip_OnChange(0);

			BYTE backupTypes = options.backup_types;
			if (backupTypes == BT_DISABLED)
				backupTypes = options.backup_types.Default();
			m_backupOnStart.SetState(backupTypes & BT_START ? TRUE : FALSE);
			m_backupOnExit.SetState(backupTypes & BT_EXIT ? TRUE : FALSE);
			m_backupPeriodic.SetState(backupTypes & BT_PERIODIC ? TRUE : FALSE);
		}
	}

	void CreateToolTip(LPTSTR ptszText, LPTSTR ptszTitle)
	{
		HWND hwndFolder = m_folder.GetHwnd();

		m_hPathTip = CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS, nullptr,
			(WS_POPUP | TTS_NOPREFIX),
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			hwndFolder, nullptr, g_plugin.getInst(), nullptr);

		if (m_hPathTip == nullptr) {
			return;
		}

		SetWindowPos(m_hPathTip, HWND_TOPMOST, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));

		TOOLINFO ti = {};
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS | TTF_CENTERTIP;
		ti.hwnd = hwndFolder;
		ti.hinst = g_plugin.getInst();
		ti.lpszText = ptszText;
		GetClientRect(hwndFolder, &ti.rect);
		ti.rect.left = -80;

		SendMessage(m_hPathTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
		SendMessage(m_hPathTip, TTM_SETTITLE, 1, (LPARAM)ptszTitle);
		SendMessage(m_hPathTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)650);

		SetTimer(m_hwnd, 0, 3000, nullptr);
	}

	static int CALLBACK BrowseProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM)
	{
		switch (uMsg) {
		case BFFM_INITIALIZED:
			wchar_t backupfolder[MAX_PATH];
			PathToAbsoluteW(VARSW(options.folder), backupfolder);
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)backupfolder);
			break;
		}
		return FALSE;
	}

	static int EnumCloudFileServices(const CFSERVICEINFO *serviceInfo, void *param)
	{
		CCtrlCombo &combo = *(CCtrlCombo*)param;
		int pos = combo.GetCount();
		combo.InsertString(serviceInfo->userName, pos, (LPARAM)serviceInfo->accountName);
		if (mir_strcmp(serviceInfo->accountName, options.cloudfile_service) == 0)
			combo.SetCurSel(pos);
		return 0;
	}

	CCtrlCheck m_disable;
	CCtrlCheck m_backupOnStart;
	CCtrlCheck m_backupOnExit;
	CCtrlCheck m_backupPeriodic;
	CCtrlButton m_backup;
	CCtrlSpin m_period;
	CCtrlCombo m_periodType;
	CCtrlEdit m_folder;
	CCtrlButton m_browseFolder;
	CCtrlHyperlink m_foldersPageLink;
	CCtrlSpin m_numBackups;
	CCtrlCheck m_disableProgress;
	CCtrlCheck m_disablePopups;
	CCtrlCheck m_useZip;
	CCtrlCheck m_backupProfile;
	CCtrlCheck m_useCloudFile;
	CCtrlCombo m_cloudFileService;

	HWND m_hPathTip;

public:
	COptionsDlg() :
		CDlgBase(g_plugin, IDD_OPTIONS),
		m_disable(this, IDC_RAD_DISABLED), m_backupOnStart(this, IDC_RAD_START),
		m_backupOnExit(this, IDC_RAD_EXIT), m_backupPeriodic(this, IDC_RAD_PERIODIC),
		m_backup(this, IDC_BUT_NOW), m_backupProfile(this, IDC_BACKUPPROFILE),
		m_period(this, SPIN_PERIOD), m_periodType(this, IDC_PT),
		m_folder(this, IDC_ED_FOLDER), m_browseFolder(this, IDC_BUT_BROWSE),
		m_foldersPageLink(this, IDC_LNK_FOLDERS, nullptr), m_numBackups(this, SPIN_NUMBACKUPS),
		m_disableProgress(this, IDC_CHK_NOPROG), m_disablePopups(this, IDC_CHK_NOPOPUP),
		m_useZip(this, IDC_CHK_USEZIP), m_useCloudFile(this, IDC_CLOUDFILE),
		m_cloudFileService(this, IDC_CLOUDFILESEVICE)
	{
		CreateLink(m_period, options.period);
		CreateLink(m_numBackups, options.num_backups);
		CreateLink(m_disableProgress, options.disable_progress);
		CreateLink(m_disablePopups, options.disable_popups);
		CreateLink(m_useZip, options.use_zip);
		CreateLink(m_backupProfile, options.backup_profile);
		CreateLink(m_useCloudFile, options.use_cloudfile);

		m_disable.OnChange = Callback(this, &COptionsDlg::Disable_OnChange);
		m_backupOnStart.OnChange = Callback(this, &COptionsDlg::BackupType_OnChange);
		m_backupOnExit.OnChange = Callback(this, &COptionsDlg::BackupType_OnChange);
		m_backupPeriodic.OnChange = Callback(this, &COptionsDlg::BackupType_OnChange);
		m_useCloudFile.OnChange = Callback(this, &COptionsDlg::UseCloudFile_OnChange);
		m_useZip.OnChange = Callback(this, &COptionsDlg::UseZip_OnChange);

		m_backup.OnClick = Callback(this, &COptionsDlg::Backup_OnClick);
		m_browseFolder.OnClick = Callback(this, &COptionsDlg::BrowseFolder_OnClick);
		m_foldersPageLink.OnClick = Callback(this, &COptionsDlg::FoldersPageLink_OnClick);
	}

	bool OnInitDialog() override
	{
		m_disable.SetState(options.backup_types == BT_DISABLED);
		m_backupOnStart.SetState(options.backup_types & BT_START ? TRUE : FALSE);
		m_backupOnExit.SetState(options.backup_types & BT_EXIT ? TRUE : FALSE);
		m_backupPeriodic.SetState(options.backup_types & BT_PERIODIC ? TRUE : FALSE);

		m_period.SetRange(60, 1);

		m_numBackups.SetRange(9999, 1);
		m_numBackups.SetPosition(options.num_backups);

		m_periodType.AddString(TranslateT("days"));
		m_periodType.AddString(TranslateT("hours"));
		m_periodType.AddString(TranslateT("minutes"));
		m_periodType.SetCurSel(options.period_type);

		if (ServiceExists(MS_FOLDERS_GET_PATH)) {
			m_folder.Hide();
			m_browseFolder.Hide();
			m_foldersPageLink.Show();
		}
		else {
			m_folder.SetText(options.folder);

			wchar_t tszTooltipText[4096];
			mir_snwprintf(tszTooltipText, L"%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s",
				L"%miranda_path%", TranslateT("path to Miranda root folder"),
				L"%miranda_profilesdir%", TranslateT("path to folder containing Miranda profiles"),
				L"%miranda_profilename%", TranslateT("name of current Miranda profile (filename, without extension)"),
				L"%miranda_userdata%", TranslateT("will return parsed string %miranda_profilesdir%\\%miranda_profilename%"),
				L"%appdata%", TranslateT("same as environment variable %APPDATA% for currently logged-on Windows user"),
				L"%username%", TranslateT("username for currently logged-on Windows user"),
				L"%mydocuments%", TranslateT("\"My Documents\" folder for currently logged-on Windows user"),
				L"%desktop%", TranslateT("\"Desktop\" folder for currently logged-on Windows user"),
				L"%xxxxxxx%", TranslateT("any environment variable defined in current Windows session (like %systemroot%, %allusersprofile%, etc.)"));
			CreateToolTip(tszTooltipText, TranslateT("Variables"));
		}

		m_disablePopups.Show(ServiceExists(MS_POPUP_ADDPOPUPT));

		m_useCloudFile.Enable(ServiceExists(MS_CLOUDFILE_UPLOAD));
		if (ServiceExists(MS_CLOUDFILE_ENUMSERVICES)) {
			m_cloudFileService.Enable();
			CallService(MS_CLOUDFILE_ENUMSERVICES, (WPARAM)&COptionsDlg::EnumCloudFileServices, (LPARAM)&m_cloudFileService);
		}

		SetDialogState();
		return true;
	}

	bool OnApply() override
	{
		BYTE backupTypes = BT_DISABLED;

		if (m_backupOnStart.IsChecked())
			backupTypes |= BT_START;
		else
			backupTypes &= ~BT_START;

		if (m_backupOnExit.IsChecked())
			backupTypes |= BT_EXIT;
		else
			backupTypes &= ~BT_EXIT;

		if (m_backupPeriodic.IsChecked())
			backupTypes |= BT_PERIODIC;
		else
			backupTypes &= ~BT_PERIODIC;

		options.backup_types = backupTypes;

		SetBackupTimer();

		options.period_type = m_periodType.GetCurSel();

		ptrW folder(m_folder.GetText());

		wchar_t backupfolder[MAX_PATH];
		PathToAbsoluteW(VARSW(folder), backupfolder);
		int err = CreateDirectoryTreeW(backupfolder);
		if (err != ERROR_ALREADY_EXISTS && err != 0) {
			wchar_t msg[512];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, msg, 512, nullptr);
			MessageBox(nullptr, msg, TranslateT("Error creating backup folder"), MB_OK | MB_ICONERROR);
			return false;
		}

		wcsncpy_s(options.folder, folder, _TRUNCATE);
		db_set_ws(0, MODULENAME, "Folder", folder);

		int currentService = m_cloudFileService.GetCurSel();
		options.cloudfile_service = currentService >= 0
			? (char*)m_cloudFileService.GetItemData(currentService)
			: nullptr;
		return true;
	}

	void OnTimer(CTimer*) override
	{
		if (IsWindow(m_hPathTip))
			KillTimer(m_hwnd, 4); // It will prevent tooltip autoclosing
	}

	void OnDestroy() override
	{
		if (m_hPathTip) {
			KillTimer(m_hwnd, 0);
			DestroyWindow(m_hPathTip);
			m_hPathTip = nullptr;
		}
	}

	void Disable_OnChange(CCtrlBase*)
	{
		SetDialogState();
	}

	void BackupType_OnChange(CCtrlBase*)
	{
		if (!m_backupOnStart.IsChecked() &&
			!m_backupOnExit.IsChecked() &&
			!m_backupPeriodic.IsChecked()) {
			m_disable.SetState(TRUE);
			SetDialogState();
		}
	}

	void UseCloudFile_OnChange(CCtrlBase*)
	{
		m_cloudFileService.Enable(m_useCloudFile.IsChecked());
	}

	void UseZip_OnChange(CCtrlCheck*)
	{
		m_backupProfile.Enable(m_useZip.GetState());
	}

	void Backup_OnClick(CCtrlButton*)
	{
		BackupStart(nullptr);
	}

	void BrowseFolder_OnClick(CCtrlButton*)
	{
		wchar_t folder_buff[MAX_PATH] = { 0 };

		BROWSEINFO bi;
		bi.hwndOwner = m_hwnd;
		bi.pidlRoot = nullptr;
		bi.pszDisplayName = folder_buff;
		bi.lpszTitle = TranslateT("Select backup folder");
		bi.ulFlags = BIF_NEWDIALOGSTYLE;
		bi.lpfn = BrowseProc;
		bi.lParam = 0;
		bi.iImage = 0;

		LPCITEMIDLIST pidl = SHBrowseForFolder(&bi);
		if (pidl != nullptr) {
			SHGetPathFromIDList(pidl, folder_buff);
			m_folder.SetText(folder_buff);
			CoTaskMemFree((void*)pidl);
		}
	}

	void FoldersPageLink_OnClick(CCtrlHyperlink*)
	{
		g_plugin.openOptions(L"Customize", L"Folders");
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// module entry point

int OptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.flags = ODPF_BOLDGROUPS;
	odp.szTitle.a = LPGEN("Automatic backups");
	odp.szGroup.a = LPGEN("Database");
	odp.pDialog = new COptionsDlg();
	g_plugin.addOptions(wParam, &odp);
	return 0;
}
