#include "cycles_displaydriver.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

CyclesDisplayDriver::CyclesDisplayDriver( gltfviewer_render_callback render_callback, void* render_callback_context ) :
  ccl::DisplayDriver(),
  m_callback( render_callback ),
  m_context( render_callback_context )
{

}

void CyclesDisplayDriver::next_tile_begin()
{
 
}

bool CyclesDisplayDriver::update_begin( const Params& params, int width, int height )
{
  m_texture_buffer.resize( width * height );
  m_params = params;
  return true;
}

void CyclesDisplayDriver::update_end()
{
  constexpr size_t kNumChannels = 4;
  if ( m_callback && ( m_params.full_size.x > 0 ) && ( m_params.full_size.y > 0 ) && ( ( m_params.size.x + m_params.full_offset.x ) <= m_params.full_size.x ) && ( ( m_params.size.y + m_params.full_offset.y ) <= m_params.full_size.y ) ) {
    if ( const size_t pixel_size = m_params.full_size.x * m_params.full_size.y * kNumChannels; pixel_size != m_bitmap_pixels.size() ) {
      m_bitmap_pixels.resize( pixel_size );
    }
    m_bitmap.pixel_format = gltfviewer_image_pixelformat_floatRGBA;
    m_bitmap.width = m_params.full_size.x;
    m_bitmap.height = m_params.full_size.y;
    m_bitmap.stride_bytes = m_bitmap.width * 4 * 4;
    m_bitmap.pixels = m_bitmap_pixels.data();

    OpenImageIO_v2_3::ImageSpec sourceImageSpec( m_params.size.x, m_params.size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::HALF );
    OpenImageIO_v2_3::ImageBuf sourceImageBuf( sourceImageSpec, m_texture_buffer.data() );

    OpenImageIO_v2_3::ImageSpec targetImageSpec( m_params.full_size.x, m_params.full_size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::FLOAT );
    OpenImageIO_v2_3::ImageBuf targetImageBuf( targetImageSpec, m_bitmap_pixels.data() );
    OpenImageIO_v2_3::ImageBufAlgo::paste( targetImageBuf, m_params.full_offset.x, m_params.full_offset.y, 0, 0, sourceImageBuf );

    m_callback( &m_bitmap, m_context );
  }
}

ccl::half4* CyclesDisplayDriver::map_texture_buffer()
{
  return m_texture_buffer.data();
}

void CyclesDisplayDriver::unmap_texture_buffer()
{
}

void CyclesDisplayDriver::clear()
{
  std::fill( m_texture_buffer.begin(), m_texture_buffer.end(), ccl::half4() );
}

void CyclesDisplayDriver::draw( const Params& )
{
}
