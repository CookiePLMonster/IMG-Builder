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

/*DWORD CountTotalAmountOfFiles(vector<INIEntry>& pVector)
{
	WORD	wLoopCounter = 0;
	DWORD	dwTotalFiles = 0;
	DWORD	dwVectorSize = pVector.size();
	do
	{
		dwTotalFiles += pVector.at(wLoopCounter).GetNumberOfFiles();
		++wLoopCounter;
	}
	while ( wLoopCounter < dwVectorSize );

	return dwTotalFiles;
}*/

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

//bool cINIEntry::CountFilesInDir()

	/*WIN32_FIND_DATA	FileData;
	char			cString[MAX_PATH];

	wFilesInDir = 0;
	sprintf(cString, "%s\\*", cFileLine);

	HANDLE hSearch = FindFirstFile (cString, &FileData);
	if ( hSearch != INVALID_HANDLE_VALUE )
	{
		cout << "Counting files in " << cFileLine << " directory...\n";
		do
		{
			if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				++wFilesInDir;
		}
		while ( FindNextFile(hSearch, &FileData) );

		FindClose(hSearch);
	}
	else
	{
		cout << "Something has gone wrong while trying to access files in " << cFileLine << " dir!\n";
		return false;
	}

	dwTotalFiles += wFilesInDir;
//	wFilesInDir -= 2;

	cout << " Found " << wFilesInDir << " files\n\n";*/
//	return true;


bool FindFilesInDir(std::vector<cINIEntry>& vecSpace, const char* pDirName)
{
	WIN32_FIND_DATA	FileData;
	//wchar_t			wcString[MAX_PATH];
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
	{
		std::cout << "Something has gone wrong while trying to access files in " << pDirName << " directory!\n";
		return false;
	}

//	wFilesInDir -= 2;

	std::cout << " Found " << nFoundFiles << " files\n\n";
	return true;

	/*WIN32_FIND_DATA	FileData;
	char			cString[MAX_PATH];

	wFilesInDir = 0;
	sprintf(cString, "%s\\*", cFileLine);

	HANDLE hSearch = FindFirstFile (cString, &FileData);
	if ( hSearch != INVALID_HANDLE_VALUE )
	{
		cout << "Counting files in " << cFileLine << " directory...\n";
		do
		{
			if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				++wFilesInDir;
		}
		while ( FindNextFile(hSearch, &FileData) );

		FindClose(hSearch);
	}
	else
	{
		cout << "Something has gone wrong while trying to access files in " << cFileLine << " dir!\n";
		return false;
	}

	dwTotalFiles += wFilesInDir;
//	wFilesInDir -= 2;

	cout << " Found " << wFilesInDir << " files\n\n";*/

}


void cINIEntry::WriteEntryToIMGFile(FILE* hFile, DWORD& headerLoopCtr, DWORD& pDataPos, CIMGHeader* header) const
{
	//WIN32_FIND_DATA	FileData;
	//char			cString[MAX_PATH];

	//sprintf(cString, "%s\\*", cFileLine);

	//HANDLE hSearch = FindFirstFile (cString, &FileData);
	//if ( hSearch != INVALID_HANDLE_VALUE )
	{
//		FindNextFile(hSearch, &FileData);
//		FindNextFile(hSearch, &FileData);

		//do
		{
			//if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				std::cout << strFileName.c_str() << std::endl;
				//sprintf(cString, "%s\\%s", cFileLine, FileData.cFileName);
				if ( FILE* hFileToBePacked = _wfopen(strFullPath.c_str(), L"rb") )
				{
					//strncpy(cString, FileData.cFileName, 24);
	//				fseek(hFileToBePacked, 0, SEEK_END);
	//				DWORD dwFileSize = ftell(hFileToBePacked);
					//DWORD dwFileSize = FileData.nFileSizeLow;
					WORD wSizeToWrite = static_cast<WORD>(nFileSize / 2048 + ( (nFileSize % 2048) != 0 ));

					header[headerLoopCtr].WriteEntry(pDataPos, wSizeToWrite, strFileName.c_str());
	//				fseek(hFileToBePacked, 0, SEEK_SET);

					if ( !pMalloc )
						pMalloc = operator new(nBiggestFile);
					//BYTE* buffer = new BYTE[dwFileSize];
					fread(pMalloc, nFileSize, 1, hFileToBePacked);
					fwrite(pMalloc, nFileSize, 1, hFile);
					fclose(hFileToBePacked);
					//delete[] buffer;

					++headerLoopCtr;
					fseek(hFile, wSizeToWrite * 2048 - nFileSize, SEEK_CUR);
					pDataPos += wSizeToWrite;
				}
				else
					std::cout << "Error opening file " << strFileName.c_str() << "!\n";
			}
		}
		//while ( FindNextFile(hSearch, &FileData) );

		//FindClose(hSearch);
	}
}

