#include "cycles_renderer.h"

#include "cycles_displaydriver.h"
#include "cycles_outputdriver.h"

#include <scene/background.h>
#include <scene/camera.h>
#include <scene/integrator.h>
#include <scene/light.h>
#include <scene/mesh.h>
#include <scene/object.h>
#include <scene/shader_nodes.h>

// Transform from glTF coordinate system to Cycles.
static constexpr ccl::Transform kRootTransform = { { 1.0f, 0, 0, 0 }, { 0, 0, -1.0f, 0 }, { 0, 1.0f, 0, 0 } };

static ccl::Transform ToTransform( const gltfviewer::Matrix& matrix )
{
  const auto& values = matrix.GetValues();

  ccl::Transform transform;
  transform.x = ccl::make_float4( values[ 0 ][ 0 ], values[ 0 ][ 1 ], values[ 0 ][ 2 ], values[ 0 ][ 3 ] );
  transform.y = ccl::make_float4( values[ 1 ][ 0 ], values[ 1 ][ 1 ], values[ 1 ][ 2 ], values[ 1 ][ 3 ] );
  transform.z = ccl::make_float4( values[ 2 ][ 0 ], values[ 2 ][ 1 ], values[ 2 ][ 2 ], values[ 2 ][ 3 ] );
  return transform;
}

static void SetTextureWrapping( ccl::ImageTextureNode* imageTextureNode, const gltfviewer::Material::Texture& textureSettings )
{
  // TODO support for mirrored repeat and mixed S/T wrap modes (these will require a more complex shader chain, a la Blender).
  const auto wrapMode = textureSettings.wrapS;
  if ( imageTextureNode ) {
    ccl::ExtensionType extensionType = ccl::EXTENSION_NUM_TYPES;
    switch ( wrapMode ) {
      case Microsoft::glTF::Wrap_CLAMP_TO_EDGE : {
        extensionType = ccl::EXTENSION_EXTEND;
        break;
      }
      case Microsoft::glTF::Wrap_MIRRORED_REPEAT :
      default : {
        extensionType = ccl::EXTENSION_REPEAT;
        break;
      }
    }
    imageTextureNode->set_extension( extensionType );
  }
}

CyclesRenderer::CyclesRenderer( const gltfviewer::Model& model ) : gltfviewer::Renderer( model )
{
}

CyclesRenderer::~CyclesRenderer()
{
   StopRender();
}

bool CyclesRenderer::StartRender( const int32_t scene_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, void* render_callback_context )
{
  StopRender();
  if ( InitialiseSession( render_settings, render_callback, render_callback_context, true ) && BuildScene( scene_index ) && SetCamera( camera, render_settings ) && SetBackground( environment_settings ) ) {
    m_session->start();
    return true;
  } else {
    return false;
  }
}

void CyclesRenderer::StopRender()
{
  if ( m_session ) {
    m_session->cancel();
    m_session->wait();

    std::error_code ec;
    if ( std::filesystem::exists( m_temp_folder, ec ) ) {
      std::filesystem::remove_all( m_temp_folder, ec );
    }
  }
}

