#include "StdAfx.h"

static CModuleInfo::TXMLEnginePtr g_pXMLEngine;
static CModuleInfo::THTMLEnginePtr g_pHTMLEngine;
static mir_cs g_lmParsers;

CModuleInfo::CModuleInfo()
{
}

CModuleInfo::~CModuleInfo()
{
}

CModuleInfo& CModuleInfo::GetInstance()
{
	static CModuleInfo mi;
	return mi;
}

MWindowList CModuleInfo::GetWindowList(const std::string& rsKey, bool bAllocateIfNonExist /*= true*/)
{
	MWindowList hResult = nullptr;
	THandles::const_iterator i = m_ahWindowLists.find(rsKey);
	if (i != m_ahWindowLists.end()) {
		hResult = i->second;
	}
	else if (bAllocateIfNonExist) {
		hResult = WindowList_Create();
		if (hResult)
			m_ahWindowLists.insert(std::make_pair(rsKey, hResult));
	}

	return hResult;
}

void CModuleInfo::OnMirandaShutdown()
{
	BOOST_FOREACH(THandles::value_type p, m_ahWindowLists)
	{
		WindowList_Broadcast(p.second, WM_CLOSE, 0, 0);
	}
}

CModuleInfo::TCurrencyRatesProvidersPtr CModuleInfo::GetCurrencyRateProvidersPtr()
{
	static TCurrencyRatesProvidersPtr pProviders(new CCurrencyRatesProviders);
	return pProviders;
}

CModuleInfo::TXMLEnginePtr CModuleInfo::GetXMLEnginePtr()
{
	if (!g_pXMLEngine) {
		mir_cslock lck(g_lmParsers);
		if (!g_pXMLEngine)
			g_pXMLEngine = TXMLEnginePtr(new CXMLEngineMI);
	}

	return g_pXMLEngine;
}

CModuleInfo::THTMLEnginePtr CModuleInfo::GetHTMLEngine()
{
	if (!g_pHTMLEngine) {
		mir_cslock lck(g_lmParsers);
		if (!g_pHTMLEngine)
			g_pHTMLEngine = THTMLEnginePtr(new CHTMLEngineMS);
	}

	return g_pHTMLEngine;
}

void CModuleInfo::SetHTMLEngine(THTMLEnginePtr pEngine)
{
	g_pHTMLEngine = pEngine;
}

bool CModuleInfo::Verify()
{
	INITCOMMONCONTROLSEX icc = { 0 };
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
	if (FALSE == ::InitCommonControlsEx(&icc))
		return false;

	if (!GetXMLEnginePtr()) {
		CurrencyRates_MessageBox(nullptr, TranslateT("Miranda could not load CurrencyRates plugin. XML parser is missing."), MB_OK | MB_ICONERROR);
		return false;
	}

	if (!g_pHTMLEngine && (false == CHTMLParserMS::IsInstalled())) {
		CurrencyRates_MessageBox(nullptr,
			TranslateT("Miranda could not load CurrencyRates plugin. Microsoft HTML parser is missing."),
			MB_YESNO | MB_ICONQUESTION);
		return false;
	}

	return true;
}