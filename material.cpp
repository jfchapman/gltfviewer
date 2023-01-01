#include "material.h"

#include "utils.h"

#include <fstream>

namespace gltfviewer {

Material::Material( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::Material& material, TextureMap& textureMap ) : 
  m_name( material.name ),
  m_doubleSided( material.doubleSided ),
  m_pbrBaseColorFactor( material.metallicRoughness.baseColorFactor ),
  m_pbrMetallicFactor( material.metallicRoughness.metallicFactor ),
  m_pbrRoughnessFactor( material.metallicRoughness.roughnessFactor ),
  m_emissiveFactor( material.emissiveFactor ),
  m_alphaMode( material.alphaMode ),
  m_alphaCutoff( material.alphaCutoff )
{
  ReadTextures( document, resourceReader, material, textureMap );

  if ( material.HasExtension<KHR_materials_transmission>() ) {
    const auto& extension = material.GetExtension<KHR_materials_transmission>();
    m_transmissionFactor = extension.m_transmissionFactor;
    if ( extension.m_transmissionTexture ) {
      m_transmissionTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_transmissionTexture, *m_transmissionTexture, textureMap );
    }
  }

  if ( material.HasExtension<KHR_materials_ior>() ) {
     const auto& extension = material.GetExtension<KHR_materials_ior>();
     m_ior = extension.m_ior;
  }

  if ( material.HasExtension<KHR_materials_emissive_strength>() ) {
     const auto& extension = material.GetExtension<KHR_materials_emissive_strength>();
     m_emissiveStrength = extension.m_emissive_strength;
  }

  if ( material.HasExtension<KHR_materials_volume>() ) {
    const auto& extension = material.GetExtension<KHR_materials_volume>();
    m_volumeDensity = ( extension.m_attenutationDistance > 0 ) ? ( 1 / extension.m_attenutationDistance ) : 0;
    m_volumeColor = extension.m_attenuationColor;
  }

  if ( material.HasExtension<KHR_materials_clearcoat>() ) {
    const auto& extension = material.GetExtension<KHR_materials_clearcoat>();
    m_clearcoatFactor = extension.m_clearcoatFactor;
    if ( extension.m_clearcoatTexture ) {
      m_clearcoatTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_clearcoatTexture, *m_clearcoatTexture, textureMap );
    }
    m_clearcoatRoughnessFactor = extension.m_clearcoatRoughnessFactor;
    if ( extension.m_clearcoatRoughnessTexture ) {
      m_clearcoatRoughnessTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_clearcoatRoughnessTexture, *m_clearcoatRoughnessTexture, textureMap );
    }
  }

  if ( material.HasExtension<KHR_materials_sheen>() ) {
    const auto& extension = material.GetExtension<KHR_materials_sheen>();
    m_sheenColorFactor = extension.m_sheenColorFactor;
    if ( extension.m_sheenColorTexture ) {
      m_sheenColorTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_sheenColorTexture, *m_sheenColorTexture, textureMap );
    }
    m_sheenRoughnessFactor = extension.m_sheenRoughnessFactor;
    if ( extension.m_sheenRoughnessTexture ) {
      m_sheenRoughnessTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_sheenRoughnessTexture, *m_sheenRoughnessTexture, textureMap );
    }
  }

  if ( material.HasExtension<KHR_materials_specular>() ) {
    const auto& extension = material.GetExtension<KHR_materials_specular>();
    m_specularFactor = extension.m_specularFactor;
    if ( extension.m_specularTexture ) {
      m_specularTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_specularTexture, *m_specularTexture, textureMap );
    }
    m_specularColorFactor = extension.m_specularColorFactor;
    if ( extension.m_specularColorTexture ) {
      m_specularColorTexture = std::make_optional<Texture>();
      ReadTexture( document, resourceReader, *extension.m_specularColorTexture, *m_specularColorTexture, textureMap );
    }
  }
}

