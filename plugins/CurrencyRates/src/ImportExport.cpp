#include "StdAfx.h"
#include "CurrencyRatesProviderCurrencyConverter.h"

LPCTSTR g_pszXmlValue = L"Value";
LPCTSTR g_pszXmlName = L"Name";
LPCTSTR g_pszXmlSetting = L"Setting";
LPCTSTR g_pszXmlModule = L"Module";
LPCTSTR g_pszXmlContact = L"Contact";
LPCTSTR g_pszXmlContacts = L"Contacts";
LPCTSTR g_pszXmlType = L"type";
LPCTSTR g_pszXmlTypeByte = L"byte";
LPCTSTR g_pszXmlTypeWord = L"word";
LPCTSTR g_pszXmlTypeDword = L"dword";
LPCTSTR g_pszXmlTypeAsciiz = L"asciiz";
LPCTSTR g_pszXmlTypeWchar = L"wchar";
LPCTSTR g_pszXmlTypeUtf8 = L"utf8";
LPCTSTR g_pszXmlTypeBlob = L"blob";

struct CEnumContext
{
	CModuleInfo::TXMLEnginePtr m_pXmlEngine;
	IXMLNode::TXMLNodePtr m_pNode;
	MCONTACT m_hContact;
	LPCSTR m_pszModule;
};

struct mir_safety_dbvar
{
	mir_safety_dbvar(DBVARIANT* p) : m_p(p) {}
	~mir_safety_dbvar() { db_free(m_p); }
	DBVARIANT* m_p;
};

static int enum_contact_settings(const char* szSetting, void *lp)
{
	CEnumContext* ctx = reinterpret_cast<CEnumContext*>(lp);

	DBVARIANT dbv;
	if (0 == db_get(ctx->m_hContact, ctx->m_pszModule, szSetting, &dbv)) {
		mir_safety_dbvar sdbvar(&dbv);

		tstring sType;
		tostringstream sValue;
		sValue.imbue(GetSystemLocale());

		switch (dbv.type) {
		case DBVT_BYTE:
			sValue << dbv.bVal;
			sType = g_pszXmlTypeByte;
			break;
		case DBVT_WORD:
			sValue << dbv.wVal;
			sType = g_pszXmlTypeWord;
			break;
		case DBVT_DWORD:
			sValue << dbv.dVal;
			sType = g_pszXmlTypeDword;
			break;
		case DBVT_ASCIIZ:
			sType = g_pszXmlTypeAsciiz;
			if (dbv.pszVal)
				sValue << dbv.pszVal;
			break;
		case DBVT_WCHAR:
			sType = g_pszXmlTypeWchar;
			if (dbv.pwszVal)
				sValue << dbv.pwszVal;
			break;
		case DBVT_UTF8:
			sType = g_pszXmlTypeUtf8;
			if (dbv.pszVal)
				sValue << dbv.pszVal;
			break;
		case DBVT_BLOB:
			sType = g_pszXmlTypeBlob;
			if (dbv.pbVal) {
				ptrA buf(mir_base64_encode(dbv.pbVal, dbv.cpbVal));
				if (buf)
					sValue << buf;
			}
			break;
		}

		IXMLNode::TXMLNodePtr pXmlSet = ctx->m_pXmlEngine->CreateNode(g_pszXmlSetting, tstring());
		if (pXmlSet) {
			IXMLNode::TXMLNodePtr pXmlName = ctx->m_pXmlEngine->CreateNode(g_pszXmlName, currencyrates_a2t(szSetting));

			IXMLNode::TXMLNodePtr pXmlValue = ctx->m_pXmlEngine->CreateNode(g_pszXmlValue, sValue.str());
			if (pXmlName && pXmlValue) {
				pXmlValue->AddAttribute(g_pszXmlType, sType);

				pXmlSet->AddChild(pXmlName);
				pXmlSet->AddChild(pXmlValue);
				ctx->m_pNode->AddChild(pXmlSet);
			}
		}
	}

	return 0;
}

