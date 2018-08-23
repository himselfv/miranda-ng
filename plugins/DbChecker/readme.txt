This is the original DbChecker plugin, compatible with latest Miranda NG.

* The DLL has been renamed to DbCheckerOrig.dll
* Only db3x_mmap_origin supports it currently
* Can only be run from the command line with:
  Miranda[32/64].exe /svc:dbcheckerorig

Original DbChecker:
Last official revision: 24.02.2018, dff565f40105b20b0e8e4dba1f48ccc9b8e7ff44
Deprecation: 25.03.2018, 3583f79d974d805aa67ba8b1feb49a4974c2e4cd


Changes since last official version:
1. Original DbChecker.dll name and PID has been banned, so the plugin is now called DbCheckerOrig.dll
- src/newplugins.cpp: original PID added to bannedPlugins
- updater: dbchecker.dll added for removal (-> null)
Code which relied on calling Miranda with /svc:dbchecker will break as it assumes the second part is a dll name.

2. CheckDb() slot removed from DATABASELINK interface - replaced with DBCHECKERLINK which can be queried (see m_db_checker.h)

Interfaces removed:
3. m_db_int.h: MIDatabaseChecker interface removed - restored in m_db_checker.h
4. m_database.h: MS_DB_CHECKPROFILE service removed - restored in m_db_checker.h

Integration removed:
1. src/newplugins.cpp: EnsureCheckerLoaded function removed.
2. src/database.cpp: Database checker related calls removed:
  - MS_DB_CHECKPROFILE call in getProfile when ProfileManager has failed (!pd.bRun)
  - EGROKPRF_OBSOLETE handling by calling CHECKPROFILE in tryOpenDatabase (with EnsureCheckerLoaded<-true)
  - the disabling of dbchecker (EnsureCheckerLoaded<-false) if we're starting in /svc-mode (unclear why it's needed)
For now we'll just ignore this as non-critical.
We cannot reimplement EnsureCheckerLoaded. We can see if it's loaded with newplugins.h/IsPluginLoaded, but not load/unload it.
The rest of the calls are automatic db check/conversion on errors.

Profilemanager and profilemanagerex integration:
3. src/profilemanager.cpp/h:
  - Removed: CheckProfile() which called MS_DB_CHECKPROFILE.
  - Removed: Convert / Check profile menu entries (both = the same CHECKPROFILE call)
4. plugins/ProfileManager/src/pmanagerEx.cpp:
  - Removed: #define SRV_CHECK_DB   "Database/CheckDb"
  - Removed the option to check the database which reboots Miranda in /svc:dbchecker mode
We cannot restore these in-place. For now, for offline checks we'll rely on manual checks from the command line only. Online checks had never been possible at all.
