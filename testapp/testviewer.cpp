#include "testviewer.h"

#include <gdiplus.h>

TestViewer::TestViewer( HWND hwnd ) :
  m_hWnd( hwnd ),
	m_display_thread_stop_event( CreateEvent( NULL /*attributes*/, TRUE /*manualReset*/, FALSE /*initialState*/, L"" /*name*/ ) ),
	m_display_thread_update_event( CreateEvent( NULL /*attributes*/, TRUE /*manualReset*/, FALSE /*initialState*/, L"" /*name*/ ) ),
	m_display_thread( CreateThread( NULL /*attributes*/, 0 /*stackSize*/, DisplayThreadProc, this /*param*/, 0 /*flags*/, NULL /*threadId*/ ) )
{
  // Initialise the gltfviewer library.
  const bool init = gltfviewer_init();
  _ASSERT( init );

  // Get the color conversion looks supplied by the gltfviewer library.
  const uint32_t look_count = gltfviewer_get_look_count();
  for ( uint32_t index = 0; index < look_count; index++ ) {
    std::string look_name;
    if ( const auto nameSize = gltfviewer_get_look_name( index, nullptr, 0 ); nameSize > 0 ) {
      std::vector<char> name( nameSize );
      gltfviewer_get_look_name( index, name.data(), nameSize );
      look_name = name.data();
    }
    m_color_conversion_looks.push_back( look_name );
  }

  // Initialize GDI+
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup( &m_gdiplusToken, &gdiplusStartupInput, NULL );
}

TestViewer::~TestViewer()
{
  // Signal and wait for the display thread to quit.
  SetEvent( m_display_thread_stop_event );
  WaitForSingleObject( m_display_thread, INFINITE );

  // Free the gltfviewer library (this will release all resources).
  gltfviewer_free();

  // Shutdown GDI+
  m_display_bitmap.reset();
  Gdiplus::GdiplusShutdown( m_gdiplusToken );
}

DWORD WINAPI TestViewer::DisplayThreadProc( LPVOID lpParam )
{
	if ( TestViewer* viewer = reinterpret_cast<TestViewer*>( lpParam ); nullptr != viewer ) {
		viewer->DisplayThreadHandler();
	}
	return 0;
}

void TestViewer::DisplayThreadHandler()
{
  HANDLE handles[ 2 ] = { m_display_thread_stop_event, m_display_thread_update_event };

  // Wait for either of the display thread events being to be signalled. Quit if the stop event has been signalled, otherwise convert the current render image to a display bitmap.
	while ( WaitForMultipleObjects( 2, handles, FALSE /*waitAll*/, INFINITE ) != WAIT_OBJECT_0 ) {
    std::lock_guard<std::mutex> lock( m_display_mutex );

    if ( ( m_render_image.width > 0 ) && ( m_render_image.height > 0 ) && ( nullptr != m_render_image.pixels ) ) {

      if ( !m_display_bitmap || ( m_display_bitmap->GetWidth() != m_render_image.width ) || ( m_display_bitmap->GetHeight() != m_render_image.height ) ) {
        // Create a display bitmap which matches the dimensions of the render image.
        m_display_bitmap = std::make_unique<Gdiplus::Bitmap>( m_render_settings.width, m_render_settings.height, PixelFormat32bppARGB );
        Gdiplus::Graphics graphics( m_display_bitmap.get() );
        graphics.Clear( { 255, 255, 255 } );
      }

      if ( m_display_bitmap && ( m_display_bitmap->GetWidth() == m_render_image.width ) || ( m_display_bitmap->GetHeight() == m_render_image.height ) ) {
        // Convert the render image (in linear color space) to the display color space, using the current gltfviewer look.
        Gdiplus::Rect rect( 0, 0, m_display_bitmap->GetWidth(), m_display_bitmap->GetHeight() );
        Gdiplus::BitmapData data = {};
        if ( Gdiplus::Ok == m_display_bitmap->LockBits( &rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data ) ) {
          gltfviewer_image target_image;
          target_image.pixel_format = gltfviewer_image_pixelformat_ucharBGRA;
          target_image.width = data.Width;
          target_image.height = data.Height;
          target_image.stride_bytes = data.Stride;
          target_image.pixels = data.Scan0;

          gltfviewer_image_convert( &m_render_image, &target_image, m_color_conversion_look_index, m_exposure );

          m_display_bitmap->UnlockBits( &data );

          // Convert the top-down render image to a bottom-up bitmap.
          m_display_bitmap->RotateFlip( Gdiplus::RotateNoneFlipY );
        }
      }
    }

    // Signal that the window needs to be repainted.
    InvalidateRect( m_hWnd, nullptr, FALSE );

    // Conversion has been done, so reset the update event.
    ResetEvent( m_display_thread_update_event );
  }
}

void TestViewer::renderCallback( gltfviewer_image* image, void* context )
{
  if ( TestViewer* tester = static_cast<TestViewer*>( context ); nullptr != tester ) {
    tester->RenderCallbackThreadHandler( image );
  }
}

void TestViewer::RenderCallbackThreadHandler( gltfviewer_image* image )
{
  // The render callback will be called from the gltfviewer library on a background thread, so we must make a copy of the result and indicate that the display bitmap should be updated.
  if ( ( nullptr != image ) && ( image->width > 0 ) && ( image->height > 0 ) && ( image->stride_bytes > 0 ) && ( gltfviewer_image_pixelformat_floatRGBA == image->pixel_format ) && ( nullptr != image->pixels ) ) {
    std::lock_guard<std::mutex> lock( m_display_mutex );

    // Copy the render image result.
    m_render_image = *image;
    const float* source_pixel_data = static_cast<const float*>( image->pixels );
    const size_t pixel_data_size = image->height * image->stride_bytes / 4;
    m_render_image_pixels.resize( pixel_data_size );
    std::copy( source_pixel_data, source_pixel_data + pixel_data_size, m_render_image_pixels.data() );
    m_render_image.pixels = m_render_image_pixels.data();

    // Signal that the display bitmap needs to be updated.
    SetEvent( m_display_thread_update_event );
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

  // Get the number of cameras contained in the model's current scene.
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

  gltfviewer_render_callback render_callback = &renderCallback;

  // Start the render. 
  // This function call will return after a startup lag, after which render results will be returned via the supplied callback.
  gltfviewer_start_render( m_model_handle, m_scene_index, m_material_variant_index, m_camera, m_render_settings, m_environment_settings, render_callback, this );
}

void TestViewer::Redraw()
{
  std::lock_guard<std::mutex> lock( m_display_mutex );
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

void TestViewer::SetColorConversionLookIndex( const int32_t color_conversion_look_index )
{
  if ( ( color_conversion_look_index != m_color_conversion_look_index ) && ( color_conversion_look_index < static_cast<int32_t>( m_color_conversion_looks.size() ) ) ) {
    m_color_conversion_look_index = color_conversion_look_index;

    // Signal that the display bitmap should be updated.
    SetEvent( m_display_thread_update_event );
  }
}

void TestViewer::SetExposure( const float exposure )
{
  if ( exposure != m_exposure ) {
    m_exposure = exposure;

    // Signal that the display bitmap should be updated.
    SetEvent( m_display_thread_update_event );
  }
}

void TestViewer::OnSize( const uint32_t width, const uint32_t height )
{
  if ( ( m_render_settings.width != width ) || ( m_render_settings.height != height ) ) {
    StartRender();
  }
}