int EnumDbModules(const char *szModuleName, void *lp)
{
	CEnumContext *ctx = (CEnumContext*)lp;
	IXMLNode::TXMLNodePtr pXml = ctx->m_pNode;
	IXMLNode::TXMLNodePtr pModule = ctx->m_pXmlEngine->CreateNode(g_pszXmlModule, currencyrates_a2t(szModuleName)/*A2CT(szModuleName)*/);
	if (pModule) {
		ctx->m_pszModule = szModuleName;
		ctx->m_pNode = pModule;
		db_enum_settings(ctx->m_hContact, &enum_contact_settings, szModuleName, ctx);

		if (pModule->GetChildCount() > 0)
			pXml->AddChild(pModule);

		ctx->m_pNode = pXml;
	}

	return 0;
}

IXMLNode::TXMLNodePtr export_contact(MCONTACT hContact, const CModuleInfo::TXMLEnginePtr& pXmlEngine)
{
	IXMLNode::TXMLNodePtr pNode = pXmlEngine->CreateNode(g_pszXmlContact, tstring());
	if (pNode) {
		CEnumContext ctx;
		ctx.m_pXmlEngine = pXmlEngine;
		ctx.m_pNode = pNode;
		ctx.m_hContact = hContact;

		db_enum_modules(EnumDbModules, &ctx);
	}
	return pNode;
}

LPCTSTR prepare_filter(LPTSTR pszBuffer, size_t cBuffer)
{
	LPTSTR p = pszBuffer;
	LPCTSTR pszXml = TranslateT("XML File (*.xml)");
	mir_wstrncpy(p, pszXml, (int)cBuffer);
	size_t nLen = mir_wstrlen(pszXml) + 1;
	p += nLen;
	if (nLen < cBuffer) {
		mir_wstrncpy(p, L"*.xml", (int)(cBuffer - nLen));
		p += 6;
		nLen += 6;
	}

	if (nLen < cBuffer) {
		LPCTSTR pszAll = TranslateT("All files (*.*)");
		mir_wstrncpy(p, pszAll, (int)(cBuffer - nLen));
		size_t n = mir_wstrlen(pszAll) + 1;
		nLen += n;
		p += n;
	}

	if (nLen < cBuffer) {
		mir_wstrncpy(p, L"*.*", (int)(cBuffer - nLen));
		p += 4;
		nLen += 4;
	}

	if (nLen < cBuffer)
		*p = '\0';

	return pszBuffer;
}

bool show_open_file_dialog(bool bOpen, tstring& rsFile)
{
	wchar_t szBuffer[MAX_PATH];
	wchar_t szFilter[MAX_PATH];
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);

	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = prepare_filter(szFilter, MAX_PATH);
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	ofn.lpstrDefExt = L"xml";
	if (bOpen)
		ofn.Flags |= OFN_FILEMUSTEXIST;
	else
		ofn.Flags |= OFN_OVERWRITEPROMPT;

	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile = szBuffer;
	ofn.lpstrFile[0] = '\0';

	if (bOpen) {
		if (FALSE == GetOpenFileName(&ofn))
			return false;
	}
	else {
		if (FALSE == GetSaveFileName(&ofn))
			return false;
	}

	rsFile = szBuffer;
	return true;
}

INT_PTR CurrencyRates_Export(WPARAM wp, LPARAM lp)
{
	tstring sFileName;
	const char* pszFile = reinterpret_cast<const char*>(lp);
	if (nullptr == pszFile) {
		if (false == show_open_file_dialog(false, sFileName))
			return -1;
	}
	else sFileName = currencyrates_a2t(pszFile);//A2CT(pszFile);

	CModuleInfo::TXMLEnginePtr pXmlEngine = CModuleInfo::GetInstance().GetXMLEnginePtr();
	CModuleInfo::TCurrencyRatesProvidersPtr pProviders = CModuleInfo::GetInstance().GetCurrencyRateProvidersPtr();
	IXMLNode::TXMLNodePtr pRoot = pXmlEngine->CreateNode(g_pszXmlContacts, tstring());
	MCONTACT hContact = MCONTACT(wp);
	if (hContact) {
		CCurrencyRatesProviders::TCurrencyRatesProviderPtr pProvider = pProviders->GetContactProviderPtr(hContact);
		if (pProvider) {
			IXMLNode::TXMLNodePtr pNode = export_contact(hContact, pXmlEngine);
			if (pNode)
				pRoot->AddChild(pNode);
		}
	}
	else {
		for (auto &cc : Contacts(CURRENCYRATES_MODULE_NAME)) {
			CCurrencyRatesProviders::TCurrencyRatesProviderPtr pProvider = pProviders->GetContactProviderPtr(cc);
			if (pProvider) {
				IXMLNode::TXMLNodePtr pNode = export_contact(cc, pXmlEngine);
				if (pNode)
					pRoot->AddChild(pNode);
			}
		}
	}

	return ((true == pXmlEngine->SaveFile(sFileName, pRoot)) ? 0 : 1);
}

