This is the full version of the old db3x_mmap plugin, compatible with latest Miranda NG.

The plugins/db3x_mmap in the original Miranda NG now contains a gutted version which redirects to dbx_mdbx. This version produces the same DLL but with complete functionality, up to date with changes in Miranda NG.

MAINTENANCE:
Watch for changes in main/db3x_mmap, main/dbx_mdbx, and files m_db_int.h, m_database.h. Implement them in this plugin.


0.95.8
Changes to database interfaces since db3x_mmap deprecation and mitigations in db3x_mmap_origin:

DbChecker plugin removed -- restored as DbCheckerOrig.dll (see plugins/DbChecker).
1. DbChecker-related interfaces removed - restored in m_db_checker.h
2. CheckDb() slot removed from the DATABASELINK interface - replaced with DBCHECKERLINK which we provide (see m_db_checker.h)

3. First field of DATABASELINK instead of structure size now stores flags. We should pass 0 (or "Compact" if we support it) -- done.
  The zero-implementation for Compact() itself is not needed - MDatabaseCommon provides one automatically.
