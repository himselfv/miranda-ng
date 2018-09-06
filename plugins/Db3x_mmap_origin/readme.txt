This is the full version of the old db3x_mmap plugin, compatible with latest Miranda NG.

The plugins/db3x_mmap in the original Miranda NG now contains a gutted version which redirects to dbx_mdbx. This version produces the same DLL but with complete functionality, up to date with changes in Miranda NG.

MAINTENANCE:
Watch for changes in main/db3x_mmap, main/dbx_mdbx, and files m_db_int.h, m_database.h. Implement them in this plugin.
Update the version in src/version.h (Miranda won't load older version plugins and will certainly try to update them).


0.95.9
m_database.h, m_db_int.h: Nothing relevant.
dbx_mdbx: Nothing relevant (Explicit backup support, optional)

version.rc - everything moved to
  #include "..\..\build\Version.rc"

stdafx.h, init.cpp, ui.cpp, ui.h:
  Switch to CMPlugin() architecture.

new functions to access mirandaboot.ini in ui.cpp

ui.cpp/ui.h switched to overriden OnInitDialog/OnClose.


0.95.8
Changes to database interfaces since db3x_mmap deprecation and mitigations in db3x_mmap_origin:

DbChecker plugin removed -- restored as DbCheckerOrig.dll (see plugins/DbChecker).
1. DbChecker-related interfaces removed - restored in m_db_checker.h
2. CheckDb() slot removed from the DATABASELINK interface - replaced with DBCHECKERLINK which we provide (see m_db_checker.h)

3. First field of DATABASELINK instead of structure size now stores flags. We should pass 0 (or "Compact" if we support it) -- done.
  The zero-implementation for Compact() itself is not needed - MDatabaseCommon provides one automatically.

4. Database plugins now need to report MDB_CAPS_CREATE if they support the creation of new DBs.