bool set_contact_settings(MCONTACT hContact, DBCONTACTWRITESETTING& dbs)
{
	assert(DBVT_DELETED != dbs.value.type);
	return (0 == db_set(hContact, dbs.szModule, dbs.szSetting, &dbs.value));
}

bool handle_module(MCONTACT hContact, const IXMLNode::TXMLNodePtr& pXmlModule)
{
	size_t cCreatedRecords = 0;
	tstring sModuleName = pXmlModule->GetText();
	if (false == sModuleName.empty()) {
		DBCONTACTWRITESETTING dbs;
		std::string s = currencyrates_t2a(sModuleName.c_str());
		dbs.szModule = s.c_str();//T2CA(sModuleName.c_str());

		bool bCListModule = 0 == mir_wstrcmpi(sModuleName.c_str(), L"CList");

		size_t cChild = pXmlModule->GetChildCount();
		for (size_t i = 0; i < cChild; ++i) {
			IXMLNode::TXMLNodePtr pSetting = pXmlModule->GetChildNode(i);
			tstring sSetting = pSetting->GetName();
			if (0 == mir_wstrcmpi(g_pszXmlSetting, sSetting.c_str())) {
				size_t cSetChild = pSetting->GetChildCount();
				if (cSetChild >= 2) {
					tstring sName;
					tstring sValue;
					tstring sType;
					for (size_t j = 0; j < cSetChild; ++j) {
						IXMLNode::TXMLNodePtr pNode = pSetting->GetChildNode(j);
						tstring sNode = pNode->GetName();
						if (0 == mir_wstrcmpi(g_pszXmlName, sNode.c_str())) {
							sName = pNode->GetText();
						}
						else if (0 == mir_wstrcmpi(g_pszXmlValue, sNode.c_str())) {
							sValue = pNode->GetText();
							sType = pNode->GetAttributeValue(g_pszXmlType);
						}
					}

					if ((false == sName.empty()) && (false == sType.empty())) {
						std::string s1 = currencyrates_t2a(sName.c_str());
						dbs.szSetting = s1.c_str();
						if (0 == mir_wstrcmpi(g_pszXmlTypeByte, sType.c_str())) {
							tistringstream in(sValue.c_str());
							in.imbue(GetSystemLocale());
							dbs.value.cVal = in.get();
							if (in.good() && in.eof()) {
								dbs.value.type = DBVT_BYTE;
								if (set_contact_settings(hContact, dbs))
									++cCreatedRecords;
							}
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeWord, sType.c_str())) {
							tistringstream in(sValue.c_str());
							in.imbue(GetSystemLocale());
							in >> dbs.value.wVal;
							if (in.good() || in.eof()) {
								dbs.value.type = DBVT_WORD;
								if (set_contact_settings(hContact, dbs))
									++cCreatedRecords;
							}
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeDword, sType.c_str())) {
							tistringstream in(sValue.c_str());
							in.imbue(GetSystemLocale());
							in >> dbs.value.dVal;
							if (in.good() || in.eof()) {
								dbs.value.type = DBVT_DWORD;
								if (set_contact_settings(hContact, dbs))
									++cCreatedRecords;
							}
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeAsciiz, sType.c_str())) {
							ptrA v(mir_u2a(sValue.c_str()));
							dbs.value.pszVal = v;
							dbs.value.type = DBVT_ASCIIZ;
							if (set_contact_settings(hContact, dbs))
								++cCreatedRecords;
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeUtf8, sType.c_str())) {
							T2Utf szValue(sValue.c_str());
							dbs.value.pszVal = szValue;
							dbs.value.type = DBVT_UTF8;
							if (set_contact_settings(hContact, dbs))
								++cCreatedRecords;
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeWchar, sType.c_str())) {
							ptrW val(mir_wstrdup(sValue.c_str()));
							dbs.value.pwszVal = val;
							dbs.value.type = DBVT_WCHAR;
							if (set_contact_settings(hContact, dbs))
								++cCreatedRecords;
						}
						else if (0 == mir_wstrcmpi(g_pszXmlTypeBlob, sType.c_str())) {
							size_t bufLen;
							mir_ptr<BYTE> buf((PBYTE)mir_base64_decode(_T2A(sValue.c_str()), &bufLen));
							if (buf) {
								dbs.value.pbVal = buf;
								dbs.value.cpbVal = (WORD)bufLen;
								dbs.value.type = DBVT_BLOB;

								if (set_contact_settings(hContact, dbs))
									++cCreatedRecords;
							}
						}

						if ((true == bCListModule) && (0 == mir_wstrcmpi(sName.c_str(), L"Group")))
							Clist_GroupCreate(NULL, sValue.c_str());
					}
				}
			}
		}
	}

	return true;
}

