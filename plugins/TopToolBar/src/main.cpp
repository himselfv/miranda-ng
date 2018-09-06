
#include "stdafx.h"

CMPlugin g_plugin;

IconItem iconList[] =
{
	{ LPGEN("Execute"),               "run",          IDI_RUN },
	{ LPGEN("Hide offline contacts"), "hide_offline", IDI_HIDEOFFLINE },
	{ LPGEN("Show offline contacts"), "show_offline", IDI_SHOWOFFLINE },
	{ LPGEN("Disable groups"),        "groups_off",   IDI_GROUPSOFF },
	{ LPGEN("Enable groups"),         "groups_on",    IDI_GROUPSON },
	{ LPGEN("Disable sounds"),        "sounds_off",   IDI_SOUNDSOFF },
	{ LPGEN("Enable sounds"),         "sounds_on",    IDI_SOUNDSON },
	{ LPGEN("Disable metacontacts"),  "meta_off",     IDI_METAON },
	{ LPGEN("Enable metacontacts"),   "meta_on",      IDI_METAOFF },
	{ LPGEN("Separator"),             "separator",    IDI_SEPARATOR }
};

/////////////////////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {F593C752-51D8-4D46-BA27-37577953F55C}
	{0xf593c752, 0x51d8, 0x4d46, {0xba, 0x27, 0x37, 0x57, 0x79, 0x53, 0xf5, 0x5c}}
};

CMPlugin::CMPlugin() :
	PLUGIN<CMPlugin>(TTB_OPTDIR, pluginInfoEx)
{}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Load()
{
	g_plugin.registerIcon(TTB_OPTDIR, iconList, TTB_OPTDIR);

	LoadToolbarModule();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CMPlugin::Unload()
{
	UnloadToolbarModule();
	return 0;
}
