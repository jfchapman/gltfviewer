#pragma once

#include <filesystem>
#include <string>

constexpr float DegreeToRadian( float degree ) { return degree * M_PI / 180; }
constexpr float RadianToDegree( float radian ) { return radian * 180 / M_PI; }

// Platform specific utility functions
std::string GenerateGUID();
void UnescapeURL( std::wstring& url );
std::wstring FromUTF8( const std::string& str );
std::string ToUTF8( const std::wstring& str );
std::filesystem::path GetLibraryPath();