bool CyclesRenderer::InitialiseSession( const gltfviewer_render_settings& render_settings, gltfviewer_render_callback render_callback, void* render_callback_context, const bool forceCPU )
{
  constexpr int kDefaultTileSize = 2048;
  const uint32_t tileSize = ( 0 == render_settings.tile_size ) ? kDefaultTileSize : render_settings.tile_size;

  ccl::SessionParams sessionParams;
  sessionParams.device = SelectDevice( forceCPU );
  sessionParams.tile_size = std::clamp( tileSize, 32u, 4096u );
  sessionParams.use_auto_tile = true;
  sessionParams.background = true;
  sessionParams.samples = render_settings.samples;

  std::error_code ec;
  m_temp_folder = std::filesystem::temp_directory_path( ec ) / GenerateGUID();
  std::filesystem::create_directory( m_temp_folder, ec );
  sessionParams.temp_dir = m_temp_folder.string();

  ccl::SceneParams sceneParams;
  sceneParams.background = true;
  sceneParams.bvh_layout = ccl::BVHLayout::BVH_LAYOUT_EMBREE;
  sceneParams.bvh_type = ccl::BVHType::BVH_TYPE_STATIC;

  m_session = std::make_unique<ccl::Session>( sessionParams, sceneParams );

  m_session->full_buffer_written_cb = [this]( OpenImageIO_v2_3::string_view filename ) {
    if ( !m_session->progress.get_cancel() )
      m_session->process_full_buffer_from_disk( filename );
  };

  m_session->scene->integrator->set_use_denoise( true );
  m_session->scene->integrator->set_use_denoise_pass_albedo( true );
  m_session->scene->integrator->set_use_denoise_pass_normal( true );

  m_session->scene->passes.clear();
  const std::string kPassName( "combined" );
  if ( ccl::Pass* pass = m_session->scene->create_node<ccl::Pass>(); pass ) {
    pass->set_name( OpenImageIO_v2_3::ustring( kPassName ) );
    pass->set_type( ccl::PassType::PASS_COMBINED );
    pass->set_mode( ccl::PassMode::DENOISED );
  }

  m_session->set_display_driver( std::make_unique<CyclesDisplayDriver>( render_callback, render_callback_context ) );
  m_session->set_output_driver( std::make_unique<CyclesOutputDriver>( kPassName, render_callback, render_callback_context ) );

#ifdef _DEBUG
  m_session->progress.set_update_callback( [this]() {
    std::string status;
    std::string subStatus;
    m_session->progress.get_status( status, subStatus );
    std::string str = status;
    if ( !subStatus.empty() ) {
      if ( !str.empty() ) {
        str += " - ";
      }
      str += subStatus;
    }
    if ( !str.empty() ) {
      str += "\r\n";
      OutputDebugStringA( str.c_str() );
    }
  } );
#endif

  ccl::BufferParams bufferParams = {};
  bufferParams.width = render_settings.width;
  bufferParams.height = render_settings.height;
  bufferParams.full_width = bufferParams.width;
  bufferParams.full_height = bufferParams.height;
  bufferParams.window_width = bufferParams.width;
  bufferParams.window_height = bufferParams.height;
  m_session->reset( sessionParams, bufferParams );

  return true;
}

ccl::DeviceInfo CyclesRenderer::SelectDevice( const bool forceCPU )
{
  ccl::DeviceInfo deviceInfo = {};
  auto devices = ccl::Device::available_devices( forceCPU ? ccl::DeviceTypeMask::DEVICE_MASK_CPU : ccl::DeviceTypeMask::DEVICE_MASK_ALL );
  if ( devices.empty() ) {
    return {};
  }

  std::sort( devices.begin(), devices.end(), []( const ccl::DeviceInfo& a, const ccl::DeviceInfo& b ) {
    return static_cast<int>( a.type ) > static_cast<int>( b.type );
  } );
  return devices.front();
}

static OpenImageIO_v2_3::ustring GetUVAttributeName( const size_t textureCoordinateChannel )
{
  return OpenImageIO_v2_3::ustring( std::to_string( textureCoordinateChannel ) );
}

