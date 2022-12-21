#pragma once

#include <string>

std::string GenerateGUID();

constexpr float DegreeToRadian( float degree ) { return degree * M_PI / 180; }

constexpr float RadianToDegree( float radian ) { return radian * 180 / M_PI; }