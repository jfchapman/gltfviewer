#include "testviewer.h"

#include <gdiplus.h>

TestViewer::TestViewer( HWND hwnd ) : m_hWnd( hwnd )
{
  // Initialise the gltfviewer library.
  const bool init = gltfviewer_init();
  _ASSERT( init );

   Gdiplus::GdiplusStartupInput gdiplusStartupInput;
   GdiplusStartup( &m_gdiplusToken, &gdiplusStartupInput, NULL );
}

TestViewer::~TestViewer()
{
  // Free the gltfviewer library (this will release all resources).
  gltfviewer_free();

  m_display_bitmap.reset();
  Gdiplus::GdiplusShutdown( m_gdiplusToken );
}

void TestViewer::renderCallback( gltfviewer_image* image, void* context )
{
  if ( TestViewer* tester = static_cast<TestViewer*>( context ); nullptr != tester ) {
    tester->HandleRenderCallback( image );
  }
}

void TestViewer::HandleRenderCallback( gltfviewer_image* image )
{
  // The render callback will be called from the gltfviewer library on a background thread, so we must make a copy of the result and post a message that the window should be repainted.
  if ( ( nullptr != image ) && ( image->width > 0 ) && ( image->height > 0 ) && ( image->bits_per_pixel == 32 ) && ( image->stride == image->width * image->bits_per_pixel / 8 ) && ( nullptr != image->pixels ) ) {
    std::lock_guard<std::mutex> lock( m_mutex );
    if ( m_display_bitmap && ( image->width == m_display_bitmap->GetWidth() ) && ( image->height == m_display_bitmap->GetHeight() ) ) {
      Gdiplus::Rect rect( 0, 0, m_display_bitmap->GetWidth(), m_display_bitmap->GetHeight() );
      Gdiplus::BitmapData data = {};
      if ( Gdiplus::Ok == m_display_bitmap->LockBits( &rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data ) ) {
        uint8_t* srcPixels = static_cast<uint8_t*>( image->pixels );
        uint8_t* dstPixels = static_cast<uint8_t*>( data.Scan0 );
        std::copy( srcPixels, srcPixels + image->stride * image->height, dstPixels );
        m_display_bitmap->UnlockBits( &data );
        m_display_bitmap->RotateFlip( Gdiplus::RotateNoneFlipY );
      }
    }
    InvalidateRect( m_hWnd, nullptr, FALSE );
  }
}

bool TestViewer::LoadFile( const std::filesystem::path& filepath )
{
  // Load the model
  m_model_handle = gltfviewer_load_model( reinterpret_cast<const char*>( filepath.u8string().c_str() ) );
  if ( gltfviewer_error == m_model_handle )
    return false;

  // Get the names of all scenes contained in the model.
  const uint32_t scene_count = gltfviewer_get_scene_count( m_model_handle );
  for ( uint32_t index = 0; index < scene_count; index++ ) {
    std::string scene_name;
    if ( const auto nameSize = gltfviewer_get_scene_name( m_model_handle, index, nullptr, 0 ); nameSize > 0 ) {
      std::vector<char> name( nameSize );
      gltfviewer_get_scene_name( m_model_handle, index, name.data(), nameSize );
      scene_name = name.data();
    }
    if ( scene_name.empty() ) {
      scene_name = "Scene " + std::to_string( m_scenes.size() );
    }
    m_scenes.push_back( scene_name );
  }

  // Get the names of all material variants contained in the model.
  const uint32_t material_variant_count = gltfviewer_get_material_variants_count( m_model_handle );
  for ( uint32_t index = 0; index < material_variant_count; index++ ) {
    std::string variant_name;
    if ( const auto nameSize = gltfviewer_get_material_variant_name( m_model_handle, index, nullptr, 0 ); nameSize > 0 ) {
      std::vector<char> name( nameSize );
      gltfviewer_get_material_variant_name( m_model_handle, index, name.data(), nameSize );
      variant_name = name.data();
    }
    if ( variant_name.empty() ) {
      variant_name = "Variant " + std::to_string( m_material_variants.size() );
    }
    m_material_variants.push_back( variant_name );
  }

  // Query the number of cameras contained in the model's current scene.
  const uint32_t camera_count = gltfviewer_get_camera_count( m_model_handle, m_scene_index );
  m_model_cameras.resize( camera_count );
  if ( camera_count > 0 ) {
    gltfviewer_get_cameras( m_model_handle, m_scene_index, m_model_cameras.data(), camera_count );
  }

  StartRender();
  return true;
}

void TestViewer::StartRender()
{
  // Note that there will be lag when stopping or starting a render, so this should really be done from a separate thread in the client.
  // However, we shall just manage this on the main thread, for simplicity.
  if ( m_model_handle )
    gltfviewer_stop_render( m_model_handle );

  // Update the render settings to use the current window size.
  RECT rect = {};
  if ( GetClientRect( m_hWnd, &rect ) && ( ( rect.right - rect.left ) > 0 ) && ( ( rect.bottom - rect.top ) > 0 ) ) {
    m_render_settings.width = rect.right - rect.left;
    m_render_settings.height = rect.bottom - rect.top;
  }

  // Set up a bitmap which we will use to display the render results on screen.
  std::lock_guard<std::mutex> lock( m_mutex );
  if ( !m_display_bitmap || ( m_render_settings.width != m_display_bitmap->GetWidth() ) || ( m_render_settings.height != m_display_bitmap->GetHeight() ) ) {
    m_display_bitmap = std::make_unique<Gdiplus::Bitmap>( m_render_settings.width, m_render_settings.height, PixelFormat32bppARGB );
    Gdiplus::Graphics graphics( m_display_bitmap.get() );
    graphics.Clear( { 255, 255, 255 } );
    InvalidateRect( m_hWnd, nullptr, FALSE );
  }

  gltfviewer_render_callback render_callback = &renderCallback;

  // Start the render. 
  // This function call will return after a startup lag, after which render results will be returned via the supplied callback.
  gltfviewer_start_render( m_model_handle, m_scene_index, m_material_variant_index, m_camera, m_render_settings, m_environment_settings, render_callback, this );
}

void TestViewer::Redraw()
{
  std::lock_guard<std::mutex> lock( m_mutex );
  if ( m_display_bitmap ) {
    Gdiplus::Graphics graphics( m_hWnd );
    graphics.DrawImage( m_display_bitmap.get(), 0, 0 );
  }
}

void TestViewer::SetCameraPreset( const gltfviewer_camera_preset preset )
{
  if ( preset != m_camera.preset ) {
    m_camera.preset = preset;
    StartRender();
  }
}

void TestViewer::SetCameraFromModel( const size_t camera_index )
{
  if ( camera_index < m_model_cameras.size() ) {
    m_camera = m_model_cameras[ camera_index ];
    StartRender();
  }
}

void TestViewer::SetSceneIndex( const int32_t scene_index )
{
  if ( ( scene_index != m_scene_index ) && ( scene_index < static_cast<int32_t>( m_scenes.size() ) ) ) {
    m_scene_index = scene_index;
    StartRender();
  }
}

void TestViewer::SetMaterialVariantIndex( const int32_t material_variant_index )
{
  if ( ( material_variant_index != m_material_variant_index ) && ( material_variant_index < static_cast<int32_t>( m_material_variants.size() ) ) ) {
    m_material_variant_index = material_variant_index;
    StartRender();
  }
}

void TestViewer::OnSize( const uint32_t width, const uint32_t height )
{
  if ( ( m_render_settings.width != width ) || ( m_render_settings.height != height ) ) {
    StartRender();
  }
}