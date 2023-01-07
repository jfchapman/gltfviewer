#pragma once

#include "gltfviewer.h"
#include "camera.h"
#include "matrix.h"
#include "material.h"
#include "mesh.h"
#include "vec.h"
#include "light.h"

#include <GLTFSDK/Document.h>
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <filesystem>
#include <map>

namespace gltfviewer {

class Renderer;

// Represents a flattened out scene (geometry with no instancing and with all transformations baked in)
class Model
{
public:
  Model();
  ~Model();

  bool Load( const std::filesystem::path& filepath );

  uint32_t GetSceneCount() const;

  const std::vector<Mesh>& GetMeshes( const int32_t scene_index = -1 ) const;
  const std::vector<Light>& GetLights( const int32_t scene_index = -1 ) const;
  const std::vector<Camera>& GetCameras( const int32_t scene_index = -1 ) const;

  const Material& GetMaterial( const std::string materialID ) const;

  Bounds GetBounds( const int32_t scene_index = -1 ) const;

  // TODO move rendering methods outside of the model, they do not really belong here.
  bool StartRender( const int32_t scene_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, void* render_callback_context );
  void StopRender();

private:
  struct Scene {
    std::string id;
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<Light> lights;
    std::vector<Camera> cameras;
  };

  const Model::Scene& GetScene( const int32_t scene_index ) const;

  bool ReadScenes();
  bool ReadNode( const std::string& nodeID, Matrix matrix, Scene& scene );
  bool ReadMesh( const std::string& meshID, const Matrix& matrix, Scene& scene );
  bool ReadMaterial( const std::string& materialID );
  bool ReadTriangles( const Microsoft::glTF::MeshPrimitive& primitive, const Matrix& matrix, Scene& scene );
  bool ReadCamera( const std::string& cameraID, const Matrix& matrix, Scene& scene );
  bool ReadLight( Light light, const Matrix& matrix, Scene& scene );

  std::vector<Scene> m_scenes;
  
  std::map<std::string, Material> m_materials;

  std::vector<Light> m_lights;

  TextureMap m_textures;
 
  Microsoft::glTF::Document m_document;
  std::unique_ptr<Microsoft::glTF::GLTFResourceReader> m_resourceReader;

  std::unique_ptr<Renderer> m_renderer;

  const Material kDefaultMaterial;
  const Scene kEmptyScene;
};

} // namespace gltfviewer
