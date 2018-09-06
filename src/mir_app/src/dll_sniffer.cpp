/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-18 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdafx.h"
#include "plugins.h"

DWORD dwVersion = 0;

static void ProcessResourcesDirectory(PIMAGE_RESOURCE_DIRECTORY pIRD, PBYTE pBase, DWORD dwType);

static void ProcessResourceEntry(PIMAGE_RESOURCE_DIRECTORY_ENTRY pIRDE, PBYTE pBase, DWORD dwType)
{
	if (pIRDE->DataIsDirectory)
		ProcessResourcesDirectory(PIMAGE_RESOURCE_DIRECTORY(pBase + pIRDE->OffsetToDirectory), pBase, dwType == 0 ? pIRDE->Name : dwType);
	else if (dwType == 16) {
		PIMAGE_RESOURCE_DATA_ENTRY pItem = PIMAGE_RESOURCE_DATA_ENTRY(pBase + pIRDE->OffsetToData);
		dwVersion = pItem->OffsetToData;
	}
}

static void ProcessResourcesDirectory(PIMAGE_RESOURCE_DIRECTORY pIRD, PBYTE pBase, DWORD dwType)
{
	UINT i;

	PIMAGE_RESOURCE_DIRECTORY_ENTRY pIRDE = PIMAGE_RESOURCE_DIRECTORY_ENTRY(pIRD + 1);
	for (i = 0; i < pIRD->NumberOfNamedEntries; i++, pIRDE++)
		ProcessResourceEntry(pIRDE, pBase, dwType);

	for (i = 0; i < pIRD->NumberOfIdEntries; i++, pIRDE++)
		ProcessResourceEntry(pIRDE, pBase, dwType);
}

__forceinline bool Contains(PIMAGE_SECTION_HEADER pISH, DWORD address, DWORD size = 0)
{
	return (address >= pISH->VirtualAddress && address + size <= pISH->VirtualAddress + pISH->SizeOfRawData);
}

