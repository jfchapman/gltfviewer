#pragma once

#include "gltfviewer.h"

#include <session/output_driver.h>

class CyclesOutputDriver : public ccl::OutputDriver
{
public:
  CyclesOutputDriver( const std::string& pass, gltfviewer_render_callback render_callback, void* render_callback_context );

  void write_render_tile( const Tile& tile ) final;

private:
  const std::string m_pass;

  gltfviewer_render_callback m_callback = nullptr;
  void* m_context = nullptr;
};
