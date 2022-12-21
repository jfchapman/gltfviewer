#pragma once

#include "renderer.h"

#include <session/session.h>
#include <scene/shader.h>
#include <scene/shader_graph.h>

class CyclesRenderer : public gltfviewer::Renderer
{
public:
  CyclesRenderer( const gltfviewer::Model& model );
  ~CyclesRenderer() final;

  bool StartRender( const int32_t scene_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, void* render_callback_context ) final;
  void StopRender() final;

private:
  bool InitialiseSession( const gltfviewer_render_settings& render_settings, gltfviewer_render_callback render_callback, void* render_callback_context, const bool forceCPU = false );
  bool BuildScene( const int32_t scene_index );
  bool SetCamera( const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings );
  bool SetBackground( const gltfviewer_environment_settings& environment_settings );

  ccl::DeviceInfo SelectDevice( const bool forceCPU = false );
  ccl::Shader* GetShader( const std::string& materialID );
  ccl::Shader* CreateShader( const gltfviewer::Material& material );
  void AddBackfaceCulling( ccl::ShaderGraph* graph );
  void AddUVMapping( ccl::ShaderGraph* graph, ccl::ShaderInput* shader_input, ccl::AttributeRequestSet& attributes, const gltfviewer::Material::Texture& texture );

  std::unique_ptr<ccl::Session> m_session;
  std::map<std::string, ccl::Shader*> m_shader_map;
  std::filesystem::path m_temp_folder;
};
