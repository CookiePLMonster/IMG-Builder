#pragma once

#include <stdexcept>
#include <string>

class FileSizeException : public std::runtime_error
{
public:
	FileSizeException( std::wstring fileName )
		: std::runtime_error( "File size exceeds IMG format limits: " + std::string(fileName.cbegin(), fileName.cend()) )
	{
	}
};