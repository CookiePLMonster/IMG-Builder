#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <direct.h>
#if INC_ENCRYPTION
#include "BlowFish\Blowfish.h"
#endif

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                 ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 )) 

#include <iostream>
#undef UNICODE
#include <windows.h>
#include <vector>
using namespace std;

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
	CIMGHeader()
	{
	}

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
	char			cFileLine[MAX_PATH];
	WORD			wFilesInDir;

public:
	static DWORD	dwTotalFiles;

	void			PutStringInClass(const char* pLine)
						{ strncpy(cFileLine, pLine, MAX_PATH); };
	WORD			GetNumberOfFiles()
						{ return wFilesInDir; };
	bool			CountFilesInDir();
	void			WriteEntryToIMGFile(FILE* hFile, DWORD& headerLoopCtr, DWORD& pDataPos, CIMGHeader* header, BYTE IMGversion);
};

DWORD cINIEntry::dwTotalFiles = 0;

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

bool cINIEntry::CountFilesInDir()
{
	WIN32_FIND_DATA	FileData;
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

	cout << " Found " << wFilesInDir << " files\n\n";
	return true;
}


void cINIEntry::WriteEntryToIMGFile(FILE* hFile, DWORD& headerLoopCtr, DWORD& pDataPos, CIMGHeader* header, BYTE IMGversion)
{
	WIN32_FIND_DATA	FileData;
	char			cString[MAX_PATH];

	sprintf(cString, "%s\\*", cFileLine);

	HANDLE hSearch = FindFirstFile (cString, &FileData);
	if ( hSearch != INVALID_HANDLE_VALUE )
	{
//		FindNextFile(hSearch, &FileData);
//		FindNextFile(hSearch, &FileData);

		do
		{
			if ( !(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				cout << FileData.cFileName << endl;
				sprintf(cString, "%s\\%s", cFileLine, FileData.cFileName);
				if ( FILE* hFileToBePacked = fopen(cString, "rb") )
				{
					strncpy(cString, FileData.cFileName, 24);
	//				fseek(hFileToBePacked, 0, SEEK_END);
	//				DWORD dwFileSize = ftell(hFileToBePacked);
					DWORD dwFileSize = FileData.nFileSizeLow;
					WORD wSizeToWrite = dwFileSize / 2048 + ( (dwFileSize % 2048) != 0 );

					header[headerLoopCtr].WriteEntry(pDataPos, wSizeToWrite, cString);
	//				fseek(hFileToBePacked, 0, SEEK_SET);

					BYTE* buffer = new BYTE[dwFileSize];
					fread(buffer, dwFileSize, 1, hFileToBePacked);
					fwrite(buffer, dwFileSize, 1, hFile);
					delete[] buffer;

					++headerLoopCtr;
					fseek(hFile, wSizeToWrite * 2048 - dwFileSize, SEEK_CUR);
					pDataPos += wSizeToWrite;
					fclose(hFileToBePacked);
				}
				else
					cout << "Error opening file " << FileData.cFileName << "!\n";
			}
		}
		while ( FindNextFile(hSearch, &FileData) );

		FindClose(hSearch);
	}
}

BYTE ParseINIFile(FILE* hFile, vector<cINIEntry>& pVectorSpace, bool& bEverythingFine)
{
	char		cFileLine[MAX_PATH];
	bool		bIsVersionGet = false;
	BYTE		IMGVer;

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
							cout << "Unknown IMG version specified, using v2 IMG...\n";
							IMGVer = SA_IMG;
						}
						else
						{
							cout << "Using v1 IMG...\n";
							IMGVer = VC_IMG;
						}
					}
					else
					{
						cout << "Using v2 IMG...\n";
						IMGVer = SA_IMG;
					}
				}
				else
				{
					cout << "Using v2 IMG with Encryption...\n";
					IMGVer = VCSPC_IMG;
				}
#else
				if ( strncmp(pStrtok, "NEWIMG", 6) )
				{
					if ( strncmp(pStrtok, "OLDIMG", 6) )
					{
						cout << "Unknown IMG version specified, using v2 IMG...\n";
						IMGVer = SA_IMG;
					}
					else
					{
						cout << "Using v1 IMG...\n";
						IMGVer = VC_IMG;
					}
				}
				else
				{
					cout << "Using v2 IMG...\n";
					IMGVer = SA_IMG;
				}
