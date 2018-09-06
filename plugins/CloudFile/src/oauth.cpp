#include "stdafx.h"

COAuthDlg::COAuthDlg(CCloudService *service, const char *authUrl, CCloudService::MyThreadFunc requestAccessTokenThread)
	: CDlgBase(g_plugin, IDD_OAUTH), m_service(service),
	m_requestAccessTokenThread(requestAccessTokenThread),
	m_authorize(this, IDC_OAUTH_AUTHORIZE, authUrl),
	m_code(this, IDC_OAUTH_CODE), m_ok(this, IDOK)
{
	m_autoClose = CLOSE_ON_CANCEL;
	m_code.OnChange = Callback(this, &COAuthDlg::Code_OnChange);
	m_ok.OnClick = Callback(this, &COAuthDlg::Ok_OnClick);
}

bool COAuthDlg::OnInitDialog()
{
	CCtrlLabel &ctrl = *(CCtrlLabel*)FindControl(IDC_AUTH_TEXT);
	ptrW format(ctrl.GetText());
	wchar_t text[MAX_PATH];
	mir_snwprintf(text, (const wchar_t*)format, m_service->GetUserName());
	ctrl.SetText(text);
	return true;
}

void COAuthDlg::Code_OnChange(CCtrlBase*)
{
	ptrA requestToken(m_code.GetTextA());
	m_ok.Enable(mir_strlen(requestToken) != 0);
}

void COAuthDlg::Ok_OnClick(CCtrlButton*)
{
	m_service->ForkThread(m_requestAccessTokenThread, m_hwnd);
}