/*
	 Variables Plugin for Miranda-IM (www.miranda-im.org)
	 Copyright 2003-2006 P. Boon

	 This program is mir_free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation; either version 2 of the License, or
	 (at your option) any later version.

	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with this program; if not, write to the Free Software
	 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "stdafx.h"

/* some handles */
static HANDLE
	hOptionsHook = nullptr,
	hIconsChangedHook = nullptr;

HCURSOR hCurSplitNS;

struct ParseOptions gParseOpts;

wchar_t* getArguments(wchar_t *string, TArgList &argv)
{
	wchar_t *cur = string;
	while (*cur == ' ')
		cur++;

	if (*cur != '(')
		return nullptr;

	wchar_t *scur = cur;
	cur++;
	int brackets = 0;
	bool bDontParse = false, bNewArg = false, bDone = false;
	while (!bDone && *cur != 0) {
		switch (*cur) {
		case DONTPARSE_CHAR:
			bDontParse = !bDontParse;
			break;

		case ',':
			if ((!bDontParse) && (brackets == 0))
				bNewArg = true;
			break;

		case '(':
			if (!bDontParse)
				brackets++;
			break;

		case ')':
			if (brackets == 0 && !bDontParse)
				bDone = bNewArg = TRUE;
			else if (brackets > 0 && !bDontParse)
				brackets--;
			break;
		}
		
		if (bNewArg) {
			wchar_t *tszArg = nullptr;
			if (cur > scur)
				tszArg = mir_wstrndup(scur + 1, cur - (scur + 1));
			if (tszArg == nullptr)
				tszArg = mir_wstrdup(L"");
			argv.insert(tszArg);

			bNewArg = false;
			scur = cur;
		}
		cur++;
	}

	// set args
	if (cur[-1] != ')') {
		argv.destroy();
		return nullptr;
	}

	return cur;
}

int isValidTokenChar(wchar_t tc)
{

	return
		(tc != '(') &&
		(tc != ',') &&
		(tc != ')') &&
		(tc != FIELD_CHAR) &&
		(tc != FUNC_CHAR) &&
		(tc != FUNC_ONCE_CHAR) &&
		(tc != '/') &&
		(tc != 0);
}

