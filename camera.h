#pragma once

#include <GLTFSDK/GLTF.h>
#include "gltfviewer.h"
#include "matrix.h"
#include "utils.h"

namespace gltfviewer {

class Model;

using Bounds = std::pair<Microsoft::glTF::Vector3 /*minBounds*/, Microsoft::glTF::Vector3 /*maxBounds*/>;

// TODO support for orthographic cameras
class Camera
{
public:
  Camera() = default;
  Camera( const Microsoft::glTF::Camera& camera, const Matrix& matrix );
  Camera( const gltfviewer_camera& camera, const Model& model, const uint32_t viewWidth, const uint32_t viewHeight );

  operator gltfviewer_camera() const;

  enum Projection { Perspective, Orthographic };

  const Matrix& GetMatrix() const { return m_matrix; }

  float GetFarClip() const { return m_farclip; }
  float GetNearClip() const { return m_nearclip; }

  float GetFOVDegrees() const { return m_perspectiveFOV; }
  float GetFOVRadians() const { return DegreeToRadian( m_perspectiveFOV ); }

  float GetOrthographicWidth() const { return m_orthographicWidth; }
  float GetOrthographicHeight() const { return m_orthographicHeight; }

  static Matrix LookAt( const Vec3& from, const Vec3& to, const Vec3& up );

private:
  void CalculatePresetViewPosition( const Model& model, const uint32_t viewWidth, const uint32_t viewHeight );

  gltfviewer_camera_preset m_preset = gltfviewer_camera_preset_none;

  Projection m_projection = Projection::Perspective;

  Matrix m_matrix;

  float m_farclip = 1e9;
  float m_nearclip = 1e-6;

  float m_perspectiveFOV = 30.0f;

  float m_orthographicWidth = 0;
  float m_orthographicHeight = 0;
};

} // namespace gltfviewer
