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

STDMETHODIMP_(LONG) CDbxMDBX::GetEventCount(MCONTACT contactID)
{
	if (!contactID)
		return m_ccDummy.dbc.dwEventCount;

	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	return (cc == nullptr) ? 0 : cc->dbc.dwEventCount;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::AddEvent(MCONTACT contactID, DBEVENTINFO *dbei)
{
	if (dbei == nullptr) return 0;
	if (dbei->timestamp == 0) return 0;

	DBEvent dbe;
	dbe.contactID = contactID; // store native or subcontact's id
	dbe.iModuleId = GetModuleID(dbei->szModule);

	MCONTACT contactNotifyID = contactID;
	DBCachedContact *cc, *ccSub = nullptr;
	if (contactID != 0) {
		if ((cc = m_cache->GetCachedContact(contactID)) == nullptr)
			return 0;

		if (cc->IsSub()) {
			ccSub = cc;
			if ((cc = m_cache->GetCachedContact(cc->parentID)) == nullptr)
				return 0;

			// set default sub to the event's source
			if (!(dbei->flags & DBEF_SENT))
				db_mc_setDefault(cc->contactID, contactID, false);
			contactID = cc->contactID; // and add an event to a metahistory
			if (db_mc_isEnabled())
				contactNotifyID = contactID;
		}
	}
	else cc = &m_ccDummy;

	if (m_safetyMode)
		if (NotifyEventHooks(hEventFilterAddedEvent, contactNotifyID, (LPARAM)dbei))
			return 0;

	dbe.timestamp = dbei->timestamp;
	dbe.flags = dbei->flags;
	dbe.wEventType = dbei->eventType;
	dbe.cbBlob = dbei->cbBlob;
	BYTE *pBlob = dbei->pBlob;

	mir_ptr<BYTE> pCryptBlob;
	if (m_bEncrypted) {
		size_t len;
		BYTE *pResult = m_crypto->encodeBuffer(pBlob, dbe.cbBlob, &len);
		if (pResult != nullptr) {
			pCryptBlob = pBlob = pResult;
			dbe.cbBlob = (uint16_t)len;
			dbe.flags |= DBEF_ENCRYPTED;
		}
	}

	MEVENT dwEventId = InterlockedIncrement(&m_dwMaxEventId);
	{
		BYTE *recBuf = (BYTE*)_alloca(sizeof(DBEvent) + dbe.cbBlob);
		DBEvent *pNewEvent = (DBEvent*)recBuf;
		*pNewEvent = dbe;
		memcpy(pNewEvent + 1, pBlob, dbe.cbBlob);

		txn_ptr trnlck(StartTran());
		MDBX_val key = { &dwEventId, sizeof(MEVENT) }, data = { recBuf, sizeof(DBEvent) + dbe.cbBlob };
		if (mdbx_put(trnlck, m_dbEvents, &key, &data, 0) != MDBX_SUCCESS)
			return 0;

		// add a sorting key
		DBEventSortingKey key2 = { contactID, dwEventId, dbe.timestamp };
		key.iov_len = sizeof(key2); key.iov_base = &key2;
		data.iov_len = 1; data.iov_base = (char*)("");
		if (mdbx_put(trnlck, m_dbEventsSort, &key, &data, 0) != MDBX_SUCCESS)
			return 0;

		cc->Advance(dwEventId, dbe);
		if (contactID != 0) {
			MDBX_val keyc = { &contactID, sizeof(MCONTACT) }, datac = { &cc->dbc, sizeof(DBContact) };
			if (mdbx_put(trnlck, m_dbContacts, &keyc, &datac, 0) != MDBX_SUCCESS)
				return 0;

			// insert an event into a sub's history too
			if (ccSub != nullptr) {
				key2.hContact = ccSub->contactID;
				if (mdbx_put(trnlck, m_dbEventsSort, &key, &data, 0) != MDBX_SUCCESS)
					return 0;

				ccSub->Advance(dwEventId, dbe);
				datac.iov_base = &ccSub->dbc;
				keyc.iov_base = &ccSub->contactID;
				if (mdbx_put(trnlck, m_dbContacts, &keyc, &datac, 0) != MDBX_SUCCESS)
					return 0;
			}
		}
		else {
			uint32_t keyVal = 2;
			MDBX_val keyc = { &keyVal, sizeof(keyVal) }, datac = { &m_ccDummy.dbc, sizeof(m_ccDummy.dbc) };
			if (mdbx_put(trnlck, m_dbGlobal, &keyc, &datac, 0) != MDBX_SUCCESS)
				return 0;
		}

		if (trnlck.commit() != MDBX_SUCCESS)
			return 0;
	}

	DBFlush();

	// Notify only in safe mode or on really new events
	if (m_safetyMode)
		NotifyEventHooks(hEventAddedEvent, contactNotifyID, dwEventId);

	return dwEventId;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) CDbxMDBX::DeleteEvent(MCONTACT contactID, MEVENT hDbEvent)
{
	DBCachedContact *cc = (contactID != 0) ? m_cache->GetCachedContact(contactID) : &m_ccDummy, *cc2 = nullptr;
	if (cc == nullptr || cc->dbc.dwEventCount == 0)
		return 1;

	DBEvent dbe;
	{
		txn_ptr_ro txn(m_txn_ro);
		MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
		if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
			return 1;
		dbe = *(DBEvent*)data.iov_base;
	}

	// if we removing the sub's event using metacontact's contactID
	// we also need to remove this event from sub's history
	if (contactID != dbe.contactID)
		cc2 = m_cache->GetCachedContact(dbe.contactID);
	// or, if we removing the sub's event using sub's contactID
	// we also need to remove it from meta's history
	else if (cc->IsSub())
		cc2 = m_cache->GetCachedContact(cc->parentID);

	{
		txn_ptr trnlck(StartTran());
		DBEventSortingKey key2 = { contactID, hDbEvent, dbe.timestamp };
		MDBX_val key = { &key2, sizeof(key2) }, data;

		if (mdbx_del(trnlck, m_dbEventsSort, &key, nullptr) != MDBX_SUCCESS)
			return 1;

		if (contactID != 0) {
			cc->dbc.dwEventCount--;
			if (cc->dbc.evFirstUnread == hDbEvent)
				FindNextUnread(trnlck, cc, key2);

			MDBX_val keyc = { &contactID, sizeof(MCONTACT) };
			data.iov_len = sizeof(DBContact); data.iov_base = &cc->dbc;
			if (mdbx_put(trnlck, m_dbContacts, &keyc, &data, 0) != MDBX_SUCCESS)
				return 1;
		}
		else {
			m_ccDummy.dbc.dwEventCount--;
			if (m_ccDummy.dbc.evFirstUnread == hDbEvent)
				FindNextUnread(trnlck, &m_ccDummy, key2);

			uint32_t keyVal = 2;
			MDBX_val keyc = { &keyVal, sizeof(keyVal) }, datac = { &m_ccDummy.dbc, sizeof(m_ccDummy.dbc) };
			if (mdbx_put(trnlck, m_dbGlobal, &keyc, &datac, 0) != MDBX_SUCCESS)
				return 0;
		}

		if (cc2) {
			key2.hContact = cc2->contactID;
			if (mdbx_del(trnlck, m_dbEventsSort, &key, nullptr) != MDBX_SUCCESS)
				return 1;

			key.iov_len = sizeof(MCONTACT); key.iov_base = &contactID;
			cc2->dbc.dwEventCount--;
			if (cc2->dbc.evFirstUnread == hDbEvent)
				FindNextUnread(trnlck, cc2, key2);

			MDBX_val keyc = { &cc2->contactID, sizeof(MCONTACT) };
			data.iov_len = sizeof(DBContact); data.iov_base = &cc2->dbc;
			if (mdbx_put(trnlck, m_dbContacts, &keyc, &data, 0) != MDBX_SUCCESS)
				return 1;
		}

		// remove an event
		key.iov_len = sizeof(MEVENT); key.iov_base = &hDbEvent;
		if (mdbx_del(trnlck, m_dbEvents, &key, nullptr) != MDBX_SUCCESS)
			return 1;

		if (trnlck.commit() != MDBX_SUCCESS)
			return 1;
	}

	DBFlush();
	NotifyEventHooks(hEventDeletedEvent, contactID, hDbEvent);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(LONG) CDbxMDBX::GetBlobSize(MEVENT hDbEvent)
{
	txn_ptr_ro txn(m_txn_ro);

	MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
	if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
		return -1;
	return ((const DBEvent*)data.iov_base)->cbBlob;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) CDbxMDBX::GetEvent(MEVENT hDbEvent, DBEVENTINFO *dbei)
{
	if (dbei == nullptr) return 1;
	if (dbei->cbBlob > 0 && dbei->pBlob == nullptr) {
		dbei->cbBlob = 0;
		return 1;
	}

	const DBEvent *dbe;
	{
		txn_ptr_ro txn(m_txn_ro);

		MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
		if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
			return 1;

		dbe = (const DBEvent*)data.iov_base;
	}

	dbei->szModule = GetModuleName(dbe->iModuleId);
	dbei->timestamp = dbe->timestamp;
	dbei->flags = dbe->flags;
	dbei->eventType = dbe->wEventType;
	size_t bytesToCopy = min(dbei->cbBlob, dbe->cbBlob);
	dbei->cbBlob = dbe->cbBlob;
	if (bytesToCopy && dbei->pBlob) {
		BYTE *pSrc = (BYTE*)dbe + sizeof(DBEvent);
		if (dbe->flags & DBEF_ENCRYPTED) {
			dbei->flags &= ~DBEF_ENCRYPTED;
			size_t len;
			BYTE* pBlob = (BYTE*)m_crypto->decodeBuffer(pSrc, dbe->cbBlob, &len);
			if (pBlob == nullptr)
				return 1;

			memcpy(dbei->pBlob, pBlob, bytesToCopy);
			if (bytesToCopy > len)
				memset(dbei->pBlob + len, 0, bytesToCopy - len);
			mir_free(pBlob);
		}
		else memcpy(dbei->pBlob, pSrc, bytesToCopy);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

void CDbxMDBX::FindNextUnread(const txn_ptr &txn, DBCachedContact *cc, DBEventSortingKey &key2)
{
	cursor_ptr cursor(txn, m_dbEventsSort);

	MDBX_val key = { &key2, sizeof(key2) }, data;

	for (int res = mdbx_cursor_get(cursor, &key, &data, MDBX_SET_KEY); res == MDBX_SUCCESS; res = mdbx_cursor_get(cursor, &key, &data, MDBX_NEXT)) {
		const DBEvent *dbe = (const DBEvent*)data.iov_base;
		if (dbe->contactID != cc->contactID)
			break;
		if (!dbe->markedRead()) {
			cc->dbc.evFirstUnread = key2.hEvent;
			cc->dbc.tsFirstUnread = key2.ts;
			return;
		}
	}

	cc->dbc.evFirstUnread = cc->dbc.tsFirstUnread = 0;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) CDbxMDBX::MarkEventRead(MCONTACT contactID, MEVENT hDbEvent)
{
	if (hDbEvent == 0) return -1;

	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	if (cc == nullptr)
		return -1;

	uint32_t wRetVal = -1;
	{
		txn_ptr trnlck(StartTran());
		MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
		if (mdbx_get(trnlck, m_dbEvents, &key, &data) != MDBX_SUCCESS)
			return -1;

		const DBEvent *cdbe = (const DBEvent*)data.iov_base;
		if (cdbe->markedRead())
			return cdbe->flags;

		void *recBuf = _alloca(data.iov_len);
		memcpy(recBuf, data.iov_base, data.iov_len);
		data.iov_base = recBuf;

		DBEvent *pNewEvent = (DBEvent*)data.iov_base;
		wRetVal = (pNewEvent->flags |= DBEF_READ);

		DBEventSortingKey keyVal = { contactID, hDbEvent, cdbe->timestamp };
		if (mdbx_put(trnlck, m_dbEvents, &key, &data, 0) != MDBX_SUCCESS)
			return -1;

		FindNextUnread(trnlck, cc, keyVal);
		key.iov_len = sizeof(MCONTACT); key.iov_base = &contactID;
		data.iov_base = &cc->dbc; data.iov_len = sizeof(cc->dbc);
		if (mdbx_put(trnlck, m_dbContacts, &key, &data, 0) != MDBX_SUCCESS)
			return -1;

		if (trnlck.commit() != MDBX_SUCCESS)
			return -1;
	}

	DBFlush();
	NotifyEventHooks(hEventMarkedRead, contactID, (LPARAM)hDbEvent);
	return wRetVal;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MCONTACT) CDbxMDBX::GetEventContact(MEVENT hDbEvent)
{
	if (hDbEvent == 0)
		return INVALID_CONTACT_ID;

	txn_ptr_ro txn(m_txn_ro);

	MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
	if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
		return INVALID_CONTACT_ID;

	return ((const DBEvent*)data.iov_base)->contactID;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::FindFirstEvent(MCONTACT contactID)
{
	DBCachedContact *cc;
	if (contactID != 0) {
		cc = m_cache->GetCachedContact(contactID);
		if (cc == nullptr)
			return 0;
	}
	else cc = &m_ccDummy;

	DBEventSortingKey keyVal = { contactID, 0, 0 };
	MDBX_val key = { &keyVal, sizeof(keyVal) }, data;

	txn_ptr_ro txn(m_txn_ro);

	cursor_ptr_ro cursor(m_curEventsSort);
	if (mdbx_cursor_get(cursor, &key, &data, MDBX_SET_RANGE) != MDBX_SUCCESS)
		return cc->t_evLast = 0;

	const DBEventSortingKey *pKey = (const DBEventSortingKey*)key.iov_base;
	cc->t_tsLast = pKey->ts;
	return cc->t_evLast = (pKey->hContact == contactID) ? pKey->hEvent : 0;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::FindFirstUnreadEvent(MCONTACT contactID)
{
	DBCachedContact *cc = m_cache->GetCachedContact(contactID);
	return (cc == nullptr) ? 0 : cc->dbc.evFirstUnread;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::FindLastEvent(MCONTACT contactID)
{
	DBCachedContact *cc;
	if (contactID != 0) {
		cc = m_cache->GetCachedContact(contactID);
		if (cc == nullptr)
			return 0;
	}
	else cc = &m_ccDummy;

	DBEventSortingKey keyVal = { contactID, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF };
	MDBX_val key = { &keyVal, sizeof(keyVal) }, data;

	txn_ptr_ro txn(m_txn_ro);
	cursor_ptr_ro cursor(m_curEventsSort);

	if (mdbx_cursor_get(cursor, &key, &data, MDBX_SET_RANGE) != MDBX_SUCCESS) {
		if (mdbx_cursor_get(cursor, &key, &data, MDBX_LAST) != MDBX_SUCCESS)
			return cc->t_evLast = 0;
	}
	else {
		if (mdbx_cursor_get(cursor, &key, &data, MDBX_PREV) != MDBX_SUCCESS)
			return cc->t_evLast = 0;
	}

	const DBEventSortingKey *pKey = (const DBEventSortingKey*)key.iov_base;
	cc->t_tsLast = pKey->ts;
	return cc->t_evLast = (pKey->hContact == contactID) ? pKey->hEvent : 0;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::FindNextEvent(MCONTACT contactID, MEVENT hDbEvent)
{
	DBCachedContact *cc;
	if (contactID != 0) {
		cc = m_cache->GetCachedContact(contactID);
		if (cc == nullptr)
			return 0;
	}
	else cc = &m_ccDummy;

	if (hDbEvent == 0)
		return cc->t_evLast = 0;

	txn_ptr_ro txn(m_txn_ro);

	if (cc->t_evLast != hDbEvent) {
		MDBX_val key = { &hDbEvent, sizeof(MEVENT) }, data;
		if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
			return 0;
		cc->t_tsLast = ((DBEvent*)data.iov_base)->timestamp;
	}

	DBEventSortingKey keyVal = { contactID, hDbEvent, cc->t_tsLast };
	MDBX_val key = { &keyVal, sizeof(keyVal) }, data;

	cursor_ptr_ro cursor(m_curEventsSort);
	if (mdbx_cursor_get(cursor, &key, nullptr, MDBX_SET) != MDBX_SUCCESS)
		return cc->t_evLast = 0;

	if (mdbx_cursor_get(cursor, &key, &data, MDBX_NEXT) != MDBX_SUCCESS)
		return cc->t_evLast = 0;

	const DBEventSortingKey *pKey = (const DBEventSortingKey*)key.iov_base;
	cc->t_tsLast = pKey->ts;
	return cc->t_evLast = (pKey->hContact == contactID) ? pKey->hEvent : 0;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(MEVENT) CDbxMDBX::FindPrevEvent(MCONTACT contactID, MEVENT hDbEvent)
{
	DBCachedContact *cc;
	if (contactID != 0) {
		cc = m_cache->GetCachedContact(contactID);
		if (cc == nullptr)
			return 0;
	}
	else cc = &m_ccDummy;

	if (hDbEvent == 0)
		return cc->t_evLast = 0;

	MDBX_val data;

	txn_ptr_ro txn(m_txn_ro);

	if (cc->t_evLast != hDbEvent) {
		MDBX_val key = { &hDbEvent, sizeof(MEVENT) };
		if (mdbx_get(txn, m_dbEvents, &key, &data) != MDBX_SUCCESS)
			return 0;
		cc->t_tsLast = ((DBEvent*)data.iov_base)->timestamp;
	}

	DBEventSortingKey keyVal = { contactID, hDbEvent, cc->t_tsLast };
	MDBX_val key = { &keyVal, sizeof(keyVal) };

	cursor_ptr_ro cursor(m_curEventsSort);
	if (mdbx_cursor_get(cursor, &key, nullptr, MDBX_SET) != MDBX_SUCCESS)
		return cc->t_evLast = 0;

	if (mdbx_cursor_get(cursor, &key, &data, MDBX_PREV) != MDBX_SUCCESS)
		return cc->t_evLast = 0;

	const DBEventSortingKey *pKey = (const DBEventSortingKey*)key.iov_base;
	cc->t_tsLast = pKey->ts;
	return cc->t_evLast = (pKey->hContact == contactID) ? pKey->hEvent : 0;
}
