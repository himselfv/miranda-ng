/*
Copyright (C) 2009-2010 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"

AutoReplacement::AutoReplacement()
	: useVariables(FALSE)
{
}

AutoReplacement::AutoReplacement(const wchar_t *replace, BOOL useVariables)
	: replace(replace), useVariables(useVariables)
{
}


AutoReplaceMap::AutoReplaceMap(wchar_t *aFilename, Dictionary *dict)
{
	m_dict = dict;
	mir_wstrncpy(m_filename, aFilename, _countof(m_filename));
	loadAutoReplaceMap();
}

void AutoReplaceMap::loadAutoReplaceMap()
{
	FILE *file = _wfopen(m_filename, L"rb");
	if (file == nullptr)
		return;

	char tmp[1024];
	int c, pos = 0;
	while ((c = fgetc(file)) != EOF) {
		if (c == '\n' || c == '\r' || pos >= _countof(tmp) - 1) {
			if (pos > 0) {
				tmp[pos] = '\0';

				// Get from
				BOOL useVars = FALSE;
				char *p;
				if ((p = strstr(tmp, "->")) != nullptr) {
					*p = '\0';
					p += 2;
				}
				else if ((p = strstr(tmp, "-V>")) != nullptr) {
					*p = '\0';
					p += 3;
					useVars = TRUE;
				}

				if (p != nullptr) {
					ptrW find(mir_utf8decodeW(tmp));
					ptrW replace(mir_utf8decodeW(p));

					lstrtrim(find);
					lstrtrim(replace);

					if (find[0] != 0 && replace[0] != 0)
						m_replacements[find.get()] = AutoReplacement(replace, useVars);
				}
			}

			pos = 0;
		}
		else {
			tmp[pos] = (char)c;
			pos++;
		}
	}
	fclose(file);
}

void AutoReplaceMap::writeAutoReplaceMap()
{
	// Create path
	wchar_t *p = wcsrchr(m_filename, '\\');
	if (p != nullptr) {
		*p = 0;
		CreateDirectoryTreeW(m_filename);
		*p = '\\';
	}

	// Write it
	FILE *file = _wfopen(m_filename, L"wb");
	if (file != nullptr) {
		map<std::wstring, AutoReplacement>::iterator it = m_replacements.begin();
		for (; it != m_replacements.end(); it++) {
			AutoReplacement &ar = it->second;

			ptrA find(mir_utf8encodeW(it->first.c_str()));
			ptrA replace(mir_utf8encodeW(ar.replace.c_str()));

			if (ar.useVariables)
				fprintf(file, "%s-V>%s\n", (const char *)find, (const char *)replace);
			else
				fprintf(file, "%s->%s\n", (const char *)find, (const char *)replace);
		}
		fclose(file);
	}
}


BOOL AutoReplaceMap::isWordChar(wchar_t c)
{
	if (IsNumber(c))
		return TRUE;

	if (wcschr(L"-_.!@#$%&*()[]{}<>:?/\\=+", c) != nullptr)
		return TRUE;

	return m_dict->isWordChar(c);
}


CMStringW AutoReplaceMap::autoReplace(const wchar_t * word)
{
	ptrW from(wcslwr(mir_wstrdup(word)));

	if (m_replacements.find(from.get()) == m_replacements.end())
		return CMStringW();

	AutoReplacement &ar = m_replacements[from.get()];

	CMStringW ret;
	if (ar.useVariables)
		ret = ptrW(variables_parsedup((wchar_t *)ar.replace.c_str(), (wchar_t *)word, NULL));
	else
		ret = ar.replace.c_str();

	// Wich case to use?
	size_t len = mir_wstrlen(word);
	size_t i;
	for (i = 0; i < len; i++)
		if (IsCharLower(word[i]))
			break;

	if (i <= 0) // All lower
		return ret;

	if (i >= len) { // All upper
		ret.MakeUpper();
		return ret;
	}

	// First upper
	wchar_t tmp[2];
	tmp[0] = ret[0];
	tmp[1] = '\0';
	CharUpper(tmp);
	ret.SetAt(0, tmp[0]);
	return ret;
}

wchar_t* AutoReplaceMap::filterText(const wchar_t *find)
{
	wchar_t *ret = mir_wstrdup(find);
	size_t len = mir_wstrlen(ret);
	int pos = 0;
	for (size_t i = 0; i < len; i++)
		if (isWordChar(find[i]))
			ret[pos++] = ret[i];
	ret[pos] = 0;
	return CharLower(ret);
}

void AutoReplaceMap::add(const wchar_t * aFrom, const wchar_t * to, BOOL useVariables)
{
	ptrW from(filterText(aFrom));
	m_replacements[from.get()] = AutoReplacement(to, useVariables);

	writeAutoReplaceMap();
}

void AutoReplaceMap::copyMap(map<std::wstring, AutoReplacement> *replacements)
{
	*replacements = m_replacements;
}

void AutoReplaceMap::setMap(const map<std::wstring, AutoReplacement> &replacements)
{
	m_replacements.clear();

	map<std::wstring, AutoReplacement>::const_iterator it = replacements.begin();
	for (; it != replacements.end(); it++) {
		ptrW from(filterText(it->first.c_str()));
		m_replacements[from.get()] = it->second;
	}

	writeAutoReplaceMap();
}