size_t count_contacts(const IXMLNode::TXMLNodePtr& pXmlRoot, bool bInContactsGroup)
{
	size_t cContacts = 0;
	size_t cChild = pXmlRoot->GetChildCount();
	for (size_t i = 0; i < cChild; ++i) {
		IXMLNode::TXMLNodePtr pNode = pXmlRoot->GetChildNode(i);
		tstring sName = pNode->GetName();
		if (false == bInContactsGroup) {
			if (0 == mir_wstrcmpi(g_pszXmlContacts, sName.c_str()))
				cContacts += count_contacts(pNode, true);
			else
				cContacts += count_contacts(pNode, false);
		}
		else {
			if (0 == mir_wstrcmpi(g_pszXmlContact, sName.c_str()))
				++cContacts;
		}
	}

	return cContacts;
}

struct CImportContext
{
	CImportContext(size_t cTotalContacts) : m_cTotalContacts(cTotalContacts), m_cHandledContacts(0), m_nFlags(0) {}

	size_t m_cTotalContacts;
	size_t m_cHandledContacts;
	UINT m_nFlags;
};

struct CContactState
{
	CContactState() : m_hContact(NULL), m_bNewContact(false) {}
	MCONTACT m_hContact;
	CCurrencyRatesProviders::TCurrencyRatesProviderPtr m_pProvider;
	bool m_bNewContact;
};

IXMLNode::TXMLNodePtr find_currencyrates_module(const IXMLNode::TXMLNodePtr& pXmlContact)
{
	static const tstring g_sCurrencyRates = currencyrates_a2t(CURRENCYRATES_MODULE_NAME);
	size_t cChild = pXmlContact->GetChildCount();
	for (size_t i = 0; i < cChild; ++i) {
		IXMLNode::TXMLNodePtr pNode = pXmlContact->GetChildNode(i);
		tstring sName = pNode->GetName();
		if ((0 == mir_wstrcmpi(g_pszXmlModule, sName.c_str()))
			&& (0 == mir_wstrcmpi(g_sCurrencyRates.c_str(), pNode->GetText().c_str()))) {
			return pNode;
		}
	}

	return IXMLNode::TXMLNodePtr();
}

typedef std::pair<tstring, tstring> TNameValue;//first is name,second is value
TNameValue parse_setting_node(const IXMLNode::TXMLNodePtr& pXmlSetting)
{
	assert(pXmlSetting);

	tstring sName, sValue;
	size_t cSettingChildItems = pXmlSetting->GetChildCount();
	for (size_t j = 0; j < cSettingChildItems; ++j) {
		IXMLNode::TXMLNodePtr pXMLSetChild = pXmlSetting->GetChildNode(j);
		if (pXMLSetChild) {
			if (0 == mir_wstrcmpi(g_pszXmlName, pXMLSetChild->GetName().c_str())) {
				sName = pXMLSetChild->GetText();
			}
			else if (0 == mir_wstrcmpi(g_pszXmlValue, pXMLSetChild->GetName().c_str())) {
				sValue = pXMLSetChild->GetText();
			}
		}
	}

	return std::make_pair(sName, sValue);
}