void Material::ReadTextures( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::Material& material, TextureMap& textureMap )
{
  if ( const auto& textureInfo = material.metallicRoughness.baseColorTexture; !textureInfo.textureId.empty() ) {
    ReadTexture( document, resourceReader, textureInfo, m_pbrBaseTexture, textureMap );
  }
  if ( const auto& textureInfo = material.metallicRoughness.metallicRoughnessTexture; !textureInfo.textureId.empty() ) {
    ReadTexture( document, resourceReader, textureInfo, m_pbrMetallicRoughnessTexture, textureMap );
  }
  if ( const auto& textureInfo = material.emissiveTexture; !textureInfo.textureId.empty() ) {
    ReadTexture( document, resourceReader, textureInfo, m_emissiveTexture, textureMap );
  }
  if ( const auto& textureInfo = material.normalTexture; !textureInfo.textureId.empty() ) {
    ReadTexture( document, resourceReader, textureInfo, m_normalTexture, textureMap );
  }
}

std::string Material::ReadTexture( const Microsoft::glTF::Document& document, const Microsoft::glTF::GLTFResourceReader& resourceReader, const Microsoft::glTF::TextureInfo& textureInfo, Texture& texture, TextureMap& textureMap )
{
  std::string imageID;
  try {
    if ( document.textures.Has( textureInfo.textureId ) ) {
      imageID = document.textures.Get( textureInfo.textureId ).imageId;
      if ( ( textureMap.end() == textureMap.find( imageID ) ) && document.images.Has( imageID ) ) {
        const auto& image = document.images.Get( imageID );
        if ( const auto imageData = resourceReader.ReadBinaryData( document, image ); !imageData.empty() ) {
          auto filename = GenerateGUID();
          std::string extension;
          if ( !image.uri.empty() ) {
            std::filesystem::path p( image.uri );
            if ( p.has_extension() ) {
              extension = p.extension().string();
            }
          }
          if ( extension.empty() ) {
            if ( !image.mimeType.empty() ) {
              if ( "image/jpeg" == image.mimeType ) {
                extension = ".jpg";
              } else if ( "image/png" == image.mimeType ) {
                extension = ".png";
              }
            } else if ( ( imageData.size() >= 3 ) && ( imageData[ 0 ] == 0xFF ) && ( imageData[ 1 ] == 0xD8 ) && ( imageData[ 2 ] == 0xFF ) ) {
              extension = ".jpg";
            } else if ( ( imageData.size() >= 8 ) && ( imageData[ 0 ] == 0x89 ) && ( imageData[ 1 ] == 0x50 ) && ( imageData[ 2 ] == 0x4E ) && ( imageData[ 3 ] == 0x47 ) &&
              ( imageData[ 4 ] == 0x0D ) && ( imageData[ 5 ] == 0x0A ) && ( imageData[ 6 ] == 0x1A ) && ( imageData[ 7 ] == 0x0A ) ) {
              extension = ".png";
            }
          }
          if ( !extension.empty() ) {
            filename += extension;
          }

          std::error_code ec;
          const auto filepath = std::filesystem::temp_directory_path( ec ) / filename;

          if ( std::ofstream stream( filepath, std::ios::binary ); stream.good() ) {
            stream.write( reinterpret_cast<const char*>( imageData.data() ), imageData.size() );
            textureMap.insert( { imageID, filepath } );
          }
        }
      }

      if ( const auto it = textureMap.find( imageID ); textureMap.end() != it ) {
        texture.filepath = it->second;
        texture.textureCoordinateChannel = textureInfo.texCoord;
 
        const auto& samplerID = document.textures.Get( textureInfo.textureId ).samplerId;
        if ( !samplerID.empty() && document.samplers.Has( samplerID ) ) {
          const auto& sampler = document.samplers.Get( samplerID );
          texture.wrapS = sampler.wrapS;
          texture.wrapT = sampler.wrapT;
        }

        if ( textureInfo.HasExtension<Microsoft::glTF::KHR::TextureInfos::TextureTransform>() ) {
          const auto& textureTransform = textureInfo.GetExtension<Microsoft::glTF::KHR::TextureInfos::TextureTransform>();
          texture.offset = textureTransform.offset;
          texture.rotation = textureTransform.rotation;
          texture.scale = textureTransform.scale;
          if ( textureTransform.texCoord.HasValue() ) {
            texture.textureCoordinateChannel = textureTransform.texCoord.Get();
          }
        }
      }
    }
  } catch ( const std::runtime_error& ) {
  }

  return imageID;
}

} // namespace gltfviewer
