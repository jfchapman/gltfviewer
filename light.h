#pragma once

#include <GLTFSDK/GLTF.h>

#include <string>

#include "matrix.h"

namespace gltfviewer {

class Light
{
public:
  enum class Type { Point, Spot, Directional };

  Type m_type = Type::Point;
  std::string m_name;

  Matrix m_transform;

  // Linear color space.
  Microsoft::glTF::Color3 m_color = { 1.0f, 1.0f, 1.0f };
  
  // Candela for point & spot lights, lux for directional lights.
  float m_intensity = 1.0f;

  // Cone angles in radians (for spot lights).
  float m_innerConeAngle = 0;
  float m_outerConeAngle = M_PI_4;

  bool operator==( const Light& rhs ) const {
    return std::tie( m_type, m_name, m_transform, m_color, m_intensity, m_innerConeAngle, m_outerConeAngle ) == std::tie( rhs.m_type, rhs.m_name, rhs.m_transform, rhs.m_color, rhs.m_intensity, rhs.m_innerConeAngle, rhs.m_outerConeAngle );
  };
};

} // namespace gltfviewer