/* pretty much the main loop */
static wchar_t* replaceDynVars(FORMATINFO *fi)
{
	if (fi->szFormat.w == nullptr)
		return nullptr;

	int i, scurPos, curPos, tmpVarPos;

	wchar_t *string = mir_wstrdup(fi->szFormat.w);
	if (string == nullptr)
		return nullptr;

	TArgList argv;

	FORMATINFO afi;
	memcpy(&afi, fi, sizeof(afi));

	for (size_t pos = 0; pos < mir_wstrlen(string); pos++) {
		// string may move in memory, iterate by remembering the position in the string
		wchar_t *cur = string + pos;

		// new round
		if (*cur == DONTPARSE_CHAR) {
			memmove(cur, cur + 1, (mir_wstrlen(cur + 1) + 1)*sizeof(wchar_t));
			if (*cur == DONTPARSE_CHAR)
				continue;

			while ((*cur != DONTPARSE_CHAR) && (*cur != 0))
				cur++;

			memmove(cur, cur + 1, (mir_wstrlen(cur + 1) + 1)*sizeof(wchar_t));
			pos = cur - string - 1;
			continue;
		}
		// remove end of lines
		else if ((!wcsncmp(cur, L"\r\n", 2)) && (gParseOpts.bStripEOL)) {
			memmove(cur, cur + 2, (mir_wstrlen(cur + 2) + 1)*sizeof(wchar_t));
			pos = cur - string - 1;
			continue;
		}
		else if ((*cur == '\n' && gParseOpts.bStripEOL) || (*cur == ' ' && gParseOpts.bStripWS)) {
			memmove(cur, cur + 1, (mir_wstrlen(cur + 1) + 1)*sizeof(wchar_t));
			pos = cur - string - 1;
			continue;
		}
		// remove comments
		else if (!wcsncmp(cur, _A2W(COMMENT_STRING), _countof(COMMENT_STRING))) {
			wchar_t *scur = cur;
			while (wcsncmp(cur, L"\r\n", 2) && *cur != '\n' && *cur != 0)
				cur++;

			if (*cur == 0) {
				*scur = 0;
				string = (wchar_t*)mir_realloc(string, (mir_wstrlen(string) + 1)*sizeof(wchar_t));
				continue;
			}
			memmove(scur, cur, (mir_wstrlen(cur) + 1)*sizeof(wchar_t));
			pos = scur - string - 1;
			continue;
		}
		else if ((*cur != FIELD_CHAR) && (*cur != FUNC_CHAR) && (*cur != FUNC_ONCE_CHAR)) {
			if (gParseOpts.bStripAll) {
				memmove(cur, cur + 1, (mir_wstrlen(cur + 1) + 1)*sizeof(wchar_t));
				pos = cur - string - 1;
			}
			continue;
		}

		wchar_t *scur = cur + 1, *tcur = scur;
		while (isValidTokenChar(*tcur))
			tcur++;

		if (tcur == cur) {
			fi->eCount++;
			continue;
		}
		
		TOKENREGISTEREX *tr = nullptr;
		{
			ptrW token(mir_wstrndup(cur + 1, tcur - scur));

			// cur points to FIELD_CHAR or FUNC_CHAR
			tmpVarPos = -1;
			if (*cur == FIELD_CHAR) {
				for (i = 0; i < fi->cbTemporaryVarsSize; i += 2) {
					if (!mir_wstrcmp(fi->szTemporaryVars.w[i], token)) {
						tmpVarPos = i;
						break;
					}
				}
			}

			if (tmpVarPos < 0)
				tr = searchRegister(token, (*cur == FIELD_CHAR) ? TRF_FIELD : TRF_FUNCTION);
		}

		if (tmpVarPos < 0 && tr == nullptr) {
			fi->eCount++;
			// token not found, continue
			continue;
		}

		scur = cur; // store this pointer for later use
		if (*cur == FIELD_CHAR) {
			size_t len = mir_wstrlen(tr != nullptr ? tr->szTokenString.w : fi->szTemporaryVars.w[tmpVarPos]);
			cur++;
			if (cur[len] != FIELD_CHAR) { // the next char after the token should be %
				fi->eCount++;
				continue;
			}
			cur += len + 1;
		}
		else if ((*cur == FUNC_CHAR) || (*cur == FUNC_ONCE_CHAR)) {
			cur += mir_wstrlen(tr->szTokenString.w) + 1;
			wchar_t *argcur = getArguments(cur, argv);
			if (argcur == cur || argcur == nullptr) {
				fi->eCount++;
				// error getting arguments
				continue;
			}
			cur = argcur;
			// arguments
			for (i = 0; i < argv.getCount(); i++) {
				if (tr->flags & TRF_UNPARSEDARGS)
					continue;

				afi.szFormat.w = argv[i];
				afi.eCount = afi.pCount = 0;
				argv.put(i, formatString(&afi));
				fi->eCount += afi.eCount;
				fi->pCount += afi.pCount;
				mir_free(afi.szFormat.w);
			}
		}

		// cur should now point at the character after FIELD_CHAR or after the last ')'
		ARGUMENTSINFO ai = { 0 };
		ptrW parsedToken;
		if (tr != nullptr) {
			argv.insert(mir_wstrdup(tr->szTokenString.w), 0);

			ai.cbSize = sizeof(ai);
			ai.argc = argv.getCount();
			ai.argv.w = argv.getArray();
			ai.fi = fi;
			if ((*scur == FUNC_ONCE_CHAR) || (*scur == FIELD_CHAR))
				ai.flags |= AIF_DONTPARSE;

			parsedToken = parseFromRegister(&ai);
		}
		else parsedToken = mir_wstrdup(fi->szTemporaryVars.w[tmpVarPos + 1]);

		argv.destroy();

		if (parsedToken == NULL) {
			fi->eCount++;
			continue;
		}

		// replaced a var
		if (ai.flags & AIF_FALSE)
			fi->eCount++;
		else
			fi->pCount++;

		size_t parsedTokenLen = mir_wstrlen(parsedToken);
		size_t initStrLen = mir_wstrlen(string);
		size_t tokenLen = cur - scur;
		scurPos = scur - string;
		curPos = cur - string;
		if (tokenLen < parsedTokenLen) {
			// string needs more memory
			string = (wchar_t*)mir_realloc(string, (initStrLen - tokenLen + parsedTokenLen + 1)*sizeof(wchar_t));
			if (string == nullptr) {
				fi->eCount++;
				return nullptr;
			}
		}
		scur = string + scurPos;
		cur = string + curPos;
		memmove(scur + parsedTokenLen, cur, (mir_wstrlen(cur) + 1)*sizeof(wchar_t));
		memcpy(scur, parsedToken, parsedTokenLen*sizeof(wchar_t));
		{
			size_t len = mir_wstrlen(string);
			string = (wchar_t*)mir_realloc(string, (len + 1)*sizeof(wchar_t));
		}
		if ((ai.flags & AIF_DONTPARSE) || tmpVarPos >= 0)
			pos += parsedTokenLen;

		pos--; // parse the same pos again, it changed
	}

	return (wchar_t*)mir_realloc(string, (mir_wstrlen(string) + 1)*sizeof(wchar_t));
}

