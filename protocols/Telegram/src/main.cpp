#include "stdafx.h"

int hLangpack;

PLUGININFOEX pluginInfo =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {AE708252-0DF8-42BA-9EF9-9ACC038EEDA7}
	{0xae708252, 0xdf8, 0x42ba, {0x9e, 0xf9, 0x9a, 0xcc, 0x3, 0x8e, 0xed, 0xa7}}

};

/////////////////////////////////////////////////////////////////////////////////////////

CMPlugin g_plugin;

extern "C" _pfnCrtInit _pRawDllMain = &CMPlugin::RawDllMain;

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = { MIID_PROTOCOL, MIID_LAST };

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