CCurrencyRatesProviders::TCurrencyRatesProviderPtr find_provider(const IXMLNode::TXMLNodePtr& pXmlCurrencyRatesModule)
{
	// 		USES_CONVERSION;
	static const tstring g_sCurrencyRatesProvider = currencyrates_a2t(DB_STR_CURRENCYRATE_PROVIDER);//A2CT(DB_STR_CURRENCYRATE_PROVIDER);
	size_t cChild = pXmlCurrencyRatesModule->GetChildCount();
	for (size_t i = 0; i < cChild; ++i) {
		IXMLNode::TXMLNodePtr pXMLSetting = pXmlCurrencyRatesModule->GetChildNode(i);
		if (pXMLSetting && (0 == mir_wstrcmpi(g_pszXmlSetting, pXMLSetting->GetName().c_str()))) {
			TNameValue Item = parse_setting_node(pXMLSetting);
			if ((0 == mir_wstrcmpi(g_sCurrencyRatesProvider.c_str(), Item.first.c_str())) && (false == Item.second.empty())) {
				return CModuleInfo::GetInstance().GetCurrencyRateProvidersPtr()->FindProvider(Item.second);
			}
		}
	}

	return CCurrencyRatesProviders::TCurrencyRatesProviderPtr();
}

bool get_contact_state(const IXMLNode::TXMLNodePtr& pXmlContact, CContactState& cst)
{
	class visitor : public CCurrencyRatesProviderVisitor
	{
	public:
		visitor(const IXMLNode::TXMLNodePtr& pXmlCurrencyRates)
			: m_hContact(NULL), m_pXmlCurrencyRates(pXmlCurrencyRates)
		{
		}

		MCONTACT GetContact()const { return m_hContact; }

	private:
		virtual void Visit(const CCurrencyRatesProviderCurrencyConverter& rProvider)override
		{
			static const tstring g_sFromID = currencyrates_a2t(DB_STR_FROM_ID);//A2CT(DB_STR_FROM_ID);
			static const tstring g_sToID = currencyrates_a2t(DB_STR_TO_ID);//A2CT(DB_STR_TO_ID);

			tstring sFromID;
			tstring sToID;
			size_t cChild = m_pXmlCurrencyRates->GetChildCount();
			for (size_t i = 0; i < cChild; ++i) {
				IXMLNode::TXMLNodePtr pNode = m_pXmlCurrencyRates->GetChildNode(i);
				if (pNode && (0 == mir_wstrcmpi(g_pszXmlSetting, pNode->GetName().c_str()))) {
					TNameValue Item = parse_setting_node(pNode);
					if (0 == mir_wstrcmpi(g_sFromID.c_str(), Item.first.c_str())) {
						sFromID = Item.second;
					}
					else if (0 == mir_wstrcmpi(g_sToID.c_str(), Item.first.c_str())) {
						sToID = Item.second;
					}
				}
			}

			if ((false == sFromID.empty()) && (false == sToID.empty())) {
				m_hContact = rProvider.GetContactByID(sFromID, sToID);
			}
		}

		tstring GetXMLNodeValue(const char* pszXMLNodeName)const
		{
			tstring sXMLNodeName = currencyrates_a2t(pszXMLNodeName);

			tstring sValue;
			size_t cChild = m_pXmlCurrencyRates->GetChildCount();
			for (size_t i = 0; i < cChild; ++i) {
				IXMLNode::TXMLNodePtr pNode = m_pXmlCurrencyRates->GetChildNode(i);
				if (pNode && (0 == mir_wstrcmpi(g_pszXmlSetting, pNode->GetName().c_str()))) {
					TNameValue Item = parse_setting_node(pNode);
					if (0 == mir_wstrcmpi(Item.first.c_str(), sXMLNodeName.c_str())) {
						sValue = Item.second;
						break;
					}
				}
			}

			return sValue;
		}

	private:
		MCONTACT m_hContact;
		IXMLNode::TXMLNodePtr m_pXmlCurrencyRates;
	};

	IXMLNode::TXMLNodePtr pXmlCurrencyRates = find_currencyrates_module(pXmlContact);
	if (pXmlCurrencyRates) {
		cst.m_pProvider = find_provider(pXmlCurrencyRates);
		if (cst.m_pProvider) {
			visitor vs(pXmlCurrencyRates);
			cst.m_pProvider->Accept(vs);
			cst.m_hContact = vs.GetContact();
			return true;
		}
	}

	return false;
}

