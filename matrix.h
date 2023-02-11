#pragma once

#include <GLTFSDK/Math.h>

#include "vec.h"

namespace gltfviewer {

class Matrix
{
public:
  Matrix();
  Matrix( const Microsoft::glTF::Matrix4& matrix );
  Matrix( const Microsoft::glTF::Vector3& translation, const Microsoft::glTF::Quaternion& rotation, const Microsoft::glTF::Vector3& scale );
  Matrix( const Microsoft::glTF::Quaternion& rotation );
  Matrix( const Matrix& matrix );
  Matrix( const float matrix[ 16 ] );

  Matrix operator*( const Matrix& other ) const;
  Matrix& operator*=( const Matrix& other );
  bool operator==( const Matrix& other ) const { return m_matrix == other.m_matrix; };

  const std::array<std::array<float, 4>, 4>& GetValues() const { return m_matrix; }
  std::array<std::array<float, 4>, 4>& GetValues() { return m_matrix; }

  Vec3 Transform( const Microsoft::glTF::Vector3& value ) const;
  Vec3 Transform( const Vec4& value ) const;
  void TransformInPlace( Microsoft::glTF::Vector3& value ) const;

  static Matrix Translation( const Vec3& translation );
  static Matrix Scaling( const Vec3& scale );

private:
  std::array<std::array<float, 4>, 4> m_matrix = {};
};

} // namespace gltfviewer