MUUID* GetPluginInterfaces(const wchar_t *ptszFileName, bool &bIsPlugin)
{
	int nChecks = 0;
	bIsPlugin = false;

	HANDLE hFile = CreateFile(ptszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;

	MUUID *pResult = nullptr;
	BYTE *ptr = nullptr;
	HANDLE hMap = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);

	__try {
		__try {
			if (!hMap)
				__leave;

			DWORD dwHSize = 0, filesize = GetFileSize(hFile, &dwHSize);
			if (!filesize || filesize == INVALID_FILE_SIZE || dwHSize)
				__leave;

			if (filesize < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS))
				__leave;

			ptr = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if (ptr == nullptr)
				__leave;

			PIMAGE_NT_HEADERS pINTH = {};
			PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)ptr;
			if (pIDH->e_magic == IMAGE_DOS_SIGNATURE)
				pINTH = (PIMAGE_NT_HEADERS)(ptr + pIDH->e_lfanew);
			else
				__leave;

			if ((PBYTE)pINTH + sizeof(IMAGE_NT_HEADERS) >= ptr + filesize)
				__leave;
			if (pINTH->Signature != IMAGE_NT_SIGNATURE)
				__leave;

			DWORD nSections = pINTH->FileHeader.NumberOfSections;
			if (!nSections)
				__leave;

			// try to found correct offset independent of architectures
			INT_PTR base;
			PIMAGE_DATA_DIRECTORY pIDD;
			if (pINTH->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
				pINTH->FileHeader.SizeOfOptionalHeader >= sizeof(IMAGE_OPTIONAL_HEADER32) &&
				pINTH->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
				pIDD = (PIMAGE_DATA_DIRECTORY)((PBYTE)pINTH + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory));
				base = *(DWORD*)((PBYTE)pINTH + offsetof(IMAGE_NT_HEADERS32, OptionalHeader.ImageBase));
			}
			else if (pINTH->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
				pINTH->FileHeader.SizeOfOptionalHeader >= sizeof(IMAGE_OPTIONAL_HEADER64) &&
				pINTH->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
				pIDD = (PIMAGE_DATA_DIRECTORY)((PBYTE)pINTH + offsetof(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory));
				base = *(ULONGLONG*)((PBYTE)pINTH + offsetof(IMAGE_NT_HEADERS64, OptionalHeader.ImageBase));
			}
			else __leave;

			// Resource directory
			DWORD resAddr = pIDD[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
			DWORD resSize = pIDD[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
			if (resSize < sizeof(IMAGE_EXPORT_DIRECTORY)) __leave;

			// Export information entry
			DWORD expAddr = pIDD[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
			DWORD expSize = pIDD[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			if (expSize == 0)
				nChecks++;
			else if (expSize < sizeof(IMAGE_EXPORT_DIRECTORY))
				__leave;

			BYTE* pImage = ptr + pIDH->e_lfanew + pINTH->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_NT_HEADERS) - sizeof(IMAGE_OPTIONAL_HEADER);

			for (DWORD idx = 0; idx < nSections; idx++) {
				PIMAGE_SECTION_HEADER pISH = (PIMAGE_SECTION_HEADER)(pImage + idx * sizeof(IMAGE_SECTION_HEADER));
				if (((PBYTE)pISH + sizeof(IMAGE_SECTION_HEADER) > pImage + filesize) || (pISH->PointerToRawData + pISH->SizeOfRawData > filesize))
					__leave;

				// process export table
				if (expSize >= sizeof(IMAGE_EXPORT_DIRECTORY) && Contains(pISH, expAddr, expSize)) {
					BYTE *pSecStart = ptr + pISH->PointerToRawData - pISH->VirtualAddress;
					IMAGE_EXPORT_DIRECTORY *pED = (PIMAGE_EXPORT_DIRECTORY)&pSecStart[expAddr];
					DWORD *ptrRVA = (DWORD*)&pSecStart[pED->AddressOfNames];
					WORD  *ptrOrdRVA = (WORD*)&pSecStart[pED->AddressOfNameOrdinals];
					DWORD *ptrFuncList = (DWORD*)&pSecStart[pED->AddressOfFunctions];

					MUUID *pIds = nullptr;
					bool bHasMuuids = false;
					for (size_t i = 0; i < pED->NumberOfNames; i++, ptrRVA++, ptrOrdRVA++) {
						char *szName = (char*)&pSecStart[*ptrRVA];
						if (!mir_strcmp(szName, "MirandaInterfaces")) {
							bHasMuuids = true;
							pIds = (MUUID*)&pSecStart[ptrFuncList[*ptrOrdRVA]];
						}
					}

					nChecks++;

					// a plugin might have no interfaces
					if (bHasMuuids) {
						int nLength = 1; // one for MIID_LAST
						for (MUUID *p = pIds; *p != miid_last; p++)
							nLength++;

						pResult = (MUUID*)mir_alloc(sizeof(MUUID)*nLength);
						if (pResult)
							memcpy(pResult, pIds, sizeof(MUUID)*nLength);
					}
				}

				// process resource version
				if (resSize > 0 && Contains(pISH, resAddr, resSize)) {
					dwVersion = 0;

					BYTE *pSecStart = ptr + pISH->PointerToRawData - pISH->VirtualAddress;
					IMAGE_RESOURCE_DIRECTORY *pIRD = (IMAGE_RESOURCE_DIRECTORY*)&pSecStart[resAddr];
					ProcessResourcesDirectory(pIRD, &pSecStart[resAddr], 0);

					// patch version
					if (dwVersion) {
						BYTE *pVersionRes = &pSecStart[dwVersion];
						size_t cbLen = *(WORD*)pVersionRes;
						mir_ptr<BYTE> pData((BYTE*)mir_alloc(cbLen));
						memcpy(pData, pVersionRes, cbLen);

						UINT blockSize;
						VS_FIXEDFILEINFO *vsffi;
						VerQueryValue(pData, L"\\", (PVOID*)&vsffi, &blockSize);

						UINT v[4] = { MIRANDA_VERSION_COREVERSION };
						if (MAKELONG(v[1], v[0]) == (int)vsffi->dwProductVersionMS && MAKELONG(v[3], v[2]) == (int)vsffi->dwProductVersionLS)
							nChecks++;
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{};
	}
	__finally {
		if (ptr) UnmapViewOfFile(ptr);
		if (hMap) CloseHandle(hMap);
		CloseHandle(hFile);
	};

	bIsPlugin = nChecks == 2;
	return pResult;
}
