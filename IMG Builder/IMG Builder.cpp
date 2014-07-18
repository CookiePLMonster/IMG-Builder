#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <direct.h>
#if INC_ENCRYPTION
#include "BlowFish\Blowfish.h"
#endif

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                 ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 )) 

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <Shlwapi.h>

#define VC_IMG		0
#define SA_IMG		1
#define VCSPC_IMG	2

class CIMGHeader
{
private:
	DWORD	fileOffset;
	WORD	sizeFirstPriority;
	WORD	sizeSecondPriority;
	char	name[24];

public:
	void	WriteEntry(DWORD fOff, WORD sizeFPriority, const char* pName)
	{
		fileOffset = fOff;
		sizeFirstPriority = sizeFPriority;
		sizeSecondPriority = 0;
		strncpy(name, pName, 24);
	}

};

class cINIEntry
{
private:
	std::wstring	strFullPath;
	std::string		strFileName;
	size_t			nFileSize;

	static size_t	nBiggestFile;
	static void*	pMalloc;

public:
	cINIEntry(const WIN32_FIND_DATA& FindData)
	{
		// Get filename
		std::wstring strWideName = FindData.cFileName;
		strFileName = std::string(strWideName.cbegin(), strWideName.cend());
		
		// Get full path
		wchar_t			wcFullPath[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, wcFullPath);
		PathAppend(wcFullPath, FindData.cFileName);
		strFullPath = wcFullPath;

		// Get file size (up to 4GB)
		nFileSize = FindData.nFileSizeLow;

		if ( nFileSize > nBiggestFile )
			nBiggestFile = nFileSize;
	}

	void			WriteEntryToIMGFile(FILE* hFile, DWORD& headerLoopCtr, DWORD& pDataPos, CIMGHeader* header) const;

	static void		Init()
	{
		pMalloc = nullptr;
		nBiggestFile = 0;
	}

	static void		DeInit()
	{
		operator delete(pMalloc);
	}
};

size_t	cINIEntry::nBiggestFile;
void*	cINIEntry::pMalloc;

static unsigned char		bKeyVersion = 0;

void StrPathAppend(std::wstring& pszPath, const wchar_t* pszMore)
{
	if ( !pszPath.empty() )
	{
		if ( pszPath.back() != '\\' && pszPath.back() != '/' )
			pszPath.push_back('\\');
		pszPath += pszMore;
	}
	else
		pszPath = pszMore;
}


void FindFilesInDir(std::vector<cINIEntry>& vecSpace, const char* pDirName)
{
	WIN32_FIND_DATA	FileData;
	unsigned int	nFoundFiles = 0;

	HANDLE			hSearch = FindFirstFile(L"*", &FileData);

	if ( hSearch != INVALID_HANDLE_VALUE )
	{
		std::cout << "Parsing files from " << pDirName << " directory...\n";

		do
		{
			if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				++nFoundFiles;

				vecSpace.push_back(FileData);
			}
		}
		while ( FindNextFile(hSearch, &FileData) );

		FindClose(hSearch);
	}
	else
		throw "Can't access files in " + std::string(pDirName) + '!';

	std::cout << " Found " << nFoundFiles << " files\n\n";
}


void cINIEntry::WriteEntryToIMGFile(FILE* hFile, DWORD& headerLoopCtr, DWORD& pDataPos, CIMGHeader* header) const
{
	std::cout << strFileName.c_str() << '\n';
	
	if ( FILE* hFileToBePacked = _wfopen(strFullPath.c_str(), L"rb") )
	{
		WORD wSizeToWrite = static_cast<WORD>(nFileSize / 2048 + ( (nFileSize % 2048) != 0 ));
	
		header[headerLoopCtr].WriteEntry(pDataPos, wSizeToWrite, strFileName.c_str());
	
		if ( !pMalloc )
			pMalloc = operator new(nBiggestFile);
	
		fread(pMalloc, nFileSize, 1, hFileToBePacked);
		fwrite(pMalloc, nFileSize, 1, hFile);
		fclose(hFileToBePacked);
	
		++headerLoopCtr;
		fseek(hFile, wSizeToWrite * 2048 - nFileSize, SEEK_CUR);
		pDataPos += wSizeToWrite;
	}
	else
		throw "Error opening file " + strFileName + '!';
}

