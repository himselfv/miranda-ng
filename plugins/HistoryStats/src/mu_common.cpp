#include "stdafx.h"
#include "mu_common.h"

#include <map>
#include <set>

// {7649A9F5-E57F-4E5D-A715-7BD8B3496DBE}
static MUUID menu_id = { 0x7649a9f5, 0xe57f, 0x4e5d, 0xa7, 0x15, 0x7b, 0xd8, 0xb3, 0x49, 0x6d, 0xbe };

namespace mu
{
	/*
	 * clist
	 */

	namespace clist
	{
		HGENMENU addMainMenuItem(const wchar_t* pszName, DWORD flags, int position, HICON hIcon, const char* pszService, HGENMENU hRoot)
		{
			// TODO: support for unicode-core with unicode-aware CList
			CMenuItem mi(&g_plugin);
			mi.name.w = (wchar_t*)pszName;
			mi.flags = flags | CMIF_UNICODE;
			mi.position = position;
			mi.hIcolibItem = hIcon;
			mi.pszService = const_cast<char*>(pszService);
			mi.root = hRoot;
			mi.uid = menu_id; menu_id.d[7]++;
			return Menu_AddMainMenuItem(&mi);
		}

		HGENMENU addContactMenuItem(const wchar_t* pszName, DWORD flags, int position, HICON hIcon, const char* pszService)
		{
			// TODO: support for unicode-core with unicode-aware CList
			CMenuItem mi(&g_plugin);
			mi.name.w = (wchar_t*)pszName;
			mi.flags = flags | CMIF_UNICODE;
			mi.position = position;
			mi.hIcolibItem = hIcon;
			mi.pszService = const_cast<char*>(pszService);
			mi.uid = menu_id; menu_id.d[7]++;
			return Menu_AddContactMenuItem(&mi);
		}
	}

	/*
	 * icolib
	 */

	namespace icolib
	{
		bool _available()
		{
			return true;
		}

		void addIcon(const wchar_t* szSection, const wchar_t* szDescription, const char* szIconName, const char* szDefaultFile, int iDefaultIndex)
		{
			SKINICONDESC sid = {};
			sid.section.w = const_cast<wchar_t*>(szSection);
			sid.description.w = const_cast<wchar_t*>(szDescription);
			sid.pszName = const_cast<char*>(szIconName);
			sid.defaultFile.a = const_cast<char*>(szDefaultFile);
			sid.iDefaultIndex = iDefaultIndex;
			sid.flags = SIDF_UNICODE;
			g_plugin.addIcon(&sid);
		}

		HICON getIcon(const char* szIconName)
		{
			return IcoLib_GetIcon(szIconName);
		}
	}

	/*
	 * protosvc
	 */

	namespace protosvc
	{
		DWORD getCaps(const char* szProto, int flagNum)
		{
			return (DWORD)CallProtoService(szProto, PS_GETCAPS, static_cast<WPARAM>(flagNum), 0);
		}

		HICON loadIcon(const char* szProto, int whichIcon)
		{
			return reinterpret_cast<HICON>(CallProtoService(szProto, PS_LOADICON, static_cast<WPARAM>(whichIcon), 0));
		}
	}

	/*
	 * core interface functions
	 */

	bool load()
	{
		// check for version
		if (!isMirandaVersionOk(Miranda_GetVersion()))
			return false;

		return true;
	}

	void unload()
	{}

	DWORD getMinimalMirandaVersion()
	{
		// MEMO: version dependency check
		return PLUGIN_MAKE_VERSION(0, 6, 7, 0);
	}

	bool isMirandaVersionOk(DWORD version)
	{
		return (version >= getMinimalMirandaVersion());
	}

	bool isMirandaUnicode()
	{
		return true;
	}

	/*
	 * string handling
	 */

	char* wideToAnsiDup(const WCHAR* pszWide, UINT uCP /* = CP_ACP */)
	{
		if (!pszWide)
			return nullptr;

		int len = WideCharToMultiByte(uCP, 0, pszWide, -1, nullptr, 0, nullptr, nullptr);
		char* result = reinterpret_cast<char*>(malloc(sizeof(char)* len));
		if (!result)
			return nullptr;

		WideCharToMultiByte(uCP, 0, pszWide, -1, result, len, nullptr, nullptr);
		result[len - 1] = 0;
		return result;
	}

	WCHAR* ansiToWideDup(const char* pszAnsi, UINT uCP /* = CP_ACP */)
	{
		if (!pszAnsi)
			return nullptr;

		int len = MultiByteToWideChar(uCP, 0, pszAnsi, -1, nullptr, 0);
		WCHAR* result = reinterpret_cast<WCHAR*>(malloc(sizeof(WCHAR)* len));
		if (!result)
			return nullptr;

		MultiByteToWideChar(uCP, 0, pszAnsi, -1, result, len);
		result[len - 1] = 0;
		return result;
	}

	char* wideToAnsi(const WCHAR* pszWide, char* pszRes, int maxLen, UINT uCP /* = CP_ACP */)
	{
		if (!pszWide)
			return nullptr;

		WideCharToMultiByte(uCP, 0, pszWide, -1, pszRes, maxLen, nullptr, nullptr);
		return pszRes;
	}

	WCHAR* ansiToWide(const char* pszAnsi, WCHAR* pszRes, int maxLen, UINT uCP /* = CP_ACP */)
	{
		if (!pszAnsi)
			return nullptr;

		MultiByteToWideChar(uCP, 0, pszAnsi, -1, pszRes, maxLen);
		return pszRes;
	}
}
