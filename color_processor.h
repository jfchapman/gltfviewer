#pragma once

#include "gltfviewer.h"

#include <filesystem>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OCIO_NAMESPACE;

namespace gltfviewer {

class ColorProcessor
{
public:
  ColorProcessor( const std::filesystem::path& configPath );

  const std::vector<std::string>& GetLooks() const { return m_looks; }

  bool Convert( const float exposure, const int32_t look_index, const gltfviewer_image& sourceImage, gltfviewer_image& destImage );

private:
  bool ConvertColorSpace( const int32_t look_index, gltfviewer_image& image );

  OCIO::ConstConfigRcPtr m_config;

  std::vector<std::string> m_looks;
};

} // namespace gltfviewer
