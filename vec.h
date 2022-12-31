#pragma once

#include <GLTFSDK/Math.h>

namespace gltfviewer {

using Vec2 = Microsoft::glTF::Vector2;
using Vec4 = Microsoft::glTF::Quaternion;

struct Vec3 : public Microsoft::glTF::Vector3
{
  using Microsoft::glTF::Vector3::Vector3;

  Vec3 operator+( const Vec3& vec ) const;
  Vec3 operator-( const Vec3& vec ) const;

  Vec3 operator-() const;

  void Normalise();
  float Length() const;
  Vec3 CrossProduct( const Vec3& vec ) const;
  float DotProduct( const Vec3& vec ) const;
};

} // namespace gltfviewer