bool import_contact(const IXMLNode::TXMLNodePtr& pXmlContact, CImportContext& impctx)
{
	++impctx.m_cHandledContacts;

	CContactState cst;
	bool bResult = get_contact_state(pXmlContact, cst);
	if (bResult) {
		if (NULL == cst.m_hContact) {
			cst.m_hContact = db_add_contact();
			cst.m_bNewContact = true;
		}
		else if (impctx.m_nFlags & CURRENCYRATES_IMPORT_SKIP_EXISTING_CONTACTS)
			return true;

		if (cst.m_hContact) {
			size_t cChild = pXmlContact->GetChildCount();
			for (size_t i = 0; i < cChild && bResult; ++i) {
				IXMLNode::TXMLNodePtr pNode = pXmlContact->GetChildNode(i);
				tstring sName = pNode->GetName();
				if (0 == mir_wstrcmpi(g_pszXmlModule, sName.c_str()))
					bResult &= handle_module(cst.m_hContact, pNode);
			}

			if (cst.m_bNewContact && bResult) {
				cst.m_pProvider->AddContact(cst.m_hContact);
				cst.m_pProvider->RefreshContact(cst.m_hContact);
			}
		}
		else bResult = false;
	}

	return bResult;
}

size_t import_contacts(const IXMLNode::TXMLNodePtr& pXmlContacts, CImportContext& impctx)
{
	size_t cContacts = 0;
	size_t cChild = pXmlContacts->GetChildCount();
	for (size_t i = 0; i < cChild; ++i) {
		IXMLNode::TXMLNodePtr pNode = pXmlContacts->GetChildNode(i);
		tstring sName = pNode->GetName();
		if (0 == mir_wstrcmpi(g_pszXmlContact, sName.c_str()))
			if (true == import_contact(pNode, impctx))
				++cContacts;
	}

	return cContacts;

}

size_t handle_contacts_node(const IXMLNode::TXMLNodePtr& pXmlRoot, CImportContext& impctx)
{
	size_t cContacts = 0;
	size_t cChild = pXmlRoot->GetChildCount();
	for (size_t i = 0; i < cChild; ++i) {
		IXMLNode::TXMLNodePtr pNode = pXmlRoot->GetChildNode(i);
		tstring sName = pNode->GetName();
		if (0 == mir_wstrcmpi(g_pszXmlContacts, sName.c_str()))
			cContacts += import_contacts(pNode, impctx);
		else
			cContacts += handle_contacts_node(pNode, impctx);
	}

	return cContacts;

}

bool do_import(const IXMLNode::TXMLNodePtr& pXmlRoot, UINT nFlags)
{
	CImportContext imctx(count_contacts(pXmlRoot, false));
	imctx.m_cHandledContacts = 0;
	imctx.m_nFlags = nFlags;

	return (handle_contacts_node(pXmlRoot, imctx) > 0);
}

INT_PTR CurrencyRates_Import(WPARAM wp, LPARAM lp)
{
	// 	USES_CONVERSION;

	tstring sFileName;
	const char* pszFile = reinterpret_cast<const char*>(lp);
	if (nullptr == pszFile) {
		if (false == show_open_file_dialog(true, sFileName))
			return -1;
	}
	else sFileName = currencyrates_a2t(pszFile);//A2CT(pszFile);

	CModuleInfo::TXMLEnginePtr pXmlEngine = CModuleInfo::GetInstance().GetXMLEnginePtr();
	IXMLNode::TXMLNodePtr pXmlRoot = pXmlEngine->LoadFile(sFileName);
	if (pXmlRoot)
		return ((true == do_import(pXmlRoot, wp)) ? 0 : 1);

	return 1;
}

INT_PTR CurrencyRatesMenu_ImportAll(WPARAM, LPARAM)
{
	return CallService(MS_CURRENCYRATES_IMPORT, 0, 0);
}

INT_PTR CurrencyRatesMenu_ExportAll(WPARAM, LPARAM)
{
	return CallService(MS_CURRENCYRATES_EXPORT, 0, 0);
}