BYTE ParseINIFile(FILE* hFile, std::vector<cINIEntry>& vecSpace, const wchar_t* pOrgWorkingDir)
{
	char		cFileLine[MAX_PATH];
	bool		bIsVersionGet = false;
	BYTE		IMGVer = SA_IMG;

	while ( fgets(cFileLine, MAX_PATH, hFile) )
	{
		if ( cFileLine[0] == ';' || cFileLine[0] == '\n' )
			continue;

		if ( !bIsVersionGet )
		{
			bIsVersionGet = true;
			if ( !strncmp(cFileLine, "version", 7) )
			{
				const char* pStrtok = strtok(cFileLine, " \n =");
				pStrtok = strtok(NULL, " \n =");
#if INC_ENCRYPTION
				if ( strncmp(pStrtok, "ENCIMG", 6) )
				{
					if ( strncmp(pStrtok, "NEWIMG", 6) )
					{
						if ( strncmp(pStrtok, "OLDIMG", 6) )
						{
							std::cout << "Unknown IMG version specified, using v2 IMG...\n";
							IMGVer = SA_IMG;
						}
						else
						{
							std::cout << "Using v1 IMG...\n";
							IMGVer = VC_IMG;
						}
					}
					else
					{
						std::cout << "Using v2 IMG...\n";
						IMGVer = SA_IMG;
					}
				}
				else
				{
					std::cout << "Using v2 IMG with Encryption...\n";
					IMGVer = VCSPC_IMG;
				}
#else
				if ( strncmp(pStrtok, "NEWIMG", 6) )
				{
					if ( strncmp(pStrtok, "OLDIMG", 6) )
					{
						std::cout << "Unknown IMG version specified, using v2 IMG...\n";
						IMGVer = SA_IMG;
					}
					else
					{
						std::cout << "Using v1 IMG...\n";
						IMGVer = VC_IMG;
					}
				}
				else
				{
					std::cout << "Using v2 IMG...\n";
					IMGVer = SA_IMG;
				}
#endif
				continue;
			}
		}
		const char* pStrtok = strtok(cFileLine, " \n");

		SetCurrentDirectoryA(pStrtok);
		FindFilesInDir(vecSpace, pStrtok);
		SetCurrentDirectory(pOrgWorkingDir);
	}

	return IMGVer;
}

std::wstring MakeFullFilePath(const std::wstring& strIniPath, const std::wstring& strIniName, const wchar_t* pExtension)
{
	if ( !strIniPath.empty() )
		return strIniPath + L'\\' + strIniName.substr(0, strIniName.find_last_of('.')) + pExtension;

	return strIniName.substr(0, strIniName.find_last_of('.')) + pExtension;
}

std::wstring MakeIniPath(const std::wstring& strFullIniPath)
{
	auto	slashPos = strFullIniPath.find_last_of(L"/\\");
	if ( slashPos == std::wstring::npos )
		slashPos = 0;

	return strFullIniPath.substr(0, slashPos);
}

std::wstring MakeIniName(const std::wstring& strFullIniPath)
{
	auto slashPos = strFullIniPath.find_last_of(L"/\\");

	if ( slashPos != std::wstring::npos )
		slashPos += 1;
	else
		slashPos = 0;

	return strFullIniPath.substr(slashPos);
}

