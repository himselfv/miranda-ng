This is the original DbChecker plugin, compatible with latest Miranda NG.

See plugins/db3x_mmap_origin for more details.

Original DbChecker:
Last official revision: 24.02.2018, dff565f40105b20b0e8e4dba1f48ccc9b8e7ff44
Deprecation: 25.03.2018, 3583f79d974d805aa67ba8b1feb49a4974c2e4cd


Changes since last official version:
1. CheckDb() slot removed from DATABASELINK interface - replaced with DBCHECKERLINK which can be queried (see m_db_checker.h)
