/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org)
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////////////////
// constructor & destructor

CDbxMDBX::CDbxMDBX(const TCHAR *tszFileName, int iMode) :
	m_safetyMode(true),
	m_bReadOnly((iMode & DBMODE_READONLY) != 0),
	m_bShared((iMode & DBMODE_SHARED) != 0),
	m_maxContactId(0)
{
	m_tszProfileName = mir_wstrdup(tszFileName);

	if (!m_bReadOnly) {
		m_hwndTimer = CreateWindowExW(0, L"STATIC", nullptr, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, nullptr, g_plugin.getInst(), nullptr);
		::SetWindowLongPtr(m_hwndTimer, GWLP_USERDATA, (LONG_PTR)this);
	}
}

CDbxMDBX::~CDbxMDBX()
{
	mdbx_env_close(m_env);

	if (!m_bReadOnly)
		TouchFile();

	if (m_hwndTimer != nullptr)
		::DestroyWindow(m_hwndTimer);

	for (auto &it : hService)
		DestroyServiceFunction(it);
	UnhookEvent(hHook);

	if (m_crypto)
		m_crypto->destroy();

	DestroyHookableEvent(hContactDeletedEvent);
	DestroyHookableEvent(hContactAddedEvent);
	DestroyHookableEvent(hSettingChangeEvent);
	DestroyHookableEvent(hEventMarkedRead);

	DestroyHookableEvent(hEventAddedEvent);
	DestroyHookableEvent(hEventDeletedEvent);
	DestroyHookableEvent(hEventFilterAddedEvent);

	mir_free(m_tszProfileName);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CDbxMDBX::Load()
{
	TouchFile();

	unsigned int defFlags = MDBX_CREATE;
	{
		txn_ptr trnlck(StartTran());
		mdbx_dbi_open(trnlck, "global", defFlags | MDBX_INTEGERKEY, &m_dbGlobal);
		mdbx_dbi_open(trnlck, "crypto", defFlags, &m_dbCrypto);
		mdbx_dbi_open(trnlck, "contacts", defFlags | MDBX_INTEGERKEY, &m_dbContacts);
		mdbx_dbi_open(trnlck, "modules", defFlags | MDBX_INTEGERKEY, &m_dbModules);
		mdbx_dbi_open(trnlck, "events", defFlags | MDBX_INTEGERKEY, &m_dbEvents);

		mdbx_dbi_open_ex(trnlck, "eventsrt", defFlags, &m_dbEventsSort, DBEventSortingKey::Compare, nullptr);
		mdbx_dbi_open_ex(trnlck, "settings", defFlags, &m_dbSettings, DBSettingKey::Compare, nullptr);

		uint32_t keyVal = 1;
		MDBX_val key = { &keyVal, sizeof(keyVal) }, data;
		if (mdbx_get(trnlck, m_dbGlobal, &key, &data) == MDBX_SUCCESS) {
			const DBHeader *hdr = (const DBHeader*)data.iov_base;
			if (hdr->dwSignature != DBHEADER_SIGNATURE)
				return EGROKPRF_DAMAGED;
			if (hdr->dwVersion != DBHEADER_VERSION)
				return EGROKPRF_OBSOLETE;

			m_header = *hdr;
		}
		else {
			m_header.dwSignature = DBHEADER_SIGNATURE;
			m_header.dwVersion = DBHEADER_VERSION;
			data.iov_base = &m_header; data.iov_len = sizeof(m_header);
			mdbx_put(trnlck, m_dbGlobal, &key, &data, 0);
			DBFlush();
		}

		keyVal = 2;
		if (mdbx_get(trnlck, m_dbGlobal, &key, &data) == MDBX_SUCCESS)
			m_ccDummy.dbc = *(const DBContact*)data.iov_base;

		trnlck.commit();
	}

	mdbx_txn_begin(m_env, nullptr, MDBX_RDONLY, &m_txn_ro);
	mdbx_cursor_open(m_txn_ro, m_dbEvents, &m_curEvents);
	mdbx_cursor_open(m_txn_ro, m_dbEventsSort, &m_curEventsSort);
	mdbx_cursor_open(m_txn_ro, m_dbSettings, &m_curSettings);
	mdbx_cursor_open(m_txn_ro, m_dbModules, &m_curModules);
	mdbx_cursor_open(m_txn_ro, m_dbContacts, &m_curContacts);

	MDBX_val key, val;
	if (mdbx_cursor_get(m_curEvents, &key, &val, MDBX_LAST) == MDBX_SUCCESS)
		m_dwMaxEventId = *(MEVENT*)key.iov_base;
	if (mdbx_cursor_get(m_curContacts, &key, &val, MDBX_LAST) == MDBX_SUCCESS)
		m_maxContactId = *(MCONTACT*)key.iov_base;

	mdbx_txn_reset(m_txn_ro);

	if (InitModules()) return EGROKPRF_DAMAGED;
	if (InitCrypt())   return EGROKPRF_DAMAGED;

	// everything is ok, go on
	if (!m_bReadOnly) {
		// retrieve the event handles
		hContactDeletedEvent = CreateHookableEvent(ME_DB_CONTACT_DELETED);
		hContactAddedEvent = CreateHookableEvent(ME_DB_CONTACT_ADDED);
		hSettingChangeEvent = CreateHookableEvent(ME_DB_CONTACT_SETTINGCHANGED);
		hEventMarkedRead = CreateHookableEvent(ME_DB_EVENT_MARKED_READ);

		hEventAddedEvent = CreateHookableEvent(ME_DB_EVENT_ADDED);
		hEventDeletedEvent = CreateHookableEvent(ME_DB_EVENT_DELETED);
		hEventFilterAddedEvent = CreateHookableEvent(ME_DB_EVENT_FILTER_ADD);
	}

	FillContacts();

	return EGROKPRF_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////

size_t iDefHeaderOffset = 0;
BYTE bDefHeader[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

int CDbxMDBX::Check(void)
{
	FILE *pFile = _wfopen(m_tszProfileName, L"rb");
	if (pFile == nullptr)
		return EGROKPRF_CANTREAD;

	fseek(pFile, (LONG)iDefHeaderOffset, SEEK_SET);
	BYTE buf[_countof(bDefHeader)];
	size_t cbRead = fread(buf, 1, _countof(buf), pFile);
	fclose(pFile);
	if (cbRead != _countof(buf))
		return EGROKPRF_DAMAGED;

	return (memcmp(buf, bDefHeader, _countof(bDefHeader))) ? EGROKPRF_UNKHEADER : 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

BOOL CDbxMDBX::Compact()
{
	CMStringW wszTmpFile(FORMAT, L"%s.tmp", m_tszProfileName);

	HANDLE pFile = ::CreateFile(wszTmpFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (pFile == nullptr) {
		Netlib_Logf(0, "Temporary file <%S> cannot be created", wszTmpFile.c_str());
		return 1;
	}

	mir_cslock lck(m_csDbAccess);
	int res = mdbx_env_copy2fd(m_env, pFile, MDBX_CP_COMPACT);
	CloseHandle(pFile);

	if (res == MDBX_SUCCESS) {
		mdbx_env_close(m_env);

		DeleteFileW(m_tszProfileName);
		MoveFileW(wszTmpFile, m_tszProfileName);

		Map();
		Load();
	}
	else DeleteFileW(wszTmpFile);

	return 0;
}

BOOL CDbxMDBX::Backup(const wchar_t *pwszPath)
{
	HANDLE pFile = ::CreateFile(pwszPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (pFile == nullptr) {
		Netlib_Logf(0, "Backup file <%S> cannot be created", pwszPath);
		return 1;
	}

	int res = mdbx_env_copy2fd(m_env, pFile, MDBX_CP_COMPACT);
	CloseHandle(pFile);
	if (res == MDBX_SUCCESS)
		return 0;

	DeleteFileW(pwszPath);
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CDbxMDBX::PrepareCheck()
{
	InitModules();
	return InitCrypt();
}

/////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(void) CDbxMDBX::SetCacheSafetyMode(BOOL bIsSet)
{
	m_safetyMode = bIsSet != 0;
	DBFlush(true);
}

/////////////////////////////////////////////////////////////////////////////////////////

static void assert_func(const MDBX_env*, const char *msg, const char *function, unsigned line)
{
/*	Netlib_Logf(nullptr, "MDBX: assertion failed (%s, %d): %s", function, line, msg);

	#if defined(_DEBUG)
		_wassert(_A2T(msg), _A2T(function), line);
	#endif */
}

int CDbxMDBX::Map()
{
	if (!LockName(m_tszProfileName))
		return EGROKPRF_CANTREAD;

	mdbx_env_create(&m_env);
	mdbx_env_set_maxdbs(m_env, 10);
	mdbx_env_set_maxreaders(m_env, 244);
	mdbx_env_set_userctx(m_env, this);
	mdbx_env_set_assert(m_env, assert_func);

	#ifdef _WIN64
		__int64 upperLimit = 0x400000000ul;
	#else
		intptr_t upperLimit = 512ul << 20;
	#endif

	int rc = mdbx_env_set_geometry(m_env,
		         -1,   // minimal lower limit
		  1ul << 20,   // at least 1M for now
		 upperLimit,	// 512M upper size
		  1ul << 20,   // 1M growth step
		512ul << 10,   // 512K shrink threshold
		         -1);  // default page size
	if (rc != MDBX_SUCCESS)
		return EGROKPRF_CANTREAD;

	unsigned int mode = MDBX_NOSUBDIR | MDBX_MAPASYNC | MDBX_WRITEMAP | MDBX_NOSYNC | MDBX_COALESCE | MDBX_EXCLUSIVE;
	if (m_bReadOnly)
		mode |= MDBX_RDONLY;

	if (mdbx_env_open(m_env, _T2A(m_tszProfileName), mode, 0664) != MDBX_SUCCESS)
		return EGROKPRF_CANTREAD;

	return EGROKPRF_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CDbxMDBX::TouchFile()
{
	SYSTEMTIME st;
	::GetLocalTime(&st);

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	HANDLE hFile = CreateFileW(m_tszProfileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile) {
		SetFileTime(hFile, nullptr, &ft, &ft);
		CloseHandle(hFile);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

static VOID CALLBACK DoBufferFlushTimerProc(HWND hwnd, UINT, UINT_PTR idEvent, DWORD)
{
	KillTimer(hwnd, idEvent);

	CDbxMDBX *pDb = (CDbxMDBX *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (pDb)
		pDb->DBFlush(true);
}

void CDbxMDBX::DBFlush(bool bForce)
{
	if (bForce) {
		mdbx_env_sync(m_env, true);
	}
	else if (m_safetyMode) {
		::KillTimer(m_hwndTimer, 1);
		::SetTimer(m_hwndTimer, 1, 50, DoBufferFlushTimerProc);
	}
}
