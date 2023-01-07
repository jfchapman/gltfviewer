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
    m_bitmap_pixels.resize( m_params.full_size.x * m_params.full_size.y * kNumChannels );
    m_bitmap.width = m_params.full_size.x;
    m_bitmap.height = m_params.full_size.y;
    m_bitmap.bits_per_pixel = 32;
    m_bitmap.stride = m_bitmap.width * m_bitmap.bits_per_pixel / 8;
    m_bitmap.pixels = m_bitmap_pixels.data();

    OpenImageIO_v2_3::ImageSpec sourceImageSpec( m_params.size.x, m_params.size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::HALF );
    OpenImageIO_v2_3::ImageBuf sourceImageBuf( sourceImageSpec, m_texture_buffer.data() );

    // Swap the red & blue channels.
    m_scratch_buffer.resize( m_texture_buffer.size() );
    OpenImageIO_v2_3::ImageSpec scratchImageSpec( sourceImageSpec );
    OpenImageIO_v2_3::ImageBuf scratchImageBuf( scratchImageSpec, m_scratch_buffer.data() );
    OpenImageIO_v2_3::ImageBufAlgo::channels( scratchImageBuf, sourceImageBuf, kNumChannels, { 2, 1, 0, 3 } );

    // TODO proper color space conversion (for now, just gamma-correct the RGB channels by 2.2)
    const float gamma = 1.0f / 2.2f;
    OpenImageIO_v2_3::ImageBufAlgo::pow( scratchImageBuf, scratchImageBuf, { gamma, gamma, gamma, 1.0f } );

    OpenImageIO_v2_3::ImageSpec targetImageSpec( m_params.full_size.x, m_params.full_size.y, kNumChannels, OpenImageIO_v2_3::TypeDesc::UINT8 );
    OpenImageIO_v2_3::ImageBuf targetImageBuf( targetImageSpec, m_bitmap_pixels.data() );
    OpenImageIO_v2_3::ImageBufAlgo::paste( targetImageBuf, m_params.full_offset.x, m_params.full_offset.y, 0, 0, scratchImageBuf );

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