bool CyclesRenderer::BuildScene( const int32_t scene_index )
{
  const auto& meshes = m_model.GetMeshes( scene_index );
  for ( const auto& sourceMesh : meshes ) {
    if ( ccl::Mesh* targetMesh = m_session->scene->create_node<ccl::Mesh>(); nullptr != targetMesh ) {
      const int vertexCount = static_cast<int>( sourceMesh.m_positions.size() );
      const int faceCount = static_cast<int>( sourceMesh.m_indices.size() / 3 );

      targetMesh->reserve_mesh( vertexCount, faceCount );

      for ( const auto& vertex : sourceMesh.m_positions ) {
        targetMesh->add_vertex( ccl::make_float3( vertex.x, vertex.y, vertex.z ) );
      }

      for ( auto i = sourceMesh.m_indices.begin(); sourceMesh.m_indices.end() != i; ) {
        const int v0 = static_cast<int>( *i++ );
        const int v1 = static_cast<int>( *i++ );
        const int v2 = static_cast<int>( *i++ );
        targetMesh->add_triangle( v0, v1, v2, 0, true );
      }

      // TODO fix normals, as they don't seem to be working
      if ( sourceMesh.m_normals.size() == sourceMesh.m_positions.size() ) {
        if ( ccl::Attribute* attribute = targetMesh->attributes.add( ccl::ATTR_STD_VERTEX_NORMAL ); ( nullptr != attribute ) ) {
          if ( ccl::float3* normal = attribute->data_float3(); nullptr != normal ) {
            uint32_t normalIndex = 0;
            for ( const auto& n : sourceMesh.m_normals ) {
              normal[ normalIndex++ ] = ccl::make_float3( n.x, n.y, n.z );
            }
          }
        }
      }

      for ( size_t channel = 0; channel < sourceMesh.m_texcoords.size(); channel++ ) {
        const auto& texCoords = sourceMesh.m_texcoords[ channel ];
        if ( ccl::Attribute* attribute = targetMesh->attributes.add( GetUVAttributeName( channel ), OpenImageIO_v2_3::TypeFloat2, ccl::ATTR_ELEMENT_CORNER ); ( nullptr != attribute ) ) {
          if ( ccl::float2* uv = attribute->data_float2(); nullptr != uv ) {
            uint32_t texIndex = 0;
            for ( auto i = sourceMesh.m_indices.begin(); sourceMesh.m_indices.end() != i; ) {
              const int v0 = static_cast<int>( *i++ );
              const int v1 = static_cast<int>( *i++ );
              const int v2 = static_cast<int>( *i++ );
              uv[ texIndex++ ] = ccl::make_float2( texCoords[ v0 ].x, texCoords[ v0 ].y );
              uv[ texIndex++ ] = ccl::make_float2( texCoords[ v1 ].x, texCoords[ v1 ].y );
              uv[ texIndex++ ] = ccl::make_float2( texCoords[ v2 ].x, texCoords[ v2 ].y );
            }
          }
        }
      }

      ccl::array<ccl::Node*> usedShaders = targetMesh->get_used_shaders();
      usedShaders.push_back_slow( GetShader( sourceMesh.m_materialID ) );
      targetMesh->set_used_shaders( usedShaders );

      targetMesh->tag_update( m_session->scene, true );

      // TODO maybe group meshes into objects (i.e. maintain GLTF tree structure in our input scene), but for now just create one object per mesh.
      if ( ccl::Object* obj = m_session->scene->create_node<ccl::Object>(); nullptr != obj ) {
        obj->set_geometry( targetMesh );
        obj->set_visibility( ccl::PATH_RAY_CAMERA | ccl::PATH_RAY_REFLECT | ccl::PATH_RAY_TRANSMIT | ccl::PATH_RAY_DIFFUSE | ccl::PATH_RAY_GLOSSY | ccl::PATH_RAY_SINGULAR | ccl::PATH_RAY_TRANSPARENT | ccl::PATH_RAY_VOLUME_SCATTER | ccl::PATH_RAY_SHADOW );
        obj->set_alpha( 1.0f );
        obj->set_random_id( std::rand() );
        obj->set_tfm( kRootTransform );
        obj->tag_update( m_session->scene );
      }
    }
  }
  return true;
}

bool CyclesRenderer::SetCamera( const gltfviewer_camera& _camera, const gltfviewer_render_settings& render_settings )
{
  const gltfviewer::Camera camera( _camera, m_model, render_settings.width, render_settings.height );
  const auto matrix = camera.GetMatrix();

  const auto scaling = gltfviewer::Matrix::Scaling( { 1.0f, 1.0f, -1.0f } );
  const ccl::Transform transform = kRootTransform * ToTransform( matrix * scaling );

  m_session->scene->camera->set_matrix( transform );
  m_session->scene->camera->set_fov( camera.GetFOVRadians() );
  m_session->scene->camera->set_nearclip( camera.GetNearClip() );
  m_session->scene->camera->set_farclip( camera.GetFarClip() );
  m_session->scene->camera->set_full_width( render_settings.width );
  m_session->scene->camera->set_full_height( render_settings.height );
  m_session->scene->camera->compute_auto_viewplane();
  m_session->scene->camera->update( m_session->scene );
  return true;
}

