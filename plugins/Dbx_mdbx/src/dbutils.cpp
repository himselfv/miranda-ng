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

#define CMP_UINT(x, y) { if ((x) != (y)) return (x) < (y) ? -1 : 1; }

int DBEventSortingKey::Compare(const MDBX_val *ax, const MDBX_val *bx)
{
	const DBEventSortingKey *a = (DBEventSortingKey *)ax->iov_base;
	const DBEventSortingKey *b = (DBEventSortingKey *)bx->iov_base;

	CMP_UINT(a->hContact, b->hContact);
	CMP_UINT(a->ts, b->ts);
	CMP_UINT(a->hEvent, b->hEvent);
	return 0;
}

int DBSettingKey::Compare(const MDBX_val *ax, const MDBX_val *bx)
{
	const DBSettingKey *a = (DBSettingKey *)ax->iov_base;
	const DBSettingKey *b = (DBSettingKey *)bx->iov_base;

	CMP_UINT(a->hContact, b->hContact);
	CMP_UINT(a->dwModuleId, b->dwModuleId);
	return strcmp(a->szSettingName, b->szSettingName);
}

/////////////////////////////////////////////////////////////////////////////////////////

txn_ptr_ro::txn_ptr_ro(CMDBX_txn_ro &_txn) : 
	txn(_txn),
	lock(txn.cs)
{
	for (int nRetries = 0; nRetries < 5; nRetries++) {
		int rc = mdbx_txn_renew(txn);
		if (rc == MDBX_SUCCESS)
			break;

		#ifdef _DEBUG
			DebugBreak();
		#endif
		Netlib_Logf(nullptr, "txn_ptr_ro::txn_ptr_ro failed with error=%d, retrying...", rc);
		Sleep(0);
	}
}

txn_ptr_ro::~txn_ptr_ro()
{
	for (int nRetries = 0; nRetries < 5; nRetries++) {
		int rc = mdbx_txn_reset(txn);
		if (rc == MDBX_SUCCESS)
			break;

		#ifdef _DEBUG
			DebugBreak();
		#endif
		Netlib_Logf(nullptr, "txn_ptr_ro::~txn_ptr_ro failed with error=%d, retrying...", rc);
		Sleep(0);
	}
}
