#pragma once

#include "gltfviewer.h"

#include <Windows.h>

#include <filesystem>
#include <memory>

namespace Gdiplus {
  class Bitmap;
}

// A simple class for testing gltfviewer functionality
class TestViewer
{
public:
  TestViewer( HWND hwnd );

  ~TestViewer();

  // Loads a GLTF file and starts rendering.
  bool LoadFile( const std::filesystem::path& filepath );

  // Redraws the current render result in the client window.
  void Redraw();

  // Sets the current camera to one of the presets (and restarts the render).
  void SetCameraPreset( const gltfviewer_camera_preset preset );

  // Sets the current camera to one of the cameras in the current gltfviewer model (and restarts the render).
  void SetCameraFromModel( const size_t camera_index );

  // Returns the number of cameras contained in the current gltfviewer model (for the default scene).
  size_t GetModelCameraCount() const { return m_model_cameras.size(); }

  // Returns the names of the scenes contained in the current gltfviewer model.
  const std::vector<std::string>& GetSceneNames() const { return m_scenes; };

  // Returns the material variants contained in the current gltfviewer model.
  const std::vector<std::string>& GetMaterialVariants() const { return m_material_variants; };

  // Returns the color conversion looks provided by the gltfviewer library.
  const std::vector<std::string>& GetColorConversionLooks() const { return m_color_conversion_looks; }

  // Sets the current scene index for the current model (and restarts the render if necessary);
  void SetSceneIndex( const int32_t scene_index );

  // Sets the current material variant index for the current model (and restarts the render if necessary).
  void SetMaterialVariantIndex( const int32_t material_variant_index );

  // Sets the current gltfviewer library color conversion look index to use.
  void SetColorConversionLookIndex( const int32_t color_conversion_look_index );

  // Sets the current gltfviewer exposure value to use.
  void SetExposure( const float exposure );

  // Called when the client window size changes, with the new width & height values.
  void OnSize( const uint32_t width, const uint32_t height );

private:
  // gltfviewer render callback (NOTE that the callback will be called from a separate thread).
  static void renderCallback( gltfviewer_image* image, void* context );

	// Thread for handling the conversion of the current render image result (linear color space) to a display bitmap, using the current display options.
	static DWORD WINAPI DisplayThreadProc( LPVOID lpParam );

  // Handles a render callback.
  void RenderCallbackThreadHandler( gltfviewer_image* image );

  // Handles a change in display options by updating the display bitmap and notifying that the window needs repainting.
  void DisplayThreadHandler();

  // Starts a render (restarts if currently rendering).
  void StartRender();

  // Client window handle.
  const HWND m_hWnd;

  // Display thread stop event handle.
  const HANDLE m_display_thread_stop_event;

  // Display thread update event handle.
	const HANDLE m_display_thread_update_event;

  // Display thread handle.
  const HANDLE m_display_thread;

  // Current gltfviewer model handle.
  gltfviewer_handle m_model_handle = 0;

  // Current gltfviewer scene index to render.
  int32_t m_scene_index = gltfviewer_default_scene_index;

  // Current gltfviewer material variant index to use.
  int32_t m_material_variant_index = gltfviewer_default_scene_materials;

  // Current gltfviewer color conversion look index to use.
  std::atomic<int32_t> m_color_conversion_look_index = gltfviewer_image_convert_linear_to_srgb;

  // Current gltfviewer exposure value to use.
  std::atomic<float> m_exposure = gltfviewer_default_exposure;

  // Names of the scenes in the current gltfviewer model.
  std::vector<std::string> m_scenes;

  // Material variants contained in the current gltfviewer model.
  std::vector<std::string> m_material_variants;

  // Color conversion looks provided by the gltfviewer library.
  std::vector<std::string> m_color_conversion_looks;

  // Cameras contained in the current gltfviewer model (for the default scene).
  std::vector<gltfviewer_camera> m_model_cameras;

  // Current gltfviewer camera settings.
  gltfviewer_camera m_camera = { gltfviewer_camera_projection_perspective, gltfviewer_camera_preset_default };

  // Current gltfviewer render settings.
  gltfviewer_render_settings m_render_settings = { 1920 /*width*/, 1080 /*height*/, 32 /*samples*/, 256 /*tile size*/ };

  // Current gltfviewer environment settings.
  gltfviewer_environment_settings m_environment_settings = { 0.3f /*sky intensity*/, 0.3f /*sun intensity*/, 15.0f /*sun elevation*/, 45.0f /*sun rotation*/, false /*transparent background*/ };

  // Current render image result.
  gltfviewer_image m_render_image;

  // Current render image pixel data.
  std::vector<float> m_render_image_pixels;

  // Currently displayed render bitmap.
  std::unique_ptr<Gdiplus::Bitmap> m_display_bitmap;

  // Display bitmap mutex.
  std::mutex m_display_mutex;

  // GDI+ token
  ULONG_PTR m_gdiplusToken = 0;
};
