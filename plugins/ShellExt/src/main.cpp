#include "stdafx.h"
#include "shlcom.h"

CMPlugin g_plugin;

bool bIsVistaPlus;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		bIsVistaPlus = GetProcAddress( GetModuleHandleA("kernel32.dll"), "GetProductInfo") != nullptr;
		DisableThreadLibraryCalls(hinstDLL);
	}

	return TRUE;
}

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
	// {7993AB24-1FDA-428C-A89B-BE377A10BE3A}
	{ 0x7993ab24, 0x1fda, 0x428c, { 0xa8, 0x9b, 0xbe, 0x37, 0x7a, 0x10, 0xbe, 0x3a }}
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(SHLExt_Name, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////
// exported functions

const IID CLSID_ISHLCOM = { 0x72013A26, 0xA94C, 0x11d6, {0x85, 0x40, 0xA5, 0xE6, 0x29, 0x32, 0x71, 0x1D }};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (rclsid == CLSID_ISHLCOM) {
		TClassFactoryRec *p = new TClassFactoryRec();
		HRESULT hr = p->QueryInterface(riid, ppv);
		if ( FAILED(hr)) {
			delete p;
			return hr;
		}
		logA("DllGetClassObject succeeded\n");
		return S_OK;
	}

	#ifdef LOG_ENABLED
		RPC_CSTR szGuid;
		UuidToStringA(&riid, &szGuid);
		logA("DllGetClassObject {%08x-%04x-%04x-%08x%08x} failed\n", szGuid);
		RpcStringFreeA(&szGuid);
	#endif

	*ppv = nullptr;
	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
	logA("DllCanUnloadNow: %d %d\n", DllFactoryCount, DllObjectCount);
	if (DllFactoryCount == 0 && DllObjectCount == 0)
		return S_OK;
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct HRegKey
{
	HRegKey(HKEY hRoot, const wchar_t *ptszKey) : m_key(nullptr)
	{	RegCreateKeyEx(hRoot, ptszKey, 0, nullptr, 0, KEY_SET_VALUE | KEY_CREATE_SUB_KEY, nullptr, &m_key, nullptr);
	}
	
	~HRegKey() { if (m_key) RegCloseKey(m_key); }
	
	operator HKEY() const { return m_key; }

private:
	HKEY m_key;
};

char str1[100];
char str2[] = "{72013A26-A94C-11d6-8540-A5E62932711D}";
char str3[] = "miranda.shlext";
char str4[] = "Apartment";
 
STDAPI DllRegisterServer()
{
	HRegKey k1(HKEY_CLASSES_ROOT, L"miranda.shlext");
	if (k1 == nullptr)
		return E_FAIL;

	int str1len = sprintf_s(str1, sizeof(str1), "shlext %d.%d.%d.%d - shell context menu support for Miranda NG", __FILEVERSION_STRING);
	if ( RegSetValueA(k1, nullptr, REG_SZ, str1, str1len))
		return E_FAIL;
	if ( RegSetValueA(k1, "CLSID", REG_SZ, str2, sizeof(str2)))
		return E_FAIL;

	//////////////////////////////////////////////////////////////////////////////////////

	HRegKey kClsid(HKEY_CLASSES_ROOT, L"CLSID\\{72013A26-A94C-11d6-8540-A5E62932711D}");
	if (kClsid == nullptr)
		return E_FAIL;

	if ( RegSetValueA(kClsid, nullptr, REG_SZ, str3, sizeof(str3)))
		return E_FAIL;
	if ( RegSetValueA(kClsid, "ProgID", REG_SZ, str3, sizeof(str3)))
		return E_FAIL;

	HRegKey kInprocServer(kClsid, L"InprocServer32");
	if (kInprocServer == nullptr)
		return E_FAIL;

	wchar_t tszFileName[MAX_PATH];
	GetModuleFileName(g_plugin.getInst(), tszFileName, _countof(tszFileName));
	if ( RegSetValueEx(kInprocServer, nullptr, 0, REG_SZ, (LPBYTE)tszFileName, sizeof(wchar_t)*(lstrlen(tszFileName)+1)))
		return E_FAIL;
	if ( RegSetValueExA(kInprocServer, "ThreadingModel", 0, REG_SZ, (PBYTE)str4, sizeof(str4)))
		return E_FAIL;

	//////////////////////////////////////////////////////////////////////////////////////

	if ( RegSetValueA(HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\miranda.shlext", REG_SZ, str2, sizeof(str2)))
		return E_FAIL;
	if ( RegSetValueA(HKEY_CLASSES_ROOT, "Directory\\shellex\\ContextMenuHandlers\\miranda.shlext", REG_SZ, str2, sizeof(str2)))
		return E_FAIL;

	//////////////////////////////////////////////////////////////////////////////////////

	HRegKey k2(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");
	if (k2 == nullptr)
		return E_FAIL;
	if ( RegSetValueExA(k2, str2, 0, REG_SZ, (PBYTE)str1, str1len))
		return E_FAIL;

	return S_OK;
}

STDAPI DllUnregisterServer()
{
	return RemoveCOMRegistryEntries();
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Load()
{
	InvokeThreadServer();
	HookEvent(ME_OPT_INITIALISE, OnOptionsInit);
	DllRegisterServer();
	CheckRegisterServer();
	return 0;
}