/*
	MS_VARS_FORMATSTRING
*/
static INT_PTR formatStringService(WPARAM wParam, LPARAM)
{
	FORMATINFO *pfi = (FORMATINFO*)wParam;
	if (pfi->cbSize != sizeof(FORMATINFO))
		return 0;

	// prevent the original structure from being altered
	FORMATINFO tmpfi = *pfi;
	bool copied;
	wchar_t *tszFormat, *tszSource;
	if (tmpfi.flags & FIF_UNICODE) {
		copied = false;
		tszFormat = tmpfi.szFormat.w;
		tszSource = tmpfi.szExtraText.w;
	}
	else {
		copied = true;
		tszFormat = mir_a2u(tmpfi.szFormat.a);
		tszSource = mir_a2u(tmpfi.szExtraText.a);
		for (int i = 0; i < tmpfi.cbTemporaryVarsSize; i++)
			tmpfi.szTemporaryVars.w[i] = mir_a2u(tmpfi.szTemporaryVars.a[i]);
	}

	tmpfi.szFormat.w = tszFormat;
	tmpfi.szExtraText.w = tszSource;

	wchar_t *tRes = formatString(&tmpfi);

	INT_PTR res;
	if (!(tmpfi.flags & FIF_UNICODE)) {
		res = (INT_PTR)mir_u2a(tRes);
		mir_free(tRes);
	}
	else res = (INT_PTR)tRes;

	if (copied) {
		mir_free(tszFormat);
		mir_free(tszSource);
		for (int i = 0; i < tmpfi.cbTemporaryVarsSize; i++)
			mir_free(tmpfi.szTemporaryVars.w);
	}

	((FORMATINFO *)wParam)->eCount = tmpfi.eCount;
	((FORMATINFO *)wParam)->pCount = tmpfi.pCount;
	return res;
}

wchar_t* formatString(FORMATINFO *fi)
{
	if (fi == nullptr)
		return nullptr;
	/* the service to format a given string */
	if ((fi->eCount + fi->pCount) > 5000) {
		fi->eCount++;
		fi->pCount++;
		log_debugA("Variables: Overflow protection; %d parses", (fi->eCount + fi->pCount));
		return nullptr;
	}

	return replaceDynVars(fi);
}

int setParseOptions(struct ParseOptions *po)
{
	if (po == nullptr)
		po = &gParseOpts;

	memset(po, 0, sizeof(struct ParseOptions));
	if (!db_get_b(NULL, MODULENAME, SETTING_STRIPALL, 0)) {
		po->bStripEOL = db_get_b(NULL, MODULENAME, SETTING_STRIPCRLF, 0);
		po->bStripWS = db_get_b(NULL, MODULENAME, SETTING_STRIPWS, 0);
	}
	else po->bStripAll = TRUE;

	return 0;
}

static IconItem iconList[] = 
{
	{ LPGEN("Help"), "vars_help", IDI_V }
};

int LoadVarModule()
{
	if (initTokenRegister() != 0 || initContactModule() != 0)
		return -1;

	setParseOptions(nullptr);
	CreateServiceFunction(MS_VARS_FORMATSTRING, formatStringService);
	CreateServiceFunction(MS_VARS_REGISTERTOKEN, registerToken);
	// help dialog
	hCurSplitNS = LoadCursor(nullptr, IDC_SIZENS);

	CreateServiceFunction(MS_VARS_SHOWHELP, showHelpService);
	CreateServiceFunction(MS_VARS_SHOWHELPEX, showHelpExService);

	g_plugin.registerIcon(LPGEN("Variables"), iconList);

	hIconsChangedHook = HookEvent(ME_SKIN2_ICONSCHANGED, iconsChanged);

	CreateServiceFunction(MS_VARS_GETSKINITEM, getSkinItemService);
	hOptionsHook = HookEvent(ME_OPT_INITIALISE, OptionsInit);

	// register internal tokens
	registerExternalTokens();
	registerLogicTokens();
	registerMathTokens();
	registerMirandaTokens();
	registerStrTokens();
	registerSystemTokens();
	registerVariablesTokens();
	registerRegExpTokens();
	registerInetTokens();
	registerAliasTokens();
	registerMetaContactsTokens();

	log_debugA("Variables: Internal tokens registered");

	if (db_get_b(NULL, MODULENAME, SETTING_PARSEATSTARTUP, 0)) {
		FORMATINFO fi = { 0 };
		fi.cbSize = sizeof(fi);
		fi.szFormat.w = db_get_wsa(NULL, MODULENAME, SETTING_STARTUPTEXT);
		if (fi.szFormat.w != nullptr) {
			mir_free(formatString(&fi));
			mir_free(fi.szFormat.w);
		}
	}
	log_debugA("Variables: Init done");

	return 0;
}

int UnloadVarModule()
{
	UnhookEvent(hOptionsHook);
	if (hIconsChangedHook != nullptr)
		UnhookEvent(hIconsChangedHook);

	DestroyCursor(hCurSplitNS);
	deinitContactModule();
	deinitTokenRegister();
	unregisterAliasTokens();
	unregisterVariablesTokens();
	return 0;
}
