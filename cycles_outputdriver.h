#pragma once

#include "gltfviewer.h"

#include <session/output_driver.h>

class CyclesOutputDriver : public ccl::OutputDriver
{
public:
  CyclesOutputDriver( const std::string& pass_noisy, const std::string& pass_denoised, gltfviewer_image& result_noisy, gltfviewer_image& result_denoised );

  void write_render_tile( const Tile& tile ) final;

private:
  void UpdateImage( const Tile& tile, const std::string& pass, gltfviewer_image& image, std::vector<float>& image_pixels );

  const std::string m_pass_noisy;
  const std::string m_pass_denoised;

  gltfviewer_image& m_image_noisy;
  std::vector<float> m_pixels_noisy;

  gltfviewer_image& m_image_denoised;
  std::vector<float> m_pixels_denoised;
};
