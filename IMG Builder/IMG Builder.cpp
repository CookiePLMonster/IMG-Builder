#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <Shlwapi.h>
#include <strsafe.h>

#include "DelimStringReader.h"
#include "Exceptions.hpp"

#ifndef UNICODE
#error IMG Builder must be compiled with Unicode character set
#endif

static uint32_t MakeFourCC( char A, char B, char C, char D )
{
	return uint32_t( A | (B << 8) | (C << 16) | (D << 24) );
}

enum IMGVersion
{
	VC_IMG,
	SA_IMG
};

struct IMGHeaderEntry
{
	static const size_t	IMG_NAME_LENGTH = 24;

	typedef uint32_t offset_type;
	typedef uint16_t size_type;


	offset_type	fileOffset;
	size_type	streamedSize;
	size_type	compressedSize;
	char		name[IMG_NAME_LENGTH];

	IMGHeaderEntry( uint64_t offset, uint64_t size, const char* fname )
		: fileOffset( offset_type(offset) ), streamedSize( size_type(size) ), compressedSize(0)
	{
		StringCbCopyNExA( name, sizeof(name), fname, IMG_NAME_LENGTH, nullptr, nullptr, STRSAFE_FILL_BEHIND_NULL );
	}

};

static const uint64_t IMG_BLOCK_SIZE = 2048;
static const uint64_t MAX_IMG_FILE_SIZE = std::numeric_limits<IMGHeaderEntry::size_type>::max() * IMG_BLOCK_SIZE;
static const uint64_t MAX_IMG_FILE_OFFSET =  std::numeric_limits<IMGHeaderEntry::offset_type>::max() * IMG_BLOCK_SIZE;

struct INIEntry
{
	std::wstring	m_fullPath;
	std::string		m_fileName;
	uint64_t		m_fileSize;

	INIEntry( std::wstring fileName, std::wstring fullPath, uint64_t size )
		: m_fileName( fileName.cbegin(), fileName.cend() ), m_fullPath( std::move(fullPath) ), m_fileSize( size )
	{	
	}
};

class IMGBuild
{
public:
	IMGBuild()
		: m_biggestFile(0), m_version(SA_IMG)
	{
	}

	void ReadINI( std::wstring fileName )
	{
		const size_t SCRATCH_PAD_SIZE = 32767;
		WideDelimStringReader reader( SCRATCH_PAD_SIZE );

		// Add .\ if path is relative
		if ( PathIsRelative( fileName.c_str() ) != FALSE )
		{
			fileName.insert( 0, L".\\" );
		}

		// Get attributes
		{
			GetPrivateProfileString( L"Attribs", L"version", nullptr, reader.GetBuffer(), static_cast<DWORD>(reader.GetSize()), fileName.c_str() );

			wchar_t* buf = reader.GetBuffer();
			if ( buf[0] != '\0' )
			{
				if ( _wcsicmp( buf, L"oldimg" ) == 0 )
				{
					std::wcout << L"Using v1 IMG...\n";
					m_version = VC_IMG;
				}
				else if ( _wcsicmp( buf, L"newimg" ) == 0 )
				{
					std::wcout << L"Using v2 IMG...\n";
					m_version = SA_IMG;
				}
			}
		}

		// Read all files list
		{
			reader.Reset();
			GetPrivateProfileSection( L"Dirs", reader.GetBuffer(), static_cast<DWORD>(reader.GetSize()), fileName.c_str() );

			wchar_t			wcCurrentDir[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, wcCurrentDir);

			while ( const wchar_t* line = reader.GetString() )
			{
				SetCurrentDirectory( line );
				uint32_t foundFiles = FindFilesInDir( );
				SetCurrentDirectory( wcCurrentDir );

				std::wcout << foundFiles << L" files in " << line << L" listed\n";
			}
		}
	
	}

