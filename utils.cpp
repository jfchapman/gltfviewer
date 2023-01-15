#include "utils.h"

#include <vector>

#include <Windows.h>
#include <Shlwapi.h>

std::string GenerateGUID()
{
	std::string result;
	UUID uuid = {};
	UuidCreate( &uuid );
	RPC_CSTR uuidStr;
	if ( RPC_S_OK == UuidToStringA( &uuid, &uuidStr ) ) {
		result = reinterpret_cast<char*>( uuidStr );
		RpcStringFreeA( &uuidStr );
	}
	return result;
}

void UnescapeURL( std::wstring& url )
{
  UrlUnescapeInPlace( url.data(), URL_DONT_UNESCAPE_EXTRA_INFO );
}

std::wstring FromUTF8( const std::string& str )
{
	std::wstring result;
	if ( !str.empty() ) {
		if ( const int bufferSize = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/ ); bufferSize > 0 ) {
			std::vector<WCHAR> buffer( bufferSize );
			if ( 0 != MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1 /*strLen*/, buffer.data(), bufferSize ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::string ToUTF8( const std::wstring& str )
{
	std::string result;
	if ( !str.empty() ) {
		if ( const int bufferSize = WideCharToMultiByte( CP_UTF8, 0, str.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ ); bufferSize > 0 ) {
			std::vector<char> buffer( bufferSize );
			if ( 0 != WideCharToMultiByte( CP_UTF8, 0, str.c_str(), -1 /*strLen*/, buffer.data(), bufferSize, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::filesystem::path GetLibraryPath()
{
  std::vector<WCHAR> filename( MAX_PATH );
  if ( GetModuleFileName( nullptr, filename.data(), MAX_PATH ) ) {
    std::filesystem::path filepath( filename.data() );
    return filepath.parent_path();
  }
  return {};
}