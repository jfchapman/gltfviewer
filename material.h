#pragma once

#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/Document.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/ExtensionsKHR.h>

#include <filesystem>
#include <map>

#include "gltf_extension_transmission.h"
#include "gltf_extension_ior.h"
#include "gltf_extension_volume.h"
#include "gltf_extension_clearcoat.h"
#include "gltf_extension_sheen.h"

namespace gltfviewer {

using TextureMap = std::map<std::string, std::filesystem::path>;

class Material
{
public:
  Material( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::Material& material, TextureMap& textureMap );
  Material() : m_name( "default" ), m_pbrBaseColorFactor( 1.0f, 0, 1.0f, 1.0f ) {}

  struct Texture {
    std::filesystem::path      filepath;
    uint32_t                   textureCoordinateChannel = 0;
    Microsoft::glTF::WrapMode  wrapS = Microsoft::glTF::WrapMode::Wrap_REPEAT;
    Microsoft::glTF::WrapMode  wrapT = Microsoft::glTF::WrapMode::Wrap_REPEAT;
    Microsoft::glTF::Vector2   offset = { 0, 0 };
    float                      rotation = 0;
    Microsoft::glTF::Vector2   scale = { 1.0f, 1.0f };
  };

  std::string m_name;

  bool m_doubleSided = true;
  
  Microsoft::glTF::Color4 m_pbrBaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };   // Note the base color is a scaling factor (i.e. linear colorspace).
  float m_pbrMetallicFactor = 0;
  float m_pbrRoughnessFactor = 0;

  Microsoft::glTF::AlphaMode m_alphaMode = Microsoft::glTF::AlphaMode::ALPHA_OPAQUE;
  float m_alphaCutoff = 0;

  Texture m_pbrBaseTexture;
  Texture m_pbrMetallicRoughnessTexture;
  Texture m_normalTexture;
  Texture m_emissiveTexture;

  Microsoft::glTF::Color3 m_emissiveFactor = { 0.0f, 0.0f, 0.0f };  // Note the emission color is a scaling factor (i.e. linear colorspace).

  // Transmission extension
  std::optional<float> m_transmissionFactor;
  std::optional<Texture> m_transmissionTexture;

  // IOR extension
  float m_ior = 1.5f;

  // Volume extension
  std::optional<float> m_volumeDensity;
  Microsoft::glTF::Color3 m_volumeColor = { 1.0f, 1.0f, 1.0f };

  // Clearcoat extension
  std::optional<float> m_clearcoatFactor;
  std::optional<Texture> m_clearcoatTexture;
  std::optional<float> m_clearcoatRoughnessFactor;
  std::optional<Texture> m_clearcoatRoughnessTexture;

  // Sheen extension
  std::optional<Microsoft::glTF::Color3> m_sheenColorFactor;
  std::optional<Texture> m_sheenColorTexture;
  std::optional<float> m_sheenRoughnessFactor;
  std::optional<Texture> m_sheenRoughnessTexture;

private:
  void ReadTextures( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::Material& material, TextureMap& textureMap );
  std::string ReadTexture( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::TextureInfo& textureInfo, Texture& texture, TextureMap& textureMap );
};

} // namespace gltfviewer
