#include "gltfviewer.h"
#include "models.h"
#include "cycles_renderer.h"

#include <string>

static std::unique_ptr<gltfviewer::Models> g_models;

int32_t gltfviewer_init()
{
  g_models = std::make_unique<gltfviewer::Models>();
  return gltfviewer_success;
}

void gltfviewer_free()
{
  g_models.reset();
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

bool gltfviewer_start_render( gltfviewer_handle model_handle, int32_t scene_index, gltfviewer_camera camera, gltfviewer_render_settings render_settings, gltfviewer_environment_settings environment_settings, gltfviewer_render_callback render_callback, void* render_callback_context )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr == model )
    return false;

  return model->StartRender( scene_index, camera, render_settings, environment_settings, render_callback, render_callback_context );
}

void gltfviewer_stop_render( gltfviewer_handle model_handle )
{
  gltfviewer::Model* const model = g_models ? g_models->FindModel( model_handle ) : nullptr;
  if ( nullptr != model )
    model->StopRender();
}
