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

static float ConvertLightStrength( const gltfviewer::Light& light )
{
  constexpr float kCandela = 683.0f;

  switch ( light.m_type ) {
    case gltfviewer::Light::Type::Point :
    case gltfviewer::Light::Type::Spot :
      return light.m_intensity * 4 * M_PI / kCandela;
    case gltfviewer::Light::Type::Directional :
      return light.m_intensity / kCandela;
    default :
      return light.m_intensity;
  }
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

static OpenImageIO_v2_3::ustring GetUVAttributeName( const size_t textureCoordinateChannel )
{
  return OpenImageIO_v2_3::ustring( std::to_string( textureCoordinateChannel ) );
}

CyclesRenderer::CyclesRenderer( const gltfviewer::Model& model ) : gltfviewer::Renderer( model )
{
}

CyclesRenderer::~CyclesRenderer()
{
   StopRender();
}

bool CyclesRenderer::StartRender( const int32_t scene_index, const int32_t material_variant_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context )
{
  StopRender();

  m_render_thread = std::thread( [ this, scene_index, material_variant_index, camera, render_settings, environment_settings, render_callback, progress_callback, finish_callback, context ] () {
    if ( StartSession( scene_index, material_variant_index, camera, render_settings, environment_settings, render_callback, progress_callback, finish_callback, context ) ) {
      m_session->start();
      m_session->wait();

      if ( m_session->progress.get_error() && !m_session->progress.get_cancel() && ( nullptr != m_session->device ) && !m_forceCPU && ( ccl::DeviceType::DEVICE_CPU != m_session->device->info.type ) ) {
        m_forceCPU = true;
        Cleanup();
        if ( StartSession( scene_index, material_variant_index, camera, render_settings, environment_settings, render_callback, progress_callback, finish_callback, context ) ) {
          m_session->start();
          m_session->wait();
        }
      }

      if ( ( nullptr != finish_callback ) && !m_session->progress.get_cancel() && !m_session->progress.get_error() ) {
        gltfviewer_image* noisy_image = ( ( m_result_noisy.width > 0 ) && ( m_result_noisy.height > 0 ) && ( nullptr != m_result_noisy.pixels ) ) ? &m_result_noisy : nullptr;
        gltfviewer_image* denoised_image = ( ( m_result_denoised.width > 0 ) && ( m_result_denoised.height > 0 ) && ( nullptr != m_result_denoised.pixels ) ) ? &m_result_denoised : nullptr;
        finish_callback( &m_result_noisy, &m_result_denoised, context );
      }

      if ( nullptr != progress_callback ) {
        if ( m_session->progress.get_cancel() ) {
          progress_callback( static_cast<float>( m_session->progress.get_progress() ), "Cancelled", context );
        } else {
          progress_callback( 1.0f, m_session->progress.get_error() ? "Error" : "Finished", context );
        }
      }
    }
  } );

  return true;
}

void CyclesRenderer::StopRender()
{
  m_cancel = true;
  if ( m_render_thread.joinable() ) {
    m_render_thread.join();
  }
  m_cancel = false;
  Cleanup();
}

void CyclesRenderer::Cleanup()
{
  m_shader_map.clear();

  std::error_code ec;
  if ( std::filesystem::exists( m_temp_folder, ec ) ) {
    std::filesystem::remove_all( m_temp_folder, ec );
  }
}

bool CyclesRenderer::StartSession( const int32_t scene_index, const int32_t material_variant_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context )
{
  if ( !InitialiseSession( render_settings, render_callback, progress_callback, context ) || !m_session || m_session->progress.get_cancel() )
    return false;

  if ( !BuildScene( scene_index, material_variant_index ) || m_session->progress.get_cancel() )
    return false;

  if ( !SetCamera( camera, render_settings ) || m_session->progress.get_cancel() )
    return false;
    
  if ( !SetBackground( environment_settings ) || m_session->progress.get_cancel() )
    return false;

  return true;
}

bool CyclesRenderer::InitialiseSession( const gltfviewer_render_settings& render_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, void* context )
{
  constexpr int kDefaultTileSize = 2048;
  constexpr int kDefaultSamples = 128;
  const int tileSize = ( 0 == render_settings.tile_size ) ? kDefaultTileSize : render_settings.tile_size;
  const int samples = ( 0 == render_settings.samples ) ? kDefaultSamples : render_settings.samples;

  ccl::SessionParams sessionParams;
  sessionParams.device = SelectDevice();
  sessionParams.tile_size = std::clamp( tileSize, 32, 4096 );
  sessionParams.use_auto_tile = true;
  sessionParams.background = true;
  sessionParams.samples = render_settings.samples;

  std::error_code ec;
  m_temp_folder = std::filesystem::temp_directory_path( ec ) / gltfviewer::GenerateGUID();
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
  constexpr char kNoisyPassName[] = "combined_noisy";
  if ( ccl::Pass* pass = m_session->scene->create_node<ccl::Pass>(); pass ) {
    pass->set_name( OpenImageIO_v2_3::ustring( kNoisyPassName ) );
    pass->set_type( ccl::PassType::PASS_COMBINED );
    pass->set_mode( ccl::PassMode::NOISY );
  }
  constexpr char kDenoisedPassName[] = "combined_denoised";
  if ( ccl::Pass* pass = m_session->scene->create_node<ccl::Pass>(); pass ) {
    pass->set_name( OpenImageIO_v2_3::ustring( kDenoisedPassName ) );
    pass->set_type( ccl::PassType::PASS_COMBINED );
    pass->set_mode( ccl::PassMode::DENOISED );
  }

  m_session->set_display_driver( std::make_unique<CyclesDisplayDriver>( render_callback, context ) );
  m_session->set_output_driver( std::make_unique<CyclesOutputDriver>( kNoisyPassName, kDenoisedPassName, m_result_noisy, m_result_denoised ) );

  m_session->progress.set_cancel_callback( [ this ] () {
    if ( m_cancel ) {
      m_session->progress.set_cancel( "Cancelled" );
    }
  } );

  if ( nullptr != progress_callback ) {
    m_session->progress.set_update_callback( [this, progress_callback, context]() {
      const float progress = static_cast<float>( m_session->progress.get_progress() );

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

      progress_callback( progress, str.c_str(), context );
    } );
  }

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

ccl::DeviceInfo CyclesRenderer::SelectDevice()
{
  ccl::DeviceInfo deviceInfo = {};
  auto devices = ccl::Device::available_devices( m_forceCPU ? ccl::DeviceTypeMask::DEVICE_MASK_CPU : ccl::DeviceTypeMask::DEVICE_MASK_ALL );
  if ( devices.empty() ) {
    return {};
  }

  std::sort( devices.begin(), devices.end(), []( const ccl::DeviceInfo& a, const ccl::DeviceInfo& b ) {
    return static_cast<int>( a.type ) > static_cast<int>( b.type );
  } );
  return devices.front();
}

bool CyclesRenderer::BuildScene( const int32_t scene_index, const int32_t material_variant_index )
{
  return SetMeshes( scene_index, material_variant_index ) && SetLights( scene_index );
}

bool CyclesRenderer::SetMeshes( const int32_t scene_index, const int32_t material_variant_index )
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
        if ( ccl::Attribute* attribute = targetMesh->attributes.add( GetUVAttributeName( channel ), OpenImageIO_v2_3::TypeFloat2, ccl::ATTR_ELEMENT_CORNER ); nullptr != attribute ) {
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

      std::error_code ec;
      const auto& material = m_model.GetMaterial( sourceMesh.m_materialID );
      if ( !material.m_normalTexture.filepath.empty() && std::filesystem::exists( material.m_normalTexture.filepath, ec ) && ( material.m_normalTexture.textureCoordinateChannel < sourceMesh.m_texcoords.size() ) ) {
        if ( sourceMesh.m_tangents.size() == sourceMesh.m_positions.size() ) {
          const auto tangentAttributeName = OpenImageIO_v2_3::ustring::concat( GetUVAttributeName( material.m_normalTexture.textureCoordinateChannel ), ".tangent" );
          if ( ccl::Attribute* attribute = targetMesh->attributes.add( tangentAttributeName, OpenImageIO_v2_3::TypeVector, ccl::ATTR_ELEMENT_CORNER ); nullptr != attribute ) {
            if ( ccl::float3* tangent = attribute->data_float3(); nullptr != tangent ) {
              uint32_t tangentIndex = 0;
              for ( auto i = sourceMesh.m_indices.begin(); sourceMesh.m_indices.end() != i; ) {
                const int v0 = static_cast<int>( *i++ );
                const int v1 = static_cast<int>( *i++ );
                const int v2 = static_cast<int>( *i++ );
                tangent[ tangentIndex++ ] = ccl::make_float3( sourceMesh.m_tangents[ v0 ].x, sourceMesh.m_tangents[ v0 ].y, sourceMesh.m_tangents[ v0 ].z );
                tangent[ tangentIndex++ ] = ccl::make_float3( sourceMesh.m_tangents[ v1 ].x, sourceMesh.m_tangents[ v1 ].y, sourceMesh.m_tangents[ v1 ].z );
                tangent[ tangentIndex++ ] = ccl::make_float3( sourceMesh.m_tangents[ v2 ].x, sourceMesh.m_tangents[ v2 ].y, sourceMesh.m_tangents[ v2 ].z );
              }
            }
          }
          const auto tangentSignName = OpenImageIO_v2_3::ustring::concat( GetUVAttributeName( material.m_normalTexture.textureCoordinateChannel ), ".tangent_sign" );
          if ( ccl::Attribute* attribute = targetMesh->attributes.add( tangentSignName, OpenImageIO_v2_3::TypeFloat, ccl::ATTR_ELEMENT_CORNER ); nullptr != attribute ) {
            if ( float* tangentSign = attribute->data_float(); nullptr != tangentSign ) {
              uint32_t tangentsignIndex = 0;
              for ( auto i = sourceMesh.m_indices.begin(); sourceMesh.m_indices.end() != i; ) {
                const int v = *i++;
                tangentSign[ tangentsignIndex++ ] = sourceMesh.m_tangents[ v ].w;
              }
            }
          }
        }
      }

      ccl::array<ccl::Node*> usedShaders = targetMesh->get_used_shaders();
      usedShaders.push_back_slow( GetShader( sourceMesh.m_materialID, sourceMesh.m_materialVariants, material_variant_index ) );
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

bool CyclesRenderer::SetLights( const int32_t scene_index )
{
  constexpr float kDefaultRadius = 0.025f;
  constexpr float kDefaultAngle = 0.01f;

  const auto& lights = m_model.GetLights( scene_index );
  for ( const auto& sourceLight : lights ) {
    ccl::Light* light = m_session->scene->create_node<ccl::Light>();
    switch ( sourceLight.m_type ) {
      case gltfviewer::Light::Type::Point : {
        light->set_light_type( ccl::LIGHT_POINT );
        light->set_angle( 0 );
        light->set_size( kDefaultRadius );
        break;
      }
      case gltfviewer::Light::Type::Spot : {
        light->set_light_type( ccl::LIGHT_SPOT );
        light->set_angle( 0 );
        light->set_size( kDefaultRadius );
        light->set_spot_angle( sourceLight.m_outerConeAngle );
        light->set_spot_smooth( ( ( sourceLight.m_outerConeAngle > 0 ) && ( sourceLight.m_outerConeAngle >= sourceLight.m_innerConeAngle ) ) ? ( 1.0f - sourceLight.m_innerConeAngle / sourceLight.m_outerConeAngle ) : 0.5f );
        break;
      }
      case gltfviewer::Light::Type::Directional : {
        light->set_light_type( ccl::LIGHT_DISTANT );
        light->set_angle( kDefaultAngle );
        light->set_size( 0 );
        break;
      }
      default : {
        break;
      }
    }

    const ccl::Transform transform = kRootTransform * ToTransform( sourceLight.m_transform );
    light->set_co( ccl::transform_get_column( &transform, 3 ) );
    light->set_dir( -ccl::transform_get_column( &transform, 2 ) );
    light->set_tfm( transform );

    const float strength = ConvertLightStrength( sourceLight );
    light->set_strength( ccl::make_float3( strength, strength, strength ) );

    light->set_use_mis( true );
    light->set_use_camera( false );
    
    if ( ccl::Shader* shader = m_session->scene->create_node<ccl::Shader>(); nullptr != shader ) {
      ccl::ShaderGraph* graph = new ccl::ShaderGraph();

      ccl::EmissionNode* emissionNode = graph->create_node<ccl::EmissionNode>();
      emissionNode->set_strength( 1.0f );
      emissionNode->set_color( ccl::make_float3( sourceLight.m_color.r, sourceLight.m_color.g, sourceLight.m_color.b ) );
      graph->add( emissionNode );

      ccl::ShaderOutput* from = emissionNode->output( "Emission" );
      ccl::ShaderInput* to = graph->output()->input( "Surface" );
      graph->connect( from, to );

      light->set_shader( shader );

      shader->set_graph( graph );
      shader->tag_used( m_session->scene );
      shader->tag_update( m_session->scene );
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

  if ( render_settings.height > 0 ) {
    const float aspect = static_cast<float>( render_settings.width ) / render_settings.height;
    m_session->scene->camera->set_viewplane_left( -aspect );
    m_session->scene->camera->set_viewplane_right( aspect );
    m_session->scene->camera->set_viewplane_bottom( -1.0f );
    m_session->scene->camera->set_viewplane_top( 1.0f );
  }

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
    sky->set_sun_elevation( gltfviewer::DegreeToRadian( std::clamp( environment_settings.sun_elevation, 0.0f, 90.0f ) ) );
    sky->set_sun_rotation( gltfviewer::DegreeToRadian( environment_settings.sun_rotation ) );
    sky->set_sun_disc( environment_settings.sun_intensity > 0 );
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

    ccl::Light* light = m_session->scene->create_node<ccl::Light>();
    light->set_light_type( ccl::LIGHT_BACKGROUND );
    const float strength = 1.0f;
    light->set_strength( ccl::make_float3( strength, strength, strength ) );
    light->set_use_mis( true );
    light->set_use_camera( false );
    light->set_shader( m_session->scene->default_background );

  }
  return true;
}

ccl::Shader* CyclesRenderer::GetShader( const std::string& materialID, const gltfviewer::VariantIndexToMaterialIDMap& material_variants, const int32_t material_variant_index )
{
  std::string id = materialID;
  if ( material_variant_index >= 0 ) {
    if ( const auto it = material_variants.find( material_variant_index ); ( material_variants.end() != it ) && m_model.HasMaterial( it->second ) ) {
      id = it->second;
    }
  }

  auto it = m_shader_map.find( id );
  if ( m_shader_map.end() == it ) {
    const auto& material = m_model.GetMaterial( id );
    it = m_shader_map.insert( { id, CreateShader( material ) } ).first;   
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

  ccl::PrincipledBsdfNode* principledBsdfNode = graph->create_node<ccl::PrincipledBsdfNode>();
  graph->add( principledBsdfNode );
  if ( material.m_pbrSpecularGlossiness ) {
    // Old PBR specular-glossiness extension (convert to metallic-roughness properties)
    // https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples
    static const Microsoft::glTF::Color3 kDielectricSpecular = { 0.04f, 0.04f, 0.04f };
    static constexpr float kEpsilon = 1e-6f;

    std::function<float(float, float, float)> solveMetallic = [] ( float diffuse, float specular, float oneMinusSpecularStrength ) {
      if ( specular < kDielectricSpecular.r )
        return 0.0f;

      const auto a = kDielectricSpecular.r;
      const auto b = diffuse * oneMinusSpecularStrength / ( 1 - kDielectricSpecular.r ) + specular - 2 * kDielectricSpecular.r;
      const auto c = kDielectricSpecular.r - specular;
      const auto D = b * b - 4 * a * c;
      const auto result = std::clamp( ( -b + std::sqrtf( D ) ) / ( 2 * a ), 0.0f, 1.0f );
      return result;
    };

    std::function<float(Microsoft::glTF::Color3)> colorPerceivedBrightness = [] ( Microsoft::glTF::Color3 color ) {
      return std::sqrtf( 0.299f * color.r * color.r + 0.587f * color.g * color.g + 0.114f * color.b * color.b );
    };

    std::function<float(Microsoft::glTF::Color3)> colorMaxComponent = [] ( Microsoft::glTF::Color3 color ) {
      return std::max( color.r, std::max( color.g, color.b ) );
    };

    std::function<Microsoft::glTF::Color3(Microsoft::glTF::Color3, float)> colorScale = [] ( Microsoft::glTF::Color3 color, float scale ) {
      return Microsoft::glTF::Color3( color.r * scale, color.g * scale, color.b * scale );
    };

    std::function<Microsoft::glTF::Color3(Microsoft::glTF::Color3, Microsoft::glTF::Color3)> colorSubtract = [] ( Microsoft::glTF::Color3 color, Microsoft::glTF::Color3 other ) {
      return Microsoft::glTF::Color3( std::clamp( color.r - other.r, 0.0f, 1.0f ), std::clamp( color.g - other.g, 0.0f, 1.0f ), std::clamp( color.b - other.b, 0.0f, 1.0f ) );
    };

    std::function<Microsoft::glTF::Color3(Microsoft::glTF::Color3, Microsoft::glTF::Color3, float)> colorLerp = [] ( Microsoft::glTF::Color3 color1, Microsoft::glTF::Color3 color2, float t ) {
      return Microsoft::glTF::Color3( std::clamp( std::lerp( color1.r, color2.r, t ), 0.0f, 1.0f ), std::clamp( std::lerp( color1.g, color2.g, t ), 0.0f, 1.0f ), std::clamp( std::lerp( color1.b, color2.b, t ), 0.0f, 1.0f ) );
    };

    const Microsoft::glTF::Color3 diffuse = { material.m_pbrSpecularGlossiness->diffuseFactor.r, material.m_pbrSpecularGlossiness->diffuseFactor.g, material.m_pbrSpecularGlossiness->diffuseFactor.b };
    const Microsoft::glTF::Color3 specular = material.m_pbrSpecularGlossiness->specularFactor;
    const float glossiness = material.m_pbrSpecularGlossiness->glossinessFactor;

    const float oneMinusSpecularStrength = 1.0f - colorMaxComponent( specular );
    const float metallic = solveMetallic( colorPerceivedBrightness( diffuse ), colorPerceivedBrightness( specular ), oneMinusSpecularStrength );
    const Microsoft::glTF::Color3 baseColorFromDiffuse = colorScale( diffuse, oneMinusSpecularStrength / ( 1.0f - kDielectricSpecular.r ) / std::max( 1.0f - metallic, kEpsilon ) );
    const Microsoft::glTF::Color3 baseColorFromSpecular = colorScale( colorScale( colorSubtract( specular, kDielectricSpecular ), 1.0f - metallic ), 1.0f / std::max( metallic, kEpsilon ) );
    const Microsoft::glTF::Color3 baseColor = colorLerp( baseColorFromDiffuse, baseColorFromSpecular, metallic * metallic );

    principledBsdfNode->set_base_color( ccl::make_float3( baseColor.r, baseColor.g, baseColor.b ) );
    principledBsdfNode->set_roughness( 1.0f - std::clamp( glossiness, 0.0f, 1.0f ) );
    principledBsdfNode->set_metallic( metallic );
    principledBsdfNode->set_alpha( ( Microsoft::glTF::AlphaMode::ALPHA_OPAQUE != material.m_alphaMode ) ? material.m_pbrSpecularGlossiness->diffuseFactor.a : 1.0f );
    principledBsdfNode->set_specular( colorPerceivedBrightness( specular ) );
  } else {
    // Base PBR metallic-roughness properties
    principledBsdfNode->set_base_color( ccl::make_float3( material.m_pbrBaseColorFactor.r, material.m_pbrBaseColorFactor.g, material.m_pbrBaseColorFactor.b ) );
    principledBsdfNode->set_roughness( material.m_pbrRoughnessFactor );
    principledBsdfNode->set_metallic( material.m_pbrMetallicFactor );
    principledBsdfNode->set_alpha( ( Microsoft::glTF::AlphaMode::ALPHA_OPAQUE != material.m_alphaMode ) ? material.m_pbrBaseColorFactor.a : 1.0f );
  }

  principledBsdfNode->set_ior( material.m_ior );

  {
    ccl::ShaderOutput* from = principledBsdfNode->output( "BSDF" );
    ccl::ShaderInput* to = graph->output()->input( "Surface" );
    graph->connect( from, to );
  }

  std::error_code ec;
  if ( material.m_pbrSpecularGlossiness ) {
    // Old PBR specular-glossiness extension
    AddBaseColorTexture( material.m_pbrSpecularGlossiness->diffuseTexture, material.m_pbrSpecularGlossiness->specularFactor, material.m_alphaMode, graph, principledBsdfNode, shader );

    // Specular-glossiness texture
    if ( !material.m_pbrSpecularGlossiness->specularGlossinessTexture.filepath.empty() && std::filesystem::exists( material.m_pbrSpecularGlossiness->specularGlossinessTexture.filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_pbrSpecularGlossiness->specularGlossinessTexture.filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Linear" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_UNASSOCIATED );
      SetTextureWrapping( imageTextureNode, material.m_pbrSpecularGlossiness->specularGlossinessTexture );

      // Specular
      const auto specular = principledBsdfNode->get_specular();
      ccl::SeparateHSVNode* separateHSVNode = graph->create_node<ccl::SeparateHSVNode>();
      graph->add( separateHSVNode );
      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = separateHSVNode->input( "Color" );
        graph->connect( from, to );
      }
      if ( specular != 1.0f ) {
        ccl::MathNode* mathNode = graph->create_node<ccl::MathNode>();
        graph->add( mathNode );
        mathNode->set_math_type( ccl::NODE_MATH_MULTIPLY );
        mathNode->set_value1( specular );
        {
          ccl::ShaderOutput* from = separateHSVNode->output( "V" );
          ccl::ShaderInput* to = mathNode->input( "Value2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = mathNode->output( "Value" );
          ccl::ShaderInput* to = principledBsdfNode->input( "Specular" );
          graph->connect( from, to );
        }
      } else {
        ccl::ShaderOutput* from = separateHSVNode->output( "V" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Specular" );
        graph->connect( from, to );
      }

      // Glossiness
      ccl::InvertNode* invertNode = graph->create_node<ccl::InvertNode>();
      graph->add( invertNode );
      {
        ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
        ccl::ShaderInput* to = invertNode->input( "Color" );
        graph->connect( from, to );
      }
      {
        ccl::ShaderOutput* from = invertNode->output( "Color" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Roughness" );
        graph->connect( from, to );
      }

      AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_pbrSpecularGlossiness->specularGlossinessTexture );
    }
  } else {
    // Base PBR metallic-roughness properties
    AddBaseColorTexture( material.m_pbrBaseTexture, { material.m_pbrBaseColorFactor.r, material.m_pbrBaseColorFactor.g, material.m_pbrBaseColorFactor.b }, material.m_alphaMode, graph, principledBsdfNode, shader );
      
    // Metallic roughness texture
    if ( !material.m_pbrMetallicRoughnessTexture.filepath.empty() && std::filesystem::exists( material.m_pbrMetallicRoughnessTexture.filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_pbrMetallicRoughnessTexture.filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Linear" ) );
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
  }

  // Normal texture
  if ( !material.m_normalTexture.filepath.empty() && std::filesystem::exists( material.m_normalTexture.filepath, ec ) ) {
    ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
    graph->add( imageTextureNode );
    imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_normalTexture.filepath.string() ) );
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Non-Color" ) );
    imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
    SetTextureWrapping( imageTextureNode, material.m_normalTexture );

    ccl::NormalMapNode* normalMapNode = graph->create_node<ccl::NormalMapNode>();
    normalMapNode->set_strength( 1.0f );
    normalMapNode->set_attribute( GetUVAttributeName( material.m_normalTexture.textureCoordinateChannel ) );
    normalMapNode->set_space( ccl::NODE_NORMAL_MAP_TANGENT );
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

    // Emissive strength extension
    emissionNode->set_strength( material.m_emissiveStrength );

    ccl::AddClosureNode* addClosureNode = graph->create_node<ccl::AddClosureNode>();
    graph->add( addClosureNode );

    if ( !material.m_emissiveTexture.filepath.empty() && std::filesystem::exists( material.m_emissiveTexture.filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_emissiveTexture.filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "sRGB" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_IGNORE );
      SetTextureWrapping( imageTextureNode, material.m_emissiveTexture );

      const auto& color = emissionNode->get_color();
      if ( ( color.x != 1.0f ) || ( color.y != 1.0f ) || ( color.z != 1.0f ) ) {
        ccl::MixNode* mixNode = graph->create_node<ccl::MixNode>();
        graph->add( mixNode );
        mixNode->set_mix_type( ccl::NODE_MIX_MUL );
        mixNode->set_fac( 1.0f );
        mixNode->set_color1( color );
        {
          ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
          ccl::ShaderInput* to = mixNode->input( "Color2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = mixNode->output( "Color" );
          ccl::ShaderInput* to = emissionNode->input( "Color" );
          graph->connect( from, to );
        }
      } else {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = emissionNode->input( "Color" );
        graph->connect( from, to );
      }

      AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, material.m_emissiveTexture );
    }

    if ( ccl::ShaderInput* materialSurfaceInput = graph->output()->input( "Surface" ); nullptr != materialSurfaceInput ) {
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
    imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Linear" ) );
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

    if ( material.m_clearcoatTexture && std::filesystem::exists( material.m_clearcoatTexture->filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_clearcoatTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Linear" ) );
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

    if ( material.m_clearcoatRoughnessTexture && std::filesystem::exists( material.m_clearcoatRoughnessTexture->filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_clearcoatRoughnessTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Linear" ) );
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

  // Sheen extension
  if ( material.m_sheenColorFactor || material.m_sheenColorTexture || material.m_sheenRoughnessFactor || material.m_sheenRoughnessTexture ) {
    ccl::VelvetBsdfNode* velvetNode = graph->create_node<ccl::VelvetBsdfNode>();
    graph->add( velvetNode );

    if ( material.m_sheenColorFactor ) {
      velvetNode->set_color( ccl::make_float3( material.m_sheenColorFactor->r, material.m_sheenColorFactor->g, material.m_sheenColorFactor->b ) ); 
    } else {
      velvetNode->set_color( ccl::make_float3( 0, 0, 0 ) );
    }

    velvetNode->set_sigma( material.m_sheenRoughnessFactor.value_or( 0.0f ) );

    if ( material.m_sheenColorTexture && std::filesystem::exists( material.m_sheenColorTexture->filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_sheenColorTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Non-Color" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_UNASSOCIATED );
      SetTextureWrapping( imageTextureNode, *material.m_sheenColorTexture );

      const auto& color = velvetNode->get_color();
      if ( ( color.x != 1.0f ) || ( color.y != 1.0f ) || ( color.z != 1.0f ) ) {
        ccl::MixNode* mixNode = graph->create_node<ccl::MixNode>();
        graph->add( mixNode );
        mixNode->set_mix_type( ccl::NODE_MIX_MUL );
        mixNode->set_fac( 1.0f );
        mixNode->set_color1( color );
        {
          ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
          ccl::ShaderInput* to = mixNode->input( "Color2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = mixNode->output( "Color" );
          ccl::ShaderInput* to = velvetNode->input( "Color" );
          graph->connect( from, to );
        }
      } else {
        ccl::ShaderOutput* from = imageTextureNode->output( "Color" );
        ccl::ShaderInput* to = velvetNode->input( "Color" );
        graph->connect( from, to );
      }
    }

    if ( material.m_sheenRoughnessTexture && std::filesystem::exists( material.m_sheenRoughnessTexture->filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_sheenRoughnessTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Non-Color" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_UNASSOCIATED );
      SetTextureWrapping( imageTextureNode, *material.m_sheenRoughnessTexture );

      const auto sigma = velvetNode->get_sigma();
      if ( sigma != 1.0f ) {
        ccl::MathNode* mathNode = graph->create_node<ccl::MathNode>();
        graph->add( mathNode );
        mathNode->set_math_type( ccl::NODE_MATH_MULTIPLY );
        mathNode->set_value1( sigma );
        {
          ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
          ccl::ShaderInput* to = mathNode->input( "Value2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = mathNode->output( "Value" );
          ccl::ShaderInput* to = velvetNode->input( "Sigma" );
          graph->connect( from, to );
        }
      } else {
        ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
        ccl::ShaderInput* to = velvetNode->input( "Sigma" );
        graph->connect( from, to );
      }
    }

    if ( ccl::ShaderOutput* principledBsdfOutput = principledBsdfNode->output( "BSDF" ); ( nullptr != principledBsdfOutput ) && ( 1 == principledBsdfOutput->links.size() ) ) {
      if ( ccl::ShaderInput* previousBsdfTarget = principledBsdfOutput->links.front(); nullptr != previousBsdfTarget ) {
        principledBsdfOutput->disconnect();

        ccl::AddClosureNode* addClosureNode = graph->create_node<ccl::AddClosureNode>();
        graph->add( addClosureNode );
        {
          ccl::ShaderOutput* from = velvetNode->output( "BSDF" );
          ccl::ShaderInput* to = addClosureNode->input( "Closure1" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = principledBsdfOutput;
          ccl::ShaderInput* to = addClosureNode->input( "Closure2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = addClosureNode->output( "Closure" );
          ccl::ShaderInput* to = previousBsdfTarget;
          graph->connect( from, to );
        }
      }
    }
  }

  // Specular extension
  // TODO if/when possible, support specular tint
  if ( ( material.m_specularFactor || material.m_specularTexture ) && !material.m_pbrSpecularGlossiness ) {
    principledBsdfNode->set_specular( material.m_specularFactor.value_or( 1.0f ) );

    if ( material.m_specularTexture && std::filesystem::exists( material.m_specularTexture->filepath, ec ) ) {
      ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
      graph->add( imageTextureNode );
      imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( material.m_specularTexture->filepath.string() ) );
      imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "Non-Color" ) );
      imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_UNASSOCIATED );
      SetTextureWrapping( imageTextureNode, *material.m_specularTexture );

      const auto specular = principledBsdfNode->get_specular();
      if ( specular != 1.0f ) {
        ccl::MathNode* mathNode = graph->create_node<ccl::MathNode>();
        graph->add( mathNode );
        mathNode->set_math_type( ccl::NODE_MATH_MULTIPLY );
        mathNode->set_value1( specular );
        {
          ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
          ccl::ShaderInput* to = mathNode->input( "Value2" );
          graph->connect( from, to );
        }
        {
          ccl::ShaderOutput* from = mathNode->output( "Value" );
          ccl::ShaderInput* to = principledBsdfNode->input( "Specular" );
          graph->connect( from, to );
        }
      } else {
        ccl::ShaderOutput* from = imageTextureNode->output( "Alpha" );
        ccl::ShaderInput* to = principledBsdfNode->input( "Specular" );
        graph->connect( from, to );
      }
    }
  }

  if ( !material.m_doubleSided ) {
    AddBackfaceCulling( graph );
  }

  shader->set_graph( graph );
  shader->tag_used( m_session->scene );
  shader->tag_update( m_session->scene );
  return shader;
}

void CyclesRenderer::AddBaseColorTexture( const gltfviewer::Material::Texture& texture, const Microsoft::glTF::Color3& colorFactor, const Microsoft::glTF::AlphaMode alphaMode, ccl::ShaderGraph* graph, ccl::ShaderNode* principledBsdfNode, ccl::Shader* shader )
{
  std::error_code ec;
  if ( texture.filepath.empty() || !std::filesystem::exists( texture.filepath, ec ) )
    return;

  ccl::ImageTextureNode* imageTextureNode = graph->create_node<ccl::ImageTextureNode>();
  graph->add( imageTextureNode );
  imageTextureNode->set_filename( OpenImageIO_v2_3::ustring( texture.filepath.string() ) );
  imageTextureNode->set_colorspace( OpenImageIO_v2_3::ustring( "sRGB" ) );
  imageTextureNode->set_alpha_type( ccl::IMAGE_ALPHA_UNASSOCIATED );
  SetTextureWrapping( imageTextureNode, texture );

  if ( ( colorFactor.r != 1.0f ) || ( colorFactor.g != 1.0f ) || ( colorFactor.b != 1.0f ) ) {
    ccl::MixNode* mixNode = graph->create_node<ccl::MixNode>();
    mixNode->set_mix_type( ccl::NODE_MIX_MUL );
    mixNode->set_fac( 1.0f );
    mixNode->set_color1( ccl::make_float3( colorFactor.r, colorFactor.g, colorFactor.b ) );
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
    
  if ( Microsoft::glTF::AlphaMode::ALPHA_OPAQUE == alphaMode ) {
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

  AddUVMapping( graph, imageTextureNode->input( "Vector" ), shader->attributes, texture );
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
