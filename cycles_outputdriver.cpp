#include "cycles_outputdriver.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

CyclesOutputDriver::CyclesOutputDriver( const std::string& pass, gltfviewer_render_callback render_callback, void* render_callback_context ) : 
  ccl::OutputDriver(),
  m_pass( pass ),
  m_callback( render_callback ),
  m_context( render_callback_context )
{

}

void CyclesOutputDriver::write_render_tile( const Tile& tile )
{
  constexpr int kNumChannels = 4;
  if ( m_callback && ( tile.full_size == tile.size ) ) {
    std::vector<float> renderPixels( tile.full_size.x * tile.full_size.y * kNumChannels );
    if ( tile.get_pass_pixels( m_pass, kNumChannels, renderPixels.data() ) ) {
      std::vector<uint8_t> bitmapPixels( renderPixels.size() );
      gltfviewer_image bitmap = {};
      bitmap.pixel_format = gltfviewer_image_pixelformat_floatRGBA;
      bitmap.width = tile.full_size.x;
      bitmap.height = tile.full_size.y;
      bitmap.stride_bytes = bitmap.width * 4 * 4;
      bitmap.pixels = renderPixels.data();

      m_callback( &bitmap, m_context );
    }
  }
}
