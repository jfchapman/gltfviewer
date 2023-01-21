#include "gltfviewer.h"
#include "models.h"
#include "color_processor.h"
#include "cycles_renderer.h"

#include <string>

static std::unique_ptr<gltfviewer::Models> g_models;
static std::unique_ptr<gltfviewer::ColorProcessor> g_color_processor;

int32_t gltfviewer_init()
{
  g_models = std::make_unique<gltfviewer::Models>();
  g_color_processor = std::make_unique<gltfviewer::ColorProcessor>( gltfviewer::GetLibraryPath() / "config.ocio" );
  return gltfviewer_success;
}

void gltfviewer_free()
{
  g_models.reset();
  g_color_processor.reset();
}

gltfviewer_handle gltfviewer_load_model( const char* filename )
{
  if ( !g_models )
    return gltfviewer_error;

  auto model = std::make_unique<gltfviewer::Model>();
  const std::u8string u8s( reinterpret_cast<const char8_t*>( filename ) );
  if ( model->Load( u8s ) ) {
    return g_models->AddModel( std::move( model ) );
  }

  return gltfviewer_error;
}

void gltfviewer_free_model( gltfviewer_handle model_handle )
{
  if ( g_models )
    g_models->FreeModel( model_handle );
}

uint32_t gltfviewer_get_scene_count( gltfviewer_handle model_handle )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return 0;

  return model->GetSceneCount();
}

uint32_t gltfviewer_get_scene_name( gltfviewer_handle model_handle, uint32_t scene_index, char* name_buffer, uint32_t name_buffer_size )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return 0;

  const auto& scene_names = model->GetSceneNames();
  if ( scene_index >= scene_names.size() )
    return 0;

  const auto& name = scene_names[ scene_index ];
  if ( ( nullptr != name_buffer ) && ( name_buffer_size > name.size() ) ) {
    strncpy_s( name_buffer, name_buffer_size, name.c_str(), name_buffer_size );
  }

  return 1 + name.size();
}

uint32_t gltfviewer_get_camera_count( gltfviewer_handle model_handle, int32_t scene_index )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return 0;

  return static_cast<uint32_t>( model->GetCameras( scene_index ).size() );
}

bool gltfviewer_get_cameras( gltfviewer_handle model_handle, int32_t scene_index, gltfviewer_camera* cameras, uint32_t camera_count )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return false;

  const auto& model_cameras = model->GetCameras( scene_index );
  if ( model_cameras.size() > camera_count )
    return false;

  gltfviewer_camera* pCamera = cameras;
  for ( const auto& camera : model_cameras ) {
    *pCamera++ = camera;
  }

  return true;
}

uint32_t gltfviewer_get_material_variants_count( gltfviewer_handle model_handle )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return 0;

  return model->GetVariantCount();
}

uint32_t gltfviewer_get_material_variant_name( gltfviewer_handle model_handle, uint32_t material_variant_index, char* name_buffer, uint32_t name_buffer_size )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return 0;

  const auto& variants = model->GetVariants();
  if ( material_variant_index >= variants.size() )
    return 0;

  const auto& name = variants[ material_variant_index ];
  if ( ( nullptr != name_buffer ) && ( name_buffer_size > name.size() ) ) {
    strncpy_s( name_buffer, name_buffer_size, name.c_str(), name_buffer_size );
  }

  return 1 + name.size();
}

bool gltfviewer_start_render( gltfviewer_handle model_handle, int32_t scene_index, int32_t material_variant_index, gltfviewer_camera camera, gltfviewer_render_settings render_settings, gltfviewer_environment_settings environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return false;

  return model->StartRender( scene_index, material_variant_index, camera, render_settings, environment_settings, render_callback, progress_callback, finish_callback, context );
}

void gltfviewer_stop_render( gltfviewer_handle model_handle )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr != model )
    model->StopRender();
}

uint32_t gltfviewer_get_look_count()
{
  return g_color_processor ? static_cast<uint32_t>( g_color_processor->GetLooks().size() ) : 0;
}

uint32_t gltfviewer_get_look_name( uint32_t look_index, char* name_buffer, uint32_t name_buffer_size )
{
  if ( !g_color_processor )
    return 0;

  const auto& looks = g_color_processor->GetLooks();
  if ( look_index >= looks.size() )
    return 0;

  const auto& name = looks[ look_index ];
  if ( ( nullptr != name_buffer ) && ( name_buffer_size > name.size() ) ) {
    strncpy_s( name_buffer, name_buffer_size, name.c_str(), name_buffer_size );
  }

  return 1 + name.size();
}

bool gltfviewer_image_convert( gltfviewer_image* source_image, gltfviewer_image* target_image, int32_t look_index, float exposure )
{
  if ( !g_color_processor || ( nullptr == source_image ) || ( nullptr == target_image ) )
    return false;

  return g_color_processor->Convert( exposure, look_index, *source_image, *target_image );
}
