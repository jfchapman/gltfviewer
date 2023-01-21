#include "cycles_outputdriver.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

CyclesOutputDriver::CyclesOutputDriver( const std::string& pass_noisy, const std::string& pass_denoised, gltfviewer_image& result_noisy, gltfviewer_image& result_denoised ) : 
  ccl::OutputDriver(),
  m_pass_noisy( pass_noisy ),
  m_pass_denoised( pass_denoised ),
  m_image_noisy( result_noisy ),
  m_image_denoised( result_denoised )
{
}

void CyclesOutputDriver::write_render_tile( const Tile& tile )
{
  if ( tile.full_size == tile.size ) {
    UpdateImage( tile, m_pass_noisy, m_image_noisy, m_pixels_noisy );
    UpdateImage( tile, m_pass_denoised, m_image_denoised, m_pixels_denoised );
  }
}

void CyclesOutputDriver::UpdateImage( const Tile& tile, const std::string& pass, gltfviewer_image& image, std::vector<float>& image_pixels )
{
  constexpr int kNumChannels = 4;
  image_pixels.resize( tile.full_size.x * tile.full_size.y * kNumChannels );
  if ( tile.get_pass_pixels( pass, kNumChannels, image_pixels.data() ) ) {
    image.pixel_format = gltfviewer_image_pixelformat_floatRGBA;
    image.width = tile.full_size.x;
    image.height = tile.full_size.y;
    image.stride_bytes = image.width * 4 * 4;
    image.pixels = image_pixels.data();
  }
}
