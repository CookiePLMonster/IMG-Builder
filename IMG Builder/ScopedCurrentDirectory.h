#pragma once

#ifndef _WINDOWS_

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif

#include <stdexcept>
#include <string>

class ScopedCurrentDirectoryA
{
public:
	ScopedCurrentDirectoryA()
	{
		GetCurrentDirectoryA( _countof(m_currentDir), m_currentDir );
	}

	ScopedCurrentDirectoryA( LPCSTR newDirectory )
		: ScopedCurrentDirectoryA()
	{
		Set( newDirectory );
	}

	~ScopedCurrentDirectoryA()
	{
		SetCurrentDirectoryA( m_currentDir );
	}

	void Set( LPCSTR newDirectory ) const
	{
		if ( SetCurrentDirectoryA( newDirectory ) == 0 )
		{
			throw std::runtime_error( std::string(newDirectory) + " does not exist! Aborting.");
		}
	}

private:
	CHAR m_currentDir[MAX_PATH];
};

class ScopedCurrentDirectoryW
{
public:
	ScopedCurrentDirectoryW()
	{
		GetCurrentDirectoryW( _countof(m_currentDir), m_currentDir );
	}

	ScopedCurrentDirectoryW( LPCWSTR newDirectory )
		: ScopedCurrentDirectoryW()
	{
		Set( newDirectory );
	}

	~ScopedCurrentDirectoryW()
	{
		SetCurrentDirectoryW( m_currentDir );
	}

	void Set( LPCWSTR newDirectory ) const
	{
		if ( SetCurrentDirectoryW( newDirectory ) == 0 )
		{
			const std::wstring tmp(newDirectory);
			throw std::runtime_error( std::string(tmp.begin(), tmp.end()) + " does not exist! Aborting.");
		}
	}

private:
	WCHAR m_currentDir[MAX_PATH];
};

#ifdef UNICODE
#define ScopedCurrentDirectory ScopedCurrentDirectoryW
#else
#define ScopedCurrentDirectory ScopedCurrentDirectoryA
#endif