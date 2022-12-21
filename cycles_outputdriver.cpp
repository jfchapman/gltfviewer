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
      bitmap.width = tile.full_size.x;
      bitmap.height = tile.full_size.y;
      bitmap.bits_per_pixel = 32;
      bitmap.stride = bitmap.width * bitmap.bits_per_pixel / 8;
      bitmap.pixels = bitmapPixels.data();

      OpenImageIO_v2_3::ImageSpec sourceImageSpec( tile.full_size.x, tile.full_size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::FLOAT );
      OpenImageIO_v2_3::ImageBuf sourceImageBuf( sourceImageSpec, renderPixels.data() );

      // TODO proper color space conversion (for now, just gamma-correct by 2.2 channels 0-2 of the image, in-place)
      const float gamma = 1.0f / 2.2f;
      OpenImageIO_v2_3::ImageBufAlgo::pow( sourceImageBuf, sourceImageBuf, { gamma, gamma, gamma, 1.0f } );

      OpenImageIO_v2_3::ImageSpec targetImageSpec( tile.full_size.x, tile.full_size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::UINT8 );
      OpenImageIO_v2_3::ImageBuf targetImageBuf( targetImageSpec );
      targetImageBuf.copy( sourceImageBuf, OpenImageIO_v2_3::TypeDesc::UINT8 );
      targetImageBuf = OpenImageIO_v2_3::ImageBufAlgo::channels( targetImageBuf, kNumChannels, { 2, 1, 0, 3 } );
      targetImageBuf.get_pixels( OpenImageIO_v2_3::ROI(), OpenImageIO_v2_3::TypeDesc::UINT8, bitmapPixels.data() );

      OutputDebugString( L"[CyclesOutputDriver] - callback start\r\n" );
      m_callback( &bitmap, m_context );
      OutputDebugString( L"[CyclesOutputDriver] - callback finish\r\n" );
    }
  }
}