bool CyclesRenderer::SetBackground( const gltfviewer_environment_settings& environment_settings )
{
  if ( m_session->scene->default_background && m_session->scene->default_background->graph ) {
    ccl::BackgroundNode* background = m_session->scene->default_background->graph->create_node<ccl::BackgroundNode>();
    background->set_strength( environment_settings.sky_intensity );
    background->set_color( ccl::make_float3( 1.0f, 1.0f, 1.0f ) );
    m_session->scene->default_background->graph->add( background );

    ccl::SkyTextureNode* sky = m_session->scene->default_background->graph->create_node<ccl::SkyTextureNode>();
    sky->set_sky_type( ccl::NODE_SKY_NISHITA );
    sky->set_sun_intensity( environment_settings.sun_intensity );
    sky->set_sun_elevation( DegreeToRadian( std::clamp( environment_settings.sun_elevation, 0.0f, 90.0f ) ) );
    sky->set_sun_rotation( DegreeToRadian( environment_settings.sun_rotation ) );
    m_session->scene->default_background->graph->add( sky );

    {
      ccl::ShaderOutput* from = background->output( "Background" );
      ccl::ShaderInput* to = m_session->scene->default_background->graph->output()->input( "Surface" );
      m_session->scene->default_background->graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = sky->output( "Color" );
      ccl::ShaderInput* to = background->input( "Color" );
      m_session->scene->default_background->graph->connect( from, to );
    }

    m_session->scene->background->set_transparent( environment_settings.transparent_background );

    m_session->scene->default_background->tag_used( m_session->scene );
  }
  return true;
}

ccl::Shader* CyclesRenderer::GetShader( const std::string& materialID )
{
  auto it = m_shader_map.find( materialID );
  if ( m_shader_map.end() == it ) {
    const auto& material = m_model.GetMaterial( materialID );
    it = m_shader_map.insert( { materialID, CreateShader( material ) } ).first;   
  }
  return it->second;
}

