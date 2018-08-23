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
This has previously been included with DATABASELINK, but has been removed.
Now it's a separate interface which you can obtain from database plugins which support it
by querying Load() with DB_INTERFACE_DBCHECKERLINK (see below).
*/
struct DBCHECKERLINK
{
		/*
		Returns a pointer to the database checker or NULL if a database doesn't support checking
		When you don't need this object aanymore,  call its Destroy() method
		*/
		MIDatabaseChecker* (*CheckDB)(const wchar_t *profile, int *error);
};

/*
To extend the database with optional interfaces you can implement them in your MIDatabase* implementation,
and dynamic_cast<> to them to check availability.
But what if you need the function before the database is loaded?

Database plugins only provide DATABASELINK which these days only has Load(profile_name).
We propose to define special GUID profile_names which shall return associated structures (or error, by default).
*/
#define DB_INTERFACE_DBCHECKERLINK L"{A06CE1CF-917C-4D3A-8AB8-4651824E7A61}"

inline DBCHECKERLINK* getDBCheckerLink(DATABASELINK* dblink)
{
	if (dblink == nullptr || dblink->Load == nullptr)
		return nullptr;
	return reinterpret_cast<DBCHECKERLINK*>(dblink->Load(DB_INTERFACE_DBCHECKERLINK, true));
}



// From m_database.h

// Checks the specified profile like dbtool did.
// Implemented in the dbchecker plugins, thus it might not exist
//  wParam = (WPARAM)(wchar_t*)ptszProfileName
//  lParam = (BOOL)bConversionMode

#define MS_DB_CHECKPROFILE "DB/CheckProfile"
