#pragma once

#include <GLTFSDK/Math.h>

#include <string>
#include <vector>

namespace gltfviewer {

struct Mesh
{
  std::vector<uint32_t> m_indices;
  
  std::vector<Microsoft::glTF::Vector3> m_positions;
  std::vector<Microsoft::glTF::Vector3> m_normals;
  std::vector<std::vector<Microsoft::glTF::Vector2>> m_texcoords;

  std::string m_materialID;
};

} // namespace gltfviewer