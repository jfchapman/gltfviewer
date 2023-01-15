#pragma once

#include "gltfviewer.h"

#include <session/display_driver.h>
#include <vector>

class CyclesDisplayDriver : public ccl::DisplayDriver
{
public:
  CyclesDisplayDriver( gltfviewer_render_callback render_callback, void* render_callback_context );

  void next_tile_begin() final;
  bool update_begin( const Params &params, int width, int height ) final;
  void update_end() final;
  ccl::half4* map_texture_buffer() final;
  void unmap_texture_buffer() final;
  void clear() final;
  void draw( const Params &params ) final;

private:
  gltfviewer_render_callback m_callback = nullptr;
  void* m_context = nullptr;
  gltfviewer_image m_bitmap = {};
  std::vector<float> m_bitmap_pixels;
  std::vector<ccl::half4> m_texture_buffer;
  Params m_params = {};
};