	void BuildIMG( const std::wstring& imgName, const std::wstring& dirName )
	{
		uint64_t numTotalBlocks = BuildHeaderData();
		uint8_t* buffer = new uint8_t[static_cast<size_t>(m_biggestFile)];

		HANDLE imgFile, dirFile;

		imgFile = CreateFile( imgName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS, nullptr );
		PreExtendFile( imgFile, numTotalBlocks );

		for ( size_t i = 0, j = m_headerData.size(); i < j; i++ )
		{
			DWORD bytesRead = 0;
			HANDLE fileToWrite = CreateFile( m_files[i].m_fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
			if ( ReadFile( fileToWrite, buffer, static_cast<DWORD>(m_files[i].m_fileSize), &bytesRead, nullptr ) == FALSE )
			{
				throw std::runtime_error( "Failed to read the file" );
			}

			ULARGE_INTEGER fileOffset;
			OVERLAPPED ov = { 0 };
			fileOffset.QuadPart = m_headerData[i].fileOffset * IMG_BLOCK_SIZE;
			ov.Offset = fileOffset.LowPart;
			ov.OffsetHigh = fileOffset.HighPart;
			if ( WriteFile( imgFile, buffer, static_cast<DWORD>(m_files[i].m_fileSize), nullptr, &ov ) == FALSE )
			{
				if ( GetLastError() != ERROR_IO_PENDING )
				{
					throw std::runtime_error( "Failed to write to IMG" );
				}
				std::wcout << m_headerData[i].name << '\n';
				CloseHandle(fileToWrite);

				DWORD bytesWritten = 0;
				GetOverlappedResult( imgFile, &ov, &bytesWritten, TRUE );
			}
			else
			{
				std::wcout << m_headerData[i].name << '\n';
				CloseHandle(fileToWrite);
			}
		}
		CloseHandle(imgFile);
		delete[] buffer;

		dirFile = CreateFile( m_version == SA_IMG ? imgName.c_str() : dirName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr );

		DWORD bytesWritten = 0;
		if ( m_version == SA_IMG )
		{
			uint32_t fileHeader[2];
			fileHeader[0] = MakeFourCC('V', 'E', 'R', '2');
			fileHeader[1] = static_cast<uint32_t>(m_headerData.size());
			WriteFile( dirFile, fileHeader, sizeof(fileHeader), &bytesWritten, nullptr );
		}
		WriteFile( dirFile, m_headerData.data(), static_cast<DWORD>(m_headerData.size() * sizeof(decltype(m_headerData)::value_type)), &bytesWritten, nullptr );

		CloseHandle(dirFile);
	}

	inline size_t GetNumFiles() const { return m_files.size(); }

private:
	void PreExtendFile( HANDLE handle, uint64_t sizeInBlocks )
	{
		LARGE_INTEGER offset;
		offset.QuadPart = sizeInBlocks * IMG_BLOCK_SIZE;
		SetFilePointerEx( handle, offset, nullptr, FILE_BEGIN );
		SetEndOfFile( handle );
	}

	uint64_t BuildHeaderData()
	{
		m_headerData.reserve( m_files.size() );

		uint64_t curBlockOffset;

		if ( m_version == SA_IMG )
		{
			uint64_t totalHeaderSize = 8 + m_files.size() * sizeof(IMGHeaderEntry);
			totalHeaderSize = (totalHeaderSize + IMG_BLOCK_SIZE - 1) & ~(IMG_BLOCK_SIZE - 1); // Round up to block size

			curBlockOffset = totalHeaderSize / IMG_BLOCK_SIZE;
		}
		else
		{
			curBlockOffset = 0;
		}
		
		for ( auto& it : m_files )
		{
			uint64_t fileSize = (it.m_fileSize + IMG_BLOCK_SIZE - 1) & ~(IMG_BLOCK_SIZE - 1); // Round up to block size
			uint64_t fileSizeBlocks = fileSize / IMG_BLOCK_SIZE;
			m_headerData.emplace_back( curBlockOffset, fileSizeBlocks, it.m_fileName.c_str() );

			curBlockOffset += fileSizeBlocks;
		}
		return curBlockOffset;
	}

	void AddFile( const WIN32_FIND_DATA& FindData )
	{
		wchar_t			wcFullPath[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, wcFullPath);
		PathAppend(wcFullPath, FindData.cFileName);

		ULARGE_INTEGER fileSize;
		fileSize.LowPart = FindData.nFileSizeLow;
		fileSize.HighPart = FindData.nFileSizeHigh;

		if ( fileSize.QuadPart > MAX_IMG_FILE_SIZE )
			throw FileSizeException( FindData.cFileName );

		m_files.emplace_back( FindData.cFileName, wcFullPath, fileSize.QuadPart );

		if ( fileSize.QuadPart > m_biggestFile )
			m_biggestFile = fileSize.QuadPart;
	}

	uint32_t FindFilesInDir( )
	{
		WIN32_FIND_DATA	FileData;
		unsigned int	foundFiles = 0;

		HANDLE			hSearch = FindFirstFile(L"*", &FileData);

		if ( hSearch != INVALID_HANDLE_VALUE )
		{
			do
			{
				if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				{
					foundFiles++;
					AddFile( FileData );
				}
			}
			while ( FindNextFile(hSearch, &FileData) );

			FindClose(hSearch);
		}
		return foundFiles;
	}

private:
	std::vector<INIEntry> m_files;
	std::vector<IMGHeaderEntry> m_headerData;

	IMGVersion m_version;
	uint64_t m_biggestFile;
};

std::wstring MakeFullFilePath(const std::wstring& strIniPath, const std::wstring& strIniName, const wchar_t* pExtension)
{
	if ( !strIniPath.empty() )
		return strIniPath + L'\\' + strIniName.substr(0, strIniName.find_last_of('.')) + pExtension;

	return strIniName.substr(0, strIniName.find_last_of('.')) + pExtension;
}

std::wstring MakeIniPath(const std::wstring& strFullIniPath)
{
	std::wstring::size_type	slashPos = strFullIniPath.find_last_of(L"/\\");
	if ( slashPos == std::wstring::npos )
		return L"";

	return strFullIniPath.substr(0, slashPos);
}

std::wstring MakeIniName(const std::wstring& strFullIniPath)
{
	std::wstring::size_type slashPos = strFullIniPath.find_last_of(L"/\\");
	if ( slashPos != std::wstring::npos )
		return strFullIniPath.substr(slashPos+1);
	
	return strFullIniPath;
}

int wmain( int argc, wchar_t *argv[] )
{
	std::ios_base::sync_with_stdio(false);
	std::cout << "Native IMG Builder 2.0 by Silent\n\n";

	if ( argc < 2 || std::wstring( argv[1] ) == L"--help" )
	{
		std::cout << "Usage:\timgbuilder.exe path\\to\\ini.ini [path\\to\\output\\directory]\n\timgbuilder.exe --help - displays this help message\n\nPlease refer to doc\\example.ini for an example of input INI file\n";
		return 0;
	}

	try
	{
		std::wstring				strIniPath = MakeIniPath(argv[1]);
		std::wstring				strIniName = MakeIniName(argv[1]);

		std::wstring				strOutputFileName = MakeFullFilePath(argc > 2 ? argv[2] : strIniPath, strIniName, L".img");
		std::wstring				strHeaderFileName = MakeFullFilePath(argc > 2 ? argv[2] : strIniPath, strIniName, L".dir");

		if ( !strIniPath.empty() )
			SetCurrentDirectory(strIniPath.c_str());
		
		IMGBuild img;

		img.ReadINI( strIniName );

		std::cout << "Found " << img.GetNumFiles() << " files in total.\n";

		img.BuildIMG( strOutputFileName, strHeaderFileName );
	}
	catch ( std::exception& e )
	{
		std::cerr << e.what();
		return -1;
	}

	return 0;
}