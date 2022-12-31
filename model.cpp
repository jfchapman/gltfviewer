#include "model.h"

#include "cycles_renderer.h"

#include <fstream>
#include <memory>
#include <numeric>

#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/exceptions.h>
#include <GLTFSDK/IStreamReader.h>

class StreamReader : public Microsoft::glTF::IStreamReader
{
public:
    StreamReader( std::filesystem::path pathBase ) : m_pathBase( std::move( pathBase ) )
    {
    }

    // Resolves the relative URIs of any external resources declared in the glTF manifest
    std::shared_ptr<std::istream> GetInputStream( const std::string& filename ) const override
    {
        auto streamPath = m_pathBase / std::filesystem::path( filename );
        auto stream = std::make_shared<std::ifstream>( streamPath, std::ios_base::binary );
        if (!stream || !(*stream))
        {
            throw std::runtime_error( "Unable to create a valid input stream for uri: " + filename );
        }

        return stream;
    }

private:
    std::filesystem::path m_pathBase;
};

std::string str_tolower( std::string s )
{
  std::transform( s.begin(), s.end(), s.begin(), [] ( unsigned char c ){ return std::tolower( c ); } );
  return s;
};

namespace gltfviewer {

Model::Model()
{

}

Model::~Model()
{
  for ( const auto& [ id, filepath ] : m_textures ) {
    std::error_code ec;
    std::filesystem::remove( filepath, ec );
  }
}

bool Model::Load( const std::filesystem::path& filepath )
{
  try {

    auto streamReader = std::make_unique<StreamReader>( filepath.parent_path() );

    std::string manifest;

    const auto extension = str_tolower( filepath.extension().string() );

    if ( extension == ".gltf" ) {
      auto gltfStream = streamReader->GetInputStream( filepath.string() );
      auto gltfResourceReader = std::make_unique<Microsoft::glTF::GLTFResourceReader>( std::move( streamReader ) );

      std::stringstream manifestStream;
      manifestStream << gltfStream->rdbuf();
      manifest = manifestStream.str();

      m_resourceReader = std::move( gltfResourceReader );
    } else if ( extension == ".glb" ) {
      auto glbStream = streamReader->GetInputStream( filepath.string() );
      auto glbResourceReader = std::make_unique<Microsoft::glTF::GLBResourceReader>( std::move( streamReader ), std::move( glbStream ) );

      manifest = glbResourceReader->GetJson();

      m_resourceReader = std::move( glbResourceReader );
    }

    if ( !m_resourceReader ) {
      throw std::runtime_error( "Command line argument path filename extension must be .gltf or .glb" );
    }

    try {
      auto extensionDeserializer = Microsoft::glTF::KHR::GetKHRExtensionDeserializer();
      extensionDeserializer.AddHandler<KHR_materials_transmission, Microsoft::glTF::Material>( KHR_materials_transmission::Name, Deserialize_KHR_materials_transmission );
      extensionDeserializer.AddHandler<KHR_materials_ior, Microsoft::glTF::Material>( KHR_materials_ior::Name, Deserialize_KHR_materials_ior );
      extensionDeserializer.AddHandler<KHR_materials_volume, Microsoft::glTF::Material>( KHR_materials_volume::Name, Deserialize_KHR_materials_volume );
      extensionDeserializer.AddHandler<KHR_materials_clearcoat, Microsoft::glTF::Material>( KHR_materials_clearcoat::Name, Deserialize_KHR_materials_clearcoat );

      m_document = Microsoft::glTF::Deserialize( manifest, extensionDeserializer );
    } catch ( const Microsoft::glTF::GLTFException& ex ) {
      throw std::runtime_error( ex.what() );
    }

  } catch ( const std::runtime_error& ) {
    return false;
  }

  ReadScenes();

  return true;
}

bool Model::ReadScenes()
{
  if ( 0 == m_document.scenes.Size() ) {
    return false;
  }

  Matrix matrix;
  for ( const auto& scene : m_document.scenes.Elements() ) {
    m_scenes.push_back( { scene.id, scene.name, {}, {} } );
    for ( const auto& node : scene.nodes ) {
      ReadNode( node, matrix, m_scenes.at( m_scenes.size() -1 ) );
    }
  }

  return true;
}

bool Model::ReadNode( const std::string& nodeID, Matrix matrix, Scene& scene )
{
  if ( !m_document.nodes.Has( nodeID ) ) {
    return false;
  }

  const auto& node = m_document.nodes[ nodeID ];

  const auto transformType = node.GetTransformationType();
  switch ( transformType ) {
    case Microsoft::glTF::TRANSFORMATION_MATRIX : {
      const Matrix nodeMatrix( node.matrix );
      matrix *= nodeMatrix;
      break;
    }
    case Microsoft::glTF::TRANSFORMATION_TRS : {
      const Matrix nodeMatrix( node.translation, node.rotation, node.scale );
      matrix *= nodeMatrix;
      break;
    }
    default : {
      break;
    }
  }

  ReadMesh( node.meshId, matrix, scene );
  ReadCamera( node.cameraId, matrix, scene );

  for ( const auto& child : node.children ) {
    ReadNode( child, matrix, scene );
  }

  return true;
}

bool Model::ReadMesh( const std::string& meshID, const Matrix& matrix, Scene& scene )
{
  if ( !m_document.meshes.Has( meshID ) ) {
    return false;
  }

  const auto& mesh = m_document.meshes[ meshID ];
  for ( const auto& primitive : mesh.primitives ) {
    switch ( primitive.mode ) {
      case Microsoft::glTF::MESH_TRIANGLES : {
        ReadMaterial( primitive.materialId );
        ReadTriangles( primitive, matrix, scene );
        break;
      }
      default : {
        // TODO (maybe) support other primitive types.
        break;
      }
    }
  }

  return true;
}

bool Model::ReadCamera( const std::string& cameraID, const Matrix& matrix, Scene& scene )
{
  bool cameraRead = false;
  if ( m_document.cameras.Has( cameraID ) ) {
    const auto& gltfCamera = m_document.cameras.Get( cameraID );
    scene.cameras.push_back( { gltfCamera, matrix } );
    cameraRead = true;
  }
  return cameraRead;
}

bool Model::ReadMaterial( const std::string& materialID )
{
  if ( !m_document.materials.Has( materialID ) ) {
    return false;
  }

  if ( m_materials.end() == m_materials.find( materialID ) ) {
    const auto& material = m_document.materials[ materialID ];
    m_materials.insert( { materialID, { m_document, *m_resourceReader, material, m_textures } } );
  }

  return true;
}

template<typename T> 
std::vector<T> ReadData( const Microsoft::glTF::Document& document, const Microsoft::glTF::Accessor& accessor, const Microsoft::glTF::GLTFResourceReader& resourceReader )
{
  std::vector<T> data;

  static_assert( std::is_same<T, float>::value || std::is_same<T, uint32_t>::value );

  switch ( accessor.componentType ) {
    case Microsoft::glTF::COMPONENT_FLOAT : {
      if ( std::is_same<T, float>::value ) {
        data = resourceReader.ReadBinaryData<T>( document, accessor );
      }
      break;
    }
    case Microsoft::glTF::COMPONENT_UNSIGNED_INT : {
      if ( std::is_same<T, uint32_t>::value ) {
        data = resourceReader.ReadBinaryData<T>( document, accessor );
      } else {
        const auto tempData = resourceReader.ReadBinaryData<uint32_t>( document, accessor );
        data.reserve( tempData.size() );
        for ( const auto& i : tempData ) {
          data.push_back( static_cast<T>( i ) );
        }
      }
      break;
    }
    case Microsoft::glTF::COMPONENT_UNSIGNED_SHORT : {
      const auto tempData = resourceReader.ReadBinaryData<uint16_t>( document, accessor );
      data.reserve( tempData.size() );
      for ( const auto& i : tempData ) {
        data.push_back( static_cast<T>( i ) );
      }
      break;
    }
    case Microsoft::glTF::COMPONENT_SHORT : {
      const auto tempData = resourceReader.ReadBinaryData<int16_t>( document, accessor );
      data.reserve( tempData.size() );
      for ( const auto& i : tempData ) {
        data.push_back( static_cast<T>( i ) );
      }
      break;
    }
    case Microsoft::glTF::COMPONENT_UNSIGNED_BYTE : {
      const auto tempData = resourceReader.ReadBinaryData<uint8_t>( document, accessor );
      data.reserve( tempData.size() );
      for ( const auto& i : tempData ) {
        data.push_back( static_cast<T>( i ) );
      }
      break;
    }
    case Microsoft::glTF::COMPONENT_BYTE : {
      const auto tempData = resourceReader.ReadBinaryData<int8_t>( document, accessor );
      data.reserve( tempData.size() );
      for ( const auto& i : tempData ) {
        data.push_back( static_cast<T>( i ) );
      }
      break;
    }
    default : {
      break;
    }
  }

  return data;
}

static bool ValidateMesh( const Mesh& mesh )
{
  if ( mesh.m_indices.empty() || mesh.m_positions.empty() ) {
    return false;
  }

  uint32_t maxIndex = 0;
  for ( const auto v : mesh.m_indices ) {
    if ( v > maxIndex ) {
      maxIndex = v;
    }
  }

  bool validMesh = ( maxIndex < mesh.m_positions.size() );
  if ( validMesh && !mesh.m_normals.empty() ) {
    validMesh = ( maxIndex < mesh.m_normals.size() );
  }
  if ( validMesh && !mesh.m_tangents.empty() ) {
    validMesh = ( maxIndex < mesh.m_tangents.size() );
  }
  for ( auto tex = mesh.m_texcoords.begin(); validMesh && ( mesh.m_texcoords.end() != tex ); tex++ ) {
    validMesh = ( maxIndex < tex->size() );
  }

  return validMesh;
}

bool Model::ReadTriangles( const Microsoft::glTF::MeshPrimitive& primitive, const Matrix& matrix, Scene& scene )
{
  Mesh mesh;

  if ( std::string positionID; primitive.TryGetAttributeAccessorId( "POSITION", positionID ) && m_document.accessors.Has( positionID ) ) {
    const auto& accessor = m_document.accessors.Get( positionID );
    const auto data = ReadData<float>( m_document, accessor, *m_resourceReader );
    if ( 0 == data.size() % 3 ) {
      const auto vertexCount = data.size() / 3;
      mesh.m_positions.reserve( vertexCount );
      for ( size_t i = 0; i < vertexCount; i++ ) {
        mesh.m_positions.push_back( matrix.Transform( { data[ i * 3 ], data[ i * 3 + 1 ], data[ i * 3 + 2 ] } ) );
      }
    }
  }

  if ( std::string normalID; primitive.TryGetAttributeAccessorId( "NORMAL", normalID ) && m_document.accessors.Has( normalID ) ) {
    const auto& accessor = m_document.accessors.Get( normalID );
    const auto data = ReadData<float>( m_document, accessor, *m_resourceReader );
    if ( 0 == data.size() % 3 ) {
      const auto vertexCount = data.size() / 3;
      mesh.m_normals.reserve( vertexCount );
      for ( size_t i = 0; i < vertexCount; i++ ) {
        // TODO for non-uniform scaling, we should transform with the inverse transpose matrix
        auto n = matrix.Transform( { data[ i * 3 ], data[ i * 3 + 1 ], data[ i * 3 + 2 ], 0 } );
        n.Normalise();
        mesh.m_normals.push_back( n );
      }
    }
  }

  if ( std::string tangentID; primitive.TryGetAttributeAccessorId( "TANGENT", tangentID ) && m_document.accessors.Has( tangentID ) ) {
    const auto& accessor = m_document.accessors.Get( tangentID );
    const auto data = ReadData<float>( m_document, accessor, *m_resourceReader );
    if ( 0 == data.size() % 4 ) {
      const auto vertexCount = data.size() / 4;
      mesh.m_tangents.reserve( vertexCount );
      for ( size_t i = 0; i < vertexCount; i++ ) {
        auto t = matrix.Transform( { data[ i * 4 ], data[ i * 4 + 1 ], data[ i * 4 + 2 ], 0 } );
        t.Normalise();
        mesh.m_tangents.push_back( { t.x, t.y, t.z, data[ i * 4 + 3 ] } );
      }
    }
  }

  constexpr char kRootTexID[] = "TEXCOORD_";
  for ( size_t texIndex = 0; ; texIndex++ ) {
    if ( std::string texcoordID; primitive.TryGetAttributeAccessorId( kRootTexID + std::to_string( texIndex ), texcoordID ) && m_document.accessors.Has( texcoordID ) ) {
      const auto& accessor = m_document.accessors.Get( texcoordID );
      const auto data = ReadData<float>( m_document, accessor, *m_resourceReader );
      if ( 0 == data.size() % 2 ) {
        const auto vertexCount = data.size() / 2;
        std::vector<Vec2> texCoords;
        texCoords.reserve( vertexCount );
        for ( size_t i = 0; i < vertexCount; i++ ) {
          texCoords.push_back( { data[ i * 2 ], data[ i * 2 + 1 ] } );
        }
        mesh.m_texcoords.emplace_back( texCoords );
      }
    } else {
      break;
    }
  }
  
  if ( primitive.indicesAccessorId.empty() ) {
    // Convert non-indexed mesh to an indexed mesh
    if ( !mesh.m_positions.empty() ) {
      mesh.m_indices.resize( mesh.m_positions.size() );
      std::iota( mesh.m_indices.begin(), mesh.m_indices.end(), 0 );
    }    
  } else {
    // Indexed mesh
    if ( m_document.accessors.Has( primitive.indicesAccessorId ) ) {
      const auto& accessor = m_document.accessors.Get( primitive.indicesAccessorId );
      mesh.m_indices = ReadData<uint32_t>( m_document, accessor, *m_resourceReader );
    }
  }

  mesh.m_materialID = primitive.materialId;

  if ( mesh.m_tangents.size() != mesh.m_positions.size() ) {
    if ( const auto it = m_materials.find( mesh.m_materialID ); m_materials.end() != it ) {
      const Material& material = it->second;
      if ( !material.m_normalTexture.filepath.empty() && std::filesystem::exists( material.m_normalTexture.filepath ) ) {
        mesh.GenerateTangents( material.m_normalTexture.textureCoordinateChannel );
      }
    }
  }

  const bool validMesh = ValidateMesh( mesh );
  if ( validMesh ) {
    scene.meshes.emplace_back( mesh );
  }
  return validMesh;
}

bool Model::StartRender( const int32_t scene_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, void* render_callback_context )
{
  render_settings;
  m_renderer = std::make_unique<CyclesRenderer>( *this );
  return m_renderer->StartRender( scene_index, camera, render_settings, environment_settings, render_callback, render_callback_context );
}

void Model::StopRender()
{
  if ( m_renderer )
    m_renderer->StopRender();
}

uint32_t Model::GetSceneCount() const
{
  return static_cast<uint32_t>( m_document.scenes.Size() );
}

const Model::Scene& Model::GetScene( const int32_t scene_index ) const
{
  if ( scene_index < 0 ) {
    if ( m_document.HasDefaultScene() ) {
      const auto it = std::find_if( m_scenes.begin(), m_scenes.end(), [ id = m_document.defaultSceneId ] ( const Scene& scene ) {
        return scene.id == id;
      } );
    }
    if ( m_scenes.empty() )
      return kEmptyScene;
    else
      return m_scenes.front();
  } else if ( scene_index < m_scenes.size() ) {
    return m_scenes.at( scene_index );
  }
  
  return kEmptyScene;
}

const std::vector<Mesh>& Model::GetMeshes( const int32_t scene_index ) const
{
  return GetScene( scene_index ).meshes;
}

const std::vector<Camera>& Model::GetCameras( const int32_t scene_index ) const
{
  return GetScene( scene_index ).cameras;
}

const Material& Model::GetMaterial( const std::string materialID ) const
{
  if ( const auto material = m_materials.find( materialID ); m_materials.end() != material ) {
    return material->second;
  } else {
    return kDefaultMaterial;
  }
}

std::pair<Microsoft::glTF::Vector3 /*minBounds*/, Microsoft::glTF::Vector3 /*maxBounds*/> Model::GetBounds( const int32_t scene_index ) const
{
  auto bounds = std::make_pair( Microsoft::glTF::Vector3( FLT_MAX, FLT_MAX, FLT_MAX ), Microsoft::glTF::Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX ) );
  auto& [ minBounds, maxBounds ] = bounds;
  const auto& meshes = GetMeshes( scene_index );
  if ( meshes.empty() || meshes.front().m_positions.empty() ) {
    minBounds = Microsoft::glTF::Vector3::ZERO;
    maxBounds = Microsoft::glTF::Vector3::ZERO;
  }
  for ( const auto& mesh : meshes ) {
    for ( const auto& vertex : mesh.m_positions ) {
      if ( vertex.x < minBounds.x ) {
        minBounds.x = vertex.x;
      }
      if ( vertex.y < minBounds.y ) {
        minBounds.y = vertex.y;
      }
      if ( vertex.z < minBounds.z ) {
        minBounds.z = vertex.z;
      }
      if ( vertex.x > maxBounds.x ) {
        maxBounds.x = vertex.x;
      }
      if ( vertex.y > maxBounds.y ) {
        maxBounds.y = vertex.y;
      }
      if ( vertex.z > maxBounds.z ) {
        maxBounds.z = vertex.z;
      }
    }
  }
  return bounds;
}


} // namespace gltfviewer