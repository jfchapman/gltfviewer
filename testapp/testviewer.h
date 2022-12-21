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

  // Called when the client window size changes.
  void OnSize();

private:
  // gltfviewer render callback (NOTE that the callback will be called from a separate thread).
  static void renderCallback( gltfviewer_image* image, void* context );

  // Handles a render callback.
  void HandleRenderCallback( gltfviewer_image* image );

  // Starts a render (restarts if currently rendering).
  void StartRender();

  // Client window handle.
  const HWND m_hWnd;

  // Current gltfviewer model handle.
  gltfviewer_handle m_model_handle = 0;

  // Cameras contained in the current gltfviewer model (for the default scene).
  std::vector<gltfviewer_camera> m_model_cameras;

  // Current gltfviewer camera settings.
  gltfviewer_camera m_camera = { gltfviewer_camera_projection_perspective, gltfviewer_camera_preset_default };

  // Current gltfviewer render settings.
  gltfviewer_render_settings m_render_settings = { 1920 /*width*/, 1080 /*height*/, 32 /*samples*/, 256 /*tile size*/ };

  // Current gltfviewer environment settings.
  gltfviewer_environment_settings m_environment_settings = { 0.3f /*sky intensity*/, 0.3f /*sun intensity*/, 15.0f /*sun elevation*/, 45.0f /*sun rotation*/, false /*transparent background*/ };

  // Currently displayed render result.
  std::unique_ptr<Gdiplus::Bitmap> m_display_bitmap;

  // Bitmap mutex.
  std::mutex m_mutex;

  // GDI+ token
  ULONG_PTR m_gdiplusToken = 0;
};
