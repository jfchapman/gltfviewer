#pragma once

#include "model.h"

namespace gltfviewer {

class Renderer
{
public:
  virtual ~Renderer() {}

  virtual bool StartRender( const int32_t scene_index, const int32_t material_variant_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context ) = 0;
  virtual void StopRender() = 0;

protected:
  Renderer( const Model& model ) : m_model( model ) {}

  const Model& m_model;
};

} // namespace gltfviewer
