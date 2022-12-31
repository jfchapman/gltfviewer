#pragma once

#include <GLTFSDK/Math.h>

#include "vec.h"

#include <string>
#include <vector>

namespace gltfviewer {

class Mesh
{
public:
  std::vector<uint32_t> m_indices;

  std::vector<Microsoft::glTF::Vector3> m_positions;
  std::vector<Microsoft::glTF::Vector3> m_normals;
  std::vector<std::vector<Vec2>> m_texcoords;
  std::vector<Vec4> m_tangents;

  std::string m_materialID;

  void GenerateTangents( const size_t textureCoordinateIndex );
};

} // namespace gltfviewer