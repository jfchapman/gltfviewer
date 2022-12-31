#include "matrix.h"

namespace gltfviewer {

Matrix::Matrix()
{
  m_matrix[ 0 ][ 0 ] = 1.f;
  m_matrix[ 1 ][ 1 ] = 1.f;
  m_matrix[ 2 ][ 2 ] = 1.f;
  m_matrix[ 3 ][ 3 ] = 1.f;
}

Matrix::Matrix( const Microsoft::glTF::Matrix4& matrix )
{
  // Note the input matrix is in column major order
  size_t k = 0;
  for ( size_t i = 0; i < 4; i++ ) {
    for ( size_t j = 0; j < 4; j++, k++ ) {
      m_matrix[ j ][ i ] = matrix.values[ k ];
    }
  }
}

Matrix::Matrix( const Microsoft::glTF::Vector3& translation, const Microsoft::glTF::Quaternion& rotation, const Microsoft::glTF::Vector3& scale )
{
  Matrix t;
  t.m_matrix[ 0 ][ 3 ] = translation.x;
  t.m_matrix[ 1 ][ 3 ] = translation.y;
  t.m_matrix[ 2 ][ 3 ] = translation.z;

  const Matrix r( rotation );

  Matrix s;
  s.m_matrix[ 0 ][ 0 ] = scale.x;
  s.m_matrix[ 1 ][ 1 ] = scale.y;
  s.m_matrix[ 2 ][ 2 ] = scale.z;

  *this = t * r * s;
}

Matrix::Matrix( const Microsoft::glTF::Quaternion& _q ) : Matrix()
{
  float n = sqrtf( _q.x * _q.x + _q.y * _q.y + _q.z * _q.z + _q.w * _q.w );
  if ( 0 == n ) {
    n = 1.f;
  }
  const Microsoft::glTF::Quaternion q( _q.x / n, _q.y / n, _q.z / n, _q.w / n );

  // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
  m_matrix[ 0 ][ 0 ] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
  m_matrix[ 0 ][ 1 ] = 2 * q.x * q.y - 2 * q.z * q.w;
  m_matrix[ 0 ][ 2 ] = 2 * q.x * q.z + 2 * q.y * q.w;

  m_matrix[ 1 ][ 0 ] = 2 * q.x * q.y + 2 * q.z * q.w;
  m_matrix[ 1 ][ 1 ] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
  m_matrix[ 1 ][ 2 ] = 2 * q.y * q.z - 2 * q.x * q.w;

  m_matrix[ 2 ][ 0 ] = 2 * q.x * q.z - 2 * q.y * q.w;
  m_matrix[ 2 ][ 1 ] = 2 * q.y * q.z + 2 * q.x * q.w;
  m_matrix[ 2 ][ 2 ] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
}

Matrix::Matrix( const Matrix& matrix )
{
  m_matrix = matrix.m_matrix;
}

Matrix::Matrix( const float matrix[ 4 ][ 4 ] )
{
  for ( size_t i = 0; i < 4; i++ ) {
    for ( size_t j = 0; j < 4; j++ ) {
      m_matrix[ i ][ j ] = matrix[ i ][ j ];
    }
  }
}

Matrix Matrix::operator*( const Matrix& other ) const
{
  Matrix result;
  for ( size_t i = 0; i < 4; i++ ) {
    for ( size_t j = 0; j < 4; j++ ) {
      result.m_matrix[ i ][ j ] = 0;
      for ( size_t k = 0; k < 4; k++ ) {
        result.m_matrix[ i ][ j ] += m_matrix[ i ][ k ] * other.m_matrix[ k ][ j ];
      }
    }
  }
  return result;
}

Matrix& Matrix::operator*=( Matrix const& other )
{
  const Matrix result = *this * other;
  m_matrix = result.m_matrix;
  return *this;
}

Vec3 Matrix::Transform( const Microsoft::glTF::Vector3& value ) const
{
  Vec3 result;
  result.x = m_matrix[ 0 ][ 0 ] * value.x + m_matrix[ 0 ][ 1 ] * value.y + m_matrix[ 0 ][ 2 ] * value.z + m_matrix[ 0 ][ 3 ];
  result.y = m_matrix[ 1 ][ 0 ] * value.x + m_matrix[ 1 ][ 1 ] * value.y + m_matrix[ 1 ][ 2 ] * value.z + m_matrix[ 1 ][ 3 ];
  result.z = m_matrix[ 2 ][ 0 ] * value.x + m_matrix[ 2 ][ 1 ] * value.y + m_matrix[ 2 ][ 2 ] * value.z + m_matrix[ 2 ][ 3 ];
  return result;
}

Vec3 Matrix::Transform( const Vec4& value ) const
{
  Vec3 result;
  result.x = m_matrix[ 0 ][ 0 ] * value.x + m_matrix[ 0 ][ 1 ] * value.y + m_matrix[ 0 ][ 2 ] * value.z + m_matrix[ 0 ][ 3 ] * value.w;
  result.y = m_matrix[ 1 ][ 0 ] * value.x + m_matrix[ 1 ][ 1 ] * value.y + m_matrix[ 1 ][ 2 ] * value.z + m_matrix[ 1 ][ 3 ] * value.w;
  result.z = m_matrix[ 2 ][ 0 ] * value.x + m_matrix[ 2 ][ 1 ] * value.y + m_matrix[ 2 ][ 2 ] * value.z + m_matrix[ 2 ][ 3 ] * value.w;
  return result;
}

void Matrix::TransformInPlace( Microsoft::glTF::Vector3& value ) const
{
  value = Transform( value );
}

Matrix Matrix::Translation( const Vec3& translation )
{
  Matrix m;
  m.m_matrix[ 0 ][ 3 ] = translation.x;
  m.m_matrix[ 1 ][ 3 ] = translation.y;
  m.m_matrix[ 2 ][ 3 ] = translation.z;
  return m;
}

Matrix Matrix::Scaling( const Vec3& scale )
{
  Matrix m;
  m.m_matrix[ 0 ][ 0 ] = scale.x;
  m.m_matrix[ 1 ][ 1 ] = scale.y;
  m.m_matrix[ 2 ][ 2 ] = scale.z;
  return m;
}


} // namespace gltfviewer