void CreateIMGFile(FILE* hFile, FILE* hHeaderFile, const std::vector<cINIEntry>& pVector, BYTE Version)
{
	std::cout << "Generating the IMG, it may take a few minutes...\n\n";

	DWORD			dwHeaderEntriesCounter = 0;
	DWORD			dwCurrentDataPos;
	DWORD			dwTotalFiles = pVector.size();

#if INC_ENCRYPTION
	unsigned char	encKey[24] = {	0x81, 0x45, 0x26, 0xFA, 0xDA, 0x7C, 0x6C, 0x11,
										0x86, 0x93, 0xCC, 0x90, 0x2B, 0xB7, 0xE2, 0x32,
										0x10, 0x0F, 0x56, 0x9B, 0x02, 0x8A, 0x6C, 0x5F };

	unsigned char	dlc2key[24] = {	124, 216, 71, 196, 191, 42, 230, 227, 164, 92,
										149, 92, 214, 126, 96, 45, 11, 97, 63, 217, 62, 171, 41, 221 };
	CBlowFish	blowFish(bKeyVersion == 1 ? dlc2key : encKey, 24);
#endif

	if ( Version != VC_IMG )
	{
		DWORD fileHeader[2];
		fileHeader[0] = MAKEFOURCC('V', 'E', 'R', '2');
		fileHeader[1] = dwTotalFiles;

#if INC_ENCRYPTION
		if ( Version == VCSPC_IMG )
			blowFish.Encrypt((unsigned char*)fileHeader, 8, CBlowFish::ECB);
#endif
//		fputs("VER2", hFile);
		fwrite(fileHeader, 8, 1, hFile);

		blowFish.ResetChain();
		dwCurrentDataPos = (8 + 32 * dwTotalFiles) / 2048 + ((8 + 32 * dwTotalFiles) % 2048 != 0);
		fseek(hFile, dwCurrentDataPos * 2048, SEEK_SET);
		hHeaderFile = hFile;
	}
	else
		dwCurrentDataPos = 0;

	CIMGHeader*		header = new CIMGHeader[dwTotalFiles];

	for ( auto it = pVector.cbegin(); it != pVector.cend(); it++ )
		it->WriteEntryToIMGFile(hFile, dwHeaderEntriesCounter, dwCurrentDataPos, header);

	fseek(hHeaderFile, ( Version != VC_IMG ) * 8, SEEK_SET);
#if INC_ENCRYPTION
	if ( Version == VCSPC_IMG )
		blowFish.Encrypt((unsigned char*)header, sizeof(CIMGHeader) * dwTotalFiles, CBlowFish::CBC);
#endif
	fwrite(header, sizeof(CIMGHeader), dwTotalFiles, hHeaderFile);
	delete[] header;

	fseek(hFile, 0, SEEK_END);
	DWORD	dwWhereAmI = ftell(hFile);
	if ( dwWhereAmI % 2048 )
	{
		BYTE buf = 0;
		fseek(hFile, 2047 - (dwWhereAmI % 2048), SEEK_CUR);
		fwrite(&buf, 1, 1, hFile);
	}

#if INC_ENCRYPTION
	if ( Version == VCSPC_IMG )
	{
		WORD	wBytesToWrite = 2048 - ( (sizeof(CIMGHeader) * dwTotalFiles) % 2048);
		BYTE*	garbageData = new BYTE[wBytesToWrite];
		memset(garbageData, 0xCC, wBytesToWrite);
		blowFish.ResetChain();
		blowFish.Encrypt(garbageData, wBytesToWrite, CBlowFish::CBC);

		fwrite(garbageData, wBytesToWrite, 1, hHeaderFile);

		delete[] garbageData;
	}
#endif
}

bool FileExists(const char* pFileName)
{
	if ( FILE* hFile = fopen(pFileName, "r") )
	{
		fclose(hFile);
		return true;
	}
	return false;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	UNREFERENCED_PARAMETER(envp);

	std::cout << "Native IMG Builder 1.4 by Silent\n\n";

	try
	{
		if ( argc < 2 )
		{
			throw "Specify the INI file!\n";
		}

		if ( FILE* hInputFile = _wfopen(argv[1], L"r") )
		{
			// INI path
			std::wstring				strIniPath = MakeIniPath(argv[1]);
		
			// INI name
			std::wstring				strIniName = MakeIniName(argv[1]);

			std::vector<cINIEntry>		INIEntries;
			std::wstring				strOutputFileName = MakeFullFilePath(argc > 2 ? argv[2] : strIniPath, strIniName, L".img");
			std::wstring				strHeaderFileName = MakeFullFilePath(argc > 2 ? argv[2] : strIniPath, strIniName, L".dir");

			cINIEntry::Init();

			if ( strOutputFileName.find(L"dlc2") != std::wstring::npos )
				bKeyVersion = 1;

			wchar_t			wcCurrentDir[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, wcCurrentDir);
			if ( !strIniPath.empty() )
				SetCurrentDirectory(strIniPath.c_str());
		
			BYTE IMGVersion = ParseINIFile(hInputFile, INIEntries, wcCurrentDir);
			fclose(hInputFile);


			std::cout << "Found " << INIEntries.size() << " files in total.\n";

			if ( FILE* hOutputFile = _wfopen(strOutputFileName.c_str(), L"wb") )
			{
				if ( IMGVersion == VC_IMG )
				{
					if ( FILE* hHeaderFile = _wfopen(strHeaderFileName.c_str(), L"wb") )
					{
						CreateIMGFile(hOutputFile, hHeaderFile, INIEntries, IMGVersion);
						cINIEntry::DeInit();
						fclose(hOutputFile);
						fclose(hHeaderFile);
					}
					else
						throw L"Error creating file " + strHeaderFileName + L'!';
				}
				else
				{
					CreateIMGFile(hOutputFile, nullptr, INIEntries, IMGVersion);
					cINIEntry::DeInit();
					fclose(hOutputFile);
				}

				std::cout << "Done!\n";
			}
			else
				throw L"Error creating file " + strOutputFileName + L'!';
		}
		else
			throw L"Error opening file " + std::wstring(argv[1]) + L'!';
	}
	catch ( std::string strException )
	{
		std::cerr << strException.c_str();
		return 2;
	}
	catch ( std::wstring strException )
	{
		std::wcerr << strException.c_str();
		return 2;
	}

	return 0;
}