BYTE ParseINIFile(FILE* hFile, std::vector<cINIEntry>& vecSpace, const wchar_t* pOrgWorkingDir, bool& bEverythingFine)
{
	char		cFileLine[MAX_PATH];
	bool		bIsVersionGet = false;
	BYTE		IMGVer = SA_IMG;

	while ( fgets(cFileLine, MAX_PATH, hFile) && bEverythingFine == true )
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
			//else
				//IMGVer = SA_IMG;
		}
		const char* pStrtok = strtok(cFileLine, " \n");
		//cINIEntry entry;

		//entry.PutStringInClass(pStrtok);
		//bEverythingFine = entry.CountFilesInDir();
		//pVectorSpace.push_back(entry);

		SetCurrentDirectoryA(pStrtok);
		bEverythingFine = FindFilesInDir(vecSpace, pStrtok);
		SetCurrentDirectory(pOrgWorkingDir);
	}

	return IMGVer;
}

/*std::wstring MakeFileName(const wchar_t* pInputName, const wchar_t* pOutPath, const wchar_t* pExtension)
{
	std::wstring	strOutPath, strFileName, strInputName(pInputName);
	auto			slashPos = strInputName.find_last_of(L"/\\");
	auto			dotPos = strInputName.find_last_of('.');

	if ( slashPos == std::string::npos )
		slashPos = 0;
	else
		++slashPos;

	// Get path
	if ( pOutPath )
		strOutPath = pOutPath;
	else
		strOutPath = strInputName.substr(0, slashPos);

	// Get filename
	strFileName = strInputName.substr(slashPos, dotPos);

	StrPathAppend(strOutPath, strFileName.c_str());
	strFileName += pExtension;

	return strFileName;
}*/

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

	/*DWORD			dwLoopCounter = 0;
	DWORD			dwVectorSize = pVector.size();

	do
	{
		pVector.at(dwLoopCounter).WriteEntryToIMGFile(hFile, dwHeaderEntriesCounter, dwCurrentDataPos, header, Version);
		++dwLoopCounter;
	}
	while ( dwLoopCounter < dwVectorSize );*/

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
	if ( argc < 2 )
	{
		std::cout << "Specify the INI file!\n";
		return -1;
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
		//wchar_t					cOutputFileName[MAX_PATH];
		//wchar_t					cHeaderFileName[MAX_PATH];
		bool						bEverythingFine = true;

		//MakeOutputFileName(argv[1], argc > 2 ? argv[2] : nullptr, cOutputFileName, L"img");
		//MakeOutputFileName(argv[1], argc > 2 ? argv[2] : nullptr, cHeaderFileName, L"dir");

		cINIEntry::Init();

		//if ( wcsstr(cOutputFileName, L"dlc2") )
		if ( strOutputFileName.find(L"dlc2") != std::wstring::npos )
			bKeyVersion = 1;



		/*char	cPath[MAX_PATH];
		strncpy(cPath, argv[0], MAX_PATH);
		if ( char* pch = strrchr(cPath, '\\') )
		{
			*pch = '\0';
			SetCurrentDirectory(cPath);
		}*/

		wchar_t			wcCurrentDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, wcCurrentDir);
		if ( !strIniPath.empty() )
			SetCurrentDirectory(strIniPath.c_str());
		
		BYTE IMGVersion = ParseINIFile(hInputFile, INIEntries, wcCurrentDir, bEverythingFine);
		fclose(hInputFile);

		//SetCurrentDirectory(wcCurrentDir);

		if ( bEverythingFine )
		{
//			DWORD dwTotalFiles = CountTotalAmountOfFiles(INIEntries);

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
						std::cout << "Done!\n";
					}
					else
						std::cout << "Error creating file " << strHeaderFileName.c_str() << "!\n";
				}
				else
				{
					CreateIMGFile(hOutputFile, nullptr, INIEntries, IMGVersion);
					cINIEntry::DeInit();
					fclose(hOutputFile);
					std::cout << "Done!\n";
				}
			}
			else
				std::cout << "Error creating file " << strOutputFileName.c_str() << "!\n";
		}
		else
			std::cout << "Error while reading INI File, build cancelled...\n";
	}
	else
		std::cout << "Error opening file " << argv[1] << "!\n";

	return 0;
}