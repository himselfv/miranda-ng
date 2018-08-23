This is the original DbChecker plugin, compatible with latest Miranda NG.

See plugins/db3x_mmap_origin for more details.

Original DbChecker:
Last official revision: 24.02.2018, dff565f40105b20b0e8e4dba1f48ccc9b8e7ff44
Deprecation: 25.03.2018, 3583f79d974d805aa67ba8b1feb49a4974c2e4cd


Changes since last official version:
1. Original DbChecker.dll name and PID has been banned, so the plugin is now called DbCheckerOrig.dll
- src/newplugins.cpp: original PID added to bannedPlugins
- updater: dbchecker.dll added for removal (-> null)
Code which relied on calling Miranda with /svc:dbchecker will break as it assumes the second part is a dll name.

2. CheckDb() slot removed from DATABASELINK interface - replaced with DBCHECKERLINK which can be queried (see m_db_checker.h)

