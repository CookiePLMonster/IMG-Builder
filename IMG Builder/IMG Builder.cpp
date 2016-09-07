#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <direct.h>
#if defined INC_ENCRYPTION
#include "BlowFish\Blowfish.h"
#endif

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                 ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 )) 

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <Shlwapi.h>
#include <memory>

enum
{
	VC_IMG,
	SA_IMG,
	VCSPC_IMG
};

class CIMGHeader
{
private:
	static const size_t	IMG_NAME_LENGTH = 24;

	uint32_t	fileOffset;
	uint16_t	sizeFirstPriority;
	uint16_t	sizeSecondPriority;
	char		name[IMG_NAME_LENGTH];

public:
	void	WriteEntry(uint32_t fOff, uint16_t sizeFPriority, const char* pName)
	{
		fileOffset = fOff;
		sizeFirstPriority = sizeFPriority;
		sizeSecondPriority = 0;
		strncpy( name, pName, IMG_NAME_LENGTH-1 );
		name[IMG_NAME_LENGTH-1] = '\0';
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

	void			WriteEntryToIMGFile(FILE* hFile, size_t headerLoopCtr, size_t& pDataPos, CIMGHeader* header) const;

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


void FindFilesInDir(std::vector<cINIEntry>& vecSpace, const wchar_t* pDirName)
{
	WIN32_FIND_DATA	FileData;
	unsigned int	nFoundFiles = 0;

	HANDLE			hSearch = FindFirstFile(L"*", &FileData);

	if ( hSearch != INVALID_HANDLE_VALUE )
	{
		std::wcout << L"Parsing files from " << pDirName << L" directory...\n";

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
		throw L"Can't access files in " + std::wstring(pDirName) + L'!';

	std::cout << " Found " << nFoundFiles << " files\n\n";
}


void cINIEntry::WriteEntryToIMGFile(FILE* hFile, size_t headerLoopCtr, size_t& pDataPos, CIMGHeader* header) const
{
	std::cout << strFileName.c_str() << '\n';
	
	if ( FILE* hFileToBePacked = _wfopen(strFullPath.c_str(), L"rb") )
	{
		uint16_t wSizeToWrite = static_cast<uint16_t>( ((nFileSize + 2048 - 1) & ~(2048 - 1)) / 2048 );
	
		header[headerLoopCtr].WriteEntry(static_cast<uint32_t>(pDataPos), wSizeToWrite, strFileName.c_str());
	
		if ( pMalloc == nullptr )
			pMalloc = operator new(nBiggestFile);
	
		fread(pMalloc, nFileSize, 1, hFileToBePacked);
		fwrite(pMalloc, nFileSize, 1, hFile);
		fclose(hFileToBePacked);
	
		++headerLoopCtr;
		_fseeki64(hFile, wSizeToWrite * 2048 - nFileSize, SEEK_CUR);
		pDataPos += wSizeToWrite;
	}
	else
		throw "Error opening file " + strFileName + '!';
}

uint8_t ParseINIFile( std::wstring& iniName, std::vector<cINIEntry>& vecSpace )
{
	const size_t SCRATCH_PAD_SIZE = 32767;

	std::wstring strFileName = iniName;
	std::unique_ptr< wchar_t[] > scratch( new wchar_t[ SCRATCH_PAD_SIZE ] );
	uint8_t		IMGVer = SA_IMG;

	// Add .\ if path is relative
	if ( PathIsRelative( strFileName.c_str() ) != FALSE )
	{
		strFileName.insert( 0, L".\\" );
	}

	wchar_t			wcCurrentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, wcCurrentDir);

	// Get attributes
	GetPrivateProfileString( L"Attribs", L"version", nullptr, scratch.get(), SCRATCH_PAD_SIZE, strFileName.c_str() );
	if ( scratch[0] != '\0' )
	{
		std::wstring strVersion = scratch.get();
		for ( auto& ch : strVersion )
			ch = towlower( ch );

		if ( strVersion == L"oldimg" )
		{
			std::cout << "Using v1 IMG...\n";
			IMGVer = VC_IMG;
		}
		else if ( strVersion == L"newimg" )
		{
			std::cout << "Using v2 IMG...\n";
			IMGVer = SA_IMG;
		}
#if defined INC_ENCRYPTION
		else if ( strVersion == L"encimg" )
		{
			std::cout << "Using v2 IMG with Encryption...\n";
			IMGVer = VCSPC_IMG;
		}
#endif
	}

	// Read all files list
	GetPrivateProfileSection( L"Dirs", scratch.get(), SCRATCH_PAD_SIZE, strFileName.c_str() );

	for( wchar_t* raw = scratch.get(); *raw != '\0'; ++raw )
	{
		// Construct a std::wstring with the line
		std::wstring strTempFile;
		do
		{
			strTempFile.push_back( *raw );
			raw++;
		}
		while ( *raw != '\0' );

		SetCurrentDirectory( strTempFile.c_str() );
		FindFilesInDir(vecSpace, strTempFile.c_str() );
		SetCurrentDirectory( wcCurrentDir );
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

void CreateIMGFile(FILE* hFile, FILE* hHeaderFile, const std::vector<cINIEntry>& pVector, uint8_t Version)
{
	std::cout << "Generating the IMG, it may take a few minutes...\n\n";

	size_t			dwHeaderEntriesCounter = 0;
	size_t			dwCurrentDataPos;
	size_t			dwTotalFiles = pVector.size();

#if defined INC_ENCRYPTION
	unsigned char	encKey[24] = {	0x81, 0x45, 0x26, 0xFA, 0xDA, 0x7C, 0x6C, 0x11,
										0x86, 0x93, 0xCC, 0x90, 0x2B, 0xB7, 0xE2, 0x32,
										0x10, 0x0F, 0x56, 0x9B, 0x02, 0x8A, 0x6C, 0x5F };

	unsigned char	dlc2key[24] = {	124, 216, 71, 196, 191, 42, 230, 227, 164, 92,
										149, 92, 214, 126, 96, 45, 11, 97, 63, 217, 62, 171, 41, 221 };
	CBlowFish	blowFish(bKeyVersion == 1 ? dlc2key : encKey, 24);
#endif

	if ( Version != VC_IMG )
	{
		uint32_t fileHeader[2];
		fileHeader[0] = MAKEFOURCC('V', 'E', 'R', '2');
		fileHeader[1] = static_cast<uint32_t>(dwTotalFiles);

#if defined INC_ENCRYPTION
		if ( Version == VCSPC_IMG )
			blowFish.Encrypt((unsigned char*)fileHeader, 8, CBlowFish::ECB);
#endif
		fwrite(fileHeader, 8, 1, hFile);

#if defined INC_ENCRYPTION
		blowFish.ResetChain();
#endif
		dwCurrentDataPos = (8 + 32 * dwTotalFiles) / 2048 + ((8 + 32 * dwTotalFiles) % 2048 != 0);
		_fseeki64(hFile, dwCurrentDataPos * 2048, SEEK_SET);
		hHeaderFile = hFile;
	}
	else
		dwCurrentDataPos = 0;

	CIMGHeader*		header = new CIMGHeader[dwTotalFiles];

	for ( const auto& it : pVector )
		it.WriteEntryToIMGFile(hFile, dwHeaderEntriesCounter++, dwCurrentDataPos, header);

	_fseeki64(hHeaderFile, Version != VC_IMG ? 8 : 0, SEEK_SET);
#if defined INC_ENCRYPTION
	if ( Version == VCSPC_IMG )
		blowFish.Encrypt((unsigned char*)header, sizeof(CIMGHeader) * dwTotalFiles, CBlowFish::CBC);
#endif
	fwrite(header, sizeof(CIMGHeader), dwTotalFiles, hHeaderFile);
	delete[] header;

	_fseeki64(hFile, 0, SEEK_END);
	uint32_t	dwWhereAmI = ftell(hFile);
	if ( dwWhereAmI % 2048 )
	{
		uint8_t buf = 0;
		_fseeki64(hFile, 2047 - (dwWhereAmI % 2048), SEEK_CUR);
		fwrite(&buf, 1, 1, hFile);
	}

#if defined INC_ENCRYPTION
	if ( Version == VCSPC_IMG )
	{
		size_t	wBytesToWrite = 2048 - ( (sizeof(CIMGHeader) * dwTotalFiles) % 2048);
		uint8_t*	garbageData = new uint8_t[wBytesToWrite];
		memset(garbageData, 0xCC, wBytesToWrite);
		blowFish.ResetChain();
		blowFish.Encrypt(garbageData, wBytesToWrite, CBlowFish::CBC);

		fwrite(garbageData, wBytesToWrite, 1, hHeaderFile);

		delete[] garbageData;
	}
#endif
}

int wmain( int argc, wchar_t *argv[] )
{
	std::ios_base::sync_with_stdio(false);
	std::cout << "Native IMG Builder 1.5 by Silent\n\n";

	if ( argc < 2 || std::wstring( argv[1] ) == L"--help" )
	{
		std::cout << "Usage:\timgbuilder.exe path\\to\\ini.ini [path\\to\\output\\directory]\n\timgbuilder.exe --help - displays this help message\n\nPlease refer to doc\\example.ini for an example of input INI file\n";
		return 0;
	}

	try
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

		if ( !strIniPath.empty() )
			SetCurrentDirectory(strIniPath.c_str());
		
		uint8_t IMGVersion = ParseINIFile( strIniName, INIEntries );

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
	catch ( std::string& strException )
	{
		std::cerr << strException;
		return 2;
	}
	catch ( std::wstring& strException )
	{
		std::wcerr << strException;
		return 2;
	}

	return 0;
}