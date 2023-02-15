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

  bool StartRender( const int32_t scene_index, const int32_t material_variant_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context ) final;
  void StopRender() final;

private:
  bool StartSession( const int32_t scene_index, const int32_t material_variant_index, const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings, const gltfviewer_environment_settings& environment_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, gltfviewer_finish_callback finish_callback, void* context );
  bool InitialiseSession( const gltfviewer_render_settings& render_settings, gltfviewer_render_callback render_callback, gltfviewer_progress_callback progress_callback, void* context );
  bool BuildScene( const int32_t scene_index, const int32_t material_variant_index );
  bool SetMeshes( const int32_t scene_index, const int32_t material_variant_index );
  bool SetLights( const int32_t scene_index );
  bool SetCamera( const gltfviewer_camera& camera, const gltfviewer_render_settings& render_settings );
  bool SetBackground( const gltfviewer_environment_settings& environment_settings );
  void Cleanup();

  ccl::DeviceInfo SelectDevice();
  ccl::Shader* GetShader( const std::string& materialID, const gltfviewer::VariantIndexToMaterialIDMap& material_variants, const int32_t material_variant_index );
  ccl::Shader* CreateShader( const gltfviewer::Material& material );
  void AddBaseColorTexture( const gltfviewer::Material::Texture& texture, const Microsoft::glTF::Color3& colorFactor, const Microsoft::glTF::AlphaMode alphaMode, ccl::ShaderGraph* graph, ccl::ShaderNode* principledBsdfNode, ccl::Shader* shader );
  void AddBackfaceCulling( ccl::ShaderGraph* graph );
  void AddUVMapping( ccl::ShaderGraph* graph, ccl::ShaderInput* shader_input, ccl::AttributeRequestSet& attributes, const gltfviewer::Material::Texture& texture );

  std::unique_ptr<ccl::Session> m_session;
  std::map<std::string, ccl::Shader*> m_shader_map;
  std::filesystem::path m_temp_folder;
  std::thread m_render_thread;
  std::atomic_bool m_cancel = false;
  bool m_forceCPU = false;
  gltfviewer_image m_result_noisy = {};
  gltfviewer_image m_result_denoised = {};
};
