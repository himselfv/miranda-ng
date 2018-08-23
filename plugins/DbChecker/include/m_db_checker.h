#pragma once

// From m_db_int.h

///////////////////////////////////////////////////////////////////////////////
// basic database checker interface

#define STATUS_MESSAGE    0
#define STATUS_WARNING    1
#define STATUS_ERROR      2
#define STATUS_FATAL      3
#define STATUS_SUCCESS    4

struct DBCHeckCallback
{
	int    cbSize;
	DWORD  spaceProcessed, spaceUsed;
	HANDLE hOutFile;
	int    bCheckOnly, bBackup, bAggressive, bEraseHistory, bMarkRead, bConvertUtf;

	void (*pfnAddLogMessage)(int type, const wchar_t* ptszFormat, ...);
};

interface MIDatabaseChecker
{
	STDMETHOD_(BOOL,Start)(DBCHeckCallback *callback) PURE;
	STDMETHOD_(BOOL,CheckDb)(int phase, int firstTime) PURE;
	STDMETHOD_(VOID,Destroy)() PURE;
};

/*
This also have been removed from m_db_int.h's DATABASELINK:

	/ *
	Returns a pointer to the database checker or NULL if a database doesn't support checking
	When you don't need this object aanymore,  call its Destroy() method
	* /
	MIDatabaseChecker* (*CheckDB)(const wchar_t *profile, int *error);

It's unclear how to reintegrate this atm.
*/

// From m_database.h

// Checks the specified profile like dbtool did.
// Implemented in the dbchecker plugins, thus it might not exist
//  wParam = (WPARAM)(wchar_t*)ptszProfileName
//  lParam = (BOOL)bConversionMode

#define MS_DB_CHECKPROFILE "DB/CheckProfile"
