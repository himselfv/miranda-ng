This is the full version of the old db3x_mmap plugin, compatible with latest Miranda NG.

The plugins/db3x_mmap in the original Miranda NG now contains a gutted version which redirects to dbx_mdbx. This version produces the same DLL but with complete functionality, up to date with changes in Miranda NG.

MAINTENANCE:
Watch for changes in main/db3x_mmap, main/dbx_mdbx, and files m_db_int.h, m_database.h. Implement them in this plugin.


0.95.8
Changes to database interfaces since db3x_mmap deprecation and mitigations in db3x_mmap_origin:

Interfaces removed:
1. m_db_int.h: MIDatabaseChecker interface removed - restored in m_db_checker.h
2. m_database.h: MS_DB_CHECKPROFILE service removed - restored in m_db_checker.h
Both restored in m_db_checker.h.

DbChecker plugin itself removed:
1. DbChecker folder with all contents removed.
2. DbChecker plugin PID added to bannedPlugins in src/newplugins.cpp - we'll have to change it if restoring.
  - EnsureCheckerLoaded function removed (same module).
3. dbchecker.dll added to updater for removal (-> null). DLL name will have to be changed too.
To restore DbChecker we'll have to restore the plugin, change PID, rename it. (And /svc:dbchecker will probably break as it assumes the second part is a dll name, as well as code which depended on running Miranda with this key)

Integration removed:
1. m_db_int.h: CheckDb() slot removed from DATABASELINK interface - replaced with DBCHECKERLINK which can be queried (see m_db_checker.h)
2. src/database.cpp: Database checker related calls removed:
  - MS_DB_CHECKPROFILE call in getProfile
  - EGROKPRF_OBSOLETE handling by calling CHECKPROFILE in tryOpenDatabase
  - the disabling of dbchecker if we're starting in /svc-mode

profilemanager and profilemanagerex integration:
8. src/profilemanager.cpp/h:
  - Removed: CheckProfile() which called MS_DB_CHECKPROFILE.
  - Removed: Convert / Check profile menu entries (both = the same CHECKPROFILE call)
9. plugins/ProfileManager/src/pmanagerEx.cpp:
  - Removed: #define SRV_CHECK_DB   "Database/CheckDb"
  - Removed the option to check the database which reboots Miranda in /svc:dbchecker mode

In the last two cases we'll hardly be able to restore the removed parts. We can move the same functions elsewhere:
- Database checks on load to the database itself
- Manual checks to the command line only
This is not urgent.

Small changes to be copied:
10. First field of DATABASELINK instead of structure size now stores flags. We should pass 0 (or "Compact" if we support it) -- done.
  The zero-implementation for Compact() itself is not needed - MDatabaseCommon provides one automatically.