ccl::Shader* CyclesRenderer::CreateShader( const gltfviewer::Material& material )
{
  ccl::Shader* shader = m_session->scene->create_node<ccl::Shader>();
  if ( !shader )
    return shader;

  shader->name = material.m_name;

  ccl::ShaderGraph* graph = new ccl::ShaderGraph();

  // Base PBR properties
  ccl::PrincipledBsdfNode* principledBsdfNode = graph->create_node<ccl::PrincipledBsdfNode>();
  graph->add( principledBsdfNode );
  principledBsdfNode->set_base_color( ccl::make_float3( material.m_pbrBaseColorFactor.r, material.m_pbrBaseColorFactor.g, material.m_pbrBaseColorFactor.b ) );
  principledBsdfNode->set_roughness( material.m_pbrRoughnessFactor );
  principledBsdfNode->set_metallic( material.m_pbrMetallicFactor );
  principledBsdfNode->set_alpha( ( Microsoft::glTF::AlphaMode::ALPHA_OPAQUE != material.m_alphaMode ) ? material.m_alphaMode : 1.0f );
  principledBsdfNode->set_ior( material.m_ior );

  {
    ccl::ShaderOutput* from = principledBsdfNode->output( "BSDF" );
    ccl::ShaderInput* to = graph->output()->input( "Surface" );
    graph->connect( from, to );
  }

  // Base PBR texture
  std::error_code ec;
  if ( !material.m_pbrBaseTexture.filepath.empty() && std::filesystem::exists( material.m_pbrBaseTexture.filepath, ec ) ) {
    ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
    graph->add( imageTextureNode );
    imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_pbrBaseTexture.filepath.string() ) );
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "sRGB" ) );
    SetTextureWrapping( imageTextureNode, material.m_pbrBaseTexture );

    if ( ( material.m_pbrBaseColorFactor.r != 1.0f ) || ( material.m_pbrBaseColorFactor.g != 1.0f ) || ( material.m_pbrBaseColorFactor.b != 1.0f ) ) {
      ccl::MixNode* mixNode = graph->create_node<ccl::MixNode>();
      mixNode->set_mix_type( ccl::NODE_MIX_MUL );
      mixNode->set_fac( 1.0f );
      mixNode->set_color1( ccl::make_float3( material.m_pbrBaseColorFactor.r, material.m_pbrBaseColorFactor.g, material.m_pbrBaseColorFactor.b ) );
      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = mixNode->input( "Color2" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = mixNode->output( "Color" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Base Color" );
        graph->connect( from, to );
      }
      graph->add( mixNode );
    } else {
      ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
      ccl::ShaderInput* to = principledBsdfNode->input( "Base Color" );
      graph->connect( from, to );
    }
    
    if ( Microsoft::glTF::AlphaMode::ALPHA_OPAQUE == material.m_alphaMode ) {
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
    } else {
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_AUTO );

      ccl::MixClosureNode* mixClosureNode = graph->create_node<ccl::MixClosureNode>();
      graph->add( mixClosureNode );

      ccl::TransparentBsdfNode* transparentBsdfNode = graph->create_node<ccl::TransparentBsdfNode>();
      graph->add( transparentBsdfNode );

      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
        ccl::ShaderInput* to = mixClosureNode->input( "Fac" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = transparentBsdfNode->output( "BSDF" );
        ccl::ShaderInput* to = mixClosureNode->input( "Closure1" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = principledBsdfNode->output( "BSDF" );
        ccl::ShaderInput* to = mixClosureNode->input( "Closure2" );
        from->disconnect();
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = mixClosureNode->output( "Closure" );
        ccl::ShaderInput* to = graph->output()->input( "Surface" );
        graph->connect( from, to );
      }
    }

    AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_pbrBaseTexture );
  }

  // Metallic roughness texture
  if ( !material.m_pbrMetallicRoughnessTexture.filepath.empty() && std::filesystem::exists( material.m_pbrMetallicRoughnessTexture.filepath, ec ) ) {
    ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
    graph->add( imageTextureNode );
    imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_pbrMetallicRoughnessTexture.filepath.string() ) );
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "linear" ) );
    imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
    SetTextureWrapping( imageTextureNode, material.m_pbrMetallicRoughnessTexture );

    ccl::SeparateRGBNode* separateRGBNode = graph->create_node<ccl::SeparateRGBNode>();
    graph->add( separateRGBNode );

    {
      ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
      ccl::ShaderInput* to = separateRGBNode->input( "Image" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = separateRGBNode->output( "G" );
      ccl::ShaderInput* to = principledBsdfNode->input( "Roughness" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = separateRGBNode->output( "B" );
      ccl::ShaderInput* to = principledBsdfNode->input( "Metallic" );
      graph->connect( from, to );
    }

    AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_pbrMetallicRoughnessTexture );
  }

  // TODO fix normal maps, as they don't seem to be working
  if ( !material.m_normalTexture.filepath.empty() && std::filesystem::exists( material.m_normalTexture.filepath, ec ) ) {
    ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
    graph->add( imageTextureNode );
    imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_normalTexture.filepath.string() ) );
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "linear" ) );
    imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
    SetTextureWrapping( imageTextureNode, material.m_normalTexture );

    ccl::NormalMapNode* normalMapNode = graph->create_node<ccl::NormalMapNode>();
    normalMapNode->set_strength( 1.0f );
    normalMapNode->set_attribute( GetUVAttributeName( material.m_normalTexture.textureCoordinateChannel ) );
    graph->add( normalMapNode );

    {
      ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
      ccl::ShaderInput* to = normalMapNode->input( "Color" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = normalMapNode->output( "Normal" );
      ccl::ShaderInput* to = principledBsdfNode->input( "Normal" );
      graph->connect( from, to );
    }

    AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_normalTexture );
  }

  // Emission
  if ( ( material.m_emissiveFactor.r > 0 ) || ( material.m_emissiveFactor.g > 0 ) || ( material.m_emissiveFactor.b > 0 ) || ( !material.m_emissiveTexture.filepath.empty() && std::filesystem::exists( material.m_emissiveTexture.filepath, ec ) ) ) {
    ccl::EmissionNode* emissionNode = graph->create_node<ccl::EmissionNode>();
    emissionNode->set_color( ccl::make_float3( material.m_emissiveFactor.r, material.m_emissiveFactor.g, material.m_emissiveFactor.b ) );
    graph->add( emissionNode );

    ccl::AddClosureNode* addClosureNode = graph->create_node<ccl::AddClosureNode>();
    graph->add( addClosureNode );

    if ( !material.m_emissiveTexture.filepath.empty() && std::filesystem::exists( material.m_emissiveTexture.filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_emissiveTexture.filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "sRGB" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
      SetTextureWrapping( imageTextureNode, material.m_emissiveTexture );

      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = emissionNode->input( "Color" );
        graph->connect( from, to );
      }

      AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_emissiveTexture );
    }

    if ( ccl::ShaderInput* materialSurfaceInput = graph->output()->input( "Surface" );  nullptr != materialSurfaceInput ) {
      ccl::ShaderOutput* previousSurfaceOutput = materialSurfaceInput->link;
      previousSurfaceOutput->disconnect();
      {
        ccl::ShaderOutput* from = emissionNode->output( "Emission" );
        ccl::ShaderInput* to = addClosureNode->input( "Closure1" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = previousSurfaceOutput;
        ccl::ShaderInput* to = addClosureNode->input( "Closure2" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = addClosureNode->output( "Closure" );
        ccl::ShaderInput* to = graph->output()->input( "Surface" );
        graph->connect( from, to );
      }
    }
  }

  // Transmission extension
  if ( material.m_transmissionFactor ) {
    principledBsdfNode->set_transmission( *material.m_transmissionFactor );
  }
  if ( material.m_transmissionTexture && !material.m_transmissionTexture->filepath.empty() && std::filesystem::exists( material.m_transmissionTexture->filepath, ec ) ) {
    ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
    graph->add( imageTextureNode );
    imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_transmissionTexture->filepath.string() ) );
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "linear" ) );
    imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
    SetTextureWrapping( imageTextureNode, *material.m_transmissionTexture );

    ccl::SeparateRGBNode* separateRGBNode = graph->create_node<ccl::SeparateRGBNode>();
    graph->add( separateRGBNode );

    {
      ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
      ccl::ShaderInput* to = separateRGBNode->input( "Image" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = separateRGBNode->output( "R" );
      ccl::ShaderInput* to = principledBsdfNode->input( "Transmission" );
      graph->connect( from, to );
    }

    AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, *material.m_transmissionTexture );
  }

  // Volume extension
  if ( material.m_volumeDensity ) {
    ccl::AbsorptionVolumeNode* volumeNode = graph->create_node<ccl::AbsorptionVolumeNode>();
    graph->add( volumeNode );
    volumeNode->set_density( *material.m_volumeDensity );
    volumeNode->set_color( ccl::make_float3( material.m_volumeColor.r, material.m_volumeColor.g, material.m_volumeColor.b ) );
    {
      ccl::ShaderOutput* from = volumeNode->output( "Volume" );
      ccl::ShaderInput* to = graph->output()->input( "Volume" );
      graph->connect( from, to );
    }
  }

  // Clearcoat extension
  if ( material.m_clearcoatFactor || material.m_clearcoatRoughnessFactor || material.m_clearcoatTexture || material.m_clearcoatRoughnessTexture ) {
    if ( material.m_clearcoatFactor ) {
      principledBsdfNode->set_clearcoat( *material.m_clearcoatFactor );
    }

    if ( material.m_clearcoatRoughnessFactor ) {
      principledBsdfNode->set_clearcoat_roughness( *material.m_clearcoatRoughnessFactor );
    }

    if ( material.m_clearcoatTexture && std::filesystem::exists( material.m_clearcoatTexture->filepath ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_clearcoatTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "linear" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
      SetTextureWrapping( imageTextureNode, *material.m_clearcoatTexture );

      ccl::SeparateRGBNode* separateRGBNode = graph->create_node<ccl::SeparateRGBNode>();
      graph->add( separateRGBNode );

      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = separateRGBNode->input( "Image" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = separateRGBNode->output( "R" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Clearcoat" );
        graph->connect( from, to );
      }
      AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, *material.m_clearcoatTexture );
    }

    if ( material.m_clearcoatRoughnessTexture && std::filesystem::exists( material.m_clearcoatRoughnessTexture->filepath ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_clearcoatRoughnessTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "linear" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
      SetTextureWrapping( imageTextureNode, *material.m_clearcoatRoughnessTexture );

      ccl::SeparateRGBNode* separateRGBNode = graph->create_node<ccl::SeparateRGBNode>();
      graph->add( separateRGBNode );

      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = separateRGBNode->input( "Image" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = separateRGBNode->output( "G" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Clearcoat Roughness" );
        graph->connect( from, to );
      }
      AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, *material.m_clearcoatRoughnessTexture );
    }
  }

  if ( !material.m_doubleSided ) {
    AddBackfaceCulling( graph );
  }

  shader->set_graph( graph );
  shader->tag_used( m_session->scene );
  return shader;
}

void CyclesRenderer::AddBackfaceCulling( ccl::ShaderGraph* graph )
{
  if ( nullptr == graph )
    return;

  if ( ccl::ShaderInput* materialSurfaceInput = graph->output()->input( "Surface" );  nullptr != materialSurfaceInput ) {
    ccl::ShaderOutput* previousSurfaceOutput = materialSurfaceInput->link;
    previousSurfaceOutput->disconnect();

    ccl::GeometryNode* geometryNode = graph->create_node<ccl::GeometryNode>();
    graph->add( geometryNode );

    ccl::TransparentBsdfNode* transparentNode = graph->create_node<ccl::TransparentBsdfNode>();
    graph->add( transparentNode );

    ccl::MixClosureNode* mixNode = graph->create_node<ccl::MixClosureNode>();
    graph->add( mixNode );

    {
      ccl::ShaderOutput* from = geometryNode->output( "Backfacing" );
      ccl::ShaderInput* to = mixNode->input( "Fac" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = previousSurfaceOutput;
      ccl::ShaderInput* to = mixNode->input( "Closure1" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = transparentNode->output( "BSDF" );
      ccl::ShaderInput* to = mixNode->input( "Closure2" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = mixNode->output( "Closure" );
      ccl::ShaderInput* to = materialSurfaceInput;
      graph->connect( from, to );
    }
  }
}

void CyclesRenderer::AddUVMapping( ccl::ShaderGraph* graph, ccl::ShaderInput* shader_input, ccl::AttributeRequestSet& attributes, const gltfviewer::Material::Texture& texture )
{
  if ( ( nullptr != graph ) && ( nullptr != shader_input ) ) {
    ccl::UVMapNode* uvMapNode = graph->create_node<ccl::UVMapNode>();
    uvMapNode->set_attribute( GetUVAttributeName( texture.textureCoordinateChannel ) );
    graph->add( uvMapNode );

    ccl::MappingNode* mappingNode = graph->create_node<ccl::MappingNode>();
    graph->add( mappingNode );
    mappingNode->set_location( ccl::make_float3( texture.offset.x, 1.0f - texture.offset.y, 0 ) );
    mappingNode->set_rotation( ccl::make_float3( 0, 0, texture.rotation ) );
    mappingNode->set_scale( ccl::make_float3( texture.scale.x, -texture.scale.y, 0 ) );

    {
      ccl::ShaderOutput* from = uvMapNode->output( "UV" );
      ccl::ShaderInput* to = mappingNode->input( "Vector" );
      graph->connect( from, to );
    }
    {
      ccl::ShaderOutput* from = mappingNode->output( "Vector" );
      ccl::ShaderInput* to = shader_input;
      graph->connect( from, to );
    }

    if ( !attributes.find( GetUVAttributeName( texture.textureCoordinateChannel ) ) ) {
      attributes.add( GetUVAttributeName( texture.textureCoordinateChannel ) );
    }
  }
}
