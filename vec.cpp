#include "vec.h"

namespace gltfviewer {

Vec3 Vec3::operator+( const Vec3& vec ) const
{
  return { x + vec.x, y + vec.y, z + vec.z };
}

Vec3 Vec3::operator-( const Vec3& vec ) const
{
  return { x - vec.x, y - vec.y, z - vec.z };
}

Vec3 Vec3::operator-() const
{
  return { -x, -y, -z };
}

void Vec3::Normalise()
{
  if ( const float l = Length(); l > 0 ) {
    x /= l;
    y /= l;
    z /= l;
  }
}

float Vec3::Length() const
{
  return sqrtf( x * x + y * y + z * z );
}

Vec3 Vec3::CrossProduct( const Vec3& vec ) const
{
  Vec3 result;
  auto& [ _x, _y, _z ] = result;
  _x = y * vec.z - z * vec.y;
  _y = z * vec.x - x * vec.z;
  _z = x * vec.y - y * vec.x;
  return result;
}

float Vec3::DotProduct( const Vec3& vec ) const
{
  return x * vec.x + y * vec.y + z * vec.z;
}

} // namespace gltfviewer