#endif
				continue;
			}
			else
				IMGVer = SA_IMG;
		}
		const char* pStrtok = strtok(cFileLine, " \n");
		cINIEntry entry;

		entry.PutStringInClass(pStrtok);
		bEverythingFine = entry.CountFilesInDir();
		pVectorSpace.push_back(entry);
	}

	return IMGVer;
}

void MakeOutputFileName(const char* pInputName, char* pOutputName)
{
	strncpy(pOutputName, pInputName, MAX_PATH);
	char* pch = strrchr(pOutputName, '.');
	*(pch + 1) = '\0';
	sprintf(pOutputName, "%simg", pOutputName);
}

void MakeHeaderFileName(const char* pInputName, char* pOutputName)
{
	strncpy(pOutputName, pInputName, MAX_PATH);
	char* pch = strrchr(pOutputName, '.');
	*(pch + 1) = '\0';
	sprintf(pOutputName, "%sdir", pOutputName);
}

void CreateIMGFile(FILE* hFile, FILE* hHeaderFile, vector<cINIEntry>& pVector, DWORD dwTotalFiles, BYTE Version)
{
	cout << "Generating the IMG, it may take few minutes...\n\n";

	DWORD			dwHeaderEntriesCounter = 0;
	DWORD			dwCurrentDataPos;

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

	DWORD			dwLoopCounter = 0;
	DWORD			dwVectorSize = pVector.size();

	do
	{
		pVector.at(dwLoopCounter).WriteEntryToIMGFile(hFile, dwHeaderEntriesCounter, dwCurrentDataPos, header, Version);
		++dwLoopCounter;
	}
	while ( dwLoopCounter < dwVectorSize );

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

int main(int argc, const char* argv[])
{
	cout << "Native IMG Builder 1.32 by Silent\n\n";
	if ( argc != 2 )
	{
		cout << "Specify the INI file!\n";
		return 0;
	}

	if ( FILE* hInputFile = fopen(argv[1], "r") )
	{
		vector<cINIEntry>	INIEntries;
		char				cOutputFileName[MAX_PATH];
		char				cHeaderFileName[MAX_PATH];
		bool				bEverythingFine = true;

		MakeOutputFileName(argv[1], cOutputFileName);
		MakeHeaderFileName(argv[1], cHeaderFileName);

		if ( strstr(cOutputFileName, "dlc2") )
			bKeyVersion = 1;

		char	cPath[MAX_PATH];
		strncpy(cPath, argv[0], MAX_PATH);
		if ( char* pch = strrchr(cPath, '\\') )
		{
			*pch = '\0';
			SetCurrentDirectory(cPath);
		}
		BYTE IMGVersion = ParseINIFile(hInputFile, INIEntries, bEverythingFine);
		fclose(hInputFile);

		if ( bEverythingFine )
		{
//			DWORD dwTotalFiles = CountTotalAmountOfFiles(INIEntries);

			cout << "Found " << INIEntries.size() << " entries in the INI file, " << cINIEntry::dwTotalFiles << " files total\n";

			if ( FILE* hOutputFile = fopen(cOutputFileName, "wb") )
			{
				if ( IMGVersion == VC_IMG )
				{
					if ( FILE* hHeaderFile = fopen(cHeaderFileName, "wb") )
					{
						CreateIMGFile(hOutputFile, hHeaderFile, INIEntries, cINIEntry::dwTotalFiles, IMGVersion);
						fclose(hOutputFile);
						fclose(hHeaderFile);
						cout << "Done!\n";
					}
					else
						cout << "Error creating file " << cHeaderFileName << "!\n";
				}
				else
				{
					CreateIMGFile(hOutputFile, NULL, INIEntries, cINIEntry::dwTotalFiles, IMGVersion);
					fclose(hOutputFile);
					cout << "Done!\n";
				}
			}
			else
				cout << "Error creating file " << cOutputFileName << "!\n";
		}
		else
			cout << "Error while reading INI File, build cancelled...\n";
	}
	else
		cout << "Error opening file " << argv[1] << "!\n";

	cin.get();
	return 0;
}