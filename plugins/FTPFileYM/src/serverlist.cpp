/*
FTP File YM plugin
Copyright (C) 2007-2010 Jan Holub

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

ServerList *ServerList::instance = nullptr;
ServerList &ftpList = ServerList::getInstance();

extern Options &opt;

void ServerList::init()
{
	for (int i = 0; i < FTP_COUNT; i++) {
		ServerList::FTP *ftp = new ServerList::FTP(i);
		ftpList.add(ftp);
	}
}

void ServerList::deinit()
{
	for (UINT i = 0; i < ftpList.size(); i++)
		delete ftpList[i];

	delete this;
}

void ServerList::saveToDb() const
{
	ServerList::FTP *ftp = ftpList.getSelected();
	char buff[256];

	mir_snprintf(buff, "Password%d", opt.selected);
	DB::setAStringF(0, MODULENAME, buff, opt.selected, ftp->m_szPass);

	DB::setStringF(0, MODULENAME, "Name%d", opt.selected, ftp->m_stzName);
	DB::setAStringF(0, MODULENAME, "Server%d", opt.selected, ftp->m_szServer);
	DB::setAStringF(0, MODULENAME, "User%d", opt.selected, ftp->m_szUser);
	DB::setAStringF(0, MODULENAME, "Url%d", opt.selected, ftp->m_szUrl);
	DB::setAStringF(0, MODULENAME, "Dir%d", opt.selected, ftp->m_szDir);
	DB::setAStringF(0, MODULENAME, "Chmod%d", opt.selected, ftp->m_szChmod);
	DB::setWordF(0, MODULENAME, "FtpProto%d", opt.selected, ftp->m_ftpProto);
	DB::setWordF(0, MODULENAME, "Port%d", opt.selected, ftp->m_iPort);
	DB::setByteF(0, MODULENAME, "Passive%d", opt.selected, ftp->m_bPassive);
	DB::setByteF(0, MODULENAME, "Enabled%d", opt.selected, ftp->m_bEnabled);
	db_set_b(0, MODULENAME, "Selected", opt.selected);
	db_set_b(0, MODULENAME, "Default", opt.defaultFTP);
}

ServerList::FTP::FTP(int index)
{
	ptrA Name(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Name%d", index)));
	if (Name)
		mir_snwprintf(m_stzName, TranslateT("FTP Server %d"), index + 1);
	ptrA Pass(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Password%d", index)));
	if (Pass)
		strncpy_s(m_szPass, Pass, _TRUNCATE);
	ptrA Server(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Server%d", index)));
	if (Server)
		strncpy_s(m_szServer, Server, _TRUNCATE);
	ptrA User(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "User%d", index)));
	if (User)
		strncpy_s(m_szUser, User, _TRUNCATE);
	ptrA Url(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Url%d", index)));
	if (Url)
		strncpy_s(m_szUrl, Url, _TRUNCATE);
	ptrA Dir(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Dir%d", index)));
	if (Dir)
		strncpy_s(m_szDir, Dir, _TRUNCATE);
	ptrA Chmod(db_get_sa(NULL, MODULENAME, CMStringA(FORMAT, "Chmod%d", index)));
	if (Chmod)
		strncpy_s(m_szChmod, Chmod, _TRUNCATE);
	m_ftpProto = (FTP::EProtoType)DB::getWordF(0, MODULENAME, "FtpProto%d", index, FTP::FT_STANDARD);
	m_iPort = DB::getWordF(0, MODULENAME, "Port%d", index, 21);
	m_bPassive = DB::getByteF(0, MODULENAME, "Passive%d", index, 0) ? true : false;
	m_bEnabled = DB::getByteF(0, MODULENAME, "Enabled%d", index, 0) ? true : false;
}

ServerList::FTP* ServerList::getSelected() const
{
	return ftpList[opt.selected];
}

bool ServerList::FTP::isValid() const
{
	return (m_bEnabled && m_szServer[0] && m_szUser[0] && m_szPass[0] && m_szUrl[0]) ? true : false;
}

char* ServerList::FTP::getProtoString() const
{
	switch (m_ftpProto) {
	case FT_STANDARD:
	case FT_SSL_EXPLICIT:	return "ftp://";
	case FT_SSL_IMPLICIT:	return "ftps://";
	case FT_SSH:			return "sftp://";
	}

	return nullptr;
}
