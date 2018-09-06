#include "stdafx.h"

COptions::COptions()
	: CDlgBase(g_plugin, IDD_OPTIONS_MAIN),
	m_shortcut(this, IDC_SHORTCUT),
	m_preview(this, IDC_PREVIEW),
	m_enabled(this, IDC_CHECK_ENABLED)
{
	m_shortcut.OnClick = Callback(this, &COptions::Shortcut_OnClick);
	m_preview.OnClick = Callback(this, &COptions::Preview_OnClick);
	m_enabled.OnChange = Callback(this, &COptions::Enabled_OnChange);
}

bool COptions::OnInitDialog()
{
	m_enabled.SetState(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS));
	return true;
}

void COptions::Shortcut_OnClick(CCtrlBase*)
{
	if (FAILED(TryCreateShortcut()))
	{
		CallService(MS_POPUP_SHOWMESSAGEW, (WPARAM)TranslateT("Failed to create shortcut"), (LPARAM)SM_ERROR);
		return;
	}
	CallService(MS_POPUP_SHOWMESSAGEW, (WPARAM)TranslateT("Shortcut was added to the start menu"), (LPARAM)SM_NOTIFY);
}

void COptions::Preview_OnClick(CCtrlBase*)
{
	CallService(MS_POPUP_SHOWMESSAGEW, (WPARAM)TranslateT("Information"), (LPARAM)SM_NOTIFY);
	CallService(MS_POPUP_SHOWMESSAGEW, (WPARAM)TranslateT("Warning"), (LPARAM)SM_WARNING);
	CallService(MS_POPUP_SHOWMESSAGEW, (WPARAM)TranslateT("Error"), (LPARAM)SM_ERROR);
}

void COptions::Enabled_OnChange(CCtrlCheck* chk)
{
	if (chk->GetState())
		CallService(MS_POPUP_QUERY, PUQS_ENABLEPOPUPS);
	else 
		CallService(MS_POPUP_QUERY, PUQS_DISABLEPOPUPS);
}

/////////////////////////////////////////////////////////////////////////////////

int OnOptionsInitialized(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.szTitle.w = L"Popups"; //_T(MODULENAME);
	odp.flags = ODPF_BOLDGROUPS | ODPF_UNICODE;
	odp.position = -790000000;
	odp.szTab.w = _T(MODULENAME);
	odp.pDialog = new COptions();
	g_plugin.addOptions(wParam, &odp);

	return 0;
}

